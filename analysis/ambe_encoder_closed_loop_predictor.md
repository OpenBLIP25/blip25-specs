# AMBE Encoder Closed-Loop Predictor Feedback

**Scope:** The AMBE+2 encoder's log-magnitude prediction residual is
computed against the *reconstructed* prior-frame magnitudes (what the
decoder will actually see), not against the raw analyzed prior-frame
magnitudes. This requires running a matched decoder inside the encoder —
closed-loop predictor feedback. Skipping this step causes the encoder's
predictor state to diverge from the decoder's over ~50 frames, producing
progressive spectral drift.

**Status:** Structural invariant, documented here for cross-session
reference. Algorithm source: US8595002 col. 20 + encoder addendum §0.6.6.

**Date:** 2026-04-16

---

## 1. The Open-Loop Temptation

A naive encoder computes the log-magnitude prediction residual against
its own analyzed magnitudes from the prior frame:

```
/* OPEN-LOOP — WRONG. Diverges from decoder over time.                 */
T̂_l = log₂(M̂_l(0)) − ρ · log₂(M̂_l(−1))_analysis
```

where `M̂_l(−1)_analysis` is whatever the encoder's PCM analysis produced
for harmonic `l` last frame.

This seems correct by symmetry — encoder-decoder invertibility of a
predictor loop should just require both ends apply the same coefficient.
But the decoder doesn't have access to the encoder's analysis output.
The decoder only has what the encoder transmitted through the quantizer,
which is **lossy**. The decoder reconstructs:

```
log₂(M̃_l(0)) = T̃_l + ρ · log₂(M̃_l(−1))_reconstructed
```

where `M̃_l(−1)_reconstructed` is the decoder's own output from last frame
(post-quantization). If the encoder used `M̂_l(−1)_analysis` but the decoder
uses `M̃_l(−1)_reconstructed`, the two diverge by:

```
error(l) = ρ · [log₂(M̂_l(−1))_analysis − log₂(M̃_l(−1))_reconstructed]
```

and this error accumulates via the predictor loop. Over `n` frames the
error grows roughly as `Σ ρ^k × (per-frame quant error)` which, for
`ρ = 0.65` and typical quantization noise, sums to several dB of drift
over 50 frames.

---

## 2. The Closed-Loop Fix

The encoder must run a **matched decoder** internally:

```
/* CLOSED-LOOP — CORRECT. Encoder's prior-frame state matches decoder's. */

/* 1. Quantize current frame's prediction residual */
T̂_l          = log₂(M̂_l(0)) − ρ · log₂(M̃_l(−1))_matched  /* encoder's matched prior */
(b̂_k values) = forward_quantize(T̂_l, ...)

/* 2. Run the matched decoder on the just-emitted b̂_k to produce what
 *    the wire decoder will produce, and store it as the encoder's
 *    prior-frame state for the NEXT frame's prediction.                */
T̃_l                  = inverse_quantize(b̂_k values)
log₂(M̃_l(0))_matched = T̃_l + ρ · log₂(M̃_l(−1))_matched
/* Save M̃_l(0)_matched as M̃_l(−1)_matched for next frame               */
```

The key: the encoder's predictor operates on `M̃_l(−1)_matched`, which
tracks what the **decoder** will reconstruct, not what the encoder
originally analyzed. Every frame, the encoder decodes its own output
and stores that — not its analysis — as the next frame's prior state.

Under this scheme the encoder's `M̃_l(−1)_matched` is identical to
what the wire decoder's `M̃_l(−1)` will be (modulo the two starting
from the same cold-start initialization). No predictor drift.

---

## 3. The Matched Decoder — Minimal Subset

The matched decoder does **not** need to run the full decoder pipeline:

| Decoder spec stage | Matched decoder needs it? |
|-------------------|---------------------------|
| §3 FEC decode | No — encoder has `b̂_k` directly |
| §4.1 pitch recovery | No — already known |
| §4.2 V/UV recovery | No — already known |
| §4.3.1 block layout | **Yes** — Annex N block structure |
| §4.3.2 gain inverse quant | **Yes** — Annex O lookup |
| §4.3.3 PRBA inverse quant | **Yes** — Annex P, Q lookup |
| §4.3.4 HOC inverse quant | **Yes** — Annex R lookup |
| §4.3.5 per-block IDCT + predictor | **Yes** — produces `M̃_l(0)_matched` |
| §4.3.6 spectral enhancement | No — predictor operates on raw M̃ |
| §5 phase regeneration | No — predictor operates on magnitudes |
| §6 voiced synthesis | No |
| §7 unvoiced synthesis | No |

So the matched decoder is roughly §4.3.1–§4.3.5 of the decoder spec.
Reuse those functions directly — do not reimplement them. That way
any fix to the decoder's inverse-quantization is automatically picked
up by the encoder.

---

## 4. State Management

The encoder maintains one prior-frame state array:

```c
struct ambe_encoder_state {
    double M_tilde_prev[56];   /* matched-decoder reconstruction of last frame's M_l.
                                  NOT the encoder's analysis output; it's what the
                                  decoder saw. Initial value 1.0 per decoder cold-start. */
    uint8_t L_prev;             /* L̂ from last frame; initial 30 (matches decoder) */
    /* ... other encoder state ... */
};
```

Cold-start: `M_tilde_prev[l] = 1.0` for all l, `L_prev = 30` (full-rate)
or 15 (half-rate). Matches decoder-side initialization per BABA-A §10
Annex A.

After every frame, *regardless of frame type* (voice, silence, tone):
1. Compute `b̂_k` via forward quantization using `M_tilde_prev` as predictor input
2. Run matched decoder on `b̂_k` → `M_tilde_curr`
3. Set `M_tilde_prev = M_tilde_curr` for next frame

For silence and tone frames, per BABA-A specification the predictor is
**not updated** (the silence/tone dispatch bypasses normal voice
quantization). But `M_tilde_prev` should still be preserved — future
voice frames after silence resume prediction from the last-good
voice-frame state, not reset to cold-start.

---

## 5. Test: Bit-Exact Encoder-Decoder Round-Trip

The strongest diagnostic for closed-loop correctness:

```c
/* Encode a test PCM stream → bits. Decode the same bits through the
 * wire decoder. At each frame, the encoder's M_tilde_prev AFTER
 * that frame must equal the decoder's M̃_l(−1) BEFORE the next frame.  */

for (int frame = 0; frame < N; frame++) {
    /* Encoder side */
    uint8_t bits[9];        /* half-rate */
    encode_frame(pcm, bits, &enc_state);
    double enc_predictor_state[56];
    memcpy(enc_predictor_state, enc_state.M_tilde_prev,
           sizeof enc_predictor_state);

    /* Decoder side — full decode, then check predictor state */
    int16_t pcm_out[160];
    decode_frame(bits, pcm_out, &dec_state);
    double dec_predictor_state[56];
    memcpy(dec_predictor_state, dec_state.M_tilde_prev,
           sizeof dec_predictor_state);

    /* Invariant: these must be bit-identical */
    for (int l = 0; l < 56; l++) {
        if (enc_predictor_state[l] != dec_predictor_state[l]) {
            fail("predictor divergence at frame %d, l=%d: enc=%g dec=%g",
                 frame, l, enc_predictor_state[l], dec_predictor_state[l]);
        }
    }
}
```

Under correct closed-loop feedback, the two predictor states are
bit-identical at every frame boundary (IEEE-754 exact — no tolerance
needed; the matched decoder does the same computation on the same
inputs as the wire decoder).

Divergence at frame 0 → cold-start mismatch.
Divergence appearing at frame N>0 → encoder is updating its predictor
from analysis output instead of matched-decoder output.
Slow drift → floating-point non-determinism between encoder's matched
decoder and wire decoder (shouldn't happen if they share code).

---

## 6. Common Bugs

1. **Encoder stores analysis output as prior-frame state.** The encoder
   computes `M̂_l(0)_analysis` during PCM analysis and uses *that* as
   `M̂_l(−1)` for the next frame. Seems intuitive, wrong per §1.
2. **Encoder skips the matched decoder entirely.** The "open loop"
   predictor of §1 — just uses `M̂_l(−1)_analysis`. Produces plausible-
   looking output that drifts over time.
3. **Encoder runs matched decoder but uses its own enhancement output.**
   Enhancement is a perceptual post-filter applied *after* log-magnitude
   reconstruction. The predictor operates on the pre-enhancement
   magnitudes `M̃_l`, not the post-enhancement `M̄_l`. Running enhancement
   inside the matched decoder and feeding `M̄_l(−1)` to the predictor is
   subtly wrong — it changes the predictor coefficient's effective
   behavior because the enhancement is nonlinear.
4. **Encoder's matched decoder has a different rate configuration than
   the wire decoder.** E.g., encoder dispatches to a silence frame but
   the matched decoder doesn't know about the dispatch and runs
   voice-frame quantization. Keep the matched decoder aware of the same
   frame-type dispatch the encoder performed.

---

## 7. Related

- `ambe_predictor_state_separation.md` — the three predictor instances
  (decoder, encoder, rate converter) each need their own state
- `vocoder_analysis_encoder_addendum.md` §0.6.6 — formal statement
  of the closed-loop rule for the BABA-A analysis encoder
- `DVSI/AMBE-3000/AMBE-3000_Encoder_Implementation_Spec.md` §4.3.5
  and §8.2 — where the encoder spec calls this out as mandatory
- US8595002 col. 20 lines 30–60 — patent source for the closed-loop
  quantization architecture
