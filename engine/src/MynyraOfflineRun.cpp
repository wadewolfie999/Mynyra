#include "MynyraOfflineRun.hpp"

#include "PortfolioAllocator.hpp"
#include "StrategyPipeline.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <sstream>
#include <utility>

namespace {

constexpr std::array<std::uint32_t, 64> kSha256RoundConstants{
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
};

constexpr std::uint32_t rotateRight(std::uint32_t value, unsigned shift) noexcept
{
    return (value >> shift) | (value << (32U - shift));
}

std::string hexDigest(const std::array<std::uint32_t, 8>& state)
{
    static constexpr char hex[] = "0123456789abcdef";
    std::string output;
    output.reserve(64);
    for (const std::uint32_t word : state) {
        for (int shift = 28; shift >= 0; shift -= 4) {
            output.push_back(hex[(word >> static_cast<unsigned>(shift)) & 0x0fU]);
        }
    }
    return output;
}

bool isLowerHexSha256(std::string_view value) noexcept
{
    return value.size() == 64 && std::all_of(value.begin(), value.end(),
        [](unsigned char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}

bool isSafeRunId(std::string_view value) noexcept
{
    return !value.empty() && value.size() <= 96
        && std::all_of(value.begin(), value.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
        });
}

bool isSafeSymbol(std::string_view value) noexcept
{
    return !value.empty() && value.size() <= 32
        && std::all_of(value.begin(), value.end(), [](unsigned char c) {
            return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
                || c == '.' || c == '_' || c == '-';
        });
}

bool parseUnsigned(std::string_view input, std::uint64_t& output) noexcept
{
    if (input.empty()) return false;
    const auto [end, error] = std::from_chars(
        input.data(), input.data() + input.size(), output);
    return error == std::errc{} && end == input.data() + input.size();
}

bool parseFiniteDouble(std::string_view input, double& output) noexcept
{
    if (input.empty()) return false;
    std::istringstream parser{std::string(input)};
    parser >> output;
    return parser && parser.eof() && std::isfinite(output);
}

bool parseCandle(std::string_view line, MarketCandle& candle) noexcept
{
    std::array<std::string_view, 7> fields{};
    std::size_t start = 0;
    for (std::size_t fieldIndex = 0; fieldIndex < fields.size(); ++fieldIndex) {
        const std::size_t separator = line.find(',', start);
        if (fieldIndex + 1 == fields.size() && separator != std::string_view::npos) {
            return false;
        }
        fields[fieldIndex] = line.substr(start, separator - start);
        if (separator == std::string_view::npos) {
            if (fieldIndex + 1 != fields.size()) return false;
            break;
        }
        start = separator + 1;
    }

    if (!parseUnsigned(fields[0], candle.epochTimestamp) || !isSafeSymbol(fields[1])
        || !parseFiniteDouble(fields[2], candle.open)
        || !parseFiniteDouble(fields[3], candle.high)
        || !parseFiniteDouble(fields[4], candle.low)
        || !parseFiniteDouble(fields[5], candle.close)
        || !parseFiniteDouble(fields[6], candle.volume)) {
        return false;
    }
    candle.symbol.assign(fields[1]);
    return candle.volume >= 0.0 && candle.high >= candle.low
        && candle.high >= std::max(candle.open, candle.close)
        && candle.low <= std::min(candle.open, candle.close);
}

class ReplayDigestStrategy final : public IStrategy {
public:
    AlphaSignal generateSignal(const MarketCandle& candle) override
    {
        // This portable strategy has no order path. Its bounded conviction
        // exercises StrategyPipeline deterministically while execution stays false.
        const double conviction = candle.close > candle.open ? 0.05
            : candle.close < candle.open ? -0.05 : 0.0;
        return AlphaSignal{candle.symbol, "OFFLINE_REPLAY", conviction};
    }
};

OfflineEvidenceCodeV1 terminalEvidenceCode(OfflineTerminalResultV1 terminal) noexcept
{
    switch (terminal) {
        case OfflineTerminalResultV1::Completed: return OfflineEvidenceCodeV1::Completed;
        case OfflineTerminalResultV1::ManifestRejected: return OfflineEvidenceCodeV1::ManifestRejected;
        case OfflineTerminalResultV1::InputRejected: return OfflineEvidenceCodeV1::MalformedInput;
        case OfflineTerminalResultV1::Cancelled: return OfflineEvidenceCodeV1::Cancelled;
        case OfflineTerminalResultV1::TimedOut: return OfflineEvidenceCodeV1::TimedOut;
        case OfflineTerminalResultV1::ResourceExhausted: return OfflineEvidenceCodeV1::ResourceExhausted;
        case OfflineTerminalResultV1::EvidenceIncomplete: return OfflineEvidenceCodeV1::EvidenceIncomplete;
        case OfflineTerminalResultV1::InternalFailure: return OfflineEvidenceCodeV1::InternalFailure;
    }
    return OfflineEvidenceCodeV1::InternalFailure;
}

bool appendEvidence(OfflineRunResultV1& result,
                    OfflineEvidenceCodeV1 code,
                    IEventSink* eventSink)
{
    const std::uint64_t sequence = result.evidence.events.size() + 1;
    result.evidence.events.push_back(RedactedEvidenceEventV1{1, sequence, code});
    if (eventSink == nullptr) return true;

    MynyraEvent event;
    event.schemaVersion = 1;
    event.sessionId = result.evidence.runId;
    event.localSequence = sequence;
    event.mode = SystemMode::BACKTEST;
    event.eventType = "mynyra_offline_replay_v1";
    event.failure = FailureCategory::None;
    return eventSink->emit(event, EventFlush::LifecycleBoundary);
}

OfflineRunResultV1 initialiseResult(const RunManifestV1& manifest)
{
    OfflineRunResultV1 result;
    result.capabilities.brokerConnectivity = BrokerConnectivityV1::Forbidden;
    result.capabilities.reconciliation = ReconciliationStateV1::NotRequired;
    result.evidence.runId = manifest.runId;
    result.evidence.artifactSha256 = manifest.artifactSha256;
    result.evidence.inputSha256 = manifest.inputSha256;
    result.evidence.configSha256 = manifest.configSha256;
    if (isSafeRunId(manifest.runId)) {
        result.evidenceLocations.push_back(
            "memory://mynyra-evidence-v1/" + manifest.runId);
    }
    return result;
}

OfflineRunResultV1 finish(OfflineRunResultV1 result,
                          OfflineTerminalResultV1 terminal,
                          IEventSink* eventSink)
{
    result.terminalResult = terminal;
    result.evidence.terminalResult = terminal;
    result.evidence.complete = terminal != OfflineTerminalResultV1::EvidenceIncomplete;
    if (!result.evidence.complete) {
        appendEvidence(result, OfflineEvidenceCodeV1::EvidenceIncomplete, nullptr);
        return result;
    }
    if (!appendEvidence(result, terminalEvidenceCode(terminal), eventSink)) {
        result.terminalResult = OfflineTerminalResultV1::EvidenceIncomplete;
        result.evidence.terminalResult = OfflineTerminalResultV1::EvidenceIncomplete;
        result.evidence.complete = false;
        appendEvidence(result, OfflineEvidenceCodeV1::EvidenceIncomplete, nullptr);
    }
    return result;
}

} // namespace

std::string sha256HexV1(std::string_view value)
{
    std::array<std::uint32_t, 8> state{
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
    };

    std::string bytes(value);
    const std::uint64_t bitLength = static_cast<std::uint64_t>(bytes.size()) * 8U;
    bytes.push_back(static_cast<char>(0x80));
    while ((bytes.size() % 64U) != 56U) bytes.push_back('\0');
    for (int shift = 56; shift >= 0; shift -= 8) {
        bytes.push_back(static_cast<char>((bitLength >> static_cast<unsigned>(shift)) & 0xffU));
    }

    for (std::size_t offset = 0; offset < bytes.size(); offset += 64) {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t index = 0; index < 16; ++index) {
            const auto b0 = static_cast<std::uint8_t>(bytes[offset + index * 4]);
            const auto b1 = static_cast<std::uint8_t>(bytes[offset + index * 4 + 1]);
            const auto b2 = static_cast<std::uint8_t>(bytes[offset + index * 4 + 2]);
            const auto b3 = static_cast<std::uint8_t>(bytes[offset + index * 4 + 3]);
            words[index] = (static_cast<std::uint32_t>(b0) << 24U)
                | (static_cast<std::uint32_t>(b1) << 16U)
                | (static_cast<std::uint32_t>(b2) << 8U)
                | static_cast<std::uint32_t>(b3);
        }
        for (std::size_t index = 16; index < words.size(); ++index) {
            const std::uint32_t sigma0 = rotateRight(words[index - 15], 7)
                ^ rotateRight(words[index - 15], 18) ^ (words[index - 15] >> 3U);
            const std::uint32_t sigma1 = rotateRight(words[index - 2], 17)
                ^ rotateRight(words[index - 2], 19) ^ (words[index - 2] >> 10U);
            words[index] = words[index - 16] + sigma0 + words[index - 7] + sigma1;
        }

        std::uint32_t a = state[0];
        std::uint32_t b = state[1];
        std::uint32_t c = state[2];
        std::uint32_t d = state[3];
        std::uint32_t e = state[4];
        std::uint32_t f = state[5];
        std::uint32_t g = state[6];
        std::uint32_t h = state[7];
        for (std::size_t index = 0; index < words.size(); ++index) {
            const std::uint32_t sigma1 = rotateRight(e, 6) ^ rotateRight(e, 11)
                ^ rotateRight(e, 25);
            const std::uint32_t choice = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 = h + sigma1 + choice + kSha256RoundConstants[index]
                + words[index];
            const std::uint32_t sigma0 = rotateRight(a, 2) ^ rotateRight(a, 13)
                ^ rotateRight(a, 22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = sigma0 + majority;
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }
    return hexDigest(state);
}

OfflineRunResultV1 MynyraOfflineRunnerV1::run(
    const RunManifestV1& manifest,
    std::string_view input,
    std::string_view config,
    const IOfflineRunObserverV1& observer,
    IEventSink* eventSink) const
{
    try {
        OfflineRunResultV1 result = initialiseResult(manifest);
        const bool validManifest = manifest.schemaVersion == 1
            && isSafeRunId(manifest.runId)
            && isLowerHexSha256(manifest.artifactSha256)
            && isLowerHexSha256(manifest.inputSha256)
            && isLowerHexSha256(manifest.configSha256)
            && manifest.mode == SystemMode::BACKTEST
            && manifest.requestedResources.maxInputBytes > 0
            && manifest.requestedResources.maxConfigBytes > 0
            && manifest.requestedResources.maxRecords > 0
            && manifest.requestedResources.maxRuntimeMilliseconds > 0
            && !manifest.providerAllowed && !manifest.ordersAllowed;
        if (!validManifest) {
            return finish(std::move(result), OfflineTerminalResultV1::ManifestRejected, eventSink);
        }
        if (!appendEvidence(result, OfflineEvidenceCodeV1::ManifestAccepted, eventSink)) {
            return finish(std::move(result), OfflineTerminalResultV1::EvidenceIncomplete, nullptr);
        }
        if (input.size() > manifest.requestedResources.maxInputBytes
            || config.size() > manifest.requestedResources.maxConfigBytes
            || sha256HexV1(input) != manifest.inputSha256
            || sha256HexV1(config) != manifest.configSha256) {
            return finish(std::move(result), OfflineTerminalResultV1::InputRejected, eventSink);
        }
        if (!appendEvidence(result, OfflineEvidenceCodeV1::InputAccepted, eventSink)) {
            return finish(std::move(result), OfflineTerminalResultV1::EvidenceIncomplete, nullptr);
        }

        result.capabilities.processHealth = ProcessHealthV1::Healthy;
        result.capabilities.dataFreshness = DataFreshnessV1::HashPinnedLocal;
        if (!appendEvidence(result, OfflineEvidenceCodeV1::ReplayStarted, eventSink)) {
            return finish(std::move(result), OfflineTerminalResultV1::EvidenceIncomplete, nullptr);
        }

        ReplayDigestStrategy strategy;
        PortfolioAllocator allocator;
        StrategyPipeline pipeline({&strategy}, allocator);
        std::istringstream lines{std::string(input)};
        std::string line;
        std::uint64_t previousTimestamp = 0;
        bool sawRecord = false;
        while (std::getline(lines, line)) {
            if (observer.cancellationRequested()) {
                return finish(std::move(result), OfflineTerminalResultV1::Cancelled, eventSink);
            }
            if (observer.elapsedMilliseconds() > manifest.requestedResources.maxRuntimeMilliseconds) {
                return finish(std::move(result), OfflineTerminalResultV1::TimedOut, eventSink);
            }
            if (result.processedRecords >= manifest.requestedResources.maxRecords) {
                return finish(std::move(result), OfflineTerminalResultV1::ResourceExhausted, eventSink);
            }
            MarketCandle candle;
            if (!parseCandle(line, candle) || (sawRecord && candle.epochTimestamp <= previousTimestamp)) {
                return finish(std::move(result), OfflineTerminalResultV1::InputRejected, eventSink);
            }
            previousTimestamp = candle.epochTimestamp;
            sawRecord = true;
            pipeline.advance(candle, false);
            ++result.processedRecords;
        }
        if (!sawRecord || observer.cancellationRequested()) {
            return finish(std::move(result), observer.cancellationRequested()
                ? OfflineTerminalResultV1::Cancelled : OfflineTerminalResultV1::InputRejected, eventSink);
        }
        if (observer.elapsedMilliseconds() > manifest.requestedResources.maxRuntimeMilliseconds) {
            return finish(std::move(result), OfflineTerminalResultV1::TimedOut, eventSink);
        }

        result.deterministicResultSha256 = sha256HexV1(
            manifest.artifactSha256 + "\n" + manifest.inputSha256 + "\n"
            + manifest.configSha256 + "\n" + std::to_string(result.processedRecords)
            + "\nexecution=false\nprovider=false\norders=false\n");
        if (!appendEvidence(result, OfflineEvidenceCodeV1::ReplayProgress, eventSink)) {
            return finish(std::move(result), OfflineTerminalResultV1::EvidenceIncomplete, nullptr);
        }
        return finish(std::move(result), OfflineTerminalResultV1::Completed, eventSink);
    } catch (...) {
        OfflineRunResultV1 result = initialiseResult(manifest);
        result.capabilities.processHealth = ProcessHealthV1::Degraded;
        return finish(std::move(result), OfflineTerminalResultV1::InternalFailure, eventSink);
    }
}
