#include <benchmark/benchmark.h>

#include "helpers.hpp"

#include <slugkit/generator/pattern.hpp>

#include <userver/engine/run_standalone.hpp>

namespace slugkit::generator::benchmarks {

using namespace literals;

namespace {

const auto kDictionary = FillDictionary(
    {.name = "word",
     .language = "en",
     .size = 100'000,
     .tags =
         {
             {.tag = "tag1", .probability = 100},
             {.tag = "tag2", .probability = 50},
             {.tag = "tag3", .probability = 25},
             {.tag = "tag4", .probability = 10},
         }}
);

const std::vector<Selector> kSelectors = {
    "word"_selector,
    "word:+tag1"_selector,
    "word:+tag2"_selector,
    "word:+tag3"_selector,
    "word:+tag4"_selector,
    "word:-tag1"_selector,
    "word:-tag2"_selector,
    "word:-tag3"_selector,
    "word:-tag4"_selector,
    "word:+tag1-tag2"_selector,
};

}  // namespace

void FilterDictionary(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
        auto dictionary = kDictionary;
        const auto& selector = kSelectors[state.range(0)];
        state.SetLabel(selector.ToString());
        for ([[maybe_unused]] auto _ : state) {
            auto filtered_dictionary = dictionary.Filter(selector);
            benchmark::DoNotOptimize(filtered_dictionary);
        }
    });
}

BENCHMARK(FilterDictionary)->DenseRange(0, kSelectors.size() - 1);

}  // namespace slugkit::generator::benchmarks
