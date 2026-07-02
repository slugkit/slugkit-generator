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
