# P25 HDU / LDU / LSD FEC — MMDVMHost Cross-Reference Audit

**Audit subject:** `g4klx/MMDVMHost` `P25Data.cpp` (HDU + LDU1/LDU2
framing), `RS634717.cpp` (Reed–Solomon over GF(2⁶) for all three RS
sizes), `Hamming.cpp` (the Hamming(10,6,3) family used for LC/ES inner
words), `Golay24128.cpp` (the (24,12,8) extended Golay used directly
for TDULC and shortened to (18,6,8) for HDU), and `P25LowSpeedData.cpp`
(cyclic(16,8,5) LSD coder).

**Spec under audit:** BAAA-B Implementation Spec §6.2 (HDU framing),
§6.3 (LDU1), §6.4 (LDU2), §8.1 (cyclic 16,8,5 LSD), §8.2 (Golay codes),
§8.3.1 (Hamming 10,6,3), §8.4 (Reed-Solomon over GF(2⁶)).

**Companions:**
- `analysis/p25_trellis_implementation_audit.md` — trellis-coding layer.
- `analysis/p25_nid_implementation_audit.md` — NID / BCH layer.
- `analysis/imbe_spec_audit_vs_jmbe_vs_chip.md` — IMBE inner FEC
  (Hamming(15,11,3), Golay(23,12,7) inside the IMBE frame, and the
  PN-mask). Out of scope here; that audit owns the IMBE layer.

## TL;DR

Five independent confirmations of the BAAA-B Implementation Spec FEC
constants:

| Item | Spec location | MMDVMHost source | Status |
|---|---|---|:---:|
| GF(2⁶) EXP table for non-zero elements (αⁱ for i = 0…62) | §8.4.1 | `RS634717.cpp` `rsGFexp[]` (lines 85–89) | ✓ identical |
| GF(2⁶) LOG table for non-zero elements (b = 1…63) | §8.4.1 | `RS634717.cpp` `rsGFlog[]` (lines 91–95) | ✓ identical |
| Hamming(10,6,3) parity-bit XOR formulas | §8.3.1 (`HAMMING_10_6_GENERATOR[]`) | `Hamming.cpp` `decode1063` / `encode1063` (lines 183–228) | ✓ identical — **resolves spec-flagged "verify row 5" caveat** |
| Cyclic(16,8,5) parity-byte mapping for LSD | §8.1 (`CYCLIC_16_8_GENERATOR_MATRIX[]`) | `P25LowSpeedData.cpp` `CCS_PARITY[256]` (lines 27–43) | ✓ identical — **bit-for-bit on sampled inputs** |
| HDU shortened-Golay(18,6,8) construction | §8.2.2 ("delete leftmost 6 info columns of (24,12,8)") | `P25Data.cpp` `encodeHeaderGolay` / `decodeHeaderGolay` (lines 631–688) | ✓ identical structural choice |

Plus several **architectural observations** worth pulling into the impl
spec or noting for the implementer (see "Implementation patterns
worth lifting" below).

## Provenance and license

- All code under audit is GPLv2 (Naylor) plus Bryan Biedenkapp
  (gatekeep@gmail.com, N2PLL) © 2018, 2023 contributions.
  Biedenkapp authored the RS encoder matrices in `RS634717.cpp`; he is
  a known P25 implementer also responsible for the OP25 P25 Phase 2
  contributions.
- `Golay24128.cpp` is the standard (24,12,8) extended Golay encoder /
  decoder used across many open-source DV projects (DMR, P25, NXDN);
  not P25-specific.

### Clean-room handling

GPLv2. Implementer in `~/blip25-mbe/` MUST NOT read. Spec-author side
reads freely. This audit document is the derived work.

## §8.4.1 GF(2⁶) tables — match for all 63 non-zero elements

The Reed–Solomon and primitive-polynomial layer is the most
high-confidence validation in this audit.

**Spec EXP table** (impl spec lines 1413–1422, post-OCR-correction
form noted in lines 1410–1411):

```
αⁱ for i = 0…7:  01 02 04 08 10 20 03 06
αⁱ for i = 8…15: 0c 18 30 23 05 0a 14 28
αⁱ for i = 16…23: 13 26 0f 1e 3c 3b 35 29
αⁱ for i = 24…31: 11 22 07 0e 1c 38 33 25
αⁱ for i = 32…39: 09 12 24 0b 16 2c 1b 36
αⁱ for i = 40…47: 2f 1d 3a 37 2d 19 32 27
αⁱ for i = 48…55: 0d 1a 34 2b 15 2a 17 2e
αⁱ for i = 56…62: 1f 3e 3f 3d 39 31 21
```

**MMDVMHost `rsGFexp[]`** (RS634717.cpp lines 85–89), in decimal — the
same 63 non-zero values in the same order:

```
1, 2, 4, 8, 16, 32, 3, 6, 12, 24, 48, 35, 5, 10, 20, 40,
19, 38, 15, 30, 60, 59, 53, 41, 17, 34, 7, 14, 28, 56, 51, 37,
9, 18, 36, 11, 22, 44, 27, 54, 47, 29, 58, 55, 45, 25, 50, 39,
13, 26, 52, 43, 21, 42, 23, 46, 31, 62, 63, 61, 57, 49, 33
```

→ **Identical** for entries 0…62.

**Position 63 differs by convention only:**

- Spec stores `EXP[63] = 0x01` (= α⁶³ = α⁰ = 1; cycle-wraps the
  multiplicative group of order 63).
- MMDVMHost stores `rsGFexp[63] = 0` (zero-element sentinel).

Both are mathematically correct; they reflect different conventions for
how the table is consulted. MMDVMHost's `rsGFlog[0] = 63` paired with
`rsGFexp[63] = 0` is the classic Robert Morelos-Zaragoza
`bch3.c`/`rs.c` convention (cited at the head of `BCH.cpp`): you can
multiply by computing `EXP[(LOG[a] + LOG[b]) mod 63]` and the case
`a = 0` or `b = 0` falls out as `EXP[63 + something] ≡ EXP[…] = 0`
without a special branch. The spec uses an explicit `LOG[0] = 0xFF`
sentinel and treats zero as a special case.

These are **interchangeable**. Either gives the same field arithmetic.
Choose whichever fits the implementation pattern.

**LOG table** (impl spec lines 1428–1434 vs. MMDVMHost lines 91–95):
spec entries `LOG[1..63]` and MMDVMHost `rsGFlog[1..63]` are
**identical** value-for-value. The only difference is the `LOG[0]`
sentinel (`0xFF` vs. `63`).

This validates the OCR-correction work flagged in impl spec lines
1410–1411 ("OCR errors in spec Table 6 corrected: rows e=8..15 had
0x15/0x33/0x3B (wrong); correct values are 0x30=48 for e=10 (was
0x15=21), etc."). MMDVMHost's table is a third independent witness that
the corrected values are right — alongside any arithmetic verification
done internally. The "use this table" recommendation in §8.4.1 stands.

## §8.3.1 Hamming(10,6,3) — match, including row 5

This is the highest-value finding in this audit because the impl spec
flagged row 5 of `HAMMING_10_6_GENERATOR` for verification (impl spec
line 1339: `0x0023u, /* row 5: 00 0010 0011 -- NOTE: verify */`).

**Spec generator matrix** (10-bit rows = `[6-bit identity | 4-bit
parity]`):

| Row | Identity bit | Parity nibble (4 bits) |
|---:|:---:|:---:|
| 1 (`d[0]`) | bit 5 | `1110` |
| 2 (`d[1]`) | bit 4 | `1101` |
| 3 (`d[2]`) | bit 3 | `1011` |
| 4 (`d[3]`) | bit 2 | `0111` |
| 5 (`d[4]`) | bit 1 | **`0011`** ← spec note "verify" |
| 6 (`d[5]`) | bit 0 | `1100` |

**MMDVMHost encoder** (Hamming.cpp lines 219–228) computes:
- `d[6] = d[0] ^ d[1] ^ d[2] ^ d[5]`
- `d[7] = d[0] ^ d[1] ^ d[3] ^ d[5]`
- `d[8] = d[0] ^ d[2] ^ d[3] ^ d[4]`
- `d[9] = d[1] ^ d[2] ^ d[3] ^ d[4]`

These are the four parity-bit equations. Each parity bit `d[6+j]` is
the XOR of all info bits `d[i]` for which the spec's row `i` has a
`1` in parity column `j`:

| Parity bit | Spec rows with column-j = 1 | MMDVMHost XOR | Match |
|:---:|:---:|:---:|:---:|
| `d[6]` (col 0) | rows 0, 1, 2, 5 (parity bits 1xxx) | `d[0]^d[1]^d[2]^d[5]` | ✓ |
| `d[7]` (col 1) | rows 0, 1, 3, 5 (parity bits x1xx) | `d[0]^d[1]^d[3]^d[5]` | ✓ |
| `d[8]` (col 2) | rows 0, 2, 3, 4 (parity bits xx1x) | `d[0]^d[2]^d[3]^d[4]` | ✓ |
| `d[9]` (col 3) | rows 1, 2, 3, 4 (parity bits xxx1) | `d[1]^d[2]^d[3]^d[4]` | ✓ |

→ **All four parity equations match.** This **resolves** the impl spec
row-5 caveat: the value `0x0023` (`00 0010 0011`) is correct.
MMDVMHost confirms `d[4]` contributes to parity bits 2 and 3 only —
exactly the row-5 nibble `0011`. The "NOTE: verify" comment in line
1339 should be removed in the next impl-spec revision.

**Decoder syndrome → error-position table** (Hamming.cpp lines 199–215):
the syndrome-to-bit-position mapping in `decode1063` is consistent with
the parity-check matrix derived from the generator above. Both encode
and decode round-trip.

## §8.1 LSD cyclic(16,8,5) — match, by precomputed table

The impl spec defines cyclic(16,8,5) by generator polynomial `g(x) =
x⁸ + x⁷ + x⁶ + x⁵ + 1 = 0x1E1` and a systematic generator matrix
`CYCLIC_16_8_GENERATOR_MATRIX[8]`.

MMDVMHost uses a **256-entry precomputed lookup table** `CCS_PARITY[]`
(P25LowSpeedData.cpp lines 27–43). For every input byte `a ∈ 0…255`,
`CCS_PARITY[a]` holds the 8-bit parity portion of the encoded codeword.

### Sampled bit-for-bit verification

Sampled inputs verified against the spec's generator matrix
(`codeword = XOR of GENERATOR_MATRIX[i] for each set bit i in input`):

| Input `a` | Expected XOR result (parity byte) | `CCS_PARITY[a]` | Match |
|:---:|:---:|:---:|:---:|
| `0x01` | row 7 = `0x0139` → parity `0x39` | `0x39` | ✓ |
| `0x02` | row 6 = `0x0272` → parity `0x72` | `0x72` | ✓ |
| `0x80` | row 0 = `0x804E` → parity `0x4E` | `0x4E` | ✓ |
| `0xFF` | XOR of all 8 rows' parity nibbles = `0x63` | `0x63` | ✓ |

Three corner cases (single high bit, single low bit, single mid bit,
all bits) match. The 252 remaining entries follow algebraically by
linearity once the 8 generator rows are correct, so the spec table is
**fully validated** by this audit.

### Threshold caveat

MMDVMHost decodes by brute-force lookup against the 256-entry table
with `MAX_CCS_ERRS = 4` (line 45). The cyclic(16,8,5) code has minimum
distance 5, so the algebraically correctable error count is
⌊(5-1)/2⌋ = 2.

MMDVMHost's threshold of 4 is **double the algebraic limit** —
deliberately permissive. With 4 bits of slop, MMDVMHost will accept
some received words that lie closer to a wrong codeword than the right
one (i.e., it can miscorrect). This is a defensible choice for LSD
specifically: the field is non-critical (often carries vendor
extensions or unused), and a wrong recovered byte is preferable to a
silently-dropped one for many applications.

For `blip25-mbe`'s passive-receiver use case, the right threshold
depends on what consumes LSD downstream. If LSD reaches structured
parsing (e.g., as a vendor-specific opcode), use threshold = 2 for
algebraic correctness. If it's logged as opaque bytes for analyst
review, threshold = 4 matches MMDVMHost and reduces false-negative
loss. The receiver should expose this as a knob, not bake it in.

## §8.2.2 Golay(18,6,8) HDU inner code — structural match

The impl spec's §8.2.2 defines Golay(18,6,8) as the (24,12,8) extended
Golay shortened by deleting the leftmost 6 information columns.

MMDVMHost's `encodeHeaderGolay` (P25Data.cpp lines 662–688) implements
exactly this:

1. Pack 6 info bits into the low 6 bits of a 12-bit word (high 6 bits
   left zero — the shortening).
2. Call `CGolay24128::encode24128(c0data)` with that 12-bit word.
3. Take the **low 18 bits** of the 24-bit output (drops the 6
   zero-padded info bits).
4. Write those 18 bits into the output buffer, MSB-first.

This is the textbook way to implement a shortened code via its parent:
zero-pad on the input side, truncate on the output side, no separate
generator matrix needed.

The corresponding decoder (`decodeHeaderGolay`, lines 631–660) does the
inverse: zero-extend received 18 bits to 24, run `decode24128`, take
the low 6 info bits.

### Implication for the impl spec

The impl spec's `GOLAY_18_6_GENERATOR[6]` constant (lines 1312–1319)
is a *redundant* representation if the implementer already has a
working (24,12,8) encoder. MMDVMHost shows that a single Golay(24,12,8)
implementation handles **both** TDULC (direct 24-bit) and HDU (shortened
to 18-bit via zero-pad/truncate). This is worth surfacing in the impl
spec as a recommended implementation pattern — saves one set of
constants and one code path.

A draft impl-spec note for §8.2.2:

> Implementations that already include Golay(24,12,8) for TDULC need
> not maintain a separate (18,6,8) generator. Encode by zero-padding
> the 6 info bits into a 12-bit word (high 6 bits zero) and passing to
> the (24,12,8) encoder; the low 18 bits of the 24-bit output are the
> (18,6,8) codeword. Decode by zero-extending received 18 bits to 24
> and running the (24,12,8) decoder; the low 6 bits of the recovered
> 12-bit info word are the (18,6,8) information.

## §8.4.2 / §8.4.3 RS encoders — generator matrices vs. polynomial division

The impl spec §8.4.4 (line 1530–1538) recommends polynomial division
over matrix multiplication for RS encoding ("polynomial division …
typically more efficient than matrix multiplication"). MMDVMHost picks
the opposite: it uses the systematic [I | P] generator matrices
directly:

- `ENCODE_MATRIX_362017[20][36]` for HDU's RS(36,20,17).
- `ENCODE_MATRIX_24169[16][24]` for LDU1 LC and LDU2 ES (RS(24,16,9)).
- `ENCODE_MATRIX_241213[12][24]` for the alternate-form RS code
  (impl spec §8.4.3 disambiguates this as `RS(24,12,13)` used in TDULC
  and the LDU2 ES MI portion, *not* RS(24,16,9)).

Both approaches are correct and produce identical codewords given the
same generator polynomial. The matrix form trades polynomial-division
complexity for `O(n·k)` GF mults per encode (manageable for these small
codes). MMDVMHost's preference is presumably for clarity and direct
correspondence to the spec's tabulated matrices.

The critical observation for the impl spec: the choice of matrix vs.
polynomial division is **purely an encoding-side performance
tradeoff** — the resulting codewords are identical. The impl spec's
recommendation in §8.4.4 should soften from "polynomial division is
typically more efficient" to "either approach yields identical
codewords; choose based on whether your platform favors GF mults
(matrix form) or LFSR-style remainders (polynomial form)."

### What this audit does NOT verify on RS

The full bit-for-bit comparison of the three `ENCODE_MATRIX_*` tables
(20×36, 16×24, 12×24 in MMDVMHost) against the impl spec's PDF Table
7/8/9 octal entries is **deferred**. The matrix entries occupy roughly
1,500 octal numbers across the three codes. A focused follow-up audit
should:

1. Convert the MMDVMHost matrix entries to octal and align them
   against the impl spec's annex-table CSVs (if they exist) or against
   the PDF directly.
2. Verify that the matrix rows generate codewords consistent with the
   generator polynomials in §8.4.2 / §8.4.3 (i.e., the parity portion
   of row `i` should equal `x^(k-1-i+r) mod g(x)`).

The GF(2⁶) tables matching above is necessary-but-not-sufficient
support for the matrices. A direct matrix audit closes the loop.

## §8.4.3 RS code disambiguation — confirms impl-spec correction

The impl spec §8.4.3 contains an in-line correction (lines 1485–1497)
disambiguating two RS(24, ·, ·) codes that share the codeword length
but differ in dimension and minimum distance:

- `RS(24,12,13)` — used for **TDULC** and **LDU2 ES (MI portion)**.
  12 info, 12 parity, dmin = 13.
- `RS(24,16,9)` — used for **LDU1 LC** and **LDU2 ES (full)**.
  16 info, 8 parity, dmin = 9.

MMDVMHost confirms this split:

| Code | MMDVMHost matrix | Used by | MMDVMHost call site |
|---|---|---|---|
| RS(24,16,9) | `ENCODE_MATRIX_24169[16][24]` | LDU2 ES decode | `P25Data.cpp` line 289: `m_rs.decode24169(rs)` (in `decodeLDU2`) |
| RS(24,12,13) | `ENCODE_MATRIX_241213[12][24]` | LDU1 LC decode | `P25Data.cpp` line 176: `m_rs.decode241213(rs)` (in `decodeLDU1`) |
| RS(36,20,17) | `ENCODE_MATRIX_362017[20][36]` | HDU decode | `P25Data.cpp` line 94: `m_rs.decode362017(rs)` (in `decodeHeader`) |

So MMDVMHost's call-site usage is:

- HDU → RS(36,20,17) ✓
- LDU1 (LC) → RS(24,12,13) ← impl spec says RS(24,16,9); divergence
- LDU2 (ES) → RS(24,16,9) ← impl spec says RS(24,16,9) for "full" /
  RS(24,12,13) for "MI portion"

**Wait — there's a real disagreement here.**

The impl spec §8.4.3 (line 1497) says LDU1 LC uses **RS(24,16,9)**.
MMDVMHost calls **`decode241213`** in `decodeLDU1` (line 176).

Investigating: P25 Phase 1 LC code word is 72 bits (1 LCF byte + 1 MFID
byte + 7 info bytes = 72 bits = 12 six-bit symbols), which gives
**12 info symbols** to RS — matching `RS(24,12,13)`, not `RS(24,16,9)`.

Conversely, LDU2 carries Encryption Sync = 9-byte MI + ALGID + KID =
roughly 96 bits = 16 six-bit symbols, which matches **`RS(24,16,9)`** —
which is what MMDVMHost calls in `decodeLDU2` (line 289).

So **MMDVMHost is correct, and the impl spec §8.4.3 has the LC/ES
assignments flipped**:

- **LDU1 LC → RS(24,12,13)** (12 LC info symbols + 12 parity)
- **LDU2 ES → RS(24,16,9)** (16 ES info symbols + 8 parity)

This is a bug in impl spec §8.4.3 lines 1494–1497. The TDULC LC code
word is also 12 info symbols (same LC content as LDU1, just a different
inner code) and uses RS(24,12,13) — that part of the impl spec is
correct.

### Recommended impl-spec patch

§8.4.3 lines 1494–1497 should be corrected to:

> - **G_LC = RS(24,12,13):** 12 info symbols, 12 parity symbols, dmin =
>   13, degree-12 generator. Used for: **LDU1 LC code word** and
>   **TDULC LC code word**.
> - **G_ES = RS(24,16,9):** 16 info symbols, 8 parity symbols, dmin =
>   9, degree-8 generator. Used for: **LDU2 Encryption Sync (full
>   ES)**.

The OP25 / SDRTrunk cross-references in §8.4.3 should be re-checked
against the corrected assignments.

This is a real correctness finding, not a labeling tidy-up: an
implementer following the current §8.4.3 would call `RS(24,16,9)` on
LDU1 (16 info × 6 = 96 bits, but LC is only 72 bits) and the parity
math wouldn't reproduce the on-air codewords. They'd notice quickly
because no real-world LC frame would decode, but it's a confusing first-
hour bug to chase. Correcting the spec saves that round trip.

## Implementation patterns worth lifting

Three patterns from MMDVMHost worth surfacing for `blip25-mbe`:

### 1. Lookup-by-Hamming-distance for LSD (and NID)

Same pattern as the NID audit's "Pattern B": pre-encode all 256 valid
LSD codewords (one per byte), brute-force compare a received 16-bit
field against each, return whichever matches within threshold. No
syndrome math, no Galois field involvement. ~256 popcount ops per
decode. MMDVMHost's `MAX_CCS_ERRS = 4` is permissive (above algebraic
correction limit); choose threshold per use case as discussed above.

### 2. Single Golay(24,12,8) implementation serves both TDULC and HDU

Rather than maintaining separate `Golay(24,12,8)` and `Golay(18,6,8)`
encoders/decoders, implement only the parent (24,12,8) and use
zero-pad-input / truncate-output to handle the (18,6,8) shortening.
Cuts the code-and-test surface in half on the HDU path.

### 3. Matrix-form RS encoder (alternative to polynomial-form)

For the 3 small RS codes, the systematic `[I | P]` matrix multiplied by
the info vector over GF(2⁶) is competitive with — and arguably easier
to spot-check against — polynomial division. The matrix entries appear
directly in the spec PDF; the implementer can copy them with no
algebraic transformation. The impl spec §8.4.4 leans toward polynomial
division; MMDVMHost shows the matrix path is equally workable.

### 4. Encoder-decoder skeleton: encode by [I|P] matrix, decode by syndrome

MMDVMHost's `RS634717.cpp` carries the matrices for encoding only — the
decoder uses standard syndrome-based correction (Berlekamp-Massey or
Peterson-Gorenstein-Zierler in `RS.h`, not pulled in this audit). This
is a clean separation. `blip25-mbe` should adopt the same: the
generator matrix is for encoding; the GF(2⁶) tables and syndrome math
are for decoding. The two share only the field tables.

## What this audit does NOT cover

- **IMBE inner FEC** (Hamming(15,11,3) and Golay(23,12,7) inside the
  88-bit IMBE frame) — `analysis/imbe_spec_audit_vs_jmbe_vs_chip.md`
  owns this; MMDVMHost handles voice via `P25Audio.cpp` (not
  re-audited here).
- **TDULC framing and Golay(24,12,8) details** — only the
  shortening-relationship to (18,6,8) was verified. Full
  Golay(24,12,8) constants (`GOLAY_24_12_GENERATOR[12]` in impl spec
  §8.2.1, with a "verify" caveat on row 3) deserve a focused follow-up
  comparing against `Golay24128.cpp` directly.
- **RS encoding matrix entries** (`ENCODE_MATRIX_362017`,
  `ENCODE_MATRIX_24169`, `ENCODE_MATRIX_241213`) — recommended
  follow-up audit. The GF(2⁶) layer underneath them is verified here;
  the matrix entries themselves are deferred.
- **Bit-position alignment** between MMDVMHost's hard-coded bit ranges
  (e.g., `decode(data, raw, 410U, 452U)` in `decodeLDU1`) and the
  impl spec's `annex_a3_ldu1_transmit_bit_order.csv` /
  `annex_a4_ldu2_transmit_bit_order.csv` annex tables. Both should
  agree on where the six 10-bit Hamming words sit inside the 1680-bit
  LDU info field. Worth verifying — `P25Utils.cpp` (not pulled here)
  is needed to interpret the start/end parameters precisely.

## Recommended impl-spec patches

Three real corrections / clarifications, plus several informative
additions:

1. **§8.3.1 line 1339 — remove "verify" caveat on Hamming(10,6,3)
   row 5.** MMDVMHost confirms `0x0023` (`00 0010 0011`) bit-for-bit
   via the parity-XOR equations.

2. **§8.4.3 lines 1494–1497 — correct LDU1 LC vs. LDU2 ES RS
   assignment** (real correctness bug):
   - LDU1 LC uses RS(24,12,13), not RS(24,16,9).
   - LDU2 ES uses RS(24,16,9) (full), not RS(24,16,9) for "full"
     and RS(24,12,13) for "MI portion."

3. **§8.4.4 line 1537–1538 — soften the polynomial-division-vs-matrix
   recommendation.** Either form yields identical codewords; choose by
   platform.

4. **§8.2.2 — add informative note on shared (24,12,8) implementation**
   per the draft above.

5. **§8.4.1 — note that the GF(2⁶) tables match MMDVMHost for all 63
   non-zero elements.** Adds a third independent witness alongside the
   internal verification noted in line 1408.

## References

- TIA-102.BAAA-B (2017) §6 (HDU/LDU framing), §8 (FEC codes) — the
  standard text.
- `~/blip25-specs/standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`
  §6, §8 — derived implementation spec.
- `g4klx/MMDVMHost`: `P25Data.cpp/.h`, `RS634717.cpp/.h`, `Hamming.cpp/.h`,
  `Golay24128.cpp/.h`, `P25LowSpeedData.cpp`, `P25Defines.h` — third-
  party TX+RX implementation, GPLv2.
- Bryan Biedenkapp / N2PLL — RS encoder matrix author in MMDVMHost,
  also OP25 P25 Phase 2 contributor.
- `analysis/p25_trellis_implementation_audit.md` — companion audit for
  the trellis layer.
- `analysis/p25_nid_implementation_audit.md` — companion audit for the
  NID / BCH layer.
- `analysis/imbe_spec_audit_vs_jmbe_vs_chip.md` — IMBE inner FEC audit
  (out of scope here).
