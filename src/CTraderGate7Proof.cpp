#include "CTraderGate7Proof.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <limits>
#include <utility>

namespace tradebot::ctrader {
namespace {

void secureClear(std::string& value) noexcept
{
    volatile char* bytes = value.empty() ? nullptr : value.data();
    for (std::size_t i = 0; i < value.size(); ++i) {
        bytes[i] = 0;
    }
    value.clear();
}

void secureClear(std::int64_t& value) noexcept
{
    volatile std::int64_t* target = &value;
    *target = 0;
}

void secureClear(std::uint64_t& value) noexcept
{
    volatile std::uint64_t* target = &value;
    *target = 0;
}

void clearInstrument(InstrumentSpec& instrument) noexcept
{
    secureClear(instrument.canonicalSymbol);
    secureClear(instrument.executionAlias);
    instrument.tickSize = {};
    instrument.contractSize = {};
    instrument.minimumQuantity = {};
    instrument.maximumQuantity = {};
    instrument.quantityStep = {};
    instrument.effectiveTimestampNs = 0;
    instrument.version = 0;
    instrument.complete = false;
}

bool isUsableAccountId(std::uint64_t value) noexcept
{
    return value > 0
        && value <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
}

bool isUsableProviderId(std::int64_t value) noexcept
{
    return value > 0;
}

bool isSafeText(std::string_view value) noexcept
{
    if (value.empty() || value.size() > 256) {
        return false;
    }
    for (const unsigned char c : value) {
        if (c < 0x20 || c == 0x7f) {
            return false;
        }
    }
    return true;
}

bool containsId(const std::vector<std::int64_t>& values,
                std::int64_t id) noexcept
{
    return std::find(values.begin(), values.end(), id) != values.end();
}

void clearOptionalString(std::optional<std::string>& value) noexcept
{
    if (value.has_value()) {
        secureClear(*value);
        value.reset();
    }
}

void clearOptionalInt(std::optional<std::int64_t>& value) noexcept
{
    if (value.has_value()) {
        secureClear(*value);
        value.reset();
    }
}

} // namespace

Gate7AccountRecord::~Gate7AccountRecord()
{
    if (accountId.has_value()) secureClear(*accountId);
    if (brokerTitleShort.has_value()) secureClear(*brokerTitleShort);
}

Gate7LightSymbol::~Gate7LightSymbol()
{
    if (symbolId.has_value()) secureClear(*symbolId);
    clearOptionalString(symbolName);
}

Gate7FullSymbol::~Gate7FullSymbol()
{
    clearOptionalInt(symbolId);
    clearOptionalString(symbolName);
    if (digits.has_value()) digits.reset();
    if (pipPosition.has_value()) pipPosition.reset();
    clearOptionalInt(minVolume);
    clearOptionalInt(maxVolume);
    clearOptionalInt(stepVolume);
    clearOptionalInt(lotSize);
}

bool CTraderGate7Config::isAllowedOpenApiEndpoint(std::string_view host,
                                                   std::uint16_t port) noexcept
{
    return host == DEMO_HOST && port == DEMO_PORT;
}

bool CTraderGate7Config::isAllowedOutboundPayload(std::uint32_t payloadType) noexcept
{
    switch (payloadType) {
    case 51:   // HEARTBEAT_EVENT
    case 2100: // PROTO_OA_APPLICATION_AUTH_REQ
    case 2102: // PROTO_OA_ACCOUNT_AUTH_REQ
    case 2114: // PROTO_OA_SYMBOLS_LIST_REQ
    case 2116: // PROTO_OA_SYMBOL_BY_ID_REQ
    case 2127: // PROTO_OA_SUBSCRIBE_SPOTS_REQ
    case 2129: // PROTO_OA_UNSUBSCRIBE_SPOTS_REQ
    case 2149: // PROTO_OA_GET_ACCOUNTS_BY_ACCESS_TOKEN_REQ
        return true;
    default:
        return false;
    }
}

bool CTraderGate7Config::isAllowedInboundPayload(std::uint32_t payloadType) noexcept
{
    switch (payloadType) {
    case 50:   // ERROR_RES
    case 51:   // HEARTBEAT_EVENT
    case 2101: // PROTO_OA_APPLICATION_AUTH_RES
    case 2103: // PROTO_OA_ACCOUNT_AUTH_RES
    case 2115: // PROTO_OA_SYMBOLS_LIST_RES
    case 2117: // PROTO_OA_SYMBOL_BY_ID_RES
    case 2128: // PROTO_OA_SUBSCRIBE_SPOTS_RES
    case 2130: // PROTO_OA_UNSUBSCRIBE_SPOTS_RES
    case 2131: // PROTO_OA_SPOT_EVENT
    case 2142: // PROTO_OA_ERROR_RES
    case 2147: // PROTO_OA_ACCOUNTS_TOKEN_INVALIDATED_EVENT
    case 2148: // PROTO_OA_CLIENT_DISCONNECT_EVENT
    case 2150: // PROTO_OA_GET_ACCOUNTS_BY_ACCESS_TOKEN_RES
        return true;
    default:
        return false;
    }
}

CTraderGate7Proof::CTraderGate7Proof(std::uint64_t connectionGeneration) noexcept
    : connectionGeneration_(connectionGeneration)
{
}

CTraderGate7Proof::~CTraderGate7Proof()
{
    clearAll();
}

Gate7Decision CTraderGate7Proof::validateTransportEvidence(
    std::uint64_t generation,
    bool currentGeneration,
    bool correlationMatched) const noexcept
{
    if (!currentGeneration || generation == 0
        || generation != connectionGeneration_) {
        return Gate7Decision::StaleConnectionGeneration;
    }
    if (!correlationMatched) {
        return Gate7Decision::CorrelationRejected;
    }
    return Gate7Decision::Ready;
}

Gate7Decision CTraderGate7Proof::acceptAccountList(
    Gate7AccountListEvidence evidence) noexcept
{
    try {
        return acceptAccountListImpl(evidence);
    } catch (...) {
        return finish(Gate7Decision::ResourceExhausted);
    }
}

Gate7Decision CTraderGate7Proof::acceptAccountListImpl(
    Gate7AccountListEvidence& evidence)
{
    if (isTerminal()) return Gate7Decision::AlreadyTerminal;
    if (phase_ != Phase::AccountList) return finish(Gate7Decision::WrongPhase);

    const Gate7Decision transport = validateTransportEvidence(
        evidence.connectionGeneration,
        evidence.currentConnectionGeneration,
        evidence.correlationMatched);
    if (transport != Gate7Decision::Ready) return finish(transport);
    if (!evidence.tokenOwned) return finish(Gate7Decision::TokenOwnershipRejected);
    if (!evidence.tradingScope) return finish(Gate7Decision::TradingScopeRequired);

    std::uint64_t match = 0;
    std::size_t matches = 0;
    for (const Gate7AccountRecord& account : evidence.accounts) {
        if (!account.accountId.has_value() || !isUsableAccountId(*account.accountId)) {
            continue;
        }
        if (!account.isLive.has_value() || *account.isLive) {
            continue;
        }
        if (!account.brokerTitleShort.has_value()
            || !isSafeText(*account.brokerTitleShort)) {
            continue;
        }
        if (*account.brokerTitleShort != "FIBO") {
            continue;
        }
        match = *account.accountId;
        ++matches;
    }

    if (matches == 0) {
        secureClear(match);
        return finish(Gate7Decision::NoFiboDemoAccount);
    }
    if (matches != 1) {
        secureClear(match);
        return finish(Gate7Decision::AmbiguousFiboDemoAccount);
    }

    accountId_ = static_cast<std::int64_t>(match);
    secureClear(match);
    phase_ = Phase::AccountAuthentication;
    lastDecision_ = Gate7Decision::AccountAuthenticationReady;
    return lastDecision_;
}

std::optional<std::int64_t> CTraderGate7Proof::accountIdForAuthentication()
    const noexcept
{
    if (phase_ != Phase::AccountAuthentication || accountId_ <= 0) {
        return std::nullopt;
    }
    return accountId_;
}

Gate7Decision CTraderGate7Proof::acceptAccountAuthentication(
    std::int64_t responseAccountId) noexcept
{
    try {
        return acceptAccountAuthenticationImpl(responseAccountId);
    } catch (...) {
        secureClear(responseAccountId);
        return finish(Gate7Decision::ResourceExhausted);
    }
}

Gate7Decision CTraderGate7Proof::acceptAccountAuthenticationImpl(
    std::int64_t responseAccountId)
{
    const bool matched = phase_ == Phase::AccountAuthentication
        && responseAccountId > 0 && responseAccountId == accountId_;
    secureClear(responseAccountId);
    if (!matched) return finish(Gate7Decision::InvalidAccountIdentifier);

    phase_ = Phase::SymbolsList;
    lastDecision_ = Gate7Decision::SymbolListReady;
    return lastDecision_;
}

bool CTraderGate7Proof::isCanonicalXauusd(std::string_view symbolName) noexcept
{
    std::array<char, 6> normalized{};
    std::size_t output = 0;
    bool slashSeen = false;
    for (const unsigned char value : symbolName) {
        if (value == '/') {
            if (slashSeen) return false;
            slashSeen = true;
            continue;
        }
        if (value >= 'a' && value <= 'z') {
            if (output >= normalized.size()) return false;
            normalized[output++] = static_cast<char>(value - ('a' - 'A'));
        } else if (value >= 'A' && value <= 'Z') {
            if (output >= normalized.size()) return false;
            normalized[output++] = static_cast<char>(value);
        } else {
            return false;
        }
    }
    return output == normalized.size()
        && std::string_view(normalized.data(), normalized.size()) == "XAUUSD";
}

Gate7Decision CTraderGate7Proof::acceptSymbolsList(
    Gate7SymbolsListEvidence evidence) noexcept
{
    try {
        return acceptSymbolsListImpl(evidence);
    } catch (...) {
        return finish(Gate7Decision::ResourceExhausted);
    }
}

Gate7Decision CTraderGate7Proof::acceptSymbolsListImpl(
    Gate7SymbolsListEvidence& evidence)
{
    if (isTerminal()) return Gate7Decision::AlreadyTerminal;
    if (phase_ != Phase::SymbolsList) return finish(Gate7Decision::WrongPhase);
    const Gate7Decision transport = validateTransportEvidence(
        evidence.connectionGeneration,
        evidence.currentConnectionGeneration,
        evidence.correlationMatched);
    if (transport != Gate7Decision::Ready) return finish(transport);
    if (evidence.accountId <= 0 || evidence.accountId != accountId_) {
        secureClear(evidence.accountId);
        return finish(Gate7Decision::InvalidAccountIdentifier);
    }
    if (evidence.includeArchivedSymbols) {
        secureClear(evidence.accountId);
        return finish(Gate7Decision::MissingSymbolMetadata);
    }

    std::int64_t match = 0;
    std::string name;
    std::size_t matches = 0;
    for (const Gate7LightSymbol& symbol : evidence.symbols) {
        if (!symbol.symbolId.has_value() || !isUsableProviderId(*symbol.symbolId)
            || !symbol.symbolName.has_value() || !symbol.enabled.has_value()
            || !*symbol.enabled || symbol.archived
            || containsId(evidence.archivedSymbolIds, *symbol.symbolId)) {
            continue;
        }
        if (!isCanonicalXauusd(*symbol.symbolName)) continue;
        match = *symbol.symbolId;
        name = *symbol.symbolName;
        ++matches;
    }
    secureClear(evidence.accountId);
    secureClear(match);
    const bool foundName = !name.empty();
    secureClear(name);
    if (matches == 0) return finish(Gate7Decision::NoCanonicalXauusd);
    if (matches != 1) return finish(Gate7Decision::AmbiguousCanonicalXauusd);

    // Recompute the single candidate without retaining any provider list.
    for (const Gate7LightSymbol& symbol : evidence.symbols) {
        if (symbol.symbolId.has_value() && isUsableProviderId(*symbol.symbolId)
            && symbol.symbolName.has_value() && symbol.enabled.has_value()
            && *symbol.enabled && !symbol.archived
            && !containsId(evidence.archivedSymbolIds, *symbol.symbolId)
            && isCanonicalXauusd(*symbol.symbolName)) {
            symbolId_ = *symbol.symbolId;
            symbolName_ = *symbol.symbolName;
            break;
        }
    }
    if (symbolId_ <= 0 || !foundName) return finish(Gate7Decision::MissingSymbolMetadata);
    phase_ = Phase::FullSymbol;
    lastDecision_ = Gate7Decision::FullSymbolReady;
    return lastDecision_;
}

std::optional<std::int64_t> CTraderGate7Proof::symbolIdForFullRequest()
    const noexcept
{
    if (phase_ != Phase::FullSymbol || symbolId_ <= 0) return std::nullopt;
    return symbolId_;
}

Gate7Decision CTraderGate7Proof::acceptFullSymbol(
    Gate7FullSymbolEvidence evidence) noexcept
{
    try {
        return acceptFullSymbolImpl(evidence);
    } catch (...) {
        return finish(Gate7Decision::ResourceExhausted);
    }
}

Gate7Decision CTraderGate7Proof::acceptFullSymbolImpl(
    Gate7FullSymbolEvidence& evidence)
{
    if (isTerminal()) return Gate7Decision::AlreadyTerminal;
    if (phase_ != Phase::FullSymbol) return finish(Gate7Decision::WrongPhase);
    const Gate7Decision transport = validateTransportEvidence(
        evidence.connectionGeneration,
        evidence.currentConnectionGeneration,
        evidence.correlationMatched);
    if (transport != Gate7Decision::Ready) return finish(transport);
    if (evidence.accountId != accountId_ || evidence.accountId <= 0) {
        secureClear(evidence.accountId);
        return finish(Gate7Decision::InvalidAccountIdentifier);
    }
    if (containsId(evidence.archivedSymbolIds, symbolId_)) {
        secureClear(evidence.accountId);
        return finish(Gate7Decision::SymbolMetadataRejected);
    }

    const Gate7FullSymbol* match = nullptr;
    std::size_t matchCount = 0;
    for (const Gate7FullSymbol& symbol : evidence.symbols) {
        if (symbol.symbolId.has_value() && *symbol.symbolId == symbolId_) {
            match = &symbol;
            ++matchCount;
        }
    }
    secureClear(evidence.accountId);
    if (match == nullptr || matchCount != 1) {
        return finish(Gate7Decision::FullSymbolMismatch);
    }
    if (match->symbolName.has_value() && *match->symbolName != symbolName_) {
        return finish(Gate7Decision::SymbolMetadataRejected);
    }
    if (match->enabled.has_value() && !*match->enabled) {
        return finish(Gate7Decision::SymbolMetadataRejected);
    }
    if (match->archived.has_value() && *match->archived) {
        return finish(Gate7Decision::SymbolMetadataRejected);
    }
    if (!match->digits.has_value() || !match->pipPosition.has_value()
        || !match->minVolume.has_value() || !match->maxVolume.has_value()
        || !match->stepVolume.has_value() || !match->lotSize.has_value()) {
        return finish(Gate7Decision::MissingSymbolMetadata);
    }

    const std::int32_t digits = *match->digits;
    const std::int32_t pipPosition = *match->pipPosition;
    const std::int64_t minVolume = *match->minVolume;
    const std::int64_t maxVolume = *match->maxVolume;
    const std::int64_t stepVolume = *match->stepVolume;
    const std::int64_t lotSize = *match->lotSize;
    if (digits < 0 || pipPosition < 0
        || digits > Decimal64::MAX_SCALE
        || pipPosition > Decimal64::MAX_SCALE
        || pipPosition > digits || minVolume <= 0 || maxVolume <= 0
        || stepVolume <= 0 || lotSize <= 0 || minVolume > maxVolume
        || minVolume % stepVolume != 0 || maxVolume % stepVolume != 0) {
        return finish(Gate7Decision::SymbolMetadataRejected);
    }

    InstrumentSpec spec;
    spec.version = 1;
    spec.canonicalSymbol = "XAUUSD";
    spec.executionAlias = symbolName_;
    spec.tickSize = Decimal64{1, static_cast<std::uint8_t>(digits)};
    spec.contractSize = Decimal64{lotSize, 2};
    spec.minimumQuantity = Decimal64{minVolume, 2};
    spec.maximumQuantity = Decimal64{maxVolume, 2};
    spec.quantityStep = Decimal64{stepVolume, 2};
    spec.complete = true;
    instrument_ = std::move(spec);
    pipPosition_ = pipPosition;
    phase_ = Phase::Subscription;
    lastDecision_ = Gate7Decision::SubscriptionReady;
    return lastDecision_;
}

std::optional<std::int64_t> CTraderGate7Proof::symbolIdForSubscription()
    const noexcept
{
    if (phase_ != Phase::Subscription || symbolId_ <= 0) return std::nullopt;
    return symbolId_;
}

Gate7Decision CTraderGate7Proof::acceptSubscription(
    Gate7SubscriptionEvidence evidence) noexcept
{
    try {
        return acceptSubscriptionImpl(evidence);
    } catch (...) {
        return finish(Gate7Decision::ResourceExhausted);
    }
}

Gate7Decision CTraderGate7Proof::acceptSubscriptionImpl(
    Gate7SubscriptionEvidence& evidence)
{
    if (isTerminal()) return Gate7Decision::AlreadyTerminal;
    if (phase_ != Phase::Subscription) return finish(Gate7Decision::WrongPhase);
    const Gate7Decision transport = validateTransportEvidence(
        evidence.connectionGeneration,
        evidence.currentConnectionGeneration,
        evidence.correlationMatched);
    if (transport != Gate7Decision::Ready) return finish(transport);
    const bool matched = evidence.accountId == accountId_
        && evidence.accountId > 0 && evidence.symbolId == symbolId_
        && evidence.symbolId > 0 && evidence.requestedSymbolCount == 1;
    secureClear(evidence.accountId);
    secureClear(evidence.symbolId);
    if (!matched) return finish(Gate7Decision::SubscriptionMismatch);
    subscriptionReady_ = true;
    phase_ = Phase::Spot;
    lastDecision_ = Gate7Decision::SubscriptionReady;
    return lastDecision_;
}

std::optional<std::int64_t> CTraderGate7Proof::normalizeSignedScale5(
    std::int64_t units, std::int32_t digits) noexcept
{
    if (digits < 0 || digits > Decimal64::MAX_SCALE) return std::nullopt;
    if (digits == 5) return units;

    if (digits > 5) {
        std::int64_t factor = 1;
        for (std::int32_t i = 5; i < digits; ++i) {
            if (factor > std::numeric_limits<std::int64_t>::max() / 10) {
                return std::nullopt;
            }
            factor *= 10;
        }
        if (units > 0 && units > std::numeric_limits<std::int64_t>::max() / factor) {
            return std::nullopt;
        }
        if (units < 0 && units < std::numeric_limits<std::int64_t>::min() / factor) {
            return std::nullopt;
        }
        return units * factor;
    }

    std::int64_t factor = 1;
    for (std::int32_t i = digits; i < 5; ++i) factor *= 10;
    const std::uint64_t magnitude = units < 0
        ? static_cast<std::uint64_t>(-(units + 1)) + 1
        : static_cast<std::uint64_t>(units);
    std::uint64_t quotient = magnitude / static_cast<std::uint64_t>(factor);
    const std::uint64_t remainder = magnitude
        % static_cast<std::uint64_t>(factor);
    if (remainder * 2 >= static_cast<std::uint64_t>(factor)) {
        if (quotient == static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max())) {
            return std::nullopt;
        }
        ++quotient;
    }
    if (quotient > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max())) {
        return std::nullopt;
    }
    const std::int64_t rounded = static_cast<std::int64_t>(quotient);
    return units < 0 ? -rounded : rounded;
}

std::optional<Decimal64> CTraderGate7Proof::normalizeSpotPrice(
    std::uint64_t rawWire, std::int32_t digits) noexcept
{
    if (rawWire == 0 || rawWire > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max())) {
        return std::nullopt;
    }
    const auto normalized = normalizeSignedScale5(
        static_cast<std::int64_t>(rawWire), digits);
    if (!normalized.has_value()) return std::nullopt;
    return Decimal64{*normalized, static_cast<std::uint8_t>(digits)};
}

std::optional<Gate7TimestampProof> CTraderGate7Proof::classifyTimestamp(
    std::uint64_t rawTimestamp, std::uint64_t receiptTimestampNs) noexcept
{
    constexpr std::array<std::uint64_t, 4> multipliers = {
        1000000000ULL, 1000000ULL, 1000ULL, 1ULL
    };
    constexpr std::array<Gate7TimestampUnit, 4> units = {
        Gate7TimestampUnit::Seconds,
        Gate7TimestampUnit::Milliseconds,
        Gate7TimestampUnit::Microseconds,
        Gate7TimestampUnit::Nanoseconds
    };
    constexpr std::int64_t MAX_AGE_NS = 120LL * 1000000000LL;
    constexpr std::int64_t MAX_FUTURE_NS = 5LL * 1000000000LL;

    std::optional<Gate7TimestampProof> result;
    for (std::size_t i = 0; i < multipliers.size(); ++i) {
        if (rawTimestamp > std::numeric_limits<std::uint64_t>::max()
                / multipliers[i]) {
            continue;
        }
        const std::uint64_t timestampNs = rawTimestamp * multipliers[i];
        std::int64_t delta = 0;
        if (timestampNs <= receiptTimestampNs) {
            const std::uint64_t age = receiptTimestampNs - timestampNs;
            if (age > static_cast<std::uint64_t>(MAX_AGE_NS)) continue;
            delta = static_cast<std::int64_t>(age);
        } else {
            const std::uint64_t future = timestampNs - receiptTimestampNs;
            if (future > static_cast<std::uint64_t>(MAX_FUTURE_NS)) continue;
            delta = -static_cast<std::int64_t>(future);
        }
        if (result.has_value()) return std::nullopt;
        result = Gate7TimestampProof{
            units[i], timestampNs, receiptTimestampNs,
            delta
        };
    }
    return result;
}

Gate7Decision CTraderGate7Proof::acceptSpot(Gate7SpotEvidence evidence) noexcept
{
    try {
        return acceptSpotImpl(evidence);
    } catch (...) {
        return finish(Gate7Decision::ResourceExhausted);
    }
}

Gate7Decision CTraderGate7Proof::acceptSpotImpl(Gate7SpotEvidence& evidence)
{
    if (isTerminal()) return Gate7Decision::AlreadyTerminal;
    if (phase_ != Phase::Spot || !subscriptionReady_) {
        return finish(Gate7Decision::WrongPhase);
    }
    if (!evidence.currentConnectionGeneration
        || evidence.connectionGeneration != connectionGeneration_) {
        return finish(Gate7Decision::StaleConnectionGeneration);
    }
    if (!evidence.subscriptionMatched) {
        return finish(Gate7Decision::SubscriptionMismatch);
    }
    if (evidence.accountId != accountId_ || evidence.symbolId != symbolId_
        || evidence.accountId <= 0 || evidence.symbolId <= 0) {
        return finish(Gate7Decision::InvalidAccountIdentifier);
    }
    if (!evidence.bid.has_value() || !evidence.ask.has_value()) {
        return finish(Gate7Decision::MissingSpotSide);
    }
    const std::uint64_t rawBid = *evidence.bid;
    const std::uint64_t rawAsk = *evidence.ask;
    const std::uint64_t maxInt64 = static_cast<std::uint64_t>(
        std::numeric_limits<std::int64_t>::max());
    if (rawBid == 0 || rawAsk == 0 || rawBid > maxInt64 || rawAsk > maxInt64) {
        return finish(Gate7Decision::InvalidSpot);
    }
    if (rawBid > rawAsk) return finish(Gate7Decision::CrossedMarket);
    if (!evidence.timestamp.has_value() || *evidence.timestamp < 0) {
        return finish(Gate7Decision::TimestampUnitUnproven);
    }
    if (evidence.receiptTimestampNs == 0) {
        return finish(Gate7Decision::TimestampUnitUnproven);
    }
    if (!instrument_.has_value()) return finish(Gate7Decision::WrongPhase);
    const std::int32_t digits = instrument_->tickSize.scale;
    const auto bid = normalizeSpotPrice(rawBid, digits);
    const auto ask = normalizeSpotPrice(rawAsk, digits);
    if (!bid.has_value() || !ask.has_value()) {
        return finish(Gate7Decision::CheckedArithmeticFailed);
    }
    if (bid->units > ask->units) return finish(Gate7Decision::CrossedMarket);
    const auto timestamp = classifyTimestamp(
        static_cast<std::uint64_t>(*evidence.timestamp),
        evidence.receiptTimestampNs);
    if (!timestamp.has_value()) return finish(Gate7Decision::TimestampUnitUnproven);
    const std::int64_t rawSpread = static_cast<std::int64_t>(rawAsk)
        - static_cast<std::int64_t>(rawBid);
    const std::int64_t normalizedSpread = ask->units - bid->units;
    if (rawSpread < 0 || normalizedSpread < 0) {
        return finish(Gate7Decision::CrossedMarket);
    }

    Gate7QuoteEvidence quote;
    quote.canonicalSymbol = "XAUUSD";
    quote.executionAlias = symbolName_;
    quote.instrument = *instrument_;
    // Gate 7 retains the validated pip metadata separately because the
    // broker-neutral InstrumentSpec deliberately has no provider pip field.
    quote.pipPosition = pipPosition_;
    quote.bid = *bid;
    quote.ask = *ask;
    quote.spread = Decimal64{normalizedSpread, bid->scale};
    quote.timestamp = *timestamp;
    quoteEvidence_ = std::move(quote);
    clearSensitiveState();
    phase_ = Phase::Complete;
    lastDecision_ = Gate7Decision::QuoteProofSucceeded;
    return lastDecision_;
}

Gate7Decision CTraderGate7Proof::terminal(Gate7Decision decision) noexcept
{
    if (isTerminal()) {
        lastDecision_ = Gate7Decision::AlreadyTerminal;
        return lastDecision_;
    }
    return finish(decision);
}

Gate7Decision CTraderGate7Proof::finish(Gate7Decision decision) noexcept
{
    clearAll();
    phase_ = Phase::Terminal;
    lastDecision_ = decision;
    return lastDecision_;
}

void CTraderGate7Proof::clearSensitiveState() noexcept
{
    secureClear(accountId_);
    secureClear(symbolId_);
    pipPosition_ = 0;
    secureClear(symbolName_);
    if (instrument_.has_value()) {
        clearInstrument(*instrument_);
        instrument_.reset();
    }
    subscriptionReady_ = false;
}

void CTraderGate7Proof::clearAll() noexcept
{
    clearSensitiveState();
    if (quoteEvidence_.has_value()) {
        secureClear(quoteEvidence_->canonicalSymbol);
        secureClear(quoteEvidence_->executionAlias);
        clearInstrument(quoteEvidence_->instrument);
        quoteEvidence_.reset();
    }
    secureClear(connectionGeneration_);
}

bool CTraderGate7Proof::isTerminal() const noexcept
{
    return phase_ == Phase::Complete || phase_ == Phase::Terminal;
}

std::string_view CTraderGate7Proof::timestampUnitName(
    Gate7TimestampUnit unit) noexcept
{
    switch (unit) {
    case Gate7TimestampUnit::Seconds: return "seconds";
    case Gate7TimestampUnit::Milliseconds: return "milliseconds";
    case Gate7TimestampUnit::Microseconds: return "microseconds";
    case Gate7TimestampUnit::Nanoseconds: return "nanoseconds";
    case Gate7TimestampUnit::Unproven: return "unproven";
    }
    return "unproven";
}

std::string_view CTraderGate7Proof::safeDiagnostic(Gate7Decision decision) noexcept
{
    switch (decision) {
    case Gate7Decision::Ready: return "gate7_ready";
    case Gate7Decision::AccountAuthenticationReady: return "gate7_account_auth_ready";
    case Gate7Decision::SymbolListReady: return "gate7_symbol_list_ready";
    case Gate7Decision::FullSymbolReady: return "gate7_full_symbol_ready";
    case Gate7Decision::SubscriptionReady: return "gate7_subscription_ready";
    case Gate7Decision::QuoteProofSucceeded: return "gate7_quote_proof_succeeded";
    case Gate7Decision::AlreadyTerminal: return "gate7_terminal_replay_rejected";
    case Gate7Decision::WrongPhase: return "gate7_wrong_phase";
    case Gate7Decision::StaleConnectionGeneration: return "gate7_stale_generation";
    case Gate7Decision::CorrelationRejected: return "gate7_correlation_rejected";
    case Gate7Decision::TokenOwnershipRejected: return "gate7_token_ownership_rejected";
    case Gate7Decision::TradingScopeRequired: return "gate7_trading_scope_required";
    case Gate7Decision::InvalidAccountIdentifier: return "gate7_account_identifier_rejected";
    case Gate7Decision::NoFiboDemoAccount: return "gate7_fibo_demo_missing";
    case Gate7Decision::AmbiguousFiboDemoAccount: return "gate7_fibo_demo_ambiguous";
    case Gate7Decision::MissingSymbolMetadata: return "gate7_symbol_metadata_missing";
    case Gate7Decision::NoCanonicalXauusd: return "gate7_xauusd_missing";
    case Gate7Decision::AmbiguousCanonicalXauusd: return "gate7_xauusd_ambiguous";
    case Gate7Decision::FullSymbolMismatch: return "gate7_full_symbol_mismatch";
    case Gate7Decision::SymbolMetadataRejected: return "gate7_symbol_metadata_rejected";
    case Gate7Decision::SubscriptionMismatch: return "gate7_subscription_mismatch";
    case Gate7Decision::MissingSpotSide: return "gate7_spot_side_missing";
    case Gate7Decision::InvalidSpot: return "gate7_spot_invalid";
    case Gate7Decision::CrossedMarket: return "gate7_crossed_market";
    case Gate7Decision::CheckedArithmeticFailed: return "gate7_checked_arithmetic_failed";
    case Gate7Decision::TimestampUnitUnproven: return "timestamp_unit_unproven";
    case Gate7Decision::TimestampStale: return "gate7_timestamp_stale";
    case Gate7Decision::TimestampFuture: return "gate7_timestamp_future";
    case Gate7Decision::ProviderError: return "gate7_provider_error";
    case Gate7Decision::Timeout: return "gate7_timeout";
    case Gate7Decision::Cancelled: return "gate7_cancelled";
    case Gate7Decision::MalformedFrame: return "gate7_malformed_frame";
    case Gate7Decision::ResourceExhausted: return "gate7_resource_exhausted";
    }
    return "gate7_unknown_failure";
}

} // namespace tradebot::ctrader
