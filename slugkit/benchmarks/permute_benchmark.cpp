#include <benchmark/benchmark.h>

#include <slugkit/generator/permutations.hpp>

#include <fmt/format.h>

namespace slugkit::generator::benchmarks {

namespace {

auto FillString(std::size_t size) -> std::string {
    std::string result;
    result.reserve(size);
    for (std::size_t i = 0; i < size; ++i) {
        result.push_back('0');
    }
    return result;
}

auto PowerOf10(std::int64_t power) -> std::uint64_t {
    std::uint64_t result = 1;
    for (std::int64_t i = 0; i < power; ++i) {
        result *= 10;
    }
    return result;
}

}  // namespace

void FNV1aHash(benchmark::State& state) {
    auto str = FillString(state.range(0));
    state.SetLabel(fmt::format("{} chars", state.range(0)));
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(generator::FNV1aHash(str));
    }
}

void PermutePowerOf2(benchmark::State& state) {
    auto power = state.range(0);
    auto limit = 1ULL << static_cast<std::uint64_t>(power);
    state.SetLabel(fmt::format("2^{}", power));
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(generator::PermutePowerOf2(limit, 0, 0, kDefaultRounds));
    }
}

void Permute(benchmark::State& state) {
    auto power = state.range(0);
    auto limit = PowerOf10(static_cast<std::int64_t>(power));
    state.SetLabel(fmt::format("10^{}", power));
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(generator::Permute(limit, 0, 0, kDefaultRounds));
    }
}

BENCHMARK(FNV1aHash)->RangeMultiplier(2)->Range(1, 128);
BENCHMARK(PermutePowerOf2)->RangeMultiplier(2)->Range(1, 18);
BENCHMARK(Permute)->RangeMultiplier(2)->Range(1, 18);

}  // namespace slugkit::generator::benchmarks
