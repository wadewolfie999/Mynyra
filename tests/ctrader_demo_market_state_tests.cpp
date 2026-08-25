#include "providers/ctrader/CTraderDemoMarketState.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using namespace tradebot::ctrader;

void require(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

MarketCandle candle(std::uint64_t timestamp, double close = 2300.5)
{
    MarketCandle value;
    value.epochTimestamp = timestamp;
    value.open = 2300.0;
    value.high = 2301.0;
    value.low = 2299.0;
    value.close = close;
    value.volume = 10.0;
    value.symbol = "XAUUSD";
    return value;
}

void testIndependentBboSidesAndConservativeFreshness()
{
    CTraderDemoMarketState state;
    CTraderDemoSpotUpdate bid;
    bid.bid = Decimal64{230000, 2};
    bid.sourceTimestampNs = 100;
    require(state.apply(bid) == CTraderDemoMarketState::ApplyResult::Applied
                && !state.bbo().has_value(),
            "partial bid incorrectly published a BBO");

    CTraderDemoSpotUpdate ask;
    ask.ask = Decimal64{230010, 2};
    ask.sourceTimestampNs = 200;
    require(state.apply(ask) == CTraderDemoMarketState::ApplyResult::Applied,
            "ask update failed");
    const auto bbo = state.bbo();
    require(bbo.has_value() && bbo->sourceTimestampNs == 100,
            "BBO freshness did not use the older independent side");

    CTraderDemoSpotUpdate crossed;
    crossed.bid = Decimal64{230020, 2};
    crossed.sourceTimestampNs = 300;
    require(state.apply(crossed)
                == CTraderDemoMarketState::ApplyResult::Malformed,
            "crossed BBO was accepted");
}

void testLiveRolloverDuplicatesAndOverflow()
{
    CTraderDemoMarketState state(1);
    CTraderDemoSpotUpdate first;
    first.liveM1Bars = {candle(60), candle(60, 2300.7)};
    require(state.apply(first) == CTraderDemoMarketState::ApplyResult::Applied
                && !state.hasCompletedCandle(),
            "same-minute update emitted an unfinished bar");
    CTraderDemoSpotUpdate rollover;
    rollover.liveM1Bars = {candle(120)};
    require(state.apply(rollover) == CTraderDemoMarketState::ApplyResult::Applied
                && state.hasCompletedCandle(),
            "minute rollover did not emit the completed prior bar");
    const auto completed = state.popCompletedCandle();
    require(completed.has_value() && completed->epochTimestamp == 60
                && completed->close == 2300.7,
            "rollover emitted the wrong bar revision");

    CTraderDemoSpotUpdate secondRollover;
    secondRollover.liveM1Bars = {candle(180), candle(240)};
    require(state.apply(secondRollover)
                == CTraderDemoMarketState::ApplyResult::QueueOverflow,
            "bounded completed-candle queue did not fail closed");

    CTraderDemoMarketState ordered;
    CTraderDemoSpotUpdate later;
    later.liveM1Bars = {candle(180)};
    require(ordered.apply(later) == CTraderDemoMarketState::ApplyResult::Applied,
            "initial live bar failed");
    CTraderDemoSpotUpdate stale;
    stale.liveM1Bars = {candle(120)};
    require(ordered.apply(stale)
                == CTraderDemoMarketState::ApplyResult::Malformed,
            "out-of-order live bar was accepted");
}

void testHistoricalOrderingAndDuplicates()
{
    std::vector<MarketCandle> history{
        candle(180), candle(60), candle(120)};
    auto normalized = CTraderDemoMarketState::normalizeHistory(
        std::move(history), 3, 240);
    require(normalized.has_value()
                && (*normalized)[0].epochTimestamp == 60
                && (*normalized)[2].epochTimestamp == 180,
            "historical bars were not sorted deterministically");
    std::vector<MarketCandle> duplicate{candle(60), candle(60)};
    require(!CTraderDemoMarketState::normalizeHistory(
                 std::move(duplicate), 2, 120).has_value(),
            "duplicate historical bars were accepted");
    std::vector<MarketCandle> unfinished{candle(120)};
    require(!CTraderDemoMarketState::normalizeHistory(
                 std::move(unfinished), 1, 120).has_value(),
            "unfinished historical bar was accepted");
}

} // namespace

int main()
{
    testIndependentBboSidesAndConservativeFreshness();
    testLiveRolloverDuplicatesAndOverflow();
    testHistoricalOrderingAndDuplicates();
    std::cout << "ctrader_demo_market_state_tests: PASS\n";
    return 0;
}
