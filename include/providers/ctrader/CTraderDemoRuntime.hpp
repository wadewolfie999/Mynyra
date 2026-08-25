#pragma once

#include "SystemConfig.hpp"

namespace tradebot::ctrader {

// Runs only the fixed XAUUSD/M1 cTrader Demo composition selected by main.
// The implementation exists only in TRADEBOT_ENABLE_CTRADER_DEMO builds.
int runCTraderDemoRuntime(const SystemConfig& config);

} // namespace tradebot::ctrader
