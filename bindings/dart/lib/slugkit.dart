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

/// A deterministic human-readable ID generator backed by a compiled binary dictionary.
///
/// Construct with [Generator.fromBytes], reuse for many generations, and call [dispose]
/// to release the native handle. Generation is deterministic: the same
/// `(pattern, seed, sequence)` always yields the same slug.
class Generator {
  Generator._(this._bindings, this._handle);

  final SlugkitBindings _bindings;
  final Pointer<SlkGenerator> _handle;
  bool _disposed = false;

  /// Open the native library. On most platforms the default name resolves the bundled
  /// library; pass [libraryPath] to load an explicit file (e.g. in tests).
  static DynamicLibrary _openLibrary(String? libraryPath) {
    if (libraryPath != null) return DynamicLibrary.open(libraryPath);
    if (Platform.isMacOS || Platform.isIOS) return DynamicLibrary.open('libslugkit_c.dylib');
    if (Platform.isWindows) return DynamicLibrary.open('slugkit_c.dll');
    return DynamicLibrary.open('libslugkit_c.so');
  }

  /// Create a generator from a single compiled binary dictionary (`.bin`).
  factory Generator.fromBytes(Uint8List dictionary, {String? libraryPath, DynamicLibrary? library}) {
    final bindings = SlugkitBindings(library ?? _openLibrary(libraryPath));
    final data = malloc<Uint8>(dictionary.length);
    data.asTypedList(dictionary.length).setAll(0, dictionary);
    final errPtr = malloc<Pointer<Utf8>>()..value = nullptr;
    try {
      final handle = bindings.createOne(data, dictionary.length, errPtr);
      if (handle == nullptr) {
        throw SlugkitException(_takeError(bindings, errPtr, 'failed to create generator'));
      }
      return Generator._(bindings, handle);
    } finally {
      malloc.free(data);
      malloc.free(errPtr);
    }
  }

  /// Create a generator from several compiled binary dictionaries (e.g. an adjective and a noun
  /// dictionary for `{adjective}-{noun}`). The bytes are copied; the lists need not outlive this call.
  factory Generator.fromMultiple(List<Uint8List> dictionaries,
      {String? libraryPath, DynamicLibrary? library}) {
    final bindings = SlugkitBindings(library ?? _openLibrary(libraryPath));
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
      final handle = bindings.create(datas, lens, n, errPtr);
      if (handle == nullptr) {
        throw SlugkitException(_takeError(bindings, errPtr, 'failed to create generator'));
      }
      return Generator._(bindings, handle);
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
      if (result == nullptr) {
        throw SlugkitException(_takeError(_bindings, errPtr, 'failed to produce seed'));
      }
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
      if (status != slkOk) {
        throw SlugkitException(_takeError(_bindings, errPtr, 'failed to compute capacity'));
      }
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
      if (result == nullptr) {
        throw SlugkitException(_takeError(_bindings, errPtr, 'generation failed'));
      }
      final slug = result.toDartString();
      _bindings.stringFree(result);
      return slug;
    } finally {
      malloc.free(patternPtr);
      malloc.free(seedPtr);
      malloc.free(errPtr);
    }
  }

  /// Generate [count] slugs starting at [sequence]. Batch generation is equivalent to
  /// calling [generate] for `sequence, sequence + 1, ...` (deterministic per sequence).
  List<String> generateBatch(String pattern, String seed, int sequence, int count) {
    return List<String>.generate(count, (i) => generate(pattern, seed, sequence + i));
  }

  /// Release the native handle. The generator must not be used afterwards.
  void dispose() {
    if (_disposed) return;
    _disposed = true;
    _bindings.destroy(_handle);
  }

  void _checkAlive() {
    if (_disposed) throw StateError('Generator has been disposed');
  }

  static String _takeError(SlugkitBindings b, Pointer<Pointer<Utf8>> errPtr, String fallback) {
    final err = errPtr.value;
    if (err == nullptr) return fallback;
    final message = err.toDartString();
    b.stringFree(err);
    return message;
  }
}
