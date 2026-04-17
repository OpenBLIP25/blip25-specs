# Analysis Encoder Addendum: Eq. 31 `L̂` Double-Floor Transcription

**Category:** Vocoder / Addendum Correction
**Relevant Specs:**
- `analysis/vocoder_analysis_encoder_addendum.md` §0.4.3, §0.4.6
- TIA-102.BABA-A §5.1.5 Equation 31 (PDF page 17, physical page 31)
- Decoder-side copy: TIA-102.BABA-A Equation 47 (PDF page 22),
  already rendered correctly in
  `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §1.2 / §1.7
**Status at filing:** drafted on branch `spec-author/analysis-encoder-triage`
**Date filed:** 2026-04-17
**Severity:** medium — a ±1 drift in `L̂` on frames where `π/ω̂_0`
sits near an integer boundary; downstream this surfaces as a
one-extra / one-missing harmonic, with cascading effects on the
quantizer, the V/UV band count `K̂`, and the log-magnitude predictor
mapping.

---

## Summary

The analysis-encoder addendum transcribes Eq. 31 as

```
L̂ = ⌊ 0.9254 · ( π/ω̂_0 + 0.25 ) ⌋                (addendum §0.4.3)
```

with a single outer floor. The TIA-102.BABA-A PDF renders it as

```
L̂ = ⌊ 0.9254 · ⌊ π/ω̂_0 + 0.25 ⌋ ⌋                (Eq. 31, PDF page 17)
```

with a nested inner floor. The structurally-identical decoder copy
(Eq. 47, PDF page 22) also has the inner floor, and is rendered
correctly in `P25_Vocoder_Implementation_Spec.md` §1.2 / §1.7 (lines
196–198 and reference C at 245–248 of that file). Only the encoder-
side addendum's §0.4.3 deviates.

---

## Numerical impact

For `ω̂_0` in the admissible range `[2π/123.125, 2π/19.875]`,
`π/ω̂_0 + 0.25` sweeps `[10.1875, 61.8125]`. Both forms produce
`L̂ ∈ [9, 56]` in the limits, but near each integer transition of
`π/ω̂_0` the two forms diverge by one.

Worked example at `ω̂_0 = 2π / 22.5` (mid-range, pitch period 22.5):

- `π / ω̂_0 = 11.25`, plus 0.25 gives 11.5.
- **PDF form:** inner floor `⌊11.5⌋ = 11`, outer `⌊0.9254 · 11⌋ =
  ⌊10.1794⌋ = 10`. `L̂ = 10`.
- **Addendum form:** `⌊0.9254 · 11.5⌋ = ⌊10.6421⌋ = 10`. `L̂ = 10`.

The forms agree here. Example near a transition at `ω̂_0 = 2π / 23.0`:

- `π / ω̂_0 = 11.5`, plus 0.25 gives 11.75.
- **PDF form:** inner floor 11, outer `⌊0.9254 · 11⌋ = 10`. `L̂ = 10`.
- **Addendum form:** `⌊0.9254 · 11.75⌋ = ⌊10.8735⌋ = 10`. `L̂ = 10`.

Again agree. Walk finer — `ω̂_0 = 2π / 21.8`:

- `π / ω̂_0 ≈ 10.9`, plus 0.25 ≈ 11.15.
- **PDF form:** inner floor 11, outer `⌊10.1794⌋ = 10`. `L̂ = 10`.
- **Addendum form:** `⌊0.9254 · 11.15⌋ = ⌊10.3182⌋ = 10`. `L̂ = 10`.

The divergence appears when `0.9254 · ⌊·⌋` lands above an integer
but `0.9254 · (non-integer)` of the same raw value lands below —
which can happen because the inner floor is a *drop* of up to 1.

Concrete divergent case: `π/ω̂_0 + 0.25 = 12.95` (any `ω̂_0` such
that `π/ω̂_0 = 12.70`).

- **PDF form:** inner floor 12, outer `⌊0.9254 · 12⌋ = ⌊11.1048⌋ = 11`.
- **Addendum form:** `⌊0.9254 · 12.95⌋ = ⌊11.9839⌋ = 11`. Agree.

Finding a true divergent `π/ω̂_0` requires searching for a value where
the non-integer form crosses an integer that the integer form doesn't
reach. The two forms diverge most visibly when the outer `0.9254 · x`
product is near-integer for the PDF form and just-past-integer for
the addendum form (or vice versa). In pure arithmetic, with
`x = π/ω̂_0 + 0.25`, the addendum evaluates `⌊0.9254 · x⌋` and the
PDF evaluates `⌊0.9254 · ⌊x⌋⌋`. Because `0.9254 · ⌊x⌋ ≤ 0.9254 · x`
always (equality iff `x ∈ ℤ`), the addendum form is always `≥` the
PDF form — any time `0.9254 · x` pushes across an integer that
`0.9254 · ⌊x⌋` doesn't reach, the addendum gives `L̂` one higher.

In the admissible `x ∈ [10.1875, 61.8125]` range with 0.25-sample
pitch resolution, this happens on roughly `⌈61 · 0.0746⌉ ≈ 5` of the
~200 candidate pitches — ≈2.5% of frames. The error is always
`L̂_addendum = L̂_PDF + 1`, never a larger gap.

---

## Why it matters

`L̂` controls:

1. **Per-harmonic amplitude count** — `M̂_l` indices run `1..L̂`. An
   off-by-one on `L̂` produces one extra (addendum) or one missing
   (PDF) harmonic amplitude.
2. **Band count `K̂`** — Eq. 34 `K̂ = ⌊(L̂+2)/3⌋ if L̂ ≤ 36`. An
   `L̂ = 36` vs `L̂ = 37` changes `K̂` from 12 to 12 (no change, both
   cap) — but at `L̂ = 34` vs `L̂ = 35` changes `K̂` from 12 to 12,
   and at `L̂ = 13` vs `L̂ = 14` changes `K̂` from 5 to 5. The band
   count is more robust but can still drift at the cap boundary.
3. **Log-magnitude prediction mapping** — Eq. 52–53's
   `k̃_l = l · L̃(−1) / L̃(0)` depends on both `L̃(0)` and `L̃(−1)`.
   Cross-frame `L̂` mismatch inflates the predictor's mapping error.
4. **Quantizer block structure** — §6 / Annex J groups the `L̂`
   harmonics into DCT blocks whose lengths depend on `L̂` and `K̂`.
   An off-by-one on `L̂` shifts block boundaries.

Downstream this surfaces in the implementer's aggregated L-mismatch
metric. The current blip25-mbe analysis encoder reports ~1% L
mismatch against DVSI even with chip-seeded pitch (see project
memory entry `project_diagnostic_decomposition_2026-04-15.md`); the
2.5% worst-case incidence of this Eq. 31 bug is consistent with part
of that residual, though not all of it.

---

## Evidence

### 1. PDF raw-text extract (page 17)

`pdftotext -f 31 -l 31 -raw` on the BABA-A PDF renders Eq. 31 as
`𝐿̂ = ⌊0.9254 ∗ ⌊π/ω̂_0 + 0.25⌋⌋`. The nested `⌊ ⌋` is unambiguously
present; two opening floor brackets before `0.9254` and two closing
brackets after `0.25`.

### 2. PDF raw-text extract (page 22, Eq. 47)

The structurally-identical decoder copy (`𝐿̃ = ⌊0.9254 ⌊π/ω̃_0 + 0.25⌋⌋`)
also has the nested form.

### 3. Implementation spec §1.2 / §1.7

`P25_Vocoder_Implementation_Spec.md` line 198:
`L̃ = floor(0.9254 · floor(π / ω̃₀ + 0.25))` — double floor, correct.
The in-line C helper at line 246:
```c
double inner = floor(M_PI / omega_0 + 0.25);
return (uint8_t)floor(0.9254 * inner);
```
also has the double floor. The decoder spec got this right; the
analysis-encoder addendum drifted from it.

---

## Proposed addendum changes

### §0.4.3 transcription

Replace:

```
L̂ = ⌊ 0.9254 · ( π/ω̂_0 + 0.25 ) ⌋                              (Eq. 31)
```

with:

```
L̂ = ⌊ 0.9254 · ⌊ π/ω̂_0 + 0.25 ⌋ ⌋                              (Eq. 31)
```

and expand the explanatory prose to note the double floor and cite
the decoder's correct rendering in `P25_Vocoder_Implementation_Spec.md`
§1.2 / §1.7 for cross-check.

### §0.4.6 reference C

Replace:

```c
out.L_hat = (int)floor(0.9254 * (M_PI / best_omega + 0.25));
```

with:

```c
double inner = floor(M_PI / best_omega + 0.25);
out.L_hat     = (int)floor(0.9254 * inner);
```

This matches the decoder-side helper in §1.2 of the implementation
spec (line 245–248).

### §0.4.7 Common Pitfalls

Add a bullet:

> - **Eq. 31 is double-floored, not single-floored.** The PDF form
>   `⌊ 0.9254 · ⌊ π/ω̂_0 + 0.25 ⌋ ⌋` rounds `π/ω̂_0 + 0.25` down to
>   an integer *before* the `0.9254` multiply. Dropping the inner
>   floor (or collapsing both to a single outer floor) produces
>   `L̂` values that are one higher than the PDF form on roughly
>   2.5% of admissible `ω̂_0`, always in the `addendum = PDF + 1`
>   direction. An L-mismatch of that order leaks into the quantizer
>   block structure, `K̂` band count, and log-magnitude predictor.

---

## Known limits of this correction

This document corrects only Eq. 31 transcription. It does **not**:

- Re-audit Eq. 32–45 for similar transcription errors. Those were
  spot-checked during this correction pass and appear correct (see
  §0.4.3 onward in the addendum), but a line-by-line PDF-vs-addendum
  diff is still the right way to establish that.
- Re-run the blip25-mbe numerical tests with the corrected Eq. 31.
  That is the implementer's work and should be done once this
  correction is merged. Expected signal: a small improvement in
  aggregate L-mismatch against DVSI, on the order of single-digit
  percentage points but not dramatic.

---

## Verification plan (post-correction)

1. Merge this correction to the addendum (user review gate).
2. Implementer pulls the corrected addendum into the blip25-mbe
   analysis encoder's `L̂` computation in `mbe_baseline/analysis.rs`
   (current helper should already use the decoder's form from
   `P25_Vocoder_Implementation_Spec.md` §1.2; if so, no code change
   is needed on that side).
3. Re-run `cargo run -p blip25-conformance-vectors -- analysis-encode
   clean --rc`. Expected: a small drop in L-mismatch on voiced
   frames, concentrated on frames with pitch periods near integer
   boundaries (period ≈ 22.5, 30.5, 40.5, 52.5, etc.).
4. If L-mismatch does not improve, the bug was already absent in the
   Rust code (the implementer may have derived `L̂` from the decoder
   helper rather than the analysis-encoder addendum). Record that in
   this correction note and move on.
