#pragma once

/// @file slugkit/compat/lru_cache.hpp
/// @brief Compat shim for userver::cache::NWayLRU.
///
/// The cache is pure memoization of dictionary filtering, so eviction policy
/// never affects generated output — any correct cache works. The standalone
/// variant is a simple sharded LRU matching the NWayLRU API surface the
/// generator uses: NWayLRU(ways, way_size), Get(key) -> std::optional<Value>,
/// Put(key, value).

#ifdef SLUGKIT_USE_USERVER

#include <userver/cache/nway_lru_cache.hpp>

namespace slugkit::compat {
template <typename Key, typename Value>
using NWayLRU = userver::cache::NWayLRU<Key, Value>;
}  // namespace slugkit::compat

#else

#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace slugkit::compat {

template <typename Key, typename Value>
class NWayLRU {
public:
    NWayLRU(std::size_t ways, std::size_t way_size)
        : ways_(ways ? ways : 1), way_size_(way_size ? way_size : 1) {
        // Each Shard holds a std::mutex, which is neither movable nor copyable, so the shard
        // vector cannot be grown via resize(); hold the shards behind unique_ptr and build them
        // in place instead.
        shards_.reserve(ways_);
        for (std::size_t i = 0; i < ways_; ++i) {
            shards_.push_back(std::make_unique<Shard>());
        }
    }

    std::optional<Value> Get(const Key& key) {
        auto& shard = ShardFor(key);
        std::lock_guard lock(shard.mutex);
        auto it = shard.index.find(key);
        if (it == shard.index.end()) {
            return std::nullopt;
        }
        shard.order.splice(shard.order.begin(), shard.order, it->second);
        return it->second->second;
    }

    void Put(const Key& key, Value value) {
        auto& shard = ShardFor(key);
        std::lock_guard lock(shard.mutex);
        auto it = shard.index.find(key);
        if (it != shard.index.end()) {
            it->second->second = std::move(value);
            shard.order.splice(shard.order.begin(), shard.order, it->second);
            return;
        }
        shard.order.emplace_front(key, std::move(value));
        shard.index[key] = shard.order.begin();
        if (shard.order.size() > way_size_) {
            shard.index.erase(shard.order.back().first);
            shard.order.pop_back();
        }
    }

private:
    using Entry = std::pair<Key, Value>;
    struct Shard {
        std::list<Entry> order;
        std::unordered_map<Key, typename std::list<Entry>::iterator> index;
        std::mutex mutex;
    };

    Shard& ShardFor(const Key& key) { return *shards_[std::hash<Key>{}(key) % ways_]; }

    std::size_t ways_;
    std::size_t way_size_;
    std::vector<std::unique_ptr<Shard>> shards_;
};

}  // namespace slugkit::compat

#endif  // SLUGKIT_USE_USERVER
