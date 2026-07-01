package com.slugkit

import java.io.File

// Host-JVM verification of the JNI bridge + Kotlin wrapper. Loads libslugkit_jni from
// java.library.path and drives the real com.slugkit.Generator API against emoji.bin.
fun main(args: Array<String>) {
    require(args.isNotEmpty()) { "usage: Verify <path-to-emoji.bin>" }
    val emoji = File(args[0]).readBytes()

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

    println("ALL KOTLIN JNI CHECKS PASSED")
}
