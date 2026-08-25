// The accepted Gate 7 translation unit owns the audited Keychain, loopback
// OAuth, TLS, Protobuf framing, correlation, heartbeat, and secure-clear
// primitives. M1 compiles that provider-private implementation into this one
// Demo-only adapter translation unit so the socket remains single-owner.
#include "../../CTraderGate7Runtime.mm"

#include "providers/ctrader/CTraderProviderAdapter.hpp"
#include "providers/ctrader/CTraderDemoMarketState.hpp"

#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace tradebot::ctrader {
namespace {

constexpr std::size_t DEMO_COMMAND_CAPACITY = 64;
constexpr auto DEMO_STARTUP_TIMEOUT = std::chrono::minutes(8);
constexpr auto DEMO_COMMAND_TIMEOUT = std::chrono::minutes(2);
constexpr std::size_t DEMO_RECONNECT_ATTEMPTS = 2;
constexpr auto DEMO_RECONNECT_BACKOFF = std::chrono::milliseconds(250);
constexpr std::string_view DEMO_CANONICAL_SYMBOL = "XAUUSD";
constexpr std::string_view DEMO_ORDER_LABEL = "MynyraDemoM1";

class CurlGlobalScope final {
public:
    ~CurlGlobalScope() { curl_global_cleanup(); }
};

enum class DemoDiagnostic : std::uint8_t {
    None,
    Configuration,
    ClientIdMissing,
    ClientSecretMissing,
    KeychainRead,
    KeychainWrite,
    TokenUnavailable,
    TokenTransport,
    TokenHttp4xx,
    TokenHttp5xx,
    TokenInvalidGrant,
    TokenMalformed,
    TokenScope,
    OAuthCallback,
    Tls,
    ApplicationAuth,
    AccountDiscovery,
    AccountSelection,
    AccountAuth,
    TraderRejected,
    AccountNotEmpty,
    InstrumentRejected,
    HistoricalBarsRejected,
    SubscriptionRejected,
    Transport,
    RateLimited,
    MalformedProviderEvent,
    SpotEnvelopeMalformed,
    SpotAccountMismatch,
    SpotSymbolMismatch,
    SpotBidMalformed,
    SpotAskMalformed,
    SpotTrendbarPeriodMismatch,
    SpotTrendbarMalformed,
    SpotTrendbarEnvelopeMalformed,
    SpotTrendbarLowMalformed,
    SpotTrendbarTimestampMalformed,
    SpotTrendbarDeltaOverflow,
    SpotTrendbarOhlcMalformed,
    SpotMarketStateMalformed,
    Reconciliation,
    ExpectedMargin,
    OrderRejected,
    ResourceExhausted
};

enum class DemoSubscriptionLeg : std::uint8_t {
    None,
    Spots,
    LiveM1
};

std::string_view demoDiagnostic(DemoDiagnostic value) noexcept
{
    switch (value) {
        case DemoDiagnostic::None: return "ctrader_demo_ok";
        case DemoDiagnostic::Configuration: return "ctrader_demo_configuration_rejected";
        case DemoDiagnostic::ClientIdMissing: return "ctrader_demo_client_id_missing";
        case DemoDiagnostic::ClientSecretMissing: return "ctrader_demo_client_secret_missing";
        case DemoDiagnostic::KeychainRead: return "ctrader_demo_keychain_read_failed";
        case DemoDiagnostic::KeychainWrite: return "ctrader_demo_keychain_write_failed";
        case DemoDiagnostic::TokenUnavailable: return "ctrader_demo_token_unavailable";
        case DemoDiagnostic::TokenTransport: return "ctrader_demo_token_transport_failed";
        case DemoDiagnostic::TokenHttp4xx: return "ctrader_demo_token_http_4xx";
        case DemoDiagnostic::TokenHttp5xx: return "ctrader_demo_token_http_5xx";
        case DemoDiagnostic::TokenInvalidGrant: return "ctrader_demo_token_invalid_grant";
        case DemoDiagnostic::TokenMalformed: return "ctrader_demo_token_malformed";
        case DemoDiagnostic::TokenScope: return "ctrader_demo_token_scope_mismatch";
        case DemoDiagnostic::OAuthCallback: return "ctrader_demo_oauth_callback_failed";
        case DemoDiagnostic::Tls: return "ctrader_demo_tls_failed";
        case DemoDiagnostic::ApplicationAuth: return "ctrader_demo_application_auth_failed";
        case DemoDiagnostic::AccountDiscovery: return "ctrader_demo_account_discovery_failed";
        case DemoDiagnostic::AccountSelection: return "ctrader_demo_fibo_account_selection_failed";
        case DemoDiagnostic::AccountAuth: return "ctrader_demo_account_auth_failed";
        case DemoDiagnostic::TraderRejected: return "ctrader_demo_trader_rejected";
        case DemoDiagnostic::AccountNotEmpty: return "ctrader_demo_account_not_empty";
        case DemoDiagnostic::InstrumentRejected: return "ctrader_demo_xauusd_rejected";
        case DemoDiagnostic::HistoricalBarsRejected: return "ctrader_demo_history_rejected";
        case DemoDiagnostic::SubscriptionRejected: return "ctrader_demo_subscription_rejected";
        case DemoDiagnostic::Transport: return "ctrader_demo_transport_failed";
        case DemoDiagnostic::RateLimited: return "ctrader_demo_rate_limited";
        case DemoDiagnostic::MalformedProviderEvent: return "ctrader_demo_provider_event_malformed";
        case DemoDiagnostic::SpotEnvelopeMalformed: return "ctrader_demo_spot_envelope_malformed";
        case DemoDiagnostic::SpotAccountMismatch: return "ctrader_demo_spot_account_mismatch";
        case DemoDiagnostic::SpotSymbolMismatch: return "ctrader_demo_spot_symbol_mismatch";
        case DemoDiagnostic::SpotBidMalformed: return "ctrader_demo_spot_bid_malformed";
        case DemoDiagnostic::SpotAskMalformed: return "ctrader_demo_spot_ask_malformed";
        case DemoDiagnostic::SpotTrendbarPeriodMismatch: return "ctrader_demo_spot_trendbar_period_mismatch";
        case DemoDiagnostic::SpotTrendbarMalformed: return "ctrader_demo_spot_trendbar_malformed";
        case DemoDiagnostic::SpotTrendbarEnvelopeMalformed: return "ctrader_demo_spot_trendbar_envelope_malformed";
        case DemoDiagnostic::SpotTrendbarLowMalformed: return "ctrader_demo_spot_trendbar_low_malformed";
        case DemoDiagnostic::SpotTrendbarTimestampMalformed: return "ctrader_demo_spot_trendbar_timestamp_malformed";
        case DemoDiagnostic::SpotTrendbarDeltaOverflow: return "ctrader_demo_spot_trendbar_delta_overflow";
        case DemoDiagnostic::SpotTrendbarOhlcMalformed: return "ctrader_demo_spot_trendbar_ohlc_malformed";
        case DemoDiagnostic::SpotMarketStateMalformed: return "ctrader_demo_spot_market_state_malformed";
        case DemoDiagnostic::Reconciliation: return "ctrader_demo_reconciliation_failed";
        case DemoDiagnostic::ExpectedMargin: return "ctrader_demo_expected_margin_failed";
        case DemoDiagnostic::OrderRejected: return "ctrader_demo_order_rejected";
        case DemoDiagnostic::ResourceExhausted: return "ctrader_demo_resource_exhausted";
    }
    return "ctrader_demo_unknown_failure";
}

std::string demoSubscriptionDiagnostic(
    DemoSubscriptionLeg leg,
    Gate7ResidualFailure failure)
{
    if (failure == Gate7ResidualFailure::None) {
        failure = Gate7ResidualFailure::SubscriptionProofRejected;
    }
    constexpr std::string_view gate7Prefix = "gate7_subscription_";
    std::string_view fixed = safeGate7ResidualDiagnostic(failure);
    const std::string_view suffix = fixed.starts_with(gate7Prefix)
        ? fixed.substr(gate7Prefix.size())
        : std::string_view{"unknown_failure"};
    std::string result;
    switch (leg) {
        case DemoSubscriptionLeg::Spots:
            result = "ctrader_demo_spots_subscription_";
            break;
        case DemoSubscriptionLeg::LiveM1:
            result = "ctrader_demo_live_m1_subscription_";
            break;
        case DemoSubscriptionLeg::None:
            result = "ctrader_demo_subscription_";
            break;
    }
    result.append(suffix);
    return result;
}

FailureCategory subscriptionFailureCategory(
    Gate7ResidualFailure failure) noexcept
{
    switch (failure) {
        case Gate7ResidualFailure::SubscriptionResponseTimeout:
            return FailureCategory::Timeout;
        case Gate7ResidualFailure::SubscriptionSendFailed:
        case Gate7ResidualFailure::SubscriptionTransportClosed:
        case Gate7ResidualFailure::SubscriptionProviderUnavailable:
            return FailureCategory::Transport;
        case Gate7ResidualFailure::SubscriptionAccountRejected:
        case Gate7ResidualFailure::SubscriptionTokenInvalidated:
            return FailureCategory::Authentication;
        case Gate7ResidualFailure::SubscriptionRateLimited:
            return FailureCategory::RateLimited;
        case Gate7ResidualFailure::SubscriptionResponseMalformed:
        case Gate7ResidualFailure::SubscriptionResourceExhausted:
            return FailureCategory::MalformedEvent;
        case Gate7ResidualFailure::None:
            return FailureCategory::None;
        default:
            return FailureCategory::Validation;
    }
}

bool recoverableTransportOutcome(Gate7TransportOutcome outcome) noexcept
{
    return outcome == Gate7TransportOutcome::TransportClosed
        || outcome == Gate7TransportOutcome::AccountDisconnected
        || outcome == Gate7TransportOutcome::ClientDisconnected;
}

bool recoverableSendOutcome(Gate7SendOutcome outcome) noexcept
{
    return outcome == Gate7SendOutcome::InactiveConnection
        || outcome == Gate7SendOutcome::WriteTimeout
        || outcome == Gate7SendOutcome::TransportClosed
        || outcome == Gate7SendOutcome::WriteFailed;
}

bool mergeLocalIdentity(std::optional<std::uint64_t>& resolved,
                        std::uint64_t candidate) noexcept
{
    if (candidate == 0 || (resolved.has_value() && *resolved != candidate)) {
        return false;
    }
    resolved = candidate;
    return true;
}

bool providerFillKindMatches(ProtoOAExecutionType type,
                             std::int64_t cumulative,
                             std::int64_t requested) noexcept
{
    if (cumulative <= 0 || requested <= 0 || cumulative > requested) {
        return false;
    }
    return (type == ORDER_FILLED && cumulative == requested)
        || (type == ORDER_PARTIAL_FILL && cumulative < requested);
}

std::optional<std::int64_t> selectFiboDemoAccount(
    const Gate7AccountListEvidence& accounts)
{
    if (!accounts.tokenOwned || !accounts.tradingScope) return std::nullopt;
    std::vector<std::int64_t> candidates;
    for (const auto& account : accounts.accounts) {
        if (!account.accountId.has_value() || !account.isLive.has_value()
            || *account.isLive || !account.brokerTitleShort.has_value()
            || *account.accountId > static_cast<std::uint64_t>(
                   std::numeric_limits<std::int64_t>::max())) {
            continue;
        }
        std::string broker = *account.brokerTitleShort;
        if (broker == "FIBO") {
            candidates.push_back(static_cast<std::int64_t>(*account.accountId));
        }
        secureClear(broker);
    }
    if (candidates.size() != 1) {
        for (auto& value : candidates) secureClear(value);
        return std::nullopt;
    }
    const std::int64_t selected = candidates.front();
    secureClear(candidates.front());
    return selected;
}

FailureCategory failureCategory(DemoDiagnostic value) noexcept
{
    switch (value) {
        case DemoDiagnostic::None: return FailureCategory::None;
        case DemoDiagnostic::ClientIdMissing:
        case DemoDiagnostic::ClientSecretMissing:
        case DemoDiagnostic::KeychainRead:
        case DemoDiagnostic::KeychainWrite:
        case DemoDiagnostic::TokenUnavailable:
        case DemoDiagnostic::TokenInvalidGrant:
        case DemoDiagnostic::TokenScope:
        case DemoDiagnostic::OAuthCallback:
        case DemoDiagnostic::ApplicationAuth:
        case DemoDiagnostic::AccountDiscovery:
        case DemoDiagnostic::AccountSelection:
        case DemoDiagnostic::AccountAuth:
            return FailureCategory::Authentication;
        case DemoDiagnostic::TokenTransport:
        case DemoDiagnostic::Tls:
        case DemoDiagnostic::Transport:
            return FailureCategory::Transport;
        case DemoDiagnostic::TokenHttp5xx:
            return FailureCategory::Transport;
        case DemoDiagnostic::RateLimited:
            return FailureCategory::RateLimited;
        case DemoDiagnostic::TokenMalformed:
        case DemoDiagnostic::MalformedProviderEvent:
        case DemoDiagnostic::SpotEnvelopeMalformed:
        case DemoDiagnostic::SpotBidMalformed:
        case DemoDiagnostic::SpotAskMalformed:
        case DemoDiagnostic::SpotTrendbarPeriodMismatch:
        case DemoDiagnostic::SpotTrendbarMalformed:
        case DemoDiagnostic::SpotTrendbarEnvelopeMalformed:
        case DemoDiagnostic::SpotTrendbarLowMalformed:
        case DemoDiagnostic::SpotTrendbarTimestampMalformed:
        case DemoDiagnostic::SpotTrendbarDeltaOverflow:
        case DemoDiagnostic::SpotTrendbarOhlcMalformed:
        case DemoDiagnostic::SpotMarketStateMalformed:
        case DemoDiagnostic::HistoricalBarsRejected:
            return FailureCategory::MalformedEvent;
        case DemoDiagnostic::Reconciliation:
        case DemoDiagnostic::AccountNotEmpty:
            return FailureCategory::ReconciliationMismatch;
        case DemoDiagnostic::OrderRejected:
            return FailureCategory::Rejected;
        default:
            return FailureCategory::Validation;
    }
}

void appendUint64(std::string& output, std::uint64_t value)
{
    for (int shift = 56; shift >= 0; shift -= 8) {
        output.push_back(static_cast<char>((value >> shift) & 0xff));
    }
}

void appendSized(std::string& output, std::string_view value)
{
    appendUint32(output, static_cast<std::uint32_t>(value.size()));
    output.append(value.data(), value.size());
}

bool encodeTokenEnvelope(const TokenEnvelope& token, Sensitive& encoded) noexcept
{
    if (!tokenUsable(token) || token.expiresAtEpochSeconds <= 0
        || token.scope.size() > 8192 || token.tokenType.view().size() > 8192
        || token.accessToken.view().size() > 8192
        || token.refreshToken.view().size() > 8192) {
        return false;
    }
    std::string bytes;
    try {
        bytes = "TBG6TOK1";
        appendUint64(bytes, static_cast<std::uint64_t>(token.expiresAtEpochSeconds));
        appendSized(bytes, token.scope);
        appendSized(bytes, token.tokenType.view());
        appendSized(bytes, token.accessToken.view());
        appendSized(bytes, token.refreshToken.view());
        encoded = Sensitive(std::move(bytes));
        return true;
    } catch (...) {
        secureClear(bytes);
        return false;
    }
}

RuntimeFailure writeKeychainValue(std::string_view service,
                                  std::string_view bytes) noexcept
{
    const auto user = localUserName();
    if (!user.has_value() || bytes.empty() || bytes.size() > 65536) {
        return RuntimeFailure::KeychainRead;
    }
    CFStringRef serviceRef = makeCfString(service);
    CFStringRef accountRef = makeCfString(*user);
    CFDataRef dataRef = CFDataCreate(
        kCFAllocatorDefault,
        reinterpret_cast<const UInt8*>(bytes.data()),
        static_cast<CFIndex>(bytes.size()));
    if (serviceRef == nullptr || accountRef == nullptr || dataRef == nullptr) {
        if (serviceRef != nullptr) CFRelease(serviceRef);
        if (accountRef != nullptr) CFRelease(accountRef);
        if (dataRef != nullptr) CFRelease(dataRef);
        return RuntimeFailure::KeychainRead;
    }

    const void* queryKeys[] = {kSecClass, kSecAttrService, kSecAttrAccount};
    const void* queryValues[] = {
        kSecClassGenericPassword, serviceRef, accountRef};
    CFDictionaryRef query = CFDictionaryCreate(
        kCFAllocatorDefault, queryKeys, queryValues, 3,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    const void* updateKeys[] = {kSecValueData};
    const void* updateValues[] = {dataRef};
    CFDictionaryRef update = CFDictionaryCreate(
        kCFAllocatorDefault, updateKeys, updateValues, 1,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    OSStatus status = query != nullptr && update != nullptr
        ? SecItemUpdate(query, update) : errSecAllocate;
    if (status == errSecItemNotFound) {
        const void* addKeys[] = {
            kSecClass, kSecAttrService, kSecAttrAccount, kSecValueData,
            kSecAttrAccessible};
        const void* addValues[] = {
            kSecClassGenericPassword, serviceRef, accountRef, dataRef,
            kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly};
        CFDictionaryRef add = CFDictionaryCreate(
            kCFAllocatorDefault, addKeys, addValues, 5,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);
        status = add == nullptr ? errSecAllocate : SecItemAdd(add, nullptr);
        if (add != nullptr) CFRelease(add);
    }
    if (query != nullptr) CFRelease(query);
    if (update != nullptr) CFRelease(update);
    CFRelease(serviceRef);
    CFRelease(accountRef);
    CFRelease(dataRef);
    return status == errSecSuccess
        ? RuntimeFailure::None : RuntimeFailure::KeychainRead;
}

struct DemoTokenHttpResult {
    CURLcode transport{CURLE_FAILED_INIT};
    long status{0};
    std::string body;
};

DemoTokenHttpResult performDemoTokenRequest(Sensitive& url) noexcept
{
    DemoTokenHttpResult result;
    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        url.clear();
        return result;
    }
    curl_slist* headers = curl_slist_append(nullptr, "Accept: application/json");
    curl_slist* extended = headers == nullptr ? nullptr
        : curl_slist_append(headers, "Cache-Control: no-store");
    if (extended != nullptr
        && configureCurl(curl, url.view(), extended, result.body)) {
        result.transport = curl_easy_perform(curl);
        if (result.transport == CURLE_OK) {
            (void)curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status);
        }
    }
    curl_slist_free_all(extended == nullptr ? headers : extended);
    curl_easy_cleanup(curl);
    url.clear();
    return result;
}

DemoDiagnostic classifyTokenFailure(const DemoTokenHttpResult& result) noexcept
{
    if (result.transport != CURLE_OK) return DemoDiagnostic::TokenTransport;
    if (result.status >= 500) return DemoDiagnostic::TokenHttp5xx;
    if (result.status >= 400) {
        if (result.body.find("INVALID_GRANT") != std::string::npos
            || result.body.find("invalid_grant") != std::string::npos) {
            return DemoDiagnostic::TokenInvalidGrant;
        }
        return DemoDiagnostic::TokenHttp4xx;
    }
    if (result.status != 200) return DemoDiagnostic::TokenMalformed;
    return DemoDiagnostic::None;
}

DemoDiagnostic obtainDemoToken(Sensitive& url, TokenEnvelope& output) noexcept
{
    DemoTokenHttpResult response = performDemoTokenRequest(url);
    const DemoDiagnostic transport = classifyTokenFailure(response);
    if (transport != DemoDiagnostic::None) {
        secureClear(response.body);
        return transport;
    }
    const bool parsed = parseTokenResponse(response.body, output);
    secureClear(response.body);
    if (!parsed || !tokenUsable(output)) return DemoDiagnostic::TokenMalformed;
    return DemoDiagnostic::None;
}

DemoDiagnostic tokenUrl(std::string_view grantName,
                        std::string_view grantValue,
                        std::string_view clientId,
                        std::string_view clientSecret,
                        TokenEnvelope& output) noexcept
{
    CURL* encoder = curl_easy_init();
    if (encoder == nullptr) return DemoDiagnostic::ResourceExhausted;
    auto grant = urlEncode(encoder, grantValue);
    auto client = urlEncode(encoder, clientId);
    auto secret = urlEncode(encoder, clientSecret);
    auto redirect = grantName == "authorization_code"
        ? urlEncode(encoder, CTraderGate7Config::REDIRECT_URI)
        : std::optional<std::string>{};
    curl_easy_cleanup(encoder);
    if (!grant.has_value() || !client.has_value() || !secret.has_value()
        || (grantName == "authorization_code" && !redirect.has_value())) {
        if (grant) secureClear(*grant);
        if (client) secureClear(*client);
        if (secret) secureClear(*secret);
        if (redirect) secureClear(*redirect);
        return DemoDiagnostic::ResourceExhausted;
    }
    std::string raw;
    try {
        raw = "https://openapi.ctrader.com/apps/token?grant_type=";
        raw += grantName;
        raw += grantName == "authorization_code" ? "&code=" : "&refresh_token=";
        raw += *grant;
        if (redirect.has_value()) {
            raw += "&redirect_uri=";
            raw += *redirect;
        }
        raw += "&client_id=";
        raw += *client;
        raw += "&client_secret=";
        raw += *secret;
    } catch (...) {
        secureClear(raw);
        secureClear(*grant); secureClear(*client); secureClear(*secret);
        if (redirect) secureClear(*redirect);
        return DemoDiagnostic::ResourceExhausted;
    }
    secureClear(*grant); secureClear(*client); secureClear(*secret);
    if (redirect) secureClear(*redirect);
    Sensitive url(std::move(raw));
    return obtainDemoToken(url, output);
}

DemoDiagnostic persistToken(const TokenEnvelope& token) noexcept
{
    Sensitive encoded;
    if (!encodeTokenEnvelope(token, encoded)) {
        return DemoDiagnostic::TokenMalformed;
    }
    const RuntimeFailure result = writeKeychainValue(
        CTraderGate7Config::TOKEN_SERVICE, encoded.view());
    encoded.clear();
    return result == RuntimeFailure::None
        ? DemoDiagnostic::None : DemoDiagnostic::KeychainWrite;
}

class DemoTokenServices {
public:
    virtual ~DemoTokenServices() = default;
    virtual RuntimeFailure readStored(Sensitive& bytes) noexcept = 0;
    virtual DemoDiagnostic exchange(std::string_view grantName,
                                    std::string_view grantValue,
                                    std::string_view clientId,
                                    std::string_view clientSecret,
                                    TokenEnvelope& output) noexcept = 0;
    virtual DemoDiagnostic authorize(std::string_view clientId,
                                     Sensitive& code) noexcept = 0;
    virtual DemoDiagnostic persist(const TokenEnvelope& token) noexcept = 0;
};

class ProductionDemoTokenServices final : public DemoTokenServices {
public:
    RuntimeFailure readStored(Sensitive& bytes) noexcept override
    {
        return readKeychainValue(CTraderGate7Config::TOKEN_SERVICE, bytes);
    }

    DemoDiagnostic exchange(std::string_view grantName,
                            std::string_view grantValue,
                            std::string_view clientId,
                            std::string_view clientSecret,
                            TokenEnvelope& output) noexcept override
    {
        return tokenUrl(
            grantName, grantValue, clientId, clientSecret, output);
    }

    DemoDiagnostic authorize(std::string_view clientId,
                             Sensitive& code) noexcept override
    {
        Gate7OAuthFailure callbackFailure = Gate7OAuthFailure::None;
        auto received = authorizeInBrowser(clientId, callbackFailure);
        if (!received.has_value()) return DemoDiagnostic::OAuthCallback;
        code = std::move(*received);
        received.reset();
        return DemoDiagnostic::None;
    }

    DemoDiagnostic persist(const TokenEnvelope& token) noexcept override
    {
        return persistToken(token);
    }
};

DemoDiagnostic acquireDemoTokenWithServices(
    bool freshOAuth,
    std::string_view clientId,
    std::string_view clientSecret,
    TokenEnvelope& token,
    DemoTokenServices& services,
    bool& persistenceRequired) noexcept
{
    persistenceRequired = false;
    TokenEnvelope stored;
    if (!freshOAuth) {
        Sensitive storedBytes;
        const RuntimeFailure read = services.readStored(storedBytes);
        if (read == RuntimeFailure::None) {
            (void)parseStoredToken(storedBytes.view(), stored);
        } else if (read != RuntimeFailure::TokenUnavailable) {
            storedBytes.clear();
            return DemoDiagnostic::KeychainRead;
        }
        storedBytes.clear();
        if (tokenUsable(stored)) {
            try {
                token.accessToken = Sensitive(std::string(stored.accessToken.view()));
                token.refreshToken = Sensitive(std::string(stored.refreshToken.view()));
                token.tokenType = Sensitive(std::string(stored.tokenType.view()));
                token.expiresAtEpochSeconds = stored.expiresAtEpochSeconds;
                token.scope = stored.scope;
            } catch (...) {
                clearToken(stored);
                return DemoDiagnostic::ResourceExhausted;
            }
            clearToken(stored);
            return DemoDiagnostic::None;
        }
        if (stored.refreshToken.empty()) {
            clearToken(stored);
            return DemoDiagnostic::TokenUnavailable;
        }
        const DemoDiagnostic refreshed = services.exchange(
            "refresh_token", stored.refreshToken.view(),
            clientId, clientSecret, token);
        clearToken(stored);
        if (refreshed != DemoDiagnostic::None) return refreshed;
        persistenceRequired = true;
        return DemoDiagnostic::None;
    }

    Sensitive code;
    const DemoDiagnostic authorized = services.authorize(clientId, code);
    if (authorized != DemoDiagnostic::None) return authorized;
    const DemoDiagnostic exchanged = services.exchange(
        "authorization_code", code.view(), clientId, clientSecret, token);
    code.clear();
    if (exchanged != DemoDiagnostic::None) return exchanged;
    persistenceRequired = true;
    return DemoDiagnostic::None;
}

DemoDiagnostic acquireDemoToken(bool freshOAuth,
                                std::string_view clientId,
                                std::string_view clientSecret,
                                TokenEnvelope& token,
                                bool& persistenceRequired) noexcept
{
    ProductionDemoTokenServices services;
    return acquireDemoTokenWithServices(
        freshOAuth, clientId, clientSecret, token, services,
        persistenceRequired);
}

template <typename Request, typename Response>
bool requestResponse(StrictTransport& transport,
                     std::uint32_t requestType,
                     std::uint32_t responseType,
                     Request& request,
                     Response& response,
                     std::string_view step) noexcept
{
    std::string correlation;
    std::string payload;
    try {
        correlation = transport.nextCorrelation(step);
        if (!transport.send(requestType, request, correlation)
            || !transport.receiveExpected(responseType, correlation, payload)
            || !response.ParseFromString(payload) || !response.IsInitialized()) {
            secureClear(correlation);
            secureClear(payload);
            response.Clear();
            return false;
        }
        secureClear(correlation);
        secureClear(payload);
        return true;
    } catch (...) {
        secureClear(correlation);
        secureClear(payload);
        response.Clear();
        return false;
    }
}

std::optional<Decimal64> moneyDecimal(std::int64_t units,
                                      std::uint32_t digits) noexcept
{
    if (digits > Decimal64::MAX_SCALE) return std::nullopt;
    return Decimal64{units, static_cast<std::uint8_t>(digits)};
}

std::optional<Decimal64> unsignedMoneyDecimal(std::uint64_t units,
                                              std::uint32_t digits) noexcept
{
    if (units > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max())) {
        return std::nullopt;
    }
    return moneyDecimal(static_cast<std::int64_t>(units), digits);
}

std::optional<Decimal64> addDecimal(Decimal64 left, Decimal64 right) noexcept
{
    while (left.scale < right.scale) {
        if (left.units > std::numeric_limits<std::int64_t>::max() / 10
            || left.units < std::numeric_limits<std::int64_t>::min() / 10) {
            return std::nullopt;
        }
        left.units *= 10;
        ++left.scale;
    }
    while (right.scale < left.scale) {
        if (right.units > std::numeric_limits<std::int64_t>::max() / 10
            || right.units < std::numeric_limits<std::int64_t>::min() / 10) {
            return std::nullopt;
        }
        right.units *= 10;
        ++right.scale;
    }
    if ((right.units > 0
         && left.units > std::numeric_limits<std::int64_t>::max() - right.units)
        || (right.units < 0
            && left.units < std::numeric_limits<std::int64_t>::min() - right.units)) {
        return std::nullopt;
    }
    return Decimal64{left.units + right.units, left.scale};
}

std::string normalizedSymbolName(std::string_view name)
{
    std::string result;
    result.reserve(name.size());
    for (const unsigned char c : name) {
        if (c == '/' || c == ' ' || c == '-') continue;
        result.push_back(static_cast<char>(std::toupper(c)));
    }
    return result;
}

std::optional<MarketCandle> decodeTrendbar(
    const ProtoOATrendbar& bar,
    DemoDiagnostic* failure = nullptr)
{
    if (failure != nullptr) *failure = DemoDiagnostic::None;
    const auto reject = [failure](DemoDiagnostic reason) {
        if (failure != nullptr) *failure = reason;
        return std::optional<MarketCandle>{};
    };
    if (!bar.IsInitialized() || bar.volume() < 0) {
        return reject(DemoDiagnostic::SpotTrendbarEnvelopeMalformed);
    }
    if (!bar.has_low() || bar.low() <= 0) {
        return reject(DemoDiagnostic::SpotTrendbarLowMalformed);
    }
    if (!bar.has_utctimestampinminutes()) {
        return reject(DemoDiagnostic::SpotTrendbarTimestampMalformed);
    }
    const std::uint64_t timestampMinutes =
        static_cast<std::uint64_t>(bar.utctimestampinminutes());
    if (timestampMinutes == 0
        || timestampMinutes
               > std::numeric_limits<std::uint64_t>::max() / 60ULL) {
        return reject(DemoDiagnostic::SpotTrendbarTimestampMalformed);
    }
    const std::uint64_t low = static_cast<std::uint64_t>(bar.low());
    if (bar.deltaopen() > std::numeric_limits<std::uint64_t>::max() - low
        || bar.deltaclose() > std::numeric_limits<std::uint64_t>::max() - low
        || bar.deltahigh() > std::numeric_limits<std::uint64_t>::max() - low) {
        return reject(DemoDiagnostic::SpotTrendbarDeltaOverflow);
    }
    MarketCandle candle;
    candle.epochTimestamp = timestampMinutes * 60ULL;
    candle.open = static_cast<double>(low + bar.deltaopen()) / 100000.0;
    candle.close = static_cast<double>(low + bar.deltaclose()) / 100000.0;
    candle.high = static_cast<double>(low + bar.deltahigh()) / 100000.0;
    candle.low = static_cast<double>(low) / 100000.0;
    candle.volume = static_cast<double>(bar.volume());
    candle.symbol = std::string(DEMO_CANONICAL_SYMBOL);
    if (!(candle.low <= candle.open && candle.open <= candle.high
          && candle.low <= candle.close && candle.close <= candle.high)) {
        return reject(DemoDiagnostic::SpotTrendbarOhlcMalformed);
    }
    return candle;
}

std::optional<MarketCandle> decodeHistoricalTrendbar(const ProtoOATrendbar& bar)
{
    if (auto candle = decodeTrendbar(bar); candle.has_value()) {
        return candle;
    }
    return std::nullopt;
}

} // namespace

class CTraderTransport {};
class CTraderCodec {};
class CTraderAuthService {};
class CTraderAccountService {};
class CTraderInstrumentService {};
class CTraderMarketDataService {};
class CTraderOrderService {};

class CTraderSession final {
public:
    using AcknowledgementCallback = IBrokerAdapter::AcknowledgementCallback;
    using ExecutionCallback = IBrokerAdapter::ExecutionCallback;
    using CancelCallback = IBrokerAdapter::CancelCallback;
    using HealthCallback = IBrokerAdapter::HealthCallback;
    using MarketDataCallback = IMarketDataSource::MarketDataCallback;

    CTraderSession() = default;
    ~CTraderSession() { stop(); }

    void setAcknowledgementCallback(AcknowledgementCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_acknowledgement = std::move(callback);
    }
    void setExecutionCallback(ExecutionCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_execution = std::move(callback);
    }
    void setCancelCallback(CancelCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_cancel = std::move(callback);
    }
    void setHealthCallback(HealthCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_healthCallback = std::move(callback);
    }
    void setMarketDataCallback(MarketDataCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_marketData = std::move(callback);
    }

    bool start(bool freshOAuth)
    {
        std::lock_guard<std::mutex> startLock(m_startMutex);
        if (m_io.joinable()) return m_connected.load();
        m_stopRequested.store(false);
        setFailure(DemoDiagnostic::None);
        auto ready = std::make_shared<std::promise<bool>>();
        auto future = ready->get_future();
        try {
            m_io = std::thread([this, freshOAuth, ready] {
                try {
                    run(freshOAuth, ready);
                } catch (...) {
                    try {
                        setFailure(DemoDiagnostic::ResourceExhausted);
                    } catch (...) {
                    }
                    m_connected.store(false);
                    m_candleChanged.notify_all();
                    failPendingCommands();
                    clearProviderState();
                    try { ready->set_value(false); } catch (...) {}
                }
            });
        } catch (...) {
            setFailure(DemoDiagnostic::ResourceExhausted);
            return false;
        }
        if (future.wait_for(DEMO_STARTUP_TIMEOUT) != std::future_status::ready) {
            m_stopRequested.store(true);
            m_commandChanged.notify_all();
            setFailure(DemoDiagnostic::Transport);
            return false;
        }
        return future.get();
    }

    void stop() noexcept
    {
        m_stopRequested.store(true);
        m_commandChanged.notify_all();
        m_candleChanged.notify_all();
        if (m_io.joinable() && m_io.get_id() != std::this_thread::get_id()) {
            m_io.join();
        }
        m_connected.store(false);
        clearProviderState();
    }

    bool connected() const noexcept { return m_connected.load(); }

    bool submit(const NormalizedOrder& order)
    {
        Command command;
        command.type = CommandType::Submit;
        command.order = order;
        command.boolResult = std::make_shared<std::promise<bool>>();
        auto future = command.boolResult->get_future();
        if (!enqueue(std::move(command))) return false;
        return future.wait_for(DEMO_COMMAND_TIMEOUT) == std::future_status::ready
            && future.get();
    }

    ReconciliationSnapshot reconcile(std::uint64_t timestampNs)
    {
        Command command;
        command.type = CommandType::Reconcile;
        command.timestampNs = timestampNs;
        command.reconciliation =
            std::make_shared<std::promise<ReconciliationSnapshot>>();
        auto future = command.reconciliation->get_future();
        if (!enqueue(std::move(command))
            || future.wait_for(DEMO_COMMAND_TIMEOUT) != std::future_status::ready) {
            ReconciliationSnapshot failed;
            failed.timestampNs = timestampNs;
            failed.status = ReconciliationStatus::Unsupported;
            return failed;
        }
        return future.get();
    }

    std::optional<OrderRiskContext> riskContext(PositionSide direction)
    {
        Command command;
        command.type = CommandType::RiskContext;
        command.direction = direction;
        command.risk =
            std::make_shared<std::promise<std::optional<OrderRiskContext>>>();
        auto future = command.risk->get_future();
        if (!enqueue(std::move(command))
            || future.wait_for(DEMO_COMMAND_TIMEOUT) != std::future_status::ready) {
            return std::nullopt;
        }
        return future.get();
    }

    std::vector<MarketCandle> historical() const
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_history;
    }

    std::optional<MarketCandle> waitForCandle(std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(m_stateMutex);
        m_candleChanged.wait_for(lock, timeout, [&] {
            return m_marketState.hasCompletedCandle() || !m_connected.load()
                || m_stopRequested.load();
        });
        return m_marketState.popCompletedCandle();
    }

    AdapterHealthEvent health() const
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_health;
    }

    std::optional<AccountSnapshot> account() const
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_account.complete
            ? std::optional<AccountSnapshot>(m_account) : std::nullopt;
    }

    std::optional<InstrumentSpec> instrument(
        const std::string& canonicalSymbol) const
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (!m_instrument.complete
            || canonicalSymbol != m_instrument.canonicalSymbol) {
            return std::nullopt;
        }
        return m_instrument;
    }

    FailureCategory lastFailure() const noexcept
    {
        const DemoDiagnostic diagnostic = m_diagnostic.load();
        return diagnostic == DemoDiagnostic::SubscriptionRejected
            ? subscriptionFailureCategory(m_subscriptionFailure.load())
            : failureCategory(diagnostic);
    }

    std::string diagnostic() const
    {
        const DemoDiagnostic diagnostic = m_diagnostic.load();
        return diagnostic == DemoDiagnostic::SubscriptionRejected
            ? demoSubscriptionDiagnostic(m_subscriptionLeg.load(),
                                         m_subscriptionFailure.load())
            : std::string(demoDiagnostic(diagnostic));
    }

private:
    enum class CommandType : std::uint8_t { Submit, Reconcile, RiskContext };

    struct Command {
        CommandType type{CommandType::Reconcile};
        NormalizedOrder order;
        PositionSide direction{PositionSide::Long};
        std::uint64_t timestampNs{0};
        std::shared_ptr<std::promise<bool>> boolResult;
        std::shared_ptr<std::promise<ReconciliationSnapshot>> reconciliation;
        std::shared_ptr<std::promise<std::optional<OrderRiskContext>>> risk;
    };

    struct LocalOrderState {
        NormalizedOrder order;
        std::int64_t requestedVolume{0};
        std::int64_t cumulativeVolume{0};
        std::string syntheticExternalOrderId;
        std::optional<std::string> logicalPositionId;
        bool accepted{false};
        bool exposureAdoptedByReconciliation{false};
    };

    void clearProviderState() noexcept
    {
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            secureClear(m_account.currency);
            m_account = AccountSnapshot{};
            secureClear(m_instrument.executionAlias);
            secureClear(m_instrument.canonicalSymbol);
            m_instrument = InstrumentSpec{};
            for (auto& candle : m_history) {
                candle.epochTimestamp = 0;
                candle.open = 0.0;
                candle.high = 0.0;
                candle.low = 0.0;
                candle.close = 0.0;
                candle.volume = 0.0;
                secureClear(candle.symbol);
            }
            m_history.clear();
            m_marketState.resetLiveGeneration();
        }
        secureClear(m_accountId);
        secureClear(m_symbolId);
        secureClear(m_depositAssetId);
        secureClear(m_balanceRaw);
        volatile std::uint64_t* balanceVersion = &m_balanceVersion;
        *balanceVersion = 0;
        secureClear(m_currency);
        secureClear(m_executionAlias);

        for (auto& [_, order] : m_orders) {
            secureClear(order.syntheticExternalOrderId);
            if (order.logicalPositionId.has_value()) {
                secureClear(*order.logicalPositionId);
                order.logicalPositionId.reset();
            }
            secureClear(order.order.request.canonicalSymbol);
            secureClear(order.order.request.sourceId);
            secureClear(order.order.request.idempotencyKey);
            if (order.order.request.logicalPositionId.has_value()) {
                secureClear(*order.order.request.logicalPositionId);
                order.order.request.logicalPositionId.reset();
            }
        }
        m_orders.clear();
        while (!m_clientOrderToLocal.empty()) {
            auto node = m_clientOrderToLocal.extract(
                m_clientOrderToLocal.begin());
            secureClear(node.key());
        }
        while (!m_correlationToLocal.empty()) {
            auto node = m_correlationToLocal.extract(
                m_correlationToLocal.begin());
            secureClear(node.key());
        }
        while (!m_nativeToLogical.empty()) {
            auto node = m_nativeToLogical.extract(m_nativeToLogical.begin());
            secureClear(node.key());
            secureClear(node.mapped());
        }
        while (!m_logicalToNative.empty()) {
            auto node = m_logicalToNative.extract(m_logicalToNative.begin());
            secureClear(node.key());
            secureClear(node.mapped());
        }
        while (!m_closeByLogical.empty()) {
            auto node = m_closeByLogical.extract(m_closeByLogical.begin());
            secureClear(node.key());
        }
    }

    bool enqueue(Command command)
    {
        std::lock_guard<std::mutex> lock(m_commandMutex);
        if (!m_connected.load() || m_commands.size() >= DEMO_COMMAND_CAPACITY) {
            return false;
        }
        m_commands.push_back(std::move(command));
        m_commandChanged.notify_one();
        return true;
    }

    void publishHealth(AdapterHealthState state,
                       DemoDiagnostic diagnostic,
                       std::string eventKey)
    {
        AdapterHealthEvent event;
        event.schemaVersion = 1;
        event.state = state;
        event.timestampNs = systemTimestampNs();
        event.sequence = ++m_eventSequence;
        event.failure = diagnostic == DemoDiagnostic::SubscriptionRejected
            ? subscriptionFailureCategory(m_subscriptionFailure.load())
            : failureCategory(diagnostic);
        event.reason = diagnostic == DemoDiagnostic::SubscriptionRejected
            ? demoSubscriptionDiagnostic(m_subscriptionLeg.load(),
                                         m_subscriptionFailure.load())
            : std::string(demoDiagnostic(diagnostic));
        event.eventKey = std::move(eventKey);
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_health = event;
        }
        HealthCallback callback;
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            callback = m_healthCallback;
        }
        if (callback) callback(event);
    }

    void setFailure(DemoDiagnostic diagnostic)
    {
        if (diagnostic != DemoDiagnostic::SubscriptionRejected) {
            m_subscriptionLeg.store(DemoSubscriptionLeg::None);
            m_subscriptionFailure.store(Gate7ResidualFailure::None);
        }
        m_diagnostic.store(diagnostic);
        if (diagnostic != DemoDiagnostic::None) {
            publishHealth(AdapterHealthState::Disconnected, diagnostic,
                          "ctrader-demo-failure-" +
                              std::to_string(m_eventSequence.load() + 1));
        }
    }

    void setSubscriptionFailure(DemoSubscriptionLeg leg,
                                Gate7ResidualFailure failure)
    {
        m_subscriptionLeg.store(leg);
        m_subscriptionFailure.store(
            failure == Gate7ResidualFailure::None
                ? Gate7ResidualFailure::SubscriptionProofRejected
                : failure);
        setFailure(DemoDiagnostic::SubscriptionRejected);
    }

    static std::optional<std::int64_t> rawVolume(Decimal64 quantity) noexcept
    {
        while (quantity.scale < 2) {
            if (quantity.units > std::numeric_limits<std::int64_t>::max() / 10) {
                return std::nullopt;
            }
            quantity.units *= 10;
            ++quantity.scale;
        }
        while (quantity.scale > 2) {
            if (quantity.units % 10 != 0) return std::nullopt;
            quantity.units /= 10;
            --quantity.scale;
        }
        return quantity.units > 0
            ? std::optional<std::int64_t>(quantity.units) : std::nullopt;
    }

    bool selectAccount(StrictTransport& transport,
                       std::string_view accessToken)
    {
        auto accounts = discoverAccounts(transport, accessToken);
        if (!accounts.has_value() || !accounts->tokenOwned) {
            setFailure(DemoDiagnostic::AccountDiscovery);
            return false;
        }
        if (!accounts->tradingScope) {
            setFailure(DemoDiagnostic::TokenScope);
            return false;
        }
        auto selected = selectFiboDemoAccount(*accounts);
        if (!selected.has_value()) {
            setFailure(DemoDiagnostic::AccountSelection);
            return false;
        }
        m_accountId = *selected;
        secureClear(*selected);
        selected.reset();
        std::int64_t responseId = 0;
        if (!authenticateAccount(transport, accessToken,
                                 m_accountId, responseId)
            || responseId != m_accountId) {
            secureClear(responseId);
            setFailure(DemoDiagnostic::AccountAuth);
            return false;
        }
        secureClear(responseId);
        return true;
    }

    bool loadTraderAndCurrency(StrictTransport& transport)
    {
        ProtoOATraderReq traderRequest;
        ProtoOATraderRes traderResponse;
        traderRequest.set_ctidtraderaccountid(m_accountId);
        if (!requestResponse(transport, PROTO_OA_TRADER_REQ,
                             PROTO_OA_TRADER_RES, traderRequest,
                             traderResponse, "demo-trader")
            || traderResponse.ctidtraderaccountid() != m_accountId
            || traderResponse.trader().ctidtraderaccountid() != m_accountId) {
            traderRequest.Clear(); traderResponse.Clear();
            setFailure(DemoDiagnostic::TraderRejected);
            return false;
        }
        const ProtoOATrader& trader = traderResponse.trader();
        const bool allowed = trader.accessrights() == FULL_ACCESS
            && (trader.accounttype() == HEDGED
                || trader.accounttype() == NETTED)
            && trader.accounttype() != SPREAD_BETTING
            && (!trader.has_islimitedrisk() || !trader.islimitedrisk())
            && trader.balance() > 0
            && (!trader.has_moneydigits()
                || trader.moneydigits() <= Decimal64::MAX_SCALE);
        if (!allowed) {
            traderRequest.Clear(); traderResponse.Clear();
            setFailure(DemoDiagnostic::TraderRejected);
            return false;
        }
        m_moneyDigits = trader.has_moneydigits() ? trader.moneydigits() : 2;
        m_depositAssetId = trader.depositassetid();
        m_balanceVersion = trader.has_balanceversion()
            ? static_cast<std::uint64_t>(std::max<std::int64_t>(
                  trader.balanceversion(), 0)) : 0;
        m_balanceRaw = trader.balance();
        traderRequest.Clear(); traderResponse.Clear();

        ProtoOAAssetListReq assetRequest;
        ProtoOAAssetListRes assetResponse;
        assetRequest.set_ctidtraderaccountid(m_accountId);
        if (!requestResponse(transport, PROTO_OA_ASSET_LIST_REQ,
                             PROTO_OA_ASSET_LIST_RES, assetRequest,
                             assetResponse, "demo-assets")
            || assetResponse.ctidtraderaccountid() != m_accountId) {
            assetRequest.Clear(); assetResponse.Clear();
            setFailure(DemoDiagnostic::TraderRejected);
            return false;
        }
        for (const auto& asset : assetResponse.asset()) {
            if (asset.assetid() == m_depositAssetId && boundedText(asset.name(), 32)) {
                m_currency = asset.name();
            }
        }
        assetRequest.Clear(); assetResponse.Clear();
        if (m_currency.empty()) {
            setFailure(DemoDiagnostic::TraderRejected);
            return false;
        }
        return true;
    }

    bool requireInitiallyEmpty(StrictTransport& transport)
    {
        ProtoOAReconcileReq request;
        ProtoOAReconcileRes response;
        request.set_ctidtraderaccountid(m_accountId);
        request.set_returnprotectionorders(true);
        if (!requestResponse(transport, PROTO_OA_RECONCILE_REQ,
                             PROTO_OA_RECONCILE_RES, request, response,
                             "demo-initial-reconcile")
            || response.ctidtraderaccountid() != m_accountId) {
            request.Clear(); response.Clear();
            setFailure(DemoDiagnostic::Reconciliation);
            return false;
        }
        const bool empty = response.position_size() == 0
                        && response.order_size() == 0;
        request.Clear(); response.Clear();
        if (!empty) setFailure(DemoDiagnostic::AccountNotEmpty);
        return empty;
    }

    bool loadInstrument(StrictTransport& transport)
    {
        ProtoOASymbolsListReq listRequest;
        ProtoOASymbolsListRes listResponse;
        listRequest.set_ctidtraderaccountid(m_accountId);
        listRequest.set_includearchivedsymbols(false);
        if (!requestResponse(transport, PROTO_OA_SYMBOLS_LIST_REQ,
                             PROTO_OA_SYMBOLS_LIST_RES, listRequest,
                             listResponse, "demo-symbols")
            || listResponse.ctidtraderaccountid() != m_accountId) {
            listRequest.Clear(); listResponse.Clear();
            setFailure(DemoDiagnostic::InstrumentRejected);
            return false;
        }
        std::vector<std::pair<std::int64_t, std::string>> candidates;
        std::unordered_set<std::int64_t> archived;
        for (const auto& item : listResponse.archivedsymbol()) {
            archived.insert(item.symbolid());
        }
        for (const auto& item : listResponse.symbol()) {
            if (item.has_symbolname() && item.has_enabled() && item.enabled()
                && !archived.contains(item.symbolid())
                && normalizedSymbolName(item.symbolname())
                       == DEMO_CANONICAL_SYMBOL) {
                candidates.emplace_back(item.symbolid(), item.symbolname());
            }
        }
        listRequest.Clear(); listResponse.Clear();
        if (candidates.size() != 1 || candidates.front().first <= 0) {
            setFailure(DemoDiagnostic::InstrumentRejected);
            return false;
        }
        m_symbolId = candidates.front().first;
        m_executionAlias = candidates.front().second;

        ProtoOASymbolByIdReq fullRequest;
        ProtoOASymbolByIdRes fullResponse;
        fullRequest.set_ctidtraderaccountid(m_accountId);
        fullRequest.add_symbolid(m_symbolId);
        if (!requestResponse(transport, PROTO_OA_SYMBOL_BY_ID_REQ,
                             PROTO_OA_SYMBOL_BY_ID_RES, fullRequest,
                             fullResponse, "demo-full-symbol")
            || fullResponse.ctidtraderaccountid() != m_accountId
            || fullResponse.symbol_size() != 1
            || fullResponse.symbol(0).symbolid() != m_symbolId) {
            fullRequest.Clear(); fullResponse.Clear();
            setFailure(DemoDiagnostic::InstrumentRejected);
            return false;
        }
        const ProtoOASymbol& symbol = fullResponse.symbol(0);
        const bool complete = symbol.digits() >= 0
            && symbol.digits() <= Decimal64::MAX_SCALE
            && symbol.has_minvolume() && symbol.minvolume() > 0
            && symbol.has_maxvolume() && symbol.maxvolume() >= symbol.minvolume()
            && symbol.has_stepvolume() && symbol.stepvolume() > 0
            && symbol.minvolume() % symbol.stepvolume() == 0
            && symbol.maxvolume() % symbol.stepvolume() == 0
            && symbol.has_lotsize() && symbol.lotsize() > 0
            && symbol.tradingmode() == ENABLED;
        if (!complete) {
            fullRequest.Clear(); fullResponse.Clear();
            setFailure(DemoDiagnostic::InstrumentRejected);
            return false;
        }
        InstrumentSpec instrument;
        instrument.version = transport.generation();
        instrument.canonicalSymbol = std::string(DEMO_CANONICAL_SYMBOL);
        instrument.executionAlias = m_executionAlias;
        instrument.tickSize = Decimal64{1, static_cast<std::uint8_t>(symbol.digits())};
        instrument.contractSize = Decimal64{symbol.lotsize(), 2};
        instrument.minimumQuantity = Decimal64{symbol.minvolume(), 2};
        instrument.maximumQuantity = Decimal64{symbol.maxvolume(), 2};
        instrument.quantityStep = Decimal64{symbol.stepvolume(), 2};
        instrument.effectiveTimestampNs = systemTimestampNs();
        instrument.tradingEnabled = true;
        instrument.supportsLong = true;
        instrument.supportsShort = symbol.has_enableshortselling()
            && symbol.enableshortselling();
        instrument.complete = true;
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_instrument = instrument;
        }
        fullRequest.Clear(); fullResponse.Clear();
        return true;
    }

    bool loadHistory(StrictTransport& transport)
    {
        const std::int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        const std::int64_t minuteStart = (nowMs / 60000) * 60000;
        if (minuteStart <= 0) {
            setFailure(DemoDiagnostic::HistoricalBarsRejected);
            return false;
        }
        ProtoOAGetTrendbarsReq request;
        ProtoOAGetTrendbarsRes response;
        request.set_ctidtraderaccountid(m_accountId);
        request.set_period(M1);
        request.set_symbolid(m_symbolId);
        request.set_totimestamp(minuteStart - 1);
        request.set_count(100);
        if (!requestResponse(transport, PROTO_OA_GET_TRENDBARS_REQ,
                             PROTO_OA_GET_TRENDBARS_RES, request, response,
                             "demo-history")
            || response.ctidtraderaccountid() != m_accountId
            || response.period() != M1
            || (response.has_symbolid() && response.symbolid() != m_symbolId)) {
            request.Clear(); response.Clear();
            setFailure(DemoDiagnostic::HistoricalBarsRejected);
            return false;
        }
        std::vector<MarketCandle> received;
        received.reserve(static_cast<std::size_t>(response.trendbar_size()));
        for (const auto& bar : response.trendbar()) {
            auto candle = decodeHistoricalTrendbar(bar);
            if (!candle.has_value()) {
                request.Clear(); response.Clear();
                setFailure(DemoDiagnostic::HistoricalBarsRejected);
                return false;
            }
            received.push_back(std::move(*candle));
        }
        request.Clear(); response.Clear();
        auto history = CTraderDemoMarketState::normalizeHistory(
            std::move(received), 100,
            static_cast<std::uint64_t>(minuteStart / 1000));
        if (!history.has_value()) {
            setFailure(DemoDiagnostic::HistoricalBarsRejected);
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_history = std::move(*history);
        }
        return true;
    }

    bool subscribe(StrictTransport& transport)
    {
        ProtoOASubscribeSpotsReq spotRequest;
        ProtoOASubscribeSpotsRes spotResponse;
        Gate7ResidualFailure spotFailure{Gate7ResidualFailure::None};
        spotRequest.set_ctidtraderaccountid(m_accountId);
        spotRequest.add_symbolid(m_symbolId);
        spotRequest.set_subscribetospottimestamp(true);
        if (!liveRequestResponse(transport, PROTO_OA_SUBSCRIBE_SPOTS_REQ,
                                 PROTO_OA_SUBSCRIBE_SPOTS_RES, spotRequest,
                                 spotResponse, "demo-spots", &spotFailure)) {
            spotRequest.Clear(); spotResponse.Clear();
            if (spotFailure != Gate7ResidualFailure::None) {
                setSubscriptionFailure(DemoSubscriptionLeg::Spots, spotFailure);
            }
            return false;
        }
        if (spotResponse.ctidtraderaccountid() != m_accountId) {
            spotRequest.Clear(); spotResponse.Clear();
            setSubscriptionFailure(
                DemoSubscriptionLeg::Spots,
                Gate7ResidualFailure::SubscriptionAccountMismatch);
            return false;
        }
        spotRequest.Clear(); spotResponse.Clear();

        ProtoOASubscribeLiveTrendbarReq barRequest;
        ProtoOASubscribeLiveTrendbarRes barResponse;
        Gate7ResidualFailure barFailure{Gate7ResidualFailure::None};
        barRequest.set_ctidtraderaccountid(m_accountId);
        barRequest.set_symbolid(m_symbolId);
        barRequest.set_period(M1);
        if (!liveRequestResponse(
                transport, PROTO_OA_SUBSCRIBE_LIVE_TRENDBAR_REQ,
                PROTO_OA_SUBSCRIBE_LIVE_TRENDBAR_RES,
                barRequest, barResponse, "demo-live-bars", &barFailure)) {
            barRequest.Clear(); barResponse.Clear();
            if (barFailure != Gate7ResidualFailure::None) {
                setSubscriptionFailure(DemoSubscriptionLeg::LiveM1, barFailure);
            }
            return false;
        }
        if (barResponse.ctidtraderaccountid() != m_accountId) {
            barRequest.Clear(); barResponse.Clear();
            setSubscriptionFailure(
                DemoSubscriptionLeg::LiveM1,
                Gate7ResidualFailure::SubscriptionAccountMismatch);
            return false;
        }
        barRequest.Clear(); barResponse.Clear();
        return true;
    }

    template <typename Request, typename Response>
    bool liveRequestResponse(StrictTransport& transport,
                             std::uint32_t requestType,
                             std::uint32_t responseType,
                             Request& request,
                             Response& response,
                             std::string_view step,
                             Gate7ResidualFailure* subscriptionFailure = nullptr)
    {
        if (subscriptionFailure != nullptr) {
            *subscriptionFailure = Gate7ResidualFailure::None;
        }
        throttle(requestType == PROTO_OA_GET_TRENDBARS_REQ);
        std::string correlation = transport.nextCorrelation(step);
        const Gate7SendOutcome sent = transport.sendDetailed(
            requestType, request, correlation);
        if (sent != Gate7SendOutcome::Sent) {
            if (subscriptionFailure != nullptr) {
                *subscriptionFailure = classifyGate7SubscriptionSendFailure(sent);
            }
            if (recoverableSendOutcome(sent)) {
                m_transportRecoveryRequested = true;
            }
            secureClear(correlation);
            return false;
        }
        const auto deadline = Clock::now() + NETWORK_TIMEOUT;
        while (Clock::now() < deadline && !m_stopRequested.load()) {
            std::uint32_t type = 0;
            std::string receivedCorrelation;
            std::string payload;
            Gate7ProviderErrorCategory category = Gate7ProviderErrorCategory::None;
            const Gate7TransportOutcome outcome = transport.receiveAnyDetailed(
                type, receivedCorrelation, payload, deadline, category);
            if (outcome != Gate7TransportOutcome::Expected) {
                if (subscriptionFailure != nullptr) {
                    *subscriptionFailure =
                        outcome == Gate7TransportOutcome::UnexpectedAllowedPayload
                            || outcome == Gate7TransportOutcome::InboundTypeRejected
                        ? classifyGate7UnexpectedSubscriptionPayload(type)
                        : classifyGate7SubscriptionReceiveFailure(
                              outcome, category);
                }
                if (recoverableTransportOutcome(outcome)) {
                    m_transportRecoveryRequested = true;
                }
                secureClear(correlation); secureClear(receivedCorrelation);
                secureClear(payload);
                if (category == Gate7ProviderErrorCategory::RateLimited) {
                    setFailure(DemoDiagnostic::RateLimited);
                }
                return false;
            }
            if (type == responseType && receivedCorrelation == correlation) {
                const bool valid = response.ParseFromString(payload)
                    && response.IsInitialized();
                if (!valid && subscriptionFailure != nullptr) {
                    *subscriptionFailure =
                        Gate7ResidualFailure::SubscriptionResponseMalformed;
                }
                secureClear(correlation); secureClear(receivedCorrelation);
                secureClear(payload);
                return valid;
            }
            const bool handled = handleIncoming(
                type, receivedCorrelation, payload, transport.generation());
            secureClear(receivedCorrelation); secureClear(payload);
            if (!handled) {
                if (subscriptionFailure != nullptr
                    && m_diagnostic.load() == DemoDiagnostic::None) {
                    *subscriptionFailure =
                        classifyGate7UnexpectedSubscriptionPayload(type);
                }
                secureClear(correlation);
                return false;
            }
        }
        if (subscriptionFailure != nullptr) {
            *subscriptionFailure =
                Gate7ResidualFailure::SubscriptionResponseTimeout;
        }
        secureClear(correlation);
        return false;
    }

    void throttle(bool historical)
    {
        const auto interval = historical
            ? std::chrono::milliseconds(250)
            : std::chrono::milliseconds(25);
        const auto now = Clock::now();
        if (m_nextRequest > now) std::this_thread::sleep_until(m_nextRequest);
        m_nextRequest = Clock::now() + interval;
    }

    ReconciliationSnapshot queryReconciliation(StrictTransport& transport,
                                                std::uint64_t requestedTimestamp)
    {
        ReconciliationSnapshot failed;
        (void)requestedTimestamp;
        failed.timestampNs = systemTimestampNs();
        failed.connectionGeneration = transport.generation();
        failed.status = ReconciliationStatus::Unknown;

        ProtoOATraderReq traderRequest;
        ProtoOATraderRes traderResponse;
        traderRequest.set_ctidtraderaccountid(m_accountId);
        if (!liveRequestResponse(transport, PROTO_OA_TRADER_REQ,
                                 PROTO_OA_TRADER_RES, traderRequest,
                                 traderResponse, "demo-risk-trader")
            || traderResponse.ctidtraderaccountid() != m_accountId
            || traderResponse.trader().ctidtraderaccountid() != m_accountId) {
            traderRequest.Clear(); traderResponse.Clear();
            setFailure(DemoDiagnostic::Reconciliation);
            return failed;
        }

        ProtoOAReconcileReq reconcileRequest;
        ProtoOAReconcileRes reconcileResponse;
        reconcileRequest.set_ctidtraderaccountid(m_accountId);
        reconcileRequest.set_returnprotectionorders(true);
        if (!liveRequestResponse(transport, PROTO_OA_RECONCILE_REQ,
                                 PROTO_OA_RECONCILE_RES, reconcileRequest,
                                 reconcileResponse, "demo-reconcile")
            || reconcileResponse.ctidtraderaccountid() != m_accountId) {
            traderRequest.Clear(); traderResponse.Clear();
            reconcileRequest.Clear(); reconcileResponse.Clear();
            setFailure(DemoDiagnostic::Reconciliation);
            return failed;
        }

        ProtoOAGetPositionUnrealizedPnLReq pnlRequest;
        ProtoOAGetPositionUnrealizedPnLRes pnlResponse;
        pnlRequest.set_ctidtraderaccountid(m_accountId);
        if (!liveRequestResponse(
                transport, PROTO_OA_GET_POSITION_UNREALIZED_PNL_REQ,
                PROTO_OA_GET_POSITION_UNREALIZED_PNL_RES,
                pnlRequest, pnlResponse, "demo-unrealized-pnl")
            || pnlResponse.ctidtraderaccountid() != m_accountId
            || pnlResponse.moneydigits() > Decimal64::MAX_SCALE) {
            traderRequest.Clear(); traderResponse.Clear();
            reconcileRequest.Clear(); reconcileResponse.Clear();
            pnlRequest.Clear(); pnlResponse.Clear();
            setFailure(DemoDiagnostic::Reconciliation);
            return failed;
        }

        ReconciliationSnapshot snapshot;
        snapshot.snapshotVersion = ++m_snapshotVersion;
        snapshot.timestampNs = systemTimestampNs();
        snapshot.connectionGeneration = transport.generation();
        snapshot.pendingOrderCount = static_cast<std::size_t>(
            reconcileResponse.order_size());
        snapshot.status = ReconciliationStatus::Matched;

        std::unordered_map<std::int64_t, std::int64_t> pnlByPosition;
        bool uniquePnlPositions = true;
        for (const auto& pnl : pnlResponse.positionunrealizedpnl()) {
            uniquePnlPositions = pnlByPosition.emplace(
                pnl.positionid(), pnl.netunrealizedpnl()).second
                && uniquePnlPositions;
        }
        if (!uniquePnlPositions
            || pnlByPosition.size() != static_cast<std::size_t>(
                   reconcileResponse.position_size())) {
            traderRequest.Clear(); traderResponse.Clear();
            reconcileRequest.Clear(); reconcileResponse.Clear();
            pnlRequest.Clear(); pnlResponse.Clear();
            setFailure(DemoDiagnostic::Reconciliation);
            return failed;
        }

        const ProtoOATrader& trader = traderResponse.trader();
        const std::uint32_t moneyDigits = trader.has_moneydigits()
            ? trader.moneydigits() : m_moneyDigits;
        auto balance = moneyDecimal(trader.balance(), moneyDigits);
        Decimal64 unrealized{0, static_cast<std::uint8_t>(moneyDigits)};
        Decimal64 marginUsed{0, static_cast<std::uint8_t>(moneyDigits)};

        std::optional<std::uint64_t> adoptableEntry;
        if (reconcileResponse.position_size() == 1
            && reconcileResponse.order_size() == 0) {
            for (const auto& [localId, local] : m_orders) {
                if (local.order.request.positionEffect != PositionEffect::Open
                    || local.exposureAdoptedByReconciliation) {
                    continue;
                }
                if (adoptableEntry.has_value()) {
                    adoptableEntry.reset();
                    break;
                }
                adoptableEntry = localId;
            }
        }

        for (const auto& native : reconcileResponse.position()) {
            const auto tradeSide = native.tradedata().tradeside();
            if (!native.IsInitialized()
                || native.positionstatus() != POSITION_STATUS_OPEN
                || native.positionid() <= 0
                || native.tradedata().symbolid() != m_symbolId
                || native.tradedata().volume() <= 0
                || (tradeSide != BUY && tradeSide != SELL)
                || !native.has_price() || native.price() <= 0.0) {
                snapshot.status = ReconciliationStatus::ExternalOnly;
                snapshot.complete = false;
                break;
            }
            auto logical = m_nativeToLogical.find(native.positionid());
            if (logical == m_nativeToLogical.end()
                && adoptableEntry.has_value()
                && native.tradedata().has_label()
                && native.tradedata().label() == DEMO_ORDER_LABEL) {
                auto local = m_orders.find(*adoptableEntry);
                const PositionSide nativeSide =
                    tradeSide == BUY
                        ? PositionSide::Long : PositionSide::Short;
                if (local != m_orders.end()
                    && native.tradedata().volume() > 0
                    && native.tradedata().volume()
                           <= local->second.requestedVolume
                    && local->second.order.request.positionSide == nativeSide) {
                    const std::string adopted = "mynyra-position-"
                                              + std::to_string(*adoptableEntry);
                    m_nativeToLogical[native.positionid()] = adopted;
                    m_logicalToNative[adopted] = native.positionid();
                    local->second.logicalPositionId = adopted;
                    local->second.exposureAdoptedByReconciliation = true;
                    logical = m_nativeToLogical.find(native.positionid());
                }
            }
            const auto pnl = pnlByPosition.find(native.positionid());
            if (logical == m_nativeToLogical.end() || pnl == pnlByPosition.end()) {
                snapshot.status = ReconciliationStatus::ExternalOnly;
                snapshot.complete = false;
                break;
            }
            PositionSnapshot position;
            position.canonicalSymbol = std::string(DEMO_CANONICAL_SYMBOL);
            position.quantity = Decimal64{native.tradedata().volume(), 2};
            position.averagePrice = Decimal64::fromDouble(
                native.price(), 8, DecimalRounding::NearestTiesAwayFromZero)
                .value_or(Decimal64{});
            position.logicalPositionId = logical->second;
            position.side = tradeSide == BUY
                ? PositionSide::Long : PositionSide::Short;
            if (!position.averagePrice.isPositive()) {
                snapshot.status = ReconciliationStatus::PriceOrCostMismatch;
                snapshot.complete = false;
                break;
            }
            snapshot.positions.push_back(std::move(position));

            auto nativePnl = moneyDecimal(
                pnl->second, pnlResponse.moneydigits());
            auto accumulatedPnl = nativePnl.has_value()
                ? addDecimal(unrealized, *nativePnl) : std::nullopt;
            if (!accumulatedPnl.has_value()) {
                snapshot.status = ReconciliationStatus::PriceOrCostMismatch;
                snapshot.complete = false;
                break;
            }
            unrealized = *accumulatedPnl;

            if (!native.has_usedmargin()) {
                snapshot.status = ReconciliationStatus::PriceOrCostMismatch;
                snapshot.complete = false;
                break;
            }
            const std::uint32_t positionDigits = native.has_moneydigits()
                ? native.moneydigits() : moneyDigits;
            auto used = unsignedMoneyDecimal(native.usedmargin(), positionDigits);
            auto accumulatedMargin = used.has_value()
                ? addDecimal(marginUsed, *used) : std::nullopt;
            if (!accumulatedMargin.has_value()) {
                snapshot.status = ReconciliationStatus::PriceOrCostMismatch;
                snapshot.complete = false;
                break;
            }
            marginUsed = *accumulatedMargin;
        }

        for (const auto& nativeOrder : reconcileResponse.order()) {
            if (!nativeOrder.has_clientorderid()) {
                snapshot.status = ReconciliationStatus::ExternalOnly;
                snapshot.complete = false;
                break;
            }
            const auto local = m_clientOrderToLocal.find(
                nativeOrder.clientorderid());
            if (local == m_clientOrderToLocal.end()) {
                snapshot.status = ReconciliationStatus::ExternalOnly;
                snapshot.complete = false;
                break;
            }
            snapshot.orderStates[local->second] = OrderLifecycleState::Accepted;
            const auto orderState = m_orders.find(local->second);
            if (orderState != m_orders.end()) {
                snapshot.orderEffects[local->second] =
                    orderState->second.order.request.positionEffect;
            }
        }

        auto equity = balance.has_value()
            ? addDecimal(*balance, unrealized) : std::nullopt;
        Decimal64 negativeMargin = marginUsed;
        negativeMargin.units = -negativeMargin.units;
        auto freeMargin = equity.has_value()
            ? addDecimal(*equity, negativeMargin) : std::nullopt;
        if (snapshot.status == ReconciliationStatus::Matched
            && balance.has_value() && equity.has_value()
            && freeMargin.has_value()) {
            snapshot.account.schemaVersion = 1;
            snapshot.account.snapshotVersion = snapshot.snapshotVersion;
            snapshot.account.balance = *balance;
            snapshot.account.equity = *equity;
            snapshot.account.realizedPnl = Decimal64{0, balance->scale};
            snapshot.account.unrealizedPnl = unrealized;
            snapshot.account.marginUsed = marginUsed;
            snapshot.account.freeMargin = *freeMargin;
            snapshot.account.currency = m_currency;
            snapshot.account.sourceTimestampNs = snapshot.timestampNs;
            snapshot.account.ingestionTimestampNs = systemTimestampNs();
            snapshot.account.complete = true;
            snapshot.complete = true;
        }

        traderRequest.Clear(); traderResponse.Clear();
        reconcileRequest.Clear(); reconcileResponse.Clear();
        pnlRequest.Clear(); pnlResponse.Clear();
        if (!snapshot.complete) {
            setFailure(DemoDiagnostic::Reconciliation);
            return snapshot;
        }
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_account = snapshot.account;
        }
        return snapshot;
    }

    std::optional<Decimal64> queryExpectedMargin(
        StrictTransport& transport, PositionSide direction)
    {
        InstrumentSpec instrument;
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            instrument = m_instrument;
        }
        const auto volume = rawVolume(instrument.minimumQuantity);
        if (!volume.has_value()) return std::nullopt;
        ProtoOAExpectedMarginReq request;
        ProtoOAExpectedMarginRes response;
        request.set_ctidtraderaccountid(m_accountId);
        request.set_symbolid(m_symbolId);
        request.add_volume(*volume);
        if (!liveRequestResponse(transport, PROTO_OA_EXPECTED_MARGIN_REQ,
                                 PROTO_OA_EXPECTED_MARGIN_RES, request,
                                 response, "demo-expected-margin")
            || response.ctidtraderaccountid() != m_accountId
            || response.margin_size() != 1
            || response.margin(0).volume() != *volume
            || (response.has_moneydigits()
                && response.moneydigits() > Decimal64::MAX_SCALE)) {
            request.Clear(); response.Clear();
            setFailure(DemoDiagnostic::ExpectedMargin);
            return std::nullopt;
        }
        const std::uint32_t digits = response.has_moneydigits()
            ? response.moneydigits() : m_moneyDigits;
        const std::int64_t units = direction == PositionSide::Long
            ? response.margin(0).buymargin()
            : response.margin(0).sellmargin();
        auto margin = moneyDecimal(units, digits);
        request.Clear(); response.Clear();
        if (!margin.has_value() || !margin->isPositive()) {
            setFailure(DemoDiagnostic::ExpectedMargin);
            return std::nullopt;
        }
        return margin;
    }

    std::optional<OrderRiskContext> buildRiskContext(
        StrictTransport& transport, PositionSide direction)
    {
        ReconciliationSnapshot reconciliation = queryReconciliation(
            transport, systemTimestampNs());
        if (!reconciliation.complete) return std::nullopt;
        auto expected = queryExpectedMargin(transport, direction);
        if (!expected.has_value()) return std::nullopt;

        OrderRiskContext context;
        context.account = reconciliation.account;
        context.reconciliation = reconciliation;
        context.expectedMargin = *expected;
        context.expectedMarginSide = direction;
        context.connectionGeneration = transport.generation();
        context.sameGeneration = reconciliation.connectionGeneration
                              == transport.generation();
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            context.instrument = m_instrument;
            context.instrumentVersion = m_instrument.version;
            const auto bbo = m_marketState.bbo();
            if (bbo.has_value()) {
                context.bid = bbo->bid;
                context.ask = bbo->ask;
                context.bboSourceTimestampNs = bbo->sourceTimestampNs;
                context.bboComplete = true;
            }
        }
        double gross = 0.0;
        for (const auto& position : reconciliation.positions) {
            gross += position.quantity.toDouble()
                   * position.averagePrice.toDouble();
        }
        const auto grossExposure = Decimal64::fromDouble(
            gross, 8, DecimalRounding::NearestTiesAwayFromZero);
        if (!grossExposure.has_value() || grossExposure->isNegative()) {
            setFailure(DemoDiagnostic::Reconciliation);
            return std::nullopt;
        }
        context.grossExposure = *grossExposure;
        context.evaluationTimestampNs = systemTimestampNs();
        return context;
    }

    bool submitOrder(StrictTransport& transport,
                     const NormalizedOrder& normalized)
    {
        const auto volume = rawVolume(normalized.normalizedQuantity);
        if (!volume.has_value() || normalized.request.localOrderId == 0
            || normalized.request.canonicalSymbol != DEMO_CANONICAL_SYMBOL
            || normalized.instrumentVersion == 0) {
            return false;
        }
        LocalOrderState state;
        state.order = normalized;
        state.requestedVolume = *volume;
        state.syntheticExternalOrderId =
            "ctrader-demo-order-" +
            std::to_string(normalized.request.localOrderId);
        state.logicalPositionId = normalized.request.logicalPositionId;
        const std::string clientOrderId =
            "mynyra-" + std::to_string(normalized.request.localOrderId);
        const std::string correlation = transport.nextCorrelation(
            normalized.request.positionEffect == PositionEffect::Open
                ? "demo-entry" : "demo-close");

        Gate7SendOutcome sendOutcome = Gate7SendOutcome::PayloadRejected;
        throttle(false);
        if (normalized.request.positionEffect == PositionEffect::Open) {
            ProtoOANewOrderReq request;
            request.set_ctidtraderaccountid(m_accountId);
            request.set_symbolid(m_symbolId);
            request.set_ordertype(MARKET);
            request.set_tradeside(normalized.request.side == OrderSide::Buy
                                      ? BUY : SELL);
            request.set_volume(*volume);
            request.set_clientorderid(clientOrderId);
            request.set_label(DEMO_ORDER_LABEL.data(), DEMO_ORDER_LABEL.size());
            sendOutcome = transport.sendDetailed(
                PROTO_OA_NEW_ORDER_REQ, request, correlation);
            request.Clear();
        } else if (normalized.request.positionEffect == PositionEffect::Close
                   && normalized.request.logicalPositionId.has_value()) {
            const auto native = m_logicalToNative.find(
                *normalized.request.logicalPositionId);
            if (native == m_logicalToNative.end()) return false;
            ProtoOAClosePositionReq request;
            request.set_ctidtraderaccountid(m_accountId);
            request.set_positionid(native->second);
            request.set_volume(*volume);
            sendOutcome = transport.sendDetailed(
                PROTO_OA_CLOSE_POSITION_REQ, request, correlation);
            request.Clear();
        }
        const bool ambiguousSend = recoverableSendOutcome(sendOutcome);
        if (sendOutcome == Gate7SendOutcome::Sent || ambiguousSend) {
            m_orders[normalized.request.localOrderId] = std::move(state);
            m_clientOrderToLocal[clientOrderId] = normalized.request.localOrderId;
            m_correlationToLocal[correlation] = normalized.request.localOrderId;
            if (normalized.request.positionEffect == PositionEffect::Close
                && normalized.request.logicalPositionId.has_value()) {
                m_closeByLogical[*normalized.request.logicalPositionId] =
                    normalized.request.localOrderId;
            }
        }
        if (sendOutcome != Gate7SendOutcome::Sent) {
            m_transportRecoveryRequested = ambiguousSend;
            return false;
        }
        return true;
    }

    bool handleSpot(const std::string& payload,
                    std::uint64_t generation)
    {
        if (generation != m_connectionGeneration.load()) return false;
        ProtoOASpotEvent event;
        if (!event.ParseFromString(payload) || !event.IsInitialized()) {
            event.Clear();
            setFailure(DemoDiagnostic::SpotEnvelopeMalformed);
            return false;
        }
        if (event.ctidtraderaccountid() != m_accountId) {
            event.Clear();
            setFailure(DemoDiagnostic::SpotAccountMismatch);
            return false;
        }
        if (event.symbolid() != m_symbolId) {
            event.Clear();
            setFailure(DemoDiagnostic::SpotSymbolMismatch);
            return false;
        }
        const std::uint64_t receipt = systemTimestampNs();
        std::optional<std::uint64_t> timestamp;
        if (event.has_timestamp()) {
            const auto proof = CTraderGate7Proof::classifyTimestamp(
                event.timestamp(), receipt);
            if (proof.has_value()) timestamp = proof->timestampNs;
        }

        CTraderDemoSpotUpdate update;
        update.sourceTimestampNs = timestamp;
        if (event.has_bid()) {
            if (event.bid() == 0
                || event.bid() > static_cast<std::uint64_t>(
                                  std::numeric_limits<std::int64_t>::max())) {
                event.Clear();
                setFailure(DemoDiagnostic::SpotBidMalformed);
                return false;
            }
            update.bid = Decimal64{
                static_cast<std::int64_t>(event.bid()), 5};
        }
        if (event.has_ask()) {
            if (event.ask() == 0
                || event.ask() > static_cast<std::uint64_t>(
                                  std::numeric_limits<std::int64_t>::max())) {
                event.Clear();
                setFailure(DemoDiagnostic::SpotAskMalformed);
                return false;
            }
            update.ask = Decimal64{
                static_cast<std::int64_t>(event.ask()), 5};
        }
        update.liveM1Bars.reserve(
            static_cast<std::size_t>(event.trendbar_size()));
        for (const auto& trendbar : event.trendbar()) {
            if (trendbar.period() != M1) {
                event.Clear();
                setFailure(DemoDiagnostic::SpotTrendbarPeriodMismatch);
                return false;
            }
            DemoDiagnostic trendbarFailure{DemoDiagnostic::None};
            auto candle = decodeTrendbar(trendbar, &trendbarFailure);
            if (!candle.has_value()) {
                event.Clear();
                setFailure(trendbarFailure == DemoDiagnostic::None
                    ? DemoDiagnostic::SpotTrendbarMalformed
                    : trendbarFailure);
                return false;
            }
            update.liveM1Bars.push_back(std::move(*candle));
        }

        MarketDataCallback marketCallback;
        std::optional<MarketDataEvent> marketEvent;
        CTraderDemoMarketState::ApplyResult applied =
            CTraderDemoMarketState::ApplyResult::Malformed;
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            const bool hadCompleted = m_marketState.hasCompletedCandle();
            applied = m_marketState.apply(update);
            const auto bbo = m_marketState.bbo();
            if (applied == CTraderDemoMarketState::ApplyResult::Applied
                && (update.bid.has_value() || update.ask.has_value())
                && bbo.has_value()) {
                MarketDataEvent normalized;
                normalized.canonicalSymbol = std::string(DEMO_CANONICAL_SYMBOL);
                normalized.bid = bbo->bid;
                normalized.ask = bbo->ask;
                normalized.sourceTimestampNs = bbo->sourceTimestampNs;
                normalized.sequence = ++m_marketSequence;
                normalized.instrumentVersion = m_instrument.version;
                normalized.quality = AdapterHealthState::Connected;
                normalized.eventKey = "demo-bbo-"
                    + std::to_string(normalized.sequence);
                marketEvent = std::move(normalized);
            }
            if (!hadCompleted && m_marketState.hasCompletedCandle()) {
                m_candleChanged.notify_all();
            }
        }
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            marketCallback = m_marketData;
        }
        if (marketEvent.has_value() && marketCallback) {
            marketCallback(*marketEvent);
        }
        event.set_ctidtraderaccountid(0);
        event.set_symbolid(0);
        event.Clear();
        if (applied != CTraderDemoMarketState::ApplyResult::Applied) {
            setFailure(applied == CTraderDemoMarketState::ApplyResult::QueueOverflow
                ? DemoDiagnostic::ResourceExhausted
                : DemoDiagnostic::SpotMarketStateMalformed);
            return false;
        }
        return true;
    }

    std::optional<std::uint64_t> localOrderForExecution(
        const ProtoOAExecutionEvent& event,
        const std::string& correlation) const
    {
        std::optional<std::uint64_t> resolved;
        if (!correlation.empty()) {
            const auto correlated = m_correlationToLocal.find(correlation);
            if (correlated == m_correlationToLocal.end()
                || !mergeLocalIdentity(resolved, correlated->second)) {
                return std::nullopt;
            }
        }
        if (event.has_order() && event.order().has_clientorderid()) {
            const auto client = m_clientOrderToLocal.find(
                event.order().clientorderid());
            if (client == m_clientOrderToLocal.end()
                || !mergeLocalIdentity(resolved, client->second)) {
                return std::nullopt;
            }
        }
        std::optional<std::int64_t> nativePosition;
        if (event.has_position()) nativePosition = event.position().positionid();
        else if (event.has_deal()) nativePosition = event.deal().positionid();
        if (nativePosition.has_value()) {
            const auto logical = m_nativeToLogical.find(*nativePosition);
            if (logical != m_nativeToLogical.end()) {
                const auto close = m_closeByLogical.find(logical->second);
                if (close != m_closeByLogical.end()) {
                    if (!mergeLocalIdentity(resolved, close->second)) {
                        return std::nullopt;
                    }
                } else {
                    const auto opening = std::find_if(
                        m_orders.begin(), m_orders.end(),
                        [&logical](const auto& item) {
                            return item.second.logicalPositionId.has_value()
                                && *item.second.logicalPositionId
                                       == logical->second;
                        });
                    if (opening == m_orders.end()
                        || !mergeLocalIdentity(resolved, opening->first)) {
                        return std::nullopt;
                    }
                }
            } else if (resolved.has_value()) {
                const auto local = m_orders.find(*resolved);
                if (local == m_orders.end()
                    || local->second.order.request.positionEffect
                           != PositionEffect::Open) {
                    return std::nullopt;
                }
            }
        }
        return resolved;
    }

    bool handleExecutionEvent(const std::string& payload,
                              const std::string& correlation)
    {
        ProtoOAExecutionEvent provider;
        if (!provider.ParseFromString(payload) || !provider.IsInitialized()
            || provider.ctidtraderaccountid() != m_accountId) {
            provider.Clear();
            setFailure(DemoDiagnostic::MalformedProviderEvent);
            return false;
        }
        const auto localId = localOrderForExecution(provider, correlation);
        if (!localId.has_value()) {
            // Ignore unrelated server-side balance/swap events, but never an
            // order lifecycle event for this account.
            const bool nonOrder = provider.executiontype() == SWAP
                || provider.executiontype() == DEPOSIT_WITHDRAW
                || provider.executiontype() == BONUS_DEPOSIT_WITHDRAW;
            provider.Clear();
            if (!nonOrder) setFailure(DemoDiagnostic::MalformedProviderEvent);
            return nonOrder;
        }
        const auto stateIt = m_orders.find(*localId);
        if (stateIt == m_orders.end()) {
            provider.Clear();
            setFailure(DemoDiagnostic::MalformedProviderEvent);
            return false;
        }
        LocalOrderState& state = stateIt->second;
        const auto executionType = provider.executiontype();

        if (executionType == ORDER_ACCEPTED || executionType == ORDER_REJECTED) {
            OrderAcknowledgement acknowledgement;
            acknowledgement.localOrderId = *localId;
            acknowledgement.externalOrderId = state.syntheticExternalOrderId;
            acknowledgement.accepted = executionType == ORDER_ACCEPTED;
            acknowledgement.failure = acknowledgement.accepted
                ? FailureCategory::None : FailureCategory::Rejected;
            acknowledgement.reason = acknowledgement.accepted
                ? "ctrader_demo_order_accepted"
                : "ctrader_demo_order_rejected";
            acknowledgement.timestampNs = systemTimestampNs();
            acknowledgement.sequence = ++m_providerSequence;
            acknowledgement.eventKey = "demo-ack-"
                + std::to_string(*localId) + '-'
                + std::to_string(acknowledgement.sequence);
            state.accepted = acknowledgement.accepted;
            AcknowledgementCallback callback;
            {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                callback = m_acknowledgement;
            }
            provider.Clear();
            if (callback) callback(acknowledgement);
            if (!acknowledgement.accepted) {
                m_diagnostic.store(DemoDiagnostic::OrderRejected);
            }
            return true;
        }

        if (executionType != ORDER_FILLED
            && executionType != ORDER_PARTIAL_FILL) {
            provider.Clear();
            return true;
        }
        if (state.exposureAdoptedByReconciliation) {
            // Reconciliation already established the economic position. A
            // delayed provider fill must not apply the same exposure twice.
            provider.Clear();
            return true;
        }
        if (!provider.has_deal() || !provider.deal().has_executionprice()
            || provider.deal().executionprice() <= 0.0) {
            provider.Clear();
            setFailure(DemoDiagnostic::MalformedProviderEvent);
            return false;
        }

        std::int64_t cumulative = 0;
        if (provider.has_order() && provider.order().has_executedvolume()) {
            cumulative = provider.order().executedvolume();
        } else if (provider.deal().filledvolume() > 0
                   && state.cumulativeVolume
                       <= std::numeric_limits<std::int64_t>::max()
                          - provider.deal().filledvolume()) {
            cumulative = state.cumulativeVolume + provider.deal().filledvolume();
        }
        if (cumulative <= state.cumulativeVolume
            || !providerFillKindMatches(
                executionType, cumulative, state.requestedVolume)) {
            provider.Clear();
            setFailure(DemoDiagnostic::MalformedProviderEvent);
            return false;
        }

        if (provider.has_position()) {
            const std::int64_t nativePosition = provider.position().positionid();
            if (state.order.request.positionEffect == PositionEffect::Open) {
                const std::string logical = "mynyra-position-"
                                          + std::to_string(*localId);
                m_nativeToLogical[nativePosition] = logical;
                m_logicalToNative[logical] = nativePosition;
                state.logicalPositionId = logical;
            }
        }

        std::int64_t feeUnits = 0;
        std::uint32_t feeDigits = m_moneyDigits;
        if (provider.deal().has_commission()) {
            if (provider.deal().commission()
                == std::numeric_limits<std::int64_t>::min()) {
                provider.Clear();
                setFailure(DemoDiagnostic::MalformedProviderEvent);
                return false;
            }
            feeUnits = std::abs(provider.deal().commission());
            if (provider.deal().has_moneydigits()) {
                feeDigits = provider.deal().moneydigits();
            }
        }
        auto fee = moneyDecimal(feeUnits, feeDigits);
        auto price = Decimal64::fromDouble(
            provider.deal().executionprice(), 8,
            DecimalRounding::NearestTiesAwayFromZero);
        if (!fee.has_value() || !price.has_value() || !price->isPositive()) {
            provider.Clear();
            setFailure(DemoDiagnostic::MalformedProviderEvent);
            return false;
        }
        ExecutionEvent execution;
        execution.localOrderId = *localId;
        execution.externalOrderId = state.syntheticExternalOrderId;
        execution.cumulativeFilledQuantity = Decimal64{cumulative, 2};
        execution.remainingQuantity = Decimal64{
            state.requestedVolume - cumulative, 2};
        execution.fillPrice = *price;
        execution.fee = *fee;
        if (provider.deal().executiontimestamp() > 0
            && static_cast<std::uint64_t>(provider.deal().executiontimestamp())
               <= std::numeric_limits<std::uint64_t>::max() / 1'000'000ULL) {
            execution.timestampNs =
                static_cast<std::uint64_t>(provider.deal().executiontimestamp())
                * 1'000'000ULL;
        } else {
            execution.timestampNs = systemTimestampNs();
        }
        execution.sequence = ++m_providerSequence;
        execution.eventKey = "demo-fill-" + std::to_string(*localId)
            + '-' + std::to_string(cumulative);
        execution.positionSide = state.order.request.positionSide;
        execution.positionEffect = state.order.request.positionEffect;
        execution.acceptanceImpliedByFill = !state.accepted;
        state.accepted = true;
        state.cumulativeVolume = cumulative;
        ExecutionCallback callback;
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            callback = m_execution;
        }
        provider.Clear();
        if (callback) callback(execution);
        return true;
    }

    bool handleOrderError(const std::string& payload,
                          const std::string& correlation)
    {
        ProtoOAOrderErrorEvent provider;
        if (!provider.ParseFromString(payload) || !provider.IsInitialized()
            || provider.ctidtraderaccountid() != m_accountId) {
            provider.Clear();
            setFailure(DemoDiagnostic::MalformedProviderEvent);
            return false;
        }
        const auto correlated = m_correlationToLocal.find(correlation);
        if (correlated == m_correlationToLocal.end()) {
            provider.Clear();
            setFailure(DemoDiagnostic::MalformedProviderEvent);
            return false;
        }
        const auto state = m_orders.find(correlated->second);
        if (state == m_orders.end()) {
            provider.Clear();
            setFailure(DemoDiagnostic::MalformedProviderEvent);
            return false;
        }
        OrderAcknowledgement acknowledgement;
        acknowledgement.localOrderId = correlated->second;
        acknowledgement.externalOrderId = state->second.syntheticExternalOrderId;
        acknowledgement.accepted = false;
        acknowledgement.failure = FailureCategory::Rejected;
        acknowledgement.reason = "ctrader_demo_order_error";
        acknowledgement.timestampNs = systemTimestampNs();
        acknowledgement.sequence = ++m_providerSequence;
        acknowledgement.eventKey = "demo-order-error-"
            + std::to_string(correlated->second) + '-'
            + std::to_string(acknowledgement.sequence);
        AcknowledgementCallback callback;
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            callback = m_acknowledgement;
        }
        if (provider.has_errorcode()) secureClear(*provider.mutable_errorcode());
        if (provider.has_description()) secureClear(*provider.mutable_description());
        provider.set_ctidtraderaccountid(0);
        provider.Clear();
        m_diagnostic.store(DemoDiagnostic::OrderRejected);
        if (callback) callback(acknowledgement);
        return true;
    }

    bool handleIncoming(std::uint32_t type,
                        const std::string& correlation,
                        const std::string& payload,
                        std::uint64_t generation)
    {
        if (generation != m_connectionGeneration.load()) return false;
        switch (type) {
            case PROTO_OA_SPOT_EVENT:
                return handleSpot(payload, generation);
            case PROTO_OA_EXECUTION_EVENT:
                return handleExecutionEvent(payload, correlation);
            case PROTO_OA_ORDER_ERROR_EVENT:
                return handleOrderError(payload, correlation);
            case PROTO_OA_TRADER_UPDATE_EVENT:
            case PROTO_OA_MARGIN_CHANGED_EVENT:
                // Authoritative values are pulled in one coherent generation
                // by the next reconciliation; asynchronous deltas are not mixed.
                return true;
            default:
                return false;
        }
    }

    bool recoverTransport(StrictTransport& transport,
                          std::string_view clientId,
                          std::string_view clientSecret,
                          std::string_view accessToken)
    {
        publishHealth(AdapterHealthState::Degraded,
                      DemoDiagnostic::Transport,
                      "ctrader-demo-reconnecting-"
                          + std::to_string(m_eventSequence.load() + 1));
        std::int64_t selectedAccount = m_accountId;
        for (std::size_t attempt = 0;
             attempt < DEMO_RECONNECT_ATTEMPTS && !m_stopRequested.load();
             ++attempt) {
            if (attempt > 0) {
                std::this_thread::sleep_for(DEMO_RECONNECT_BACKOFF);
            }
            m_transportRecoveryRequested = false;
            setFailure(DemoDiagnostic::None);
            if (!transport.reconnectDemo()) continue;
            m_connectionGeneration.store(transport.generation());
            if (!applicationAuthenticate(transport, clientId, clientSecret)) {
                continue;
            }
            if (!selectAccount(transport, accessToken)
                || m_accountId != selectedAccount) {
                continue;
            }
            secureClear(m_currency);
            if (!loadTraderAndCurrency(transport)
                || !loadInstrument(transport)) {
                continue;
            }
            {
                std::lock_guard<std::mutex> lock(m_stateMutex);
                m_marketState.resetLiveGeneration();
            }
            if (!subscribe(transport)) continue;
            ReconciliationSnapshot recovered = queryReconciliation(
                transport, systemTimestampNs());
            if (!recovered.complete
                || recovered.connectionGeneration != transport.generation()) {
                continue;
            }
            setFailure(DemoDiagnostic::None);
            publishHealth(AdapterHealthState::Connected,
                          DemoDiagnostic::None,
                          "ctrader-demo-reconnected-"
                              + std::to_string(attempt + 1));
            secureClear(selectedAccount);
            return true;
        }
        secureClear(selectedAccount);
        return false;
    }

    void processCommand(StrictTransport& transport,
                        std::string_view clientId,
                        std::string_view clientSecret,
                        std::string_view accessToken,
                        Command command)
    {
        m_transportRecoveryRequested = false;
        switch (command.type) {
            case CommandType::Submit: {
                const bool result = submitOrder(transport, command.order);
                if (m_transportRecoveryRequested) {
                    // A commissioning order is never retried. Reconnect only
                    // to make the subsequent authoritative reconciliation
                    // possible.
                    (void)recoverTransport(
                        transport, clientId, clientSecret, accessToken);
                }
                if (command.boolResult) command.boolResult->set_value(result);
                break;
            }
            case CommandType::Reconcile: {
                auto result = queryReconciliation(
                    transport, command.timestampNs);
                if (m_transportRecoveryRequested
                    && recoverTransport(
                        transport, clientId, clientSecret, accessToken)) {
                    m_transportRecoveryRequested = false;
                    result = queryReconciliation(
                        transport, command.timestampNs);
                }
                if (command.reconciliation) {
                    command.reconciliation->set_value(std::move(result));
                }
                break;
            }
            case CommandType::RiskContext: {
                auto result = buildRiskContext(transport, command.direction);
                if (m_transportRecoveryRequested
                    && recoverTransport(
                        transport, clientId, clientSecret, accessToken)) {
                    m_transportRecoveryRequested = false;
                    result = buildRiskContext(transport, command.direction);
                }
                if (command.risk) command.risk->set_value(std::move(result));
                break;
            }
        }
    }

    void failPendingCommands()
    {
        std::deque<Command> pending;
        {
            std::lock_guard<std::mutex> lock(m_commandMutex);
            pending.swap(m_commands);
        }
        for (auto& command : pending) {
            try {
                if (command.boolResult) command.boolResult->set_value(false);
                if (command.reconciliation) {
                    ReconciliationSnapshot snapshot;
                    snapshot.timestampNs = command.timestampNs;
                    snapshot.status = ReconciliationStatus::Unsupported;
                    command.reconciliation->set_value(std::move(snapshot));
                }
                if (command.risk) command.risk->set_value(std::nullopt);
            } catch (...) {
            }
        }
    }

    void run(bool freshOAuth, std::shared_ptr<std::promise<bool>> ready)
    {
        bool startupCompleted = false;
        const auto finishStartup = [&](bool result) {
            if (!startupCompleted) {
                startupCompleted = true;
                try { ready->set_value(result); } catch (...) {}
            }
        };
        if (!disableCoreDumps()) {
            setFailure(DemoDiagnostic::Configuration);
            finishStartup(false);
            return;
        }
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
            setFailure(DemoDiagnostic::TokenTransport);
            finishStartup(false);
            return;
        }
        CurlGlobalScope curlScope;
        auto clientId = loadClientId();
        if (!clientId.has_value()) {
            setFailure(DemoDiagnostic::ClientIdMissing);
            finishStartup(false);
            return;
        }
        Sensitive clientSecret;
        const RuntimeFailure secretRead = readKeychainValue(
            CTraderGate7Config::CLIENT_SECRET_SERVICE, clientSecret);
        if (secretRead != RuntimeFailure::None) {
            setFailure(secretRead == RuntimeFailure::TokenUnavailable
                ? DemoDiagnostic::ClientSecretMissing
                : DemoDiagnostic::KeychainRead);
            clientId->clear(); clientId.reset();
            finishStartup(false);
            return;
        }
        TokenEnvelope token;
        bool tokenPersistenceRequired = false;
        DemoDiagnostic tokenResult = acquireDemoToken(
            freshOAuth, clientId->view(), clientSecret.view(), token,
            tokenPersistenceRequired);
        if (tokenResult != DemoDiagnostic::None) {
            setFailure(tokenResult);
            clearToken(token); clientSecret.clear();
            clientId->clear(); clientId.reset();
            finishStartup(false);
            return;
        }

        StrictTransport transport(true);
        if (!transport.connectDemo()) {
            setFailure(DemoDiagnostic::Tls);
            clearToken(token); clientSecret.clear();
            clientId->clear(); clientId.reset();
            finishStartup(false);
            return;
        }
        m_connectionGeneration.store(transport.generation());
        const auto persistValidatedToken = [&] {
            if (!tokenPersistenceRequired) return true;
            const DemoDiagnostic persisted = persistToken(token);
            if (persisted != DemoDiagnostic::None) {
                setFailure(persisted);
                return false;
            }
            tokenPersistenceRequired = false;
            return true;
        };
        if (!applicationAuthenticate(
                transport, clientId->view(), clientSecret.view())) {
            setFailure(DemoDiagnostic::ApplicationAuth);
        } else if (!selectAccount(transport, token.accessToken.view())) {
        } else if (!loadTraderAndCurrency(transport)) {
        } else if (!persistValidatedToken()) {
        } else if (!requireInitiallyEmpty(transport)) {
        } else if (!loadInstrument(transport)) {
        } else if (!loadHistory(transport)) {
        } else if (!subscribe(transport)) {
        } else {
            AccountSnapshot account;
            account.snapshotVersion = ++m_snapshotVersion;
            account.balance = moneyDecimal(m_balanceRaw, m_moneyDigits)
                .value_or(Decimal64{});
            account.equity = account.balance;
            account.realizedPnl = Decimal64{0, account.balance.scale};
            account.unrealizedPnl = Decimal64{0, account.balance.scale};
            account.marginUsed = Decimal64{0, account.balance.scale};
            account.freeMargin = account.balance;
            account.currency = m_currency;
            account.sourceTimestampNs = systemTimestampNs();
            account.ingestionTimestampNs = account.sourceTimestampNs;
            account.complete = account.balance.isPositive();
            {
                std::lock_guard<std::mutex> lock(m_stateMutex);
                m_account = account;
            }
            setFailure(DemoDiagnostic::None);
            m_connected.store(true);
            publishHealth(AdapterHealthState::Connected,
                          DemoDiagnostic::None,
                          "ctrader-demo-connected");
            finishStartup(true);

            while (!m_stopRequested.load()) {
                std::optional<Command> command;
                {
                    std::lock_guard<std::mutex> lock(m_commandMutex);
                    if (!m_commands.empty()) {
                        command = std::move(m_commands.front());
                        m_commands.pop_front();
                    }
                }
                if (command.has_value()) {
                    processCommand(
                        transport, clientId->view(), clientSecret.view(),
                        token.accessToken.view(), std::move(*command));
                    continue;
                }

                std::uint32_t type = 0;
                std::string correlation;
                std::string payload;
                Gate7ProviderErrorCategory category =
                    Gate7ProviderErrorCategory::None;
                const auto outcome = transport.receiveAnyDetailed(
                    type, correlation, payload,
                    Clock::now() + std::chrono::milliseconds(250), category);
                if (outcome == Gate7TransportOutcome::Timeout) {
                    secureClear(correlation); secureClear(payload);
                    continue;
                }
                if (outcome != Gate7TransportOutcome::Expected) {
                    secureClear(correlation); secureClear(payload);
                    if (recoverableTransportOutcome(outcome)
                        && recoverTransport(
                            transport, clientId->view(), clientSecret.view(),
                            token.accessToken.view())) {
                        continue;
                    }
                    setFailure(category == Gate7ProviderErrorCategory::RateLimited
                        ? DemoDiagnostic::RateLimited
                        : DemoDiagnostic::Transport);
                    break;
                }
                if (!handleIncoming(type, correlation, payload,
                                    transport.generation())) {
                    secureClear(correlation); secureClear(payload);
                    setFailure(DemoDiagnostic::MalformedProviderEvent);
                    break;
                }
                secureClear(correlation); secureClear(payload);
            }
        }

        finishStartup(false);
        m_connected.store(false);
        m_candleChanged.notify_all();
        failPendingCommands();
        transport.close();
        clearToken(token); clientSecret.clear();
        clientId->clear(); clientId.reset();
        secureClear(m_accountId); secureClear(m_symbolId);
    }

    mutable std::mutex m_startMutex;
    mutable std::mutex m_stateMutex;
    mutable std::mutex m_callbackMutex;
    mutable std::mutex m_commandMutex;
    std::condition_variable m_commandChanged;
    std::condition_variable m_candleChanged;
    std::thread m_io;
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_connected{false};
    std::atomic<DemoDiagnostic> m_diagnostic{DemoDiagnostic::None};
    std::atomic<DemoSubscriptionLeg> m_subscriptionLeg{
        DemoSubscriptionLeg::None};
    std::atomic<Gate7ResidualFailure> m_subscriptionFailure{
        Gate7ResidualFailure::None};
    std::atomic<std::uint64_t> m_connectionGeneration{0};
    std::atomic<std::uint64_t> m_eventSequence{0};
    std::uint64_t m_providerSequence{1000};
    std::uint64_t m_marketSequence{0};
    std::uint64_t m_snapshotVersion{0};
    Clock::time_point m_nextRequest{};
    bool m_transportRecoveryRequested{false};

    AcknowledgementCallback m_acknowledgement;
    ExecutionCallback m_execution;
    CancelCallback m_cancel;
    HealthCallback m_healthCallback;
    MarketDataCallback m_marketData;
    AdapterHealthEvent m_health;
    AccountSnapshot m_account;
    InstrumentSpec m_instrument;
    std::vector<MarketCandle> m_history;
    CTraderDemoMarketState m_marketState{16};
    std::deque<Command> m_commands;

    std::int64_t m_accountId{0};
    std::int64_t m_symbolId{0};
    std::int64_t m_depositAssetId{0};
    std::int64_t m_balanceRaw{0};
    std::uint32_t m_moneyDigits{2};
    std::uint64_t m_balanceVersion{0};
    std::string m_currency;
    std::string m_executionAlias;
    std::unordered_map<std::uint64_t, LocalOrderState> m_orders;
    std::unordered_map<std::string, std::uint64_t> m_clientOrderToLocal;
    std::unordered_map<std::string, std::uint64_t> m_correlationToLocal;
    std::unordered_map<std::int64_t, std::string> m_nativeToLogical;
    std::unordered_map<std::string, std::int64_t> m_logicalToNative;
    std::unordered_map<std::string, std::uint64_t> m_closeByLogical;
};

CTraderProviderAdapter::CTraderProviderAdapter(bool freshOAuth)
    : m_transport(std::make_unique<CTraderTransport>())
    , m_codec(std::make_unique<CTraderCodec>())
    , m_auth(std::make_unique<CTraderAuthService>())
    , m_accounts(std::make_unique<CTraderAccountService>())
    , m_instruments(std::make_unique<CTraderInstrumentService>())
    , m_marketData(std::make_unique<CTraderMarketDataService>())
    , m_orders(std::make_unique<CTraderOrderService>())
    , m_session(std::make_unique<CTraderSession>())
    , m_freshOAuth(freshOAuth)
{
    m_health.schemaVersion = 1;
    m_health.state = AdapterHealthState::Disconnected;
    m_health.failure = FailureCategory::None;
    m_health.reason = "ctrader_demo_not_started";
    m_health.eventKey = "ctrader-demo-not-started";
}

CTraderProviderAdapter::~CTraderProviderAdapter() = default;

void CTraderProviderAdapter::setAcknowledgementCallback(
    AcknowledgementCallback callback)
{
    m_acknowledgementCallback = callback;
    m_session->setAcknowledgementCallback(std::move(callback));
}

void CTraderProviderAdapter::setExecutionCallback(ExecutionCallback callback)
{
    m_executionCallback = callback;
    m_session->setExecutionCallback(std::move(callback));
}

void CTraderProviderAdapter::setCancelCallback(CancelCallback callback)
{
    m_cancelCallback = callback;
    m_session->setCancelCallback(std::move(callback));
}

void CTraderProviderAdapter::setHealthCallback(HealthCallback callback)
{
    m_healthCallback = callback;
    m_session->setHealthCallback(std::move(callback));
}

void CTraderProviderAdapter::setMarketDataCallback(MarketDataCallback callback)
{
    m_marketDataCallback = callback;
    m_session->setMarketDataCallback(std::move(callback));
}

bool CTraderProviderAdapter::connect()
{
    const bool connected = m_session->start(m_freshOAuth);
    m_health = m_session->health();
    return connected;
}

void CTraderProviderAdapter::disconnect() noexcept
{
    m_session->stop();
    m_health = m_session->health();
}

bool CTraderProviderAdapter::isConnected() const noexcept
{
    return m_session->connected();
}

bool CTraderProviderAdapter::submit(const NormalizedOrder& order)
{
    return m_session->submit(order);
}

bool CTraderProviderAdapter::cancel(const CancelRequest& request)
{
    (void)request;
    return false;
}

ReconciliationSnapshot CTraderProviderAdapter::reconcile(
    std::uint64_t timestampNs)
{
    return m_session->reconcile(timestampNs);
}

AdapterHealthEvent CTraderProviderAdapter::health() const
{
    return m_session->health();
}

std::optional<AccountSnapshot> CTraderProviderAdapter::accountSnapshot() const
{
    return m_session->account();
}

std::optional<InstrumentSpec> CTraderProviderAdapter::instrumentSpec(
    const std::string& canonicalSymbol) const
{
    return m_session->instrument(canonicalSymbol);
}

std::vector<MarketCandle> CTraderProviderAdapter::historicalCandles() const
{
    return m_session->historical();
}

std::optional<MarketCandle> CTraderProviderAdapter::waitForCompletedCandle(
    std::chrono::milliseconds timeout)
{
    return m_session->waitForCandle(timeout);
}

std::optional<OrderRiskContext> CTraderProviderAdapter::riskContext(
    PositionSide direction)
{
    return m_session->riskContext(direction);
}

FailureCategory CTraderProviderAdapter::lastFailure() const noexcept
{
    return m_session->lastFailure();
}

std::string CTraderProviderAdapter::lastDiagnostic() const
{
    return m_session->diagnostic();
}

void CTraderProviderAdapter::publishDisabledHealth() noexcept
{
}

} // namespace tradebot::ctrader
