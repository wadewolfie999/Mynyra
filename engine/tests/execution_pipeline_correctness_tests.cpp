#include "BrokerGateway.hpp"
#include "PortfolioManager.hpp"
#include "RiskEngine.hpp"
#include "SystemConfig.hpp"

#define private public
#include "ExecutionEngine.hpp"
#undef private

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Fixture {
    SystemConfig config;
    PortfolioManager portfolio;
    RiskEngine risk;
    BrokerGateway gateway;
    ExecutionEngine engine;
    std::vector<ExecutionEvent> executions;

    static SystemConfig makePaperConfig()
    {
        SystemConfig config;
        config.mode = SystemMode::PAPER;
        return config;
    }

    Fixture()
        : config(makePaperConfig())
        , risk(portfolio, 2)
        , gateway(config, portfolio)
        , engine(portfolio, risk, "BTCUSDT", 0.001, 5.0, 0.01)
    {
        risk.setSystemConfig(&config);
        gateway.addExecutionCallback(
            [this](const ExecutionEvent& event) { executions.push_back(event); });
        gateway.connect();
        engine.bindBrokerGateway(&gateway);
    }
};

void testRiskDecisionIsAuthoritative()
{
    Fixture fixture;
    require(fixture.risk.evaluateOrder(OrderSide::Buy).allowed,
            "RiskEngine should allow the initial buy");
    fixture.risk.setTotalDrawdown(0.10);
    const auto denied = fixture.risk.evaluateOrder(OrderSide::Buy);
    require(!denied.allowed, "RiskEngine should reject new exposure at the limit");
    require(fixture.risk.evaluateOrder(OrderSide::Sell).allowed,
            "RiskEngine should continue to allow position reduction");
    require(!fixture.engine.execute(Signal::BUY, 100.0, 1, "RISK"),
            "ExecutionEngine bypassed the rejected RiskEngine decision");
}

void testDuplicateExecutionDoesNotMutateState()
{
    Fixture fixture;
    require(fixture.engine.execute(Signal::BUY, 100.0, 1, "DUP"),
            "buy submission failed");
    require(fixture.executions.size() == 1, "buy execution was not observed");
    const auto before = fixture.portfolio.snapshotState();
    fixture.engine.onBrokerExecution(fixture.executions.back());
    const auto after = fixture.portfolio.snapshotState();
    require(after.cash == before.cash && after.tradeLog.size() == before.tradeLog.size()
                && std::fabs(after.totalEquity - before.totalEquity) < 1e-9,
            "duplicate execution mutated portfolio, cash, or trade log");
}

void testStaleExecutionDoesNotAdvancePartialContext()
{
    Fixture fixture;
    fixture.gateway.injectNextPartialFill(0.5);
    require(fixture.engine.execute(Signal::BUY, 100.0, 1, "STALE"),
            "partial buy submission failed");
    require(fixture.executions.size() == 1, "partial execution was not observed");
    const auto first = fixture.executions.back();
    const auto before = fixture.portfolio.snapshotState();
    const auto orderId = fixture.engine.lastBrokerOrderId();
    const double pendingBefore = fixture.engine.pendingBrokerQuantity(orderId);

    auto stale = first;
    stale.sequence = first.sequence - 1;
    stale.eventKey = "stale-execution";
    fixture.engine.onBrokerExecution(stale);

    const auto after = fixture.portfolio.snapshotState();
    require(after.cash == before.cash && after.tradeLog.size() == before.tradeLog.size()
                && std::fabs(after.totalEquity - before.totalEquity) < 1e-9,
            "stale execution mutated portfolio state");
    require(fixture.engine.pendingBrokerQuantity(orderId) == pendingBefore,
            "stale execution advanced partial-fill context");
}

void testRejectedAcknowledgementCleansContext()
{
    Fixture fixture;
    fixture.gateway.injectNextOrderError("rejected by deterministic adapter");
    require(fixture.engine.execute(Signal::BUY, 100.0, 1, "REJECT"),
            "rejected order was not submitted to gateway");
    require(fixture.engine.m_gatewayOrderContexts.empty(),
            "rejected acknowledgement retained execution context");
    require(!fixture.portfolio.hasPosition("BTCUSDT"),
            "rejected acknowledgement mutated the portfolio");
}

void testPartialAndFinalSellLifecycle()
{
    Fixture fixture;
    require(fixture.engine.execute(Signal::BUY, 100.0, 1, "SELL"),
            "entry buy failed");
    const double held = fixture.portfolio.getPositionQuantity("BTCUSDT");
    fixture.gateway.injectNextPartialFill(0.5);
    require(fixture.engine.execute(Signal::SELL, 101.0, 2, "SELL"),
            "partial sell submission failed");
    const auto sellPartial = fixture.executions.back();
    const double afterPartial = fixture.portfolio.getPositionQuantity("BTCUSDT");
    require(afterPartial > 0.0 && afterPartial < held,
            "partial sell did not reduce the position");

    BrokerFill finalFill;
    finalFill.orderId = fixture.engine.lastBrokerOrderId();
    finalFill.symbol = "BTCUSDT";
    finalFill.isBuy = false;
    finalFill.requestedPrice = 101.0;
    finalFill.fillPrice = 100.95;
    finalFill.quantity = sellPartial.remainingQuantity.toDouble();
    finalFill.filledQuantity = finalFill.quantity;
    finalFill.remainingQuantity = 0.0;
    finalFill.feePaid = finalFill.fillPrice * finalFill.filledQuantity * 0.001;
    finalFill.fillTimestamp = 3;
    finalFill.success = true;
    fixture.gateway.simulateFillConfirmation(finalFill);
    require(!fixture.portfolio.hasPosition("BTCUSDT"),
            "final sell did not close the remaining position");
}

void testOverCloseDoesNotMutateOrAdvanceContext()
{
    Fixture fixture;
    require(fixture.engine.execute(Signal::BUY, 100.0, 1, "OVER"),
            "entry buy failed");
    fixture.gateway.injectNextPartialFill(0.5);
    require(fixture.engine.execute(Signal::SELL, 101.0, 2, "OVER"),
            "partial sell submission failed");
    const auto partial = fixture.executions.back();
    const auto orderId = fixture.engine.lastBrokerOrderId();
    const auto before = fixture.portfolio.snapshotState();
    const double pendingBefore = fixture.engine.pendingBrokerQuantity(orderId);

    // Simulate an external reduction before the next execution event. The
    // remaining broker delta is now an over-close for local portfolio state.
    fixture.portfolio.closePosition("BTCUSDT", 101.0, 3, 0.0, "EXTERNAL");
    const auto beforeRejectedEvent = fixture.portfolio.snapshotState();
    auto overClose = partial;
    overClose.cumulativeFilledQuantity = Decimal64{
        partial.cumulativeFilledQuantity.units
            + partial.remainingQuantity.units,
        partial.cumulativeFilledQuantity.scale};
    overClose.remainingQuantity = Decimal64{0, partial.remainingQuantity.scale};
    overClose.sequence = partial.sequence + 1;
    overClose.timestampNs = partial.timestampNs + 1;
    overClose.eventKey = "over-close-execution";
    fixture.engine.onBrokerExecution(overClose);

    const auto after = fixture.portfolio.snapshotState();
    require(after.cash == beforeRejectedEvent.cash
                && after.tradeLog.size() == beforeRejectedEvent.tradeLog.size()
                && std::fabs(after.totalEquity - beforeRejectedEvent.totalEquity) < 1e-9,
            "over-close execution mutated portfolio state");
    require(fixture.engine.pendingBrokerQuantity(orderId) == pendingBefore,
            "over-close execution advanced or corrupted execution context");
    require(before.tradeLog.size() < after.tradeLog.size(),
            "test setup did not perform the external reduction");
}

} // namespace

int main()
{
    try {
        testRiskDecisionIsAuthoritative();
        testDuplicateExecutionDoesNotMutateState();
        testStaleExecutionDoesNotAdvancePartialContext();
        testRejectedAcknowledgementCleansContext();
        testPartialAndFinalSellLifecycle();
        testOverCloseDoesNotMutateOrAdvanceContext();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "execution pipeline correctness tests passed\n";
    return EXIT_SUCCESS;
}
