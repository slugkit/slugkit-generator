#include <benchmark/benchmark.h>

#include <slugkit/test_utils/helpers.hpp>

#include <slugkit/generator/dictionary.hpp>
#include <slugkit/generator/pattern_generator.hpp>

#include <userver/engine/run_standalone.hpp>

#include <fmt/format.h>

namespace slugkit::generator::benchmarks {

using namespace literals;

void GenerateFromDictionary(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
        const auto dict_size = state.range(0);
        Dictionary dictionary = FillDictionary(
            {.name = "word", .language = "en", .size = dict_size, .tags = {}, .min_length = 0, .max_length = 0}
        );
        state.SetLabel(fmt::format("{} words", dict_size));
        auto filtered_dictionary = dictionary.Filter("word"_selector);
        SelectorSubstitutionGenerator generator{filtered_dictionary, {dict_size, dict_size}};
        auto seed_hash = PatternGenerator::SeedHash("test");
        auto sequence_number = 0;
        for ([[maybe_unused]] auto _ : state) {
            auto result = generator.Generate(seed_hash, sequence_number++ % dict_size);
            benchmark::DoNotOptimize(result);
        }
    });
}

void GenerateFromDictionaryUppercase(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
        const auto dict_size = state.range(0);
        Dictionary dictionary = FillDictionary(
            {.name = "word", .language = "en", .size = dict_size, .tags = {}, .min_length = 0, .max_length = 0}
        );
        state.SetLabel(fmt::format("{} words", dict_size));
        auto filtered_dictionary = dictionary.Filter("WORD"_selector);
        SelectorSubstitutionGenerator generator{filtered_dictionary, {dict_size, dict_size}};
        auto seed_hash = PatternGenerator::SeedHash("test");
        auto sequence_number = 0;
        for ([[maybe_unused]] auto _ : state) {
            auto result = generator.Generate(seed_hash, sequence_number++ % dict_size);
            benchmark::DoNotOptimize(result);
        }
    });
}

void GenerateFromDictionaryTitleCase(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
        const auto dict_size = state.range(0);
        Dictionary dictionary = FillDictionary(
            {.name = "word", .language = "en", .size = dict_size, .tags = {}, .min_length = 0, .max_length = 0}
        );
        state.SetLabel(fmt::format("{} words", dict_size));
        auto filtered_dictionary = dictionary.Filter("Word"_selector);
        SelectorSubstitutionGenerator generator{filtered_dictionary, {dict_size, dict_size}};
        auto seed_hash = PatternGenerator::SeedHash("test");
        auto sequence_number = 0;
        for ([[maybe_unused]] auto _ : state) {
            auto result = generator.Generate(seed_hash, sequence_number++ % dict_size);
            benchmark::DoNotOptimize(result);
        }
    });
}

void GenerateFromDictionaryMixedCase(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
        const auto dict_size = state.range(0);
        Dictionary dictionary = FillDictionary(
            {.name = "word", .language = "en", .size = dict_size, .tags = {}, .min_length = 0, .max_length = 0}
        );
        state.SetLabel(fmt::format("{} words", dict_size));
        auto filtered_dictionary = dictionary.Filter("wOrD"_selector);
        SelectorSubstitutionGenerator generator{filtered_dictionary, {dict_size, dict_size}};
        auto seed_hash = PatternGenerator::SeedHash("test");
        auto sequence_number = 0;
        for ([[maybe_unused]] auto _ : state) {
            auto result = generator.Generate(seed_hash, sequence_number++ % dict_size);
            benchmark::DoNotOptimize(result);
        }
    });
}

BENCHMARK(GenerateFromDictionary)->RangeMultiplier(10)->Range(1000, 1'000'000);
BENCHMARK(GenerateFromDictionaryUppercase)->RangeMultiplier(10)->Range(1000, 1'000'000);
BENCHMARK(GenerateFromDictionaryTitleCase)->RangeMultiplier(10)->Range(1000, 1'000'000);
BENCHMARK(GenerateFromDictionaryMixedCase)->RangeMultiplier(10)->Range(1000, 1'000'000);

}  // namespace slugkit::generator::benchmarks
