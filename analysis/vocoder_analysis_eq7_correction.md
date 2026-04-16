# Analysis Encoder Addendum: Eq. 7 `r(t)` Transcription Error

**Category:** Vocoder / Addendum Correction
**Relevant Specs:**
- `analysis/vocoder_analysis_encoder_addendum.md` §0.3.2, §0.3.3, §0.3.8, §0.3.9
- TIA-102.BABA-A §5.1.1 "Determination of E(P)" (PDF page 13), Equations 5–7
**Status at filing:** blocks progress on blip25-mbe analysis-encoder pitch tracker
**Date filed:** 2026-04-15
**Severity:** high — the derived addendum is inconsistent with the PDF, and the
addendum's own "Common Pitfalls" note §0.3.9 enshrines the error as a feature.

---

## Summary

The addendum transcribes Eq. 7 with `s_LPF` squared on both factors:

> ```
> r(t) = Σ_{j=−150}^{150}  s²_LPF(j) · w²_I(j) · s²_LPF(j+t) · w²_I(j+t)
> ```
> (addendum `vocoder_analysis_encoder_addendum.md:557`)

The published PDF (TIA-102.BABA-A §5.1.1, page 13, Eq. 7) shows `s_LPF`
**not** squared — only the window is squared:

```
          150
r(t)  =   Σ     s_LPF(j) · w²_I(j) · s_LPF(j+t) · w²_I(j+t)          (Eq. 7)
        j=−150
```

Implementing the addendum's form leads to catastrophic pathology on voiced
content: `E(P)` — which the PDF and addendum both describe as "close to 0
at the true pitch, close to 1 otherwise" — diverges to values of
magnitude 10⁴–10⁶ (both signs) on loud voiced frames, and the pitch
tracker's decision logic degenerates as a consequence.

The PDF form is also the one consistent with dimensional analysis of Eq. 5.

---

## Evidence

### 1. The PDF image

Eq. 5 (§5.1.1, PDF page 13) has `s²_LPF` in both the numerator energy
term and the denominator's first bracket — signal squared, window squared.
Eq. 7 (same section, three lines below) has `s_LPF(j) · w²_I(j) ·
s_LPF(j+t) · w²_I(j+t)` — signal **not** squared, window squared. The
superscript-2 is present on `w_I` and absent on `s_LPF` in the PDF
rendering.

### 2. Dimensional analysis

Eq. 5's numerator subtracts `Σ s²_LPF · w²_I` from `P · Σ r(n·P)`. For the
subtraction to be dimensionally meaningful, `r(t)` must share units with
`Σ s²_LPF · w²_I`, which is `[s²]`.

- PDF form: `r(t) = Σ s·w²·s·w² ~ [s²·w⁴]`, which equals `[s²]` since `w` is
  dimensionless. ✓
- Addendum form: `r(t) = Σ s²·w²·s²·w² ~ [s⁴·w⁴] = [s⁴]`. ✗

Under the addendum's form, Eq. 5's numerator is `[s²] − P·[s⁴]` — the
subtraction is unit-incoherent.

### 3. Empirical — blip25-mbe implementation

`crates/blip25-mbe/src/codecs/mbe_baseline/analysis.rs` followed the
addendum literally (`sw_sq[j] = (s_LPF · w_I)²`; `r(t) = Σ sw_sq[j] ·
sw_sq[j+t]`). Running against DVSI `tv-rc/clean` with the
`conformance/vectors` harness + new `--dump-frames` E-trace shows:

| frame | chip voicing | E(P̂_I) | E(P_chip) |
|-------|--------------|--------|-----------|
| 2–27  | UUU (silence/onset) | 0.97 – 1.70 | 0.22 – 1.14 |
| 28    | transition   | −7.93      | −39.96     |
| 29    | VVVV…        | −5.08 × 10⁴ | −8.37 × 10⁴ |
| 30    | VVVV…        | −2.27 × 10⁶ | −3.84 × 10⁶ |
| 31    | VVVV…        | −8.21 × 10⁶ | −1.34 × 10⁷ |

`E(P)` is supposed to be bounded in roughly `[0, 1]`. The negative
excursion starts exactly at the first voiced frame and scales with signal
energy. Both pitches land negative, and the "better" one is merely "more
negative" — the local-minimum structure the pitch tracker is supposed to
navigate is effectively destroyed.

With the PDF form (`s_LPF` not squared in Eq. 7), `r(t)` stays in
`[s²·w⁴]` and the numerator's subtraction remains bounded.

### 4. Annex B normalization is fine (not the problem)

`Σ w²_I(j) = 1.000000` and `Σ w⁴_I(j) = 0.006110` computed from
`annex_tables/annex_b_analysis_window.csv`. Eq. 6 holds; the fourth-moment
is small enough that `1 − P · Σ w⁴_I > 0` across the whole pitch grid.
The denominator-sign branch in `PitchSearch::e_of_p` is not firing. The
issue is strictly the numerator's r-accumulation having the wrong power
of `s_LPF`.

---

## Root cause

The addendum likely mis-read the PDF during transcription: the
superscript-2 on `w²_I` was duplicated onto `s_LPF` in §0.3.2's Eq. 7,
and the resulting "squared envelope" reading was then reinforced in two
downstream sections:

- **§0.3.8 Reference C Pseudocode** repeats the error (`acc += (a*a) *
  (b*b)`).
- **§0.3.9 Common Pitfalls** promotes the error to a pitfall warning
  (`r(t) uses squared samples. Eq. 7 correlates the energy envelope
  …, **not** a linear autocorrelation. If you call a linear
  autocorrelation routine … you will be off by a square.`). This note is
  the most dangerous part — it primes future implementers to treat the
  wrong form as the right one and to suspect correct code of being
  incorrect.

---

## Known limits of this correction

This document corrects **only Eq. 7 and the three downstream sites in the
addendum that propagated its error** (§0.3.2, §0.3.8, §0.3.9). It does
**not** re-audit the rest of the addendum for similar
duplicated-superscript transcription mistakes. Sections in particular
that should be re-verified against the PDF before being relied on:

- **§0.2** (Eq. 24–30, `S_w(m, ω₀)` basis, `A_l(ω₀)` harmonic
  projections).
- **§0.4** (Eq. 24, 28, 31, 45 — pitch refinement and `L̂` derivation).
- **§0.5** (spectral amplitude estimation `M̂_l` — inner products
  involving `w_R`).
- **§0.7** (V/UV discriminant — the `θ_G` threshold expressions and
  the band-energy ratios).

Any of those could carry an analogous mis-transcription and would
similarly only reveal itself at numerical runtime. Once this correction
lands, a future pass that diffs the addendum equations against the PDF
line-by-line (ideally independent of the current author) is warranted
before the addendum is treated as globally authoritative.

A reader of the corrected addendum should not assume the rest is free
of the same class of error just because this site has been fixed.

---

## Proposed addendum changes

### §0.3.2 — Eq. 7 transcription

Replace:

```
r(t) = Σ_{j=−150}^{150}  s²_LPF(j) · w²_I(j) · s²_LPF(j+t) · w²_I(j+t)
```

with:

```
r(t) = Σ_{j=−150}^{150}  s_LPF(j) · w²_I(j) · s_LPF(j+t) · w²_I(j+t)
```

and replace the explanatory sentence

> Note the **squared** sample values and the **squared** window values
> on both factors: `r(t)` is the correlation of the energy envelope
> `[s_LPF(j) · w_I(j)]²`, not a linear autocorrelation.

with something like

> Note that `s_LPF` appears to the first power and `w_I` to the second
> (squared). `r(t)` is the autocorrelation of `s_LPF(j) · w²_I(j)` —
> the LPF'd signal weighted by the *squared* analysis window. This is
> a linear autocorrelation of a window-squared envelope, not a
> correlation of `s²_LPF`.

### §0.3.8 — Reference C pseudocode

Change:

```c
double a = sLPF[j+150] * wI[j+150];
double b = sLPF[jt+150] * wI[jt+150];
acc += (a*a) * (b*b);
```

to:

```c
/* Per frame, hoist sw2[j] = s_LPF(j) · w²_I(j) out of the (j, t)
 * double loop — it's reused across all 123 lag values. */
double sw2[301];
for (int j = -150; j <= 150; ++j) {
    sw2[j+150] = sLPF[j+150] * wI[j+150] * wI[j+150];
}
/* …then the r(t) accumulation reads the precomputed envelope: */
for (int t = 0; t <= 122; ++t) {
    double acc = 0.0;
    for (int j = -150; j <= 150; ++j) {
        int jt = j + t;
        if (jt < -150 || jt > 150) continue;
        acc += sw2[j+150] * sw2[jt+150];
    }
    r[t] = acc;
}
```

### §0.3.9 — Common Pitfalls

Remove the bullet

> - **`r(t)` uses squared samples.** Eq. 7 correlates the **energy
>   envelope** `[s_LPF(j) · w_I(j)]²`, not `s_LPF` directly. If you
>   call a linear autocorrelation routine (OP25's `mbe_pitch_search`,
>   most generic DSP libraries) on `s_LPF(j) · w_I(j)` you will be
>   off by a square …

Replace with

> - **`r(t)` is a linear autocorrelation of `s_LPF · w²_I`, not of
>   `s_LPF`** (plain) and **not** of `(s_LPF · w_I)²`. The
>   distinguishing feature is the window's second power — the signal
>   itself appears linearly.
>
>   Concretely: build the per-frame sequence `sw2[j] = s_LPF(j) ·
>   w²_I(j)` for `j ∈ [−150, 150]` (i.e. the LPF'd signal pre-multiplied
>   by the squared window), then pass `sw2[]` to a generic **unwindowed**
>   linear autocorrelation. Routines that apply their own window
>   internally (as many DSP libraries do — scipy's `correlate`,
>   FFTW-based helpers that window before the FFT, etc.) will
>   double-window and produce the wrong answer; disable any built-in
>   windowing or pass a rectangular window. Callers who pass
>   `s_LPF(j) · w_I(j)` (single window power) or `(s_LPF(j) · w_I(j))²`
>   (the previously-documented incorrect form) will also not match the
>   PDF.

### §0.3.3 — Interpretation prose

The existing prose ("`E(P)` is small when P is the true pitch, close to
1 otherwise, …") is correct and does not need changes, but a reader who
has implemented the wrong Eq. 7 and is staring at `E(P) = −2 × 10⁶` will
benefit from an added sentence at the end of §0.3.3:

> If an implementation observes `E(P)` diverging to magnitudes far
> outside `[0, 1]` on voiced content, the most likely cause is a
> mis-transcription of Eq. 7 with `s_LPF` squared: a squared-signal
> formulation of `r(t)` has units `[s⁴]` and breaks Eq. 5's numerator
> subtraction. See `analysis/vocoder_analysis_eq7_correction.md` for
> the correction history.

---

## Why third-party source is not an acceptable substitute

Per project CLAUDE.md the blip25-mbe code cannot source from OP25,
SDRTrunk, JMBE, dsdcc, or imbe_vocoder. Those implementations do likely
use the PDF form (a linear autocorrelation library call on `s_LPF · w²_I`
— or on `s_LPF` directly — with appropriate windowing), but confirming
that is a one-way sanity check at best. The authoritative source is the
PDF, and the addendum's job is to transcribe it faithfully. This
correction restores that faithfulness; once landed, the implementation
can proceed from the addendum alone per project policy.

---

## Verification plan (post-correction)

Once the addendum is corrected:

1. In `crates/blip25-mbe/src/codecs/mbe_baseline/analysis.rs`,
   `PitchSearch::new` replaces the `sw_sq` precomputation with a
   `sw2[j] = s_LPF(j) · w²_I(j)` precomputation; `r(t)` accumulates
   `sw2[j] · sw2[j+t]`.
2. Re-run `cargo run -p blip25-conformance-vectors -- analysis-encode
   clean --rc --dump-frames 40 --chip-offset=-2`. Expected:
   `E(P̂_I)`, `E(P_chip)` ∈ ~`[0, 1]` on voiced frames; `E(P)` minimum
   structure is now informative; cumulative errors `CE_B`, `CE_F`
   bounded and the `CE_B ≤ 0.48` gate in `decide_initial_pitch` works
   as a confidence gate rather than an unconditional takeover.
3. Aggregate baseline on `clean` should improve substantially from the
   current `1171 cents / 98.8% L mismatch / 78.2% voicing` — the exact
   numbers will characterize whether the Eq. 7 fix alone is sufficient
   or whether additional discrepancies remain (e.g. §0.3.6 decision
   rules, §0.3.5 sub-multiple cascade, `P̂_I` → `ω̂_0` refinement).
4. If `E(P)` is now bounded but the picks still disagree with DVSI
   systematically, the residual is a separate issue (likely in §0.4
   refinement or §0.3.5 cascade), not in `E(P)` itself.
