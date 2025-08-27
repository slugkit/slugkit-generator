#include <benchmark/benchmark.h>

#include <slugkit/generator/pattern_generator.hpp>

#include <userver/engine/run_standalone.hpp>

namespace slugkit::generator::benchmarks {

using namespace literals;

void GenerateHexNumbers16(benchmark::State& state) {
    auto generator = NumberSubstitutionGenerator{"number:16x"_number_gen};
    auto seed_hash = PatternGenerator::SeedHash("test");
    for ([[maybe_unused]] auto _ : state) {
        auto result = generator.Generate(seed_hash, 0);
        benchmark::DoNotOptimize(result);
    }
}

void GenerateDecNumbers18(benchmark::State& state) {
    auto generator = NumberSubstitutionGenerator{"number:18d"_number_gen};
    auto seed_hash = PatternGenerator::SeedHash("test");
    for ([[maybe_unused]] auto _ : state) {
        auto result = generator.Generate(seed_hash, 0);
        benchmark::DoNotOptimize(result);
    }
}

// 15 is the maximum number of digits for roman numerals
void GenerateRomanNumbersUppercase(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
        auto generator = RomanSubstitutionGenerator{"number:15R"_number_gen};
        auto seed_hash = PatternGenerator::SeedHash("test");
        for ([[maybe_unused]] auto _ : state) {
            auto result = generator.Generate(seed_hash, 0);
            benchmark::DoNotOptimize(result);
        }
    });
}

void GenerateRomanNumbersLowercase(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
        auto generator = RomanSubstitutionGenerator{"number:15r"_number_gen};
        auto seed_hash = PatternGenerator::SeedHash("test");
        for ([[maybe_unused]] auto _ : state) {
            auto result = generator.Generate(seed_hash, 0);
            benchmark::DoNotOptimize(result);
        }
    });
}

BENCHMARK(GenerateHexNumbers16);
BENCHMARK(GenerateDecNumbers18);
BENCHMARK(GenerateRomanNumbersUppercase);
BENCHMARK(GenerateRomanNumbersLowercase);

}  // namespace slugkit::generator::benchmarks
