import XCTest
import Foundation
@testable import Slugkit

final class SlugkitTests: XCTestCase {
    /// Locate the committed emoji.bin in the generator tree (no need to duplicate the binary here).
    private func emojiDictionary() throws -> Data {
        // .../bindings/swift/Tests/SlugkitTests/SlugkitTests.swift -> generator root
        let genRoot = URL(fileURLWithPath: #filePath)
            .deletingLastPathComponent()  // SlugkitTests
            .deletingLastPathComponent()  // Tests
            .deletingLastPathComponent()  // swift
            .deletingLastPathComponent()  // bindings
            .deletingLastPathComponent()  // generator root
        let bin = genRoot
            .appendingPathComponent("slugkit/tests/data/emoji.bin")
        return try Data(contentsOf: bin)
    }

    private func adverbDictionary() throws -> Data {
        let genRoot = URL(fileURLWithPath: #filePath)
            .deletingLastPathComponent()  // SlugkitTests
            .deletingLastPathComponent()  // Tests
            .deletingLastPathComponent()  // swift
            .deletingLastPathComponent()  // bindings
            .deletingLastPathComponent()  // generator root
        return try Data(contentsOf: genRoot.appendingPathComponent("slugkit/tests/data/test-adv.slugs"))
    }

    /// A small multi-language dictionary (kind "colour") with distinct en/fr/de pools.
    private func colourDictionary() throws -> Data {
        let genRoot = URL(fileURLWithPath: #filePath)
            .deletingLastPathComponent()  // SlugkitTests
            .deletingLastPathComponent()  // Tests
            .deletingLastPathComponent()  // swift
            .deletingLastPathComponent()  // bindings
            .deletingLastPathComponent()  // generator root
        return try Data(contentsOf: genRoot.appendingPathComponent("slugkit/tests/data/multilang.colour.bin"))
    }

    /// Multi-dictionary (adverb + emoji, n=2). Golden values (seed "foobar") are byte-identical
    /// across all language bindings.
    private func multi() throws -> Generator {
        try Generator(binaryDictionaries: [adverbDictionary(), emojiDictionary()])
    }

    func testMultiCapacity() throws {
        let cap = try multi().capacity(of: "{adverb}-{emoji}")
        // Opt-in `nsfw` (4 adverbs) hidden by default -> pool 3615, 3615 * 1154 = 4171710.
        XCTAssertEqual(cap.value, "4171710")
        XCTAssertEqual(cap.maxLength, 22)
    }

    func testMultiGoldenPrefixes() throws {
        let gen = try multi()
        XCTAssertTrue(try gen.generate("{adverb}-{emoji}", seed: "foobar", sequence: 0).hasPrefix("impotently-"))
        XCTAssertTrue(try gen.generate("{adverb}-{emoji}", seed: "foobar", sequence: 1).hasPrefix("speechlessly-"))
        XCTAssertTrue(try gen.generate("{adverb}-{emoji}", seed: "foobar", sequence: 2).hasPrefix("diagonally-"))
    }

    func testMultiLengthAndNumberGolden() throws {
        let gen = try multi()
        XCTAssertEqual(try gen.generate("{adverb:<=5}-{number:3d}", seed: "foobar", sequence: 0), "ago-887")
        XCTAssertEqual(try gen.generate("{adverb:<=5}-{number:3d}", seed: "foobar", sequence: 1), "aloud-774")
        XCTAssertEqual(try gen.generate("{adverb:<=5}-{number:3d}", seed: "foobar", sequence: 2), "apart-661")
    }

    func testMultiBatchCollisionFree() throws {
        var slugs: [String] = []
        try multi().generate("{adverb}-{emoji}", seed: "foobar", sequence: 0, count: 20) { slugs.append($0) }
        XCTAssertEqual(Set(slugs).count, 20)
    }

    /// Multi-language selection is built into the binary dictionary: kind "colour" holds distinct
    /// en/fr/de pools, and {colour@lang} selects within it.
    func testMultiLanguageSelection() throws {
        let gen = try Generator(binaryDictionary: try colourDictionary())
        XCTAssertEqual(try gen.capacity(of: "{colour@en}").value, "3")
        XCTAssertEqual(try gen.capacity(of: "{colour@fr}").value, "2")
        XCTAssertEqual(try gen.capacity(of: "{colour@de}").value, "4")
        // No language defaults to English -- same capacity and byte-identical output.
        XCTAssertEqual(try gen.capacity(of: "{colour}").value, "3")
        XCTAssertEqual(try gen.generate("{colour}", seed: "foobar", sequence: 0),
                       try gen.generate("{colour@en}", seed: "foobar", sequence: 0))
        // A French colour is French.
        let fr = try gen.generate("{colour@fr}", seed: "foobar", sequence: 0)
        XCTAssertTrue(fr == "rouge" || fr == "vert")
        // Two languages combine: LCM(3, 4) = 12.
        XCTAssertEqual(try gen.capacity(of: "{colour@en}-{colour@de}").value, "12")
        // An absent language is an error.
        XCTAssertThrowsError(try gen.generate("{colour@es}", seed: "foobar", sequence: 0))
    }

    func testVersion() throws {
        let gen = try Generator(binaryDictionary: emojiDictionary())
        XCTAssertFalse(gen.version.isEmpty)
    }

    func testRandomSeedNonEmpty() throws {
        let gen = try Generator(binaryDictionary: try emojiDictionary())
        XCTAssertFalse(try gen.randomSeed().isEmpty)
    }

    func testCapacity() throws {
        let gen = try Generator(binaryDictionary: try emojiDictionary())
        let cap = try gen.capacity(of: "{emoji}")
        XCTAssertEqual(cap.value, "1154")
        XCTAssertEqual(cap.maxLength, 1)
    }

    func testDeterministicGeneration() throws {
        let gen = try Generator(binaryDictionary: try emojiDictionary())
        let a = try gen.generate("{emoji}", seed: "foobar", sequence: 0)
        let b = try gen.generate("{emoji}", seed: "foobar", sequence: 0)
        XCTAssertFalse(a.isEmpty)
        XCTAssertEqual(a, b)
    }

    func testBatch() throws {
        let gen = try Generator(binaryDictionary: try emojiDictionary())
        var slugs: [String] = []
        try gen.generate("{emoji}", seed: "foobar", sequence: 0, count: 5) { slugs.append($0) }
        XCTAssertEqual(slugs.count, 5)
        // Same seed/sequence as the single-shot call -> first batch slug matches generate(seq: 0).
        XCTAssertEqual(slugs.first, try gen.generate("{emoji}", seed: "foobar", sequence: 0))
    }

    func testMalformedPatternThrows() throws {
        let gen = try Generator(binaryDictionary: try emojiDictionary())
        XCTAssertThrowsError(try gen.generate("{", seed: "foobar", sequence: 0))
    }
}
