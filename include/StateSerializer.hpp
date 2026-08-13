#pragma once

#include "PortfolioAllocator.hpp"
#include "PortfolioManager.hpp"
#include "RegimeDetector.hpp"
#include "RiskEngine.hpp"

#include <cstdint>
#include <string>

// Versioned BACKTEST restart-state serializer. Version 12 uses a strict,
// checksummed JSON envelope and atomic same-directory replacement. Earlier
// versions are deliberately rejected; no implicit migration exists.
class StateSerializer {
public:
    static constexpr uint64_t SNAPSHOT_VERSION = 12;

    bool saveSnapshot(const PortfolioManager& portfolio,
                      const RiskEngine& riskEngine,
                      const RegimeDetector& regimeDetector,
                      const PortfolioAllocator& allocator,
                      uint64_t checkpointTs,
                      const std::string& filepath = "data/results/snapshot.json") const;

    bool saveSnapshot(const PortfolioManager& portfolio,
                      const RiskEngine& riskEngine,
                      uint64_t checkpointTs,
                      const std::string& filepath = "data/results/snapshot.json") const;

    bool loadSnapshot(PortfolioManager& portfolio,
                      RiskEngine& riskEngine,
                      RegimeDetector& regimeDetector,
                      PortfolioAllocator& allocator,
                      uint64_t& checkpointTs,
                      const std::string& filepath = "data/results/snapshot.json") const;

    bool loadSnapshot(PortfolioManager& portfolio,
                      RiskEngine& riskEngine,
                      uint64_t& checkpointTs,
                      const std::string& filepath = "data/results/snapshot.json") const;

    const std::string& lastError() const noexcept { return m_lastError; }

private:
    mutable std::string m_lastError;
};
