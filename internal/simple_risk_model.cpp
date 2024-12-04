/// Internal
#include "simple_risk_model.hpp"

#include <limits>

SimpleRiskModel::SimpleRiskModel(const TopOfBookCache &tobCache)
    : m_tobCache{tobCache}
{
}

Price SimpleRiskModel::getFairPrice(const Asset &asset) const
{
    if (asset == "USD")
        return 1;
        
    auto it = m_tobCache.getCachedRecord(asset + "/USD");
    if (it)
    {
        return (it->bidPrice + it->askPrice) / 2;
    }

    it = m_tobCache.getCachedRecord(std::string("USD/") + asset);
    if (it)
    {
        return Price{2.0} / (it->bidPrice + it->askPrice);
    }

    return std::numeric_limits<Price>::quiet_NaN();
}
