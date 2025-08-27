#pragma once

#include <userver/utils/strong_typedef.hpp>

#include <optional>
#include <string>
#include <unordered_set>
#include <variant>

namespace slugkit::generator {

using Slug = userver::utils::StrongTypedef<class SlugTag, std::string>;
using OptionalSlug = std::optional<Slug>;

using WordTags = std::unordered_set<std::string>;

enum class CaseType {
    kNone,
    kLower,
    kUpper,
    kTitle,
    kMixed,
};

inline auto ToString(CaseType case_type) -> std::string_view {
    switch (case_type) {
        case CaseType::kNone:
            return "none";
        case CaseType::kLower:
            return "lower";
        case CaseType::kUpper:
            return "upper";
        case CaseType::kTitle:
            return "title";
        case CaseType::kMixed:
            return "mixed";
    }
}

/// @brief A word is a word in a dictionary.
/// @note The word is immutable.
template <typename TagsType>
struct BasicWord {
    std::string word;
    TagsType tags;

    auto operator==(const BasicWord& other) const -> bool {
        return word == other.word && tags == other.tags;
    }

    [[nodiscard]] auto ToString() const -> std::string;
};

using Word = BasicWord<WordTags>;

}  // namespace slugkit::generator
