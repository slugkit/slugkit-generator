#pragma once

#include <slugkit/generator/pattern.hpp>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/formats/serialize/to.hpp>

namespace slugkit::generator {

inline auto ToString(CompareOperator op) -> std::string_view {
    switch (op) {
        case CompareOperator::kNone:
            return "none";
        case CompareOperator::kEq:
            return "eq";
        case CompareOperator::kNe:
            return "ne";
        case CompareOperator::kGt:
            return "gt";
        case CompareOperator::kGe:
            return "ge";
        case CompareOperator::kLt:
            return "lt";
        case CompareOperator::kLe:
            return "le";
    }
}

template <typename Format>
auto Serialize(const CompareOperator& op, userver::formats::serialize::To<Format>) -> Format {
    typename Format::Builder builder(ToString(op));
    return builder.ExtractValue();
}

template <typename Format>
auto Serialize(const SizeLimit& size_limit, userver::formats::serialize::To<Format>) -> Format {
    typename Format::Builder builder;
    builder["op"] = ToString(size_limit.op);
    builder["value"] = size_limit.value;
    return builder.ExtractValue();
}

template <typename Format>
auto Serialize(const Selector& selector, userver::formats::serialize::To<Format>) -> Format {
    typename Format::Builder builder;
    builder["kind"] = std::string{selector.kind};
    builder["case"] = selector.GetCase();
    builder["include_tags"] = selector.include_tags;
    builder["exclude_tags"] = selector.exclude_tags;
    builder["language"] = selector.language;
    builder["size_limit"] = selector.size_limit;
    // TODO: Serialize options, currently string_view is not supported as a key
    // builder["options"] = selector.options;
    return builder.ExtractValue();
}

inline std::string_view ToString(NumberBase number_base) {
    switch (number_base) {
        case NumberBase::kDec:
            return "dec";
        case NumberBase::kHex:
            return "hex";
        case NumberBase::kHexUpper:
            return "hex_upper";
        case NumberBase::kRoman:
            return "roman";
        case NumberBase::kRomanLower:
            return "roman_lower";
    }
}

template <typename Format>
auto Serialize(const NumberBase& number_base, userver::formats::serialize::To<Format>) -> Format {
    typename Format::Builder builder(ToString(number_base));
    return builder.ExtractValue();
}

template <typename Format>
Format Serialize(const NumberGen& number_gen, userver::formats::serialize::To<Format>) {
    typename Format::Builder builder;
    builder["max_length"] = number_gen.max_length;
    builder["base"] = number_gen.base;
    return builder.ExtractValue();
}

template <typename Format>
Format Serialize(const SpecialCharGen& special_char_gen, userver::formats::serialize::To<Format>) {
    typename Format::Builder builder;
    builder["min_length"] = special_char_gen.min_length;
    builder["max_length"] = special_char_gen.max_length;
    return builder.ExtractValue();
}

template <typename Format>
auto Serialize(const Pattern& pattern, userver::formats::serialize::To<Format>) -> Format {
    typename Format::Builder builder;
    builder["pattern"] = pattern.pattern;
    return builder.ExtractValue();
}

template <typename Format>
auto Serialize(const EmojiGen& emoji_gen, userver::formats::serialize::To<Format>) -> Format {
    typename Format::Builder builder;
    builder["min_count"] = emoji_gen.min_count;
    builder["max_count"] = emoji_gen.max_count;
    return builder.ExtractValue();
}

}  // namespace slugkit::generator
