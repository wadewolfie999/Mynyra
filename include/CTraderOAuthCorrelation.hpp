#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

class CTraderOAuthCorrelationTestAccess;

// Offline-verifiable, one-shot correlation guard for the separately gated
// cTrader OAuth loopback callback. This class performs no network, browser,
// OAuth, token, account, market-data, or trading operation.
class CTraderOAuthCorrelationGuard {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    static constexpr std::string_view LOOPBACK_ADDRESS = "127.0.0.1";
    static constexpr uint16_t LOOPBACK_PORT = 18080;
    static constexpr std::string_view CALLBACK_PATH = "/ctrader/oauth/callback";
    static constexpr std::string_view CALLBACK_HOST = "127.0.0.1:18080";
    static constexpr std::chrono::seconds CORRELATION_LIFETIME{60};
    static constexpr std::size_t ENTROPY_BYTES = 32;

    struct ListenerBinding {
        std::string_view address;
        uint16_t port{0};
    };

    struct CallbackRequest {
        std::string_view remoteAddress;
        std::string_view method;
        std::string_view host;
        std::string_view path;
        std::string_view rawQuery;
    };

    enum class Decision {
        Unarmed,
        Armed,
        ListenerBindingRejected,
        EntropyUnavailable,
        AlreadyTerminal,
        CallbackBeforeArming,
        CallbackExpired,
        UnexpectedRemote,
        UnexpectedMethod,
        UnexpectedHost,
        UnexpectedPath,
        MalformedQuery,
        DuplicateParameter,
        AuthorizationRejected,
        StateMissing,
        StateMismatch,
        CodeMissing,
        CorrelationMatchedCodeDiscarded
    };

    CTraderOAuthCorrelationGuard() = default;
    ~CTraderOAuthCorrelationGuard();

    CTraderOAuthCorrelationGuard(const CTraderOAuthCorrelationGuard&) = delete;
    CTraderOAuthCorrelationGuard& operator=(const CTraderOAuthCorrelationGuard&) = delete;
    CTraderOAuthCorrelationGuard(CTraderOAuthCorrelationGuard&& other) noexcept;
    CTraderOAuthCorrelationGuard& operator=(CTraderOAuthCorrelationGuard&& other) noexcept;

    // Uses the operating system CSPRNG. Failure is terminal and leaves no
    // correlation value armed.
    bool arm(ListenerBinding binding, TimePoint now) noexcept;

    // Sensitive, process-memory-only value for the one authorization request.
    // It must never be logged, persisted, or included in diagnostics.
    std::string_view stateForAuthorizationRequest() const noexcept;

    // Consumes the first callback attempt. The authorization code is checked
    // only for presence and is never returned or retained. Every outcome after
    // arming is terminal; later calls are replay rejection.
    Decision consume(const CallbackRequest& request, TimePoint now) noexcept;

    Decision lastDecision() const noexcept { return lastDecision_; }
    bool isArmed() const noexcept;
    bool isTerminal() const noexcept;

    // Returns only fixed, non-sensitive categories suitable for diagnostics.
    static std::string_view safeDiagnostic(Decision decision) noexcept;

private:
    friend class CTraderOAuthCorrelationTestAccess;

    enum class Phase { Unarmed, Armed, Terminal };

    bool armWithEntropy(ListenerBinding binding,
                        TimePoint now,
                        const std::array<uint8_t, ENTROPY_BYTES>& entropy) noexcept;
    Decision finish(Decision decision) noexcept;
    void clearSensitiveState() noexcept;

    Phase phase_{Phase::Unarmed};
    Decision lastDecision_{Decision::Unarmed};
    TimePoint armedAt_{};
    TimePoint expiresAt_{};
    std::string state_;
};
