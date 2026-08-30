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

// Offline-only validation seam for the libcurl option boundary. It configures
// a handle but performs no request and handles no credentials or tokens.
bool validateCTraderTokenTransportConfigurationOffline() noexcept;

// Offline-only validation seam for the immutable-Keychain-data ownership
// boundary. The supplied bytes are read-only; only the process-owned copy is
// cleared. It performs no Keychain operation.
bool validateCTraderKeychainCopyBoundaryOffline(std::string_view input) noexcept;

} // namespace tradebot::ctrader
