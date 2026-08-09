#include "CTraderOAuthCorrelation.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <utility>

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) \
    || defined(__NetBSD__)
#include <cstdlib>
#elif defined(__linux__)
#include <sys/random.h>
#endif

namespace {

constexpr std::string_view BASE64URL_ALPHABET =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
constexpr std::size_t MAX_QUERY_PARAMETERS = 16;

void secureClear(std::string& value) noexcept
{
    volatile char* bytes = value.empty() ? nullptr : value.data();
    for (std::size_t i = 0; i < value.size(); ++i) {
        bytes[i] = 0;
    }
    value.clear();
}

bool fillSecureEntropy(
    std::array<uint8_t, CTraderOAuthCorrelationGuard::ENTROPY_BYTES>& out) noexcept
{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) \
    || defined(__NetBSD__)
    ::arc4random_buf(out.data(), out.size());
    return true;
#elif defined(__linux__)
    std::size_t offset = 0;
    while (offset < out.size()) {
        const ssize_t count = ::getrandom(out.data() + offset,
                                          out.size() - offset,
                                          0);
        if (count > 0) {
            offset += static_cast<std::size_t>(count);
            continue;
        }
        if (count < 0 && errno == EINTR) {
            continue;
        }
        return false;
    }
    return true;
#else
    (void)out;
    return false;
#endif
}

std::string base64UrlEncode(
    const std::array<uint8_t, CTraderOAuthCorrelationGuard::ENTROPY_BYTES>& bytes)
{
    std::string result;
    result.reserve((bytes.size() * 4 + 2) / 3);

    std::size_t i = 0;
    while (i + 3 <= bytes.size()) {
        const uint32_t block = (static_cast<uint32_t>(bytes[i]) << 16)
                             | (static_cast<uint32_t>(bytes[i + 1]) << 8)
                             | static_cast<uint32_t>(bytes[i + 2]);
        result.push_back(BASE64URL_ALPHABET[(block >> 18) & 0x3f]);
        result.push_back(BASE64URL_ALPHABET[(block >> 12) & 0x3f]);
        result.push_back(BASE64URL_ALPHABET[(block >> 6) & 0x3f]);
        result.push_back(BASE64URL_ALPHABET[block & 0x3f]);
        i += 3;
    }

    const std::size_t remaining = bytes.size() - i;
    if (remaining == 1) {
        const uint32_t block = static_cast<uint32_t>(bytes[i]) << 16;
        result.push_back(BASE64URL_ALPHABET[(block >> 18) & 0x3f]);
        result.push_back(BASE64URL_ALPHABET[(block >> 12) & 0x3f]);
    } else if (remaining == 2) {
        const uint32_t block = (static_cast<uint32_t>(bytes[i]) << 16)
                             | (static_cast<uint32_t>(bytes[i + 1]) << 8);
        result.push_back(BASE64URL_ALPHABET[(block >> 18) & 0x3f]);
        result.push_back(BASE64URL_ALPHABET[(block >> 12) & 0x3f]);
        result.push_back(BASE64URL_ALPHABET[(block >> 6) & 0x3f]);
    }

    return result;
}

bool isHex(char c) noexcept
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
        || (c >= 'A' && c <= 'F');
}

bool hasValidEncoding(std::string_view value) noexcept
{
    for (std::size_t i = 0; i < value.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);
        if (c < 0x20 || c == 0x7f) {
            return false;
        }
        if (value[i] == '%') {
            if (i + 2 >= value.size() || !isHex(value[i + 1])
                || !isHex(value[i + 2])) {
                return false;
            }
            i += 2;
        }
    }
    return true;
}

bool constantTimeEqual(std::string_view expected, std::string_view actual) noexcept
{
    if (expected.size() != actual.size()) {
        return false;
    }
    uint8_t difference = 0;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        difference |= static_cast<uint8_t>(expected[i])
                    ^ static_cast<uint8_t>(actual[i]);
    }
    return difference == 0;
}

struct ParsedQuery {
    CTraderOAuthCorrelationGuard::Decision decision{
        CTraderOAuthCorrelationGuard::Decision::MalformedQuery};
    std::string_view state;
};

ParsedQuery parseQuery(std::string_view query) noexcept
{
    using Decision = CTraderOAuthCorrelationGuard::Decision;

    if (query.empty() || query.front() == '?' || query.back() == '&') {
        return {Decision::MalformedQuery, {}};
    }

    std::array<std::string_view, MAX_QUERY_PARAMETERS> names{};
    std::size_t nameCount = 0;
    bool hasCode = false;
    bool hasError = false;
    std::string_view state;

    std::size_t start = 0;
    while (start < query.size()) {
        if (nameCount == names.size()) {
            return {Decision::MalformedQuery, {}};
        }

        const std::size_t separator = query.find('&', start);
        const std::size_t end = separator == std::string_view::npos
                              ? query.size() : separator;
        const std::string_view parameter = query.substr(start, end - start);
        const std::size_t equals = parameter.find('=');
        if (parameter.empty() || equals == std::string_view::npos
            || equals == 0) {
            return {Decision::MalformedQuery, {}};
        }

        const std::string_view name = parameter.substr(0, equals);
        const std::string_view value = parameter.substr(equals + 1);
        if (!hasValidEncoding(name) || !hasValidEncoding(value)) {
            return {Decision::MalformedQuery, {}};
        }
        if (std::find(names.begin(), names.begin() + nameCount, name)
            != names.begin() + nameCount) {
            return {Decision::DuplicateParameter, {}};
        }
        names[nameCount++] = name;

        if (name == "code") {
            if (value.empty()) {
                return {Decision::CodeMissing, {}};
            }
            hasCode = true;
        } else if (name == "state") {
            if (value.empty()) {
                return {Decision::StateMissing, {}};
            }
            state = value;
        } else if (name == "error") {
            if (value.empty()) {
                return {Decision::MalformedQuery, {}};
            }
            hasError = true;
        }

        if (separator == std::string_view::npos) {
            break;
        }
        start = separator + 1;
        if (start == query.size()) {
            return {Decision::MalformedQuery, {}};
        }
    }

    if (hasError) {
        if (hasCode) {
            return {Decision::MalformedQuery, {}};
        }
        return {Decision::AuthorizationRejected, {}};
    }
    if (state.empty()) {
        return {Decision::StateMissing, {}};
    }
    if (!hasCode) {
        return {Decision::CodeMissing, {}};
    }
    return {Decision::CorrelationMatchedCodeDiscarded, state};
}

} // namespace

CTraderOAuthCorrelationGuard::~CTraderOAuthCorrelationGuard()
{
    clearSensitiveState();
}

CTraderOAuthCorrelationGuard::CTraderOAuthCorrelationGuard(
    CTraderOAuthCorrelationGuard&& other) noexcept
    : phase_(other.phase_),
      lastDecision_(other.lastDecision_),
      armedAt_(other.armedAt_),
      expiresAt_(other.expiresAt_),
      state_(std::move(other.state_))
{
    other.clearSensitiveState();
    other.phase_ = Phase::Terminal;
    other.lastDecision_ = Decision::AlreadyTerminal;
}

CTraderOAuthCorrelationGuard& CTraderOAuthCorrelationGuard::operator=(
    CTraderOAuthCorrelationGuard&& other) noexcept
{
    if (this == &other) {
        return *this;
    }
    clearSensitiveState();
    phase_ = other.phase_;
    lastDecision_ = other.lastDecision_;
    armedAt_ = other.armedAt_;
    expiresAt_ = other.expiresAt_;
    state_ = std::move(other.state_);
    other.clearSensitiveState();
    other.phase_ = Phase::Terminal;
    other.lastDecision_ = Decision::AlreadyTerminal;
    return *this;
}

bool CTraderOAuthCorrelationGuard::arm(ListenerBinding binding,
                                       TimePoint now) noexcept
{
    std::array<uint8_t, ENTROPY_BYTES> entropy{};
    if (phase_ != Phase::Unarmed) {
        if (phase_ == Phase::Armed) {
            finish(Decision::AlreadyTerminal);
        } else {
            lastDecision_ = Decision::AlreadyTerminal;
        }
        return false;
    }
    if (!fillSecureEntropy(entropy)) {
        phase_ = Phase::Terminal;
        lastDecision_ = Decision::EntropyUnavailable;
        return false;
    }
    const bool armed = armWithEntropy(binding, now, entropy);
    volatile uint8_t* bytes = entropy.data();
    for (std::size_t i = 0; i < entropy.size(); ++i) {
        bytes[i] = 0;
    }
    return armed;
}

bool CTraderOAuthCorrelationGuard::armWithEntropy(
    ListenerBinding binding,
    TimePoint now,
    const std::array<uint8_t, ENTROPY_BYTES>& entropy) noexcept
{
    if (phase_ != Phase::Unarmed) {
        if (phase_ == Phase::Armed) {
            finish(Decision::AlreadyTerminal);
        } else {
            lastDecision_ = Decision::AlreadyTerminal;
        }
        return false;
    }
    if (binding.address != LOOPBACK_ADDRESS || binding.port != LOOPBACK_PORT) {
        phase_ = Phase::Terminal;
        lastDecision_ = Decision::ListenerBindingRejected;
        return false;
    }

    const Clock::duration lifetime =
        std::chrono::duration_cast<Clock::duration>(CORRELATION_LIFETIME);
    if (now.time_since_epoch() > Clock::duration::max() - lifetime) {
        phase_ = Phase::Terminal;
        lastDecision_ = Decision::CallbackExpired;
        return false;
    }

    try {
        state_ = base64UrlEncode(entropy);
    } catch (...) {
        clearSensitiveState();
        phase_ = Phase::Terminal;
        lastDecision_ = Decision::EntropyUnavailable;
        return false;
    }
    armedAt_ = now;
    expiresAt_ = now + lifetime;
    phase_ = Phase::Armed;
    lastDecision_ = Decision::Armed;
    return true;
}

std::string_view CTraderOAuthCorrelationGuard::stateForAuthorizationRequest()
    const noexcept
{
    return phase_ == Phase::Armed ? std::string_view(state_)
                                  : std::string_view{};
}

CTraderOAuthCorrelationGuard::Decision CTraderOAuthCorrelationGuard::consume(
    const CallbackRequest& request,
    TimePoint now) noexcept
{
    if (phase_ == Phase::Unarmed) {
        return finish(Decision::CallbackBeforeArming);
    }
    if (phase_ == Phase::Terminal) {
        lastDecision_ = Decision::AlreadyTerminal;
        return lastDecision_;
    }

    if (now < armedAt_) {
        return finish(Decision::CallbackBeforeArming);
    }
    if (now >= expiresAt_) {
        return finish(Decision::CallbackExpired);
    }
    if (request.remoteAddress != LOOPBACK_ADDRESS) {
        return finish(Decision::UnexpectedRemote);
    }
    if (request.method != "GET") {
        return finish(Decision::UnexpectedMethod);
    }
    if (request.host != CALLBACK_HOST) {
        return finish(Decision::UnexpectedHost);
    }
    if (request.path != CALLBACK_PATH) {
        return finish(Decision::UnexpectedPath);
    }

    const ParsedQuery parsed = parseQuery(request.rawQuery);
    if (parsed.decision != Decision::CorrelationMatchedCodeDiscarded) {
        return finish(parsed.decision);
    }
    if (!constantTimeEqual(state_, parsed.state)) {
        return finish(Decision::StateMismatch);
    }
    return finish(Decision::CorrelationMatchedCodeDiscarded);
}

CTraderOAuthCorrelationGuard::Decision
CTraderOAuthCorrelationGuard::expireIfDue(TimePoint now) noexcept
{
    if (phase_ == Phase::Unarmed) {
        return finish(Decision::CallbackBeforeArming);
    }
    if (phase_ == Phase::Terminal) {
        lastDecision_ = Decision::AlreadyTerminal;
        return lastDecision_;
    }
    if (now < armedAt_) {
        return finish(Decision::CallbackBeforeArming);
    }
    if (now < expiresAt_) {
        lastDecision_ = Decision::Armed;
        return lastDecision_;
    }
    return finish(Decision::CallbackExpired);
}

CTraderOAuthCorrelationGuard::Decision
CTraderOAuthCorrelationGuard::cancel() noexcept
{
    if (phase_ == Phase::Terminal) {
        lastDecision_ = Decision::AlreadyTerminal;
        return lastDecision_;
    }
    return finish(Decision::Cancelled);
}

CTraderOAuthCorrelationGuard::Decision CTraderOAuthCorrelationGuard::finish(
    Decision decision) noexcept
{
    clearSensitiveState();
    phase_ = Phase::Terminal;
    lastDecision_ = decision;
    return decision;
}

void CTraderOAuthCorrelationGuard::clearSensitiveState() noexcept
{
    secureClear(state_);
}

bool CTraderOAuthCorrelationGuard::isArmed() const noexcept
{
    return phase_ == Phase::Armed;
}

bool CTraderOAuthCorrelationGuard::isTerminal() const noexcept
{
    return phase_ == Phase::Terminal;
}

std::string_view CTraderOAuthCorrelationGuard::safeDiagnostic(
    Decision decision) noexcept
{
    switch (decision) {
    case Decision::Unarmed: return "oauth_correlation_unarmed";
    case Decision::Armed: return "oauth_correlation_armed";
    case Decision::ListenerBindingRejected: return "oauth_loopback_binding_rejected";
    case Decision::EntropyUnavailable: return "oauth_secure_entropy_unavailable";
    case Decision::AlreadyTerminal: return "oauth_callback_replay_rejected";
    case Decision::CallbackBeforeArming: return "oauth_callback_before_arming";
    case Decision::CallbackExpired: return "oauth_callback_expired";
    case Decision::Cancelled: return "oauth_correlation_cancelled";
    case Decision::UnexpectedRemote: return "oauth_callback_remote_rejected";
    case Decision::UnexpectedMethod: return "oauth_callback_method_rejected";
    case Decision::UnexpectedHost: return "oauth_callback_host_rejected";
    case Decision::UnexpectedPath: return "oauth_callback_path_rejected";
    case Decision::MalformedQuery: return "oauth_callback_malformed";
    case Decision::DuplicateParameter: return "oauth_callback_duplicate_parameter";
    case Decision::AuthorizationRejected: return "oauth_authorization_rejected";
    case Decision::StateMissing: return "oauth_correlation_missing";
    case Decision::StateMismatch: return "oauth_correlation_mismatch";
    case Decision::CodeMissing: return "oauth_authorization_code_missing";
    case Decision::CorrelationMatchedCodeDiscarded:
        return "oauth_correlation_matched_code_discarded";
    }
    return "oauth_correlation_unknown_failure";
}
