#pragma once

#include <slugkit/generator/binary_dictionary.hpp>
#include <slugkit/generator/dictionary.hpp>

#include <span>
#include <vector>

namespace slugkit::generator::test {

extern const std::span<const std::byte> kDictionaryTestData;

// The real emoji dictionary (kind "emoji") compiled from db/dicts/emoji.yaml into
// tests/data/emoji.bin, used to exercise the file-based emoji path and in-memory/binary parity.
extern const std::span<const std::byte> kEmojiTestData;

// Decompile a binary dictionary into in-memory Dictionaries (one per language), carrying the same
// words and per-word tags. Lets tests build an in-memory set from identical data, or assemble a
// mixed set (e.g. fake selectors plus the real emoji dictionary).
std::vector<Dictionary> Decompile(const binary::BinaryDictionary& dict);

}  // namespace slugkit::generator::test
