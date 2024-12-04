/// Public
#pragma once

#include <chrono>
#include <functional>

using TimestampNs = std::chrono::nanoseconds;
using Event = std::function<void()>;

/**
 * @brief Abstract base class for an event dispatcher.
 *
 * The EventDispatcher serves as the main abstraction of an event loop, managing the execution of tasks and events based on time.
 * It provides access to the underlying clock, allowing for retrieval of the current time. Additionally, it offers a mechanism to
 * schedule future tasks. When multiple events are scheduled to occur at the same point in time, they are executed in the order in
 * which they were scheduled. This ensures a predictable and orderly handling of time-based events.
 */
struct EventDispatcher
{
    /**
     * @brief Virtual destructor.
     *
     * Ensures derived class destructors are called properly.
     */
    virtual ~EventDispatcher() = default;

    /**
     * @brief Gets the current event time.
     *
     * @return TimestampNs The current time in nanoseconds.
     */
    virtual TimestampNs getEventTime() const = 0;

    /**
     * @brief Schedules an event to be executed after a specified duration.
     *
     * @param deltaTimeNs The duration after which the event should be executed, in nanoseconds.
     * @param event The event to be executed.
     */
    virtual void postEvent(TimestampNs deltaTimeNs, const Event &event) = 0;
};
