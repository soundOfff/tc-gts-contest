#pragma once

#include "../market_data.hpp"

#include "event_loop.hpp"
#include "pub_internal.hpp"

#include <istream>

class MarketDataReplayer
{
public:
    MarketDataReplayer(EventLoop &, pub::Publisher<md::TopOfBook> &, std::istream &inputData);

    ~MarketDataReplayer();

    MarketDataReplayer(const MarketDataReplayer &) = delete;
    MarketDataReplayer &operator=(const MarketDataReplayer &) = delete;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};