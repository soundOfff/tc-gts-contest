/// Public
#pragma once

#include <functional>
#include <string_view>

namespace pub
{
    /**
     * @brief A generic consumer interface for subscribing to records of type `Record`.
     *
     * This class defines a generic interface for consumers that can subscribe to records
     * of a specific type (`Record`). It provides a callback mechanism (`Callback`) that
     * notifies subscribers of updates to records on a given topic.
     */
    template <typename Record>
    class Consumer
    {
    public:
        /// Notification of a modified record. It is safe to use the referenced record outside of the callback.
        using Callback = std::function<void(std::string_view topic, const Record &record)>;

        virtual ~Consumer() = default;

        /**
         * @brief Subscribe to updates for records on a specific topic.
         *
         * @param topic The topic to subscribe to.
         * @param cb The callback function to be invoked when a record on the topic is updated.
         */
        virtual void subscribe(std::string_view topic, Callback cb) = 0;
    };

    /**
     * @brief A generic subscriber interface for handling updates and batch processing of records.
     *
     * This class defines an interface for subscribers that receive notifications of new records
     * (`notify`) and batch completion (`endOfBatch`) from a `Consumer` of type `Record`.
     */
    template <typename Record>
    class Subscriber
    {
    public:
        virtual ~Subscriber() = default;

        /**
         * @brief Notification of a new record.
         *
         * This method is called by the `Consumer` to notify the subscriber of a new record
         * available on a specific topic.
         *
         * @param consumer The consumer providing the record updates.
         * @param topic The topic associated with the new record.
         * @param record The new record being provided.
         */
        virtual void notify(Consumer<Record> &consumer, std::string_view topic, const Record &record) = 0;

        /**
         * @brief Notification that a batch of concurrent updates is done.
         *
         * This method is called by the `Consumer` to notify the subscriber when a batch of
         * concurrent updates to records has been completed.
         *
         * @param consumer The consumer that completed the batch updates.
         */
        virtual void endOfBatch(Consumer<Record> &consumer) = 0;
    };

} // namespace pub
