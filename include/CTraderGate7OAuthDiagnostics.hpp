#pragma once

#include "CTraderOAuthCorrelation.hpp"

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>

namespace tradebot::ctrader {

// Fixed, non-sensitive categories for the isolated Gate 7 OAuth boundary.
// These values must never contain provider text, query material, identifiers,
// or credential-derived data.
enum class Gate7OAuthFailure {
    None,
    ListenerSocketFailed,
    ListenerBindFailed,
    ListenerListenFailed,
    ListenerNonBlockingFailed,
    CorrelationArmFailed,
    AuthorizationUrlFailed,
    BrowserLaunchFailed,
    CallbackTimeout,
    CallbackWaitFailed,
    CallbackAcceptFailed,
    CallbackReadFailed,
    CallbackMalformed,
    CallbackRemoteRejected,
    CallbackMethodRejected,
    CallbackHostRejected,
    CallbackPathRejected,
    CallbackQueryMalformed,
    AuthorizationDenied,
    CallbackBeforeArming,
    StateMissing,
    StateMismatch,
    CodeMissing,
    CodeExtractionFailed,
    CallbackReplayRejected,
    CallbackCancelled,
    ResourceExhausted
};

using Gate7OAuthClock = std::chrono::steady_clock;
constexpr std::chrono::seconds GATE7_OAUTH_CALLBACK_READ_TIMEOUT{2};

Gate7OAuthClock::time_point gate7OAuthCallbackReadDeadline(
    Gate7OAuthClock::time_point correlationDeadline,
    Gate7OAuthClock::time_point now) noexcept;

Gate7OAuthFailure appendGate7OAuthCallbackBytes(
    std::string& request,
    std::string_view bytes,
    std::size_t maximumBytes) noexcept;

Gate7OAuthFailure classifyOAuthCorrelationFailure(
    CTraderOAuthCorrelationGuard::Decision decision) noexcept;

std::string_view safeOAuthDiagnostic(Gate7OAuthFailure failure) noexcept;

} // namespace tradebot::ctrader
