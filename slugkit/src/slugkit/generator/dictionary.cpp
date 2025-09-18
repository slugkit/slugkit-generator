#include <slugkit/generator/dictionary.hpp>

#include <slugkit/generator/constants.hpp>
#include <slugkit/generator/detail/caches.hpp>
#include <slugkit/generator/detail/indexes.hpp>
#include <slugkit/utils/text.hpp>

namespace slugkit::generator {

FilteredDictionary::FilteredDictionary(
    WordContainerPtr words,
    CaseType case_type,
    StorageType&& storage,
    std::size_t max_length
)
    : dictionary_{std::move(words)}
    , case_type_{case_type}
    , words_{std::move(storage)}
    , max_length_{max_length} {
}

std::string FilteredDictionary::operator[](std::size_t index) const {
    const auto& word = *words_[index];
    // TODO: get locale based on language
    const auto locale = utils::text::kEnUsLocale;
    switch (case_type_) {
        case CaseType::kNone:
            return word.word;
        case CaseType::kLower:
            // no need to convert to lower case as the dictionary is expected to be in lower case
            return word.word;
        case CaseType::kUpper:
            return utils::text::ToUpper(word.word, locale);
        case CaseType::kTitle:
            return utils::text::Capitalize(word.word, locale);
        case CaseType::kMixed:
            // Mixed case is implemented by the pattern generator
            // it uses a stable permutation of case masks for the word
            return word.word;
    }
}

//-----------------------------------------------------------------------------
// Dictionary
//-----------------------------------------------------------------------------
struct Dictionary::Impl {
    using Iterator = FilteredDictionary::Iterator;
    using NoCache = detail::FilteredDictionaryNoCache;
    using Cache = detail::FilteredDictionaryCache;

    std::string kind_;
    std::string language_;
    WordContainerPtr words_;
    std::shared_ptr<detail::FilteredDictionaryCacheBase> cache_;

    Impl(std::string_view kind, std::string_view language, std::vector<Word>&& words, bool use_cache)
        : kind_{kind}
        , language_{language}
        , words_{std::make_shared<WordContainer>(std::move(words))}
        , cache_{} {
        if (use_cache) {
            cache_ = std::make_shared<Cache>(words_);
        } else {
            cache_ = std::make_shared<NoCache>(words_);
        }
    }

    auto Filter(const Selector& selector) const -> FilteredDictionaryConstPtr {
        return cache_->Get(selector);
    }

    auto Filter(const EmojiGen::TagsType& include_tags, const EmojiGen::TagsType& exclude_tags) const
        -> FilteredDictionaryConstPtr {
        return cache_->Get(include_tags, exclude_tags);
    }

    auto GetStats() const -> DictionaryStats {
        return DictionaryStats{kind_, language_, static_cast<std::int64_t>(words_->size())};
    }

    auto GetTagDefinitions() const -> std::vector<TagDefinition> {
        return cache_->GetTagDefinitions(kind_);
    }
};

Dictionary::Dictionary(std::string_view kind, std::string_view language, std::vector<Word> words, bool use_cache)
    : pimpl_{kind, language, std::move(words), use_cache} {
}

Dictionary::Dictionary(const Dictionary& other) noexcept = default;
Dictionary::Dictionary(Dictionary&& other) noexcept = default;
Dictionary& Dictionary::operator=(const Dictionary& other) noexcept = default;
Dictionary& Dictionary::operator=(Dictionary&& other) noexcept = default;

Dictionary::~Dictionary() = default;

auto Dictionary::GetKind() const -> const std::string& {
    return pimpl_->kind_;
}

auto Dictionary::GetLanguage() const -> const std::string& {
    return pimpl_->language_;
}

auto Dictionary::GetWord(std::size_t index) const -> const Word& {
    return (*pimpl_->words_)[index];
}

auto Dictionary::operator[](std::size_t index) const -> const std::string& {
    return (*pimpl_->words_)[index].word;
}

auto Dictionary::size() const -> std::size_t {
    return pimpl_->words_->size();
}

auto Dictionary::empty() const -> bool {
    return pimpl_->words_->empty();
}

auto Dictionary::Filter(const Selector& selector) const -> FilteredDictionaryConstPtr {
    auto kind = utils::text::ToLower(selector.kind, utils::text::kEnUsLocale);
    if (kind != pimpl_->kind_) {
        return {};
    }
    if (selector.language.has_value() && selector.language.value() != pimpl_->language_) {
        return {};
    }

    return pimpl_->Filter(selector);
}

auto Dictionary::Filter(const EmojiGen::TagsType& include_tags, const EmojiGen::TagsType& exclude_tags) const
    -> FilteredDictionaryConstPtr {
    return pimpl_->Filter(include_tags, exclude_tags);
}

auto Dictionary::GetStats() const -> DictionaryStats {
    return pimpl_->GetStats();
}

auto Dictionary::GetTagDefinitions() const -> std::vector<TagDefinition> {
    return pimpl_->GetTagDefinitions();
}

//-----------------------------------------------------------------------------
// DictionarySet
//-----------------------------------------------------------------------------
DictionarySet::DictionarySet(std::vector<Dictionary> dictionaries)
    : dictionaries_{} {
    for (auto& dictionary : dictionaries) {
        std::string key = dictionary.GetKind();
        const auto& language = dictionary.GetLanguage();
        if (!language.empty()) {
            key += "-" + language;
        } else {
            language_agnostic_kinds_.insert(key);
        }
        dictionaries_.emplace(std::move(key), std::move(dictionary));
    }
}

FilteredDictionaryConstPtr DictionarySet::Filter(const Selector& selector) const {
    auto key = utils::text::ToLower(selector.kind, utils::text::kEnUsLocale);
    // TODO maybe merge language-agnostic and language-specific dictionaries
    if (language_agnostic_kinds_.find(key) != language_agnostic_kinds_.end()) {
        // maybe language-agnostic dictionary
        // check if there are language-specific dictionaries
        if (selector.language.has_value()) {
            auto lang_key =
                fmt::format("{}-{}", key, utils::text::ToLower(selector.language.value(), utils::text::kEnUsLocale));
            auto dict = dictionaries_.find(lang_key);
            if (dict != dictionaries_.end()) {
                return dict->second.Filter(selector);
            }
        }
    } else {
        // language-specific dictionary
        if (selector.language.has_value()) {
            key += fmt::format("-{}", utils::text::ToLower(selector.language.value(), utils::text::kEnUsLocale));
        } else {
            key += "-en";
        }
    }
    auto dict = dictionaries_.find(key);
    if (dict == dictionaries_.end()) {
        return {};
    }
    return dict->second.Filter(selector);
}

}  // namespace slugkit::generator
