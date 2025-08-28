#include <benchmark/benchmark.h>

#include <slugkit/test_utils/helpers.hpp>

#include <slugkit/generator/detail/indexes.hpp>
#include <slugkit/generator/pattern.hpp>

#include <userver/engine/run_standalone.hpp>

namespace slugkit::generator::benchmarks {

using namespace literals;

namespace {

const auto kWords = GenerateWords(
    {.name = "word",
     .language = "en",
     .size = 100'000,
     .tags =
         {
             {.tag = "tag1", .probability = 100},
             {.tag = "tag2", .probability = 50},
             {.tag = "tag3", .probability = 25},
             {.tag = "tag4", .probability = 10},
         },
     .min_length = 3,
     .max_length = 20}
);

const auto kDictionary = Dictionary{"word", "en", std::move(kWords)};

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

}  // namespace

void BuildDictionary(benchmark::State& state) {
    for (auto _ : state) {
        auto dictionary = Dictionary{"word", "en", kWords};
        benchmark::DoNotOptimize(dictionary);
    }
}

void FilterDictionary(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
        const auto& dictionary = kDictionary;
        const auto& selector = kSelectors[state.range(0)];
        state.SetLabel(selector.ToString());
        for ([[maybe_unused]] auto _ : state) {
            auto filtered_dictionary = dictionary.Filter(selector);
            benchmark::DoNotOptimize(filtered_dictionary);
        }
    });
}

BENCHMARK(BuildDictionary);
BENCHMARK(FilterDictionary)->DenseRange(0, kSelectors.size() - 1);

}  // namespace slugkit::generator::benchmarks
