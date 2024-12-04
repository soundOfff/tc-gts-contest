#pragma once

#pragma once

#include <string>

/**
 * @brief Symbol represents a currency pair in Forex (FX) trading.
 *
 * In the context of FX trading, a currency pair is denoted in the format
 * "CCY1/CCY2", where "CCY1" is the base currency and "CCY2" is the quote currency.
 * Each currency is represented by its three-character ISO 4217 code.
 * For a list of ISO 4217 currency codes, you can refer to the ISO's official website:
 * https://www.iso.org/iso-4217-currency-codes.html
 * Wikipedia with the list of the codes:
 * https://en.wikipedia.org/wiki/ISO_4217
 */
using Symbol = std::string;
using Asset = std::string;

/**
 * @brief Extracts the base currency from a currency pair.
 *
 * In Forex (FX) trading, a currency pair typically represents two currencies
 * in the format "CCY1/CCY2", where "CCY1" is the base currency and "CCY2" is
 * the quote currency. Each currency is represented by its three-character ISO
 * code. This function returns the base currency part of the pair.
 *
 * @param symbol The currency pair in the format "CCY1/CCY2".
 * @return Asset The base currency extracted from the pair.
 */
inline Asset getBaseAsset(const Symbol &symbol)
{
    return symbol.substr(0, 3);
}

/**
 * @brief Extracts the quote currency from a currency pair.
 *
 * In Forex (FX) trading, a currency pair typically represents two currencies
 * in the format "CCY1/CCY2", where "CCY1" is the base currency and "CCY2" is
 * the quote currency. Each currency is represented by its three-character ISO
 * code. This function returns the quote currency part of the pair.
 *
 * @param symbol The currency pair in the format "CCY1/CCY2".
 * @return Asset The quote currency extracted from the pair.
 */
inline Asset getQuoteAsset(const Symbol &symbol)
{
    return symbol.substr(4, 3);
}
