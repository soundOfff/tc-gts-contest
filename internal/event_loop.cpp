/// Internal
#include "event_loop.hpp"

#include <deque>
#include <queue>
#include <tuple>
#include <unordered_set>

using namespace std::literals::chrono_literals;

class EventLoop::Impl
{
private:
    static constexpr size_t MAX_REPLAYABLES = 4096;
    using EventId = int64_t;

    struct TimedEvent
    {

        TimedEvent()
        {
        }

        TimedEvent(EventId eventId, TimestampNs expireTime, const Event &event)
            : m_eventId{eventId},
              m_expireTime{expireTime},
              m_event{event}
        {
        }

        inline void dispatch()
        {
            m_event();
        }

        inline bool operator<(const TimedEvent &other) const
        {
            return std::tie(m_expireTime, m_eventId) > std::tie(other.m_expireTime, other.m_eventId);
        }

        EventId m_eventId;
        TimestampNs m_expireTime;
        Event m_event;
    };

    class ReplayableDispatcher
    {

    public:
        ReplayableDispatcher()
            : m_replayable(nullptr),
              m_eventDispatcher(nullptr)
        {
        }

        ReplayableDispatcher(Impl &eventLoop, Replayable &replayable)
            : m_replayable(&replayable),
              m_eventDispatcher(&eventLoop)
        {
            start();
        }

    private:
        inline void start()
        {
            m_replayable->skip(m_eventDispatcher->getEventTime());
            postNextEvent();
        }

        inline void dispatch()
        {
            m_replayable->dispatchNextEvent();
            postNextEvent();
        }

        inline void postNextEvent()
        {
            const auto now = m_eventDispatcher->getEventTime();
            auto next = m_replayable->getNextEventTime();
            if (next < TimestampNs::max())
            {
                next = (next > now) ? next - now : 0ns;
                m_eventDispatcher->postEvent(next, [this]()
                                             { dispatch(); });
            }
            else
            {
                m_eventDispatcher->onReplayableDone(*m_replayable);
            }
        }

    private:
        Replayable *m_replayable;
        Impl *m_eventDispatcher;
    };

public:
    Impl(TimestampNs start)
        : m_now(start),
          m_futureEvents(),
          m_chores(),
          m_replayableDispatchers(),
          m_activeReplayableDispatchersCount(0),
          m_lastEventId(0),
          m_enable(true)
    {
        m_replayableDispatchers.reserve(MAX_REPLAYABLES);
    }

public:
    void add(Replayable &replayable)
    {
        if (m_replayableDispatchers.size() < MAX_REPLAYABLES)
        {
            m_replayableDispatchers.emplace_back(*this, replayable);
            ++m_activeReplayableDispatchersCount;
        }
        else
        {
            throw std::runtime_error("EventLoop::add. Replayables limit reached.");
        }
    }

    void onReplayableDone(Replayable &)
    {
        if (--m_activeReplayableDispatchersCount == 0)
        {
            stop(0ns);
        }
    }

public:
    inline TimestampNs getEventTime() const
    {
        return m_now;
    }

    void dispatch(bool fastForwardTillNextFutureEvent)
    {
        m_enable = true;

        if (fastForwardTillNextFutureEvent && !m_futureEvents.empty())
        {
            m_now = m_futureEvents.top().m_expireTime;
        }

        while (m_enable & ((!m_futureEvents.empty()) | (!m_chores.empty())))
        {
            dispatchChores();
            dispatchNextFutureEvent();
        }
    }

    void stop(TimestampNs deltaTimeNs)
    {
        addFutureEvent(getEventTime() + deltaTimeNs, [this]()
                       { m_enable = false; }, std::numeric_limits<EventId>::max());
    }

    EventId postEvent(TimestampNs deltaTimeNs, const Event &event)
    {
        EventId eventId = m_lastEventId += 2;
        if (deltaTimeNs == 0ns)
        {
            m_chores.emplace_back(eventId, event);
        }
        else
        {
            ++eventId;
            addFutureEvent(getEventTime() + deltaTimeNs, event, eventId);
        }
        return eventId;
    }

private:
    void dispatchChores()
    {
        while (m_enable && !m_chores.empty())
        {
            auto event = std::get<1>(m_chores.front());
            m_chores.pop_front();
            event();
        }
    }

    void dispatchNextFutureEvent()
    {
        if (m_enable && !m_futureEvents.empty())
        {
            TimedEvent event = m_futureEvents.top();
            m_futureEvents.pop();

            m_now = event.m_expireTime;
            event.dispatch();
        }
    }

    void addFutureEvent(TimestampNs ts, const Event &event, EventId eventId)
    {
        m_futureEvents.emplace(eventId, ts, event);
    }

private:
    TimestampNs m_now;
    std::priority_queue<TimedEvent> m_futureEvents;
    std::deque<std::tuple<EventId, Event>> m_chores;
    std::vector<ReplayableDispatcher> m_replayableDispatchers;
    int m_activeReplayableDispatchersCount;
    EventId m_lastEventId;
    bool m_enable;
};

EventLoop::EventLoop(TimestampNs start)
    : m_impl(new Impl(start))
{
}

EventLoop::~EventLoop() = default;

void EventLoop::add(Replayable &replayable)
{
    m_impl->add(replayable);
}

TimestampNs EventLoop::getEventTime() const
{
    return m_impl->getEventTime();
}

void EventLoop::dispatch()
{
    m_impl->dispatch(true);
}

void EventLoop::stop(TimestampNs deltaTimeNs)
{
    m_impl->stop(deltaTimeNs);
}

void EventLoop::postEvent(TimestampNs deltaTimeNs, const Event &event)
{
    m_impl->postEvent(deltaTimeNs, event);
}
