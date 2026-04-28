# AMBE-3000 Patent Reference

**Purpose:** Technical reference for DVSI patents relevant to AMBE+2™ software
implementation. The expired patents are public domain and serve as detailed
algorithmic documentation for the synthesis, quantization, and rate conversion
techniques used in the AMBE-3000 chip. **Note:** Not all relevant patents have
expired — see §6 (US8359197) for an active half-rate vocoder patent that
covers the same disclosure as US8595002 but remains in force until 2028-05-20
due to patent term adjustment.

**Date:** 2026-04-13 (last updated 2026-04-27 with §§6–11)

---

## Patent Index

| # | Patent | Title | Expires | Status (2026-04-27) | Section |
|---|--------|-------|---------|---------|---------|
| 1 | US5701390 | MBE Synthesis with Regenerated Phase | 2015-02-22 | Expired | [§1](#1-us5701390--mbe-synthesis-with-regenerated-phase) |
| 2 | US8595002 | Half-Rate Vocoder (AMBE+2) | 2023-04-01 | Expired | [§2](#2-us8595002--half-rate-vocoder-ambe2) |
| 3 | US6199037 | Joint Quantization of Voicing and Pitch | 2017-12-04 | Expired | [§3](#3-us6199037--joint-quantization-of-voicing-and-pitch) |
| 4 | US7634399 | Voice Transcoder | 2025-11-07 | Expired | [§4](#4-us7634399--voice-transcoder) |
| 5 | US8315860 | Interoperable Vocoder | 2022-11-13 | Expired | [§5](#5-us8315860--interoperable-vocoder) |
| **6** | **US8359197** | **Half-Rate Vocoder (sibling of US8595002)** | **2028-05-20** | **ACTIVE** ⚠️ | [§6](#6-us8359197--half-rate-vocoder-active-until-2028) |
| 7 | US8265937 | Breathing-Apparatus Speech Enhancement (Fireground) | ~2032 | Active | [§7](#7-us8265937--breathing-apparatus-speech-enhancement-fireground-patent) |
| **8** | **US12254895** | **Detecting and Compensating for Speaker Mask** | **~2045** | **ACTIVE** ⚠️ | [§8](#8-us12254895--detecting-and-compensating-for-speaker-mask-active-until-2045) |
| **9** | **US11990144** | **Reducing Perceived Effects of Non-Voice Data** | **~2041** | **ACTIVE** | [§9](#9-us11990144--reducing-perceived-effects-of-non-voice-data-active) |
| **10** | **US12451151** | **Tone Frame Detector (PTAB-confirmed)** | **~2042** | **ACTIVE** ⚠️ | [§10](#10-us12451151--tone-frame-detector-active-ptab-confirmed) |
| 11 | EP 1,465,158 + DVSI foundational MBE prior art | Foreign prosecution overview (EP) + US 5,715,365, US 5,826,222 | 2023-04 (EP) | Expired | [§11](#11-foreign-prosecution-overview--ep-1465158-half-rate-vocoder-european) |

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

### Prosecution — Examiner's Reasons for Allowance

USPTO application 10/353,974, examined by Examiner David D. Knepper
(Art Unit 2626 — different examiner from the half-rate / interoperable
vocoder patents which were examined by Angela A. Armstrong). Allowed
2009-09-04 with all 63 originally filed claims intact.

The examiner's statement of reasons for allowance:

> "The prior art teaches coding such as Multi-Band Excitation (MBE)
> that divides parameters into at least two portions for transmission.
> However, the prior art does not utilize the divisions as claimed
> which **apply error control encoding which application thereof must
> precede the computation of speech parameters**."

The patentable insight identified by the examiner: applying FEC
**decoding** to the source bitstream **before** parameter recomputation
in the rate converter. Prior MBE rate conversion either (a) did
PCM-roundtrip transcoding (decode → resynthesize → re-analyze →
re-encode) or (b) manipulated parameters without proper FEC handling.
US7634399 is the combination — FEC-decode source → parametric
conversion → FEC-encode target — without going through PCM. This is
the parametric repeater-mode operation used in the AMBE-3000's
`PKT_RPT_MODE`.

This prosecution validates §4's existing framing: US7634399 is
specifically the patent on **parametric (no-PCM-roundtrip) rate
conversion**.

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

### Family Relationship — Parent US7970606

US8315860 is a **continuation** of US7970606 (also titled "Interoperable
Vocoder"). The two patents share the same disclosure and the same
2002-11-13 priority date but pursued different claim scopes:

| | US7970606 (parent) | US8315860 (continuation) |
|---|---|---|
| Application no. | 10/292,460 | 13/169,642 |
| Filed | 2002-11-13 | 2011-06-27 |
| Granted | 2011-06-28 | 2012-11-20 |
| Expired | 2025-09-08 (700 days PTA) | 2025-09-08 (terminal disclaimer to '606) |
| Claim count | 58 (allowed 1–58) | 35 (allowed 1–35) |
| Examiner | Angela A. Armstrong | Angela A. Armstrong |
| Key prosecution event | 3rd appeal brief (2010-11-29) found persuasive | Terminal Disclaimer (2012-06-29) + single Examiner's Amendment |

The continuation strategy: '606 prosecuted to allowance through hard
rejections and appeals over 8.6 years, securing the broader 58-claim
scope. '860 was filed shortly before '606 issued and pursued a
narrower 35-claim scope tightly drafted around interoperability with
APCO Project 25. DVSI filed a Terminal Disclaimer in '860 to overcome
obviousness-type double-patenting, which ties '860's expiration to
'606's. Both expired together on 2025-09-08.

### Explicit P25 Interoperability Claim

US8315860's claim 21 (added by Examiner's Amendment 2012-07-16) recites:

> "The speech coder of claim 17 wherein the produced bit stream is
> **interoperable with the standard coder used for APCO Project 25**."

This is one of the few DVSI patent claims that explicitly names APCO
Project 25 in its operative language. It captures the patent's
commercial role as the legal hook for "enhanced vocoder produces a
P25-compatible bitstream that backward-compatible decoders can still
parse, while enhanced decoders extract higher-quality information."

### Prosecution — US7970606 Parent's Examiner Reasons for Allowance

The 2011-03-17 Notice of Allowance for US7970606 contains the
substantive Reasons for Allowance (box 8 checked) — these reasons
apply to the same disclosure that backs US8315860:

> "Applicant's arguments, see Appeal Brief (pages 4-8), filed
> November 29, 2010, with respect to claims 1-19, 28-43, and 47-54
> have been fully considered and are persuasive. The rejection of the
> claims has been withdrawn."

> "The following is an examiner's statement of reasons for allowance:
> the prior art reference of **Hardwick, Coulter, and Kutaragi** fails
> to specifically teach or fairly disclose **modifying a parameter
> conveying pitch information to designate the determined voicing
> state of a frame if the determined voicing state of the frame is
> equal to one of a set of multiple reserved voicing states**.
> Additionally, the prior art of Hardwick, Coulter, and Kutaragi fails
> to specifically teach or disclose **quantizing the model parameters,
> including the pitch parameter and the spectral parameters to which
> values are assigned to approximate the detected tone signal if the
> digital speech samples for the frame are determined to correspond to
> the tone signal**, to generate quantizer bits which are used to
> produce the bit stream."

The examiner identified **two distinct patentable inventions** in this
patent family, mirroring what we observed in US8359197:

1. **Three-state voicing encoded in reserved pitch values** — the
   reserved-pitch-field repurposing documented in §5's "Core Innovation"
   above. The examiner's words: "modifying a parameter conveying pitch
   information to designate the determined voicing state of a frame if
   the determined voicing state of the frame is equal to one of a set
   of multiple reserved voicing states."
2. **Tone signal handling via shared parameter quantizer** — instead of
   a separate tone path, tone signals are encoded by assigning specific
   pitch and spectral parameter values that approximate the tone, then
   quantized through the same parameter machinery as voice. The
   examiner's words: "quantizing the model parameters, including the
   pitch parameter and the spectral parameters to which values are
   assigned to approximate the detected tone signal."

Prior art cited in the rejection: **Hardwick + Coulter + Kutaragi**
(Ken Kutaragi of Sony — the PlayStation creator held vocoder patents
from his Sony career in the 1990s). The "Hardwick" reference is
DVSI's own prior work; combining it with the Sony-era voice-coding
patents was the examiner's obviousness theory, which DVSI distinguished
in pages 4–8 of the November 2010 Appeal Brief.

### Cross-Patent Interview Note

The 2009-09-10 examiner interview that finalized US8359197's "less
than all of the quantizer bits" amendment (see §6) was held the **same
day** as a separate interview on US7970606. John C. Hardwick was
negotiating both patents in parallel with Examiner Armstrong on a
single visit to the USPTO — a coordinated negotiation strategy.

### Active Family Member: US7970606 vs. US8315860 Status (2026-04-27)

Both expired 2025-09-08. The interoperable-vocoder family is **fully
public domain** as of late 2025. Implementations of three-state V/UV/
pulsed encoding using reserved pitch values, and tone-handling via
shared parameter quantizers, are no longer encumbered by either
US7970606 or US8315860.

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

---

## 6. US8359197 — Half-Rate Vocoder (active until 2028)

- **Inventor:** John C. Hardwick
- **Assignee:** Digital Voice Systems Inc
- **Filed:** 2003-04-01
- **Granted:** 2013-01-22
- **Anticipated expiration:** **2028-05-20** (patent term adjustment for ~10 years of USPTO prosecution delay)
- **Family:** US20050278169A1 (publication), US8595002B2 (continuation), CA2461704C, EP1465158B1, EP1748425B1, JP2004310088A, DE602004021438D1, DE602004003610T2

### Why This Patent Matters

US8359197 is the parent half-rate vocoder grant from the same 2003-04-01
priority filing as US8595002. Both share inventor, disclosure, and
subject matter. The crucial difference is **expiration**: US8595002 expired
2023-04-01 (no PTA), while US8359197 picked up nearly five years of patent
term adjustment and remains in force until **2028-05-20**.

The disclosure is essentially identical to US8595002's. What this section
documents is the **claim language**, since that is what governs
infringement and is the only part that differs meaningfully from §2.

### Claim Coverage Summary

The patent has 87 claims with four independent claims:

| Independent Claim | Subject |
|---|---|
| Claim 1 | Encoding method (frame → params → mixed sub-codeword → FEC) |
| Claim 42 | Decoding method (FEC → params → speech samples) |
| Claim 60 | Decoding method with tone/speech discrimination |
| Claim 72 | Decoding using bit-count-dependent spectral codebook index |

### Claim 1 (Verbatim, Independent Encoder)

> A method of encoding a sequence of digital speech samples into a bit
> stream, the method comprising:
> - dividing the digital speech samples into one or more frames;
> - computing model parameters for a frame;
> - quantizing the model parameters to produce pitch bits conveying pitch
>   information, voicing bits conveying voicing information, and gain bits
>   conveying signal level information, wherein the pitch bits, the voicing
>   bits and the gain bits are included in quantizer bits for the frame;
> - **combining one or more of the pitch bits with one or more of the
>   voicing bits and one or more of the gain bits to create a first
>   parameter codeword that includes less than all of the quantizer bits
>   for the frame;**
> - encoding the first parameter codeword with an error control code to
>   produce a first FEC ("forward error control") codeword; and
> - including the first FEC codeword in a bit stream for the frame.

The **bolded limitation** is the novelty: forming a sub-codeword by
mixing pitch + voicing + gain bits (rather than FEC-protecting each
parameter separately), then FEC-encoding that mixed codeword.

### Narrowing-Claim Chain Mapping to AMBE+2 Half-Rate

| Claim | Limitation | Matches BABA-A half-rate? |
|---|---|---|
| 2 | Fundamental freq + voicing decisions + spectral params | ✓ |
| 3 | MBE speech model | ✓ |
| 5 | Joint voicing quantization | ✓ |
| 6 | Voicing codebook with redundant entries (multiple indices → same vector) | ✓ (Table 2: indices 0,1 → all voiced; 16–31 → all unvoiced) |
| 7 | First codeword = 12 bits | ✓ |
| 8 | 12 bits = 4 pitch + 4 voicing + 4 gain | ✓ exact |
| 9 | First codeword Golay-encoded | ✓ ([24,12]) |
| 10 | Log-spectral magnitudes; gain = mean of logmags | ✓ |
| 11 | Spectral bits → second codeword → second FEC | ✓ ([23,12]) |
| 12 | Bits split into important/less-important; less-important sent unprotected | ✓ |
| 13 | 7 pitch (4+3), 5 voicing (4+1), 5 gain (4+1) | ✓ exact |
| 14 | 12 most-important spectral bits with Golay | ✓ |
| 15 | Scrambling sequence derived from first parameter codeword applied to second FEC codeword | ✓ exact AMBE+2 data-dependent scrambling |
| 16–19 | Tone detection; tone identifier + amplitude bits in first codeword; tone identifier = "disallowed pitch bits" | ✓ matches 0x3F tone-frame identifier |

### Decoder Side (Claims 42–71)

Claim 42 is the mirror image of claim 1. The chain claims 47–51 add the
error-metric path: Golay decoder → modulation key recovery from first
codeword → descrambling → second Golay decode → error metric (sum of
corrected errors from both Golay decoders) → frame error processing if
metric ≥ threshold (claim 51 specifies "threshold value"; the disclosure
identifies threshold = 6, and frame error processing = repeat previous
frame → mute after consecutive invalid frames).

### Practical Infringement Read

A clean-room AMBE+2 **half-rate** decoder/encoder that implements:
- 4 pitch + 4 voicing + 4 gain MSBs combined into a 12-bit first codeword
- [24,12] extended Golay protecting that codeword
- Data-dependent scrambling keyed from the first codeword's Golay output
- Redundant-index voicing codebook (multiple indices → same vector)
- Tone-frame identifier in disallowed pitch-bit values (0x3F)

reads on claims 8, 9, 13, 14, and 15 (and necessarily claim 1).

**Full-rate (7200 bps) IMBE decoders are likely outside** the narrowing
claims because claims 7–14 specify bit allocations specific to the
half-rate format. Claim 1 alone is broader, but practical reads against
full-rate IMBE depend on whether the IMBE first codeword groups pitch +
voicing + gain bits — a question for the BABA-A full-rate FEC structure,
not resolved here.

### Implications for Implementation Roadmap

US8359197 does **not** disclose new algorithmic content beyond what
§2 (US8595002) already documents — disclosure and figures are essentially
identical. What it changes is the **legal posture**:

- A faithful AMBE+2 half-rate clean-room implementation that passes
  bit-exact interop with AMBE-3000 will read on US8359197 claims 8–15.
- Encoder and decoder are both covered (claims 1–41 encoder side,
  42–71 decoder side, plus claim 60 tone-handling decoder).
- Patent expires 2028-05-20. Receive-only/decoder-only software for
  research or personal use has weaker practical enforcement risk than
  vendor product, but the patent claims still cover decoder
  implementations.
- A non-AMBE+2-compatible half-rate vocoder that uses a different first-
  codeword grouping (e.g., 6 pitch + 6 spectral instead of 4+4+4) might
  fall outside the narrowing claims while still landing on claim 1.

### Prosecution History — How the Independent Claims Were Narrowed

US8359197 prosecuted from 2003-04-01 to 2013-01-22 (nearly 10 years), with
~5.1 years of patent term adjustment. Key prosecution events:

| Date | Event |
|---|---|
| 2003-04-01 | Application filed (87 claims) |
| 2005-12-15 | Published as US 2005/0278169 A1 |
| 2007-02-07 | First Non-Final Rejection (rejection #1) |
| 2007-11-26 | Final Rejection #1 |
| 2008-04-29 | Notice of Appeal #1 (examiner reopened) |
| 2009-04-13 | Non-Final Rejection #2 |
| 2009-09-21 | RCE filed (after Notice of Appeal #2) |
| 2009-12-11 | Non-Final Rejection #3 |
| 2010-06-09 | Non-Final Rejection #4 |
| 2011-03-21 | Final Rejection #2 (all 87 claims) |
| 2011-07-21 | Notice of Appeal #3 |
| 2011-11-21 | Appeal Brief filed |
| 2012-04-07 | Ex Parte Quayle Action — examiner closed prosecution on the merits |
| 2012-11-26 | Notice of Allowance |
| 2013-01-22 | Patent issued |

Prior art cited in the rejections: Hardwick '037 (US 6,199,037),
Griffin '974 (US 5,754,974), Hardwick '089 (US 6,161,089), Hardwick '511
(US 5,517,511) — all DVSI's own earlier patents. Huang (US 5,496,798) was
cited briefly and dropped. The examiner argued obviousness over
combinations of DVSI's own prior work.

#### Examiner's Statements of Reasons for Allowance

The 2012-04-12 Ex Parte Quayle Action mailed by Examiner Angela A.
Armstrong (Art Unit 2626) closed prosecution on the merits and contains
the only statements of reasons for allowance for this patent. (The
2012-11-26 Notice of Allowance does not have the "Examiner's Statement
of Reasons for Allowance" attachment box checked — that statement was
already issued in the Quayle Action.) The Quayle Action says:

> "Applicant's arguments, see Appeal Brief (pages 3-9), filed 11/21/11,
> with respect to claims 1-87 have been fully considered and are
> persuasive. The rejection of claims 1-87 has been withdrawn."

Pages 3–9 of the 2011-11-21 Appeal Brief are the substantive argument
section that convinced the examiner.

The Quayle Action contains **two separate** examiner statements of
reasons for allowance — confirming the two-inventions framing:

**Reasons for allowance — claims 1, 42, 60 and dependents** (mixed-
codeword chain):

> "Regarding claim 1, the prior art of Hardwick (US Patent No.
> 6,199,037), Griffin (US Patent No. 5,754,974) and Hardwick (US Patent
> No. 6,161,089) **fails to specifically teach or disclose** a method of
> encoding...combining one or more of the pitch bits with one or more
> of the voicing bits and one or more of the gain bits to create a
> first parameter codeword that includes less than all of the quantizer
> bits for the frame; encoding the first parameter codeword with an
> error control code to produce a first FEC codeword..."

The same "fails to teach" finding is recited for claim 42 (decoder
mirror) and claim 60 (tone-aware decoder).

**Reasons for allowance — claims 72–87** (bit-count-dependent spectral
codebook chain):

> "Regarding claim 72, Hardwick (US Patent No. 6,199,037), Griffin (US
> Patent No. 5,754,974) and Hardwick (US Patent No. 6,161,089) **fails
> to specifically teach or disclose** a method for decoding a frame of
> bits into speech samples...using one or more of the spectral bits to
> form a spectral codebook index, wherein the index is determined at
> least in part by the number of bits in the frame of bits..."

The same prior-art trio failed to disclose **either** invention — the
examiner had to concede on both grounds, not just one.

**Final formal objection (the only thing remaining):**

> "Claims 1-71 are objected to because of the following informalities:
> Claims 1, 42, and 60 recite the term 'FEC' without a description of
> the term at its first use. Appropriate correction is required."

DVSI's 2012-04-12 response to the Quayle Action added the parenthetical
"('forward error control')" after the first use of "FEC" in each of
claims 1, 42, and 60. This is the only post-Quayle amendment in the
file wrapper. No substantive claim language changed between the Quayle
Action and the grant.

#### The September 10, 2009 Interview — Where Scope Was Negotiated

The patent's enforceable scope was effectively settled at an in-person
examiner interview on 2009-09-10. Participants:

- Examiner Angela A. Armstrong (Art Unit 2626)
- John F. Hayden (Fish & Richardson, attorney)
- **John C. Hardwick** — the named inventor himself

The Interview Summary's continuation sheet records the agreements
reached:

> "Regarding claim 1, an agreement was reached to amend the claims to
> specify that **a subset combined pitch, voicing, and gain bits** are
> used to form the first parameter codeword used in the FEC encoding.
> Regarding claims 42 and 60, an agreement was reached to amend the
> claims to specify that the bits of the FEC codeword represent only a
> subset of all available pitch, voicing and gain bits. Regarding
> claim 72, an agreement was reached that the prior art of Hardwick
> (6,199,037) does not teach the claim limitations."

The "less than all of the quantizer bits" amendment originated here in
2009 — not in the 2011 Appeal Brief. The 2½ years of subsequent
prosecution (Dec 2009 OA → June 2010 OA → March 2011 Final → Nov 2011
Appeal Brief → April 2012 Quayle) were spent re-fighting the same battle
as the examiner introduced new prior art (Hardwick '089 first appeared
in the December 2009 Office Action) and then withdrew it.

**Claim 72 was conceded at the September 2009 interview** — Hardwick
'037 alone was admitted not to teach the bit-count-dependent spectral
codebook. The examiner only resisted on claims 1, 42, 60 thereafter.

#### The Patent's Technical Foundation — Unexpected Voicing Sensitivity

The 2011-11-21 Appeal Brief (pages 3–4) articulates the technical
insight that supports patentability under § 103. The argument is a
classic "unexpected results" rebuttal:

> "When coding voice, voice quality in the presence of bit errors may be
> determined by the weakest or most sensitive parameter... Since the
> inventor was developing low bit rate coding techniques, very few bits
> were available for error correction. In determining how best to make
> use of the available error correction bits, **the inventor was
> surprised to determine that performance was limited to a large extent
> by bit errors in the pitch, gain and voicing information. This was a
> surprising and unexpected result because the inventor had generally
> not considered the voicing information to be very sensitive to bit
> errors.** Faced with this unexpected result, the inventor decided to
> use available error correction bits to encode a codeword formed by
> combining one or more pitch bits, one or more voicing bits and one or
> more gain bits, but less than all of the quantizer bits for the
> frame, as recited in claim 1."

Pre-2003 MBE vocoders (including DVSI's own Hardwick '511 with its
seven FEC codewords) routinely **excluded voicing bits from the
strongest FEC protection**. '511 puts voicing in weaker Hamming codes.
Hardwick '089's bit allocation (Table 2 at col. 10) puts voicing among
the 33 unprotected bits. Both reflected a prevailing assumption that
voicing was less error-sensitive than pitch/gain.

The patent rests on the discovery that **voicing IS error-sensitive at
half-rate** — and therefore a half-rate vocoder must include voicing
bits in its strongest FEC codeword to achieve acceptable quality under
typical mobile-radio error conditions. This is the reason BABA-A
specifies the 4-pitch + 4-voicing + 4-gain MSB grouping in the
[24,12]-Golay-protected first codeword.

#### The Specific Factual Hook — Hardwick '089 Table 2 at Col. 10

The Appeal Brief's argument that flipped the examiner is precise and
fact-bound (page 6):

> "...this passage of Hardwick '089 describes a system that employs six
> FEC codewords (four Golay, two Hamming). In the system described by
> this passage of Hardwick '089, **voicing bits are not included in the
> extended Golay code** that protects the more sensitive fundamental
> frequency bits and gain bits. Indeed, it appears that the voicing
> bits would be included among the 33 bits that are left unencoded (see
> **Table 2 at col. 10 of Hardwick '089**, which sets forth the bit
> allocation)."

The examiner had cited '089 col. 16 lines 6–61 for "12 highest priority
bits...protected by a Golay code." DVSI's response was a precise
factual rebuttal: those 12 priority bits don't include voicing — see
'089's own bit-allocation table. Once the examiner verified this, the
rejection collapsed.

#### Implications for Design-Around — Narrower Than Initially Suggested

The unexpected-results / voicing-sensitivity foundation has a practical
consequence: **the patent's scope is anchored to the technical insight
that voicing bits need FEC protection at half-rate**. A design-around
that reverts to the pre-2003 prior-art structure (voicing bits
unprotected, or voicing bits in a separate weaker FEC code) would not
infringe — but it would also produce worse audio quality under bit-
error conditions, because the underlying technical insight is real.

For BABA-A-compatible AMBE+2 implementations, this is academic: the
wire format mandates the protected mixed codeword. But for hypothetical
non-interoperable half-rate vocoder designs, the patent does not
preclude a vocoder that simply makes the (technically inferior)
choice to leave voicing bits unprotected.

#### Claim Amendments (Original 2003 → Issued 2013)

The 87 originally filed claims all issued with the same numbering — none
were canceled. Substantive amendments were concentrated in the four
independent claims (1, 42, 60, 72). The dependent claims that specify
the AMBE+2 format details (4+4+4 grouping in claim 8, [24,12]+[23,12]
Golay in claims 9/14, data-dependent scrambling in claim 15, redundant
voicing codebook in claim 6, 0x3F tone identifier in claims 16–19, 7+5+5
bit allocations in claim 13) were **filed as written in 2003** and
issued unamended.

**Claim 1 — encoder side amendment:**

Original 2003 (claim 1, last two operative steps):
> "...combining one or more of the pitch bits with one or more of the
> voicing bits and one or more of the gain bits to create a first
> parameter codeword;"

Issued 2013:
> "...quantizing the model parameters to produce pitch bits...,
> voicing bits..., and gain bits..., **wherein the pitch bits, the
> voicing bits and the gain bits are included in quantizer bits for the
> frame**; combining one or more of the pitch bits with one or more of
> the voicing bits and one or more of the gain bits to create a first
> parameter codeword **that includes less than all of the quantizer
> bits for the frame**;"

The added phrase **"that includes less than all of the quantizer bits
for the frame"** is the prosecution-driven narrowing that overcame
Hardwick '089. The 2010-09-09 reply (response to the 2010-06-09 OA)
distinguished '089 by arguing that '089's voicing bits "would be
included among the 33 bits that are left unencoded" — i.e., '089
protected pitch + gain MSBs in its FEC codeword but did not protect
voicing bits in the same codeword. By restricting claim 1 to a first
codeword that is a strict subset of the total quantizer bits, DVSI
distinguished from '089's FEC structure.

**Claims 42 and 60 (decoder side)** received parallel amendments adding
"the extracted pitch bits, voicing bits and gain bits including less
than all of a set of quantizer bits for the frame."

**Claim 72 (variable-bit-rate spectral codebook decoder) was NOT
amended.** Original 2003 wording is identical to issued 2013 wording.
This claim chain (72–87) was conceded by the examiner well before the
Quayle Action — DVSI was forced to fight only over the encoder/decoder
mixed-codeword chain (1, 42, 60), not over the variable-rate spectral
chain. Effectively, US8359197 contains **two independent patentable
inventions** in one patent:

1. **Mixed pitch+voicing+gain FEC-protected first codeword** (claims
   1–71, narrowed to "less than all quantizer bits" during prosecution)
2. **Bit-count-dependent spectral codebook indexing** (claims 72–87,
   allowed as filed)

#### Doctrine of Equivalents — Surrendered Scope

By adding "less than all of the quantizer bits" through amendment to
overcome Hardwick '089, DVSI surrendered the scope where the first
parameter codeword contains **all** the frame's quantizer bits. Under
the *Festo* line of cases, prosecution history estoppel forecloses
DVSI from re-capturing that surrendered territory via the doctrine of
equivalents. This is a narrow carve-out: a hypothetical implementation
that puts every parameter bit inside the FEC-protected first codeword
would not infringe. AMBE+2 half-rate does not do this (the 12-bit
first codeword is far smaller than the ~49 voice bits per frame), so
the carve-out is academic for BABA-A-compatible implementations.

#### Sharper Design-Around Analysis

Combined with the prosecution-history limitation, the practical
infringement reads are:

- **Decoder side (any BABA-A-compatible half-rate decoder):
  unavoidable infringement.** The wire format mandates extracting the
  mixed pitch+voicing+gain first codeword. Both inventions
  (mixed-codeword chain and variable-rate spectral chain) are forced
  by the BABA-A bit allocations. Claim 72's "spectral codebook index
  determined at least in part by the number of bits in the frame"
  reads on AMBE+2's variable 25–32 spectral bits → variable codebook
  selection.
- **Encoder side (any BABA-A-interoperable half-rate encoder): same
  read.** To produce a bitstream consumable by an AMBE-3000, the
  encoder must produce the mixed first codeword.
- **Non-interoperable design-around space exists** but produces a
  vocoder that doesn't exchange voice frames with AMBE-3000. Examples:
  separate FEC codewords for each parameter type (avoids claims 1,
  42, 60); fixed-length spectral codebook independent of frame bit
  count (avoids claim 72). Either choice breaks bitstream
  interoperability.

### Cross-References to Existing Analysis

- `analysis/vocoder_missing_specs.md` — "Path Forward for Clean-Room
  Implementation" section needs the legal-status caveat reflecting
  US8359197's 2028 expiration
- `DVSI/AMBE-3000/AMBE-3000_Patent_Reference.md` §2 — US8595002
  disclosure (identical to US8359197 disclosure; algorithm details apply
  to both)
- USPTO file wrapper for application 10/402,938 — source for prosecution
  history, claim amendments, and prior-art discussion in this section.
  Pull from Patent Center or USPTO API for future updates.


---

## 7. US8265937 — Breathing-Apparatus Speech Enhancement (Fireground Patent)

- **Inventors:** Daniel W. Griffin et al.
- **Assignee:** Digital Voice Systems Inc
- **Application:** 12/021,789
- **Filed:** 2008-01-29
- **Granted:** 2012-09-11
- **PTA:** 516 days
- **Expired:** ~2032 (estimated based on PTA)
- **Examiner:** Talivaldis Ivars Smits (Art Unit 2626)

### Why This Patent Matters

This is the SCBA / fireground patent that connects directly to TIA-102.BABG's
15-noise-condition test plan (see `analysis/vocoder_missing_specs.md`).
BABG explicitly tests vocoder performance under fireground noise types
including PASS alarm, SCBA low-air alarm, fog nozzle, rotary saw, and
chainsaw. US8265937 is the patent on the techniques DVSI uses to handle
two of those conditions specifically: SCBA-environment speech enhancement
and PASS alarm detection.

### Examiner's Reasons for Allowance — Two Distinct Inventions

The 2012-06-06 Notice of Allowance contains the examiner's statement of
reasons for allowance, identifying two patentable inventions:

#### Invention 1 — Filtered Reference Noise Cancellation (Claims 1–13, 16–17)

> "Independent claim 1 is allowed because the closest prior art of
> record, **Yang**, does not teach **filtering the reference signal
> (interference plus noise) prior to subtracting the resultant estimate
> of the interference signal from the primary signal (speech plus
> interference plus other noise), to enhance speech in a breathing
> apparatus**. **Yang, and other prior art such as Kushner**, subtract
> an unprocessed noise signal (interference plus any additional noise)
> picked up from a separate channel from the noisy speech in the primary
> signal to remove extraneous noise contaminating the noise (interference)
> signal therefrom. This, in combination with the other recited
> limitations, makes claim 1 allowable."

The architecture is two-microphone noise cancellation specific to SCBA:

- Primary mic: speech + SCBA interference + ambient noise
- Reference mic: SCBA interference + ambient noise (no speech, placed
  behind mask or near regulator)
- Prior art (Yang, Kushner): subtract the reference signal directly from
  the primary signal
- US8265937: **filter the reference signal first**, then subtract — the
  filtering removes the "other noise" component contaminating the
  reference, producing a cleaner estimate of just the SCBA interference

#### Invention 2 — PASS Alarm Detection (Claims 20–21)

> "Independent claims 20 and 21 are allowed because they recite
> **declaring the exclusive presence of an alarm signal in a breathing
> apparatus when an alarm count exceeds a fourth threshold, by
> determining a peak count of consecutive energy samples below a first
> threshold, a valley count of consecutive energy samples above a second
> threshold, and an alarm count of the number of consecutive samples for
> which the peak count and valley count are below a third threshold**."

This is PASS (Personal Alert Safety System) alarm detection. The PASS
alarm is the loud chirping alarm carried by every firefighter on
fireground operations. The detection method uses a four-threshold
cascade:

1. **Peak count**: count of consecutive energy samples below threshold 1
   (low-energy gap between alarm chirps)
2. **Valley count**: count of consecutive energy samples above threshold 2
   (high-energy alarm chirp)
3. **Alarm count**: count of consecutive samples for which both peak and
   valley counts fall below threshold 3 (consistent alarm cadence)
4. **Declaration threshold**: alarm count ≥ threshold 4 → declare alarm

This four-stage cascade makes the detector robust to general loud
fireground noise (which has irregular peak/valley structure) while
sensitive to the regular cadence of an actual PASS alarm.

### Connection to BABG Test Conditions

TIA-102.BABG's 15 fireground noise environments include:

| BABG noise type | Maps to US8265937 |
|---|---|
| PASS alarm | Invention 2 (alarm detection cascade) |
| SCBA low-air alarm | Invention 2 (similar cadence-based detection) |
| SCBA mask breathing | Invention 1 (reference-channel filtering of breathing artifacts) |
| Fog nozzle, rotary saw, chainsaw | General-purpose noise reduction (not specifically claimed) |

US8265937 is therefore one of the practical techniques DVSI uses to
**pass** BABG conformance testing on fireground conditions, particularly
the alarm-tolerant decoding requirement.

### Implementation Status

The patent's expiration is estimated at ~2032 based on the 516-day PTA.
Until then, the filtered-reference noise cancellation and the four-
threshold PASS alarm detector are encumbered. For an open-source
implementation:

- **Generic two-mic noise cancellation** (without the filtering-before-
  subtraction step) is in the prior art (Yang, Kushner) and free to use
- **PASS alarm detection** can be done by frequency-domain matched
  filtering on the PASS alarm's specific spectral signature (~3 kHz
  fundamental, ~3 sec on / 5 sec off cadence per NFPA 1982) — this
  is a different technique not covered by claims 20–21

### Claim Form — System (Hardware) vs. Method

Per the granted patent XML, claim 1 is a **system claim requiring
physical hardware**:

> "1. A breathing apparatus speech enhancement system comprising:
> a breathing mask; a primary sensor on the breathing mask and
> configured to produce a primary signal; ..."

This is significant for design-around. Pure software implementations
of the filtered-reference noise cancellation algorithm — with no
physical SCBA hardware — likely fall outside claim 1's system-claim
scope. A general-purpose audio processing library that implements the
"filter the reference channel before subtracting" technique without
including a breathing apparatus and physical sensors does not infringe
the system claim.

The PASS alarm detection claims (20–21) need separate analysis — if
they are method claims rather than system claims, their scope reaches
software-only implementations regardless of hardware context.

---

## 8. US12254895 — Detecting and Compensating for Speaker Mask (active until ~2045)

- **Inventors:** Thomas Clark et al. (NEW DVSI inventor — neither
  Hardwick nor Griffin is the named first inventor)
- **Assignee:** Digital Voice Systems Inc
- **Application:** 17/366,782
- **Filed:** 2021-07-02
- **Granted:** 2025-03-18
- **Status:** **ACTIVE** — anticipated expiration ~2045 (no PTA per AIA
  rule changes; PTA determined post-issuance)
- **Examiner:** Satwant K Singh (Art Unit 2653 — different art unit
  than the half-rate vocoder family in 2626)
- **Examination type:** AIA (First-Inventor-to-File) — post-2013

### Why This Patent Matters

US12254895 is the modern successor to US8265937 — same problem domain
(mask environment speech) but addressed at a different layer: rather
than enhancing the input signal before encoding, this patent **detects
that a speaker mask is present in the speech signal and compensates**
within the codec. Active for another ~20 years, this is the most
forward-looking patent in DVSI's current portfolio for fireground use.

### Examiner's Reasons for Allowance — The Critical Citation Finding

> "The following is an examiner's statement of reasons for allowance:
> Claims 1, 12, and 23 of the current application teaches similar
> subject matter as the prior art of **Usher (US 2020/0077177)** and
> **Hardwick (US 2005/0278169)**. However, claims 1, 12, and 23 are
> allowed for the reasons pointed out by the applicant's remarks filed
> on 10/30/2024 (pages 7-9)."

**Hardwick US 2005/0278169 is the published application of US10/402,938
— DVSI's own AMBE+2 half-rate vocoder disclosure** (the application
that became US8359197 and US8595002). The examiner cited DVSI's own
AMBE+2 disclosure as the closest prior art to this 2025 mask-detection
patent.

This is significant for the project's understanding:

1. **DVSI is layering modern inventions on top of their (expiring or
   expired) AMBE+2 disclosure.** The half-rate vocoder framework is now
   the prior-art baseline that newer DVSI patents must distinguish from.
2. **Pages 7–9 of the 2024-10-30 applicant response** contain the
   substantive distinguishing argument. That document would describe
   what specifically about the AMBE+2 disclosure is insufficient for
   mask-environment speech, and what US12254895 adds.
3. **Mask detection is now layered ON TOP of the AMBE+2 framework**,
   not replacing it. An open-source AMBE+2 decoder is not encumbered by
   US12254895; only an AMBE+2 decoder that *also* implements specific
   mask-detection-and-compensation techniques would be.

### Cited but Not Relied Upon

> "Ostrand et al. (US 2023/0186942) discloses **detecting face mask
> usage based on a crowd sound**."

Ostrand has a face-mask detection patent application from 2023 in a
different domain (crowd-sound based). The examiner found it but did
not use it for rejection.

### Inventor-Line Implication

The first named inventor is **Thomas Clark**, not Hardwick or Griffin.
This is the first major DVSI patent in our reference set with a new
first inventor. DVSI's IP development is expanding beyond the founders
into a broader engineering team. The post-2020 patent line (US11715477
through US12462814) is being built by a mix of Clark, Griffin, and
Hardwick — the next-generation IP base for AMBE-4020.

### Implications for Open-Source Implementation

- **AMBE+2 half-rate decoder per BABA-A**: not affected by US12254895
- **AMBE+2 decoder + speaker-mask detection + parameter compensation**:
  encumbered until ~2045
- **Generic noise-robust speech processing without mask-specific
  detection**: not affected
- A research-quality fireground-tolerant open vocoder could implement
  the mask detection without compensation, or implement compensation
  without mask detection, and likely fall outside the patent's specific
  combination claims.

### The 2024-10-30 Remarks — What AMBE+2 Disclosure Specifically Lacks

DVSI's 2024-10-30 applicant response (pages 7–9) is the substantive
patentability argument. Pulled from the file wrapper, it reveals the
precise three-step pipeline that distinguishes US12254895 from both
Usher and the AMBE+2 disclosure:

1. **Suitability gate**: "using the speech parameters for the subframe
   in determining whether **the subframe is suitable for use in
   detecting a mask** worn by the speaker"
2. **Mask detection**: "upon determining that the subframe is suitable
   for use in detecting a mask, **using the speech parameters for the
   subframe in determining whether a mask is present**"
3. **Compensation**: "upon determining that a mask is present,
   **modifying the speech parameters for the subframe to produce
   modified speech parameters that compensate the speech signal for
   the presence of the mask**"

**Architectural distinction from Usher** (DVSI's primary distinguishing
argument):

> "Fundamentally, Usher is not directed to detecting the presence of a
> mask worn by a speaker in order to compensate the speech of the
> speaker based on the presence of the mask. Rather, Usher is directed
> to compensating for the impact of headwear worn by a user on a
> received signal that is used to modify the sound level of a signal
> delivered to the user. Stated another way, Usher is directed to
> modifying a signal delivered to the user while the claims are
> directed to modifying speech produced by the user."

Usher = receiver-side ambient sound modification (e.g., earplug
loudspeaker compensating for headwear muffling). US12254895 = encoder-
side speech modification (compensating the speaker's outgoing speech
for their mask).

**Distinction from AMBE+2 disclosure** (Hardwick US 2005/0278169) is
trivially short — DVSI dispensed with it in one sentence:

> "Hardwick does not remedy this failure of Usher."

Implication: **the AMBE+2 disclosure provides zero teaching of
speech-parameter-level mask detection or speech-parameter-level mask
compensation.** The mask-detection invention is genuinely additive to
AMBE+2, not a recombination of AMBE+2's existing components. A clean-
room AMBE+2 implementation does not inadvertently practice US12254895
unless it specifically adds the three-step pipeline above.

For implementation roadmap purposes, this confirms that BABG fireground
quality targets can be met by AMBE+2 + generic noise reduction without
falling into US12254895's scope. The mask-specific compensation in
US12254895 is **above and beyond** what BABG requires for conformance.

---

## 9. US11990144 — Reducing Perceived Effects of Non-Voice Data (active)

- **Inventor:** John C. Hardwick
- **Assignee:** Digital Voice Systems Inc
- **Application:** 17/387,412
- **Filed:** 2021-07-28
- **Granted:** 2024-05-21
- **Status:** ACTIVE (anticipated expiration ~2041)
- **Examiner:** Edgar X Guerra-Erazo (Art Unit 2656)
- **Examination type:** AIA

### Why This Patent Matters

This is an AMBE-4020-era patent specifically about discriminating
voice from non-voice data and reducing the perceptual artifacts when
non-voice content (data, tones, noise frames) leaks through the
codec. Likely covers DTX, comfort-noise insertion, and voice/data
discrimination at the codec level. Active for another ~16 years.

### Examiner's Reasons for Allowance (Brief)

- **Allowed claims:** 1–12 and 30–32 (claims 13–29 were canceled
  during prosecution — the original claim numbering had gaps after
  amendment)
- **Closest prior art:** Caffrey et al. US 2004/0253925 + Huang et al.
  US 2005/0281289
- The Reasons for Allowance themselves are boilerplate ("specific
  combinations of limitations stated in the claims"); the substantive
  distinction is in the 10/10/2023 applicant response, which would be
  the next document to pull if deeper analysis is needed.
- Two independent claims (1 and 30) — independent encoder and decoder
  variants is the typical DVSI pattern.

### Implications

- Doesn't directly affect AMBE+2 implementation
- Would affect any implementation that adds DTX-style silence handling
  or sophisticated voice/non-voice discrimination on top of an AMBE+2
  codec
- The 10/10/2023 applicant response (pulled from the file wrapper)
  documents the patent's load-bearing limitations in DVSI's own words,
  see "Substantive Distinguishing Argument" below.

### Substantive Distinguishing Argument (2023-10-10 Remarks)

The 2023-10-10 applicant response identifies claim 1's load-bearing
limitation as recited:

> "...selecting a frame of voice bits to carry the non-voice data;
> placing **non-voice identifier bits in a first portion** of the
> voice bits in the selected frame; and placing the **non-voice data
> in a second portion** of the voice bits in the selected frame..."

**This is the architectural pattern of in-band non-voice signaling
within an AMBE+2 voice channel** — generalizing the tone-frame
identifier of US8359197 to arbitrary non-voice payload. Where US8359197
flagged tone frames specifically (via the 0x3F disallowed pitch
identifier), US11990144 covers any non-voice data type (data,
signaling, diagnostics, etc.) being embedded in a voice frame slot.

**Distinguishing from Caffrey** (US 2004/0253925, the primary prior
art) — DVSI's argument:

> "Caffrey, which is directed to arranging data to correspond to **CD
> standard format** for subsequent transmission, does not employ
> frames of voice bits, does not select a frame of voice bits, does
> not place non-voice identifier bits in a first portion of the voice
> bits in the selected frame, and does not place the non-voice data
> in a second portion of the voice bits in the selected frame...
> Caffrey places non-voice data in a portion of the Red Book CD format
> frames already reserved for non-voice data. Accordingly, Caffrey does
> not replace voice bits with non-voice data..."

Caffrey is about CD audio (27-bit sync + 8-bit SUBCODE + 96-bit data
+ 32-bit parity + 96-bit data + 32-bit parity per frame) where non-
audio data lives in the reserved SUBCODE field. DVSI's distinction:
US11990144 takes a voice frame slot and **replaces voice bits with
non-voice data**, marked by an identifier in a designated first portion.

Distinguishing from Huang (US 2005/0281289) was again dispensed with
in one sentence: "Huang does not remedy this failure of Caffrey."

### Connection to P25 Implementation

US11990144's claim 1 architecture maps directly onto **P25's in-band
non-voice signaling**. P25 voice channels carry Link Control words,
Encryption Sync, and Low-Speed Data alongside voice frames; each
non-voice frame has an identifier prefix and a payload portion. The
patent's "first portion identifier + second portion data" structure
is the general legal scope covering this pattern.

### Tone/Non-Voice Frame Generation Map (Updated)

US11990144 fits as a fourth generation in DVSI's lineage of patents
covering non-voice frame handling within voice channels:

| Generation | Patent | Scope | Status (2026-04-27) |
|---|---|---|---|
| Gen 1 (2002) | US7970606 (§5) | Shared parameter quantizer for tones | Expired 2025-09-08 |
| Gen 2 (2003) | US8359197 (§6) | 0x3F disallowed-pitch identifier flagging tone frames + tone-amplitude data in first parameter codeword | Active to 2028-05-20 |
| Gen 3 (2025) | US12451151 (§10) | Template-matching tone detector with error-criteria threshold gate | Active to ~2042 |
| Gen 4 (2024) | US11990144 (this section) | **Generalized non-voice-data identifier + payload split** — covers arbitrary non-voice payload, not just tones | Active to ~2041 |

For a clean-room AMBE+2 implementation that handles BABA-A's specified
non-voice frame types, all four generations need to be considered.
Gen 1 is fully public domain; Gens 2–4 are encumbered until 2028, 2042,
and 2041 respectively.

---

## 10. US12451151 — Tone Frame Detector (active, PTAB-confirmed)

- **Inventor:** Thomas Clark et al. (same inventor as US12254895 §8)
- **Assignee:** Digital Voice Systems Inc
- **Application:** 17/716,845
- **Filed:** 2022-04-08
- **Granted:** 2025-10-21
- **Status:** ACTIVE (anticipated expiration ~2042)
- **Examiner:** Douglas Godbold (Art Unit 2655)
- **Examination type:** AIA

### Why This Patent Matters

US12451151 is the third-generation DVSI tone-handling patent, granted
following a PTAB decision that **reversed** the examiner's original
§ 103 rejection. This is the first patent in our reference set where
the appeal actually went to the Board and DVSI won on the merits —
earlier appeals (US8359197, US7970606) were withdrawn by the examiner
before reaching a PTAB decision.

### PTAB Win — More Authoritative Than Examiner Withdrawal

The 2025-09-09 Notice of Allowance explicitly states:

> "This Office Action is in response to **board decision dated 25
> August 2025, reversing the rejections** made in the Office Action
> dated 22 December 2023."

PTAB-confirmed patentability is a stronger legal posture than examiner
withdrawal because:
- The Board independently reviewed the prior art and the rejection
- The decision creates persuasive authority for subsequent prosecutions
- A challenger contesting validity would have to overcome both the
  examiner's eventual allowance AND the Board's affirmation

### Examiner's Statement of Reasons (per PTAB Decision)

> "Consider claim 1, **Hardwick** teaches a method for detecting and
> extracting tone data embedded in a voice bit stream that includes
> frames of bits (abstract), with some of the frames of bits being
> frames of non-tone bits and some of the frames of bits being frames
> of tone bits (0053, 0063, tone and voice), the method comprising:
> selecting a frame of bits from the voice bit stream (0080, receiving
> MBE parameters, 0050, parameters sent in frames from encoder);
> analyzing the selected frame of bits to determine whether the
> selected frame of bits is a frame of tone bits (0080, checking
> parameters to see if they are valid tone parameters); and when the
> selected frame of bits is a frame of tone bits, extracting tone
> data from the selected frame of bits (0080, valid tone frames are
> extracted and subjected towards additional noise filtering to
> remove coding noise)."

**The "Hardwick" reference is again the AMBE+2 disclosure** (Hardwick
US 2005/0278169). The cited paragraphs (0050, 0053, 0063, 0080) match
the AMBE+2 published application's tone-handling sections. AMBE+2's
tone handling validates parameters against a known set; that's prior
art for this 2025 patent.

What the PTAB held the prior art does NOT teach:

> "However, **per the board decision**, the prior art of record does
> not teach or suggest the limitations of: 'wherein analyzing the
> selected frame of bits comprises:
> - **comparing bits of the selected frame of bits to sets of tone
>   data to produce error criteria** representative of differences
>   between the selected frame of bits and each of multiple sets of
>   tone data,
> - **based on the error criteria, selecting a set of tone data that
>   most closely corresponds** to the bits of the selected frame of
>   bits,
> - and when the **error criteria corresponding to the selected set
>   of tone data satisfies a set of thresholds**, designating the
>   selected frame of bits as a frame of tone bits, and wherein
>   extracting tone data from the selected frame of bits comprises
>   **providing the selected set of tone data as the extracted tone
>   data**.'"

This is a **template-matching tone detector**:
1. Pre-compute or store a set of tone-data templates
2. Compare received frame bits against each template
3. Compute error criteria (Hamming distance, correlation, etc.)
4. Select the best-matching template
5. Apply a threshold gate — only declare a tone frame if the best
   match's error is below threshold
6. Output the selected template's tone data as the decoded result

### Three Generations of DVSI Tone Handling

This patent reveals that DVSI has incrementally improved tone detection
across three patent generations, each distinct from the previous:

| Generation | Patent | Approach | Status (2026-04-27) |
|---|---|---|---|
| Gen 1 (2002) | US7970606 (§5) | Shared parameter quantizer — encode tones via specific pitch + spectral values; decode by recognizing reserved values | Expired 2025-09-08 |
| Gen 2 (2003) | US8359197 (§6) | Disallowed-pitch identifier (0x3F) — flag tone frames via reserved pitch field values; tone amplitude in first parameter codeword | Active to 2028-05-20 |
| Gen 3 (2025) | US12451151 (this section) | Template matching with error-criteria threshold gate — store tone-data templates, find best match, threshold-gate the declaration | Active to ~2042 |

A modern open-source tone detector targeting AMBE+2 streams must
distinguish between generations: implementing Gen 1's approach is now
free, Gen 2 is encumbered until 2028, and Gen 3 is encumbered until
~2042 — each requires different design-around analysis.

### Implications for Open-Source Implementation

- **Decoding tone frames per BABA-A's specified format**: not affected
  by US12451151 — that follows the Gen 2 reserved-pitch approach which
  is in the BABA-A wire format
- **Implementing template-matching tone detection on AMBE+2 streams**:
  encumbered by US12451151 until ~2042
- **Doing tone detection by parameter validation alone** (the Gen 2
  Hardwick approach the PTAB held as prior art): not affected by
  US12451151, but still affected by US8359197 until 2028
- **For research/personal-use implementations**: Gen 1's shared-
  quantizer approach is fully free to use as of late 2025



---

## 11. Foreign Prosecution Overview — EP 1,465,158 (Half-Rate Vocoder, European)

The half-rate vocoder family was prosecuted internationally under the
Paris Convention. The European counterpart of US8359197 / US8595002 is
EP 1,465,158 ("Half-rate vocoder"), with separately-prosecuted divisional
EP 1,748,425. The technical disclosure is identical to the US application
(treaty requirement); only the claims and prosecution differ.

### EP 1,465,158 Prosecution Timeline

| Date | Event |
|---|---|
| 2003-04-01 | Priority date (US application 10/402,938) |
| 2004-03-26 | EP application filed (corresponds to US 2005/0278169 publication) |
| 2004-10-06 | Published as EP 1465158 A2 (without search report) |
| 2005-08-05 | EP search report dispatched (A3 publication) |
| 2006-02-20 | Examination requested |
| 2006-05-23 | **Communication of intention to grant** — single examination round |
| 2006-12-13 | **Granted as B1** |
| 2007-09-14 | Opposition deadline passed — **no opposition filed** |
| ~2023-04-01 | **Expired** in Europe (20 years from priority date) |

**Total prosecution time: 2.75 years**, with a single examination round
and no rejection. Compare to US prosecution: ~10 years, 4 Office Actions,
2 Final Rejections, 3 Notices of Appeal, an RCE, an in-person interview,
and an Ex Parte Quayle Action.

The dramatic prosecution-time difference suggests either:
- The EPO examiner viewed DVSI's distinguishing arguments as clearer than
  the USPTO examiner did
- The "less than all of the quantizer bits" narrowing required to
  overcome Hardwick '089 in the US wasn't required by the EPO

### EPO Prior-Art Citations — All DVSI's Own Work

The EPO search report cited (categorized X/Y/A — relevance designations
not directly visible in our extraction):

| Reference | Inventor / Assignee | Subject |
|---|---|---|
| US 5,081,681 | Hardwick (DVSI) | Method and apparatus for phase synthesis for speech processing |
| US 5,517,511 | Hardwick et al. (DVSI) | Bit prioritization with mixed Golay/Hamming FEC — known to us as "Hardwick '511" |
| US 5,664,051 | (likely DVSI) | (subject not verified) |
| US 5,715,365 | **Griffin + Jae S. Lim (DVSI)** | **Excitation parameter estimation via nonlinear operations on frequency bands — foundational MBE pitch/voicing math** |
| US 5,754,974 | Griffin (DVSI) | "Griffin '974" — known |
| US 5,826,222 | **Griffin (DVSI)** | **Hybrid excitation parameter estimation — extension of '365** |
| US 5,870,405 | (likely DVSI) | (subject not verified) |
| US 6,199,037 | Hardwick (DVSI) | Joint V/pitch quantization — known to us as §3 |
| **EP 0,893,791** | Hardwick + Jae S. Lim (DVSI) | Methods for encoding/enhancing/synthesizing speech via spectral-amplitude prediction-residual blocks |
| **TIA-102.BABA** | TIA (NPL — non-patent literature) | The IMBE standard itself, cited as prior art against AMBE+2 |

**No third-party prior art was cited.** Every patent reference is
DVSI's own work. The EPO examiner's case was that AMBE+2 was an
incremental improvement over DVSI's own foundational MBE patent
portfolio. DVSI distinguished, and the EPO accepted, in one round.

### Newly-Visible DVSI Foundational Patents

The EPO citations surface two DVSI patents that didn't appear in the
USPTO file wrapper for US8359197 but are foundational to MBE-family
excitation parameter estimation:

**US 5,715,365** — "Estimation of excitation parameters"
- Inventors: Daniel Wayne Griffin + Jae S. Lim
- Filed 1994-04-04, granted 1998-02-03, **expired 2015-02-03**
- Technical content: divides digitized speech into frequency bands,
  applies nonlinear operations to at least one band to emphasize the
  fundamental frequency, then estimates pitch and voicing from the
  modified band signals
- Public domain since 2015 — implementations of nonlinear-band-
  emphasis pitch detection are unencumbered

**US 5,826,222** — "Estimation of excitation parameters"
- Inventor: Daniel Wayne Griffin
- Filed 1997-04-14, granted 1998-10-20, **expired 2015-01-12**
- Technical content: hybrid extension of '365 — uses two different
  estimation methods on the modified frequency band signals and
  combines results for improved pitch/voicing accuracy across
  conditions
- Public domain since 2015

Both patents pre-date AMBE+2 by ~10 years and represent the
mathematical foundation of MBE-family pitch/voicing detection. They
are likely the **best public-domain reference for implementing
excitation parameter estimation** in a clean-room MBE/AMBE-compatible
codec — better than reverse-engineering AMBE+2 binaries because the
math is explicitly disclosed in the patent specifications.

Worth adding to the implementation roadmap as a positive (use-this)
reference rather than a constraint.

### EP 1,748,425 — Divisional

The EPO required DVSI to split EP 1,465,158 into two applications via
divisional, confirming that the EPO held the parent contained two
distinct inventions (matching our US claim 1/42/60 vs. claim 72
two-inventions analysis from §6). EP 1,748,425 covers the spectral-
codebook chain that would have been claim 72 in the US filing.

Both EP 1,465,158 and EP 1,748,425 expired ~2023 (20 years from
priority date). Fully public domain in Europe.

### No Opposition — Family Uncontested in Europe

The 9-month post-grant opposition window for EP 1,465,158 closed
2007-09-14 with no third-party opposition. Despite this being the
foundational AMBE+2 patent, no European competitor (no third-party
codec vendor, no European telecom equipment maker) challenged
validity. This is consistent with no commercial competitor offering
an AMBE+2-compatible codec — DVSI's licensing model captures the
market without challenge.

### Implications

- The EPO patent's broader scope (no "less than all" narrowing) was
  available to DVSI for ~17 years (2006-2023) but is now expired.
  All technical content and all claims of EP 1,465,158 are public
  domain in Europe today.
- The newly-visible US 5,715,365 and US 5,826,222 DVSI patents are
  positive references for clean-room MBE excitation estimation work,
  with disclosure of the nonlinear-band-emphasis pitch detection
  mathematics.
- No hidden European prior art exists in the AMBE+2 citation graph.
  The patent family is built entirely on DVSI's own incremental work,
  and the EPO accepted the patentability story without resistance.

### DVSI Foundational MBE Patents (Pre-2003) — Public Domain References

The EPO citation set surfaced the existence of DVSI's foundational
pre-AMBE+2 MBE patent portfolio. The granted-patent specifications
have been pulled and characterized below. All three are expired and
in the public domain — they are **positive references** for clean-
room MBE/AMBE-compatible work, not constraints.

#### US 5,081,681 — Phase Synthesis (Predecessor to US5701390)

- **Inventors:** John C. Hardwick + Jae S. Lim
- **Assignee:** Digital Voice Systems Inc, Cambridge MA
- **Filed:** 1989-11-30
- **Granted:** 1992-01-14
- **Expired:** ~2010 (17-year term from grant per pre-1995 rules)
- **Claims:** 22

This is the **original phase synthesis patent** from the founders of
DVSI, predating both AMBE+2 and the spectral-envelope phase
regeneration of US5701390. The technical content:

- Recreates phase signals for voiced harmonics from fundamental
  frequency and V/UV information (rather than transmitting phase
  directly)
- Adds a random component to the recreated phase to improve
  synthesized speech quality
- Foundation for all subsequent MBE-family phase synthesis work

For clean-room implementation, US 5,081,681's specification is the
authoritative public-domain disclosure of how to do basic phase-
recreation-from-fundamental in an MBE decoder. It pre-dates the more
sophisticated spectral-envelope-derived phase of US5701390 (which is
also now expired, see §1) but is the simpler approach that an
implementation can start from.

#### US 5,664,051 — Speech Decoder with Random Phase

- **Inventors:** Hardwick + Jae S. Lim (DVSI, Burlington MA)
- **Filed:** 1994-06-23
- **Granted:** 1997-09-02
- **Expired:** ~2014

Decoder-side patent: an analyzer that processes the digitized speech
bit stream to recover sinusoidal component parameters (angular
frequencies, magnitudes), a random phase generator producing time
sequences of random phase components, a phase synthesizer that
generates synthesized phases from angular frequencies + random
components, and a synthesizer that combines all three. Closer to
the IMBE decoder architecture than '681; documents a complete
sinusoidal speech synthesizer with random phase injection.

#### US 5,870,405 — Spectral Parameter Smoothing for Robustness

- **Inventors:** Hardwick + Jae S. Lim (DVSI)
- **Filed:** 1996-03-04
- **Granted:** 1999-02-09
- **Expired:** ~2016

Title mentions "communication channel" — patent on smoothing spectral
parameters in a speech decoder, almost certainly for **error robustness
in narrow-band digital land mobile radio applications**. Cited
references include Brandstein "A Real-Time Implementation of the
Improved MBE Speech Coder" (IEEE 1990) and "Application of the IMBE
Speech Coder for narrow-band Digital Land Mobile Radio" (IEEE 1990) —
these are the early P25/IMBE references that connect this DVSI patent
directly to the land-mobile-radio context.

This is likely the public-domain reference for **decoder-side spectral
smoothing under bit errors** — a technique any P25-compatible
implementation needs for fielded radio conditions. Worth reading the
full specification when implementing error-robust MBE decoding.

### Updated Citation Graph — DVSI's Pre-2003 Portfolio

Combining USPTO citations (from §§4–10) and EPO citations (this
section), DVSI's pre-2003 MBE patent portfolio that supports the
AMBE+2 family includes:

| Patent | Inventors | Subject | Expired |
|---|---|---|---|
| US 4,797,926 | Hardwick + Lim | Original IMBE digital speech vocoder | ~2007 |
| US 5,081,681 | Hardwick + Lim | Phase synthesis (this section) | ~2010 |
| US 5,517,511 | Hardwick et al. | Bit prioritization with mixed Golay/Hamming FEC ("'511") | ~2015 |
| US 5,664,051 | Hardwick + Lim | Speech decoder with random phase (this section) | ~2014 |
| US 5,701,390 | Griffin + Hardwick | MBE synthesis with regenerated phase (§1) | 2015-02-22 |
| US 5,715,365 | Griffin + Lim | Excitation parameter estimation via nonlinear ops on frequency bands | 2015-02-03 |
| US 5,754,974 | Griffin | Spectral magnitude representation ("'974") | (date in EPO grant) |
| US 5,826,222 | Griffin | Hybrid excitation parameter estimation (extension of '365) | 2015-01-12 |
| US 5,870,405 | Hardwick + Lim | Spectral parameter smoothing (this section) | ~2016 |
| US 6,161,089 | Hardwick et al. | "'089" — bit prioritization with [24,12] Golay (the prior art the USPTO examiner used against US8359197) | ~2017 |
| US 6,199,037 | Hardwick | Joint V/pitch quantization (§3) | 2017-12-04 |
| EP 0,893,791 | Hardwick + Lim | Spectral-amplitude prediction-residual blocks (European; corresponds to US 5,754,974 family) | (EP) |

**All twelve patents are now public domain.** This is the complete
mathematical foundation of MBE/AMBE-family speech coding, freely
available for clean-room implementation. The AMBE+2-specific
techniques (§§2, 6, 8, 10) layer on top; an implementation built on
these foundational disclosures plus the BABA-A wire format (TIA
specification) covers everything needed for IMBE/AMBE+2-compatible
decoding without infringing the active patents listed in §§6, 7, 8,
9, 10.
