#include "BrokerGateway.hpp"
#include "ExecutionEngine.hpp"
#include "LiveDataAdapter.hpp"
#include "PortfolioManager.hpp"
#include "RiskEngine.hpp"
#include "SystemConfig.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void testModeParsingIsExplicit()
{
    require(parseModeFlag("backtest") == SystemMode::BACKTEST,
            "BACKTEST mode parse failed");
    require(parseModeFlag("paper") == SystemMode::PAPER,
            "PAPER mode parse failed");
    require(parseModeFlag("live") == SystemMode::LIVE,
            "LIVE mode parse failed");
    require(!parseModeFlag("production").has_value(),
            "invalid mode silently downgraded instead of failing");
}

void testBuildAndRuntimeGatesCompose()
{
    SystemConfig config;
    config.mode = SystemMode::LIVE;
    config.liveRuntimeUnlocked = false;
    config.apiKey = "synthetic-key-not-a-secret";
    config.apiSecret = "synthetic-secret-not-a-secret";
    config.wssEndpoint = "mock://must-not-connect";

    bool integrity = true;
    LiveDataAdapter adapter(config);
    adapter.setIntegrityCallback([&](bool healthy) { integrity = healthy; });
    adapter.connect();

    require(!config.canEnterLiveRuntime(),
            "LIVE runtime entered without both containment gates");

    if (liveRuntimeBuildEnabled()) {
        config.liveRuntimeUnlocked = true;
        require(config.canEnterLiveRuntime(),
                "opt-in build did not honor the separate runtime gate");
        config.liveRuntimeUnlocked = false;
    }
    require(!adapter.isConnected(),
            "LIVE adapter connected without runtime authorization");
    require(!integrity,
            "blocked LIVE adapter did not report degraded integrity");

    adapter.simulateReconnect();
    require(!adapter.isConnected(),
            "LIVE reconnect bypassed the containment gates");
}

void testBoundUnavailableGatewayCannotCreateLocalFill()
{
    SystemConfig config;
    config.mode = SystemMode::LIVE;

    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 2);
    risk.setSystemConfig(&config);
    BrokerGateway gateway(config, portfolio);
    gateway.connect();

    ExecutionEngine execution(portfolio, risk, "BTCUSDT");
    execution.bindBrokerGateway(&gateway);

    const double cashBefore = portfolio.getCashBalance();
    const bool executed = execution.execute(Signal::BUY, 100.0, 1, "WP0_TEST");
    require(!executed,
            "unavailable bound gateway fell back to local execution");
    require(!portfolio.hasPosition("BTCUSDT"),
            "unavailable bound gateway created a local position");
    require(portfolio.getCashBalance() == cashBefore,
            "unavailable bound gateway changed local cash");
    require(execution.getFilledCount() == 0,
            "unavailable bound gateway incremented fill count");
    require(execution.getBlockedCount() == 1,
            "unavailable bound gateway did not record a block");
}

void testLiveCannotApprovePaperAdapter()
{
    SystemConfig config;
    config.mode = SystemMode::LIVE;

    PortfolioManager portfolio;
    BrokerGateway gateway(
        config,
        portfolio,
        std::make_unique<DeterministicBrokerAdapter>(),
        true);
    gateway.connect();

    require(!gateway.isConnected(),
            "LIVE accepted the local deterministic PAPER adapter");
}

void testBacktestAndPaperRemainExplicit()
{
    SystemConfig backtest;
    require(backtest.isBacktest(), "SystemConfig no longer defaults to BACKTEST");

    SystemConfig paper;
    paper.mode = SystemMode::PAPER;
    LiveDataAdapter data(paper);
    data.connect();
    require(data.isConnected(), "PAPER data simulation did not connect locally");

    PortfolioManager portfolio;
    BrokerGateway gateway(paper, portfolio);
    gateway.connect();
    require(gateway.isConnected(), "PAPER broker simulation did not connect locally");
}

} // namespace

int main()
{
    testModeParsingIsExplicit();
    testBuildAndRuntimeGatesCompose();
    testBoundUnavailableGatewayCannotCreateLocalFill();
    testLiveCannotApprovePaperAdapter();
    testBacktestAndPaperRemainExplicit();
    std::cout << "wp0_containment_tests: PASS\n";
    return 0;
}
