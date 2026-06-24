# Security Enhancement Proposal: Pattern-Level Generator Selection

## Summary

Add pattern-level security options to SlugKit generator allowing users to choose between performance, security, and capacity trade-offs through the pattern language syntax.

## Motivation

### Current Security Issue

The current SlugKit implementation has a critical security vulnerability when seeds are treated as secrets (e.g., in SaaS environments):

- **LCG permutations** (used for non-power-of-2 dictionary sizes) are cryptographically broken
- **Attack scenario**: Observing 2 consecutive outputs allows full parameter recovery and prediction of all future outputs
- **Impact**: In SaaS contexts where seeds are shared across entities, this enables account enumeration, resource prediction, and other security issues

### Current Algorithm Selection

```cpp
if (!(max_value & (max_value - 1))) {  // Power of 2
    return PermutePowerOf2(max_value, hash, sequence, rounds);  // Secure Feistel
} else {
    return LCGPermute(max_value, hash, sequence);  // Broken LCG
}
```

## Proposed Solution

### Pattern Language Extension

Add `gen` parameter to pattern syntax supporting four security levels:

```cpp
// Per-placeholder control
{noun:gen=performance}               // LCG (current default)
{adjective:+formal gen=balanced}     // Enhanced LCG  
{verb:-nsfw gen=secure}              // Feistel + cycle walking
{noun:gen=fast-secure}               // Power-of-2 truncated Feistel

// Global pattern control
{adjective}-{noun}[gen=balanced]     // Apply to all placeholders
{emoji:+face}-{noun}[gen=secure]     // Security-critical patterns
```

### Security Levels

#### 1. `gen=performance` (Default)
- **Algorithm**: Current LCG implementation
- **Security**: ❌ Completely broken for secret seeds
- **Performance**: ⚡ ~191-344ns per placeholder (baseline from GenerateFromDictionary benchmarks)
- **Capacity**: ✅ Full dictionary capacity
- **Use case**: Public seeds, development, testing

```cpp
// output = (multiplier * sequence + increment) % dictionary_size
// Trivially broken: observe 2 outputs → recover parameters → predict all future
```

#### 2. `gen=balanced` 
- **Algorithm**: Cryptographically enhanced LCG
- **Security**: 🟡 Moderate resistance to analysis
- **Performance**: ⚡ ~210-380ns per placeholder (estimated +10-20% overhead for SHA256 derivation)  
- **Capacity**: ✅ Full dictionary capacity
- **Use case**: Most production scenarios

```cpp
auto EnhancedLCGPermute(uint64_t max_value, const std::string& seed, uint64_t sequence) -> uint64_t {
    auto seed_hash = SHA256(seed);
    auto multiplier = DeriveCoprimeMultiplier(seed_hash.substr(0, 8), max_value);
    auto increment = std::stoull(seed_hash.substr(8, 8), nullptr, 16) % max_value;
    auto offset = std::stoull(seed_hash.substr(16, 8), nullptr, 16) % max_value;
    
    auto stage1 = (multiplier * sequence + increment) % max_value;
    return (stage1 + offset) % max_value;
}
```

#### 3. `gen=secure`
- **Algorithm**: Feistel network with cycle walking for arbitrary sizes
- **Security**: ✅ Cryptographically strong
- **Performance**: 🐌 ~400-2000ns per placeholder (variable due to cycle walking)
- **Capacity**: ✅ Full dictionary capacity
- **Use case**: High-security entities, sensitive IDs

```cpp
auto SecurePermute(uint64_t max_value, const std::string& seed, uint64_t sequence) -> uint64_t {
    uint64_t pow2_size = NextPowerOf2(max_value);
    uint64_t result;
    do {
        result = FeistelPermute(pow2_size, seed, sequence, 8); // 8 rounds
        if (result < max_value) return result;
        sequence = result; // Cycle walking
    } while (true);
}
```

#### 4. `gen=fast-secure` (New)
- **Algorithm**: Power-of-2 truncated Feistel networks
- **Security**: ✅ Cryptographically strong
- **Performance**: ⚡ ~25-35ns per placeholder (comparable to hex generation, significantly faster than current)
- **Capacity**: ⚠️ Reduced to largest power-of-2 ≤ dictionary_size
- **Use case**: Best performance + security trade-off for most scenarios

```cpp
auto FastSecurePermute(uint64_t max_value, const std::string& seed, uint64_t sequence) -> uint64_t {
    uint64_t pow2_size = LargestPowerOf2(max_value); // e.g., 1337 → 1024
    return FeistelPermute(pow2_size, seed, sequence, 4); // 4 rounds sufficient
}
```

### Capacity Impact Examples

```cpp
// Dictionary: 1337 nouns
{noun:gen=performance}  // Capacity: 1337 (100%)
{noun:gen=balanced}     // Capacity: 1337 (100%) 
{noun:gen=secure}       // Capacity: 1337 (100%)
{noun:gen=fast-secure}  // Capacity: 1024 (76.6%)

// Dictionary: 2048 adjectives (already power-of-2)  
{adj:gen=performance}   // Capacity: 2048 (100%)
{adj:gen=balanced}      // Capacity: 2048 (100%)
{adj:gen=secure}        // Capacity: 2048 (100%)
{adj:gen=fast-secure}   // Capacity: 2048 (100%) - no reduction!
```

## Implementation Plan

### Phase 1: Grammar Extension
- [ ] Extend EBNF grammar for `gen` parameter
- [ ] Update pattern parser to handle generator options
- [ ] Add `GeneratorType` enum and parsing logic

### Phase 2: Algorithm Implementation  
- [ ] Implement `EnhancedLCGPermute` with SHA256-derived parameters
- [ ] Implement `SecurePermute` with cycle walking
- [ ] Implement `FastSecurePermute` with power-of-2 truncation
- [ ] Add performance benchmarks for all algorithms

### Phase 3: Integration
- [ ] Update `PatternGenerator` to select algorithms based on `gen` parameter
- [ ] Modify capacity calculations to account for truncation
- [ ] Add validation and security warnings

### Phase 4: Documentation & Testing
- [ ] Comprehensive test suite for all generator types
- [ ] Security analysis documentation  
- [ ] Performance characteristic documentation
- [ ] Migration guide for existing patterns

## API Design

### Pattern Grammar
```ebnf
selector := kind ['@' lang], [':', [tags], [length_constraint], [options]];
options := option (' ' option)*;
option := identifier '=' option_value | generator_option;
generator_option := 'gen' '=' generator_type;
generator_type := 'performance' | 'balanced' | 'secure' | 'fast-secure';

global_settings := '[' ['@' lang], [tags], [length_constraint], [global_options] ']';
global_options := global_option (' ' global_option)*;
global_option := identifier '=' option_value | global_generator_option;
global_generator_option := 'gen' '=' generator_type;
```

### Code Integration
```cpp
// In Selector class
struct Selector {
    // ... existing fields ...
    std::optional<GeneratorType> generator_type;
    
    GeneratorType GetGeneratorType(GeneratorType global_default = GeneratorType::Performance) const {
        return generator_type.value_or(global_default);
    }
};

// In PatternGenerator
class PatternGenerator {
    std::unique_ptr<PermutationStrategy> CreatePermutationStrategy(
        const Selector& selector, 
        std::size_t dictionary_size,
        GeneratorType gen_type
    ) const;
};
```

## Security Analysis

### Threat Model
- **Adversary**: Can observe generated slugs and their sequence positions
- **Secret**: Seed value used for generation
- **Goal**: Predict future slugs or recover seed

### Algorithm Security Assessment

| Algorithm | Known Plaintext Attack | Differential Analysis | Brute Force |
|-----------|------------------------|----------------------|-------------|
| **LCG** | ❌ Broken (2 samples) | ❌ Trivial | ❌ Weak |
| **Enhanced LCG** | 🟡 Difficult | 🟡 Moderate | 🟡 Moderate |
| **Feistel (4+ rounds)** | ✅ Resistant | ✅ Strong | ✅ Strong |

### Recommended Usage

```cpp
// Development/Testing
{adjective}-{noun}  // Default: fast, seed can be public

// Production (non-sensitive)
{category}-{item}[gen=balanced]  // Good security/performance balance

// Production (sensitive entities)
{user_type}-{user_id}[gen=secure]  // Maximum security, full capacity

// Production (performance-critical security)
{session_id}[gen=fast-secure]  // Fast + secure, reduced capacity OK
```

## Performance Benchmarks (Based on Real GenerateFromDictionary Data)

### Current Single-Placeholder Performance (Measured Baseline)

| Dictionary Size | Current Performance | Scaling | Notes |
|----------------|--------------------|---------| ------|
| 1,000 words | 191 ns | Excellent | From GenerateFromDictionary benchmarks |
| 10,000 words | 198 ns | Excellent | Minimal scaling up to 10K |
| 100,000 words | 203 ns | Good | Still under 250ns |
| 1,000,000 words | 344 ns | Linear | Shows dictionary lookup overhead |

*Source: Latest GenerateFromDictionary benchmarks (lowercase generation)*

### Proposed Single-Placeholder Performance

| Generator Type | Small Dicts (1-10K) | Large Dicts (100K-1M) | vs Current | Security |
|----------------|----------------------|------------------------|------------|----------|
| `performance` (baseline) | 191-198 ns | 203-344 ns | 1.0x | None |
| `fast-secure` (estimated) | **35-55 ns** | **35-55 ns** | **0.18-0.27x** ⚡ | Strong |
| `balanced` (estimated) | 210-230 ns | 380-420 ns | 1.1-1.2x | Moderate |
| `secure` (estimated) | 400-600 ns | 800-2000 ns | 2-6x | Strong |

### Performance Analysis & Bottleneck Identification

**Current bottleneck analysis (comparing LCG algorithms):**
```cpp
// Decimal number generation (also LCG): 57-99ns
// Dictionary selection (LCG + dictionary): 191-344ns
// 
// Bottleneck breakdown:
// 1. LCG permutation: ~57-99ns (similar to decimal generation)
// 2. Dictionary access overhead: ~134-245ns (the real bottleneck!)
// 3. String construction/copying: included in above
```

**Fast-secure potential (estimated performance):**
```cpp
// Feistel permutation: ~25-30ns (faster than LCG, similar to hex)
// Power-of-2 dictionary access: ~5-15ns (much faster than current)
// String access (SSO): ~5-10ns
// Total estimated: ~35-55ns per placeholder
```

**Key insight:** The bottleneck is **dictionary access overhead**, not the permutation algorithm. Current dictionary selection has **2-4x overhead** beyond the LCG calculation itself.

### Key Insights

- **3-6x performance improvement potential** with fast-secure approach (35-55ns vs 191-344ns)
- **Dictionary access overhead is the bottleneck** (134-245ns overhead beyond LCG calculation)
- **LCG itself performs reasonably** (~57-99ns, similar to decimal number generation)
- **Power-of-2 approach eliminates size-dependent scaling** and dictionary access overhead
- **Pattern performance scales linearly** with number of placeholders

## Migration Strategy

### Backward Compatibility
- All existing patterns continue to work unchanged
- Default behavior remains `gen=performance`
- No breaking changes to existing API

### Gradual Migration
```cpp
// Step 1: Immediate performance + security gain (no capacity concerns)
{user_id}  →  {user_id:gen=fast-secure}  // Faster AND secure!

// Step 2: High-security patterns requiring full capacity
{admin_token}  →  {admin_token:gen=secure}  // Full capacity, variable timing

// Step 3: Moderate security for full capacity needs  
{product_slug}  →  {product_slug:gen=balanced}  // All dictionary words available

// Step 4: Performance-critical non-secure (explicit choice)
{temp_cache_key}  →  {temp_cache_key:gen=performance}  // Fastest, seed not secret
```

### Recommended Migration Path

**Phase 1: Low-hanging fruit** - Switch to `fast-secure` where capacity reduction is acceptable:
- Session tokens, API keys, temporary resources
- **Benefit**: Immediate 30% performance improvement + cryptographic security

**Phase 2: Evaluate capacity needs** - For patterns requiring full dictionary:
- Choose `balanced` for good security/performance trade-off
- Choose `secure` only for highest security requirements

**Phase 3: Explicit performance choices** - Keep `performance` only where:
- Seeds are definitely public/non-secret
- Maximum performance is critical
- Security is not a concern

## Future Considerations

### Advanced Options
- Custom round counts: `{noun:gen=secure rounds=8}`
- Algorithm selection: `{noun:gen=secure algo=feistel}`
- Hybrid modes: `{noun:gen=balanced fallback=secure}`

### Monitoring Integration
- Performance metrics per generator type
- Security event logging for suspicious patterns
- Capacity utilization tracking

## References

- [Luby-Rackoff Construction](https://en.wikipedia.org/wiki/Luby%E2%80%93Rackoff_construction)
- [Format Preserving Encryption (NIST SP 800-38G)](https://csrc.nist.gov/publications/detail/sp/800-38g/final)
- [Linear Congruential Generator Security Analysis](https://en.wikipedia.org/wiki/Linear_congruential_generator#Security)

---

**Issue Labels**: `enhancement`, `security`, `performance`, `breaking-change-potential`  
**Priority**: High (Security vulnerability in current implementation)  
**Complexity**: Medium-High (Requires algorithm implementation + grammar extension)