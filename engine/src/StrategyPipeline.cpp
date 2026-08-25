#include "StrategyPipeline.hpp"

#include <stdexcept>
#include <utility>

StrategyPipeline::StrategyPipeline(std::vector<IStrategy*> strategies,
                                   PortfolioAllocator& allocator,
                                   RegimeDetector* regimeDetector)
    : m_strategies(std::move(strategies))
    , m_allocator(allocator)
    , m_regimeDetector(regimeDetector)
{
    if (m_strategies.empty()) {
        throw std::invalid_argument("StrategyPipeline requires strategies");
    }
    for (const IStrategy* strategy : m_strategies) {
        if (strategy == nullptr) {
            throw std::invalid_argument("StrategyPipeline received null strategy");
        }
    }
}

StrategyDecision StrategyPipeline::advance(const MarketCandle& candle,
                                            bool executionEligible)
{
    StrategyDecision decision;
    decision.canonicalSymbol = candle.symbol;
    decision.referencePrice = candle.close;
    decision.candleTimestamp = candle.epochTimestamp;
    decision.pipelineSequence = ++m_sequence;
    decision.executionEligible = executionEligible;

    if (m_regimeDetector != nullptr) {
        decision.regime = m_regimeDetector->update(
            candle.high, candle.low, candle.close);
    }

    decision.componentSignals.reserve(m_strategies.size());
    for (IStrategy* strategy : m_strategies) {
        decision.componentSignals.push_back(strategy->generateSignal(candle));
    }

    const AllocationResult allocation = m_allocator.ensemble(
        decision.componentSignals);
    decision.action = allocation.action;
    decision.totalConviction = allocation.totalConviction;
    decision.regime = allocation.regime;
    decision.strategyAttribution = allocation.dominantStrategyId.empty()
        ? "ENSEMBLE" : allocation.dominantStrategyId;
    return decision;
}
