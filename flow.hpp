/// Public
#pragma once

#include "symbology.hpp" // Included for Symbol definition

#include <cstdint> // For uint64_t

namespace flow
{

    /// Enumeration for defining the side of an order (Buy or Sell)
    enum class Side : int
    {
        Buy = 1,
        Sell = -1
    };

    /// Enumeration for defining the status of an order after completion
    enum class DoneStatus : int
    {
        // Cancelled or fully filled without error
        Done,
        // Rejected by the exchange
        Rejected,
        // Rejected for risk or other issue
        InternalReject,
    };

    /// Enumeration for defining Time In Force (TIF) for orders
    enum class TIF : int
    {
        // Good Till Cancel
        GTC,
        // Immediate Or Cancel
        IOC
    };

    /// Type alias for OrderId, representing a unique identifier for an order
    using OrderId = uint64_t;

    /// Type alias for Price, representing the price of an order
    using Price = double;

    /// Type alias for Quantity, representing the quantity of an order
    using Quantity = double;

    /// Interface for receiving order updates on orders sent through an OrderSender
    class OrderStateObserver
    {
    protected:
        virtual ~OrderStateObserver() = default;

    public:
        /**
         * @brief Called when an order is acknowledged.
         *
         * @param symbol The symbol associated with the order.
         * @param orderId The unique identifier of the order.
         * @param side The side of the order (Buy or Sell).
         * @param price The price at which the order was placed.
         * @param qty The total quantity of the order.
         * @param tif The Time In Force (TIF) of the order.
         */
        virtual void onAck(const Symbol &symbol, OrderId orderId, Side side, Price price, Quantity qty, TIF tif) = 0;

        /**
         * @brief Called when an order is partially or fully filled.
         *
         * @param symbol The symbol associated with the order.
         * @param orderId The unique identifier of the order.
         * @param dealt The signed quantity filled in this execution:
         *              - For Buy orders: Positive value indicates bought quantity.
         *              - For Sell orders: Negative value indicates sold quantity.
         * @param contra The signed quantity filled against in this execution:
         *               - For Buy orders: Negative value indicates contra sold quantity.
         *               - For Sell orders: Positive value indicates contra bought quantity.
         *
         * @note This method may be called multiple times for a single order as executions occur.
         * The `dealt` parameter represents the quantity filled in each execution, while `contra`
         * represents any contra quantity filled against the order.
         * The total quantity filled (`dealt`) in all executions will not exceed the original order quantity.
         */
        virtual void onFill(const Symbol &symbol, OrderId orderId, Quantity dealt, Quantity contra) = 0;

        /**
         * @brief Called when an order is terminated (fully filled, rejected, etc.).
         *
         * @param symbol The symbol associated with the order.
         * @param orderId The unique identifier of the order.
         * @param status The status of the order after termination.
         */
        virtual void onTerminated(const Symbol &symbol, OrderId orderId, DoneStatus status) = 0;
    };

    /// Interface for sending orders on a specific product.
    class OrderSender
    {
    protected:
        virtual ~OrderSender() = default;

    public:
        /**
         * @brief Sends an order to buy or sell a specified quantity of a product.
         *
         * @param side The side of the order (Buy or Sell).
         * @param price The price at which to execute the order.
         * @param qty The quantity of the product to buy (positive) or sell (negative).
         * @param tif The Time In Force (TIF) specifying how long the order remains active.
         * @return OrderId A unique identifier assigned to the order.
         *
         * @note This method is non-reentrant and always returns a valid OrderId.
         * Rejections or acknowledgments are communicated asynchronously through the
         * OrderStateObserver interface methods.
         */
        virtual OrderId sendOrder(Side side, Price price, Quantity qty, TIF tif) = 0;
    };

    /// Interface to retrieve an OrderSender for a particular symbol
    /// Represents a single exchange or venue
    class Gateway
    {
    public:
        virtual ~Gateway() = default;

        /// Returns an OrderSender for the specified symbol, registering the observer for updates
        virtual OrderSender &getOrderSender(const Symbol &, OrderStateObserver &) = 0;
    };

    //-------------------------------------------------------------------------------------------------------------------------------------
    // Helper Functions

    /// Converts Side enum to integer sign
    inline int sideToSign(Side side)
    {
        return static_cast<int>(side);
    }

    /// Converts DoneStatus enum to string representation
    inline const char *asString(const DoneStatus status)
    {
        switch (status)
        {
        case DoneStatus::Done:
            return "Done";
        case DoneStatus::Rejected:
            return "Rejected";
        case DoneStatus::InternalReject:
            return "InternalReject";
        default:
            return "<unknown>";
        }
    }

    /// Converts Side enum to string representation
    inline const char *asString(const Side side)
    {
        switch (side)
        {
        case Side::Buy:
            return "Buy";
        case Side::Sell:
            return "Sell";
        default:
            return "<unknown>";
        }
    }

} // namespace flow
