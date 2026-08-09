#include "CTraderGate6Runtime.hpp"

#include "CTraderGate6Proof.hpp"
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
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <poll.h>
#include <google/protobuf/runtime_version.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tradebot::ctrader {
namespace {

static_assert(PROTOBUF_VERSION == 7035001,
              "Gate 6 requires Protobuf C++ runtime headers 7.35.1");

using Clock = std::chrono::steady_clock;
constexpr std::size_t MAX_HTTP_REQUEST_BYTES = 8192;
constexpr std::size_t MAX_TOKEN_RESPONSE_BYTES = 65536;
constexpr uint32_t MAX_PROTO_FRAME_BYTES = 4U * 1024U * 1024U;
constexpr std::chrono::seconds NETWORK_TIMEOUT{8};
constexpr std::chrono::seconds CLEAN_REDIRECT_TIMEOUT{2};

enum class RuntimeFailure {
    None,
    LocalHardeningFailed,
    MissingClientId,
    MissingClientSecret,
    KeychainReadFailed,
    KeychainWriteFailed,
    ListenerBindFailed,
    BrowserOpenFailed,
    OAuthCorrelationFailed,
    OAuthTimeout,
    TokenTransportFailed,
    TokenResponseRejected,
    DemoTlsConnectionFailed,
    ApplicationAuthFailed,
    AccountDiscoveryFailed,
    AccountSelectionFailed,
    CheckpointCancelled,
    AccountAuthenticationFailed
};

std::string_view safeRuntimeDiagnostic(RuntimeFailure failure) noexcept
{
    switch (failure) {
    case RuntimeFailure::None: return "gate6_runtime_ready";
    case RuntimeFailure::LocalHardeningFailed: return "gate6_local_hardening_failed";
    case RuntimeFailure::MissingClientId: return "gate6_client_id_missing";
    case RuntimeFailure::MissingClientSecret: return "gate6_client_secret_missing";
    case RuntimeFailure::KeychainReadFailed: return "gate6_keychain_read_failed";
    case RuntimeFailure::KeychainWriteFailed: return "gate6_keychain_write_failed";
    case RuntimeFailure::ListenerBindFailed: return "gate6_loopback_listener_failed";
    case RuntimeFailure::BrowserOpenFailed: return "gate6_browser_open_failed";
    case RuntimeFailure::OAuthCorrelationFailed: return "gate6_oauth_correlation_failed";
    case RuntimeFailure::OAuthTimeout: return "gate6_oauth_timeout";
    case RuntimeFailure::TokenTransportFailed: return "gate6_token_transport_failed";
    case RuntimeFailure::TokenResponseRejected: return "gate6_token_response_rejected";
    case RuntimeFailure::DemoTlsConnectionFailed: return "gate6_demo_tls_failed";
    case RuntimeFailure::ApplicationAuthFailed: return "gate6_application_auth_failed";
    case RuntimeFailure::AccountDiscoveryFailed: return "gate6_account_discovery_failed";
    case RuntimeFailure::AccountSelectionFailed: return "gate6_account_selection_failed";
    case RuntimeFailure::CheckpointCancelled: return "gate6_checkpoint_cancelled";
    case RuntimeFailure::AccountAuthenticationFailed: return "gate6_account_auth_failed";
    }
    return "gate6_runtime_unknown_failure";
}

void secureClear(std::string& value) noexcept
{
    volatile char* bytes = value.empty() ? nullptr : value.data();
    for (std::size_t i = 0; i < value.size(); ++i) {
        bytes[i] = 0;
    }
    value.clear();
}

void secureClearBytes(void* bytes, std::size_t size) noexcept
{
    volatile unsigned char* target = static_cast<volatile unsigned char*>(bytes);
    for (std::size_t i = 0; i < size; ++i) {
        target[i] = 0;
    }
}

void secureClear(int64_t& value) noexcept
{
    volatile int64_t* target = &value;
    *target = 0;
}

bool constantTimeEqual(std::string_view lhs, std::string_view rhs) noexcept
{
    if (lhs.size() != rhs.size()) {
        return false;
    }
    uint8_t difference = 0;
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        difference |= static_cast<uint8_t>(lhs[i])
                    ^ static_cast<uint8_t>(rhs[i]);
    }
    return difference == 0;
}

bool isBoundedText(std::string_view value, std::size_t maximum) noexcept
{
    if (value.empty() || value.size() > maximum) {
        return false;
    }
    for (const unsigned char c : value) {
        if (c < 0x20 || c == 0x7f) {
            return false;
        }
    }
    return true;
}

bool disableCoreDumps() noexcept
{
    const rlimit noCore{0, 0};
    return ::setrlimit(RLIMIT_CORE, &noCore) == 0;
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

std::optional<std::string> localUserName() noexcept
{
    const char* user = std::getenv("USER");
    if (user == nullptr) {
        return std::nullopt;
    }
    std::string value(user);
    if (!isBoundedText(value, 256)) {
        secureClear(value);
        return std::nullopt;
    }
    return value;
}

RuntimeFailure readKeychainSecret(std::string_view service,
                                  SensitiveString& output) noexcept
{
    const std::optional<std::string> user = localUserName();
    if (!user.has_value()) {
        return RuntimeFailure::KeychainReadFailed;
    }

    CFStringRef serviceRef = makeCfString(service);
    CFStringRef accountRef = makeCfString(*user);
    if (serviceRef == nullptr || accountRef == nullptr) {
        if (serviceRef != nullptr) CFRelease(serviceRef);
        if (accountRef != nullptr) CFRelease(accountRef);
        return RuntimeFailure::KeychainReadFailed;
    }

    const void* keys[] = {
        kSecClass, kSecAttrService, kSecAttrAccount,
        kSecReturnData, kSecMatchLimit
    };
    const void* values[] = {
        kSecClassGenericPassword, serviceRef, accountRef,
        kCFBooleanTrue, kSecMatchLimitOne
    };
    CFDictionaryRef query = CFDictionaryCreate(
        kCFAllocatorDefault, keys, values, 5,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    CFTypeRef result = nullptr;
    const OSStatus status = query == nullptr
        ? errSecAllocate
        : SecItemCopyMatching(query, &result);

    if (query != nullptr) CFRelease(query);
    CFRelease(serviceRef);
    CFRelease(accountRef);

    if (status == errSecItemNotFound) {
        return RuntimeFailure::MissingClientSecret;
    }
    if (status != errSecSuccess || result == nullptr
        || CFGetTypeID(result) != CFDataGetTypeID()) {
        if (result != nullptr) CFRelease(result);
        return RuntimeFailure::KeychainReadFailed;
    }

    CFDataRef data = reinterpret_cast<CFDataRef>(result);
    const CFIndex length = CFDataGetLength(data);
    const UInt8* bytes = CFDataGetBytePtr(data);
    if (length <= 0 || length > 4096 || bytes == nullptr) {
        if (length > 0 && bytes != nullptr) {
            secureClearBytes(const_cast<UInt8*>(bytes),
                             static_cast<std::size_t>(length));
        }
        CFRelease(result);
        return RuntimeFailure::KeychainReadFailed;
    }

    std::string value(reinterpret_cast<const char*>(bytes),
                      static_cast<std::size_t>(length));
    secureClearBytes(const_cast<UInt8*>(bytes),
                     static_cast<std::size_t>(length));
    CFRelease(result);
    if (!isBoundedText(value, 4096)) {
        secureClear(value);
        return RuntimeFailure::KeychainReadFailed;
    }
    output = SensitiveString(std::move(value));
    return RuntimeFailure::None;
}

RuntimeFailure writeKeychainValue(std::string_view service,
                                  std::string_view value) noexcept
{
    const std::optional<std::string> user = localUserName();
    if (!user.has_value() || value.empty()) {
        return RuntimeFailure::KeychainWriteFailed;
    }

    CFStringRef serviceRef = makeCfString(service);
    CFStringRef accountRef = makeCfString(*user);
    CFMutableDataRef dataRef = CFDataCreateMutable(
        kCFAllocatorDefault, static_cast<CFIndex>(value.size()));
    if (dataRef != nullptr) {
        CFDataAppendBytes(dataRef,
            reinterpret_cast<const UInt8*>(value.data()),
            static_cast<CFIndex>(value.size()));
    }
    if (serviceRef == nullptr || accountRef == nullptr || dataRef == nullptr) {
        if (serviceRef != nullptr) CFRelease(serviceRef);
        if (accountRef != nullptr) CFRelease(accountRef);
        if (dataRef != nullptr) {
            if (CFDataGetLength(dataRef) > 0) {
                secureClearBytes(CFDataGetMutableBytePtr(dataRef),
                    static_cast<std::size_t>(CFDataGetLength(dataRef)));
            }
            CFRelease(dataRef);
        }
        return RuntimeFailure::KeychainWriteFailed;
    }

    const void* queryKeys[] = {kSecClass, kSecAttrService, kSecAttrAccount};
    const void* queryValues[] = {
        kSecClassGenericPassword, serviceRef, accountRef
    };
    CFDictionaryRef query = CFDictionaryCreate(
        kCFAllocatorDefault, queryKeys, queryValues, 3,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);
    const void* updateKeys[] = {kSecValueData};
    const void* updateValues[] = {dataRef};
    CFDictionaryRef update = CFDictionaryCreate(
        kCFAllocatorDefault, updateKeys, updateValues, 1,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    OSStatus status = (query == nullptr || update == nullptr)
        ? errSecAllocate
        : SecItemUpdate(query, update);
    if (status == errSecItemNotFound && query != nullptr) {
        const void* addKeys[] = {
            kSecClass, kSecAttrService, kSecAttrAccount, kSecValueData
        };
        const void* addValues[] = {
            kSecClassGenericPassword, serviceRef, accountRef, dataRef
        };
        CFDictionaryRef add = CFDictionaryCreate(
            kCFAllocatorDefault, addKeys, addValues, 4,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);
        status = add == nullptr ? errSecAllocate : SecItemAdd(add, nullptr);
        if (add != nullptr) CFRelease(add);
    }

    if (query != nullptr) CFRelease(query);
    if (update != nullptr) CFRelease(update);
    if (CFDataGetLength(dataRef) > 0) {
        secureClearBytes(CFDataGetMutableBytePtr(dataRef),
                         static_cast<std::size_t>(CFDataGetLength(dataRef)));
    }
    CFRelease(dataRef);
    CFRelease(serviceRef);
    CFRelease(accountRef);
    return status == errSecSuccess ? RuntimeFailure::None
                                   : RuntimeFailure::KeychainWriteFailed;
}

std::optional<SensitiveString> loadClientId() noexcept
{
    const char* raw = std::getenv("TRADEBOT_CTRADER_CLIENT_ID");
    if (raw == nullptr) {
        return std::nullopt;
    }
    std::string value(raw);
    if (!isBoundedText(value, 512)
        || value == "REPLACE_WITH_LOCAL_CLIENT_ID") {
        secureClear(value);
        return std::nullopt;
    }
    return SensitiveString(std::move(value));
}

bool openBrowserUrl(std::string_view url) noexcept
{
    @autoreleasepool {
        NSString* text = [[NSString alloc]
            initWithBytes:url.data()
                   length:url.size()
                 encoding:NSUTF8StringEncoding];
        if (text == nil) {
            return false;
        }
        NSURL* target = [NSURL URLWithString:text];
        const BOOL opened = target != nil
            && [[NSWorkspace sharedWorkspace] openURL:target];
        [text release];
        return opened == YES;
    }
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
        const int timeout = static_cast<int>(
            std::clamp<int64_t>(remaining.count(), 1, 1000));
        pollfd descriptor{fd, events, 0};
        const int result = ::poll(&descriptor, 1, timeout);
        if (result > 0) {
            return (descriptor.revents & events) != 0
                && (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) == 0;
        }
        if (result < 0 && errno != EINTR) {
            return false;
        }
    }
    return false;
}

bool sendFixedHttpResponse(int fd, std::string_view response) noexcept
{
    std::size_t offset = 0;
    while (offset < response.size()) {
        const ssize_t count = ::send(fd, response.data() + offset,
                                     response.size() - offset, 0);
        if (count > 0) {
            offset += static_cast<std::size_t>(count);
            continue;
        }
        if (count < 0 && errno == EINTR) {
            continue;
        }
        return false;
    }
    return true;
}

bool readHttpHeaders(int fd, std::string& output) noexcept
{
    output.clear();
    output.reserve(2048);
    std::array<char, 1024> buffer{};
    while (output.size() < MAX_HTTP_REQUEST_BYTES) {
        const ssize_t count = ::recv(fd, buffer.data(), buffer.size(), 0);
        if (count > 0) {
            output.append(buffer.data(), static_cast<std::size_t>(count));
            if (output.find("\r\n\r\n") != std::string::npos) {
                return true;
            }
            continue;
        }
        if (count < 0 && errno == EINTR) {
            continue;
        }
        return false;
    }
    return false;
}

struct ParsedHttpRequest {
    std::string_view method;
    std::string_view path;
    std::string_view query;
    std::string_view host;
};

std::optional<ParsedHttpRequest> parseHttpRequest(std::string_view raw) noexcept
{
    const std::size_t firstLineEnd = raw.find("\r\n");
    if (firstLineEnd == std::string_view::npos) {
        return std::nullopt;
    }
    const std::string_view firstLine = raw.substr(0, firstLineEnd);
    const std::size_t firstSpace = firstLine.find(' ');
    const std::size_t secondSpace = firstSpace == std::string_view::npos
        ? std::string_view::npos : firstLine.find(' ', firstSpace + 1);
    if (firstSpace == std::string_view::npos
        || secondSpace == std::string_view::npos
        || firstLine.substr(secondSpace + 1) != "HTTP/1.1") {
        return std::nullopt;
    }
    const std::string_view method = firstLine.substr(0, firstSpace);
    const std::string_view target = firstLine.substr(
        firstSpace + 1, secondSpace - firstSpace - 1);
    const std::size_t question = target.find('?');
    const std::string_view path = target.substr(0, question);
    const std::string_view query = question == std::string_view::npos
        ? std::string_view{} : target.substr(question + 1);

    std::string_view host;
    std::size_t cursor = firstLineEnd + 2;
    while (cursor < raw.size()) {
        const std::size_t end = raw.find("\r\n", cursor);
        if (end == std::string_view::npos || end == cursor) {
            break;
        }
        const std::string_view line = raw.substr(cursor, end - cursor);
        const std::size_t colon = line.find(':');
        if (colon == std::string_view::npos) {
            return std::nullopt;
        }
        const std::string_view name = line.substr(0, colon);
        std::string_view value = line.substr(colon + 1);
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
            value.remove_prefix(1);
        }
        if (name == "Host" || name == "host") {
            if (!host.empty()) {
                return std::nullopt;
            }
            host = value;
        }
        cursor = end + 2;
    }
    if (host.empty()) {
        return std::nullopt;
    }
    return ParsedHttpRequest{method, path, query, host};
}

int hexValue(char c) noexcept
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::optional<std::string> percentDecode(std::string_view value) noexcept
{
    std::string decoded;
    decoded.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%') {
            if (i + 2 >= value.size()) {
                secureClear(decoded);
                return std::nullopt;
            }
            const int high = hexValue(value[i + 1]);
            const int low = hexValue(value[i + 2]);
            if (high < 0 || low < 0) {
                secureClear(decoded);
                return std::nullopt;
            }
            decoded.push_back(static_cast<char>((high << 4) | low));
            i += 2;
        } else if (value[i] == '+') {
            decoded.push_back(' ');
        } else {
            decoded.push_back(value[i]);
        }
    }
    if (!isBoundedText(decoded, 4096)) {
        secureClear(decoded);
        return std::nullopt;
    }
    return decoded;
}

std::optional<SensitiveString> extractAuthorizationCode(
    std::string_view query) noexcept
{
    std::size_t start = 0;
    while (start < query.size()) {
        const std::size_t end = query.find('&', start);
        const std::string_view parameter = query.substr(
            start, end == std::string_view::npos ? query.size() - start
                                                 : end - start);
        const std::size_t equals = parameter.find('=');
        if (equals != std::string_view::npos
            && parameter.substr(0, equals) == "code") {
            std::optional<std::string> decoded =
                percentDecode(parameter.substr(equals + 1));
            if (!decoded.has_value()) {
                return std::nullopt;
            }
            return SensitiveString(std::move(*decoded));
        }
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return std::nullopt;
}

std::optional<std::string> urlEncode(CURL* curl, std::string_view value) noexcept
{
    char* escaped = curl_easy_escape(curl, value.data(),
                                     static_cast<int>(value.size()));
    if (escaped == nullptr) {
        return std::nullopt;
    }
    std::string result(escaped);
    curl_free(escaped);
    return result;
}

struct OAuthResult {
    RuntimeFailure failure{RuntimeFailure::OAuthCorrelationFailed};
    std::optional<SensitiveString> code;
};

OAuthResult receiveCorrelatedAuthorizationCode(std::string_view clientId) noexcept
{
    int listener = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        return {RuntimeFailure::ListenerBindFailed, std::nullopt};
    }
    int reuse = 1;
    (void)::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(CTraderOAuthCorrelationGuard::LOOPBACK_PORT);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0
        || ::listen(listener, 2) != 0
        || !setNonBlocking(listener)) {
        ::close(listener);
        return {RuntimeFailure::ListenerBindFailed, std::nullopt};
    }

    CTraderOAuthCorrelationGuard guard;
    const auto armedAt = CTraderOAuthCorrelationGuard::Clock::now();
    if (!guard.arm({CTraderOAuthCorrelationGuard::LOOPBACK_ADDRESS,
                    CTraderOAuthCorrelationGuard::LOOPBACK_PORT}, armedAt)) {
        ::close(listener);
        return {RuntimeFailure::OAuthCorrelationFailed, std::nullopt};
    }

    CURL* encoder = curl_easy_init();
    if (encoder == nullptr) {
        guard.cancel();
        ::close(listener);
        return {RuntimeFailure::BrowserOpenFailed, std::nullopt};
    }
    std::optional<std::string> encodedClient = urlEncode(encoder, clientId);
    std::optional<std::string> encodedRedirect = urlEncode(
        encoder, CTraderGate6Config::REDIRECT_URI);
    curl_easy_cleanup(encoder);
    if (!encodedClient.has_value() || !encodedRedirect.has_value()) {
        guard.cancel();
        ::close(listener);
        return {RuntimeFailure::BrowserOpenFailed, std::nullopt};
    }

    std::string authorizationUrl =
        "https://id.ctrader.com/my/settings/openapi/grantingaccess/?client_id="
        + *encodedClient + "&redirect_uri=" + *encodedRedirect
        + "&scope=" + std::string(CTraderGate6Config::OAUTH_SCOPE)
        + "&product=web&state="
        + std::string(guard.stateForAuthorizationRequest());
    secureClear(*encodedClient);
    secureClear(*encodedRedirect);
    SensitiveString sensitiveUrl(std::move(authorizationUrl));
    const bool opened = openBrowserUrl(sensitiveUrl.view());
    sensitiveUrl.clear();
    if (!opened) {
        guard.cancel();
        ::close(listener);
        return {RuntimeFailure::BrowserOpenFailed, std::nullopt};
    }

    const auto deadline = armedAt + CTraderOAuthCorrelationGuard::CORRELATION_LIFETIME;
    int client = -1;
    sockaddr_in remote{};
    while (Clock::now() < deadline) {
        if (!waitForFd(listener, POLLIN, deadline)) {
            break;
        }
        remote = {};
        socklen_t remoteLength = sizeof(remote);
        client = ::accept(listener, reinterpret_cast<sockaddr*>(&remote),
                          &remoteLength);
        if (client >= 0) {
            break;
        }
        if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
            break;
        }
    }
    if (client < 0) {
        const auto terminalDecision = guard.expireIfDue(
            CTraderOAuthCorrelationGuard::Clock::now());
        if (terminalDecision
            != CTraderOAuthCorrelationGuard::Decision::CallbackExpired) {
            (void)guard.cancel();
        }
        ::close(listener);
        return {RuntimeFailure::OAuthTimeout, std::nullopt};
    }

    timeval timeout{2, 0};
    (void)::setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    std::string rawRequest;
    if (!readHttpHeaders(client, rawRequest)) {
        guard.cancel();
        ::close(client);
        ::close(listener);
        secureClear(rawRequest);
        return {RuntimeFailure::OAuthCorrelationFailed, std::nullopt};
    }

    const std::optional<ParsedHttpRequest> request = parseHttpRequest(rawRequest);
    if (!request.has_value()) {
        guard.cancel();
        (void)sendFixedHttpResponse(client,
            "HTTP/1.1 400 Bad Request\r\nConnection: close\r\nContent-Length: 0\r\n\r\n");
        ::close(client);
        ::close(listener);
        secureClear(rawRequest);
        return {RuntimeFailure::OAuthCorrelationFailed, std::nullopt};
    }

    char remoteText[INET_ADDRSTRLEN]{};
    const char* converted = ::inet_ntop(AF_INET, &remote.sin_addr,
                                        remoteText, sizeof(remoteText));
    const std::string_view remoteAddress = converted == nullptr
        ? std::string_view{} : std::string_view(remoteText);
    const auto decision = guard.consume(
        {remoteAddress, request->method, request->host, request->path,
         request->query},
        CTraderOAuthCorrelationGuard::Clock::now());
    if (decision != CTraderOAuthCorrelationGuard::Decision::CorrelationMatchedCodeDiscarded) {
        (void)sendFixedHttpResponse(client,
            "HTTP/1.1 400 Bad Request\r\nConnection: close\r\nContent-Length: 0\r\n\r\n");
        ::close(client);
        ::close(listener);
        secureClear(rawRequest);
        return {RuntimeFailure::OAuthCorrelationFailed, std::nullopt};
    }

    std::optional<SensitiveString> code = extractAuthorizationCode(request->query);
    (void)sendFixedHttpResponse(client,
        "HTTP/1.1 303 See Other\r\nLocation: /ctrader/oauth/complete\r\n"
        "Cache-Control: no-store\r\nConnection: close\r\nContent-Length: 0\r\n\r\n");
    ::close(client);
    secureClear(rawRequest);
    if (!code.has_value()) {
        ::close(listener);
        return {RuntimeFailure::OAuthCorrelationFailed, std::nullopt};
    }

    const auto cleanDeadline = Clock::now() + CLEAN_REDIRECT_TIMEOUT;
    if (waitForFd(listener, POLLIN, cleanDeadline)) {
        sockaddr_in cleanRemote{};
        socklen_t cleanLength = sizeof(cleanRemote);
        const int cleanClient = ::accept(listener,
            reinterpret_cast<sockaddr*>(&cleanRemote), &cleanLength);
        if (cleanClient >= 0) {
            (void)::setsockopt(cleanClient, SOL_SOCKET, SO_RCVTIMEO,
                               &timeout, sizeof(timeout));
            std::string cleanRequest;
            if (readHttpHeaders(cleanClient, cleanRequest)) {
                const std::optional<ParsedHttpRequest> parsed =
                    parseHttpRequest(cleanRequest);
                if (parsed.has_value() && parsed->method == "GET"
                    && parsed->path == "/ctrader/oauth/complete"
                    && parsed->query.empty()
                    && parsed->host == CTraderOAuthCorrelationGuard::CALLBACK_HOST) {
                    (void)sendFixedHttpResponse(cleanClient,
                        "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\n"
                        "Cache-Control: no-store\r\nConnection: close\r\n"
                        "Content-Length: 43\r\n\r\nAuthorization received. Return to TradeBot.");
                }
            }
            secureClear(cleanRequest);
            ::close(cleanClient);
        }
    }
    ::close(listener);
    return {RuntimeFailure::None, std::move(code)};
}

class JsonCursor {
public:
    explicit JsonCursor(std::string_view input) : input_(input) {}

    void skipWhitespace() noexcept
    {
        while (position_ < input_.size()
               && (input_[position_] == ' ' || input_[position_] == '\t'
                   || input_[position_] == '\r' || input_[position_] == '\n')) {
            ++position_;
        }
    }

    bool consume(char expected) noexcept
    {
        skipWhitespace();
        if (position_ >= input_.size() || input_[position_] != expected) {
            return false;
        }
        ++position_;
        return true;
    }

    bool atEnd() noexcept
    {
        skipWhitespace();
        return position_ == input_.size();
    }

    std::optional<std::string> stringValue() noexcept
    {
        skipWhitespace();
        if (position_ >= input_.size() || input_[position_] != '"') {
            return std::nullopt;
        }
        ++position_;
        std::string result;
        while (position_ < input_.size()) {
            const char c = input_[position_++];
            if (c == '"') {
                return result;
            }
            if (static_cast<unsigned char>(c) < 0x20) {
                secureClear(result);
                return std::nullopt;
            }
            if (c != '\\') {
                result.push_back(c);
                continue;
            }
            if (position_ >= input_.size()) {
                secureClear(result);
                return std::nullopt;
            }
            const char escaped = input_[position_++];
            switch (escaped) {
            case '"': result.push_back('"'); break;
            case '\\': result.push_back('\\'); break;
            case '/': result.push_back('/'); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            default:
                secureClear(result);
                return std::nullopt;
            }
        }
        secureClear(result);
        return std::nullopt;
    }

    std::optional<int64_t> integerValue() noexcept
    {
        skipWhitespace();
        const std::size_t start = position_;
        if (position_ < input_.size() && input_[position_] == '-') ++position_;
        while (position_ < input_.size()
               && input_[position_] >= '0' && input_[position_] <= '9') {
            ++position_;
        }
        if (position_ == start) return std::nullopt;
        int64_t value = 0;
        const auto parsed = std::from_chars(input_.data() + start,
                                            input_.data() + position_, value);
        if (parsed.ec != std::errc{} || parsed.ptr != input_.data() + position_) {
            return std::nullopt;
        }
        return value;
    }

    bool nullValue() noexcept
    {
        skipWhitespace();
        if (input_.substr(position_, 4) != "null") return false;
        position_ += 4;
        return true;
    }

private:
    std::string_view input_;
    std::size_t position_{0};
};

struct TokenEnvelope {
    SensitiveString accessToken;
    SensitiveString refreshToken;
    SensitiveString tokenType;
    int64_t expiresAtEpochSeconds{0};
};

bool tokenHasExecutionLifetime(const TokenEnvelope& token) noexcept
{
    constexpr int64_t MINIMUM_REMAINING_SECONDS = 60;
    const int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return token.expiresAtEpochSeconds > now
        && token.expiresAtEpochSeconds - now >= MINIMUM_REMAINING_SECONDS;
}

bool parseTokenResponse(std::string_view body, TokenEnvelope& output) noexcept
{
    JsonCursor cursor(body);
    if (!cursor.consume('{')) return false;
    bool haveAccess = false;
    bool haveRefresh = false;
    bool haveType = false;
    bool haveExpires = false;
    bool errorWasNull = false;
    std::vector<std::string> keys;

    while (true) {
        cursor.skipWhitespace();
        if (cursor.consume('}')) break;
        std::optional<std::string> key = cursor.stringValue();
        if (!key.has_value() || !cursor.consume(':')
            || std::find(keys.begin(), keys.end(), *key) != keys.end()) {
            if (key.has_value()) secureClear(*key);
            return false;
        }
        keys.push_back(*key);

        if (*key == "accessToken" || *key == "refreshToken"
            || *key == "tokenType") {
            std::optional<std::string> value = cursor.stringValue();
            if (!value.has_value() || !isBoundedText(*value, 8192)) {
                if (value.has_value()) secureClear(*value);
                secureClear(*key);
                return false;
            }
            if (*key == "accessToken") {
                output.accessToken = SensitiveString(std::move(*value));
                haveAccess = true;
            } else if (*key == "refreshToken") {
                output.refreshToken = SensitiveString(std::move(*value));
                haveRefresh = true;
            } else {
                output.tokenType = SensitiveString(std::move(*value));
                haveType = true;
            }
        } else if (*key == "expiresIn") {
            const std::optional<int64_t> value = cursor.integerValue();
            if (!value.has_value() || *value <= 0
                || *value > 10LL * 365LL * 24LL * 60LL * 60LL) {
                secureClear(*key);
                return false;
            }
            const int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            if (now > std::numeric_limits<int64_t>::max() - *value) {
                secureClear(*key);
                return false;
            }
            output.expiresAtEpochSeconds = now + *value;
            haveExpires = true;
        } else if (*key == "errorCode") {
            if (!cursor.nullValue()) {
                std::optional<std::string> ignored = cursor.stringValue();
                if (ignored.has_value()) secureClear(*ignored);
                secureClear(*key);
                return false;
            }
            errorWasNull = true;
        } else {
            std::optional<std::string> ignored = cursor.stringValue();
            if (ignored.has_value()) {
                secureClear(*ignored);
            } else if (!cursor.nullValue() && !cursor.integerValue().has_value()) {
                secureClear(*key);
                return false;
            }
        }
        secureClear(*key);
        cursor.skipWhitespace();
        if (cursor.consume(',')) continue;
        if (cursor.consume('}')) break;
        return false;
    }

    return cursor.atEnd() && haveAccess && haveRefresh && haveType
        && haveExpires && errorWasNull
        && output.tokenType.view() == "bearer";
}

std::size_t tokenWriteCallback(char* data, std::size_t size,
                               std::size_t count, void* userData) noexcept
{
    const std::size_t bytes = size * count;
    auto* output = static_cast<std::string*>(userData);
    if (output == nullptr || bytes > MAX_TOKEN_RESPONSE_BYTES
        || output->size() > MAX_TOKEN_RESPONSE_BYTES - bytes) {
        return 0;
    }
    output->append(data, bytes);
    return bytes;
}

bool configureTokenCurl(CURL* curl, const char* url, curl_slist* headers,
                        std::string& response) noexcept
{
    return curl != nullptr && url != nullptr && headers != nullptr
        && curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK
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
        && curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L) == CURLE_OK
        && curl_easy_setopt(curl, CURLOPT_USERAGENT, "TradeBot-Gate6/1.0") == CURLE_OK;
}

RuntimeFailure exchangeAuthorizationCode(std::string_view clientId,
                                         std::string_view clientSecret,
                                         std::string_view authorizationCode,
                                         TokenEnvelope& output) noexcept
{
    CURL* curl = curl_easy_init();
    if (curl == nullptr) return RuntimeFailure::TokenTransportFailed;

    std::optional<std::string> encodedCode = urlEncode(curl, authorizationCode);
    std::optional<std::string> encodedRedirect = urlEncode(
        curl, CTraderGate6Config::REDIRECT_URI);
    std::optional<std::string> encodedClient = urlEncode(curl, clientId);
    std::optional<std::string> encodedSecret = urlEncode(curl, clientSecret);
    if (!encodedCode.has_value() || !encodedRedirect.has_value()
        || !encodedClient.has_value() || !encodedSecret.has_value()) {
        curl_easy_cleanup(curl);
        return RuntimeFailure::TokenTransportFailed;
    }

    std::string url = "https://openapi.ctrader.com/apps/token?grant_type="
        "authorization_code&code=" + *encodedCode
        + "&redirect_uri=" + *encodedRedirect
        + "&client_id=" + *encodedClient
        + "&client_secret=" + *encodedSecret;
    secureClear(*encodedCode);
    secureClear(*encodedRedirect);
    secureClear(*encodedClient);
    secureClear(*encodedSecret);
    SensitiveString sensitiveUrl(std::move(url));
    std::string response;

    curl_slist* headers = curl_slist_append(nullptr, "Accept: application/json");
    curl_slist* extendedHeaders = headers == nullptr ? nullptr
        : curl_slist_append(headers, "Cache-Control: no-store");
    if (extendedHeaders == nullptr) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        sensitiveUrl.clear();
        return RuntimeFailure::TokenTransportFailed;
    }
    headers = extendedHeaders;

    const bool configured = configureTokenCurl(
        curl, sensitiveUrl.view().data(), headers, response);
    if (!configured) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        sensitiveUrl.clear();
        return RuntimeFailure::TokenTransportFailed;
    }
    const CURLcode performed = curl_easy_perform(curl);
    long status = 0;
    if (performed == CURLE_OK) {
        (void)curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    sensitiveUrl.clear();

    if (performed != CURLE_OK || status != 200) {
        secureClear(response);
        return RuntimeFailure::TokenTransportFailed;
    }
    const bool parsed = parseTokenResponse(response, output);
    secureClear(response);
    return parsed ? RuntimeFailure::None
                  : RuntimeFailure::TokenResponseRejected;
}

void appendUint32(std::string& output, uint32_t value)
{
    output.push_back(static_cast<char>((value >> 24) & 0xff));
    output.push_back(static_cast<char>((value >> 16) & 0xff));
    output.push_back(static_cast<char>((value >> 8) & 0xff));
    output.push_back(static_cast<char>(value & 0xff));
}

void appendUint64(std::string& output, uint64_t value)
{
    for (int shift = 56; shift >= 0; shift -= 8) {
        output.push_back(static_cast<char>((value >> shift) & 0xff));
    }
}

bool appendSized(std::string& output, std::string_view value)
{
    if (value.size() > std::numeric_limits<uint32_t>::max()) return false;
    appendUint32(output, static_cast<uint32_t>(value.size()));
    output.append(value);
    return true;
}

RuntimeFailure persistTokenEnvelope(const TokenEnvelope& envelope) noexcept
{
    std::string serialized("TBG6TOK1", 8);
    appendUint64(serialized,
        static_cast<uint64_t>(envelope.expiresAtEpochSeconds));
    if (!appendSized(serialized, CTraderGate6Config::OAUTH_SCOPE)
        || !appendSized(serialized, envelope.tokenType.view())
        || !appendSized(serialized, envelope.accessToken.view())
        || !appendSized(serialized, envelope.refreshToken.view())) {
        secureClear(serialized);
        return RuntimeFailure::KeychainWriteFailed;
    }
    SensitiveString encoded(std::move(serialized));
    const RuntimeFailure result = writeKeychainValue(
        CTraderGate6Config::TOKEN_SERVICE, encoded.view());
    encoded.clear();
    return result;
}

class StrictProtobufTransport {
public:
    StrictProtobufTransport() noexcept
        : generation_(++nextGeneration_)
    {
    }

    ~StrictProtobufTransport() { close(); }
    StrictProtobufTransport(const StrictProtobufTransport&) = delete;
    StrictProtobufTransport& operator=(const StrictProtobufTransport&) = delete;

    bool connectDemo() noexcept
    {
        close();
        if (!CTraderGate6Config::isAllowedOpenApiEndpoint(
                CTraderGate6Config::DEMO_HOST, CTraderGate6Config::DEMO_PORT)) {
            return false;
        }

        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* resolved = nullptr;
        const std::string port = std::to_string(CTraderGate6Config::DEMO_PORT);
        if (::getaddrinfo(std::string(CTraderGate6Config::DEMO_HOST).c_str(),
                          port.c_str(), &hints, &resolved) != 0) {
            return false;
        }

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
            || SSL_set_tlsext_host_name(
                ssl_, CTraderGate6Config::DEMO_HOST.data()) != 1
            || SSL_set1_host(ssl_, CTraderGate6Config::DEMO_HOST.data()) != 1) {
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
            } else {
                break;
            }
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

    uint64_t generation() const noexcept { return generation_; }

    std::string nextCorrelation(std::string_view step)
    {
        return "g6-" + std::to_string(generation_) + "-"
             + std::to_string(++sequence_) + "-" + std::string(step);
    }

    bool send(uint32_t payloadType,
              const google::protobuf::MessageLite& message,
              std::string_view clientMsgId) noexcept
    {
        if (!connected_
            || !CTraderGate6Config::isAllowedOutboundPayload(payloadType)
            || clientMsgId.empty() || clientMsgId.size() > 128
            || !message.IsInitialized()) {
            return false;
        }
        std::string payload;
        if (!message.SerializeToString(&payload)) {
            secureClear(payload);
            return false;
        }
        ProtoMessage envelope;
        envelope.set_payloadtype(payloadType);
        envelope.set_payload(payload);
        envelope.set_clientmsgid(clientMsgId.data(), clientMsgId.size());
        secureClear(payload);
        std::string serialized;
        const bool encoded = envelope.IsInitialized()
            && envelope.SerializeToString(&serialized)
            && serialized.size() <= MAX_PROTO_FRAME_BYTES;
        secureClear(*envelope.mutable_payload());
        envelope.clear_clientmsgid();
        envelope.Clear();
        if (!encoded) {
            secureClear(serialized);
            return false;
        }

        std::string frame;
        frame.reserve(serialized.size() + 4);
        appendUint32(frame, static_cast<uint32_t>(serialized.size()));
        frame.append(serialized);
        secureClear(serialized);
        const bool sent = writeExact(frame, Clock::now() + NETWORK_TIMEOUT);
        secureClear(frame);
        return sent;
    }

    bool receiveExpected(uint32_t expectedPayloadType,
                         std::string_view expectedClientMsgId,
                         std::string& payload) noexcept
    {
        const auto deadline = Clock::now() + NETWORK_TIMEOUT;
        while (Clock::now() < deadline) {
            ProtoMessage envelope;
            if (!readEnvelope(envelope, deadline)) return false;
            const uint32_t type = envelope.payloadtype();
            if (type == static_cast<uint32_t>(HEARTBEAT_EVENT)) {
                if (envelope.has_payload()) {
                    secureClear(*envelope.mutable_payload());
                }
                envelope.Clear();
                continue;
            }
            if (type == static_cast<uint32_t>(ERROR_RES)
                || type == static_cast<uint32_t>(PROTO_OA_ERROR_RES)
                || type == static_cast<uint32_t>(PROTO_OA_CLIENT_DISCONNECT_EVENT)
                || type == static_cast<uint32_t>(PROTO_OA_ACCOUNTS_TOKEN_INVALIDATED_EVENT)
                || type != expectedPayloadType
                || !envelope.has_clientmsgid()
                || envelope.clientmsgid() != expectedClientMsgId
                || !envelope.has_payload()) {
                if (envelope.has_payload()) {
                    secureClear(*envelope.mutable_payload());
                }
                envelope.clear_clientmsgid();
                envelope.Clear();
                return false;
            }
            payload.assign(envelope.payload());
            secureClear(*envelope.mutable_payload());
            envelope.clear_clientmsgid();
            envelope.Clear();
            return true;
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
                                            bytes.size() - offset,
                                            static_cast<std::size_t>(INT_MAX))));
            if (count > 0) {
                offset += static_cast<std::size_t>(count);
                continue;
            }
            const int error = SSL_get_error(ssl_, count);
            if (error == SSL_ERROR_WANT_READ) {
                if (!waitForFd(fd_, POLLIN, deadline)) return false;
            } else if (error == SSL_ERROR_WANT_WRITE) {
                if (!waitForFd(fd_, POLLOUT, deadline)) return false;
            } else {
                return false;
            }
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
                                           size - offset,
                                           static_cast<std::size_t>(INT_MAX))));
            if (count > 0) {
                offset += static_cast<std::size_t>(count);
                continue;
            }
            const int error = SSL_get_error(ssl_, count);
            if (error == SSL_ERROR_WANT_READ) {
                if (!waitForFd(fd_, POLLIN, deadline)) return false;
            } else if (error == SSL_ERROR_WANT_WRITE) {
                if (!waitForFd(fd_, POLLOUT, deadline)) return false;
            } else {
                return false;
            }
        }
        return offset == size;
    }

    bool readEnvelope(ProtoMessage& envelope,
                      Clock::time_point deadline) noexcept
    {
        std::array<unsigned char, 4> header{};
        if (!readExact(reinterpret_cast<char*>(header.data()), header.size(),
                       deadline)) {
            return false;
        }
        const uint32_t length = (static_cast<uint32_t>(header[0]) << 24)
                              | (static_cast<uint32_t>(header[1]) << 16)
                              | (static_cast<uint32_t>(header[2]) << 8)
                              | static_cast<uint32_t>(header[3]);
        if (length == 0 || length > MAX_PROTO_FRAME_BYTES) return false;
        std::string frame(length, '\0');
        if (!readExact(frame.data(), frame.size(), deadline)) {
            secureClear(frame);
            return false;
        }
        const bool parsed = envelope.ParseFromString(frame)
            && envelope.IsInitialized();
        secureClear(frame);
        if (!parsed) {
            if (envelope.has_payload()) {
                secureClear(*envelope.mutable_payload());
            }
            envelope.clear_clientmsgid();
            envelope.Clear();
        }
        return parsed;
    }

    inline static std::atomic<uint64_t> nextGeneration_{0};
    uint64_t generation_{0};
    uint64_t sequence_{0};
    int fd_{-1};
    SSL_CTX* context_{nullptr};
    SSL* ssl_{nullptr};
    bool connected_{false};
};

void clearApplicationAuthRequest(ProtoOAApplicationAuthReq& request) noexcept
{
    secureClear(*request.mutable_clientid());
    secureClear(*request.mutable_clientsecret());
    request.Clear();
}

void clearAccountListRequest(ProtoOAGetAccountListByAccessTokenReq& request) noexcept
{
    secureClear(*request.mutable_accesstoken());
    request.Clear();
}

void clearAccountAuthRequest(ProtoOAAccountAuthReq& request) noexcept
{
    request.set_ctidtraderaccountid(0);
    secureClear(*request.mutable_accesstoken());
    request.Clear();
}

void clearAccountListResponse(ProtoOAGetAccountListByAccessTokenRes& response) noexcept
{
    secureClear(*response.mutable_accesstoken());
    for (ProtoOACtidTraderAccount& account :
         *response.mutable_ctidtraderaccount()) {
        account.set_ctidtraderaccountid(0);
        if (account.has_traderlogin()) account.set_traderlogin(0);
        if (account.has_brokertitleshort()) {
            secureClear(*account.mutable_brokertitleshort());
        }
        account.Clear();
    }
    response.Clear();
}

bool applicationAuthenticate(StrictProtobufTransport& transport,
                             std::string_view clientId,
                             std::string_view clientSecret) noexcept
{
    ProtoOAApplicationAuthReq request;
    request.set_clientid(clientId.data(), clientId.size());
    request.set_clientsecret(clientSecret.data(), clientSecret.size());
    const std::string correlation = transport.nextCorrelation("app");
    const bool sent = transport.send(PROTO_OA_APPLICATION_AUTH_REQ,
                                     request, correlation);
    clearApplicationAuthRequest(request);
    if (!sent) return false;

    std::string payload;
    if (!transport.receiveExpected(PROTO_OA_APPLICATION_AUTH_RES,
                                   correlation, payload)) {
        secureClear(payload);
        return false;
    }
    ProtoOAApplicationAuthRes response;
    const bool parsed = response.ParseFromString(payload)
        && response.IsInitialized()
        && static_cast<uint32_t>(response.payloadtype())
            == static_cast<uint32_t>(PROTO_OA_APPLICATION_AUTH_RES);
    secureClear(payload);
    response.Clear();
    return parsed;
}

std::optional<Gate6AccountListEvidence> discoverAccounts(
    StrictProtobufTransport& transport,
    std::string_view accessToken) noexcept
{
    ProtoOAGetAccountListByAccessTokenReq request;
    request.set_accesstoken(accessToken.data(), accessToken.size());
    const std::string correlation = transport.nextCorrelation("trading");
    const bool sent = transport.send(
        PROTO_OA_GET_ACCOUNTS_BY_ACCESS_TOKEN_REQ, request, correlation);
    clearAccountListRequest(request);
    if (!sent) return std::nullopt;

    std::string payload;
    if (!transport.receiveExpected(PROTO_OA_GET_ACCOUNTS_BY_ACCESS_TOKEN_RES,
                                   correlation, payload)) {
        secureClear(payload);
        return std::nullopt;
    }
    ProtoOAGetAccountListByAccessTokenRes response;
    if (!response.ParseFromString(payload) || !response.IsInitialized()
        || static_cast<uint32_t>(response.payloadtype())
            != static_cast<uint32_t>(PROTO_OA_GET_ACCOUNTS_BY_ACCESS_TOKEN_RES)) {
        secureClear(payload);
        clearAccountListResponse(response);
        return std::nullopt;
    }
    secureClear(payload);

    Gate6AccountListEvidence evidence;
    evidence.currentConnectionGeneration = transport.generation() > 0;
    evidence.correlationMatched = true;
    evidence.tokenOwned = constantTimeEqual(response.accesstoken(), accessToken);
    evidence.tradingScope = response.has_permissionscope()
        && response.permissionscope() == SCOPE_TRADE;
    evidence.accounts.reserve(
        static_cast<std::size_t>(response.ctidtraderaccount_size()));
    for (const ProtoOACtidTraderAccount& account : response.ctidtraderaccount()) {
        Gate6AccountRecord record;
        record.accountId = account.ctidtraderaccountid();
        if (account.has_islive()) record.isLive = account.islive();
        if (account.has_brokertitleshort()) {
            record.brokerTitleShort = account.brokertitleshort();
        }
        evidence.accounts.push_back(std::move(record));
    }
    clearAccountListResponse(response);
    return evidence;
}

bool accountAuthenticate(StrictProtobufTransport& transport,
                         std::string_view accessToken,
                         int64_t accountId,
                         int64_t& responseAccountId) noexcept
{
    ProtoOAAccountAuthReq request;
    request.set_ctidtraderaccountid(accountId);
    secureClear(accountId);
    request.set_accesstoken(accessToken.data(), accessToken.size());
    const std::string correlation = transport.nextCorrelation("account-auth");
    const bool sent = transport.send(PROTO_OA_ACCOUNT_AUTH_REQ,
                                     request, correlation);
    clearAccountAuthRequest(request);
    if (!sent) return false;

    std::string payload;
    if (!transport.receiveExpected(PROTO_OA_ACCOUNT_AUTH_RES,
                                   correlation, payload)) {
        secureClear(payload);
        return false;
    }
    ProtoOAAccountAuthRes response;
    const bool parsed = response.ParseFromString(payload)
        && response.IsInitialized()
        && static_cast<uint32_t>(response.payloadtype())
            == static_cast<uint32_t>(PROTO_OA_ACCOUNT_AUTH_RES);
    secureClear(payload);
    if (!parsed) {
        response.Clear();
        return false;
    }
    responseAccountId = response.ctidtraderaccountid();
    response.set_ctidtraderaccountid(0);
    response.Clear();
    return true;
}

RuntimeFailure performDiscovery(std::string_view clientId,
                                std::string_view clientSecret,
                                std::string_view accessToken,
                                Gate6AccountListEvidence& evidence) noexcept
{
    StrictProtobufTransport transport;
    if (!transport.connectDemo()) {
        return RuntimeFailure::DemoTlsConnectionFailed;
    }
    if (!applicationAuthenticate(transport, clientId, clientSecret)) {
        return RuntimeFailure::ApplicationAuthFailed;
    }
    std::optional<Gate6AccountListEvidence> discovered =
        discoverAccounts(transport, accessToken);
    transport.close();
    if (!discovered.has_value()) {
        return RuntimeFailure::AccountDiscoveryFailed;
    }
    evidence = std::move(*discovered);
    return RuntimeFailure::None;
}

int fail(RuntimeFailure failure) noexcept
{
    std::cout << safeRuntimeDiagnostic(failure) << '\n';
    return 1;
}

} // namespace

bool validateCTraderTokenResponseOffline(std::string_view response) noexcept
{
    TokenEnvelope parsed;
    return parseTokenResponse(response, parsed)
        && tokenHasExecutionLifetime(parsed);
}

bool validateCTraderTokenTransportConfigurationOffline() noexcept
{
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        return false;
    }
    CURL* curl = curl_easy_init();
    std::string response;
    curl_slist* headers = curl_slist_append(nullptr, "Accept: application/json");
    curl_slist* extendedHeaders = headers == nullptr ? nullptr
        : curl_slist_append(headers, "Cache-Control: no-store");
    if (extendedHeaders != nullptr) {
        headers = extendedHeaders;
    }
    const bool configured = extendedHeaders != nullptr && configureTokenCurl(
        curl, "https://openapi.ctrader.com/apps/token", headers, response);
    curl_slist_free_all(headers);
    if (curl != nullptr) curl_easy_cleanup(curl);
    secureClear(response);
    curl_global_cleanup();
    return configured;
}

int runCTraderGate6Proof(bool preflightOnly)
{
    std::cout << "gate6_offline_controls_verified_required" << '\n';
    if (!disableCoreDumps()) {
        return fail(RuntimeFailure::LocalHardeningFailed);
    }
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        return fail(RuntimeFailure::TokenTransportFailed);
    }

    std::optional<SensitiveString> clientId = loadClientId();
    if (!clientId.has_value()) {
        curl_global_cleanup();
        return fail(RuntimeFailure::MissingClientId);
    }
    SensitiveString clientSecret;
    const RuntimeFailure secretResult = readKeychainSecret(
        CTraderGate6Config::CLIENT_SECRET_SERVICE, clientSecret);
    if (secretResult != RuntimeFailure::None) {
        curl_global_cleanup();
        return fail(secretResult);
    }
    if (preflightOnly) {
        clientSecret.clear();
        clientId->clear();
        clientId.reset();
        curl_global_cleanup();
        std::cout << "gate6_secret_prerequisites_ready" << '\n';
        std::cout << "gate6_provider_traffic_not_started" << '\n';
        return 0;
    }

    std::cout << "gate6_oauth_authorization_starting" << '\n';
    OAuthResult oauth = receiveCorrelatedAuthorizationCode(clientId->view());
    if (oauth.failure != RuntimeFailure::None || !oauth.code.has_value()) {
        curl_global_cleanup();
        return fail(oauth.failure);
    }
    std::cout << "gate6_oauth_correlation_verified" << '\n';

    TokenEnvelope token;
    const RuntimeFailure tokenResult = exchangeAuthorizationCode(
        clientId->view(), clientSecret.view(), oauth.code->view(), token);
    oauth.code->clear();
    oauth.code.reset();
    if (tokenResult != RuntimeFailure::None) {
        curl_global_cleanup();
        return fail(tokenResult);
    }
    if (!tokenHasExecutionLifetime(token)) {
        curl_global_cleanup();
        return fail(RuntimeFailure::TokenResponseRejected);
    }
    if (persistTokenEnvelope(token) != RuntimeFailure::None) {
        curl_global_cleanup();
        return fail(RuntimeFailure::KeychainWriteFailed);
    }
    std::cout << "gate6_view_token_stored" << '\n';

    Gate6AccountListEvidence gate6aEvidence;
    RuntimeFailure runtime = performDiscovery(
        clientId->view(), clientSecret.view(), token.accessToken.view(),
        gate6aEvidence);
    if (runtime != RuntimeFailure::None) {
        curl_global_cleanup();
        return fail(runtime);
    }

    CTraderGate6AccountProof proof;
    const Gate6Decision gate6a = proof.acceptGate6A(std::move(gate6aEvidence));
    if (gate6a != Gate6Decision::AwaitingWadeCheckpoint) {
        std::cout << CTraderGate6AccountProof::safeDiagnostic(gate6a) << '\n';
        curl_global_cleanup();
        return 1;
    }

    std::cout << "GATE6A_DISCOVERY_COMPLETE" << '\n';
    for (const Gate6SafeCandidate& candidate : proof.safeCandidates()) {
        std::cout << "isLive=false" << '\n';
        std::cout << "brokerTitleShort=" << candidate.brokerTitleShort << '\n';
        std::cout << "---" << '\n';
    }
    std::cout << "WADE_CHECKPOINT_REQUIRED" << '\n';
    std::cout << "Enter CONFIRM followed by the exact brokerTitleShort, or CANCEL."
              << std::endl;

    std::string confirmation;
    if (!std::getline(std::cin, confirmation) || confirmation.size() > 256
        || confirmation == "CANCEL") {
        secureClear(confirmation);
        proof.cancel();
        curl_global_cleanup();
        return fail(RuntimeFailure::CheckpointCancelled);
    }
    constexpr std::string_view prefix = "CONFIRM ";
    if (!confirmation.starts_with(prefix)) {
        secureClear(confirmation);
        proof.cancel();
        curl_global_cleanup();
        return fail(RuntimeFailure::AccountSelectionFailed);
    }
    const std::string selectedTitle = confirmation.substr(prefix.size());
    secureClear(confirmation);
    if (proof.confirmWadeSelection(selectedTitle)
        != Gate6Decision::ReadyForGate6B) {
        curl_global_cleanup();
        return fail(RuntimeFailure::AccountSelectionFailed);
    }
    std::cout << "gate6_wade_checkpoint_confirmed" << '\n';

    if (!tokenHasExecutionLifetime(token)) {
        proof.cancel();
        curl_global_cleanup();
        return fail(RuntimeFailure::TokenResponseRejected);
    }

    StrictProtobufTransport gate6bTransport;
    if (!gate6bTransport.connectDemo()) {
        proof.cancel();
        curl_global_cleanup();
        return fail(RuntimeFailure::DemoTlsConnectionFailed);
    }
    if (!applicationAuthenticate(gate6bTransport, clientId->view(),
                                 clientSecret.view())) {
        proof.cancel();
        curl_global_cleanup();
        return fail(RuntimeFailure::ApplicationAuthFailed);
    }
    std::optional<Gate6AccountListEvidence> gate6bEvidence =
        discoverAccounts(gate6bTransport, token.accessToken.view());
    if (!gate6bEvidence.has_value()) {
        proof.cancel();
        gate6bTransport.close();
        curl_global_cleanup();
        return fail(RuntimeFailure::AccountDiscoveryFailed);
    }
    if (proof.acceptGate6B(std::move(*gate6bEvidence))
        != Gate6Decision::ReadyForAccountAuthentication) {
        gate6bTransport.close();
        curl_global_cleanup();
        return fail(RuntimeFailure::AccountSelectionFailed);
    }
    std::optional<int64_t> selectedId = proof.accountIdForAuthentication();
    if (!selectedId.has_value()) {
        proof.cancel();
        gate6bTransport.close();
        curl_global_cleanup();
        return fail(RuntimeFailure::AccountSelectionFailed);
    }
    int64_t responseId = 0;
    int64_t requestId = *selectedId;
    secureClear(*selectedId);
    selectedId.reset();
    const bool authenticated = accountAuthenticate(
        gate6bTransport, token.accessToken.view(), requestId, responseId);
    secureClear(requestId);
    gate6bTransport.close();
    if (!authenticated
        || proof.acceptAccountAuthentication(responseId)
            != Gate6Decision::AccountProofSucceeded) {
        secureClear(responseId);
        proof.cancel();
        curl_global_cleanup();
        return fail(RuntimeFailure::AccountAuthenticationFailed);
    }
    secureClear(responseId);
    curl_global_cleanup();
    std::cout << "GATE6_READ_ONLY_ACCOUNT_PROOF_SUCCEEDED" << '\n';
    std::cout << "GATE7_NOT_STARTED" << '\n';
    return 0;
}

} // namespace tradebot::ctrader
