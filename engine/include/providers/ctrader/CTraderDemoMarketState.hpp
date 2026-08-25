#pragma once

#include "BrokerAdapterContracts.hpp"
#include "MarketCandle.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace tradebot::ctrader {

struct CTraderDemoBboSnapshot {
    Decimal64 bid;
    Decimal64 ask;
    std::uint64_t sourceTimestampNs{0};
};

struct CTraderDemoSpotUpdate {
    std::optional<Decimal64> bid;
    std::optional<Decimal64> ask;
    std::optional<std::uint64_t> sourceTimestampNs;
    std::vector<MarketCandle> liveM1Bars;
};

class CTraderDemoMarketState final {
public:
    enum class ApplyResult : std::uint8_t {
        Applied,
        Malformed,
        QueueOverflow
    };

    explicit CTraderDemoMarketState(std::size_t completedCapacity = 16) noexcept
        : m_completedCapacity(completedCapacity)
    {}

    ApplyResult apply(const CTraderDemoSpotUpdate& update)
    {
        if (update.bid.has_value()) {
            if (!update.bid->isPositive()) return ApplyResult::Malformed;
            m_bid = update.bid;
            m_bidTimestampNs = update.sourceTimestampNs.value_or(0);
        }
        if (update.ask.has_value()) {
            if (!update.ask->isPositive()) return ApplyResult::Malformed;
            m_ask = update.ask;
            m_askTimestampNs = update.sourceTimestampNs.value_or(0);
        }
        if (m_bid.has_value() && m_ask.has_value()
            && m_bid->scale == m_ask->scale
            && m_ask->units <= m_bid->units) {
            return ApplyResult::Malformed;
        }

        for (const auto& candle : update.liveM1Bars) {
            if (!validCandle(candle)) return ApplyResult::Malformed;
            if (!m_currentLiveBar.has_value()) {
                m_currentLiveBar = candle;
            } else if (candle.epochTimestamp
                       == m_currentLiveBar->epochTimestamp) {
                m_currentLiveBar = candle;
            } else if (candle.epochTimestamp
                       > m_currentLiveBar->epochTimestamp) {
                if (m_completedCandles.size() >= m_completedCapacity) {
                    return ApplyResult::QueueOverflow;
                }
                m_completedCandles.push_back(*m_currentLiveBar);
                m_currentLiveBar = candle;
            } else {
                return ApplyResult::Malformed;
            }
        }
        return ApplyResult::Applied;
    }

    std::optional<CTraderDemoBboSnapshot> bbo() const noexcept
    {
        if (!m_bid.has_value() || !m_ask.has_value()
            || m_bidTimestampNs == 0 || m_askTimestampNs == 0
            || m_bid->scale != m_ask->scale
            || m_ask->units <= m_bid->units) {
            return std::nullopt;
        }
        return CTraderDemoBboSnapshot{
            *m_bid, *m_ask, std::min(m_bidTimestampNs, m_askTimestampNs)};
    }

    bool hasCompletedCandle() const noexcept
    {
        return !m_completedCandles.empty();
    }

    std::optional<MarketCandle> popCompletedCandle()
    {
        if (m_completedCandles.empty()) return std::nullopt;
        MarketCandle candle = m_completedCandles.front();
        m_completedCandles.pop_front();
        return candle;
    }

    void resetLiveGeneration() noexcept
    {
        m_bid.reset();
        m_ask.reset();
        m_bidTimestampNs = 0;
        m_askTimestampNs = 0;
        m_currentLiveBar.reset();
        m_completedCandles.clear();
    }

    static std::optional<std::vector<MarketCandle>> normalizeHistory(
        std::vector<MarketCandle> input,
        std::size_t expectedCount,
        std::uint64_t completedBeforeEpoch)
    {
        std::map<std::uint64_t, MarketCandle> ordered;
        for (auto& candle : input) {
            if (!validCandle(candle)
                || candle.epochTimestamp >= completedBeforeEpoch
                || !ordered.emplace(candle.epochTimestamp,
                                    std::move(candle)).second) {
                return std::nullopt;
            }
        }
        if (ordered.size() != expectedCount) return std::nullopt;
        std::vector<MarketCandle> normalized;
        normalized.reserve(expectedCount);
        for (auto& [_, candle] : ordered) {
            normalized.push_back(std::move(candle));
        }
        return normalized;
    }

private:
    static bool validCandle(const MarketCandle& candle) noexcept
    {
        return candle.epochTimestamp > 0 && !candle.symbol.empty()
            && candle.low > 0.0 && candle.low <= candle.open
            && candle.open <= candle.high && candle.low <= candle.close
            && candle.close <= candle.high && candle.volume >= 0.0;
    }

    std::size_t m_completedCapacity{16};
    std::optional<Decimal64> m_bid;
    std::optional<Decimal64> m_ask;
    std::uint64_t m_bidTimestampNs{0};
    std::uint64_t m_askTimestampNs{0};
    std::optional<MarketCandle> m_currentLiveBar;
    std::deque<MarketCandle> m_completedCandles;
};

} // namespace tradebot::ctrader
