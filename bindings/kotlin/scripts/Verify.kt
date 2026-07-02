package com.slugkit

import java.io.File

// Host-JVM verification of the JNI bridge + Kotlin wrapper. Loads libslugkit_jni from
// java.library.path and drives the real com.slugkit.Generator API against emoji.bin.
fun main(args: Array<String>) {
    require(args.size >= 2) { "usage: Verify <path-to-emoji.bin> <path-to-test-adv.slugs>" }
    val emoji = File(args[0]).readBytes()
    val adverb = File(args[1]).readBytes()

    Generator.fromBinaryDictionary(emoji).use { gen ->
        check(gen.version.isNotEmpty()) { "version empty" }
        check(gen.randomSeed().isNotEmpty()) { "seed empty" }

        val cap = gen.capacity("{emoji}")
        check(cap.value == "1154") { "capacity=${cap.value}" }
        check(cap.maxLength == 1) { "maxLength=${cap.maxLength}" }

        val a = gen.generate("{emoji}", "foobar", 0)
        val b = gen.generate("{emoji}", "foobar", 0)
        check(a.isNotEmpty() && a == b) { "not deterministic: $a vs $b" }

        val batch = gen.generate("{emoji}", "foobar", 0, 5)
        check(batch.size == 5) { "batch size=${batch.size}" }
        check(batch.first() == a) { "batch[0]=${batch.first()} != $a" }

        var threw = false
        try {
            gen.generate("{", "foobar", 0)
        } catch (e: Exception) {
            threw = true
        }
        check(threw) { "malformed pattern should throw" }
    }

    // Multi-dictionary (adverb + emoji, n=2) with real word patterns.
    // Golden values (seed "foobar") are byte-identical across all language bindings.
    Generator.fromBinaryDictionaries(listOf(adverb, emoji)).use { gen ->
        val cap = gen.capacity("{adverb}-{emoji}")
        // Opt-in `nsfw` (4 adverbs) hidden by default -> pool 3615, 3615 * 1154 = 4171710.
        check(cap.value == "4171710") { "multi capacity=${cap.value}" }
        check(cap.maxLength == 22) { "multi maxLength=${cap.maxLength}" }

        check(gen.generate("{adverb}-{emoji}", "foobar", 0).startsWith("impotently-")) { "seq0 prefix" }
        check(gen.generate("{adverb}-{emoji}", "foobar", 1).startsWith("speechlessly-")) { "seq1 prefix" }
        check(gen.generate("{adverb}-{emoji}", "foobar", 2).startsWith("diagonally-")) { "seq2 prefix" }

        check(gen.generate("{adverb:<=5}-{number:3d}", "foobar", 0) == "ago-887") { "num seq0" }
        check(gen.generate("{adverb:<=5}-{number:3d}", "foobar", 1) == "aloud-774") { "num seq1" }
        check(gen.generate("{adverb:<=5}-{number:3d}", "foobar", 2) == "apart-661") { "num seq2" }

        val batch = gen.generate("{adverb}-{emoji}", "foobar", 0, 20)
        check(batch.toSet().size == 20) { "batch not collision-free: ${batch.toSet().size}" }
    }

    println("ALL KOTLIN JNI CHECKS PASSED")
}
