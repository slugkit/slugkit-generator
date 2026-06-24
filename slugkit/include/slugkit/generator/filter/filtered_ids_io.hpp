#pragma once

#include <slugkit/generator/filter/filtered_ids.hpp>

#include <iostream>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace slugkit::generator::format {

struct NoContextFormatter {
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) {
        // For simple cases, just return the iterator
        // This accepts empty format specs like {}
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw fmt::format_error("invalid format");
        }
        return it;
    }
};

}  // namespace slugkit::generator::format

template <typename Char>
struct fmt::formatter<slugkit::generator::filter::IndexRange, Char>
    : public slugkit::generator::format::NoContextFormatter {
    template <typename FormatContext>
    auto format(const slugkit::generator::filter::IndexRange& range, FormatContext& ctx) const -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "[{}, {})", range.front(), range.end());
    }
};

template <typename Char>
struct fmt::range_format_kind<slugkit::generator::filter::IndexRangeSequence, Char>
    : std::integral_constant<fmt::range_format, fmt::range_format::disabled> {};

template <typename Char>
struct fmt::formatter<slugkit::generator::filter::IndexRangeSequence, Char>
    : public slugkit::generator::format::NoContextFormatter {
    template <typename FormatContext>
    auto format(const slugkit::generator::filter::IndexRangeSequence& sequence, FormatContext& ctx) const
        -> decltype(ctx.out()) {
        if (sequence.empty()) {
            return fmt::format_to(ctx.out(), "empty sequence");
        }
        return fmt::format_to(ctx.out(), "{{{}}}", fmt::join(sequence, ", "));
    }
};

namespace slugkit::generator::filter {

inline auto operator<<(std::ostream& os, const IndexRange& range) -> std::ostream& {
    return os << fmt::format("{}", range);
}

inline auto operator<<(std::ostream& os, const IndexRangeSequence& sequence) -> std::ostream& {
    return os << fmt::format("{}", sequence);
}

}  // namespace slugkit::generator::filter