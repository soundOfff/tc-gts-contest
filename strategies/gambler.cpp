/// \file trader.cpp
/// \brief Sample code to demonstrate the use of a trading platform API.

#include "../strategy.hpp"
#include <functional>
#include <iostream>

using namespace flow;
using namespace std::literals::chrono_literals;
using namespace std::placeholders;

/**
 * \class Trader
 * \brief Implements trading logic based on market data updates.
 *
 * This class represents a Trader that trades a given instrument based on predefined rules.
 * The trading logic has no alpha and can be considered more of a gamble.
 *
 * Trading Logic:
 * - When there is no position, the trader waits for the spread to be below a certain threshold
 *   and buys up to its max target position.
 * - Once in a position, the trader waits for the market to move above the take profit threshold
 *   or below the stop loss threshold to exit the position.
 */
class Trader : public OrderStateObserver
{
    static constexpr double MIN_ENTRY_SPREAD = 1e-5;      ///< Minimum spread to enter a trade
    static constexpr double TAKE_PROFIT_THRESHOLD = 5e-5; ///< Take profit threshold
    static constexpr double STOP_LOSS_THRESHOLD = -5e-4;  ///< Stop loss threshold
    static constexpr Quantity TARGET_POSITION = 1e6;      ///< Target position size

public:
    /**
     * \brief Constructor
     *
     * The constructor retrieves the `OrderSender` for the given symbol from the gateway and caches it for performance reasons.
     *
     * \param symbol The trading symbol
     * \param gateway The gateway for sending orders
     */
    explicit Trader(const Symbol &symbol, Gateway &gateway)
        : m_orderSender{gateway.getOrderSender(symbol, *this)}
    {
    }

    /**
     * \brief Handles updates to the top of the book.
     * \param symbol The trading symbol
     * \param book The top of the book data
     */
    void onTopOfBook(std::string_view, const md::TopOfBook &book)
    {
        if (m_position < 0.01)
        {   
        }
        else
        {
        }
    }

    /// \name OrderStateObserver interface
    /// @{
    void onAck(const Symbol &, OrderId orderId, Side, Price, Quantity, TIF) override
    {
    }

    void onFill(const Symbol &, OrderId, Quantity dealt, Quantity contra) override
    {
        m_position += dealt;
    }

    void onTerminated(const Symbol &, OrderId, DoneStatus status) override
    {
        m_openOrder = false;
    }
    /// @}

private:
    /**
     * \brief Sends an order if no other order is currently open.
     * \param side The side of the order (Buy/Sell)
     * \param price The price at which to send the order
     * \param qty The quantity to order
     */
    void sendOrder(Side side, Price price, Quantity qty)
    {
        if (!m_openOrder)
        {
            m_orderSender.sendOrder(side, price, qty, TIF::IOC);
            m_openOrder = true;
        }
    }

private:
    OrderSender &m_orderSender; ///< Reference to the order sender
    Price m_entryPrice;         ///< The entry price for the current position
    Quantity m_position = 0;    ///< The current position size
    bool m_openOrder = false;   ///< Flag to indicate if there is an open order
};

/**
 * \class Gambler
 * \brief A strategy class that manages multiple traders and logs PnL and positions.
 *
 * This class demonstrates the creation of specific Traders that will trade a given instrument.
 * The logic of the Trader is more of a gamble, as it trades based on predefined rules without alpha.
 */
class Gambler : public Strategy
{
public:
    /**
     * \brief Constructor
     * \param eventDispatcher The event dispatcher for scheduling tasks and getting the current time
     * \param gateway The gateway for sending orders
     * \param risk The risk management object
     */
    Gambler(EventDispatcher &eventDispatcher, Gateway &gateway, Risk &risk)
        : m_eventDispatcher{eventDispatcher}, m_risk{risk}
    {
        onMinute();
        addTrader("EUR/USD", gateway);
        // addTrader("USD/JPY", gateway);
        // addTrader("GBP/USD", gateway);
    }

    /**
     * \brief Notification handler for top of book updates.
     * \param consumer The consumer of the top of book updates
     * \param symbol The trading symbol
     * \param book The top of the book data
     */
    void notify(pub::Consumer<md::TopOfBook> &consumer, std::string_view symbol, const md::TopOfBook &book) override
    {
        auto it = m_traders.find(Symbol(symbol));
        if (it != m_traders.end())
        {
            consumer.subscribe(symbol, std::bind(&Trader::onTopOfBook, it->second.get(), _1, _2));
        }
    }

    void endOfBatch(pub::Consumer<md::TopOfBook> &) override {}

    /**
     * \brief Notification handler for position updates.
     * \param positionsConsumer The consumer of the position updates
     * \param asset The trading asset
     * \param position The position data
     */
    void notify(pub::Consumer<Position> &positionsConsumer, std::string_view asset, const Position &position) override
    {
        auto onPosition = [this](std::string_view asset, const Position &position)
        {
            m_positions[Asset(asset)] = position;
        };

        onPosition(asset, position);
        positionsConsumer.subscribe(asset, onPosition);
    }

    void endOfBatch(pub::Consumer<Position> &) override
    {
        // Logs positions
        std::cout << m_eventDispatcher.getEventTime().count() << ",positions";
        for (auto &position : m_positions)
        {
            std::cout << "," << position.first << ":" << position.second;
        }
        std::cout << std::endl;
    }

private:
    /**
     * \brief Schedules and logs PnL every minute.
     */
    void onMinute()
    {
        // Logs PnL
        std::cout << m_eventDispatcher.getEventTime().count() << ",pnl," << m_risk.getPnL(m_positions) << std::endl;

        m_eventDispatcher.postEvent(60s, [this]()
                                    { onMinute(); });
    }

    /**
     * \brief Adds a trader for a specific symbol.
     * \param symbol The trading symbol
     * \param gateway The gateway for sending orders
     */
    void addTrader(const Symbol &symbol, Gateway &gateway)
    {
        m_traders.emplace(symbol, std::make_unique<Trader>(symbol, gateway));
    }

private:
    EventDispatcher &m_eventDispatcher;                            ///< Reference to the event dispatcher
    Risk &m_risk;                                                  ///< Reference to the risk management object
    std::unordered_map<Asset, Position> m_positions;               ///< Map of asset positions
    std::unordered_map<Symbol, std::unique_ptr<Trader>> m_traders; ///< Map of symbol to trader objects
};

/**
 * \brief Factory function to create a new Gambler strategy.
 * \param eventDispatcher The event dispatcher for scheduling tasks and getting the current time
 * \param gateway The gateway for sending orders
 * \param risk The risk management object
 * \return A unique pointer to the created strategy
 */
std::unique_ptr<Strategy> createStrategy(EventDispatcher &eventDispatcher, Gateway &gateway, Risk &risk)
{
    return std::make_unique<Gambler>(eventDispatcher, gateway, risk);
}
