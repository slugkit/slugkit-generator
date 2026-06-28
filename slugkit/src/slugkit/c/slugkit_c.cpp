#include <slugkit/c/slugkit_c.h>

#include <slugkit/generator/binary_dictionary.hpp>
#include <slugkit/generator/dictionary_types.hpp>
#include <slugkit/generator/generator.hpp>

#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <span>
#include <string>
#include <vector>

// Opaque handle: just owns a Generator. The Generator's binary::DictionarySet retains the
// dictionary byte buffers (passed as keepalives at construction), so nothing else is needed here.
struct slk_generator {
    slugkit::generator::Generator gen;
};

namespace {

// Duplicate a std::string into a malloc'd, NUL-terminated C string (freeable with slk_string_free).
char* DupString(const std::string& s) {
    char* out = static_cast<char*>(std::malloc(s.size() + 1));
    if (out == nullptr) {
        return nullptr;
    }
    std::memcpy(out, s.data(), s.size());
    out[s.size()] = '\0';
    return out;
}

void SetErr(char** err_out, const std::string& msg) {
    if (err_out != nullptr) {
        *err_out = DupString(msg);
    }
}

// Run a body that may throw, translating exceptions to a status code + message.
template <typename Fn>
slk_status Guard(char** err_out, Fn&& fn) {
    try {
        return fn();
    } catch (const std::exception& e) {
        SetErr(err_out, e.what());
        return SLK_ERR_INTERNAL;
    } catch (...) {
        SetErr(err_out, "unknown error");
        return SLK_ERR_INTERNAL;
    }
}

}  // namespace

extern "C" {

slk_generator* slk_generator_create(const uint8_t* const* datas, const size_t* lens, size_t n, char** err_out) {
    if ((datas == nullptr || lens == nullptr) && n != 0) {
        SetErr(err_out, "slk_generator_create: null datas/lens");
        return nullptr;
    }
    try {
        slugkit::generator::binary::DictionarySet dictset;
        for (size_t i = 0; i < n; ++i) {
            if (datas[i] == nullptr) {
                SetErr(err_out, "slk_generator_create: null dictionary buffer");
                return nullptr;
            }
            // Copy the caller's bytes into a buffer we keep alive for the dictionary's lifetime.
            auto buf = std::make_shared<std::vector<std::byte>>(lens[i]);
            std::memcpy(buf->data(), datas[i], lens[i]);
            std::span<const std::byte> data{buf->data(), buf->size()};
            dictset.Add(data, buf);
        }
        return new slk_generator{slugkit::generator::Generator(std::move(dictset))};
    } catch (const std::exception& e) {
        SetErr(err_out, e.what());
        return nullptr;
    } catch (...) {
        SetErr(err_out, "unknown error");
        return nullptr;
    }
}

slk_generator* slk_generator_create_one(const uint8_t* data, size_t len, char** err_out) {
    const uint8_t* datas[1] = {data};
    const size_t lens[1] = {len};
    return slk_generator_create(datas, lens, 1, err_out);
}

void slk_generator_destroy(slk_generator* gen) { delete gen; }

char* slk_random_seed(slk_generator* gen, char** err_out) {
    if (gen == nullptr) {
        SetErr(err_out, "slk_random_seed: null generator");
        return nullptr;
    }
    try {
        return DupString(gen->gen.RandomSeed());
    } catch (const std::exception& e) {
        SetErr(err_out, e.what());
        return nullptr;
    } catch (...) {
        SetErr(err_out, "unknown error");
        return nullptr;
    }
}

slk_status slk_capacity(slk_generator* gen, const char* pattern, char** capacity_decimal_out, int32_t* max_len_out,
                        char** err_out) {
    if (gen == nullptr || pattern == nullptr) {
        SetErr(err_out, "slk_capacity: null argument");
        return SLK_ERR_INVALID_ARG;
    }
    return Guard(err_out, [&]() -> slk_status {
        auto settings = gen->gen.GetCapacity(std::string_view{pattern});
        if (capacity_decimal_out != nullptr) {
            *capacity_decimal_out = DupString(settings.capacity.str());
        }
        if (max_len_out != nullptr) {
            *max_len_out = settings.max_pattern_length;
        }
        return SLK_OK;
    });
}

slk_status slk_generate(slk_generator* gen, const char* pattern, const char* seed, uint64_t sequence, char* out_buf,
                        size_t out_cap, size_t* out_len, char** err_out) {
    if (gen == nullptr || pattern == nullptr || seed == nullptr || out_buf == nullptr) {
        SetErr(err_out, "slk_generate: null argument");
        return SLK_ERR_INVALID_ARG;
    }
    return Guard(err_out, [&]() -> slk_status {
        std::string slug =
            gen->gen.Generate(std::string_view{pattern}, std::string_view{seed}, static_cast<std::size_t>(sequence));
        if (out_len != nullptr) {
            *out_len = slug.size();
        }
        if (slug.size() + 1 > out_cap) {
            return SLK_ERR_BUFFER_TOO_SMALL;
        }
        std::memcpy(out_buf, slug.data(), slug.size());
        out_buf[slug.size()] = '\0';
        return SLK_OK;
    });
}

char* slk_generate_alloc(slk_generator* gen, const char* pattern, const char* seed, uint64_t sequence, char** err_out) {
    if (gen == nullptr || pattern == nullptr || seed == nullptr) {
        SetErr(err_out, "slk_generate_alloc: null argument");
        return nullptr;
    }
    try {
        return DupString(
            gen->gen.Generate(std::string_view{pattern}, std::string_view{seed}, static_cast<std::size_t>(sequence)));
    } catch (const std::exception& e) {
        SetErr(err_out, e.what());
        return nullptr;
    } catch (...) {
        SetErr(err_out, "unknown error");
        return nullptr;
    }
}

slk_status slk_generate_batch(slk_generator* gen, const char* pattern, const char* seed, uint64_t sequence,
                              size_t count, slk_emit_cb cb, void* ctx, char** err_out) {
    if (gen == nullptr || pattern == nullptr || seed == nullptr || cb == nullptr) {
        SetErr(err_out, "slk_generate_batch: null argument");
        return SLK_ERR_INVALID_ARG;
    }
    return Guard(err_out, [&]() -> slk_status {
        gen->gen.Generate(std::string_view{pattern}, std::string_view{seed}, static_cast<std::size_t>(sequence),
                          static_cast<std::size_t>(count),
                          [&](std::string slug) { cb(ctx, slug.c_str(), slug.size()); });
        return SLK_OK;
    });
}

void slk_string_free(char* s) { std::free(s); }

const char* slk_version(void) { return "0.1.0"; }

}  // extern "C"
