# AMBE-3000 Decoder Implementation Spec (Channel Bits → PCM)

**Status:** Draft  
**Date:** 2026-04-16  
**Author:** Chance Lindsey  
**Scope:** Software decoder functionally equivalent to the DVSI AMBE-3000 chip
in decoder direction, for P25 full-rate (7200 bps, r33) and P25 half-rate
(3600 bps, r34) as the normative targets, parameterized over the chip's full
rate table (r0–r63) where the same algorithmic skeleton applies.

**Sources:**
- TIA-102.BABA-A Revision A (February 2014) — IMBE full-rate and AMBE+2
  half-rate wire format, FEC, quantizer tables, and baseline MBE synthesis
  (Chapters 1–17 + Annexes A–T).
- US5701390 (expired 2015-02-22) — phase regeneration from spectral
  envelope, four-case voiced synthesis, synthesis window with β=50.
- US8595002 (expired 2023-04-01) — half-rate AMBE+2 frame structure,
  quantization, FEC, per-harmonic voicing, error handling thresholds.
- US6199037 (expired 2017-12-04) — joint subframe voicing/pitch
  quantization (relevant when reconstructing 2-subframe-encoded rates).
- US7634399 (expires 2025-11-07) — ρ=0.65 prior-frame predictor used when
  rates transcode spectral magnitudes across fundamental-frequency changes
  (referenced by §4.3.5 for completeness; primary relevance is to the
  rate-converter spec).
- DVSI AMBE-3000F Vocoder Chip Users Manual v4.0 (October 2021) — rate
  table, rate control word format, wire protocol. See
  `AMBE-3000_Protocol_Spec.md` for the packet layer.
- AMBE-3000 HDK test vectors — `/mnt/share/P25-IQ-Samples/DVSI
  Software/Docs/AMBE-3000_HDK_tv/`. See `AMBE-3000_Test_Vector_Reference.md`.

**Companion documents in this directory:**
- `AMBE-3000_Objectives.md` — project goals, methodology, patent landscape
- `AMBE-3000_Patent_Reference.md` — expired-patent algorithmic reference
- `AMBE-3000_Protocol_Spec.md` — host ↔ chip USB-3000 packet protocol
- `AMBE-3000_Operational_Notes.md` — reset, init, firmware errata
- `annex_tables/rate_index_table.csv` — rate r0..r63 metadata
- `annex_tables/rate_control_words.csv` — RCW packing per rate

**Language convention:** C reference snippets are illustrative, not normative
Rust. Spec is language-neutral per project conventions.

---

## 1. Scope and Architecture

### 1.1 What This Decoder Does

Given a stream of channel bits (framed per the active rate), produce 8 kHz
PCM speech samples that are perceptually equivalent to what an AMBE-3000
chip produces from the same input, and bit-exact at the parameter-recovery
layer.

**Input:** one channel frame per 20 ms. Size depends on rate:
- Full-rate P25 IMBE (r33): 144 bits
- Half-rate P25 AMBE+2 (r34): 72 bits
- Other chip rates: 49–88 info bits per frame with a rate-specific FEC
  overhead; see §10

**Output:** 160 PCM samples per frame (16-bit signed, Q15), 20 ms at 8 kHz.

**Cross-frame state:** ~12 state items (pitch, voicing, spectral magnitudes,
phase accumulators, noise buffer, error-rate estimator, repeat counter).
See §4.4 and §9.

### 1.2 Block Diagram

```
  72 or 144 channel bits
         │
         ▼
┌────────────────────┐
│  §3 FEC decode     │    [24,12] ext Golay + [23,12] Golay + PN mod + Annex S
│  (rate-specific)   │    (half-rate) | Golay/Hamming + Annex H (full-rate)
└────────┬───────────┘
         │ û₀..û_N information bits + error metric
         ▼
┌────────────────────┐
│  §4 Parameter      │    pitch b̂₀ → ω̃₀, L̃
│  recovery          │    voicing b̂₁ → codebook expand → ṽ_l
│  (inverse quant)   │    gain + PRBA + HOC → per-block IDCT → M̃_l
└────────┬───────────┘
         │ (ω̃₀, L̃, ṽ_l, M̃_l)
         ▼
┌────────────────────┐
│  §4.3.6 Enhance    │    BABA-A §1.10 applied verbatim (half-rate §15
│  spectral mags     │    points to §8 unchanged). Produces M̄_l.
└────────┬───────────┘
         │ M̄_l
         ▼
┌────────────────────┐
│  §5 Phase regen    │    φ_l = Σ h(m)·B_{l+m},  B_l = log₂(M̄_l)
│  (US5701390)       │    h(m) = 2/(π m), m ∈ [-19..19]\{0}
└────────┬───────────┘
         │ φ_l
         ▼
┌──────────────────────────┐    ┌──────────────────────┐
│ §6 Voiced synthesis      │    │ §7 Unvoiced synth    │
│ 4-case transition model  │    │ white noise → FFT →  │
│ + 2nd-order phase poly   │    │ per-band scale →     │
│ (BABA-A §1.12.2 + patent)│    │ IFFT → OLA (BABA-A)  │
└──────────┬───────────────┘    └──────────┬───────────┘
           │ s̃_v(n)                         │ s̃_uv(n)
           └──────────────┬─────────────────┘
                          ▼
                    s̃(n) = s̃_v(n) + s̃_uv(n)
                          │
                          ▼
                   160 Q15 PCM samples
```

### 1.3 Relationship to BABA-A §1.12 / §2.9

BABA-A §1.12 defines the complete baseline MBE synthesizer (Eq. 117–142).
BABA-A §2.9 is a one-line cross-reference: "the half-rate decoder uses
the full-rate synthesis pipeline verbatim." The AMBE+2 decoder is
therefore **the BABA-A §1.12 synthesis with US5701390 phase regeneration
injected at the phase-state-update step (Eq. 140)**, plus rate-specific
FEC and parameter recovery.

This spec restates the four-case voiced transition table inline (§6) because
it is short, phase-regen-aware, and load-bearing for bit-exactness. The
long unvoiced pipeline (FFT/OLA/γ_w) is cross-referenced to BABA-A §1.12.1
rather than duplicated; see §7.

### 1.4 Relationship to the DVSI Rate Table

The AMBE-3000 supports 64 rates (r0–r63) selected via a 6×16-bit Rate
Control Word (RCW). Each rate fixes:
- Voice information bits (b̂₀..b̂_N), their bit widths, and their codebooks
- FEC scheme (none, single Golay, double Golay, Hamming, ad-hoc)
- Interleaver
- Optional scrambling / PN modulation
- Number of sub-frames per frame (1 or 2)

For P25 the two canonical rates are:

| Rate | Bit rate | Frame size | Sub-frames | FEC | Use |
|------|----------|------------|-----------:|-----|-----|
| r33  | 7200 bps | 144 bits   | 1          | §1.5 BABA-A | P25 Phase 1 FDMA (LDU) |
| r34  | 3600 bps | 72 bits    | 1          | §2.4 BABA-A | P25 Phase 2 TDMA (4V/2V) |

Other rates share the synthesis core (§5–§7) and differ only in how §3–§4
extract and de-quantize parameters. See §10 for the dispatch table.

### 1.5 Frame Timing

| Quantity | Value |
|----------|-------|
| Sample rate | 8000 Hz |
| Samples per frame (N) | 160 |
| Frame duration | 20 ms |
| Analysis window (encoder) | ~25–30 ms (rate-dependent) |
| Synthesis window `w_S(n)` span | 211 samples, n ∈ [−105, 105] |
| End-to-end algorithmic delay | ~80 ms (analysis + look-ahead + synth-OLA) |

The synthesizer needs two consecutive frames of reconstructed parameters
(current frame `(0)` and previous frame `(−1)`) to produce output samples
for the current frame. On cold start the `(−1)` state is taken from
§4.4 init values.

---

## 2. Input Frame Formats

### 2.1 Full-Rate IMBE (r33, P25 Phase 1 FDMA)

**Channel frame:** 144 bits, organised as 8 prioritized bit vectors
`ĉ₀..ĉ₇` with per-vector FEC:

| Vector | Info bits | FEC | Coded bits |
|--------|----------:|-----|----------:|
| ĉ₀ | 12 | [23,12] Golay | 23 |
| ĉ₁ | 12 | [23,12] Golay | 23 |
| ĉ₂ | 12 | [23,12] Golay | 23 |
| ĉ₃ | 12 | [23,12] Golay | 23 |
| ĉ₄ | 11 | [15,11] Hamming | 15 |
| ĉ₅ | 11 | [15,11] Hamming | 15 |
| ĉ₆ | 11 | [15,11] Hamming | 15 |
| ĉ₇ | 7  | none | 7 |
| **Total** | **88** | | **144** |

Info bit allocation is L-dependent (number of harmonics 9 ≤ L ≤ 56) and
is driven by BABA-A Annexes E–G bit-allocation tables.

**Decoder responsibility:** defer the full chain (Annex H deinterleave →
Golay/Hamming decode → PN-demodulation of ĉ₁ → bit-prioritization inverse
scan → parameter recovery) to the P25 FDMA implementation spec:

> `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §1.5–§1.8

The output `(ω̃₀, L̃, ṽ_k for k=1..K̃, M̃_l for l=1..L̃)` feeds §4.3.6
enhancement, §5 phase regeneration, and §6–§7 synthesis in this spec.

### 2.2 Half-Rate AMBE+2 (r34, P25 Phase 2 TDMA)

**Channel frame:** 72 bits, organised as 4 prioritized bit vectors
`ĉ₀..ĉ₃`:

| Vector | Info bits | FEC | Coded bits |
|--------|----------:|-----|----------:|
| ĉ₀ | 12 | [24,12] ext Golay | 24 |
| ĉ₁ | 12 | [23,12] Golay + PN | 23 |
| ĉ₂ | 11 | none | 11 |
| ĉ₃ | 14 | none | 14 |
| **Total** | **49** | | **72** |

Only ĉ₀ and ĉ₁ are FEC-protected. ĉ₂ and ĉ₃ are uncoded (BABA-A §14.2
Eq. 191–192). The single PN-modulation vector `m̂₁` is applied to ĉ₁ only.

**Information bit allocation** (BABA-A §2.2, this spec):

| Symbol | Bits | Levels | Codebook | Content |
|--------|-----:|-------:|----------|---------|
| b̂₀ | 7 | 120 | Annex L | Pitch → (ω̃₀, L̃) |
| b̂₁ | 5 | 32  | Annex M | V/UV codebook index |
| b̂₂ | 5 | 32  | Annex O | Differential gain |
| b̂₃ | 9 | 512 | Annex P | PRBA₂₄ vector |
| b̂₄ | 7 | 128 | Annex Q | PRBA₅₈ vector |
| b̂₅ | 5 | 32  | Annex R | HOC₁ |
| b̂₆ | 4 | 16  | Annex R | HOC₂ |
| b̂₇ | 4 | 16  | Annex R | HOC₃ |
| b̂₈ | 3 | 8   | Annex R | HOC₄ |
| **Total** | **49** | | | |

The 49 → 4×vector packing map (Tables 15–18 of BABA-A, pages 67–68) is
committed as `standards/TIA-102.BABA-A/annex_tables/ambe_bit_prioritization.csv`.

**Tone / silence / erasure frame detection:** if `b̂₀ ∈ [120, 125]` the
frame is a silence or erasure frame (§9). If `b̂₀ ∈ [126, 127]` it is a
tone frame — dispatch to the BABA-A §2.10 tone-frame pipeline instead of
the voice pipeline (§4).

---

## 3. FEC Layer

### 3.1 Full-Rate (r33) FEC — Reference Only

Defer to `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §1.5
(Golay/Hamming), §1.6 (PN modulation), and §1.7 (Annex H interleaver).
Decoder output is the 88 information bits plus error metrics ε₀..ε₃ (one
per Golay codeword, for the §4.3.8 error-rate estimator).

### 3.2 Half-Rate (r34) FEC — Specified Here

Source: BABA-A §14.2 Eq. 189–192, §14.3 PN, §14.4 Annex S, page 66–69,
corresponding to US8595002 col. 23 lines 1–60 at the patent level.

#### 3.2.1 Receive Pipeline (In Order)

Given a 72-bit raw frame, extract `(ω̃₀, L̃, ṽ_l, M̃_l, error metric)`:

1. **Annex S de-interleave.** The 72 channel bits are packed as 36 dibits
   (2 bits each) with a fixed permutation. De-interleave to recover the
   4 code vectors `c̃₀ (24b), c̃₁ (23b), c̃₂ (11b), c̃₃ (14b)`.
2. **[24,12] extended Golay decode of c̃₀.** Strip the overall parity
   bit, run the [23,12] Golay decoder, verify overall parity. Output:
   12 information bits `û₀` plus error count ε₀ (number of bits
   corrected, 0..3, or −1 if uncorrectable).
3. **PN-demodulate c̃₁.** Derive the 23-bit modulation vector `m̂₁`
   from the 8 MSBs of `û₀` (the pitch quantizer b̂₀) using the linear
   congruential sequence (BABA-A §2.5):
   ```c
   /* p_r(0) = 16 * u0,  p_r(n) = (173 * p_r(n-1) + 13849) mod 65536 */
   static void halfrate_pn_init(uint16_t u0_8msb, uint32_t p_r[24]) {
       p_r[0] = ((uint32_t)u0_8msb) * 16u;
       for (int n = 1; n < 24; n++)
           p_r[n] = (173u * p_r[n-1] + 13849u) & 0xFFFFu;
   }
   /* m̂₁(n) = p_r(n+1) >> 15, i.e. MSB of p_r(n+1), for n = 0..22.   */
   static uint32_t halfrate_pn_modulation(const uint32_t p_r[24]) {
       uint32_t m1 = 0;
       for (int n = 0; n < 23; n++)
           m1 |= ((p_r[n+1] >> 15) & 1u) << n;
       return m1;   /* XOR this with c̃₁ before Golay decode */
   }
   ```
   XOR `m̂₁` with `c̃₁` to undo the encoder's PN modulation.
4. **[23,12] Golay decode of descrambled c̃₁.** Output: 12 information
   bits `û₁` plus error count ε₁.
5. **Pass-through c̃₂, c̃₃.** Uncoded: `û₂ = c̃₂` (11 bits),
   `û₃ = c̃₃` (14 bits).
6. **Inverse bit prioritization.** Map `(û₀..û₃)` → `(b̂₀..b̂₈)` via
   BABA-A Tables 15–18 (committed CSV: see §2.2). This is a fixed 49-entry
   (src_vec, src_bit) → (dst_param, dst_bit) permutation.
7. **Error metric.** `ε_T = ε₀ + ε₁` (Eq. 196). Update the smoothed
   error-rate estimator per §4.3.8.

#### 3.2.2 Data-Dependent Scrambling — Relationship to Patent Text

US8595002 describes the half-rate FEC as "a scrambling key derived from
the [24,12] output bits." BABA-A's Eq. 190 (P25-specific instantiation) is
equivalent: `16 * u0` where `u0` denotes the 8-bit b̂₀ (pitch) field. Since
the [24,12] code is systematic, the 8 MSBs of the decoded [24,12] output
bits equal b̂₀. The two descriptions are therefore consistent. The P25
specialization picks a particular seed derivation; the AMBE-3000 chip's
non-P25 rates use the same LCG but with rate-specific seed/length tuning
(flagged in §10; not normatively specified here).

#### 3.2.3 Invalid-Frame Detection

Source: BABA-A §14.6 Eq. 198–199 (page 70), US8595002 col. 26 lines 5–20.

A frame is **invalid** (trigger frame-repeat per §9) if any of:
- `ε₀ ≥ 4` (Golay-24 exhausted its correction capability)
- `ε₀ ≥ 2` **and** `ε_T ≥ 6`
- `b̂₀ ∈ [120, 123]` (silence / erasure sentinel per §13.1 BABA-A)

The patent's rule-of-thumb "sum of Golay decode errors ≥ 6 → invalid"
(Patent_Reference §2 final paragraph) is the `ε_T ≥ 6` branch of the
above; BABA-A adds the `ε₀ ≥ 4` hard-limit as an additional trigger.
Both must be checked.

#### 3.2.4 Mute Trigger

Source: BABA-A §14.7 (page 71).

Mute output if either:
- `ε_R(0) > 0.096` (smoothed error rate above threshold, §4.3.8)
- 4 consecutive voice frames have been marked for repeat

Patent_Reference §2 mentions "three consecutive invalid frames" as the
patent-era threshold; BABA-A updates this to 4 for the P25 profile. The
software decoder uses 4 (BABA-A is authoritative for P25).

### 3.3 C Reference — Golay Decoders

The [23,12] and [24,12] decoders are standard. A compact syndrome-table
implementation:

```c
/* 12-bit generator polynomial: G(x) = x^11 + x^10 + x^6 + x^5 + x^4 + x^2 + 1
 * (0xC75 = 110001110101 in MSB-first), standard for [23,12] Golay.
 * Returns 0..3 = number of corrections, or -1 if uncorrectable. */
int golay_23_12_decode(uint32_t codeword_23b, uint16_t *info_out);

/* [24,12] extended Golay = [23,12] + even parity. Decodes by stripping
 * the LSB parity bit, running [23,12], then verifying overall parity.
 * Returns 0..3 = corrections, or -1 if parity check fails (4 errors).    */
int golay_24_12_decode(uint32_t codeword_24b, uint16_t *info_out);
```

Implementation is shared with the P25 FDMA decoder (same [23,12] code);
see that spec's §1.5.1 for the syndrome table.

### 3.4 Annex S Interleaver — Complete Mapping

BABA-A Annex S Table lists all 36 dibit positions. The first six were
transcribed in BABA-A impl spec §2.6; the full 36-row table belongs in
`annex_tables/annex_s_interleave.csv` (committed as part of the BABA-A
extraction, not duplicated here). The decoder uses it as a lookup to
scatter/gather 72 bits across 36 dibit positions.

**Important:** for TDMA air-interface use, the 72-bit half-rate frame also
passes through the TDMA burst bit-position tables in TIA-102.BBAC-1
Annex E (`standards/TIA-102.BBAC-1/P25_TDMA_Annex_E_Burst_Bit_Tables.md`).
That is a separate burst-level interleaving, not the vocoder-level Annex S.

---

## 4. Parameter Recovery

Given `(û₀..û_N)` from §3, reconstruct the MBE model parameters needed by
the synthesizer: fundamental frequency ω̃₀, number of harmonics L̃,
voicing decisions ṽ_l (per-harmonic), spectral magnitudes M̃_l, and
(for half-rate only) the V/UV-band count K̃ derived from Annex L.

### 4.1 Fundamental Frequency

#### 4.1.1 Full-Rate (r33)

Analytical inverse of the BABA-A Eq. 45–48 quantizer. Defer to
BABA-A impl spec §1.3.1 for complete code. Initial-state value:
`ω̃₀(−1) = 0.02985·π ≈ 0.0937 rad/sample` (L̃ = 30).

#### 4.1.2 Half-Rate (r34)

7-bit logarithmic quantizer. Source: BABA-A §13.1 Eq. 143–144 (page 58),
US8595002 col. 17 lines 35–50. Inverse:

```
ω̃₀ = 2π · f_0   where f_0 comes from Annex L CSV:
      (f_0, L̃)  = ANNEX_L[b̂₀]    for b̂₀ ∈ [0, 119]
      b̂₀ ∈ [120, 127]   — reserved: silence, erasure, tone frames (§9, BABA-A §13.1)
```

Annex L is committed as
`standards/TIA-102.BABA-A/annex_tables/annex_l_pitch.csv` (120 rows, columns
`b0, f0, omega_0, L`). Row indexing is direct: `b̂₀ → row`.

#### 4.1.3 Encoder's Log Quantizer (For Reference)

The forward quantizer used by the encoder (US8595002 col. 17):
```
b̂_fund = 0                                    if f_0 > 0.0503
b̂_fund = 119                                  if f_0 < 0.00811
b̂_fund = ⌊−195.626 − 45.368·log₂(f_0)⌋        otherwise
```
Annex L is generated by walking b̂_fund 0..119 back through the inverse
of this formula (plus the f_0 → L̃ = ⌊3812.5 / (8000·f_0)⌋ relation).

### 4.2 Voicing Decisions

#### 4.2.1 Full-Rate (r33)

Per-band, uncoded. Defer to BABA-A impl spec §1.3.2.

#### 4.2.2 Half-Rate (r34)

5-bit codebook index → 8-entry vector, then per-harmonic expansion.
Source: BABA-A §13.2 Eq. 145–149 (page 59), US8595002 col. 18 lines 20–55,
Annex M (32 × 8 codebook).

```c
/* Expand 5-bit V/UV codebook index to per-harmonic voicing decisions. */
void halfrate_vuv_expand(uint8_t b1,           /* b̂₁, 0..31 */
                         double omega_0,       /* rad/sample */
                         uint8_t L,            /* harmonic count */
                         uint8_t v_harm[/*L*/])
{
    /* Annex M row: ambe_vuv_codebook[b1][0..7] ∈ {0,1}. */
    for (uint8_t l = 1; l <= L; l++) {
        int j = (int)floor((double)l * 16.0 * omega_0 / (2.0 * M_PI));
        if (j < 0) j = 0;
        if (j > 7) j = 7;
        v_harm[l - 1] = ambe_vuv_codebook[b1][j];
    }
}
```

**Silence override.** Per BABA-A §13.1 Table 14, `b̂₀ ∈ [120, 123]` or
`b̂₁ = 0` together may trigger silence semantics. The all-voiced or
all-unvoiced codebook rows at `b̂₁ ∈ {0, 1}` and `b̂₁ ∈ [16, 31]` respectively
give deterministic V/UV vectors for upgrade-compatibility (see US8595002
col. 19 lines 10–30 and BABA-A §13.2).

#### 4.2.3 Per-Harmonic vs Per-Band

US8595002 describes AMBE+2's key decoder improvement as "a separate voicing
decision for each harmonic rather than for groups of three or more harmonics
as in baseline IMBE." The §4.2.2 expansion step achieves this — the 8-entry
codebook represents a logical 8-band decomposition, but the synthesizer
consumes ṽ_l for each harmonic l = 1..L̃, so the physical granularity is
per-harmonic. This gives smoother voiced/unvoiced transitions than full-rate's
per-band ṽ_k, especially for mixed-voicing sounds and at high L̃.

### 4.3 Spectral Magnitudes

The half-rate pipeline (BABA-A §13.4, pages 60–66) is the most algorithmically
dense part of the decoder. Full-rate spectral recovery is BABA-A impl spec
§1.8; half-rate is BABA-A impl spec §2.11–§2.13. This section consolidates
the half-rate pipeline and adds AMBE-3000 specifics.

#### 4.3.1 Block Structure

Source: BABA-A Annex N (§12.10), which fixes the 4 frequency blocks. For
each L̃, Annex N gives the number of harmonics in each of the 4 blocks
(J_1..J_4) with J_1 + J_2 + J_3 + J_4 = L̃.

Blocks are **contiguous, ordered** (block 1 = lowest harmonics, block 4 =
highest) and **approximately equal in logical band width**, not equal harmonic
counts.

#### 4.3.2 Differential Gain Recovery (5 bits, b̂₂ → γ̃)

Source: BABA-A §13.4.1 Eq. 168 (page 63), Annex O (32-entry quantizer).

```
Δ̃_γ = AMBE_GAIN_QUANTIZER[b̂₂]          (Annex O, 32 entries, centered on 0)
γ̃(0) = Δ̃_γ + 0.5 · γ̃(−1)                (Eq. 168)
```

`γ̃(−1)` is the reconstructed gain from the **previous voice frame** only
(not updated on tone, silence, or erasure). Initial value 0.

Patent reference: US8595002 calls this "differential gain quantization, 5 bits"
(Patent_Reference §2). The 0.5 predictor coefficient is implicit in the
patent and explicit in BABA-A.

#### 4.3.3 PRBA Vector Recovery (b̂₃ → PRBA₂₄; b̂₄ → PRBA₅₈)

Source: BABA-A §13.4.2 Eq. 169–178 (page 64), Annex P (9-bit VQ), Annex Q
(7-bit VQ). US8595002 Table 3–4.

```
PRBA₂₄ vector (3 elements: G_2, G_3, G_4) = ANNEX_P[b̂₃]
PRBA₅₈ vector (4 elements: G_5, G_6, G_7, G_8) = ANNEX_Q[b̂₄]
```

The 8-element PRBA vector `(G_2..G_8, plus G_1 = 0 reserved)` represents
the first two DCT coefficients per block (DC and first-non-DC). G_2 and G_3
are block-1's 2 PRBA coefficients; G_4 and G_5 are block-2's; and so on.

BABA-A includes a second intermediate transform (Eq. 171–178) mixing PRBA
coefficients before they are distributed back to blocks. Implement per
BABA-A §13.4.2 verbatim.

#### 4.3.4 HOC Vector Recovery (b̂₅..b̂₈ → HOC₁..HOC₄)

Source: BABA-A §13.4.3 Eq. 179–181 (page 65), Annex R (5/4/4/3-bit VQs).

```
HOC_k vector (4 elements) = ANNEX_R[k][b̂_{4+k}]       for k = 1..4
```

The 4 HOC vectors populate DCT coefficients beyond index 1 within each
block. Per US8595002 col. 21–22, if a block has only 1 or 2 harmonics,
the corresponding HOC entries are zeroed.

**Bit-reduction trick** (Patent_Reference §2): half-rate vocoder variants
can drop bits by using a subset of the Annex R codebook — implemented as
a reduced lookup table with fewer rows. Handled by the rate-dispatch
layer (§10), not here.

#### 4.3.5 Per-Block Inverse DCT + Prediction Residual → Log-Magnitudes

Source: BABA-A §13.4.4 Eq. 182–188 (page 65–66).

For each block i = 1..4:
1. Assemble the block's DCT coefficient vector `C_i = (G_{2i}, G_{2i+1},
   H_{i,1}, H_{i,2}, H_{i,3}, H_{i,4})` — first 2 from PRBA, next 4 from HOC
   (zero-padded where J_i < 6).
2. Compute length-J_i inverse DCT → per-harmonic log-magnitude residual
   `T̃_l` for l in block i.
3. Assemble prediction residual:
   ```
   T̃ = (T̃_1, T̃_2, ..., T̃_L̃)           (Eq. 182)
   ```
4. Reconstruct log-magnitudes using prior-frame prediction:
   ```
   log₂(M̃_l(0)) = T̃_l + ρ · log₂(M̃_l(−1))       (BABA-A Eq. 185, half-rate)
   ```
   Half-rate AMBE+2 uses the **literal constant ρ = 0.65** per BABA-A
   Eq. 185 (page 65 in the half-rate decoder section). Full-rate uses
   an L̂(0)-dependent schedule (Eq. 55, range 0.4–0.7) — not relevant
   here since the full-rate decode path defers to BABA-A impl spec
   §1.8.5. The US7634399 rate-converter predictor (rate converter
   spec §4.5) is **also literally 0.65** — same numeric value as
   half-rate intra-rate, but a distinct predictor-state instance.
   See [`analysis/vocoder_decode_disambiguations.md`](../../analysis/vocoder_decode_disambiguations.md)
   §3 for the full full-rate-vs-half-rate ρ analysis.
5. Convert to linear: `M̃_l(0) = 2^{log₂(M̃_l(0))}`.

When `L̃(0) ≠ L̃(−1)`, prior-frame magnitudes beyond `min(L̃(0), L̃(−1))`
are interpolated/resampled per BABA-A §13.4.4 final paragraph — the same
structural operation the rate-converter does continuously, but here only
across frame-to-frame L̃ changes.

#### 4.3.6 Spectral Amplitude Enhancement

Source: BABA-A §15 (half-rate points back to §8 unchanged). Eq. 105–111.

Enhancement produces `M̄_l(0)` from `M̃_l(0)`. This is a perceptual post-filter
that sharpens formants and adjusts energy. Defer to BABA-A impl spec §1.10
for the full algorithm, constants, and C reference.

**Do not skip this stage.** Phase regeneration (§5) takes `log₂(M̄_l)`, not
`log₂(M̃_l)`, as input. Skipping enhancement changes the phase as well as
the magnitude.

#### 4.3.7 Frame Repeat / Mute Overrides

On invalid-frame (§3.2.3), copy forward per BABA-A Eq. 200–205 (page 71):
```
ω̃₀(0) = ω̃₀(−1)
L̃(0)  = L̃(−1)
K̃(0)  = K̃(−1)
ṽ_l(0) = ṽ_l(−1)    for 1 ≤ l ≤ L̃(−1)
M̃_l(0) = M̃_l(−1)   for 1 ≤ l ≤ L̃(−1)
M̄_l(0) = M̄_l(−1)   for 1 ≤ l ≤ L̃(−1)
```

Full-rate uses Eq. 99–104 with equivalent semantics (per-band ṽ_k rather
than per-harmonic ṽ_l).

**State not restored:** the error-rate estimator ε_R, phase accumulators
φ_l / ψ_l, noise buffer u(n), and τ_M threshold state continue to evolve
normally. See §9 and BABA-A impl spec §1.13.

#### 4.3.8 Error-Rate Estimator

Source: BABA-A §14.5 Eq. 196–197 (page 70). Half-rate:
```
ε_T = ε₀ + ε₁                                              (Eq. 196)
ε_R(0) = 0.95 · ε_R(−1) + 0.001064 · ε_T                   (Eq. 197)
```
Initial `ε_R(−1) = 0`. The 0.001064 coefficient is half-rate specific.

Full-rate uses a similar recurrence with different weights; see BABA-A
impl spec §1.11.

### 4.4 Cross-Frame State Table

Extension of BABA-A impl spec §1.13 with AMBE-3000 decoder-specific state:

| State | Init | Updated by | Consumed by |
|-------|------|-----------|-------------|
| ω̃₀(−1) | 0.02985·π (full) / Annex L row 30 (half) | §4.1 | §5 phase regen scaling, §6 voiced synth |
| L̃(−1) | 30 | §4.1 | §4.3.5, §6 |
| K̃(−1) | 10 (full only) | §4.2 | §1.11.3 V/UV smoothing (BABA-A) |
| ṽ_l(−1) for l=1..56 | 0 (all UV) | §4.2 | §6 voiced synth |
| M̃_l(−1) for l=1..56 | 1.0 | §4.3.5 | §4.3.5 predictor next frame |
| γ̃(−1) | 0 | §4.3.2 | §4.3.2 next frame |
| M̄_l(−1) for l=1..56 | 0 | §4.3.6, §1.11.3 | §6 voiced synth, §5 phase regen |
| S_E(−1) | 75000 | §4.3.6 enhancement Eq. 111 | V/UV smoothing, §4.3.6 next frame |
| τ_M(−1) | 20480 | V/UV smoothing Eq. 115 | V/UV smoothing next frame |
| ε_R(−1) | 0 | §4.3.8 | §3.2.4 mute trigger, §4.3.6 |
| φ_l(−1) for l=1..56 | 0 | §6 Eq. 140 | §6 next frame |
| ψ_l(−1) for l=1..56 | 0 | §6 Eq. 139 | §6 next frame |
| ũ_w(n, −1) for n=−128..127 | 0 | §7 Eq. 125 | §7 OLA next frame |
| u(n) noise seq | u(−105) = 3147 | §7 Eq. 117 | §7 each frame |
| Repeat counter | 0 | §3.2.4, §9 | §3.2.4 mute trigger |

Cold start = init column values exactly. Frame-repeat preserves phase/noise/
ε_R/τ_M state (don't reset). Mute preserves everything for post-mute
re-acquisition.

---

## 5. Phase Regeneration (US5701390)

### 5.1 Why This Matters

Baseline IMBE (BABA-A §1.12.2 Eq. 141) seeds the voiced-synthesis phase
using noise-derived perturbations: `ρ_l(0) = (2π/53125)·u(l) − π`, uniform
in [−π, π). Without structural coupling between phase and spectrum, the
synthesized voiced signal sounds "buzzy" or "reverberant" (US5701390
col. 1 lines 45–60).

AMBE+2 fixes this by **deriving phase from the shape of the log-spectral
envelope itself**. Sharp spectral peaks (formants, poles) get coherent
phase; smooth regions get phase values that track the local envelope
gradient. The effect is a dramatic perceptual improvement at unchanged
bit rate — the central contribution of US5701390.

Per §1.3 of this spec, the AMBE-3000 decoder's speech synthesizer is
BABA-A §1.12 **with Eq. 141 replaced by the phase regenerator**. Every
other part of BABA-A §1.12 continues unchanged.

### 5.2 Algorithm

Source: US5701390 Eq. 8–9 + col. 7 lines 30–55. Kernel is fully specified.

**Step 1 — Log-compressed spectral envelope:**
```
B_l = log₂(M̄_l)       for 1 ≤ l ≤ L̃(0)
```
Use the enhanced magnitudes `M̄_l(0)` from §4.3.6, not the raw `M̃_l(0)`.
Indices beyond L̃(0) treat `B_l = 0` (patent col. 7 lines 20–28;
equivalent to `M̄_l = 1`, which is the prediction default for uninitialized
harmonics).

**Step 2 — Edge-detection kernel:**
```
           ┌ 2 / (π · m)     for m ≠ 0
h(m)  =    ┤
           └ 0               for m = 0

m ∈ [−D, +D]     with   D = 19
```

Properties (from US5701390 col. 7 lines 40–55):
- Antisymmetric: `h(−m) = −h(m)`
- `h(m) > 0` for `m > 0`
- Decays as `1/m` — slow spectral smoothing, long temporal support
- `h(0) = 0` (required; otherwise φ_l would contain a useless DC term)

**D = 19** is the patent-recommended half-width, "essentially equivalent
to longer lengths." Kernel thus has 2D + 1 = 39 taps of which 38 are
non-zero (h(0) = 0).

**Step 3 — Regenerated phase:**
```
φ_l(0) = γ · Σ_{m=-D, m≠0}^{+D}  h(m) · B_{l+m}       (Eq. 8)
```

with scaling `γ = 0.44` (patent col. 7 line 55, post-Eq. 9). Indices
`l + m` outside `[1, L̃(0)]` use `B = 0` (zero-padding).

```c
#define PHASE_KERNEL_D 19

/* Precomputed h(m) · γ for m = 1..19, where γ = 0.44.
 * h(m) = 2/(π·m), so h(m)·γ = 0.88/(π·m).                         */
static const double phase_kernel[PHASE_KERNEL_D + 1] = {
    0.0,                                  /* h(0) = 0 */
    0.88 / (M_PI * 1.0),
    0.88 / (M_PI * 2.0),
    0.88 / (M_PI * 3.0),
    /* ... through m = 19                                           */
    0.88 / (M_PI * 19.0),
};

/* Regenerate phase φ_l for l = 1..L. Reads B_l = log2(M̄_l).
 * Writes phi[l-1] for l = 1..L. Harmonic indices outside 1..L
 * contribute 0 (zero-padding).                                     */
void ambe_phase_regen(const double M_bar[/* L */],
                      int L,
                      double phi[/* L */])
{
    double B[64];   /* L ≤ 56, room to spare */
    for (int l = 1; l <= L; l++)
        B[l - 1] = log2(M_bar[l - 1]);

    for (int l = 1; l <= L; l++) {
        double acc = 0.0;
        /* Antisymmetric kernel: sum of [h(m) · (B_{l+m} - B_{l-m})]
         * for m = 1..D, since h(-m) = -h(m).                       */
        for (int m = 1; m <= PHASE_KERNEL_D; m++) {
            double B_plus  = (l + m >= 1 && l + m <= L) ? B[l + m - 1] : 0.0;
            double B_minus = (l - m >= 1 && l - m <= L) ? B[l - m - 1] : 0.0;
            acc += phase_kernel[m] * (B_plus - B_minus);
        }
        phi[l - 1] = acc;
    }
}
```

### 5.3 Integration with BABA-A §1.12.2

Replace BABA-A Eq. 140–141 with:

```
ψ_l(0) = ψ_l(−1) + [ω̃₀(−1) + ω̃₀(0)] · l·N/2                   (Eq. 139 unchanged)

         ┌ ψ_l(0)                            for 1 ≤ l ≤ ⌊L̃/4⌋
φ_l(0) = ┤
         └ ψ_l(0) + φ_regen,l                for ⌊L̃/4⌋ < l ≤ max(L̃(−1), L̃(0))
```

where `φ_regen,l` is §5.2 step 3 output.

**Lower-harmonic band (l ≤ L̃/4).** Per US5701390 col. 8 lines 15–30, the
lowest ~quarter of the harmonic range gets the accumulated phase `ψ_l(0)`
**without** regeneration — those harmonics are typically pitch-locked,
and accumulating phase from the previous frame's pitch is more stable
than regenerating them. Above the quarter point, regeneration takes over.

BABA-A Eq. 141 (noise-perturbed phase) is dropped entirely for the AMBE+2
decoder. The noise sequence u(n) remains in use for the unvoiced
synthesis (§7, Eq. 117); only the phase-perturbation term is removed.

### 5.4 Numerical Notes

- `log2(0)` is −∞. When `M̄_l = 0` (unvoiced harmonics past enhancement),
  clamp `B_l` to a floor (e.g., `log2(1e-10)` ≈ −33) or set `B_l = 0`.
  Both are observed in existing open-source MBE decoders; the bit-exact
  DVSI behavior is under investigation (candidate for an analysis/ entry
  after test-vector validation).
- Kernel coefficients `h(m) = 2/(π m)` are the continuous-domain
  Hilbert-transform impulse response. The phase regenerator is
  effectively computing the imaginary part of the analytic signal of
  `B_l` — i.e., approximately the complex cepstrum of the log-spectrum.
  US5701390 col. 7 lines 25–40 states this equivalence and motivates
  `h(m) = 2/(π m)` as the optimum continuous kernel.
- Scaling `γ = 0.44` is empirical; patent col. 7 line 55 notes only that
  "γ = 0.44 has been found to work well." Fine-tuning γ against test
  vectors may yield closer bit-match to DVSI reference output and is a
  candidate for the `analysis/` log if mismatches are observed.

### 5.5 Cold-Start and Mute Interaction

On cold start (frame 0), `M̄_l(−1) = 0` everywhere and no meaningful
spectral envelope history exists. Compute `φ_l(0)` from the current
frame's `M̄_l(0)` alone — the regenerator is memoryless across frames.
`ψ_l(0)` uses `ψ_l(−1) = 0` and `ω̃₀(−1) = 0.02985·π` as documented in
§4.4.

After a mute period, `M̄_l(−1)` has continued to track per BABA-A Eq.
200–205 copy-forward, so phase regeneration resumes naturally on the
first valid frame post-mute. No special handling.

---

## 6. Voiced Synthesis

Source: BABA-A §1.12.2 Eq. 127–138, US5701390 Eq. 13–17 (col. 8–10).

### 6.1 Structure

```
s̃_v(n) = Σ_{l=1}^{max(L̃(−1), L̃(0))}  2 · s̃_{v,l}(n)         for 0 ≤ n < N  (Eq. 127)
```

Out-of-range magnitudes are treated as zero-and-unvoiced:
```
M̄_l(0)  = 0, ṽ_l(0)  = 0    for l > L̃(0)
M̄_l(−1) = 0, ṽ_l(−1) = 0    for l > L̃(−1)
```

### 6.2 Four-Case Transition Model

Per-harmonic rule for `s̃_{v,l}(n)` depending on V/UV across frames:

| Prev | Curr | Condition | Formula | BABA-A |
|:----:|:----:|-----------|---------|-------:|
| UV | UV | — | `s̃_{v,l}(n) = 0` | Eq. 130 |
| V  | UV | — | `w_S(n) · M̄_l(−1) · cos[ω̃₀(−1)·n·l + φ_l(−1)]` | Eq. 131 |
| UV | V  | — | `w_S(n−N) · M̄_l(0) · cos[ω̃₀(0)·(n−N)·l + φ_l(0)]` | Eq. 132 |
| V  | V  | `l ≥ 8` **or** `|Δω̃₀·l/ω̃₀(0)| ≥ 0.1` | sum of two terms above | Eq. 133 |
| V  | V  | `l < 8` **and** `|Δω̃₀·l/ω̃₀(0)| < 0.1` | ramp: `a_l(n)·cos(θ_l(n))` | Eq. 134 |

`Δω̃₀ = ω̃₀(0) − ω̃₀(−1)`. Phase regen (§5) produces `φ_l(0)` as an input
to this table; `φ_l(−1)` is the previous frame's stored regenerated phase.

### 6.3 Low-Harmonic Continuous-Phase Branch (Case 4b)

Source: BABA-A Eq. 134–138.

```
s̃_{v,l}(n) = a_l(n) · cos(θ_l(n))                              (Eq. 134)

a_l(n) = M̄_l(−1) + (n/N) · [M̄_l(0) − M̄_l(−1)]                 (Eq. 135)

θ_l(n) = φ_l(−1)
       + [ω̃₀(−1)·l + Δω_l(0)] · n
       + [ω̃₀(0)·l − ω̃₀(−1)·l] · n² / (2N)                      (Eq. 136)

Δφ_l(n) = φ_l(0) − φ_l(−1) − [ω̃₀(−1) + ω̃₀(0)] · l·N/2         (Eq. 137)

Δω_l(0) = (1/N) · [ Δφ_l(0) − 2π · ⌊(Δφ_l(0) + π) / (2π)⌋ ]    (Eq. 138)
```

The 2nd-order phase polynomial ensures:
1. Phase continuity with the previous frame at `n = 0`: `θ_l(0) = φ_l(−1)`.
2. Phase match to regenerated target at `n = N`: `θ_l(N) = φ_l(0) + 2π·k`
   for some integer k (Δω_l closes the fractional residual).

The `2π · ⌊(Δφ_l + π)/(2π)⌋` term wraps phase discontinuity into the
smallest `[−π, π)` residual and corrects via the linear `Δω_l` term
rather than allowing phase to jump.

### 6.4 Synthesis Window

Source: BABA-A Annex I (impl spec §12.7). Full 211-sample window
`w_S(n)` for n ∈ [−105, +105], committed as
`standards/TIA-102.BABA-A/annex_tables/annex_i_synthesis_window.csv`.

The window satisfies `w_S(n) + w_S(n + N) = 1` for `0 ≤ n < N` (overlap-add
constraint). US5701390 Eq. 22 describes the same window with β=50 as the
linear-interpolation implementation, essentially equivalent to the Annex I
tabulated values.

### 6.5 Phase-State Update

Per frame, for `1 ≤ l ≤ 56` regardless of V/UV status:

```
ψ_l(0) = ψ_l(−1) + [ω̃₀(−1) + ω̃₀(0)] · l·N/2                   (Eq. 139)

         ┌ ψ_l(0)                            for 1 ≤ l ≤ ⌊L̃/4⌋
φ_l(0) = ┤                                                      (Modified Eq. 140)
         └ ψ_l(0) + φ_regen,l                for ⌊L̃/4⌋ < l ≤ max(L̃(−1), L̃(0))
```

`φ_regen,l` is §5.2 output. BABA-A Eq. 141 (noise-based phase) is
**dropped** for the AMBE+2 decoder, per §5.3.

**Initial state:** `ψ_l(−1) = 0`, `φ_l(−1) = 0` for all l (BABA-A §4.4).

### 6.6 Numerical Considerations

- Large-L harmonic sums accumulate floating-point error. Double precision
  is sufficient; float would lose ~2–3 dB SNR on test vectors near the
  Nyquist band (empirically observed in JMBE/mbelib ports).
- `cos` inside the per-sample inner loop is the hot path. A 2048-entry
  quarter-wave lookup with linear interpolation costs ~0.5 dB SNR and
  is adequate for real-time use. For bit-exactness against reference,
  use library `cos`.

---

## 7. Unvoiced Synthesis

Source: BABA-A §1.12.1 Eq. 117–126.

Cross-reference. Reproducing the FFT/OLA pipeline here would duplicate
~150 lines of BABA-A impl spec §1.12.1 without value. The pipeline is:

1. **White noise** `u(n)` via the Lehmer LCG `u(n+1) = (171·u(n) +
   11213) mod 53125`, initial `u(−105) = 3147` (Eq. 117).
2. **Windowed 256-point DFT** of `u(n) · w_S(n)` (Eq. 118).
3. **Per-band spectral shaping** — zero bins falling in voiced bands,
   match magnitude `M̄_l` in unvoiced bands (Eq. 119–124), scaled by
   a precomputable constant γ_w (Eq. 121).
4. **Inverse 256-point DFT** (Eq. 125).
5. **Weighted overlap-add** with the previous frame's buffer (Eq. 126)
   → `s̃_uv(n)` for `0 ≤ n < 160`.

**γ_w calibration open issue.** The BABA-A spec value γ_w = 146.643 produces
unvoiced output ~150× louder than DVSI reference on test vectors (BABA-A
impl spec §1.12.1 calibration table). Root cause is under investigation
per `standards/TIA-102.BABA-A/analysis/vocoder_decode_disambiguations.md`
§11. The AMBE-3000 decoder inherits this open issue. Candidates: different
`M̃_l` scale convention at the Annex E quantizer, unvoiced-norm formula
mismatch, or DVSI-specific normalization constant. Flag for empirical
calibration against tv-std test vectors; expect to resolve alongside the
BABA-A investigation.

**Per-harmonic voicing integration.** Because AMBE+2 produces ṽ_l
(per-harmonic) rather than ṽ_k (per-band), the Eq. 119 voicing check
becomes per-harmonic: for each FFT bin m, find the harmonic l it belongs
to (via `ã_l ≤ |m| < b̃_l`), then zero the bin if ṽ_l = 1. BABA-A §2.9
"uses the synthesizer verbatim" — the synthesizer's per-band loop
operates unchanged because the per-harmonic ṽ_l vector, viewed as a
per-bin classifier (via band assignment), is equivalent in structure to
the full-rate per-band ṽ_k.

---

## 8. Output Formation

```
s̃(n) = s̃_v(n) + s̃_uv(n)             for 0 ≤ n < 160               (BABA-A Eq. 142)
```

### 8.1 PCM Scaling

The AMBE-3000 chip emits 16-bit signed PCM at 8 kHz, Q15 format. Software
decoder output should be in the same scale. If the BABA-A synthesizer
produces values in range ~[−32768, 32767] natively (the M̃_l scale
implies this), no post-scaling is needed — clip to `int16_t`.

If intermediate computation is in unit-amplitude normalized form
(e.g., `M̄_l` scaled to [0, 1]), multiply by 32768 and clip before
emission. Verify against tv-std reference PCM.

### 8.2 De-emphasis / Post-Filter

None in the BABA-A pipeline. The AMBE-3000 chip does not apply additional
post-filtering to the decoded signal on the output side; any perceptual
post-filter is part of §4.3.6 enhancement, which this decoder already
applies. Do not add a post-emphasis or post-filtering stage.

### 8.3 Soft-Decision Input Path

If input bits arrive as soft-decision values (4-bit per-bit LLR, per
DVSI's PKT_CHAND soft-decision mode — see `AMBE-3000_Protocol_Spec.md`
§3 packet types), the FEC decoders (§3.3) should use soft-decision
Golay decoders rather than hard-decision. The parameter-recovery layer
and synthesizer are unchanged. Soft-decision input improves frame
validity rate under marginal RF conditions; see DVSI
"Soft_Decision_Error_Decoding.pdf" for the 4-bit encoding.

---

## 9. Error and State Management

### 9.1 Invalid Frame Handling

Triggers: §3.2.3 conditions (full-rate has analogous triggers, BABA-A
impl spec §1.11).

Action: frame repeat. Parameters for the current frame are copied forward
per BABA-A Eq. 200–205 (half-rate) or Eq. 99–104 (full-rate). Then:
- Enhancement (§4.3.6) runs on the copied `M̃_l` → produces `M̄_l(0)`
  that matches `M̄_l(−1)` by construction
- Phase regen (§5) computes `φ_regen,l` from the copied magnitudes
- Synthesis (§6–§7) runs normally

The synthesizer does **not** know the frame was repeated. This keeps the
auditory transition smooth; a listener hears the prior content continuing
for 20 ms rather than silence or noise.

### 9.2 Mute

Triggers: §3.2.4 conditions.

Action:
- Copy forward parameters per Eq. 200–205 (preserves all state for
  post-mute re-acquisition)
- **Bypass** synthesis — emit silence (`s̃(n) = 0`) or low-amplitude
  noise as per BABA-A §14.7
- Phase accumulators (ψ_l, φ_l) and noise buffer u(n) **continue to
  evolve** as if synthesis had run. This is critical: if the mute
  period is long and phase state freezes, the first frame after
  un-mute will have a discontinuity.

Implementation: run §6.5 phase-state update even when bypassing §6.1–§6.4
per-sample synthesis. Run the noise LCG §7 step 1 even when not emitting
unvoiced samples.

### 9.3 Cold-Start / Reset

Reset semantics should match what the AMBE-3000 chip does on a
`PKT_RESET` packet or hardware-reset — see `AMBE-3000_Operational_Notes.md`.
Software-decoder state initialization:
- All table §4.4 "Init" values loaded
- Repeat counter, consecutive-invalid counter → 0
- First frame processed as a normal frame; no special ramp-up

**Note:** the chip may require 2–3 frames of "warmup" before producing
bit-perfect output. If the software decoder is bit-exact to the chip
after frame 1, that is evidence of implementation bugs in the chip's
post-reset behavior rather than a software problem. Flag in the analysis
log if observed.

### 9.4 Silence / Erasure / Tone Frames

| `b̂₀` range | Meaning | Handler |
|-----------|---------|---------|
| [0, 119] | Normal voice frame | §4–§7 pipeline |
| [120, 123] | Erasure (force frame-repeat) | §3.2.3 → §9.1 |
| [124, 125] | Silence frame | emit 0 for 20 ms; preserve state per §9.2 |
| [126, 127] | Tone frame | BABA-A §2.10 tone pipeline → §6 MBE bridge |

Tone frames are parsed via BABA-A impl spec §2.10 (tone-frame layout,
Annex T lookup, MBE-parameter derivation). After the tone → `(ω̃₀, L̃,
ṽ_l, M̃_l)` conversion (Eq. 206–209), the standard §6–§7 synthesizer
runs. Phase regen (§5) operates normally on the tone-frame spectral
envelope (which is impulsive — one or two non-zero harmonics).

---

## 10. Rate Dispatch (r0..r63)

The AMBE-3000 chip supports 64 rate configurations via 6×16-bit Rate
Control Words. Each rate selects:
1. **Bit rate** and **frame size**
2. **Sub-frame count** (1 or 2 per transmission frame; 2 subframes → 160
   PCM out per subframe × 2 = 320 per transmission frame)
3. **FEC scheme** (which Golay/Hamming/Conv combination)
4. **Bit allocation** (which b̂_k widths sum to the info bits)
5. **Scrambling / PN** (on/off, seed derivation)
6. **Interleaver** (Annex S, Annex H, or rate-specific)

**Committed metadata:**
- `annex_tables/rate_index_table.csv` — r0..r63 with bit rate, frame
  size, sub-frame count, FEC scheme label
- `annex_tables/rate_control_words.csv` — packed RCW values per rate

### 10.1 P25 Rates

| Rate | Name | Info bits | Total bits | §4 pipeline |
|------|------|----------:|-----------:|------------|
| r33  | P25 full-rate (IMBE) | 88 | 144 | BABA-A §1, defer to BABA-A impl spec §1 |
| r34  | P25 half-rate (AMBE+2) | 49 | 72 | This spec §4, BABA-A §2 |

### 10.2 AMBE+2 Half-Rate Variants

Rates r35..r40 and several in r0..r32 use the same AMBE+2 half-rate
algorithm skeleton (§4.2 codebook-based V/UV, §4.3 PRBA+HOC) but with
different bit budgets, different FEC schemes, and in some cases 2
sub-frames per transmission frame. The synthesizer (§6–§7) is unchanged.
The per-rate bit allocation is **not normatively specified in this
spec** — it comes from:
- DVSI AMBE-3000F Manual Appendix (rate table summary)
- US6199037 (for 2-subframe joint quantization)
- US8595002 (for the single-subframe half-rate family)

A full bit-allocation matrix `rate_bit_allocations.csv` can be derived
by inspecting the rate-index × RCW-field encoding in the DVSI manual,
but this belongs to the rate-converter spec and is left as a TODO from
this decoder spec.

### 10.3 Non-AMBE+2 Rates

A subset of r0..r32 use different MBE variants (AMBE, AMBE+, IMBE) with
different quantizer tables and FEC. Algorithmic coverage of those rates
is outside the scope of this P25-focused decoder spec. The synthesizer
(§5–§7) is MBE-generic and will produce output given the `(ω̃₀, L̃, ṽ_l,
M̃_l)` tuple regardless of which rate produced it; the per-rate parameter
recovery needs to be specified per rate.

### 10.4 Dispatch Implementation

```c
typedef struct {
    uint8_t rate_index;        /* 0..63 */
    uint16_t bit_rate_bps;
    uint8_t frame_bits;        /* total channel bits per frame */
    uint8_t info_bits;         /* pre-FEC parameter bits */
    uint8_t subframes;         /* 1 or 2 */
    uint8_t fec_scheme;        /* enum */
    uint8_t interleaver;       /* enum: ANNEX_S, ANNEX_H, NONE, ... */
    uint8_t scrambling;        /* enum: PN_P25_HALF, NONE, ... */
    const ambe_bit_allocation_t *bit_allocation;
} ambe_rate_descriptor_t;

extern const ambe_rate_descriptor_t ambe_rate_table[64];

int ambe_decode_frame(uint8_t rate,
                      const uint8_t *frame,       /* frame_bits bits */
                      int16_t *pcm_out            /* 160 or 320 samples */);
```

The decoder dispatches on `ambe_rate_table[rate]`, runs the appropriate
§3 FEC, §4 recovery, then the shared §5–§7 synthesis. P25 rates
(r33, r34) are fully specified here + in BABA-A impl spec; non-P25
rates use the generic synthesizer with rate-specific recovery hooks.

---

## 11. Validation Plan

### 11.1 Test Vector Pipeline

```
  .bit file           ┌─────────────┐
  (rate r, 20ms    →  │  software   │  →  software .pcm
  frames per         │  decoder    │
  second)            └─────────────┘
                                            ┌───── compare ────→  metrics
  .bit file           ┌─────────────┐       │
  (same)          →   │  AMBE-3000  │  →  reference .pcm (tv-std/rN/)
                      │  chip       │
                      └─────────────┘
```

Test vectors in `/mnt/share/P25-IQ-Samples/DVSI Software/Docs/AMBE-3000_HDK_tv/`
(see `AMBE-3000_Test_Vector_Reference.md` for the full catalog).

### 11.2 Expected Match Quality by Stage

| Stage | Expected vs chip | Notes |
|-------|------------------|-------|
| §3 FEC decode (Golay, Hamming) | bit-exact | well-defined codes |
| §4.1 pitch inverse quant | bit-exact | deterministic lookup |
| §4.2 V/UV codebook expand | bit-exact | deterministic lookup |
| §4.3.2 gain recovery | bit-exact on valid frames | Annex O lookup |
| §4.3.3–4 PRBA/HOC VQ | bit-exact | Annex P/Q/R lookups |
| §4.3.5 IDCT + predictor | bit-exact | ρ = 0.65 literal per BABA-A Eq. 185 (half-rate) |
| §4.3.6 enhancement | bit-exact | BABA-A §1.10 |
| §5 phase regen | perceptual only | kernel values known; γ=0.44 may need empirical fit |
| §6 voiced synth | sample-near-exact | if phase matches, cos lookup equivalent |
| §7 unvoiced synth | **not bit-exact** | γ_w open issue, random-noise LCG matches but scaling TBD |
| §8 PCM output | target SNR ≥ 15 dB, MCD < 1.5 dB | on tv-std/r34 |

### 11.3 Recommended Test Order

1. **r34 silence frames** (tv-std/r34 silence inputs) — validates §3 FEC
   and frame-type dispatch (§9.4) without needing synthesis
2. **r34 single-harmonic tones** (tv-std/r34 DTMF) — validates §4
   parameter recovery, §5 phase on trivial spectra, §6 low-L synthesis
3. **r34 voiced speech** (tv-std/r34 'alert' test file) — end-to-end §4–§7
4. **r33 full-rate** (tv-std/r33) — validates the IMBE path shares the
   §5 regen and §6 synth with half-rate correctly
5. **r34 with induced bit errors** — validates §3.2.3, §3.2.4, §9.1–§9.2
6. **r34 with long mute periods** — validates §9.2 state preservation
7. **cmpp25.txt scenario** (DVSI test recipe) — full P25 integration

### 11.4 Debug Order on Mismatch

When software PCM doesn't match reference:

1. **Diff parameters first.** Dump `(ω̃₀, L̃, ṽ_l, M̃_l, M̄_l)` per frame
   for both software and a reference implementation (OP25, mbelib, JMBE).
   99% of PCM divergence is in §4, not §5–§7.
2. **Diff phase**. If parameters match, compute `φ_l(0)` per frame and
   verify against a reference implementation. Discrepancy → §5 kernel
   scaling or indexing bug.
3. **Diff synthesis**. Compute `s̃_v(n)` and `s̃_uv(n)` separately. If
   voiced matches but unvoiced doesn't → γ_w calibration (§7). If
   voiced doesn't match despite matching phase → §6 case-4b polynomial
   or wS windowing.
4. **Only then** suspect the chip. DVSI's chip is mature; software bugs
   are vastly more likely than chip behavior divergence.

### 11.5 Round-Trip Validation

Once the decoder converges against reference PCM:
- **Software encode → chip decode**: validates encoder (later spec)
- **Chip encode → software decode**: re-validates decoder on chip-sourced
  (not test-vector-sourced) bit streams. Guards against test vectors
  being synthetic rather than chip-originating.

---

## 12. Open Questions / Analysis TODOs

Candidates for `analysis/` entries. Each of these is an empirical
calibration or documentation gap where public sources fall short of
bit-exact replication.

1. **Phase regen scaling γ = 0.44.** Patent-stated value; may need
   empirical re-fit against DVSI reference. File: `analysis/ambe3000_phase_gamma.md`.
2. **Unvoiced γ_w calibration.** Inherited from BABA-A open issue. File:
   `standards/TIA-102.BABA-A/analysis/vocoder_decode_disambiguations.md`
   §11 — extend with AMBE+2-specific findings.
3. **Low-L̃ branch of phase regen.** Patent says "lowest quarter"
   (l ≤ ⌊L̃/4⌋) uses `ψ_l` unmodified, but the boundary is imprecise.
   Test whether `l ≤ L̃/4` vs `l < L̃/4` vs `l ≤ floor(L̃/4)+1` matches
   DVSI reference. File: `analysis/ambe_phase_quarter_boundary.md`.
4. **PN seed for non-P25 half-rate variants.** P25 uses `p_r(0) = 16·u0`.
   Other rates (r30, r31, ...) may use a different seed. Not relevant
   for the P25-focused validation in §11, but flagged for the
   rate-converter spec. File: `analysis/ambe_pn_seed_per_rate.md`.
5. **Log-magnitude floor in phase regen (§5.4).** `log2(M̄_l = 0)`
   behavior: floor at e.g. `log2(1e-10)` vs treat-as-zero vs skip.
   Resolve against test vectors.

Previously listed here and now resolved:
- ~~BABA-A half-rate predictor ρ value~~ — resolved via
  `analysis/vocoder_decode_disambiguations.md` §3:
  literal 0.65 per BABA-A Eq. 185 on page 65 (earlier draft cited
  Eq. 200, which is actually the first of the frame-repeat copy-forward
  equations 200–205, not the predictor). Matches the US7634399
  rate-converter numeric value; distinct predictor-state instance.
  See §4.3.5 above.

---

## 13. Appendices

### Appendix A — C Reference Fragments (Consolidated)

```c
/* =========================================================================
 *  AMBE-3000 decoder reference — non-normative C skeleton.
 *  Use alongside the normative sections above.
 * ========================================================================= */

#include <math.h>
#include <stdint.h>

/* --- §3.3 Golay decoders (shared with BABA-A §1.5.1) ------------------ */
int golay_23_12_decode(uint32_t cw, uint16_t *info);
int golay_24_12_decode(uint32_t cw, uint16_t *info);

/* --- §3.2.1 half-rate PN modulation ------------------------------------ */
void halfrate_pn_init(uint16_t b0_pitch, uint32_t p_r[24]);
uint32_t halfrate_pn_modulation(const uint32_t p_r[24]);

/* --- §4.1.2 pitch inverse quant (Annex L lookup) ---------------------- */
typedef struct { double f0; double omega_0; uint8_t L; } annex_l_row_t;
extern const annex_l_row_t annex_l_table[120];
/* Usage: if (b0 < 120) { row = &annex_l_table[b0]; ...; } */

/* --- §4.2.2 V/UV codebook (Annex M lookup + expansion) ---------------- */
extern const uint8_t ambe_vuv_codebook[32][8];
void halfrate_vuv_expand(uint8_t b1, double omega_0, uint8_t L,
                         uint8_t v_harm[/* L */]);

/* --- §4.3 spectral magnitude recovery --------------------------------- */
void ambe_gain_recover(uint8_t b2, double *gamma, double gamma_prev);
void ambe_prba_recover(uint8_t b3, uint8_t b4, double G[9]);
void ambe_hoc_recover(uint8_t b5, uint8_t b6, uint8_t b7, uint8_t b8,
                      double H[4][4]);
void ambe_idct_assemble(const double G[9], const double H[4][4],
                        uint8_t L, double T[/* L */]);
/* Log-magnitude → linear + predictor application:
 *   log2(M_tilde[l]) = T[l] + rho * log2(M_tilde_prev[l])
 *   M_tilde[l] = pow(2, log2(M_tilde[l]));                              */
void ambe_mag_reconstruct(const double T[/* L */], uint8_t L,
                          const double M_prev[/* L_prev */], uint8_t L_prev,
                          double rho, double M_out[/* L */]);

/* --- §5 phase regeneration -------------------------------------------- */
#define PHASE_KERNEL_D 19
void ambe_phase_regen(const double M_bar[/* L */], int L,
                      double phi[/* L */]);

/* --- §6–§7 synthesis (delegated to BABA-A §1.12) ---------------------- */
void mbe_synthesize_voiced(const double omega_0[2],       /* [prev, curr] */
                           const uint8_t L[2],
                           const double M_bar[2][/* L */],
                           const uint8_t v_harm[2][/* L */],
                           const double phi[2][/* L */],
                           double s_v[160]);

void mbe_synthesize_unvoiced(double omega_0_curr, uint8_t L_curr,
                             const double M_bar[/* L */],
                             const uint8_t v_harm[/* L */],
                             double s_uv[160]);
```

### Appendix B — Test Vector Coverage Matrix

See `AMBE-3000_Test_Vector_Reference.md` for the full per-rate test vector
catalog. This spec's validation targets (§11.3) are summarized:

| Test vector | Rate | Exercises | Coverage priority |
|-------------|------|-----------|-------------------|
| `tv-std/r34/*silence*` | r34 | §3, §9.4 | 1 |
| `tv-std/r34/*tone*` | r34 | §4, §5, §6 low-L | 2 |
| `tv-std/r34/alert*` | r34 | full §4–§7 voice path | 3 |
| `tv-std/r33/*` | r33 | §5–§7 shared core | 4 |
| `tv-std/r34/*` (error-injected) | r34 | §3.2.3, §9.1 | 5 |
| `cmpp25.txt` scenario | r33 + P25 FEC | P25 integration | 7 |

### Appendix C — Cross-Reference Quick Index

| Topic | This spec | BABA-A impl spec | Patent |
|-------|-----------|------------------|--------|
| Full-rate frame format | §2.1 | §1.2 | — |
| Half-rate frame format | §2.2 | §2.2 | US8595002 Table 1 |
| Full-rate FEC | §3.1 (ref only) | §1.5–§1.7 | — |
| Half-rate FEC | §3.2 | §2.4–§2.6 | US8595002 col. 23 |
| Pitch quantization | §4.1 | §1.3.1 / §2.2 | US8595002 col. 17 |
| V/UV codebook | §4.2 | §2.3.6 | US8595002 col. 18 |
| Spectral mag recovery | §4.3 | §1.8 / §2.11–§2.13 | US8595002 col. 20–22 |
| Enhancement | §4.3.6 (ref) | §1.10 / §2.7 | — |
| Phase regeneration | §5 | —  | US5701390 Eq. 8–9 |
| Voiced synthesis | §6 | §1.12.2 | US5701390 Eq. 13–17 |
| Unvoiced synthesis | §7 (ref) | §1.12.1 | US5701390 col. 9 |
| Frame repeat / mute | §9 | §1.11, §2.8 | US8595002 col. 26 |
| Rate dispatch | §10 | — | DVSI manual Appendix |
