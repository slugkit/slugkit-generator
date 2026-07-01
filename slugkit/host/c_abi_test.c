/* C (not C++) smoke test for the slugkit_c ABI: proves the header is valid C and that the
 * full surface works end-to-end against a real compiled binary dictionary (emoji.bin). */
#include <slugkit/c/slugkit_c.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SLK_EMOJI_BIN_PATH
#error "SLK_EMOJI_BIN_PATH must be defined by the build"
#endif

static int failures = 0;
#define CHECK(cond, msg)                                       \
    do {                                                       \
        if (!(cond)) {                                         \
            fprintf(stderr, "FAIL: %s\n", (msg));              \
            ++failures;                                        \
        } else {                                               \
            fprintf(stderr, "ok:   %s\n", (msg));              \
        }                                                      \
    } while (0)

static unsigned char* read_file(const char* path, size_t* out_len) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* buf = (unsigned char*)malloc((size_t)sz);
    if (buf && sz > 0) {
        if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
            free(buf);
            fclose(f);
            return NULL;
        }
    }
    fclose(f);
    *out_len = (size_t)sz;
    return buf;
}

static size_t batch_count = 0;
static void on_slug(void* ctx, const char* slug, size_t len) {
    (void)ctx;
    (void)len;
    if (slug && slug[0]) ++batch_count;
}

int main(void) {
    fprintf(stderr, "slugkit_c version: %s\n", slk_version());

    size_t len = 0;
    unsigned char* data = read_file(SLK_EMOJI_BIN_PATH, &len);
    CHECK(data != NULL && len > 0, "read emoji.bin");
    if (!data) return 1;

    char* err = NULL;
    slk_generator* gen = slk_generator_create_one(data, len, &err);
    CHECK(gen != NULL, "create generator from buffer");
    free(data); /* buffer is copied internally; safe to free now */
    if (!gen) {
        fprintf(stderr, "  error: %s\n", err ? err : "(none)");
        slk_string_free(err);
        return 1;
    }

    /* random seed */
    char* seed = slk_random_seed(gen, &err);
    CHECK(seed != NULL && strlen(seed) > 0, "random seed");

    const char* pattern = "{emoji}";
    const char* fixed_seed = "foobar";

    /* capacity */
    char* cap = NULL;
    int32_t max_len = -1;
    slk_status st = slk_capacity(gen, pattern, &cap, &max_len, &err);
    CHECK(st == SLK_OK && cap != NULL && strlen(cap) > 0 && max_len > 0, "capacity({emoji})");
    fprintf(stderr, "  capacity=%s max_len=%d\n", cap ? cap : "?", max_len);

    /* generate (alloc) -- determinism: same seed+seq => same slug */
    char* a = slk_generate_alloc(gen, pattern, fixed_seed, 0, &err);
    char* b = slk_generate_alloc(gen, pattern, fixed_seed, 0, &err);
    CHECK(a != NULL && b != NULL && strcmp(a, b) == 0, "generate_alloc deterministic");
    fprintf(stderr, "  slug[0]=%s\n", a ? a : "?");

    /* generate (fixed buffer) -- must match the alloc variant */
    char buf[256];
    size_t n = 0;
    st = slk_generate(gen, pattern, fixed_seed, 0, buf, sizeof(buf), &n, &err);
    CHECK(st == SLK_OK && a != NULL && strcmp(buf, a) == 0 && n == strlen(a), "generate into buffer matches alloc");

    /* too-small buffer -> SLK_ERR_BUFFER_TOO_SMALL, reports required length */
    char tiny[1];
    size_t need = 0;
    st = slk_generate(gen, pattern, fixed_seed, 0, tiny, sizeof(tiny), &need, &err);
    CHECK(st == SLK_ERR_BUFFER_TOO_SMALL && need == strlen(a), "buffer-too-small reports required length");

    /* batch of 5 */
    batch_count = 0;
    st = slk_generate_batch(gen, pattern, fixed_seed, 0, 5, on_slug, NULL, &err);
    CHECK(st == SLK_OK && batch_count == 5, "batch generates 5 slugs");

    /* bad pattern -> error, not crash */
    char* bad = slk_generate_alloc(gen, "{", fixed_seed, 0, &err);
    CHECK(bad == NULL, "malformed pattern returns NULL");

    /* --- multi-dictionary word patterns: load adverb + emoji dictionaries (n=2) --- */
    /* Golden values below (seed "foobar") are byte-identical across every language binding. */
    {
        size_t la = 0, le = 0;
        unsigned char* adv = read_file(SLK_ADVERB_BIN_PATH, &la);
        unsigned char* emo = read_file(SLK_EMOJI_BIN_PATH, &le);
        CHECK(adv != NULL && emo != NULL, "read adverb + emoji dictionaries");
        const uint8_t* datas[2] = {adv, emo};
        const size_t lens[2] = {la, le};
        char* merr = NULL;
        slk_generator* mg = slk_generator_create(datas, lens, 2, &merr);
        CHECK(mg != NULL, "create generator from two dictionaries");
        free(adv);
        free(emo);

        if (mg != NULL) {
            char* mc = NULL;
            int32_t mml = 0;
            slk_capacity(mg, "{adverb}-{emoji}", &mc, &mml, &err);
            CHECK(mc && strcmp(mc, "4176326") == 0 && mml == 22, "capacity({adverb}-{emoji}) == 4176326");
            slk_string_free(mc);

            /* {adverb}-{emoji}: adverb prefix is ASCII and deterministic (emoji suffix varies by build's font, not bytes) */
            char* w0 = slk_generate_alloc(mg, "{adverb}-{emoji}", "foobar", 0, &err);
            char* w1 = slk_generate_alloc(mg, "{adverb}-{emoji}", "foobar", 1, &err);
            char* w2 = slk_generate_alloc(mg, "{adverb}-{emoji}", "foobar", 2, &err);
            CHECK(w0 && strncmp(w0, "lustfully-", 10) == 0, "{adverb}-{emoji} seq0 starts 'lustfully-'");
            CHECK(w1 && strncmp(w1, "abroad-", 7) == 0, "{adverb}-{emoji} seq1 starts 'abroad-'");
            CHECK(w2 && strncmp(w2, "maladroitly-", 12) == 0, "{adverb}-{emoji} seq2 starts 'maladroitly-'");
            slk_string_free(w0);
            slk_string_free(w1);
            slk_string_free(w2);

            /* length constraint + number generator, fully ASCII golden */
            char* n0 = slk_generate_alloc(mg, "{adverb:<=5}-{number:3d}", "foobar", 0, &err);
            char* n1 = slk_generate_alloc(mg, "{adverb:<=5}-{number:3d}", "foobar", 1, &err);
            char* n2 = slk_generate_alloc(mg, "{adverb:<=5}-{number:3d}", "foobar", 2, &err);
            CHECK(n0 && strcmp(n0, "ago-887") == 0, "{adverb:<=5}-{number:3d} seq0 == ago-887");
            CHECK(n1 && strcmp(n1, "aloud-774") == 0, "{adverb:<=5}-{number:3d} seq1 == aloud-774");
            CHECK(n2 && strcmp(n2, "apart-661") == 0, "{adverb:<=5}-{number:3d} seq2 == apart-661");
            slk_string_free(n0);
            slk_string_free(n1);
            slk_string_free(n2);

            slk_generator_destroy(mg);
        }
    }

    slk_string_free(a);
    slk_string_free(b);
    slk_string_free(cap);
    slk_string_free(seed);
    slk_string_free(err);
    slk_generator_destroy(gen);

    if (failures == 0) {
        fprintf(stderr, "\nALL C ABI CHECKS PASSED\n");
        return 0;
    }
    fprintf(stderr, "\n%d C ABI CHECK(S) FAILED\n", failures);
    return 1;
}
