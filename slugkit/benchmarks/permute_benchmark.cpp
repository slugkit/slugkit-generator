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

void UniquePermutationArgs(benchmark::internal::Benchmark* bench) {
    for (std::int64_t sequence_length = 2; sequence_length <= 8; sequence_length *= 2) {
        for (std::int64_t alphabet_size = 32; alphabet_size <= 2048; alphabet_size *= 2) {
            bench->Args({alphabet_size, sequence_length});
        }
    }
    for (std::int64_t sequence_length = 2; sequence_length <= 8; ++sequence_length) {
        bench->Args({1200, sequence_length});
    }
}

void UniquePermutation(benchmark::State& state) {
    auto alphabet_size = state.range(0);
    auto sequence_length = state.range(1);
    state.SetLabel(fmt::format("sequence length {} alphabet size {}", sequence_length, alphabet_size));
    auto i = 0ULL;
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(generator::UniquePermutation(alphabet_size, sequence_length, i++));
    }
}

void UniquePermutationSeedHash(benchmark::State& state) {
    auto alphabet_size = state.range(0);
    auto sequence_length = state.range(1);
    auto seed_hash = generator::FNV1aHash("test");
    state.SetLabel(fmt::format("sequence length {} alphabet size {}", sequence_length, alphabet_size));
    auto i = 0ULL;
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(generator::UniquePermutation(seed_hash, alphabet_size, sequence_length, i++));
    }
}

void NonUniquePermutation(benchmark::State& state) {
    auto alphabet_size = state.range(0);
    auto sequence_length = state.range(1);
    state.SetLabel(fmt::format("sequence length {} alphabet size {}", sequence_length, alphabet_size));
    auto i = 0ULL;
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(generator::NonUniquePermutation(alphabet_size, sequence_length, i++));
    }
}

void NonUniquePermutationSeedHash(benchmark::State& state) {
    auto alphabet_size = state.range(0);
    auto sequence_length = state.range(1);
    auto seed_hash = generator::FNV1aHash("test");
    state.SetLabel(fmt::format("sequence length {} alphabet size {}", sequence_length, alphabet_size));
    auto i = 0ULL;
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(generator::NonUniquePermutation(seed_hash, alphabet_size, sequence_length, i++));
    }
}

BENCHMARK(FNV1aHash)->RangeMultiplier(2)->Range(1, 128);
BENCHMARK(PermutePowerOf2)->RangeMultiplier(2)->Range(1, 18);
BENCHMARK(Permute)->RangeMultiplier(2)->Range(1, 18);
BENCHMARK(UniquePermutation)->Apply(UniquePermutationArgs);
BENCHMARK(UniquePermutationSeedHash)->Apply(UniquePermutationArgs);
BENCHMARK(NonUniquePermutation)->Apply(UniquePermutationArgs);
BENCHMARK(NonUniquePermutationSeedHash)->Apply(UniquePermutationArgs);

}  // namespace slugkit::generator::benchmarks
