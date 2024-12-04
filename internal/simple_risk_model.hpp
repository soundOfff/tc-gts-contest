#pragma once

#include "../risk.hpp"
#include "../market_data.hpp"
#include "pub_internal.hpp"

class SimpleRiskModel : public Risk
{
public:
    using TopOfBookCache = pub::CacheSubscriber<md::TopOfBook>;

    explicit SimpleRiskModel(const TopOfBookCache &);

    virtual Price getFairPrice(const Asset &) const override;

private:
    const TopOfBookCache &m_tobCache;
};
