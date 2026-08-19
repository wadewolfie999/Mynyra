#pragma once

#include <cstdint>
#include <optional>

namespace Financial {

inline constexpr std::uint8_t SCALE = 8;
inline constexpr std::int64_t SCALE_FACTOR = 100'000'000;

enum class Rounding : std::uint8_t {
    TowardZero,
    RejectUnaligned,
    NearestTiesAwayFromZero
};

struct PriceTag {};
struct QuantityTag {};
struct MoneyTag {};
struct FractionTag {};

template <typename Tag>
struct Value {
    std::int64_t units{0};

    double toDouble() const noexcept
    {
        return static_cast<double>(units) / static_cast<double>(SCALE_FACTOR);
    }

    bool isPositive() const noexcept { return units > 0; }
    bool isNegative() const noexcept { return units < 0; }
    bool isZero() const noexcept { return units == 0; }

    friend bool operator==(const Value&, const Value&) = default;
};

using Price = Value<PriceTag>;
using Quantity = Value<QuantityTag>;
using Money = Value<MoneyTag>;
using Fraction = Value<FractionTag>;

std::optional<Price> price(double value,
                           Rounding rounding = Rounding::NearestTiesAwayFromZero) noexcept;
std::optional<Quantity> quantity(
    double value,
    Rounding rounding = Rounding::TowardZero) noexcept;
std::optional<Money> money(double value,
                          Rounding rounding = Rounding::NearestTiesAwayFromZero) noexcept;
std::optional<Fraction> fraction(
    double value,
    Rounding rounding = Rounding::NearestTiesAwayFromZero) noexcept;

std::optional<Money> add(Money lhs, Money rhs) noexcept;
std::optional<Money> subtract(Money lhs, Money rhs) noexcept;
std::optional<Quantity> add(Quantity lhs, Quantity rhs) noexcept;
std::optional<Quantity> subtract(Quantity lhs, Quantity rhs) noexcept;

std::optional<Money> notional(
    Price priceValue,
    Quantity quantityValue,
    Rounding rounding = Rounding::NearestTiesAwayFromZero) noexcept;
std::optional<Money> fee(
    Money notionalValue,
    Fraction rate,
    Rounding rounding = Rounding::NearestTiesAwayFromZero) noexcept;
std::optional<Price> applySlippage(
    Price marketPrice,
    Fraction slippageRate,
    bool buySide,
    Rounding rounding = Rounding::NearestTiesAwayFromZero) noexcept;
std::optional<Quantity> quantityForNotional(
    Money notionalBudget,
    Price priceValue,
    Rounding rounding = Rounding::TowardZero) noexcept;
std::optional<Money> notionalBeforeFee(
    Money totalBudget,
    Fraction feeRate,
    Rounding rounding = Rounding::TowardZero) noexcept;
std::optional<Price> averagePrice(
    Money costBasis,
    Quantity quantityValue,
    Rounding rounding = Rounding::NearestTiesAwayFromZero) noexcept;
std::optional<Money> proportional(
    Money total,
    Quantity part,
    Quantity whole,
    Rounding rounding = Rounding::NearestTiesAwayFromZero) noexcept;

} // namespace Financial
