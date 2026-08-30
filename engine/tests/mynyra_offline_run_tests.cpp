#include "MynyraOfflineRun.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

class TestObserver final : public IOfflineRunObserverV1 {
public:
    bool cancelled{false};
    std::uint64_t elapsed{0};

    bool cancellationRequested() const noexcept override { return cancelled; }
    std::uint64_t elapsedMilliseconds() const noexcept override { return elapsed; }
};

class RecordingSink final : public IEventSink {
public:
    explicit RecordingSink(bool accept) : m_accept(accept) {}

    bool emit(const MynyraEvent& event, EventFlush) noexcept override
    {
        events.push_back(event);
        return m_accept;
    }

    std::vector<MynyraEvent> events;

private:
    bool m_accept;
};

const std::string kInput =
    "1,XAUUSD,100,102,99,101,10\n"
    "2,XAUUSD,101,103,100,102,11\n";
const std::string kConfig = "runner_schema=1\nstrategy=offline_replay\n";

RunManifestV1 validManifest()
{
    RunManifestV1 manifest;
    manifest.runId = "offline-replay-001";
    manifest.artifactSha256 = sha256HexV1("frozen-artifact");
    manifest.inputSha256 = sha256HexV1(kInput);
    manifest.configSha256 = sha256HexV1(kConfig);
    manifest.requestedResources = RequestedResourceLimitsV1{1024, 1024, 8, 100};
    return manifest;
}

void testSha256KnownVector()
{
    require(sha256HexV1("abc")
                == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            "SHA-256 known vector changed");
}

void testHashPinnedDeterministicReplay()
{
    MynyraOfflineRunnerV1 runner;
    TestObserver observer;
    RecordingSink sink(true);
    const auto first = runner.run(validManifest(), kInput, kConfig, observer, &sink);
    const auto second = runner.run(validManifest(), kInput, kConfig, observer);

    require(first.terminalResult == OfflineTerminalResultV1::Completed,
            "valid local replay did not complete");
    require(first.evidence.complete, "completed replay evidence was incomplete");
    require(first.processedRecords == 2, "replay record count changed");
    require(first.deterministicResultSha256 == second.deterministicResultSha256,
            "same pinned replay produced a different deterministic result");
    require(first.evidence.events.size() == second.evidence.events.size(),
            "same pinned replay produced different evidence cardinality");
    require(first.capabilities.processHealth == ProcessHealthV1::Healthy,
            "successful replay did not report process health separately");
    require(first.capabilities.dataFreshness == DataFreshnessV1::HashPinnedLocal,
            "successful replay did not report local hash-pinned freshness");
    require(first.capabilities.brokerConnectivity == BrokerConnectivityV1::Forbidden,
            "offline replay permitted a broker connectivity capability");
    require(first.capabilities.executionEligibility
                == ExecutionEligibilityV1::IneligibleByManifest,
            "offline replay became execution eligible");
    require(first.evidenceLocations == std::vector<std::string>{
                "memory://mynyra-evidence-v1/offline-replay-001"},
            "offline replay exposed a non-memory evidence location");
    require(!sink.events.empty(), "configured event sink received no redacted event");
    for (const auto& event : sink.events) {
        require(event.mode == SystemMode::BACKTEST,
                "offline evidence event left BACKTEST mode");
        require(event.eventType == "mynyra_offline_replay_v1",
                "offline evidence event exposed an unversioned event type");
        require(event.canonicalSymbol.empty() && !event.localOrderId.has_value(),
                "offline evidence event exposed market or order material");
    }
}

void testManifestAndInputRejection()
{
    MynyraOfflineRunnerV1 runner;
    TestObserver observer;

    auto providerManifest = validManifest();
    providerManifest.providerAllowed = true;
    const auto providerResult = runner.run(providerManifest, kInput, kConfig, observer);
    require(providerResult.terminalResult == OfflineTerminalResultV1::ManifestRejected,
            "provider permission was accepted by the offline runner");

    auto badHashManifest = validManifest();
    badHashManifest.inputSha256 = sha256HexV1("not-the-input");
    const auto badHashResult = runner.run(badHashManifest, kInput, kConfig, observer);
    require(badHashResult.terminalResult == OfflineTerminalResultV1::InputRejected,
            "mismatched local input hash was accepted");

    const std::string malformedInput = "1,lowercase,100,101,99,100,1\n";
    auto malformedManifest = validManifest();
    malformedManifest.inputSha256 = sha256HexV1(malformedInput);
    const auto malformedResult = runner.run(
        malformedManifest, malformedInput, kConfig, observer);
    require(malformedResult.terminalResult == OfflineTerminalResultV1::InputRejected,
            "malformed replay record was accepted");

    auto oversizedConfigManifest = validManifest();
    oversizedConfigManifest.requestedResources.maxConfigBytes = 1;
    const auto oversizedConfigResult = runner.run(
        oversizedConfigManifest, kInput, kConfig, observer);
    require(oversizedConfigResult.terminalResult == OfflineTerminalResultV1::InputRejected,
            "configuration resource limit was not enforced");
}

void testCancellationTimeoutAndResourceFailures()
{
    MynyraOfflineRunnerV1 runner;

    TestObserver cancelled;
    cancelled.cancelled = true;
    const auto cancellationResult = runner.run(validManifest(), kInput, kConfig, cancelled);
    require(cancellationResult.terminalResult == OfflineTerminalResultV1::Cancelled,
            "cancellation was not preserved as a terminal result");

    TestObserver timedOut;
    timedOut.elapsed = 101;
    const auto timeoutResult = runner.run(validManifest(), kInput, kConfig, timedOut);
    require(timeoutResult.terminalResult == OfflineTerminalResultV1::TimedOut,
            "elapsed limit did not stop the replay");

    auto constrainedManifest = validManifest();
    constrainedManifest.requestedResources.maxRecords = 1;
    const auto resourceResult = runner.run(constrainedManifest, kInput, kConfig, TestObserver{});
    require(resourceResult.terminalResult == OfflineTerminalResultV1::ResourceExhausted,
            "record limit did not stop the replay");
}

void testIncompleteEvidenceFailsClosed()
{
    MynyraOfflineRunnerV1 runner;
    TestObserver observer;
    RecordingSink failingSink(false);
    const auto result = runner.run(validManifest(), kInput, kConfig, observer, &failingSink);
    require(result.terminalResult == OfflineTerminalResultV1::EvidenceIncomplete,
            "failed evidence sink did not fail the run closed");
    require(!result.evidence.complete,
            "failed evidence sink was incorrectly reported as complete");
}

} // namespace

int main()
{
    testSha256KnownVector();
    testHashPinnedDeterministicReplay();
    testManifestAndInputRejection();
    testCancellationTimeoutAndResourceFailures();
    testIncompleteEvidenceFailsClosed();
    std::cout << "Mynyra offline replay tests passed\n";
    return 0;
}
