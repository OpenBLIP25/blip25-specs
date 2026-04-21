# 0001 — Chip-probe menu for §11 / γ_w / M̃_l / D_k bucket-(b) items

**Status:** in-probes (first pass complete; results inconclusive — see §9 below)
**Filed:** 2026-04-16
**Filer:** implementer agent (blip25-mbe)
**For:** spec-author agent
**Related:** `analysis/vocoder_analysis_encoder_gap.md` §§4–6; memory note
`project_diagnostic_decomposition_2026-04-15` (the live "chip-blocked"
inventory); memory note `project_chip_oracle_investigation_2026-04-16`
(chip oracle caveats).

---

## 1. Purpose

The current analysis-encoder conformance residuals against DVSI are now
decomposed into three tightly bounded, chip-blocked items:

1. **M̃_l voiced scale**: 7.6–8.8 dB amplitude floor on L+pitch-matched
   voice frames (`project_diagnostic_decomposition_2026-04-15`). The
   quantizer roundtrip isolates this to the pre-quantizer stage,
   implicating §5 (Eq. 43 magnitude-only estimator) or §11 (synthesis-
   side amplitude rebalancing) or a missing gain scalar applied before
   log-magnitude prediction.
2. **γ_w unvoiced synthesis scale**: ~150× too loud (2026-04-14
   note in `project_state_2026-04-15`). Implicates §1.12 unvoiced
   synthesis normalization, specifically the `w_R`-derived window-energy
   factor.
3. **§11 voiced-phase residual**: not a simple sign flip; already flagged
   as needing chip diagnostics (`project_state_2026-04-15`).

A fourth bucket-(b) item is the **per-band V/UV discriminant D_k**
systematically running ~0.05–0.1 higher than chip (chip-seed override
only improves voicing by +1%). This is in the Eq. 35/36 DFT / `W_R`
numerical domain.

None of these are spec ambiguities we can solve by re-reading the
implementation spec; they all require evidence about DVSI's internal
numerical conventions. This report proposes a menu of chip probes the
spec-author can use to disambiguate.

## 2. Probe constraints

The chip is a black box at the PCM/bits boundary:

- **Decode side** exposes `bits → PCM`. We can construct MbeParams,
  encode through our quantizer, and observe the chip's synthesis PCM.
- **Encode side** exposes `PCM → bits`. We can play controlled audio
  into the chip, recover the encoded bits, and decode them through
  *our* decoder (or inspect bit-field layout directly) to read back
  what MbeParams the chip inferred.
- **No internal state is exposed** — no M̃_l dump, no γ_w readout, no
  per-band D_k, no ξ_max. All evidence is indirect.
- **Stationary-signal suppression** (per `project_chip_oracle_investigation_2026-04-16`):
  the chip attenuates constant-input decodes indefinitely and mutes
  frame 0 of synthetic encoder output. Probes must either (a) vary the
  input across frames, or (b) accept that only decode-side steady-state
  measurements are reliable after a warmup period.
- **Chip rate**: P25 full-rate IMBE, 7200 bps with FEC (already wired
  in `conformance/chip/python/dvsi_driver.py`). Encoding via chip is
  implied by the AMBE-3000R "Rate Converter" designation and the
  docstring at `dvsi_driver.py:184` ("decode/encode"); the Python
  driver currently only implements decode. **Chip encode path would
  need to be wired first** — see §4 below.
- **Probe cost**: each chip roundtrip over the USB-3000 takes
  ≈100–200 ms/frame. A 200-frame scan is a few seconds. Parameter
  sweeps across pitch × L × amplitude × voicing are feasible in the
  minutes-to-tens-of-minutes range.
- **Safety**: PKT_RESET (0x33) is permanently disabled per the
  `feedback_never_send_pkt_reset` memory note.

## 3. Proposed probes — decode side (bits → PCM)

These isolate synthesis behavior. All use hand-constructed MbeParams
fed through our quantizer (shared with the chip), decoded by the chip,
and measured in the chip's output PCM.

Each probe lists: (a) the intermediate we want to measure, (b) the
construction that makes that intermediate dominate the PCM output, (c)
the observable to read from the chip PCM, (d) the expected spec-side
formula the measurement anchors.

### 3.1 γ_w unvoiced synthesis scale (§1.12.3 / Eq. 128?)

- **Measure**: the exact value of γ_w used in unvoiced synthesis.
- **Construction**: MbeParams with all v̂_k = 0 (fully unvoiced),
  L = mid (say 20), pitch = mid (say ω₀ = 2π/50), M̃_l = 1.0 on all
  harmonics. Run 20 frames of the *same* MbeParams to push the chip
  past its cold-start attenuation; measure RMS on frames 10–19 only.
  Vary M̃_l across a log-spaced sweep {0.1, 1.0, 10, 100, 1000} in
  separate 20-frame blocks to find the linear regime and confirm γ_w
  is amplitude-proportional.
- **Observe**: frame-body RMS of chip PCM output (skip the first 6.25 ms
  OLA transient at frame boundaries).
- **Anchor**: PDF §1.12.3 unvoiced synthesis formula — γ_w is the
  scalar that relates the white-noise basis to output amplitude.
  Comparing RMS{chip} / RMS{our decoder at γ_w=1} gives the multiplier
  we are missing.
- **Caveats**: the stationary-signal suppressor will likely catch this.
  Mitigation: alternate between two param sets (e.g. M̃_l = 1.0 and
  0.99) per frame so the input isn't bit-identical but the magnitude
  estimand is well-defined.

### 3.2 M̃_l voiced scale (§1.10 / Eq. 127?)

- **Measure**: the multiplier converting M̃_l (decoder dequantize output)
  to voiced-harmonic PCM amplitude.
- **Construction**: MbeParams with a single voiced harmonic at l = 10,
  M̃_10 = known value, all other harmonics unvoiced with M̃ = 0. Pitch
  chosen so l=10 sits at a clean frequency (e.g. ω₀ = 2π/50 → l=10 at
  1600 Hz). 20-frame block, non-identical inputs per §3.1. Amplitude
  sweep {64, 256, 1024, 4096} to confirm linearity.
- **Observe**: peak PCM amplitude of the l=10 sinusoid in the
  steady-state frames. FFT the output; the peak magnitude at the l·ω₀
  bin is the observable.
- **Anchor**: §1.10 voiced synthesis Eq. 127 is of the form
  `s_v(n) = Σ M̃_l · cos(l·ω₀·n + φ_l)`; peak at bin l = M̃_l (up to
  the phase-coherent scaling we're trying to pin down).
- **Caveats**: phase residual (§1.11) can rotate the peak across the
  two adjacent FFT bins; take `|·|²` sum across both bins rather than
  single-bin read. Run at multiple pitches to rule out bin-alignment
  artifacts.

### 3.3 §11 voiced-phase convention

- **Measure**: the exact phase relationship between successive frames'
  voiced synthesis output.
- **Construction**: two adjacent voiced frames with identical M̃_l and
  identical ω₀, but varying across frames per the stationary-signal
  workaround (e.g. l=10 vs l=11 voiced, rest unvoiced). Both frames
  use the same expected `φ_l(0)` initial phase per §1.11.
- **Observe**: cross-correlation of the two 160-sample frame bodies.
  A pure sine of known frequency has a known inter-frame phase
  advance; the chip's observed phase advance disambiguates the §1.11
  residual sign / offset.
- **Anchor**: §1.11 phase prediction Eq. 119–126. The measurement
  gives one constraint per frame pair; a few pitches suffice to
  resolve sign + offset.

### 3.4 `W_R` normalization cross-check (bridges §0.5 and γ_w)

- **Measure**: whether `W_R(k)` (Eq. 30, used in Eq. 43 denominator
  and cross-referenced to γ_w per addendum §0.2.8) is normalized to
  unit peak, unit energy, or raw Annex-C values.
- **Construction**: single-harmonic voiced frame at l=10, M̃_10 = 1024,
  all else zero/unvoiced. 20-frame block.
- **Observe**: same as §3.2. The ratio `peak_chip / peak_our_synth`
  tells us the ratio of conventions. Repeat with an unvoiced-only
  version (§3.1 construction) — if the ratio matches γ_w's, the fix
  is a shared `W_R` normalization; if it differs, the two scales are
  independent.
- **Anchor**: addendum §0.2.8 already flagged this cross-reference.

## 4. Proposed probes — encode side (PCM → bits)

These require enabling the chip's encoder path first. The AMBE-3000R
manual (§6.* per `reference_dvsi_chip_access`) documents it. Each probe
plays controlled audio into the chip and inspects the 18-byte FEC
frame the chip emits. We decode the bits locally (our decoder is known
to be near bit-exact with DVSI test vectors on FEC-protected fields)
to read chip-inferred MbeParams.

### 4.1 D_k numerical reference

- **Measure**: the chip's per-band V/UV decisions for controlled
  harmonic-mixture inputs, to anchor the Eq. 35/36 D_k computation.
- **Construction**: synthesize PCM at 8 kHz as a pure tone train at
  known ω₀ with known per-harmonic amplitude envelope. For each band k,
  construct a version where band k is purely voiced (pure sinusoids)
  and another where band k is pure white noise modulated by the
  harmonic amplitude envelope. Step k across all 12 bands.
- **Observe**: the v̂_k bits (b̂_2 block in the Eq. 48–50 priority
  assignment) in the chip's emitted frames.
- **Anchor**: a clean-voiced band should always produce v̂_k = 1; a
  clean-unvoiced band should always produce v̂_k = 0. The threshold
  of noise-to-signal ratio at which the chip flips v̂_k gives us the
  effective Θ_ξ(k, ω̂₀) curve — this disambiguates whether our
  implementation's ξ_max trajectory or Θ_ξ mapping is miscalibrated.
- **Caveats**: requires chip encode path wired. Rolling 20-frame
  blocks to avoid stationary-signal suppression.

### 4.2 M̃_l scale (encoder side, complement to §3.2)

- **Measure**: the scalar relating input PCM peak to chip-quantized
  M̃_l, to cross-check against §3.2 decode-side measurement.
- **Construction**: pure single-harmonic PCM input at known frequency
  and peak amplitude. Sweep peak amplitude {1000, 4000, 16000,
  32000} across log-spaced blocks.
- **Observe**: the b̂_3..b̂_{L+2} quantized amplitude bits in the chip's
  output frame, dequantized through our decoder.
- **Anchor**: Eq. 43 voiced spectral amplitude is
  `M̂_l = sqrt(Σ|S_w(m)|² / Σ|W_R(k_m)|²)`. A pure sinusoid at l·ω₀
  gives S_w concentrated at exactly bin m = 64·l·ω₀/2π. The
  chip-observed M̂_l at that input tells us the input-PCM to M̂_l
  scalar. Compare with our analysis encoder's output on identical
  PCM — residual gap is exactly the unknown scalar.
- **Caveats**: chip encoder may internally HPF, AGC, or pre-emphasize.
  Run at multiple frequencies + sweep amplitude to check linearity.

### 4.3 Silence / DTX threshold

- **Measure**: the energy threshold at which the chip switches to the
  silence frame (`project_diagnostic_decomposition_2026-04-15`
  flagged silence dispatch as one of the axes where our decisions
  differ from chip).
- **Construction**: PCM ramp from 0 to full-scale peak over a
  minute-long input. Step through one 20-ms frame at a time.
- **Observe**: look for the IMBE silence frame pattern in the chip's
  output stream. Threshold PCM RMS gives the chip's silence cutoff.
- **Anchor**: addendum §0.8.4 is "honest-but-partial" — the threshold
  is currently a pragmatic guess. Chip data replaces the guess with
  a measurement.

### 4.4 Tone-entry criteria (bonus, low priority)

- **Measure**: whether the chip enters tone frames on DTMF / pure
  sinusoid input, and if so at what frequency/amplitude threshold.
- **Construction**: pure tone input at DTMF frequencies (697, 770, …);
  pure 1 kHz sinusoid; two-tone inputs.
- **Observe**: IMBE tone-frame bit pattern in chip output.
- **Anchor**: addendum §0.8.5 explicitly says tone entry is
  DVSI-black-box. This probe gives us the black-box behavior.

## 5. Chip-probe cost and prerequisites

### 5.1 Already available

- `conformance/chip/python/dvsi_driver.py` — decode-side driver, fully
  working. Probes §3.1–§3.4 can be built on top of it with modest
  extensions (no new packet types; just new scenario generation in
  the existing bisection harness `conformance/chip/src/main.rs`).
- `conformance/chip/src/main.rs` — Rust-side synthetic-MbeParams
  generator; `bisect-generate` already produces 6 scenarios. Adding
  a scenario set parameterized on M̃_l / pitch / voicing pattern is a
  straightforward extension.

### 5.2 Needs implementation before encode-side probes

- **Chip encode path** in `dvsi_driver.py`. AMBE-3000R encode
  packet format is in the manual at
  `/mnt/share/P25-IQ-Samples/P25/AMBE-3000R_manual.pdf` (per
  `reference_dvsi_chip_access` note). Rough shape: SPEECH packet
  (PKT_TYPE_SPEECH = 0x02) with 320-byte PCM payload in the same
  big-endian i16 format used in the response. The chip returns a
  CHANNEL packet with 18 bytes of FEC.
- This is 1–2 hours of driver work, not a spec question.

### 5.3 Not available without additional spec work

- The decode probes in §3 all depend on the spec-author pinning down
  *what* intermediate we are measuring a scalar *against*. E.g.
  §3.1 "γ_w" is only meaningful if the addendum contains the §1.12.3
  formula with γ_w as an explicit named constant in a known position.
  Current decoder spec §1.12 already has γ_w defined (our
  `GAMMA_W` const is imported in the bisection harness), so §3.1 is
  ready to run.
- The §3.2 M̃_l and §3.3 §11 probes depend on §1.10 / §1.11 already
  being in the decoder spec, which they are.

## 6. Suggested probe ordering

Ranked by evidence-per-probe-hour and diagnostic leverage:

1. **§3.1 γ_w unvoiced scale** — highest-confidence probe (fully
   unvoiced frames have no pitch/voicing cascade, so the observable
   is cleanly a single scalar). Decode-side only, no new driver code.
   Gives a single number that may close the ~150× gap.
2. **§3.2 M̃_l voiced scale** — same rig as §3.1, different scenario.
   Closes the 7.6–8.8 dB amplitude gap if it's a missing M̃_l
   multiplier.
3. **§3.4 `W_R` normalization cross-check** — cheap (reuses §3.1/§3.2
   scenarios). Tells us whether γ_w and M̃_l share a root cause.
4. **§4.3 silence threshold** — requires encode path but is a single
   scalar. Worth doing whenever encode path lands.
5. **§4.1 D_k reference** — biggest probe (band × amplitude sweep).
   Defer until γ_w/M̃_l resolve; some of the D_k residual may be
   downstream of the same `W_R` convention error.
6. **§3.3 §11 phase** — defer; phase residual is qualitatively
   "not a simple sign flip" per memory, and the measurement is
   noisier than the amplitude ones. Worth running after amplitudes
   are calibrated so the comparison isn't confounded.
7. **§4.2 M̃_l scale encode-side** — redundant with §3.2 if the chip
   is symmetric; run as a cross-check if §3.2 gives ambiguous results.
8. **§4.4 tone entry** — lowest priority; already deferred in the
   addendum per §0.8.5.

## 7. Spec-author action requested

This report is asking the spec-author to:

1. **Confirm** (from PDF §1.10, §1.11, §1.12, and the full text) which
   of the decode-side intermediates (γ_w, M̃_l gain, §11 phase
   convention) are expressed as single scalars in the standard versus
   embedded in a larger expression where the scalar can't be isolated
   by PCM observation. This shapes whether the decode probes (§3) will
   produce useful numbers or just composite ratios.
2. **Identify** whether any of the listed probes is unnecessary
   because the answer is already in the PDF (e.g. if §1.12.3 already
   gives γ_w as a named numeric constant that we simply mis-transcribed
   in decoder code — in which case the chip probe is wasted effort).
3. **Flag** any additional probes that would be valuable for the
   §11 / γ_w / M̃_l / D_k area that the implementer (who doesn't have
   PDF access) hasn't thought of.
4. **Optionally prioritize** the probes if the spec-author has a
   strong prior on which intermediate is most likely to be the actual
   bug source.

The implementer will build and run the probes the spec-author greenlights.
Probes §3.1 / §3.2 / §3.4 can run today against the existing
decode-only driver; §4.x probes need the ~1–2 hour encode-path driver
work first.

## 8. Why chip observations aren't just "read the PDF harder"

Per `project_diagnostic_decomposition_2026-04-15`, all four gap-report
"Known limits" callouts from the analysis addendum have already been
spec-audited clean (§0.2/§0.4/§0.5/§0.7). The addendum faithfully
transcribes Eq. 5–45 per PDF. The residuals that remain are below
the transcription level: they're either

- a DVSI-specific numerical convention that the PDF states in a
  context-dependent way (e.g. `W_R` normalization being implicit in
  §1.10 / §5.3 without a single "`W_R` is normalized to X" line), or
- a DVSI-specific implementation choice that the PDF permits but
  doesn't constrain (e.g. the silence-entry threshold), or
- a chip-proprietary behavior layered on the spec (silence detection,
  stationary-signal suppression) that isn't in the spec at all.

In all three cases the chip is the more authoritative source than
further PDF-reading. The project's explicit goal (blip25-mbe CLAUDE.md
§"DVSI emulation target") is DVSI equivalence, not PDF equivalence, so
chip observations are the correctness target the implementer needs
anyway.

## 9. Results — first pass (2026-04-17)

Probes §3.1 / §3.B / §3.C / §3.2 ran against the USB-3000 / AMBE-3000R
on `pve`. Harness: `conformance/chip` `probe-generate` + `probe-compare`
subcommands (new), 12 scenarios total, 20 frames each, chip decode via
`dvsi decode` Python driver.

### 9.1 Headline: chip-oracle limitations are worse than anticipated

The chip applies **aggressive proprietary post-synthesis gating** that
makes clean linear-fit measurements of γ_w / M̃_l impossible on
synthetic inputs. Three distinct chip behaviors observed:

1. **Amplitude-dependent mute.** Single-harmonic voiced inputs above a
   narrow amplitude window (roughly `M̃_l > 300` unvoiced,
   `M̃_l > 1600` voiced-low-l) produce chip PCM peak=1 for all 20
   frames. The chip flags them as non-speech and suppresses the output.
2. **Internal cap near peak=10000.** At moderate amplitudes the chip
   output clips at ±10000, well below i16 max. This is a DVSI-internal
   amplitude limit, not full-range PCM.
3. **Stationary-signal detector sensitive to < 1% amplitude jitter.**
   With `jitter_factor = 0.99 / 1.00` alternating frame-to-frame to
   defeat the detector, the chip treats odd vs even frames
   asymmetrically — odd-index (factor 0.99) frames get suppressed,
   even-index (1.00) frames pass through. Bigger jitter would help but
   destroys the clean-scalar-measurement premise.

Per memory `project_chip_oracle_investigation_2026-04-16`, this was
anticipated to some degree. The degree is higher than anticipated.

### 9.2 §3.1 γ_w — non-monotonic with amplitude

All-unvoiced flat M̃_l at ω₀=0.1404, L=44. Amplitudes {10, 30, 100, 300}:

| amp | local peak (max/block) | chip peak (max/block) | ratio c/l |
|----:|-----------------------:|----------------------:|----------:|
|  10 |                    289 |                  2475 |      8.56 |
|  30 |                    830 |                   509 |      0.61 |
| 100 |                  2740 |                 10000 |      3.65 |
| 300 |                  7576 |                     1 |      0.00 |
|  — | | | |

The ratio is non-monotonic and the 300 case is fully suppressed. **No
clean γ_w scalar recoverable from this data.** Chip is doing
amplitude-dependent processing beyond spec §1.12.3.

Odd/even frame analysis on amp=10 (see full per-frame peak trace in
session log): even-index frames peak at 1125/1546/1621/…/2475 (ramping
up), odd-index frames at 53/91/144/189/256 (different ramp). The
chip distinguishes M̃_l=10 and M̃_l=9.9 as different kinds of input.

### 9.3 §3.B §1.10 bypass — chip saturates before the test point

Same ω₀=0.1404, L=44. lowL: l=2 (§1.10 identity). highL: l=15
(§1.10 fires). Amplitudes {100, 400}:

| config       | amp | local peak | chip peak (raw) | note |
|-------------|----:|-----------:|----------------:|------|
| lowL  (l=2)  | 100 |        138 |           32768 | chip saturates (i16 max) |
| highL (l=15) | 100 |         75 |            4607 | ~60× our local |
| lowL  (l=2)  | 400 |        188 |           32768 | chip still saturated |
| highL (l=15) | 400 |         93 |            4567 | amplitude-independent?! |

Two confounds make this probe inconclusive:

1. **lowL saturates at the i16 rail for both amp=100 and amp=400.** We
   cannot tell if chip's true (uncapped) output scales with amplitude
   or is internally limited.
2. **highL output is nearly identical at amp=100 and amp=400** (4607
   vs 4567). The chip's output for high-l single-harmonic is
   amplitude-insensitive across a 4× range. This is unexpected —
   suggests chip applies some AGC / normalization specific to high-l
   single-harmonic content, independent of whether §1.10 is doing its
   energy-preserving rescale.

**Tentative conclusion:** spec-author's prediction was that lowL and
highL should produce identical chip peak if §1.10 is energy-preserving.
We can't test that because the chip is saturating lowL and
amplitude-normalizing highL. The probe needs a redesign to get into
the chip's linear regime — possibly using *multi-harmonic* voiced
signals where the chip doesn't see "stationary sinusoid" as
suppressible.

### 9.4 §3.C predictor-state verification — post-task-#6 clean

Silence → voiced-flat-500 (the same scenario that pre-task-#6 showed
catastrophic M̃_l divergence). After Proposal B (quantizer predictor
state fix, commit pending), the round-trip is clean.

| metric | pre-task-#6 (memory) | post-task-#6 |
|---|---:|---:|
| Local frame-2 M̃_rt RMS | ~1.2 | ~389 |
| Chip peak / local peak | (not measured) | **1.56×** |

The 1.56× chip/local ratio is a clean measurement — single number,
consistent across the block. Matches the 1.7× memory note for voiced
amplitude error (`project_state_2026-04-15`). **§3.C passes**:
the predictor fix is working as intended; the remaining gap is
the ~1.56× voiced-amplitude mismatch already catalogued as γ_w/§11/M̃_l
territory.

### 9.5 §3.2 M̃_l voiced scale — partial signal

l=2, L=20 (`8·l=16 ≤ L` so §1.10 identity). Amplitudes {100, 400, 1600}:

| amp  | local peak | chip peak | ratio c/l |
|-----:|-----------:|----------:|----------:|
|  100 |        212 |     11084 |     52.28 |
|  400 |        649 |      3602 |      5.55 |
| 1600 |       1306 |      1643 |      1.26 |

Ratio **decreases monotonically** as amplitude increases — classic
chip-side compression signature. Ideal M̃_l scalar cannot be extracted
from this without knowing the chip's compression curve. Worth noting:
at amp=1600 the chip output (1643) is very close to 2·amp = 3200 /
2 = 1600, which would match Eq. 127's factor-of-2 if the chip applied
±1 harmonic-only and divided by 2 (single harmonic spread across a
±50% overlap window). But the compression makes this circumstantial.

### 9.6 What we did learn

Despite the noisy absolute measurements, one thing is now firm:

- **§3.C is clean post-task-#6**: predictor-state fix is verified
  against chip behavior on a realistic scenario.
- **The 1.56× voiced-amplitude ratio in §3.C is our best chip-vs-local
  single-scalar measurement** to date. Matches the "1.7× too loud"
  historical note on voiced synthesis.
- **Chip is not a clean oracle for single-harmonic synthetic inputs.**
  Future probing should use multi-harmonic realistic magnitudes (closer
  to real speech) where the chip's noise-detector / stationary-signal
  suppressor doesn't kick in.

### 9.7 Recommendations to spec-author

1. **Don't update §1.12.3 γ_w on this probe data.** The measurement is
   not linear enough to trust a scalar fit.
2. **Don't update §1.10 on §3.B data.** Probe saturated before it
   could test the hypothesis.
3. **Do consider updating `analysis/vocoder_decode_disambiguations.md`
   or adding an `analysis/ambe3000_chip_oracle_caveats.md`** documenting
   the three proprietary behaviors (amplitude-dependent mute, ±10000
   internal cap, jitter-sensitive stationary-signal detector). This is
   important context for any future implementer doing chip-based
   validation.
4. **Probe §3.C result is usable**: the 1.56× voiced-amplitude chip/local
   ratio is consistent with the existing §1.12 γ_w note (commit
   `741eeef`). This is the first independent chip measurement of the
   voiced-amplitude gap; previous measurements were
   encoder-output-compared-to-chip-reencode, which is confounded by
   state trajectory.

### 9.8 Next-probe ideas (deferred; escalating to user)

1. **Multi-harmonic realistic-distribution probe** — instead of
   single-harmonic synthetic, use M̃_l values drawn from a real speech
   frame's dequantize output (from `tv-rc/clean.bit` frame N). The
   chip won't flag those as "stationary single sinusoid" so suppression
   should not trigger.
2. **Chip encode-path probe** (§4.x in this gap report) — feed clean
   reference PCM through the chip encoder, observe the quantized bits,
   compare with our analysis encoder's bits on the same PCM. This
   side-steps the decoder-side processing.
3. **Chip DCMODE control bits** — per spec-author's AMBE-3000 protocol
   spec §6.3, decoder-side only exposes `LOST_FRAME`/`CNI_FRAME`/
   `CP_ENABLE`/`TS_ENABLE` — none of which disable the internal
   amplitude-dependent processing. So there's no "disable post-proc"
   option to pull.

Escalating to user for direction on whether to pursue (1) or (2) next,
or mark this gap-report series as "chip-oracle unreliable, use
PCM-level tv-rc baseline as ground truth" and defer the γ_w / M̃_l
absolute-scale questions entirely.
