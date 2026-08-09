#include "CTraderGate6Proof.hpp"

#include <algorithm>
#include <climits>
#include <cstddef>
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

void secureClear(uint64_t& value) noexcept
{
    volatile uint64_t* target = &value;
    *target = 0;
}

void secureClear(int64_t& value) noexcept
{
    volatile int64_t* target = &value;
    *target = 0;
}

bool isSafeBrokerTitle(std::string_view value) noexcept
{
    if (value.empty() || value.size() > 128) {
        return false;
    }
    for (const unsigned char c : value) {
        if (c < 0x20 || c == 0x7f) {
            return false;
        }
    }
    return true;
}

bool isUsableAccountId(uint64_t accountId) noexcept
{
    return accountId > 0
        && accountId <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
}

} // namespace

SensitiveString::SensitiveString(std::string value) noexcept
    : value_(std::move(value))
{
}

SensitiveString::~SensitiveString()
{
    clear();
}

SensitiveString::SensitiveString(SensitiveString&& other) noexcept
    : value_(std::move(other.value_))
{
    other.clear();
}

SensitiveString& SensitiveString::operator=(SensitiveString&& other) noexcept
{
    if (this == &other) {
        return *this;
    }
    clear();
    value_ = std::move(other.value_);
    other.clear();
    return *this;
}

void SensitiveString::clear() noexcept
{
    secureClear(value_);
}

Gate6AccountRecord::~Gate6AccountRecord()
{
    secureClear(accountId);
    if (brokerTitleShort.has_value()) {
        secureClear(*brokerTitleShort);
    }
}

bool CTraderGate6Config::isAllowedOpenApiEndpoint(std::string_view host,
                                                   uint16_t port) noexcept
{
    return host == DEMO_HOST && port == DEMO_PORT;
}

bool CTraderGate6Config::isAllowedOutboundPayload(uint32_t payloadType) noexcept
{
    switch (payloadType) {
    case 51:   // ProtoHeartbeatEvent
    case 2100: // ProtoOAApplicationAuthReq
    case 2102: // ProtoOAAccountAuthReq
    case 2149: // ProtoOAGetAccountListByAccessTokenReq
        return true;
    default:
        return false;
    }
}

CTraderGate6AccountProof::~CTraderGate6AccountProof()
{
    clearAll();
}

Gate6Decision CTraderGate6AccountProof::validateEvidence(
    const Gate6AccountListEvidence& evidence) noexcept
{
    if (!evidence.currentConnectionGeneration) {
        return Gate6Decision::StaleConnectionGeneration;
    }
    if (!evidence.correlationMatched) {
        return Gate6Decision::CorrelationRejected;
    }
    if (!evidence.tokenOwned) {
        return Gate6Decision::TokenOwnershipRejected;
    }
    if (!evidence.tradingScope) {
        return Gate6Decision::TradingScopeRequired;
    }
    return Gate6Decision::Ready;
}

Gate6Decision CTraderGate6AccountProof::acceptGate6A(
    Gate6AccountListEvidence evidence) noexcept
{
    if (phase_ == Phase::Terminal || phase_ == Phase::Complete) {
        lastDecision_ = Gate6Decision::AlreadyTerminal;
        return lastDecision_;
    }
    if (phase_ != Phase::Gate6AReady) {
        return finish(Gate6Decision::WrongPhase);
    }

    const Gate6Decision validation = validateEvidence(evidence);
    if (validation != Gate6Decision::Ready) {
        return finish(validation);
    }

    clearAccountIdentifiers();
    safeCandidates_.clear();

    for (Gate6AccountRecord& account : evidence.accounts) {
        if (!account.isLive.has_value()) {
            continue;
        }
        if (*account.isLive) {
            continue;
        }
        if (!account.brokerTitleShort.has_value()) {
            continue;
        }
        if (!isSafeBrokerTitle(*account.brokerTitleShort)) {
            return finish(Gate6Decision::UnsafeBrokerMetadata);
        }
        if (!isUsableAccountId(account.accountId)) {
            return finish(Gate6Decision::InvalidAccountIdentifier);
        }

        volatileCandidates_.push_back(
            {account.accountId, std::move(*account.brokerTitleShort)});
    }

    if (volatileCandidates_.empty()) {
        return finish(Gate6Decision::NoEligibleDemoAccount);
    }

    std::sort(volatileCandidates_.begin(), volatileCandidates_.end(),
              [](const VolatileCandidate& lhs, const VolatileCandidate& rhs) {
                  return lhs.brokerTitleShort < rhs.brokerTitleShort;
              });
    for (std::size_t i = 1; i < volatileCandidates_.size(); ++i) {
        if (volatileCandidates_[i - 1].brokerTitleShort
            == volatileCandidates_[i].brokerTitleShort) {
            return finish(Gate6Decision::AmbiguousDemoAccount);
        }
    }

    safeCandidates_.reserve(volatileCandidates_.size());
    for (const VolatileCandidate& candidate : volatileCandidates_) {
        safeCandidates_.push_back({false, candidate.brokerTitleShort});
    }
    phase_ = Phase::AwaitingWade;
    lastDecision_ = Gate6Decision::AwaitingWadeCheckpoint;
    return lastDecision_;
}

Gate6Decision CTraderGate6AccountProof::confirmWadeSelection(
    std::string_view brokerTitleShort) noexcept
{
    if (phase_ == Phase::Terminal || phase_ == Phase::Complete) {
        lastDecision_ = Gate6Decision::AlreadyTerminal;
        return lastDecision_;
    }
    if (phase_ != Phase::AwaitingWade || !isSafeBrokerTitle(brokerTitleShort)) {
        return finish(Gate6Decision::WadeSelectionRejected);
    }

    const auto match = std::find_if(
        volatileCandidates_.begin(), volatileCandidates_.end(),
        [brokerTitleShort](const VolatileCandidate& candidate) {
            return candidate.brokerTitleShort == brokerTitleShort;
        });
    if (match == volatileCandidates_.end()) {
        return finish(Gate6Decision::WadeSelectionRejected);
    }

    approvedBrokerTitle_.assign(brokerTitleShort);
    clearAccountIdentifiers();
    safeCandidates_.clear();
    phase_ = Phase::Gate6BReady;
    lastDecision_ = Gate6Decision::ReadyForGate6B;
    return lastDecision_;
}

Gate6Decision CTraderGate6AccountProof::acceptGate6B(
    Gate6AccountListEvidence evidence) noexcept
{
    if (phase_ == Phase::Terminal || phase_ == Phase::Complete) {
        lastDecision_ = Gate6Decision::AlreadyTerminal;
        return lastDecision_;
    }
    if (phase_ != Phase::Gate6BReady || approvedBrokerTitle_.empty()) {
        return finish(Gate6Decision::WrongPhase);
    }

    const Gate6Decision validation = validateEvidence(evidence);
    if (validation != Gate6Decision::Ready) {
        return finish(validation);
    }

    uint64_t match = 0;
    std::size_t matchCount = 0;
    for (const Gate6AccountRecord& account : evidence.accounts) {
        if (!account.isLive.has_value() || *account.isLive
            || !account.brokerTitleShort.has_value()) {
            continue;
        }
        if (!isSafeBrokerTitle(*account.brokerTitleShort)) {
            return finish(Gate6Decision::UnsafeBrokerMetadata);
        }
        if (*account.brokerTitleShort != approvedBrokerTitle_) {
            continue;
        }
        if (!isUsableAccountId(account.accountId)) {
            return finish(Gate6Decision::InvalidAccountIdentifier);
        }
        match = account.accountId;
        ++matchCount;
    }

    if (matchCount == 0) {
        secureClear(match);
        return finish(Gate6Decision::NoEligibleDemoAccount);
    }
    if (matchCount != 1) {
        secureClear(match);
        return finish(Gate6Decision::AmbiguousDemoAccount);
    }

    selectedAccountId_ = match;
    secureClear(match);
    phase_ = Phase::AccountAuthReady;
    lastDecision_ = Gate6Decision::ReadyForAccountAuthentication;
    return lastDecision_;
}

std::optional<int64_t> CTraderGate6AccountProof::accountIdForAuthentication()
    const noexcept
{
    if (phase_ != Phase::AccountAuthReady
        || !isUsableAccountId(selectedAccountId_)) {
        return std::nullopt;
    }
    return static_cast<int64_t>(selectedAccountId_);
}

Gate6Decision CTraderGate6AccountProof::acceptAccountAuthentication(
    int64_t responseAccountId) noexcept
{
    if (phase_ == Phase::Terminal || phase_ == Phase::Complete) {
        secureClear(responseAccountId);
        lastDecision_ = Gate6Decision::AlreadyTerminal;
        return lastDecision_;
    }
    const bool matches = phase_ == Phase::AccountAuthReady
        && responseAccountId > 0
        && static_cast<uint64_t>(responseAccountId) == selectedAccountId_;
    secureClear(responseAccountId);
    if (!matches) {
        return finish(Gate6Decision::AccountAuthenticationMismatch);
    }

    clearAll();
    phase_ = Phase::Complete;
    lastDecision_ = Gate6Decision::AccountProofSucceeded;
    return lastDecision_;
}

Gate6Decision CTraderGate6AccountProof::cancel() noexcept
{
    if (phase_ == Phase::Terminal || phase_ == Phase::Complete) {
        lastDecision_ = Gate6Decision::AlreadyTerminal;
        return lastDecision_;
    }
    return finish(Gate6Decision::Cancelled);
}

Gate6Decision CTraderGate6AccountProof::finish(Gate6Decision decision) noexcept
{
    clearAll();
    phase_ = Phase::Terminal;
    lastDecision_ = decision;
    return lastDecision_;
}

void CTraderGate6AccountProof::clearAccountIdentifiers() noexcept
{
    for (VolatileCandidate& candidate : volatileCandidates_) {
        secureClear(candidate.accountId);
        secureClear(candidate.brokerTitleShort);
    }
    volatileCandidates_.clear();
    secureClear(selectedAccountId_);
}

void CTraderGate6AccountProof::clearAll() noexcept
{
    clearAccountIdentifiers();
    for (Gate6SafeCandidate& candidate : safeCandidates_) {
        secureClear(candidate.brokerTitleShort);
    }
    safeCandidates_.clear();
    secureClear(approvedBrokerTitle_);
}

bool CTraderGate6AccountProof::isAwaitingWadeCheckpoint() const noexcept
{
    return phase_ == Phase::AwaitingWade;
}

bool CTraderGate6AccountProof::isTerminal() const noexcept
{
    return phase_ == Phase::Terminal || phase_ == Phase::Complete;
}

std::string_view CTraderGate6AccountProof::safeDiagnostic(
    Gate6Decision decision) noexcept
{
    switch (decision) {
    case Gate6Decision::Ready: return "gate6_ready";
    case Gate6Decision::AwaitingWadeCheckpoint: return "gate6_checkpoint_wait";
    case Gate6Decision::ReadyForGate6B: return "gate6b_ready";
    case Gate6Decision::ReadyForAccountAuthentication: return "gate6_account_auth_ready";
    case Gate6Decision::AccountProofSucceeded: return "gate6_account_proof_succeeded";
    case Gate6Decision::Cancelled: return "gate6_cancelled";
    case Gate6Decision::AlreadyTerminal: return "gate6_terminal_replay_rejected";
    case Gate6Decision::WrongPhase: return "gate6_wrong_phase";
    case Gate6Decision::StaleConnectionGeneration: return "gate6_stale_generation";
    case Gate6Decision::CorrelationRejected: return "gate6_correlation_rejected";
    case Gate6Decision::TokenOwnershipRejected: return "gate6_token_ownership_rejected";
    case Gate6Decision::TradingScopeRequired: return "gate6_trading_scope_required";
    case Gate6Decision::LiveAccountExcluded: return "gate6_live_account_excluded";
    case Gate6Decision::MissingAccountMetadata: return "gate6_account_metadata_missing";
    case Gate6Decision::UnsafeBrokerMetadata: return "gate6_broker_metadata_rejected";
    case Gate6Decision::InvalidAccountIdentifier: return "gate6_account_identifier_rejected";
    case Gate6Decision::NoEligibleDemoAccount: return "gate6_demo_account_missing";
    case Gate6Decision::AmbiguousDemoAccount: return "gate6_demo_account_ambiguous";
    case Gate6Decision::WadeSelectionRejected: return "gate6_wade_selection_rejected";
    case Gate6Decision::AccountAuthenticationMismatch: return "gate6_account_auth_mismatch";
    }
    return "gate6_unknown_failure";
}

} // namespace tradebot::ctrader
