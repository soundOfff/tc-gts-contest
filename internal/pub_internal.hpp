#pragma once

#include "../pub.hpp"

#include <cassert>
#include <memory>
#include <utility>
#include <unordered_map>

namespace pub
{

    class PublisherEntry
    {
    public:
        virtual ~PublisherEntry() = default;

        virtual void publish() = 0;
    };

    template <typename RecordT>
    class Publisher
    {
    public:
        using Record = RecordT;

        virtual ~Publisher() = default;
        virtual PublisherEntry &createEntry(std::string_view topic, const Record &r) = 0;
        virtual void endBatch() = 0;
    };

    template <typename RecordT>
    class DirectConsumer : public Consumer<RecordT>, public Publisher<RecordT>
    {
    public:
        using Record = RecordT;
        using typename Consumer<Record>::Callback;

        explicit DirectConsumer(Subscriber<Record> &subscriber) : m_subscriber{subscriber} {}
        DirectConsumer(const DirectConsumer<RecordT> &) = delete;
        DirectConsumer<RecordT> &operator=(const DirectConsumer<RecordT> &) = delete;

        class Entry : public PublisherEntry
        {
        public:
            Entry(std::string_view topic, bool &got_updates, const Callback &callback) : m_topic{topic}, m_got_updates{got_updates}, m_callback{callback} {}

        public:
            void publish()
            {
                assert(m_data);
                if (m_callback != nullptr)
                {
                    m_callback(m_topic, *m_data);
                    m_got_updates = true;
                }
            }
            std::string_view getTopic() const { return m_topic; }

            void setCallback(const Callback &cb) { m_callback = cb; }
            void setData(const Record &record)
            {
                m_data = &record;
            }

        private:
            const std::string m_topic;
            const Record *m_data = nullptr;
            bool &m_got_updates;
            Callback m_callback;
        };

        Entry &createEntry(std::string_view topic, const Record &data) override
        {
            auto &e = getCreateEntry(topic, data);
            m_subscriber.notify(*this, topic, data);
            return e;
        }

        void endBatch() override
        {
            if (std::exchange(m_updates_received, false))
                m_subscriber.endOfBatch(*this);
        }

        Entry &getCreateEntry(std::string_view topic, const Record &data)
        {

            auto [pos, inserted] = m_entries.try_emplace(std::string(topic), nullptr);
            if (inserted)
                pos->second = std::make_unique<Entry>(topic, m_updates_received, [](std::string_view topic, const Record &record) {});
            pos->second->setData(data);
            return *pos->second;
        }

        void subscribe(std::string_view topic, Callback cb) override
        {
            if (auto [pos, inserted] = m_entries.try_emplace(std::string(topic), nullptr); !inserted)
            {
                pos->second->setCallback(cb);
            }
            else
            { // entry was created
                pos->second = std::make_unique<Entry>(topic, m_updates_received, cb);
            }
        }

    private:
        bool m_updates_received = false;
        Subscriber<Record> &m_subscriber;
        std::unordered_map<std::string, std::unique_ptr<Entry>> m_entries;
    };

    /// Subscriber which caches pointers to each topic record for later retrieval.
    template <typename Record>
    class CacheSubscriber : public Subscriber<Record>
    {
    public:
        CacheSubscriber() = default;
        CacheSubscriber(const CacheSubscriber<Record> &) = delete;
        CacheSubscriber<Record> &operator=(const CacheSubscriber<Record> &) = delete;

        void notify(Consumer<Record> &, std::string_view topic, const Record &record) override
        {
            m_cache.insert_or_assign(std::string(topic), &record);
        }

        void endOfBatch(Consumer<Record> &) override {}

        const Record *getCachedRecord(std::string_view topic) const
        {
            if (auto pos = m_cache.find(std::string(topic)); pos != m_cache.end())
                return pos->second;
            return nullptr;
        }

        auto begin() const
        {
            return m_cache.begin();
        }

        auto end() const
        {
            return m_cache.end();
        }

    private:
        std::unordered_map<std::string, const Record *> m_cache;
    };

    template <typename Record>
    class Proxy : public Subscriber<Record>
    {
    public:
        Proxy() = default;
        Proxy(const Proxy<Record> &) = delete;
        Proxy<Record> &operator=(const Proxy<Record> &) = delete;

        void notify(Consumer<Record> &consumer, std::string_view topic, const Record &record) override
        {
            if (auto [pos, inserted] = m_entries.try_emplace(std::string(topic), record); inserted)
            {
                for (auto *publisher : m_publishers)
                    pos->second.m_entries.push_back(&publisher->createEntry(topic, record));
                consumer.subscribe(topic,
                                   [&entry = pos->second](std::string_view topic, const Record &record)
                                   { entry.onUpdate(topic, record); });
            }
        }

        void endOfBatch(Consumer<Record> &) override
        {
            for (auto *p : m_publishers)
                p->endBatch();
        }

        template <typename Where>
        void add(Publisher<Record> &p, Where where)
        {
            m_publishers.emplace(where(m_publishers), &p);
            for (auto &[sym, e] : m_entries)
                e.m_entries.emplace(where(e.m_entries), &p.createEntry(sym.c_str(), *e.m_record));
        }

        void addFront(Publisher<Record> &p)
        {
            add(p, [](auto &c)
                {
            using std::cbegin;
            return cbegin(c); });
        }

        void addBack(Publisher<Record> &p)
        {
            add(p, [](auto &c)
                {
            using std::cend;
            return cend(c); });
        }

    private:
        struct TopicEntry
        {
            TopicEntry(const Record &record) : m_record{&record} {}

            void onUpdate(std::string_view, const Record &)
            {
                for (const auto &p : m_entries)
                    p->publish();
            }

            std::vector<PublisherEntry *> m_entries;
            const Record *m_record;
        };

        std::unordered_map<std::string, TopicEntry> m_entries;
        std::vector<Publisher<Record> *> m_publishers;
    };

} // namespace pub
