import 'package:flutter/services.dart' show rootBundle;
import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:slugkit/slugkit.dart';

// End-to-end test that runs on a real device/desktop, so the native SlugKit framework is linked
// into the app and DynamicLibrary.process() resolves the C ABI. Golden values (seed "foobar") match
// the C ABI / Swift / Kotlin / Dart bindings — proving byte-identical output through the plugin.
void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  late Generator gen;

  setUpAll(() async {
    final emoji = (await rootBundle.load('assets/emoji.bin')).buffer.asUint8List();
    final adverb = (await rootBundle.load('assets/test-adv.slugs')).buffer.asUint8List();
    gen = Generator.fromMultiple([adverb, emoji]);
  });

  tearDownAll(() => gen.dispose());

  test('engine version and seed', () {
    expect(gen.version, isNotEmpty);
    expect(gen.randomSeed(), isNotEmpty);
  });

  test('capacity of {adverb}-{emoji}', () {
    final cap = gen.capacity('{adverb}-{emoji}');
    // Opt-in `nsfw` (4 adverbs) hidden by default -> pool 3615, 3615 * 1154 = 4171710.
    expect(cap.value, '4171710');
    expect(cap.maxLength, 22);
  });

  test('{adverb}-{emoji} golden prefixes', () {
    expect(gen.generate('{adverb}-{emoji}', 'foobar', 0), startsWith('impotently-'));
    expect(gen.generate('{adverb}-{emoji}', 'foobar', 1), startsWith('speechlessly-'));
    expect(gen.generate('{adverb}-{emoji}', 'foobar', 2), startsWith('diagonally-'));
  });

  test('{adverb:<=5}-{number:3d} golden values', () {
    expect(gen.generate('{adverb:<=5}-{number:3d}', 'foobar', 0), 'ago-887');
    expect(gen.generate('{adverb:<=5}-{number:3d}', 'foobar', 1), 'aloud-774');
    expect(gen.generate('{adverb:<=5}-{number:3d}', 'foobar', 2), 'apart-661');
  });

  test('batch is collision-free', () {
    final slugs = gen.generateBatch('{adverb}-{emoji}', 'foobar', 0, 20);
    expect(slugs.toSet().length, 20);
  });

  test('malformed pattern throws', () {
    expect(() => gen.generate('{', 'foobar', 0), throwsA(isA<SlugkitException>()));
  });
}
