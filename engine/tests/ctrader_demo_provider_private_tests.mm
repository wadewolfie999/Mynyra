#include "../src/providers/ctrader/CTraderProviderAdapterDemo.mm"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

namespace {

using namespace tradebot::ctrader;

void require(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

std::int64_t futureExpiry()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() + 3600;
}

Sensitive encodedToken(std::int64_t expiry)
{
    std::string bytes = "TBG6TOK1";
    appendUint64(bytes, static_cast<std::uint64_t>(expiry));
    appendSized(bytes, "trading");
    appendSized(bytes, "bearer");
    appendSized(bytes, "TEST_ACCESS_VALUE");
    appendSized(bytes, "TEST_REFRESH_VALUE");
    return Sensitive(std::move(bytes));
}

void setUsableToken(TokenEnvelope& token)
{
    clearToken(token);
    token.accessToken = Sensitive("TEST_NEW_ACCESS");
    token.refreshToken = Sensitive("TEST_NEW_REFRESH");
    token.tokenType = Sensitive("bearer");
    token.expiresAtEpochSeconds = futureExpiry();
    token.scope = "trading";
}

class FakeTokenServices final : public DemoTokenServices {
public:
    RuntimeFailure readStored(Sensitive& bytes) noexcept override
    {
        ++readCalls;
        if (readResult == RuntimeFailure::None) {
            bytes = Sensitive(std::string(stored.view()));
        }
        return readResult;
    }

    DemoDiagnostic exchange(std::string_view grantName,
                            std::string_view grantValue,
                            std::string_view clientId,
                            std::string_view clientSecret,
                            TokenEnvelope& output) noexcept override
    {
        ++exchangeCalls;
        lastGrant = std::string(grantName);
        sawNonEmptyGrant = !grantValue.empty();
        sawCredentials = !clientId.empty() && !clientSecret.empty();
        if (exchangeResult == DemoDiagnostic::None) setUsableToken(output);
        return exchangeResult;
    }

    DemoDiagnostic authorize(std::string_view clientId,
                             Sensitive& code) noexcept override
    {
        ++authorizeCalls;
        sawClientId = !clientId.empty();
        if (authorizeResult == DemoDiagnostic::None) {
            code = Sensitive("TEST_AUTHORIZATION_CODE");
        }
        return authorizeResult;
    }

    DemoDiagnostic persist(const TokenEnvelope& token) noexcept override
    {
        ++persistCalls;
        persistedUsableToken = tokenUsable(token);
        return persistResult;
    }

    RuntimeFailure readResult{RuntimeFailure::TokenUnavailable};
    DemoDiagnostic exchangeResult{DemoDiagnostic::None};
    DemoDiagnostic authorizeResult{DemoDiagnostic::None};
    DemoDiagnostic persistResult{DemoDiagnostic::None};
    Sensitive stored;
    int readCalls{0};
    int exchangeCalls{0};
    int authorizeCalls{0};
    int persistCalls{0};
    bool sawNonEmptyGrant{false};
    bool sawCredentials{false};
    bool sawClientId{false};
    bool persistedUsableToken{false};
    std::string lastGrant;
};

void testStoredAndRefreshFlows()
{
    FakeTokenServices valid;
    valid.readResult = RuntimeFailure::None;
    valid.stored = encodedToken(futureExpiry());
    TokenEnvelope output;
    bool persistenceRequired = true;
    require(acquireDemoTokenWithServices(
                false, "TEST_CLIENT", "TEST_SECRET", output, valid,
                persistenceRequired)
                == DemoDiagnostic::None,
            "valid stored token was rejected");
    require(tokenUsable(output) && valid.exchangeCalls == 0
                && valid.authorizeCalls == 0 && valid.persistCalls == 0
                && !persistenceRequired,
            "normal startup performed an unnecessary OAuth side effect");
    clearToken(output);

    FakeTokenServices refresh;
    refresh.readResult = RuntimeFailure::None;
    refresh.stored = encodedToken(futureExpiry() - 7200);
    require(acquireDemoTokenWithServices(
                false, "TEST_CLIENT", "TEST_SECRET", output, refresh,
                persistenceRequired)
                == DemoDiagnostic::None,
            "expired token did not refresh once");
    require(refresh.exchangeCalls == 1
                && refresh.lastGrant == "refresh_token"
                && refresh.sawNonEmptyGrant && refresh.sawCredentials
                && refresh.authorizeCalls == 0 && refresh.persistCalls == 0
                && persistenceRequired,
            "refresh flow did not defer replacement until validation");
    require(refresh.persist(output) == DemoDiagnostic::None
                && refresh.persistCalls == 1
                && refresh.persistedUsableToken,
            "validated refresh token did not persist exactly once");
    clearToken(output);

    FakeTokenServices invalidGrant;
    invalidGrant.readResult = RuntimeFailure::None;
    invalidGrant.stored = encodedToken(futureExpiry() - 7200);
    invalidGrant.exchangeResult = DemoDiagnostic::TokenInvalidGrant;
    require(acquireDemoTokenWithServices(
                false, "TEST_CLIENT", "TEST_SECRET", output, invalidGrant,
                persistenceRequired)
                == DemoDiagnostic::TokenInvalidGrant
                && invalidGrant.exchangeCalls == 1
                && invalidGrant.authorizeCalls == 0
                && invalidGrant.persistCalls == 0
                && !persistenceRequired,
            "refresh failure opened a browser or lost its typed cause");
}

void testFreshAndKeychainFailureFlows()
{
    TokenEnvelope output;
    FakeTokenServices fresh;
    bool persistenceRequired = false;
    require(acquireDemoTokenWithServices(
                true, "TEST_CLIENT", "TEST_SECRET", output, fresh,
                persistenceRequired)
                == DemoDiagnostic::None,
            "fresh OAuth flow failed");
    require(fresh.readCalls == 0 && fresh.authorizeCalls == 1
                && fresh.exchangeCalls == 1
                && fresh.lastGrant == "authorization_code"
                && fresh.persistCalls == 0 && persistenceRequired,
            "fresh OAuth did not deliberately bypass stored credentials");
    require(fresh.persist(output) == DemoDiagnostic::None
                && fresh.persistCalls == 1 && fresh.persistedUsableToken,
            "validated fresh token did not persist exactly once");
    clearToken(output);

    FakeTokenServices keychain;
    keychain.readResult = RuntimeFailure::KeychainRead;
    require(acquireDemoTokenWithServices(
                false, "TEST_CLIENT", "TEST_SECRET", output, keychain,
                persistenceRequired)
                == DemoDiagnostic::KeychainRead
                && keychain.exchangeCalls == 0
                && keychain.authorizeCalls == 0
                && !persistenceRequired,
            "Keychain failure did not fail closed without browser fallback");
}

void testTypedHttpClassification()
{
    DemoTokenHttpResult response;
    response.transport = CURLE_COULDNT_CONNECT;
    require(classifyTokenFailure(response) == DemoDiagnostic::TokenTransport,
            "transport failure classification changed");
    response.transport = CURLE_OK;
    response.status = 400;
    response.body = R"({"errorCode":"invalid_grant"})";
    require(classifyTokenFailure(response) == DemoDiagnostic::TokenInvalidGrant,
            "invalid grant classification changed");
    response.body = R"({"errorCode":"unauthorized_client"})";
    require(classifyTokenFailure(response) == DemoDiagnostic::TokenHttp4xx,
            "HTTP 4xx classification changed");
    response.status = 503;
    require(classifyTokenFailure(response) == DemoDiagnostic::TokenHttp5xx,
            "HTTP 5xx classification changed");
    response.status = 302;
    require(classifyTokenFailure(response) == DemoDiagnostic::TokenMalformed,
            "unexpected HTTP status classification changed");
}

void testProviderLifecycleBoundaryHelpers()
{
    std::optional<std::uint64_t> identity;
    require(mergeLocalIdentity(identity, 41)
                && mergeLocalIdentity(identity, 41)
                && !mergeLocalIdentity(identity, 42),
            "conflicting provider identities were accepted");
    require(providerFillKindMatches(ORDER_PARTIAL_FILL, 50, 100)
                && providerFillKindMatches(ORDER_FILLED, 100, 100)
                && !providerFillKindMatches(ORDER_FILLED, 50, 100)
                && !providerFillKindMatches(ORDER_PARTIAL_FILL, 100, 100),
            "provider fill label and quantity mismatch was accepted");
}

void testSubscriptionDiagnosticsAreLegSpecificAndRedacted()
{
    require(demoSubscriptionDiagnostic(
                DemoSubscriptionLeg::Spots,
                Gate7ResidualFailure::SubscriptionResponseTimeout)
                == "ctrader_demo_spots_subscription_response_timeout",
            "spot subscription timeout diagnostic lost its leg");
    require(demoSubscriptionDiagnostic(
                DemoSubscriptionLeg::LiveM1,
                Gate7ResidualFailure::SubscriptionSymbolRejected)
                == "ctrader_demo_live_m1_subscription_symbol_rejected",
            "live M1 provider diagnostic lost its leg or fixed category");
    require(subscriptionFailureCategory(
                Gate7ResidualFailure::SubscriptionTokenInvalidated)
                == FailureCategory::Authentication
                && subscriptionFailureCategory(
                    Gate7ResidualFailure::SubscriptionRateLimited)
                    == FailureCategory::RateLimited
                && subscriptionFailureCategory(
                    Gate7ResidualFailure::SubscriptionTransportClosed)
                    == FailureCategory::Transport,
            "subscription failure categories were collapsed");
    const std::string diagnostic = demoSubscriptionDiagnostic(
        DemoSubscriptionLeg::Spots,
        Gate7ResidualFailure::SubscriptionProviderRejected);
    require(diagnostic.find("account") == std::string::npos
                && diagnostic.find("token") == std::string::npos
                && diagnostic.find("http") == std::string::npos,
            "subscription diagnostic admitted value-shaped material");

    require(demoDiagnostic(DemoDiagnostic::SpotEnvelopeMalformed)
                == "ctrader_demo_spot_envelope_malformed"
                && demoDiagnostic(DemoDiagnostic::SpotAccountMismatch)
                    == "ctrader_demo_spot_account_mismatch"
                && demoDiagnostic(DemoDiagnostic::SpotSymbolMismatch)
                    == "ctrader_demo_spot_symbol_mismatch"
                && demoDiagnostic(DemoDiagnostic::SpotBidMalformed)
                    == "ctrader_demo_spot_bid_malformed"
                && demoDiagnostic(DemoDiagnostic::SpotAskMalformed)
                    == "ctrader_demo_spot_ask_malformed"
                && demoDiagnostic(DemoDiagnostic::SpotTrendbarPeriodMismatch)
                    == "ctrader_demo_spot_trendbar_period_mismatch"
                && demoDiagnostic(DemoDiagnostic::SpotTrendbarMalformed)
                    == "ctrader_demo_spot_trendbar_malformed"
                && demoDiagnostic(DemoDiagnostic::SpotMarketStateMalformed)
                    == "ctrader_demo_spot_market_state_malformed",
            "spot validation diagnostics lost their fixed redacted categories");
}

Gate7AccountRecord accountRecord(std::uint64_t id,
                                 std::optional<bool> isLive,
                                 std::string broker)
{
    Gate7AccountRecord record;
    record.accountId = id;
    record.isLive = isLive;
    record.brokerTitleShort = std::move(broker);
    return record;
}

void testDemoEndpointAndAccountContainment()
{
    require(CTraderGate7Config::isAllowedOpenApiEndpoint(
                "demo.ctraderapi.com", 5035)
                && !CTraderGate7Config::isAllowedOpenApiEndpoint(
                    "live.ctraderapi.com", 5035)
                && !CTraderGate7Config::isAllowedOpenApiEndpoint(
                    "demo.ctraderapi.com", 5036),
            "Demo transport endpoint allowlist widened");
    require(demoOutboundPayloadAllowed(PROTO_OA_NEW_ORDER_REQ)
                && demoOutboundPayloadAllowed(PROTO_OA_CLOSE_POSITION_REQ)
                && !demoOutboundPayloadAllowed(PROTO_OA_CANCEL_ORDER_REQ)
                && !demoOutboundPayloadAllowed(
                    PROTO_OA_AMEND_POSITION_SLTP_REQ),
            "Demo outbound trading allowlist is not limited to entry and close");

    Gate7AccountListEvidence evidence;
    evidence.tokenOwned = true;
    evidence.tradingScope = true;
    evidence.accounts.push_back(accountRecord(11, false, "FIBO"));
    require(selectFiboDemoAccount(evidence) == 11,
            "one explicit FIBO Demo account was not selected");

    evidence.accounts.clear();
    evidence.accounts.push_back(accountRecord(12, true, "FIBO"));
    require(!selectFiboDemoAccount(evidence).has_value(),
            "live account was accepted by Demo selection");
    evidence.accounts.clear();
    evidence.accounts.push_back(accountRecord(13, std::nullopt, "FIBO"));
    require(!selectFiboDemoAccount(evidence).has_value(),
            "account without explicit isLive=false was accepted");
    evidence.accounts.clear();
    evidence.accounts.push_back(accountRecord(14, false, "FIBO"));
    evidence.accounts.push_back(accountRecord(15, false, "FIBO"));
    require(!selectFiboDemoAccount(evidence).has_value(),
            "ambiguous multiple FIBO Demo accounts were accepted");
    evidence.accounts.clear();
    evidence.accounts.push_back(accountRecord(16, false, "NOT FIBO"));
    require(!selectFiboDemoAccount(evidence).has_value(),
            "non-exact broker title was accepted as FIBO");
}

void testTrendbarTimestampBoundary()
{
    ProtoOATrendbar bar;
    bar.set_volume(1);
    bar.set_period(M1);
    bar.set_low(200000000);
    bar.set_deltaopen(10);
    bar.set_deltaclose(20);
    bar.set_deltahigh(30);
    bar.set_utctimestampinminutes(1);
    const auto valid = decodeTrendbar(bar);
    require(valid.has_value() && valid->epochTimestamp == 60,
            "valid completed trendbar timestamp was rejected");

    const auto expectFailure = [](ProtoOATrendbar candidate,
                                  DemoDiagnostic expected,
                                  std::string_view fixedDiagnostic) {
        DemoDiagnostic failure{DemoDiagnostic::None};
        require(!decodeTrendbar(candidate, &failure).has_value()
                    && failure == expected
                    && demoDiagnostic(failure) == fixedDiagnostic,
                "trendbar failure lost its fixed redacted classification");
    };
    ProtoOATrendbar malformed = bar;
    malformed.clear_volume();
    expectFailure(malformed, DemoDiagnostic::SpotTrendbarEnvelopeMalformed,
                  "ctrader_demo_spot_trendbar_envelope_malformed");
    malformed = bar;
    malformed.clear_low();
    expectFailure(malformed, DemoDiagnostic::SpotTrendbarLowMalformed,
                  "ctrader_demo_spot_trendbar_low_malformed");
    malformed = bar;
    malformed.clear_deltaopen();
    malformed.clear_deltaclose();
    malformed.clear_deltahigh();
    const auto defaultDeltas = decodeTrendbar(malformed);
    require(defaultDeltas.has_value()
                && defaultDeltas->open == defaultDeltas->low
                && defaultDeltas->close == defaultDeltas->low
                && defaultDeltas->high == defaultDeltas->low,
            "absent optional deltas did not retain their Protobuf zero default");
    malformed = bar;
    malformed.clear_deltaclose();
    const auto defaultClose = decodeTrendbar(malformed);
    require(defaultClose.has_value()
                && defaultClose->close == defaultClose->low,
            "absent optional close delta was rejected instead of defaulting to zero");
    malformed = bar;
    malformed.clear_deltahigh();
    expectFailure(malformed, DemoDiagnostic::SpotTrendbarOhlcMalformed,
                  "ctrader_demo_spot_trendbar_ohlc_malformed");
    malformed = bar;
    malformed.clear_utctimestampinminutes();
    expectFailure(malformed, DemoDiagnostic::SpotTrendbarTimestampMalformed,
                  "ctrader_demo_spot_trendbar_timestamp_malformed");
    malformed = bar;
    malformed.set_deltaopen(std::numeric_limits<std::uint64_t>::max());
    expectFailure(malformed, DemoDiagnostic::SpotTrendbarDeltaOverflow,
                  "ctrader_demo_spot_trendbar_delta_overflow");
    malformed = bar;
    malformed.set_deltaopen(10);
    malformed.set_deltahigh(5);
    expectFailure(malformed, DemoDiagnostic::SpotTrendbarOhlcMalformed,
                  "ctrader_demo_spot_trendbar_ohlc_malformed");
    malformed = bar;
    malformed.set_utctimestampinminutes(0);
    expectFailure(malformed, DemoDiagnostic::SpotTrendbarTimestampMalformed,
                  "ctrader_demo_spot_trendbar_timestamp_malformed");
}

void testOAuthCompletionPage()
{
    const int listener = ::socket(AF_INET, SOCK_STREAM, 0);
    require(listener >= 0, "OAuth completion test listener creation failed");
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    require(::bind(listener, reinterpret_cast<sockaddr*>(&address),
                   sizeof(address)) == 0
                && ::listen(listener, 1) == 0,
            "OAuth completion test listener setup failed");
    socklen_t addressLength = sizeof(address);
    require(::getsockname(listener, reinterpret_cast<sockaddr*>(&address),
                          &addressLength) == 0,
            "OAuth completion test listener address failed");

    std::thread server([listener] { serveOAuthCompletion(listener); });
    const int client = ::socket(AF_INET, SOCK_STREAM, 0);
    require(client >= 0
                && ::connect(client, reinterpret_cast<sockaddr*>(&address),
                             sizeof(address)) == 0,
            "OAuth completion test client connection failed");
    require(sendAll(client,
                "GET /ctrader/oauth/complete HTTP/1.1\r\n"
                "Host: 127.0.0.1:18080\r\nConnection: close\r\n\r\n"),
            "OAuth completion test request failed");

    std::string response;
    std::array<char, 512> bytes{};
    while (true) {
        const ssize_t count = ::recv(client, bytes.data(), bytes.size(), 0);
        if (count > 0) {
            response.append(bytes.data(), static_cast<std::size_t>(count));
            continue;
        }
        if (count < 0 && errno == EINTR) continue;
        break;
    }
    ::close(client);
    server.join();
    ::close(listener);
    require(response.find("HTTP/1.1 200 OK") != std::string::npos
                && response.find(
                    "Authorization received. Return to TradeBot.")
                       != std::string::npos,
            "OAuth clean completion response was not served");
    secureClear(response);
}

} // namespace

int main()
{
    testStoredAndRefreshFlows();
    testFreshAndKeychainFailureFlows();
    testTypedHttpClassification();
    testProviderLifecycleBoundaryHelpers();
    testSubscriptionDiagnosticsAreLegSpecificAndRedacted();
    testDemoEndpointAndAccountContainment();
    testTrendbarTimestampBoundary();
    testOAuthCompletionPage();
    std::cout << "ctrader_demo_provider_private_tests: PASS\n";
    return 0;
}
