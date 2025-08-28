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

const std::vector<Selector> kLengthSelectors = {
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
};

const std::vector<Selector> kTagSelectors = {
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
};

const std::vector<Selector> kCombinedSelectors = {
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
    "word:+tag1==10"_selector,
    "word:+tag1 +tag2==10"_selector,
    "word:+tag1 +tag2!=10"_selector,
    "word:+tag1 +tag2 +tag3==15"_selector,
    "word:+tag1 +tag2 -tag3 +tag4==20"_selector,
    "word:+tag1<8"_selector,
    "word:+tag1>=8"_selector,
};

}  // namespace

void BuildLengthIndex(benchmark::State& state) {
    for (auto _ : state) {
        detail::LengthIndex index(kWords);
        benchmark::DoNotOptimize(index);
    }
}

void QueryLengthIndex(benchmark::State& state) {
    detail::LengthIndex index(kWords);
    const auto& selector = kLengthSelectors[state.range(0)];
    state.SetLabel(selector.ToString());
    for (auto _ : state) {
        auto result = index.Query(selector);
        benchmark::DoNotOptimize(result);
    }
}

void QueryLengthIndexToSet(benchmark::State& state) {
    detail::LengthIndex index(kWords);
    const auto& selector = kLengthSelectors[state.range(0)];
    state.SetLabel(selector.ToString());
    for (auto _ : state) {
        auto result = index.Query(selector).ToSet();
        benchmark::DoNotOptimize(result);
    }
}

void BuildTagIndex(benchmark::State& state) {
    for (auto _ : state) {
        detail::TagIndex index(kWords);
        benchmark::DoNotOptimize(index);
    }
}

void QueryTagIndex(benchmark::State& state) {
    detail::TagIndex index(kWords);
    const auto& selector = kTagSelectors[state.range(0)];
    state.SetLabel(selector.ToString());
    for (auto _ : state) {
        auto result = index.Query(selector);
        benchmark::DoNotOptimize(result);
    }
}

void EstimateTagIndexWordCount(benchmark::State& state) {
    detail::TagIndex index(kWords);
    const auto& selector = kTagSelectors[state.range(0)];
    state.SetLabel(selector.ToString());
    for (auto _ : state) {
        auto result = index.MaxWordCount(selector);
        benchmark::DoNotOptimize(result);
    }
}

void BuildCombinedIndex(benchmark::State& state) {
    for (auto _ : state) {
        detail::CombinedIndex index(kWords);
        benchmark::DoNotOptimize(index);
    }
}

void QueryCombinedIndex(benchmark::State& state) {
    detail::CombinedIndex index(kWords);
    const auto& selector = kCombinedSelectors[state.range(0)];
    state.SetLabel(selector.ToString());
    for (auto _ : state) {
        auto result = index.Query(selector);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(BuildLengthIndex);
BENCHMARK(QueryLengthIndex)->DenseRange(0, kLengthSelectors.size() - 1);
// BENCHMARK(QueryLengthIndexToSet)->DenseRange(0, kLengthSelectors.size() - 1);
BENCHMARK(BuildTagIndex);
BENCHMARK(QueryTagIndex)->DenseRange(0, kTagSelectors.size() - 1);
BENCHMARK(EstimateTagIndexWordCount)->DenseRange(0, kTagSelectors.size() - 1);
BENCHMARK(BuildCombinedIndex);
BENCHMARK(QueryCombinedIndex)->DenseRange(0, kCombinedSelectors.size() - 1);

}  // namespace slugkit::generator::benchmarks