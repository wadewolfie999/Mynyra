#pragma once

#include "IStrategy.hpp"
#include "MarketCandle.hpp"
#include "PortfolioAllocator.hpp"
#include "RegimeDetector.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct StrategyDecision {
    std::uint32_t schemaVersion{1};
    std::string canonicalSymbol;
    Signal action{Signal::NONE};
    double totalConviction{0.0};
    double referencePrice{0.0};
    std::string strategyAttribution;
    MarketRegime regime{MarketRegime::CHAOS};
    std::uint64_t candleTimestamp{0};
    std::uint64_t pipelineSequence{0};
    bool executionEligible{false};
    std::vector<AlphaSignal> componentSignals;
};

// Side-effect-free with respect to execution: advancing strategy/regime state
// returns a decision but cannot submit an order.
class StrategyPipeline {
public:
    StrategyPipeline(std::vector<IStrategy*> strategies,
                     PortfolioAllocator& allocator,
                     RegimeDetector* regimeDetector = nullptr);

    StrategyDecision advance(const MarketCandle& candle,
                             bool executionEligible);
    std::uint64_t sequence() const noexcept { return m_sequence; }

private:
    std::vector<IStrategy*> m_strategies;
    PortfolioAllocator& m_allocator;
    RegimeDetector* m_regimeDetector{nullptr};
    std::uint64_t m_sequence{0};
};
