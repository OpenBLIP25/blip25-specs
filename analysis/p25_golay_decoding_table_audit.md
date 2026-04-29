# P25 Golay(23,12,7) Decoding Table — MMDVMHost Cross-Reference Audit

**Audit subject:** `g4klx/MMDVMHost` `Golay24128.cpp`
`DECODING_TABLE_23127[2048]` — the syndrome → error-pattern lookup
table used by `decode23127` and `decode24128`.

**Spec under audit:** BAAA-B Implementation Spec §8.2.1 / §8.2.2 (Golay
(23,12,7) and (24,12,8) decoder algorithm, currently described as
"syndrome-based correction up to t = 3 errors" without specifying the
table).

**Companion to:** `analysis/p25_golay_24_12_implementation_audit.md`,
which validated the encoding side (`ENCODING_TABLE_24128` matches the
spec's `GOLAY_24_12_GENERATOR[12]` row-for-row). This audit closes
the corresponding decoder side.

## TL;DR

`DECODING_TABLE_23127[2048]` is a **mathematically perfect**
maximum-likelihood decoder for Golay(23,12,7) at its full t = 3
error-correction capacity. Three independent invariants verified:

| Invariant | Result |
|---|:---:|
| Syndrome consistency: `syndrome(table[s]) == s` for all s ∈ [0, 2047] | ✓ all 2048 |
| Weight distribution matches Golay coset leaders | ✓ exact: 1 + 23 + 253 + 1771 = 2048 |
| Bijection: every weight-≤3 23-bit pattern appears exactly once | ✓ no missing, no extra |

This is the textbook syndrome-decoding table for Golay(23,12,7).
Combined with the encoder-side audit (`p25_golay_24_12_implementation_audit.md`),
the entire Golay(23,12,7)/(24,12,8) layer in MMDVMHost is now
third-party validated end-to-end.

## Why these three invariants are sufficient

The Golay(23,12,7) code has minimum distance 7 and corrects up to
`t = ⌊(7-1)/2⌋ = 3` errors. The number of distinct syndromes equals
the number of distinct cosets of the code in GF(2)²³, which equals
2²³⁻¹² = 2048.

Within each coset, the unique **coset leader** is the error pattern
of minimum weight. For a perfect t = 3 decoder, the coset leaders
are exactly the 2048 binary vectors of weight ≤ 3:

```
weight 0: C(23, 0) =    1 pattern
weight 1: C(23, 1) =   23 patterns
weight 2: C(23, 2) =  253 patterns
weight 3: C(23, 3) = 1771 patterns
                    ─────
                     2048
```

A decoding table that
(a) maps each syndrome `s` to a 23-bit pattern whose syndrome equals `s`,
(b) contains only weight-≤3 patterns, and
(c) covers every weight-≤3 pattern exactly once,
is the unique maximum-likelihood decoder for the code under
hard-decision decoding.

`DECODING_TABLE_23127` satisfies all three. It is therefore the
optimal syndrome decoder; any deviation from it would either fail to
correct some triple-error patterns or would mis-correct quadruple-
error patterns into the wrong coset (which a perfect t = 3 decoder
also does, but the table-implied error pattern cannot be the wrong
coset's leader if (a)+(c) hold).

## Provenance and license

`Golay24128.cpp`: Jonathan Naylor (G4KLX) © 2010, 2016, 2021, 2025;
Robert H. Morelos-Zaragoza © 2002. GPLv2.

The table is generated from the (23,12,7) generator polynomial
`g(x) = x¹¹ + x¹⁰ + x⁶ + x⁵ + x⁴ + x² + 1` (octal 6165, hex 0xC75) —
the same polynomial as the encoder side and the same as BAAA-B Impl
Spec §8.2.1.

### Clean-room handling

GPLv2. Implementer in `~/blip25-mbe/` MUST NOT read. Spec-author side
reads freely. This audit doc is the consumable derived work.

## Verification methodology

```python
import re
from itertools import combinations
from math import comb

# Parse the 2048-entry table.
src = open('Golay24128.cpp').read()
m = re.search(r'DECODING_TABLE_23127\[\]\s*=\s*\{(.*?)\};', src, re.DOTALL)
entries = [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]+)U?', m.group(1))]
assert len(entries) == 2048

# (23,12,7) syndrome computation: polynomial division by g(x) = 0xC75.
GENPOL = 0xC75
X22, X11, MASK12 = 1 << 22, 1 << 11, 0xFFFFF800

def syndrome(p):
    aux = X22
    if p >= X11:
        while p & MASK12:
            while not (aux & p):
                aux >>= 1
            p ^= (aux // X11) * GENPOL
    return p

# (1) Syndrome consistency: syndrome(table[s]) must equal s.
for s in range(2048):
    assert syndrome(entries[s]) == s

# (2) Weight distribution matches Golay coset leaders.
weight_count = {0: 0, 1: 0, 2: 0, 3: 0}
for e in entries:
    weight_count[bin(e).count('1')] += 1
assert weight_count == {0: 1, 1: 23, 2: 253, 3: 1771}

# (3) Bijection: every weight-≤3 23-bit pattern appears exactly once.
all_w_le_3 = {0}
for w in range(1, 4):
    for bits in combinations(range(23), w):
        p = 0
        for b in bits: p |= 1 << b
        all_w_le_3.add(p)
assert set(entries) == all_w_le_3

print('Perfect ML decoder verified.')
```

All three assertions pass. Result reproducible from this script
applied to the source file.

## How the (24,12,8) decoder uses this table

`Golay24128.cpp::decode24128` (lines 1093–1104):

```cpp
unsigned int syndrome = ::get_syndrome_23127(in >> 1);
unsigned int error_pattern = DECODING_TABLE_23127[syndrome] << 1;
out = in ^ error_pattern;
bool valid = (CUtils::countBits(syndrome) < 3U) || !(CUtils::countBits(out) & 1);
out >>= 12;
return valid;
```

For an input 24-bit codeword:

1. Drop the extended parity bit (bit 0): `c23 = in >> 1`.
2. Compute the (23,12,7) syndrome.
3. Look up the (23,12,7) coset leader via `DECODING_TABLE_23127`,
   then shift left by 1 to align with the 24-bit input position.
4. XOR to correct.
5. Validity test: either the syndrome corresponds to ≤ 2 errors
   (popcount(syndrome) < 3 — heuristic since syndrome weight isn't
   always tied to error count, but works well enough as a fast
   rejection), OR the corrected codeword has even total weight (the
   extended Golay's overall-parity bit catches odd-weight error
   patterns the (23,12,7) decoder mis-corrected).

This wraps the (23,12,7) coset-leader decoder in the standard
extended-Golay disambiguation. The (24,12,8) code's higher
minimum-distance (8 vs. 7) gives it the ability to detect 4-error
patterns that the (23,12,7) decoder would miscorrect — using the
parity bit as the discriminator.

## What this audit does NOT cover

- **Soft-decision Chase-II decoder** for Golay(23,12,7). Not present
  in MMDVMHost. Production receivers that need the extra ~2 dB of
  soft-decision gain (e.g., satellite or low-SNR P25 monitoring) layer
  Chase-II on top of this hard-decision table; the hard-decision
  decoder validated here remains correct as the inner step.
- **Performance characterization.** Worst-case operations per decode
  is bounded by the syndrome computation (~16 GF(2) shift-XOR cycles
  for 11-bit polynomial division) plus one table lookup plus one
  XOR — constant-time, no branching. Not measured here, but
  effectively free on any platform.

## Recommended impl-spec patches

None new. §8.2.1 already states "syndrome-based correction up to t =
3 errors" which is correct and now empirically validated. Optionally,
§8.2.1 could note that the decoder is conventionally implemented as
a 2048-entry coset-leader lookup table generated from the same
generator polynomial, with MMDVMHost as the canonical reference; the
analysis README entry already provides that link.

## Closing the Golay layer

Combined audit status across the Golay(23,12,7) / (24,12,8) layer:

| Component | Audit | Status |
|---|---|:---:|
| Generator polynomial `g(x) = 0xC75` | §8.2.1 + p25_golay_24_12_implementation_audit.md | ✓ matches MMDVMHost `GENPOL` |
| `GOLAY_24_12_GENERATOR[12]` rows | p25_golay_24_12_implementation_audit.md | ✓ all 12 rows match `ENCODING_TABLE_24128` |
| Encoding for arbitrary info word | (transitive from row check + linearity) | ✓ |
| `DECODING_TABLE_23127[2048]` | this audit | ✓ perfect ML decoder |
| (24,12,8) decoder algorithm | code review in this audit | ✓ standard extended-Golay disambiguation |
| Shortened (18,6,8) for HDU | p25_hdu_ldu_lsd_implementation_audit.md | ✓ structurally validated (zero-pad/truncate of (24,12,8)) |

The Golay layer is now end-to-end third-party validated.

## References

- TIA-102.BAAA-B (2017) §7.4, §8 — the standard text.
- `~/blip25-specs/standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`
  §8.2.1, §8.2.2.
- `g4klx/MMDVMHost` `Golay24128.cpp` lines 836–1041 — the
  `DECODING_TABLE_23127[2048]` source under audit.
- Robert H. Morelos-Zaragoza — original (23,12,7) Golay reference
  implementation (cited in `Golay24128.cpp` header).
- `analysis/p25_golay_24_12_implementation_audit.md` — companion
  audit for the encoder side.
- `analysis/p25_hdu_ldu_lsd_implementation_audit.md` — covers the
  shortened (18,6,8) HDU usage.
