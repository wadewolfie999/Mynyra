#pragma once

#include "BrokerAdapterContracts.hpp"
#include "IStrategy.hpp"
#include "SystemConfig.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

enum class EventFlush : std::uint8_t { Buffered, LifecycleBoundary };

struct MynyraEvent {
    std::uint32_t schemaVersion{1};
    std::string sessionId;
    std::uint64_t localSequence{0};
    std::uint64_t sourceTimestampNs{0};
    std::uint64_t emittedTimestampNs{0};
    SystemMode mode{SystemMode::DEMO};
    std::string canonicalSymbol;
    std::string eventType;
    Signal strategyAction{Signal::NONE};
    double strategyConviction{0.0};
    std::string strategyAttribution;
    std::optional<std::uint64_t> localOrderId;
    std::optional<std::string> logicalPositionId;
    std::optional<OrderLifecycleState> lifecycleState;
    FailureCategory failure{FailureCategory::None};
    bool acceptanceImpliedByFill{false};
};

class IEventSink {
public:
    virtual ~IEventSink() = default;
    virtual bool emit(const MynyraEvent& event, EventFlush flush) noexcept = 0;
};

class ConsoleEventSink final : public IEventSink {
public:
    explicit ConsoleEventSink(std::ostream& output) noexcept;
    bool emit(const MynyraEvent& event, EventFlush flush) noexcept override;

private:
    std::ostream& m_output;
    std::mutex m_mutex;
};

class NdjsonEventSink final : public IEventSink {
public:
    explicit NdjsonEventSink(const std::filesystem::path& path) noexcept;
    bool emit(const MynyraEvent& event, EventFlush flush) noexcept override;
    bool isOpen() const noexcept;
    const std::filesystem::path& path() const noexcept { return m_path; }

private:
    std::filesystem::path m_path;
    mutable std::mutex m_mutex;
    std::ofstream m_output;
};

class CompositeEventSink final : public IEventSink {
public:
    void add(std::shared_ptr<IEventSink> sink);
    bool emit(const MynyraEvent& event, EventFlush flush) noexcept override;

private:
    std::vector<std::shared_ptr<IEventSink>> m_sinks;
};

std::string serializeMynyraEvent(const MynyraEvent& event);
const char* failureCategoryName(FailureCategory category) noexcept;
const char* lifecycleStateName(OrderLifecycleState state) noexcept;
