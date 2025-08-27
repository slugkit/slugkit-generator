#pragma once

#include <slugkit/generator/pattern.hpp>

#include <slugkit/generator/types.hpp>

#include <iosfwd>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace slugkit::generator {

class Dictionary;

/// @brief A filtered dictionary is a dictionary that contains only the
/// words that match the selector.
/// @note The filtered dictionary is immutable.
/// @note The filtered dictionary is only valid as long as the original dictionary is alive.
class FilteredDictionary {
public:
    // TODO use a more efficient storage type, we don't need kind and language here
    using WordContainer = std::vector<Word>;
    using WordContainerPtr = std::shared_ptr<WordContainer>;
    using Iterator = WordContainer::const_iterator;
    using StorageType = std::vector<Iterator>;

public:
    FilteredDictionary(WordContainerPtr words, const Selector& selector);

    std::string operator[](std::size_t index) const;

    const Selector& GetSelector() const {
        return selector_;
    }

    CaseType GetCase() const {
        return selector_.GetCase();
    }

    const Word& GetWord(std::size_t index) const {
        return *words_[index];
    }

    std::size_t size() const {
        return words_.size();
    }

    bool empty() const {
        return words_.empty();
    }

    std::size_t GetMaxLength() const {
        return max_length_;
    }

private:
    // we hold the pointer to the original dictionary to avoid copying the words
    // and iterators not to be invalidated
    WordContainerPtr dictionary_;
    Selector selector_;
    StorageType words_;
    std::size_t max_length_;
};

using FilteredDictionaryPtr = std::shared_ptr<FilteredDictionary>;
using FilteredDictionaryConstPtr = std::shared_ptr<const FilteredDictionary>;

/// @brief A dictionary is a collection of words of the same kind
/// that are used to generate human-readable IDs.
/// @note The dictionary is immutable.
class Dictionary {
    using WordContainer = FilteredDictionary::WordContainer;
    using WordContainerPtr = FilteredDictionary::WordContainerPtr;

public:
    Dictionary(std::string_view kind, std::string_view language, std::vector<Word> words)
        : kind_(kind)
        , language_(language)
        , words_(std::make_shared<WordContainer>(std::move(words))) {
    }

    Dictionary(const Dictionary& other) = default;
    Dictionary(Dictionary&& other) = default;
    Dictionary& operator=(const Dictionary& other) = default;
    Dictionary& operator=(Dictionary&& other) = default;

    const std::string& GetKind() const {
        return kind_;
    }
    const std::string& GetLanguage() const {
        return language_;
    }

    const Word& GetWord(std::size_t index) const {
        return (*words_)[index];
    }

    const std::string& operator[](std::size_t index) const {
        return (*words_)[index].word;
    }

    std::size_t size() const {
        return words_->size();
    }

    bool empty() const {
        return words_->empty();
    }

    /// @brief Filters the dictionary by the selector.
    /// Will return an empty dictionary if the selector is empty or if the selector's kind is not the same as the
    /// dictionary's kind.
    /// @param selector The selector to use for filtering.
    /// @return The filtered dictionary.
    FilteredDictionaryConstPtr Filter(const Selector& selector) const;

private:
    std::string kind_;
    std::string language_;
    WordContainerPtr words_;
    // TODO Build tag indexes for fast filtering
};

/// @brief A set of dictionaries that can be used to generate human-readable IDs.
/// @note The dictionary set is immutable.
class DictionarySet {
public:
    DictionarySet(std::vector<Dictionary> dictionaries);

    FilteredDictionaryConstPtr Filter(const Selector& selector) const;

    auto size() const -> std::size_t {
        return dictionaries_.size();
    }

    /// @brief Parses a dictionary set from a format string.
    ///
    /// @param data The data to parse from.
    /// @return The parsed dictionary set.
    template <typename Format>
    static auto Parse(const std::string& data) -> DictionarySet;

    /// @brief Parses a dictionary set from a format stream.
    ///
    /// @param stream The stream to parse from.
    /// @return The parsed dictionary set.
    template <typename Format>
    static auto Parse(std::istream& stream) -> DictionarySet;

    /// @brief Parses a dictionary set from a format object.
    ///
    /// @param format The format object to parse from.
    /// @return The parsed dictionary set.
    template <typename Value>
    static auto Parse(const Value& value) -> DictionarySet;

private:
    std::map<std::string, Dictionary> dictionaries_;
    // a set of language-agnostic dictionaries
    std::set<std::string> language_agnostic_kinds_;
    // TODO LRU cache for filtered dictionaries
};

}  // namespace slugkit::generator
