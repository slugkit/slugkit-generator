/**
 * @file slugkit/c/slugkit_c.h
 * @brief Stable C ABI for the SlugKit generator engine.
 *
 * This is the FFI substrate for native bindings (Swift, Kotlin/JNI, Dart ffi, ...).
 * It exposes only C types across the boundary: opaque handles, C strings (UTF-8),
 * sized integers and a callback. No C++ types leak; no exceptions cross the boundary
 * (they are translated to status codes + an error message).
 *
 * Threading: a slk_generator is safe to use concurrently for generation from multiple
 * threads (the underlying engine guards its caches). It does NOT require any coroutine
 * or runtime context in the standalone (userver-free) build.
 *
 * Memory ownership:
 *  - Buffers passed to slk_generator_create* are COPIED internally; the caller may free
 *    them immediately after the call returns.
 *  - Any `char*` returned by this API (slugs, capacity strings, error messages) is
 *    heap-allocated and must be released with slk_string_free().
 */
#ifndef SLUGKIT_C_H
#define SLUGKIT_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque generator handle. */
typedef struct slk_generator slk_generator;

/** Status codes returned by the API. */
typedef enum slk_status {
    SLK_OK = 0,                  /**< success */
    SLK_ERR_INVALID_ARG = 1,     /**< a required argument was NULL or otherwise invalid */
    SLK_ERR_BAD_DICTIONARY = 2,  /**< a dictionary buffer could not be parsed */
    SLK_ERR_PATTERN = 3,         /**< the pattern failed to parse/validate */
    SLK_ERR_GENERATION = 4,      /**< generation failed (e.g. sequence out of capacity) */
    SLK_ERR_BUFFER_TOO_SMALL = 5,/**< the provided output buffer was too small */
    SLK_ERR_INTERNAL = 6         /**< an unexpected internal error */
} slk_status;

/**
 * Create a generator from one or more compiled binary dictionaries (.bin).
 * @param datas  array of @p n buffer pointers (each a compiled binary dictionary).
 * @param lens   array of @p n buffer lengths.
 * @param n      number of dictionaries.
 * @param err_out optional; on failure receives a heap-allocated message (free with slk_string_free).
 * @return a new handle, or NULL on failure. Buffers are copied; caller may free them after return.
 */
slk_generator* slk_generator_create(const uint8_t* const* datas, const size_t* lens, size_t n, char** err_out);

/** Convenience: create from a single binary dictionary buffer. */
slk_generator* slk_generator_create_one(const uint8_t* data, size_t len, char** err_out);

/** Destroy a generator handle. NULL is allowed (no-op). */
void slk_generator_destroy(slk_generator* gen);

/**
 * Produce a fresh random seed.
 * @return heap-allocated UTF-8 string (free with slk_string_free), or NULL on error.
 */
char* slk_random_seed(slk_generator* gen, char** err_out);

/**
 * Compute the capacity of a pattern.
 * @param capacity_decimal_out on success receives a heap-allocated decimal string of the
 *        (arbitrary-precision) capacity. Free with slk_string_free. May be NULL to skip.
 * @param max_len_out on success receives the engine's max_pattern_length for the pattern
 *        (the upper bound on slug length as the engine counts it). May be NULL.
 */
slk_status slk_capacity(slk_generator* gen, const char* pattern,
                        char** capacity_decimal_out, int32_t* max_len_out, char** err_out);

/**
 * Generate a single slug into a caller-provided buffer.
 * @param out_buf  destination for a NUL-terminated UTF-8 slug.
 * @param out_cap  capacity of @p out_buf in bytes (including space for the NUL).
 * @param out_len  optional; receives the slug length in bytes (excluding the NUL). If the
 *                 buffer is too small, receives the REQUIRED length (excluding the NUL) and
 *                 the function returns SLK_ERR_BUFFER_TOO_SMALL.
 */
slk_status slk_generate(slk_generator* gen, const char* pattern, const char* seed, uint64_t sequence,
                        char* out_buf, size_t out_cap, size_t* out_len, char** err_out);

/**
 * Generate a single slug, allocating the result. Simplest variant for FFI.
 * @return heap-allocated UTF-8 slug (free with slk_string_free), or NULL on error.
 */
char* slk_generate_alloc(slk_generator* gen, const char* pattern, const char* seed,
                         uint64_t sequence, char** err_out);

/** Callback invoked once per generated slug. @p slug is valid only during the call. */
typedef void (*slk_emit_cb)(void* ctx, const char* slug, size_t slug_len);

/**
 * Generate @p count slugs starting at @p sequence, delivering each to @p cb.
 * This avoids per-slug allocation and is the preferred bulk path.
 */
slk_status slk_generate_batch(slk_generator* gen, const char* pattern, const char* seed, uint64_t sequence,
                              size_t count, slk_emit_cb cb, void* ctx, char** err_out);

/** Free a string returned by this API. NULL is allowed (no-op). */
void slk_string_free(char* s);

/** Library version string (static storage; do not free). */
const char* slk_version(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* SLUGKIT_C_H */
