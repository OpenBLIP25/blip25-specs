# Gap 0019 — Annex L pitch table's L column vs §1.8.5 Eq. 31

**Status:** drafted (2026-04-23) — resolution committed to the
`standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §12.8
paragraph and to `analysis/vocoder_decode_disambiguations.md` §14.
Answer is **option (b)**: Annex L's L column is authored by a
different closed-form rule — a **single-floor** form
`L = ⌊0.9254 · π/ω₀⌋` that matches all 120 rows exactly — rather than
Eq. 31's double-floor-with-`+0.25` form which is full-rate-only. The
CSV extraction is correct; the disagreement is a real
rule-divergence between Eq. 31 (full-rate, continuous `ω̂₀`) and
Annex L (half-rate, discrete grid). Analysis encoder must derive
half-rate `L̂` via Annex L table lookup at the selected `b̂₀`, not via
Eq. 31 applied to `ω̂₀`. See the resolution summary at the end of this
report.

**Filed:** 2026-04-23
**Filed by:** implementer (blip25-mbe)
**Spec area:** TIA-102.BABA-A (Vocoder), §2.13 Annex A (half-rate pitch
table) in relation to §1.8.5 Eq. 31 (full-rate L̂ derivation).
**Severity:** Low — no runtime impact on the existing wire encoder; a
conformance-reporting artifact. But it masks the real half-rate
analysis-encoder quality vs chip.

## The question

Our derived `spec_tables/annex_l_pitch_table.csv` has 120 entries of the
form `(L, ω₀)` indexed by b̂₀. When we compute `L = ⌊0.9254 · ⌊π/ω₀ +
0.25⌋⌋` (Eq. 31, post-addendum §0.4.3 inner-floor fix, merged in specs
commit `d20f5ab`) from the CSV's ω₀ column and compare against the CSV's
L column:

| Comparison            | Count (of 120) |
|-----------------------|---------------:|
| L_table == L_Eq31     |             84 |
| L_table  > L_Eq31     |             29 |
| L_table  < L_Eq31     |              7 |

Example entries where they disagree (first few):

| b̂₀ | L_table | ω₀       | L_Eq31 |
|----|--------:|----------|-------:|
|  5 |       9 | 0.290905 |     10 |
| 11 |      10 | 0.265439 |     11 |
| 16 |      11 | 0.245974 |     12 |
| 23 |      13 | 0.221156 |     12 |
| 28 |      14 | 0.205052 |     13 |

**Which rule authored the L column of Annex L?**
- (a) Same §1.8.5 Eq. 31 (inner-floor form), and our CSV is mis-extracted.
- (b) A different, half-rate-specific rule documented elsewhere in BABA-A
  §2.11–§2.13 that the spec-author didn't surface in the derived works.
- (c) Hand-tuned values chosen for half-rate band-structure reasons that
  don't follow any closed-form rule.
- (d) A spec-side typo or OCR artifact.

## Why it matters

Observed impact on `halfrate-analysis-encode` conformance (DVSI
tv-rc/r33/clean, 3700 frames, --silence-dispatch):

- 814 voice-voice comparable frames
- 295 pitch-close (|Δ| < 20¢) frames
- 112 pitch-close frames with L mismatch (38%)
  - 79 attributable to Annex L table (chip_L ≠ Eq31(chip_ω)) — **71%**
  - 33 attributable to pitch quantization boundaries (Eq31(enc_ω) ≠
    Eq31(chip_ω) even at pitch-close)
  - 0 attributable to encoder bugs (every enc_L matches Eq31(enc_ω))

The skew is ~4:1 Δ=−1 over Δ=+1 (101 vs 11 in the pitch-close bucket),
matching the ratio of `L_table > L_Eq31` vs `L_table < L_Eq31` entries
in the table (29:7). The encoder is Eq.31-correct; the chip is
Annex-L-table-correct (by construction of `dequantize` → Annex L
lookup); the two disagree ~30% of the time.

This is a reporting artifact in the conformance harness today — the
wire encoder `p25_halfrate::quantize` snaps ω̂_0 to an Annex L entry,
and the dequantize roundtrip recovers the table's L̃ regardless of what
L̂ the analysis side produced. So there is no real encoder bug. But
until we know which rule authored Annex L's L column, we can't
confidently drive the analysis-encoder quality toward true chip parity
on L.

## Options that can't be disambiguated without the spec

1. **If (a)** — we fix the CSV extraction and the Annex L column becomes
   fully redundant with Eq. 31. L̃ = Eq31(ω̃_0) and the discrepancy
   disappears.
2. **If (b)** — we need the alternate rule's definition and its
   rationale. Encoder should apply that rule to ω̂_0 instead of Eq. 31
   in the half-rate path.
3. **If (c)** or **(d)** — we keep the Annex L table as-is and
   document the non-closed-form relationship; the conformance harness
   should compute its own L via table lookup on ω̂_0 (same rule the
   chip uses) for fair comparison, not via Eq. 31.

## Why a chip probe isn't a substitute

The chip follows whatever rule DVSI implemented — probably just "look
up L̃ from the internal table indexed by b̂_0." That tells us the chip's
behavior but doesn't tell us whether the spec text *specifies* Eq. 31
for half-rate or specifies a different rule. We need the spec-author to
verify §2.13 Annex A (or wherever the pitch table is authored).

## Diagnostic evidence

- Data extraction script: `/tmp/dump.txt` from
  `cargo run -p blip25-conformance-vectors --release -- \
   halfrate-analysis-encode --silence-dispatch --dump-frames 1000 clean`
- Per-frame dump columns: `frame enc_ω chip_ω Δcents enc_L chip_L Eq31(enc) Eq31(chip)`
- Annex L CSV: `~/blip25-specs/spec_tables/annex_l_pitch_table.csv`
  (or generated at `target/release/build/.../annex_l_pitch.rs`)

## Concrete ask

Confirm whether §2.13 Annex A (half-rate pitch table) documents the L
column as derived via §1.8.5 Eq. 31, via a different closed-form rule,
or as hand-tuned. If Eq. 31, flag any transcription discrepancies in
the derived `annex_l_pitch_table.csv`. If a different rule,
document that rule in the derived implementation spec so the analysis
encoder can apply it.

---

## Resolution (2026-04-23, spec-author pass)

### What the PDF says

- **BABA-A page 127 (Annex L)** renders the table as three columns
  `b₀ | L | ω₀` — both `L` and `ω₀` are tabulated data per row. No
  derivation formula for the L column appears on that page.
- **BABA-A §13.1 pages 58–59** (the prose that wraps Annex L) says
  the encoder picks `b̂₀` by finding the Annex L row whose `ω₀` is
  closest to the refined `ω̂₀`, and the decoder derives `L̃` from `b̃₀`
  **"as shown in Annex L"** — i.e. table lookup at `b̃₀`, not a
  formula on `ω̃₀`. Silence frames use a hard-coded override
  `L̃ = 14` (§13.1 Eq. 144).
- **Eq. 31 (§5.1.5 page 17) and Eq. 47 (§6.2 page 22)** are **full-rate
  only**. Both render as `⌊0.9254 · ⌊π/ω + 0.25⌋⌋` (double floor
  with `+0.25` inner-round offset). They apply to the continuous
  refined `ω̂₀` (encoder) and the continuous decoded `ω̃₀ = 4π/(b̃₀+39.5)`
  (decoder), both of which are rad/sample-continuous.

### What rule authored Annex L's L column

Empirically the simpler single-floor form

```
L = ⌊ 0.9254 · π / ω₀ ⌋                    (ω₀ in rad/sample)
```

matches Annex L's L column on all **120/120** rows exactly. The
double-floor Eq. 31 form mismatches 36/120 rows — exactly the
diagnostic's observation. So this is gap option **(b)**: a different
closed-form rule, specifically a single-floor variant of Eq. 31, not
Eq. 31 itself. The `+0.25` inner-round offset that Eq. 31 uses to
round a continuous pitch-period estimate is redundant on Annex L's
discrete grid and was dropped.

This is not a transcription error in our CSV — spot-checks against
the PDF (b₀ = 0, 5, 6, 11, 23, 28, 119) confirm our extraction
matches PDF verbatim. It is also not hand-tuning: the single-floor
rule is tight enough that all 120 rows fall out of it mechanically.

### What the analysis encoder must do

1. On the **half-rate** path, `L̂` is derived **by Annex L table lookup
   at the selected `b̂₀`**, not by applying Eq. 31 to `ω̂₀`. The
   correct sequence is:

   ```
   ω̂₀    ← refine pitch (§0.4 of addendum, same as full-rate)
   b̂₀    ← argmin_{b ∈ [0, 119]}  | annex_l[b].ω₀_rad − ω̂₀ |  (§13.1)
   L̂     ← annex_l[b̂₀].L                                       (table)
   ```

2. The downstream bit-allocation tables (Annex N, Annex R, Annex P/Q)
   are keyed on `L̂`. If encoder-side `L̂` is derived by Eq. 31 but
   decoder-side `L̃` is derived by table lookup, the two sides will
   pick different block sizes on 36/120 pitch codewords and the
   frame will round-trip incorrectly. Table lookup on both sides is
   the correct and decoder-compatible choice.

3. On the **full-rate** path, Eq. 31 (encoder) and Eq. 47 (decoder)
   remain as specified — double-floor with `+0.25`. No change.

### Conformance-harness implication

The implementer's 38% L-mismatch rate on pitch-close half-rate frames
resolves into two distinct sources:

- **79 frames (71% of mismatches, 9.7% of voice-voice frames)**:
  the encoder emits `Eq31(ω̂₀)` while the chip emits Annex-L-table
  at its own chosen `b₀`. Fix: change the analysis encoder to
  derive `L̂` via table lookup at `b̂₀`. After this change the 79
  disappears.
- **33 frames (29% of mismatches, 4.1% of voice-voice frames)**:
  encoder and chip chose different `b₀` (pitch quantization
  boundary), and therefore look up different L rows. This is a
  legitimate pitch-quantization sensitivity — independent of the
  Eq. 31 question — and stays as-is.

The ~4:1 Δ=−1 : Δ=+1 skew in the diagnostic exactly matches the
29:7 asymmetry of `L_table > L_Eq31` vs `L_table < L_Eq31` in the
table — independent corroboration that the rule divergence, not a
random encoder bug, is the single dominant driver.

### Spec updates in this pass

1. `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §12.8 —
   new paragraph after the units annotation, documenting: (a) the L
   column is tabulated data; (b) Eq. 31/47 are full-rate only; (c)
   the single-floor rule matches 120/120; (d) the implementation
   rule for half-rate analysis encoding (table lookup, not formula).
2. `analysis/vocoder_decode_disambiguations.md` §14 (new) — detailed
   audit of the rule divergence with the 120/120 verification
   script so the finding survives pipeline reruns and is visible to
   other Claude Code sessions.
3. This gap report — status and resolution summary.

### Section citations for cross-checking

The gap report uses "§1.8.5 Eq. 31" and "§2.13 Annex A" — those are
synthesized section numbers from our extraction's heading layout.
The PDF section numbers to cite when verifying against the PDF are:

- **Eq. 31** lives in PDF §5.1.5 "Pitch Refinement", page 17
  (full-rate encoder L̂).
- **Eq. 47** lives in PDF §6.2 "Spectral Amplitude Decoding",
  page 22 (full-rate decoder L̃). Same double-floor form as Eq. 31.
- **Annex L** is at PDF page 127 with its table and the prose is at
  PDF §13.1 "Fundamental Frequency Encoding and Decoding" pages
  58–59.
