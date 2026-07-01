import 'dart:io';

import 'package:slugkit/slugkit.dart';
import 'package:test/test.dart';

String _firstExisting(List<String> paths) =>
    paths.firstWhere((p) => File(p).existsSync(), orElse: () => paths.first);

void main() {
  // The native library is built by scripts/build-native.sh (or pass SLUGKIT_LIB).
  final libPath = Platform.environment['SLUGKIT_LIB'] ??
      _firstExisting([
        '.build/host/libslugkit_c.dylib',
        '.build/host/libslugkit_c.so',
      ]);
  final emojiPath = Platform.environment['SLUGKIT_EMOJI_BIN'] ??
      '../../slugkit/tests/data/emoji.bin';
  final dict = File(emojiPath).readAsBytesSync();

  Generator open() => Generator.fromBytes(dict, libraryPath: libPath);

  test('version is non-empty', () {
    final g = open();
    expect(g.version, isNotEmpty);
    g.dispose();
  });

  test('random seed is non-empty', () {
    final g = open();
    expect(g.randomSeed(), isNotEmpty);
    g.dispose();
  });

  test('capacity of {emoji}', () {
    final g = open();
    final cap = g.capacity('{emoji}');
    expect(cap.value, '1154');
    expect(cap.maxLength, 1);
    g.dispose();
  });

  test('generation is deterministic', () {
    final g = open();
    final a = g.generate('{emoji}', 'foobar', 0);
    final b = g.generate('{emoji}', 'foobar', 0);
    expect(a, isNotEmpty);
    expect(a, b);
    g.dispose();
  });

  test('batch generates count slugs, aligned with single-shot', () {
    final g = open();
    final batch = g.generateBatch('{emoji}', 'foobar', 0, 5);
    expect(batch.length, 5);
    expect(batch.first, g.generate('{emoji}', 'foobar', 0));
    g.dispose();
  });

  test('malformed pattern throws', () {
    final g = open();
    expect(() => g.generate('{', 'foobar', 0), throwsA(isA<SlugkitException>()));
    g.dispose();
  });
}
