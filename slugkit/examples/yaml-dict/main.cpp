#include <slugkit/generator/generator.hpp>
#include <slugkit/generator/structured_loader.hpp>

#include <userver/engine/run_standalone.hpp>
#include <userver/formats/yaml.hpp>

#include <CLI/CLI.hpp>
#include <fmt/format.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

// Example: generate slugs directly from a YAML dictionary. YAML loading goes through userver's
// formats, so this example only builds in the userver build (SLUGKIT_USE_USERVER=ON). For a
// userver-free workflow, compile the YAML to a binary dictionary with the `compile-dict` tool and
// use the standalone `slugkit` CLI.

int main(int argc, char* argv[]) try {
    CLI::App app{"yaml-dict -- generate slugs from a YAML dictionary (userver build)"};

    std::string file_name;
    std::string pattern;
    std::string seed;
    std::size_t count = 1;
    std::size_t sequence = 0;

    app.add_option("-f,--file", file_name, "YAML dictionary file")->required()->check(CLI::ExistingFile);
    app.add_option("-p,--pattern", pattern, "Pattern to generate")->required();
    app.add_option("-c,--count", count, "Number of slugs to generate")->capture_default_str();
    app.add_option("-n,--sequence", sequence, "Starting sequence number")->capture_default_str();
    app.add_option("-s,--seed", seed, "Seed (random if omitted)");

    CLI11_PARSE(app, argc, argv);

    std::ifstream file(file_name);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open file: {}", file_name));
    }

    auto dictionary_set = slugkit::generator::DictionarySet::Parse<userver::formats::yaml::Value>(file);
    slugkit::generator::Generator generator(std::move(dictionary_set));
    if (seed.empty()) {
        seed = generator.RandomSeed();
    }

    userver::engine::RunStandalone([&] {
        auto pattern_ptr = std::make_shared<slugkit::generator::Pattern>(pattern);
        std::cerr << "Pattern complexity: " << pattern_ptr->Complexity() << "\n---\n";
        if (count == 1) {
            std::cout << generator(pattern_ptr, seed, sequence) << '\n';
        } else {
            generator(pattern_ptr, seed, sequence, count, [](const std::string& slug) { std::cout << slug << '\n'; });
        }
    });

} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
}
