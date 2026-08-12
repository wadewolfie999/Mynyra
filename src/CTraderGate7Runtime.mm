#include "CTraderGate7Runtime.hpp"

#include "CTraderGate7OAuthDiagnostics.hpp"
#include "CTraderGate7Proof.hpp"
#include "CTraderOAuthCorrelation.hpp"
#include "OpenApiCommonMessages.pb.h"
#include "OpenApiCommonModelMessages.pb.h"
#include "OpenApiMessages.pb.h"
#include "OpenApiModelMessages.pb.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#include <Security/Security.h>

#include <arpa/inet.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <poll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <climits>
#include <cstddef>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace tradebot::ctrader {
namespace {

using Clock = std::chrono::steady_clock;
constexpr auto NETWORK_TIMEOUT = std::chrono::seconds(20);
constexpr auto SPOT_TIMEOUT = std::chrono::seconds(60);
constexpr std::size_t MAX_PROTO_FRAME_BYTES = 4U * 1024U * 1024U;
constexpr std::size_t MAX_HTTP_REQUEST_BYTES = 16U * 1024U;
constexpr std::size_t MAX_TOKEN_RESPONSE_BYTES = 64U * 1024U;

class Sensitive final {
public:
    Sensitive() = default;
    explicit Sensitive(std::string value) noexcept : value_(std::move(value)) {}
    ~Sensitive() { clear(); }

    Sensitive(const Sensitive&) = delete;
    Sensitive& operator=(const Sensitive&) = delete;
    Sensitive(Sensitive&& other) noexcept : value_(std::move(other.value_))
    {
        other.clear();
    }
    Sensitive& operator=(Sensitive&& other) noexcept
    {
        if (this != &other) {
            clear();
            value_ = std::move(other.value_);
            other.clear();
        }
        return *this;
    }

    std::string_view view() const noexcept { return value_; }
    bool empty() const noexcept { return value_.empty(); }
    void clear() noexcept
    {
        volatile char* bytes = value_.empty() ? nullptr : value_.data();
        for (std::size_t i = 0; i < value_.size(); ++i) bytes[i] = 0;
        value_.clear();
    }

private:
    std::string value_;
};

enum class RuntimeFailure {
    None,
    LocalHardening,
    InvalidConfiguration,
    MissingClientId,
    MissingClientSecret,
    KeychainRead,
    TokenUnavailable,
    TokenRefreshFailed,
    TokenExchangeFailed,
    TokenRejected,
    DemoTlsConnection,
    ApplicationAuthentication,
    AccountDiscovery,
    AccountSelection,
    AccountAuthentication,
    SymbolList,
    FullSymbol,
    Subscription,
    SpotTimeout,
    SpotProof,
    ResourceExhausted
};

std::string_view diagnostic(RuntimeFailure failure) noexcept
{
    switch (failure) {
    case RuntimeFailure::None: return "gate7_ok";
    case RuntimeFailure::LocalHardening: return "gate7_local_hardening_failed";
    case RuntimeFailure::InvalidConfiguration: return "gate7_configuration_rejected";
    case RuntimeFailure::MissingClientId: return "gate7_client_id_missing";
    case RuntimeFailure::MissingClientSecret: return "gate7_client_secret_missing";
    case RuntimeFailure::KeychainRead: return "gate7_keychain_read_failed";
    case RuntimeFailure::TokenUnavailable: return "gate7_token_unavailable";
    case RuntimeFailure::TokenRefreshFailed: return "gate7_token_refresh_failed";
    case RuntimeFailure::TokenExchangeFailed: return "gate7_token_exchange_failed";
    case RuntimeFailure::TokenRejected: return "gate7_token_rejected";
    case RuntimeFailure::DemoTlsConnection: return "gate7_demo_tls_connection_failed";
    case RuntimeFailure::ApplicationAuthentication: return "gate7_application_auth_failed";
    case RuntimeFailure::AccountDiscovery: return "gate7_account_discovery_failed";
    case RuntimeFailure::AccountSelection: return "gate7_account_selection_failed";
    case RuntimeFailure::AccountAuthentication: return "gate7_account_auth_failed";
    case RuntimeFailure::SymbolList: return "gate7_symbol_list_failed";
    case RuntimeFailure::FullSymbol: return "gate7_full_symbol_failed";
    case RuntimeFailure::Subscription: return "gate7_subscription_failed";
    case RuntimeFailure::SpotTimeout: return "gate7_spot_timeout";
    case RuntimeFailure::SpotProof: return "gate7_spot_proof_failed";
    case RuntimeFailure::ResourceExhausted: return "gate7_resource_exhausted";
    }
    return "gate7_unknown_failure";
}

int fail(RuntimeFailure failure) noexcept
{
    std::cout << diagnostic(failure) << '\n';
    return 1;
}

int failOAuth(Gate7OAuthFailure failure) noexcept
{
    std::cout << safeOAuthDiagnostic(failure) << '\n';
    return 1;
}

void secureClear(std::string& value) noexcept
{
    volatile char* bytes = value.empty() ? nullptr : value.data();
    for (std::size_t i = 0; i < value.size(); ++i) bytes[i] = 0;
    value.clear();
}

void secureClearBytes(void* bytes, std::size_t size) noexcept
{
    volatile unsigned char* target = static_cast<volatile unsigned char*>(bytes);
    for (std::size_t i = 0; i < size; ++i) target[i] = 0;
}

void secureClear(std::int64_t& value) noexcept
{
    volatile std::int64_t* target = &value;
    *target = 0;
}

bool constantTimeEqual(std::string_view left, std::string_view right) noexcept
{
    const std::size_t length = std::max(left.size(), right.size());
    unsigned char difference = static_cast<unsigned char>(left.size() ^ right.size());
    for (std::size_t index = 0; index < length; ++index) {
        const unsigned char leftByte = index < left.size()
            ? static_cast<unsigned char>(left[index]) : 0;
        const unsigned char rightByte = index < right.size()
            ? static_cast<unsigned char>(right[index]) : 0;
        difference = static_cast<unsigned char>(difference | (leftByte ^ rightByte));
    }
    return difference == 0;
}

struct VolatileId final {
    std::int64_t value{0};
    ~VolatileId() { secureClear(value); }
};

bool boundedText(std::string_view value, std::size_t maximum) noexcept
{
    if (value.empty() || value.size() > maximum) return false;
    for (const unsigned char c : value) {
        if (c < 0x20 || c == 0x7f) return false;
    }
    return true;
}

bool disableCoreDumps() noexcept
{
    const rlimit noCore{0, 0};
    return ::setrlimit(RLIMIT_CORE, &noCore) == 0;
}

std::optional<std::string> localUserName() noexcept
{
    const char* value = std::getenv("USER");
    if (value == nullptr) return std::nullopt;
    try {
        std::string result(value);
        if (!boundedText(result, 256)) {
            secureClear(result);
            return std::nullopt;
        }
        return result;
    } catch (...) {
        return std::nullopt;
    }
}

CFStringRef makeCfString(std::string_view value) noexcept
{
    return CFStringCreateWithBytes(
        kCFAllocatorDefault,
        reinterpret_cast<const UInt8*>(value.data()),
        static_cast<CFIndex>(value.size()),
        kCFStringEncodingUTF8,
        false);
}

RuntimeFailure readKeychainValue(std::string_view service,
                                 Sensitive& output) noexcept
{
    output.clear();
    const auto user = localUserName();
    if (!user.has_value()) return RuntimeFailure::KeychainRead;
    CFStringRef serviceRef = makeCfString(service);
    CFStringRef accountRef = makeCfString(*user);
    if (serviceRef == nullptr || accountRef == nullptr) {
        if (serviceRef != nullptr) CFRelease(serviceRef);
        if (accountRef != nullptr) CFRelease(accountRef);
        return RuntimeFailure::KeychainRead;
    }
    const void* keys[] = {kSecClass, kSecAttrService, kSecAttrAccount,
                          kSecReturnData, kSecMatchLimit};
    const void* values[] = {kSecClassGenericPassword, serviceRef, accountRef,
                            kCFBooleanTrue, kSecMatchLimitOne};
    CFDictionaryRef query = CFDictionaryCreate(
        kCFAllocatorDefault, keys, values, 5,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFTypeRef result = nullptr;
    const OSStatus status = query == nullptr
        ? errSecAllocate : SecItemCopyMatching(query, &result);
    if (query != nullptr) CFRelease(query);
    CFRelease(serviceRef);
    CFRelease(accountRef);
    if (status == errSecItemNotFound) return RuntimeFailure::TokenUnavailable;
    if (status != errSecSuccess || result == nullptr
        || CFGetTypeID(result) != CFDataGetTypeID()) {
        if (result != nullptr) CFRelease(result);
        return RuntimeFailure::KeychainRead;
    }
    CFDataRef data = reinterpret_cast<CFDataRef>(result);
    const CFIndex length = CFDataGetLength(data);
    const UInt8* bytes = CFDataGetBytePtr(data);
    if (bytes == nullptr || length <= 0 || length > 65536) {
        CFRelease(result);
        return RuntimeFailure::KeychainRead;
    }
    std::string copy;
    try {
        copy.assign(reinterpret_cast<const char*>(bytes),
                    static_cast<std::size_t>(length));
        output = Sensitive(std::move(copy));
    } catch (...) {
        secureClear(copy);
        CFRelease(result);
        return RuntimeFailure::ResourceExhausted;
    }
    CFRelease(result);
    return RuntimeFailure::None;
}

struct TokenEnvelope final {
    Sensitive accessToken;
    Sensitive refreshToken;
    Sensitive tokenType;
    std::int64_t expiresAtEpochSeconds{0};
    std::string scope;
};

void clearToken(TokenEnvelope& token) noexcept
{
    token.accessToken.clear();
    token.refreshToken.clear();
    token.tokenType.clear();
    secureClear(token.scope);
    secureClear(token.expiresAtEpochSeconds);
}

bool tokenUsable(const TokenEnvelope& token) noexcept
{
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return token.scope == CTraderGate7Config::OAUTH_SCOPE
        && token.tokenType.view() == "bearer"
        && !token.accessToken.empty() && !token.refreshToken.empty()
        && token.expiresAtEpochSeconds > now
        && token.expiresAtEpochSeconds - now >= 60;
}

bool readBigEndian64(std::string_view input, std::size_t& offset,
                     std::uint64_t& value) noexcept
{
    if (input.size() - offset < 8) return false;
    value = 0;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8)
            | static_cast<std::uint8_t>(input[offset++]);
    }
    return true;
}

bool readSized(std::string_view input, std::size_t& offset,
               std::string& value) noexcept
{
    std::uint64_t length = 0;
    if (!readBigEndian64(input, offset, length)
        || length > 8192 || length > input.size() - offset) return false;
    try {
        value.assign(input.substr(offset, static_cast<std::size_t>(length)));
        offset += static_cast<std::size_t>(length);
        return boundedText(value, 8192);
    } catch (...) {
        secureClear(value);
        return false;
    }
}

bool parseStoredToken(std::string_view encoded, TokenEnvelope& output) noexcept
{
    clearToken(output);
    try {
        if (encoded.size() < 16 || encoded.substr(0, 8) != "TBG6TOK1") {
            return false;
        }
        std::size_t offset = 8;
        std::uint64_t expiry = 0;
        if (!readBigEndian64(encoded, offset, expiry)
            || expiry > static_cast<std::uint64_t>(
                   std::numeric_limits<std::int64_t>::max())) return false;
        std::string scope;
        std::string type;
        std::string access;
        std::string refresh;
        if (!readSized(encoded, offset, scope)
            || !readSized(encoded, offset, type)
            || !readSized(encoded, offset, access)
            || !readSized(encoded, offset, refresh)
            || offset != encoded.size()
            || scope != CTraderGate7Config::OAUTH_SCOPE
            || type != "bearer") {
            secureClear(scope);
            secureClear(type);
            secureClear(access);
            secureClear(refresh);
            return false;
        }
        output.expiresAtEpochSeconds = static_cast<std::int64_t>(expiry);
        output.scope = std::move(scope);
        output.tokenType = Sensitive(std::move(type));
        output.accessToken = Sensitive(std::move(access));
        output.refreshToken = Sensitive(std::move(refresh));
        return true;
    } catch (...) {
        clearToken(output);
        return false;
    }
}

std::optional<Sensitive> loadClientId() noexcept
{
    const char* raw = std::getenv("TRADEBOT_CTRADER_CLIENT_ID");
    if (raw == nullptr) return std::nullopt;
    try {
        std::string value(raw);
        if (!boundedText(value, 512)
            || value == "REPLACE_WITH_LOCAL_CLIENT_ID") {
            secureClear(value);
            return std::nullopt;
        }
        return Sensitive(std::move(value));
    } catch (...) {
        return std::nullopt;
    }
}

bool keychainItemPresentOnly(std::string_view service) noexcept
{
    // Presence-only preflight: fixed service name, no secret bytes and no
    // account value are requested or printed. The actual proof uses the
    // reviewed in-process Security.framework read after this gate.
    if (service != CTraderGate7Config::CLIENT_SECRET_SERVICE) return false;
    return std::system(
        "/usr/bin/security find-generic-password -s "
        "'TradeBot.cTraderOpenApi.client-secret' -w >/dev/null 2>&1") == 0;
}

std::optional<std::string> urlEncode(CURL* curl, std::string_view value) noexcept
{
    if (curl == nullptr) return std::nullopt;
    char* escaped = curl_easy_escape(curl, value.data(),
                                     static_cast<int>(value.size()));
    if (escaped == nullptr) return std::nullopt;
    try {
        std::string result(escaped);
        secureClearBytes(escaped, std::strlen(escaped));
        curl_free(escaped);
        return result;
    } catch (...) {
        secureClearBytes(escaped, std::strlen(escaped));
        curl_free(escaped);
        return std::nullopt;
    }
}

std::size_t tokenWriteCallback(char* data, std::size_t size,
                               std::size_t count, void* userData) noexcept
{
    if (size != 0 && count > std::numeric_limits<std::size_t>::max() / size) {
        return 0;
    }
    const std::size_t bytes = size * count;
    auto* output = static_cast<std::string*>(userData);
    if (output == nullptr || bytes > MAX_TOKEN_RESPONSE_BYTES
        || output->size() > MAX_TOKEN_RESPONSE_BYTES - bytes) return 0;
    try {
        output->append(data, bytes);
        return bytes;
    } catch (...) {
        secureClear(*output);
        return 0;
    }
}

bool configureCurl(CURL* curl, std::string_view url,
                   curl_slist* headers, std::string& response) noexcept
{
    try {
        return curl != nullptr
            && curl_easy_setopt(curl, CURLOPT_URL, url.data()) == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L) == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers) == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, tokenWriteCallback) == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response) == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L) == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L) == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2) == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L) == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https") == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https") == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_PROXY, "") == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_NOPROXY, "*") == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L) == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L) == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L) == CURLE_OK
            && curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L) == CURLE_OK;
    } catch (...) {
        return false;
    }
}

bool performTokenUrl(Sensitive& url, std::string& response) noexcept
{
    CURL* curl = curl_easy_init();
    if (curl == nullptr) return false;
    curl_slist* headers = curl_slist_append(nullptr, "Accept: application/json");
    curl_slist* extended = headers == nullptr ? nullptr
        : curl_slist_append(headers, "Cache-Control: no-store");
    if (extended == nullptr) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }
    const bool configured = configureCurl(curl, url.view(), extended, response);
    const CURLcode result = configured ? curl_easy_perform(curl) : CURLE_FAILED_INIT;
    long status = 0;
    if (result == CURLE_OK) (void)curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_slist_free_all(extended);
    curl_easy_cleanup(curl);
    url.clear();
    return result == CURLE_OK && status == 200;
}

struct JsonCursor final {
    explicit JsonCursor(std::string_view input) : input(input) {}
    void whitespace() noexcept
    {
        while (position < input.size()
               && (input[position] == ' ' || input[position] == '\t'
                   || input[position] == '\r' || input[position] == '\n')) ++position;
    }
    bool consume(char expected) noexcept
    {
        whitespace();
        if (position >= input.size() || input[position] != expected) return false;
        ++position;
        return true;
    }
    bool end() noexcept { whitespace(); return position == input.size(); }
    std::optional<std::string> stringValue()
    {
        whitespace();
        if (position >= input.size() || input[position] != '"') return std::nullopt;
        ++position;
        std::string result;
        try {
            while (position < input.size()) {
                const char c = input[position++];
                if (c == '"') return result;
                if (static_cast<unsigned char>(c) < 0x20) {
                    secureClear(result);
                    return std::nullopt;
                }
                if (c != '\\') {
                    result.push_back(c);
                    continue;
                }
                if (position >= input.size()) break;
                const char escaped = input[position++];
                if (escaped == '"' || escaped == '\\' || escaped == '/') result.push_back(escaped);
                else if (escaped == 'n') result.push_back('\n');
                else if (escaped == 'r') result.push_back('\r');
                else if (escaped == 't') result.push_back('\t');
                else {
                    secureClear(result);
                    return std::nullopt;
                }
            }
        } catch (...) {
            secureClear(result);
            throw;
        }
        secureClear(result);
        return std::nullopt;
    }
    std::optional<std::int64_t> integerValue() noexcept
    {
        whitespace();
        const std::size_t start = position;
        if (position < input.size() && input[position] == '-') ++position;
        while (position < input.size() && input[position] >= '0'
               && input[position] <= '9') ++position;
        if (position == start) return std::nullopt;
        std::int64_t value = 0;
        const auto parsed = std::from_chars(input.data() + start,
                                            input.data() + position, value);
        return parsed.ec == std::errc{} && parsed.ptr == input.data() + position
            ? std::optional<std::int64_t>(value) : std::nullopt;
    }
    bool nullValue() noexcept
    {
        whitespace();
        if (input.substr(position, 4) != "null") return false;
        position += 4;
        return true;
    }
    std::string_view input;
    std::size_t position{0};
};

bool parseTokenResponse(std::string_view body, TokenEnvelope& output) noexcept
{
    clearToken(output);
    try {
        JsonCursor cursor(body);
        if (!cursor.consume('{')) return false;
        bool access = false;
        bool refresh = false;
        bool type = false;
        bool expiry = false;
        bool errorNull = false;
        std::array<std::string, 8> keys;
        std::size_t keyCount = 0;
        while (true) {
            cursor.whitespace();
            if (cursor.consume('}')) break;
            auto key = cursor.stringValue();
            if (!key.has_value() || !cursor.consume(':') || keyCount == keys.size()) return false;
            for (std::size_t i = 0; i < keyCount; ++i) {
                if (keys[i] == *key) {
                    secureClear(*key);
                    return false;
                }
            }
            keys[keyCount++] = std::move(*key);
            const std::string& current = keys[keyCount - 1];
            if (current == "accessToken" || current == "refreshToken"
                || current == "tokenType") {
                auto value = cursor.stringValue();
                if (!value.has_value() || !boundedText(*value, 8192)) return false;
                if (current == "accessToken") {
                    output.accessToken = Sensitive(std::move(*value)); access = true;
                } else if (current == "refreshToken") {
                    output.refreshToken = Sensitive(std::move(*value)); refresh = true;
                } else {
                    output.tokenType = Sensitive(std::move(*value)); type = true;
                }
            } else if (current == "expiresIn") {
                const auto value = cursor.integerValue();
                if (!value.has_value() || *value <= 0
                    || *value > 10LL * 365LL * 24LL * 60LL * 60LL) return false;
                const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                if (now > std::numeric_limits<std::int64_t>::max() - *value) return false;
                output.expiresAtEpochSeconds = now + *value;
                expiry = true;
            } else if (current == "errorCode") {
                if (!cursor.nullValue()) return false;
                errorNull = true;
            } else {
                if (!cursor.nullValue() && !cursor.stringValue().has_value()
                    && !cursor.integerValue().has_value()) return false;
            }
            cursor.whitespace();
            if (cursor.consume(',')) continue;
            if (cursor.consume('}')) break;
            return false;
        }
        if (!cursor.end() || !access || !refresh || !type || !expiry || !errorNull
            || output.tokenType.view() != "bearer") return false;
        output.scope = std::string(CTraderGate7Config::OAUTH_SCOPE);
        return true;
    } catch (...) {
        clearToken(output);
        return false;
    }
}

RuntimeFailure obtainTokenFromUrl(Sensitive& url, TokenEnvelope& output)
{
    std::string response;
    if (!performTokenUrl(url, response)) {
        secureClear(response);
        return RuntimeFailure::TokenExchangeFailed;
    }
    const bool parsed = parseTokenResponse(response, output);
    secureClear(response);
    if (!parsed) return RuntimeFailure::TokenRejected;
    return tokenUsable(output) ? RuntimeFailure::None
                               : RuntimeFailure::TokenRejected;
}

RuntimeFailure refreshTokenOnce(const TokenEnvelope& current,
                                std::string_view clientId,
                                std::string_view clientSecret,
                                TokenEnvelope& output) noexcept
{
    CURL* encoder = curl_easy_init();
    if (encoder == nullptr) return RuntimeFailure::TokenRefreshFailed;
    auto refresh = urlEncode(encoder, current.refreshToken.view());
    auto client = urlEncode(encoder, clientId);
    auto secret = urlEncode(encoder, clientSecret);
    curl_easy_cleanup(encoder);
    if (!refresh.has_value() || !client.has_value() || !secret.has_value()) {
        if (refresh.has_value()) secureClear(*refresh);
        if (client.has_value()) secureClear(*client);
        if (secret.has_value()) secureClear(*secret);
        return RuntimeFailure::TokenRefreshFailed;
    }
    std::string raw;
    try {
        raw = "https://openapi.ctrader.com/apps/token?grant_type=refresh_token&refresh_token=";
        raw += *refresh;
        raw += "&client_id=";
        raw += *client;
        raw += "&client_secret=";
        raw += *secret;
    } catch (...) {
        secureClear(raw);
        secureClear(*refresh); secureClear(*client); secureClear(*secret);
        return RuntimeFailure::ResourceExhausted;
    }
    secureClear(*refresh); secureClear(*client); secureClear(*secret);
    Sensitive url(std::move(raw));
    return obtainTokenFromUrl(url, output) == RuntimeFailure::None
        ? RuntimeFailure::None : RuntimeFailure::TokenRefreshFailed;
}

bool setNonBlocking(int fd) noexcept
{
    const int flags = ::fcntl(fd, F_GETFL, 0);
    return flags >= 0 && ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

bool waitForFd(int fd, short events, Clock::time_point deadline) noexcept
{
    while (Clock::now() < deadline) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - Clock::now());
        const int timeout = static_cast<int>(std::clamp<std::int64_t>(
            remaining.count(), 1, 1000));
        pollfd descriptor{fd, events, 0};
        const int result = ::poll(&descriptor, 1, timeout);
        if (result > 0) {
            return (descriptor.revents & events) != 0
                && (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) == 0;
        }
        if (result < 0 && errno != EINTR) return false;
    }
    return false;
}

bool sendAll(int fd, std::string_view bytes) noexcept
{
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const ssize_t count = ::send(fd, bytes.data() + offset,
                                     bytes.size() - offset, 0);
        if (count > 0) {
            offset += static_cast<std::size_t>(count);
            continue;
        }
        if (count < 0 && errno == EINTR) continue;
        return false;
    }
    return true;
}

struct HttpRequest final {
    std::string_view method;
    std::string_view path;
    std::string_view query;
    std::string_view host;
};

std::optional<HttpRequest> parseHttpRequest(std::string_view raw) noexcept
{
    const auto lineEnd = raw.find("\r\n");
    if (lineEnd == std::string_view::npos) return std::nullopt;
    const auto line = raw.substr(0, lineEnd);
    const auto first = line.find(' ');
    const auto second = first == std::string_view::npos
        ? std::string_view::npos : line.find(' ', first + 1);
    if (first == std::string_view::npos || second == std::string_view::npos
        || line.substr(second + 1) != "HTTP/1.1") return std::nullopt;
    const auto target = line.substr(first + 1, second - first - 1);
    const auto question = target.find('?');
    std::string_view host;
    std::size_t cursor = lineEnd + 2;
    while (cursor < raw.size()) {
        const auto end = raw.find("\r\n", cursor);
        if (end == std::string_view::npos || end == cursor) break;
        const auto header = raw.substr(cursor, end - cursor);
        const auto colon = header.find(':');
        if (colon == std::string_view::npos) return std::nullopt;
        auto name = header.substr(0, colon);
        auto value = header.substr(colon + 1);
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
            value.remove_prefix(1);
        }
        if ((name == "Host" || name == "host")) {
            if (!host.empty()) return std::nullopt;
            host = value;
        }
        cursor = end + 2;
    }
    if (host.empty()) return std::nullopt;
    return HttpRequest{line.substr(0, first),
                       target.substr(0, question),
                       question == std::string_view::npos
                           ? std::string_view{} : target.substr(question + 1),
                       host};
}

std::optional<Sensitive> extractCode(std::string_view query) noexcept
{
    std::size_t start = 0;
    while (start < query.size()) {
        const auto end = query.find('&', start);
        const auto parameter = query.substr(
            start, end == std::string_view::npos ? query.size() - start
                                                 : end - start);
        const auto equals = parameter.find('=');
        if (equals != std::string_view::npos
            && parameter.substr(0, equals) == "code") {
            std::string decoded;
            try {
                for (std::size_t i = equals + 1; i < parameter.size(); ++i) {
                    if (parameter[i] == '%') {
                        if (i + 2 >= parameter.size()) return std::nullopt;
                        auto hex = [](char value) -> int {
                            if (value >= '0' && value <= '9') return value - '0';
                            if (value >= 'a' && value <= 'f') return value - 'a' + 10;
                            if (value >= 'A' && value <= 'F') return value - 'A' + 10;
                            return -1;
                        };
                        const int high = hex(parameter[i + 1]);
                        const int low = hex(parameter[i + 2]);
                        if (high < 0 || low < 0) return std::nullopt;
                        decoded.push_back(static_cast<char>((high << 4) | low));
                        i += 2;
                    } else if (parameter[i] == '+') {
                        decoded.push_back(' ');
                    } else {
                        decoded.push_back(parameter[i]);
                    }
                }
                if (!boundedText(decoded, 8192)) {
                    secureClear(decoded);
                    return std::nullopt;
                }
                return Sensitive(std::move(decoded));
            } catch (...) {
                secureClear(decoded);
                return std::nullopt;
            }
        }
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return std::nullopt;
}

bool openBrowserUrl(std::string_view url) noexcept
{
    @autoreleasepool {
        NSString* value = [[NSString alloc] initWithBytes:url.data()
                                                     length:url.size()
                                                   encoding:NSUTF8StringEncoding];
        if (value == nil) return false;
        NSURL* target = [NSURL URLWithString:value];
        const BOOL opened = target != nil
            && [[NSWorkspace sharedWorkspace] openURL:target];
        [value release];
        return opened == YES;
    }
}

std::optional<Sensitive> authorizeInBrowser(
    std::string_view clientId, Gate7OAuthFailure& failure) noexcept
{
    failure = Gate7OAuthFailure::None;
    const auto reject = [&failure](Gate7OAuthFailure reason) noexcept
        -> std::optional<Sensitive> {
        failure = reason;
        return std::nullopt;
    };

    int listener = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) return reject(Gate7OAuthFailure::ListenerSocketFailed);
    int reuse = 1;
    (void)::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(CTraderOAuthCorrelationGuard::LOOPBACK_PORT);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        ::close(listener);
        return reject(Gate7OAuthFailure::ListenerBindFailed);
    }
    if (::listen(listener, 1) != 0) {
        ::close(listener);
        return reject(Gate7OAuthFailure::ListenerListenFailed);
    }
    if (!setNonBlocking(listener)) {
        ::close(listener);
        return reject(Gate7OAuthFailure::ListenerNonBlockingFailed);
    }

    CTraderOAuthCorrelationGuard guard;
    const auto armed = Clock::now();
    if (!guard.arm({CTraderOAuthCorrelationGuard::LOOPBACK_ADDRESS,
                    CTraderOAuthCorrelationGuard::LOOPBACK_PORT}, armed)) {
        const Gate7OAuthFailure reason = classifyOAuthCorrelationFailure(
            guard.lastDecision());
        ::close(listener);
        return reject(reason == Gate7OAuthFailure::None
                          ? Gate7OAuthFailure::CorrelationArmFailed : reason);
    }
    CURL* encoder = curl_easy_init();
    if (encoder == nullptr) {
        guard.cancel();
        ::close(listener);
        return reject(Gate7OAuthFailure::AuthorizationUrlFailed);
    }
    auto encodedClient = urlEncode(encoder, clientId);
    auto encodedRedirect = urlEncode(encoder, CTraderGate7Config::REDIRECT_URI);
    curl_easy_cleanup(encoder);
    if (!encodedClient.has_value() || !encodedRedirect.has_value()) {
        guard.cancel();
        ::close(listener);
        if (encodedClient.has_value()) secureClear(*encodedClient);
        if (encodedRedirect.has_value()) secureClear(*encodedRedirect);
        return reject(Gate7OAuthFailure::AuthorizationUrlFailed);
    }
    Sensitive url;
    try {
        std::string raw = "https://id.ctrader.com/my/settings/openapi/grantingaccess/?client_id=";
        raw += *encodedClient;
        raw += "&redirect_uri=";
        raw += *encodedRedirect;
        raw += "&scope=";
        raw += CTraderGate7Config::OAUTH_SCOPE;
        raw += "&product=web&state=";
        raw += guard.stateForAuthorizationRequest();
        url = Sensitive(std::move(raw));
    } catch (...) {
        guard.cancel();
        ::close(listener);
        secureClear(*encodedClient); secureClear(*encodedRedirect);
        return reject(Gate7OAuthFailure::AuthorizationUrlFailed);
    }
    secureClear(*encodedClient);
    secureClear(*encodedRedirect);
    if (!openBrowserUrl(url.view())) {
        url.clear(); guard.cancel(); ::close(listener);
        return reject(Gate7OAuthFailure::BrowserLaunchFailed);
    }

    std::string rawRequest;
    Sensitive code;
    Gate7OAuthFailure terminalFailure = Gate7OAuthFailure::None;
    const auto deadline = armed + CTraderOAuthCorrelationGuard::CORRELATION_LIFETIME;
    while (Clock::now() < deadline) {
        if (!waitForFd(listener, POLLIN, deadline)) {
            terminalFailure = Clock::now() >= deadline
                ? Gate7OAuthFailure::CallbackTimeout
                : Gate7OAuthFailure::CallbackWaitFailed;
            break;
        }
        sockaddr_in remote{};
        socklen_t remoteLength = sizeof(remote);
        const int client = ::accept(
            listener, reinterpret_cast<sockaddr*>(&remote), &remoteLength);
        if (client < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
            terminalFailure = Gate7OAuthFailure::CallbackAcceptFailed;
            break;
        }

        if (!setNonBlocking(client)) {
            terminalFailure = Gate7OAuthFailure::CallbackReadFailed;
            (void)sendAll(client,
                "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n"
                "Connection: close\r\n\r\n");
            ::close(client);
            break;
        }

        std::array<char, 2048> buffer{};
        bool requestComplete = false;
        while (rawRequest.size() < MAX_HTTP_REQUEST_BYTES) {
            const auto readDeadline = gate7OAuthCallbackReadDeadline(
                deadline, Clock::now());
            if (!waitForFd(client, POLLIN, readDeadline)) {
                terminalFailure = Clock::now() >= deadline
                    ? Gate7OAuthFailure::CallbackTimeout
                    : Gate7OAuthFailure::CallbackReadFailed;
                break;
            }
            const ssize_t count = ::recv(client, buffer.data(), buffer.size(), 0);
            if (count > 0) {
                const Gate7OAuthFailure appendFailure =
                    appendGate7OAuthCallbackBytes(
                        rawRequest,
                        {buffer.data(), static_cast<std::size_t>(count)},
                        MAX_HTTP_REQUEST_BYTES);
                if (appendFailure != Gate7OAuthFailure::None) {
                    terminalFailure = appendFailure;
                    break;
                }
                if (rawRequest.find("\r\n\r\n") != std::string::npos) {
                    requestComplete = true;
                    break;
                }
                continue;
            }
            if (count < 0 && errno == EINTR) continue;
            terminalFailure = count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)
                ? Gate7OAuthFailure::CallbackReadFailed
                : Gate7OAuthFailure::CallbackMalformed;
            break;
        }
        if (!requestComplete && terminalFailure == Gate7OAuthFailure::None) {
            terminalFailure = Gate7OAuthFailure::CallbackMalformed;
        }

        const auto request = requestComplete
            ? parseHttpRequest(rawRequest) : std::optional<HttpRequest>{};
        if (!request.has_value()) {
            guard.cancel();
            (void)sendAll(client,
                "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n"
                "Connection: close\r\n\r\n");
            ::close(client);
            break;
        }

        char remoteText[INET_ADDRSTRLEN]{};
        const char* converted = ::inet_ntop(AF_INET, &remote.sin_addr,
                                            remoteText, sizeof(remoteText));
        const std::string_view remoteAddress = converted == nullptr
            ? std::string_view{} : std::string_view(remoteText);
        const auto decision = guard.consume(
            {remoteAddress, request->method, request->host, request->path,
             request->query}, Clock::now());
        if (decision != CTraderOAuthCorrelationGuard::Decision::
                CorrelationMatchedCodeDiscarded) {
            terminalFailure = classifyOAuthCorrelationFailure(decision);
            (void)sendAll(client,
                "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n"
                "Connection: close\r\n\r\n");
            ::close(client);
            break;
        }

        auto extracted = extractCode(request->query);
        if (!extracted.has_value()) {
            terminalFailure = Gate7OAuthFailure::CodeExtractionFailed;
            (void)sendAll(client,
                "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n"
                "Connection: close\r\n\r\n");
            ::close(client);
            break;
        }
        code = std::move(*extracted);
        (void)sendAll(client,
            "HTTP/1.1 303 See Other\r\nLocation: /ctrader/oauth/complete\r\n"
            "Content-Length: 0\r\nConnection: close\r\n\r\n");
        ::close(client);
        break;
    }
    const auto now = Clock::now();
    if (code.empty()) {
        if (now >= deadline) {
            (void)guard.expireIfDue(now);
            if (terminalFailure == Gate7OAuthFailure::None) {
                terminalFailure = Gate7OAuthFailure::CallbackTimeout;
            }
        } else if (!guard.isTerminal()) {
            (void)guard.cancel();
            if (terminalFailure == Gate7OAuthFailure::None) {
                terminalFailure = Gate7OAuthFailure::CallbackWaitFailed;
            }
        }
    }
    secureClear(rawRequest);
    url.clear();
    ::close(listener);
    if (code.empty()) {
        failure = terminalFailure == Gate7OAuthFailure::None
            ? Gate7OAuthFailure::ResourceExhausted : terminalFailure;
        return std::nullopt;
    }
    failure = Gate7OAuthFailure::None;
    return std::optional<Sensitive>(std::move(code));
}

void appendUint32(std::string& output, std::uint32_t value)
{
    output.push_back(static_cast<char>((value >> 24) & 0xff));
    output.push_back(static_cast<char>((value >> 16) & 0xff));
    output.push_back(static_cast<char>((value >> 8) & 0xff));
    output.push_back(static_cast<char>(value & 0xff));
}

class StrictTransport final {
public:
    StrictTransport() noexcept : generation_(++nextGeneration_) {}
    ~StrictTransport() { close(); }
    StrictTransport(const StrictTransport&) = delete;
    StrictTransport& operator=(const StrictTransport&) = delete;

    bool connectDemo() noexcept
    {
        if (!CTraderGate7Config::isAllowedOpenApiEndpoint(
                CTraderGate7Config::DEMO_HOST, CTraderGate7Config::DEMO_PORT)) {
            return false;
        }
        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* resolved = nullptr;
        std::array<char, 8> port{};
        const auto converted = std::to_chars(
            port.data(), port.data() + port.size() - 1,
            CTraderGate7Config::DEMO_PORT);
        if (converted.ec != std::errc{}) return false;
        *converted.ptr = '\0';
        if (::getaddrinfo(CTraderGate7Config::DEMO_HOST.data(), port.data(),
                          &hints, &resolved) != 0) return false;
        const auto deadline = Clock::now() + NETWORK_TIMEOUT;
        for (addrinfo* current = resolved; current != nullptr;
             current = current->ai_next) {
            const int candidate = ::socket(current->ai_family,
                                           current->ai_socktype,
                                           current->ai_protocol);
            if (candidate < 0 || !setNonBlocking(candidate)) {
                if (candidate >= 0) ::close(candidate);
                continue;
            }
            const int result = ::connect(candidate, current->ai_addr,
                                         current->ai_addrlen);
            if (result == 0 || (errno == EINPROGRESS
                && waitForFd(candidate, POLLOUT, deadline))) {
                int socketError = 0;
                socklen_t length = sizeof(socketError);
                if (::getsockopt(candidate, SOL_SOCKET, SO_ERROR,
                                 &socketError, &length) == 0
                    && socketError == 0) {
                    fd_ = candidate;
                    break;
                }
            }
            ::close(candidate);
        }
        ::freeaddrinfo(resolved);
        if (fd_ < 0) return false;
        context_ = SSL_CTX_new(TLS_client_method());
        if (context_ == nullptr
            || SSL_CTX_set_min_proto_version(context_, TLS1_2_VERSION) != 1
            || SSL_CTX_set_default_verify_paths(context_) != 1) {
            close();
            return false;
        }
        SSL_CTX_set_verify(context_, SSL_VERIFY_PEER, nullptr);
        ssl_ = SSL_new(context_);
        if (ssl_ == nullptr || SSL_set_fd(ssl_, fd_) != 1
            || SSL_set_tlsext_host_name(ssl_, CTraderGate7Config::DEMO_HOST.data()) != 1
            || SSL_set1_host(ssl_, CTraderGate7Config::DEMO_HOST.data()) != 1) {
            close();
            return false;
        }
        while (Clock::now() < deadline) {
            const int result = SSL_connect(ssl_);
            if (result == 1) {
                if (SSL_get_verify_result(ssl_) != X509_V_OK) {
                    close();
                    return false;
                }
                connected_ = true;
                return true;
            }
            const int error = SSL_get_error(ssl_, result);
            if (error == SSL_ERROR_WANT_READ) {
                if (!waitForFd(fd_, POLLIN, deadline)) break;
            } else if (error == SSL_ERROR_WANT_WRITE) {
                if (!waitForFd(fd_, POLLOUT, deadline)) break;
            } else break;
        }
        close();
        return false;
    }

    void close() noexcept
    {
        connected_ = false;
        if (ssl_ != nullptr) {
            (void)SSL_shutdown(ssl_);
            SSL_free(ssl_);
            ssl_ = nullptr;
        }
        if (context_ != nullptr) {
            SSL_CTX_free(context_);
            context_ = nullptr;
        }
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    std::uint64_t generation() const noexcept { return generation_; }

    std::string nextCorrelation(std::string_view step)
    {
        return "g7-" + std::to_string(generation_) + "-"
            + std::to_string(++sequence_) + "-" + std::string(step);
    }

    bool send(std::uint32_t payloadType,
              const google::protobuf::MessageLite& message,
              std::string_view correlation) noexcept
    {
        if (!connected_ || !CTraderGate7Config::isAllowedOutboundPayload(payloadType)
            || correlation.empty() || correlation.size() > 128
            || !message.IsInitialized()) return false;
        std::string payload;
        std::string encoded;
        std::string frame;
        ProtoMessage envelope;
        try {
            if (!message.SerializeToString(&payload)) return false;
            envelope.set_payloadtype(payloadType);
            envelope.set_payload(payload);
            envelope.set_clientmsgid(correlation.data(), correlation.size());
            secureClear(payload);
            if (!envelope.IsInitialized()
                || !envelope.SerializeToString(&encoded)
                || encoded.empty() || encoded.size() > MAX_PROTO_FRAME_BYTES) {
                secureClear(encoded);
                envelope.Clear();
                return false;
            }
            frame.reserve(encoded.size() + 4);
            appendUint32(frame, static_cast<std::uint32_t>(encoded.size()));
            frame.append(encoded);
            secureClear(encoded);
            const bool result = writeExact(frame, Clock::now() + NETWORK_TIMEOUT);
            secureClear(frame);
            envelope.Clear();
            return result;
        } catch (...) {
            secureClear(payload); secureClear(encoded); secureClear(frame);
            envelope.Clear();
            return false;
        }
    }

    bool receiveExpected(std::uint32_t expected, std::string_view correlation,
                         std::string& payload) noexcept
    {
        const auto deadline = Clock::now() + NETWORK_TIMEOUT;
        while (Clock::now() < deadline) {
            std::uint32_t type = 0;
            std::string receivedCorrelation;
            if (!receiveOne(type, receivedCorrelation, payload, deadline)) return false;
            if (type == HEARTBEAT_EVENT) {
                secureClear(receivedCorrelation); secureClear(payload);
                continue;
            }
            const bool matched = type == expected
                && receivedCorrelation == correlation;
            secureClear(receivedCorrelation);
            if (!matched) {
                secureClear(payload);
                return false;
            }
            return true;
        }
        return false;
    }

    bool receiveSpot(std::string& payload, Clock::time_point deadline) noexcept
    {
        while (Clock::now() < deadline) {
            std::uint32_t type = 0;
            std::string correlation;
            if (!receiveOne(type, correlation, payload, deadline)) return false;
            secureClear(correlation);
            if (type == HEARTBEAT_EVENT) {
                secureClear(payload);
                continue;
            }
            return type == PROTO_OA_SPOT_EVENT;
        }
        return false;
    }

private:
    bool writeExact(std::string_view bytes, Clock::time_point deadline) noexcept
    {
        std::size_t offset = 0;
        while (offset < bytes.size() && Clock::now() < deadline) {
            const int count = SSL_write(ssl_, bytes.data() + offset,
                static_cast<int>(std::min<std::size_t>(
                    bytes.size() - offset, static_cast<std::size_t>(INT_MAX))));
            if (count > 0) {
                offset += static_cast<std::size_t>(count);
                continue;
            }
            const int error = SSL_get_error(ssl_, count);
            if (error == SSL_ERROR_WANT_READ) {
                if (!waitForFd(fd_, POLLIN, deadline)) return false;
            } else if (error == SSL_ERROR_WANT_WRITE) {
                if (!waitForFd(fd_, POLLOUT, deadline)) return false;
            } else return false;
        }
        return offset == bytes.size();
    }

    bool readExact(char* bytes, std::size_t size,
                   Clock::time_point deadline) noexcept
    {
        std::size_t offset = 0;
        while (offset < size && Clock::now() < deadline) {
            const int count = SSL_read(ssl_, bytes + offset,
                static_cast<int>(std::min<std::size_t>(
                    size - offset, static_cast<std::size_t>(INT_MAX))));
            if (count > 0) {
                offset += static_cast<std::size_t>(count);
                continue;
            }
            const int error = SSL_get_error(ssl_, count);
            if (error == SSL_ERROR_WANT_READ) {
                if (!waitForFd(fd_, POLLIN, deadline)) return false;
            } else if (error == SSL_ERROR_WANT_WRITE) {
                if (!waitForFd(fd_, POLLOUT, deadline)) return false;
            } else return false;
        }
        return offset == size;
    }

    bool receiveOne(std::uint32_t& type, std::string& correlation,
                    std::string& payload, Clock::time_point deadline) noexcept
    {
        std::array<unsigned char, 4> header{};
        if (!readExact(reinterpret_cast<char*>(header.data()), header.size(), deadline)) return false;
        const std::uint32_t length = (static_cast<std::uint32_t>(header[0]) << 24)
            | (static_cast<std::uint32_t>(header[1]) << 16)
            | (static_cast<std::uint32_t>(header[2]) << 8)
            | static_cast<std::uint32_t>(header[3]);
        if (length == 0 || length > MAX_PROTO_FRAME_BYTES) return false;
        std::string frame;
        try {
            frame.resize(length, '\0');
            if (!readExact(frame.data(), frame.size(), deadline)) {
                secureClear(frame);
                return false;
            }
            ProtoMessage envelope;
            if (!envelope.ParseFromString(frame) || !envelope.IsInitialized()
                || !CTraderGate7Config::isAllowedInboundPayload(
                    envelope.payloadtype())) {
                secureClear(frame);
                return false;
            }
            type = envelope.payloadtype();
            if (envelope.has_clientmsgid()) correlation = envelope.clientmsgid();
            if (envelope.has_payload()) payload = envelope.payload();
            secureClear(frame);
            envelope.Clear();
            return true;
        } catch (...) {
            secureClear(frame); secureClear(correlation); secureClear(payload);
            return false;
        }
    }

    inline static std::atomic<std::uint64_t> nextGeneration_{0};
    std::uint64_t generation_{0};
    std::uint64_t sequence_{0};
    int fd_{-1};
    SSL_CTX* context_{nullptr};
    SSL* ssl_{nullptr};
    bool connected_{false};
};

void clearApplicationRequest(ProtoOAApplicationAuthReq& request) noexcept
{
    if (request.has_clientid()) secureClear(*request.mutable_clientid());
    if (request.has_clientsecret()) secureClear(*request.mutable_clientsecret());
    request.Clear();
}

bool applicationAuthenticate(StrictTransport& transport,
                             std::string_view clientId,
                             std::string_view clientSecret) noexcept
{
    ProtoOAApplicationAuthReq request;
    ProtoOAApplicationAuthRes response;
    std::string correlation;
    std::string payload;
    try {
        request.set_clientid(clientId.data(), clientId.size());
        request.set_clientsecret(clientSecret.data(), clientSecret.size());
        correlation = transport.nextCorrelation("application");
        const bool sent = transport.send(PROTO_OA_APPLICATION_AUTH_REQ,
                                         request, correlation);
        clearApplicationRequest(request);
        if (!sent || !transport.receiveExpected(PROTO_OA_APPLICATION_AUTH_RES,
                                                 correlation, payload)) return false;
        const bool valid = response.ParseFromString(payload)
            && response.IsInitialized()
            && response.payloadtype() == PROTO_OA_APPLICATION_AUTH_RES;
        secureClear(correlation); secureClear(payload); response.Clear();
        return valid;
    } catch (...) {
        clearApplicationRequest(request); response.Clear();
        secureClear(correlation); secureClear(payload);
        return false;
    }
}

void clearAccountResponse(ProtoOAGetAccountListByAccessTokenRes& response) noexcept
{
    if (response.has_accesstoken()) secureClear(*response.mutable_accesstoken());
    for (auto& account : *response.mutable_ctidtraderaccount()) {
        account.set_ctidtraderaccountid(0);
        if (account.has_traderlogin()) account.set_traderlogin(0);
        if (account.has_brokertitleshort()) {
            secureClear(*account.mutable_brokertitleshort());
        }
        account.Clear();
    }
    response.Clear();
}

std::optional<Gate7AccountListEvidence> discoverAccounts(
    StrictTransport& transport, std::string_view accessToken) noexcept
{
    ProtoOAGetAccountListByAccessTokenReq request;
    ProtoOAGetAccountListByAccessTokenRes response;
    std::string correlation;
    std::string payload;
    Gate7AccountListEvidence evidence;
    try {
        request.set_accesstoken(accessToken.data(), accessToken.size());
        correlation = transport.nextCorrelation("accounts");
        const bool sent = transport.send(
            PROTO_OA_GET_ACCOUNTS_BY_ACCESS_TOKEN_REQ, request, correlation);
        if (request.has_accesstoken()) secureClear(*request.mutable_accesstoken());
        request.Clear();
        if (!sent || !transport.receiveExpected(
                PROTO_OA_GET_ACCOUNTS_BY_ACCESS_TOKEN_RES,
                correlation, payload)) return std::nullopt;
        if (!response.ParseFromString(payload) || !response.IsInitialized()
            || response.payloadtype() != PROTO_OA_GET_ACCOUNTS_BY_ACCESS_TOKEN_RES) {
            secureClear(payload); clearAccountResponse(response);
            return std::nullopt;
        }
        evidence.connectionGeneration = transport.generation();
        evidence.currentConnectionGeneration = true;
        evidence.correlationMatched = true;
        evidence.tokenOwned = response.has_accesstoken()
            && constantTimeEqual(response.accesstoken(), accessToken);
        evidence.tradingScope = response.has_permissionscope()
            && response.permissionscope() == SCOPE_TRADE;
        for (const auto& account : response.ctidtraderaccount()) {
            Gate7AccountRecord record;
            record.accountId = account.ctidtraderaccountid();
            if (account.has_islive()) record.isLive = account.islive();
            if (account.has_brokertitleshort()) {
                record.brokerTitleShort = account.brokertitleshort();
            }
            evidence.accounts.push_back(std::move(record));
        }
        secureClear(payload); secureClear(correlation); clearAccountResponse(response);
        return evidence;
    } catch (...) {
        secureClear(payload); secureClear(correlation);
        clearAccountResponse(response);
        if (request.has_accesstoken()) secureClear(*request.mutable_accesstoken());
        request.Clear();
        return std::nullopt;
    }
}

bool authenticateAccount(StrictTransport& transport, std::string_view token,
                         std::int64_t accountId,
                         std::int64_t& responseId) noexcept
{
    ProtoOAAccountAuthReq request;
    ProtoOAAccountAuthRes response;
    std::string correlation;
    std::string payload;
    try {
        request.set_ctidtraderaccountid(accountId);
        request.set_accesstoken(token.data(), token.size());
        correlation = transport.nextCorrelation("account");
        const bool sent = transport.send(PROTO_OA_ACCOUNT_AUTH_REQ,
                                         request, correlation);
        if (request.has_accesstoken()) secureClear(*request.mutable_accesstoken());
        request.set_ctidtraderaccountid(0); request.Clear();
        if (!sent || !transport.receiveExpected(PROTO_OA_ACCOUNT_AUTH_RES,
                                                 correlation, payload)) return false;
        const bool valid = response.ParseFromString(payload)
            && response.IsInitialized()
            && response.payloadtype() == PROTO_OA_ACCOUNT_AUTH_RES;
        if (valid) responseId = response.ctidtraderaccountid();
        secureClear(payload); secureClear(correlation);
        response.set_ctidtraderaccountid(0); response.Clear();
        return valid;
    } catch (...) {
        if (request.has_accesstoken()) secureClear(*request.mutable_accesstoken());
        request.set_ctidtraderaccountid(0); request.Clear();
        response.set_ctidtraderaccountid(0); response.Clear();
        secureClear(payload); secureClear(correlation); secureClear(responseId);
        return false;
    }
}

std::optional<Gate7SymbolsListEvidence> requestSymbols(
    StrictTransport& transport, std::int64_t accountId) noexcept
{
    ProtoOASymbolsListReq request;
    ProtoOASymbolsListRes response;
    std::string correlation;
    std::string payload;
    Gate7SymbolsListEvidence evidence;
    try {
        request.set_ctidtraderaccountid(accountId);
        request.set_includearchivedsymbols(false);
        correlation = transport.nextCorrelation("symbols");
        const bool sent = transport.send(PROTO_OA_SYMBOLS_LIST_REQ,
                                         request, correlation);
        request.set_ctidtraderaccountid(0); request.Clear();
        if (!sent || !transport.receiveExpected(PROTO_OA_SYMBOLS_LIST_RES,
                                                 correlation, payload)) return std::nullopt;
        if (!response.ParseFromString(payload) || !response.IsInitialized()
            || response.payloadtype() != PROTO_OA_SYMBOLS_LIST_RES) {
            secureClear(payload); response.Clear(); return std::nullopt;
        }
        evidence.connectionGeneration = transport.generation();
        evidence.currentConnectionGeneration = true;
        evidence.correlationMatched = true;
        evidence.accountId = response.ctidtraderaccountid();
        evidence.includeArchivedSymbols = false;
        for (const auto& symbol : response.symbol()) {
            Gate7LightSymbol light;
            light.symbolId = symbol.symbolid();
            if (symbol.has_symbolname()) light.symbolName = symbol.symbolname();
            if (symbol.has_enabled()) light.enabled = symbol.enabled();
            evidence.symbols.push_back(std::move(light));
        }
        for (const auto& symbol : response.archivedsymbol()) {
            evidence.archivedSymbolIds.push_back(symbol.symbolid());
        }
        secureClear(payload); secureClear(correlation);
        response.Clear();
        return evidence;
    } catch (...) {
        secureClear(payload); secureClear(correlation); response.Clear();
        request.set_ctidtraderaccountid(0); request.Clear();
        return std::nullopt;
    }
}

std::optional<Gate7FullSymbolEvidence> requestFullSymbol(
    StrictTransport& transport, std::int64_t accountId,
    std::int64_t symbolId) noexcept
{
    ProtoOASymbolByIdReq request;
    ProtoOASymbolByIdRes response;
    std::string correlation;
    std::string payload;
    Gate7FullSymbolEvidence evidence;
    try {
        request.set_ctidtraderaccountid(accountId);
        request.add_symbolid(symbolId);
        correlation = transport.nextCorrelation("full-symbol");
        const bool sent = transport.send(PROTO_OA_SYMBOL_BY_ID_REQ,
                                         request, correlation);
        request.set_ctidtraderaccountid(0); request.clear_symbolid(); request.Clear();
        if (!sent || !transport.receiveExpected(PROTO_OA_SYMBOL_BY_ID_RES,
                                                 correlation, payload)) return std::nullopt;
        if (!response.ParseFromString(payload) || !response.IsInitialized()
            || response.payloadtype() != PROTO_OA_SYMBOL_BY_ID_RES) {
            secureClear(payload); response.Clear(); return std::nullopt;
        }
        evidence.connectionGeneration = transport.generation();
        evidence.currentConnectionGeneration = true;
        evidence.correlationMatched = true;
        evidence.accountId = response.ctidtraderaccountid();
        for (const auto& symbol : response.symbol()) {
            Gate7FullSymbol full;
            full.symbolId = symbol.symbolid();
            full.digits = symbol.digits();
            full.pipPosition = symbol.pipposition();
            if (symbol.has_maxvolume()) full.maxVolume = symbol.maxvolume();
            if (symbol.has_minvolume()) full.minVolume = symbol.minvolume();
            if (symbol.has_stepvolume()) full.stepVolume = symbol.stepvolume();
            if (symbol.has_lotsize()) full.lotSize = symbol.lotsize();
            evidence.symbols.push_back(std::move(full));
        }
        for (const auto& symbol : response.archivedsymbol()) {
            evidence.archivedSymbolIds.push_back(symbol.symbolid());
        }
        secureClear(payload); secureClear(correlation); response.Clear();
        return evidence;
    } catch (...) {
        secureClear(payload); secureClear(correlation); response.Clear();
        request.set_ctidtraderaccountid(0); request.clear_symbolid(); request.Clear();
        return std::nullopt;
    }
}

std::optional<Gate7SubscriptionEvidence> subscribeToSpot(
    StrictTransport& transport, std::int64_t accountId,
    std::int64_t symbolId) noexcept
{
    ProtoOASubscribeSpotsReq request;
    ProtoOASubscribeSpotsRes response;
    std::string correlation;
    std::string payload;
    try {
        request.set_ctidtraderaccountid(accountId);
        request.add_symbolid(symbolId);
        request.set_subscribetospottimestamp(true);
        correlation = transport.nextCorrelation("spot-subscription");
        const bool sent = transport.send(PROTO_OA_SUBSCRIBE_SPOTS_REQ,
                                         request, correlation);
        request.set_ctidtraderaccountid(0); request.clear_symbolid(); request.Clear();
        if (!sent || !transport.receiveExpected(PROTO_OA_SUBSCRIBE_SPOTS_RES,
                                                 correlation, payload)) return std::nullopt;
        if (!response.ParseFromString(payload) || !response.IsInitialized()
            || response.payloadtype() != PROTO_OA_SUBSCRIBE_SPOTS_RES) {
            secureClear(payload); response.Clear(); return std::nullopt;
        }
        Gate7SubscriptionEvidence evidence{
            transport.generation(), true, true,
            response.ctidtraderaccountid(), symbolId, 1};
        secureClear(payload); secureClear(correlation);
        response.set_ctidtraderaccountid(0); response.Clear();
        return evidence;
    } catch (...) {
        secureClear(payload); secureClear(correlation); response.Clear();
        request.set_ctidtraderaccountid(0); request.clear_symbolid(); request.Clear();
        return std::nullopt;
    }
}

std::uint64_t systemTimestampNs() noexcept
{
    const auto duration = std::chrono::system_clock::now().time_since_epoch();
    const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    return nanos <= 0 ? 0 : static_cast<std::uint64_t>(nanos);
}

bool receiveFirstSpot(StrictTransport& transport, std::int64_t accountId,
                     std::int64_t symbolId, CTraderGate7Proof& proof,
                     Gate7Decision& decision) noexcept
{
    const auto deadline = Clock::now() + SPOT_TIMEOUT;
    std::string payload;
    if (!transport.receiveSpot(payload, deadline)) {
        secureClear(accountId); secureClear(symbolId);
        return false;
    }
    ProtoOASpotEvent event;
    try {
        if (!event.ParseFromString(payload) || !event.IsInitialized()
            || event.payloadtype() != PROTO_OA_SPOT_EVENT) {
            secureClear(payload); event.Clear();
            secureClear(accountId); secureClear(symbolId);
            return false;
        }
        Gate7SpotEvidence evidence{
            transport.generation(), true, true,
            event.ctidtraderaccountid(), event.symbolid(),
            event.has_bid() ? std::optional<std::uint64_t>(event.bid()) : std::nullopt,
            event.has_ask() ? std::optional<std::uint64_t>(event.ask()) : std::nullopt,
            event.has_timestamp() ? std::optional<std::int64_t>(event.timestamp()) : std::nullopt,
            systemTimestampNs()};
        secureClear(payload); event.set_ctidtraderaccountid(0);
        event.set_symbolid(0); event.Clear();
        if (evidence.accountId != accountId || evidence.symbolId != symbolId) {
            decision = Gate7Decision::InvalidAccountIdentifier;
        } else {
            decision = proof.acceptSpot(std::move(evidence));
        }
        secureClear(accountId); secureClear(symbolId);
        return decision == Gate7Decision::QuoteProofSucceeded;
    } catch (...) {
        secureClear(payload); event.set_ctidtraderaccountid(0);
        event.set_symbolid(0); event.Clear();
        secureClear(accountId); secureClear(symbolId);
        decision = Gate7Decision::ResourceExhausted;
        return false;
    }
}

std::string decimalEvidence(const Decimal64& value)
{
    return "units=" + std::to_string(value.units)
        + ",scale=" + std::to_string(value.scale);
}

std::string utcSecond(std::uint64_t timestampNs)
{
    const std::time_t seconds = static_cast<std::time_t>(timestampNs / 1000000000ULL);
    std::tm value{};
    if (gmtime_r(&seconds, &value) == nullptr) return "unavailable";
    char buffer[32]{};
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &value) == 0) {
        return "unavailable";
    }
    return buffer;
}

} // namespace

bool validateCTraderGate7OfflineConfiguration() noexcept
{
    if (!CTraderGate7Config::isAllowedOpenApiEndpoint(
            CTraderGate7Config::DEMO_HOST, CTraderGate7Config::DEMO_PORT)
        || CTraderGate7Config::OAUTH_SCOPE != "trading") return false;
    constexpr std::array<std::uint32_t, 8> allowed = {
        51, 2100, 2102, 2114, 2116, 2127, 2129, 2149};
    for (const auto payload : allowed) {
        if (!CTraderGate7Config::isAllowedOutboundPayload(payload)) return false;
    }
    return !CTraderGate7Config::isAllowedOutboundPayload(2106)
        && !CTraderGate7Config::isAllowedOutboundPayload(2155);
}

int runCTraderGate7Proof(bool preflightOnly)
{
    try {
        std::cout << "gate7_offline_controls_verified_required\n";
        if (!validateCTraderGate7OfflineConfiguration()) {
            return fail(RuntimeFailure::InvalidConfiguration);
        }
        if (!disableCoreDumps()) return fail(RuntimeFailure::LocalHardening);
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
            return fail(RuntimeFailure::TokenExchangeFailed);
        }

        auto clientId = loadClientId();
        if (!clientId.has_value()) {
            curl_global_cleanup();
            return fail(RuntimeFailure::MissingClientId);
        }
        if (preflightOnly) {
            const bool secretPresent = keychainItemPresentOnly(
                CTraderGate7Config::CLIENT_SECRET_SERVICE);
            clientId->clear(); clientId.reset();
            curl_global_cleanup();
            if (!secretPresent) return fail(RuntimeFailure::MissingClientSecret);
            std::cout << "gate7_secret_prerequisites_ready\n"
                         "gate7_provider_traffic_not_started\n";
            return 0;
        }
        Sensitive clientSecret;
        const RuntimeFailure secret = readKeychainValue(
            CTraderGate7Config::CLIENT_SECRET_SERVICE, clientSecret);
        if (secret != RuntimeFailure::None) {
            clientId->clear(); clientId.reset(); curl_global_cleanup();
            return fail(secret == RuntimeFailure::TokenUnavailable
                            ? RuntimeFailure::MissingClientSecret : secret);
        }
        TokenEnvelope stored;
        Sensitive storedBytes;
        RuntimeFailure storedResult = readKeychainValue(
            CTraderGate7Config::TOKEN_SERVICE, storedBytes);
        if (storedResult == RuntimeFailure::None) {
            (void)parseStoredToken(storedBytes.view(), stored);
        }
        storedBytes.clear();
        TokenEnvelope token;
        if (tokenUsable(stored)) {
            token.accessToken = Sensitive(std::string(stored.accessToken.view()));
            token.refreshToken = Sensitive(std::string(stored.refreshToken.view()));
            token.tokenType = Sensitive(std::string(stored.tokenType.view()));
            token.expiresAtEpochSeconds = stored.expiresAtEpochSeconds;
            token.scope = stored.scope;
        } else if (!stored.refreshToken.empty()) {
            const RuntimeFailure refreshed = refreshTokenOnce(
                stored, clientId->view(), clientSecret.view(), token);
            clearToken(stored);
            if (refreshed != RuntimeFailure::None) {
                clientSecret.clear(); clientId->clear(); clientId.reset();
                curl_global_cleanup();
                return fail(RuntimeFailure::TokenRefreshFailed);
            }
        } else {
            clearToken(stored);
            std::cout << "gate7_oauth_authorization_starting\n";
            Gate7OAuthFailure oauthFailure = Gate7OAuthFailure::None;
            auto code = authorizeInBrowser(clientId->view(), oauthFailure);
            if (!code.has_value()) {
                clientSecret.clear(); clientId->clear(); clientId.reset();
                curl_global_cleanup();
                return failOAuth(oauthFailure);
            }
            CURL* encoder = curl_easy_init();
            auto encodedCode = encoder == nullptr
                ? std::optional<std::string>{}
                : urlEncode(encoder, code->view());
            auto encodedClient = encoder == nullptr
                ? std::optional<std::string>{}
                : urlEncode(encoder, clientId->view());
            auto encodedSecret = encoder == nullptr
                ? std::optional<std::string>{}
                : urlEncode(encoder, clientSecret.view());
            auto encodedRedirect = encoder == nullptr
                ? std::optional<std::string>{}
                : urlEncode(encoder, CTraderGate7Config::REDIRECT_URI);
            if (encoder != nullptr) curl_easy_cleanup(encoder);
            code->clear(); code.reset();
            if (!encodedCode.has_value() || !encodedClient.has_value()
                || !encodedSecret.has_value() || !encodedRedirect.has_value()) {
                if (encodedCode.has_value()) secureClear(*encodedCode);
                if (encodedClient.has_value()) secureClear(*encodedClient);
                if (encodedSecret.has_value()) secureClear(*encodedSecret);
                if (encodedRedirect.has_value()) secureClear(*encodedRedirect);
                clientSecret.clear(); clientId->clear(); clientId.reset();
                curl_global_cleanup();
                return fail(RuntimeFailure::TokenExchangeFailed);
            }
            std::string raw;
            raw = "https://openapi.ctrader.com/apps/token?grant_type=authorization_code&code=";
            raw += *encodedCode; raw += "&redirect_uri="; raw += *encodedRedirect;
            raw += "&client_id="; raw += *encodedClient;
            raw += "&client_secret="; raw += *encodedSecret;
            secureClear(*encodedCode); secureClear(*encodedClient);
            secureClear(*encodedSecret); secureClear(*encodedRedirect);
            Sensitive url(std::move(raw));
            const RuntimeFailure exchanged = obtainTokenFromUrl(url, token);
            if (exchanged != RuntimeFailure::None) {
                clientSecret.clear(); clientId->clear(); clientId.reset();
                curl_global_cleanup();
                return fail(exchanged);
            }
        }
        clearToken(stored);
        if (!tokenUsable(token)) {
            clearToken(token); clientSecret.clear(); clientId->clear(); clientId.reset();
            curl_global_cleanup();
            return fail(RuntimeFailure::TokenRejected);
        }

        StrictTransport transport;
        if (!transport.connectDemo()) {
            clearToken(token); clientSecret.clear(); clientId->clear(); clientId.reset();
            curl_global_cleanup();
            return fail(RuntimeFailure::DemoTlsConnection);
        }
        CTraderGate7Proof proof(transport.generation());
        if (!applicationAuthenticate(transport, clientId->view(), clientSecret.view())) {
            transport.close(); clearToken(token); clientSecret.clear(); clientId->clear(); clientId.reset();
            curl_global_cleanup(); return fail(RuntimeFailure::ApplicationAuthentication);
        }
        auto accounts = discoverAccounts(transport, token.accessToken.view());
        if (!accounts.has_value()
            || proof.acceptAccountList(std::move(*accounts))
                != Gate7Decision::AccountAuthenticationReady) {
            transport.close(); clearToken(token); clientSecret.clear(); clientId->clear(); clientId.reset();
            curl_global_cleanup(); return fail(RuntimeFailure::AccountSelection);
        }
        auto selectedAccount = proof.accountIdForAuthentication();
        if (!selectedAccount.has_value()) {
            transport.close(); clearToken(token); clientSecret.clear(); clientId->clear(); clientId.reset();
            curl_global_cleanup(); return fail(RuntimeFailure::AccountSelection);
        }
        std::int64_t authenticatedId = 0;
        VolatileId requestAccount{*selectedAccount};
        secureClear(*selectedAccount);
        selectedAccount.reset();
        if (!authenticateAccount(transport, token.accessToken.view(),
                                 requestAccount.value, authenticatedId)
            || proof.acceptAccountAuthentication(authenticatedId)
                != Gate7Decision::SymbolListReady) {
            secureClear(authenticatedId); transport.close(); clearToken(token);
            clientSecret.clear(); clientId->clear(); clientId.reset(); curl_global_cleanup();
            return fail(RuntimeFailure::AccountAuthentication);
        }
        secureClear(authenticatedId);

        auto symbols = requestSymbols(transport, requestAccount.value);
        if (!symbols.has_value()
            || proof.acceptSymbolsList(std::move(*symbols))
                != Gate7Decision::FullSymbolReady) {
            transport.close(); clearToken(token); clientSecret.clear(); clientId->clear(); clientId.reset(); curl_global_cleanup();
            return fail(RuntimeFailure::SymbolList);
        }
        auto selectedSymbol = proof.symbolIdForFullRequest();
        if (!selectedSymbol.has_value()) {
            transport.close(); clearToken(token); clientSecret.clear(); clientId->clear(); clientId.reset(); curl_global_cleanup();
            return fail(RuntimeFailure::FullSymbol);
        }
        VolatileId requestSymbol{*selectedSymbol};
        secureClear(*selectedSymbol);
        selectedSymbol.reset();
        auto full = requestFullSymbol(transport, requestAccount.value,
                                      requestSymbol.value);
        if (!full.has_value()
            || proof.acceptFullSymbol(std::move(*full))
                != Gate7Decision::SubscriptionReady) {
            transport.close(); clearToken(token); clientSecret.clear(); clientId->clear(); clientId.reset(); curl_global_cleanup();
            return fail(RuntimeFailure::FullSymbol);
        }
        auto subscriptionSymbol = proof.symbolIdForSubscription();
        if (!subscriptionSymbol.has_value()) {
            transport.close(); clearToken(token); clientSecret.clear(); clientId->clear(); clientId.reset(); curl_global_cleanup();
            return fail(RuntimeFailure::Subscription);
        }
        VolatileId subscribedSymbol{*subscriptionSymbol};
        secureClear(*subscriptionSymbol);
        subscriptionSymbol.reset();
        auto subscription = subscribeToSpot(transport, requestAccount.value,
                                            subscribedSymbol.value);
        if (!subscription.has_value()
            || proof.acceptSubscription(std::move(*subscription))
                != Gate7Decision::SubscriptionReady) {
            transport.close(); clearToken(token); clientSecret.clear(); clientId->clear(); clientId.reset(); curl_global_cleanup();
            return fail(RuntimeFailure::Subscription);
        }
        Gate7Decision quoteDecision = Gate7Decision::Timeout;
        const bool received = receiveFirstSpot(transport, requestAccount.value,
                                               subscribedSymbol.value, proof,
                                               quoteDecision);
        transport.close();
        const auto quote = proof.quoteEvidence();
        clearToken(token); clientSecret.clear(); clientId->clear(); clientId.reset();
        curl_global_cleanup();
        if (!received || quoteDecision != Gate7Decision::QuoteProofSucceeded
            || !quote.has_value()) {
            return fail(received ? RuntimeFailure::SpotProof
                                 : RuntimeFailure::SpotTimeout);
        }

        std::cout << "gate7_provider_sequence_complete\n"
                     "canonical_symbol=XAUUSD\n"
                  << "symbol_name=" << quote->executionAlias << '\n'
                  << "digits=" << static_cast<unsigned>(quote->instrument.tickSize.scale) << '\n'
                  << "pip_position=" << quote->pipPosition << '\n'
                  << "min_volume=" << decimalEvidence(quote->instrument.minimumQuantity) << '\n'
                  << "max_volume=" << decimalEvidence(quote->instrument.maximumQuantity) << '\n'
                  << "step_volume=" << decimalEvidence(quote->instrument.quantityStep) << '\n'
                  << "lot_size=" << decimalEvidence(quote->instrument.contractSize) << '\n'
                  << "bid=" << decimalEvidence(quote->bid) << '\n'
                  << "ask=" << decimalEvidence(quote->ask) << '\n'
                  << "spread=" << decimalEvidence(quote->spread) << '\n'
                  << "timestamp_unit=" << CTraderGate7Proof::timestampUnitName(quote->timestamp.unit) << '\n'
                  << "timestamp_utc=" << utcSecond(quote->timestamp.timestampNs) << '\n'
                  << "receipt_utc=" << utcSecond(quote->timestamp.receiptTimestampNs) << '\n'
                  << "freshness_delta_ns=" << quote->timestamp.freshnessDeltaNs << '\n'
                  <<
                     "gate7_exit_code=0\n";
        return 0;
    } catch (...) {
        return fail(RuntimeFailure::ResourceExhausted);
    }
}

} // namespace tradebot::ctrader
