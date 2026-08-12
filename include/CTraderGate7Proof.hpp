#pragma once

#include "BrokerAdapterContracts.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tradebot::ctrader {

struct CTraderGate7Config final {
    static constexpr std::string_view AUTHORIZATION_HOST = "id.ctrader.com";
    static constexpr std::string_view TOKEN_HOST = "openapi.ctrader.com";
    static constexpr std::string_view DEMO_HOST = "demo.ctraderapi.com";
    static constexpr std::uint16_t DEMO_PORT = 5035;
    static constexpr std::string_view OAUTH_SCOPE = "trading";
    static constexpr std::string_view REDIRECT_URI =
        "http://127.0.0.1:18080/ctrader/oauth/callback";
    static constexpr std::string_view CLIENT_SECRET_SERVICE =
        "TradeBot.cTraderOpenApi.client-secret";
    static constexpr std::string_view TOKEN_SERVICE =
        "TradeBot.cTraderOpenApi.tokens.trading";

    static bool isAllowedOpenApiEndpoint(std::string_view host,
                                         std::uint16_t port) noexcept;
    static bool isAllowedOutboundPayload(std::uint32_t payloadType) noexcept;
    static bool isAllowedInboundPayload(std::uint32_t payloadType) noexcept;
};

enum class Gate7Decision {
    Ready,
    AccountAuthenticationReady,
    SymbolListReady,
    FullSymbolReady,
    SubscriptionReady,
    QuoteProofSucceeded,
    AlreadyTerminal,
    WrongPhase,
    StaleConnectionGeneration,
    CorrelationRejected,
    TokenOwnershipRejected,
    TradingScopeRequired,
    InvalidAccountIdentifier,
    NoFiboDemoAccount,
    AmbiguousFiboDemoAccount,
    MissingSymbolMetadata,
    NoCanonicalXauusd,
    AmbiguousCanonicalXauusd,
    FullSymbolMismatch,
    SymbolMetadataRejected,
    SubscriptionMismatch,
    IncompleteSpotSide,
    IncompleteSpotTimestamp,
    SpotAccountMismatch,
    SpotSymbolMismatch,
    InvalidSpot,
    CrossedMarket,
    CheckedArithmeticFailed,
    TimestampUnitUnproven,
    TimestampStale,
    TimestampFuture,
    ProviderError,
    Timeout,
    Cancelled,
    MalformedFrame,
    ResourceExhausted
};

enum class Gate7TimestampUnit {
    Seconds,
    Milliseconds,
    Microseconds,
    Nanoseconds,
    Unproven
};

enum class Gate7ProviderErrorCategory {
    None,
    AccountRejected,
    TokenInvalidated,
    SymbolRejected,
    RateLimited,
    ProviderUnavailable,
    Other
};

enum class Gate7SendOutcome {
    Sent,
    InactiveConnection,
    PayloadRejected,
    CorrelationRejected,
    MessageUninitialized,
    SerializationFailed,
    FrameTooLarge,
    WriteTimeout,
    TransportClosed,
    WriteFailed,
    ResourceExhausted
};

enum class Gate7TransportOutcome {
    Expected,
    Timeout,
    TransportClosed,
    CommonProviderError,
    OpenApiProviderError,
    TokenInvalidated,
    AccountDisconnected,
    ClientDisconnected,
    UnexpectedAllowedPayload,
    CorrelationMismatch,
    MalformedEnvelope,
    InboundTypeRejected,
    ResourceExhausted
};

enum class Gate7ResidualFailure {
    None,
    SubscriptionStateUnavailable,
    SubscriptionSendFailed,
    SubscriptionResponseTimeout,
    SubscriptionTransportClosed,
    SubscriptionAccountRejected,
    SubscriptionTokenInvalidated,
    SubscriptionSymbolRejected,
    SubscriptionRateLimited,
    SubscriptionProviderUnavailable,
    SubscriptionProviderRejected,
    SubscriptionUnexpectedPayload,
    SubscriptionCorrelationRejected,
    SubscriptionResponseMalformed,
    SubscriptionAccountMismatch,
    SubscriptionProofRejected,
    SubscriptionResourceExhausted,
    SpotResponseTimeout,
    SpotTransportClosed,
    SpotAccountRejected,
    SpotTokenInvalidated,
    SpotSymbolRejected,
    SpotRateLimited,
    SpotProviderUnavailable,
    SpotProviderRejected,
    SpotUnexpectedPayload,
    SpotResponseMalformed,
    SpotAccountMismatch,
    SpotSymbolMismatch,
    SpotIncompleteSideTimeout,
    SpotTimestampMissingTimeout,
    SpotCompleteBboTimeout,
    SpotProofRejected,
    SpotResourceExhausted
};

class Gate7HeartbeatCadence final {
public:
    using Clock = std::chrono::steady_clock;
    static constexpr auto INTERVAL = std::chrono::seconds(9);

    void markOutbound(Clock::time_point now) noexcept;
    bool due(Clock::time_point now) const noexcept;
    Clock::time_point boundedWaitDeadline(
        Clock::time_point absoluteDeadline,
        Clock::time_point now) const noexcept;

private:
    Clock::time_point nextDue_{};
    bool armed_{false};
};

struct Gate7AccountRecord final {
    std::optional<std::uint64_t> accountId;
    std::optional<bool> isLive;
    std::optional<std::string> brokerTitleShort;

    ~Gate7AccountRecord();
};

struct Gate7AccountListEvidence final {
    std::uint64_t connectionGeneration{0};
    bool currentConnectionGeneration{false};
    bool correlationMatched{false};
    bool tokenOwned{false};
    bool tradingScope{false};
    std::vector<Gate7AccountRecord> accounts;
};

struct Gate7LightSymbol final {
    std::optional<std::int64_t> symbolId;
    std::optional<std::string> symbolName;
    std::optional<bool> enabled;
    bool archived{false};

    ~Gate7LightSymbol();
};

struct Gate7SymbolsListEvidence final {
    std::uint64_t connectionGeneration{0};
    bool currentConnectionGeneration{false};
    bool correlationMatched{false};
    std::int64_t accountId{0};
    bool includeArchivedSymbols{false};
    std::vector<Gate7LightSymbol> symbols;
    std::vector<std::int64_t> archivedSymbolIds;
};

struct Gate7FullSymbol final {
    std::optional<std::int64_t> symbolId;
    std::optional<std::string> symbolName;
    std::optional<bool> enabled;
    std::optional<bool> archived;
    std::optional<std::int32_t> digits;
    std::optional<std::int32_t> pipPosition;
    std::optional<std::int64_t> minVolume;
    std::optional<std::int64_t> maxVolume;
    std::optional<std::int64_t> stepVolume;
    std::optional<std::int64_t> lotSize;

    ~Gate7FullSymbol();
};

struct Gate7FullSymbolEvidence final {
    std::uint64_t connectionGeneration{0};
    bool currentConnectionGeneration{false};
    bool correlationMatched{false};
    std::int64_t accountId{0};
    std::vector<Gate7FullSymbol> symbols;
    std::vector<std::int64_t> archivedSymbolIds;
};

struct Gate7SubscriptionEvidence final {
    std::uint64_t connectionGeneration{0};
    bool currentConnectionGeneration{false};
    bool correlationMatched{false};
    std::int64_t accountId{0};
    std::int64_t symbolId{0};
    std::size_t requestedSymbolCount{0};

    ~Gate7SubscriptionEvidence();
};

struct Gate7SpotEvidence final {
    std::uint64_t connectionGeneration{0};
    bool currentConnectionGeneration{false};
    bool subscriptionMatched{false};
    std::int64_t accountId{0};
    std::int64_t symbolId{0};
    std::optional<std::uint64_t> bid;
    std::optional<std::uint64_t> ask;
    std::optional<std::int64_t> timestamp;
    std::uint64_t receiptTimestampNs{0};

    ~Gate7SpotEvidence();
};

struct Gate7TimestampProof final {
    Gate7TimestampUnit unit{Gate7TimestampUnit::Unproven};
    std::uint64_t timestampNs{0};
    std::uint64_t receiptTimestampNs{0};
    std::int64_t freshnessDeltaNs{0};
};

struct Gate7TimestampClassification final {
    Gate7Decision decision{Gate7Decision::TimestampUnitUnproven};
    std::optional<Gate7TimestampProof> proof;
};

struct Gate7QuoteEvidence final {
    std::string canonicalSymbol;
    std::string executionAlias;
    InstrumentSpec instrument;
    std::int32_t pipPosition{0};
    Decimal64 bid;
    Decimal64 ask;
    Decimal64 spread;
    Gate7TimestampProof timestamp;
};

class CTraderGate7Proof final {
public:
    explicit CTraderGate7Proof(std::uint64_t connectionGeneration) noexcept;
    ~CTraderGate7Proof();

    CTraderGate7Proof(const CTraderGate7Proof&) = delete;
    CTraderGate7Proof& operator=(const CTraderGate7Proof&) = delete;
    CTraderGate7Proof(CTraderGate7Proof&&) = delete;
    CTraderGate7Proof& operator=(CTraderGate7Proof&&) = delete;

    Gate7Decision acceptAccountList(Gate7AccountListEvidence evidence) noexcept;
    std::optional<std::int64_t> accountIdForAuthentication() const noexcept;
    Gate7Decision acceptAccountAuthentication(std::int64_t responseAccountId) noexcept;

    Gate7Decision acceptSymbolsList(Gate7SymbolsListEvidence evidence) noexcept;
    std::optional<std::int64_t> symbolIdForFullRequest() const noexcept;
    Gate7Decision acceptFullSymbol(Gate7FullSymbolEvidence evidence) noexcept;
    std::optional<std::int64_t> symbolIdForSubscription() const noexcept;
    Gate7Decision acceptSubscription(Gate7SubscriptionEvidence evidence) noexcept;
    Gate7Decision acceptSpot(Gate7SpotEvidence evidence) noexcept;

    Gate7Decision terminal(Gate7Decision decision) noexcept;
    Gate7Decision lastDecision() const noexcept { return lastDecision_; }
    bool isTerminal() const noexcept;
    const std::optional<Gate7QuoteEvidence>& quoteEvidence() const noexcept
    {
        return quoteEvidence_;
    }

    static bool isCanonicalXauusd(std::string_view symbolName) noexcept;
    static std::optional<std::int64_t> normalizeSignedScale5(
        std::int64_t units, std::int32_t digits) noexcept;
    static std::optional<Decimal64> normalizeSpotPrice(
        std::uint64_t rawWire, std::int32_t digits) noexcept;
    static std::optional<Gate7TimestampProof> classifyTimestamp(
        std::uint64_t rawTimestamp, std::uint64_t receiptTimestampNs) noexcept;
    static Gate7TimestampClassification classifyTimestampDetailed(
        std::uint64_t rawTimestamp, std::uint64_t receiptTimestampNs) noexcept;
    static std::string_view timestampUnitName(Gate7TimestampUnit unit) noexcept;
    static std::string_view safeDiagnostic(Gate7Decision decision) noexcept;

private:
    enum class Phase {
        AccountList,
        AccountAuthentication,
        SymbolsList,
        FullSymbol,
        Subscription,
        Spot,
        Complete,
        Terminal
    };

    Gate7Decision acceptAccountListImpl(Gate7AccountListEvidence& evidence);
    Gate7Decision acceptAccountAuthenticationImpl(std::int64_t responseAccountId);
    Gate7Decision acceptSymbolsListImpl(Gate7SymbolsListEvidence& evidence);
    Gate7Decision acceptFullSymbolImpl(Gate7FullSymbolEvidence& evidence);
    Gate7Decision acceptSubscriptionImpl(Gate7SubscriptionEvidence& evidence);
    Gate7Decision acceptSpotImpl(Gate7SpotEvidence& evidence);
    Gate7Decision validateTransportEvidence(std::uint64_t generation,
                                            bool currentGeneration,
                                            bool correlationMatched) const noexcept;
    void clearSensitiveState() noexcept;
    void clearAll() noexcept;
    Gate7Decision finish(Gate7Decision decision) noexcept;

    std::uint64_t connectionGeneration_{0};
    Phase phase_{Phase::AccountList};
    Gate7Decision lastDecision_{Gate7Decision::Ready};
    std::int64_t accountId_{0};
    std::int64_t symbolId_{0};
    std::int32_t pipPosition_{0};
    std::string symbolName_;
    std::optional<InstrumentSpec> instrument_;
    bool subscriptionReady_{false};
    std::optional<Gate7QuoteEvidence> quoteEvidence_;
};

Gate7ProviderErrorCategory classifyGate7ProviderError(
    std::string_view errorCode) noexcept;
Gate7ResidualFailure classifyGate7SubscriptionSendFailure(
    Gate7SendOutcome outcome) noexcept;
Gate7ResidualFailure classifyGate7SubscriptionReceiveFailure(
    Gate7TransportOutcome outcome,
    Gate7ProviderErrorCategory providerCategory) noexcept;
Gate7ResidualFailure classifyGate7SpotReceiveFailure(
    Gate7TransportOutcome outcome,
    Gate7ProviderErrorCategory providerCategory) noexcept;
std::string_view safeGate7ResidualDiagnostic(
    Gate7ResidualFailure failure) noexcept;

} // namespace tradebot::ctrader
