#pragma once

#include <string_view>

namespace tradebot::ctrader {

// Executes the local-only Gate 6A -> Wade checkpoint -> Gate 6B proof. The
// implementation emits only fixed diagnostics and explicitly approved safe
// broker metadata. It has no symbol, market-data, position, or order API.
int runCTraderGate6Proof(bool preflightOnly = false);

// Offline-only validation seam for synthetic token-response fixtures. It
// performs no network, Keychain, browser, or provider operation and retains no
// parsed token material after returning.
bool validateCTraderTokenResponseOffline(std::string_view response) noexcept;

} // namespace tradebot::ctrader
