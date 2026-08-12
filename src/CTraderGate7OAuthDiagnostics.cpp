#include "CTraderGate7OAuthDiagnostics.hpp"

#include <algorithm>

namespace tradebot::ctrader {

Gate7OAuthClock::time_point gate7OAuthCallbackReadDeadline(
    Gate7OAuthClock::time_point correlationDeadline,
    Gate7OAuthClock::time_point now) noexcept
{
    if (now >= correlationDeadline) return correlationDeadline;
    const auto timeout = std::chrono::duration_cast<Gate7OAuthClock::duration>(
        GATE7_OAUTH_CALLBACK_READ_TIMEOUT);
    if (now.time_since_epoch() > Gate7OAuthClock::duration::max() - timeout) {
        return correlationDeadline;
    }
    return std::min(correlationDeadline, now + timeout);
}

Gate7OAuthFailure appendGate7OAuthCallbackBytes(
    std::string& request,
    std::string_view bytes,
    std::size_t maximumBytes) noexcept
{
    if (request.size() > maximumBytes
        || bytes.size() > maximumBytes - request.size()) {
        return Gate7OAuthFailure::CallbackMalformed;
    }
    try {
        request.append(bytes.data(), bytes.size());
    } catch (...) {
        return Gate7OAuthFailure::ResourceExhausted;
    }
    return Gate7OAuthFailure::None;
}

Gate7OAuthFailure classifyOAuthCorrelationFailure(
    CTraderOAuthCorrelationGuard::Decision decision) noexcept
{
    using Decision = CTraderOAuthCorrelationGuard::Decision;
    switch (decision) {
    case Decision::Armed:
        return Gate7OAuthFailure::None;
    case Decision::Unarmed:
    case Decision::ListenerBindingRejected:
    case Decision::EntropyUnavailable:
        return Gate7OAuthFailure::CorrelationArmFailed;
    case Decision::AlreadyTerminal:
        return Gate7OAuthFailure::CallbackReplayRejected;
    case Decision::CallbackBeforeArming:
        return Gate7OAuthFailure::CallbackBeforeArming;
    case Decision::CallbackExpired:
        return Gate7OAuthFailure::CallbackTimeout;
    case Decision::Cancelled:
        return Gate7OAuthFailure::CallbackCancelled;
    case Decision::UnexpectedRemote:
        return Gate7OAuthFailure::CallbackRemoteRejected;
    case Decision::UnexpectedMethod:
        return Gate7OAuthFailure::CallbackMethodRejected;
    case Decision::UnexpectedHost:
        return Gate7OAuthFailure::CallbackHostRejected;
    case Decision::UnexpectedPath:
        return Gate7OAuthFailure::CallbackPathRejected;
    case Decision::MalformedQuery:
    case Decision::DuplicateParameter:
        return Gate7OAuthFailure::CallbackQueryMalformed;
    case Decision::AuthorizationRejected:
        return Gate7OAuthFailure::AuthorizationDenied;
    case Decision::StateMissing:
        return Gate7OAuthFailure::StateMissing;
    case Decision::StateMismatch:
        return Gate7OAuthFailure::StateMismatch;
    case Decision::CodeMissing:
        return Gate7OAuthFailure::CodeMissing;
    case Decision::CorrelationMatchedCodeDiscarded:
        return Gate7OAuthFailure::None;
    }
    return Gate7OAuthFailure::ResourceExhausted;
}

std::string_view safeOAuthDiagnostic(Gate7OAuthFailure failure) noexcept
{
    switch (failure) {
    case Gate7OAuthFailure::None: return "gate7_oauth_ok";
    case Gate7OAuthFailure::ListenerSocketFailed:
        return "gate7_oauth_listener_socket_failed";
    case Gate7OAuthFailure::ListenerBindFailed:
        return "gate7_oauth_listener_bind_failed";
    case Gate7OAuthFailure::ListenerListenFailed:
        return "gate7_oauth_listener_listen_failed";
    case Gate7OAuthFailure::ListenerNonBlockingFailed:
        return "gate7_oauth_listener_nonblocking_failed";
    case Gate7OAuthFailure::CorrelationArmFailed:
        return "gate7_oauth_correlation_arm_failed";
    case Gate7OAuthFailure::AuthorizationUrlFailed:
        return "gate7_oauth_authorization_url_failed";
    case Gate7OAuthFailure::BrowserLaunchFailed:
        return "gate7_oauth_browser_launch_failed";
    case Gate7OAuthFailure::CallbackTimeout:
        return "gate7_oauth_callback_timeout";
    case Gate7OAuthFailure::CallbackWaitFailed:
        return "gate7_oauth_callback_wait_failed";
    case Gate7OAuthFailure::CallbackAcceptFailed:
        return "gate7_oauth_callback_accept_failed";
    case Gate7OAuthFailure::CallbackReadFailed:
        return "gate7_oauth_callback_read_failed";
    case Gate7OAuthFailure::CallbackMalformed:
        return "gate7_oauth_callback_malformed";
    case Gate7OAuthFailure::CallbackRemoteRejected:
        return "gate7_oauth_callback_remote_rejected";
    case Gate7OAuthFailure::CallbackMethodRejected:
        return "gate7_oauth_callback_method_rejected";
    case Gate7OAuthFailure::CallbackHostRejected:
        return "gate7_oauth_callback_host_rejected";
    case Gate7OAuthFailure::CallbackPathRejected:
        return "gate7_oauth_callback_path_rejected";
    case Gate7OAuthFailure::CallbackQueryMalformed:
        return "gate7_oauth_callback_query_malformed";
    case Gate7OAuthFailure::AuthorizationDenied:
        return "gate7_oauth_authorization_denied";
    case Gate7OAuthFailure::CallbackBeforeArming:
        return "gate7_oauth_callback_before_arming";
    case Gate7OAuthFailure::StateMissing:
        return "gate7_oauth_state_missing";
    case Gate7OAuthFailure::StateMismatch:
        return "gate7_oauth_state_mismatch";
    case Gate7OAuthFailure::CodeMissing:
        return "gate7_oauth_code_missing";
    case Gate7OAuthFailure::CodeExtractionFailed:
        return "gate7_oauth_code_extraction_failed";
    case Gate7OAuthFailure::CallbackReplayRejected:
        return "gate7_oauth_callback_replay_rejected";
    case Gate7OAuthFailure::CallbackCancelled:
        return "gate7_oauth_callback_cancelled";
    case Gate7OAuthFailure::ResourceExhausted:
        return "gate7_oauth_resource_exhausted";
    }
    return "gate7_oauth_failure_unknown";
}

} // namespace tradebot::ctrader
