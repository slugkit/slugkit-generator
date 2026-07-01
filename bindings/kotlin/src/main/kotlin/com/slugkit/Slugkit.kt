package com.slugkit

/** The capacity of a pattern: total number of distinct slugs plus the maximum length. */
data class Capacity(
    /** Total capacity; may exceed 64 bits, so it is exposed as a decimal string. */
    val value: String,
    /** The engine's upper bound on slug length for the pattern. */
    val maxLength: Int,
)

/**
 * A deterministic human-readable ID generator backed by one or more compiled binary dictionaries.
 *
 * Thread-safe for concurrent generation. Construct once and reuse; call [close] (or use
 * `use { ... }`) to release the native handle.
 */
class Generator private constructor(private val handle: Long) : AutoCloseable {

    /** The native library version. */
    val version: String get() = SlugkitJni.version()

    /** Produce a fresh random seed. */
    fun randomSeed(): String = SlugkitJni.randomSeed(handle)

    /** Compute a pattern's capacity. */
    fun capacity(pattern: String): Capacity {
        val (value, maxLen) = SlugkitJni.capacity(handle, pattern)
        return Capacity(value, maxLen.toInt())
    }

    /** Generate a single slug for `(pattern, seed, sequence)`. */
    fun generate(pattern: String, seed: String, sequence: Long): String =
        SlugkitJni.generate(handle, pattern, seed, sequence)

    /** Generate [count] slugs starting at [sequence]. */
    fun generate(pattern: String, seed: String, sequence: Long, count: Int): List<String> =
        SlugkitJni.generateBatch(handle, pattern, seed, sequence, count).asList()

    override fun close() = SlugkitJni.destroy(handle)

    companion object {
        /** Create a generator from one or more compiled binary dictionaries (`.bin`). */
        fun fromBinaryDictionaries(dictionaries: List<ByteArray>): Generator =
            Generator(SlugkitJni.create(dictionaries.toTypedArray()))

        /** Create a generator from a single compiled binary dictionary. */
        fun fromBinaryDictionary(dictionary: ByteArray): Generator =
            fromBinaryDictionaries(listOf(dictionary))
    }
}

/**
 * Low-level JNI entry points into libslugkit_jni. Prefer [Generator]; this is public only so the
 * native symbol names (Java_com_slugkit_SlugkitJni_*) are stable.
 */
internal object SlugkitJni {
    init {
        System.loadLibrary("slugkit_jni")
    }

    external fun create(dictionaries: Array<ByteArray>): Long
    external fun destroy(handle: Long)
    external fun randomSeed(handle: Long): String
    /** Returns [value, maxLength] as strings. */
    external fun capacity(handle: Long, pattern: String): Array<String>
    external fun generate(handle: Long, pattern: String, seed: String, sequence: Long): String
    external fun generateBatch(handle: Long, pattern: String, seed: String, sequence: Long, count: Int): Array<String>
    external fun version(): String
}
