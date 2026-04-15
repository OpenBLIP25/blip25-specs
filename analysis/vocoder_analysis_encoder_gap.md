# BABA-A Analysis Encoder (PCM → MbeParams): Implementation-Spec Gap

**Category:** Vocoder / Spec Gap Report
**Relevant Specs:** TIA-102.BABA-A (§§2–7 of the standard, i.e. analysis + full-rate
encoding; PDF pages roughly 5–22)
**Status at filing:** blocks the blip25-mbe "analysis encoder" work item
**Date filed:** 2026-04-15

---

## Summary

The derived implementation spec
`standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` is currently
**decode-only**. Its §§1.1–1.13 cover dequantize → enhance → smooth →
synthesize for full-rate; §§2.1–2.13 do the same for half-rate; §§3–6 handle
frame placement, FEC pipelines, tone/silence, and the MBE parameter
interface. The forward path PCM → MbeParams — i.e. what the implementation
needs to turn analog audio into quantizer inputs — is not rendered as derived,
code-ready material anywhere in the spec.

The only analysis-side artifacts present today are the annex tables:

- **Annex B** analysis window `w_I(n)` — 301 values, extracted to
  `annex_tables/annex_b_analysis_window.csv`
- **Annex C** pitch refinement window `w_R(n)` — 221 values, extracted to
  `annex_tables/annex_c_pitch_refinement_window.csv`
- **Annex D** FIR LPF coefficients `h_LPF(n)` — 21 values, inlined at
  implementation-spec §7.4

That's the window/filter data, not the algorithms.

The `TIA-102-BABA-A_Full_Text.md` extraction paraphrases BABA-A §§2–5 but
spells out only a small handful of equations (Eq. 31, 34, and a one-line
form of Eq. 32–33 lacking the `S_w(m, l)` basis). Equations 5–9 (initial
pitch autocorrelation), 24–33 (pitch refinement + spectral amplitude
estimation), and 35–42 (V/UV discriminant and θ_G threshold) are *cited but
not transcribed*. Re-running the extraction pipeline will not close the
gap; the PDF pages have to be worked directly.

This matches the project CLAUDE.md policy that blip25-mbe must source code
from the derived implementation spec, not from the full-text extract and
not from open-source P25 projects. The gap has to be closed in the
implementation spec before analysis-side code can be written.

---

## What the analysis encoder must produce

The wire encoders (`p25_fullrate::quantize`, `p25_halfrate::quantize`, and
`dvsi_3000::quantize`) already consume `MbeParams`. The analysis encoder's
job is to take 20 ms of 8 kHz 16-bit PCM (plus 80 ms of algorithmic
look-ahead / look-back per BABA-A §1.2) and produce one `MbeParams`:

- `ω̂₀` — fundamental frequency, radians/sample, in the full-rate encoder
  range `[2π/123.125, 2π/19.875]`
- `L̂` — harmonic count, derived from `ω̂₀` via Eq. 31
- `v̂_k` — per-band voicing, k = 1..K̂ (K̂ via Eq. 34)
- `M̂_l` — per-harmonic spectral amplitudes, l = 1..L̂

Plus the frame-type dispatch the wires expect:

- **Tone-frame detection** — when to emit a tone frame instead of a voice
  frame (relevant to half-rate / `p25_halfrate::tone`; full-rate has no
  dedicated tone slot per implementation spec §5.4).
- **Silence detection** — when to emit the canonical silence frame
  (implementation spec §6).
- **Bad-input / preroll handling** — first few frames before the lookahead
  buffer is full.

The pipeline ends at `MbeParams`. Wire-side block-DCT / gain / HOC / PRBA
quantization lives in the existing `quantize::` modules; the analysis
encoder does not own them, but it does have to produce the log-magnitude
prediction residual that feeds the quantizer — see §4 below.

---

## Gap inventory

What follows is a section-by-section walk of what is missing, with PDF
pointers. Each entry lists the specific questions an addendum must answer
before Rust can be written against it. The cross-cutting structure of the
needed addendum is proposed in the final section.

### 1. Input framing and windowing (BABA-A §§2.1–2.3, pages ~5–7)

**What the full text says:** 8 kHz 16-bit PCM, 20 ms (160-sample) frames,
Annex B window `w_I(n)` applied, 256-point DFT computed.

**What the implementation spec needs:**

- Frame timing relative to "current frame" index — i.e. which 321 samples
  does `w_I(n)` multiply, and how is that aligned with the 160 samples
  the frame represents in output? The standard's 80 ms algorithmic delay
  breaks into "73.75 ms analysis lookahead + 6.25 ms synthesis" per §1.2;
  the lookahead number suggests the analysis window is centered ~3.5
  frames ahead of the output frame, but this has to be stated exactly.
- The 256-point DFT indexing convention. §2.3 shows `S(m) = Σ_{n=-105}^{105}
  s(n)·w_A(n)·e^{−j2πmn/256}` — but `w_A` is not otherwise defined in the
  full-text paraphrase; is `w_A = w_I` (Annex B, support n=−150..150) with
  zero-padding outside n=±105, or is it a truncated version? The paraphrase
  note in §2.2 implies zero-padding at the edges, but the implementation
  spec should state it as equations, not as prose.
- Whether the initial-pitch `E(P)` autocorrelation uses the same windowed
  spectrum or a separate LPF'd time-domain sequence (Annex D is 21 taps).
  Eq. 5 versus Eq. 25–28 resolves this.

### 2. Initial pitch estimation (BABA-A §3 / Eq. 5–9, pages ~8–11)

**What the full text says:** autocorrelation `E(P)` via LPF, scan `P` from
21 to 122 in 0.5-sample steps, look-back tracking with threshold `0.8·P̂_{−4}`
and composite error `CE_B = E(P̂_B) + E_{−4}(P̂_{−4}) + E_{−2}(P̂_{−2})`,
look-ahead tracking with three-frame composite `CE_F` and cascade thresholds
`0.85 / 0.4 / 0.05`, final selection `P̂ = P̂_B` if `CE_B ≤ 0.48 or CE_B ≤ CE_F`.

**What the implementation spec needs:**

- **Equation 5–9** written out. The paraphrase says "autocorrelation via FIR
  low-pass" but the actual formula for `E(P)` at non-integer `P`
  (0.5-sample resolution) is not in hand. BABA-A Eq. 5 presumably defines
  `r(P)` as an autocorrelation of `h_LPF * s`; Eq. 6–9 presumably define
  the interpolation/windowing around it. These need to be transcribed.
- The weighting factors on look-back and look-ahead (`E_{−4}`, `E_{−2}`,
  `E_1`, `E_2`) — are they plain values of `E` evaluated at past/future
  pitch candidates, or are they renormalized? The paraphrase is
  ambiguous.
- The cascade of thresholds in the look-ahead path (`CE_F ≤ 0.85 and ≤
  1.7·CE_F(P̂₀)`, etc.) needs its precise acceptance predicate — is it a
  disjunction across the three rows, or does each row gate a different
  candidate `P_F`?
- How the ratios `0.48`, `0.85`, `0.4`, `0.05`, `0.8`, `1.2`, `1.7`, `3.5`
  are applied when `E` is zero (silence / early startup): divisions blow
  up; the spec needs a guard convention.
- **Initial conditions.** On cold start there is no `P̂_{−4}` / `P̂_{−2}`
  and no `E_1` / `E_2` lookahead. BABA-A §10 (Annex A initialization) may
  address this for synthesis state but is silent on analysis-side state;
  the addendum needs explicit init values and a "preroll" policy for the
  first ~4 frames.

### Critical path: the `S_w(m, l)` basis function

Before walking the individual sections: the single most load-bearing
item in this gap is the harmonic basis `S_w(m, l)` from BABA-A Eq. 25–28.
It is referenced by §3 (pitch refinement residual), §5 (spectral amplitude
estimator), and §4 (V/UV discriminant, which compares per-harmonic
projections against the raw spectrum) simultaneously. Transcribing this
one equation block — including window source (`w_R(n)` from Annex C),
integration limits, phase convention, and `2/256` normalization — unblocks
roughly 60% of the addendum. Suggest making it §0.2 of the addendum and
landing it first.

### 3. Pitch refinement (BABA-A §3.2 / Eq. 24–33, pages ~11–14)

**What the full text says:** refine over `[P̂ − 1.125, P̂ + 1.125]` in
0.25-sample steps; for each candidate compute `S_w(m, ω₀)` and the residual
`E_R(ω₀)` via Eq. 24–28; pick the `ω̂₀` minimizing `E_R`; and then extract
`â_l`, `b̂_l` via Eq. 32–33 at the selected `ω̂₀`.

**What the implementation spec needs:**

- **The basis function `S_w(m, l)` from Eq. 25–28.** This is the
  harmonically-placed analysis basis that appears in both the refinement
  residual and the amplitude extractor. Without it the rest of §3 and all
  of §5 are unimplementable. From Annex C we have the window `w_R(n)`
  (221 values, n=−110..110); presumably `S_w(m, l)` is the 256-point DFT
  of `w_R(n)·e^{j·l·ω₀·n}` but the phase convention, the integration
  limits, and the normalization (`2/256` shows up in Eq. 32–33; is it
  present in Eq. 25–28?) all need to be stated.
- **The residual `E_R(ω₀)` (Eq. 24).** Its form dictates whether the
  estimator is a mean-squared-error minimization in the 256-bin spectral
  domain, a per-harmonic sum of squared residuals, or a `Σ|S(m)|² − Σ(â_l²
  + b̂_l²)·|S_w(m,l)|²` style decomposition. Different forms of Eq. 24
  give numerically different `ω̂₀` near local minima.
- **Harmonic cut-off.** How many harmonics does the residual sum over —
  the L derived from the candidate `ω₀`, or a fixed upper bin like m=122
  (from the §2.4 autocorrelation upper limit)?
- **Tie-breaking.** Eight candidates in the `±1.125 / 0.25` grid; if two
  tie on `E_R`, which is chosen? The decoder's quantizer step is
  half-sample pitch resolution (8-bit `b̂₀` from Eq. 45), so tie-breaking
  matters for bit-exactness against DVSI.
- **Range clamp.** §1.3.1 of the implementation spec says the encoder
  restricts `ω̂₀` to `[2π/123.125, 2π/19.875]` (i.e. `b̂₀ ∈ [0, 207]`). If
  the refined `ω̂₀` falls outside this range after minimization, what is
  the encoder's behavior — clamp, reject the frame, or emit silence?

### 4. Log-magnitude prediction residual (encoder side of Eq. 75–79)

**What the implementation spec has (decoder side):** §1.8.5 "Inverse
Log-Magnitude Prediction" and §2.13 for half-rate render the decoder's
`R̃_l = T̃_l + ρ·R̃_l(−1)` direction with `ρ = 0.65` (per
`vocoder_decode_disambiguations.md §3`).

**What the implementation spec needs (encoder side):**

- **`Ê_l = R̂_l − f(R̃_l(−1))` form.** §5.3 of the full text writes this as
  `Ê_l = R̂_l − R̃_l(−1)` (no `ρ`), but that is incompatible with the
  decoder's `R̃_l = T̃_l + 0.65·R̃_l(−1)` identity; either the encoder's
  predictor also includes `ρ = 0.65` (most likely) or the notation in §5.3
  silently absorbs it into `R̃_l(−1)`. The addendum has to pick one and
  back it with the PDF equation number.
- **L-mismatch handling.** When `L̂ ≠ L̃(−1)` the previous-frame vector
  has a different length than the current. The decoder deals with this via
  a re-indexing rule (see `vocoder_decode_disambiguations.md §7`); the
  encoder needs the symmetric rule. The spec mentions "Annex J block
  lengths" but not the per-frame `L̂ ↔ L̃(−1)` interpolation.
- **`R̃_l(−1)` source.** At the encoder, the previous frame's `R̃_l` is
  conceptually the decoder's reconstruction of its own prior frame. Is the
  encoder expected to run a matched decoder internally (so quantization
  noise is in the predictor loop)? The standard appears to imply yes
  (closed-loop AR predictor), but the spec currently says nothing.
- **Cold start.** First-frame `R̃_l(−1)` is undefined. Annex A §10 of the
  implementation spec initializes synthesizer state; the analysis-side
  equivalent (zero vector? silence-frame `R̃`?) is not stated.

### 5. V/UV determination (BABA-A §4 / Eq. 35–42, pages ~14–17)

**What the full text says:** per-band discriminant `D_k` compares voiced
energy to unvoiced noise floor; auxiliary metrics `ξ_LF`, `ξ_HF`, `ξ_G`,
`ξ_max`; mapping `M(ξ)`; decision via threshold `θ_G(k, ω̂₀)`.

**What the implementation spec needs:**

- **`D_k` formula (Eq. 35 or 36).** The discriminant is the core of V/UV.
  Without it we cannot compute a voicing decision at all. The likely form
  is `D_k = Σ_{m ∈ band_k} |S(m) − â_l·S_w(m,l)|² / Σ_{m ∈ band_k} |S(m)|²`
  but the exact limits, the per-harmonic amplitude source (whether `â_l`
  is from Eq. 32 directly or from a V-hypothesis re-estimation), and
  the band endpoint bin indexing all need to be transcribed from the PDF.
- **θ_G(k, ω̂₀) threshold table or closed form (Eq. 37).** The threshold
  is a function of both band index and pitch. Possibilities: (a) a
  closed-form expression involving `k·ω̂₀`; (b) a lookup table indexed by
  quantized band center and pitch bin; (c) a polynomial fit. BABA-A
  contains the formula, but none of it is in our spec today.
- **ξ-family metrics (Eq. 38–42).** `ξ_LF`, `ξ_HF`, `ξ_G`, `ξ_max`, `M(ξ)`
  — their role (pre-threshold scaling? noise-floor tracking across frames?
  speech-activity gating?) and their exact formulas.
- **Cross-frame state for `ξ_max`.** §4.2 says "updated via Eq. 41"; the
  update is stateful, so the encoder state structure needs a field for it
  plus an Annex-A-style init value. Decay constants (if any) belong in
  the Rust type.
- **Aggregate band K̂.** §4.3 says the aggregate (partial) band is
  handled "similarly"; the spec should spell out whether the partial band
  uses the same `D_k` with a truncated frequency range or a different
  formula.
- **Half-rate codebook mapping.** Half-rate quantizes V/UV via the
  Annex M codebook (32 patterns). The analysis encoder produces per-band
  bits but the half-rate wire needs the codebook index. This mapping is
  currently documented in the half-rate wire's `quantize::` module — the
  analysis encoder can deliver per-band bits and let the wire pack them,
  which is the natural split, but the addendum should state it.

### 6. Spectral amplitude estimation (BABA-A §5 / Eq. 32–33, pages ~17–19)

**What the full text says:**

```
â_l = (2/256) · Re[ Σ_{m} S_w(m) · S_w*(m,l) ]
b̂_l = (2/256) · Im[ Σ_{m} S_w(m) · S_w*(m,l) ]
M̂_l = sqrt(â_l² + b̂_l²),  1 ≤ l ≤ L̂
```

**What the implementation spec needs:**

- **`S_w(m, l)` basis.** Same requirement as §3 above — without this the
  amplitude extractor is a pseudocode sketch, not a formula.
- **Integration limits on `Σ_m`.** A harmonic's dominant spectral energy
  is in a narrow band around `m ≈ 256·l·ω̂₀/(2π)`; the sum is presumably
  over a per-harmonic `[a_l, b_l]` window from Eq. 32 of the PDF, but
  those bin boundaries are not transcribed in the paraphrase.
- **Complex conjugate convention.** `S_w*(m, l)` as the conjugate of the
  basis versus of the signal changes the sign of `b̂_l`. The decoder's
  phase tracking (`vocoder_decode_disambiguations.md §11`) is already
  known-broken; getting the encoder's conjugate convention right the
  first time will save another round of PCM-only debugging.
- **Partial-harmonic edge case.** When `L̂` places the highest harmonic
  near `π`, the basis `S_w(m, L̂)` may overlap the Nyquist bin. The PDF
  either addresses this (reduces the summation) or silently drops it;
  either way the addendum should state the behavior.
- **Cross-reference to the `γ_w` mismatch investigation.** `blip25-specs`
  commit `741eeef` flagged that decoder-side `γ_w` (unvoiced synthesis
  scale, derived from `w_S · w_R`) is ~150× off DVSI measured output.
  That mismatch currently isolates to synthesis, but `w_R` also appears
  on the encoder side of `S_w(m, l)` and (via the window-energy term
  implicit in Eq. 32–33's `2/256`) in the amplitude estimator
  normalization. If any of the V/UV metrics `D_k`, `ξ_LF`, `ξ_HF`, `ξ_G`
  carry a similar window-energy factor, they inherit the same DVSI
  ambiguity. Worth an explicit callout when the addendum's §0.5 / §0.7
  are drafted, so encoder-side calibration doesn't re-discover the
  same uncertainty from scratch.

### 7. Frame-type dispatch (silence, tone, voice)

**What the implementation spec has:** §5–6 cover tone and silence on the
decoder side (what bits represent each, how they synthesize).

**What the implementation spec needs on the encoder side:**

- **Silence decision.** When does the analysis encoder choose to emit a
  silence frame instead of a voice frame? The full text doesn't state
  this; it's likely an energy-threshold check on the input frame, but
  the threshold and hysteresis are not specified.
- **Tone-frame detection.** Per implementation spec §2.10 the half-rate
  wire has a dedicated tone frame with `f_0`, `l_1`, `l_2` amplitudes.
  Detecting "this input is a pure tone / DTMF / signaling tone" from the
  MBE analysis side is non-trivial — by amplitude concentration in one
  or two harmonics? by V/UV pattern? by a separate side-channel? The
  spec is silent on the encoder's tone-detection algorithm. Absent a
  real analysis-side algorithm, the MVP can pass this through ("upstream
  SAP provides tone metadata"), but the spec should at minimum state
  that this is how the DVSI chip actually behaves.
- **Tone detection is the exception to "PDF first, chip second."** The
  project's general rule (§"Why OSS cannot fill this gap" below) puts
  PDF fidelity ahead of DVSI black-box equivalence: when they conflict,
  the PDF wins. Tone detection is the one place where that ordering
  probably has to invert. The PDF may not specify the detection
  algorithm at all, only the emitted tone-frame payload; in that case
  the addendum's §0.8 should state explicitly that tone-frame *emission*
  follows the PDF (bit layout, `f_0`/`l_1`/`l_2` quantization) but
  tone-frame *entry criteria* are DVSI-black-box-defined and should be
  matched to the chip's observed behavior rather than invented.

### 8. Cross-frame encoder state

The analysis encoder has at least the following cross-frame state
dependencies visible from the paraphrased §§2–5:

- `P̂_{−4}`, `P̂_{−2}` pitch history (look-back tracking, §3.1)
- `P̂_{1}`, `P̂_{2}` pitch lookahead (§3.1)
- `R̃_l(−1)` prediction predictor state (§5.3)
- `ξ_max(−1)` V/UV threshold history (§4.2)
- `E_{−4}`, `E_{−2}`, `E_1`, `E_2` autocorrelation history (§3.1)

A `struct AnalysisEncoderState { … }` cannot be designed against the
current spec because the initial values, retention windows, and reset
conditions aren't documented. The addendum should include an Annex A-style
"Encoder Variable Initialization" table matching implementation-spec §10.

### 9. Reference C implementation

The decoder side of the implementation spec includes reference C
pseudocode in almost every subsection (e.g. §1.8.7 "Complete C Pipeline"
for spectral amplitude reconstruction). The analysis side has none. The
addendum should include at least one end-to-end C reference for
`pcm_frame + state → MbeParams`, so implementers have a fixed target to
numerically match.

---

## Why open-source / third-party sources cannot fill this gap

The project CLAUDE.md constrains blip25-mbe to source code from derived
implementation specs, not from OP25, SDRTrunk, imbe_vocoder, JMBE, dsdcc,
or similar. That constraint applies here doubly:

1. **DVSI equivalence is the bar.** The project's correctness target is
   the DVSI chip's output on known test vectors, not any open-source
   implementation. Seeding the encoder from OP25 would lock in OP25's
   V/UV threshold errors (documented in `oss_implementations_lessons_learned.md`)
   as our behavior, then fight them later.
2. **Patents.** The AMBE generations (+, +2) that matter for real-world
   audio quality have expired-patent encoder improvements that are
   enumerated in `DVSI/AMBE-3000/` for the synthesis side. The analysis
   side has similar patent-expired prior art (adaptive V/UV thresholds,
   joint pitch/V-UV search), but the baseline BABA-A encoder is distinct
   from those and has to be implemented cleanly before the enhanced
   variants are layered on.
3. **Licensing of full-text.** The BABA-A PDF and our
   `TIA-102-BABA-A_Full_Text.md` extract are copyrighted, internal-only
   material. Any derivation of `E(P)`, `E_R(ω₀)`, `D_k`, `θ_G`, or
   `S_w(m, l)` must go through the implementation spec, not through the
   full text.

---

## Proposed structure of the addendum

A new section inserted into
`standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` — likely
numbered §0 ("Analysis Encoder: PCM → MbeParams") so it precedes the
existing §1 full-rate decoder — with subsections mirroring the gap
inventory above:

```
§0 Analysis Encoder: PCM → MbeParams
  §0.1 Input framing, lookahead buffer, windowing (Annex B)
  §0.2 256-point DFT and S_w(m, l) basis (Eq. 25–28, Annex C) [CRITICAL PATH]
  §0.3 Initial pitch estimation (Eq. 5–9, LPF from Annex D)
  §0.4 Pitch refinement (Eq. 24–31) + pitch quantization (Eq. 45)
  §0.5 Spectral amplitude estimation (Eq. 32–33)
  §0.6 Log-magnitude prediction residual and L-mismatch handling
  §0.7 V/UV determination (Eq. 34–42) including θ_G and ξ_max state
  §0.8 Frame-type dispatch (voice / tone / silence / erasure)
       — tone-entry criteria are DVSI-black-box, not PDF
  §0.9 Encoder state structure + Annex A initialization values
  §0.10 Reference C pipeline (end-to-end: pcm → MbeParams)
  §0.11 Numerical cross-checks against DVSI test vectors
```

§0.2 should be drafted first; it unblocks §0.4, §0.5, and §0.7.

Each subsection should follow the existing spec's conventions:

- Direct PDF equation transcriptions (not paraphrases)
- Named constants with their decimal values
- Cross-references to the annex CSVs where applicable
- A reference C block per subsection
- A "common pitfalls" note where the algorithm has a known ambiguity
  (mirroring `vocoder_decode_disambiguations.md` for the decode side)

When this addendum lands, `crates/blip25-mbe/src/analysis/` can be
implemented module-by-module against it. Until it lands, the analysis
module should remain scaffolding only — entry point, frame buffering,
window/LPF loading via `build.rs`, and `unimplemented!()` stubs
pointing at this gap report.

---

## Expected scope of the addendum

Rough order-of-magnitude for the user's planning:

- PDF source pages: BABA-A pages ~5–22 for analysis, plus §7 encoding
  (pages ~27–35) for the pitch/gain/HOC forward quantizers the wire side
  already consumes.
- Addendum length: 20–30 pages of derived spec, comparable to the
  existing §1 (full-rate decode) in density. The tables are already
  extracted; what needs writing is the algorithmic body.
- Implementation-spec time: multi-day extraction-and-derivation effort
  per the pattern set by `vocoder_decode_disambiguations.md`.
- Implementation-code time (blip25-mbe): multi-week per the project
  memory estimate, on top of the addendum.
