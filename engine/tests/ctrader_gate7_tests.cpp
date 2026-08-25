#include "CTraderGate7OAuthDiagnostics.hpp"
#include "CTraderGate7Proof.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

std::atomic<bool> failNextAllocation{false};

void* allocateForTest(std::size_t size)
{
    if (failNextAllocation.exchange(false)) {
        throw std::bad_alloc();
    }
    if (void* memory = std::malloc(size == 0 ? 1 : size)) {
        return memory;
    }
    throw std::bad_alloc();
}

} // namespace

void* operator new(std::size_t size)
{
    return allocateForTest(size);
}

void* operator new[](std::size_t size)
{
    return allocateForTest(size);
}

void operator delete(void* memory) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, std::size_t) noexcept
{
    std::free(memory);
}

namespace {

using tradebot::ctrader::CTraderGate7Config;
using tradebot::ctrader::CTraderGate7Proof;
using tradebot::ctrader::Gate7OAuthFailure;
using ::CTraderOAuthCorrelationGuard;
using tradebot::ctrader::Gate7AccountListEvidence;
using tradebot::ctrader::Gate7AccountRecord;
using tradebot::ctrader::Gate7Decision;
using tradebot::ctrader::Gate7FullSymbol;
using tradebot::ctrader::Gate7FullSymbolEvidence;
using tradebot::ctrader::Gate7HeartbeatCadence;
using tradebot::ctrader::Gate7LightSymbol;
using tradebot::ctrader::Gate7ProviderErrorCategory;
using tradebot::ctrader::Gate7ResidualFailure;
using tradebot::ctrader::Gate7SendOutcome;
using tradebot::ctrader::Gate7SpotEvidence;
using tradebot::ctrader::Gate7SubscriptionEvidence;
using tradebot::ctrader::Gate7SymbolsListEvidence;
using tradebot::ctrader::Gate7TimestampUnit;
using tradebot::ctrader::Gate7TransportOutcome;

constexpr std::uint64_t GENERATION = 77;
constexpr std::int64_t ACCOUNT = 101;
constexpr std::int64_t SYMBOL = 202;
constexpr std::uint64_t RECEIPT_NS = 1700000000ULL * 1000000000ULL;
constexpr std::int64_t TIMESTAMP_SECONDS = 1700000000;

void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

Gate7AccountRecord account(std::uint64_t id,
                           std::optional<bool> isLive,
                           std::optional<std::string> broker)
{
    return {id, isLive, std::move(broker)};
}

Gate7AccountListEvidence accountEvidence(
    std::vector<Gate7AccountRecord> accounts)
{
    return {GENERATION, true, true, true, true, std::move(accounts)};
}

Gate7LightSymbol light(std::int64_t id, std::string name, bool enabled = true)
{
    return {id, std::move(name), enabled, false};
}

Gate7FullSymbol full(std::int64_t id = SYMBOL)
{
    return {id, std::nullopt, std::nullopt, false,
            2, 1, 100, 500, 50, 10000};
}

Gate7SymbolsListEvidence symbolsEvidence(
    std::vector<Gate7LightSymbol> symbols)
{
    return {GENERATION, true, true, ACCOUNT, false,
            std::move(symbols), {}};
}

Gate7FullSymbolEvidence fullEvidence(Gate7FullSymbol symbol)
{
    return {GENERATION, true, true, ACCOUNT, {std::move(symbol)}, {}};
}

Gate7SubscriptionEvidence subscriptionEvidence()
{
    return {GENERATION, true, true, ACCOUNT, SYMBOL, 1};
}

Gate7SpotEvidence spotEvidence(std::optional<std::uint64_t> bid = 23456789,
                               std::optional<std::uint64_t> ask = 23456800,
                               std::optional<std::int64_t> timestamp =
                                   TIMESTAMP_SECONDS)
{
    return {GENERATION, true, true, ACCOUNT, SYMBOL, bid, ask, timestamp,
            RECEIPT_NS};
}

void prepareForSymbols(CTraderGate7Proof& clean)
{
    require(clean.acceptAccountList(accountEvidence({
                account(ACCOUNT, true, "FIBO"),
                account(ACCOUNT, false, "FIBO")
            })) == Gate7Decision::AccountAuthenticationReady,
            "fresh exact FIBO demo account was not selected");
    require(clean.acceptAccountAuthentication(ACCOUNT)
                == Gate7Decision::SymbolListReady,
            "account authentication did not advance the proof");
}

void prepareForSpot(CTraderGate7Proof& proof)
{
    prepareForSymbols(proof);
    require(proof.acceptSymbolsList(symbolsEvidence({
                light(SYMBOL, "xau/usd"),
                light(SYMBOL + 1, "EURUSD")
            })) == Gate7Decision::FullSymbolReady,
            "canonical XAUUSD light symbol was not resolved");
    require(proof.symbolIdForFullRequest().value_or(0) == SYMBOL,
            "full-symbol request did not use the response-derived ID");
    require(proof.acceptFullSymbol(fullEvidence(full()))
                == Gate7Decision::SubscriptionReady,
            "complete full symbol metadata was rejected");
    require(proof.acceptSubscription(subscriptionEvidence())
                == Gate7Decision::SubscriptionReady,
            "one-symbol subscription was not accepted");
}

void test_endpoint_and_allowlist()
{
    require(CTraderGate7Config::isAllowedOpenApiEndpoint(
                CTraderGate7Config::DEMO_HOST, CTraderGate7Config::DEMO_PORT),
            "fixed demo endpoint rejected");
    require(!CTraderGate7Config::isAllowedOpenApiEndpoint(
                "live.ctraderapi.com", CTraderGate7Config::DEMO_PORT),
            "live endpoint accepted");
    require(!CTraderGate7Config::isAllowedOpenApiEndpoint(
                CTraderGate7Config::DEMO_HOST, 5036),
            "wrong endpoint port accepted");

    constexpr std::array<std::uint32_t, 8> allowed = {
        51, 2100, 2102, 2114, 2116, 2127, 2129, 2149
    };
    for (const auto payload : allowed) {
        require(CTraderGate7Config::isAllowedOutboundPayload(payload),
                "required Gate 7 payload rejected");
    }
    constexpr std::array<std::uint32_t, 14> forbidden = {
        2106, 2108, 2109, 2111, 2112, 2121, 2124, 2133,
        2135, 2137, 2145, 2155, 2156, 2175
    };
    for (const auto payload : forbidden) {
        require(!CTraderGate7Config::isAllowedOutboundPayload(payload),
                "trading, position, depth, trendbar, or history payload allowed");
    }
    require(CTraderGate7Config::isAllowedInboundPayload(2131),
            "spot event not admitted inbound");
    require(CTraderGate7Config::isAllowedInboundPayload(2142),
            "provider error not admitted inbound");
    require(CTraderGate7Config::isAllowedInboundPayload(2164),
            "account disconnect event not admitted inbound");
    require(!CTraderGate7Config::isAllowedInboundPayload(2126),
            "execution event admitted inbound");
}

void test_residual_transport_and_provider_classification()
{
    struct ProviderCase {
        std::string_view code;
        Gate7ProviderErrorCategory expected;
    };
    constexpr std::array<ProviderCase, 6> providerCases = {{
        {"ACCOUNT_NOT_AUTHORIZED", Gate7ProviderErrorCategory::AccountRejected},
        {"CH_ACCESS_TOKEN_INVALID", Gate7ProviderErrorCategory::TokenInvalidated},
        {"SYMBOL_NOT_FOUND", Gate7ProviderErrorCategory::SymbolRejected},
        {"BLOCKED_PAYLOAD_TYPE", Gate7ProviderErrorCategory::RateLimited},
        {"SERVER_IS_UNDER_MAINTENANCE",
         Gate7ProviderErrorCategory::ProviderUnavailable},
        {"PRIVATE_TEXT", Gate7ProviderErrorCategory::Other}
    }};
    for (const auto& testCase : providerCases) {
        require(tradebot::ctrader::classifyGate7ProviderError(testCase.code)
                    == testCase.expected,
                "provider error code was misclassified");
    }

    struct SendCase {
        Gate7SendOutcome outcome;
        Gate7ResidualFailure expected;
    };
    constexpr std::array<SendCase, 11> sendCases = {{
        {Gate7SendOutcome::Sent, Gate7ResidualFailure::None},
        {Gate7SendOutcome::InactiveConnection,
         Gate7ResidualFailure::SubscriptionTransportClosed},
        {Gate7SendOutcome::PayloadRejected,
         Gate7ResidualFailure::SubscriptionSendFailed},
        {Gate7SendOutcome::CorrelationRejected,
         Gate7ResidualFailure::SubscriptionSendFailed},
        {Gate7SendOutcome::MessageUninitialized,
         Gate7ResidualFailure::SubscriptionSendFailed},
        {Gate7SendOutcome::SerializationFailed,
         Gate7ResidualFailure::SubscriptionSendFailed},
        {Gate7SendOutcome::FrameTooLarge,
         Gate7ResidualFailure::SubscriptionSendFailed},
        {Gate7SendOutcome::WriteTimeout,
         Gate7ResidualFailure::SubscriptionSendFailed},
        {Gate7SendOutcome::TransportClosed,
         Gate7ResidualFailure::SubscriptionTransportClosed},
        {Gate7SendOutcome::WriteFailed,
         Gate7ResidualFailure::SubscriptionSendFailed},
        {Gate7SendOutcome::ResourceExhausted,
         Gate7ResidualFailure::SubscriptionResourceExhausted}
    }};
    for (const auto& testCase : sendCases) {
        require(tradebot::ctrader::classifyGate7SubscriptionSendFailure(
                    testCase.outcome) == testCase.expected,
                "subscription send outcome was misclassified");
    }

    struct ReceiveCase {
        Gate7TransportOutcome outcome;
        Gate7ProviderErrorCategory provider;
        Gate7ResidualFailure subscription;
        Gate7ResidualFailure spot;
    };
    constexpr std::array<ReceiveCase, 13> receiveCases = {{
        {Gate7TransportOutcome::Expected, Gate7ProviderErrorCategory::None,
         Gate7ResidualFailure::None, Gate7ResidualFailure::None},
        {Gate7TransportOutcome::Timeout, Gate7ProviderErrorCategory::None,
         Gate7ResidualFailure::SubscriptionResponseTimeout,
         Gate7ResidualFailure::SpotResponseTimeout},
        {Gate7TransportOutcome::TransportClosed,
         Gate7ProviderErrorCategory::None,
         Gate7ResidualFailure::SubscriptionTransportClosed,
         Gate7ResidualFailure::SpotTransportClosed},
        {Gate7TransportOutcome::CommonProviderError,
         Gate7ProviderErrorCategory::AccountRejected,
         Gate7ResidualFailure::SubscriptionAccountRejected,
         Gate7ResidualFailure::SpotAccountRejected},
        {Gate7TransportOutcome::OpenApiProviderError,
         Gate7ProviderErrorCategory::SymbolRejected,
         Gate7ResidualFailure::SubscriptionSymbolRejected,
         Gate7ResidualFailure::SpotSymbolRejected},
        {Gate7TransportOutcome::TokenInvalidated,
         Gate7ProviderErrorCategory::None,
         Gate7ResidualFailure::SubscriptionTokenInvalidated,
         Gate7ResidualFailure::SpotTokenInvalidated},
        {Gate7TransportOutcome::AccountDisconnected,
         Gate7ProviderErrorCategory::None,
         Gate7ResidualFailure::SubscriptionTransportClosed,
         Gate7ResidualFailure::SpotTransportClosed},
        {Gate7TransportOutcome::ClientDisconnected,
         Gate7ProviderErrorCategory::None,
         Gate7ResidualFailure::SubscriptionTransportClosed,
         Gate7ResidualFailure::SpotTransportClosed},
        {Gate7TransportOutcome::UnexpectedAllowedPayload,
         Gate7ProviderErrorCategory::None,
         Gate7ResidualFailure::SubscriptionUnexpectedPayload,
         Gate7ResidualFailure::SpotUnexpectedPayload},
        {Gate7TransportOutcome::CorrelationMismatch,
         Gate7ProviderErrorCategory::None,
         Gate7ResidualFailure::SubscriptionCorrelationRejected,
         Gate7ResidualFailure::SpotUnexpectedPayload},
        {Gate7TransportOutcome::MalformedEnvelope,
         Gate7ProviderErrorCategory::None,
         Gate7ResidualFailure::SubscriptionResponseMalformed,
         Gate7ResidualFailure::SpotResponseMalformed},
        {Gate7TransportOutcome::InboundTypeRejected,
         Gate7ProviderErrorCategory::None,
         Gate7ResidualFailure::SubscriptionUnexpectedPayload,
         Gate7ResidualFailure::SpotUnexpectedPayload},
        {Gate7TransportOutcome::ResourceExhausted,
         Gate7ProviderErrorCategory::None,
         Gate7ResidualFailure::SubscriptionResourceExhausted,
         Gate7ResidualFailure::SpotResourceExhausted}
    }};
    for (const auto& testCase : receiveCases) {
        require(tradebot::ctrader::classifyGate7SubscriptionReceiveFailure(
                    testCase.outcome, testCase.provider)
                    == testCase.subscription,
                "subscription receive outcome was misclassified");
        require(tradebot::ctrader::classifyGate7SpotReceiveFailure(
                    testCase.outcome, testCase.provider) == testCase.spot,
                "spot receive outcome was misclassified");
    }

    struct ProviderMappingCase {
        Gate7ProviderErrorCategory category;
        Gate7ResidualFailure subscription;
        Gate7ResidualFailure spot;
    };
    constexpr std::array<ProviderMappingCase, 6> providerMappings = {{
        {Gate7ProviderErrorCategory::AccountRejected,
         Gate7ResidualFailure::SubscriptionAccountRejected,
         Gate7ResidualFailure::SpotAccountRejected},
        {Gate7ProviderErrorCategory::TokenInvalidated,
         Gate7ResidualFailure::SubscriptionTokenInvalidated,
         Gate7ResidualFailure::SpotTokenInvalidated},
        {Gate7ProviderErrorCategory::SymbolRejected,
         Gate7ResidualFailure::SubscriptionSymbolRejected,
         Gate7ResidualFailure::SpotSymbolRejected},
        {Gate7ProviderErrorCategory::RateLimited,
         Gate7ResidualFailure::SubscriptionRateLimited,
         Gate7ResidualFailure::SpotRateLimited},
        {Gate7ProviderErrorCategory::ProviderUnavailable,
         Gate7ResidualFailure::SubscriptionProviderUnavailable,
         Gate7ResidualFailure::SpotProviderUnavailable},
        {Gate7ProviderErrorCategory::Other,
         Gate7ResidualFailure::SubscriptionProviderRejected,
         Gate7ResidualFailure::SpotProviderRejected}
    }};
    for (const auto& testCase : providerMappings) {
        require(tradebot::ctrader::classifyGate7SubscriptionReceiveFailure(
                    Gate7TransportOutcome::OpenApiProviderError,
                    testCase.category) == testCase.subscription,
                "subscription provider category was misclassified");
        require(tradebot::ctrader::classifyGate7SpotReceiveFailure(
                    Gate7TransportOutcome::OpenApiProviderError,
                    testCase.category) == testCase.spot,
                "spot provider category was misclassified");
    }

    struct UnexpectedSubscriptionCase {
        std::uint32_t payloadType;
        Gate7ResidualFailure expected;
    };
    constexpr std::array<UnexpectedSubscriptionCase, 18> unexpectedCases = {{
        {2101, Gate7ResidualFailure::SubscriptionPriorStageResponse},
        {2103, Gate7ResidualFailure::SubscriptionPriorStageResponse},
        {2115, Gate7ResidualFailure::SubscriptionPriorStageResponse},
        {2117, Gate7ResidualFailure::SubscriptionPriorStageResponse},
        {2150, Gate7ResidualFailure::SubscriptionPriorStageResponse},
        {2130, Gate7ResidualFailure::SubscriptionUnrequestedUnsubscribeResponse},
        {2131, Gate7ResidualFailure::SubscriptionSpotBeforeAcknowledgement},
        {2120, Gate7ResidualFailure::SubscriptionSymbolChangedEvent},
        {2123, Gate7ResidualFailure::SubscriptionTraderUpdatedEvent},
        {2107, Gate7ResidualFailure::SubscriptionProhibitedAsyncEvent},
        {2126, Gate7ResidualFailure::SubscriptionProhibitedAsyncEvent},
        {2132, Gate7ResidualFailure::SubscriptionProhibitedAsyncEvent},
        {2141, Gate7ResidualFailure::SubscriptionProhibitedAsyncEvent},
        {2155, Gate7ResidualFailure::SubscriptionProhibitedAsyncEvent},
        {2171, Gate7ResidualFailure::SubscriptionProhibitedAsyncEvent},
        {2172, Gate7ResidualFailure::SubscriptionProhibitedAsyncEvent},
        {2188, Gate7ResidualFailure::SubscriptionOtherSchemaPayload},
        {999999, Gate7ResidualFailure::SubscriptionUnknownPayload}
    }};
    for (const auto& testCase : unexpectedCases) {
        require(tradebot::ctrader::classifyGate7UnexpectedSubscriptionPayload(
                    testCase.payloadType) == testCase.expected,
                "unexpected subscription payload category was not closed");
    }
}

void test_heartbeat_cadence_is_bounded()
{
    using Clock = Gate7HeartbeatCadence::Clock;
    Gate7HeartbeatCadence cadence;
    const auto now = Clock::time_point{} + std::chrono::seconds(100);
    require(!cadence.due(now), "unarmed heartbeat cadence was due");
    cadence.markOutbound(now);
    require(!cadence.due(now + std::chrono::seconds(8)),
            "heartbeat cadence fired early");
    require(cadence.due(now + std::chrono::seconds(9)),
            "heartbeat cadence missed its bounded interval");
    require(cadence.boundedWaitDeadline(
                now + std::chrono::seconds(20), now)
                == now + std::chrono::seconds(9),
            "heartbeat wait did not stop at the cadence deadline");
    require(cadence.boundedWaitDeadline(
                now + std::chrono::seconds(5), now)
                == now + std::chrono::seconds(5),
            "heartbeat cadence extended the absolute deadline");
}

void test_oauth_diagnostics_are_fixed_and_complete()
{
    using Failure = Gate7OAuthFailure;
    struct FailureCase {
        Failure failure;
        std::string_view diagnostic;
    };
    constexpr std::array<FailureCase, 27> failures = {{
        {Failure::None, "gate7_oauth_ok"},
        {Failure::ListenerSocketFailed, "gate7_oauth_listener_socket_failed"},
        {Failure::ListenerBindFailed, "gate7_oauth_listener_bind_failed"},
        {Failure::ListenerListenFailed, "gate7_oauth_listener_listen_failed"},
        {Failure::ListenerNonBlockingFailed,
         "gate7_oauth_listener_nonblocking_failed"},
        {Failure::CorrelationArmFailed, "gate7_oauth_correlation_arm_failed"},
        {Failure::AuthorizationUrlFailed, "gate7_oauth_authorization_url_failed"},
        {Failure::BrowserLaunchFailed, "gate7_oauth_browser_launch_failed"},
        {Failure::CallbackTimeout, "gate7_oauth_callback_timeout"},
        {Failure::CallbackWaitFailed, "gate7_oauth_callback_wait_failed"},
        {Failure::CallbackAcceptFailed, "gate7_oauth_callback_accept_failed"},
        {Failure::CallbackReadFailed, "gate7_oauth_callback_read_failed"},
        {Failure::CallbackMalformed, "gate7_oauth_callback_malformed"},
        {Failure::CallbackRemoteRejected,
         "gate7_oauth_callback_remote_rejected"},
        {Failure::CallbackMethodRejected,
         "gate7_oauth_callback_method_rejected"},
        {Failure::CallbackHostRejected, "gate7_oauth_callback_host_rejected"},
        {Failure::CallbackPathRejected, "gate7_oauth_callback_path_rejected"},
        {Failure::CallbackQueryMalformed,
         "gate7_oauth_callback_query_malformed"},
        {Failure::AuthorizationDenied, "gate7_oauth_authorization_denied"},
        {Failure::CallbackBeforeArming,
         "gate7_oauth_callback_before_arming"},
        {Failure::StateMissing, "gate7_oauth_state_missing"},
        {Failure::StateMismatch, "gate7_oauth_state_mismatch"},
        {Failure::CodeMissing, "gate7_oauth_code_missing"},
        {Failure::CodeExtractionFailed,
         "gate7_oauth_code_extraction_failed"},
        {Failure::CallbackReplayRejected,
         "gate7_oauth_callback_replay_rejected"},
        {Failure::CallbackCancelled, "gate7_oauth_callback_cancelled"},
        {Failure::ResourceExhausted, "gate7_oauth_resource_exhausted"}
    }};
    for (const FailureCase& testCase : failures) {
        const std::string_view actual =
            tradebot::ctrader::safeOAuthDiagnostic(testCase.failure);
        require(actual == testCase.diagnostic,
                "OAuth diagnostic category changed unexpectedly");
        require(actual.size() <= 64 && actual.find('=') == std::string_view::npos,
                "OAuth diagnostic was unbounded or value-like");
    }

    using Decision = CTraderOAuthCorrelationGuard::Decision;
    struct DecisionCase {
        Decision decision;
        Failure failure;
    };
    constexpr std::array<DecisionCase, 19> decisions = {{
        {Decision::Unarmed, Failure::CorrelationArmFailed},
        {Decision::Armed, Failure::None},
        {Decision::ListenerBindingRejected, Failure::CorrelationArmFailed},
        {Decision::EntropyUnavailable, Failure::CorrelationArmFailed},
        {Decision::AlreadyTerminal, Failure::CallbackReplayRejected},
        {Decision::CallbackBeforeArming, Failure::CallbackBeforeArming},
        {Decision::CallbackExpired, Failure::CallbackTimeout},
        {Decision::Cancelled, Failure::CallbackCancelled},
        {Decision::UnexpectedRemote, Failure::CallbackRemoteRejected},
        {Decision::UnexpectedMethod, Failure::CallbackMethodRejected},
        {Decision::UnexpectedHost, Failure::CallbackHostRejected},
        {Decision::UnexpectedPath, Failure::CallbackPathRejected},
        {Decision::MalformedQuery, Failure::CallbackQueryMalformed},
        {Decision::DuplicateParameter, Failure::CallbackQueryMalformed},
        {Decision::AuthorizationRejected, Failure::AuthorizationDenied},
        {Decision::StateMissing, Failure::StateMissing},
        {Decision::StateMismatch, Failure::StateMismatch},
        {Decision::CodeMissing, Failure::CodeMissing},
        {Decision::CorrelationMatchedCodeDiscarded, Failure::None}
    }};
    for (const DecisionCase& testCase : decisions) {
        require(tradebot::ctrader::classifyOAuthCorrelationFailure(
                    testCase.decision) == testCase.failure,
                "OAuth correlation failure was misclassified");
    }
}

void test_oauth_callback_read_hardening()
{
    using Clock = tradebot::ctrader::Gate7OAuthClock;
    const auto now = Clock::time_point{} + std::chrono::seconds(100);
    const auto correlationDeadline = now + std::chrono::seconds(60);
    require(tradebot::ctrader::gate7OAuthCallbackReadDeadline(
                correlationDeadline, now)
                == now + tradebot::ctrader::GATE7_OAUTH_CALLBACK_READ_TIMEOUT,
            "callback read did not use the bounded inactivity deadline");
    const auto nearDeadline = now + std::chrono::seconds(1);
    require(tradebot::ctrader::gate7OAuthCallbackReadDeadline(
                nearDeadline, now) == nearDeadline,
            "callback read exceeded the absolute correlation deadline");
    require(tradebot::ctrader::gate7OAuthCallbackReadDeadline(
                nearDeadline, nearDeadline) == nearDeadline,
            "expired callback read extended its deadline");

    std::string bounded = "TEST";
    require(tradebot::ctrader::appendGate7OAuthCallbackBytes(
                bounded, "_ONLY", 9) == Gate7OAuthFailure::None,
            "bounded callback bytes were rejected");
    require(bounded == "TEST_ONLY", "callback bytes were not appended exactly");
    require(tradebot::ctrader::appendGate7OAuthCallbackBytes(
                bounded, "X", 9) == Gate7OAuthFailure::CallbackMalformed,
            "oversized callback bytes were not rejected");
    require(bounded == "TEST_ONLY",
            "oversized callback bytes changed retained material");

    std::string allocationTarget;
    const std::string allocationInput(2048, 'x');
    failNextAllocation = true;
    const Gate7OAuthFailure allocationFailure =
        tradebot::ctrader::appendGate7OAuthCallbackBytes(
            allocationTarget, allocationInput, 4096);
    failNextAllocation = false;
    require(allocationFailure == Gate7OAuthFailure::ResourceExhausted,
            "callback allocation failure was not sanitized");
    require(allocationTarget.empty(),
            "callback allocation failure retained partial material");
}

void test_canonical_symbol_rule()
{
    for (const std::string_view value : {"XAUUSD", "xauusd", "XAU/USD",
                                         "xAu/UsD"}) {
        require(CTraderGate7Proof::isCanonicalXauusd(value),
                "valid canonical XAUUSD spelling rejected");
    }
    for (const std::string_view value : {"GOLD", "XAUUSD.r", "XAU/USD.r",
                                         "XAU//USD", "XAU/USDX", ""}) {
        require(!CTraderGate7Proof::isCanonicalXauusd(value),
                "invalid XAUUSD alias accepted");
    }
}

void test_account_selection_is_fresh_exact_and_fail_closed()
{
    CTraderGate7Proof missing(GENERATION);
    require(missing.acceptAccountList(accountEvidence({
                account(1, true, "FIBO"), account(2, std::nullopt, "FIBO"),
                account(3, false, std::nullopt), account(4, false, "OTHER")
            })) == Gate7Decision::NoFiboDemoAccount,
            "live/missing/non-FIBO accounts produced a match");

    CTraderGate7Proof multiple(GENERATION);
    require(multiple.acceptAccountList(accountEvidence({
                account(1, false, "FIBO"), account(2, false, "FIBO")
            })) == Gate7Decision::AmbiguousFiboDemoAccount,
            "multiple exact FIBO demo accounts accepted");

    CTraderGate7Proof invalid(GENERATION);
    require(invalid.acceptAccountList(accountEvidence({
                account(0, false, "FIBO")
            })) == Gate7Decision::NoFiboDemoAccount,
            "zero account ID authorized the flow");

    CTraderGate7Proof scope(GENERATION);
    auto noScope = accountEvidence({account(1, false, "FIBO")});
    noScope.tradingScope = false;
    require(scope.acceptAccountList(std::move(noScope))
                == Gate7Decision::TradingScopeRequired,
            "non-trade account-list scope accepted");

    CTraderGate7Proof fresh(GENERATION);
    require(fresh.acceptAccountList(accountEvidence({
                account(7, false, "FIBO")
            })) == Gate7Decision::AccountAuthenticationReady,
            "fresh account response not used");
    require(fresh.accountIdForAuthentication().value_or(0) == 7,
            "account auth ID was not response-derived");
    require(fresh.acceptAccountAuthentication(8)
                == Gate7Decision::InvalidAccountIdentifier,
            "account authentication mismatch accepted");
    require(!fresh.accountIdForAuthentication().has_value(),
            "mismatched account ID survived terminal cleanup");
}

void test_generation_correlation_and_ordering()
{
    CTraderGate7Proof stale(GENERATION);
    auto evidence = accountEvidence({account(1, false, "FIBO")});
    evidence.connectionGeneration = GENERATION + 1;
    require(stale.acceptAccountList(std::move(evidence))
                == Gate7Decision::StaleConnectionGeneration,
            "wrong connection generation accepted");

    CTraderGate7Proof correlation(GENERATION);
    auto mismatch = accountEvidence({account(1, false, "FIBO")});
    mismatch.correlationMatched = false;
    require(correlation.acceptAccountList(std::move(mismatch))
                == Gate7Decision::CorrelationRejected,
            "correlation mismatch accepted");

    CTraderGate7Proof ordering(GENERATION);
    require(ordering.acceptSubscription(subscriptionEvidence())
                == Gate7Decision::WrongPhase,
            "subscription was accepted before account and symbol proof");
    require(!ordering.accountIdForAuthentication().has_value(),
            "ordering failure retained account state");
}

void test_symbol_resolution_and_metadata_contract()
{
    CTraderGate7Proof disabled(GENERATION);
    prepareForSymbols(disabled);
    require(disabled.acceptSymbolsList(symbolsEvidence({
                light(SYMBOL, "XAUUSD", false)
            })) == Gate7Decision::NoCanonicalXauusd,
            "disabled symbol was eligible");

    CTraderGate7Proof archived(GENERATION);
    prepareForSymbols(archived);
    auto archivedEvidence = symbolsEvidence({light(SYMBOL, "XAUUSD")});
    archivedEvidence.archivedSymbolIds.push_back(SYMBOL);
    require(archived.acceptSymbolsList(std::move(archivedEvidence))
                == Gate7Decision::NoCanonicalXauusd,
            "archived symbol was eligible");

    CTraderGate7Proof multiple(GENERATION);
    prepareForSymbols(multiple);
    require(multiple.acceptSymbolsList(symbolsEvidence({
                light(SYMBOL, "XAUUSD"), light(SYMBOL + 1, "XAU/USD")
            })) == Gate7Decision::AmbiguousCanonicalXauusd,
            "multiple canonical XAUUSD symbols accepted");

    for (const auto mutate : {0, 1, 2, 3, 4, 5, 6}) {
        CTraderGate7Proof proof(GENERATION);
        prepareForSymbols(proof);
        require(proof.acceptSymbolsList(symbolsEvidence({
                    light(SYMBOL, "XAUUSD")
                })) == Gate7Decision::FullSymbolReady,
                "metadata fixture did not resolve XAUUSD");
        Gate7FullSymbol candidate = full();
        switch (mutate) {
        case 0: candidate.digits.reset(); break;
        case 1: candidate.pipPosition = 3; break;
        case 2: candidate.minVolume = 125; break;
        case 3: candidate.maxVolume = 125; break;
        case 4: candidate.stepVolume = 0; break;
        case 5: candidate.lotSize = 0; break;
        case 6: candidate.minVolume = 600; break;
        default: break;
        }
        require(proof.acceptFullSymbol(fullEvidence(std::move(candidate)))
                    == (mutate == 0 ? Gate7Decision::MissingSymbolMetadata
                                     : Gate7Decision::SymbolMetadataRejected),
                "invalid full-symbol metadata accepted");
    }

    CTraderGate7Proof contradictory(GENERATION);
    prepareForSymbols(contradictory);
    require(contradictory.acceptSymbolsList(symbolsEvidence({
                light(SYMBOL, "XAUUSD")
            })) == Gate7Decision::FullSymbolReady,
            "contradiction fixture did not resolve XAUUSD");
    Gate7FullSymbol bad = full();
    bad.symbolName = "XAU/USD";
    require(contradictory.acceptFullSymbol(fullEvidence(std::move(bad)))
                == Gate7Decision::SymbolMetadataRejected,
            "contradictory full/light symbol metadata accepted");
}

void test_integer_price_conversion_and_timestamp_proof()
{
    require(CTraderGate7Proof::normalizeSignedScale5(123455, 4).value_or(0)
                == 12346,
            "positive midpoint was not ties-away-from-zero");
    require(CTraderGate7Proof::normalizeSignedScale5(-123455, 4).value_or(0)
                == -12346,
            "negative midpoint was not ties-away-from-zero");
    require(CTraderGate7Proof::normalizeSignedScale5(123456, 6).value_or(0)
                == 1234560,
            "scale expansion was not checked and exact");
    require(!CTraderGate7Proof::normalizeSpotPrice(
                std::numeric_limits<std::uint64_t>::max(), 5).has_value(),
            "oversized raw price accepted");
    require(!CTraderGate7Proof::normalizeSpotPrice(
                std::numeric_limits<std::uint64_t>::max() / 2, 12).has_value(),
            "scale-expansion overflow accepted");

    const auto seconds = CTraderGate7Proof::classifyTimestamp(
        static_cast<std::uint64_t>(TIMESTAMP_SECONDS), RECEIPT_NS);
    require(seconds.has_value()
                && seconds->unit == Gate7TimestampUnit::Seconds
                && seconds->freshnessDeltaNs == 0,
            "fresh seconds timestamp was not uniquely proven");
    require(!CTraderGate7Proof::classifyTimestamp(
                static_cast<std::uint64_t>(TIMESTAMP_SECONDS - 121), RECEIPT_NS)
                .has_value(),
            "stale timestamp accepted");
    require(CTraderGate7Proof::classifyTimestampDetailed(
                static_cast<std::uint64_t>(TIMESTAMP_SECONDS - 121), RECEIPT_NS)
                .decision == Gate7Decision::TimestampStale,
            "stale timestamp did not retain its fixed category");
    require(!CTraderGate7Proof::classifyTimestamp(
                static_cast<std::uint64_t>(TIMESTAMP_SECONDS + 6), RECEIPT_NS)
                .has_value(),
            "future timestamp accepted");
    require(CTraderGate7Proof::classifyTimestampDetailed(
                static_cast<std::uint64_t>(TIMESTAMP_SECONDS + 6), RECEIPT_NS)
                .decision == Gate7Decision::TimestampFuture,
            "future timestamp did not retain its fixed category");
    require(!CTraderGate7Proof::classifyTimestamp(1, 1000000000ULL)
                .has_value(),
            "ambiguous timestamp unit accepted");
}

void test_spot_proof_and_terminal_clearing()
{
    CTraderGate7Proof proof(GENERATION);
    prepareForSpot(proof);
    require(proof.acceptSpot(spotEvidence()) == Gate7Decision::QuoteProofSucceeded,
            "fresh usable BBO was rejected");
    require(proof.isTerminal(), "successful quote proof was not terminal");
    require(!proof.accountIdForAuthentication().has_value(),
            "successful proof retained account ID");
    require(!proof.symbolIdForSubscription().has_value(),
            "successful proof retained symbol ID");
    require(proof.quoteEvidence().has_value()
                && proof.quoteEvidence()->canonicalSymbol == "XAUUSD"
                && proof.quoteEvidence()->executionAlias == "xau/usd"
                && proof.quoteEvidence()->bid.units == 23457
                && proof.quoteEvidence()->ask.units == 23457
                && proof.quoteEvidence()->spread.units == 0
                && proof.quoteEvidence()->instrument.complete,
            "sanitized normalized quote evidence was incomplete");

    CTraderGate7Proof partialSide(GENERATION);
    prepareForSpot(partialSide);
    require(partialSide.acceptSpot(spotEvidence(std::nullopt, 23456800))
                == Gate7Decision::IncompleteSpotSide,
            "partial provider spot event was not classified as incomplete");
    require(!partialSide.isTerminal(),
            "partial spot event terminally consumed the proof");
    require(partialSide.acceptSpot(spotEvidence())
                == Gate7Decision::QuoteProofSucceeded,
            "later single complete BBO was not accepted");

    CTraderGate7Proof partialTimestamp(GENERATION);
    prepareForSpot(partialTimestamp);
    require(partialTimestamp.acceptSpot(
                spotEvidence(23456789, 23456800, std::nullopt))
                == Gate7Decision::IncompleteSpotTimestamp,
            "timestamp-free event was not classified as incomplete");
    require(!partialTimestamp.isTerminal(),
            "timestamp-free event terminally consumed the proof");
    require(partialTimestamp.acceptSpot(spotEvidence())
                == Gate7Decision::QuoteProofSucceeded,
            "complete event after missing timestamp was not accepted");

    for (const auto& invalidSpot : {
             spotEvidence(0, 23456800),
             spotEvidence(std::numeric_limits<std::uint64_t>::max(), 23456800),
             spotEvidence(23456800, 23456789),
             spotEvidence(23456800, 23456789, std::nullopt)}) {
        CTraderGate7Proof rejected(GENERATION);
        prepareForSpot(rejected);
        const Gate7Decision decision = rejected.acceptSpot(invalidSpot);
        require(decision != Gate7Decision::QuoteProofSucceeded
                    && rejected.isTerminal(),
                "invalid spot proof did not fail closed");
        require(!rejected.accountIdForAuthentication().has_value()
                    && !rejected.symbolIdForSubscription().has_value(),
                "invalid spot proof retained provider identifiers");
    }

    CTraderGate7Proof mismatch(GENERATION);
    prepareForSpot(mismatch);
    auto wrong = spotEvidence();
    wrong.subscriptionMatched = false;
    require(mismatch.acceptSpot(std::move(wrong))
                == Gate7Decision::SubscriptionMismatch,
            "subscription mismatch accepted");

    CTraderGate7Proof wrongAccount(GENERATION);
    prepareForSpot(wrongAccount);
    auto accountMismatch = spotEvidence();
    accountMismatch.accountId = ACCOUNT + 1;
    require(wrongAccount.acceptSpot(std::move(accountMismatch))
                == Gate7Decision::SpotAccountMismatch,
            "spot account mismatch was not distinguished");

    CTraderGate7Proof wrongSymbol(GENERATION);
    prepareForSpot(wrongSymbol);
    auto symbolMismatch = spotEvidence();
    symbolMismatch.symbolId = SYMBOL + 1;
    require(wrongSymbol.acceptSpot(std::move(symbolMismatch))
                == Gate7Decision::SpotSymbolMismatch,
            "spot symbol mismatch was not distinguished");
}

void test_terminal_errors_and_sanitized_diagnostics()
{
    constexpr std::array<Gate7Decision, 36> decisions = {
        Gate7Decision::Ready,
        Gate7Decision::AccountAuthenticationReady,
        Gate7Decision::SymbolListReady,
        Gate7Decision::FullSymbolReady,
        Gate7Decision::SubscriptionReady,
        Gate7Decision::QuoteProofSucceeded,
        Gate7Decision::AlreadyTerminal,
        Gate7Decision::WrongPhase,
        Gate7Decision::StaleConnectionGeneration,
        Gate7Decision::CorrelationRejected,
        Gate7Decision::TokenOwnershipRejected,
        Gate7Decision::TradingScopeRequired,
        Gate7Decision::InvalidAccountIdentifier,
        Gate7Decision::NoFiboDemoAccount,
        Gate7Decision::AmbiguousFiboDemoAccount,
        Gate7Decision::MissingSymbolMetadata,
        Gate7Decision::NoCanonicalXauusd,
        Gate7Decision::AmbiguousCanonicalXauusd,
        Gate7Decision::FullSymbolMismatch,
        Gate7Decision::SymbolMetadataRejected,
        Gate7Decision::SubscriptionMismatch,
        Gate7Decision::IncompleteSpotSide,
        Gate7Decision::IncompleteSpotTimestamp,
        Gate7Decision::SpotAccountMismatch,
        Gate7Decision::SpotSymbolMismatch,
        Gate7Decision::InvalidSpot,
        Gate7Decision::CrossedMarket,
        Gate7Decision::CheckedArithmeticFailed,
        Gate7Decision::TimestampUnitUnproven,
        Gate7Decision::TimestampStale,
        Gate7Decision::TimestampFuture,
        Gate7Decision::ProviderError,
        Gate7Decision::Timeout,
        Gate7Decision::Cancelled,
        Gate7Decision::MalformedFrame,
        Gate7Decision::ResourceExhausted
    };
    for (const auto decision : decisions) {
        const std::string_view diagnostic = CTraderGate7Proof::safeDiagnostic(decision);
        require(!diagnostic.empty() && diagnostic.size() <= 64,
                "diagnostic was empty or unbounded");
        require(diagnostic.find_first_of("=?& \t\r\n")
                    == std::string_view::npos,
                "diagnostic contained value-like/provider text");
        require(diagnostic.find("FIBO") == std::string_view::npos,
                "diagnostic contained provider metadata");
    }
    require(CTraderGate7Proof::safeDiagnostic(
                Gate7Decision::TimestampUnitUnproven) == "timestamp_unit_unproven",
            "timestamp failure class was not canonical");

    for (const auto decision : {Gate7Decision::ProviderError,
                                Gate7Decision::Timeout,
                                Gate7Decision::Cancelled,
                                Gate7Decision::MalformedFrame,
                                Gate7Decision::ResourceExhausted}) {
        CTraderGate7Proof proof(GENERATION);
        require(proof.terminal(decision) == decision,
                "terminal failure was not preserved");
        require(proof.isTerminal()
                    && !proof.accountIdForAuthentication().has_value()
                    && !proof.symbolIdForSubscription().has_value(),
                "terminal failure retained volatile state");
    }

    constexpr std::array<Gate7ResidualFailure, 42> residualFailures = {
        Gate7ResidualFailure::None,
        Gate7ResidualFailure::SubscriptionStateUnavailable,
        Gate7ResidualFailure::SubscriptionSendFailed,
        Gate7ResidualFailure::SubscriptionResponseTimeout,
        Gate7ResidualFailure::SubscriptionTransportClosed,
        Gate7ResidualFailure::SubscriptionAccountRejected,
        Gate7ResidualFailure::SubscriptionTokenInvalidated,
        Gate7ResidualFailure::SubscriptionSymbolRejected,
        Gate7ResidualFailure::SubscriptionRateLimited,
        Gate7ResidualFailure::SubscriptionProviderUnavailable,
        Gate7ResidualFailure::SubscriptionProviderRejected,
        Gate7ResidualFailure::SubscriptionUnexpectedPayload,
        Gate7ResidualFailure::SubscriptionPriorStageResponse,
        Gate7ResidualFailure::SubscriptionUnrequestedUnsubscribeResponse,
        Gate7ResidualFailure::SubscriptionSpotBeforeAcknowledgement,
        Gate7ResidualFailure::SubscriptionSymbolChangedEvent,
        Gate7ResidualFailure::SubscriptionTraderUpdatedEvent,
        Gate7ResidualFailure::SubscriptionProhibitedAsyncEvent,
        Gate7ResidualFailure::SubscriptionOtherSchemaPayload,
        Gate7ResidualFailure::SubscriptionUnknownPayload,
        Gate7ResidualFailure::SubscriptionCorrelationRejected,
        Gate7ResidualFailure::SubscriptionResponseMalformed,
        Gate7ResidualFailure::SubscriptionAccountMismatch,
        Gate7ResidualFailure::SubscriptionProofRejected,
        Gate7ResidualFailure::SubscriptionResourceExhausted,
        Gate7ResidualFailure::SpotResponseTimeout,
        Gate7ResidualFailure::SpotTransportClosed,
        Gate7ResidualFailure::SpotAccountRejected,
        Gate7ResidualFailure::SpotTokenInvalidated,
        Gate7ResidualFailure::SpotSymbolRejected,
        Gate7ResidualFailure::SpotRateLimited,
        Gate7ResidualFailure::SpotProviderUnavailable,
        Gate7ResidualFailure::SpotProviderRejected,
        Gate7ResidualFailure::SpotUnexpectedPayload,
        Gate7ResidualFailure::SpotResponseMalformed,
        Gate7ResidualFailure::SpotAccountMismatch,
        Gate7ResidualFailure::SpotSymbolMismatch,
        Gate7ResidualFailure::SpotIncompleteSideTimeout,
        Gate7ResidualFailure::SpotTimestampMissingTimeout,
        Gate7ResidualFailure::SpotCompleteBboTimeout,
        Gate7ResidualFailure::SpotProofRejected,
        Gate7ResidualFailure::SpotResourceExhausted
    };
    for (const auto failure : residualFailures) {
        const std::string_view diagnostic =
            tradebot::ctrader::safeGate7ResidualDiagnostic(failure);
        require(!diagnostic.empty() && diagnostic.size() <= 64,
                "residual diagnostic was empty or unbounded");
        require(diagnostic.find_first_of("=?& \t\r\n")
                    == std::string_view::npos,
                "residual diagnostic contained value-like/provider text");
    }
}

} // namespace

int main()
{
    std::cerr << "[ctrader_gate7_tests] endpoint and allowlists...\n";
    test_endpoint_and_allowlist();
    std::cerr << "[ctrader_gate7_tests] OAuth diagnostics...\n";
    test_oauth_diagnostics_are_fixed_and_complete();
    std::cerr << "[ctrader_gate7_tests] OAuth callback read hardening...\n";
    test_oauth_callback_read_hardening();
    std::cerr << "[ctrader_gate7_tests] residual transport outcomes...\n";
    test_residual_transport_and_provider_classification();
    std::cerr << "[ctrader_gate7_tests] heartbeat cadence...\n";
    test_heartbeat_cadence_is_bounded();
    std::cerr << "[ctrader_gate7_tests] canonical symbol rule...\n";
    test_canonical_symbol_rule();
    std::cerr << "[ctrader_gate7_tests] account selection...\n";
    test_account_selection_is_fresh_exact_and_fail_closed();
    std::cerr << "[ctrader_gate7_tests] generation and ordering...\n";
    test_generation_correlation_and_ordering();
    std::cerr << "[ctrader_gate7_tests] symbol metadata...\n";
    test_symbol_resolution_and_metadata_contract();
    std::cerr << "[ctrader_gate7_tests] numeric/timestamp contract...\n";
    test_integer_price_conversion_and_timestamp_proof();
    std::cerr << "[ctrader_gate7_tests] spot proof and cleanup...\n";
    test_spot_proof_and_terminal_clearing();
    std::cerr << "[ctrader_gate7_tests] terminal diagnostics...\n";
    test_terminal_errors_and_sanitized_diagnostics();
    std::cout << "[ctrader_gate7_tests] All tests passed.\n";
    return 0;
}
