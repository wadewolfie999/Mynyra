#include "MynyraEventSink.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace {

std::string jsonString(std::string_view value)
{
    std::string result;
    result.reserve(value.size() + 2);
    result.push_back('"');
    constexpr char hex[] = "0123456789abcdef";
    for (const unsigned char c : value) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (c < 0x20) {
                    result += "\\u00";
                    result.push_back(hex[(c >> 4) & 0x0f]);
                    result.push_back(hex[c & 0x0f]);
                } else {
                    result.push_back(static_cast<char>(c));
                }
        }
    }
    result.push_back('"');
    return result;
}

const char* signalName(Signal signal) noexcept
{
    switch (signal) {
        case Signal::NONE: return "NONE";
        case Signal::BUY: return "BUY";
        case Signal::SELL: return "SELL";
    }
    return "NONE";
}

} // namespace

const char* failureCategoryName(FailureCategory category) noexcept
{
    switch (category) {
        case FailureCategory::None: return "none";
        case FailureCategory::Validation: return "validation";
        case FailureCategory::Rejected: return "rejected";
        case FailureCategory::Transport: return "transport";
        case FailureCategory::Timeout: return "timeout";
        case FailureCategory::Authentication: return "authentication";
        case FailureCategory::RateLimited: return "rate_limited";
        case FailureCategory::StaleData: return "stale_data";
        case FailureCategory::MalformedEvent: return "malformed_event";
        case FailureCategory::ReconciliationMismatch: return "reconciliation_mismatch";
        case FailureCategory::Unknown: return "unknown";
    }
    return "unknown";
}

const char* lifecycleStateName(OrderLifecycleState state) noexcept
{
    switch (state) {
        case OrderLifecycleState::Created: return "CREATED";
        case OrderLifecycleState::RiskChecked: return "RISK_CHECKED";
        case OrderLifecycleState::Submitted: return "SUBMITTED";
        case OrderLifecycleState::Accepted: return "ORDER_ACCEPTED";
        case OrderLifecycleState::PartiallyFilled: return "ORDER_PARTIAL_FILL";
        case OrderLifecycleState::Filled: return "ORDER_FILLED";
        case OrderLifecycleState::CancelRequested: return "CANCEL_REQUESTED";
        case OrderLifecycleState::Canceled: return "CANCELED";
        case OrderLifecycleState::Expired: return "EXPIRED";
        case OrderLifecycleState::Rejected: return "REJECTED";
        case OrderLifecycleState::Timeout: return "TIMEOUT";
        case OrderLifecycleState::Unknown: return "UNKNOWN";
        case OrderLifecycleState::Reconciled: return "RECONCILED";
    }
    return "UNKNOWN";
}

std::string serializeMynyraEvent(const MynyraEvent& event)
{
    std::ostringstream output;
    output << '{'
           << "\"schema_version\":" << event.schemaVersion
           << ",\"session_id\":" << jsonString(event.sessionId)
           << ",\"local_sequence\":" << event.localSequence
           << ",\"source_timestamp_ns\":" << event.sourceTimestampNs
           << ",\"emitted_timestamp_ns\":" << event.emittedTimestampNs
           << ",\"mode\":" << jsonString(modeName(event.mode))
           << ",\"symbol\":" << jsonString(event.canonicalSymbol)
           << ",\"event_type\":" << jsonString(event.eventType)
           << ",\"strategy_action\":" << jsonString(signalName(event.strategyAction))
           << ",\"strategy_conviction\":";
    if (std::isfinite(event.strategyConviction)) {
        output << std::setprecision(17) << event.strategyConviction;
    } else {
        output << '0';
    }
    output << ",\"strategy_attribution\":"
           << jsonString(event.strategyAttribution)
           << ",\"local_order_id\":";
    if (event.localOrderId.has_value()) output << *event.localOrderId;
    else output << "null";
    output << ",\"logical_position_id\":";
    if (event.logicalPositionId.has_value()) {
        output << jsonString(*event.logicalPositionId);
    } else {
        output << "null";
    }
    output << ",\"lifecycle_state\":";
    if (event.lifecycleState.has_value()) {
        output << jsonString(lifecycleStateName(*event.lifecycleState));
    } else {
        output << "null";
    }
    output << ",\"failure_category\":"
           << jsonString(failureCategoryName(event.failure))
           << ",\"acceptance_implied_by_fill\":"
           << (event.acceptanceImpliedByFill ? "true" : "false")
           << '}';
    return output.str();
}

ConsoleEventSink::ConsoleEventSink(std::ostream& output) noexcept
    : m_output(output)
{}

bool ConsoleEventSink::emit(const MynyraEvent& event, EventFlush flush) noexcept
{
    try {
        std::lock_guard<std::mutex> lock(m_mutex);
        // The fixed event type is itself the milestone marker. No provider
        // identifiers or raw errors are accepted by MynyraEvent.
        m_output << event.eventType
                 << " session=" << event.sessionId
                 << " sequence=" << event.localSequence
                 << " failure=" << failureCategoryName(event.failure)
                 << '\n';
        if (flush == EventFlush::LifecycleBoundary) m_output.flush();
        return static_cast<bool>(m_output);
    } catch (...) {
        return false;
    }
}

NdjsonEventSink::NdjsonEventSink(const std::filesystem::path& path) noexcept
    : m_path(path)
{
    try {
        const auto parent = m_path.parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
        m_output.open(m_path, std::ios::out | std::ios::app);
    } catch (...) {
        m_output.setstate(std::ios::badbit);
    }
}

bool NdjsonEventSink::emit(const MynyraEvent& event, EventFlush flush) noexcept
{
    try {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_output.is_open()) return false;
        m_output << serializeMynyraEvent(event) << '\n';
        if (flush == EventFlush::LifecycleBoundary) m_output.flush();
        return static_cast<bool>(m_output);
    } catch (...) {
        return false;
    }
}

bool NdjsonEventSink::isOpen() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_output.is_open() && static_cast<bool>(m_output);
}

void CompositeEventSink::add(std::shared_ptr<IEventSink> sink)
{
    if (sink) m_sinks.push_back(std::move(sink));
}

bool CompositeEventSink::emit(const MynyraEvent& event,
                              EventFlush flush) noexcept
{
    bool success = !m_sinks.empty();
    for (const auto& sink : m_sinks) {
        success = sink->emit(event, flush) && success;
    }
    return success;
}
