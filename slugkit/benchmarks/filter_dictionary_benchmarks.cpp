#include <benchmark/benchmark.h>

#include "test_selectors.hpp"

#include <slugkit/test_utils/helpers.hpp>

#include <slugkit/generator/detail/indexes.hpp>
#include <slugkit/generator/pattern.hpp>

#include <userver/engine/run_standalone.hpp>

namespace slugkit::generator::benchmarks {

using namespace literals;

namespace {

const auto kWords = GenerateWords(
    {.name = "word",
     .language = "en"_lang,
     .size = 100'000,
     .tags =
         {
             {.tag = "tag1"_tag, .probability = 100},
             {.tag = "tag2"_tag, .probability = 50},
             {.tag = "tag3"_tag, .probability = 25},
             {.tag = "tag4"_tag, .probability = 10},
         },
     .min_length = 3,
     .max_length = 20}
);

const auto kDictionaryNoCache = Dictionary{"word", "en"_lang_view, kWords, false};
const auto kDictionaryWithCache = Dictionary{"word", "en"_lang_view, kWords, true};

}  // namespace

void BuildDictionaryNoCache(benchmark::State& state) {
    for (auto _ : state) {
        auto dictionary = Dictionary{"word", "en"_lang_view, kWords, false};
        benchmark::DoNotOptimize(dictionary);
    }
}

void BuildDictionaryWithCache(benchmark::State& state) {
    for (auto _ : state) {
        auto dictionary = Dictionary{"word", "en"_lang_view, kWords, true};
        benchmark::DoNotOptimize(dictionary);
    }
}

void FilterDictionaryNoCache(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
        const auto& dictionary = kDictionaryNoCache;
        const auto& selector = kSelectors[state.range(0)];
        state.SetLabel(selector.ToString());
        for ([[maybe_unused]] auto _ : state) {
            auto filtered_dictionary = dictionary.Filter(selector);
            benchmark::DoNotOptimize(filtered_dictionary);
        }
    });
}

void FilterDictionary(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
        const auto& dictionary = kDictionaryWithCache;
        const auto& selector = kSelectors[state.range(0)];
        state.SetLabel(selector.ToString());
        for ([[maybe_unused]] auto _ : state) {
            auto filtered_dictionary = dictionary.Filter(selector);
            benchmark::DoNotOptimize(filtered_dictionary);
        }
    });
}

BENCHMARK(BuildDictionaryNoCache);
BENCHMARK(BuildDictionaryWithCache);
BENCHMARK(FilterDictionaryNoCache)->DenseRange(0, kSelectors.size() - 1);
BENCHMARK(FilterDictionary)->DenseRange(0, kSelectors.size() - 1);

}  // namespace slugkit::generator::benchmarks
