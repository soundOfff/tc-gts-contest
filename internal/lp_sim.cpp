/// Internal
#include "lp_sim.hpp"

#include "simple_risk_model.hpp"

#include <cmath>
#include <map>
#include <tuple>
#include <string>

namespace flow
{

    class LPSim::Impl
    {
    private:
        struct Order
        {
            OrderId orderId;
            Side side;
            Price price;
            Quantity qty;
            TIF tif;
        };

    public:
        bool isPriceImprovementEnabled() const
        {
            return true;
        }

        TimestampNs getInboundDelay() const
        {
            return m_settings.inboundDelay;
        }

        TimestampNs getOutboundDelay() const
        {
            return m_settings.outboundDelay;
        }

        TimestampNs getMinOrderGap() const
        {
            return m_settings.minOrderGap;
        }

        Quantity getMaxNOP() const
        {
            return m_settings.maxNOP;
        }

        EventDispatcher &getEventDispatcher()
        {
            return m_eventDispatcher;
        }

        Quantity getNOP() const
        {
            return m_risk.getNOP(m_positions);
        }

        PositionsPublisher &getPositionsPublisher()
        {
            return m_positionsPub;
        }

        const auto *getCachedRecord(const Symbol &symbol)
        {
            return m_tobCache.getCachedRecord(symbol);
        }

        OrderId getNextOrderId()
        {
            return ++m_lastOrderId;
        }

    private:
        class ExecutorImpl : public OrderSender
        {
        public:
            ExecutorImpl(Impl &service, const Symbol &symbol, OrderStateObserver &observer)
                : m_service{service},
                  m_observer{observer},
                  m_symbol{symbol},
                  m_baseAsset{getBaseAsset(m_symbol)},
                  m_quoteAsset{getQuoteAsset(m_symbol)},
                  m_basePosition{m_service.getPosition(m_baseAsset)},
                  m_quotePosition{m_service.getPosition(m_quoteAsset)},
                  m_basePositionEntry{m_service.getPositionEntry(m_baseAsset)},
                  m_quotePositionEntry{m_service.getPositionEntry(m_quoteAsset)},
                  m_book{nullptr},
                  m_lastOrderSendTime{}
            {
            }

            ExecutorImpl(const ExecutorImpl &) = delete;
            ExecutorImpl &operator=(const ExecutorImpl &) = delete;

        public:
            // Order Flow
            OrderId sendOrder(Side side, Price price, Quantity qty, TIF tif) override
            {
                if (m_book == nullptr)
                {
                    m_book = m_service.getCachedRecord(m_symbol);
                }

                auto orderId = m_service.getNextOrderId();
                m_service.getEventDispatcher().postEvent(
                    m_service.getInboundDelay(),
                    [=]()
                    { process(Order{orderId, side, price, qty, tif}); });

                return orderId;
            }

        public:
            void process(const Order &order)
            {
                ackOrder(order);
                DoneStatus doneStatus;

                if (validateOrder(order))
                {
                    m_lastOrderSendTime = m_service.getEventDispatcher().getEventTime();
                    auto qtyAtTop = (order.side == Side::Buy) ? m_book->askSize : m_book->bidSize;
                    auto topPrice = (order.side == Side::Buy) ? m_book->askPrice : m_book->bidPrice;
                    doneStatus = agress(order, qtyAtTop, topPrice);
                }
                else
                {
                    doneStatus = DoneStatus::InternalReject;
                }
                enqueOrderDone(order, doneStatus);
            }

            bool validateOrder(const Order &order) const
            {
                return m_book &&
                       order.tif == TIF::IOC &&
                       order.qty > 0 &&
                       m_service.getEventDispatcher().getEventTime() - m_lastOrderSendTime >= m_service.getMinOrderGap();
            }

        private:
            void ackOrder(const Order &order) const
            {
                m_observer.onAck(m_symbol, order.orderId, order.side, order.price, order.qty, order.tif);
            }

            // This logic for now is quite simple. It only takes liquidity from top of book;
            DoneStatus agress(const Order &order, Quantity qtyAtTop, Price topPrice) const
            {
                static constexpr Price priceTolerance = 1e-8;

                const int sideSign = sideToSign(order.side);

                if (std::isnan(topPrice) || order.price * sideSign < topPrice * sideSign - priceTolerance)
                {
                    return DoneStatus::Done;
                }

                const auto matchedPrice = (std::isnan(order.price) || m_service.isPriceImprovementEnabled()) ? topPrice : order.price;
                const auto matchedQty = std::min(qtyAtTop, order.qty);

                if (matchedQty > 0)
                {
                    const auto dealt = sideSign * matchedQty;
                    const auto contra = -dealt * matchedPrice;
                    if (!validateNOPChange(dealt, contra))
                    {
                        return DoneStatus::InternalReject;
                    }
                    enqueueFill(order.orderId, dealt, contra);
                }
                return DoneStatus::Done;
            }

            bool validateNOPChange(Quantity dealt, Quantity contra) const
            {
                if (std::isnan(dealt) || std::isnan(contra))
                {
                    return false;
                }

                auto currentNOP = m_service.getNOP();
                m_basePosition += dealt;
                m_quotePosition += contra;
                auto newNOP = m_service.getNOP();
                m_basePosition -= dealt;
                m_quotePosition -= contra;

                return newNOP < currentNOP || newNOP <= m_service.getMaxNOP();
            }

            void enqueueFill(OrderId orderId, Quantity dealt, Quantity contra) const
            {
                m_service.getEventDispatcher().postEvent(
                    m_service.getOutboundDelay(),
                    [=]()
                    {
                    m_basePosition += dealt;
                    m_quotePosition += contra;

                    m_basePositionEntry.publish();
                    m_quotePositionEntry.publish();

                    m_observer.onFill(m_symbol, orderId, dealt, contra);

                    m_service.getPositionsPublisher().endBatch(); });
            }

            void enqueOrderDone(const Order &order, DoneStatus status) const
            {
                m_service.getEventDispatcher().postEvent(
                    m_service.getOutboundDelay(),
                    [orderId = order.orderId, status, this]()
                    {
                        m_observer.onTerminated(m_symbol, orderId, status);
                    });
            }

        private:
            Impl &m_service;
            OrderStateObserver &m_observer;

            Symbol m_symbol;

            Asset m_baseAsset;
            Asset m_quoteAsset;

            Position &m_basePosition;
            Position &m_quotePosition;

            pub::PublisherEntry &m_basePositionEntry;
            pub::PublisherEntry &m_quotePositionEntry;

            const md::TopOfBook *m_book;

            TimestampNs m_lastOrderSendTime;
        };

    public:
        Impl(EventDispatcher &eventDispatcher, const TopOfBookCache &tobCache, PositionsPublisher &positionsPub, const Settings &settings)
            : m_eventDispatcher{eventDispatcher},
              m_tobCache{tobCache},
              m_risk{m_tobCache},
              m_positionsPub{positionsPub},
              m_positions{},
              m_executors{},
              m_settings{settings},
              m_lastOrderId{0}
        {
        }

        ~Impl()
        {
            for (auto entry : m_executors)
            {
                delete entry.second;
            }
        }

    public:
        OrderSender &getOrderSender(const Symbol &symbol, OrderStateObserver &observer)
        {
            auto it = m_executors.find(std::make_tuple(symbol, &observer));
            if (it == m_executors.end())
            {
                it = m_executors.emplace(std::piecewise_construct,
                                         std::forward_as_tuple(symbol, &observer),
                                         std::forward_as_tuple(createExecutor(symbol, observer)))
                         .first;
            }

            return *it->second;
        }

    private:
        ExecutorImpl *createExecutor(const Symbol &symbol, OrderStateObserver &observer)
        {
            return new ExecutorImpl(*this, symbol, observer);
        }

        Position &getPosition(const Asset &asset)
        {
            return m_positions[asset];
        }

        pub::PublisherEntry &getPositionEntry(const Asset &asset)
        {
            return m_positionsPub.createEntry(asset, getPosition(asset));
        }

    private:
        EventDispatcher &m_eventDispatcher;

        const TopOfBookCache &m_tobCache;
        SimpleRiskModel m_risk;
        PositionsPublisher &m_positionsPub;

        std::unordered_map<Asset, Quantity> m_positions;
        std::map<std::tuple<Symbol, OrderStateObserver *>, ExecutorImpl *> m_executors;

        Settings m_settings;
        OrderId m_lastOrderId;
    };

    LPSim::LPSim(EventDispatcher &eventDispatcher, const TopOfBookCache &tobCache, PositionsPublisher &positionsPub, const Settings &settings)
        : m_impl{new Impl(eventDispatcher, tobCache, positionsPub, settings)}
    {
    }

    LPSim::~LPSim() = default;

    OrderSender &LPSim::getOrderSender(const Symbol &symbol,
                                       OrderStateObserver &observer)
    {
        return m_impl->getOrderSender(symbol, observer);
    }

} // namespace flow
