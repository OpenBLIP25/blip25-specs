# AMBE Predictor State Separation Invariant

**Scope:** The three AMBE pipeline roles — decoder, encoder, rate converter —
each maintain a prior-frame log-magnitude predictor `M̃_l(−1)`. Although the
coefficients often coincide numerically (half-rate intra-rate = 0.65,
rate-converter cross-rate = 0.65), the **state instances must be kept
independent**. Sharing state across roles produces AR-loop instability and
drifts output quality over ~50 frames (~1 second of speech).

**Status:** Structural invariant, documented here for cross-session reference.
Not empirically derived — follows directly from US7634399 col. 7 and the
mathematical structure of the three predictors.

**Date:** 2026-04-16

---

## 1. The Three Predictors

| Role | Coefficient | Update rule | State instance |
|------|-------------|-------------|----------------|
| Decoder intra-rate (half-rate) | 0.65 literal (BABA-A Eq. 185 page 65) | T̃_l + 0.65·(interp from prior frame's M̃_l(−1)) + correction terms | `dec.M̃_l(−1)` |
| Decoder intra-rate (full-rate) | ρ per Eq. 55 schedule (0.4–0.7 range, L̂-dependent) | T̃_l + ρ·(interp from prior frame's M̃_l(−1)) + correction terms | `dec.M̃_l(−1)` (same slot, coefficient varies frame-to-frame) |
| Encoder intra-rate (matched-decoder feedback) | Same as decoder at the encoding rate | T̂_l residual → forward quantize → dequantize → reconstruct = encoder's predicted M̃_l(0) for its own `M̃_l(−1)` | `enc.M̃_l(−1)` |
| Rate converter cross-rate | 0.65 literal (US7634399 col. 7) | Resample prior-frame magnitudes onto current target grid via R-ratio interpolation, then apply 0.65 coefficient | `rc.M̃_l(−1)` |

Three distinct storage locations. Even when the coefficient matches
numerically (half-rate decoder and rate converter both use 0.65), the
predictor-state arrays track different quantities.

---

## 2. Why Sharing State Breaks

The decoder and encoder at the same rate *can* in principle converge their
predictor states because the encoder runs a matched decoder (closed-loop
predictor feedback, see [ambe_encoder_closed_loop_predictor.md](ambe_encoder_closed_loop_predictor.md)).
If the encoder's matched decoder is bit-identical to the wire decoder and
both start from the same cold-start conditions, their `M̃_l(−1)` states
evolve identically.

**But** the rate converter's state cannot be shared with either, because:

1. **Different input data.** The rate converter operates on
   *post-transformation* magnitudes after §4 of the rate-converter spec
   resamples onto the target grid. These differ from the source decoder's
   magnitudes (different fundamental frequency) and from the target
   encoder's direct PCM analysis output (different because the source
   magnitudes went through quantization noise first).
2. **Different update rule.** The rate converter resamples prior-frame
   magnitudes onto the current target grid via linear interpolation
   before applying the coefficient. Intra-rate predictors apply the
   coefficient directly to same-L̂-gridded prior magnitudes with no
   resampling.
3. **Different coefficient in full-rate case.** When target is full-rate,
   the encoder's intra-rate ρ follows Eq. 55 (L̂-dependent). The rate
   converter uses literal 0.65. Sharing state would mean either
   mis-applying Eq. 55 values to rate-converter updates or mis-applying
   0.65 to encoder updates.

Sharing state produces a first-order AR loop with ρ_effective = ρ_1 + ρ_2
where the two predictors are inadvertently cascaded through the shared
state. If `ρ_effective` exceeds 1.0 the loop is unstable. Even at
marginal values (`ρ_effective` near 1.0) the predictor becomes a
near-integrator, amplifying numerical noise over ~50 frames until output
magnitudes drift off the quantization grid.

---

## 3. Observable Symptom

Shared predictor state is hardest to catch in unit testing because a
single frame decoded correctly doesn't reveal it. The symptom appears
as **progressive quality degradation over sustained voicing**:

- Frame 1–10: output sounds correct
- Frame 10–30: spectral envelopes start drifting (perceived as
  timbre shift)
- Frame 30–50: harmonics audibly wobble; "underwater" or
  "phase-rotation" quality
- Frame 50+: output may become unstable or mute

The drift rate depends on how close `ρ_effective` is to 1.0. At 0.65 +
0.65 = 1.30 the loop is strongly unstable and drift is fast. At 0.65 +
0.4 = 1.05 the drift is slow but still accumulates.

---

## 4. Test: Sustained Sinusoid Round-Trip

The cheapest diagnostic is a sustained-signal round-trip:

```c
/* Feed a pure sinusoid at f_0 = 200 Hz through encode → decode for
 * 100 consecutive frames (2 seconds). Expected: output PCM should
 * be a stable sinusoid with <3 dB envelope variation after the
 * first 5 frames (cold-start transient).                              */
int16_t pcm_in[160];
generate_sinusoid(pcm_in, 200.0 /* Hz */, 32000 /* Q15 peak */);

for (int frame = 0; frame < 100; frame++) {
    uint8_t bits[18];
    encode(pcm_in, bits);   /* 144 bits for full-rate */
    int16_t pcm_out[160];
    decode(bits, pcm_out);

    double rms = compute_rms(pcm_out);
    if (frame >= 5 && fabs(rms - stable_rms) > 0.5 * stable_rms) {
        /* More than ±3 dB drift from frame 5's RMS → predictor leak */
        fail("predictor state drift at frame %d: rms=%.1f vs stable=%.1f",
             frame, rms, stable_rms);
    }
}
```

Under correctly-separated predictor state, `rms` is constant after
frame 5 (cold-start settling). Under shared state, `rms` drifts or
oscillates.

For the rate converter, the analogous test feeds a sustained sinusoid
through the converter for 100 frames and measures output-bit stability
(the sinusoid should produce repetitive or near-repetitive output bit
patterns after settling).

---

## 5. Storage Layout

Three distinct state blocks in a composed pipeline. The AMBE-3000 rate
converter spec §7 calls this out explicitly — quoting:

> Three predictor pipelines run in parallel:
> (decoder's `M̃_l_A` predictor state)
> (rate converter's `M̃_l_B` predictor state, ρ = 0.65 + grid-resample)
> (encoder's `M̃_l_B` predictor state, intra-rate, same-grid update)

The storage footprint is small: 56 doubles each (maximum L̂) × 3 = 168
doubles = 1.3 KB per converter instance. Memory is not the concern;
state separation is.

Do not optimize by "reusing" one array across roles, even if a particular
implementation happens to run only one role per frame boundary.

---

## 6. Related

- `ambe_encoder_closed_loop_predictor.md` — the encoder's matched-decoder
  feedback mechanism that keeps the encoder's predictor state aligned
  with what the wire decoder will see
- `vocoder_decode_disambiguations.md` §3 — full-rate ρ schedule (Eq. 55)
  vs half-rate 0.65 literal (Eq. 185)
- `DVSI/AMBE-3000/AMBE-3000_Rate_Converter_Implementation_Spec.md` §4.5
  and §5 — cross-rate predictor design
- US7634399 col. 7 lines 5–25 — patent source for the rate-converter
  ρ = 0.65 and the state-separation warning
