import 'dart:ffi';
import 'package:ffi/ffi.dart';

/// Opaque native generator handle.
final class SlkGenerator extends Opaque {}

// --- Native signatures ---------------------------------------------------------------------------

typedef _CreateOneNative = Pointer<SlkGenerator> Function(
    Pointer<Uint8>, Size, Pointer<Pointer<Utf8>>);
typedef _CreateOneDart = Pointer<SlkGenerator> Function(
    Pointer<Uint8>, int, Pointer<Pointer<Utf8>>);

// const uint8_t* const* datas, const size_t* lens, size_t n, char** err_out.
// The lens array uses IntPtr (pointer-width, matching size_t on all targets).
typedef _CreateNative = Pointer<SlkGenerator> Function(
    Pointer<Pointer<Uint8>>, Pointer<IntPtr>, Size, Pointer<Pointer<Utf8>>);
typedef _CreateDart = Pointer<SlkGenerator> Function(
    Pointer<Pointer<Uint8>>, Pointer<IntPtr>, int, Pointer<Pointer<Utf8>>);

typedef _DestroyNative = Void Function(Pointer<SlkGenerator>);
typedef _DestroyDart = void Function(Pointer<SlkGenerator>);

typedef _RandomSeedNative = Pointer<Utf8> Function(
    Pointer<SlkGenerator>, Pointer<Pointer<Utf8>>);
typedef _RandomSeedDart = Pointer<Utf8> Function(
    Pointer<SlkGenerator>, Pointer<Pointer<Utf8>>);

typedef _CapacityNative = Int32 Function(Pointer<SlkGenerator>, Pointer<Utf8>,
    Pointer<Pointer<Utf8>>, Pointer<Int32>, Pointer<Pointer<Utf8>>);
typedef _CapacityDart = int Function(Pointer<SlkGenerator>, Pointer<Utf8>,
    Pointer<Pointer<Utf8>>, Pointer<Int32>, Pointer<Pointer<Utf8>>);

typedef _GenerateAllocNative = Pointer<Utf8> Function(Pointer<SlkGenerator>,
    Pointer<Utf8>, Pointer<Utf8>, Uint64, Pointer<Pointer<Utf8>>);
typedef _GenerateAllocDart = Pointer<Utf8> Function(Pointer<SlkGenerator>,
    Pointer<Utf8>, Pointer<Utf8>, int, Pointer<Pointer<Utf8>>);

typedef _StringFreeNative = Void Function(Pointer<Utf8>);
typedef _StringFreeDart = void Function(Pointer<Utf8>);

typedef _VersionNative = Pointer<Utf8> Function();
typedef _VersionDart = Pointer<Utf8> Function();

/// Thin lookup table over `libslugkit_c` exposing the C ABI to Dart.
class SlugkitBindings {
  SlugkitBindings(DynamicLibrary lib)
      : createOne =
            lib.lookupFunction<_CreateOneNative, _CreateOneDart>('slk_generator_create_one'),
        create = lib.lookupFunction<_CreateNative, _CreateDart>('slk_generator_create'),
        destroy = lib.lookupFunction<_DestroyNative, _DestroyDart>('slk_generator_destroy'),
        randomSeed =
            lib.lookupFunction<_RandomSeedNative, _RandomSeedDart>('slk_random_seed'),
        capacity = lib.lookupFunction<_CapacityNative, _CapacityDart>('slk_capacity'),
        generateAlloc = lib
            .lookupFunction<_GenerateAllocNative, _GenerateAllocDart>('slk_generate_alloc'),
        stringFree =
            lib.lookupFunction<_StringFreeNative, _StringFreeDart>('slk_string_free'),
        version = lib.lookupFunction<_VersionNative, _VersionDart>('slk_version');

  final _CreateOneDart createOne;
  final _CreateDart create;
  final _DestroyDart destroy;
  final _RandomSeedDart randomSeed;
  final _CapacityDart capacity;
  final _GenerateAllocDart generateAlloc;
  final _StringFreeDart stringFree;
  final _VersionDart version;
}

/// Status codes returned by the C ABI (mirrors `slk_status`).
const int slkOk = 0;
