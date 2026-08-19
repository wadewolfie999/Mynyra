#include "FinancialMath.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Financial {
namespace {

struct UInt128 {
    std::uint64_t high{0};
    std::uint64_t low{0};
};

std::uint64_t magnitude(std::int64_t value) noexcept
{
    if (value >= 0) {
        return static_cast<std::uint64_t>(value);
    }
    return static_cast<std::uint64_t>(-(value + 1)) + 1;
}

UInt128 multiplyWide(std::uint64_t lhs, std::uint64_t rhs) noexcept
{
    constexpr std::uint64_t MASK = 0xffff'ffffULL;
    const std::uint64_t lhsLow = lhs & MASK;
    const std::uint64_t lhsHigh = lhs >> 32;
    const std::uint64_t rhsLow = rhs & MASK;
    const std::uint64_t rhsHigh = rhs >> 32;

    const std::uint64_t lowLow = lhsLow * rhsLow;
    const std::uint64_t lowHigh = lhsLow * rhsHigh;
    const std::uint64_t highLow = lhsHigh * rhsLow;
    const std::uint64_t highHigh = lhsHigh * rhsHigh;

    const std::uint64_t middle = (lowLow >> 32)
                               + (lowHigh & MASK)
                               + (highLow & MASK);
    return UInt128{
        highHigh + (lowHigh >> 32) + (highLow >> 32) + (middle >> 32),
        (middle << 32) | (lowLow & MASK)
    };
}

bool bitAt(const UInt128& value, int index) noexcept
{
    return index >= 64
        ? ((value.high >> (index - 64)) & 1ULL) != 0
        : ((value.low >> index) & 1ULL) != 0;
}

std::optional<std::uint64_t> divideWide(UInt128 numerator,
                                        std::uint64_t denominator,
                                        std::uint64_t& remainder) noexcept
{
    if (denominator == 0) {
        return std::nullopt;
    }

    std::uint64_t quotient = 0;
    remainder = 0;
    for (int bit = 127; bit >= 0; --bit) {
        const bool incoming = bitAt(numerator, bit);
        const bool carry = (remainder >> 63) != 0;
        std::uint64_t shifted = (remainder << 1) | (incoming ? 1ULL : 0ULL);
        bool quotientBit = false;
        if (carry || shifted >= denominator) {
            shifted -= denominator;
            quotientBit = true;
        }
        remainder = shifted;

        if (quotientBit) {
            if (bit >= 64) {
                return std::nullopt;
            }
            quotient |= (1ULL << bit);
        }
    }
    return quotient;
}

std::optional<std::int64_t> signedQuotient(UInt128 numerator,
                                           std::uint64_t denominator,
                                           bool negative,
                                           Rounding rounding) noexcept
{
    std::uint64_t remainder = 0;
    auto quotient = divideWide(numerator, denominator, remainder);
    if (!quotient.has_value()) {
        return std::nullopt;
    }

    if (rounding == Rounding::RejectUnaligned && remainder != 0) {
        return std::nullopt;
    }
    if (rounding == Rounding::NearestTiesAwayFromZero
        && remainder >= (denominator / 2 + denominator % 2)) {
        if (*quotient == std::numeric_limits<std::uint64_t>::max()) {
            return std::nullopt;
        }
        ++*quotient;
    }

    const std::uint64_t positiveLimit = static_cast<std::uint64_t>(
        std::numeric_limits<std::int64_t>::max());
    const std::uint64_t negativeLimit = positiveLimit + 1;
    if ((!negative && *quotient > positiveLimit)
        || (negative && *quotient > negativeLimit)) {
        return std::nullopt;
    }
    if (negative && *quotient == negativeLimit) {
        return std::numeric_limits<std::int64_t>::min();
    }
    const auto signedValue = static_cast<std::int64_t>(*quotient);
    return negative ? -signedValue : signedValue;
}

std::optional<std::int64_t> normalize(double value, Rounding rounding) noexcept
{
    if (!std::isfinite(value)) {
        return std::nullopt;
    }
    const long double scaled = static_cast<long double>(value)
                             * static_cast<long double>(SCALE_FACTOR);
    long double rounded = 0.0L;
    switch (rounding) {
        case Rounding::TowardZero:
            rounded = std::trunc(scaled);
            break;
        case Rounding::RejectUnaligned:
            rounded = std::round(scaled);
            {
                const long double representationTolerance = std::max(
                    1e-7L,
                    std::fabs(scaled)
                        * static_cast<long double>(std::numeric_limits<double>::epsilon())
                        * 2.0L);
                if (std::fabs(scaled - rounded) > representationTolerance) {
                    return std::nullopt;
                }
            }
            break;
        case Rounding::NearestTiesAwayFromZero:
            rounded = std::round(scaled);
            break;
    }
    const long double minimum = static_cast<long double>(
        std::numeric_limits<std::int64_t>::min());
    const long double maximum = static_cast<long double>(
        std::numeric_limits<std::int64_t>::max());
    if (rounded < minimum || rounded > maximum) {
        return std::nullopt;
    }
    return static_cast<std::int64_t>(rounded);
}

std::optional<std::int64_t> checkedAdd(std::int64_t lhs,
                                       std::int64_t rhs) noexcept
{
    if ((rhs > 0 && lhs > std::numeric_limits<std::int64_t>::max() - rhs)
        || (rhs < 0 && lhs < std::numeric_limits<std::int64_t>::min() - rhs)) {
        return std::nullopt;
    }
    return lhs + rhs;
}

std::optional<std::int64_t> checkedSubtract(std::int64_t lhs,
                                            std::int64_t rhs) noexcept
{
    if ((rhs > 0 && lhs < std::numeric_limits<std::int64_t>::min() + rhs)
        || (rhs < 0 && lhs > std::numeric_limits<std::int64_t>::max() + rhs)) {
        return std::nullopt;
    }
    return lhs - rhs;
}

std::optional<std::int64_t> scaledProduct(std::int64_t lhs,
                                          std::int64_t rhs,
                                          Rounding rounding) noexcept
{
    const bool negative = (lhs < 0) != (rhs < 0);
    return signedQuotient(multiplyWide(magnitude(lhs), magnitude(rhs)),
                          static_cast<std::uint64_t>(SCALE_FACTOR),
                          negative,
                          rounding);
}

std::optional<std::int64_t> scaledRatio(std::int64_t numerator,
                                        std::int64_t denominator,
                                        Rounding rounding) noexcept
{
    if (denominator == 0) {
        return std::nullopt;
    }
    const bool negative = (numerator < 0) != (denominator < 0);
    return signedQuotient(
        multiplyWide(magnitude(numerator),
                     static_cast<std::uint64_t>(SCALE_FACTOR)),
        magnitude(denominator),
        negative,
        rounding);
}

std::optional<std::int64_t> proportionalUnits(std::int64_t total,
                                              std::int64_t part,
                                              std::int64_t whole,
                                              Rounding rounding) noexcept
{
    if (whole == 0) {
        return std::nullopt;
    }
    const bool negative = ((total < 0) != (part < 0)) != (whole < 0);
    return signedQuotient(multiplyWide(magnitude(total), magnitude(part)),
                          magnitude(whole), negative, rounding);
}

template <typename T>
std::optional<T> normalized(double value, Rounding rounding) noexcept
{
    const auto units = normalize(value, rounding);
    if (!units.has_value()) {
        return std::nullopt;
    }
    return T{*units};
}

} // namespace

std::optional<Price> price(double value, Rounding rounding) noexcept
{
    return normalized<Price>(value, rounding);
}

std::optional<Quantity> quantity(double value, Rounding rounding) noexcept
{
    return normalized<Quantity>(value, rounding);
}

std::optional<Money> money(double value, Rounding rounding) noexcept
{
    return normalized<Money>(value, rounding);
}

std::optional<Fraction> fraction(double value, Rounding rounding) noexcept
{
    return normalized<Fraction>(value, rounding);
}

std::optional<Money> add(Money lhs, Money rhs) noexcept
{
    const auto units = checkedAdd(lhs.units, rhs.units);
    return units.has_value() ? std::optional<Money>{Money{*units}} : std::nullopt;
}

std::optional<Money> subtract(Money lhs, Money rhs) noexcept
{
    const auto units = checkedSubtract(lhs.units, rhs.units);
    return units.has_value() ? std::optional<Money>{Money{*units}} : std::nullopt;
}

std::optional<Quantity> add(Quantity lhs, Quantity rhs) noexcept
{
    const auto units = checkedAdd(lhs.units, rhs.units);
    return units.has_value() ? std::optional<Quantity>{Quantity{*units}} : std::nullopt;
}

std::optional<Quantity> subtract(Quantity lhs, Quantity rhs) noexcept
{
    const auto units = checkedSubtract(lhs.units, rhs.units);
    return units.has_value() ? std::optional<Quantity>{Quantity{*units}} : std::nullopt;
}

std::optional<Money> notional(Price priceValue, Quantity quantityValue,
                              Rounding rounding) noexcept
{
    const auto units = scaledProduct(priceValue.units, quantityValue.units, rounding);
    return units.has_value() ? std::optional<Money>{Money{*units}} : std::nullopt;
}

std::optional<Money> fee(Money notionalValue, Fraction rate,
                         Rounding rounding) noexcept
{
    const auto units = scaledProduct(notionalValue.units, rate.units, rounding);
    return units.has_value() ? std::optional<Money>{Money{*units}} : std::nullopt;
}

std::optional<Price> applySlippage(Price marketPrice, Fraction slippageRate,
                                   bool buySide, Rounding rounding) noexcept
{
    if (!marketPrice.isPositive() || slippageRate.isNegative()
        || slippageRate.units >= SCALE_FACTOR) {
        return std::nullopt;
    }
    const std::int64_t multiplier = buySide
        ? SCALE_FACTOR + slippageRate.units
        : SCALE_FACTOR - slippageRate.units;
    const auto units = scaledProduct(marketPrice.units, multiplier, rounding);
    return units.has_value() && *units > 0
        ? std::optional<Price>{Price{*units}} : std::nullopt;
}

std::optional<Quantity> quantityForNotional(Money notionalBudget,
                                            Price priceValue,
                                            Rounding rounding) noexcept
{
    if (notionalBudget.isNegative() || !priceValue.isPositive()) {
        return std::nullopt;
    }
    const auto units = scaledRatio(notionalBudget.units, priceValue.units, rounding);
    return units.has_value() ? std::optional<Quantity>{Quantity{*units}} : std::nullopt;
}

std::optional<Money> notionalBeforeFee(Money totalBudget, Fraction feeRate,
                                       Rounding rounding) noexcept
{
    if (totalBudget.isNegative() || feeRate.isNegative()) {
        return std::nullopt;
    }
    const auto denominator = checkedAdd(SCALE_FACTOR, feeRate.units);
    if (!denominator.has_value() || *denominator <= 0) {
        return std::nullopt;
    }
    const auto units = scaledRatio(totalBudget.units, *denominator, rounding);
    return units.has_value() ? std::optional<Money>{Money{*units}} : std::nullopt;
}

std::optional<Price> averagePrice(Money costBasis, Quantity quantityValue,
                                  Rounding rounding) noexcept
{
    if (costBasis.isNegative() || !quantityValue.isPositive()) {
        return std::nullopt;
    }
    const auto units = scaledRatio(costBasis.units, quantityValue.units, rounding);
    return units.has_value() ? std::optional<Price>{Price{*units}} : std::nullopt;
}

std::optional<Money> proportional(Money total, Quantity part, Quantity whole,
                                  Rounding rounding) noexcept
{
    if (part.isNegative() || !whole.isPositive() || part.units > whole.units) {
        return std::nullopt;
    }
    const auto units = proportionalUnits(total.units, part.units, whole.units, rounding);
    return units.has_value() ? std::optional<Money>{Money{*units}} : std::nullopt;
}

} // namespace Financial
