#include "MynyraOfflineRun.hpp"

#include <atomic>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace {

constexpr std::string_view kUsage =
    "usage: mynyra_offline_replay --manifest PATH --input PATH --config PATH "
    "--evidence-dir PATH\n";

volatile std::sig_atomic_t gCancellationRequested = 0;

extern "C" void requestCancellation(int) noexcept
{
    gCancellationRequested = 1;
}

class ProcessObserver final : public IOfflineRunObserverV1 {
public:
    bool cancellationRequested() const noexcept override
    {
        return gCancellationRequested != 0;
    }

    std::uint64_t elapsedMilliseconds() const noexcept override
    {
        const auto elapsed = std::chrono::steady_clock::now() - m_started;
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
    }

private:
    const std::chrono::steady_clock::time_point m_started{std::chrono::steady_clock::now()};
};

struct Arguments final {
    std::filesystem::path manifest;
    std::filesystem::path input;
    std::filesystem::path config;
    std::filesystem::path evidenceDirectory;
};

bool parseArguments(int argc, char** argv, Arguments& output, std::string& error)
{
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        std::cout << kUsage;
        std::exit(0);
    }
    if (argc != 9) {
        error = "expected exactly four named paths";
        return false;
    }

    std::map<std::string_view, std::filesystem::path*> fields{
        {"--manifest", &output.manifest},
        {"--input", &output.input},
        {"--config", &output.config},
        {"--evidence-dir", &output.evidenceDirectory},
    };
    for (int index = 1; index < argc; index += 2) {
        const auto found = fields.find(argv[index]);
        if (found == fields.end() || !found->second->empty() || argv[index + 1][0] == '\0') {
            error = "invalid, duplicate, or empty argument";
            return false;
        }
        *found->second = argv[index + 1];
    }
    for (const auto& [name, value] : fields) {
        if (value->empty()) {
            error = "missing " + std::string(name);
            return false;
        }
    }
    return true;
}

bool parseUnsigned(std::string_view value, std::uint64_t& output) noexcept
{
    if (value.empty()) return false;
    const auto [end, error] = std::from_chars(
        value.data(), value.data() + value.size(), output);
    return error == std::errc{} && end == value.data() + value.size();
}

bool parseSize(std::string_view value, std::size_t& output) noexcept
{
    std::uint64_t parsed = 0;
    if (!parseUnsigned(value, parsed) || parsed > static_cast<std::uint64_t>(SIZE_MAX)) {
        return false;
    }
    output = static_cast<std::size_t>(parsed);
    return true;
}

bool parseManifest(const std::string& text, RunManifestV1& manifest, std::string& error)
{
    std::map<std::string, std::string> fields;
    std::istringstream input{text};
    std::string line;
    while (std::getline(input, line)) {
        const auto separator = line.find('=');
        if (separator == std::string::npos || separator == 0
            || separator + 1 == line.size() || line.find('=', separator + 1) != std::string::npos) {
            error = "manifest contains malformed line";
            return false;
        }
        const auto [_, inserted] = fields.emplace(
            line.substr(0, separator), line.substr(separator + 1));
        if (!inserted) {
            error = "manifest contains duplicate key";
            return false;
        }
    }

    static const std::map<std::string, std::size_t> expected{
        {"schema_version", 0}, {"run_id", 0}, {"artifact_sha256", 0},
        {"input_sha256", 0}, {"config_sha256", 0}, {"mode", 0},
        {"max_input_bytes", 0}, {"max_config_bytes", 0}, {"max_records", 0},
        {"max_runtime_milliseconds", 0}, {"provider_allowed", 0},
        {"orders_allowed", 0},
    };
    if (fields.size() != expected.size()) {
        error = "manifest key set is incomplete";
        return false;
    }
    for (const auto& [key, value] : fields) {
        (void)value;
        if (!expected.contains(key)) {
            error = "manifest contains unknown key";
            return false;
        }
    }

    std::uint64_t schemaVersion = 0;
    if (!parseUnsigned(fields["schema_version"], schemaVersion) || schemaVersion != 1
        || fields["mode"] != "BACKTEST" || fields["provider_allowed"] != "false"
        || fields["orders_allowed"] != "false"
        || !parseSize(fields["max_input_bytes"], manifest.requestedResources.maxInputBytes)
        || !parseSize(fields["max_config_bytes"], manifest.requestedResources.maxConfigBytes)
        || !parseSize(fields["max_records"], manifest.requestedResources.maxRecords)
        || !parseUnsigned(fields["max_runtime_milliseconds"],
                          manifest.requestedResources.maxRuntimeMilliseconds)) {
        error = "manifest contains invalid value";
        return false;
    }
    manifest.schemaVersion = 1;
    manifest.runId = fields["run_id"];
    manifest.artifactSha256 = fields["artifact_sha256"];
    manifest.inputSha256 = fields["input_sha256"];
    manifest.configSha256 = fields["config_sha256"];
    manifest.mode = SystemMode::BACKTEST;
    manifest.providerAllowed = false;
    manifest.ordersAllowed = false;
    return true;
}

bool readRegularFile(const std::filesystem::path& path,
                     std::size_t maximumBytes,
                     std::string& output,
                     std::string& error)
{
    std::error_code statusError;
    if (!std::filesystem::is_regular_file(path, statusError) || statusError) {
        error = "path is not a regular file";
        return false;
    }
    const auto size = std::filesystem::file_size(path, statusError);
    if (statusError || size > maximumBytes) {
        error = "file exceeds declared resource limit";
        return false;
    }
    std::ifstream input(path, std::ios::binary);
    output.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    if (!input.good() && !input.eof()) {
        error = "unable to read file";
        return false;
    }
    return true;
}

const char* terminalName(OfflineTerminalResultV1 value) noexcept
{
    switch (value) {
        case OfflineTerminalResultV1::Completed: return "COMPLETED";
        case OfflineTerminalResultV1::ManifestRejected: return "MANIFEST_REJECTED";
        case OfflineTerminalResultV1::InputRejected: return "INPUT_REJECTED";
        case OfflineTerminalResultV1::Cancelled: return "CANCELLED";
        case OfflineTerminalResultV1::TimedOut: return "TIMED_OUT";
        case OfflineTerminalResultV1::ResourceExhausted: return "RESOURCE_EXHAUSTED";
        case OfflineTerminalResultV1::EvidenceIncomplete: return "EVIDENCE_INCOMPLETE";
        case OfflineTerminalResultV1::InternalFailure: return "INTERNAL_FAILURE";
    }
    return "INTERNAL_FAILURE";
}

const char* evidenceName(OfflineEvidenceCodeV1 value) noexcept
{
    switch (value) {
        case OfflineEvidenceCodeV1::ManifestAccepted: return "MANIFEST_ACCEPTED";
        case OfflineEvidenceCodeV1::ManifestRejected: return "MANIFEST_REJECTED";
        case OfflineEvidenceCodeV1::InputAccepted: return "INPUT_ACCEPTED";
        case OfflineEvidenceCodeV1::ReplayStarted: return "REPLAY_STARTED";
        case OfflineEvidenceCodeV1::ReplayProgress: return "REPLAY_PROGRESS";
        case OfflineEvidenceCodeV1::Completed: return "COMPLETED";
        case OfflineEvidenceCodeV1::Cancelled: return "CANCELLED";
        case OfflineEvidenceCodeV1::TimedOut: return "TIMED_OUT";
        case OfflineEvidenceCodeV1::MalformedInput: return "MALFORMED_INPUT";
        case OfflineEvidenceCodeV1::ResourceExhausted: return "RESOURCE_EXHAUSTED";
        case OfflineEvidenceCodeV1::EvidenceIncomplete: return "EVIDENCE_INCOMPLETE";
        case OfflineEvidenceCodeV1::InternalFailure: return "INTERNAL_FAILURE";
    }
    return "INTERNAL_FAILURE";
}

const char* processHealthName(ProcessHealthV1 value) noexcept
{
    switch (value) {
        case ProcessHealthV1::NotStarted: return "NOT_STARTED";
        case ProcessHealthV1::Healthy: return "HEALTHY";
        case ProcessHealthV1::Degraded: return "DEGRADED";
    }
    return "DEGRADED";
}

const char* dataFreshnessName(DataFreshnessV1 value) noexcept
{
    switch (value) {
        case DataFreshnessV1::NotAvailable: return "NOT_AVAILABLE";
        case DataFreshnessV1::HashPinnedLocal: return "HASH_PINNED_LOCAL";
    }
    return "NOT_AVAILABLE";
}

const char* brokerConnectivityName(BrokerConnectivityV1 value) noexcept
{
    switch (value) {
        case BrokerConnectivityV1::NotAttempted: return "NOT_ATTEMPTED";
        case BrokerConnectivityV1::Forbidden: return "FORBIDDEN";
    }
    return "FORBIDDEN";
}

const char* reconciliationName(ReconciliationStateV1 value) noexcept
{
    switch (value) {
        case ReconciliationStateV1::NotRequired: return "NOT_REQUIRED";
        case ReconciliationStateV1::Unavailable: return "UNAVAILABLE";
    }
    return "UNAVAILABLE";
}

std::string jsonString(std::string_view value)
{
    std::string result{"\""};
    for (const unsigned char character : value) {
        if (character == '"') result += "\\\"";
        else if (character == '\\') result += "\\\\";
        else if (character == '\n') result += "\\n";
        else if (character == '\r') result += "\\r";
        else if (character == '\t') result += "\\t";
        else if (character < 0x20) result += "?";
        else result.push_back(static_cast<char>(character));
    }
    result.push_back('"');
    return result;
}

std::string serializeResult(const OfflineRunResultV1& result)
{
    std::ostringstream output;
    output << "{\"schema_version\":1"
           << ",\"terminal_result\":" << jsonString(terminalName(result.terminalResult))
           << ",\"processed_records\":" << result.processedRecords
           << ",\"deterministic_result_sha256\":"
           << jsonString(result.deterministicResultSha256)
           << ",\"capabilities\":{\"process_health\":"
           << jsonString(processHealthName(result.capabilities.processHealth))
           << ",\"data_freshness\":"
           << jsonString(dataFreshnessName(result.capabilities.dataFreshness))
           << ",\"broker_connectivity\":"
           << jsonString(brokerConnectivityName(result.capabilities.brokerConnectivity))
           << ",\"reconciliation\":"
           << jsonString(reconciliationName(result.capabilities.reconciliation))
           << ",\"execution_eligibility\":\"INELIGIBLE_BY_MANIFEST\"}"
           << ",\"evidence\":{\"schema_version\":1"
           << ",\"run_id\":" << jsonString(result.evidence.runId)
           << ",\"artifact_sha256\":" << jsonString(result.evidence.artifactSha256)
           << ",\"input_sha256\":" << jsonString(result.evidence.inputSha256)
           << ",\"config_sha256\":" << jsonString(result.evidence.configSha256)
           << ",\"events\":[";
    for (std::size_t index = 0; index < result.evidence.events.size(); ++index) {
        const auto& event = result.evidence.events[index];
        if (index != 0) output << ',';
        output << "{\"schema_version\":1,\"sequence\":" << event.sequence
               << ",\"code\":" << jsonString(evidenceName(event.code)) << '}';
    }
    output << "] ,\"terminal_result\":" << jsonString(terminalName(result.evidence.terminalResult))
           << ",\"complete\":" << (result.evidence.complete ? "true" : "false")
           << "}}\n";
    return output.str();
}

bool writeAtomically(const std::filesystem::path& directory,
                     std::string_view contents,
                     std::string& error)
{
    std::error_code filesystemError;
    if (std::filesystem::exists(directory, filesystemError) || filesystemError
        || !std::filesystem::exists(directory.parent_path(), filesystemError)
        || filesystemError || !std::filesystem::is_directory(directory.parent_path())) {
        error = "evidence directory must not exist and its parent must exist";
        return false;
    }
    if (!std::filesystem::create_directory(directory, filesystemError) || filesystemError) {
        error = "unable to create evidence directory";
        return false;
    }
    const auto temporary = directory / ".result.json.partial";
    const auto finalPath = directory / "result.json";
    const int descriptor = ::open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (descriptor < 0) {
        error = "unable to create temporary evidence file";
        return false;
    }
    std::size_t offset = 0;
    while (offset < contents.size()) {
        const ssize_t written = ::write(descriptor, contents.data() + offset,
                                        contents.size() - offset);
        if (written <= 0) {
            ::close(descriptor);
            error = "unable to write evidence";
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    if (::fsync(descriptor) != 0 || ::close(descriptor) != 0
        || ::rename(temporary.c_str(), finalPath.c_str()) != 0) {
        error = "unable to finalize evidence";
        return false;
    }
    const int directoryDescriptor = ::open(directory.c_str(), O_RDONLY);
    if (directoryDescriptor < 0 || ::fsync(directoryDescriptor) != 0
        || ::close(directoryDescriptor) != 0) {
        error = "unable to synchronize evidence directory";
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    Arguments arguments;
    std::string error;
    if (!parseArguments(argc, argv, arguments, error)) {
        std::cerr << "offline replay rejected: " << error << '\n' << kUsage;
        return 2;
    }

    std::string manifestText;
    if (!readRegularFile(arguments.manifest, 64 * 1024, manifestText, error)) {
        std::cerr << "offline replay rejected: " << error << '\n';
        return 2;
    }
    RunManifestV1 manifest;
    if (!parseManifest(manifestText, manifest, error)) {
        std::cerr << "offline replay rejected: " << error << '\n';
        return 2;
    }

    std::error_code canonicalError;
    const auto executable = std::filesystem::canonical(argv[0], canonicalError);
    if (canonicalError) {
        std::cerr << "offline replay rejected: unable to resolve executable\n";
        return 2;
    }
    std::string executableBytes;
    if (!readRegularFile(executable, SIZE_MAX, executableBytes, error)
        || sha256HexV1(executableBytes) != manifest.artifactSha256) {
        std::cerr << "offline replay rejected: artifact hash mismatch\n";
        return 2;
    }

    std::string input;
    std::string config;
    if (!readRegularFile(arguments.input, manifest.requestedResources.maxInputBytes, input, error)
        || !readRegularFile(arguments.config, manifest.requestedResources.maxConfigBytes, config, error)) {
        std::cerr << "offline replay rejected: " << error << '\n';
        return 2;
    }

    std::signal(SIGINT, requestCancellation);
    std::signal(SIGTERM, requestCancellation);
    ProcessObserver observer;
    const OfflineRunResultV1 result = MynyraOfflineRunnerV1{}.run(
        manifest, input, config, observer);
    if (!writeAtomically(arguments.evidenceDirectory, serializeResult(result), error)) {
        std::cerr << "offline replay failed: " << error << '\n';
        return 3;
    }
    std::cout << result.deterministicResultSha256 << '\n';
    return result.terminalResult == OfflineTerminalResultV1::Completed ? 0 : 1;
}
