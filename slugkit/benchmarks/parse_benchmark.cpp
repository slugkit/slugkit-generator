#include <benchmark/benchmark.h>

#include <slugkit/generator/pattern.hpp>

#include <fmt/format.h>

namespace slugkit::generator::benchmarks {

namespace {
const std::vector<std::string> kPlaceholders = {
    "{number:8d}",
    "{special:8}",
    "{special:8-12}",
    "{noun}",
    "{Noun}",
    "{NOUN}",
    "{nOun}",
    "{adjective}",
    "{ADJECTIVE}",
    "{Adjective}",
    "{aDjective}",
    "{adjective:+tag}",
    "{adjective:+tag1-tag2}",
    "{adjective:==10}",
    "{adjective:+tag1-tag2==10}",
    "{adjective}-{noun}",
    "{adjective}-{noun}-{verb}",
    "{adverb}-{adjective}-{noun}-{number:4x}",
};
}  // namespace

void ParsePattern(benchmark::State& state) {
    auto pattern_str = kPlaceholders[state.range(0)];
    state.SetLabel(pattern_str);
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{pattern_str};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

BENCHMARK(ParsePattern)->DenseRange(0, kPlaceholders.size() - 1);

}  // namespace slugkit::generator::benchmarks
