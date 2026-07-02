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
  final adverbPath = Platform.environment['SLUGKIT_ADVERB_BIN'] ??
      '../../slugkit/tests/data/test-adv.slugs';
  final dict = File(emojiPath).readAsBytesSync();
  final adverb = File(adverbPath).readAsBytesSync();

  Generator open() => Generator.fromBytes(dict, libraryPath: libPath);
  Generator openMulti() =>
      Generator.fromMultiple([adverb, dict], libraryPath: libPath);

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

  // Multi-dictionary: load adverb + emoji (n=2) and generate real word patterns.
  // Golden values (seed "foobar") are byte-identical across all language bindings.
  group('multi-dictionary word patterns', () {
    test('capacity of {adverb}-{emoji}', () {
      final g = openMulti();
      final cap = g.capacity('{adverb}-{emoji}');
      // Opt-in tags are hidden by default: test-adv marks `nsfw` opt-in (4 adverbs), so the
      // adverb pool is 3615 and 3615 * 1154 emoji = 4171710.
      expect(cap.value, '4171710');
      expect(cap.maxLength, 22);
      g.dispose();
    });

    test('{adverb}-{emoji} golden prefixes', () {
      final g = openMulti();
      expect(g.generate('{adverb}-{emoji}', 'foobar', 0), startsWith('impotently-'));
      expect(g.generate('{adverb}-{emoji}', 'foobar', 1), startsWith('speechlessly-'));
      expect(g.generate('{adverb}-{emoji}', 'foobar', 2), startsWith('diagonally-'));
      g.dispose();
    });

    test('{adverb:<=5}-{number:3d} golden values', () {
      final g = openMulti();
      expect(g.generate('{adverb:<=5}-{number:3d}', 'foobar', 0), 'ago-887');
      expect(g.generate('{adverb:<=5}-{number:3d}', 'foobar', 1), 'aloud-774');
      expect(g.generate('{adverb:<=5}-{number:3d}', 'foobar', 2), 'apart-661');
      g.dispose();
    });

    test('batch of a large capacity pattern is collision-free', () {
      final g = openMulti();
      final slugs = g.generateBatch('{adverb}-{emoji}', 'foobar', 0, 20);
      expect(slugs.toSet().length, 20);
      g.dispose();
    });
  });
}
