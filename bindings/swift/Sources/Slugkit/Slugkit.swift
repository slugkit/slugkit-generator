import CSlugkit
import Foundation

/// An error thrown by the SlugKit engine, carrying the message from the native layer.
public struct SlugkitError: Error, CustomStringConvertible {
    public let message: String
    public init(_ message: String) { self.message = message }
    public var description: String { message }
}

/// The capacity of a pattern: how many distinct slugs it can produce, plus the maximum length.
public struct Capacity {
    /// Total capacity as a decimal string (the value can exceed 64 bits).
    public let value: String
    /// The engine's upper bound on slug length for the pattern.
    public let maxLength: Int32
}

/// A deterministic human-readable ID generator backed by one or more compiled binary dictionaries.
///
/// Thread-safe for concurrent generation. Construct once and reuse.
public final class Generator {
    private let handle: OpaquePointer

    /// Create a generator from one or more compiled binary dictionaries (`.bin`).
    /// The dictionary bytes are copied internally; the `Data` values need not outlive this call.
    public init(binaryDictionaries dictionaries: [Data]) throws {
        var err: UnsafeMutablePointer<CChar>?
        // Flatten each Data into a stable base-address + length, then hand the C API arrays of both.
        let handle: OpaquePointer? = Generator.withDataPointers(dictionaries) { datas, lens in
            slk_generator_create(datas, lens, dictionaries.count, &err)
        }
        guard let handle else {
            throw Generator.takeError(err, fallback: "failed to create generator")
        }
        self.handle = handle
    }

    /// Create a generator from a single compiled binary dictionary.
    public convenience init(binaryDictionary data: Data) throws {
        try self.init(binaryDictionaries: [data])
    }

    deinit { slk_generator_destroy(handle) }

    /// The native library version.
    public var version: String { String(cString: slk_version()) }

    /// Produce a fresh random seed.
    public func randomSeed() throws -> String {
        var err: UnsafeMutablePointer<CChar>?
        guard let c = slk_random_seed(handle, &err) else {
            throw Generator.takeError(err, fallback: "failed to produce seed")
        }
        defer { slk_string_free(c) }
        return String(cString: c)
    }

    /// Compute a pattern's capacity.
    public func capacity(of pattern: String) throws -> Capacity {
        var capacityStr: UnsafeMutablePointer<CChar>?
        var maxLen: Int32 = 0
        var err: UnsafeMutablePointer<CChar>?
        let status = slk_capacity(handle, pattern, &capacityStr, &maxLen, &err)
        guard status == SLK_OK, let capacityStr else {
            throw Generator.takeError(err, fallback: "failed to compute capacity")
        }
        defer { slk_string_free(capacityStr) }
        return Capacity(value: String(cString: capacityStr), maxLength: maxLen)
    }

    /// Generate a single slug for `(pattern, seed, sequence)`.
    public func generate(_ pattern: String, seed: String, sequence: UInt64) throws -> String {
        var err: UnsafeMutablePointer<CChar>?
        guard let c = slk_generate_alloc(handle, pattern, seed, sequence, &err) else {
            throw Generator.takeError(err, fallback: "generation failed")
        }
        defer { slk_string_free(c) }
        return String(cString: c)
    }

    /// Generate `count` slugs starting at `sequence`, delivering each to `body`.
    /// Avoids per-slug allocation on the native side.
    public func generate(
        _ pattern: String, seed: String, sequence: UInt64, count: Int,
        _ body: (String) -> Void
    ) throws {
        var err: UnsafeMutablePointer<CChar>?
        let status = withoutActuallyEscaping(body) { body -> slk_status in
            var sink = body
            return withUnsafeMutablePointer(to: &sink) { ctx in
                slk_generate_batch(handle, pattern, seed, sequence, count, { rawCtx, slug, len in
                    guard let rawCtx, let slug else { return }
                    let sink = rawCtx.assumingMemoryBound(to: ((String) -> Void).self).pointee
                    sink(String(cString: slug))
                }, ctx, &err)
            }
        }
        guard status == SLK_OK else {
            throw Generator.takeError(err, fallback: "batch generation failed")
        }
    }

    // MARK: - Helpers

    /// Consume a `char*` error out-param: build a Swift string and free the native buffer.
    private static func takeError(_ err: UnsafeMutablePointer<CChar>?, fallback: String) -> SlugkitError {
        guard let err else { return SlugkitError(fallback) }
        defer { slk_string_free(err) }
        return SlugkitError(String(cString: err))
    }

    /// Invoke `body` with parallel C arrays of each Data's base address and length.
    private static func withDataPointers<R>(
        _ dictionaries: [Data],
        _ body: (_ datas: UnsafePointer<UnsafePointer<UInt8>?>?, _ lens: UnsafePointer<Int>?) -> R
    ) -> R {
        func recurse(_ index: Int, _ ptrs: inout [UnsafePointer<UInt8>?], _ lens: inout [Int]) -> R {
            if index == dictionaries.count {
                return ptrs.withUnsafeBufferPointer { pbuf in
                    lens.withUnsafeBufferPointer { lbuf in
                        body(pbuf.baseAddress, lbuf.baseAddress)
                    }
                }
            }
            return dictionaries[index].withUnsafeBytes { raw in
                ptrs.append(raw.bindMemory(to: UInt8.self).baseAddress)
                lens.append(raw.count)
                return recurse(index + 1, &ptrs, &lens)
            }
        }
        var ptrs: [UnsafePointer<UInt8>?] = []
        var lens: [Int] = []
        ptrs.reserveCapacity(dictionaries.count)
        lens.reserveCapacity(dictionaries.count)
        return recurse(0, &ptrs, &lens)
    }
}
