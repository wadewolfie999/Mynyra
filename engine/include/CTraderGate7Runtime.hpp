#pragma once

#include <string_view>

namespace tradebot::ctrader {

// Executes one isolated Gate 7 account-to-fresh-XAUUSD-spot proof. The target
// is opt-in, macOS-only, and detached from all normal TradeBot runtime modes.
int runCTraderGate7Proof(bool preflightOnly = false);

// Offline-only configuration seam. It performs no Keychain, browser, socket,
// provider, token, account, symbol, or market-data operation.
bool validateCTraderGate7OfflineConfiguration() noexcept;

} // namespace tradebot::ctrader
