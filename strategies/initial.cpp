/// \file flipper.cpp
/// \brief Sample code to demonstrate the use of a trading platform API with a simple flipping strategy.

#include "../strategy.hpp"
#include <iostream>

using namespace flow;
using namespace std::literals::chrono_literals;

/**
 * \class Flipper
 * \brief Implements a simple flipping trading strategy.
 *
 * This class represents a Flipper strategy that trades only one symbol, EUR/USD.
 * The logic is very simple and has no chance of making any money.
 *
 * Trading Logic:
 * - Every minute, the strategy flips its position between its target position.
 */
class Flipper : public Strategy, public OrderStateObserver
{
public:
    /**
     * \brief Constructor
     *
     * The constructor initializes the strategy with the event dispatcher, gateway, and risk management objects.
     * It also schedules the first position flip event.
     *
     * \param eventDispatcher The event dispatcher for scheduling tasks and getting the current time
     * \param gateway The gateway for sending orders
     * \param risk The risk management object
     */
    Flipper(EventDispatcher &eventDispatcher, Gateway &gateway, Risk &risk)
        : m_eventDispatcher{eventDispatcher}, m_gateway{gateway}, m_risk{risk}
    {
        onMinute();
    }

    /**
     * \brief Notification handler for top of book updates.
     *
     * This callback is called when there is a new symbol available to be published. The client can keep a reference to the record or subscribe to receive callbacks every time the record changes.
     *
     * \param consumer The consumer of the top of book updates
     * \param symbol The trading symbol
     * \param book The top of the book data
     */
    void notify(pub::Consumer<md::TopOfBook> &, std::string_view symbol, const md::TopOfBook &book) override
    {
        m_bookSnapshots.insert_or_assign(std::string(symbol), &book);
    }

    /**
     * \brief End of batch handler for top of book updates.
     *
     * This method is called after a batch of concurrent updates. For example, after a trade, the positions of the deal and contra currencies change, and after those two changes are propagated, the endOfBatch method is called.
     *
     * \param consumer The consumer of the top of book updates
     */
    void endOfBatch(pub::Consumer<md::TopOfBook> &) override {}

    /**
     * \brief Notification handler for position updates.
     *
     * This callback is called when there is a new symbol available to be published. The client can keep a reference to the record or subscribe to receive callbacks every time the record changes.
     *
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

    /**
     * \brief End of batch handler for position updates.
     *
     * This method is called after a batch of concurrent updates. For example, after a trade, the positions of the deal and contra currencies change, and after those two changes are propagated, the endOfBatch method is called.
     *
     * \param consumer The consumer of the position updates
     */
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

    /// OrderStateObserver interface
    /**
     * \brief Notification handler for order acknowledgments.
     * \param symbol The trading symbol
     * \param orderId The order ID
     * \param side The side of the order (Buy/Sell)
     * \param price The price at which the order was placed
     * \param qty The quantity of the order
     * \param tif The time-in-force of the order
     */
    void onAck(const Symbol &symbol, OrderId orderId, Side side, Price price, Quantity qty, TIF) override
    {
    }

    /**
     * \brief Notification handler for order fills.
     * \param symbol The trading symbol
     * \param orderId The order ID
     * \param dealt The quantity dealt in the fill
     * \param contra The quantity dealt by the contra party
     */
    void onFill(const Symbol &, OrderId, Quantity dealt, Quantity contra) override
    {
    }

    /**
     * \brief Notification handler for order terminations.
     * \param symbol The trading symbol
     * \param orderId The order ID
     * \param status The done status of the order
     */
    void onTerminated(const Symbol &, OrderId, DoneStatus status) override
    {
        --m_openOrders;
    }

private:
    /**
     * \brief Handles the logic to be executed every minute.
     *
     * This method logs the PnL, flips the position if there are no open orders, and schedules the next flip event.
     */
    void onMinute()
    {
        // Logs PnL
        std::cout << m_eventDispatcher.getEventTime().count() << ",pnl," << m_risk.getPnL(m_positions) << std::endl;

        float EUR_USD_ASK_CURVE_M = -1.80143393e-17;
        float EUR_USD_ASK_CURVE_H = 32.027486162749;

        float EUR_USD_BID_CURVE_M = -1.80199265e-17;
        float EUR_USD_BID_CURVE_H = 32.03706511936761;

        std::function<float(long long)> EUR_USD_BID_CURVE = [EUR_USD_BID_CURVE_M, EUR_USD_BID_CURVE_H](long long t)
        {
            return (EUR_USD_BID_CURVE_M * t) + EUR_USD_BID_CURVE_H;
        };

        std::function<float(long long)> EUR_USD_ASK_CURVE = [EUR_USD_ASK_CURVE_M, EUR_USD_ASK_CURVE_H](long long t)
        {
            return (EUR_USD_ASK_CURVE_M * t) + EUR_USD_ASK_CURVE_H;
        };

        // static constexpr Position targetPosition = 100e3;
        auto eurusdBook = m_bookSnapshots["EUR/USD"];
        // If there are no open orders, it tries to flip the current EUR position
        if (eurusdBook && !m_openOrders)
        {
            //     Position eurPosition = m_positions["EUR"];
            //     if (eurPosition > 0)
            //     {
            //         sendOrder("EUR/USD", Side::Sell, eurusdBook->bidPrice, targetPosition + eurPosition);
            //     }
            //     else
            //     {
            //         sendOrder("EUR/USD", Side::Buy, eurusdBook->askPrice, targetPosition - eurPosition);
            //     }
        }
        m_eventDispatcher.postEvent(60s, [this]()
                                    { onMinute(); });
    }

    /**
     * \brief Sends an order if no other order is currently open.
     * \param symbol The trading symbol
     * \param side The side of the order (Buy/Sell)
     * \param price The price at which the order is placed
     * \param qty The quantity to order
     */
    void sendOrder(const Symbol &symbol, Side side, Price price, Quantity qty)
    {
        m_gateway.getOrderSender(symbol, *this).sendOrder(side, price, qty, TIF::IOC);
        ++m_openOrders;
    }

private:
    EventDispatcher &m_eventDispatcher;                                ///< Reference to the event dispatcher
    Gateway &m_gateway;                                                ///< Reference to the gateway for sending orders
    Risk &m_risk;                                                      ///< Reference to the risk management object
    std::unordered_map<Asset, Position> m_positions;                   ///< Map of asset positions
    std::unordered_map<Symbol, const md::TopOfBook *> m_bookSnapshots; ///< Map of top of book snapshots
    int m_openOrders = 0;                                              ///< Counter for open orders
};

/**
 * \brief Factory function to create a new Flipper strategy.
 * \param eventDispatcher The event dispatcher for scheduling tasks and getting the current time
 * \param gateway The gateway for sending orders
 * \param risk The risk management object
 * \return A unique pointer to the created strategy
 */
std::unique_ptr<Strategy> createStrategy(EventDispatcher &eventDispatcher, Gateway &gateway, Risk &risk)
{
    return std::make_unique<Flipper>(eventDispatcher, gateway, risk);
}
