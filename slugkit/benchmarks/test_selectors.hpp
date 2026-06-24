#pragma once

#include <slugkit/generator/pattern.hpp>

namespace slugkit::generator::benchmarks {

using namespace literals;

// clang-format off
const std::vector<Selector> kSelectors = {
    "word"_selector,
    "word:==5"_selector,
    "word:==10"_selector,
    "word:==15"_selector,
    "word:==20"_selector,
    "word:<10"_selector,
    "word:>10"_selector,
    "word:<=8"_selector,
    "word:>=12"_selector,
    "word:!=10"_selector,
    "word:!=15"_selector,
    "word:+tag1"_selector,
    "word:+tag2"_selector,
    "word:+tag3"_selector,
    "word:+tag4"_selector,
    "word:-tag1"_selector,
    "word:-tag2"_selector,
    "word:-tag3"_selector,
    "word:-tag4"_selector,
    "word:+tag1-tag2"_selector,
    "word:+tag1 +tag2"_selector,
    "word:+tag1 +tag2 +tag3"_selector,
    "word:+tag1 +tag2 -tag3 +tag4"_selector,
    "word:+tag1==5"_selector,
    "word:+tag1 +tag2==10"_selector,
    "word:+tag1 +tag2!=10"_selector,
    "word:+tag1 +tag2 +tag3==15"_selector,
    "word:+tag1 +tag2 -tag3 +tag4==20"_selector,
    "word:+tag1<8"_selector,
    "word:+tag1>=8"_selector,
};
// clang-format on

}  // namespace slugkit::generator::benchmarks