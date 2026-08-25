#pragma once

#include "MynyraEventSink.hpp"
#include "SystemConfig.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// These contracts deliberately model only an offline replay. They provide no
// shell, process, filesystem, network, credential, provider, or order API.

struct RequestedResourceLimitsV1 {
    std::size_t maxInputBytes{0};
    std::size_t maxRecords{0};
    std::uint64_t maxRuntimeMilliseconds{0};
};

struct RunManifestV1 {
    std::uint32_t schemaVersion{1};
    std::string runId;
    std::string artifactSha256;
    std::string inputSha256;
    std::string configSha256;
    SystemMode mode{SystemMode::BACKTEST};
    RequestedResourceLimitsV1 requestedResources;
    bool providerAllowed{false};
    bool ordersAllowed{false};
};

enum class ProcessHealthV1 : std::uint8_t { NotStarted, Healthy, Degraded };
enum class DataFreshnessV1 : std::uint8_t { NotAvailable, HashPinnedLocal };
enum class BrokerConnectivityV1 : std::uint8_t { NotAttempted, Forbidden };
enum class ReconciliationStateV1 : std::uint8_t { NotRequired, Unavailable };
enum class ExecutionEligibilityV1 : std::uint8_t { IneligibleByManifest };

// Deliberately separate dimensions: consumers must not collapse this report
// into a generic "green" status.
struct CapabilityReportV1 {
    ProcessHealthV1 processHealth{ProcessHealthV1::NotStarted};
    DataFreshnessV1 dataFreshness{DataFreshnessV1::NotAvailable};
    BrokerConnectivityV1 brokerConnectivity{BrokerConnectivityV1::NotAttempted};
    ReconciliationStateV1 reconciliation{ReconciliationStateV1::Unavailable};
    ExecutionEligibilityV1 executionEligibility{
        ExecutionEligibilityV1::IneligibleByManifest};
};

enum class OfflineEvidenceCodeV1 : std::uint8_t {
    ManifestAccepted,
    ManifestRejected,
    InputAccepted,
    ReplayStarted,
    ReplayProgress,
    Completed,
    Cancelled,
    TimedOut,
    MalformedInput,
    ResourceExhausted,
    EvidenceIncomplete,
    InternalFailure
};

struct RedactedEvidenceEventV1 {
    std::uint32_t schemaVersion{1};
    std::uint64_t sequence{0};
    OfflineEvidenceCodeV1 code{OfflineEvidenceCodeV1::InternalFailure};
};

enum class OfflineTerminalResultV1 : std::uint8_t {
    Completed,
    ManifestRejected,
    InputRejected,
    Cancelled,
    TimedOut,
    ResourceExhausted,
    EvidenceIncomplete,
    InternalFailure
};

struct EvidenceEnvelopeV1 {
    std::uint32_t schemaVersion{1};
    std::string runId;
    std::string artifactSha256;
    std::string inputSha256;
    std::string configSha256;
    std::vector<RedactedEvidenceEventV1> events;
    OfflineTerminalResultV1 terminalResult{OfflineTerminalResultV1::InternalFailure};
    bool complete{false};
};

struct OfflineRunResultV1 {
    OfflineTerminalResultV1 terminalResult{OfflineTerminalResultV1::InternalFailure};
    CapabilityReportV1 capabilities;
    EvidenceEnvelopeV1 evidence;
    std::string deterministicResultSha256;
    std::vector<std::string> evidenceLocations;
    std::size_t processedRecords{0};
};

class IOfflineRunObserverV1 {
public:
    virtual ~IOfflineRunObserverV1() = default;
    virtual bool cancellationRequested() const noexcept = 0;
    virtual std::uint64_t elapsedMilliseconds() const noexcept = 0;
};

class PassiveOfflineRunObserverV1 final : public IOfflineRunObserverV1 {
public:
    bool cancellationRequested() const noexcept override { return false; }
    std::uint64_t elapsedMilliseconds() const noexcept override { return 0; }
};

class MynyraOfflineRunnerV1 final {
public:
    // `input` and `config` are caller-owned local bytes. The runner validates
    // their pinned hashes but never opens a path or makes an external request.
    OfflineRunResultV1 run(const RunManifestV1& manifest,
                           std::string_view input,
                           std::string_view config,
                           const IOfflineRunObserverV1& observer,
                           IEventSink* eventSink = nullptr) const;
};

std::string sha256HexV1(std::string_view value);
