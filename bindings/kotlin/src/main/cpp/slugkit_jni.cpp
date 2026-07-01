// JNI bridge from the JVM (Kotlin/Java) to the slugkit_c C ABI.
//
// The JVM side is `object com.slugkit.SlugkitJni` with @JvmStatic external funs; its native symbols
// are Java_com_slugkit_SlugkitJni_<name>. Handles are passed as jlong. Native errors are surface as
// Java RuntimeExceptions.
#include <slugkit/c/slugkit_c.h>

#include <jni.h>

#include <string>
#include <vector>

namespace {

// Throw java.lang.RuntimeException with `message` (or a native error string) and free it.
void ThrowRuntime(JNIEnv* env, const char* message) {
    jclass cls = env->FindClass("java/lang/RuntimeException");
    if (cls != nullptr) {
        env->ThrowNew(cls, message != nullptr ? message : "slugkit: unknown error");
    }
}

// Consume a slugkit_c char* error out-param into a Java exception (and free it).
void ThrowFromErr(JNIEnv* env, char* err) {
    ThrowRuntime(env, err != nullptr ? err : "slugkit: unknown error");
    slk_string_free(err);
}

slk_generator* AsGenerator(jlong handle) { return reinterpret_cast<slk_generator*>(handle); }

// Copy a modified-UTF-8 jstring into a std::string (std::string is fine for ASCII/UTF-8 patterns).
std::string ToUtf8(JNIEnv* env, jstring s) {
    if (s == nullptr) {
        return {};
    }
    const char* chars = env->GetStringUTFChars(s, nullptr);
    std::string out{chars != nullptr ? chars : ""};
    if (chars != nullptr) {
        env->ReleaseStringUTFChars(s, chars);
    }
    return out;
}

}  // namespace

extern "C" {

JNIEXPORT jlong JNICALL Java_com_slugkit_SlugkitJni_create(JNIEnv* env, jobject, jobjectArray dicts) {
    const jsize n = dicts != nullptr ? env->GetArrayLength(dicts) : 0;
    std::vector<std::vector<jbyte>> owned;
    std::vector<const uint8_t*> datas;
    std::vector<size_t> lens;
    owned.reserve(n);
    datas.reserve(n);
    lens.reserve(n);
    for (jsize i = 0; i < n; ++i) {
        auto arr = reinterpret_cast<jbyteArray>(env->GetObjectArrayElement(dicts, i));
        const jsize len = env->GetArrayLength(arr);
        std::vector<jbyte> buf(static_cast<size_t>(len));
        env->GetByteArrayRegion(arr, 0, len, buf.data());
        owned.push_back(std::move(buf));
        datas.push_back(reinterpret_cast<const uint8_t*>(owned.back().data()));
        lens.push_back(static_cast<size_t>(len));
        env->DeleteLocalRef(arr);
    }
    char* err = nullptr;
    slk_generator* gen = slk_generator_create(datas.data(), lens.data(), static_cast<size_t>(n), &err);
    if (gen == nullptr) {
        ThrowFromErr(env, err);
        return 0;
    }
    return reinterpret_cast<jlong>(gen);
}

JNIEXPORT void JNICALL Java_com_slugkit_SlugkitJni_destroy(JNIEnv*, jobject, jlong handle) {
    slk_generator_destroy(AsGenerator(handle));
}

JNIEXPORT jstring JNICALL Java_com_slugkit_SlugkitJni_randomSeed(JNIEnv* env, jobject, jlong handle) {
    char* err = nullptr;
    char* seed = slk_random_seed(AsGenerator(handle), &err);
    if (seed == nullptr) {
        ThrowFromErr(env, err);
        return nullptr;
    }
    jstring out = env->NewStringUTF(seed);
    slk_string_free(seed);
    return out;
}

// Returns String[2] = { capacity-decimal, maxLength-as-decimal }.
JNIEXPORT jobjectArray JNICALL Java_com_slugkit_SlugkitJni_capacity(JNIEnv* env, jobject, jlong handle,
                                                                    jstring pattern) {
    const std::string pat = ToUtf8(env, pattern);
    char* cap = nullptr;
    int32_t max_len = 0;
    char* err = nullptr;
    if (slk_capacity(AsGenerator(handle), pat.c_str(), &cap, &max_len, &err) != SLK_OK) {
        ThrowFromErr(env, err);
        return nullptr;
    }
    jclass string_cls = env->FindClass("java/lang/String");
    jobjectArray out = env->NewObjectArray(2, string_cls, nullptr);
    env->SetObjectArrayElement(out, 0, env->NewStringUTF(cap != nullptr ? cap : "0"));
    env->SetObjectArrayElement(out, 1, env->NewStringUTF(std::to_string(max_len).c_str()));
    slk_string_free(cap);
    return out;
}

JNIEXPORT jstring JNICALL Java_com_slugkit_SlugkitJni_generate(JNIEnv* env, jobject, jlong handle, jstring pattern,
                                                               jstring seed, jlong sequence) {
    const std::string pat = ToUtf8(env, pattern);
    const std::string sd = ToUtf8(env, seed);
    char* err = nullptr;
    char* slug = slk_generate_alloc(AsGenerator(handle), pat.c_str(), sd.c_str(),
                                    static_cast<uint64_t>(sequence), &err);
    if (slug == nullptr) {
        ThrowFromErr(env, err);
        return nullptr;
    }
    jstring out = env->NewStringUTF(slug);
    slk_string_free(slug);
    return out;
}

// Batch: collect into a String[] on the native side, then return it (simple, JVM-idiomatic).
namespace {
struct BatchCtx {
    JNIEnv* env;
    jobjectArray array;
    jsize index;
};
void EmitToArray(void* ctx, const char* slug, size_t /*len*/) {
    auto* c = static_cast<BatchCtx*>(ctx);
    c->env->SetObjectArrayElement(c->array, c->index++, c->env->NewStringUTF(slug));
}
}  // namespace

JNIEXPORT jobjectArray JNICALL Java_com_slugkit_SlugkitJni_generateBatch(JNIEnv* env, jobject, jlong handle,
                                                                         jstring pattern, jstring seed,
                                                                         jlong sequence, jint count) {
    const std::string pat = ToUtf8(env, pattern);
    const std::string sd = ToUtf8(env, seed);
    jclass string_cls = env->FindClass("java/lang/String");
    jobjectArray out = env->NewObjectArray(count, string_cls, nullptr);
    BatchCtx ctx{env, out, 0};
    char* err = nullptr;
    if (slk_generate_batch(AsGenerator(handle), pat.c_str(), sd.c_str(), static_cast<uint64_t>(sequence),
                           static_cast<size_t>(count), EmitToArray, &ctx, &err) != SLK_OK) {
        ThrowFromErr(env, err);
        return nullptr;
    }
    return out;
}

JNIEXPORT jstring JNICALL Java_com_slugkit_SlugkitJni_version(JNIEnv* env, jobject) {
    return env->NewStringUTF(slk_version());
}

}  // extern "C"
