#include "BrokerAdapterContracts.hpp"
#include "BrokerGateway.hpp"
#include "PortfolioManager.hpp"
#include "SystemConfig.hpp"
#include "providers/ctrader/CTraderProviderAdapter.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void testNormalizedMarketDataContract()
{
    MarketDataEvent event;
    event.canonicalSymbol = "SYNTH-USD";
    event.bid = Decimal64{10000, 2};
    event.ask = Decimal64{10001, 2};
    event.sourceTimestampNs = 1000;
    event.sequence = 1;
    event.instrumentVersion = 2;
    event.quality = AdapterHealthState::Connected;
    event.eventKey = "synthetic-bbo-1";
    require(event.ask.units > event.bid.units,
            "normalized event must retain an independently comparable BBO");
}

void testDefaultDisabledProviderSkeleton()
{
    tradebot::ctrader::CTraderProviderAdapter adapter;
#if TRADEBOT_ENABLE_CTRADER_DEMO
    require(!adapter.isConnected(),
            "opt-in provider must not perform implicit startup I/O");
    require(!adapter.accountSnapshot().has_value(),
            "inactive opt-in provider must not invent account state");
    NormalizedOrder order;
    order.request.localOrderId = 1;
    require(!adapter.submit(order),
            "inactive opt-in provider must reject dispatch without connecting");
    require(adapter.reconcile(1000).status == ReconciliationStatus::Unsupported,
            "inactive opt-in provider must reject reconciliation without I/O");
#else
    int healthEvents = 0;
    adapter.setHealthCallback([&](const AdapterHealthEvent& event) {
        ++healthEvents;
        require(event.state == AdapterHealthState::Disconnected,
                "skeleton must remain disconnected");
        require(event.failure == FailureCategory::Validation,
                "skeleton must advertise fail-closed validation state");
    });
    IMarketDataSource& marketData = adapter;
    marketData.setMarketDataCallback([](const MarketDataEvent&) {});

    require(!adapter.isConnected(), "skeleton connected before any operation");
    require(!adapter.connect(), "skeleton must not activate a provider connection");
    require(healthEvents == 1, "failed connect must publish one health event");
    require(!adapter.accountSnapshot().has_value(),
            "skeleton must not expose an account snapshot");
    require(!adapter.instrumentSpec("SYNTH-USD").has_value(),
            "skeleton must not expose provider instrument metadata");

    NormalizedOrder order;
    order.request.localOrderId = 1;
    require(!adapter.submit(order), "skeleton submit must fail closed");
    CancelRequest cancel;
    cancel.localOrderId = 1;
    require(!adapter.cancel(cancel), "skeleton cancel must fail closed");
    require(adapter.reconcile(1000).status == ReconciliationStatus::Unsupported,
            "skeleton reconciliation must remain unsupported");
#endif
}

void testGatewayFansOutNormalizedLifecycleEvents()
{
    SystemConfig config;
    config.mode = SystemMode::PAPER;
    PortfolioManager portfolio;
    BrokerGateway gateway(config, portfolio);
    int acknowledgements = 0;
    int executions = 0;
    gateway.addAcknowledgementCallback([&](const OrderAcknowledgement& event) {
        require(event.accepted, "synthetic adapter acknowledgement must be normalized");
        ++acknowledgements;
    });
    gateway.addExecutionCallback([&](const ExecutionEvent& event) {
        require(event.cumulativeFilledQuantity.isPositive(),
                "synthetic execution must include a normalized fill quantity");
        ++executions;
    });
    gateway.connect();

    OrderRequest request;
    request.localOrderId = 99;
    request.canonicalSymbol = "SYNTH-USD";
    request.side = OrderSide::Buy;
    request.type = BrokerOrderType::Market;
    request.quantity = Decimal64{100, 2};
    request.referencePrice = Decimal64{10000, 2};
    request.timestampNs = 1000;
    request.sequence = 1;
    request.idempotencyKey = "provider-architecture-99";
    const auto normalized = gateway.normalizeOrder(request);
    require(normalized.has_value(), "gateway must normalize the synthetic order");
    RiskDecision risk;
    risk.allowed = true;
    require(gateway.dispatchOrder(*normalized, risk).dispatched,
            "gateway must dispatch through IBrokerAdapter");
    require(acknowledgements == 1 && executions == 1,
            "gateway must fan out acknowledgement and execution independently");
}

} // namespace

int main()
{
    testNormalizedMarketDataContract();
    testDefaultDisabledProviderSkeleton();
    testGatewayFansOutNormalizedLifecycleEvents();
    std::cout << "ctrader_provider_architecture_tests: PASS\n";
    return 0;
}
