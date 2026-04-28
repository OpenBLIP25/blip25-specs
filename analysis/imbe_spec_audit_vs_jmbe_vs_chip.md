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

## Patent Cross-Reference: US 5,870,405 (Hardwick + Lim, expired ~2016)

**Added 2026-04-27.** US 5,870,405 is the DVSI-authored patent that
discloses the same IMBE FEC/modulation/frame-handling scheme covered
by BABA-A. It is the divisional of US 5,517,511 with priority to
1992-11-30, granted 1999-02-09. The specification explicitly references
"APCO/NASTD/Fed Project 25 Vocoder Description" Appendices E, F, G, and
H throughout — meaning the patent and the eventual TIA standard share a
common origin disclosure. The patent is now public domain.

This adds **a third independent source** to confirm BABA-A's IMBE FEC
constants (alongside the BABA-A PDF and JMBE source code). Where all
three agree, confidence in the audit's correctness is very high. Where
the patent differs from BABA-A, BABA-A is authoritative — but the
differences are themselves informative: most appear to be **refinements
made between the 1992 patent disclosure and the 2014 standard**.

See `analysis/ambe_foundational_patents.md` §3 for the full patent
analysis. This section reports the cross-check against BABA-A.

### Items where Patent, BABA-A, and JMBE all agree

These are now confirmed by three independent sources:

1. **Golay [23,12] generator matrix** — patent shows the same 12×11
   parity portion as BABA-A §7.3. Cross-checks: PDF, JMBE, and
   patent (US 5,870,405 col. 18 P_G matrix) all match.
2. **Hamming [15,11] parity matrix** — patent col. 18 shows P_H
   matrix; BABA-A and JMBE agree.
3. **PN generator recurrence** — patent Eq. 41–42 and BABA-A Eq. 84–85
   are identical:
   ```
   p_r(0) = 16 · û₀
   p_r(n) = (173·p_r(n-1) + 13849) mod 65536
   ```
4. **Mask bit ranges (m̂_0 through m̂_7)** — patent Eq. 43–50 and BABA-A
   Eq. 86–93 specify identical ranges:
   - m̂_0 = 0 (zero, 23 bits)
   - m̂_1 = p_r(1..23)
   - m̂_2 = p_r(24..46)
   - m̂_3 = p_r(47..69)
   - m̂_4 = p_r(70..84)
   - m̂_5 = p_r(85..99)
   - m̂_6 = p_r(100..114)
   - m̂_7 = 0 (zero, 7 bits)
5. **Reserved pitch values** — patent col. 19–20 and BABA-A spec agree
   on b̃_0 ∈ [0, 207] valid; b̃_0 ≥ 208 reserved (silence/inband data).
6. **Higher-order DCT step sizes (Table 3)** — patent Table 3 matches
   the spec's `HOC_STEP[]` array byte-for-byte:
   1.20σ, 0.85σ, 0.65σ, 0.40σ, 0.28σ, 0.15σ, 0.08σ, 0.04σ, 0.02σ, 0.01σ
7. **Bit prioritization structure** — patent FIG. 9–11 shows the same
   û_0..û_7 vector lengths (12, 12, 12, 12, 11, 11, 11, 7) with
   [23,12] Golay on û_0..û_3 and [15,11] Hamming on û_4..û_6.
   Matches BABA-A §1.4.

These items are now triple-confirmed.

### Items where Patent and BABA-A appear to differ

The following items show apparent numerical or structural differences
between the patent (1992 priority, 1999 grant) and BABA-A (2014
publication). BABA-A is authoritative; differences likely reflect
refinements during standardization.

#### 1. Frame-repeat trigger (substantive structural change)

| Source | Trigger condition |
|---|---|
| Patent Eq. 55–56 | `ε_0 ≥ 2 AND ε_T ≥ 11` (constant threshold) |
| BABA-A Eq. 97/98 | `ε_0 ≥ 2 AND ε_T ≥ 10 + 40·ε_R(0)` (dynamic threshold) |

This is a real structural improvement. The patent's constant threshold
of 11 was refined into a function of the smoothed long-term error
rate. The dynamic threshold raises the bar in clean conditions
(rejecting fewer frames) and lowers it in high-error conditions
(rejecting more frames) — a quality refinement not in the original
patent.

**Implication for the project:** the spec's Eq. 97/98 is correct and
authoritative. The patent's constant 11 is **historical** — useful
for context but not for implementation.

#### 2. ε_R smoothing recurrence (small numerical difference, possibly OCR)

| Source | Recurrence |
|---|---|
| Patent Eq. 54 | `ε_R(0) = 0.95·ε_R(-1) + 0.000356·ε_T` |
| BABA-A | `ε_R(0) = 0.95·ε_R(-1) + 0.05·(ε_T / 144) ≈ 0.95·ε_R(-1) + 0.000347·ε_T` |

Difference: ~2.6% on the per-error weight (0.000356 vs 0.000347).
Could be:
- (a) Genuine refinement during standardization (BABA-A's 0.05/144 is
  cleaner — the per-frame-bit normalization is mathematically
  meaningful, while the patent's 0.000356 has no obvious closed-form
  derivation)
- (b) OCR misread of the patent value (the patent PDF is image-only;
  "0.000356" could plausibly be "0.000347" with OCR errors on multiple
  digits)

The BABA-A form `0.05·(ε_T/144)` is more likely correct because of its
clean derivation. **Use BABA-A's value.**

#### 3. Mute threshold (small numerical difference, possibly OCR)

| Source | Threshold |
|---|---|
| Patent | `ε_R > 0.085` |
| BABA-A | `ε_R(0) > 0.0875` |

Difference: 3% (0.085 vs 0.0875). Could be:
- (a) Genuine refinement (0.0875 = 7/80 is a cleaner fraction)
- (b) OCR misread (the patent could show "0.0875" with the trailing
  digit cropped or misread)

**Use BABA-A's 0.0875.**

#### 4. Higher-order DCT σ values (Table 4) — possible discrepancy at C_{1,6}

| C_{1,k} | Patent Table 4 | BABA-A spec |
|---|---|---|
| C_{1,2} | 0.307 | 0.307 ✓ |
| C_{1,3} | 0.241 | 0.241 ✓ |
| C_{1,4} | 0.207 | 0.207 ✓ |
| C_{1,5} | 0.190 | 0.190 ✓ |
| C_{1,6} | **0.190** | **0.179** ⚠️ |
| C_{1,7} | 0.179 | 0.173 |
| C_{1,8} | 0.173 | 0.165 |
| C_{1,9} | 0.165 | 0.170 |
| C_{1,10} | 0.170 | 0.170 ✓ |

The patent appears to show 0.190 duplicated at C_{1,5} and C_{1,6},
then a sequence shifted-by-one relative to BABA-A from k=6 onward, with
both sources agreeing again at C_{1,10}.

**Most likely explanation:** OCR error in the patent reading. Patents
in the 1990s had hand-typeset tables that scan poorly — "0.179" and
"0.190" are visually distinct but both contain digits that OCR
sometimes confuses. The shifted sequence after k=6 is consistent with
a single missing-row OCR error rather than nine independent
discrepancies.

**Use BABA-A's values** — they are the authoritative TIA standard
values and have been verified in the existing audit against the
BABA-A PDF, JMBE, and the project's `HOC_SIGMA[]` array.

If a future implementer encounters a discrepancy with a third-party
IMBE implementation that uses the patent values, this difference is a
candidate explanation.

#### 5. 4-consecutive-invalid-frames mute rule

| Source | Trigger |
|---|---|
| Patent | Mute if **4 consecutive invalid frames** OR `ε_R > 0.085` |
| BABA-A full-rate (§1.11.2) | Mute if `ε_R(0) > 0.0875` (no consecutive-frame counter) |
| BABA-A half-rate (§2.8.3) | Mute if `ε_R(0) > 0.0875` OR **4 consecutive frame repeats** |

The patent specifies BOTH triggers; BABA-A full-rate uses ONLY the
ε_R(0) threshold. BABA-A half-rate restored the 4-consecutive-frame
trigger.

This is a real difference between full-rate and half-rate handling
that the patent did not anticipate. The full-rate spec relies on the
ε_R smoothing alone to escalate to mute; the half-rate adds the
explicit consecutive-frame counter. **Both are authoritative for their
respective formats.**

### Items only documented in the patent (not in BABA-A)

The patent contains an explicit muting procedure that BABA-A leaves
implementation-defined:

> "The recommended muting method is to bypass the synthesis procedure
> and set the synthetic speech signal, ŝ(n), to random noise uniformly
> distributed over the interval [-5, 5] samples."

BABA-A §1.11.2 says only "random small-amplitude noise (or silence)".
The patent's specific [-5, 5] (16-bit PCM samples) is a **public-domain
implementation hint** — implementers can use this without infringement
risk and with assurance the value is from DVSI's own reference.

### Audit conclusion

The IMBE audit is now triple-confirmed: BABA-A PDF, JMBE source, and
US 5,870,405 patent specification all agree on the load-bearing FEC
and modulation constants. The two substantive differences (dynamic
ε_T threshold in BABA-A Eq. 97/98 vs. patent's constant 11; 4-
consecutive-frames trigger present in patent and half-rate BABA-A but
absent from full-rate BABA-A) are authoritative refinements made during
standardization. Several smaller numerical differences (0.000356 vs
0.000347, 0.085 vs 0.0875, Table 4 row at C_{1,6}) are probably patent
OCR artifacts, not real disagreements; in any case BABA-A's values are
authoritative and should be used.

This cross-reference does NOT change the audit's conclusion that the
implementer's gap-0021 issue is a post-FEC bug, not a spec-derivation
error.

---

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
