#include "BrokerGateway.hpp"
#include "FinancialMath.hpp"
#include "PortfolioManager.hpp"
#include "SystemConfig.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void requireNear(double actual, double expected, const char* message)
{
    if (std::fabs(actual - expected) > 1e-8) {
        throw std::runtime_error(
            std::string(message) + ": expected=" + std::to_string(expected)
            + " actual=" + std::to_string(actual));
    }
}

void testGoldenFixedPointVectors()
{
    using namespace Financial;

    require(price(1.234567899)->units == 123'456'790,
            "price rounding vector changed");
    require(quantity(1.234567899)->units == 123'456'789,
            "quantity toward-zero vector changed");
    require(!money(1.000000001, Rounding::RejectUnaligned).has_value(),
            "unaligned money was accepted");
    require(!price(std::numeric_limits<double>::infinity()).has_value(),
            "non-finite price was accepted");

    const Price p{123'456'789};
    const Quantity q{200'000'000};
    const auto n = notional(p, q);
    require(n.has_value() && n->units == 246'913'578,
            "price-times-quantity vector changed");
    const auto chargedFee = fee(*n, Fraction{100'000});
    require(chargedFee.has_value() && chargedFee->units == 246'914,
            "fee rounding vector changed");
    require(notional(Price{1}, Quantity{50'000'000})->units == 1,
            "half-unit notional did not round away from zero");
    require(notional(Price{1}, Quantity{50'000'000}, Rounding::TowardZero)->units == 0,
            "toward-zero notional vector changed");
    require(!notional(Price{1}, Quantity{50'000'000},
                      Rounding::RejectUnaligned).has_value(),
            "fractional notional was accepted as aligned");
    require(quantityForNotional(Money{100'000'000}, Price{300'000'000})->units
                == 33'333'333,
            "quantity division vector changed");
    require(averagePrice(Money{100'000'000}, Quantity{300'000'000})->units
                == 33'333'333,
            "average-price division vector changed");
    require(proportional(Money{100'000'001}, Quantity{100'000'000},
                         Quantity{300'000'000})->units == 33'333'334,
            "proportional allocation vector changed");
    require(applySlippage(Price{10'000'000'000}, Fraction{50'000}, true)->units
                == 10'005'000'000,
            "buy slippage vector changed");
    require(applySlippage(Price{10'000'000'000}, Fraction{50'000}, false)->units
                == 9'995'000'000,
            "sell slippage vector changed");
    require(!notional(Price{std::numeric_limits<std::int64_t>::max()},
                      Quantity{200'000'000}).has_value(),
            "notional overflow did not fail closed");
    require(!add(Money{std::numeric_limits<std::int64_t>::max()}, Money{1}).has_value(),
            "money addition overflow did not fail closed");
}

void testBuyAddReduceCloseAccounting()
{
    PortfolioManager portfolio;
    portfolio.openLong("X", 100.0, 10, 1.0, 1'001.0, 10.0, "S1");
    requireNear(portfolio.getCashBalance(), 98'999.0, "entry cash mismatch");
    requireNear(portfolio.getTotalEquity(), 99'999.0, "entry equity mismatch");

    portfolio.addToLong("X", 120.0, 5.0, 0.6, 600.6, "S1");
    requireNear(portfolio.getCashBalance(), 98'398.4, "add-on cash mismatch");
    requireNear(portfolio.getPositionQuantity("X"), 15.0, "add-on quantity mismatch");
    requireNear(portfolio.getEntryPrice("X"), 106.66666667,
                "weighted average entry mismatch");
    auto state = portfolio.snapshotState();
    requireNear(state.unrealizedPnL, 200.0, "add-on unrealized P&L mismatch");
    requireNear(state.totalFeesPaid, 1.6, "add-on entry fees were not accumulated");

    portfolio.closePosition("X", 130.0, 20, 0.78, "S1", 6.0);
    requireNear(portfolio.getCashBalance(), 99'177.62, "partial-close cash mismatch");
    requireNear(portfolio.getPositionQuantity("X"), 9.0,
                "partial-close quantity mismatch");
    requireNear(portfolio.getTotalEquity(), 100'347.62,
                "partial-close equity mismatch");
    state = portfolio.snapshotState();
    requireNear(state.unrealizedPnL, 210.0,
                "partial-close unrealized P&L mismatch");
    require(state.tradeLog.size() == 1, "partial close did not create one trade record");
    requireNear(state.tradeLog.back().grossPnL, 140.0,
                "partial-close gross P&L mismatch");
    requireNear(state.tradeLog.back().realizedPnL, 138.58,
                "partial-close realized P&L mismatch");
    requireNear(state.tradeLog.back().totalFees, 1.42,
                "partial-close fee allocation mismatch");

    portfolio.closePosition("X", 110.0, 30, 0.99, "S1", 9.0);
    require(!portfolio.hasPosition("X"), "full close retained a position");
    requireNear(portfolio.getCashBalance(), 100'166.63, "full-close cash mismatch");
    requireNear(portfolio.getTotalEquity(), 100'166.63,
                "full-close equity mismatch");
    requireNear(portfolio.getTotalFeesPaid(), 3.37, "total fee mismatch");
    require(portfolio.getRoundTripCount() == 1, "round-trip count mismatch");
    requireNear(portfolio.getTradeLog().back().realizedPnL, 28.05,
                "final realized P&L mismatch");
}

void testMultiSymbolMarksRemainIndependent()
{
    PortfolioManager portfolio;
    portfolio.openLong("A", 100.0, 1, 0.0, 100.0, 1.0);
    portfolio.openLong("B", 50.0, 2, 0.0, 100.0, 2.0);
    portfolio.updatePnL("A", 120.0);
    requireNear(portfolio.getTotalEquity(), 100'020.0,
                "first symbol mark did not affect equity correctly");
    requireNear(portfolio.snapshotState().unrealizedPnL, 20.0,
                "first symbol unrealized P&L mismatch");

    portfolio.updatePnL("B", 40.0);
    requireNear(portfolio.getTotalEquity(), 100'000.0,
                "second symbol reset the first symbol mark");
    requireNear(portfolio.snapshotState().unrealizedPnL, 0.0,
                "aggregate multi-symbol unrealized P&L mismatch");

    portfolio.updatePnL("UNKNOWN", 1.0);
    requireNear(portfolio.getTotalEquity(), 100'000.0,
                "unknown symbol changed portfolio marks");
}

void testReversalAndInvalidInputFailBeforeMutation()
{
    PortfolioManager portfolio;
    portfolio.openLong("X", 100.0, 1, 0.0, 100.0, 1.0);
    const auto before = portfolio.snapshotState();

    bool reversalRejected = false;
    try {
        portfolio.closePosition("X", 110.0, 2, 0.0, "", 2.0);
    } catch (const std::logic_error&) {
        reversalRejected = true;
    }
    require(reversalRejected, "unsupported reversal was not rejected");

    bool overflowRejected = false;
    try {
        portfolio.addToLong("X", std::numeric_limits<double>::max(),
                            1.0, 0.0, 0.0);
    } catch (const std::exception&) {
        overflowRejected = true;
    }
    require(overflowRejected, "overflowing add-on was not rejected");

    const auto after = portfolio.snapshotState();
    require(before.cash == after.cash && before.totalEquity == after.totalEquity
            && before.totalFeesPaid == after.totalFeesPaid
            && before.tradeCount == after.tradeCount
            && before.positions.size() == after.positions.size()
            && before.positions.front().position.quantity
                == after.positions.front().position.quantity,
            "rejected accounting input mutated portfolio state");
}

void testPaperCostsUseCanonicalContract()
{
    SystemConfig config;
    config.mode = SystemMode::PAPER;
    PortfolioManager portfolio;
    BrokerGateway gateway(config, portfolio);
    require(gateway.setPaperSimulationCosts(0.002, 10.0),
            "valid PAPER costs were rejected");
    gateway.connect();

    ExecutionEvent observed;
    bool received = false;
    gateway.setExecutionCallback([&](const ExecutionEvent& execution) {
        observed = execution;
        received = true;
    });

    OrderRequest request;
    request.localOrderId = 1;
    request.canonicalSymbol = "X";
    request.side = OrderSide::Buy;
    request.type = BrokerOrderType::Market;
    request.quantity = Decimal64::fromDouble(2.0, 8).value();
    request.referencePrice = Decimal64::fromDouble(100.0, 8).value();
    request.sourceId = "WP2";
    request.timestampNs = 1;
    request.sequence = 1;
    request.idempotencyKey = "wp2-costs-1";
    const auto normalized = gateway.normalizeOrder(request);
    require(normalized.has_value(), "PAPER order normalization failed");
    RiskDecision riskDecision;
    riskDecision.allowed = true;
    riskDecision.riskIncreasing = true;
    const auto dispatch = gateway.dispatchOrder(*normalized, riskDecision);
    require(dispatch.dispatched && received, "deterministic PAPER execution failed");
    requireNear(observed.fillPrice.toDouble(), 100.1, "PAPER slippage mismatch");
    requireNear(observed.cumulativeFilledQuantity.toDouble(), 2.0,
                "PAPER quantity mismatch");
    requireNear(observed.fee.toDouble(), 0.4004, "PAPER fee mismatch");
    require(!gateway.setPaperSimulationCosts(-0.001, 5.0),
            "negative PAPER fee rate was accepted");
}

void testMalformedFillConfirmationFailsClosed()
{
    SystemConfig config;
    config.mode = SystemMode::PAPER;
    PortfolioManager portfolio;
    BrokerGateway gateway(config, portfolio);
    gateway.connect();
    gateway.injectNextPartialFill(0.5);

    OrderRequest request;
    request.localOrderId = 1;
    request.canonicalSymbol = "X";
    request.side = OrderSide::Buy;
    request.type = BrokerOrderType::Market;
    request.quantity = Decimal64::fromDouble(2.0, 8).value();
    request.referencePrice = Decimal64::fromDouble(100.0, 8).value();
    request.sourceId = "WP2";
    request.timestampNs = 1;
    request.sequence = 1;
    request.idempotencyKey = "wp2-partial-1";
    const auto normalized = gateway.normalizeOrder(request);
    require(normalized.has_value(), "partial PAPER normalization failed");
    RiskDecision riskDecision;
    riskDecision.allowed = true;
    riskDecision.riskIncreasing = true;
    require(gateway.dispatchOrder(*normalized, riskDecision).dispatched,
            "partial PAPER fixture did not dispatch");

    const auto before = gateway.orderLifecycle(request.localOrderId);
    require(before.has_value()
            && before->state == OrderLifecycleState::PartiallyFilled,
            "partial PAPER fixture did not produce an open lifecycle");
    const auto errorsBefore = gateway.totalApiErrors();

    ExecutionEvent malformed;
    malformed.localOrderId = request.localOrderId;
    malformed.externalOrderId = before->externalOrderId;
    malformed.cumulativeFilledQuantity = Decimal64{
        std::numeric_limits<std::int64_t>::max(), 8};
    malformed.remainingQuantity = Decimal64{0, 8};
    malformed.fillPrice = Decimal64::fromDouble(100.0, 8).value();
    malformed.fee = Decimal64{0, 8};
    malformed.timestampNs = 2;
    malformed.sequence = before->lastSequence + 1;
    malformed.eventKey = "wp2-malformed-1";
    gateway.simulateExecutionEvent(malformed);

    const auto after = gateway.orderLifecycle(request.localOrderId);
    require(after.has_value()
            && after->filledQuantity == before->filledQuantity
            && after->remainingQuantity == before->remainingQuantity,
            "overflowing fill confirmation mutated lifecycle state");
    require(gateway.totalApiErrors() == errorsBefore + 1,
            "rejected fill confirmation was not counted as an API error");
}

void testLongRunConservation()
{
    PortfolioManager portfolio;
    double realized = 0.0;
    for (std::uint64_t i = 0; i < 1'000; ++i) {
        portfolio.openLong("X", 100.0, i * 2, 0.01, 100.01, 1.0, "LOOP");
        portfolio.closePosition("X", 100.0, i * 2 + 1, 0.01, "LOOP", 1.0);
        realized += portfolio.getTradeLog().back().realizedPnL;
    }
    requireNear(realized, -20.0, "long-run realized P&L drifted");
    requireNear(portfolio.getCashBalance(), 99'980.0, "long-run cash drifted");
    requireNear(portfolio.getTotalEquity(), 99'980.0, "long-run equity drifted");
    requireNear(portfolio.getTotalFeesPaid(), 20.0, "long-run fees drifted");
    require(portfolio.getTradeCount() == 2'000, "long-run fill count mismatch");
    require(portfolio.getRoundTripCount() == 1'000,
            "long-run round-trip count mismatch");
}

} // namespace

int main()
{
    try {
        testGoldenFixedPointVectors();
        testBuyAddReduceCloseAccounting();
        testMultiSymbolMarksRemainIndependent();
        testReversalAndInvalidInputFailBeforeMutation();
        testPaperCostsUseCanonicalContract();
        testMalformedFillConfirmationFailsClosed();
        testLongRunConservation();
        std::cout << "WP-2 accounting tests passed.\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "WP-2 accounting test failed: " << exception.what() << "\n";
        return 1;
    }
}
