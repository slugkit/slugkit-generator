#pragma once

#include <slugkit/generator/detail/indexes.hpp>
#include <slugkit/generator/dictionary.hpp>
#include <slugkit/generator/hash.hpp>

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
    [[nodiscard]] virtual auto Get(const EmojiGen::TagsType& include_tags, const EmojiGen::TagsType& exclude_tags) const
        -> FilteredDictionaryConstPtr = 0;

    [[nodiscard]] auto Filter(const Selector& selector) const -> FilteredDictionaryConstPtr {
        auto data = combined_index_.Query(selector);
        return std::make_shared<const FilteredDictionary>(
            words_, selector.GetCase(), std::move(data.words), data.max_length
        );
    }

    [[nodiscard]] auto Filter(const EmojiGen::TagsType& include_tags, const EmojiGen::TagsType& exclude_tags) const
        -> FilteredDictionaryConstPtr {
        auto data = combined_index_.Query(include_tags, exclude_tags);
        return std::make_shared<const FilteredDictionary>(
            words_, CaseType::kNone, std::move(data.words), data.max_length
        );
    }

    [[nodiscard]] auto GetTagDefinitions(std::string_view kind) const -> std::vector<TagDefinition> {
        return combined_index_.GetTagDefinitions(kind);
    }
};

struct FilteredDictionaryNoCache : FilteredDictionaryCacheBase {
    using FilteredDictionaryCacheBase::FilteredDictionaryCacheBase;

    [[nodiscard]] auto Get(const Selector& selector) const -> FilteredDictionaryConstPtr override {
        return Filter(selector);
    }

    [[nodiscard]] auto Get(const EmojiGen::TagsType& include_tags, const EmojiGen::TagsType& exclude_tags) const
        -> FilteredDictionaryConstPtr override {
        return Filter(include_tags, exclude_tags);
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

    std::int64_t GetTagsHash(const EmojiGen::TagsType& include_tags, const EmojiGen::TagsType& exclude_tags) const {
        auto seed = include_tags.size() + exclude_tags.size();
        for (const auto& tag : include_tags) {
            boost::hash_combine(seed, StrHash(tag.data(), tag.size()));
        }
        for (const auto& tag : exclude_tags) {
            boost::hash_combine(seed, StrHash(tag.data(), tag.size()));
        }
        return seed;
    }

    [[nodiscard]] auto Get(const EmojiGen::TagsType& include_tags, const EmojiGen::TagsType& exclude_tags) const
        -> FilteredDictionaryConstPtr override {
        auto hash = GetTagsHash(include_tags, exclude_tags);
        auto dictionary = cache_.Get(hash);
        if (dictionary) {
            return dictionary.value();
        }
        auto filtered_dictionary = Filter(include_tags, exclude_tags);
        cache_.Put(hash, filtered_dictionary);
        return filtered_dictionary;
    }
};

}  // namespace slugkit::generator::detail
