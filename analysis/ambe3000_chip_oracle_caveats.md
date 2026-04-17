# AMBE-3000R as a BABA-A Decode Oracle: Chip-Side Caveats

**Category:** AMBE-3000 / DVSI chip behavior / validation methodology
**Relevant Specs:**
- `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §1.10 (enhancement), §1.11 (adaptive smoothing), §1.12 (synthesis)
- `DVSI/AMBE-3000/AMBE-3000_Decoder_Implementation_Spec.md`
- Companion probe evidence: `gap_reports/0001_chip_probe_menu.md` §9
**Date filed:** 2026-04-17

---

## Summary

The DVSI AMBE-3000R P25 full-rate decoder is **not a clean BABA-A
§11 oracle** for synthetic-MbeParams validation. On top of the
spec-prescribed decode pipeline (inverse quantization → inverse
log-magnitude prediction → spectral amplitude enhancement → synthesis),
the chip applies at least three proprietary post-synthesis behaviors
that are not in BABA-A and cannot be disabled via the documented
DCMODE control bits. Implementers calibrating against the chip on
synthetic inputs should be aware of these and design probes that
avoid them.

This note catalogs the three observed chip behaviors, explains why
each defeats a "clean scalar" measurement of spec quantities like
`γ_w` (Eq. 121) or the `M̃_l → PCM` voiced-synthesis scale, and
proposes a probing methodology that works around them.

**TL;DR for the implementer.** Single-harmonic synthetic-MbeParams
decode will not yield clean scalar measurements. Use multi-harmonic
realistic-distribution magnitudes (e.g. drawn from a dequantized real
voice frame) for chip-vs-local comparison, and accept that absolute
amplitude scales can only be recovered to within the chip's
proprietary-processing envelope.

---

## 1. Three observed chip behaviors

All observations from `conformance/chip/src/main.rs` `probe-generate`
/ `probe-compare` runs against an AMBE-3000R on USB-3000
(P25 full-rate, rate 33). Synthetic MbeParams were constructed
per BABA-A §1.12's inputs (`ω̃_0`, `L̃`, `v̂_k`, `M̃_l`), passed
through the chip's decode-PCM path, and compared against a
`blip25-mbe` decoder exercising the same MbeParams.

### 1.1 Amplitude-dependent mute

**Observation.** Single-harmonic voiced MbeParams with `M̃_l`
above a threshold (empirically `~1600` at low `l`,
`~300` for all-unvoiced flat) produce chip PCM output with peak
value `1` for all 20 observed frames — i.e. the chip effectively
mutes the frame.

**Inferred mechanism.** The AMBE-3000 appears to include a
speech-activity classifier that flags "does not look like voice"
input (sustained sinusoid above a loudness threshold) and
suppresses its audio output. The exact threshold is
amplitude-and-harmonic-dependent and not documented in the
AMBE-3000 manual or the US6199037 / US7634399 patents.

**Why it defeats probes.** Any probe designed to push `M̃_l`
through the decoder's linear amplitude regime will hit the mute
threshold and return zero output. The threshold sits below PCM
saturation (peak 32767) and below the chip's internal cap
(§1.2), so there is no amplitude range where a single-harmonic
synthetic input both (a) exceeds noise floor for a meaningful
local-reference comparison and (b) avoids muting.

### 1.2 Internal cap near peak = 10000

**Observation.** At moderate amplitudes below the mute threshold,
chip PCM output clips at approximately `±10000` — well below the
i16 full scale (`±32768`). The clip appears hard rather than
soft-knee.

**Inferred mechanism.** A post-synthesis limiter, either in the
AMBE-3000's digital gain stage or in the internal µ-law companding
prior to its PCM output stage. Some chip datasheets refer to a
"voice band level control" that maps internal ±32768 to a headroom-
reserved output range.

**Why it defeats probes.** Probes that expect `chip_peak ∝ M̃_l` see
the chip peak saturating at 10000 long before `M̃_l` reaches the
mute threshold. The chip's output amplitude is only
locally-proportional to input amplitude in a narrow band below the
cap, and within that band the proportionality is already confounded
by §1.1's detector.

### 1.3 Stationary-signal detector sensitive to `<1%` amplitude jitter

**Observation.** With `jitter_factor = 0.99 / 1.00` alternating
frame-to-frame (to defeat a suspected "same params twice → repeat"
detector), the chip treats odd- and even-indexed frames differently:

- Even-indexed frames (factor `1.00`, unperturbed): pass through
  with expected amplitude scaling up to §1.2's cap.
- Odd-indexed frames (factor `0.99`, 1% down): attenuated relative
  to even frames, sometimes to near-silence.

The asymmetry is not a noise-detector "good/bad frame" flag — both
frame classes have identical `ω̂_0`, `L̂`, `v̂_k`, and within-1%
`M̃_l`. The chip appears to track a very-short-horizon (~2-frame)
copy of the previous frame's spectral envelope and apply a
cross-frame smoothing or frame-repeat-prediction weight when the
current frame's envelope is "too similar" to the previous.

**Inferred mechanism.** Likely an internal variant of BABA-A §1.11
"Adaptive Smoothing and Frame Repeat / Mute", but triggered at a
finer amplitude-jitter threshold than the spec's `ε_R`
bit-error-rate criterion suggests. §1.11 is nominally
*FEC-triggered* (repeat on high error rate); the chip appears to
also apply *envelope-similarity-triggered* smoothing that is not in
the public spec.

**Why it defeats probes.** A probe that constructs N identical
MbeParams frames (the natural way to average out noise) trips the
similarity detector and gets different per-frame output than
expected. A probe that tries to beat the detector with `<1%`
frame-to-frame jitter introduces the odd/even asymmetry above.
Jitter >5% avoids the asymmetry but destroys the
"single-parameter-swept" premise of scalar measurement.

---

## 2. What DCMODE / control registers cannot disable

Per `DVSI/AMBE-3000/AMBE-3000_Decoder_Implementation_Spec.md` §6.3
(DCMODE register layout) and the AMBE-3000 manual, the
documented decode-side control bits are:

| Bit | Field | What it controls |
|-----|-------|------------------|
| various | `LOST_FRAME` | Mark input as erased → force frame-repeat |
| various | `CNI_FRAME`  | Insert comfort-noise on repeat |
| various | `CP_ENABLE`  | Enable / disable some companding path |
| various | `TS_ENABLE`  | Tone-squelch enable |

**None of these expose a "disable internal post-processing" bit.**
The three behaviors above are not switchable from software. The
chip is not advertised as a spec-faithful reference decoder — it is
a production-grade voice codec with proprietary output gating.

---

## 3. Recommended probing methodology

For implementers doing chip-vs-local decoder validation, design
probes that avoid the chip's proprietary gates:

### 3.1 Use multi-harmonic realistic-distribution magnitudes

Instead of hand-constructed single-harmonic MbeParams, draw
`M̃_l` values from the dequantized output of a **real** voice
frame (e.g. dequantize DVSI `tv-rc/clean.bit` at frame N). The
resulting magnitudes are spread across 30–56 harmonics with a
speech-like spectral envelope, which:

- Does not match the chip's "stationary sinusoid" heuristic
  (§1.1) → no mute.
- Peaks below the internal cap naturally because speech spreads
  energy across harmonics rather than concentrating at one bin
  (§1.2) → no clip.
- Changes frame-to-frame in realistic ways (small pitch drift,
  small amplitude variation) so the similarity detector (§1.3)
  does not fire on arbitrary jitter boundaries.

The tradeoff: individual spec quantities (e.g. `γ_w` in
Eq. 121, the single-harmonic factor-of-2 in Eq. 127) are not
isolable in realistic speech. What *is* isolable is the aggregate
ratio `chip_frame_rms / local_frame_rms` — useful for overall
voiced-amplitude calibration (cf. probe §3.C's 1.56× result) but
not for pinning a specific spec scalar.

### 3.2 Use encoder-side probes

The chip's encode path (`PCM → bits`) is less proprietary-gated
than the decode path — the output is the BABA-A bit stream itself,
not post-synthesis PCM. Encoder probes side-step §1.1 / §1.2 / §1.3
entirely. See gap report 0001 §4 for the encode-probe menu.

### 3.3 Treat one-scalar result as partial evidence, not ground truth

Even on realistic inputs, a single-scalar chip measurement should
be interpreted as "consistent with spec" or "inconsistent with
spec" rather than "the spec value must be X". The chip's
processing envelope hides up to a ~2× absolute-scale uncertainty
that no probing methodology known to us can eliminate without
additional DVSI disclosure or reverse-engineering.

---

## 4. What this means for the amplitude investigation

The blip25-mbe decoder reports a `~150×` discrepancy between
`γ_w = 146.64` from BABA-A Eq. 121 and the empirical optimum on
DVSI reference PCM (impl spec §1.12.1, 2026-04-14 commit
`741eeef`). This note **does not** refute that observation — the
reference PCM for that calibration came from DVSI test vectors
(`tv-rc/alert.bit` through the chip), which are real speech, not
synthetic input, and therefore not subject to §1.1 / §1.2 / §1.3
the way synthetic probes are.

However, the `~150×` figure is also not a clean linear
measurement — it includes any chip post-processing that survives
the realistic-input regime. The true BABA-A-spec γ_w may be
anywhere in `[1, 146.64]` range depending on which of the chip's
proprietary stages compound into the observed output. Further
disambiguation requires either:

- Access to the chip's fixed-point reference code (not publicly
  available) to see exactly what post-synthesis processing is
  applied.
- A second, independently-developed BABA-A decoder that faithfully
  implements §1.10 / §1.11 / §1.12 without proprietary additions,
  to triangulate.

Gap report 0001 §3.C provided the first clean chip-vs-local
scalar on realistic input: `1.56×` voiced-amplitude ratio,
consistent with the historical `~1.7×` memory note
(`project_state_2026-04-15`). That number is more trustworthy
than the synthetic-probe γ_w measurements because it survived
the chip's realistic-input regime.

---

## 5. What to do in the implementation spec

**Do not update** §1.10 / §1.11 / §1.12 / §1.12.3 on the basis of
the first-pass probe data. The observations are not clean enough
to support spec edits. The existing calibration note in
implementation-spec §1.12.1 (lines 1225–1246, the "γ_w = 1.0 RMS
error" table) remains the right posture: commit the spec value,
document the mismatch, continue investigating.

**Do reference this note** from any future implementer who tries
to validate decoder amplitude scales against the chip. The probe
tooling under `conformance/chip/` is re-usable; the
methodological conclusions are what change.

---

## 6. Not in scope

- **Reverse-engineering DVSI's post-processing** to identify the
  specific algorithms under §1.1 / §1.2 / §1.3. This would require
  disassembly and is both legally fraught (the AMBE-3000 firmware
  is copyrighted DVSI IP) and likely unnecessary for blip25-mbe's
  primary goal of BABA-A interoperability at the bit level.
- **Encoder-side proprietary behaviors.** The encode path may have
  its own set of proprietary gates (silence detection, tone
  detection, preroll frame content); those are documented
  separately in `vocoder_analysis_encoder_addendum.md` §0.8 and
  in gap report 0001 §4.
- **Half-rate-specific chip behaviors.** This note is P25 full-rate
  (rate 33) only. Half-rate AMBE+2 through the AMBE-3000 may
  exhibit different proprietary behaviors; those would need their
  own probe pass.

---

## 7. References

- Observed probe data: `gap_reports/0001_chip_probe_menu.md` §9
- Chip oracle limitations memory: `project_chip_oracle_investigation_2026-04-16`
- γ_w calibration note: `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §1.12.1 lines 1225–1246
- Chip control-register docs: `DVSI/AMBE-3000/AMBE-3000_Decoder_Implementation_Spec.md` §6.3
- DVSI chip access procedure: memory `reference_dvsi_chip_access`
