/// Internal
#pragma once

#include "../event_dispatcher.hpp"
#include "../flow.hpp"
#include "../market_data.hpp"
#include "../risk.hpp"
#include "pub_internal.hpp"


#include <memory>

namespace flow
{

    class LPSim : public Gateway
    {
    public:
        using TopOfBookCache = pub::CacheSubscriber<md::TopOfBook>;
        using PositionsPublisher = pub::Publisher<Position>;

        struct Settings
        {
            TimestampNs inboundDelay;
            TimestampNs outboundDelay;
            TimestampNs minOrderGap;
            Quantity maxNOP; 
        };

        LPSim(EventDispatcher &, const TopOfBookCache &, PositionsPublisher &, const Settings &);
        virtual ~LPSim();

        LPSim(const LPSim &) = delete;
        LPSim &operator=(const LPSim &) = delete;

    public:
        OrderSender &getOrderSender(const Symbol&, OrderStateObserver &) override;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace flow
