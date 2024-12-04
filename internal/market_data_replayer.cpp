/// Internal
#include "market_data_replayer.hpp"

#include <fstream>
#include <sstream>

#include <iostream> // TODO remove

namespace
{
    using TimestampNs = TimestampNs;
    using namespace std::literals::chrono_literals;
}

class MarketDataReplayer::Impl : public Replayable
{
public:
    Impl(EventLoop &eventLoop,
         pub::Publisher<md::TopOfBook> &publisher,
         std::istream &inputData) : m_publisher{publisher},
                                    m_inputData{inputData},
                                    m_records{},
                                    m_nextLine{0ns, "", {0, 0, 0, 0}}
    {
        readNextLine();
        eventLoop.add(*this);
    }

    TimestampNs getNextEventTime() const override { return m_nextLine.timestamp; }
    void dispatchNextEvent() override
    {
        auto startTime = getNextEventTime();
        if (startTime < TimestampNs::max())
        {
            do
            {
                publish();
                readNextLine();
            } while (getNextEventTime() == startTime);
            m_publisher.endBatch();
        }
    }

    void skip(TimestampNs ts) override
    {
        while (getNextEventTime() < ts)
        {
            readNextLine();
        }
    }

private:
    void publish()
    {
        auto [pos, inserted] = m_records.try_emplace(m_nextLine.symbol, std::make_pair(m_nextLine.book, nullptr));
        if (inserted)
        {
            pos->second.second = &m_publisher.createEntry(m_nextLine.symbol, pos->second.first);
        }
        else
        {
            pos->second.first = m_nextLine.book;
        }
        pos->second.second->publish();
    }

    void readNextLine()
    {
        std::string line;

        if (std::getline(m_inputData, line))
        {
            std::istringstream lineStream(line);
            std::string cell;

            // Read timestamp
            std::getline(lineStream, cell, ',');
            m_nextLine.timestamp = std::chrono::nanoseconds(std::stoull(cell));

            std::getline(lineStream, m_nextLine.symbol, ',');

            std::getline(lineStream, cell, ',');
            m_nextLine.book.bidSize = std::stod(cell);

            std::getline(lineStream, cell, ',');
            m_nextLine.book.bidPrice = std::stod(cell);

            std::getline(lineStream, cell, ',');
            m_nextLine.book.askSize = std::stod(cell);

            std::getline(lineStream, cell, ',');
            m_nextLine.book.askPrice = std::stod(cell);
        }
        else
        {
            m_nextLine.timestamp = TimestampNs::max();
        }
    }

private:
    pub::Publisher<md::TopOfBook> &m_publisher;
    std::istream &m_inputData;
    std::unordered_map<std::string, std::pair<md::TopOfBook, pub::PublisherEntry *>> m_records;

    struct
    {
        TimestampNs timestamp;
        std::string symbol;
        md::TopOfBook book;
    } m_nextLine;
};

MarketDataReplayer::MarketDataReplayer(EventLoop &eventLoop, pub::Publisher<md::TopOfBook> &publisher, std::istream &inputData)
    : m_impl(new Impl(eventLoop, publisher, inputData))
{
}

MarketDataReplayer::~MarketDataReplayer() = default;