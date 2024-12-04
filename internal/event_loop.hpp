/// Internal
#pragma once

#include "../event_dispatcher.hpp"

#include <memory>

class Replayable
{
public:
    virtual ~Replayable()
    {
    }

    virtual TimestampNs getNextEventTime() const = 0;
    virtual void dispatchNextEvent() = 0;
    virtual void skip(TimestampNs) = 0;
};

class EventLoop : public EventDispatcher
{
public:
    explicit EventLoop(TimestampNs start);
    virtual ~EventLoop();

    EventLoop(const EventLoop &) = delete;
    EventLoop &operator=(const EventLoop &) = delete;

public:
    TimestampNs getEventTime() const override final;
    void postEvent(TimestampNs deltaTimeNs, const Event &event) override final;

public:
    void add(Replayable &);
    void dispatch();
    void stop(TimestampNs deltaTimeNs);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
