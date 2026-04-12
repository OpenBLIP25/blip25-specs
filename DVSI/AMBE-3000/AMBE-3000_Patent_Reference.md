# AMBE-3000 Patent Reference

**Purpose:** Technical reference for expired DVSI patents relevant to
AMBE+2™ software implementation. These patents are public domain and serve
as detailed algorithmic documentation for the synthesis, quantization,
and rate conversion techniques used in the AMBE-3000 chip.

**Date:** 2026-04-13

---

## Patent Index

| # | Patent | Title | Expired | Section |
|---|--------|-------|---------|---------|
| 1 | US5701390 | MBE Synthesis with Regenerated Phase | 2015-02-22 | [Section 1](#1-us5701390--mbe-synthesis-with-regenerated-phase) |
| 2 | US8595002 | Half-Rate Vocoder (AMBE+2) | 2023-04-01 | [Section 2](#2-us8595002--half-rate-vocoder-ambe2) |
| 3 | US6199037 | Joint Quantization of Voicing and Pitch | 2017-12-04 | [Section 3](#3-us6199037--joint-quantization-of-voicing-and-pitch) |
| 4 | US7634399 | Voice Transcoder | 2025-11-07 | [Section 4](#4-us7634399--voice-transcoder) |
| 5 | US8315860 | Interoperable Vocoder | 2022-11-13 | [Section 5](#5-us8315860--interoperable-vocoder) |

---

## 1. US5701390 — MBE Synthesis with Regenerated Phase

- **Inventors:** Daniel W. Griffin, John C. Hardwick
- **Assignee:** Digital Voice Systems Inc
- **Filed:** 1995-02-22
- **Granted:** 1997-12-23
- **Expired:** 2015-02-22

### Why This Patent Matters

This is the foundational decoder improvement. Prior MBE decoders used
random phase for voiced synthesis, producing "buzzy" or "reverberant"
artifacts. This patent replaces random phase with phase regenerated from
the spectral envelope shape, producing dramatically more natural speech.

### Phase Regeneration Algorithm

Phase values (φ_l) for each harmonic l are derived from spectral
envelope smoothness via an edge detection kernel:

```
φ_l = Σ B_l · h(m)
```

Where:
- `B_l = log₂(M_l)` — log-compressed spectral magnitudes
- `h(m)` — edge detection kernel (typically 19 coefficients)
- Kernel properties: h(0) = 0, h(m) ≥ 0 for m > 0, h(m) = -h(-m)
  (antisymmetric), h(m) inversely proportional to m

**Physical justification:** Spectral phase depends on pole and zero
locations in the vocal tract transfer function. The algorithm links
phase to the level of smoothness in the spectral magnitudes — smooth
regions (poles/formants) get coherent phase, rough regions (zeros/
anti-formants) get less constrained phase.

### Voicing-Independent Spectral Magnitudes

A key prerequisite for good phase regeneration: spectral magnitudes are
computed independent of voicing decisions using a compensated total
energy method:

```
M_i = Σ |S_w(ω)|² · G(ω)
```

Where G(ω) compensates for FFT sampling grid misalignment and preserves
total spectral energy. This eliminates discontinuities at voiced/
unvoiced transitions and produces a smoother representation that
improves both quantization efficiency and phase estimation.

### Voiced Synthesis — Four Cases

The patent defines four synthesis cases based on frame-to-frame voicing
transitions for each harmonic:

**Case 1 — Unvoiced in both frames:**
```
s_v,l(n) = 0
```
(Handled entirely by unvoiced synthesis path)

**Case 2 — Voiced → Unvoiced transition:**
```
s_v,l(n) = w_s(n+S) · M_l(-S) · cos[ω₀(-S)(n+S)l + θ_l(-S)]
```
Fadeout window applied over synthesis interval.

**Case 3 — Unvoiced → Voiced transition:**
```
s_v,l(n) = w_s(n) · M_l(0) · cos[ω₀(0)nl + θ_l(0)]
```
Fadein window applied over synthesis interval.

**Case 4a — Large fundamental frequency change (l ≥ 8 or |Δω| ≥ 0.1ω₀):**
Overlap-add method combines previous and current frame contributions
separately.

**Case 4b — Continuous phase transition (small Δω, l < 8):**
```
s_v,l(n) = a_l(n) · cos[θ_l(n)]
```
Where:
- `a_l(n)` linearly interpolates amplitude between frames
- `θ_l(n) = φ_l(0) + Δω_l·n + (1/2)·α_l·n²` — second-order polynomial
  ensuring phase continuity at frame boundaries while matching
  regenerated phase values at endpoints

### Synthesis Window

Window function satisfies the overlap-add constraint:
```
w_s(n) + w_s(n+S) = 1
```
Implementation uses linear interpolation window with β=50 for 20ms
frames at 8 kHz.

### Unvoiced Synthesis

White noise filtered with:
- Zero response in voiced frequency bands
- Magnitude-matched response in unvoiced bands
- Implemented via weighted overlap-add using forward and inverse FFT

### Combined Output
```
Total speech = Σ(voiced harmonic components) + filtered unvoiced noise
```

---

## 2. US8595002 — Half-Rate Vocoder (AMBE+2)

- **Inventor:** John C. Hardwick
- **Assignee:** Digital Voice Systems Inc
- **Filed:** 2003-04-01 (priority), 2013-01-18 (this filing)
- **Granted:** 2013-11-26
- **Expired:** 2023-04-01

### Why This Patent Matters

This is the definitive AMBE+2 patent. It describes the complete half-rate
(3600 bps) vocoder that achieves quality comparable to or better than the
original 7200 bps IMBE vocoder. It covers both the encoder (analysis +
quantization) and decoder (reconstruction + synthesis).

### Frame Structure

| Parameter | Full-Rate (7200 bps) | Half-Rate (3600 bps) |
|-----------|---------------------|----------------------|
| Total bits/frame | 144 | 72 |
| FEC bits | 56 | 23 |
| Sync bits | 1 | 0 |
| Parameter bits | 87 | 49 |
| Pitch bits | 8 | 7 |
| Voicing bits | 3–12 (variable) | 5 (fixed, vector-quantized) |
| Spectral bits | 67–76 | 25–32 (variable) |
| Gain bits | — | 5 |
| Frame duration | 20 ms | 20 ms |

### Fundamental Frequency Quantization (7 bits)

Logarithmic quantization:
```
b_fund = 0,                              if f₀ > 0.0503
b_fund = 119,                            if f₀ < 0.00811
b_fund = ⌊−195.626 − 45.368·log₂(f₀)⌋,  otherwise
```

### Voicing Decision System

8 frequency bands covering [0, 4000 Hz] in 500 Hz increments.

For each band k, a voicing metric is computed:
```
lv_k = max[0.0, min[1.0, 0.5·(1.0 − log₂(v_err_k / (T_k · v_ener_k)))]]
```

Where:
- `v_err_k` = voicing error in band k
- `v_ener_k` = voicing energy in band k
- `T_k` = threshold

The 8-band voicing vector is then quantized using a 5-bit vector
quantizer with a 32-element codebook (Table 2). Multiple codebook
indices map to identical voicing states for upgrade compatibility:
- Indices 0 and 1 both → all voiced
- Indices 16–31 all → all unvoiced

### Spectral Magnitude Quantization (25–32 bits)

Multi-stage quantization pipeline:

**Step 1 — Log transform:**
```
log₂(M_l)  for 1 ≤ l ≤ L
```

**Step 2 — Mean removal:**
Zero-mean spectral magnitudes computed.

**Step 3 — Differential gain (5 bits):**
Non-uniform scalar quantizer (Table 3) produces gain bits.

**Step 4 — Prediction residual quantization:**
- Divide harmonics into 4 frequency blocks (variable size)
- DCT each block
- Extract PRBA vector (8 elements) from DCT coefficients
- Extract HOC vectors (4 types) for higher-order coefficients
- Split vector quantization:
  - PRBA₁₋₃: 9-bit codebook
  - PRBA₄₋₇: 7-bit codebook
  - HOC₀: 5-bit codebook
  - HOC₁, HOC₂: 4-bit codebooks each
  - HOC₃: 3-bit codebook

**Bit reduction trick:** Bits per frame reduced from nominal 32 by using
only a subset of allowable quantization vectors in one or more codebooks.

### Per-Harmonic Voicing (Key Improvement)

The half-rate vocoder computes "a separate voicing decision for each
harmonic" rather than for groups of three or more harmonics as in
baseline IMBE. This finer granularity improves quality at transitions
and for mixed-voicing sounds.

### FEC Structure

- First 12 parameter bits → [24,12] extended Golay code
- Next 12 bits → [23,12] Golay code with data-dependent scrambling
- Scrambling key derived from the [24,12] output bits
- [4×18] row-column interleaver applied to reduce burst error effects

### Error Handling

Error detection threshold: if the sum of Golay decode errors ≥ 6,
frame declared invalid. After three consecutive invalid frames,
decoder mutes output.

---

## 3. US6199037 — Joint Quantization of Voicing and Pitch

- **Inventor:** John C. Hardwick
- **Assignee:** Digital Voice Systems Inc
- **Filed:** 1997-12-04
- **Granted:** 2001-03-06
- **Expired:** 2017-12-04

### Why This Patent Matters

This patent describes how to efficiently quantize voicing decisions and
fundamental frequency jointly across subframes, exploiting the temporal
correlation between consecutive subframes. This is the encoder-side
complement to the synthesis improvements.

### Joint Voicing Metrics Quantization

**Processing pipeline:**
1. **Smoother:** Applies smoothing to voicing metrics across two
   consecutive subframes, producing smoothed values based on ratios
   of voicing error to voicing energy
2. **Non-linear transform:** Converts smoothed metrics to logarithmic
   form, constrained to [0.0, 1.0] range
3. **Vector quantization:** Two methods:
   - Full vector (16-element): 6-bit codebook (64 entries)
   - Split vector (8+8 elements): Divide by subframe, each quantized
     independently with same codebook

### Joint Fundamental Frequency Quantization

**Method 1 — Scalar + Vector hybrid:**
1. Convert f₀ for both subframes to log₂
2. 4-bit scalar quantizer on average of log-frequencies (16 levels)
3. 6-bit vector quantizer on residual differences as 2D vector
4. Codebook more densely clustered around small changes (exploiting
   the fact that pitch varies slowly)

**Method 2 — Scalar + Interpolation:**
1. N-bit scalar quantization of first subframe f₀
2. Second subframe f₀ derived via interpolation from prior frame
3. 2-bit index selects interpolation rule

### Codebook Structure

- Voicing metrics: 6-bit codebook, 64 entries, 16-element vectors
- Fundamental frequency: 6-bit codebook, 64 entries, 2D vectors
  (inter-subframe differences)
- Spectral magnitudes: 8-bit mean vector codebook (256 entries) with
  block DCT quantizers for harmonic components

---

## 4. US7634399 — Voice Transcoder

- **Inventor:** John C. Hardwick
- **Assignee:** Digital Voice Systems Inc
- **Filed:** 2003-01-30
- **Granted:** 2009-12-15
- **Expired:** 2025-11-07

### Why This Patent Matters

This is the rate conversion patent. It describes how to convert between
full-rate (7200 bps) and half-rate (3600 bps) parametrically — at the
MBE model parameter level — without a PCM round-trip. This is what the
AMBE-3000 chip does in repeater mode (`PKT_RPT_MODE`).

### Parametric Transcoding Algorithm

1. **Receive** encoded voice frame at source rate
2. **FEC decode** to extract parameter bits
3. **Reconstruct** MBE model parameters:
   - Fundamental frequency (inverse quantization of pitch bits)
   - Voicing decisions
   - Spectral magnitudes (using prediction residuals + prior frame)
4. **Convert** parameter representations between rates
5. **Re-quantize** parameters per target rate specification
6. **FEC encode** to produce output frame at target rate

### Key Technical Details

**Voicing band conversion:**
Variable-band voicing decisions resampled to fixed 8-band
representation, "favoring the voiced state" when resampling. This
normalization simplifies downstream quantization.

**Spectral magnitude handling:**
When fundamental frequency quantization error exceeds 1%, spectral
magnitudes are linearly interpolated and resampled based on ratio R
of reconstructed f₀ over input f₀. An energy-preserving offset of
`0.5·log(R)` is applied.

**Spectral compensation for unvoiced bands:**
Offset of `0.5·log(256·f₀)` applied to unvoiced bands, accounting
for scaling differences between source and target vocoders.

**Prior-frame prediction:**
Spectral magnitudes from the previous frame are stored, interpolated,
and resampled based on fundamental frequency ratio. Prediction uses
scaling factor ρ = 0.65.

**Invalid frame handling:**
Frames with bit errors exceeding threshold are substituted with known
frame-repeat bits, triggering the decoder's repeat function rather
than propagating corrupted parameters.

### FEC Details by Rate

**Full-rate (7200 bps) decoding:**
- [23,12] Golay codes and [15,11] Hamming codes
- Data-dependent descrambling
- Deinterleaving

**Half-rate (3600 bps) encoding:**
- [24,12] extended Golay code followed by [23,12] Golay code
- Scrambling based on modulation key from first extended Golay output
- Row-column interleaving

### Continuation Patent

US7957963B2 is a continuation of this patent covering the same
technology. Filed 2009-12-14, granted 2011-06-07, expired 2023-01-30.

---

## 5. US8315860 — Interoperable Vocoder

- **Inventor:** John C. Hardwick
- **Assignee:** Digital Voice Systems Inc
- **Filed:** 2002-11-13 (priority), 2011-06-27 (this filing)
- **Granted:** 2012-11-20
- **Expired:** 2022-11-13

### Why This Patent Matters

This patent describes enhanced full-rate (7200 bps) encoding that
improves quality while maintaining bit-stream interoperability with
standard APCO Project 25 decoders. A standard decoder will produce
intelligible output; an enhanced decoder will produce higher quality
by extracting additional information embedded in the standard bit fields.

### Core Innovation: Three-State Voicing

Instead of binary voiced/unvoiced, the enhanced vocoder uses three
states per frequency band: **voiced**, **unvoiced**, and **pulsed**.

When no frequency band is voiced, the fundamental frequency field (b₀)
is repurposed to encode the voicing state distribution via a 57-entry
lookup table (Table 2). The voicing quantizer b₁ is simultaneously
set to zero, so a standard decoder sees "all unvoiced" and produces
acceptable output.

An enhanced decoder checks:
1. Is b₁ = 0? (all unvoiced)
2. Is b₀ a reserved value from Table 2?
3. If yes to both, perform table lookup to recover 8-channel voicing
   decisions with pulsed/unvoiced detail

### Table 2 Structure

57 mappings (quantizer values 128–184) between channel voicing
decision patterns and fundamental frequency values. Each entry encodes
8-channel voicing decisions for up to 2 subframes with a corresponding
fundamental frequency (79.80–248.0 Hz).

### Additional Features

**Tone detection:**
FFT-based analysis validating SNR, frequency tolerance (~3%), and twist
for DTMF. Table 1 maps tone frequencies to fundamental frequency
values, non-zero harmonic indices, and voicing band assignments.

**Noise suppression:**
Spectral subtraction with maximum ~15 dB attenuation. 16-band energy
tracking with noise floor estimation.

**Voice Activity Detection (VAD):**
16-band energy tracking classifies frames as speech or silence.

**Sidelobe suppression (decoder):**
20 dB attenuation of non-harmonic spectral components for detected
tone frames.

---

## Cross-Patent Algorithm Flow

The complete AMBE+2 encoder-decoder chain, combining all five patents:

### Encoder (PCM → Channel Bits)

```
PCM input (160 samples, 20ms)
    │
    ▼
Pitch estimation + windowed FFT
    │
    ▼
Fundamental frequency quantization (7 bits, US8595002)
    │
    ▼
Joint voicing metrics quantization (5 bits, US6199037)
  └─ 8 frequency bands, smoothed across subframes
  └─ Three-state V/UV/pulsed model (US8315860)
    │
    ▼
Voicing-independent spectral magnitudes (US5701390)
  └─ Compensated total energy method
    │
    ▼
Spectral magnitude quantization (25-32 bits, US8595002)
  └─ Log transform → mean removal → differential gain (5 bits)
  └─ DCT per frequency block → PRBA + HOC vectors
  └─ Split vector quantization with prediction residuals
    │
    ▼
FEC encoding (US8595002)
  └─ [24,12] extended Golay + [23,12] Golay
  └─ Data-dependent scrambling
  └─ [4×18] row-column interleaving
    │
    ▼
72 channel bits output (half-rate)
```

### Decoder (Channel Bits → PCM)

```
72 channel bits input (half-rate)
    │
    ▼
FEC decoding (US8595002)
  └─ Deinterleave → descramble → Golay decode
  └─ Error detection (threshold ≥ 6 → invalid frame)
    │
    ▼
Parameter reconstruction (US8595002)
  └─ Inverse quantization of pitch, voicing, spectral amplitudes
  └─ Per-harmonic voicing decisions
    │
    ▼
Phase regeneration from spectral envelope (US5701390)
  └─ Edge detection kernel on log₂(M_l)
  └─ Links phase to local spectral smoothness
    │
    ▼
Voiced synthesis (US5701390)
  └─ Harmonic oscillator bank
  └─ Four-case transition handling (V→V, V→UV, UV→V, UV→UV)
  └─ Second-order phase polynomial for continuity
    │
    ▼
Unvoiced synthesis (US5701390)
  └─ White noise filtered via overlap-add FFT method
    │
    ▼
Combined PCM output (160 samples, 20ms)
```

### Rate Converter (Channel Bits → Channel Bits)

```
Channel bits at source rate
    │
    ▼
FEC decode (source rate scheme)
    │
    ▼
Reconstruct MBE parameters (US7634399)
  └─ f₀, voicing decisions, spectral magnitudes
  └─ Using prior-frame prediction (ρ = 0.65)
    │
    ▼
Parameter conversion (US7634399)
  └─ Voicing band normalization to 8 fixed bands
  └─ Spectral magnitude interpolation/resampling (ratio R = f₀_out/f₀_in)
  └─ Energy-preserving offset: 0.5·log(R)
  └─ Unvoiced compensation: 0.5·log(256·f₀)
    │
    ▼
Re-quantize to target rate (US7634399, US8595002)
    │
    ▼
FEC encode (target rate scheme)
    │
    ▼
Channel bits at target rate
```
