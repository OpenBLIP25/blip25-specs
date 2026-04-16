# AMBE-3000 Encoder Implementation Spec (PCM → Channel Bits)

**Status:** Draft  
**Date:** 2026-04-16  
**Author:** Chance Lindsey  
**Scope:** Software encoder functionally equivalent to the DVSI AMBE-3000 chip
in encoder direction, for P25 full-rate (7200 bps, r33) and P25 half-rate
(3600 bps, r34) as the normative targets, parameterized over the chip's
rate table (r0–r63) where the same algorithmic skeleton applies.

**Sources:**
- TIA-102.BABA-A Revision A (February 2014) — IMBE full-rate and AMBE+2
  half-rate wire format, FEC, quantizer tables, and forward MBE analysis.
- `analysis/vocoder_analysis_encoder_addendum.md` — derived spec for the
  BABA-A baseline analysis encoder (PCM → MbeParams). This spec **defers
  the entire PCM analysis pipeline** (HPF, windowing, DFT, pitch estimation,
  V/UV determination, spectral magnitude estimation, prediction residual,
  frame-type dispatch) to the addendum and covers only the wire/quantization
  layer that converts MbeParams → channel bits.
- US8595002 (expired 2023-04-01) — half-rate AMBE+2 quantization
  (gain, PRBA, HOC vector quantizers; V/UV codebook).
- US6199037 (expired 2017-12-04) — joint subframe voicing/pitch
  quantization (relevant for 2-subframe rates such as r17/r18).
- US5701390 (expired 2015-02-22) — voicing-independent spectral
  magnitude estimation (already absorbed into BABA-A Eq. 43/44).
- DVSI AMBE-3000F Vocoder Chip Users Manual v4.0 (October 2021) — rate
  table, RCW format, wire protocol. See `AMBE-3000_Protocol_Spec.md`.
- AMBE-3000 HDK test vectors — `/mnt/share/P25-IQ-Samples/DVSI
  Software/Docs/AMBE-3000_HDK_tv/`. See `AMBE-3000_Test_Vector_Reference.md`.

**Companion documents in this directory:**
- `AMBE-3000_Decoder_Implementation_Spec.md` — channel bits → PCM (mirror
  of this spec). Many cross-frame state and rate-dispatch sections are
  shared.
- `AMBE-3000_Objectives.md`, `AMBE-3000_Patent_Reference.md`,
  `AMBE-3000_Protocol_Spec.md`, `AMBE-3000_Operational_Notes.md`
- `annex_tables/rate_index_table.csv`, `annex_tables/rate_control_words.csv`

**Language convention:** C reference snippets are illustrative, not
normative Rust. Spec is language-neutral per project conventions.

---

## 1. Scope and Architecture

### 1.1 What This Encoder Does

Given a stream of 8 kHz 16-bit PCM samples, produce 144 bits per 20 ms
(full-rate r33) or 72 bits per 20 ms (half-rate r34) that, when fed to
either an AMBE-3000 chip decoder or to the software decoder
(`AMBE-3000_Decoder_Implementation_Spec.md`), reproduce the input speech
with the standard's algorithmic delay (~80 ms) and quantization quality.

The encoder factors into two layers:

1. **PCM analysis layer** — produces the MBE model parameters
   `(ω̂₀, L̂, v̂_k, M̂_l)` from PCM. Entirely deferred to
   `analysis/vocoder_analysis_encoder_addendum.md`.
2. **Wire/quantization layer** — converts MbeParams to channel bits.
   This spec.

### 1.2 Block Diagram

```
   8 kHz Q15 PCM (160 samples per 20 ms,
                  + 80 ms lookahead per addendum §0.1.3)
            │
            ▼
   ┌────────────────────────────┐
   │  PCM analysis pipeline     │   addendum §0.1–§0.10 — black box
   │  (HPF → window → DFT →     │   to this spec. Outputs MbeParams.
   │  pitch → V/UV → magnitudes │
   │  → log-mag prediction)     │
   └─────────────┬──────────────┘
                 │ MbeParams: (ω̂₀, L̂, v̂_k, M̂_l, T̂_l, frame_type)
                 ▼
   ┌────────────────────────────┐
   │  §3 Frame-type dispatch    │   silence / tone / erasure / voice
   └─────────────┬──────────────┘
                 │
                 ▼
   ┌────────────────────────────┐
   │  §4 Forward quantization   │   §4.1 pitch (b̂₀)
   │  (per rate)                │   §4.2 V/UV codebook search (b̂₁ — half-rate)
   │                            │   §4.3 spectral mags (b̂₂..b̂_N)
   └─────────────┬──────────────┘
                 │ b̂₀..b̂_N
                 ▼
   ┌────────────────────────────┐
   │  §5 Bit prioritization     │   pack b̂_k into û_0..û_M vectors
   │  (forward scan)            │   per BABA-A Tables 15–18 / Annexes
   └─────────────┬──────────────┘
                 │ û_0..û_M
                 ▼
   ┌────────────────────────────┐
   │  §6 Forward FEC            │   [24,12] ext Golay + [23,12] Golay
   │  (per rate)                │   + PN modulation of ĉ₁
   └─────────────┬──────────────┘
                 │ ĉ_0..ĉ_M
                 ▼
   ┌────────────────────────────┐
   │  §7 Interleaver            │   Annex S (half-rate) or Annex H (full-rate)
   └─────────────┬──────────────┘
                 │
                 ▼
        72 or 144 channel bits (one frame)
                 │
                 ▼
   ┌────────────────────────────┐
   │  §8 Closed-loop decoder     │   internal matched decoder runs in parallel
   │  feedback (encoder-internal │   to update R̃_l(−1) predictor state per
   │  decode, addendum §0.6.6)   │   addendum §0.6.6 closed-loop convention
   └────────────────────────────┘
```

### 1.3 Relationship to Existing Specs

This spec **does not duplicate** any of the following:

| Topic | Source of truth |
|-------|-----------------|
| PCM analysis (HPF, windowing, DFT, pitch, V/UV, magnitude estimation) | `analysis/vocoder_analysis_encoder_addendum.md` |
| Bit prioritization tables and CSVs | `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §1.4, §2.3 + committed CSVs |
| Forward Golay/Hamming encoder algorithms | `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §1.5 |
| PN sequence generator | BABA-A impl spec §1.6, §2.5 (also restated in decoder spec §3.2.1) |
| Annex H / Annex S interleavers | BABA-A impl spec §1.7, §2.6 + committed CSVs |
| Annex E/F/G/L/M/N/O/P/Q/R quantizer tables | BABA-A impl spec §12 + committed CSVs |
| Rate table (r0..r63 metadata) | `annex_tables/rate_index_table.csv`, `annex_tables/rate_control_words.csv` |

This spec **adds**:
- Forward VQ search for V/UV (Annex M codebook), gain (Annex O), PRBA
  (Annex P, Q), and HOC (Annex R) — the encoder-side counterparts to the
  decoder lookups
- AMBE-3000-specific frame-type dispatch ordering
- Cross-frame predictor state for the closed-loop encoder (matched-decoder
  feedback, addendum §0.6.6)
- Bit-budget reduction conventions for non-P25 half-rate variants
  (US8595002 col. 22 "subset codebook" trick)
- Validation plan against tv-std encoder-side test vectors

### 1.4 Frame Timing

Same as decoder spec §1.5. Encoder additionally requires:
- **Lookahead buffer** of ~73.75 ms (≈ 590 samples at 8 kHz) per
  addendum §0.1.3 to align analysis windows ahead of the output frame
- **Cold-start preroll** of ≥ 4 frames to populate pitch history
  `P̂_{−4}` and predictor state `R̃_l(−1)` per addendum §0.3.7 + §0.6.7
- **End-to-end algorithmic delay**: ~80 ms (analysis lookahead + synthesis
  OLA on the receive side)

---

## 2. Output Frame Formats

Mirror of decoder spec §2; restated for completeness so this spec is
self-contained on the wire interface.

### 2.1 Full-Rate IMBE (r33, P25 Phase 1 FDMA)

**Channel frame:** 144 bits per 20 ms = 7200 bps.

| Vector | Info bits | FEC out | Content |
|--------|----------:|--------:|---------|
| ĉ₀ | 12 | 23 ([23,12] Golay) | Pitch MSBs + spectral MSBs |
| ĉ₁ | 12 | 23 ([23,12] Golay + PN) | Spectral magnitude bits |
| ĉ₂ | 12 | 23 ([23,12] Golay) | Spectral magnitude bits |
| ĉ₃ | 12 | 23 ([23,12] Golay) | Spectral magnitude bits |
| ĉ₄ | 11 | 15 ([15,11] Hamming) | Spectral magnitude bits |
| ĉ₅ | 11 | 15 ([15,11] Hamming) | Spectral magnitude bits |
| ĉ₆ | 11 | 15 ([15,11] Hamming) | Spectral magnitude bits |
| ĉ₇ | 7  | 7 (uncoded) | Lowest-priority spectral bits |
| **Total** | **88** | **144** | |

PN modulation applied to ĉ₁ only. Annex H interleaver applied to the 144
output bits before transmission. Forward chain: `b̂_k → bit prioritization
→ û_k → FEC → ĉ_k → PN modulate ĉ₁ → Annex H interleave → 144 bits`.

### 2.2 Half-Rate AMBE+2 (r34, P25 Phase 2 TDMA)

**Channel frame:** 72 bits per 20 ms = 3600 bps.

| Vector | Info bits | FEC out | Content |
|--------|----------:|--------:|---------|
| ĉ₀ | 12 | 24 ([24,12] ext Golay) | Pitch + V/UV + gain MSBs |
| ĉ₁ | 12 | 23 ([23,12] Golay + PN) | PRBA MSBs |
| ĉ₂ | 11 | 11 (uncoded) | HOC MSBs |
| ĉ₃ | 14 | 14 (uncoded) | All LSBs |
| **Total** | **49** | **72** | |

PN modulation applied to ĉ₁ only. Annex S interleaver applied to the 72
output bits.

**Information bit allocation** (BABA-A §13, this spec §4):

| Symbol | Bits | Codebook | Role |
|--------|-----:|----------|------|
| b̂₀ | 7 | Annex L | Pitch (forward of decoder §4.1.2) |
| b̂₁ | 5 | Annex M | V/UV codebook index — VQ search per §4.2 |
| b̂₂ | 5 | Annex O | Differential gain — scalar quant per §4.3.2 |
| b̂₃ | 9 | Annex P | PRBA₂₄ — VQ search per §4.3.3 |
| b̂₄ | 7 | Annex Q | PRBA₅₈ — VQ search per §4.3.3 |
| b̂₅ | 5 | Annex R | HOC₁ — VQ search per §4.3.4 |
| b̂₆ | 4 | Annex R | HOC₂ — VQ search per §4.3.4 |
| b̂₇ | 4 | Annex R | HOC₃ — VQ search per §4.3.4 |
| b̂₈ | 3 | Annex R | HOC₄ — VQ search per §4.3.4 |
| **Total** | **49** | | |

---

## 3. Frame-Type Dispatch (Encoder Side)

Source: addendum §0.8 covers the full algorithm including DVSI-black-box
silence and tone detection. This section is a one-page summary of how
the dispatch outputs feed the §4 quantizers.

### 3.1 Dispatch Decision

After PCM analysis produces MbeParams + frame energy + tone-detect flag,
choose one of:

| Decision | Trigger | Encoded as | §4 path |
|----------|---------|------------|---------|
| Voice | default | b̂₀ ∈ [0, 119] | §4 normal |
| Silence | input energy below threshold | b̂₀ = 124 or 125 (BABA-A §13.1 Table 14) | §4 skipped; emit silence frame template |
| Tone | tone-detect flag set | b̂₀ ∈ [126, 127] | §4 skipped; emit BABA-A §2.10 tone-frame layout |
| Erasure | bad input / preroll | b̂₀ ∈ [120, 123] | §4 skipped; emit erasure marker |

### 3.2 Silence Frame Template

For half-rate (r34), per BABA-A §6.1 + addendum §0.8.4:
- Set `b̂₀ ∈ {124, 125}` (124 = standard silence; 125 = comfort-noise variant
  if rate supports it)
- Set `b̂₁ = 0` (all-voiced codebook entry; combined with b̂₀ ∈ silence range,
  the decoder treats the frame as silence per its §9.4)
- All other `b̂_k = 0`

Skip §4 entirely; pack the silence bits into û_0..û_3 per Tables 15–18,
then run §6 FEC and §7 interleave normally.

### 3.3 Tone Frame Template

For half-rate (r34), per BABA-A §16 + addendum §0.8.5:
- Tone detection input: `(I_D, A_D)` from addendum §0.8.5 detector
- Pack per BABA-A Table 20 (decoder spec also references this in §9.4):
  - û₀(11..6) = `0x3F` (signature)
  - û₀(5..0) = `A_D(6..1)`
  - û₁(11..4) = `I_D(7..0)` (copy 1)
  - û₁(3..0) + û₂(10..7) = `I_D(7..0)` (copy 2, split)
  - û₂(6..0) + û₃(13) = `I_D(7..0)` (copy 3, split)
  - û₃(12..5) = `I_D(7..0)` (copy 4)
  - û₃(4) = `A_D(0)`
  - û₃(3..0) = `0000`

Skip §4 entirely; the tone-frame û_0..û_3 are then FEC encoded and
interleaved normally. Tone frames use the same FEC + PN + Annex S as
voice frames (BABA-A §16.2 final paragraph).

### 3.4 Tone Detection — DVSI-Black-Box

Per addendum §0.8.5: tone-detect *entry criteria* are not specified in
the BABA-A PDF. The PDF specifies only the emitted bit layout. Match
the AMBE-3000 chip's observed behavior empirically (e.g., FFT-based
peak detection with ~3% frequency tolerance and SNR threshold).

**Open question:** the chip's exact detection thresholds and hysteresis
are not documented. Reference behavior can be characterized by feeding
known DTMF-and-noise PCM into the chip and recording when it transitions
from voice frames to tone frames. Flagged in §12.

---

## 4. Forward Quantization

The encoder side of decoder §4. Decoder direction was "look up codebook
entry by index"; encoder direction is "find the codebook entry whose
reconstruction best matches the input."

### 4.1 Pitch Quantization (b̂₀)

#### 4.1.1 Full-Rate (r33)

Source: BABA-A §6.1 Eq. 45–48 (page 21–22). Forward of decoder spec §4.1.1.

```
b̂₀ = ⌊ (P̂ − 19.875) · 4 ⌋        clamped to [0, 207]      (Eq. 45)
```
where `P̂` is the refined pitch period in samples (range 19.875..123.125 per
addendum §0.4.5). The 4× multiplier corresponds to 0.25-sample resolution.

```c
uint8_t imbe_pitch_quantize(double P_hat) {
    if (P_hat < 19.875) P_hat = 19.875;
    if (P_hat > 71.875) P_hat = 71.875;   /* upper for 8-bit field */
    int32_t b0 = (int32_t)floor((P_hat - 19.875) * 4.0);
    if (b0 < 0)   b0 = 0;
    if (b0 > 207) b0 = 207;
    return (uint8_t)b0;
}
```

#### 4.1.2 Half-Rate (r34)

Source: BABA-A §13.1 Eq. 143–144 (page 58), US8595002 col. 17 lines 35–50.

7-bit logarithmic quantizer covering 120 levels. Forward formula:

```
b̂_fund = 0                                  if f₀ > 0.0503
b̂_fund = 119                                if f₀ < 0.00811
b̂_fund = ⌊ −195.626 − 45.368 · log₂(f₀) ⌋   otherwise
```

where `f₀ = ω̂₀ / (2π)` is the fundamental frequency normalized to [0, 0.5].

```c
uint8_t ambe_pitch_quantize(double omega_0) {
    double f0 = omega_0 / (2.0 * M_PI);
    if (f0 > 0.0503)  return 0;
    if (f0 < 0.00811) return 119;
    double q = floor(-195.626 - 45.368 * log2(f0));
    if (q < 0)   q = 0;
    if (q > 119) q = 119;
    return (uint8_t)q;
}
```

**Quantization error feedback.** The forward quantizer's reconstructed
`f̂₀ = f₀(b̂_fund)` (via Annex L lookup) does not equal the input `f₀`
exactly. Subsequent stages (§4.3 spectral magnitudes; §6.5 phase regen on
the decoder) operate on the *quantized* `f̂₀`, not the input. The encoder
must therefore re-derive `L̂` from the *quantized* `f̂₀` for use in its own
§4.3 quantization (closed-loop matching with the decoder).

### 4.2 V/UV Codebook Quantization (b̂₁ — Half-Rate Only)

Source: BABA-A §13.2 Eq. 145–149, US8595002 col. 18 lines 20–55, Annex M
(32-entry × 8-element codebook).

The encoder receives per-band V/UV decisions `v̂_k` for k = 1..K̂ from
the analysis pipeline (addendum §0.7). It must choose the 5-bit codebook
index `b̂₁` whose decoded per-harmonic vector best matches the input.

#### 4.2.1 Reduce v̂_k to 8-Element Logical Band Vector

The encoder's analysis produces K̂ per-band decisions. The half-rate
codebook is fixed at 8 logical bands. Map the K̂-element vector to an
8-element vector by replicating each `v̂_k` across the harmonics it
covers, then re-sampling onto the codebook's 8-band grid via the same
indexing the decoder uses (decoder §4.2.2):

```c
/* Project per-harmonic V/UV (length L̂) to 8-band logical V/UV vector
 * using the same codebook indexing as the decoder. The 8-band vector
 * is a *target* — we then VQ-search the Annex M codebook to find the
 * closest match.                                                       */
void ambe_vuv_project(const uint8_t v_harm[/* L */],
                      double omega_0,
                      uint8_t L,
                      double v_target[8])
{
    int count[8] = {0};
    int sum[8]   = {0};
    for (uint8_t l = 1; l <= L; l++) {
        int j = (int)floor((double)l * 16.0 * omega_0 / (2.0 * M_PI));
        if (j < 0) j = 0;
        if (j > 7) j = 7;
        sum[j]   += v_harm[l - 1];
        count[j] += 1;
    }
    for (int j = 0; j < 8; j++)
        v_target[j] = (count[j] > 0) ? (double)sum[j] / count[j] : 0.0;
}
```

The `v_target[j]` values are in [0, 1] — fractional voicing per band,
weighted by the number of harmonics in each band.

#### 4.2.2 VQ Search Over Annex M

For each of the 32 codebook entries, compute a distance to `v_target`
and pick the minimum. Distance is harmonic-count-weighted Hamming-like:

```c
/* Annex M codebook: ambe_vuv_codebook[n][0..7] ∈ {0, 1}. Search returns
 * the index n that minimizes the harmonic-weighted squared distance to
 * v_target.                                                            */
uint8_t ambe_vuv_quantize(const double v_target[8],
                          const int harms_per_band[8])
{
    double best_d = 1e9;
    uint8_t best_n = 0;
    for (int n = 0; n < 32; n++) {
        double d = 0.0;
        for (int j = 0; j < 8; j++) {
            double diff = v_target[j] - (double)ambe_vuv_codebook[n][j];
            d += harms_per_band[j] * diff * diff;
        }
        if (d < best_d) { best_d = d; best_n = (uint8_t)n; }
    }
    return best_n;
}
```

**Codebook entry semantics** (US8595002 col. 19 lines 10–30):
- Indices 0 and 1 both decode to "all voiced" (8-element all-1s vector).
  Encoder may pick either; convention is to pick the lower index when
  both score equally.
- Indices 16–31 all decode to "all unvoiced" (8-element all-0s vector).
  Same tie-breaking rule.
- The 14 indices 2..15 cover the meaningful mixed-voicing patterns.

The dual-mapping for all-voiced / all-unvoiced is for upgrade
compatibility: a V1 decoder that doesn't know the full 32-entry codebook
can still produce sensible output if the encoder uses the canonical
indices (0, 16) for the dual-mapped patterns.

#### 4.2.3 Silence-Frame Override

When §3.1 dispatches a silence frame, force `b̂₁ = 0` regardless of
analysis output (decoder spec §9.4 silence handling expects this).

### 4.3 Spectral Magnitude Quantization

The forward of decoder §4.3. Order:
1. **Compute log-magnitude prediction residual** `T̂_l` from input
   magnitudes — addendum §0.6 covers this.
2. **Quantize gain** (b̂₂) — §4.3.2
3. **Per-block DCT** of residual → coefficient blocks — §4.3.3
4. **Extract PRBA vector** from first 2 DCT coeffs of each block — §4.3.3
5. **Extract HOC vectors** from coeffs 3..6 of each block — §4.3.4
6. **VQ search** for PRBA (Annex P, Q) and HOC (Annex R) — §4.3.3, §4.3.4

The pipeline ends with `(b̂₂..b̂₈)` ready for bit prioritization (§5).

#### 4.3.1 Block Layout

Same Annex N partition the decoder uses (decoder §4.3.1). For each L̂,
Annex N gives 4 contiguous block sizes `(J_1, J_2, J_3, J_4)` summing
to L̂. Block 1 = lowest harmonics; block 4 = highest.

#### 4.3.2 Gain Quantization (b̂₂)

Source: BABA-A §13.4.1 Eq. 168 reversed. Forward of decoder §4.3.2.

The encoder computes the differential gain:
```
γ̂(0)    = mean(T̂_l)                          (frame mean log-magnitude)
Δ̂_γ    = γ̂(0) − 0.5 · γ̃(−1)                (Eq. 168 reversed)
```
`γ̃(−1)` is the *reconstructed* gain from the previous voice frame (closed-
loop predictor; see §8.1).

Forward quantization is a nearest-neighbor search over Annex O's 32-entry
quantizer (the decoder's `AMBE_GAIN_QUANTIZER[32]` table):

```c
uint8_t ambe_gain_quantize(double delta_gamma) {
    /* AMBE_GAIN_QUANTIZER is the same 32-entry table the decoder uses;
     * ordered by reconstruction value. Find the entry minimizing the
     * absolute difference. Linear search is fine — table is tiny.     */
    double best_d = 1e9;
    uint8_t best_b = 0;
    for (int b = 0; b < 32; b++) {
        double d = fabs(delta_gamma - AMBE_GAIN_QUANTIZER[b]);
        if (d < best_d) { best_d = d; best_b = (uint8_t)b; }
    }
    return best_b;
}
```

The reconstructed gain Δ̂_γ_rec = `AMBE_GAIN_QUANTIZER[b̂₂]` then drives
the encoder's matched-decoder predictor: `γ̃(0) = Δ̂_γ_rec + 0.5·γ̃(−1)`.

#### 4.3.3 PRBA Vector Quantization (b̂₃, b̂₄)

Source: BABA-A §13.4.2 Eq. 169–178 reversed, US8595002 col. 20 lines 5–50,
Annex P (9-bit, 512-entry, 3-element codebook), Annex Q (7-bit, 128-entry,
4-element codebook).

**Step 1 — Per-block DCT.** For each block i = 1..4, compute the length-J_i
DCT of the residual `T̂_l` for harmonics in block i:
```
C_i = DCT_{J_i}(T̂_{l_start(i)..l_end(i)})
```

The first two DCT coefficients of each block become the PRBA elements:
```
G_{2i}     = C_i[0]    (DC coefficient of block i)
G_{2i+1}   = C_i[1]    (first AC coefficient of block i)
```
yielding G_2..G_9 (8 elements, indexed 2..9 by BABA-A convention; G_1 = 0
is reserved). After the §13.4.2 mixing transform (Eq. 171–178), these split
as PRBA₂₄ = `(G_2, G_3, G_4)` and PRBA₅₈ = `(G_5, G_6, G_7, G_8)`. (G_9 is
not transmitted; recompute as needed for closed-loop matching.)

**Step 2 — VQ search.**

```c
/* PRBA24: 9-bit codebook, 512 entries × 3 elements (Annex P).         */
uint16_t ambe_prba24_quantize(const double prba24[3]) {
    double best_d = 1e9;
    uint16_t best_b = 0;
    for (int b = 0; b < 512; b++) {
        double d = 0.0;
        for (int k = 0; k < 3; k++) {
            double diff = prba24[k] - ANNEX_P[b][k];
            d += diff * diff;
        }
        if (d < best_d) { best_d = d; best_b = (uint16_t)b; }
    }
    return best_b;
}

/* PRBA58: 7-bit codebook, 128 entries × 4 elements (Annex Q).         */
uint8_t ambe_prba58_quantize(const double prba58[4]) {
    double best_d = 1e9;
    uint8_t best_b = 0;
    for (int b = 0; b < 128; b++) {
        double d = 0.0;
        for (int k = 0; k < 4; k++) {
            double diff = prba58[k] - ANNEX_Q[b][k];
            d += diff * diff;
        }
        if (d < best_d) { best_d = d; best_b = (uint8_t)b; }
    }
    return best_b;
}
```

Linear search over 512 + 128 = 640 entries with 3+4 = 7 multiplies per
entry costs ~5000 multiplies per frame — well within real-time budget on
any modern CPU. No tree-structured search is needed; the codebooks are
small enough.

**Distance metric.** Plain unweighted Euclidean above. US8595002 does not
specify a perceptual weighting in the search; the codebooks were trained
on plain Euclidean distance and the encoder's optimum is also plain
Euclidean. Adding psychoacoustic weighting at the encoder will
*degrade* match to DVSI reference rather than improve perceptual quality.

#### 4.3.4 HOC Vector Quantization (b̂₅, b̂₆, b̂₇, b̂₈)

Source: BABA-A §13.4.3 Eq. 179–181 reversed, US8595002 col. 21–22, Annex R
(four codebooks: 5/4/4/3-bit, 4-element each).

For each block i = 1..4, the higher-order DCT coefficients (indices 2..5
within the block; indices beyond J_i − 1 are zero) form the HOC vector:
```
HOC_i = (C_i[2], C_i[3], C_i[4], C_i[5])
```
zero-padded if J_i < 6.

```c
/* HOC search: ANNEX_R[k] is the codebook for HOC block k+1
 * (so ANNEX_R[0] = HOC1, sized 32×4; ANNEX_R[3] = HOC4, sized 8×4).   */
uint8_t ambe_hoc_quantize(int k, const double hoc[4]) {
    int N_entries = annex_r_sizes[k];   /* 32, 16, 16, 8 */
    double best_d = 1e9;
    uint8_t best_b = 0;
    for (int b = 0; b < N_entries; b++) {
        double d = 0.0;
        for (int e = 0; e < 4; e++) {
            double diff = hoc[e] - ANNEX_R[k][b][e];
            d += diff * diff;
        }
        if (d < best_d) { best_d = d; best_b = (uint8_t)b; }
    }
    return best_b;
}
```

**Block-too-small handling.** If `J_i < 3`, the block has no meaningful
higher-order coefficients. Per US8595002 col. 21 line 30 and BABA-A §13.4.3,
the corresponding HOC bits should still be transmitted (encode as the
codebook entry closest to the all-zeros vector — typically `b̂_{4+i} = 0`).
The decoder zero-fills these per its §4.3.4 zeroing rule, so the round-trip
is consistent.

**Bit-reduction trick (US8595002 col. 22 lines 10–25).** Some non-P25
half-rate variants drop bits by restricting the search to a subset of
codebook entries. Implement this by parameterizing `N_entries` per rate
(e.g., r35 might use only the first 16 entries of HOC1 → b̂₅ = 4 bits
instead of 5). The reduced-bit codebook entries are the first 2^k entries
of the full codebook; the search algorithm itself is unchanged. Per-rate
limits live in the rate-dispatch table (§10).

#### 4.3.5 Closed-Loop Decoder for Predictor Update

After §4.3.2–§4.3.4 produce `(b̂₂..b̂₈)`, the encoder must update its
internal `R̃_l(−1)` predictor state using the *quantized* values, not the
input. This is the "matched-decoder feedback" of addendum §0.6.6:

```c
/* Run the decoder's §4.3 reconstruction on the just-emitted b̂_k to get
 * the new M̃_l(0). This becomes M̃_l(−1) for the next frame's predictor.
 * Same code as the decoder's §4.3.5 ambe_mag_reconstruct().            */
double M_tilde_curr[L_hat];
ambe_mag_reconstruct(/* T̃_l from b̂₂..b̂₈ */, L_hat,
                     state->M_tilde_prev, state->L_prev,
                     /* rho per BABA-A Eq. 185 */, M_tilde_curr);
memcpy(state->M_tilde_prev, M_tilde_curr, L_hat * sizeof(double));
state->L_prev = L_hat;
```

Without this step the encoder's predictor diverges from the decoder's,
causing a slow drift in spectral magnitudes that shows up as quality
degradation after ~50 frames (1 second of speech). Closed-loop is
mandatory for correct AR predictor behavior.

### 4.4 Cross-Frame State (Encoder Side)

Mirror of decoder spec §4.4, with encoder-specific additions per addendum
§0.9.1:

| State | Init | Updated by | Consumed by |
|-------|------|-----------|-------------|
| HPF state | 0 | addendum §0.1.2 | addendum §0.1.2 next sample |
| PCM lookahead buffer | 0 | per-sample input | addendum §0.1.3 windowing |
| `P̂_{−4}, P̂_{−2}` pitch history | 21.0 (low-pitch sentinel) | addendum §0.3.4 | addendum §0.3.4 next frame |
| `E_{−4}, E_{−2}` autocorr history | 1.0 (no-error-floor sentinel) | addendum §0.3.5 | addendum §0.3.5 |
| `ω̂₀(−1), L̂(−1)` | analysis cold-start values | §4.1.2 reconstruction | §4.3.5 closed-loop predictor |
| `ξ_max(−1)` V/UV envelope | 0 | addendum §0.7.4 | addendum §0.7.4 |
| `M̃_l(−1)` matched-decoder mags | 1.0 for l=1..56 | §4.3.5 closed-loop | §4.3.5 next frame |
| `γ̃(−1)` matched-decoder gain | 0 | §4.3.2 closed-loop | §4.3.2 next frame |
| Frame index / preroll counter | 0 | per frame | §3 dispatch (preroll → erasure) |

Cold start = init column. The first ~4 frames are emitted as **erasure
frames** (b̂₀ ∈ [120, 123]) per addendum §0.8.6 + §0.9.4 preroll policy
because the lookahead and pitch-history buffers aren't yet populated.

---

## 5. Bit Prioritization (Forward)

Source: BABA-A §1.4 (full-rate), §2.3 (half-rate). The encoder packs the
quantized `b̂_k` parameters into the prioritized bit vectors `û_0..û_M`.

### 5.1 Half-Rate (r34)

Direct table lookup. Use the same `ambe_bit_map[49]` CSV that the decoder
walks in reverse — committed at
`standards/TIA-102.BABA-A/annex_tables/ambe_bit_prioritization.csv`.

```c
/* Forward direction: b[9] in, u[4] out. Walks the same 49-entry map
 * the decoder uses, but reads from b[] and writes to u[].             */
void ambe_pack(const uint16_t b[9], uint16_t u[4]) {
    u[0] = u[1] = u[2] = u[3] = 0;
    for (int i = 0; i < 49; i++) {
        const ambe_bit_map_entry_t *e = &ambe_bit_map[i];
        uint8_t bit = (b[e->src_param] >> e->src_bit) & 1u;
        u[e->dst_vec] |= ((uint16_t)bit) << e->dst_bit;
    }
}
```

49 bit-copy operations per frame — trivial cost.

### 5.2 Full-Rate (r33)

Source: BABA-A §1.4 priority scan. The full-rate map is L̂-dependent
(spectral amplitude bit allocations vary with the number of harmonics).

Defer to BABA-A impl spec §1.4 + the L̂-indexed CSVs at
`standards/TIA-102.BABA-A/annex_tables/imbe_bit_prioritization_*.csv`.
The forward direction reads the CSV to map `(b̂_k, bit_in_b̂_k) →
(û_n, bit_in_û_n)` and copies bits accordingly.

The full-rate scan algorithm itself (Figure 22 of BABA-A) is encoded in
the CSV; the encoder is a table walk, not an algorithm.

---

## 6. Forward FEC

### 6.1 Half-Rate (r34) Forward FEC

Per BABA-A §2.4 Eq. 189–192 (page 68):
```
ĉ₀ = û₀ · G_{24,12}            (Eq. 189) — 24-bit codeword
ĉ₁ = û₁ · G_{23,12} ⊕ m̂₁       (Eq. 190) — 23-bit codeword, PN-modulated
ĉ₂ = û₂                         (Eq. 191) — 11 bits, uncoded
ĉ₃ = û₃                         (Eq. 192) — 14 bits, uncoded
```

#### 6.1.1 [24,12] Extended Golay Encode

Standard systematic encoder. Apply [23,12] Golay, then append overall
parity bit:

```c
uint32_t golay_24_12_encode(uint16_t info_12b) {
    uint32_t golay23 = golay_23_12_encode(info_12b);  /* 23 bits */
    uint32_t parity  = (uint32_t)__builtin_parity(golay23);
    return (golay23 << 1) | parity;                   /* 24 bits */
}
```

The [23,12] forward encoder is shared with full-rate; see BABA-A impl
spec §1.5.1 for the generator polynomial and bit-mux conventions.

#### 6.1.2 [23,12] Golay Encode

Same code as full-rate. Generator polynomial `G(x) = x^11 + x^10 + x^6
+ x^5 + x^4 + x^2 + 1` (0xC75 MSB-first). Defer to BABA-A impl spec §1.5.1.

#### 6.1.3 PN Modulation of ĉ₁

Source: BABA-A §2.5 (decoder spec §3.2.1 has the C reference).

```c
/* p_r(0) = 16 * b̂₀ (8-bit pitch),  p_r(n+1) = (173·p_r(n) + 13849) mod 2^16
 * m̂₁(n) = MSB(p_r(n+1)) for n = 0..22.                                */
static uint32_t halfrate_pn_modulation(uint16_t b0_pitch) {
    uint32_t p_r = ((uint32_t)b0_pitch) * 16u;
    uint32_t m1  = 0;
    for (int n = 0; n < 23; n++) {
        p_r = (173u * p_r + 13849u) & 0xFFFFu;
        m1 |= ((p_r >> 15) & 1u) << n;
    }
    return m1;
}

/* Apply: c1_pn = c1 ^ halfrate_pn_modulation(b0)                       */
```

Note the seed uses the **8-bit pitch field** — for half-rate this is the
7-bit `b̂₀` shifted left by 1 (b̂₀ value 0..119 maps to seed nibble 0..238).
Verify against test vectors: an off-by-one in the seed shift will produce
72-bit frames that decode correctly (Golay tolerates the bit flips) but
fail bit-exact match to DVSI reference.

#### 6.1.4 Uncoded Pass-Through

`ĉ₂ = û₂` (11 bits) and `ĉ₃ = û₃` (14 bits). Direct copy. No PN.

### 6.2 Full-Rate (r33) Forward FEC

Defer entirely to BABA-A impl spec §1.5:
- ĉ₀..ĉ₃ : [23,12] Golay encode of û₀..û₃
- ĉ₄..ĉ₆ : [15,11] Hamming encode of û₄..û₆
- ĉ₇    : uncoded copy of û₇
- PN modulation applied to ĉ₁ (and ĉ₂, ĉ₃, ĉ₄ per BABA-A §1.6 — verify
  against the impl spec, since full-rate PN coverage is more complex than
  half-rate's "ĉ₁ only").

---

## 7. Interleaving

### 7.1 Half-Rate (Annex S)

Source: BABA-A §2.6 + Annex S CSV (all 36 dibit positions).

Forward: scatter `(ĉ₀, ĉ₁, ĉ₂, ĉ₃)` (24+23+11+14 = 72 bits) into 36
dibit positions per the Annex S table.

```c
/* Inverse of decoder_spec §3.4 deinterleave. Reads c[0..3] (24/23/11/14 bits),
 * writes 36 dibits in 0..3.                                            */
void interleave_ambe_halfrate(const uint32_t c[4], uint8_t dibits_out[36]);
```

CSV: `standards/TIA-102.BABA-A/annex_tables/annex_s_interleave.csv`
(36 rows, columns `symbol, msb_src, msb_bit, lsb_src, lsb_bit`).

### 7.2 Full-Rate (Annex H)

Source: BABA-A §1.7 + Annex H CSV. Forward: scatter `(ĉ₀..ĉ₇)` (144 bits)
into the air-interface position per Annex H.

Defer to BABA-A impl spec §1.7. The TDMA burst-level interleaver (BBAC-1
Annex E) is **separate** from the vocoder-level Annex H/S — handled by
the air-interface layer downstream of this encoder.

---

## 8. Output Formation and Closed-Loop Feedback

### 8.1 Output

After §7 interleaving:
- Half-rate: 72 bits → 36 dibits (passed to TDMA burst layer per
  TIA-102.BBAC-1 / TIA-102.BBAC-A) or 9 bytes (host-protocol per
  `AMBE-3000_Protocol_Spec.md` PKT_CHAND packet)
- Full-rate: 144 bits → 18 bytes (host-protocol PKT_CHAND) or LDU1/LDU2
  embedded position per TIA-102.BAAA-B

The host-protocol packetization is documented in `AMBE-3000_Protocol_Spec.md`
§3 (PKT_CHAND). The encoder's responsibility ends at the 72/144-bit
channel-bits boundary; packetization for transport is a separate concern.

### 8.2 Closed-Loop Decoder Feedback

After emitting the channel bits, the encoder runs an internal decode of
its own quantized parameters and updates the `R̃_l(−1)`, `M̃_l(−1)`,
`γ̃(−1)` predictor state. This is mandatory; see §4.3.5 and addendum §0.6.6.

The matched decoder is *not* the full §5–§7 decoder (no synthesis, no
phase regen, no PCM output) — only the inverse-quantization stage of
decoder spec §4.3 is needed. Reuse `ambe_mag_reconstruct()` and the
gain/PRBA/HOC inverse-lookup helpers directly.

---

## 9. Encoder Error and State Management

### 9.1 No Invalid-Frame Path

The encoder always produces a valid 72/144-bit frame. There is no
"give up on this frame" path equivalent to the decoder's invalid-frame
handling — all input PCM is encoded. Bad inputs (silence, noise,
out-of-range pitch) get mapped to silence or erasure frames at §3
dispatch, but a frame is always emitted.

### 9.2 Cold Start / Reset

On encoder reset, initialize per the §4.4 state table. The first ~4
frames after reset emit erasure markers per §3.4 preroll policy until
the analysis lookahead is populated.

### 9.3 Saturation and Numerical Limits

PCM input is Q15. Internal computations use double precision per
addendum §0.10. After spectral magnitude estimation, magnitudes are
typically in [0, 32768] range; log₂ of zero is handled by the analysis
addendum (clamps to a floor before §4.3 quantization).

If quantizer search distances overflow (extremely loud input or very
small previous-frame predictor), clamp the search distance accumulator
to `1e18` rather than letting it become Inf. None of the §4 search
loops above include such guards because the codebook entries are
bounded — verify under stress testing.

---

## 10. Rate Dispatch (r0..r63)

Mirror of decoder spec §10. Same RCW format and rate-table CSVs apply.
Encoder additions per rate:

| Rate field | Effect on encoder |
|------------|-------------------|
| Bit rate / frame size | Output buffer size |
| Sub-frame count | Whether to encode 2 frames per RCW (joint quant per US6199037) |
| FEC scheme | Which §6 path |
| Bit allocation | Which `b̂_k` widths (per-rate `info_bits` table) |
| Scrambling | PN seed derivation per rate (P25 uses `16·b̂₀`; others may differ) |
| Interleaver | Annex H / Annex S / rate-specific |
| Codebook subset | §4.3 VQ search may use reduced `N_entries` per rate |

### 10.1 P25 Rates (Encoder)

| Rate | Bit rate | §3 dispatch | §4 quantization | §6 FEC | §7 interleave |
|------|---------:|-------------|-----------------|--------|---------------|
| r33  | 7200 bps | full-rate dispatch (BABA-A §6.1) | §4.1.1 + BABA-A §1.8 forward | BABA-A §1.5 | Annex H |
| r34  | 3600 bps | half-rate dispatch (BABA-A §13.1) | §4.1.2 + §4.2 + §4.3 | §6.1 | Annex S |

### 10.2 Multi-Subframe Rates (US6199037)

Rates r17, r18, and a few others encode **2 subframes per frame** — the
encoder analyzes 40 ms of input (two 20 ms subframes) and produces a
single channel-bits frame containing jointly-quantized parameters.

US6199037 specifies:
- **Joint pitch quantization**: 4-bit scalar quantizer on average
  log-pitch + 6-bit vector quantizer on the difference between subframes
- **Joint voicing quantization**: 6-bit codebook on a 16-element vector
  (8 bands × 2 subframes), or split as two 8-element 6-bit codebooks

These rates are out of scope for the P25-focused validation in §11 but
the data structures must support them — the encoder's `b̂_k` array
becomes `b̂_k[2]` for two-subframe rates, and §4 expands accordingly.

### 10.3 Dispatch C Skeleton

```c
typedef struct {
    /* same descriptor as decoder spec §10.4, plus encoder-only fields */
    const ambe_codebook_subset_t *codebook_subset;   /* per-rate VQ subset */
    uint16_t pn_seed_multiplier;                     /* 16 for P25 half */
    uint8_t  subframes_per_frame;                    /* 1 or 2 */
} ambe_rate_descriptor_t;

int ambe_encode_frame(uint8_t rate,
                      const int16_t *pcm_in,        /* 160 or 320 samples */
                      uint8_t *frame_out,           /* frame_bits bits packed */
                      ambe_encoder_state_t *state);
```

---

## 11. Validation Plan

### 11.1 Test Vector Pipeline

```
  .pcm file           ┌─────────────┐
  (rate r,         →  │  software   │  →  software .bit
   8 kHz Q15)         │  encoder    │
                      └─────────────┘
                                            ┌───── compare ────→  metrics
  .pcm file           ┌─────────────┐       │
  (same)          →   │  AMBE-3000  │  →  reference .bit (tv-std/rN/)
                      │  chip       │
                      └─────────────┘
```

Test vectors: `/mnt/share/P25-IQ-Samples/DVSI Software/Docs/AMBE-3000_HDK_tv/`
(same dataset as decoder; `.pcm` files are encoder inputs).

### 11.2 Expected Match Quality by Stage

Encoder bit-exactness is **harder** than decoder bit-exactness. The
analysis pipeline (addendum §0.1–§0.7) involves floating-point accumulators,
threshold tests, and tie-breaking that can diverge from DVSI's reference.

| Stage | Expected vs chip | Notes |
|-------|------------------|-------|
| §3 dispatch (silence/voice/tone) | **likely diverges** | DVSI thresholds black-box; expect ~95% agreement |
| §4.1 pitch quantization | bit-exact **given matching analysis** | quantizer is deterministic |
| §4.2 V/UV codebook search | bit-exact **given matching analysis** | linear search is deterministic |
| §4.3.2 gain quantization | bit-exact given matching γ̂(0) | nearest-neighbor over Annex O |
| §4.3.3 PRBA VQ search | bit-exact given matching residual | no tie-breaking ambiguity in 9/7-bit codebooks |
| §4.3.4 HOC VQ search | bit-exact given matching residual | same |
| §5 bit prioritization | bit-exact | deterministic table walk |
| §6 FEC encode | bit-exact | well-defined codes |
| §7 interleave | bit-exact | deterministic permutation |
| **Output frame bits** | partial bit-exact | dominated by §3 + analysis pipeline divergence |

The realistic target is "decoder fed the encoder's output produces PCM
that matches DVSI's chip-decoded output **of DVSI's own encode** within
~2–3 dB SNR." Bit-exact matching of the encoder is a much higher bar
that requires reverse-engineering DVSI's analysis-pipeline tie-breaking.

### 11.3 Recommended Test Order

1. **Silence input** (`tv-std/r34` silence inputs) — validates §3.1
   dispatch and silence-frame template (§3.2)
2. **Single-sinusoid input** (synthetic PCM at known pitch) — validates
   §4.1 pitch quantization and §4.3 spectral magnitudes (only one
   harmonic active, so the PRBA/HOC quantization is constrained)
3. **DTMF input** — validates §3.3 tone-frame detection + §3.4 tone
   bit-layout
4. **Voiced speech** (`tv-std/r34` 'alert' input) — end-to-end §3–§7;
   compare bit-by-bit with reference
5. **Mixed-voicing speech** — exercises V/UV codebook search edge cases
6. **Round-trip via decoder spec**: encode `tv-std/r34/in.pcm` to bits,
   decode back to PCM, compare to `tv-std/r34/out.pcm`. Round-trip SNR
   target: ≥ 12 dB (encoder + decoder cumulative)

### 11.4 Debug Order on Mismatch

1. **Diff MbeParams first.** Dump `(ω̂₀, L̂, v̂_k, M̂_l, T̂_l)` for each
   frame from software analysis vs a reference (e.g., dump from a
   previous-known-good run, or back-derive from chip-decoded PCM via
   inverse pipeline). Most mismatches are in the analysis layer
   (addendum §0.1–§0.7), not in §4–§7.
2. **Diff per-stage `b̂_k`.** If MbeParams match but `b̂_k` differ, the
   forward quantizer has a bug — usually a tie-breaking mismatch in
   §4.2 / §4.3 VQ search.
3. **Diff `û_k`.** If `b̂_k` match but `û_k` differ, the bit
   prioritization map is misindexed — verify CSV row alignment.
4. **Diff `ĉ_k`.** If `û_k` match but `ĉ_k` differ, the FEC encoder or
   PN modulator has a bug. Most common: PN seed shift (off-by-one),
   wrong [23,12] generator polynomial bit-order.
5. **Diff dibit output.** If `ĉ_k` match but final bits differ, the
   interleaver is misindexed.

### 11.5 Round-Trip Validation

Once both encoder and decoder converge against reference:
- **Software encode → software decode → measure**: validates round-trip
  closure within software (no chip dependency)
- **Software encode → chip decode → measure**: validates encoder against
  chip's decode side
- **Chip encode → software decode → measure**: validates decoder against
  chip-sourced bits (already covered by decoder spec §11.5)
- **All-software vs chip-only round-trip**: if SNR within 3 dB and MCD
  within 0.5 dB, declare functional equivalence

---

## 12. Open Questions / Analysis TODOs

Inherited from decoder spec §12 + encoder-specific items.

1. **DVSI tone-detect entry criteria** (§3.4). Black-box; characterize
   empirically. File: `analysis/ambe3000_tone_detect_entry.md`.
2. **DVSI silence-detect threshold** (addendum §0.8.4). Black-box;
   characterize empirically. File: `analysis/ambe3000_silence_threshold.md`.
3. **PN seed for non-P25 rates** (§6.1.3). Same item as decoder spec §12
   item 5. P25 uses `16·b̂₀`; others may differ.
4. **VQ search tie-breaking convention** (§4.2, §4.3). When two codebook
   entries score equally, DVSI may pick lowest index, highest index, or
   most-recent index. Characterize empirically to bit-match. File:
   `analysis/ambe3000_vq_tiebreak.md`.
5. **Multi-subframe joint quantization details** (§10.2). US6199037
   describes joint pitch and voicing quantization; the exact codebooks
   for chip rates r17/r18 are not in BABA-A. Black-box characterization
   needed for non-P25 rate-conversion targets.
6. **Closed-loop predictor convergence** (§4.3.5, §8.2). The matched
   decoder must be bit-identical to the wire decoder. Validate by
   round-tripping a known sinusoid through encode → decode and verifying
   the encoder's `M̃_l(−1)` predictor state matches what a fresh decoder
   would have computed at the same frame.
7. **Inherited from decoder**: γ_w unvoiced calibration, BABA-A Eq. 185
   ρ predictor coefficient, log_2(0) floor. Same investigations.

---

## 13. Appendices

### Appendix A — C Reference Fragments (Consolidated)

```c
/* =========================================================================
 *  AMBE-3000 encoder reference — non-normative C skeleton.
 *  Use alongside addendum (PCM analysis) and decoder spec (cross-frame
 *  state, FEC primitives).
 * ========================================================================= */

#include <math.h>
#include <stdint.h>

/* --- §3 frame-type dispatch ------------------------------------------ */
typedef enum {
    AMBE_FRAME_VOICE,
    AMBE_FRAME_SILENCE,
    AMBE_FRAME_TONE,
    AMBE_FRAME_ERASURE
} ambe_frame_type_t;

ambe_frame_type_t ambe_dispatch(const ambe_analysis_out_t *a,
                                 const ambe_encoder_state_t *state);

/* --- §4.1 pitch quantization ----------------------------------------- */
uint8_t imbe_pitch_quantize(double P_hat);          /* full-rate */
uint8_t ambe_pitch_quantize(double omega_0);        /* half-rate */

/* --- §4.2 V/UV codebook quantization (half-rate) --------------------- */
void ambe_vuv_project(const uint8_t v_harm[], double omega_0, uint8_t L,
                      double v_target[8]);
uint8_t ambe_vuv_quantize(const double v_target[8],
                          const int harms_per_band[8]);

/* --- §4.3 spectral magnitude quantization (half-rate) ---------------- */
uint8_t  ambe_gain_quantize(double delta_gamma);
uint16_t ambe_prba24_quantize(const double prba24[3]);
uint8_t  ambe_prba58_quantize(const double prba58[4]);
uint8_t  ambe_hoc_quantize(int k, const double hoc[4]);

/* --- §5 bit prioritization (half-rate) ------------------------------- */
void ambe_pack(const uint16_t b[9], uint16_t u[4]);

/* --- §6 forward FEC (half-rate) -------------------------------------- */
uint32_t golay_24_12_encode(uint16_t info_12b);
uint32_t golay_23_12_encode(uint16_t info_12b);
uint32_t halfrate_pn_modulation(uint16_t b0_pitch);

/* --- §7 interleave (half-rate) --------------------------------------- */
void interleave_ambe_halfrate(const uint32_t c[4], uint8_t dibits_out[36]);

/* --- §8 closed-loop feedback ----------------------------------------- */
void ambe_encoder_predictor_update(const uint16_t b[9],
                                   uint8_t L_hat,
                                   ambe_encoder_state_t *state);

/* --- top-level orchestration ----------------------------------------- */
int ambe_encode_frame(uint8_t rate,
                      const int16_t *pcm_in,
                      uint8_t *frame_out,
                      ambe_encoder_state_t *state);
```

### Appendix B — Test Vector Coverage Matrix (Encoder Side)

| Test vector | Rate | Exercises | Coverage priority |
|-------------|------|-----------|-------------------|
| `tv-std/r34/*silence*.pcm` | r34 | §3.1, §3.2 | 1 |
| Synthetic single-sinusoid PCM | r34 | §4.1, §4.3 trivial | 2 |
| `tv-std/r34/*dtmf*.pcm` | r34 | §3.3, §3.4 tone path | 3 |
| `tv-std/r34/alert.pcm` | r34 | full §3–§7 voice | 4 |
| `tv-std/r33/*.pcm` | r33 | full-rate path | 5 |
| Round-trip (encoder + decoder spec §11) | both | cumulative SNR | 6 |
| `cmpp25.txt` recipe | r33 + P25 FEC | P25 integration | 7 |

### Appendix C — Cross-Reference Quick Index

| Topic | This spec | Addendum | Decoder spec | BABA-A impl spec |
|-------|-----------|----------|--------------|------------------|
| PCM HPF, windowing, framing | — | §0.1 | — | — |
| 256-pt DFT, S_w(m, ω₀) basis | — | §0.2 | — | — |
| Initial pitch estimation | — | §0.3 | — | — |
| Pitch refinement | — | §0.4 | — | — |
| Spectral magnitude estimation (Eq. 43/44) | — | §0.5 | — | — |
| Log-magnitude prediction residual | — | §0.6 | — | — |
| V/UV determination (D_k, ξ_*, θ_G) | — | §0.7 | — | — |
| Frame-type dispatch | §3 | §0.8 | §9.4 | §5–§6 |
| Pitch quantization (forward) | §4.1 | §0.4.4 | §4.1 (inverse) | §6.1 |
| V/UV codebook quantization | §4.2 | §0.7 + map to codebook | §4.2 (inverse) | §13.2 |
| Spectral mag quantization (gain/PRBA/HOC forward) | §4.3 | references | §4.3 (inverse) | §13.4 |
| Closed-loop predictor | §4.3.5, §8.2 | §0.6.6 | — | — |
| Bit prioritization (forward) | §5 | — | — | §1.4 / §2.3 |
| FEC encoding (forward) | §6 | — | §3 (inverse) | §1.5 / §2.4 |
| PN modulation | §6.1.3 | — | §3.2.1 | §1.6 / §2.5 |
| Interleaving | §7 | — | §3.4 (inverse) | §1.7 / §2.6 |
| Encoder state | §4.4 | §0.9 | — | — |
| End-to-end orchestration | §1.2, App. A | §0.10 | — | — |
| Rate dispatch | §10 | — | §10 | — |
