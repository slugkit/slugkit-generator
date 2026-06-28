#pragma once

#include <slugkit/compat/strong_typedef.hpp>

#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <variant>

namespace slugkit::generator {

using Slug = slugkit::compat::StrongTypedef<class SlugTag, std::string>;
using OptionalSlug = std::optional<Slug>;

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

using LanguageCode = slugkit::compat::StrongTypedef<class LanguageCodeTag, std::string, slugkit::compat::StrongTypedefOps::kCompareTransparent>;
using LanguageCodeView = slugkit::compat::StrongTypedef<class LanguageCodeTag, std::string_view, slugkit::compat::StrongTypedefOps::kCompareTransparent>;

using LanguageCodeSet = std::set<LanguageCodeView>;

inline auto operator==(const LanguageCode& lhs, const LanguageCodeView& rhs) noexcept -> bool {
    return lhs.GetUnderlying() == rhs.GetUnderlying();
}

inline auto operator==(const LanguageCodeView& lhs, const LanguageCode& rhs) noexcept -> bool {
    return lhs.GetUnderlying() == rhs.GetUnderlying();
}

inline auto operator!=(const LanguageCode& lhs, const LanguageCodeView& rhs) noexcept -> bool {
    return lhs.GetUnderlying() != rhs.GetUnderlying();
}

inline auto operator!=(const LanguageCodeView& lhs, const LanguageCode& rhs) noexcept -> bool {
    return lhs.GetUnderlying() != rhs.GetUnderlying();
}

inline auto operator<(const LanguageCode& lhs, const LanguageCodeView& rhs) noexcept -> bool {
    return lhs.GetUnderlying() < rhs.GetUnderlying();
}

inline auto operator<(const LanguageCodeView& lhs, const LanguageCode& rhs) noexcept -> bool {
    return lhs.GetUnderlying() < rhs.GetUnderlying();
}

using Tag =
    slugkit::compat::StrongTypedef<class TagTag, std::string, slugkit::compat::StrongTypedefOps::kCompareTransparent>;
using TagView = slugkit::compat::StrongTypedef<class TagTag, std::string_view, slugkit::compat::StrongTypedefOps::kCompareTransparent>;

using TagSet = std::set<TagView>;

inline auto operator==(const Tag& lhs, const TagView& rhs) noexcept -> bool {
    return lhs.GetUnderlying() == rhs.GetUnderlying();
}

inline auto operator==(const TagView& lhs, const Tag& rhs) noexcept -> bool {
    return lhs.GetUnderlying() == rhs.GetUnderlying();
}

inline auto operator!=(const Tag& lhs, const TagView& rhs) noexcept -> bool {
    return lhs.GetUnderlying() != rhs.GetUnderlying();
}

inline auto operator!=(const TagView& lhs, const Tag& rhs) noexcept -> bool {
    return lhs.GetUnderlying() != rhs.GetUnderlying();
}

inline auto operator<(const Tag& lhs, const TagView& rhs) noexcept -> bool {
    return lhs.GetUnderlying() < rhs.GetUnderlying();
}

inline auto operator<(const TagView& lhs, const Tag& rhs) noexcept -> bool {
    return lhs.GetUnderlying() < rhs.GetUnderlying();
}

using WordTags = std::set<Tag>;
using Word = BasicWord<WordTags>;

using size_type = std::uint32_t;

using IndexType =
    slugkit::compat::StrongTypedef<class IndexTypeTag, size_type, slugkit::compat::StrongTypedefOps::kCompareTransparent>;
using SizeType =
    slugkit::compat::StrongTypedef<class SizeTypeTag, size_type, slugkit::compat::StrongTypedefOps::kCompareTransparent>;

namespace literals {

inline constexpr auto operator""_lang(const char* str, std::size_t len) noexcept -> LanguageCode {
    return LanguageCode{str, len};
}

inline constexpr auto operator""_lang_view(const char* str, std::size_t len) noexcept -> LanguageCodeView {
    return LanguageCodeView{str, len};
}

inline constexpr auto operator""_tag(const char* str, std::size_t len) noexcept -> Tag {
    return Tag{str, len};
}

inline constexpr auto operator""_tag_view(const char* str, std::size_t len) noexcept -> TagView {
    return TagView{str, len};
}

inline constexpr auto operator""_idx(unsigned long long value) noexcept -> IndexType {
    return IndexType(static_cast<IndexType::UnderlyingType>(value));
}

inline constexpr auto operator""_size(unsigned long long value) noexcept -> SizeType {
    return SizeType(static_cast<SizeType::UnderlyingType>(value));
}

}  // namespace literals

inline constexpr auto operator+(IndexType lhs, IndexType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() + rhs.GetUnderlying());
}
inline constexpr auto operator+(IndexType lhs, IndexType::UnderlyingType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() + rhs);
}
inline constexpr auto operator-(IndexType lhs, IndexType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() - rhs.GetUnderlying());
}
inline constexpr auto operator-(IndexType lhs, IndexType::UnderlyingType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() - rhs);
}
inline constexpr auto operator*(IndexType lhs, IndexType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() * rhs.GetUnderlying());
}
inline constexpr auto operator/(IndexType lhs, IndexType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() / rhs.GetUnderlying());
}
inline constexpr auto operator/(IndexType lhs, IndexType::UnderlyingType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() / rhs);
}
inline constexpr auto operator%(IndexType lhs, IndexType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() % rhs.GetUnderlying());
}
inline constexpr auto operator%(IndexType lhs, IndexType::UnderlyingType rhs) noexcept -> IndexType {
    return IndexType(lhs.GetUnderlying() % rhs);
}

}  // namespace slugkit::generator
