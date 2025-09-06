#include <slugkit/generator/detail/pattern_parser.hpp>

namespace slugkit::generator::detail {

auto ParseInteger(
    std::string_view original_pattern,
    std::string_view::const_iterator& pos,
    std::string_view::const_iterator e
) -> std::uint64_t {
    auto start = pos;
    while (pos != e && std::isdigit(*pos)) {
        ++pos;
    }
    if (start == pos) {
        throw PatternSyntaxError(
            fmt::format("Pattern parse error: expected number at column {}", pos - original_pattern.begin())
        );
    }
    return std::stoull(std::string(start, pos));
}

auto ParseRange(
    std::string_view original_pattern,
    std::string_view::const_iterator& pos,
    std::string_view::const_iterator e
) -> IntOrRange {
    IntOrRange result{0, 0};
    if (pos == e) {
        return result;
    }
    if (std::isdigit(*pos)) {
        result.min = ParseInteger(original_pattern, pos, e);
    }
    if (pos != e && *pos == '-') {
        ++pos;
        result.max = ParseInteger(original_pattern, pos, e);
        if (result.min > result.max) {
            throw PatternSyntaxError(fmt::format(
                "Pattern parse error: min count {} is greater than max count {} at column {}",
                result.min,
                result.max,
                pos - original_pattern.begin()
            ));
        }
    } else {
        result.max = result.min;
    }
    return result;
}

}  // namespace slugkit::generator::detail