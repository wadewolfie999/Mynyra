#include "BrokerGateway.hpp"
#include "EventLoop.hpp"
#include "ExecutionEngine.hpp"
#include "IStrategy.hpp"
#include "PortfolioAllocator.hpp"
#include "PortfolioManager.hpp"
#include "RiskEngine.hpp"
#include "SystemConfig.hpp"

#include <stdexcept>
#include <vector>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

InstrumentSpec traceInstrument()
{
    InstrumentSpec spec;
    spec.version = 1;
    spec.canonicalSymbol = "TRACE";
    spec.executionAlias = "TRACE";
    spec.tickSize = Decimal64{1, 8};
    spec.contractSize = Decimal64{1, 0};
    spec.minimumQuantity = Decimal64{1, 8};
    spec.maximumQuantity = Decimal64{1'000'000'000'000LL, 8};
    spec.quantityStep = Decimal64{1, 8};
    spec.complete = true;
    return spec;
}

MarketCandle candle(std::uint64_t timestamp, double close)
{
    MarketCandle c;
    c.symbol = "TRACE";
    c.epochTimestamp = timestamp;
    c.open = close;
    c.high = close + 0.1;
    c.low = close - 0.1;
    c.close = close;
    c.volume = 1.0;
    return c;
}

class BuyThenSellStrategy final : public IStrategy {
public:
    AlphaSignal generateSignal(const MarketCandle& c) override
    {
        const double conviction = m_calls++ == 0 ? 1.0 : -1.0;
        return AlphaSignal{c.symbol, "TRACE_STRATEGY", conviction};
    }
private:
    int m_calls{0};
};

class BuyStrategy final : public IStrategy {
public:
    AlphaSignal generateSignal(const MarketCandle& c) override
    {
        return AlphaSignal{c.symbol, "TRACE_STRATEGY", 1.0};
    }
};

void test_strategy_to_filled_lifecycle_and_risk_reducing_close()
{
    SystemConfig config;
    config.mode = SystemMode::PAPER;

    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 1, 0.0);
    risk.setSystemConfig(&config);
    BrokerGateway gateway(config, portfolio);
    gateway.setInstrumentSpec(traceInstrument());
    gateway.connect();
    require(gateway.isConnected(), "canonical PAPER gateway did not connect");

    ExecutionEngine execution(portfolio, risk, "TRACE", 0.0, 0.0, 0.01);
    execution.bindBrokerGateway(&gateway);

    BuyThenSellStrategy strategy;
    PortfolioAllocator allocator(0.3, 0.3);
    allocator.setWeight("TRACE_STRATEGY", 1.0);
    std::vector<IStrategy*> strategies{&strategy};
    EventLoop loop(std::move(strategies), allocator, risk, execution, portfolio);

    loop.processCandle(candle(1, 100.0));
    require(portfolio.hasPosition("TRACE"),
            "canonical BUY did not reach PortfolioManager through execution event");
    require(gateway.totalOrdersSubmitted() == 1,
            "canonical BUY did not dispatch exactly once");
    const auto buyId = execution.lastBrokerOrderId();
    const auto buyLifecycle = gateway.orderLifecycle(buyId);
    require(buyLifecycle.has_value(), "canonical BUY lifecycle is missing");
    require(buyLifecycle->state == OrderLifecycleState::Filled,
            "canonical BUY lifecycle did not reach Filled");
    require(buyLifecycle->request.sourceId == "TRACE_STRATEGY",
            "strategy attribution was lost before lifecycle ownership");

    risk.setHaltTrading(true);
    loop.processCandle(candle(2, 101.0));
    require(!portfolio.hasPosition("TRACE"),
            "risk-reducing SELL was blocked by entry-risk halt");
    require(gateway.totalOrdersSubmitted() == 2,
            "canonical SELL did not dispatch exactly once");
    const auto sellId = execution.lastBrokerOrderId();
    require(sellId != buyId, "BUY and SELL reused lifecycle identity");
    const auto sellLifecycle = gateway.orderLifecycle(sellId);
    require(sellLifecycle.has_value()
                && sellLifecycle->state == OrderLifecycleState::Filled,
            "canonical SELL lifecycle did not reach Filled");
}

void test_final_risk_rejection_never_enters_lifecycle()
{
    SystemConfig config;
    config.mode = SystemMode::PAPER;

    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 1, 0.0);
    risk.setSystemConfig(&config);
    risk.setHaltTrading(true);

    BrokerGateway gateway(config, portfolio);
    gateway.setInstrumentSpec(traceInstrument());
    gateway.connect();

    ExecutionEngine execution(portfolio, risk, "TRACE", 0.0, 0.0, 0.01);
    execution.bindBrokerGateway(&gateway);

    BuyStrategy strategy;
    PortfolioAllocator allocator(0.3, 0.3);
    allocator.setWeight("TRACE_STRATEGY", 1.0);
    std::vector<IStrategy*> strategies{&strategy};
    EventLoop loop(std::move(strategies), allocator, risk, execution, portfolio);

    loop.processCandle(candle(1, 100.0));
    require(!portfolio.hasPosition("TRACE"),
            "risk-rejected BUY mutated portfolio state");
    require(gateway.totalOrdersSubmitted() == 0,
            "risk-rejected BUY reached the adapter");
    require(gateway.totalFillsReceived() == 0,
            "risk-rejected BUY produced an execution event");
    require(execution.getBlockedCount() == 1,
            "final risk rejection was not owned by ExecutionEngine");
}

} // namespace

int main()
{
    test_strategy_to_filled_lifecycle_and_risk_reducing_close();
    test_final_risk_rejection_never_enters_lifecycle();
    return 0;
}
