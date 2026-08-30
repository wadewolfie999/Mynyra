#include "StateSerializer.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <type_traits>
#include <vector>

namespace {
constexpr std::size_t MAX_ITEMS = 1'000'000;
constexpr std::size_t MAX_STRING = 1'048'576;
constexpr std::size_t MAX_PAYLOAD = 128 * 1024 * 1024;

class Writer {
public:
    void u8(std::uint8_t value) { m_bytes.push_back(value); }
    void u32(std::uint32_t value) { integer(value); }
    void u64(std::uint64_t value) { integer(value); }
    void i64(std::int64_t value) {
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        u64(bits);
    }
    void i32(std::int32_t value) { integer(static_cast<std::uint32_t>(value)); }
    void boolean(bool value) { u8(value ? 1 : 0); }
    void number(double value) {
        std::uint64_t bits = 0;
        static_assert(sizeof(bits) == sizeof(value));
        std::memcpy(&bits, &value, sizeof(bits));
        u64(bits);
    }
    void string(const std::string& value) {
        u64(value.size());
        m_bytes.insert(m_bytes.end(), value.begin(), value.end());
    }
    const std::vector<std::uint8_t>& bytes() const { return m_bytes; }

private:
    template <typename T> void integer(T value) {
        static_assert(std::is_unsigned_v<T>);
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            u8(static_cast<std::uint8_t>((value >> (i * 8)) & 0xff));
        }
    }
    std::vector<std::uint8_t> m_bytes;
};

class Reader {
public:
    explicit Reader(const std::vector<std::uint8_t>& bytes) : m_bytes(bytes) {}

    bool u8(std::uint8_t& out) {
        if (m_offset >= m_bytes.size()) { return fail("truncated payload"); }
        out = m_bytes[m_offset++];
        return true;
    }
    bool u32(std::uint32_t& out) { return integer(out); }
    bool u64(std::uint64_t& out) { return integer(out); }
    bool i64(std::int64_t& out) {
        std::uint64_t bits = 0;
        if (!u64(bits)) { return false; }
        std::memcpy(&out, &bits, sizeof(out));
        return true;
    }
    bool i32(std::int32_t& out) {
        std::uint32_t value = 0;
        if (!u32(value)) { return false; }
        out = static_cast<std::int32_t>(value);
        return true;
    }
    bool boolean(bool& out) {
        std::uint8_t value = 0;
        if (!u8(value) || value > 1) { return fail("invalid boolean"); }
        out = value != 0;
        return true;
    }
    bool number(double& out) {
        std::uint64_t bits = 0;
        if (!u64(bits)) { return false; }
        std::memcpy(&out, &bits, sizeof(out));
        return std::isfinite(out) || fail("non-finite number");
    }
    bool string(std::string& out) {
        std::uint64_t size = 0;
        if (!u64(size) || size > MAX_STRING || size > remaining()) {
            return fail("invalid string length");
        }
        out.assign(reinterpret_cast<const char*>(m_bytes.data() + m_offset),
                   static_cast<std::size_t>(size));
        m_offset += static_cast<std::size_t>(size);
        return true;
    }
    bool count(std::size_t& out) {
        std::uint64_t value = 0;
        if (!u64(value) || value > MAX_ITEMS) { return fail("invalid item count"); }
        out = static_cast<std::size_t>(value);
        return true;
    }
    bool done() const { return m_offset == m_bytes.size(); }
    const std::string& error() const { return m_error; }

private:
    template <typename T> bool integer(T& out) {
        static_assert(std::is_unsigned_v<T>);
        if (remaining() < sizeof(T)) { return fail("truncated payload"); }
        out = 0;
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            out |= static_cast<T>(m_bytes[m_offset++]) << (i * 8);
        }
        return true;
    }
    std::size_t remaining() const { return m_bytes.size() - m_offset; }
    bool fail(const char* message) {
        if (m_error.empty()) { m_error = message; }
        return false;
    }
    const std::vector<std::uint8_t>& m_bytes;
    std::size_t m_offset{0};
    std::string m_error;
};

void writeDoubles(Writer& writer, const std::deque<double>& values) {
    writer.u64(values.size());
    for (double value : values) { writer.number(value); }
}
void writeDoubles(Writer& writer, const std::vector<double>& values) {
    writer.u64(values.size());
    for (double value : values) { writer.number(value); }
}
bool readDoubles(Reader& reader, std::deque<double>& values) {
    std::size_t count = 0;
    if (!reader.count(count)) { return false; }
    for (std::size_t i = 0; i < count; ++i) {
        double value = 0;
        if (!reader.number(value)) { return false; }
        values.push_back(value);
    }
    return true;
}
bool readDoubles(Reader& reader, std::vector<double>& values) {
    std::size_t count = 0;
    if (!reader.count(count)) { return false; }
    values.resize(count);
    for (double& value : values) {
        if (!reader.number(value)) { return false; }
    }
    return true;
}

void writePortfolio(Writer& w, const PortfolioManager::Snapshot& s) {
    const auto moneyUnits = [](double value) {
        return Financial::money(value, Financial::Rounding::RejectUnaligned)->units;
    };
    const auto priceUnits = [](double value) {
        return Financial::price(value, Financial::Rounding::RejectUnaligned)->units;
    };
    const auto quantityUnits = [](double value) {
        return Financial::quantity(value, Financial::Rounding::RejectUnaligned)->units;
    };
    w.i64(moneyUnits(s.cash)); w.i64(moneyUnits(s.unrealizedPnL));
    w.i64(moneyUnits(s.totalEquity)); w.i64(moneyUnits(s.maxEquity));
    w.number(s.currentDrawdown); w.number(s.maxDrawdown);
    w.i32(s.tradeCount); w.i64(moneyUnits(s.totalFeesPaid)); w.i32(s.roundTripCount);
    w.u64(s.positions.size());
    for (const auto& p : s.positions) {
        w.string(p.position.symbol); w.i64(quantityUnits(p.position.quantity));
        w.i64(priceUnits(p.position.entryPrice)); w.boolean(p.position.isLong);
        w.u64(p.entryTimestamp); w.i64(moneyUnits(p.entryFee));
        w.i64(moneyUnits(p.costBasis)); w.i64(priceUnits(p.lastMarkPrice));
        w.string(p.strategyId);
    }
    w.u64(s.tradeLog.size());
    for (const auto& t : s.tradeLog) {
        w.string(t.symbol); w.u64(t.openTimestamp); w.u64(t.closeTimestamp);
        w.i64(priceUnits(t.entryPrice)); w.i64(priceUnits(t.exitPrice));
        w.i64(quantityUnits(t.quantity)); w.i64(moneyUnits(t.totalFees));
        w.i64(moneyUnits(t.realizedPnL)); w.i64(moneyUnits(t.grossPnL));
        w.string(t.strategy_id);
    }
    w.u64(s.pendingOrders.size());
    for (const auto& o : s.pendingOrders) {
        w.string(o.symbol); w.u8(static_cast<std::uint8_t>(o.orderType));
        w.boolean(o.isBuy); w.i64(priceUnits(o.limitPrice));
        w.i64(priceUnits(o.trailOffset)); w.i64(priceUnits(o.trailBest));
        w.i64(quantityUnits(o.quantity)); w.i64(moneyUnits(o.capitalToCommit));
        w.u64(o.placedTimestamp); w.u64(o.orderId);
    }
    w.u64(s.nextOrderId);
}

bool readPortfolio(Reader& r, PortfolioManager::Snapshot& s) {
    std::int32_t tradeCount = 0, roundTripCount = 0;
    std::int64_t cash = 0, unrealized = 0, equity = 0, maxEquity = 0, fees = 0;
    if (!r.i64(cash) || !r.i64(unrealized) || !r.i64(equity)
        || !r.i64(maxEquity) || !r.number(s.currentDrawdown)
        || !r.number(s.maxDrawdown) || !r.i32(tradeCount)
        || !r.i64(fees) || !r.i32(roundTripCount)) { return false; }
    s.cash = Financial::Money{cash}.toDouble();
    s.unrealizedPnL = Financial::Money{unrealized}.toDouble();
    s.totalEquity = Financial::Money{equity}.toDouble();
    s.maxEquity = Financial::Money{maxEquity}.toDouble();
    s.totalFeesPaid = Financial::Money{fees}.toDouble();
    s.tradeCount = tradeCount; s.roundTripCount = roundTripCount;
    std::size_t count = 0;
    if (!r.count(count)) { return false; }
    s.positions.resize(count);
    for (auto& p : s.positions) {
        std::int64_t quantity = 0, price = 0, entryFee = 0, costBasis = 0, mark = 0;
        if (!r.string(p.position.symbol) || !r.i64(quantity) || !r.i64(price)
            || !r.boolean(p.position.isLong) || !r.u64(p.entryTimestamp)
            || !r.i64(entryFee) || !r.i64(costBasis) || !r.i64(mark)
            || !r.string(p.strategyId)) { return false; }
        p.position.quantity = Financial::Quantity{quantity}.toDouble();
        p.position.entryPrice = Financial::Price{price}.toDouble();
        p.entryFee = Financial::Money{entryFee}.toDouble();
        p.costBasis = Financial::Money{costBasis}.toDouble();
        p.lastMarkPrice = Financial::Price{mark}.toDouble();
    }
    if (!r.count(count)) { return false; }
    s.tradeLog.resize(count);
    for (auto& t : s.tradeLog) {
        std::int64_t entry = 0, exit = 0, quantity = 0;
        std::int64_t totalFees = 0, realized = 0, gross = 0;
        if (!r.string(t.symbol) || !r.u64(t.openTimestamp) || !r.u64(t.closeTimestamp)
            || !r.i64(entry) || !r.i64(exit) || !r.i64(quantity)
            || !r.i64(totalFees) || !r.i64(realized)
            || !r.i64(gross) || !r.string(t.strategy_id)) { return false; }
        t.entryPrice = Financial::Price{entry}.toDouble();
        t.exitPrice = Financial::Price{exit}.toDouble();
        t.quantity = Financial::Quantity{quantity}.toDouble();
        t.totalFees = Financial::Money{totalFees}.toDouble();
        t.realizedPnL = Financial::Money{realized}.toDouble();
        t.grossPnL = Financial::Money{gross}.toDouble();
    }
    if (!r.count(count)) { return false; }
    for (std::size_t i = 0; i < count; ++i) {
        OrderRecord o; std::uint8_t type = 0;
        std::int64_t limit = 0, offset = 0, best = 0, quantity = 0, capital = 0;
        if (!r.string(o.symbol) || !r.u8(type) || type > static_cast<std::uint8_t>(OrderType::TRAILING_STOP)
            || !r.boolean(o.isBuy) || !r.i64(limit) || !r.i64(offset)
            || !r.i64(best) || !r.i64(quantity) || !r.i64(capital)
            || !r.u64(o.placedTimestamp)
            || !r.u64(o.orderId)) { return false; }
        o.orderType = static_cast<OrderType>(type);
        o.limitPrice = Financial::Price{limit}.toDouble();
        o.trailOffset = Financial::Price{offset}.toDouble();
        o.trailBest = Financial::Price{best}.toDouble();
        o.quantity = Financial::Quantity{quantity}.toDouble();
        o.capitalToCommit = Financial::Money{capital}.toDouble();
        s.pendingOrders.push_back(std::move(o));
    }
    return r.u64(s.nextOrderId);
}

template <typename Map> auto sortedKeys(const Map& values) {
    std::vector<std::string> keys;
    keys.reserve(values.size());
    for (const auto& [key, unused] : values) { (void)unused; keys.push_back(key); }
    std::sort(keys.begin(), keys.end());
    return keys;
}

void writeRisk(Writer& w, const RiskEngine::Snapshot& s) {
    w.u64(s.configuredMaxPositions); w.number(s.configuredVarLimit);
    w.u64(s.configuredVarWindow);
    w.number(s.totalDrawdown); w.number(s.dailyDrawdown); w.number(s.prevEquity);
    w.boolean(s.prevEquityValid); writeDoubles(w, s.returnWindow);
    w.number(s.currentVaR95); w.number(s.returnStdDev);
    const auto assetKeys = sortedKeys(s.assetStates);
    w.u64(assetKeys.size());
    for (const auto& key : assetKeys) {
        const auto& a = s.assetStates.at(key);
        w.string(key); writeDoubles(w, a.returnWindow); w.number(a.prevPrice);
        w.boolean(a.prevPriceValid); w.number(a.positionValue);
    }
    writeDoubles(w, s.covarianceMatrix);
    w.u64(s.assetOrder.size()); for (const auto& symbol : s.assetOrder) { w.string(symbol); }
    w.boolean(s.multiAssetMode); w.boolean(s.closeOnly); w.u32(s.lastLatencyMs);
    w.boolean(s.halted);
    const auto positionKeys = sortedKeys(s.syncedPositions);
    w.u64(positionKeys.size());
    for (const auto& key : positionKeys) { w.string(key); w.number(s.syncedPositions.at(key)); }
    w.u64(s.effectiveMaxPositions); w.number(s.effectiveVarLimit);
    w.boolean(s.volatilityScaled);
}

bool readRisk(Reader& r, RiskEngine::Snapshot& s) {
    std::uint64_t configuredMax = 0, configuredWindow = 0;
    if (!r.u64(configuredMax) || configuredMax > std::numeric_limits<std::size_t>::max()
        || !r.number(s.configuredVarLimit) || !r.u64(configuredWindow)
        || configuredWindow > std::numeric_limits<std::size_t>::max()
        || !r.number(s.totalDrawdown) || !r.number(s.dailyDrawdown)
        || !r.number(s.prevEquity) || !r.boolean(s.prevEquityValid)
        || !readDoubles(r, s.returnWindow) || !r.number(s.currentVaR95)
        || !r.number(s.returnStdDev)) { return false; }
    s.configuredMaxPositions = static_cast<std::size_t>(configuredMax);
    s.configuredVarWindow = static_cast<std::size_t>(configuredWindow);
    std::size_t count = 0;
    if (!r.count(count)) { return false; }
    for (std::size_t i = 0; i < count; ++i) {
        std::string key; RiskEngine::AssetReturnState a;
        if (!r.string(key) || key.empty() || !readDoubles(r, a.returnWindow)
            || !r.number(a.prevPrice) || !r.boolean(a.prevPriceValid)
            || !r.number(a.positionValue)) { return false; }
        a.symbol = key;
        if (!s.assetStates.emplace(key, std::move(a)).second) { return false; }
    }
    if (!readDoubles(r, s.covarianceMatrix) || !r.count(count)) { return false; }
    s.assetOrder.resize(count);
    for (auto& symbol : s.assetOrder) { if (!r.string(symbol)) { return false; } }
    if (!r.boolean(s.multiAssetMode) || !r.boolean(s.closeOnly)
        || !r.u32(s.lastLatencyMs) || !r.boolean(s.halted) || !r.count(count)) { return false; }
    for (std::size_t i = 0; i < count; ++i) {
        std::string symbol; double quantity = 0;
        if (!r.string(symbol) || symbol.empty() || !r.number(quantity)
            || !s.syncedPositions.emplace(symbol, quantity).second) { return false; }
    }
    std::uint64_t effective = 0;
    if (!r.u64(effective) || effective > std::numeric_limits<std::size_t>::max()
        || !r.number(s.effectiveVarLimit) || !r.boolean(s.volatilityScaled)) { return false; }
    s.effectiveMaxPositions = static_cast<std::size_t>(effective);
    return true;
}

void writeRegime(Writer& w, const RegimeDetector::State& s) {
    w.number(s.smoothTR); w.number(s.smoothDMPlus); w.number(s.smoothDMMinus);
    w.number(s.adx); w.i32(s.adxObservations); writeDoubles(w, s.returnWindow);
    w.number(s.prevClose); w.boolean(s.prevCloseValid); w.number(s.variance);
    w.number(s.prevHigh); w.number(s.prevLow); w.boolean(s.prevHLValid);
    w.u8(static_cast<std::uint8_t>(s.regime));
}
bool readRegime(Reader& r, RegimeDetector::State& s) {
    std::int32_t observations = 0; std::uint8_t regime = 0;
    if (!r.number(s.smoothTR) || !r.number(s.smoothDMPlus) || !r.number(s.smoothDMMinus)
        || !r.number(s.adx) || !r.i32(observations) || !readDoubles(r, s.returnWindow)
        || !r.number(s.prevClose) || !r.boolean(s.prevCloseValid) || !r.number(s.variance)
        || !r.number(s.prevHigh) || !r.number(s.prevLow) || !r.boolean(s.prevHLValid)
        || !r.u8(regime) || regime > static_cast<std::uint8_t>(MarketRegime::CHAOS)) { return false; }
    s.adxObservations = observations; s.regime = static_cast<MarketRegime>(regime);
    return true;
}

void writePhi(Writer& w, const std::unordered_map<std::string, PortfolioAllocator::PhiState>& states) {
    const auto keys = sortedKeys(states); w.u64(keys.size());
    for (const auto& key : keys) {
        const auto& state = states.at(key);
        w.string(key); writeDoubles(w, state.pnlWindow); w.number(state.phi);
    }
}
bool readPhi(Reader& r, std::unordered_map<std::string, PortfolioAllocator::PhiState>& states) {
    std::size_t count = 0; if (!r.count(count)) { return false; }
    for (std::size_t i = 0; i < count; ++i) {
        std::string key; PortfolioAllocator::PhiState state;
        if (!r.string(key) || key.empty() || !readDoubles(r, state.pnlWindow)
            || !r.number(state.phi)) { return false; }
        state.strategyId = key;
        if (!states.emplace(key, std::move(state)).second) { return false; }
    }
    return true;
}

std::uint64_t checksum(const std::vector<std::uint8_t>& bytes) {
    std::uint64_t value = 1469598103934665603ULL;
    for (std::uint8_t byte : bytes) { value ^= byte; value *= 1099511628211ULL; }
    return value;
}
std::string hex64(std::uint64_t value) {
    std::ostringstream out; out << std::hex << std::setfill('0') << std::setw(16) << value;
    return out.str();
}
std::string hexEncode(const std::vector<std::uint8_t>& bytes) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string out; out.resize(bytes.size() * 2);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        out[i * 2] = digits[bytes[i] >> 4]; out[i * 2 + 1] = digits[bytes[i] & 0xf];
    }
    return out;
}
int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
bool hexDecode(const std::string& text, std::vector<std::uint8_t>& bytes) {
    if ((text.size() % 2) != 0 || text.size() / 2 > MAX_PAYLOAD) { return false; }
    bytes.resize(text.size() / 2);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        const int hi = hexDigit(text[i * 2]), lo = hexDigit(text[i * 2 + 1]);
        if (hi < 0 || lo < 0) { return false; }
        bytes[i] = static_cast<std::uint8_t>((hi << 4) | lo);
    }
    return true;
}

bool jsonString(const std::string& json, const std::string& key, std::string& value) {
    const std::string marker = "\"" + key + "\"";
    const auto keyPos = json.find(marker);
    if (keyPos == std::string::npos || json.find(marker, keyPos + marker.size()) != std::string::npos) return false;
    auto pos = json.find(':', keyPos + marker.size());
    if (pos == std::string::npos) return false;
    pos = json.find('"', pos + 1); if (pos == std::string::npos) return false;
    const auto end = json.find('"', pos + 1); if (end == std::string::npos) return false;
    value = json.substr(pos + 1, end - pos - 1); return true;
}
bool jsonVersion(const std::string& json, std::uint64_t& version) {
    const std::string marker = "\"version\""; const auto keyPos = json.find(marker);
    if (keyPos == std::string::npos || json.find(marker, keyPos + marker.size()) != std::string::npos) return false;
    auto pos = json.find(':', keyPos + marker.size()); if (pos == std::string::npos) return false;
    ++pos; while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
    const auto start = pos; while (pos < json.size() && std::isdigit(static_cast<unsigned char>(json[pos]))) ++pos;
    if (start == pos) return false;
    try { version = std::stoull(json.substr(start, pos - start)); } catch (...) { return false; }
    return true;
}

bool validate(const PortfolioManager::Snapshot& p, const RiskEngine::Snapshot& r,
              std::string& error) {
    const auto finite = [](double value) { return std::isfinite(value); };
    const auto finiteRange = [&](const auto& values) {
        return std::all_of(values.begin(), values.end(), finite);
    };
    const auto alignedMoney = [](double value) {
        return Financial::money(value, Financial::Rounding::RejectUnaligned).has_value();
    };
    const auto alignedPrice = [](double value) {
        return Financial::price(value, Financial::Rounding::RejectUnaligned).has_value();
    };
    const auto alignedQuantity = [](double value) {
        return Financial::quantity(value, Financial::Rounding::RejectUnaligned).has_value();
    };
    if (p.cash < 0 || p.totalEquity < 0 || p.maxEquity < 0 || p.totalFeesPaid < 0
        || p.tradeCount < 0 || p.roundTripCount < 0 || p.nextOrderId == 0
        || !finite(p.cash) || !finite(p.unrealizedPnL) || !finite(p.totalEquity)
        || !finite(p.maxEquity) || !finite(p.currentDrawdown)
        || !finite(p.maxDrawdown) || !finite(p.totalFeesPaid)
        || !alignedMoney(p.cash) || !alignedMoney(p.unrealizedPnL)
        || !alignedMoney(p.totalEquity) || !alignedMoney(p.maxEquity)
        || !alignedMoney(p.totalFeesPaid)) {
        error = "invalid portfolio accounting state"; return false;
    }
    std::set<std::string> symbols;
    for (const auto& position : p.positions) {
        if (position.position.symbol.empty() || position.position.quantity <= 0
            || position.position.entryPrice <= 0 || !position.position.isLong
            || !finite(position.position.quantity) || !finite(position.position.entryPrice)
            || !finite(position.entryFee) || position.entryFee < 0
            || !finite(position.costBasis) || position.costBasis <= 0
            || !finite(position.lastMarkPrice) || position.lastMarkPrice <= 0
            || !alignedQuantity(position.position.quantity)
            || !alignedPrice(position.position.entryPrice)
            || !alignedMoney(position.entryFee)
            || !alignedMoney(position.costBasis)
            || !alignedPrice(position.lastMarkPrice)
            || !symbols.insert(position.position.symbol).second) {
            error = "invalid or duplicate open position"; return false;
        }
    }
    std::uint64_t maxOrderId = 0;
    std::set<std::uint64_t> orderIds;
    for (const auto& order : p.pendingOrders) {
        if (order.symbol.empty() || order.orderId == 0
            || !finite(order.limitPrice) || !finite(order.trailOffset)
            || !finite(order.trailBest) || !finite(order.quantity)
            || !finite(order.capitalToCommit)
            || !alignedPrice(order.limitPrice) || !alignedPrice(order.trailOffset)
            || !alignedPrice(order.trailBest) || !alignedQuantity(order.quantity)
            || !alignedMoney(order.capitalToCommit)
            || !orderIds.insert(order.orderId).second) {
            error = "invalid or duplicate pending order"; return false;
        }
        maxOrderId = std::max(maxOrderId, order.orderId);
    }
    if (p.nextOrderId <= maxOrderId) { error = "pending-order identity would be reused"; return false; }
    for (const auto& trade : p.tradeLog) {
        if (trade.symbol.empty() || !finite(trade.entryPrice) || !finite(trade.exitPrice)
            || !finite(trade.quantity) || !finite(trade.totalFees)
            || !finite(trade.realizedPnL) || !finite(trade.grossPnL)
            || trade.entryPrice <= 0 || trade.exitPrice <= 0
            || trade.quantity <= 0 || trade.totalFees < 0
            || !alignedPrice(trade.entryPrice) || !alignedPrice(trade.exitPrice)
            || !alignedQuantity(trade.quantity) || !alignedMoney(trade.totalFees)
            || !alignedMoney(trade.realizedPnL) || !alignedMoney(trade.grossPnL)) {
            error = "invalid trade-log state"; return false;
        }
    }
    try {
        PortfolioManager detached;
        detached.restoreState(p);
    } catch (const std::exception& exception) {
        error = std::string("inconsistent portfolio accounting state: ")
              + exception.what();
        return false;
    }
    if (!finite(r.configuredVarLimit) || !finite(r.totalDrawdown)
        || !finite(r.dailyDrawdown) || !finite(r.prevEquity)
        || !finiteRange(r.returnWindow) || !finite(r.currentVaR95)
        || !finite(r.returnStdDev) || !finiteRange(r.covarianceMatrix)
        || !finite(r.effectiveVarLimit)) {
        error = "invalid risk state"; return false;
    }
    for (const auto& [symbol, asset] : r.assetStates) {
        if (symbol.empty() || !finiteRange(asset.returnWindow)
            || !finite(asset.prevPrice) || !finite(asset.positionValue)) {
            error = "invalid asset-risk state"; return false;
        }
    }
    for (const auto& [symbol, quantity] : r.syncedPositions) {
        if (symbol.empty() || !finite(quantity)) {
            error = "invalid synchronized position"; return false;
        }
    }
    const auto n = r.assetOrder.size();
    if (!r.covarianceMatrix.empty() && r.covarianceMatrix.size() != n * n) {
        error = "invalid covariance dimensions"; return false;
    }
    for (const auto& symbol : r.assetOrder) {
        if (r.assetStates.find(symbol) == r.assetStates.end()) {
            error = "covariance symbol lacks return state"; return false;
        }
    }
    return true;
}

bool validateExtended(const RegimeDetector::State& regime,
                      const std::unordered_map<std::string, PortfolioAllocator::PhiState>& phi,
                      std::string& error) {
    const auto finite = [](double value) { return std::isfinite(value); };
    if (!finite(regime.smoothTR) || !finite(regime.smoothDMPlus)
        || !finite(regime.smoothDMMinus) || !finite(regime.adx)
        || !finite(regime.prevClose) || !finite(regime.variance)
        || !finite(regime.prevHigh) || !finite(regime.prevLow)
        || !std::all_of(regime.returnWindow.begin(), regime.returnWindow.end(), finite)) {
        error = "invalid regime state"; return false;
    }
    for (const auto& [key, state] : phi) {
        if (key.empty() || !finite(state.phi)
            || !std::all_of(state.pnlWindow.begin(), state.pnlWindow.end(), finite)) {
            error = "invalid allocator state"; return false;
        }
    }
    return true;
}

bool atomicWrite(const std::string& filepath, const std::string& contents, std::string& error) {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path target(filepath);
    if (!target.parent_path().empty()) {
        fs::create_directories(target.parent_path(), ec);
        if (ec) { error = "cannot create snapshot directory: " + ec.message(); return false; }
    }
    static std::atomic<std::uint64_t> sequence{0};
    fs::path temporary = target;
    temporary += ".tmp." + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
               + "." + std::to_string(sequence.fetch_add(1));
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out) { error = "cannot open snapshot temporary file"; return false; }
        out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        out.flush();
        if (!out) { error = "cannot write complete snapshot"; out.close(); fs::remove(temporary, ec); return false; }
    }
    fs::rename(temporary, target, ec);
    if (ec) { error = "cannot atomically replace snapshot: " + ec.message(); fs::remove(temporary, ec); return false; }
    return true;
}

bool saveImpl(const PortfolioManager& portfolio, const RiskEngine& risk,
              const RegimeDetector* regime, const PortfolioAllocator* allocator,
              std::uint64_t checkpointTs, const std::string& filepath,
              std::string& error) {
    error.clear();
    if (risk.currentErrorRate() != 0) {
        error = "transient API-error window is not restart-compatible";
        return false;
    }
    const auto portfolioState = portfolio.snapshotState();
    const auto riskState = risk.snapshotState();
    if (!validate(portfolioState, riskState, error)) { return false; }
    RegimeDetector::State regimeState;
    std::unordered_map<std::string, PortfolioAllocator::PhiState> phiStates;
    if (regime && allocator) {
        regimeState = regime->getState();
        phiStates = allocator->getPhiStates();
        if (!validateExtended(regimeState, phiStates, error)) { return false; }
    }
    Writer writer;
    writer.string("TradeBotState"); writer.boolean(regime && allocator); writer.u64(checkpointTs);
    writePortfolio(writer, portfolioState); writeRisk(writer, riskState);
    if (regime && allocator) { writeRegime(writer, regimeState); writePhi(writer, phiStates); }
    const auto& payload = writer.bytes();
    std::ostringstream json;
    json << "{\n  \"schema\": \"tradebot.backtest-state\",\n"
         << "  \"version\": " << StateSerializer::SNAPSHOT_VERSION << ",\n"
         << "  \"payloadEncoding\": \"hex-v1\",\n"
         << "  \"payloadChecksum\": \"" << hex64(checksum(payload)) << "\",\n"
         << "  \"payload\": \"" << hexEncode(payload) << "\"\n}\n";
    return atomicWrite(filepath, json.str(), error);
}

bool loadImpl(PortfolioManager& portfolio, RiskEngine& risk,
              RegimeDetector* regime, PortfolioAllocator* allocator,
              std::uint64_t& checkpointTs, const std::string& filepath,
              std::string& error) {
    error.clear(); std::ifstream in(filepath, std::ios::binary);
    if (!in) { error = "cannot open snapshot"; return false; }
    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (json.size() > MAX_PAYLOAD * 2 + 4096) { error = "snapshot exceeds size limit"; return false; }
    std::string schema, encoding, expectedChecksum, encoded;
    std::uint64_t version = 0;
    if (!jsonString(json, "schema", schema) || !jsonVersion(json, version)
        || !jsonString(json, "payloadEncoding", encoding)
        || !jsonString(json, "payloadChecksum", expectedChecksum)
        || !jsonString(json, "payload", encoded)) { error = "malformed snapshot envelope"; return false; }
    std::ostringstream canonical;
    canonical << "{\n  \"schema\": \"" << schema << "\",\n"
              << "  \"version\": " << version << ",\n"
              << "  \"payloadEncoding\": \"" << encoding << "\",\n"
              << "  \"payloadChecksum\": \"" << expectedChecksum << "\",\n"
              << "  \"payload\": \"" << encoded << "\"\n}\n";
    if (json != canonical.str()) { error = "snapshot envelope is not canonical"; return false; }
    if (schema != "tradebot.backtest-state") { error = "unsupported snapshot schema"; return false; }
    if (version != StateSerializer::SNAPSHOT_VERSION) {
        error = "unsupported snapshot version " + std::to_string(version) + "; migration required"; return false;
    }
    if (encoding != "hex-v1") { error = "unsupported payload encoding"; return false; }
    std::vector<std::uint8_t> bytes;
    if (!hexDecode(encoded, bytes) || hex64(checksum(bytes)) != expectedChecksum) {
        error = "snapshot payload is corrupt"; return false;
    }
    Reader reader(bytes); std::string magic; bool hasExtended = false;
    PortfolioManager::Snapshot portfolioState; RiskEngine::Snapshot riskState;
    RegimeDetector::State regimeState;
    std::unordered_map<std::string, PortfolioAllocator::PhiState> phiStates;
    std::uint64_t restoredTs = 0;
    if (!reader.string(magic) || magic != "TradeBotState" || !reader.boolean(hasExtended)
        || !reader.u64(restoredTs) || !readPortfolio(reader, portfolioState)
        || !readRisk(reader, riskState)) { error = reader.error().empty() ? "invalid snapshot payload" : reader.error(); return false; }
    if (hasExtended && (!readRegime(reader, regimeState) || !readPhi(reader, phiStates))) {
        error = reader.error().empty() ? "invalid extended snapshot state" : reader.error(); return false;
    }
    if (!reader.done()) { error = "snapshot payload has trailing data"; return false; }
    if ((regime != nullptr || allocator != nullptr) && (!regime || !allocator || !hasExtended)) {
        error = "snapshot lacks required regime/allocation state"; return false;
    }
    if (!validate(portfolioState, riskState, error)
        || !risk.canRestoreSnapshot(riskState, error)) { return false; }
    if (hasExtended && !validateExtended(regimeState, phiStates, error)) { return false; }

    // Commit only after every field and cross-field invariant has validated.
    portfolio.restoreState(portfolioState); risk.restoreState(riskState);
    if (regime && allocator) { regime->restoreState(regimeState); allocator->restorePhiStates(phiStates); }
    checkpointTs = restoredTs;
    return true;
}
} // namespace

bool StateSerializer::saveSnapshot(const PortfolioManager& p, const RiskEngine& r,
                                   const RegimeDetector& d, const PortfolioAllocator& a,
                                   uint64_t ts, const std::string& path) const {
    return saveImpl(p, r, &d, &a, ts, path, m_lastError);
}
bool StateSerializer::saveSnapshot(const PortfolioManager& p, const RiskEngine& r,
                                   uint64_t ts, const std::string& path) const {
    return saveImpl(p, r, nullptr, nullptr, ts, path, m_lastError);
}
bool StateSerializer::loadSnapshot(PortfolioManager& p, RiskEngine& r,
                                   RegimeDetector& d, PortfolioAllocator& a,
                                   uint64_t& ts, const std::string& path) const {
    return loadImpl(p, r, &d, &a, ts, path, m_lastError);
}
bool StateSerializer::loadSnapshot(PortfolioManager& p, RiskEngine& r,
                                   uint64_t& ts, const std::string& path) const {
    return loadImpl(p, r, nullptr, nullptr, ts, path, m_lastError);
}
