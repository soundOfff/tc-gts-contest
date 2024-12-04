/// Public
#pragma once

#include "event_dispatcher.hpp"
#include "flow.hpp"
#include "market_data.hpp"
#include "pub.hpp"
#include "risk.hpp"

#include <memory>

/**
 * \brief Represents a trading strategy that subscribes to market data and position updates.
 *
 * This base class allows subscribing to market data updates (TopOfBook) and position updates.
 * Derived classes implement specific trading logic.
 */
struct Strategy : public pub::Subscriber<md::TopOfBook>, public pub::Subscriber<Position>
{
};

/**
 * \brief Factory function to create a strategy instance.
 *
 * \param eventDispatcher The event dispatcher for time-related operations.
 * \param gateway The gateway providing access to order sending functionality.
 * \param risk The risk management component for PnL calculations.
 * \return A unique pointer to the created strategy.
 *
 * Creates and returns a strategy instance based on the provided dependencies.
 * The strategy should use the event dispatcher exclusively for time-related operations,
 * ensuring compatibility with both simulations and real-time trading environments.
 */
std::unique_ptr<Strategy> createStrategy(EventDispatcher &eventDispatcher, flow::Gateway &gateway, Risk &risk);
