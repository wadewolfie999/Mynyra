#include "CTraderOAuthCorrelation.hpp"

#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

class CTraderOAuthCorrelationTestAccess {
public:
    static bool arm(CTraderOAuthCorrelationGuard& guard,
                    CTraderOAuthCorrelationGuard::ListenerBinding binding,
                    CTraderOAuthCorrelationGuard::TimePoint now,
                    const std::array<
                        uint8_t,
                        CTraderOAuthCorrelationGuard::ENTROPY_BYTES>& entropy)
        noexcept
    {
        return guard.armWithEntropy(binding, now, entropy);
    }
};

namespace {

using Guard = CTraderOAuthCorrelationGuard;
using Decision = Guard::Decision;
constexpr std::string_view TEST_ONLY_EXPECTED_STATE =
    "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8";

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::array<uint8_t, Guard::ENTROPY_BYTES> syntheticEntropy(uint8_t offset = 0)
{
    std::array<uint8_t, Guard::ENTROPY_BYTES> bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<uint8_t>(offset + i);
    }
    return bytes;
}

Guard::ListenerBinding validBinding()
{
    return {Guard::LOOPBACK_ADDRESS, Guard::LOOPBACK_PORT};
}

Guard::CallbackRequest validCallback(std::string_view query)
{
    return {Guard::LOOPBACK_ADDRESS, "GET", Guard::CALLBACK_HOST,
            Guard::CALLBACK_PATH, query};
}

Guard armedGuard(Guard::TimePoint now, uint8_t entropyOffset = 0)
{
    Guard guard;
    require(CTraderOAuthCorrelationTestAccess::arm(
                guard, validBinding(), now, syntheticEntropy(entropyOffset)),
            "synthetic guard failed to arm");
    return guard;
}

std::string matchingQuery(const Guard& guard)
{
    return "code=TEST_ONLY_SYNTHETIC_CODE&state="
         + std::string(guard.stateForAuthorizationRequest());
}

void test_secure_generation_and_fixed_binding()
{
    const auto now = Guard::Clock::now();

    Guard production;
    require(production.arm(validBinding(), now),
            "operating-system secure entropy failed");
    const std::string_view state = production.stateForAuthorizationRequest();
    require(state.size() == 43, "256-bit base64url state must be 43 bytes");
    require(state.find_first_not_of(
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_")
                == std::string_view::npos,
            "state contained a non-base64url character");

    Guard deterministic = armedGuard(now);
    require(deterministic.stateForAuthorizationRequest()
                == TEST_ONLY_EXPECTED_STATE,
            "synthetic entropy did not produce the expected base64url state");
    Guard different = armedGuard(now, 1);
    require(different.stateForAuthorizationRequest()
                != deterministic.stateForAuthorizationRequest(),
            "distinct synthetic entropy produced the same state");

    Guard wrongAddress;
    require(!CTraderOAuthCorrelationTestAccess::arm(
                wrongAddress, {"0.0.0.0", Guard::LOOPBACK_PORT}, now,
                syntheticEntropy()),
            "non-loopback listener binding was accepted");
    require(wrongAddress.lastDecision() == Decision::ListenerBindingRejected,
            "wrong binding failure category");
    require(wrongAddress.stateForAuthorizationRequest().empty(),
            "rejected binding retained state");

    Guard wrongPort;
    require(!CTraderOAuthCorrelationTestAccess::arm(
                wrongPort, {Guard::LOOPBACK_ADDRESS, 18081}, now,
                syntheticEntropy()),
            "unexpected listener port was accepted");

    Guard overflow;
    require(!CTraderOAuthCorrelationTestAccess::arm(
                overflow, validBinding(), Guard::TimePoint::max(),
                syntheticEntropy()),
            "overflowing expiry deadline was accepted");
    require(overflow.isTerminal(), "expiry-overflow rejection was not terminal");
}

void test_exact_match_discards_code_and_rejects_replay()
{
    const auto now = Guard::Clock::now();
    Guard guard = armedGuard(now);
    const std::string state(guard.stateForAuthorizationRequest());
    const std::string query = matchingQuery(guard);

    const Decision accepted = guard.consume(validCallback(query),
                                             now + std::chrono::seconds(1));
    require(accepted == Decision::CorrelationMatchedCodeDiscarded,
            "matching callback was not accepted");
    require(guard.isTerminal(), "matching callback did not close the guard");
    require(guard.stateForAuthorizationRequest().empty(),
            "matching callback retained correlation state");

    const std::string_view diagnostic = Guard::safeDiagnostic(accepted);
    require(diagnostic.find(state) == std::string_view::npos,
            "diagnostic exposed correlation state");
    require(diagnostic.find("TEST_ONLY_SYNTHETIC_CODE") == std::string_view::npos,
            "diagnostic exposed authorization code");

    require(guard.consume(validCallback(query), now + std::chrono::seconds(2))
                == Decision::AlreadyTerminal,
            "callback replay was not rejected");
}

void test_mismatch_and_expiry_are_terminal()
{
    const auto now = Guard::Clock::now();

    Guard mismatch = armedGuard(now, 1);
    require(mismatch.consume(
                validCallback("code=TEST_ONLY_SYNTHETIC_CODE&state=WRONG_STATE"),
                now + std::chrono::seconds(1)) == Decision::StateMismatch,
            "mismatched state was not rejected");
    require(mismatch.isTerminal(), "state mismatch was not terminal");
    require(mismatch.stateForAuthorizationRequest().empty(),
            "state mismatch retained correlation state");
    require(mismatch.consume(validCallback("code=X&state=Y"),
                             now + std::chrono::seconds(2))
                == Decision::AlreadyTerminal,
            "mismatch retry was not rejected as replay");

    Guard expired = armedGuard(now, 2);
    const std::string query = matchingQuery(expired);
    require(expired.consume(validCallback(query),
                            now + Guard::CORRELATION_LIFETIME)
                == Decision::CallbackExpired,
            "callback at expiry boundary was not rejected");
    require(expired.stateForAuthorizationRequest().empty(),
            "expired guard retained correlation state");
}

void test_callback_binding_and_early_rejection()
{
    const auto now = Guard::Clock::now();
    Guard unarmed;
    require(unarmed.consume(validCallback("code=X&state=Y"), now)
                == Decision::CallbackBeforeArming,
            "callback before arming was not rejected");
    require(unarmed.isTerminal(), "early callback was not terminal");
    require(!CTraderOAuthCorrelationTestAccess::arm(
                unarmed, validBinding(), now, syntheticEntropy()),
            "guard was armed after an early callback");

    Guard rearm = armedGuard(now, 9);
    require(!CTraderOAuthCorrelationTestAccess::arm(
                rearm, validBinding(), now, syntheticEntropy(10)),
            "armed guard accepted a replacement correlation value");
    require(rearm.isTerminal(), "rearm attempt was not terminal");
    require(rearm.stateForAuthorizationRequest().empty(),
            "rearm attempt retained correlation state");

    struct Case {
        Guard::CallbackRequest request;
        Decision expected;
    };

    const std::string placeholderQuery = "code=X&state=Y";
    const std::vector<Case> cases = {
        {{"127.0.0.2", "GET", Guard::CALLBACK_HOST, Guard::CALLBACK_PATH,
          placeholderQuery}, Decision::UnexpectedRemote},
        {{Guard::LOOPBACK_ADDRESS, "POST", Guard::CALLBACK_HOST,
          Guard::CALLBACK_PATH, placeholderQuery}, Decision::UnexpectedMethod},
        {{Guard::LOOPBACK_ADDRESS, "GET", "localhost:18080",
          Guard::CALLBACK_PATH, placeholderQuery}, Decision::UnexpectedHost},
        {{Guard::LOOPBACK_ADDRESS, "GET", Guard::CALLBACK_HOST,
          "/unexpected", placeholderQuery}, Decision::UnexpectedPath}
    };

    uint8_t entropyOffset = 10;
    for (const Case& testCase : cases) {
        Guard guard = armedGuard(now, entropyOffset++);
        require(guard.consume(testCase.request, now + std::chrono::seconds(1))
                    == testCase.expected,
                "callback binding rejection category mismatch");
        require(guard.isTerminal(), "binding rejection was not terminal");
    }
}

void test_malformed_duplicate_and_rejected_callbacks()
{
    const auto now = Guard::Clock::now();
    struct Case {
        std::string query;
        Decision expected;
    };
    const std::vector<Case> cases = {
        {"", Decision::MalformedQuery},
        {"code=X&code=Y&state=Z", Decision::DuplicateParameter},
        {"code=X&state=Y&state=Z", Decision::DuplicateParameter},
        {"code&state=Z", Decision::MalformedQuery},
        {"code=X&&state=Z", Decision::MalformedQuery},
        {"code=%GG&state=Z", Decision::MalformedQuery},
        {"code=&state=Z", Decision::CodeMissing},
        {"code=X", Decision::StateMissing},
        {"state=Z", Decision::CodeMissing},
        {"error=access_denied&state=Z", Decision::AuthorizationRejected},
        {"error=access_denied&code=X&state=Z", Decision::MalformedQuery},
        {"x=1&x=2&code=X&state=Z", Decision::DuplicateParameter}
    };

    uint8_t entropyOffset = 30;
    for (const Case& testCase : cases) {
        Guard guard = armedGuard(now, entropyOffset++);
        require(guard.consume(validCallback(testCase.query),
                              now + std::chrono::seconds(1))
                    == testCase.expected,
                "malformed/rejected callback category mismatch");
        require(guard.isTerminal(), "rejected callback was not terminal");
        require(guard.stateForAuthorizationRequest().empty(),
                "rejected callback retained correlation state");
    }
}

void test_all_diagnostics_are_bounded_and_redacted()
{
    constexpr std::array<Decision, 18> decisions = {
        Decision::Unarmed,
        Decision::Armed,
        Decision::ListenerBindingRejected,
        Decision::EntropyUnavailable,
        Decision::AlreadyTerminal,
        Decision::CallbackBeforeArming,
        Decision::CallbackExpired,
        Decision::UnexpectedRemote,
        Decision::UnexpectedMethod,
        Decision::UnexpectedHost,
        Decision::UnexpectedPath,
        Decision::MalformedQuery,
        Decision::DuplicateParameter,
        Decision::AuthorizationRejected,
        Decision::StateMissing,
        Decision::StateMismatch,
        Decision::CodeMissing,
        Decision::CorrelationMatchedCodeDiscarded
    };

    for (const Decision decision : decisions) {
        const std::string_view diagnostic = Guard::safeDiagnostic(decision);
        require(!diagnostic.empty(), "empty safe diagnostic");
        require(diagnostic.size() <= 64, "unbounded safe diagnostic");
        require(diagnostic.find("TEST_ONLY") == std::string_view::npos,
                "safe diagnostic contains fixture material");
        require(diagnostic.find('=') == std::string_view::npos,
                "safe diagnostic resembles a query/value pair");
    }
}

} // namespace

int main()
{
    std::cerr << "[ctrader_gate5_1_tests] secure generation and binding...\n";
    test_secure_generation_and_fixed_binding();
    std::cerr << "[ctrader_gate5_1_tests] match, discard, replay...\n";
    test_exact_match_discards_code_and_rejects_replay();
    std::cerr << "[ctrader_gate5_1_tests] mismatch and expiry...\n";
    test_mismatch_and_expiry_are_terminal();
    std::cerr << "[ctrader_gate5_1_tests] callback binding and early rejection...\n";
    test_callback_binding_and_early_rejection();
    std::cerr << "[ctrader_gate5_1_tests] malformed and duplicate rejection...\n";
    test_malformed_duplicate_and_rejected_callbacks();
    std::cerr << "[ctrader_gate5_1_tests] bounded redacted diagnostics...\n";
    test_all_diagnostics_are_bounded_and_redacted();
    std::cout << "[ctrader_gate5_1_tests] All tests passed.\n";
    return 0;
}
