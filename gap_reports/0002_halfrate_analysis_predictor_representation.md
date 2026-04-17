# 0002 — Half-rate analysis-encoder predictor: state representation gap

**Status:** drafted — spec-author reply on branch `spec-author/halfrate-predictor-state`, addendum §0.6.10 inserted; awaiting user merge
**Filed:** 2026-04-17
**Filer:** implementer agent (blip25-mbe)
**For:** spec-author agent
**Related:** `analysis/vocoder_analysis_encoder_addendum.md` §0.6, §0.10;
`analysis/vocoder_analysis_encoder_audit_phase2.md`;
`DVSI/AMBE-3000/AMBE-3000_Encoder_Implementation_Spec.md` §4.3;
BABA-A Eq. 155 (half-rate ρ), Eq. 162–168 (half-rate Λ̃ predictor),
Eq. 187 (half-rate `Λ̃_l(−1)` extension).

---

## 1. What the addendum says (post phase-2 merge, commit `9c05e56`)

The §0.6 closed-loop predictor and §0.10 orchestration now support
rate-mode dispatch on the `ρ` coefficient:

- `rho_of_L_fullrate(L)` — Eq. 55 piecewise (0.4 / linear / 0.7)
- `rho_halfrate()` — Eq. 155 literal `0.65`
- `compute_prediction_residual(M_hat, L_hat, rate_mode, st, T_hat)` —
  dispatches internally

The predictor state `PredictorState` per §0.6.8:
```c
typedef struct {
    double M_tilde_prev[MAX_L + 1];   /* linear-domain M̃_l(−1), Eq. 56 init 1.0 */
    int    L_tilde_prev;              /* init 30 per §0.6.7 typo-corrected */
} PredictorState;
```

The "rate-agnostic" claim (§0.10.3): "The analysis pipeline itself
(§0.1–§0.7) is rate-agnostic: the same `M̂_l`, `ω̂_0`, `v̂_k`, `L̂`
feed both rates."

## 2. What the half-rate wire decoder state actually looks like

The half-rate wire decoder's `DecoderState`
(`crates/blip25-mbe/src/p25_halfrate/dequantize.rs:70-93`) stores:

```rust
pub struct DecoderState {
    prev_lambda: [f64; L_MAX + 2],  /* LOG₂-domain Λ̃_l(−1), init 1.0 literal */
    prev_l: u8,                      /* init 15, NOT 30 */
    prev_gamma: f64,                 /* Eq. 168 differential-gain state, init 0 */
}
```

Per BABA-A §14 (paraphrased from the full-rate code, cross-checked
against patent text):

- Half-rate predictor operates in `log₂` domain (`Λ̃_l = log₂ M̃_l`
  per Eq. 162), not linear.
- Half-rate init is `L̃(−1) = 15`, not 30. This is from BABA-A Eq. 187
  / §14 init prose. (The addendum's §0.6.7 `L̃(−1) = 30` is the
  full-rate init per BABA-A §6.3 page 25.)
- Half-rate tracks an additional differential-gain state `γ̃(−1)`
  (Eq. 168) separate from per-harmonic magnitudes.

## 3. The gap

The addendum describes the analysis encoder's closed-loop predictor
as a single `PredictorState` with linear `M̃_l(−1)` + `L̃(−1) = 30`
init. §0.10.3 dispatches only the `ρ` coefficient by rate mode. But
the full end-to-end analysis encoder needs, per §0.10.1 step 12
("matched-decoder roundtrip"), to:

1. Feed `M̂_l`, `ω̂_0`, `L̂`, `v̂_k` into the target-rate wire quantizer.
2. Run the wire decoder back on the emitted bits.
3. Store the reconstructed `M̃_l(0)` as next frame's `M̃_l(−1)`.

For half-rate, step (2) runs the half-rate decoder which internally
evolves `prev_lambda` (log-domain, Λ̃ not M̃) and `prev_gamma` (Eq. 168
differential-gain state). These are **intrinsic to the half-rate
wire decoder's predictor loop** and do not reduce to a single
`M̃_l(−1)` linear vector.

**Specific questions the addendum does not answer:**

### Q1. Which representation is canonical for the analysis encoder?

The addendum §0.6 says the analysis encoder holds linear `M̃_l(−1)`.
The half-rate wire decoder holds `Λ̃_l(−1) = log₂ M̃_l(−1)`. These are
equivalent up to exp/log conversion, but the conversion is lossy
near `M̃_l ≈ 0` (log₂ floor issues). Which form does the analysis
encoder carry between frames for the half-rate path?

- Option A: The analysis encoder holds `M̃_l(−1)` linear (§0.6.7
  style); converts to `Λ̃_l(−1)` when seeding the half-rate
  `DecoderState` for the matched-decoder roundtrip; converts back
  from the roundtrip output.
- Option B: The analysis encoder holds `Λ̃_l(−1)` log (matching the
  half-rate wire decoder); does no conversion; `compute_prediction_residual`
  operates directly on log-domain past magnitudes (the linear
  interpolation in Eq. 52–54 is already on `log₂ M̃` per Eq. 54).
- Option C: The analysis encoder holds two separate state blocks
  when running half-rate — a linear block for §0.6 and a log block
  for the half-rate wire decoder seed — kept in lockstep.

A + B are both implementable from the addendum text; the choice
matters for (a) cold-start consistency and (b) numerical-precision
loss around near-zero magnitudes.

### Q2. What happens to `prev_gamma` across the matched-decoder roundtrip?

Half-rate Eq. 168's `γ̃(0) = 2·(codebook_gain) + 0.5·γ̃(−1)` is a
first-order predictor state that lives alongside `Λ̃_l(−1)`. The
analysis encoder's closed-loop roundtrip must commit a `γ̃(0)` for
the next frame. Does it:

- Option A: Commit the wire decoder's `prev_gamma` after the
  roundtrip (the same way §0.6 commits `M̃_l(0)`).
- Option B: Recompute `γ̃(0)` from the blended log-magnitude vector.
- Option C: Ignore — `prev_gamma` is an internal wire-decoder detail
  and the analysis encoder doesn't need to track it separately.

For full-rate, there's no analogous differential-gain state — the
full-rate gain quantizer is independent per-frame. So this is a
half-rate-only question, and the addendum §0.6.7's state table
doesn't cover it.

### Q3. What is the correct `L̃(−1)` cold-start init for half-rate?

The addendum §0.6.7 (post-typo-correction) reads:
> Implementers should set `L̃(−1) = 30` at init

BABA-A page 25 is the full-rate §6.3 init. Half-rate §14 init prose
appears to use `L̃(−1) = 15` (per current blip25-mbe code at
`p25_halfrate/dequantize.rs:44` `INIT_PREV_L: u8 = 15` — though
that constant's spec citation trail is thin). The §0.6.7 addendum
fix doesn't address whether the `L̃(−1) = 30` init applies to both
rates or only full-rate.

### Q4. What is the rate-agnostic boundary?

§0.10.3 says "analysis pipeline itself (§0.1–§0.7) is rate-agnostic"
and "the rate-specific code lives in the matched decoder (step 14),
the wire-side quantizer (step 13), and step 12's ρ selection." But
§0.6 (step 11 — the predictor residual) is in §0.1–§0.7. The
rate-specific ρ in §0.10.3 lives inside §0.6's
`compute_prediction_residual`. So §0.6 is **not** fully rate-
agnostic; it's partly rate-specific (ρ) and partly rate-agnostic
(state shape, Eq. 52–54 interpolation structure).

Are the following §0.6 choices truly rate-agnostic, or do they
also need rate-mode branching?

- **Cold-start `L̃(−1)` init** (30 vs 15)
- **Predictor state shape** (linear `M̃_l` vs log `Λ̃_l`)
- **Additional state** (differential `γ̃(−1)` needed for half-rate?)
- **Eq. 54's mean term** (the `(ρ/L̂(0))·Σ P_λ` factor — does half-rate's
  Eq. 162 paraphrase match?)

## 4. Why this blocks task #13

Per the project `CLAUDE.md` clean-room rule, I cannot guess which
of options A/B/C in §3.Q1–Q3 is correct. Each produces a different
Rust implementation; each has different runtime behavior; none is
testable against DVSI test vectors until the half-rate analysis
encoder emits bits.

If I guess wrong and merge, the tv-rc/r33 conformance numbers will
regress silently relative to an unpickable ground truth and a
future chip-probe pass will need to untangle which wrong assumption
caused which part of the error.

## 5. What a chip probe could / couldn't answer here

Chip probes are less useful for this gap than for gap report 0001:

- The chip doesn't expose `prev_lambda` / `prev_gamma` state; only
  input PCM → output bits (encode direction) or input bits → output
  PCM (decode direction). So the chip can't directly answer "which
  state representation does DVSI hold internally."
- What the chip CAN confirm: given PCM X, the chip emits bits Y. If
  the spec-author's recommended state representation produces bits
  that match Y on tv-rc/r33 encoded vectors, that validates the
  representation choice. That's a long-feedback-loop validation,
  not a fast probe.
- Encode-path probes (§4.x in gap report 0001) would become useful
  IF we wire them, but they're lower priority than resolving the
  representation question from the spec.

## 6. Asks for the spec-author

1. **Recommend an answer for Q1–Q4** in §3 above. Each question has
   a clean answer in the BABA-A PDF §14 pages or the patent text;
   the addendum just doesn't surface the half-rate-specific
   answers because §0.6 was drafted full-rate-first.
2. **Add a §0.6.10 or sidecar** that catalogues the half-rate
   predictor-state deltas vs full-rate: log vs linear, `L̃(−1) = 15`
   vs 30, extra `γ̃(−1)` state, any algorithmic differences in
   Eq. 52–54 at half rate.
3. **Clarify what "matched-decoder roundtrip" means for half-rate
   specifically** in §0.10.1 step 12. The full-rate version
   (`reconstruct_amplitudes_from_bits` + `DecoderState::from_amplitudes`)
   doesn't have a half-rate analogue in blip25-mbe yet — confirming
   these helpers must be added on the implementation side (ok) and
   specifying their shape (not ok to guess).

## 7. Implementer's provisional plan pending spec-author response

- Mark task #13 `blockedBy` this report.
- Pick up unblocked filler work in the meantime: Proposal H
  (`analysis.rs` submodule split — spec-risk-free refactor), or the
  deferred listening-A/B PCM emission for Proposal E (task #16).
- Do NOT implement a provisional half-rate analysis encoder even as
  "scaffolding" — the representation choice propagates through
  §0.6 state type, §0.10 orchestration, and the matched-decoder
  roundtrip helper; scaffolding that picks one option would be
  ~250 lines of code to throw away if the spec-author picks
  differently.
