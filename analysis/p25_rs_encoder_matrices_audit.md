# P25 Reed–Solomon Encoder Matrices — MMDVMHost Cross-Reference Audit

**Audit subject:** `g4klx/MMDVMHost` `RS634717.cpp` — the three
systematic `[I | P]` Reed–Solomon encoder matrices over GF(2⁶):

- `ENCODE_MATRIX_362017` (20 × 36) — RS(36,20,17), used for HDU
- `ENCODE_MATRIX_24169`  (16 × 24) — RS(24,16,9), used for LDU2 ES
- `ENCODE_MATRIX_241213` (12 × 24) — RS(24,12,13), used for LDU1 LC and TDULC

**Spec under audit:** BAAA-B Implementation Spec §8.4.2, §8.4.3
(generator polynomials and corresponding generator matrices), against
TIA-102.BAAA-B PDF Tables 7, 8, 9.

**Companion to:** `analysis/p25_hdu_ldu_lsd_implementation_audit.md`,
which validated the GF(2⁶) tables underlying these matrices but
deferred the matrix-entry comparison itself. This audit closes that
deferred follow-up.

## TL;DR

All three RS encoder matrices in MMDVMHost (≈1,500 GF(2⁶) entries
total across 48 parity rows) are **mathematically consistent** with
the generator polynomials given in BAAA-B Impl Spec §8.4.2 / §8.4.3.

| Matrix | Dimensions | Identity portion | Parity rows match polynomial form? |
|---|:---:|:---:|:---:|
| `ENCODE_MATRIX_362017` (RS(36,20,17)) | 20 × 36 | ✓ I_20 | ✓ all 20 rows |
| `ENCODE_MATRIX_24169` (RS(24,16,9))   | 16 × 24 | ✓ I_16 | ✓ all 16 rows |
| `ENCODE_MATRIX_241213` (RS(24,12,13)) | 12 × 24 | ✓ I_12 | ✓ all 12 rows |

The validation works by computing, for each row `i` of each matrix,
the expected parity portion as `(eᵢ · x^r) mod g(x)` over GF(2⁶) (where
`eᵢ` is the i-th basis vector and `g(x)` is the spec's generator
polynomial), and comparing against MMDVMHost's stored entry. This
confirms the matrices are derivable from the polynomials — they are
not an independent choice of `[I | P]` representation but the
canonical systematic encoder for the given polynomial.

## Convention identification

For implementers building their own matrix-form RS encoder from the
impl spec's generator polynomials, this audit also pins down the
indexing convention used by MMDVMHost (which matches the BAAA-B PDF's
tabulated form):

- **Row indexing: MSB-first.** Row 0 corresponds to the
  highest-degree info-polynomial coefficient (i.e., info polynomial =
  x^(k-1) for row 0, x^(k-2) for row 1, …, x^0 for row k-1).
- **Parity-column indexing: MSB-first.** Within the parity portion,
  the leftmost parity column carries the highest-degree remainder
  coefficient (x^(r-1)), and the rightmost carries the constant term
  (x^0).

So for any row `i`, the stored parity values are:

```
parity_columns[k..k+r-1] = (x^(r) · x^(k-1-i) mod g(x))   [coefficients in DECREASING degree order]
                         = remainder when shifted info is divided by g(x), high-to-low
```

A LSB-first row indexing or LSB-first parity-column ordering would
not match MMDVMHost — and would not match the BAAA-B PDF tables
either; the convention is uniform across the standard, the matrices,
and the polynomials.

## Provenance and license

`RS634717.cpp` / `RS634717.h`: Jonathan Naylor (G4KLX) © 2016, 2023,
2024, 2025; Bryan Biedenkapp (gatekeep@gmail.com, N2PLL) © 2018, 2023.
GPLv2.

The encoder matrices in this file were authored by Biedenkapp (an
independent P25 implementer also active on the OP25 P25 Phase 2 work).
He extracted them directly from the TIA-102.BAAA-B PDF Tables 7, 8,
9. This audit transitively validates that those PDF tables are
consistent with the generator polynomials in §8 of the same PDF.

### Clean-room handling

GPLv2. Implementer in `~/blip25-mbe/` MUST NOT read. Spec-author side
reads freely. This audit doc is the consumable derived work.

## Verification methodology

Same approach as the §8.4.1 GF(2⁶) audit and the §8.2.1 Golay
generator audit: parse MMDVMHost's C source, compute the spec-derived
expected values from a clean reference implementation (the generator
polynomial divided into shifted basis vectors over GF(2⁶) using the
impl spec's EXP/LOG tables), and compare entry-by-entry.

```python
import re

# Parse all three matrices from RS634717.cpp (octal literals decoded by leading-zero rule).
def parse_matrix(text, name):
    pat = rf'const unsigned char {name}\[(\d+)U?\]\[(\d+)U?\]\s*=\s*\{{(.*?)\}};'
    m = re.search(pat, text, re.DOTALL)
    rows, cols = int(m.group(1)), int(m.group(2))
    body = m.group(3)
    tokens = re.findall(r'\b\d+\b', body)
    vals = [int(t, 8) if len(t) > 1 and t.startswith('0') else int(t) for t in tokens]
    return [vals[r*cols:(r+1)*cols] for r in range(rows)]

# GF(2^6) tables (verified to match impl spec in p25_hdu_ldu_lsd_implementation_audit.md).
GF_EXP = [1, 2, 4, 8, 16, 32, 3, 6, 12, 24, 48, 35, ...]   # 64 entries, sentinel exp[63]=0
GF_LOG = [63, 0, 1, 6, 2, 12, 7, 26, ...]                   # log[0]=63 sentinel

def gf_mul(a, b):
    if a == 0 or b == 0: return 0
    return GF_EXP[(GF_LOG[a] + GF_LOG[b]) % 63]

# Generator polynomials from impl spec §8.4.2 / §8.4.3 (octal coefficients,
# stored lowest-degree first; g[-1] = 1 for monic).
g_hdr = [int(c, 8) for c in ['60','73','46','51','73','05','42','64',
                              '33','22','27','21','23','02','35','34','1']]
g_es  = [int(c, 8) for c in ['26','06','24','57','60','45','75','67','1']]
g_lc  = [int(c, 8) for c in ['50','41','02','74','11','60','34','71',
                              '03','55','05','71','1']]

def gf_poly_mod(p, g):
    """Compute p mod g over GF(2^6). g leading coefficient (g[-1]) must be 1."""
    p = list(p)
    while len(p) >= len(g):
        if p[-1] == 0:
            p.pop(); continue
        coef = p[-1]
        for i in range(len(g)):
            p[len(p) - len(g) + i] ^= gf_mul(coef, g[i])
        assert p[-1] == 0
        p.pop()
    while len(p) < len(g) - 1:
        p.append(0)
    return p

def predicted_row(k, r, gen, i):
    """Return the expected parity portion for row i of [I_k | P]."""
    # MSB-first row indexing: row i corresponds to info polynomial x^(k-1-i).
    info = [0] * k
    info[k - 1 - i] = 1
    shifted = [0] * r + info        # info * x^r
    parity = gf_poly_mod(shifted, gen)   # remainder, lowest-degree first
    return parity[::-1]               # MSB-first parity columns: highest-degree leftmost

# Verify all 48 parity rows.
matrices = [
    ('ENCODE_MATRIX_362017', m362017, 20, 16, g_hdr),
    ('ENCODE_MATRIX_24169',  m24169,  16, 8,  g_es),
    ('ENCODE_MATRIX_241213', m241213, 12, 12, g_lc),
]
for name, M, k, r, gen in matrices:
    for i in range(k):
        observed = M[i][k:]
        predicted = predicted_row(k, r, gen, i)
        assert observed == predicted, f"{name} row {i}: observed != predicted"
    print(f"{name}: all {k} parity rows match polynomial-derived predictions ✓")
```

(Full driver script preserved in commit history for the audit
branch.)

## Per-matrix detail

### ENCODE_MATRIX_362017 (RS(36,20,17), HDU)

**Dimensions:** 20 rows × 36 columns. Left 20 columns = I_20. Right 16
columns = parity.

**Generator polynomial** (impl spec §8.4.2, octal coefficients,
lowest-degree first):

```
g(x) = 60 + 73x + 46x² + 51x³ + 73x⁴ + 5x⁵ + 42x⁶ + 64x⁷
     + 33x⁸ + 22x⁹ + 27x¹⁰ + 21x¹¹ + 23x¹² + 2x¹³
     + 35x¹⁴ + 34x¹⁵ + x¹⁶
```

Sample: row 0 of the matrix has identity column at position 0 and
parity columns `074, 037, 034, 006, 002, 007, 044, 064, 026, 014,
026, 044, 054, 013, 077, 005`. Predicted: `(e₀ · x¹⁶) mod g(x)` where
`e₀ = x¹⁹` (info polynomial for row 0 in MSB-first convention) →
remainder polynomial in decreasing-degree order is exactly `074,
037, 034, …, 005`. ✓

All 20 rows verified the same way.

### ENCODE_MATRIX_24169 (RS(24,16,9), LDU2 ES)

**Dimensions:** 16 × 24. Left 16 columns = I_16. Right 8 columns =
parity.

**Generator polynomial** (impl spec §8.4.3):

```
g(x) = 26 + 6x + 24x² + 57x³ + 60x⁴ + 45x⁵ + 75x⁶ + 67x⁷ + x⁸
```

All 16 rows verified.

### ENCODE_MATRIX_241213 (RS(24,12,13), LDU1 LC, TDULC)

**Dimensions:** 12 × 24. Left 12 columns = I_12. Right 12 columns =
parity.

**Generator polynomial** (impl spec §8.4.3):

```
g(x) = 50 + 41x + 2x² + 74x³ + 11x⁴ + 60x⁵ + 34x⁶ + 71x⁷
     + 3x⁸ + 55x⁹ + 5x¹⁰ + 71x¹¹ + x¹²
```

All 12 rows verified.

## Implications for the impl spec

§8.4 of the BAAA-B Impl Spec already gives the three generator
polynomials. With this audit, the spec can confidently state two
additional things:

1. The PDF Tables 7, 8, 9 generator-matrix entries are derivable
   from the polynomials given in §8.4 (no independent choice).
2. The matrix-form encoder used by MMDVMHost (and any conformant
   implementation that follows the same convention) is bit-compatible
   with the polynomial-form encoder.

So implementers can pick either form (matrix-multiply or
polynomial-divide) without worrying about cross-implementation
compatibility — both produce the same codewords.

This was already stated in §8.4.4 (after the
`baaa-b-impl-spec-audit-patches` patch in commit `fe95510`); this
audit is the empirical proof.

## What this audit does NOT cover

- **Decoder side.** RS decoders are not specified by P25; any
  conformant Berlekamp–Massey, Peterson–Gorenstein–Zierler, or
  syndrome-table decoder will recover the encoded info from the
  matrices verified here. MMDVMHost's `RS.h` uses
  Berlekamp–Massey via the standard Reed–Solomon class; constants
  there are derived from `rsGFexp` / `rsGFlog` (already validated)
  and the same generator polynomials (validated here transitively).
- **End-to-end test vectors.** A separate audit could feed sample
  info words through both MMDVMHost and a reference implementation
  and compare codewords. The current audit covers the matrix entries
  themselves, which is mathematically equivalent and avoids the test
  vector / endianness friction.

## Recommended impl-spec patches

None new from this audit. The §8.4 content is already correct;
§8.4.4's "either form yields identical codewords" claim (added in
`fe95510`) is now empirically proven for all three RS codes.

Optionally, §8.4.4 could cite this audit explicitly; the analysis
README entry already provides that link.

## References

- TIA-102.BAAA-B (2017) §8 (FEC), Tables 7, 8, 9 — generator
  matrices in the standard.
- `~/blip25-specs/standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`
  §8.4.2, §8.4.3, §8.4.4 — derived implementation spec.
- `g4klx/MMDVMHost` `RS634717.cpp` — third-party encoder matrices,
  GPLv2 (Naylor) + Bryan Biedenkapp / N2PLL contributions.
- `analysis/p25_hdu_ldu_lsd_implementation_audit.md` — companion
  audit covering the GF(2⁶) layer underlying these matrices.
- `analysis/p25_golay_24_12_implementation_audit.md`,
  `p25_imbe_frame_extraction_audit.md`,
  `p25_trellis_implementation_audit.md`,
  `p25_nid_implementation_audit.md` — companion MMDVMHost audits.
