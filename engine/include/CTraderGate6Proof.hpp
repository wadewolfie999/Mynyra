#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tradebot::ctrader {

class SensitiveString {
public:
    SensitiveString() = default;
    explicit SensitiveString(std::string value) noexcept;
    ~SensitiveString();

    SensitiveString(const SensitiveString&) = delete;
    SensitiveString& operator=(const SensitiveString&) = delete;
    SensitiveString(SensitiveString&& other) noexcept;
    SensitiveString& operator=(SensitiveString&& other) noexcept;

    std::string_view view() const noexcept { return value_; }
    bool empty() const noexcept { return value_.empty(); }
    void clear() noexcept;

private:
    std::string value_;
};

struct CTraderGate6Config final {
    static constexpr std::string_view AUTHORIZATION_HOST = "id.ctrader.com";
    static constexpr std::string_view TOKEN_HOST = "openapi.ctrader.com";
    static constexpr std::string_view DEMO_HOST = "demo.ctraderapi.com";
    static constexpr uint16_t DEMO_PORT = 5035;
    static constexpr std::string_view OAUTH_SCOPE = "trading";
    static constexpr std::string_view REDIRECT_URI =
        "http://127.0.0.1:18080/ctrader/oauth/callback";
    static constexpr std::string_view CLIENT_SECRET_SERVICE =
        "TradeBot.cTraderOpenApi.client-secret";
    static constexpr std::string_view TOKEN_SERVICE =
        "TradeBot.cTraderOpenApi.tokens.trading";

    static bool isAllowedOpenApiEndpoint(std::string_view host,
                                         uint16_t port) noexcept;
    static bool isAllowedOutboundPayload(uint32_t payloadType) noexcept;
};

enum class Gate6Decision {
    Ready,
    AwaitingWadeCheckpoint,
    ReadyForGate6B,
    ReadyForAccountAuthentication,
    AccountProofSucceeded,
    Cancelled,
    AlreadyTerminal,
    WrongPhase,
    StaleConnectionGeneration,
    CorrelationRejected,
    TokenOwnershipRejected,
    TradingScopeRequired,
    LiveAccountExcluded,
    MissingAccountMetadata,
    UnsafeBrokerMetadata,
    InvalidAccountIdentifier,
    NoEligibleDemoAccount,
    AmbiguousDemoAccount,
    WadeSelectionRejected,
    AccountAuthenticationMismatch,
    ResourceExhausted
};

struct Gate6AccountRecord {
    uint64_t accountId{0};
    std::optional<bool> isLive;
    std::optional<std::string> brokerTitleShort;

    ~Gate6AccountRecord();
};

struct Gate6AccountListEvidence {
    bool currentConnectionGeneration{false};
    bool correlationMatched{false};
    bool tokenOwned{false};
    bool tradingScope{false};
    std::vector<Gate6AccountRecord> accounts;
};

struct Gate6SafeCandidate {
    bool isLive{false};
    std::string brokerTitleShort;
};

class CTraderGate6AccountProof {
public:
    CTraderGate6AccountProof() = default;
    ~CTraderGate6AccountProof();

    CTraderGate6AccountProof(const CTraderGate6AccountProof&) = delete;
    CTraderGate6AccountProof& operator=(const CTraderGate6AccountProof&) = delete;
    CTraderGate6AccountProof(CTraderGate6AccountProof&&) = delete;
    CTraderGate6AccountProof& operator=(CTraderGate6AccountProof&&) = delete;

    Gate6Decision acceptGate6A(Gate6AccountListEvidence evidence) noexcept;
    const std::vector<Gate6SafeCandidate>& safeCandidates() const noexcept
    {
        return safeCandidates_;
    }

    Gate6Decision confirmWadeSelection(std::string_view brokerTitleShort) noexcept;
    Gate6Decision acceptGate6B(Gate6AccountListEvidence evidence) noexcept;
    std::optional<int64_t> accountIdForAuthentication() const noexcept;
    Gate6Decision acceptAccountAuthentication(int64_t responseAccountId) noexcept;
    Gate6Decision cancel() noexcept;

    Gate6Decision lastDecision() const noexcept { return lastDecision_; }
    bool isAwaitingWadeCheckpoint() const noexcept;
    bool isTerminal() const noexcept;

    static std::string_view safeDiagnostic(Gate6Decision decision) noexcept;

private:
    enum class Phase {
        Gate6AReady,
        AwaitingWade,
        Gate6BReady,
        AccountAuthReady,
        Complete,
        Terminal
    };

    struct VolatileCandidate {
        uint64_t accountId{0};
        std::string brokerTitleShort;
    };

    Gate6Decision acceptGate6AImpl(Gate6AccountListEvidence& evidence);
    Gate6Decision confirmWadeSelectionImpl(std::string_view brokerTitleShort);
    Gate6Decision validateEvidence(const Gate6AccountListEvidence& evidence) noexcept;
    Gate6Decision finish(Gate6Decision decision) noexcept;
    void clearAccountIdentifiers() noexcept;
    void clearAll() noexcept;

    Phase phase_{Phase::Gate6AReady};
    Gate6Decision lastDecision_{Gate6Decision::Ready};
    std::vector<VolatileCandidate> volatileCandidates_;
    std::vector<Gate6SafeCandidate> safeCandidates_;
    std::string approvedBrokerTitle_;
    uint64_t selectedAccountId_{0};
};

} // namespace tradebot::ctrader
