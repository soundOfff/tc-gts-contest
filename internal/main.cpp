/// Internal
#include <iostream>
#include "../strategy.hpp"

#include "event_loop.hpp"
#include "pub_internal.hpp"
#include "lp_sim.hpp"
#include "market_data_replayer.hpp"
#include "simple_risk_model.hpp"


using namespace std::literals::chrono_literals;

int main()
{
    std::string inputData = "../data/data.20240115.csv";

    EventLoop eventLoop{0ns};

    // Market Data infrastructure
    pub::Proxy<md::TopOfBook> mdProxy;
    pub::CacheSubscriber<md::TopOfBook> mdCache;
    pub::DirectConsumer<md::TopOfBook> mdPub(mdProxy), mdCachePub(mdCache);
    mdProxy.addFront(mdCachePub);
    MarketDataReplayer replayer(eventLoop, mdPub, std::cin);

    // Risk model
    SimpleRiskModel risk{mdCache};

    // Positions infrastructure
    pub::Proxy<Position> positionsProxy;
    pub::CacheSubscriber<Position> positionsCache;
    pub::DirectConsumer<Position> positionsPub(positionsProxy), positionsCachePub(positionsCache);
    positionsProxy.addFront(positionsCachePub);

    // Gateway infrastructure
    flow::LPSim lpSim(eventLoop, mdCache, positionsPub, {1ms, 1ms, 10s, 10e6});

    // Quick dispatch to start the event loop at the time of the first market data event
    eventLoop.postEvent(0ns, [&]()
                 { eventLoop.stop(0ns); });

    eventLoop.dispatch();

    // Creates and wires strategy
    auto strategy = createStrategy(eventLoop, lpSim, risk);
    pub::DirectConsumer<md::TopOfBook> mdStrategyPub(*strategy);
    mdProxy.addBack(mdStrategyPub);
    pub::DirectConsumer<Position> strategyPositionsPub(*strategy);
    positionsProxy.addBack(strategyPositionsPub);

    // Runs simulation
    eventLoop.dispatch();


    std::cout << "lastEventTime:" << eventLoop.getEventTime().count() << ",pnl:" << risk.getPnL(positionsCache) << " ,nop:" << risk.getNOP(positionsCache) << std::endl;

    return 0;
}