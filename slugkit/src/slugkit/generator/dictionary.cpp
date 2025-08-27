#include <slugkit/generator/dictionary.hpp>

#include <slugkit/generator/constants.hpp>
#include <slugkit/utils/text.hpp>
// #include <iostream>

namespace slugkit::generator {

FilteredDictionary::FilteredDictionary(FilteredDictionary::WordContainerPtr words, const Selector& selector)
    : dictionary_{std::move(words)}
    , selector_{selector}
    , words_{}
    , max_length_{0} {
    for (auto it = dictionary_->begin(); it != dictionary_->end(); ++it) {
        if (Matches(selector, *it)) {
            words_.push_back(it);
            max_length_ = std::max(max_length_, it->word.size());
        }
    }
}

std::string FilteredDictionary::operator[](std::size_t index) const {
    const auto& word = *words_[index];
    const auto& case_type = selector_.GetCase();
    // TODO: get locale based on language
    const auto locale = utils::text::kEnUsLocale;
    switch (case_type) {
        case CaseType::kNone:
            return word.word;
        case CaseType::kLower:
            return utils::text::ToLower(word.word, locale);
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

FilteredDictionaryConstPtr Dictionary::Filter(const Selector& selector) const {
    auto kind = utils::text::ToLower(selector.kind, utils::text::kEnUsLocale);
    if (kind != kind_) {
        return {};
    }
    if (selector.language.has_value() && selector.language.value() != language_) {
        return {};
    }

    return std::make_shared<const FilteredDictionary>(words_, selector);
}

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
