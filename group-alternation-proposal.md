# Group alternation & locked tags — design proposal

**Status:** draft / RFC
**Depends on:** placeholder alternation (`{a}|{b}`, PRs #29/#30)
**Goal:** let a run of placeholders + text be grouped with `( … )` and alternated,
so correlated ("locked") choices across placeholders are expressible:

```
({Name:+given+male-unisex} {Name:+surname+male-unisex})
  |({Name:+given+female-unisex} {Name:+surname+female-unisex})
  |({Name:+given+unisex} {Name:+surname+unisex})
```

A given name and surname now share the same gender branch — no female given name
paired with a male surname.

---

## 1. Why the engine can't do this today

Placeholders are composed **independently**: each placeholder permutes the *same*
sequence number under a seed-stepped seed, and the pattern capacity is the **LCM**
of the per-placeholder capacities. There is deliberately no shared state between
placeholders, so `{Name:+given} {Name:+surname}` cannot correlate a hidden
gender dimension.

Group alternation introduces the correlation *structurally*: each branch is a
self-contained sub-pattern whose tags are already consistent, and generation
picks exactly one branch. Tags are "locked" by construction; nothing new is
needed in the per-placeholder machinery.

## 2. Decisions (agreed)

1. **Flat groups** — a group contains simple placeholders and literal text only;
   no nested groups/alternations in v1.
2. **Grouping binds the whole group contents** — so `(foo)|(bar)` is valid (a
   group may be pure literal text), and `pre-(…)|(…)-post` binds the whole
   `(…)|(…)` run as one element with surrounding text.
3. **Per-position disjointness** — the validation sweep checks branch overlap
   position-by-position (cheap and exact for same-shape groups) rather than
   enumerating full cross-products.
4. **Parentheses are reserved** — `(` and `)` are operators; `\(` and `\)` are
   literal parentheses (consistent with `\|`).

## 3. This is a small step from what already exists

The alternation generator built in #29 is **child-agnostic**: it holds
`std::vector<SubstitutionGeneratorPtr>`, and selects a child by cumulative
capacity (`capacity = Σ children`, offset → child, collision-free). A *group* is
just a **mini pattern generator** — exactly the logic in `PatternGenerator::Impl::Generate`:

```cpp
for (auto& g : sub_generators) { seed += kSeedStep; parts.push_back(g->Generate(seed, seq)); }
return Format(sub_text_chunks, parts);   // LCM capacity across the group's placeholders
```

So the plan is:
- add a `GroupSubstitutionGenerator` implementing the `SubstitutionGenerator`
  interface (LCM capacity, seed-stepped sub-generation, formats with the group's
  own text chunks);
- feed groups into the **existing** `AlternationSubstitutionGenerator` unchanged
  (Σ of group capacities, offset selection).

A bare placeholder alternative `{noun}` becomes a degenerate one-placeholder
group, unifying placeholder- and group-alternation on one code path.

## 4. Grammar

New reserved characters `(` `)`; `\(` `\)` escape to literals. Additions:

```ebnf
element        := alternation | group_or_ph ;
alternation    := group_or_ph, { '|', group_or_ph } ;   (* >= 2 distinct after collapse *)
group_or_ph    := group | placeholder ;
group          := '(', group_body, ')' ;
group_body     := ARBITRARY, { placeholder, ARBITRARY } ;  (* flat: placeholders + text, no '(' , ')' , '|' *)
```

- A **placeholder** alternative (no parens) is a one-placeholder group with empty
  surrounding text.
- A **group** may contain zero placeholders (pure text): `(foo)` → capacity 1,
  output `"foo"`.
- Whitespace is allowed around `|` (as today). `|` and `(`/`)` inside `group_body`
  are only valid escaped.

Examples:

| Pattern | Meaning |
|---|---|
| `{a}|{b}` | placeholder alternation (unchanged) |
| `({a} {b})|({c} {d})` | choose one two-placeholder group |
| `(foo)|(bar)` | choose one literal |
| `pre-({a} {b})|({c})-post` | one alternation element with surrounding text |
| `({a})` (lone group) | see §9 (inlined — parens transparent) |

## 5. AST / model

```cpp
struct Pattern {
    using SimplePlaceholder = std::variant<Selector, NumberGen, SpecialCharGen, EmojiGen>;

    // A flat sub-pattern: literal text interleaved with simple placeholders.
    // Invariant: text_chunks.size() == placeholders.size() + 1  (as for Pattern).
    struct Group {
        TextChunks text_chunks;
        std::vector<SimplePlaceholder> placeholders;
        auto ToString() const -> std::string;   // canonical, re-parseable
        auto GetHash() const -> std::int64_t;
        auto Complexity() const -> std::int32_t;
        auto IsNSFW() const -> bool;
    };

    struct Alternation {
        std::vector<Group> alternatives;   // was vector<SimplePlaceholder>
        // ToString/GetHash/Complexity/IsNSFW as today, delegating to Group.
    };

    using PatternElement = std::variant<Selector, NumberGen, SpecialCharGen, EmojiGen, Alternation>;
};
```

The alternation's alternatives change from `SimplePlaceholder` to `Group`. Bare
placeholders wrap into single-placeholder groups at parse time.

## 6. Generator

```cpp
class GroupSubstitutionGenerator : public SubstitutionGenerator {
    std::vector<SubstitutionGeneratorPtr> generators;   // one per placeholder
    Pattern::TextChunks text_chunks;                    // views into the pattern source
    numeric::BigInt capacity_;      // LCM of the placeholder capacities (1 if none)
    std::size_t max_length_;        // Σ text lengths + Σ placeholder max lengths
public:
    std::string Generate(uint32_t seed, size_t seq) const override {
        std::vector<std::string> parts;
        for (auto& g : generators) { seed += kSeedStep; parts.push_back(g->Generate(seed, seq)); }
        return FormatChunks(text_chunks, parts);   // same interleave as SlugFormatter
    }
    numeric::BigInt GetCapacity() const override { return capacity_; }
    std::size_t GetMaxLength() const override { return max_length_; }
};
```

`AlternationSubstitutionGenerator` is reused verbatim: its children are now
`GroupSubstitutionGenerator`s. `SlugFormatter`'s interleave loop is factored into
a free `FormatChunks(text_chunks, substitutions, unescape)` helper shared by the
top-level formatter and groups (groups un-escape like the top level).

## 7. Capacity, determinism, collision-freeness

- **Group capacity** = `LCM` of its placeholder capacities (empty group = 1),
  identical to how the top-level pattern composes.
- **Alternation capacity** = `Σ` group capacities (unchanged from #29).
- **Generation** = permute the sequence over the sum, select a group block, pass
  the in-block **offset** as the group's sequence (collision-free), then the group
  LCM-composes its placeholders on that offset.
- **Max length** = max over groups of (Σ text + Σ placeholder max lengths).

Worked example (`({given+male} {surname+male})|(…female…)|(…unisex…)`):
`capacity = LCM(|g_male|,|s_male|) + LCM(|g_female|,|s_female|) + LCM(|g_uni|,|s_uni|)`.
No cross-gender pairs — exactly the desired constraint.

## 8. Collapse (from #30, extended to groups)

Equivalent alternatives still collapse (they add no variance). The dedup key is
`Group::GetHash` = combine(text chunks, each placeholder's `GetHash`). So
`(foo)|(foo)` → `(foo)`, and `({a} {b})|({a} {b})` → one group. If all collapse to
one, the element is not an alternation (see §9).

## 9. Lone group (no `|`)

A group not part of an alternation adds no semantics — `({a} {b})` ≡ `{a} {b}`.
**Recommendation:** the parser *inlines* a lone group's text + placeholders into
the enclosing pattern (parentheses are transparent when not alternated). This
avoids a redundant wrapper and keeps `ToString` clean. `(foo)` alone ≡ `foo`.

## 10. Disjointness validation (per-position refinement)

Two branches must not produce the same slug, else the alternation is not
collision-free. Enumerating full group cross-products is too expensive for real
name dictionaries, so:

For a pair of groups `G1`, `G2`:
1. **Different shape** (different sequence of literal chunks / placeholder slots):
   fall back to bounded full-output enumeration; skip (with a logged note) if
   either exceeds the enumerate cap. (Handles e.g. a text-only `(cat)` colliding
   with a `({noun})` that can produce `"cat"`.)
2. **Same shape** (aligned text chunks + aligned placeholder kinds):
   - if any aligned literal text chunk differs → **disjoint** (all slugs differ);
   - else the branches are disjoint iff **some** aligned placeholder slot has
     disjoint output sets: `(A₀×A₁×…) ∩ (B₀×B₁×…) = (A₀∩B₀)×(A₁∩B₁)×…`, empty iff
     any `Aᵢ ∩ Bᵢ = ∅`. Each `Aᵢ ∩ Bᵢ` is a single-placeholder output-set
     intersection (bounded per placeholder, not per product).

For the name example this is cheap and exact: the `-unisex` in the male/female
branches makes the given-name slot disjoint from the unisex branch, so the whole
branch is provably disjoint without enumerating any cross-product.

## 11. Interactions

- **Global settings** `[@lang +tag …]` propagate into group placeholders (extend
  the existing visitor recursion to `Group`).
- **ToString** round-trips: groups render with parentheses; lone groups inline;
  escapes preserved (generation un-escapes, canonical form does not).
- **Serialization**: patterns persist as the source string and are re-parsed, so
  no `io/` changes (same as #29).
- **Escaping**: `\(` `\)` added to the escaped set alongside `\|`.

## 12. Edge cases

- `()` — empty group: capacity 1, empty output. Allow (equivalent to empty text)
  or reject as useless — **reject** with a clear error (matches "useless
  generator" checks elsewhere).
- `(   )` whitespace-only — treated as literal text group of that whitespace.
- Unbalanced `(` or `)` — parse error with column.
- Nested `((…))` — parse error in v1 (flat only).
- `|` or unescaped `(`/`)` inside a group body — parse error (reserved).

## 13. Implementation plan

1. Parser: reserve `(` `)`, `\(` `\)` escapes; parse `group_body`; assemble
   `Group`s; unify bare placeholders as one-placeholder groups; lone-group
   inlining; collapse equivalent groups.
2. Model: `Pattern::Group`; change `Alternation::alternatives` to `vector<Group>`;
   `Group` ToString/GetHash/Complexity/IsNSFW.
3. Generator: factor `FormatChunks`; add `GroupSubstitutionGenerator`; build it in
   `BuildAlternationGenerator` (children become groups).
4. Validation: per-position disjointness (with different-shape enumeration
   fallback), run in the settings sweep.
5. Tests: parser (grouping, escaping, lone-group inline, errors), generator
   (capacity = Σ LCM, determinism, collision-free), disjointness (locked-tag
   example passes; a forgotten `-unisex` errors), plus host golden/c_abi checks.

## 14. Risks / open questions

- **Different-shape disjointness** still needs bounded enumeration; very large
  mismatched groups are skipped (documented, best-effort) — acceptable?
- **Capacity intuition**: authors may expect `product`-like growth; document that
  alternation capacity is the *sum* of branch LCMs.
- **Verbosity**: the group form repeats tags per branch. A future "linked tag"
  sugar could lower to this, but is out of scope here.
- **userver build**: as with #29/#30, the service build is not compiled in the
  dev environment; a devcontainer build is the final check.
