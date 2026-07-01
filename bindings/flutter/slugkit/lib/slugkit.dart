import 'dart:ffi';
import 'dart:io';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

import 'src/ffi_bindings.dart';

/// An error thrown by the SlugKit engine, carrying the message from the native layer.
class SlugkitException implements Exception {
  SlugkitException(this.message);
  final String message;
  @override
  String toString() => 'SlugkitException: $message';
}

/// The capacity of a pattern: total number of distinct slugs plus the maximum length.
class Capacity {
  Capacity(this.value, this.maxLength);

  /// Total capacity; may exceed 64 bits, so it is exposed as a decimal string.
  final String value;

  /// The engine's upper bound on slug length for the pattern.
  final int maxLength;
}

/// Open the native SlugKit library bundled by the Flutter plugin.
///
/// On iOS/macOS the engine is a dynamic framework linked into the app, so its symbols live in the
/// process. On Android it is `libslugkit_c.so` bundled in the APK.
DynamicLibrary _openLibrary() {
  if (Platform.isIOS || Platform.isMacOS) return DynamicLibrary.process();
  if (Platform.isAndroid || Platform.isLinux) return DynamicLibrary.open('libslugkit_c.so');
  if (Platform.isWindows) return DynamicLibrary.open('slugkit_c.dll');
  return DynamicLibrary.process();
}

final SlugkitBindings _bindings = SlugkitBindings(_openLibrary());

/// A deterministic human-readable ID generator backed by compiled binary dictionaries.
///
/// Construct with [Generator.fromBytes] or [Generator.fromMultiple], reuse for many generations,
/// and call [dispose] to release the native handle. Generation is deterministic: the same
/// `(pattern, seed, sequence)` always yields the same slug.
class Generator {
  Generator._(this._handle);

  final Pointer<SlkGenerator> _handle;
  bool _disposed = false;

  /// Create a generator from a single compiled binary dictionary (`.bin`).
  factory Generator.fromBytes(Uint8List dictionary) => Generator.fromMultiple([dictionary]);

  /// Create a generator from several compiled binary dictionaries (e.g. an adjective and a noun
  /// dictionary for `{adjective}-{noun}`). The bytes are copied.
  factory Generator.fromMultiple(List<Uint8List> dictionaries) {
    final n = dictionaries.length;
    final datas = malloc<Pointer<Uint8>>(n);
    final lens = malloc<IntPtr>(n);
    final errPtr = malloc<Pointer<Utf8>>()..value = nullptr;
    final buffers = <Pointer<Uint8>>[];
    try {
      for (var i = 0; i < n; i++) {
        final dict = dictionaries[i];
        final buf = malloc<Uint8>(dict.length);
        buf.asTypedList(dict.length).setAll(0, dict);
        buffers.add(buf);
        datas[i] = buf;
        lens[i] = dict.length;
      }
      final handle = _bindings.create(datas, lens, n, errPtr);
      if (handle == nullptr) {
        throw SlugkitException(_takeError(errPtr, 'failed to create generator'));
      }
      return Generator._(handle);
    } finally {
      for (final b in buffers) {
        malloc.free(b);
      }
      malloc.free(datas);
      malloc.free(lens);
      malloc.free(errPtr);
    }
  }

  /// The native library version.
  String get version => _bindings.version().toDartString();

  /// Produce a fresh random seed.
  String randomSeed() {
    _checkAlive();
    final errPtr = malloc<Pointer<Utf8>>()..value = nullptr;
    try {
      final result = _bindings.randomSeed(_handle, errPtr);
      if (result == nullptr) throw SlugkitException(_takeError(errPtr, 'failed to produce seed'));
      final seed = result.toDartString();
      _bindings.stringFree(result);
      return seed;
    } finally {
      malloc.free(errPtr);
    }
  }

  /// Compute a pattern's capacity.
  Capacity capacity(String pattern) {
    _checkAlive();
    final patternPtr = pattern.toNativeUtf8();
    final capPtr = malloc<Pointer<Utf8>>()..value = nullptr;
    final maxLenPtr = malloc<Int32>();
    final errPtr = malloc<Pointer<Utf8>>()..value = nullptr;
    try {
      final status = _bindings.capacity(_handle, patternPtr, capPtr, maxLenPtr, errPtr);
      if (status != slkOk) throw SlugkitException(_takeError(errPtr, 'failed to compute capacity'));
      final value = capPtr.value.toDartString();
      _bindings.stringFree(capPtr.value);
      return Capacity(value, maxLenPtr.value);
    } finally {
      malloc.free(patternPtr);
      malloc.free(capPtr);
      malloc.free(maxLenPtr);
      malloc.free(errPtr);
    }
  }

  /// Generate a single slug for `(pattern, seed, sequence)`.
  String generate(String pattern, String seed, int sequence) {
    _checkAlive();
    final patternPtr = pattern.toNativeUtf8();
    final seedPtr = seed.toNativeUtf8();
    final errPtr = malloc<Pointer<Utf8>>()..value = nullptr;
    try {
      final result = _bindings.generateAlloc(_handle, patternPtr, seedPtr, sequence, errPtr);
      if (result == nullptr) throw SlugkitException(_takeError(errPtr, 'generation failed'));
      final slug = result.toDartString();
      _bindings.stringFree(result);
      return slug;
    } finally {
      malloc.free(patternPtr);
      malloc.free(seedPtr);
      malloc.free(errPtr);
    }
  }

  /// Generate [count] slugs starting at [sequence].
  List<String> generateBatch(String pattern, String seed, int sequence, int count) =>
      List<String>.generate(count, (i) => generate(pattern, seed, sequence + i));

  /// Release the native handle. The generator must not be used afterwards.
  void dispose() {
    if (_disposed) return;
    _disposed = true;
    _bindings.destroy(_handle);
  }

  void _checkAlive() {
    if (_disposed) throw StateError('Generator has been disposed');
  }

  static String _takeError(Pointer<Pointer<Utf8>> errPtr, String fallback) {
    final err = errPtr.value;
    if (err == nullptr) return fallback;
    final message = err.toDartString();
    _bindings.stringFree(err);
    return message;
  }
}
