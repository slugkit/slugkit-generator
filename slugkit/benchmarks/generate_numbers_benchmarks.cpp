#include <benchmark/benchmark.h>

#include <slugkit/generator/pattern_generator.hpp>

#include <userver/engine/run_standalone.hpp>

namespace slugkit::generator::benchmarks {

using namespace literals;

void GenerateHexNumbers(benchmark::State& state) {
    auto length = static_cast<std::uint8_t>(state.range(0));
    auto number_gen = NumberGen{length, NumberBase::kHex};
    auto generator = NumberSubstitutionGenerator{number_gen};
    auto seed_hash = PatternGenerator::SeedHash("test");
    state.SetLabel(number_gen.ToString());
    for ([[maybe_unused]] auto _ : state) {
        auto result = generator.Generate(seed_hash, 0);
        benchmark::DoNotOptimize(result);
    }
}

void GenerateHexNumbersUppercase(benchmark::State& state) {
    auto length = static_cast<std::uint8_t>(state.range(0));
    auto number_gen = NumberGen{length, NumberBase::kHexUpper};
    auto generator = NumberSubstitutionGenerator{number_gen};
    auto seed_hash = PatternGenerator::SeedHash("test");
    state.SetLabel(number_gen.ToString());
    for ([[maybe_unused]] auto _ : state) {
        auto result = generator.Generate(seed_hash, 0);
        benchmark::DoNotOptimize(result);
    }
}

void GenerateDecNumbers(benchmark::State& state) {
    auto length = static_cast<std::uint8_t>(state.range(0));
    auto number_gen = NumberGen{length, NumberBase::kDec};
    auto generator = NumberSubstitutionGenerator{number_gen};
    auto seed_hash = PatternGenerator::SeedHash("test");
    state.SetLabel(number_gen.ToString());
    for ([[maybe_unused]] auto _ : state) {
        auto result = generator.Generate(seed_hash, 0);
        benchmark::DoNotOptimize(result);
    }
}

// 15 is the maximum number of digits for roman numerals
void GenerateRomanNumbersUppercase(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
        auto length = static_cast<std::uint8_t>(state.range(0));
        auto number_gen = NumberGen{length, NumberBase::kRoman};
        auto generator = RomanSubstitutionGenerator{number_gen};
        auto seed_hash = PatternGenerator::SeedHash("test");
        state.SetLabel(number_gen.ToString());
        for ([[maybe_unused]] auto _ : state) {
            auto result = generator.Generate(seed_hash, 0);
            benchmark::DoNotOptimize(result);
        }
    });
}

void GenerateRomanNumbersLowercase(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
        auto length = static_cast<std::uint8_t>(state.range(0));
        auto number_gen = NumberGen{length, NumberBase::kRomanLower};
        auto generator = RomanSubstitutionGenerator{number_gen};
        auto seed_hash = PatternGenerator::SeedHash("test");
        state.SetLabel(number_gen.ToString());
        for ([[maybe_unused]] auto _ : state) {
            auto result = generator.Generate(seed_hash, 0);
            benchmark::DoNotOptimize(result);
        }
    });
}

BENCHMARK(GenerateHexNumbers)->RangeMultiplier(2)->Range(1, 16);
BENCHMARK(GenerateHexNumbersUppercase)->RangeMultiplier(2)->Range(1, 16);
BENCHMARK(GenerateDecNumbers)->RangeMultiplier(2)->Range(1, 18);
BENCHMARK(GenerateRomanNumbersUppercase)->RangeMultiplier(2)->Range(1, 15);
BENCHMARK(GenerateRomanNumbersLowercase)->RangeMultiplier(2)->Range(1, 15);

}  // namespace slugkit::generator::benchmarks
