#pragma once

#include <slugkit/generator/detail/indexes.hpp>
#include <slugkit/generator/dictionary.hpp>

#include <userver/cache/nway_lru_cache.hpp>

namespace slugkit::generator::detail {

struct FilteredDictionaryCacheBase {
    using WordContainer = std::vector<Word>;
    using WordContainerPtr = std::shared_ptr<WordContainer>;

    WordContainerPtr words_;
    CombinedIndex combined_index_;

    FilteredDictionaryCacheBase(WordContainerPtr words)
        : words_{std::move(words)}
        , combined_index_{*words_} {
    }

    virtual ~FilteredDictionaryCacheBase() = default;
    [[nodiscard]] virtual auto Get(const Selector& selector) const -> FilteredDictionaryConstPtr = 0;

    [[nodiscard]] auto Filter(const Selector& selector) const -> FilteredDictionaryConstPtr {
        auto data = combined_index_.Query(selector);
        return std::make_shared<const FilteredDictionary>(words_, selector, std::move(data.words), data.max_length);
    }
};

struct FilteredDictionaryNoCache : FilteredDictionaryCacheBase {
    using FilteredDictionaryCacheBase::FilteredDictionaryCacheBase;

    [[nodiscard]] auto Get(const Selector& selector) const -> FilteredDictionaryConstPtr override {
        return Filter(selector);
    }
};

struct FilteredDictionaryCache : FilteredDictionaryCacheBase {
    using LruCache = userver::cache::NWayLRU<std::int64_t, FilteredDictionaryConstPtr>;

    static constexpr std::size_t kWays = 16UL;
    static constexpr std::size_t kWaySize = 1024UL;

    mutable LruCache cache_;

    FilteredDictionaryCache(WordContainerPtr words)
        : FilteredDictionaryCacheBase(words)
        , cache_(kWays, kWaySize) {
    }

    [[nodiscard]] auto Get(const Selector& selector) const -> FilteredDictionaryConstPtr override {
        auto hash = selector.GetHash();
        // lookup cache
        auto dictionary = cache_.Get(hash);
        if (dictionary) {
            return dictionary.value();
        }
        auto filtered_dictionary = Filter(selector);
        // insert to cache
        cache_.Put(hash, filtered_dictionary);
        return filtered_dictionary;
    }
};

}  // namespace slugkit::generator::detail
