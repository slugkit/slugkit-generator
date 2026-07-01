import 'package:flutter/material.dart';
import 'package:flutter/services.dart' show rootBundle;
import 'package:slugkit/slugkit.dart';

void main() => runApp(const SlugkitApp());

class SlugkitApp extends StatelessWidget {
  const SlugkitApp({super.key});

  @override
  Widget build(BuildContext context) => MaterialApp(
        title: 'SlugKit',
        theme: ThemeData(colorSchemeSeed: Colors.indigo, useMaterial3: true),
        home: const HomePage(),
      );
}

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  final _pattern = TextEditingController(text: '{adverb}-{emoji}');
  final _seed = TextEditingController(text: 'foobar');
  Generator? _generator;
  String _version = '';
  String _capacity = '';
  List<String> _slugs = [];
  String? _error;

  @override
  void initState() {
    super.initState();
    _load();
  }

  Future<void> _load() async {
    // Load the two compiled binary dictionaries bundled as assets, and build one generator over both.
    final emoji = (await rootBundle.load('assets/emoji.bin')).buffer.asUint8List();
    final adverb = (await rootBundle.load('assets/test-adv.slugs')).buffer.asUint8List();
    final gen = Generator.fromMultiple([adverb, emoji]);
    setState(() {
      _generator = gen;
      _version = gen.version;
    });
    _generate();
  }

  void _generate() {
    final gen = _generator;
    if (gen == null) return;
    try {
      final cap = gen.capacity(_pattern.text);
      setState(() {
        _error = null;
        _capacity = '${cap.value} (max length ${cap.maxLength})';
        _slugs = gen.generateBatch(_pattern.text, _seed.text, 0, 12);
      });
    } on SlugkitException catch (e) {
      setState(() {
        _error = e.message;
        _capacity = '';
        _slugs = [];
      });
    }
  }

  @override
  void dispose() {
    _generator?.dispose();
    _pattern.dispose();
    _seed.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('SlugKit'),
        actions: [
          Padding(
            padding: const EdgeInsets.only(right: 16),
            child: Center(child: Text('engine $_version', style: const TextStyle(fontSize: 12))),
          ),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            TextField(
              controller: _pattern,
              decoration: const InputDecoration(labelText: 'Pattern', border: OutlineInputBorder()),
              onSubmitted: (_) => _generate(),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: _seed,
              decoration: const InputDecoration(labelText: 'Seed', border: OutlineInputBorder()),
              onSubmitted: (_) => _generate(),
            ),
            const SizedBox(height: 12),
            FilledButton.icon(
              onPressed: _generator == null ? null : _generate,
              icon: const Icon(Icons.casino),
              label: const Text('Generate'),
            ),
            const SizedBox(height: 12),
            if (_error != null)
              Text('Error: $_error', style: const TextStyle(color: Colors.red))
            else
              Text('Capacity: $_capacity', style: Theme.of(context).textTheme.bodySmall),
            const Divider(),
            Expanded(
              child: ListView.separated(
                itemCount: _slugs.length,
                separatorBuilder: (_, __) => const Divider(height: 1),
                itemBuilder: (_, i) => ListTile(
                  dense: true,
                  leading: Text(i.toString().padLeft(2, '0')),
                  title: Text(_slugs[i], style: const TextStyle(fontSize: 18)),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
