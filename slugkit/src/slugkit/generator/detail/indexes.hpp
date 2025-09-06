#pragma once

#include <slugkit/generator/dictionary_types.hpp>
#include <slugkit/generator/pattern.hpp>
#include <slugkit/generator/types.hpp>

#include <map>
#include <numeric>
#include <set>
#include <vector>

namespace slugkit::generator::detail {

/// @brief A container that stores ranges of values.
template <typename UnderlyingIterator>
class RangeContainer {
public:
    using RangeType = std::pair<UnderlyingIterator, UnderlyingIterator>;
    using StorageType = std::vector<RangeType>;

    class ConstIterator {
    public:
        using value_type = typename std::iterator_traits<UnderlyingIterator>::value_type;
        ConstIterator() = default;

        ConstIterator(const RangeContainer& container)
            : container_(&container)
            , range_it_(container.ranges_.begin())
            , value_it_(!container.ranges_.empty() ? container.ranges_.begin()->first : UnderlyingIterator()) {
        }

        auto GetUnderlying() const -> UnderlyingIterator {
            return value_it_;
        }

        auto operator==(const ConstIterator& other) const -> bool {
            return value_it_ == other.value_it_;
        }

        auto operator!=(const ConstIterator& other) const -> bool {
            return !(*this == other);
        }

        auto operator++() -> ConstIterator& {
            ++value_it_;
            if (value_it_ == range_it_->second) {
                ++range_it_;
                if (range_it_ != container_->ranges_.end()) {
                    value_it_ = range_it_->first;
                } else {
                    value_it_ = UnderlyingIterator();
                }
            }
            return *this;
        }

        auto operator++(int) -> ConstIterator {
            auto result = *this;
            ++(*this);
            return result;
        }

        auto operator*() const -> value_type {
            return *value_it_;
        }

        auto operator->() const -> UnderlyingIterator {
            return value_it_;
        }

    private:
        const RangeContainer* container_ = nullptr;
        StorageType::const_iterator range_it_{};
        UnderlyingIterator value_it_{};
    };

public:
    RangeContainer() = default;
    RangeContainer(UnderlyingIterator begin, UnderlyingIterator end)
        : ranges_() {
        AddRange(begin, end);
    }

    void AddRange(UnderlyingIterator begin, UnderlyingIterator end) {
        if (begin == end) {
            return;
        }
        ranges_.emplace_back(begin, end);
    }

    auto size() const -> std::size_t {
        return std::accumulate(ranges_.begin(), ranges_.end(), 0, [](std::size_t sum, const auto& range) {
            return sum + std::distance(range.first, range.second);
        });
    }

    auto empty() const -> bool {
        return ranges_.empty();
    }

    auto begin() const -> ConstIterator {
        return ConstIterator(*this);
    }

    auto end() const -> ConstIterator {
        return ConstIterator();
    }

    auto front() const -> UnderlyingIterator {
        return ranges_.front().first;
    }

    auto back() const -> UnderlyingIterator {
        return ranges_.back().second;
    }

private:
    friend class ConstIterator;

    std::vector<RangeType> ranges_;
};

/// @brief An index that stores words by their length.
struct LengthIndex {
    using WordContainer = std::vector<Word>;
    using Iterator = WordContainer::const_iterator;
    using FilteredWords = std::vector<Iterator>;
    using LengthMap = std::multimap<std::size_t, Iterator>;

    struct QueryResult {
        RangeContainer<LengthMap::const_iterator> words;
        std::size_t max_length;

        auto ToSet() const -> FilteredWords {
            FilteredWords result;
            for (auto it = words.begin(); it != words.end(); ++it) {
                result.push_back(it->second);
            }
            std::sort(result.begin(), result.end());
            return result;
        }

        auto MaxLength() const -> std::size_t {
            return max_length;
        }

        auto MinLength() const -> std::size_t {
            if (words.empty()) {
                return 0;
            }
            return words.begin()->first;
        }
    };

    LengthMap lengths;

    LengthIndex() = default;

    LengthIndex(const WordContainer& words);
    LengthIndex(const FilteredWords& words);

    // This method is used to add words to the index in parallel with tags index.
    void Add(Iterator it);

    [[nodiscard]] auto MaxLength() const -> std::size_t;

    [[nodiscard]] auto Eq(std::size_t length) const -> QueryResult;
    [[nodiscard]] auto Ne(std::size_t length) const -> QueryResult;

    [[nodiscard]] auto Lt(std::size_t length) const -> QueryResult;
    [[nodiscard]] auto Le(std::size_t length) const -> QueryResult;

    [[nodiscard]] auto Gt(std::size_t length) const -> QueryResult;
    [[nodiscard]] auto Ge(std::size_t length) const -> QueryResult;

    [[nodiscard]] auto Query(const Selector& selector) const -> QueryResult;

private:
    [[nodiscard]] auto QueryWithSizeLimit(const SizeLimit& size_limit) const -> QueryResult;
    [[nodiscard]] auto QueryWithoutSizeLimit() const -> QueryResult;
};

class TagIndex {
public:
    using WordContainer = std::vector<Word>;
    using Iterator = WordContainer::const_iterator;
    using FilteredWords = std::vector<Iterator>;
    using LengthMap = std::multimap<std::size_t, Iterator>;
    using TagMap = std::map<std::string, FilteredWords>;

    TagMap tags;
    FilteredWords all_words;

    TagIndex() = default;
    TagIndex(const WordContainer& words);
    TagIndex(const FilteredWords& words);

    void Add(Iterator it);

    [[nodiscard]] auto Query(const Selector& selector) const -> FilteredWords;
    [[nodiscard]] auto Query(const EmojiGen::TagsType& include_tags, const EmojiGen::TagsType& exclude_tags) const
        -> FilteredWords;
    [[nodiscard]] auto MaxWordCount(const Selector& selector) const -> std::size_t;

    [[nodiscard]] auto GetTagDefinitions(std::string_view kind) const -> std::vector<TagDefinition>;
};

struct CombinedIndex {
    using WordContainer = std::vector<Word>;
    using Iterator = WordContainer::const_iterator;
    using FilteredWords = std::vector<Iterator>;

    struct QueryResult {
        FilteredWords words;
        std::size_t max_length;
    };

    TagIndex tag_index;
    LengthIndex length_index;

    CombinedIndex() = default;
    CombinedIndex(const WordContainer& words);

    void Add(Iterator it);

    [[nodiscard]] auto Query(const Selector& selector) const -> QueryResult;
    [[nodiscard]] auto Query(const EmojiGen::TagsType& include_tags, const EmojiGen::TagsType& exclude_tags) const
        -> QueryResult;

    [[nodiscard]] auto GetTagDefinitions(std::string_view kind) const -> std::vector<TagDefinition>;
};

}  // namespace slugkit::generator::detail
