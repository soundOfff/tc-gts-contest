/// Public
#pragma once

#include "symbology.hpp"

using Position = double;
using Price = double;
using Quantity = double;

class Risk
{
protected:
    virtual ~Risk() = default;

public:
    /**
     * @brief Calculates and returns the Profit and Loss (PnL) of a portfolio of positions.
     *
     * This function computes the total PnL of a portfolio by iterating over each position.
     * Each position is represented as a key-value pair, where the key is the asset and the
     * value is the quantity of the position.
     *
     * @tparam Positions A container type that holds the positions in the portfolio.
     *                   The container should support iteration, and each element should
     *                   be a key-value pair where the key represents the asset and the
     *                   value represents the position quantity.
     * @param positions  A constant reference to a container holding the positions.
     *
     * @return The total PnL of the portfolioas a Quantity.
     */
    template <typename Positions>
    Quantity getPnL(const Positions &positions) const
    {
        Quantity pnl{0};
        for (const auto &entry : positions)
        {
            auto asset = entry.first;
            auto position = getPosition(entry.second);
            pnl += position * getFairPrice(asset);
        }
        return pnl;
    }

    /**
     * @brief Calculates and returns the Net Open Positions (NOP) of a portfolio of positions.
     *
     * This function computes the NOP of a portfolio by iterating over each position.
     * Each position is represented as a key-value pair, where the key is the asset and the
     * value is the quantity of the position. The NOP is calculated by separately summing
     * the fair value of long and short positions, and then taking the maximum of these sums.
     *
     * @tparam Positions A container type that holds the positions in the portfolio.
     *                   The container should support iteration, and each element should
     *                   be a key-value pair where the key represents the asset and the
     *                   value represents the position quantity.
     * @param positions  A constant reference to a container holding the positions.
     *
     * @return The NOP of the portfolio as a Quantity.
     */
    template <typename Positions>
    Quantity getNOP(const Positions &positions) const
    {
        Quantity longs{0}, shorts{0};
        for (const auto &entry : positions)
        {
            auto asset = entry.first;
            auto position = getPosition(entry.second);
            if (position >= 0)
            {
                longs += position * getFairPrice(asset);
            }
            else
            {
                shorts -= position * getFairPrice(asset);
            }
        }
        return std::max(longs, shorts);
    }

    /**
     * @brief Calculates and returns the fair price of a given asset.
     *
     * This function calculates the fair price of the specified asset.
     * The fair price is typically derived from market data or a pricing model.
     * This is a pure virtual function, meaning that it must be implemented
     * by any derived class.
     *
     * @param asset A constant reference to the asset for which the fair price is to be calculated.
     *
     * @return The fair price of the asset as a Price.
     */
    virtual Price getFairPrice(const Asset &) const = 0;

private:
    static inline Position getPosition(Position position) { return position; }
    static inline Position getPosition(const Position *position) { return *position; }
};