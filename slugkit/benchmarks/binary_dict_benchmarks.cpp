#include <benchmark/benchmark.h>

#include <slugkit/test_utils/data_config.hpp>
#include <slugkit/test_utils/test_dictionary.hpp>

#include <slugkit/generator/binary_dictionary.hpp>
#include <slugkit/utils/memory_mapped_file.hpp>

#include <fmt/format.h>

namespace slugkit::generator::binary::benchmarks {

using namespace literals;
using namespace generator::literals;

namespace {
// clang-format off
const std::vector<Selector> kSelectors = {
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
    "word:+det"_selector,
    "word:+emo"_selector,
    "word:+obj"_selector,
    "word:+nsfw"_selector,
    "word:-det"_selector,
    "word:-emo"_selector,
    "word:-obj"_selector,
    "word:-nsfw"_selector,
    "word:+det -pos"_selector,
    "word:+det +pos"_selector,
    "word:+det +pos -nsfw"_selector,
    "word:+det==5"_selector,
    "word:+det +pos==10"_selector,
    "word:+det +pos!=10"_selector,
    "word:+det +pos -nsfw==15"_selector,
    "word:+det +pos -nsfw==20"_selector,
    "word:+det<8"_selector,
    "word:+det>=8"_selector,
};
// clang-format on
}  // namespace

void BinaryDictionaryInMemLoad(benchmark::State& state) {
    auto dictionary = BinaryDictionary(test::kDictionaryTestData);
    state.SetLabel(fmt::format("InMemLoad ({} words)", dictionary.Count()));
    for ([[maybe_unused]] auto _ : state) {
        auto dictionary = BinaryDictionary(test::kDictionaryTestData);
        benchmark::DoNotOptimize(dictionary);
    }
}

void BinaryDictionaryMappedLoad(benchmark::State& state) {
    utils::MemoryMappedFile file(test::kTestDictionaryFile);
    auto dictionary = BinaryDictionary(file.data());
    state.SetLabel(fmt::format("MappedLoad ({} words)", dictionary.Count()));
    for ([[maybe_unused]] auto _ : state) {
        utils::MemoryMappedFile file(test::kTestDictionaryFile);
        auto dictionary = BinaryDictionary(file.data());
        benchmark::DoNotOptimize(dictionary);
    }
}

void BinaryDictionaryInMemAccess(benchmark::State& state) {
    auto dictionary = BinaryDictionary(test::kDictionaryTestData);
    state.SetLabel(fmt::format("InMemAccess ({} words)", dictionary.Count()));
    auto count = dictionary.Count();
    auto index = 0_idx;
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(dictionary[index]);
        index = (index + 1) % count;
    }
}

void BinaryDictionaryMappedAccess(benchmark::State& state) {
    utils::MemoryMappedFile file(test::kTestDictionaryFile);
    auto dictionary = BinaryDictionary(file.data());
    state.SetLabel(fmt::format("MappedAccess ({} words)", dictionary.Count()));
    auto count = dictionary.Count();
    auto index = 0_idx;
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(dictionary[index]);
        index = (index + 1) % count;
    }
}

void BinaryDictionaryInMemFilterNoCache(benchmark::State& state) {
    auto dictionary = BinaryDictionary(test::kDictionaryTestData);
    auto selector = kSelectors[state.range(0)];
    state.SetLabel(fmt::format("InMemFilter ({} words) {}", dictionary.Count(), selector.ToString()));
    for ([[maybe_unused]] auto _ : state) {
        auto filtered_dictionary = dictionary.Filter(selector);
        benchmark::DoNotOptimize(filtered_dictionary);
    }
}

void BinaryDictionaryMappedFilterNoCache(benchmark::State& state) {
    utils::MemoryMappedFile file(test::kTestDictionaryFile);
    auto dictionary = BinaryDictionary(file.data());
    auto selector = kSelectors[state.range(0)];
    state.SetLabel(fmt::format("MappedFilter ({} words) {}", dictionary.Count(), selector.ToString()));
    for ([[maybe_unused]] auto _ : state) {
        auto filtered_dictionary = dictionary.Filter(selector);
        benchmark::DoNotOptimize(filtered_dictionary);
    }
}

BENCHMARK(BinaryDictionaryInMemLoad);
BENCHMARK(BinaryDictionaryMappedLoad);
BENCHMARK(BinaryDictionaryInMemAccess);
BENCHMARK(BinaryDictionaryMappedAccess);
BENCHMARK(BinaryDictionaryInMemFilterNoCache)->DenseRange(0, kSelectors.size() - 1);
BENCHMARK(BinaryDictionaryMappedFilterNoCache)->DenseRange(0, kSelectors.size() - 1);

}  // namespace slugkit::generator::binary::benchmarks