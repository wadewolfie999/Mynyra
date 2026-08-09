#include "CTraderGate6Proof.hpp"
#include "CTraderGate6Runtime.hpp"
#include "CTraderOAuthCorrelation.hpp"
#include "OpenApiCommonModelMessages.pb.h"
#include "OpenApiModelMessages.pb.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <new>
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

using tradebot::ctrader::CTraderGate6AccountProof;
using tradebot::ctrader::CTraderGate6Config;
using tradebot::ctrader::Gate6AccountListEvidence;
using tradebot::ctrader::Gate6AccountRecord;
using tradebot::ctrader::Gate6Decision;
using tradebot::ctrader::SensitiveString;
using tradebot::ctrader::validateCTraderTokenResponseOffline;
using tradebot::ctrader::validateCTraderTokenTransportConfigurationOffline;
using tradebot::ctrader::validateCTraderKeychainCopyBoundaryOffline;

constexpr uint64_t TEST_ONLY_ACCOUNT_A = 101;
constexpr uint64_t TEST_ONLY_ACCOUNT_B = 202;

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

Gate6AccountRecord account(uint64_t id,
                           std::optional<bool> isLive,
                           std::optional<std::string> broker)
{
    return {id, isLive, std::move(broker)};
}

Gate6AccountListEvidence evidence(std::vector<Gate6AccountRecord> accounts)
{
    return {true, true, true, true, std::move(accounts)};
}

void test_fixed_demo_boundary_and_payload_allowlist()
{
    static_assert(HEARTBEAT_EVENT == 51);
    static_assert(PROTO_OA_APPLICATION_AUTH_REQ == 2100);
    static_assert(PROTO_OA_ACCOUNT_AUTH_REQ == 2102);
    static_assert(PROTO_OA_GET_ACCOUNTS_BY_ACCESS_TOKEN_REQ == 2149);
    require(CTraderGate6Config::isAllowedOpenApiEndpoint(
                CTraderGate6Config::DEMO_HOST, CTraderGate6Config::DEMO_PORT),
            "fixed demo endpoint was rejected");
    require(!CTraderGate6Config::isAllowedOpenApiEndpoint(
                "LIVE_ENDPOINT_FORBIDDEN", CTraderGate6Config::DEMO_PORT),
            "forbidden endpoint input was accepted");
    require(!CTraderGate6Config::isAllowedOpenApiEndpoint(
                CTraderGate6Config::DEMO_HOST, 5036),
            "JSON port was accepted");
    require(CTraderGate6Config::OAUTH_SCOPE == "trading",
            "authorized Gate 6 scope changed");
    require(CTraderGate6Config::TOKEN_SERVICE
                == "TradeBot.cTraderOpenApi.tokens.trading",
            "trading token service changed");
    require(validateCTraderTokenTransportConfigurationOffline(),
            "token transport options were rejected locally");

    require(CTraderGate6Config::isAllowedOutboundPayload(51),
            "heartbeat was rejected");
    require(CTraderGate6Config::isAllowedOutboundPayload(2100),
            "application auth was rejected");
    require(CTraderGate6Config::isAllowedOutboundPayload(2149),
            "account list was rejected");
    require(CTraderGate6Config::isAllowedOutboundPayload(2102),
            "account auth was rejected");

    constexpr std::array<uint32_t, 10> prohibited = {
        2106, 2108, 2109, 2111, 2114, 2121, 2124, 2127, 2133, 2167
    };
    for (const uint32_t payload : prohibited) {
        require(!CTraderGate6Config::isAllowedOutboundPayload(payload),
                "market/order payload passed the Gate 6 allowlist");
    }
}

void test_token_response_parser_fails_closed()
{
    constexpr std::string_view valid =
        R"({"accessToken":"TEST_ONLY_ACCESS","tokenType":"bearer","expiresIn":2628000,"refreshToken":"TEST_ONLY_REFRESH","errorCode":null,"description":null})";
    require(validateCTraderTokenResponseOffline(valid),
            "synthetic valid token response was rejected");

    constexpr std::array<std::string_view, 8> rejected = {
        R"({"accessToken":"TEST_ONLY_ACCESS","accessToken":"TEST_ONLY_DUPLICATE","tokenType":"bearer","expiresIn":1,"refreshToken":"TEST_ONLY_REFRESH","errorCode":null})",
        R"({"accessToken":"TEST_ONLY_ACCESS","tokenType":"Bearer","expiresIn":1,"refreshToken":"TEST_ONLY_REFRESH","errorCode":null})",
        R"({"accessToken":"TEST_ONLY_ACCESS","tokenType":"bearer","expiresIn":0,"refreshToken":"TEST_ONLY_REFRESH","errorCode":null})",
        R"({"accessToken":"TEST_ONLY_ACCESS","tokenType":"bearer","expiresIn":1,"refreshToken":"TEST_ONLY_REFRESH","errorCode":"TEST_ONLY_ERROR"})",
        R"({"accessToken":"TEST_ONLY_ACCESS","tokenType":"bearer","expiresIn":1,"errorCode":null})",
        R"({"accessToken":"TEST_ONLY_ACCESS","tokenType":"bearer","expiresIn":1,"refreshToken":"TEST_ONLY_REFRESH"})",
        R"({"accessToken":"TEST_ONLY\nCONTROL","tokenType":"bearer","expiresIn":1,"refreshToken":"TEST_ONLY_REFRESH","errorCode":null})",
        R"({"accessToken":"TEST_ONLY_ACCESS","tokenType":"bearer","expiresIn":1,"refreshToken":"TEST_ONLY_REFRESH","errorCode":null)"
    };
    for (const std::string_view response : rejected) {
        require(!validateCTraderTokenResponseOffline(response),
                "unsafe synthetic token response was accepted");
    }
}

void test_keychain_copy_owns_and_clears_only_mutable_bytes()
{
    const std::string immutableFixture = "TEST_ONLY_KEYCHAIN_VALUE";
    const std::string snapshot = immutableFixture;
    require(validateCTraderKeychainCopyBoundaryOffline(immutableFixture),
            "immutable Keychain copy boundary rejected synthetic bytes");
    require(immutableFixture == snapshot,
            "immutable source bytes were modified");
    require(!validateCTraderKeychainCopyBoundaryOffline({}),
            "empty Keychain bytes were accepted");
}

void test_allocation_failures_are_terminal_and_sanitized()
{
    CTraderOAuthCorrelationGuard correlation;
    failNextAllocation = true;
    const bool armed = correlation.arm(
        {CTraderOAuthCorrelationGuard::LOOPBACK_ADDRESS,
         CTraderOAuthCorrelationGuard::LOOPBACK_PORT},
        CTraderOAuthCorrelationGuard::Clock::now());
    failNextAllocation = false;
    require(!armed && correlation.isTerminal(),
            "correlation allocation failure was not terminal");
    require(correlation.stateForAuthorizationRequest().empty(),
            "correlation allocation failure retained state");
    require(correlation.lastDecision()
                == CTraderOAuthCorrelationGuard::Decision::EntropyUnavailable,
            "correlation allocation failure was not sanitized");

    CTraderGate6AccountProof proof;
    Gate6AccountListEvidence input = evidence({
        account(TEST_ONLY_ACCOUNT_A, false, "TEST_ONLY_FIBO")
    });
    failNextAllocation = true;
    const Gate6Decision decision = proof.acceptGate6A(std::move(input));
    failNextAllocation = false;
    require(decision == Gate6Decision::ResourceExhausted,
            "state-machine allocation failure was not sanitized");
    require(proof.isTerminal(),
            "state-machine allocation failure was not terminal");
    require(proof.safeCandidates().empty(),
            "allocation failure retained checkpoint state");
    require(!proof.accountIdForAuthentication().has_value(),
            "allocation failure retained an account identifier");
    require(CTraderGate6AccountProof::safeDiagnostic(decision)
                == "gate6_resource_exhausted",
            "allocation failure diagnostic was not fixed");

    constexpr std::string_view validTokenFixture =
        R"({"accessToken":"TEST_ONLY_ACCESS","tokenType":"bearer","expiresIn":2628000,"refreshToken":"TEST_ONLY_REFRESH","errorCode":null,"description":null})";
    failNextAllocation = true;
    const bool parsed = validateCTraderTokenResponseOffline(validTokenFixture);
    failNextAllocation = false;
    require(!parsed,
            "token parser allocation failure did not fail closed");
}

void test_gate6a_safe_candidates_and_checkpoint()
{
    CTraderGate6AccountProof proof;
    std::vector<Gate6AccountRecord> accounts;
    accounts.push_back(account(TEST_ONLY_ACCOUNT_A, false, "TEST_ONLY_BROKER_A"));
    accounts.push_back(account(TEST_ONLY_ACCOUNT_B, false, "TEST_ONLY_BROKER_B"));
    accounts.push_back(account(303, true, "TEST_ONLY_LIVE_BROKER"));
    accounts.push_back(account(404, std::nullopt, "TEST_ONLY_UNKNOWN_MODE"));
    accounts.push_back(account(505, false, std::nullopt));

    require(proof.acceptGate6A(evidence(std::move(accounts)))
                == Gate6Decision::AwaitingWadeCheckpoint,
            "Gate 6A did not reach the checkpoint");
    require(proof.isAwaitingWadeCheckpoint(),
            "checkpoint state was not retained");
    require(proof.safeCandidates().size() == 2,
            "unsafe/ineligible account entered safe candidates");
    require(!proof.safeCandidates()[0].isLive
                && proof.safeCandidates()[0].brokerTitleShort
                    == "TEST_ONLY_BROKER_A",
            "first safe candidate changed");
    require(!proof.safeCandidates()[1].isLive
                && proof.safeCandidates()[1].brokerTitleShort
                    == "TEST_ONLY_BROKER_B",
            "second safe candidate changed");
    require(!proof.accountIdForAuthentication().has_value(),
            "account auth became possible before Wade confirmation");

    require(proof.confirmWadeSelection("TEST_ONLY_BROKER_B")
                == Gate6Decision::ReadyForGate6B,
            "exact Wade selection was rejected");
    require(proof.safeCandidates().empty(),
            "Gate 6A safe candidates survived confirmation");
    require(!proof.accountIdForAuthentication().has_value(),
            "Gate 6A identifier was reused for account auth");
}

void test_gate6a_rejects_ambiguous_and_unsafe_metadata()
{
    CTraderGate6AccountProof duplicate;
    require(duplicate.acceptGate6A(evidence({
                account(TEST_ONLY_ACCOUNT_A, false, "TEST_ONLY_DUPLICATE"),
                account(TEST_ONLY_ACCOUNT_B, false, "TEST_ONLY_DUPLICATE")
            })) == Gate6Decision::AmbiguousDemoAccount,
            "indistinguishable demo accounts were accepted");
    require(duplicate.isTerminal(), "ambiguity was not terminal");
    require(duplicate.safeCandidates().empty(),
            "ambiguity retained safe candidates");

    CTraderGate6AccountProof unsafe;
    require(unsafe.acceptGate6A(evidence({
                account(TEST_ONLY_ACCOUNT_A, false, "TEST_ONLY\nINJECTION")
            })) == Gate6Decision::UnsafeBrokerMetadata,
            "control character in broker metadata was accepted");

    CTraderGate6AccountProof invalidId;
    require(invalidId.acceptGate6A(evidence({
                account(0, false, "TEST_ONLY_BROKER")
            })) == Gate6Decision::InvalidAccountIdentifier,
            "zero account identifier was accepted");

    CTraderGate6AccountProof noEligible;
    require(noEligible.acceptGate6A(evidence({
                account(TEST_ONLY_ACCOUNT_A, true, "TEST_ONLY_LIVE"),
                account(TEST_ONLY_ACCOUNT_B, std::nullopt, "TEST_ONLY_UNKNOWN")
            })) == Gate6Decision::NoEligibleDemoAccount,
            "missing demo account did not fail closed");
}

void test_evidence_ownership_and_scope_fail_closed()
{
    struct Case {
        Gate6AccountListEvidence evidence;
        Gate6Decision expected;
    };
    std::vector<Case> cases;
    cases.push_back({{false, true, true, true, {}},
                     Gate6Decision::StaleConnectionGeneration});
    cases.push_back({{true, false, true, true, {}},
                     Gate6Decision::CorrelationRejected});
    cases.push_back({{true, true, false, true, {}},
                     Gate6Decision::TokenOwnershipRejected});
    cases.push_back({{true, true, true, false, {}},
                     Gate6Decision::TradingScopeRequired});

    for (Case& testCase : cases) {
        CTraderGate6AccountProof proof;
        require(proof.acceptGate6A(std::move(testCase.evidence))
                    == testCase.expected,
                "account-list evidence failure category mismatch");
        require(proof.isTerminal(), "evidence rejection was not terminal");
    }
}

void test_wade_selection_is_exact_and_single_use()
{
    CTraderGate6AccountProof proof;
    require(proof.acceptGate6A(evidence({
                account(TEST_ONLY_ACCOUNT_A, false, "TEST_ONLY_FIBO")
            })) == Gate6Decision::AwaitingWadeCheckpoint,
            "checkpoint setup failed");
    require(proof.confirmWadeSelection("test_only_fibo")
                == Gate6Decision::WadeSelectionRejected,
            "case-folded Wade selection was accepted");
    require(proof.isTerminal(), "rejected selection was not terminal");
    require(proof.confirmWadeSelection("TEST_ONLY_FIBO")
                == Gate6Decision::AlreadyTerminal,
            "selection retry was not rejected");
}

void test_gate6b_fresh_match_and_account_auth_clear_state()
{
    CTraderGate6AccountProof proof;
    require(proof.acceptGate6A(evidence({
                account(TEST_ONLY_ACCOUNT_A, false, "TEST_ONLY_FIBO")
            })) == Gate6Decision::AwaitingWadeCheckpoint,
            "Gate 6A setup failed");
    require(proof.confirmWadeSelection("TEST_ONLY_FIBO")
                == Gate6Decision::ReadyForGate6B,
            "Wade checkpoint failed");

    require(proof.acceptGate6B(evidence({
                account(TEST_ONLY_ACCOUNT_B, false, "TEST_ONLY_FIBO"),
                account(303, true, "TEST_ONLY_FIBO")
            })) == Gate6Decision::ReadyForAccountAuthentication,
            "fresh Gate 6B match failed");
    const std::optional<int64_t> id = proof.accountIdForAuthentication();
    require(id.has_value() && *id == static_cast<int64_t>(TEST_ONLY_ACCOUNT_B),
            "fresh response-derived identifier was not selected");
    require(proof.acceptAccountAuthentication(*id)
                == Gate6Decision::AccountProofSucceeded,
            "matching account-auth response was rejected");
    require(proof.isTerminal(), "successful proof was not terminal");
    require(!proof.accountIdForAuthentication().has_value(),
            "successful proof retained the account identifier");
}

void test_gate6b_zero_multiple_and_mismatch_fail_closed()
{
    auto readyForGate6B = []() {
        auto proof = std::make_unique<CTraderGate6AccountProof>();
        require(proof->acceptGate6A(evidence({
                    account(TEST_ONLY_ACCOUNT_A, false, "TEST_ONLY_FIBO")
                })) == Gate6Decision::AwaitingWadeCheckpoint,
                "Gate 6A setup failed");
        require(proof->confirmWadeSelection("TEST_ONLY_FIBO")
                    == Gate6Decision::ReadyForGate6B,
                "checkpoint setup failed");
        return proof;
    };

    auto zero = readyForGate6B();
    require(zero->acceptGate6B(evidence({
                account(TEST_ONLY_ACCOUNT_B, false, "TEST_ONLY_OTHER")
            })) == Gate6Decision::NoEligibleDemoAccount,
            "zero Gate 6B matches did not fail");

    auto multiple = readyForGate6B();
    require(multiple->acceptGate6B(evidence({
                account(TEST_ONLY_ACCOUNT_A, false, "TEST_ONLY_FIBO"),
                account(TEST_ONLY_ACCOUNT_B, false, "TEST_ONLY_FIBO")
            })) == Gate6Decision::AmbiguousDemoAccount,
            "multiple Gate 6B matches did not fail");

    auto mismatch = readyForGate6B();
    require(mismatch->acceptGate6B(evidence({
                account(TEST_ONLY_ACCOUNT_B, false, "TEST_ONLY_FIBO")
            })) == Gate6Decision::ReadyForAccountAuthentication,
            "Gate 6B setup failed");
    require(mismatch->acceptAccountAuthentication(
                static_cast<int64_t>(TEST_ONLY_ACCOUNT_A))
                == Gate6Decision::AccountAuthenticationMismatch,
            "account-auth response mismatch was accepted");
    require(!mismatch->accountIdForAuthentication().has_value(),
            "mismatch retained the account identifier");
}

void test_cancel_and_sensitive_string_clear()
{
    SensitiveString secret(std::string("TEST_ONLY_INVALID_TOKEN"));
    require(!secret.empty(), "synthetic sensitive value missing");
    SensitiveString moved(std::move(secret));
    require(secret.empty(), "moved-from sensitive value was retained");
    moved.clear();
    require(moved.empty(), "sensitive value did not clear");

    CTraderGate6AccountProof proof;
    require(proof.acceptGate6A(evidence({
                account(TEST_ONLY_ACCOUNT_A, false, "TEST_ONLY_FIBO")
            })) == Gate6Decision::AwaitingWadeCheckpoint,
            "checkpoint setup failed");
    require(proof.cancel() == Gate6Decision::Cancelled,
            "cancellation returned wrong category");
    require(proof.isTerminal(), "cancellation was not terminal");
    require(proof.safeCandidates().empty(),
            "cancellation retained safe metadata");
    require(!proof.accountIdForAuthentication().has_value(),
            "cancellation retained account identifier");
}

void test_diagnostics_are_fixed_and_redacted()
{
    constexpr std::array<Gate6Decision, 21> decisions = {
        Gate6Decision::Ready,
        Gate6Decision::AwaitingWadeCheckpoint,
        Gate6Decision::ReadyForGate6B,
        Gate6Decision::ReadyForAccountAuthentication,
        Gate6Decision::AccountProofSucceeded,
        Gate6Decision::Cancelled,
        Gate6Decision::AlreadyTerminal,
        Gate6Decision::WrongPhase,
        Gate6Decision::StaleConnectionGeneration,
        Gate6Decision::CorrelationRejected,
        Gate6Decision::TokenOwnershipRejected,
        Gate6Decision::TradingScopeRequired,
        Gate6Decision::LiveAccountExcluded,
        Gate6Decision::MissingAccountMetadata,
        Gate6Decision::UnsafeBrokerMetadata,
        Gate6Decision::InvalidAccountIdentifier,
        Gate6Decision::NoEligibleDemoAccount,
        Gate6Decision::AmbiguousDemoAccount,
        Gate6Decision::WadeSelectionRejected,
        Gate6Decision::AccountAuthenticationMismatch,
        Gate6Decision::ResourceExhausted
    };
    for (const Gate6Decision decision : decisions) {
        const std::string_view diagnostic =
            CTraderGate6AccountProof::safeDiagnostic(decision);
        require(!diagnostic.empty() && diagnostic.size() <= 64,
                "diagnostic was empty or unbounded");
        require(diagnostic.find("TEST_ONLY") == std::string_view::npos,
                "diagnostic exposed fixture material");
        require(diagnostic.find('=') == std::string_view::npos,
                "diagnostic resembled a value pair");
    }
}

} // namespace

int main()
{
    std::cerr << "[ctrader_gate6_tests] endpoint and payload allowlist...\n";
    test_fixed_demo_boundary_and_payload_allowlist();
    std::cerr << "[ctrader_gate6_tests] token-response boundary...\n";
    test_token_response_parser_fails_closed();
    std::cerr << "[ctrader_gate6_tests] Keychain copy ownership...\n";
    test_keychain_copy_owns_and_clears_only_mutable_bytes();
    std::cerr << "[ctrader_gate6_tests] allocation failure boundaries...\n";
    test_allocation_failures_are_terminal_and_sanitized();
    std::cerr << "[ctrader_gate6_tests] Gate 6A checkpoint...\n";
    test_gate6a_safe_candidates_and_checkpoint();
    std::cerr << "[ctrader_gate6_tests] ambiguity and metadata...\n";
    test_gate6a_rejects_ambiguous_and_unsafe_metadata();
    std::cerr << "[ctrader_gate6_tests] evidence ownership...\n";
    test_evidence_ownership_and_scope_fail_closed();
    std::cerr << "[ctrader_gate6_tests] exact Wade selection...\n";
    test_wade_selection_is_exact_and_single_use();
    std::cerr << "[ctrader_gate6_tests] fresh Gate 6B proof...\n";
    test_gate6b_fresh_match_and_account_auth_clear_state();
    std::cerr << "[ctrader_gate6_tests] Gate 6B failures...\n";
    test_gate6b_zero_multiple_and_mismatch_fail_closed();
    std::cerr << "[ctrader_gate6_tests] cancellation and clearing...\n";
    test_cancel_and_sensitive_string_clear();
    std::cerr << "[ctrader_gate6_tests] redacted diagnostics...\n";
    test_diagnostics_are_fixed_and_redacted();
    std::cout << "[ctrader_gate6_tests] All tests passed.\n";
    return 0;
}
