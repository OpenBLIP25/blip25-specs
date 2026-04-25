# IMBE FEC / Interleave / PN — Spec Audit, JMBE Comparison, Chip Boundary

**Triggered by:** `gap_reports/0021_chip_bit_fec_convention_mismatch.md`
(implementer observes deterministic 3-Golay-error + 1-Hamming-error pattern
per frame when their spec-derived decoder runs on DVSI `chip.bit`; JMBE
decodes the same bytes and produces plausible audio).

**Scope:** spec-author audit of the TIA-102.BABA-A derived tables and
constants in `P25_Vocoder_Implementation_Spec.md` §§1.5.1, 1.5.2, 1.6,
1.7 against the authoritative PDF (§§7.1, 7.3, 7.4, 7.5, Annex H), plus
a structural comparison against JMBE's open-source IMBE decoder.

## TL;DR

The derived spec is correct. The implementer's derivation of Golay,
Hamming, PN, and Annex H matches the PDF bit-for-bit. JMBE uses the same
tables and the same wire-to-vector bit mapping. There is no spec-level
convention ambiguity causing the gap report's 3-Golay-error pattern.

Option (a), (b), (c), (d) from the gap report as written are all **ruled
out at the spec-derivation level**. The remaining explanation is option
(a) relocated: a subtle bug downstream of FEC (most likely in bit-
deprioritization §1.4.5 or dequantize §1.8) in the implementer's
current code path. See "Recommended next step" below.

## What I verified

### §1.5.1 Golay(23, 12) generator matrix — matches PDF

PDF BABA-A §7.3 gives the 12×23 generator matrix `gG` in systematic form.
The derived spec's `GOLAY_23_12_GEN` constant (12 u32 entries) matches
that matrix row-for-row when each row is read as 12 identity bits +
11 parity bits. Verified by comparing every row to the PDF text:

| Row | PDF identity + parity          | Derived `GOLAY_23_12_GEN` | Match |
|----:|--------------------------------|---------------------------|:-----:|
|   0 | `100000000000` \| `11000111010`  | `0x40063A` = same         | ✓ |
|   1 | `010000000000` \| `01100011101`  | `0x20031D` = same         | ✓ |
|   2 | `001000000000` \| `11110110100`  | `0x1007B4` = same         | ✓ |
|   3 | `000100000000` \| `01111011010`  | `0x0803DA` = same         | ✓ |
|   4 | `000010000000` \| `00111101101`  | `0x0401ED` = same         | ✓ |
|   5 | `000001000000` \| `11011001100`  | `0x0206CC` = same         | ✓ |
|   6 | `000000100000` \| `01101100110`  | `0x010366` = same         | ✓ |
|   7 | `000000010000` \| `00110110011`  | `0x0081B3` = same         | ✓ |
|   8 | `000000001000` \| `11011100011`  | `0x0046E3` = same         | ✓ |
|   9 | `000000000100` \| `10101001011`  | `0x00254B` = same         | ✓ |
|  10 | `000000000010` \| `10010011111`  | `0x00149F` = same         | ✓ |
|  11 | `000000000001` \| `10001110101`  | `0x000C75` = same         | ✓ |

**JMBE cross-check.** JMBE's `Golay23.CHECKSUMS` array holds the
11-bit parity portion of each systematic row:

```
0x63A, 0x31D, 0x7B4, 0x3DA, 0x1ED, 0x6CC, 0x366, 0x1B3, 0x6E3, 0x54B, 0x49F, 0x475
```

These match the parity columns of our `GOLAY_23_12_GEN` exactly (entries
0-11 of the array — entries 12-22 are unit syndromes for parity bits).

### §1.5.2 Hamming(15, 11) parity matrix — matches PDF

PDF §7.3 shows the 11×15 generator `gH` in systematic form. The derived
spec's `HAMMING_15_11_PARITY` table holds the 4-bit parity rows:
`{0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x7, 0x6, 0x5, 0x3}`. Match to PDF:
row 0 parity `1111` = `0xF`, row 1 `1110` = `0xE`, ..., row 10 `0011` = `0x3`.
All 11 rows agree.

### §1.6 PN sequence and mask ranges — matches PDF

PDF §7.4 Eqs. 84–93 define `p_r(0) = 16·û₀` and the LCG recursion
`p_r(n) = (173·p_r(n-1) + 13849) mod 65536`. The derived spec's
`imbe_pn_sequence_fullrate` reference C and the implementer's
`pn_sequence` Rust both implement this exactly. Each mask bit is the
MSB of the corresponding `p_r(n)` — Eqs. 86–93.

The PN index ranges match Eqs. 87–92:
- m̂₁ ← p_r(1..23)
- m̂₂ ← p_r(24..46)
- m̂₃ ← p_r(47..69)
- m̂₄ ← p_r(70..84)
- m̂₅ ← p_r(85..99)
- m̂₆ ← p_r(100..114)
- m̂₀ = m̂₇ = 0 (Eqs. 86, 93)

**Known convention pitfall (already disambiguated).** The row-vector
bracket notation in Eqs. 87–92 lists `p_r(start)` first; per §7.3's
"leftmost is MSB" rule this is the first-transmitted bit, which in
codeword storage lands at bit `len-1`, not bit 0. The derived spec's
§1.6 reference C uses `<< (length - 1 - k)` (correct), and
`analysis/vocoder_decode_disambiguations.md §10` ("PN Mask Alignment Is
Transmission-Order, Not Storage-Order") captures the earlier naive
"element-k-at-bit-k" mis-reading. Implementer's Rust follows the correct
transmission-order packing.

**JMBE cross-check.** JMBE's `IMBEFrame.derandomize()` flips the frame
bit at `position (x + 23)` based on `p_r(x+1)` for `x ∈ 0..114`. After
JMBE's deinterleave, position `x + 23` is c1's `x+1`-th bit from the
start of c1, where c1 position 0 = MSB per our spec convention. So
JMBE maps p_r(1) → c1 MSB, p_r(2) → c1 bit 21, ..., p_r(23) → c1 LSB.
Identical to the spec and to the implementer's transmission-order
packing.

### §1.7 Annex H interleave — CSV matches PDF

PDF Annex H, page 102, is the 72-symbol × 2-bits/symbol table. I
cross-checked every row range against the derived CSV at
`standards/TIA-102.BABA-A/annex_tables/annex_h_interleave.csv`:
- All 144 (vector, bit-index) entries accounted for.
- The OCR fix at symbol 51 (PDF renders as "c9(5)"; correct value is
  "c0(5)") is documented in the CSV header and applied.

**JMBE cross-check.** JMBE's `IMBEInterleave.DEINTERLEAVE` array is a
flat 144-element permutation `{0, 24, 48, 72, 96, 120, 25, 1, ...}`. I
derived the same permutation from the CSV + the rule "symbol k's high
bit is stream position 2k; low bit is stream position 2k+1" + "c_j bit
N-1 is the MSB and sits at JMBE position `offset_j + 0`." The first 8
entries derived from the CSV (`{0, 24, 48, 72, 96, 120, 25, 1, ...}`)
match JMBE exactly; the pattern continues for all 144 entries.

### §1.4 Bit prioritization scan — matches PDF

PDF §7.1 describes the priority scan of b̂₃..b̂_{L+1} and the specific
insertion order into u₀..u₇. The derived spec's §1.4 walks the same
algorithm. I did not verify every opcode of the dequantize tables (out
of scope for this audit) but the prioritization structure itself is
consistent.

## What that means for gap 0021

Re-reading the gap's four candidate causes:

**(a) Our implementation has a subtle interleave/layout bug.** At the
spec-table level this is ruled out: tables match the PDF. If the bug
exists, it is in the implementer's Rust code below the FEC layer —
bit-deprioritization (`priority.rs`), dequantize (`dequantize.rs`),
or a mask-alignment detail in the encode direction that doesn't surface
in self-roundtrip. This is still the most plausible option, but it
moves from "spec error" to "implementation error in a layer the spec
has not fully disambiguated for that particular direction."

**(b) Chip uses non-spec Golay.** Ruled out. JMBE's `Golay23` uses the
spec-compliant generator matrix (matches our `GOLAY_23_12_GEN`) and
decodes `chip.bit` to PESQ 2.66. A non-spec Golay would cause JMBE to
fail the same way the implementer fails.

**(c) Chip emits pre-air-interface raw form.** Ruled out on the same
grounds — JMBE's air-interface decoder succeeds on the bits.

**(d) Ambiguity in §8.1 / Annex H.** Ruled out for the specific items
audited. The table and row-vector convention are unambiguous in the
PDF and both our spec and JMBE resolve them the same way.

## What I did NOT fully verify

- **§1.4 priority scan details** — I confirmed the *structure* matches
  the PDF (b̂₀ MSBs into u₀[11..6], then alternating insertion into
  u₀..u₇ as specified). I did not walk every bit placement byte-by-byte.
- **§1.8 dequantize tables** — out of scope.
- **§1.9+ enhancement / smoothing / synthesis** — out of scope.

These are where the remaining candidate bug likely lives.

## The surprising empirical fact

Since JMBE and the spec-derived pipeline should be bit-identical at
every step (deinterleave → Golay c₀ → PN derive from û₀ → demod →
Golay/Hamming c₁..c₆ → bit-deprioritization → dequantize → synth),
**they must produce the same u₀..u₇ for the same chip.bit frames**.
That is: both decode the same (non-codeword) c₀ = 0x10a881 via
3-error Golay correction to u₀ = 0x015, both generate the same PN
from that (wrong) u₀, both demod c₁..c₆ against the same (uncorrelated)
masks, both extract the same u₁..u₇ values.

If that's true, the divergence into "PESQ 1.32" (implementer) vs
"PESQ 2.66" (JMBE) happens in the post-FEC stage. The gap report's
"recovered params themselves are wrong" finding is consistent with this:
the raw params from chip.bit via spec-compliant FEC are noise (because
c₀ has 3+ true errors, PN inverts wrong), but two implementations of
the post-FEC pipeline can interpret that noise differently based on
saturation, voicing-decision smoothing, or synthesis-window placement.

JMBE's PESQ 2.66 is surprising. Note that:
- Chip's *own* decoder on `chip.bit` yields PESQ 2.39 (per the gap
  report's measurements table) — so even the canonical decoder
  produces degraded audio from these bits.
- JMBE at 2.66 is slightly better than chip-self-decode, suggesting
  JMBE's synthesis is more forgiving of the noise, not that JMBE
  is extracting "correct" params.
- Implementer at 1.32 is significantly below chip-self-decode,
  suggesting their post-FEC pipeline has a specific bug that amplifies
  the FEC-layer noise into mute-triggering territory.

## Recommended next step (for the implementer)

Bypass the full decode pipeline and dump just u₀..u₇ from a single
chip.bit frame. Run the same frame through JMBE (instrument
`IMBEFrame.decode()` to print the 88 post-FEC bits) and compare.

Expected outcomes:
- **u_i match byte-for-byte**: confirms FEC/PN/interleave equivalence
  (as this audit predicts). Bug is in `priority.rs`, `dequantize.rs`,
  or `synthesize_frame`. Next step: A/B the dequantize output against
  JMBE's corresponding fields (gain, spectral amplitudes, voicing).
- **u_i differ**: the pipelines disagree somewhere in FEC/PN/
  interleave despite matching the derived spec. That would be a real
  disambiguation finding. Most likely site would then be:
  - A subtle off-by-one in how the MASK_RANGE packing interacts with
    the deinterleave output (even though both seem correct in
    isolation).
  - A difference in how the implementer's code iterates the PN
    sequence vs how JMBE does (JMBE uses `prX = (173*prX + 13849) %
    65536` with Java int arithmetic; implementer uses `wrapping_mul`
    + `& 0xFFFF`. These should be equivalent for non-negative u16
    values but worth double-checking).

Either way: the spec itself is not the gap. This is an
implementation-level investigation.

## Cross-references

- PDF: `standards/TIA-102.BABA-A/84_TIA 102 BABA A Final Published
  Document 2014 02 25.pdf`, §§7.1 (p. 34), 7.3 (p. 36), 7.4 (p. 37),
  7.5 (p. 39), Annex H (p. 102).
- Derived spec: `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md`
  §§1.4, 1.5.1, 1.5.2, 1.6, 1.7.
- CSV: `standards/TIA-102.BABA-A/annex_tables/annex_h_interleave.csv`.
- Earlier disambiguation on PN alignment: `analysis/vocoder_decode_disambiguations.md` §10.
- JMBE source: `~/jmbe/codec/src/main/java/jmbe/`
  (`edac/Golay23.java`, `codec/imbe/IMBEInterleave.java`,
  `codec/imbe/IMBEFrame.java`, `binary/BinaryFrame.java`).
- Implementer's code: `~/blip25-mbe/crates/blip25-mbe/src/fec.rs`,
  `src/p25_fullrate/fec.rs`, `src/p25_fullrate/frame.rs`.
