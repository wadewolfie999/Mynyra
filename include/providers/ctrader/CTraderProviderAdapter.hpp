#pragma once

#include "IBrokerAdapter.hpp"

#include <memory>

namespace tradebot::ctrader {

class CTraderSession;
class CTraderCodec;
class CTraderTransport;
class CTraderAuthService;
class CTraderAccountService;
class CTraderInstrumentService;
class CTraderMarketDataService;
class CTraderOrderService;

// Default-disabled structural adapter. It deliberately contains no Open API
// SDK, socket, OAuth, credential-store, or runtime-selection behavior.
class CTraderProviderAdapter final : public IBrokerAdapter,
                                     public IMarketDataSource {
public:
    CTraderProviderAdapter();
    ~CTraderProviderAdapter() override;

    CTraderProviderAdapter(const CTraderProviderAdapter&) = delete;
    CTraderProviderAdapter& operator=(const CTraderProviderAdapter&) = delete;

    void setAcknowledgementCallback(AcknowledgementCallback callback) override;
    void setExecutionCallback(ExecutionCallback callback) override;
    void setCancelCallback(CancelCallback callback) override;
    void setHealthCallback(HealthCallback callback) override;
    void setMarketDataCallback(MarketDataCallback callback) override;

    bool connect() override;
    void disconnect() noexcept override;
    bool isConnected() const noexcept override;
    bool submit(const NormalizedOrder& order) override;
    bool cancel(const CancelRequest& request) override;
    ReconciliationSnapshot reconcile(std::uint64_t timestampNs) override;

    AdapterHealthEvent health() const override;
    std::optional<AccountSnapshot> accountSnapshot() const override;
    std::optional<InstrumentSpec> instrumentSpec(
        const std::string& canonicalSymbol) const override;

private:
    void publishDisabledHealth() noexcept;

    std::unique_ptr<CTraderTransport> m_transport;
    std::unique_ptr<CTraderCodec> m_codec;
    std::unique_ptr<CTraderAuthService> m_auth;
    std::unique_ptr<CTraderAccountService> m_accounts;
    std::unique_ptr<CTraderInstrumentService> m_instruments;
    std::unique_ptr<CTraderMarketDataService> m_marketData;
    std::unique_ptr<CTraderOrderService> m_orders;
    std::unique_ptr<CTraderSession> m_session;
    AcknowledgementCallback m_acknowledgementCallback;
    ExecutionCallback m_executionCallback;
    CancelCallback m_cancelCallback;
    HealthCallback m_healthCallback;
    MarketDataCallback m_marketDataCallback;
    AdapterHealthEvent m_health;
};

} // namespace tradebot::ctrader
