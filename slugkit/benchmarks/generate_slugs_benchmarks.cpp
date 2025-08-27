#include <benchmark/benchmark.h>

#include "helpers.hpp"

#include <slugkit/generator/generator.hpp>
#include <slugkit/generator/permutations.hpp>

#include <userver/engine/run_standalone.hpp>

namespace slugkit::generator::benchmarks {

namespace {
const std::vector<std::string> kPatterns = {
    "{verb}-{adverb}",
    "{adverb}-{noun}-{verb}",
    "{adverb}-{noun}-{verb}-{number:4x}",
    "{adverb}-{noun}-{verb}-{adverb}-{noun}-{verb}",
    "{adverb}-{noun}-{verb}-{adverb}-{noun}-{verb}-{adverb}-{noun}-{verb}",
};

const auto kDictionaries = GenerateSet({
    {.name = "adjective", .language = "en", .size = 30'000, .tags = {}},
    {.name = "adverb", .language = "en", .size = 10'000, .tags = {}},
    {.name = "noun", .language = "en", .size = 100'000, .tags = {}},
    {.name = "verb", .language = "en", .size = 20'000, .tags = {}},
});

constexpr auto kSeed = "test";

}  // namespace

void CalculateSettings(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
        auto generator = Generator{kDictionaries};
        auto pattern_str = kPatterns[state.range(0)];
        auto pattern = std::make_shared<Pattern>(pattern_str);
        state.SetLabel(pattern_str);
        for ([[maybe_unused]] auto _ : state) {
            auto settings = generator.GetCapacity(pattern);
            benchmark::DoNotOptimize(settings);
        }
    });
}

void GenerateSlugs(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
        const auto dict_size = state.range(0);
        auto generator = Generator{kDictionaries};
        auto pattern_str = kPatterns[state.range(0)];
        auto pattern = std::make_shared<Pattern>(pattern_str);
        auto settings = generator.GetCapacity(pattern);
        auto sequence_number = 0;
        state.SetLabel(pattern_str);
        for ([[maybe_unused]] auto _ : state) {
            auto result = generator.Generate(settings, pattern, kSeed, sequence_number++ % dict_size);
            benchmark::DoNotOptimize(result);
        }
    });
}

BENCHMARK(CalculateSettings)->DenseRange(0, kPatterns.size() - 1);
BENCHMARK(GenerateSlugs)->DenseRange(0, kPatterns.size() - 1);

}  // namespace slugkit::generator::benchmarks