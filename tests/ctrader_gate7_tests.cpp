#include "CTraderGate7Proof.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using tradebot::ctrader::CTraderGate7Config;
using tradebot::ctrader::CTraderGate7Proof;
using tradebot::ctrader::Gate7AccountListEvidence;
using tradebot::ctrader::Gate7AccountRecord;
using tradebot::ctrader::Gate7Decision;
using tradebot::ctrader::Gate7FullSymbol;
using tradebot::ctrader::Gate7FullSymbolEvidence;
using tradebot::ctrader::Gate7LightSymbol;
using tradebot::ctrader::Gate7SpotEvidence;
using tradebot::ctrader::Gate7SubscriptionEvidence;
using tradebot::ctrader::Gate7SymbolsListEvidence;
using tradebot::ctrader::Gate7TimestampUnit;

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
    require(!CTraderGate7Config::isAllowedInboundPayload(2126),
            "execution event admitted inbound");
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
    require(!CTraderGate7Proof::classifyTimestamp(
                static_cast<std::uint64_t>(TIMESTAMP_SECONDS + 6), RECEIPT_NS)
                .has_value(),
            "future timestamp accepted");
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

    for (const auto& invalidSpot : {
             spotEvidence(std::nullopt, 23456800),
             spotEvidence(0, 23456800),
             spotEvidence(std::numeric_limits<std::uint64_t>::max(), 23456800),
             spotEvidence(23456800, 23456789),
             spotEvidence(23456789, 23456800, std::nullopt)}) {
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
}

void test_terminal_errors_and_sanitized_diagnostics()
{
    constexpr std::array<Gate7Decision, 33> decisions = {
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
        Gate7Decision::MissingSpotSide,
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
        require(diagnostic.find('=') == std::string_view::npos,
                "diagnostic contained a value-like assignment");
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
}

} // namespace

int main()
{
    std::cerr << "[ctrader_gate7_tests] endpoint and allowlists...\n";
    test_endpoint_and_allowlist();
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
