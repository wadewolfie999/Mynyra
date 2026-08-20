#include "providers/ctrader/CTraderProviderAdapter.hpp"

#include <utility>

namespace tradebot::ctrader {

class CTraderTransport {};
class CTraderCodec {};
class CTraderAuthService {};
class CTraderAccountService {};
class CTraderInstrumentService {};
class CTraderMarketDataService {};
class CTraderOrderService {};
class CTraderSession {};

CTraderProviderAdapter::CTraderProviderAdapter()
    : m_transport(std::make_unique<CTraderTransport>())
    , m_codec(std::make_unique<CTraderCodec>())
    , m_auth(std::make_unique<CTraderAuthService>())
    , m_accounts(std::make_unique<CTraderAccountService>())
    , m_instruments(std::make_unique<CTraderInstrumentService>())
    , m_marketData(std::make_unique<CTraderMarketDataService>())
    , m_orders(std::make_unique<CTraderOrderService>())
    , m_session(std::make_unique<CTraderSession>())
{
    m_health.schemaVersion = 1;
    m_health.state = AdapterHealthState::Disconnected;
    m_health.failure = FailureCategory::Validation;
    m_health.reason = "cTrader provider module is default-disabled";
    m_health.eventKey = "ctrader-provider-disabled";
}

CTraderProviderAdapter::~CTraderProviderAdapter() = default;

void CTraderProviderAdapter::setAcknowledgementCallback(
    AcknowledgementCallback callback)
{
    m_acknowledgementCallback = std::move(callback);
}

void CTraderProviderAdapter::setExecutionCallback(ExecutionCallback callback)
{
    m_executionCallback = std::move(callback);
}

void CTraderProviderAdapter::setCancelCallback(CancelCallback callback)
{
    m_cancelCallback = std::move(callback);
}

void CTraderProviderAdapter::setHealthCallback(HealthCallback callback)
{
    m_healthCallback = std::move(callback);
}

void CTraderProviderAdapter::setMarketDataCallback(MarketDataCallback callback)
{
    m_marketDataCallback = std::move(callback);
}

bool CTraderProviderAdapter::connect()
{
    publishDisabledHealth();
    return false;
}

void CTraderProviderAdapter::disconnect() noexcept
{
    publishDisabledHealth();
}

bool CTraderProviderAdapter::isConnected() const noexcept
{
    return false;
}

bool CTraderProviderAdapter::submit(const NormalizedOrder& order)
{
    (void)order;
    return false;
}

bool CTraderProviderAdapter::cancel(const CancelRequest& request)
{
    (void)request;
    return false;
}

ReconciliationSnapshot CTraderProviderAdapter::reconcile(
    std::uint64_t timestampNs)
{
    ReconciliationSnapshot snapshot;
    snapshot.timestampNs = timestampNs;
    snapshot.status = ReconciliationStatus::Unsupported;
    return snapshot;
}

AdapterHealthEvent CTraderProviderAdapter::health() const
{
    return m_health;
}

std::optional<AccountSnapshot> CTraderProviderAdapter::accountSnapshot() const
{
    return std::nullopt;
}

std::optional<InstrumentSpec> CTraderProviderAdapter::instrumentSpec(
    const std::string& canonicalSymbol) const
{
    (void)canonicalSymbol;
    return std::nullopt;
}

void CTraderProviderAdapter::publishDisabledHealth() noexcept
{
    if (m_healthCallback) {
        m_healthCallback(m_health);
    }
}

} // namespace tradebot::ctrader
