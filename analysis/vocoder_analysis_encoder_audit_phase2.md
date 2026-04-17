# Analysis Encoder Addendum — Phase 2 Audit Findings

**Category:** Vocoder / Addendum Audit
**Relevant Specs:** `analysis/vocoder_analysis_encoder_addendum.md`
**Date filed:** 2026-04-17
**Scope:** line-by-line PDF cross-check of addendum sections NOT
covered in the phase-1 audit (task #1, which covered §0.2 / §0.4 /
§0.5 / §0.7). Sections covered in this pass: §0.1, §0.3, §0.6, §0.8,
§0.9, §0.10, §0.11.

---

## Summary

The remaining seven addendum sections verify cleanly against the
BABA-A PDF, with **two minor issues** found and fixed on branch
`spec-author/analysis-encoder-audit-phase2`:

1. **§0.6.7 cold-start `L̃(−1) = 30` quote** — the addendum presented
   the init sentence as a verbatim quote, but the PDF page 25 text
   reads `L̂(−1) = 30` (hat, not tilde). The hat form is a PDF typo
   — Eq. 52 references `L̃(−1)` not `L̂(−1)`, and only the tilde
   reading makes the predictor well-defined on frame 0. Fixed by
   removing "verbatim" framing and adding a "PDF typo note" that
   documents the hat→tilde correction explicitly.
2. **§0.10.2 / §0.10.3 rate-mode `ρ` consistency** — §0.10.3 prose
   claimed a helper `PredictorState.rho_of_L` would dispatch between
   full-rate (Eq. 55 piecewise) and half-rate (Eq. 155 literal 0.65),
   but the reference C in §0.6.8 defined only a single
   `rho_of_L()` that implemented the full-rate schedule
   unconditionally, and §0.10.2's orchestration passed no rate-mode
   argument to `compute_prediction_residual`. Fixed by adding
   `rho_halfrate()`, renaming the full-rate helper to
   `rho_of_L_fullrate()`, extending `compute_prediction_residual`
   with a `rate_mode` parameter, and updating the §0.10.2 call
   site + §0.10.3 prose.

Neither is an equation transcription error in the "Eq. 7" or
"Eq. 31" sense. #1 is a quote-fidelity issue that the PDF's own
typo triggered; #2 is a C-reference consistency issue spanning two
subsections.

---

## Per-section audit notes

### §0.1 — Input framing, HPF, windows

Verified against PDF §3 (Eq. 1), §4 (front-end), §5.1 (page 11–12,
Figure 7–8), Eq. 3, Eq. 6. All match:

- Eq. 1 `s_w(n) = s(n)·w(n)` ✓
- Eq. 3 HPF transfer function `H(z) = (1−z⁻¹)/(1−0.99·z⁻¹)` ✓
- 160-sample frame shift, 20 ms cadence ✓
- Window centering convention `w_R(n) = w_R(−n)`, `w_I(n) = w_I(−n)` ✓
- 40-sample offset between `w_R`'s and `w_I`'s first nonzero points ✓
- Overlap: `w_I` = 141 samples, `w_R` = 61 samples ✓
- Eq. 6 normalization `Σ w_I²(j) = 1.0` ✓
- HPF init (zero state) correctly flagged as "not specified by PDF"

Clean. No edits needed.

### §0.3 — Initial pitch estimation

Verified against PDF §5.1.1–5.1.5 (pages 13–15), Eq. 5 through 23.
All match:

- Eq. 5 `E(P)` numerator/denominator structure ✓
- Eq. 7 `r(t) = Σ s_LPF(j)·w²_I(j)·s_LPF(j+t)·w²_I(j+t)` — corrected
  in the earlier Eq. 7 note; still correct here ✓
- Eq. 8 linear interpolation ✓
- Eq. 9 LPF convolution ✓
- Eq. 10, 13, 14, 16 pitch continuity constraints (`0.8·P̂`, `1.2·P̂`) ✓
- Eq. 11, 15 candidate set `{21, 21.5, …, 122}` ✓
- Eq. 12 backward cumulative `CE_B = E(P̂_B) + E_{-1}(P̂_{-1}) + E_{-2}(P̂_{-2})` ✓
- Eq. 17 forward cumulative `CE_F = E(P̂_0) + E_1(P̂_1) + E_2(P̂_2)` ✓
- Eq. 18 cascade row 1: `CE_F(P̂_0/n) ≤ 0.85 AND ratio ≤ 1.7` ✓
- Eq. 19 cascade row 2: `CE_F(P̂_0/n) ≤ 0.4 AND ratio ≤ 3.5` ✓
- Eq. 20 cascade row 3: `CE_F(P̂_0/n) ≤ 0.05` ✓
- Eq. 21–23 backward-vs-forward selection ✓
- §5.1.3 init prose `E_{-1}(P) = E_{-2}(P) = 0`, `P̂_{-1} = P̂_{-2} = 100` ✓

Clean. No edits needed.

### §0.6 — Log-magnitude prediction residual

Verified against PDF §6.3 (pages 25–27), Eq. 52 through 57. All
match with one caveat:

- Eq. 52 `k̂_l = (L̃(-1)/L̂(0)) · l` ✓
- Eq. 53 `δ̂_l = k̂_l − ⌊k̂_l⌋` ✓
- Eq. 54 `T̂_l = log₂ M̂_l(0) − ρ·P_l + (ρ/L̂(0))·Σ P_λ`, mean term
  with `+` sign ✓
- Eq. 55 piecewise ρ schedule (0.4 / linear / 0.7) ✓
- Eq. 56 `M̃_0(-1) = 1.0` ✓
- Eq. 57 extension `M̃_l(-1) = M̃_{L̃(-1)}(-1)` for `l > L̃(-1)` ✓
- §6.3 prose "encoder simulates the decoder ... setting L̃ = L̂" ✓

**Edit applied:** the init sentence cited "L̃(−1) = 30" inside a
verbatim blockquote, but the PDF actually reads "L̂(−1) = 30". This
is almost certainly a typo in the PDF (Eq. 52 references L̃ not
L̂), but the blockquote framing suggested the addendum had the PDF
text exactly. Reworded to present the paraphrase explicitly as a
typo correction, with rationale.

### §0.8 — Frame-type dispatch

Verified against PDF §6.1 (page 22, full-rate `b̂_0` range), §13.1
Table 14 (page 58), §13.1 Eq. 143–145, §13.3 (silence payload),
§16 (tone frame). All match:

- Full-rate `b̂_0 ∈ [0, 207]`, 208–255 reserved ✓
- Table 14: voice 0–119 / erasure 120–123 / silence 124–125 / tone 126–127 ✓
- "If the frame is a silence frame, then `b̂_0 = 124`" ✓
- Eq. 143 `ω̃_0 = 2π/32` (verified with `π` prefix) ✓
- Eq. 144 `L̃ = 14` ✓
- Eq. 145 `ṽ_l = 0` ✓
- §13.3 prose "silence frames … are not updated" (predictor freeze) ✓
- §16 tone-frame payload defers to Annex T ✓
- The "NOT in the PDF" callouts for encoder silence/tone entry
  criteria are correctly framed.

Clean. No edits needed.

### §0.9 — Encoder state structure and initialization

Synthesis subsection. Pulls together cold-start values from §0.1,
§0.3, §0.6, §0.7 (all PDF-cited where PDF-sourced; inferred values
marked `(inferred)` with rationale). No direct PDF transcriptions
to re-verify beyond what §0.6.7 already covers. The `L̃(−1) = 30`
entry in the cross-frame state table (row 10) carries the same
typo-correction as §0.6.7; the updated prose there is sufficient
for a reader arriving via §0.9.

Clean. No edits needed beyond the §0.6.7 update.

### §0.10 — End-to-end reference C pipeline

Orchestration subsection. Composes §0.1–§0.8's reference C into a
single per-frame function. No new equations.

**Edit applied:** §0.10.3 (rate-mode fork) claimed a helper
`PredictorState.rho_of_L` would dispatch between full-rate
(Eq. 55) and half-rate (Eq. 155 literal 0.65) ρ values, but the
§0.6.8 reference C defined only a single `rho_of_L(L_hat)` that
implemented the full-rate schedule unconditionally, and §0.10.2's
call to `compute_prediction_residual` passed no rate argument.
Half-rate encoding would therefore have used the full-rate
piecewise ρ — a silent correctness bug.

Fixed by:
- Renaming `rho_of_L` → `rho_of_L_fullrate` and adding
  `rho_halfrate()` (returns the literal 0.65, cites Eq. 155) in
  §0.6.8.
- Extending `compute_prediction_residual` with a `rate_mode`
  parameter; the function dispatches on it.
- Updating §0.10.2's call site to pass `rate_mode`.
- Rewriting the §0.10.3 prose to describe the actual dispatch.

### §0.11 — Numerical cross-checks

Validation-policy subsection. Tolerances, test-vector sources,
checkpoint scheme, debugging workflow. No PDF equations to audit;
the PDF does not specify validation procedure. The subsection
correctly states this.

Clean. No edits needed.

---

## Branch state

All fixes on `spec-author/analysis-encoder-audit-phase2`. One
commit expected. Pending user merge. Implementer should not
consume these changes until after user review.

---

## Limits of this audit

This pass verified **equation transcriptions and prose claims that
the addendum presents as PDF-sourced** in the seven named sections.
It did not:

- Re-audit §0.2 / §0.4 / §0.5 / §0.7 (covered in task #1, plus the
  Eq. 31 correction).
- Audit the addendum's **inferred** init values (§0.9 rows 1, 2, 6,
  7, 8, 11) — these are explicitly marked inferred and the
  rationale is given; they're correctness-by-construction, not
  PDF-sourced claims to verify.
- Audit the §0.10.2 reference C end-to-end. It composes helpers
  defined in §0.1.7, §0.2.7, §0.3.8, §0.4.6, §0.5.5, §0.6.8,
  §0.7.9 — any bug in those composes into §0.10.2, but §0.10.2
  itself does not introduce new math.
- Audit the §0.11 validation harness design against DVSI's actual
  test-vector format. That's a tooling concern, not a spec-audit
  concern.

With this phase and phase 1 together, **every equation the
addendum cites from BABA-A §§3–13 has been line-by-line verified
against the PDF**, modulo the three corrections already documented
(Eq. 7, Eq. 31, Eq. 43/44) and the §0.6.7 typo-correction noted
here.

A future pass that re-audits §0.2 / §0.4 / §0.5 / §0.7 from an
independent reader would still be valuable — the phase-1 audit
was performed by the same author who drafted the addendum. Phase
2 (this document) was performed by a different author pass with
full PDF access, which is how the Eq. 31 and §0.6.7 issues
surfaced.
