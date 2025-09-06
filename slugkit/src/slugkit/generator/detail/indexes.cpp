#include <slugkit/generator/detail/indexes.hpp>

namespace slugkit::generator::detail {

LengthIndex::LengthIndex(const WordContainer& words) {
    for (auto it = words.begin(); it != words.end(); ++it) {
        lengths.emplace(it->word.size(), it);
    }
}

LengthIndex::LengthIndex(const FilteredWords& words) {
    for (auto it : words) {
        lengths.emplace(it->word.size(), it);
    }
}

void LengthIndex::Add(Iterator it) {
    lengths.emplace(it->word.size(), it);
}

auto LengthIndex::MaxLength() const -> std::size_t {
    if (lengths.empty()) {
        return 0;
    }
    return lengths.rbegin()->first;
}

auto LengthIndex::Eq(std::size_t length) const -> QueryResult {
    auto range = lengths.equal_range(length);
    return {{range.first, range.second}, length};
}

auto LengthIndex::Ne(std::size_t length) const -> QueryResult {
    auto less = Lt(length);
    auto greater = Gt(length);
    QueryResult result{less.words, greater.max_length};
    if (!greater.words.empty()) {
        result.words.AddRange(greater.words.begin().GetUnderlying(), lengths.end());
    }
    return result;
}

auto LengthIndex::Lt(std::size_t length) const -> QueryResult {
    auto not_less = lengths.lower_bound(length);
    std::size_t max_length = 0;
    if (not_less != lengths.begin()) {
        max_length = std::prev(not_less)->first;
    }
    return {{lengths.begin(), not_less}, max_length};
}

auto LengthIndex::Le(std::size_t length) const -> QueryResult {
    auto not_less = lengths.lower_bound(length + 1);
    std::size_t max_length = 0;
    if (not_less != lengths.begin()) {
        max_length = std::prev(not_less)->first;
    }
    return {{lengths.begin(), not_less}, max_length};
}

auto LengthIndex::Gt(std::size_t length) const -> QueryResult {
    auto greater = lengths.upper_bound(length);
    return {{greater, lengths.end()}, MaxLength()};
}

auto LengthIndex::Ge(std::size_t length) const -> QueryResult {
    auto greater = lengths.lower_bound(length);
    return {{greater, lengths.end()}, MaxLength()};
}

auto LengthIndex::Query(const Selector& selector) const -> QueryResult {
    if (selector.HasSizeLimit()) {
        return QueryWithSizeLimit(selector.size_limit.value());
    }
    return QueryWithoutSizeLimit();
}

auto LengthIndex::QueryWithSizeLimit(const SizeLimit& size_limit) const -> QueryResult {
    switch (size_limit.op) {
        case CompareOperator::kEq:
            return Eq(size_limit.value);
        case CompareOperator::kNe:
            return Ne(size_limit.value);
        case CompareOperator::kGt:
            return Gt(size_limit.value);
        case CompareOperator::kGe:
            return Ge(size_limit.value);
        case CompareOperator::kLt:
            return Lt(size_limit.value);
        case CompareOperator::kLe:
            return Le(size_limit.value);
        case CompareOperator::kNone:
            return QueryWithoutSizeLimit();
    }
}

auto LengthIndex::QueryWithoutSizeLimit() const -> QueryResult {
    return {{lengths.begin(), lengths.end()}, MaxLength()};
}

TagIndex::TagIndex(const WordContainer& words) {
    all_words.reserve(words.size());
    for (auto it = words.begin(); it != words.end(); ++it) {
        Add(it);
    }
}

TagIndex::TagIndex(const FilteredWords& words) {
    all_words = words;
    for (auto it : all_words) {
        Add(it);
    }
}

void TagIndex::Add(Iterator it) {
    for (const auto& tag : it->tags) {
        tags[tag].push_back(it);
    }
    all_words.push_back(it);
}

auto TagIndex::Query(const Selector& selector) const -> FilteredWords {
    return Query(selector.include_tags, selector.exclude_tags);
}

auto TagIndex::Query(const EmojiGen::TagsType& include_tags, const EmojiGen::TagsType& exclude_tags) const
    -> FilteredWords {
    if (include_tags.empty() && exclude_tags.empty()) {
        return all_words;
    }
    // exclude tags apparently will result in a bigger set,
    // so we filter by include tags first
    FilteredWords result;
    if (include_tags.empty()) {
        result = all_words;
    } else {
        std::vector<TagMap::const_iterator> matched_tags;
        for (const auto& tag : include_tags) {
            auto it = tags.find(std::string(tag));
            if (it == tags.end()) {
                continue;
            }
            matched_tags.push_back(it);
        }
        if (matched_tags.empty()) {
            return result;
        }
        // we want to start with the smallest set of words
        std::sort(matched_tags.begin(), matched_tags.end(), [](const auto& a, const auto& b) {
            return a->second.size() < b->second.size();
        });
        // the result will be no bigger than the smallest set of words
        result = matched_tags.front()->second;
        for (auto it = std::next(matched_tags.begin()); it != matched_tags.end(); ++it) {
            FilteredWords filtered_words;
            std::set_intersection(
                result.begin(),
                result.end(),
                (*it)->second.begin(),
                (*it)->second.end(),
                std::back_inserter(filtered_words)
            );
            std::swap(result, filtered_words);
        }
    }

    // exclude tags
    if (!exclude_tags.empty()) {
        std::vector<TagMap::const_iterator> matched_tags;
        for (const auto& tag : exclude_tags) {
            auto it = tags.find(std::string(tag));
            if (it == tags.end()) {
                continue;
            }
            matched_tags.push_back(it);
        }
        if (matched_tags.empty()) {
            return result;
        }
        // we want to start with the biggest set of words
        std::sort(matched_tags.begin(), matched_tags.end(), [](const auto& a, const auto& b) {
            return a->second.size() < b->second.size();
        });
        for (const auto& it : matched_tags) {
            FilteredWords filtered_words;
            std::set_difference(
                result.begin(), result.end(), it->second.begin(), it->second.end(), std::back_inserter(filtered_words)
            );
            std::swap(result, filtered_words);
            if (result.empty()) {
                return result;
            }
        }
    }
    return result;
}

auto TagIndex::MaxWordCount(const Selector& selector) const -> std::size_t {
    if (selector.NoFilter()) {
        return all_words.size();
    }
    std::size_t max_word_count = 0;
    // find the smallest set of words with include tags
    if (!selector.include_tags.empty()) {
        std::vector<TagMap::const_iterator> matched_tags;
        for (const auto& tag : selector.include_tags) {
            auto it = tags.find(std::string(tag));
            if (it == tags.end()) {
                continue;
            }
            matched_tags.push_back(it);
        }
        if (matched_tags.empty()) {
            return 0;
        }
        max_word_count = matched_tags.front()->second.size();
        for (auto it = std::next(matched_tags.begin()); it != matched_tags.end(); ++it) {
            max_word_count = std::min(max_word_count, (*it)->second.size());
        }
    }

    // find the biggest set of words with exclude tags
    if (!selector.exclude_tags.empty()) {
        std::vector<TagMap::const_iterator> matched_tags;
        for (const auto& tag : selector.exclude_tags) {
            auto it = tags.find(std::string(tag));
            if (it == tags.end()) {
                continue;
            }
            matched_tags.push_back(it);
        }
        if (matched_tags.empty()) {
            return max_word_count;
        }
        for (auto it = std::next(matched_tags.begin()); it != matched_tags.end(); ++it) {
            max_word_count = std::min(max_word_count, all_words.size() - (*it)->second.size());
        }
    }

    return max_word_count;
}

auto TagIndex::GetTagDefinitions(std::string_view kind) const -> std::vector<TagDefinition> {
    std::vector<TagDefinition> result;
    for (const auto& [tag, words] : tags) {
        result.push_back({std::string(kind), std::string(tag), {}, false, static_cast<std::int32_t>(words.size())});
    }
    return result;
}

// ---------------------------------------------------------------------------------------------------------------------
CombinedIndex::CombinedIndex(const WordContainer& words)
    : tag_index{}
    , length_index{} {
    tag_index.all_words.reserve(words.size());
    for (auto it = words.begin(); it != words.end(); ++it) {
        Add(it);
    }
}

void CombinedIndex::Add(Iterator it) {
    tag_index.Add(it);
    length_index.Add(it);
}

auto CombinedIndex::Query(const Selector& selector) const -> QueryResult {
    if (!selector.HasSizeLimit()) {
        auto data = tag_index.Query(selector);
        std::size_t max_length = 0;
        for (const auto& it : data) {
            max_length = std::max(max_length, it->word.size());
        }
        return {data, max_length};
    }
    auto by_length = length_index.Query(selector);
    if (by_length.words.empty()) {
        return {};
    }
    if (!selector.HasTags()) {
        return {by_length.ToSet(), by_length.max_length};
    }
    // Here we have a size limit and tags.
    auto result = tag_index.Query(selector);
    auto size_limit = selector.size_limit.value();
    std::size_t max_length = 0;
    std::erase_if(result, [&](const auto& it) {
        max_length = std::max(max_length, it->word.size());
        return !size_limit.Matches(it->word.size());
    });
    return {result, max_length};
}

auto CombinedIndex::Query(const EmojiGen::TagsType& include_tags, const EmojiGen::TagsType& exclude_tags) const
    -> QueryResult {
    auto result = tag_index.Query(include_tags, exclude_tags);
    std::size_t max_length = 0;
    for (const auto& it : result) {
        max_length = std::max(max_length, it->word.size());
    }
    return {result, max_length};
}

auto CombinedIndex::GetTagDefinitions(std::string_view kind) const -> std::vector<TagDefinition> {
    return tag_index.GetTagDefinitions(kind);
}

}  // namespace slugkit::generator::detail
