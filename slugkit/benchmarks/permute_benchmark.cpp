#include <benchmark/benchmark.h>

#include <slugkit/generator/permutations.hpp>

namespace slugkit::generator::benchmarks {

void FNV1aHash(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(generator::FNV1aHash("00000000-0000-0000-0000-000000000000"));
    }
}

void PermutePowerOf2(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(generator::PermutePowerOf2(2ULL << 63, 0, 0, kDefaultRounds));
    }
}

void Permute(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(generator::Permute(100'000'000'000, 0, 0, kDefaultRounds));
    }
}

BENCHMARK(FNV1aHash);
BENCHMARK(PermutePowerOf2);
BENCHMARK(Permute);

}  // namespace slugkit::generator::benchmarks
