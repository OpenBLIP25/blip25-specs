# Gap 0026 — Chip applies a spectral-discontinuity attenuation not in §1.11.3

## Spec location

- TIA-102.BABA-A §1.11.3 "V/UV-and-amplitude smoothing", Eq. 112–116.
- §2.10 inherits §1.11.3 verbatim for half-rate AMBE+2.

## What I need to know

Does the DVSI AMBE+2 decoder (AMBE-3000R) apply an amplitude attenuation
on frames where the spectral envelope changes abruptly *from the previous
frame*, in addition to the spec's τ_M / γ_M magnitude clamp (Eq. 115–116)?

If so, what is the trigger metric (per-bin correlation? cosine similarity?
Δ ω₀? ΔL?) and the attenuation formula?

## What we observe (probe data, 2026-05-14)

Two controlled probes, **zero FEC errors**, AMBE+2 half-rate via PKT_RATET
rate-index 33 on AMBE-3000R at 192.168.1.6:

### Probe A — gain-only spike (gain index `b̂₂` = 8 → 28 at frame 50, back to 8)
Pitch and L unchanged across the spike.

```
frame chip_peak chip_rms ours_peak ours_rms peak_ratio
   49      1603    700        1527    682     1.05
   50      8802   2511        9612   2708     0.92    ← spike
   51      8377   2802        9549   2899     0.88
   52      3545   1401        3725   1366     0.95
```

Chip and ours within ±10% throughout. **No clamp on gain-only spike.**

### Probe B — pitch/L jump (b̂₀ = 30 → 110 at frame 50, back to 30)
L jumps 14 → 49 at frame 50, then back to 14. Gain held constant.

```
frame chip_peak chip_rms ours_peak ours_rms rms_ratio
   49      9644   4564       8853   4245     1.08
   50     10795   3030      10853   4144     0.73    ← jump frame
   51     14999   4216       8553   3026     1.39
   52      9397   4492       8517   4203     1.07
```

At the jump frame the chip's **RMS is 27 % lower than ours** (3030 vs 4144).
Our codec (which faithfully implements Eq. 115–116) does not clamp because
A_M < τ_M (with τ_M = 20480 from the ε_R ≤ 0.005 ∧ ε_T ≤ 6 branch).

### Probe C — spectral *step* (b̂₀ = 30 for f<50, = 110 for f≥50)

The chip's per-frame attenuation is **transient**: the 27 % clamp fires
only at the step frame, then the chip recovers to within ~10 % of ours
over the next ~5 frames as steady-state at the new envelope settles.

## What options exist

1. **Per-bin amplitude lowpass** — chip maintains a per-bin M̄_l running
   estimate and clamps current-frame M̄_l to e.g. min(M̄_l_new,
   K · M̄_l_prev_smoothed) for some K > 1. Plausible because clamp is
   transient and per-bin reshuffling at an L change would naturally drop
   total energy.
2. **Frame-to-frame spectral-similarity gate** — compute correlation
   between current and previous (re-binned) M̄ vectors; if low, scale
   current frame by similarity. Plausible because gain-only spike
   (high similarity) doesn't trigger clamp but L-jump (low similarity)
   does.
3. **Overlap-add crossfade with longer window** — chip's synthesis
   window may straddle frame boundaries more than ours, redistributing
   the jump-frame energy across f=50 and f=51 (our Probe B shows chip
   f=51 peak grows to 14999 vs ours 8553, consistent with this).
4. **§1.11.3 is incomplete** — there is an additional clamp the BABA-A
   text does not document, but appears in DVSI internal notes
   (US8595002/US8315860 disclose split-VQ but not synthesis post-clamp).

Cannot disambiguate from the spec alone — the spec only describes the
single-pole τ_M recurrence with magnitude-based γ_M clamp.

## Why a chip probe is/isn't sufficient

The chip probes above already characterize the *trigger* (spectral
discontinuity, not gain) and *time profile* (transient, single frame).
Identifying the *exact formula* needs:

- Fitting a candidate per-bin lowpass time constant against PCM output
  on a 2-D sweep of (b̂₀_low, b̂₀_high) pairs — feasible, but a lot of
  chip time.
- Reverse-engineering the AMBE-3000R firmware — out of scope.

Spec language clarifying §1.11.3 (or §2.10's inheritance for half-rate)
would short-circuit both.

## Diagnostic evidence

- Real call: `/tmp/call_3537.ambe9` frame 772 (post-Golay-corrected
  ε₀=2): chip peak 10228, our peak 16323 (chip is 63 % of ours, similar
  to the 73 % in Probe B). L jumps 18→31 between f=771 and f=772 in our
  decode trace.
- Probe streams: `/tmp/spike.ambe9` (gain-only) and `/tmp/jump.ambe9` /
  `/tmp/step.ambe9` (pitch+L) are reproducible with
  `cargo run --release --example amplitude_spike_probe` /
  `spectral_jump_probe` / `spectral_step_probe`.

## Implementation status

Our `apply_smoothing` (crates/blip25-mbe/src/codecs/mbe_baseline.rs:308)
implements Eq. 112–116 faithfully. No additional clamp is applied. On
the synthetic probes this is audibly close to chip on gain-only changes
but louder than chip on pitch/L jumps and on real noisy GMRS audio
where Golay-corrected (but spectrally jumpy) frames are common.

## Suggested next step

Spec-author: check whether BABA-A §1.11.3 (or §2.10 commentary, or
§US8315860 noise-suppression section) describes a frame-to-frame
spectral-similarity attenuation. If not, this is an additional
beyond-spec chip behavior — we can either replicate it heuristically
behind a flag (for chip-parity) or document it as a known divergence.
