#include <slugkit/generator/binary_dictionary.hpp>
#include <slugkit/generator/generator.hpp>

#ifdef SLUGKIT_USE_USERVER
#include <userver/engine/run_standalone.hpp>
#endif

#include <CLI/CLI.hpp>
#include <fmt/format.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <vector>

// slugkit: generate human-readable IDs from compiled binary (.bin/.slugs) dictionaries. This is
// the shipped, userver-free CLI -- build the generator standalone (SLUGKIT_USE_USERVER=OFF) and it
// depends only on fmt, utf8proc, and the header-only CLI11. Compile source YAML dictionaries into
// the binary format with the `compile-dict` Python tool first.

namespace {

std::shared_ptr<std::vector<std::byte>> ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open file: {}", path));
    }
    const auto size = static_cast<std::size_t>(file.tellg());
    file.seekg(0);
    auto buffer = std::make_shared<std::vector<std::byte>>(size);
    file.read(reinterpret_cast<char*>(buffer->data()), static_cast<std::streamsize>(size));
    return buffer;
}

}  // namespace

int main(int argc, char* argv[]) try {
    CLI::App app{"slugkit -- generate human-readable IDs from compiled binary dictionaries"};

    std::vector<std::string> bin_files;
    std::string pattern;
    std::string seed;
    std::vector<std::string> enable_opt_ins;
    std::size_t count = 1;
    std::size_t sequence = 0;
    bool quiet = false;

    app.add_option("-b,--bin", bin_files, "Compiled .bin/.slugs dictionary file(s) to load")
        ->required()
        ->expected(-1)  // one or more
        ->check(CLI::ExistingFile);
    app.add_option("-p,--pattern", pattern, "Pattern to generate, e.g. '{adjective}-{noun}-{number:3d}'")->required();
    app.add_option("-c,--count", count, "Number of slugs to generate")->capture_default_str();
    app.add_option("-n,--sequence", sequence, "Starting sequence number")->capture_default_str();
    app.add_option("-s,--seed", seed, "Seed (random if omitted)");
    app.add_option("--enable-opt-in", enable_opt_ins, "Un-hide an opt-in tag (repeatable), e.g. --enable-opt-in nsfw");
    app.add_flag("-q,--quiet", quiet, "Only print the generated slugs (no header/timing on stderr)");

    CLI11_PARSE(app, argc, argv);

    auto run = [&] {
        slugkit::generator::binary::DictionarySet dictionaries;
        for (const auto& path : bin_files) {
            auto buffer = ReadFile(path);
            std::span<const std::byte> data{buffer->data(), buffer->size()};
            dictionaries.Add(data, buffer);  // buffer kept alive by the DictionarySet
            if (!quiet) {
                std::cerr << fmt::format("Loaded {} ({} bytes)\n", path, buffer->size());
            }
        }

        slugkit::generator::Generator generator(std::move(dictionaries));
        for (const auto& tag : enable_opt_ins) {
            generator.EnableOptIn(tag);
        }
        if (seed.empty()) {
            seed = generator.RandomSeed();
        }

        auto pattern_ptr = std::make_shared<slugkit::generator::Pattern>(pattern);
        if (!quiet) {
            std::cerr << fmt::format(
                "Pattern: {}\nComplexity: {}\nSeed: {}\n---\n", pattern, pattern_ptr->Complexity(), seed
            );
        }

        const auto start = std::chrono::steady_clock::now();
        if (count == 1) {
            std::cout << generator(pattern_ptr, seed, sequence) << '\n';
        } else {
            generator(pattern_ptr, seed, sequence, count, [](const std::string& slug) { std::cout << slug << '\n'; });
        }
        if (!quiet) {
            const auto elapsed = std::chrono::steady_clock::now() - start;
            const auto ms = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() / 1000.0;
            std::cerr << fmt::format(
                "---\nGenerated {} slug(s) in {:.3f} ms ({:.3f} ms/slug)\n", count, ms, ms / static_cast<double>(count)
            );
        }
    };

#ifdef SLUGKIT_USE_USERVER
    // The userver build's Generator uses userver::engine synchronisation primitives, which need a
    // coroutine context; run inside one. The standalone build uses std::mutex and runs directly.
    userver::engine::RunStandalone(run);
#else
    run();
#endif

    return 0;
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
}
