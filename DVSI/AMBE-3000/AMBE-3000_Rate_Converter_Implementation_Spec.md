# AMBE-3000 Rate Converter Implementation Spec (Channel Bits → Channel Bits, Parametric)

**Status:** Draft  
**Date:** 2026-04-16  
**Author:** Chance Lindsey  
**Scope:** Software parametric rate converter functionally equivalent to the
DVSI AMBE-3000 chip in repeater/transcoder mode (`PKT_RPT_MODE`). Converts
channel bits at one AMBE rate to channel bits at another rate **without a
PCM round-trip**, operating entirely in MBE parameter space. Primary
targets: P25 full-rate (p25_fullrate) ↔ P25 half-rate (p25_halfrate), and P25 ↔ external
AMBE-based standards (DMR, D-STAR) via shared MBE parameter representation.

**Sources:**
- US7634399 (expires 2025-11-07) — voice transcoder. Primary algorithmic
  source for parametric rate conversion. Patent describes the core
  parameter-space conversion algorithm: voicing band normalization,
  spectral magnitude interpolation, unvoiced-band compensation, ρ=0.65
  prior-frame prediction, invalid-frame substitution.
- US7957963 (expired 2023-01-30) — continuation of US7634399 covering
  same technology with different claim language. Same algorithm.
- TIA-102.BABA-A — wire format and quantizer tables for both source and
  target rates when both are P25.
- US8595002 (expired 2023-04-01) — half-rate quantization tables (Annex
  L/M/N/O/P/Q/R) used at both ends of P25-half bridges.
- US6199037 (expired 2017-12-04) — joint subframe quantization for
  multi-subframe rates (relevant to chip rates r17/r18 and similar).
- DVSI AMBE-3000F Vocoder Chip Users Manual v4.0 — `PKT_RPT_MODE`
  configuration, repeater rate-pair selection, RCW field semantics.
- DVSI test vectors — `/mnt/share/P25-IQ-Samples/DVSI Vectors/tv-rc/`
  (97 input files + nested per-source-rate output directories spanning
  64 rate indices + D-STAR + P25 cross-overs; driver `cmprc.txt`).
  See `AMBE-3000_Test_Vector_Reference.md` §1.1 for the distinction
  between this normative tree and the HDK evaluation-kit tree.

**Companion documents in this directory:**
- `AMBE-3000_Decoder_Implementation_Spec.md` — channel bits → MbeParams
  (this spec reuses §3 + §4 of the decoder verbatim as its front half).
- `AMBE-3000_Encoder_Implementation_Spec.md` — MbeParams → channel bits
  (this spec reuses §4 + §5 + §6 + §7 of the encoder as its back half).
- `AMBE-3000_Objectives.md` — "Why parametric rate conversion" rationale
  (12% intelligibility improvement over tandeming).
- `AMBE-3000_Patent_Reference.md` §4 (US7634399) — patent algorithmic
  detail.
- `AMBE-3000_Protocol_Spec.md` — `PKT_RPT_MODE` packet layer.
- `annex_tables/rate_index_table.csv`, `annex_tables/rate_control_words.csv`
- `annex_tables/rate_conversion_pairs.csv` — TODO; per-rate-pair
  conversion configuration (proposed by §10).

**Language convention:** C reference snippets are illustrative, not
normative Rust. Spec is language-neutral per project conventions.

---

## 1. Scope and Architecture

### 1.1 What This Rate Converter Does

Convert channel bits encoded at AMBE rate **A** (source) into channel bits
encoded at AMBE rate **B** (target), without ever synthesizing PCM
audio. The conversion preserves MBE model parameters as faithfully as
possible across the rate change, applying compensations for differences
in pitch quantization granularity, voicing band structure, and spectral
magnitude scale.

The high-level transform:

```
  channel bits (rate A)            channel bits (rate B)
        │                                ▲
        ▼                                │
  ┌───────────┐    ┌─────────────┐   ┌───────────┐
  │ FEC dec A │ →  │ MBE params  │ → │ FEC enc B │
  └───────────┘    │ + transform │   └───────────┘
                   └─────────────┘
                          ▲
                          │
                  no PCM, ever
```

Per Objectives.md, this is the "parametric rate conversion" path that
DVSI's own testing showed achieves a **12% intelligibility improvement
over tandeming** (decode-to-PCM-then-re-encode) — the keystone capability
for the project's gateway/bridge use case.

### 1.2 Block Diagram

```
   channel bits (rate A, 49–88 info bits per 20 ms)
            │
            ▼
   ┌────────────────────────────┐
   │  §3.A Decoder front half   │    Decoder spec §3 FEC decode
   │  (rate A FEC + recovery)   │    + §4 parameter recovery
   └─────────────┬──────────────┘
                 │ MBE params at rate A: (ω̃₀_A, L̃_A, ṽ_l_A, M̃_l_A)
                 ▼
   ┌────────────────────────────┐
   │  §4.1 Pitch transform      │    re-quant ω̃₀ to rate B grid
   │                            │    compute ratio R = f̂₀_B / f̃₀_A
   └─────────────┬──────────────┘
                 │ ω̂₀_B, R
                 ▼
   ┌────────────────────────────┐
   │  §4.2 Voicing normalization │   resample variable-K to 8 bands
   │  (US7634399 col. 7–8)      │    "favoring voiced state"
   └─────────────┬──────────────┘
                 │ v̂_k_B (8 bands)
                 ▼
   ┌────────────────────────────┐
   │  §4.3 Magnitude resampling │   linear interp on log-mags
   │  (when |R−1| > 0.01)       │   with 0.5·log(R) energy offset
   └─────────────┬──────────────┘
                 │ M̂_l_B
                 ▼
   ┌────────────────────────────┐
   │  §4.4 Unvoiced compensation│   add 0.5·log(256·f₀) to UV bands
   └─────────────┬──────────────┘
                 │ M̂_l_B (compensated)
                 ▼
   ┌────────────────────────────┐
   │  §4.5 Predictor update     │   ρ = 0.65 across rates (NOT same as
   │  (cross-rate, ρ=0.65)      │   intra-rate decoder predictor!)
   └─────────────┬──────────────┘
                 │ MBE params at rate B
                 ▼
   ┌────────────────────────────┐
   │  §3.B Encoder back half    │   Encoder spec §4 quantization
   │  (rate B quant + FEC)      │   + §5 bit prio + §6 FEC + §7 interleave
   └─────────────┬──────────────┘
                 │
                 ▼
        channel bits (rate B)
```

### 1.3 Relationship to Existing Specs

This spec **does not duplicate**:

| Topic | Source of truth |
|-------|-----------------|
| FEC decode (any rate) | `AMBE-3000_Decoder_Implementation_Spec.md` §3 |
| MBE parameter recovery (inverse quant) | Decoder spec §4 |
| MBE parameter quantization (forward quant) | `AMBE-3000_Encoder_Implementation_Spec.md` §4 |
| Bit prioritization (forward) | Encoder spec §5 |
| FEC encode (any rate) | Encoder spec §6 + BABA-A impl spec |
| Interleaving (forward) | Encoder spec §7 + BABA-A annex CSVs |
| PCM analysis | **Not used.** Rate converter never touches PCM. |
| PCM synthesis | **Not used.** Rate converter never touches PCM. |
| Phase regeneration | **Not used.** Phase is only needed for synthesis. |

This spec **adds**:
- Cross-rate MBE parameter transformations (§4.1–§4.5) — the unique
  algorithmic content
- Cross-rate predictor management (§5) — distinct from decoder/encoder
  intra-rate predictor
- Invalid-frame substitution at the rate-converter level (§6) — different
  from decoder's frame-repeat
- Rate-pair dispatch (§10) — which (source, target) combinations are
  supported and the per-pair compensation tuning

### 1.4 What Rate Converter Skips Compared to Tandeming

Tandem path (decode → PCM → encode) executes:
1. Source-rate FEC decode
2. Source-rate parameter recovery
3. Spectral enhancement (decoder §4.3.6)
4. Phase regeneration (decoder §5)
5. Voiced + unvoiced synthesis → PCM (decoder §6, §7)
6. Frame buffering for analysis lookahead (encoder addendum §0.1.3)
7. Re-window, re-FFT, re-pitch-estimate (addendum §0.2–§0.4)
8. Re-V/UV-determine (addendum §0.7)
9. Re-spectral-magnitude-estimate (addendum §0.5)
10. Re-quantize (encoder §4)
11. Target-rate FEC encode

Parametric path (this spec) skips steps 3–9 entirely:
1. Source-rate FEC decode
2. Source-rate parameter recovery
3. **§4 cross-rate parameter transform** (cheap, deterministic)
4. Re-quantize to target rate (encoder §4)
5. Target-rate FEC encode

The skipped steps are exactly the ones that *introduce* quality loss in
tandeming — synthesis blurs pitch/phase, re-analysis re-quantizes pitch
with fresh estimation noise, etc. The rate converter avoids both
sources of error. This is why it outperforms tandem (12% intelligibility
improvement per Objectives.md).

**One side effect:** the rate converter inherits the **source rate's
quantization quality** rather than re-estimating from cleaner audio. If
the source rate is half-rate (3600 bps) and the target is full-rate
(7200 bps), the output cannot be better than the source — full-rate
encoding only helps when the input was full-rate or higher. The converter
is most useful when source and target are similar quality (e.g., both
3600 bps half-rate variants for cross-standard bridging).

### 1.5 Frame Timing

| Quantity | Value |
|----------|-------|
| Source frame rate | 50 Hz (20 ms per frame, source rate dependent) |
| Target frame rate | 50 Hz (20 ms per frame, target rate dependent) |
| Algorithmic delay | ~1 frame (20 ms) for the predictor history |
| End-to-end latency | source FEC + §4 transform + target FEC = sub-millisecond on modern CPU |

Compared to the ~80 ms tandem delay (analysis lookahead alone is 73.75
ms), parametric conversion is **~80× faster end-to-end**. This is the
"near-zero algorithmic delay" benefit Objectives.md describes — though
the wall-clock difference is dominated by the audio-frame boundary
(20 ms), not the per-frame compute.

### 1.6 Multi-Subframe Considerations

When source and target rates have different subframe counts (e.g.,
single-subframe p25_halfrate → two-subframe r17), the converter must time-align
the parameter streams. Two-subframe rates produce 320 samples worth of
parameters per channel frame; single-subframe rates produce 160. Cross-
rate alignment:

| Source → Target | Alignment |
|-----------------|-----------|
| 1 → 1 (e.g., p25_halfrate → p25_halfrate variant) | one-to-one, trivial |
| 1 → 2 (e.g., p25_halfrate → r17) | accumulate two source frames per target frame |
| 2 → 1 (e.g., r17 → p25_halfrate) | emit two target frames per source frame |
| 2 → 2 | one-to-one if subframe boundaries align; else interpolate |

Out of scope for this spec's first cut: the P25-focused validation in §11
covers only 1↔1 conversions (p25_fullrate ↔ p25_halfrate). Multi-subframe rates (US6199037
joint quantization) are flagged as TODO in §10.

---

## 2. Pipeline Reuse from Decoder and Encoder Specs

### 2.1 Decoder Front Half (§3.A in §1.2 diagram)

For source rate A, the rate converter executes decoder spec §3 (FEC
decode for rate A) + decoder spec §4 (parameter recovery for rate A) and
**stops at the MbeParams boundary**. Specifically:

| Decoder spec section | Rate converter usage |
|---------------------|----------------------|
| §3.1 Full-rate FEC decode | Used as-is for source = p25_fullrate |
| §3.2 Half-rate FEC decode | Used as-is for source = p25_halfrate |
| §4.1 Pitch recovery | Used as-is |
| §4.2 V/UV recovery | Used as-is — produces ṽ_l (per-harmonic) |
| §4.3.1–4 PRBA/HOC recovery | Used as-is — produces M̃_l |
| §4.3.5 IDCT + predictor | Used as-is — produces M̃_l with intra-rate ρ |
| §4.3.6 Enhancement | **SKIPPED** — phase regen and synthesis not needed; raw M̃_l feeds §4.3 here |
| §4.3.7 Frame repeat / mute | Adapted — see §6 |
| §4.3.8 Error-rate estimator | Used as-is — feeds §6 invalid-frame logic |
| §5 Phase regeneration | **SKIPPED** — no synthesis |
| §6 Voiced synthesis | **SKIPPED** |
| §7 Unvoiced synthesis | **SKIPPED** |

Output of decoder front half: `(ω̃₀_A, L̃_A, ṽ_l_A for l=1..L̃_A,
M̃_l_A for l=1..L̃_A, error metric, frame_type)`. Note **enhancement is
skipped** — the rate converter operates on the pre-enhancement
magnitudes M̃_l, because enhancement is a perceptual post-filter for
synthesis; transcoding to another rate works better on the raw
quantized values.

### 2.2 Encoder Back Half (§3.B in §1.2 diagram)

For target rate B, the rate converter executes encoder spec §4
(forward quantization for rate B) + §5–§7 (bit prio, FEC, interleave),
**starting from MbeParams**. Specifically:

| Encoder spec section | Rate converter usage |
|---------------------|----------------------|
| §1–§3 Analysis pipeline | **SKIPPED** — MbeParams already in hand |
| §3 Frame-type dispatch | Used — but operates on cross-rate-transformed MbeParams (§4 of this spec) and the source rate's frame_type, with overrides per §6 |
| §4.1 Pitch quantization | Used as-is on transformed ω̂₀_B |
| §4.2 V/UV codebook quantization | Used as-is on transformed v̂_k_B (8-band) |
| §4.3 Spectral magnitude quantization | Used as-is on transformed M̂_l_B |
| §4.3.5 Closed-loop predictor update | Used — but predictor state is **shared** with this spec's §5, not the encoder's intra-rate predictor |
| §5 Bit prioritization | Used as-is |
| §6 FEC encoding | Used as-is for target rate B |
| §7 Interleaving | Used as-is for target rate B |
| §8 Closed-loop feedback | Used — see §5 of this spec |

The encoder analysis pipeline (addendum §0.1–§0.7) is entirely bypassed.
That is the central optimization of parametric rate conversion.

### 2.3 What's Genuinely New in This Spec

Sections §4 (parameter transformation), §5 (cross-rate predictor), §6
(invalid-frame substitution), and §10 (rate-pair dispatch). Everything
else is composition of decoder and encoder specs.

---

## 3. Source and Target Rate Dispatch

### 3.1 Rate-Pair Selection

The DVSI chip's `PKT_RPT_MODE` packet specifies a pair of RCW values:
one for the source rate, one for the target rate. Both rates must be
in r0..r63. Software equivalent:

```c
typedef struct {
    uint8_t source_rate;        /* 0..63 */
    uint8_t target_rate;        /* 0..63 */
    /* Per-pair conversion configuration (§10) — pulled from
     * annex_tables/rate_conversion_pairs.csv if exists, else use
     * the §4 generic algorithm.                                          */
    const ambe_rc_pair_config_t *pair_config;
} ambe_rate_converter_state_t;

int ambe_rate_convert_frame(ambe_rate_converter_state_t *state,
                            const uint8_t *src_frame_bits,  /* rate A */
                            uint8_t *dst_frame_bits);       /* rate B */
```

### 3.2 Supported Pair Categories

| Pair category | Example | Status in this spec |
|---------------|---------|--------------------|
| Same rate (passthrough) | p25_halfrate → p25_halfrate | trivial; identity transform |
| P25 ↔ P25 (within standard) | p25_fullrate ↔ p25_halfrate | §4 fully specified |
| P25 ↔ external AMBE+2 half | p25_halfrate ↔ r-other-3600 | §4 fully specified |
| Full-rate ↔ Half-rate (P25 internal) | p25_fullrate ↔ p25_halfrate | §4 with extra care for L̃_A vs L̃_B mismatch |
| Multi-subframe ↔ single-subframe | r17 ↔ p25_halfrate | TODO §10; out of P25 scope |
| Vocoder family change (AMBE → AMBE+2) | r0 → p25_halfrate | §4 generic + per-rate quirks; expect lower quality |

The **P25 ↔ external** case is the keystone use case from Objectives.md:
DMR uses 3600 bps AMBE+2 (similar to p25_halfrate); D-STAR uses 3600 bps AMBE
(closer to r28); converting between them and P25-half via this spec
preserves intelligibility across gateways.

### 3.3 Rate-Mismatch-Driven Quality

| Source quality | Target quality | Output quality |
|----------------|----------------|----------------|
| Higher (full-rate) | Lower (half-rate) | Limited by target — half-rate quantization noise dominates |
| Lower (half-rate) | Higher (full-rate) | Limited by source — full-rate bits buy nothing |
| Same rate, different family | Same | Limited by family-conversion mismatch — typically minor |
| Same family, same bit budget | Same | Highest quality — converter is near-lossless |

The rate converter is most useful for **same-quality cross-standard
bridging** (e.g., DMR-half ↔ P25-half). Cross-quality conversions
(half ↔ full) work but don't gain anything over re-encoding from PCM
in the easier direction.

---

## 4. MBE Parameter Transformation (Cross-Rate)

This is the unique algorithmic content of this spec. The transformations
are applied in order: pitch → voicing → magnitudes → unvoiced
compensation → predictor.

### 4.1 Fundamental Frequency Transformation

Source: US7634399 col. 5 lines 25–60, col. 9 lines 5–30.

#### 4.1.1 Re-quantize on Target Rate Grid

The source rate's pitch quantizer and target rate's pitch quantizer
have different grids. Re-quantize:

```c
/* Re-quantize ω̃₀_A on target rate's pitch grid; return the new index
 * b̂₀_B and the reconstructed ω̂₀_B (which is what downstream stages use).
 * Both grids are in the rate-table CSV; encoder spec §4.1 has the
 * forward quantizers per rate.                                           */
void rate_xform_pitch(double omega_0_A,
                      uint8_t target_rate,
                      uint8_t *b0_B_out,
                      double  *omega_0_B_out)
{
    /* Forward quantize per target rate */
    if (target_rate == 33)        /* P25 full-rate */
        *b0_B_out = imbe_pitch_quantize(2.0 * M_PI / omega_0_A);
    else if (target_rate == 34)   /* P25 half-rate */
        *b0_B_out = ambe_pitch_quantize(omega_0_A);
    /* else: rate-specific quantizer per §10 dispatch table */

    /* Inverse quantize to get the actual reconstructed ω̂₀_B */
    *omega_0_B_out = ambe_pitch_dequantize(target_rate, *b0_B_out);
}
```

#### 4.1.2 Pitch Quantization Ratio

```
R = f̂₀_B / f̃₀_A
  = ω̂₀_B / ω̃₀_A
```

R measures how much the pitch shifted under target-rate re-quantization.
Per US7634399 col. 5 line 50: when `|R − 1| > 0.01` (1% threshold), the
spectral magnitudes (§4.3) need linear interpolation/resampling to
account for the harmonic-grid shift. When R is within 1% of unity, the
harmonic indices align closely enough that direct copy is acceptable.

### 4.2 Voicing Band Normalization

Source: US7634399 col. 6 lines 30–60, "favoring the voiced state."

The decoder front half produces `ṽ_l_A` (per-harmonic, length L̃_A).
The encoder back half wants `v̂_k_B` matched to the target rate's voicing
representation. For half-rate target rates, that's the 8-band logical
voicing vector (§4.2.2 of decoder spec; §4.2.1 of encoder spec).

#### 4.2.1 Resample Per-Harmonic to 8 Logical Bands

For each of 8 logical bands j = 0..7, find the harmonics that fall in
band j (per the decoder's `j_l = ⌊l · 16 · ω̃₀ / (2π)⌋` mapping, decoder
§4.2.2) and apply a "favor voiced" rule:

```c
/* US7634399: "If any harmonic in band j is voiced, declare band j voiced"  */
void rate_xform_voicing_to_8band(const uint8_t v_harm[/* L_A */],
                                  uint8_t L_A,
                                  double omega_0_A,
                                  uint8_t v_8band[8])
{
    for (int j = 0; j < 8; j++) v_8band[j] = 0;
    for (uint8_t l = 1; l <= L_A; l++) {
        int j = (int)floor((double)l * 16.0 * omega_0_A / (2.0 * M_PI));
        if (j < 0) j = 0;
        if (j > 7) j = 7;
        if (v_harm[l - 1]) v_8band[j] = 1;     /* favor voiced */
    }
}
```

The "favor voiced" rule is asymmetric on purpose: voicing → unvoiced
mis-classification produces audible buzz, while unvoiced → voiced
mis-classification produces (subtler) phasiness. In the cross-rate
context where information is being compressed, biasing toward voiced
preserves the more perceptually salient case.

The encoder's `ambe_vuv_quantize()` (encoder spec §4.2.2) then performs
the codebook search against this 8-band target.

#### 4.2.2 Same-Family Identity Path

When source and target are both half-rate (e.g., p25_halfrate → p25_halfrate), the source's
b̂₁ codebook index can in principle be passed through unchanged. The
spec recommends always re-running the §4.2.1 → §4.2.2 path anyway:
- The codebook search re-runs cheaply (32 entries × 8 distances)
- It naturally handles tie-breaking and voiced-favor consistently
- Identity transforms emerge automatically when the input is bit-exact

Identity-detect optimizations are out of scope for the first
implementation.

### 4.3 Spectral Magnitude Resampling

Source: US7634399 col. 5 lines 50–65, col. 6 lines 1–25.

#### 4.3.1 No Resampling (R Within 1%)

```
if |R − 1| ≤ 0.01:
    M̂_l_B = M̃_l_A      for l = 1..min(L̃_A, L̂_B)
    /* no offset, no interpolation */
```

The harmonic grids align closely enough; just copy. If `L̂_B > L̃_A`,
zero-pad the upper harmonics. If `L̂_B < L̃_A`, drop the upper harmonics.

#### 4.3.2 Linear Interpolation (R Outside 1%)

```
Convert M̃_l_A to log-domain:  log₂(M̃_l_A)
Linear-interpolate from source grid to target grid:
   target harmonic l_B at position l_B · ω̂₀_B
   maps to fractional source position:  l_B · ω̂₀_B / ω̃₀_A
   = l_B / R
Interpolate log₂(M̃) at fractional position l_B / R using neighboring
   integer harmonics from the source.
Convert back to linear:  M̂_l_B = 2^{interp(log₂(M̃_l_A), l_B / R)}
```

C reference:
```c
/* Linear interpolation in log-magnitude domain.
 * Source grid is M̃_l_A for l = 1..L_A at fundamental ω̃₀_A.
 * Target grid is l = 1..L_B at fundamental ω̂₀_B.
 * R = ω̂₀_B / ω̃₀_A. Pre-compute log₂ on input, exp₂ on output.       */
void rate_xform_magnitudes(const double M_A[/* L_A */],
                           uint8_t L_A,
                           double omega_0_A,
                           double omega_0_B,
                           uint8_t L_B,
                           double M_B_out[/* L_B */])
{
    double R = omega_0_B / omega_0_A;
    if (fabs(R - 1.0) <= 0.01) {
        /* §4.3.1 no-resampling fast path */
        for (uint8_t l = 1; l <= L_B; l++)
            M_B_out[l - 1] = (l <= L_A) ? M_A[l - 1] : 0.0;
        return;
    }

    double logM[64];   /* L_A ≤ 56, room to spare */
    for (uint8_t l = 1; l <= L_A; l++)
        logM[l - 1] = log2(fmax(M_A[l - 1], 1e-10));

    double energy_offset = 0.5 * log2(R);   /* §4.3.3 */

    for (uint8_t l_B = 1; l_B <= L_B; l_B++) {
        double src_pos = (double)l_B / R;     /* fractional source index */
        if (src_pos < 1.0 || src_pos > L_A) {
            M_B_out[l_B - 1] = 0.0;            /* outside source grid */
            continue;
        }
        int    pos_lo   = (int)floor(src_pos);
        int    pos_hi   = pos_lo + 1;
        double frac     = src_pos - (double)pos_lo;
        double v_lo     = logM[pos_lo - 1];
        double v_hi     = (pos_hi <= L_A) ? logM[pos_hi - 1] : v_lo;
        double interp   = v_lo + frac * (v_hi - v_lo);
        M_B_out[l_B - 1] = pow(2.0, interp + energy_offset);
    }
}
```

#### 4.3.3 Energy-Preserving Offset

Per US7634399 col. 5 line 60: when resampling, an offset of
`0.5 · log(R)` (in log₂ domain) is applied to all magnitudes. This
preserves total spectral energy across the harmonic-grid change:

```
Σ |M̂_l_B|² ≈ Σ |M̃_l_A|²
```

Without the offset, R > 1 (target has fewer harmonics) would lose
energy and R < 1 (target has more harmonics) would inflate energy.
The 0.5·log₂(R) factor in log-domain corresponds to a √R factor in
linear-magnitude domain, which is correct for energy preservation
(power = |M|² / R harmonic count).

### 4.4 Unvoiced Band Compensation

Source: US7634399 col. 6 lines 15–25.

```
For each harmonic l in an unvoiced band of target rate B:
    log₂(M̂_l_B) += 0.5 · log₂(256 · f̂₀_B)
```

where `f̂₀_B = ω̂₀_B / (2π)` is the target fundamental in cycles/sample.

Reason (per patent): unvoiced bands' magnitudes encode total noise energy
per band rather than per-harmonic amplitude. As the harmonic count
changes (different L̂_B than L̃_A), the per-harmonic share of the band's
unvoiced energy shifts. The `256 · f̂₀_B` factor accounts for the band
width in DFT bins (256-bin FFT, band width ω₀ in radians = 256·f₀ in
bins), and the 0.5·log corresponds to amplitude scaling for that bin
width.

```c
void rate_xform_unvoiced_compensation(double M_B[/* L_B */],
                                      const uint8_t v_8band_B[8],
                                      double omega_0_B,
                                      uint8_t L_B)
{
    double f0_B = omega_0_B / (2.0 * M_PI);
    double offset_lin = sqrt(256.0 * f0_B);   /* 0.5·log₂ → √ in linear */
    for (uint8_t l = 1; l <= L_B; l++) {
        int j = (int)floor((double)l * 16.0 * omega_0_B / (2.0 * M_PI));
        if (j < 0) j = 0;
        if (j > 7) j = 7;
        if (!v_8band_B[j]) {
            /* Unvoiced harmonic */
            M_B[l - 1] *= offset_lin;
        }
    }
}
```

Apply **after** §4.3 magnitude resampling — the offset operates on the
target-rate magnitudes, indexed by the target's voicing decisions.

### 4.5 Cross-Rate Predictor (ρ = 0.65)

Source: US7634399 col. 7 lines 5–25.

The rate converter maintains its **own** prior-frame magnitude predictor
state, distinct from the source decoder's intra-rate predictor and the
target encoder's intra-rate predictor:

```
M̂_l_B(0) = (current frame's transformed magnitudes from §4.3, §4.4)
M̃_l_B(−1) = (previous frame's reconstructed magnitudes after target rate
              quantization, used only by this spec)

If |R − 1| ≤ 0.01:
    final M̂_l_B = M̂_l_B(0)            /* fast path: no prior-frame mixing */
else:
    /* Resample prior frame magnitudes onto current target grid via the
     * same §4.3 algorithm with R_prev = ω̂₀_B(0) / ω̂₀_B(−1).        */
    M̃_l_B_resampled(−1) = rate_xform_magnitudes(M̃_l_B(−1), ...)
    final M̂_l_B = 0.65 · M̃_l_B_resampled(−1)  +  0.35 · M̂_l_B(0)
```

The `ρ = 0.65` rate-converter predictor coefficient is the same literal
value that BABA-A Eq. 185 (half-rate decoder, page 65) uses for the
half-rate intra-rate predictor —
the important separation is **predictor-state**, not coefficient value:

- The rate-converter's `M̃_l_B(−1)` is a **distinct array** from the
  target encoder's `M̃_l_B(−1)` (even though both apply the same 0.65
  coefficient when the target is half-rate).
- The full-rate decoder's intra-rate predictor uses the Eq. 55
  L̂(0)-dependent schedule (range 0.4–0.7) — here the coefficient
  **does** differ from the rate-converter's literal 0.65.
- The rate-converter's predictor additionally **resamples prior-frame
  magnitudes** onto the current target grid before applying the
  coefficient (§4.3 linear interp). The intra-rate predictors apply the
  coefficient directly to same-L̂-gridded prior magnitudes with no
  resampling step. Different update rules even when the coefficient
  matches.

The rate converter therefore tracks **three** prior-frame magnitude
states in its full state structure (§7): source decoder's, rate-
converter's own, and target encoder's.

Coincidence-of-value explanation: both predictors are tuned for the
same task (smoothing frame-to-frame variability in log-magnitudes) on
the same 20 ms / 160-sample grid, so a similar coefficient falling out
is expected. 0.65 is described as empirically tuned in both contexts.

---

## 5. Cross-Rate Predictor State

The rate converter's predictor state is separate from both the source
decoder's predictor and the target encoder's predictor. It tracks:

| State | Init | Updated by | Consumed by |
|-------|------|-----------|-------------|
| `ω̂₀_B(−1)` | Annex L row 30 | §4.1 each frame | §4.5 prior-frame resample |
| `L̂_B(−1)` | 30 | §4.1 each frame | §4.5 |
| `M̃_l_B(−1)` for l=1..56 | 1.0 | §4.5 closed-loop after target encode | §4.5 next frame |
| `v̂_k_B(−1)` for k=1..8 | 0 (all UV) | §4.2 each frame | §4.5 (if voicing-aware predictor variant) |

The decoder front half (decoder spec §4.4 state table) and encoder back
half (encoder spec §4.4 state table) maintain their own predictor states
unchanged. Three predictor pipelines run in parallel:

```
       Source rate A bits ──→ [decoder front] ──→ MbeParams
                                    │
                                    ▼ (decoder's M̃_l_A predictor state,
                                       coefficient per source rate)
       MbeParams ──→ [§4 transform] ──→ MbeParams (target rate B)
                          │
                          ▼ (rate converter's M̃_l_B predictor state,
                             ρ = 0.65 + grid-resample update rule)
       MbeParams ──→ [encoder back] ──→ Target rate B bits
                          │
                          ▼ (encoder's M̃_l_B predictor state,
                             coefficient per target rate,
                             same-grid update rule)
```

The encoder's intra-rate predictor and the rate converter's predictor
both operate on target-rate magnitudes. For half-rate targets they even
share the same literal coefficient (0.65). They remain separate because:
- The rate converter resamples prior-frame magnitudes onto the current
  target grid before applying the coefficient (§4.3); the intra-rate
  predictor doesn't
- State is independent: the rate converter's `M̃_l_B(−1)` evolves across
  the parameter transformation, while the encoder's tracks its own
  closed-loop matched-decoder reconstruction
Sharing state across the three predictors produces AR-loop instability
per US7634399 col. 7 warning text.

---

## 6. Invalid-Frame Handling

Source: US7634399 col. 8 lines 10–30, "Invalid frame substitution."

### 6.1 Detection (Source-Rate FEC Errors)

The decoder front half (decoder spec §3.2.3) produces an error metric.
The rate converter declares a frame **invalid** if:
- Source FEC declares the frame invalid (decoder spec §3.2.3 conditions)
- Source frame is dispatched as silence or erasure by source rate's
  frame-type table (decoder spec §9.4)

### 6.2 Substitution (Not Synthesis)

Per US7634399: **do not** synthesize a frame-repeat by re-encoding the
prior MbeParams. Instead, **substitute known frame-repeat bits** at the
target rate's wire format:

```c
/* Each target rate has a canonical "frame-repeat" bit pattern. The
 * downstream decoder receives these bits, recognizes them as frame-
 * repeat (per its §9.1 logic), and copies forward its own state.       */
extern const uint8_t target_rate_frame_repeat_bits[64][18];
                /* indexed by rate, up to 18 bytes (144 bits)           */

void rate_xform_substitute_invalid(uint8_t target_rate, uint8_t *out_bits) {
    /* For half-rate targets: 9 bytes; full-rate: 18 bytes */
    int n_bytes = ambe_rate_table[target_rate].frame_bits / 8;
    memcpy(out_bits, target_rate_frame_repeat_bits[target_rate], n_bytes);
}
```

For P25 half-rate (p25_halfrate), the frame-repeat bits set `b̂₀ ∈ [120, 123]`
(the erasure sentinel range, decoder spec §9.4). The receiving decoder
sees this and triggers its own frame-repeat (which preserves *that
decoder's* prior state — not the converter's).

**Why substitution and not re-encode-from-prior-MbeParams.** Re-encoding
prior parameters would produce valid-looking frames at the target rate,
which the receiver would integrate into its predictor state. If the
source kept dropping frames, the receiver would drift further and
further from reality with each "fake" valid frame. Substituting erasure
bits triggers the receiver's own frame-repeat — which is bounded
(after 3–4 consecutive repeats it mutes per decoder spec §3.2.4) and
keeps both ends in sync.

### 6.3 Invalid-Frame State Propagation in This Spec

When a source frame is invalid, the rate converter:
1. Substitutes erasure bits per §6.2
2. Does **not** update its §4 transformation state (no fresh MbeParams
   to update from)
3. **Does** advance frame counters and any timer-driven state (e.g., for
   tracking long mute periods)

If the source emits valid bits again later, the converter's predictor
state is stale by however many frames were invalid. This is handled by
the ρ=0.65 coefficient — after a few valid frames, the predictor
re-converges. Empirically: 3–5 frames of valid input restore predictor
quality after a 10-frame invalid run.

---

## 7. Cross-Frame State Summary

Combined state for the full rate-converter pipeline:

| State block | Origin | Section |
|-------------|--------|---------|
| Source decoder front-half state | Decoder spec §4.4 | §2.1 |
| Cross-rate predictor (this spec) | This spec §5 | §5 |
| Target encoder back-half state | Encoder spec §4.4 | §2.2 |
| Rate-pair config | RCW + this spec §3 | §3.1 |
| Frame counters (invalid run, mute) | This spec §6 | §6.3 |

The three predictor states are independent. Total memory footprint:
~3 × 56 doubles for magnitudes + small scalars ≈ 1.5 KB per converter
instance. Negligible.

---

## 8. Same-Rate Passthrough (Identity Conversion)

When source rate = target rate, the rate converter degenerates to a
"validate-and-repack" operation. Useful for:
- Frame validity filtering (drop invalid frames at a gateway before
  retransmit)
- Testing the converter pipeline on known-good identity inputs
- Multi-hop chains where intermediate hops are identity

Implementation: run §3.A decoder, skip §4 entirely (it's identity for
same rates within 1% R), run §3.B encoder. The output bits should be
**identical to the input bits** for valid input (modulo soft-decision
re-quantization noise, if soft input was used).

If output != input on identity passthrough, there is a bug in either
§3.A or §3.B. This is the cheapest end-to-end smoke test for the
converter.

---

## 9. Alternative: Bit-Layer Substitution (Out of Scope)

A non-parametric alternative would be to operate on the **encoded bit
streams** directly: `b̂_k_A → table_lookup → b̂_k_B` without ever
reconstructing MbeParams. Conceivable for some same-family rate pairs.

This is **not** what the chip does and not what this spec covers. Per
US7634399 col. 4 lines 30–40, the patent explicitly distinguishes
parametric (MBE-domain) conversion from bit-level transcoding and
recommends the parametric approach for better cross-quantizer matching.

Bit-level transcoding can be added later as a specialized fast path for
identity or near-identity rate pairs, but it is out of scope here.

---

## 10. Rate-Pair Dispatch

### 10.1 Configuration File

**Proposed:** `annex_tables/rate_conversion_pairs.csv` — per
(source_rate, target_rate) pair:

```
source_rate, target_rate, supported, voicing_xform, magnitude_xform,
unvoiced_compensation, predictor_rho, special_notes
```

Most pairs use the §4 generic algorithm with default parameters
(`predictor_rho = 0.65`, all transforms enabled). A few pairs need
overrides:
- Identity pairs (source = target): all transforms become no-ops; just
  validate and repack
- Same-family pairs within 1% pitch quantizer: skip §4.3 resampling
- Multi-subframe pairs: §1.6 special handling

The CSV is currently a TODO — populate as rate-pair-specific testing
proceeds. Until the CSV exists, the converter uses §4 defaults for all
pairs (which works for the P25-internal p25_fullrate ↔ p25_halfrate pair this spec
primarily targets).

### 10.2 P25 Rate Pairs (Primary Targets)

| Pair | Use case | §4 path |
|------|----------|---------|
| p25_fullrate → p25_halfrate | P25 full-rate ingress, half-rate egress | full §4.1–§4.5; expect quality limited by half-rate target |
| p25_halfrate → p25_fullrate | P25 half-rate ingress, full-rate egress | full §4.1–§4.5; quality limited by half-rate source |
| p25_halfrate → p25_halfrate | P25 half-rate identity (validation) | §8 passthrough |
| p25_fullrate → p25_fullrate | P25 full-rate identity (validation) | §8 passthrough |

### 10.3 Cross-Standard Pairs (Future Targets)

| Pair | Bridges | Notes |
|------|---------|-------|
| p25_halfrate ↔ DMR-half (r-other) | P25 ↔ DMR | both 3600 bps AMBE+2 — near-identity |
| p25_halfrate ↔ D-STAR-half (r-other) | P25 ↔ D-STAR | both 3600 bps but D-STAR is plain AMBE, not AMBE+2 — vocoder-family change adds quality loss |
| Multi-subframe AMBE rates | various | TODO; needs §1.6 |

The exact rate indices for DMR and D-STAR variants live in the DVSI
manual rate table (`annex_tables/rate_index_table.csv`). Cross-standard
validation is out of scope for this spec's first cut.

### 10.4 Multi-Subframe Rate Pairs (US6199037)

§1.6 above defined the time-alignment matrix between single-subframe
and two-subframe rates. This section gives the implementation.

**Which rates are two-subframe is an open question** — see §12 and
`analysis/ambe3000_multi_subframe_rate_mapping.md`. Dispatch on
`annex_tables/multi_subframe_rates.csv`, which is `pending` for 60
of 64 rate indices.

The four cases from §1.6 map to these §4 paths:

| Src → Tgt | §4 path |
|-----------|---------|
| 1 → 1     | §4 as specified — current P25 scope (p25_fullrate ↔ p25_halfrate).            |
| 1 → 2     | Accumulate two source frames → joint-encode per decoder §10.5 via target-encoder §10.2. |
| 2 → 1     | Split source frame into two MbeParams (decoder §10.5) → emit two target frames via encoder (single-subframe §4). |
| 2 → 2     | Two paths: (a) split-and-rejoin per-subframe §4; (b) direct joint-to-joint bit repack if codebooks match. |

#### 10.4.1 State

The rate converter's per-pair state grows for cross-subframe-count
pairs:

```c
typedef struct {
    /* ... existing single-subframe state ... */

    /* For 1 → 2 conversions: buffer one source frame awaiting its
     * pair to joint-quantize. */
    mbe_params_t src_subframe_buffer;
    bool         src_subframe_buffer_valid;

    /* For 2 → 1 conversions: buffer the second subframe's params
     * awaiting emit on the next tick. */
    mbe_params_t tgt_subframe_buffer;
    bool         tgt_subframe_buffer_valid;

    /* Predictor state advances per-subframe (decoder spec §10.5.5,
     * encoder spec §10.2.5) — not per-frame — for multi-subframe ends. */
    double prev_subframe_log_mag[MAX_L];
} rate_converter_state_t;
```

#### 10.4.2 1 → 2 Conversion

Delay: +1 source frame.

```c
int convert_1_to_2(const uint8_t *src_frame_bits,
                   uint8_t *tgt_frame_bits_or_null,
                   rate_converter_state_t *state) {
    /* Decode source frame → MbeParams. */
    mbe_params_t src_mbe;
    decode_source_single_subframe(src_frame_bits, &src_mbe);

    if (!state->src_subframe_buffer_valid) {
        /* Hold this frame; pair with next. */
        state->src_subframe_buffer = src_mbe;
        state->src_subframe_buffer_valid = true;
        return 0;   /* no target frame emitted this tick */
    }

    /* We now have two consecutive single-subframe sources.
     * Joint-quantize via encoder spec §10.2.2–§10.2.4. */
    mbe_params_t mbe_pair[2] = { state->src_subframe_buffer, src_mbe };
    encode_multi_subframe(mbe_pair, tgt_frame_bits_or_null, state);

    state->src_subframe_buffer_valid = false;
    return 1;   /* target frame emitted */
}
```

#### 10.4.3 2 → 1 Conversion

No extra delay; emits two target frames per source tick.

```c
int convert_2_to_1(const uint8_t *src_frame_bits,
                   uint8_t *tgt_frame_bits_sf0,
                   uint8_t *tgt_frame_bits_sf1,
                   rate_converter_state_t *state) {
    /* Decode source frame → two MbeParams via decoder spec §10.5. */
    mbe_params_t src_mbe[2];
    decode_source_multi_subframe(src_frame_bits, src_mbe);

    /* Emit each subframe's params as a single-subframe target frame. */
    encode_single_subframe(&src_mbe[0], tgt_frame_bits_sf0, state);
    encode_single_subframe(&src_mbe[1], tgt_frame_bits_sf1, state);

    return 2;
}
```

#### 10.4.4 2 → 2 Conversion

Two strategies. Per-pair selection depends on whether source and
target multi-subframe codebooks match.

**Strategy A (split-and-rejoin):** always works, higher compute.

```c
int convert_2_to_2_via_split(const uint8_t *src_frame_bits,
                             uint8_t *tgt_frame_bits,
                             rate_converter_state_t *state) {
    mbe_params_t src_mbe[2], tgt_mbe[2];
    decode_source_multi_subframe(src_frame_bits, src_mbe);

    /* Apply §4 transforms per subframe. */
    for (int sf = 0; sf < 2; sf++)
        apply_section_4_transforms(&src_mbe[sf], &tgt_mbe[sf], state);

    encode_multi_subframe(tgt_mbe, tgt_frame_bits, state);
    return 1;
}
```

**Strategy B (direct joint-to-joint bit repack):** only when source and
target share identical joint-quantization layout (same Method, same
codebooks). Then `b̂_k[2]_A → b̂_k[2]_B` is a FEC-domain repack at the
channel-bits level; no MbeParams reconstruction needed. Applicable
when the `multi_subframe_rates.csv` + `halfrate_bit_allocations.csv`
combination indicates identical per-subframe bit allocations and
joint-quantization methods.

#### 10.4.5 Codebook Assumption

All four conversion paths depend on the decoder §10.5 and encoder
§10.2 joint-quantization codebooks being populated for the source
and target rates. Until `multi_subframe_rates.csv` is resolved (and
per-rate codebook contents characterized in `~/blip25-mbe`), §10.4
conversions involving unresolved rates inherit the `pending` status
from `rate_conversion_pairs.csv`.

---

## 11. Validation Plan

### 11.1 Test Vector Pipeline

```
  .bit file (rate A)         ┌─────────────┐
  from tv-rc/rA/             │  software   │  →  software .bit (rate B)
                          →  │  rate       │
                             │  converter  │
                             └─────────────┘
                                            ┌─── compare ────→  metrics
  .bit file (rate A)         ┌─────────────┐│
  same                    →  │  AMBE-3000  │ → reference .bit (rate B,
                             │  PKT_RPT    │   from tv-rc/rA/rB/)
                             └─────────────┘
```

Test vectors: `/mnt/share/P25-IQ-Samples/DVSI Vectors/tv-rc/`
(97 input files + nested per-source-rate output directories; rate-
conversion outputs live at `tv-rc/<sourceRate>/<targetRate>/<file>.bit`).

Cross-rate coverage is sparse: each source rate `rA/` contains conversion
outputs only for `{r0, r33, r34, r62, r63, dstar, p25, p25_nofec}` —
not all 64 targets. See `AMBE-3000_Test_Vector_Reference.md` §3.2.

Test recipe: `cmprc.txt` (same directory).

### 11.2 Expected Match Quality by Stage

| Stage | Expected vs chip | Notes |
|-------|------------------|-------|
| §3.A decoder front | bit-exact | inherited from decoder spec §11 |
| §4.1 pitch transform | bit-exact | deterministic re-quant |
| §4.2 voicing 8-band normalize | bit-exact | deterministic |
| §4.3 magnitude resample (no-interp path) | bit-exact | direct copy |
| §4.3 magnitude resample (linear interp) | within ~0.1 dB per harmonic | floating-point interp; expect small numerical drift |
| §4.4 unvoiced compensation | bit-exact given matching §4.3 output | scalar multiply |
| §4.5 ρ=0.65 predictor | bit-exact given matching §4.3 + state | linear combination |
| §3.B encoder back | bit-exact given matching MbeParams | inherited from encoder spec |
| **Output frame bits** | bit-exact for identity pairs; near-bit-exact for non-identity | dominated by §4.3 interp drift on non-identity pairs |

### 11.3 Recommended Test Order

1. **Identity passthrough** (§8): p25_halfrate → p25_halfrate bit-exact verification.
   Bit-exact is achievable; if not, bug is in §3.A or §3.B (composition
   issue, not transform issue).
2. **p25_fullrate → p25_halfrate** (full to half): exercise §4.3 magnitude resampling
   (R typically near unity but not exact) and §4.4 unvoiced compensation
3. **p25_halfrate → p25_fullrate** (half to full): exercise grid expansion direction,
   §4.3 zero-padding for L̂_B > L̃_A
4. **p25_fullrate → p25_fullrate** identity (validation of full-rate path)
5. **`cmprc.txt` recipe scenarios** — DVSI's official test plan
6. **Cross-standard pairs** (P25 ↔ D-STAR via shared MBE, e.g.
   `tv-rc/p25/dstar/*`, `tv-rc/dstar/p25/*`) — future, not in first cut.
   DVSI vectors do not include DMR reference outputs.

### 11.4 Debug Order on Mismatch

1. **Diff source MbeParams.** Run §3.A decoder front and dump
   `(ω̃₀_A, L̃_A, ṽ_l_A, M̃_l_A)`. Compare with what the decoder spec
   §11 validation produced for the same source bits. Discrepancy →
   bug in decoder spec, not this spec.
2. **Diff transformed MbeParams.** Dump `(ω̂₀_B, L̂_B, v̂_k_B, M̂_l_B)`
   after §4. Most rate-converter bugs are here. Common: pitch quantizer
   off-by-one, voicing 8-band mapping index error, R threshold off.
3. **Diff target bits.** If transformed MbeParams match a reference but
   target bits don't, the bug is in §3.B encoder (composition with
   encoder spec).
4. **End-to-end roundtrip.** rA → rA → rA via converter should be
   bit-stable after one round; unstable round-trip means the converter's
   predictor (§5) or the encoder's predictor (§4.3.5) is misconfigured.

### 11.5 Quality Metrics

Bit-exact match is achievable for identity pairs and near-achievable for
P25-internal pairs. For cross-quality and cross-family pairs, switch to
PCM-domain metrics:

- **Decode both source and converter output to PCM via the decoder spec,
  then measure PCM-domain SNR / MCD between the two.**
  - Identity pair: SNR ≥ 30 dB (essentially lossless)
  - P25-internal (p25_fullrate ↔ p25_halfrate): SNR ≥ 15 dB
  - Cross-standard (P25 ↔ DMR): SNR ≥ 12 dB (per DVSI's 12% intelligibility
    improvement claim — this is the target the project Objectives.md is
    measured against)
  - Tandeming baseline: typically 5–8 dB SNR loss compared to source —
    the converter should clearly beat this

### 11.6 Tandeming Comparison (Quality Baseline)

The whole point of this spec is to outperform tandeming. Validation
should include side-by-side:

```
  (1) Tandem path:
      rA bits → decoder spec → PCM → encoder spec → rB bits → decoder spec → PCM
                                                                            ↓
                                                                  Tandem PCM output
  (2) Parametric path (this spec):
      rA bits → rate converter → rB bits → decoder spec → PCM
                                                          ↓
                                                Parametric PCM output
```

Compare both to the original PCM (if available) or to a "ground-truth"
tandem of just the source rate's encode→decode. Per Objectives.md, the
parametric path should achieve ~12% better intelligibility on standard
ABC-MRT tests. Numerically: target SNR(parametric vs original) >
SNR(tandem vs original) + 3 dB on representative speech samples.

---

## 12. Open Questions / Analysis TODOs

1. **Per-rate-pair compensation tuning** (§10.1). The default §4 algorithm
   covers the common case; some pairs may need special-cased parameters
   (different ρ, additional offset, etc.). Populate
   `annex_tables/rate_conversion_pairs.csv` as testing surfaces these.
2. **Multi-subframe pair handling** (§1.6, §10.4). Algorithm now
   drafted in §10.4; the remaining gap is rate-index mapping. See
   `analysis/ambe3000_multi_subframe_rate_mapping.md` and
   `annex_tables/multi_subframe_rates.csv` (60 of 64 rows `pending`).
   Sub-items:
   (a) subframe count per rate index,
   (b) per-rate joint-quant codebook contents (shared between decoder
       §10.5, encoder §10.2, and this §10.4),
   (c) whether source/target codebooks match for §10.4.4 Strategy B
       (direct joint-to-joint repack) on 2→2 pairs.
   Prior wording cited r17/r18 specifically; those chip indices are
   labeled AMBE-2000 in the manual rate table and their multi-subframe
   status is unverified.
3. **Cross-vocoder-family conversion** (e.g., AMBE → AMBE+2). When source
   and target use different vocoder families, the magnitude scale, voicing
   model, and gain quantizer differ in ways §4 doesn't fully address.
   Patent US7634399 col. 9 lines 5–30 hints at family-conversion
   compensations but doesn't tabulate them.
4. **DVSI rate-pair table** (`annex_tables/rate_conversion_pairs.csv`).
   Empirically derived per-pair config table. TODO; build incrementally
   as test vectors land.
5. **Bit-level fast path** (§9). For identity and near-identity pairs,
   a bit-only transcoder would be much faster than the parametric path.
   Out of scope for first cut; could be added once parametric path is
   validated.
6. **Inherited from decoder spec**: γ_w calibration (not relevant here —
   no synthesis), phase regen kernel (not relevant — no synthesis).
   (The half-rate intra-rate ρ question from earlier drafts is resolved
   per `analysis/vocoder_decode_disambiguations.md` §3 — literal 0.65,
   same numeric value as the rate-converter predictor but a distinct
   state instance; see §4.5.)

---

## 13. Appendices

### Appendix A — C Reference Fragments (Consolidated)

```c
/* =========================================================================
 *  AMBE-3000 rate converter reference — non-normative C skeleton.
 *  Composes decoder spec front half, this spec's §4 transformations,
 *  and encoder spec back half.
 * ========================================================================= */

#include <math.h>
#include <stdint.h>

/* --- §4.1 pitch transform -------------------------------------------- */
void rate_xform_pitch(double omega_0_A, uint8_t target_rate,
                      uint8_t *b0_B_out, double *omega_0_B_out);

/* --- §4.2 voicing 8-band normalization ------------------------------- */
void rate_xform_voicing_to_8band(const uint8_t v_harm[/* L_A */],
                                  uint8_t L_A,
                                  double omega_0_A,
                                  uint8_t v_8band[8]);

/* --- §4.3 magnitude resampling --------------------------------------- */
void rate_xform_magnitudes(const double M_A[/* L_A */],
                           uint8_t L_A,
                           double omega_0_A,
                           double omega_0_B,
                           uint8_t L_B,
                           double M_B_out[/* L_B */]);

/* --- §4.4 unvoiced band compensation --------------------------------- */
void rate_xform_unvoiced_compensation(double M_B[/* L_B */],
                                      const uint8_t v_8band_B[8],
                                      double omega_0_B,
                                      uint8_t L_B);

/* --- §4.5 cross-rate predictor (ρ = 0.65) ---------------------------- */
void rate_xform_predictor_apply(double M_B_curr[/* L_B */],
                                const double M_B_prev[/* L_B_prev */],
                                uint8_t L_B_curr,
                                uint8_t L_B_prev,
                                double R,
                                double M_B_out[/* L_B_curr */]);

/* --- §6 invalid-frame substitution ----------------------------------- */
void rate_xform_substitute_invalid(uint8_t target_rate, uint8_t *out_bits);

/* --- top-level orchestration ----------------------------------------- */
typedef struct {
    /* source decoder state (decoder spec §4.4)                          */
    ambe_decoder_state_t src_dec_state;
    /* cross-rate predictor state (this spec §5, §7)                     */
    double   M_tilde_B_prev[56];
    double   omega_0_B_prev;
    uint8_t  L_B_prev;
    /* target encoder state (encoder spec §4.4)                          */
    ambe_encoder_state_t dst_enc_state;
    /* config                                                            */
    uint8_t  source_rate;
    uint8_t  target_rate;
} ambe_rate_converter_state_t;

int ambe_rate_convert_frame(ambe_rate_converter_state_t *state,
                            const uint8_t *src_frame_bits,
                            uint8_t *dst_frame_bits);
```

### Appendix B — Test Vector Coverage Matrix

Rate-conversion outputs are stored nested: `tv-rc/<sourceRate>/<targetRate>/<file>.bit`.
(r33 = AMBE+2 half-rate w/FEC = on-air p25_halfrate; r34 = AMBE+2
half-rate no-FEC.) Coverage matrix:

| Test vector | Source → Target | Exercises | Coverage priority |
|-------------|-----------------|-----------|-------------------|
| `tv-rc/r33/*.bit` (identity encode-decode) | p25_halfrate | §8 passthrough | 1 |
| `tv-rc/r34/*.bit` (identity encode-decode) | p25_halfrate no-FEC | §8 passthrough | 2 |
| `tv-rc/r33/r34/*.bit` (cmp against `tv-rc/r33/dam.bit` source) | r33 → r34 (strip FEC) | §4.1, §4.3, §4.4 | 3 |
| `tv-rc/r34/r33/*.bit` | r34 → r33 (add FEC) | §4.1 grid, §4.3 | 4 |
| `tv-rc/p25/r33/*.bit`, `tv-rc/p25_nofec/r33/*.bit` | p25_fullrate → p25_halfrate | §4.1, §4.3 (IMBE → AMBE+2) | 5 |
| `tv-rc/r33/p25/*.bit`, `tv-rc/r33/p25_nofec/*.bit` | p25_halfrate → p25_fullrate | §4.1 grid expand, §4.3 zero-pad | 6 |
| `cmprc.txt` recipe | DVSI's official | full coverage (all rA/rB pairs present on disk) | 7 |

### Appendix C — Cross-Reference Quick Index

| Topic | This spec | Decoder spec | Encoder spec | Patent |
|-------|-----------|--------------|--------------|--------|
| Source FEC decode | §2.1 | §3 | — | — |
| Source MbeParams recovery | §2.1 | §4 | — | — |
| Pitch transform (re-quant on target grid) | §4.1 | §4.1 (recovery only) | §4.1 (forward only) | US7634399 col. 5 |
| Voicing 8-band normalization | §4.2 | §4.2 (8-band → per-harm) | §4.2 (per-harm → 8-band) | US7634399 col. 6 |
| Magnitude resampling | §4.3 | — | — | US7634399 col. 5 |
| Unvoiced band compensation | §4.4 | — | — | US7634399 col. 6 |
| Cross-rate predictor (ρ=0.65) | §4.5, §5 | §4.3.5 (intra ρ) | §4.3.5 (intra ρ) | US7634399 col. 7 |
| Invalid frame substitution | §6 | §3.2.3, §9.1 | — | US7634399 col. 8 |
| Target FEC encode | §2.2 | — | §6 | — |
| Bit prioritization (forward) | §2.2 | — | §5 | — |
| Interleave (forward) | §2.2 | — | §7 | — |
| Identity passthrough | §8 | — | — | — |
| Quality metric framework | §11.5 | — | — | Objectives.md |
| Tandeming comparison | §11.6 | — | — | Objectives.md |
