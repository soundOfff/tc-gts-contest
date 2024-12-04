/// Public
#pragma once

namespace md
{

    /**
     * @brief Represents the top-of-book market data for a symbol.
     *
     * The `TopOfBook` struct encapsulates essential information about the best
     * available bid and ask prices, along with their corresponding sizes. This
     * data is critical for traders and trading algorithms to make informed
     * decisions based on the current market conditions.
     *
     * This data is continuously updated and used extensively by traders and
     * trading algorithms to monitor market liquidity, identify trading opportunities,
     * manage risks effectively, and determine the prevailing market sentiment.
     * The bid and ask prices reflect the supply and demand dynamics in the market,
     * allowing traders to gauge price levels where buying and selling interest is
     * concentrated.
     */
    struct TopOfBook
    {
        double bidSize;  ///< Quantity available at the best bid price.
        double bidPrice; ///< Highest price a buyer is willing to pay.
        double askSize;  ///< Quantity available at the best ask price.
        double askPrice; ///< Lowest price a seller is willing to accept.
    };

} // namespace md
