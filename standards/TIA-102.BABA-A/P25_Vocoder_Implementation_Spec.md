# P25 Vocoder Frame Structure and Bit Ordering -- Implementation Specification

**Source:** TIA-102.BABA-A (Revision A, February 2014)  
**Classification:** VOCODER  
**Phase:** 3 -- Implementation-ready  
**Extracted:** 2026-04-12  
**Purpose:** Self-contained spec for extracting vocoder parameters from P25 air interface
frames. Covers frame structure, bit ordering, FEC, and MBE parameter reconstruction for
both full-rate IMBE (Phase 1 FDMA) and half-rate AMBE+2 (Phase 2 TDMA).

**Cross-references:**
- TIA-102.BABA-A -- IMBE Vocoder and Half-Rate Vocoder (primary source for this spec)
- TIA-102.BABA-1 -- Half-Rate Vocoder Annex (original half-rate addendum)
- TIA-102.BAAA-B -- FDMA Common Air Interface: LDU1/LDU2 voice frame positions
  (see `P25_FDMA_Common_Air_Interface_Implementation_Spec.md` Section 6)
- TIA-102.BBAC-A -- TDMA MAC Layer: 4V/2V burst voice bit positions
  (see `P25_TDMA_MAC_Layer_Implementation_Spec.md`)
- TIA-102.BBAC-1 Annex E -- Burst bit location tables for TDMA voice
  (see `P25_TDMA_Annex_E_Burst_Bit_Tables.md`)

**NOTE ON FRAME FORMAT VS CODEC:** The IMBE frame format is a serialization container
for MBE model parameters — it is not a vocoder. After unpacking the frame and recovering
the MBE parameters (ω₀, V/UV decisions, spectral amplitudes), any MBE-compatible
synthesis engine can reconstruct speech. See `analysis/vocoder_wire_vs_codec.md` for
a detailed discussion of this distinction. The synthesis algorithms described in
TIA-102.BABA-A (Sections 8, 9, 15) define a baseline approach; improved synthesis
(e.g., AMBE+2-class) produces better audio from the same parameters.

**Existing open-source implementations** (JMBE, OP25, DSD/mbelib, SDRTrunk) can be
referenced as working examples of this spec, but this document is derived from the
TIA-102.BABA-A standard and should be self-sufficient for implementation.

---

## 0. Analysis Encoder: PCM → MbeParams (Addendum Pointer)

**Source:** TIA-102.BABA-A §§3–6 (PDF pages ~5–22).

The forward path — PCM input through initial pitch estimation, pitch
refinement, spectral amplitude estimation, V/UV decision, log-magnitude
prediction residual, and frame-type dispatch into `MbeParams` ready for
the wire-side quantizer — is rendered as a separate derived work at

> [`analysis/vocoder_analysis_encoder_addendum.md`](../../analysis/vocoder_analysis_encoder_addendum.md)

That file is §0 for the purposes of this implementation spec: subsections
§0.1 through §0.11 match the structure the gap report
(`analysis/vocoder_analysis_encoder_gap.md`) originally proposed and
together cover the complete PCM → MbeParams pipeline.

| Addendum | BABA-A source | Topic |
|---|---|---|
| §0.1 | §3, §4, §5.1, Figure 7/8 | Input framing, HPF (Eq. 3), analysis windows `w_I` / `w_R` |
| §0.2 | §5.1.5 (Eq. 24–30) | 256-point DFT and `S_w(m, ω₀)` basis function |
| §0.3 | §5.1 (Eq. 4–23), Annex D | Initial pitch estimation, `E(P)` and look-back / look-ahead tracking |
| §0.4 | §5.1.5 (Eq. 24, 31–33, 45) | Pitch refinement and pitch quantization |
| §0.5 | §5.3 (Eq. 43, 44) | Spectral amplitude estimation `M̂_l` |
| §0.6 | §6.4 (Eq. 52–57) | Log-magnitude prediction residual `T̂_l` and closed-loop predictor |
| §0.7 | §5.2 (Eq. 34–42) | V/UV determination, `D_k`, `ξ`-family metrics, `Θ_ξ` threshold |
| §0.8 | §6.1, §13.1 (+ §16) | Frame-type dispatch (voice / silence / tone / erasure) |
| §0.9 | Annex A (ext.) | Encoder state structure and Annex-A-style initialization |
| §0.10 | synthesis | End-to-end reference C pipeline |
| §0.11 | informative | Numerical cross-checks against DVSI test vectors |

### 0.0.1 Items not prescribed by the standard

Per BABA-A, the encoder's *decision* layer (as opposed to its *analysis*
layer) is left to the implementer:

- **Silence-frame entry criteria** — the standard specifies the silence
  payload (`b̂_0 = 124` half-rate) but not the energy threshold or
  hysteresis that triggers it. See addendum §0.8.4 for an implementation
  baseline and the companion gap report for chip-probe requirements.
- **Tone-frame entry criteria** — the standard specifies the Annex T
  tone payload but not how the encoder decides "this is a tone." See
  addendum §0.8.5.
- **Pitch `E(P)` grid-max on silence** — the `E(P)` formula (Eq. 5–7) is
  well-defined, but in the absence of signal it degenerates to a flat
  surface. The addendum notes this in §0.3.9; in practice it is handled
  by the silence-dispatch layer (§0.8.4), not by modifying `E(P)`.

These are explicitly called out as "NOT in the PDF" in the corresponding
addendum subsections. Implementers should calibrate thresholds against
DVSI observed behavior rather than the literature.

### 0.0.2 Absolute-amplitude residual (open investigation)

A cross-cutting investigation thread — independent of the decoder-side
`γ_w` localization in §11 below — concerns the absolute scale of `M̂_l`
out of Eq. 43/44 versus DVSI chip-observed amplitudes. Addendum §0.5.6
and §0.2.8 record the current status. Resolving it requires DVSI probe
data, not further spec work.

### 0.0.3 Corrections applied

Two transcription errors were found and corrected during the addendum
drafting pass; both are documented in-line in the addendum header and
in dedicated correction notes:

- **Eq. 7 `r(t)`** — see
  [`analysis/vocoder_analysis_eq7_correction.md`](../../analysis/vocoder_analysis_eq7_correction.md).
  The original draft duplicated the squared-window onto `s_LPF`; the
  PDF form has `s_LPF` to the first power. Corrected in addendum
  §0.3.2, §0.3.3, §0.3.8, §0.3.9.
- **Eq. 43/44 `M̂_l`** — the original gap report §6 paraphrased the
  spectral amplitude estimator as `â_l + j·b̂_l` (complex). The PDF
  instead uses magnitude-only Eq. 43 (voiced) and Eq. 44 (unvoiced);
  Eq. 32–33 are per-harmonic bin endpoints, not spectral-amplitude
  projections. Corrected in addendum §0.5 and acknowledged in the
  gap report.
- **Full-rate `ρ` piecewise** — `ρ` in Eq. 54/77 is defined piecewise
  in `L̂` by Eq. 55 (0.4 / `0.03·L̂ − 0.05` / 0.7), not the constant
  0.65 that appears in half-rate Eq. 155/185. Corrected in addendum
  header and §0.6.2; companion correction sweep in
  `vocoder_decode_disambiguations.md` §3 and this file's §1.8.5 was
  applied upstream (commits `7e35238`, `20d7ca4`).
- **Eq. 31 `L̂` double floor** — see
  [`analysis/vocoder_analysis_eq31_correction.md`](../../analysis/vocoder_analysis_eq31_correction.md).
  The addendum's original §0.4.3 transcribed Eq. 31 with a single
  outer floor (`⌊ 0.9254 · (π/ω̂_0 + 0.25) ⌋`). The PDF form has a
  nested inner floor (`⌊ 0.9254 · ⌊ π/ω̂_0 + 0.25 ⌋ ⌋`), matching the
  decoder-side Eq. 47 already rendered correctly at §1.2 / §1.7 of
  this file. Impact: ≈2.5% of admissible `ω̂_0` get `L̂_addendum =
  L̂_PDF + 1`. Corrected in addendum §0.4.3 and §0.4.6.

---

## 1. IMBE Full-Rate Vocoder (Phase 1 FDMA)

### 1.1 Fundamental Parameters

| Parameter | Value |
|-----------|-------|
| Bit rate (total) | 7,200 bps |
| Voice parameter bits | 88 per frame |
| FEC bits | 56 per frame |
| Total bits per frame | 144 |
| Frame duration | 20 ms |
| Frames per second | 50 |
| Sample rate | 8,000 Hz |
| Samples per frame | 160 |
| Pitch range | 21-122 samples (65-381 Hz) |
| Harmonics L | 9 to 56 |
| V/UV bands K | up to 12 |
| Analysis/synthesis delay | 80 ms total |

### 1.2 The 88 Information Bits -- Bit Vector Structure

The 88 voice parameter bits are organized into 8 bit vectors u0 through u7, ordered
by significance (u0 = most significant, u7 = least significant). These vectors carry
the quantized speech model parameters.

| Vector | Information Bits | FEC Code | Encoded Bits | Contents |
|--------|-----------------|----------|-------------|----------|
| u0 | 12 | [23,12] Golay | 23 | Pitch (b0, 8 bits) + MSBs of spectral params |
| u1 | 12 | [23,12] Golay | 23 | Spectral amplitude MSBs |
| u2 | 12 | [23,12] Golay | 23 | Spectral amplitude bits |
| u3 | 12 | [23,12] Golay | 23 | Spectral amplitude bits |
| u4 | 11 | [15,11] Hamming | 15 | Spectral amplitude bits |
| u5 | 11 | [15,11] Hamming | 15 | Spectral amplitude bits |
| u6 | 11 | [15,11] Hamming | 15 | Spectral amplitude bits |
| u7 | 7 | None (uncoded) | 7 | Least significant spectral bits |
| **Total** | **88** | | **144** | |

**Note:** The table in TIA-102.BAAA-B Section 6.1 shows a slightly different grouping
(c0-c7 with 12,12,12,12,11,11,10,8 = 88 bits) reflecting the code word boundaries after
bit prioritization. The exact bit counts per vector depend on L (number of harmonics).
The mapping between voice parameters b0..b_{L+2} and the 8 vectors is defined by
the bit prioritization algorithm (BABA-A Section 10).

### 1.3 Voice Parameter Breakdown

The 88 bits encode these speech model parameters:

| Parameter | Symbol | Bits | Range | Description |
|-----------|--------|------|-------|-------------|
| Fundamental frequency | b0 | 8 | 0-255 | 256-level pitch quantizer, maps to omega_0 |
| V/UV decisions | v_k | K (variable) | binary per band | Voiced/unvoiced for each of K frequency bands |
| Gain (DCT block averages) | b2 | 6 | 0-63 | 64-level gain quantizer (Annex E) |
| Gain DCT coefficients | b3..b7 | variable | per L | Bit allocation from Annex F, depends on L |
| Higher-order DCT coefficients | -- | variable | per L | Bit allocation from Annex G, depends on L |

Total bits across all parameters = 88 for every valid L value. The bit allocation
tables (Annexes F and G) ensure the budget sums to 88 regardless of L.

#### 1.3.1 Pitch Index ↔ Fundamental Frequency Mapping

Source: BABA-A Section 6.1 Fundamental Frequency Encoding and Decoding,
pages 21–22 (Eqs. 45–48) and Section 5.1.5 (Eq. 31 for L̂ from ω̂₀).

The full-rate pitch quantizer is analytical, not a lookup table. Eight bits of
b̂₀ (unsigned, 0–255) represent ω̂₀ at half-sample pitch resolution.

**Encode (Eq. 45):**
```
b̂₀ = floor(4π / ω̂₀ − 39)
```

**Decode (Eq. 46):**
```
ω̃₀ = 4π / (b̃₀ + 39.5)
```

**Number of harmonics L̃ (Eq. 47, same form as Eq. 31):**
```
L̃ = floor(0.9254 · floor(π / ω̃₀ + 0.25))
```
Note the double-floor: the inner floor rounds π/ω̃₀ to the nearest integer with
a 0.25 offset before scaling. Constrained to 9 ≤ L̃ ≤ 56.

**Number of V/UV bands K̃ (Eq. 48):**
```
K̃ = floor((L̃ + 2) / 3)   if L̃ ≤ 36
K̃ = 12                    otherwise
```

**Valid range and reserved values:** Per §6.1, the pitch estimator restricts ω̂₀
to [2π/123.125, 2π/19.875] rad/sample, which confines b̂₀ to **0 ≤ b̂₀ ≤ 207**.
The 48 values 208–255 are **reserved for future use** and should not be
transmitted. A decoder receiving b̃₀ ∈ [208, 255] has encountered either
uncorrectable FEC errors or a non-conformant encoder; handle as a bad frame.

At 8 kHz sample rate, this range corresponds to fundamental frequencies of
roughly 65 Hz (b̂₀ = 207) to 405 Hz (b̂₀ = 0).

**Robustness note:** K̃ and L̃ control all subsequent bit allocation at the
receiver, so they must equal K̂ and L̂ exactly. This is why the six MSBs of b̂₀
are placed in û₀ (protected by the strongest [23,12] Golay FEC) — see §1.4.

```c
#include <math.h>
#include <stdint.h>

/* Encode a refined fundamental frequency omega_0 (rad/sample) into the
 * 8-bit pitch index b_0. Caller must ensure omega_0 is within the pitch
 * estimator's range (2*pi/123.125 to 2*pi/19.875). */
uint8_t imbe_pitch_encode(double omega_0) {
    int32_t b0 = (int32_t)floor((4.0 * M_PI / omega_0) - 39.0);
    /* assert(b0 >= 0 && b0 <= 207); */
    return (uint8_t)b0;
}

/* Decode the 8-bit pitch index b_0 to the reconstructed fundamental
 * frequency omega_0 (rad/sample). Returns 0.0 for reserved values (208-255);
 * caller should treat reserved b_0 as an uncorrectable frame. */
double imbe_pitch_decode(uint8_t b0) {
    if (b0 > 207) return 0.0;                    /* reserved */
    return 4.0 * M_PI / ((double)b0 + 39.5);
}

/* Compute the number of harmonics L from omega_0, per Eq. 31/47.
 * Result is always in 9..=56 for valid omega_0. */
uint8_t imbe_harmonics_count(double omega_0) {
    double inner = floor(M_PI / omega_0 + 0.25);
    return (uint8_t)floor(0.9254 * inner);
}

/* Compute the number of V/UV bands K from L, per Eq. 48. */
uint8_t imbe_vuv_band_count(uint8_t L) {
    return (L <= 36) ? (uint8_t)((L + 2) / 3) : 12;
}
```

#### 1.3.2 V/UV Band-to-Harmonic Mapping

Source: BABA-A §5.2 (encoder band definition, pages 17–18) and §6.2
(decoder reconstruction, page 23). The encoder computes one V/UV bit v̂_k
per frequency band k ∈ [1, K̂], but the MBE synthesizer needs a per-harmonic
decision ṽ_l for l ∈ [1, L̃]. The expansion rule:

```
k_l = floor((l + 2) / 3)    if l ≤ 36
k_l = 12                    otherwise                   (same form as Eq. 34/48)
ṽ_l = v̂_{k_l}             for 1 ≤ l ≤ L̃
```

i.e. all harmonics in band k inherit that band's voiced/unvoiced decision.
In full-rate, bands 1..K̂−1 each cover exactly 3 harmonics (band k covers
l = 3k−2, 3k−1, 3k per PDF Eq. 32/33 with `â_{3k-2} ≤ m < b̂_{3k}`); the
highest band K̂ absorbs any remainder (harmonics 3K̂−2 through L̃).

Worked cases:

| L̃ | K̂ | Band 1 | Band 2 | … | Band K̂ (highest) |
|---:|---:|--------|--------|---|-------------------|
| 9  | 3  | l=1..3 | l=4..6 |   | l=7..9            |
| 16 | 6  | l=1..3 | l=4..6 | … | l=16..16  (1 harmonic) — no, let me recompute |

Actually for L̃=16, K̂=floor(18/3)=6, band 6 covers l=16 only (16..16 = 1 harmonic)? Let me re-derive: bands 1..K̂-1 = 1..5 cover l=1..15 (5 bands × 3 harmonics). Band K̂=6 absorbs l=16..L̃=16, i.e. 1 harmonic. ✓

For L̃=37, K̂=12: bands 1..11 cover l=1..33, band 12 covers l=34..37 (4 harmonics).
For L̃=56, K̂=12: bands 1..11 cover l=1..33, band 12 covers l=34..56 (23 harmonics).

```c
/* Expand full-rate per-band V/UV decisions into per-harmonic decisions.
 * v_band: K̂ entries (from b̂₁, MSB-first as extracted in §1.4.2).
 * v_harm: L̃ entries written. */
void imbe_vuv_expand(uint8_t L, uint8_t K, const uint8_t v_band[/* K */],
                    uint8_t v_harm[/* L */])
{
    for (uint8_t l = 1; l <= L; l++) {
        uint8_t k = (l <= 36) ? (uint8_t)((l + 2) / 3) : 12;
        if (k > K) k = K;                       /* clamp to highest band */
        v_harm[l - 1] = v_band[k - 1];
    }
}
```

**Known extraction bug:** `TIA-102-BABA-A_Full_Text.md` §4.1 paraphrases
Eq. 34 as `K̂ = floor(2ω̂₀/π × 56), K̂_max = 12`. That's a Phase 2 extraction
error — the PDF Eq. 34 is actually `floor((L̂+2)/3) if L̂ ≤ 36 else 12`,
identical to §1.3.1 Eq. 48 above. The impl spec and the formulas in this
subsection are authoritative; the Full_Text paraphrase is wrong and will be
corrected the next time that file is regenerated.

### 1.4 Bit Prioritization Scan Algorithm

Source: BABA-A Section 7.1 "Bit Prioritization", pages 33–35 (prose only —
Figures 22, 23, 24 are illustrative, not normative).

This section maps the 88 voice-parameter information bits of one full-rate
IMBE frame into the 8 bit vectors û₀..û₇ in *significance order*. û₀ receives
the most perceptually important bits (protected by the strongest Golay FEC),
û₇ receives the least important (uncoded). Vector widths:

```
û₀..û₃ = 12 bits each    (total 48, all FEC-protected)
û₄..û₆ = 11 bits each    (total 33, all FEC-protected)
û₇     =  7 bits         (uncoded)
Grand total             =  88 information bits
```

Within each vector, bit (N−1) is the MSB and bit 0 is the LSB (N = vector width).

#### 1.4.1 Priority Scan of Spectral Amplitudes b̂₃..b̂_{L̂+1} (Figure 22)

The scan that threads through û₀..û₇ uses the spectral-amplitude quantizer
values b̂₃, b̂₄, …, b̂_{L̂+1} (the transformed-gain DCT coefficients from
Annex F for indices 3..7, and the higher-order DCT coefficients from Annex G
for indices 8..L̂+1). Each b̂_l has B_l allocated bits (5..10 for Annex F,
0..3 for Annex G; zero-bit coefficients are effectively absent from the scan).

Arrange b̂₃..b̂_{L̂+1} as columns in Figure 22, with the MSB of each column at
the top. The scan visits every allocated bit in the following order:

```
for bit_position from (max_B − 1) down to 0:           # MSB plane first
    for l from 3 to L̂ + 1:                            # coefficient index
        if B_l > bit_position:                          # this bit is allocated
            emit (l, bit_position)
```

i.e. MSB plane of all coefficients (left-to-right), then next-MSB plane, and
so on. This yields a total of Σ B_l bits, equal to **73 − K̃** (derived
below; K̃ is the V/UV band count from §1.3.1, Eq. 48).

#### 1.4.2 Placement Order

The emission sequence from §1.4.1, together with the fixed assignments below,
fills û₀..û₇ in the following order (each line below consumes bits strictly
in the order shown; "scan" means "next bit from the §1.4.1 sequence"):

```
û₀[11..6]  = b̂₀[7..2]            # 6 MSBs of pitch
û₀[5..3]   = b̂₂[5..3]            # 3 MSBs of gain
û₀[2..0]   = 3 scan bits          # first 3 bits of the §1.4.1 scan
û₁[11..0]  = 12 scan bits
û₂[11..0]  = 12 scan bits
û₃[11..0]  = 12 scan bits          # end of highest-priority section (48 bits)
──────────────────────────────────
û₄[10..0]  ─┐
û₅[10..0]  ─┤ 36 bits total, filled in the following strict order:
û₆[10..0]  ─┤
û₇[6..4]   ─┤  1. all K̃ bits of b̂₁ (V/UV), MSB-first
           ─┤  2. b̂₂[2], then b̂₂[1]  (2 more gain bits)
           ─┘  3. remaining bits from the §1.4.1 scan
──────────────────────────────────
û₇[3]      = b̂₂[0]                # LSB of gain
û₇[2]      = b̂₀[1]                # bit 1 of pitch
û₇[1]      = b̂₀[0]                # LSB of pitch
û₇[0]      = b̂_{L̂+2}[0]          # LSB of the final spectral amplitude
```

The scan from §1.4.1 produces exactly **(3) + (36) + (36 − K̃ − 2) = 73 − K̃**
bits, which must equal Σ (B_l for l = 3..L̂+1) for any valid (L̂, K̃) pair.
This is a useful invariant for testing: if your Annex F + Annex G bit sums
don't satisfy it, something is mis-looked-up.

#### 1.4.3 Full-Rate Bit Budget Identity

```
88  =  8           (pitch b̂₀, all bits used)
    +  K̃          (V/UV b̂₁, all K̃ bits used)
    +  6           (gain b̂₂, all 6 bits used)
    +  (73 − K̃)   (spectral amplitudes b̂₃..b̂_{L̂+1} via scan)
    +  1           (final spectral LSB b̂_{L̂+2}[0])
```

An implementation should assert this equality at table-load time. K̃ is given
by Eq. 48 (see §1.3.1): K̃ = ⌊(L̃+2)/3⌋ if L̃ ≤ 36, else 12.

#### 1.4.4 Pre-Computed CSV

For implementers who prefer a table lookup over running the scan algorithm
at load time, the file
[`annex_tables/imbe_bit_prioritization.csv`](annex_tables/imbe_bit_prioritization.csv)
contains the fully-expanded `(L, src_param, src_bit) → (dst_vec, dst_bit)`
mapping for every L ∈ [9, 56]. **4224 rows = 48 L values × 88 bits per frame.**

All three invariants above (88-bit budget, src coverage, dst coverage) were
verified for every L during CSV generation (the generator script aborts on
invariant failure). The CSV is therefore authoritative — if the scan
algorithm in §1.4.1/§1.4.2 appears to disagree with the CSV, the CSV is
right and the algorithm text is buggy. Encoders and decoders can index the
CSV by `(L, src_param, src_bit)` to pack, or by `(L, dst_vec, dst_bit)` to
unpack.

#### 1.4.5 Decode Pseudocode

The prioritization is invertible. Decoder:

```c
/* Inverse of §1.4: unpack parameters b̂_l from bit vectors û_0..û_7. */
void imbe_unpack(const uint16_t u[8], int L, int K, int B[],
                 uint8_t *b0, uint16_t *b1, uint8_t *b2,
                 uint32_t b_spec[/* L-1 */]);
/* B[] has length L+2, indexed by l (with B[0]=8, B[1]=K, B[2]=6,
 * B[3..7] from Annex F for this L, B[8..L+1] from Annex G for this L,
 * and B[L+2]=1 implicitly). */

/* Step 1: pitch (must be decoded first to compute L and K):
 *   b̂₀[7..2] = û₀[11..6];   b̂₀[1..0] = û₇[2..1]
 * Step 2: omega_0 = 4π / (b̂₀ + 39.5)          (Eq. 46)
 *         L = floor(0.9254 * floor(π/omega_0 + 0.25))   (Eq. 31/47)
 *         K = (L <= 36) ? (L + 2) / 3 : 12               (Eq. 48)
 * Step 3: load Annex F (for l=3..7) and Annex G (for l=8..L+1) → B[l].
 * Step 4: gain b̂₂:
 *   b̂₂[5..3] = û₀[5..3];    b̂₂[2..1] and b̂₂[0] come out of the §1.4.2
 *   placement order (second block for [2..1]; fixed slot for [0]).
 * Step 5: run §1.4.1 scan forward to build the ordered list of (l, bitpos)
 *         emissions, then read bits back from û₀..û₇ in §1.4.2 placement
 *         order into those (l, bitpos) slots. All 73-K scan bits should
 *         land. */
```

> **Implementation tip:** build the scan as a vector of (l, bitpos) tuples
> once per (L, K, B[]) tuple and cache it. The placement order is identical
> for encode and decode — encode reads from b̂_l and writes to û_i; decode
> does the reverse. Keep the two as one `enum { ENCODE, DECODE }` function
> to guarantee they cannot drift.

### 1.5 FEC Encoding -- Full-Rate

#### 1.5.1 [23,12] Golay Code

Applied to u0, u1, u2, u3. Corrects up to 3 bit errors per code word.

Source: BABA-A Section 7.3, page 37 (Equation 81, matrix below Figure 24).
Generator polynomial: g(x) = x¹¹ + x⁹ + x⁷ + x⁶ + x⁵ + x + 1 (octal 6265, 0xAE3).

Generator matrix g_G (12 x 23) -- systematic form [I_12 | P].
Parity columns ordered leftmost = x⁰, rightmost = x¹⁰.

```
Parity portion P (12 x 11):
Row 0:  1 1 0 0 0 1 1 1 0 1 0
Row 1:  0 1 1 0 0 0 1 1 1 0 1
Row 2:  1 1 1 1 0 1 1 0 1 0 0
Row 3:  0 1 1 1 1 0 1 1 0 1 0
Row 4:  0 0 1 1 1 1 0 1 1 0 1
Row 5:  1 1 0 1 1 0 0 1 1 0 0
Row 6:  0 1 1 0 1 1 0 0 1 1 0
Row 7:  0 0 1 1 0 1 1 0 0 1 1
Row 8:  1 1 0 1 1 1 0 0 0 1 1
Row 9:  1 0 1 0 1 0 0 1 0 1 1
Row 10: 1 0 0 1 0 0 1 1 1 1 1
Row 11: 1 0 0 0 1 1 1 0 1 0 1
```

```c
/* [23,12] Golay generator matrix (systematic form).
 * Each row is 23 bits: 12 identity bits (columns 22..11) followed by 11 parity
 * bits (columns 10..0). Stored as uint32_t with bit 22 = identity column 0,
 * bit 0 = parity x^10. Binary layout shown in the comments for reference. */
static const uint32_t GOLAY_23_12_GEN[12] = {
    0x40063Au,  /* Row 0:  100000000000_11000111010 */
    0x20031Du,  /* Row 1:  010000000000_01100011101 */
    0x1007B4u,  /* Row 2:  001000000000_11110110100 */
    0x0803DAu,  /* Row 3:  000100000000_01111011010 */
    0x0401EDu,  /* Row 4:  000010000000_00111101101 */
    0x0206CCu,  /* Row 5:  000001000000_11011001100 */
    0x010366u,  /* Row 6:  000000100000_01101100110 */
    0x0081B3u,  /* Row 7:  000000010000_00110110011 */
    0x0046E3u,  /* Row 8:  000000001000_11011100011 */
    0x00254Bu,  /* Row 9:  000000000100_10101001011 */
    0x00149Fu,  /* Row 10: 000000000010_10010011111 */
    0x000C75u,  /* Row 11: 000000000001_10001110101 */
};

/* Golay decode: correct up to 3 errors in a 23-bit code word.
 * On success, writes the 12 corrected info bits to *info_bits and returns
 * the number of bits corrected (0..3). Returns -1 if uncorrectable. */
int golay_23_12_decode(uint32_t codeword, uint16_t *info_bits);
/* Reference implementation: compute the 11-bit syndrome S = codeword * H^T,
 * look up S in a 2048-entry syndrome table to obtain the error pattern E,
 * XOR E into the codeword, and extract the upper 12 bits. If weight(E) > 3
 * the codeword is uncorrectable. */
```

#### 1.5.2 [15,11] Hamming Code

Applied to u4, u5, u6. Corrects 1 bit error per code word.

Generator matrix g_H (11 x 15) -- systematic form [I_11 | P]:

```
Parity portion P (11 x 4):
Row 0:  1 1 1 1
Row 1:  1 1 1 0
Row 2:  1 1 0 1
Row 3:  1 1 0 0
Row 4:  1 0 1 1
Row 5:  1 0 1 0
Row 6:  1 0 0 1
Row 7:  0 1 1 1
Row 8:  0 1 1 0
Row 9:  0 1 0 1
Row 10: 0 0 1 1
```

```c
/* [15,11] Hamming generator matrix parity columns.
 * For each of 11 info bits, the 4 parity bits to append. */
static const uint8_t HAMMING_15_11_PARITY[11] = {
    0xF,  /* row 0:  1111 */
    0xE,  /* row 1:  1110 */
    0xD,  /* row 2:  1101 */
    0xC,  /* row 3:  1100 */
    0xB,  /* row 4:  1011 */
    0xA,  /* row 5:  1010 */
    0x9,  /* row 6:  1001 */
    0x7,  /* row 7:  0111 */
    0x6,  /* row 8:  0110 */
    0x5,  /* row 9:  0101 */
    0x3,  /* row 10: 0011 */
};

/* Hamming decode: correct up to 1 error in a 15-bit code word.
 * On success, writes the 11 info bits to *info_bits and returns the number
 * of bits corrected (0 or 1). Returns -1 if the syndrome indicates an
 * uncorrectable pattern. */
int hamming_15_11_decode(uint16_t codeword, uint16_t *info_bits);
```

#### 1.5.3 u7 -- Uncoded

u7 (7 bits) is transmitted without FEC protection. These are the least significant
spectral amplitude bits.

### 1.6 PN Sequence Modulation

Source: BABA-A Section 7.4 Bit Modulation, pages 37–38, Equations 84–94.

After FEC encoding, each code vector v̂_i is XORed with a pseudo-noise mask m̂_i
derived from the 12-bit Golay info word û₀. Vector v̂₀ is deliberately left
unmodulated (m̂₀ = 0) so the decoder can Golay-decode c̃₀ first and reconstruct
û₀ before demodulating the other vectors. Vector v̂₇ is also unmodulated
(m̂₇ = 0) since it is uncoded pass-through.

**PN sequence generator (Eq. 84–85) — a linear congruential generator mod 65536:**

```
p_r(0) = 16 · û₀                                  where û₀ ∈ [0, 4095]
p_r(n) = (173 · p_r(n-1) + 13849) mod 65536       for 1 ≤ n ≤ 114
```

The seed uses all 12 bits of û₀ (pitch b0 plus spectral MSBs), not just the
8-bit b0 pitch value. Each p_r(n) is a 16-bit unsigned integer in [0, 65535].

**Mask bit extraction (Eq. 86–93):**

Each mask bit is `⌊p_r(n) / 32768⌋`, i.e. bit 15 (MSB) of p_r(n) — equivalently,
1 if p_r(n) ≥ 32768, else 0. Each vector consumes a contiguous range of PN
indices:

| Mask | Length | PN indices | Notes |
|------|-------:|-----------:|-------|
| m̂₀   | 23     | —          | All zeros (Eq. 86); v̂₀ is the Golay codeword carrying û₀ |
| m̂₁   | 23     | 1..23      | Eq. 87 |
| m̂₂   | 23     | 24..46     | Eq. 88 |
| m̂₃   | 23     | 47..69     | Eq. 89 |
| m̂₄   | 15     | 70..84     | Eq. 90 |
| m̂₅   | 15     | 85..99     | Eq. 91 |
| m̂₆   | 15     | 100..114   | Eq. 92 |
| m̂₇   | 7      | —          | All zeros (Eq. 93); v̂₇ is uncoded |

Total PN indices consumed: 114. Total mask bits: 23+23+23+23+15+15+15+7 = 144,
matching the full-rate frame size.

**Modulated code vectors (Eq. 94):**

```
c̃_i = v̂_i ⊕ m̂_i    for 0 ≤ i ≤ 7    (addition mod 2)
```

```c
/* Generate the 115-element PN sequence p_r(0..=114) for full-rate IMBE
 * bit modulation. Seed is the 12-bit Golay info word u0 (range [0, 4095]). */
void imbe_pn_sequence_fullrate(uint16_t u0, uint16_t pr_out[115]) {
    /* assert(u0 < 4096); */
    pr_out[0] = (uint16_t)(16u * u0);                  /* p_r(0) = 16*u0, fits in u16 */
    for (int n = 1; n < 115; n++) {
        pr_out[n] = (uint16_t)((173u * pr_out[n-1] + 13849u) & 0xFFFFu);
    }
}

/* Extract one mask bit from p_r(n): bit 15 of the 16-bit value. */
static inline uint8_t imbe_pn_mask_bit(uint16_t pr_n) {
    return (uint8_t)(pr_n >> 15);
}

/* Modulation vector descriptor: packed bits (transmission order — element 0
 * at bit len−1, element len−1 at bit 0) and the vector's length in bits.
 *
 * CRITICAL: see analysis/vocoder_decode_disambiguations.md §10 — the mask
 * must be XOR'd against a codeword whose bit (len−1) is the first-transmitted
 * bit. This alignment matches DVSI and OTA captures. A naive "element k at
 * bit k" packing (LSB-first) is internally consistent for a round-trip but
 * produces bit-reversed-within-vector-width output when compared against any
 * external reference. Self-consistent tests will NOT catch that bug. */
typedef struct { uint32_t bits; uint8_t len; } imbe_mod_vec_t;

/* Compute the 8 modulation vectors m_0..m_7 for full-rate IMBE. */
void imbe_modulation_vectors(uint16_t u0, imbe_mod_vec_t mv[8]) {
    uint16_t pr[115];
    imbe_pn_sequence_fullrate(u0, pr);

    /* Transmission-order packing (Eq. 87–92 bracket convention + §7.3
     * row-vector MSB-leftmost rule): element 0 of the bracket list is
     * the first-transmitted bit, so it goes at bit (len−1). */
    #define MASK_RANGE(start, length) ({                                      \
        uint32_t _bits = 0;                                                   \
        for (int _k = 0; _k < (length); _k++)                                 \
            _bits |= ((uint32_t)imbe_pn_mask_bit(pr[(start)+_k]))             \
                     << ((length) - 1 - _k);                                  \
        _bits;                                                                \
    })

    mv[0] = (imbe_mod_vec_t){ 0,                     23 };  /* m_0 (Eq. 86) */
    mv[1] = (imbe_mod_vec_t){ MASK_RANGE(1,   23),   23 };  /* m_1 (Eq. 87) */
    mv[2] = (imbe_mod_vec_t){ MASK_RANGE(24,  23),   23 };  /* m_2 (Eq. 88) */
    mv[3] = (imbe_mod_vec_t){ MASK_RANGE(47,  23),   23 };  /* m_3 (Eq. 89) */
    mv[4] = (imbe_mod_vec_t){ MASK_RANGE(70,  15),   15 };  /* m_4 (Eq. 90) */
    mv[5] = (imbe_mod_vec_t){ MASK_RANGE(85,  15),   15 };  /* m_5 (Eq. 91) */
    mv[6] = (imbe_mod_vec_t){ MASK_RANGE(100, 15),   15 };  /* m_6 (Eq. 92) */
    mv[7] = (imbe_mod_vec_t){ 0,                      7 };  /* m_7 (Eq. 93) */

    #undef MASK_RANGE
}
```

Note: the `MASK_RANGE` macro above uses GCC/Clang statement-expressions
(`({ ... })`) for brevity. In strictly portable C, replace with an explicit
helper function taking `(const uint16_t *pr, int start, int len)`.

**Decode order for PN demodulation:**
1. Golay-decode c̃₀ to recover û₀ (since m̂₀ = 0, so c̃₀ = v̂₀)
2. Seed the PN generator: p_r(0) = 16·û₀ (uses the full 12-bit û₀)
3. Compute masks m̂₁..m̂₆ from p_r(1..114); m̂₇ = 0
4. XOR c̃_i with m̂_i to recover v̂_i for 1 ≤ i ≤ 6
5. FEC-decode v̂₁..v̂₆ to recover û₁..û₆; û₇ = c̃₇ directly

### 1.7 Interleaving -- Full-Rate (Annex H)

The 144 FEC-encoded bits (from 8 vectors c0..c7) are interleaved across 72 dibit
symbols (144 bits = 72 symbols x 2 bits/symbol). Annex H provides the complete
72-row x 2-column mapping table.

The interleaving pattern from the extraction (Annex S, shown for half-rate; Annex H
for full-rate) distributes bits from different code words across the symbol stream to
spread burst errors across multiple FEC blocks.

**For implementation:** The interleaving table is identical to the one used by the
air interface layer. At the receiver:
1. Collect 72 consecutive dibit symbols for one IMBE frame
2. Deinterleave using the inverse of Annex H to recover c0..c7
3. Proceed with PN demodulation and FEC decoding

```c
/* Deinterleave a 144-bit IMBE frame (72 dibits) into 8 code word vectors.
 * dibits: 72 elements, each in 0..=3 (bit 1 = MSB, bit 0 = LSB).
 * c_out:  8 elements; c_out[0..=3] hold Golay codewords (23 bits each),
 *         c_out[4..=6] hold Hamming codewords (15 bits each), c_out[7]
 *         holds uncoded bits (7 bits). Bits packed LSB-first.
 *
 * The Annex H table (see annex_tables/annex_h_interleave.csv) maps
 *   (symbol_index, bit_position) -> (vector_index, bit_index). */
void deinterleave_imbe_fullrate(const uint8_t dibits[72], uint32_t c_out[8]);
```

### 1.8 Spectral Amplitude Reconstruction (Full-Rate)

Source: BABA-A §6.4 "Spectral Amplitudes Decoding", pages 29–31
(Equations 65–79) and §6.5 "Synchronization Encoding and Decoding"
(Equation 80, page 31).

This subsection covers the last decode stage: turning the dequantized
quantizer indices b̃₂..b̃_{L̃+1} (plus the previous frame's reconstructed
spectral amplitudes) into the L̃ spectral amplitudes M̃_l that feed the MBE
synthesizer. The pipeline has four steps: **dequantize → inverse DCT per
block → concatenate → inverse log-magnitude prediction.**

#### 1.8.1 Block Layout (Eq. 65–67)

The L̃ spectral amplitudes are partitioned into 6 blocks of lengths
J̃_1..J̃_6 (from Annex J / §12.5). Blocks satisfy:

```
Σ J̃_i = L̃                                                     (Eq. 65)
⌊L̃/6⌋ ≤ J̃_i ≤ J̃_{i+1} ≤ ⌈L̃/6⌉    for 1 ≤ i ≤ 5             (Eq. 66)
```

Each block has one "mean" coefficient C̃_{i,1} plus (J̃_i − 1) higher-order
coefficients C̃_{i,2}..C̃_{i,J̃_i}. The mean is set from the decoded gain
vector: **C̃_{i,1} = R̃_i**  (Eq. 67).

#### 1.8.2 Inverse Uniform Quantizer (Eq. 68, 71)

Both the transformed-gain coefficients G̃_2..G̃_6 (Annex F) and the
higher-order DCT coefficients C̃_{i,k} (Annex G) use an identical
midtread uniform quantizer. The decode formula with `+0.5` offset
(midtread convention) is:

```
value = 0                                    if B̃_m = 0
value = Δ̃_m · (b̃_m − 2^{B̃_m−1} + 0.5)      otherwise          (Eq. 68 / 71)
```

- **For Annex F (gain)**: m = 3..7, Δ̃_m from `annex_f_gain_allocation.csv`
  directly (already the final step size).
- **For Annex G (HOC)**: m = 8..L̃+1, Δ̃_m = **Table 3 step × Table 4 σ**:

| B̃_m | Table 3 step | | C_{i,k} index k | Table 4 σ |
|------|-------------:|---|-----------------|----------:|
| 1    | 1.20 σ      | | k = 2           | 0.307     |
| 2    | 0.85 σ      | | k = 3           | 0.241     |
| 3    | 0.65 σ      | | k = 4           | 0.207     |
| 4    | 0.40 σ      | | k = 5           | 0.190     |
| 5    | 0.28 σ      | | k = 6           | 0.179     |
| 6    | 0.15 σ      | | k = 7           | 0.173     |
| 7    | 0.08 σ      | | k = 8           | 0.165     |
| 8    | 0.04 σ      | | k = 9           | 0.170     |
| 9    | 0.02 σ      | | k = 10          | 0.170     |
| 10   | 0.01 σ      | |                 |           |

So for HOC coefficient C̃_{i,k} with B̃_m bits:
**Δ̃_m = (Table 3)[B̃_m] · (Table 4)[k]**

Example: a 4-bit allocation at k=3 → Δ̃ = 0.40 · 0.241 = 0.0964.

The relationship between m, i, and k (Eq. 64/72):
```
m = 6 + k + Σ_{n=1}^{i-1} J̃_n                                 (Eq. 72)
```
walks `[b̃₈..b̃_{L̃+1}] → [C̃_{1,2}, C̃_{1,3}, …, C̃_{1,J̃_1}, C̃_{2,2}, …, C̃_{6,J̃_6}]`
(Annex G's `b_m`, `C_i`, `C_k` columns spell this out explicitly).

#### 1.8.3 Gain Vector Recovery (Eq. 68, 69, 70)

```
G̃_1 = IMBE_GAIN_QUANTIZER[b̃₂]                    (Annex E / §12.1)
G̃_2..G̃_6 per Eq. 68 above

R̃_i = Σ_{m=1}^{6} α(m) · G̃_m · cos[ π·(m−1)·(i − 0.5) / J̃_i ]
                                                     for 1 ≤ i ≤ 6   (Eq. 69)
α(1) = 1,  α(m) = 2  for m > 1                                       (Eq. 70)
```

**SPEC BUG — Eq. 69 denominator is `6`, not `J̃_i`.** As printed in
BABA-A page 30, Eq. 69 has `J̃_i` in the cosine denominator. Verified
against the PDF on 2026-04-16. This is an editorial error in the
published standard:

- Eq. 61 (encoder forward DCT, page 27) uses denominator `6`.
- `R̂_i` and `Ĝ_m` are both 6-element vectors (Eq. 60, 61, 67). A
  forward/inverse DCT pair between two 6-element vectors is 6-point by
  definition; `J̃_i` makes the "inverse" mathematically unrelated to
  the forward, and the round-trip identity `R̂ → Ĝ → R̃` fails for
  every `L̃ ≠ 36` (36 is the only `L̃` where Annex J gives `J̃_i = 6`
  for all six blocks).
- The PDF text immediately preceding Eq. 69 calls it "an inverse DCT
  of `G̃_m`" — phrasing consistent with a fixed 6-point transform, not
  a per-block operation.
- The `J̃_i` was almost certainly copy-pasted from Eq. 73 (per-block
  HOC inverse DCT, page 30), where `J̃_i` is correct because each block's
  HOCs form a `J̃_i`-element vector.
- Every working reference implementation (JMBE, mbelib, OP25, SDRTrunk)
  uses denominator `6` in Eq. 69.

**Implementers must use denominator `6`:**

```
R̃_i = Σ_{m=1}^{6} α(m) · G̃_m · cos[ π·(m−1)·(i − 0.5) / 6 ]
                                                     for 1 ≤ i ≤ 6   (Eq. 69, corrected)
α(1) = 1,  α(m) = 2  for m > 1                                       (Eq. 70)
```

Tracked in [`analysis/phase4_findings_log.md`](../../analysis/phase4_findings_log.md)
2026-04-16 entry (flagged by downstream p25-decoder implementer, resolved
via PDF inspection the same day).

**Asymmetric forward/inverse convention:** the encoder's forward DCT
(Eq. 60 / 61) uses a uniform `1/N` factor with no α weighting; the
inverse here uses α(k) = {1, 2, …, 2} with no `1/N`. This is not an
orthonormal DCT-II / DCT-III pair. Round-trip tests that use the
textbook orthonormal forward (`√(1/N)`, `√(2/N)` scaling) will drift —
see [`analysis/vocoder_decode_disambiguations.md`](../../analysis/vocoder_decode_disambiguations.md)
§9 for the full pairing and reference implementations.

#### 1.8.4 Higher-Order DCT Reconstruction (Eq. 73–74)

After dequantizing all C̃_{i,k} per §1.8.2 and setting C̃_{i,1} = R̃_i,
each block runs an inverse DCT to produce the per-block prediction-residual
vector c̃_{i,j}:

```
c̃_{i,j} = Σ_{k=1}^{J̃_i} α(k) · C̃_{i,k} · cos[ π·(k−1)·(j − 0.5) / J̃_i ]
                                                     for 1 ≤ j ≤ J̃_i   (Eq. 73)
α(1) = 1,  α(k) = 2  for k > 1                                         (Eq. 74)
```

Concatenate the 6 blocks to form T̃_l for 1 ≤ l ≤ L̃:

```
T̃_l = c̃_{i,j}   where block i and position j satisfy
                  l = j + Σ_{n=1}^{i-1} J̃_n
```

**T̃_l is the log₂-domain spectral-amplitude prediction residual** for the
current frame.

#### 1.8.5 Inverse Log-Magnitude Prediction (Eq. 75–79)

Final reconstruction requires the previous frame's spectral amplitudes
M̃_l(−1) and the previous harmonic count L̃(−1). Per-harmonic mapping to
the prior frame uses linear interpolation:

```
k̃_l = L̃(−1) · l / L̃(0)                                       (Eq. 75)
δ̃_l = k̃_l − ⌊k̃_l⌋                                            (Eq. 76)
```

The reconstructed log-magnitude:

```
log₂ M̃_l(0) = T̃_l
             + ρ·(1−δ̃_l)·log₂ M̃_{⌊k̃_l⌋}(−1)
             + ρ·δ̃_l    ·log₂ M̃_{⌊k̃_l⌋+1}(−1)
             − (ρ / L̃(0)) · Σ_{λ=1}^{L̃(0)} [
                   (1−δ̃_λ)·log₂ M̃_{⌊k̃_λ⌋}(−1)
                 + δ̃_λ    ·log₂ M̃_{⌊k̃_λ⌋+1}(−1) ]           (Eq. 77)
```

where **ρ is defined by Eq. 55** (not a constant 0.65). Per BABA-A
§6.3 page 25, Eq. 55 gives a piecewise-linear schedule in `L̃(0)`:

```
ρ = 0.4                           if L̃(0) ≤ 15
ρ = 0.03 · L̃(0) − 0.05            if 15 < L̃(0) ≤ 24                 (Eq. 55)
ρ = 0.7                           otherwise
```

The symbol `ρ` in Eq. 77 refers back to this schedule. An earlier
version of this section asserted a constant `ρ = 0.65` based on the
paraphrase in `vocoder_decode_disambiguations.md` §3; that assertion
was wrong for full-rate — see the corrected disambiguation note and
`vocoder_analysis_encoder_addendum.md` §0.6.2 for the full table of
values and discussion.

**Half-rate differs.** The half-rate decoder (§2.13 of this spec,
BABA-A Eq. 185 on page 65) does use a literal constant `0.65` with no
`L̃` dependence — the `ρ = 0.65` claim is correct there and only there.
(Eq. 200 is a different equation — the first of the Eq. 200–205
frame-repeat copy-forward block; earlier drafts of this spec and the
disambiguation notes mis-cited the half-rate predictor as Eq. 200.)

Edge cases (Eq. 78–79):
```
M̃_0(−1) = 1.0
M̃_l(−1) = M̃_{L̃(−1)}(−1)  for l > L̃(−1)
```

**Initialization** (first frame): `M̃_l(−1) = 1 for all l; L̃(−1) = 30`
(matches §10 Annex A decoder state init).

**Final spectral amplitudes** (for the MBE synthesizer):
```
M̃_l = 2^{log₂ M̃_l(0)}   for 1 ≤ l ≤ L̃
```

#### 1.8.6 Frame Synchronization Bit b̂_{L̂+2} (Eq. 80)

Source: BABA-A §6.5, page 31.

The `b̂_{L̂+2}` bit carved out by §1.4.2's final placement slot (û₇[0])
**is not a spectral amplitude coefficient** — it's a frame-sync helper.
The encoder toggles it each frame:

```
b̂_{L̂+2}(0) = 0  if b̂_{L̂+2}(−1) = 1
b̂_{L̂+2}(0) = 1  otherwise                                     (Eq. 80)
```

Initial value after reset: 0. The decoder may use this for frame-boundary
detection (the bit is placed at a fixed offset in the 144-bit frame; see
§1.4.2) or ignore it when external sync (e.g. from the air-interface
frame format) is already established.

#### 1.8.7 Complete C Pipeline

```c
/* Input:  quantizer values b̃₀..b̃_{L+2} already unpacked from û₀..û₇
 * Output: M̃_l for l=1..L̃ (spectral amplitudes) and updated decoder state
 * Requires:  Annex E (gain quantizer), Annex F (gain allocation),
 *            Annex G (HOC allocation), Annex J (block lengths),
 *            Tables 3 & 4 (HOC step size & σ) — inlined in §1.8.2.    */

/* Table 3 & 4 values (§1.8.2). */
static const float HOC_STEP[11]  = { 0.0f, 1.20f, 0.85f, 0.65f, 0.40f, 0.28f,
                                      0.15f, 0.08f, 0.04f, 0.02f, 0.01f };
static const float HOC_SIGMA[11] = { 0.0f, 0.0f, 0.307f, 0.241f, 0.207f, 0.190f,
                                      0.179f, 0.173f, 0.165f, 0.170f, 0.170f };
/* HOC_SIGMA indexed by k (DCT coefficient position, k=2..10); k=0,1 unused.
 * Values k ≥ 10 clamp to 0.170 per Table 4 (last row). */

/* Full-rate prediction gain ρ is L̃-dependent per Eq. 55, not a constant. */
static inline float fullrate_rho(uint8_t L) {
    if (L <= 15)        return 0.40f;
    else if (L <= 24)   return 0.03f * (float)L - 0.05f;
    else                return 0.70f;
}

/* One-shot decoder: call per frame. prev_M has length prev_L+1 (index 0..prev_L).
 * prev_M[0] = 1.0f always; prev_M[prev_L] used as fallback for l > prev_L.    */
void imbe_reconstruct_spectral_amplitudes(
    uint8_t  L,                       /* harmonics this frame, Eq. 47       */
    uint8_t  prev_L,                  /* harmonics previous frame           */
    const float prev_M[/* prev_L+1 */],
    const uint8_t J[6],               /* block lengths from Annex J          */
    float    G_tilde[6],              /* G̃_1..G̃_6, already dequantized   */
    const float C_tilde[6][10],       /* C̃_{i,k} for k=2..J_i, dequantized  */
    float    M_out[/* L+1 */]);       /* M̃_l written to index 1..L         */
/* Reference implementation outline:
 *   1. Build R̃_i per Eq. 69 for i=1..6 using G̃_1..G̃_6.
 *   2. Set C̃_{i,1} = R̃_i.
 *   3. Inverse block DCT per Eq. 73 to get c̃_{i,j} for j=1..J̃_i.
 *   4. Concatenate into T̃_l per §1.8.4.
 *   5. Apply Eq. 75–79 to get log₂ M̃_l(0).
 *   6. M̃_l = exp2f(log2_M_l).
 *   7. Caller updates decoder state (prev_M, prev_L) for next frame. */
```

### 1.9 Spectral Amplitude Reconstruction is Cross-Frame Stateful

Unlike pitch, V/UV, and gain which are self-contained per frame, the
reconstruction in §1.8.5 references the previous frame's M̃_l(−1) values.
This means:
- The decoder must maintain rolling state: last reconstructed M̃_l and L̃.
- After a **frame mute** (§6), the previous-frame values should still be
  preserved for use in the next valid frame, per BABA-A error-concealment
  guidance.
- After **frame repeat**, the repeated M̃_l becomes the "previous frame"
  for the frame after.
- After a **decoder cold start**, use the initialization values from
  §10 (Annex A): `M̃_l(−1) = 1 for all l, L̃(−1) = 30`. Since the
  initial log-amplitudes are zero (log₂ 1 = 0), the predictor
  contributions on the first few frames vanish regardless of `ρ`, and
  the predictor settles toward steady state over ≈ 3–5 frames as the
  real reconstructed amplitudes populate `M̃_l(−1)`.

### 1.10 Spectral Amplitude Enhancement (Full-Rate)

Source: BABA-A §8 "Full-Rate Spectral Amplitude Enhancement", pages 41–42,
Equations 105–111.

Before speech synthesis, the reconstructed spectral amplitudes `M̃_l` are
passed through a psycho-acoustic weighting step. The **unenhanced** `M̃_l`
are preserved for use as `M̃_l(−1)` in the next frame's prediction (§1.8.5);
the **enhanced** `M̄_l` are what the synthesizer actually consumes.

```
R_M0 = Σ_{l=1}^{L̃} M̃_l²                                          (Eq. 105)
R_M1 = Σ_{l=1}^{L̃} M̃_l² · cos(ω̃₀ · l)                            (Eq. 106)

W_l = √(M̃_l) · [ 0.96 · (R_M0² + R_M1² − 2·R_M0·R_M1·cos(ω̃₀·l))
                 / (ω̃₀ · R_M0 · (R_M0 − R_M1)) ]^(1/4)             (Eq. 107)
```

Apply the weighting with guardrails:

```
          M̃_l                  if 8·l ≤ L̃           (Eq. 108, first branch)
M̄_l =    1.2 · M̃_l            else if W_l > 1.2    (clamp high)
          0.5 · M̃_l            else if W_l < 0.5    (clamp low)
          W_l · M̃_l            otherwise
```

The `8·l ≤ L̃` branch disables the weighting on the lowest `⌊L̃/8⌋`
harmonics (where perceptual weighting is counterproductive).

Energy-preserving rescale (Eq. 109–110):

```
γ = sqrt( R_M0 / Σ_{l=1}^{L̃} |M̄_l|² )
M̄_l ← γ · M̄_l     for 1 ≤ l ≤ L̃
```

Local energy state update (Eq. 111):

```
S_E(0) = max( 0.95·S_E(−1) + 0.05·R_M0, 10000.0 )
```

`S_E` is initialized to **75000** (BABA-A §10 Annex A); feeds Eq. 112 in the
next subsection.

```c
/* Inputs:  L (current harmonic count), omega_0, M_tilde[1..L] (unenhanced,
 *          output of §1.8.5), S_E_prev (previous frame's S_E).
 * Outputs: M_bar[1..L] (enhanced, fed to synthesizer), S_E_out (updated). */
void imbe_enhance_spectral_amplitudes(
    uint8_t L, double omega_0,
    const double M_tilde[/* L+1 */],
    double S_E_prev,
    double M_bar[/* L+1 */],
    double *S_E_out)
{
    /* Eq. 105-106 */
    double R_M0 = 0.0, R_M1 = 0.0;
    for (uint8_t l = 1; l <= L; l++) {
        double m2 = M_tilde[l] * M_tilde[l];
        R_M0 += m2;
        R_M1 += m2 * cos(omega_0 * (double)l);
    }

    /* Eq. 107-108 */
    double sum_sq = 0.0;
    for (uint8_t l = 1; l <= L; l++) {
        double bar;
        if (8 * l <= L) {
            bar = M_tilde[l];
        } else {
            double denom = omega_0 * R_M0 * (R_M0 - R_M1);
            double num = R_M0 * R_M0 + R_M1 * R_M1
                         - 2.0 * R_M0 * R_M1 * cos(omega_0 * (double)l);
            double W_l = sqrt(M_tilde[l]) * pow(0.96 * num / denom, 0.25);
            if      (W_l > 1.2) bar = 1.2 * M_tilde[l];
            else if (W_l < 0.5) bar = 0.5 * M_tilde[l];
            else                bar = W_l * M_tilde[l];
        }
        M_bar[l] = bar;
        sum_sq += bar * bar;
    }

    /* Eq. 109-110: energy-preserving scale */
    double gamma = sqrt(R_M0 / sum_sq);
    for (uint8_t l = 1; l <= L; l++) M_bar[l] *= gamma;

    /* Eq. 111: update local energy */
    double s_e = 0.95 * S_E_prev + 0.05 * R_M0;
    *S_E_out = (s_e >= 10000.0) ? s_e : 10000.0;
}
```

### 1.11 Adaptive Smoothing and Frame Repeat / Mute (Full-Rate)

Source: BABA-A §7.7 "Frame Repeat" (Eq. 97–104, page 40), §7.8 "Frame Muting"
(page 40), §9 "Adaptive Smoothing" (Eq. 112–116, pages 43–44).

#### 1.11.1 Frame Repeat Trigger

Count bit errors per code vector during FEC decode (§1.5) as `ε₀..ε₆`.
Compute:

```
ε_T = Σ ε_i                                                        (total errors)
ε_R(0) = 0.95·ε_R(−1) + 0.05·(ε_T / 144)     (smoothed error rate — recommended)
```

Trigger a frame repeat if **any** of these hold:

```
b̂₀ ∉ [0, 207]                                    (invalid pitch)  (§6.1)
ε₀ ≥ 2   AND   ε_T ≥ 10 + 40·ε_R(0)               (Eq. 97/98 joint)
```

On repeat: **do not synthesize from the current frame's parameters**. Instead,
substitute the previous frame's parameters via Eq. 99–104:

```
ω̃₀(0) = ω̃₀(−1)                                                  (Eq. 99)
L̃(0)  = L̃(−1)                                                   (Eq. 100)
K̃(0)  = K̃(−1)                                                   (Eq. 101)
ṽ_k(0) = ṽ_k(−1)   for 1 ≤ k ≤ K̃                                 (Eq. 102)
M̃_l(0) = M̃_l(−1)  for 1 ≤ l ≤ L̃                                 (Eq. 103)
M̄_l(0) = M̄_l(−1)  for 1 ≤ l ≤ L̃                                 (Eq. 104)
```

Then run the rest of the synthesis pipeline (§1.10–§1.12) against these
repeated parameters. The "current" frame becomes the previous frame for
the next one.

#### 1.11.2 Frame Mute Trigger

Mute if `ε_R(0) > 0.0875`. Recommended muting procedure (§7.8):

1. Run Eq. 99–104 as if for a repeat (preserves state for future
   re-acquisition).
2. **Bypass the synthesizer** entirely. Output `s̃(n) = random small-amplitude
   noise` for one frame (or true silence if the application prefers).

#### 1.11.3 V/UV Smoothing (Eq. 112–116)

Runs **after** §1.10 enhancement and **before** §1.12 synthesis. Computes an
adaptive voicing threshold `V_M` from the smoothed error rate `ε_R(0)`
(in `[0, 1]`) and the current-frame energy `S_E(0)`:

```
       ┌ ∞                              if ε_R(0) ≤ 0.005   AND ε_T ≤ 4
V_M = ┤ 45.255·S_E(0)^0.375
       │   · exp(−277.26·ε_R(0))        else if ε_R(0) ≤ 0.0125 AND ε₄ = 0    (Eq. 112)
       └ 1.414·S_E(0)^0.375              otherwise
```

`V_M = ∞` effectively disables per-harmonic override.

Per-harmonic V/UV override:

```
v̄_l = 1          if M̄_l > V_M         (force voiced regardless of decoded v̂_k)
v̄_l = ṽ_l       otherwise             (keep the §1.3.2 expansion)      (Eq. 113)
```

Amplitude smoothing (Eq. 114–116):

```
A_M = Σ_{l=1}^{L̃} M̄_l                                                (Eq. 114)

        ┌ 20480                         if ε_R(0) ≤ 0.005 AND ε_T(0) ≤ 6
τ_M(0) = ┤                                                              (Eq. 115)
        └ 6000 − 300·ε_T + τ_M(−1)       otherwise

γ_M = 1.0              if τ_M(0) > A_M                                 (Eq. 116)
γ_M = τ_M(0) / A_M     otherwise
```

Then `M̄_l ← γ_M · M̄_l` for `1 ≤ l ≤ L̃`. The smoothed V/UV decisions
`v̄_l` and amplitudes `M̄_l` are the synthesizer's input.

Initial state: `τ_M(−1) = 20480` (Annex A §10).

### 1.12 Speech Synthesis (Full-Rate)

Source: BABA-A §11 "Speech Synthesis", pages 51–55, Equations 117–142.

Produces `s̃(n)` for `0 ≤ n < N` where `N = 160` samples (20 ms at 8 kHz).
Output is voiced + unvoiced:

```
s̃(n) = s̃_uv(n) + s̃_v(n)    for 0 ≤ n < 160                         (Eq. 142)
```

Both components use the Annex I synthesis window `wS(n)` (211 values,
`n = −105..105`, see §12.7) and a cross-frame state block described in
§1.13.

#### 1.12.1 Unvoiced Component (Eq. 117–126)

1. **White noise** (Eq. 117):
   ```
   u(n+1) = (171·u(n) + 11213) mod 53125
   u(−105) = 3147           (initial state; Annex A §10)
   ```
   Shift the noise buffer by N = 160 samples each frame (49 sample overlap
   with the 209-sample window wS).

2. **Windowed 256-point DFT** (Eq. 118):
   ```
   U_w(m) = Σ_{n=−104}^{104} u(n) · wS(n) · e^{−j2π·m·n/256}
            for −128 ≤ m ≤ 127
   ```

3. **Spectral shaping per band** — scale `U_w(m)` to match `M̄_l` where
   band `l` is unvoiced, zero it where band `l` is voiced:
   ```
   ã_l = (256 / 2π) · (l − 0.5) · ω̃₀                             (Eq. 122)
   b̃_l = (256 / 2π) · (l + 0.5) · ω̃₀                             (Eq. 123)

   For each l in 1..L̃, for m with ⌈ã_l⌉ ≤ |m| ≤ ⌈b̃_l⌉:
       if v̄_l = 1 (voiced):    Ũ_w(m) = 0                        (Eq. 119)
       else (unvoiced):
           norm = sqrt( Σ_{η=⌈ã_l⌉}^{⌈b̃_l⌉−1} |U_w(η)|²
                       / (⌈b̃_l⌉ − ⌈ã_l⌉) )
           Ũ_w(m) = γ_w · M̄_l(0) · U_w(m) / norm                 (Eq. 120)

   Below band 1 and above band L̃:                                (Eq. 124)
       Ũ_w(m) = 0  for |m| < ⌈ã_1⌉  or  ⌈b̃_L̃⌉ ≤ |m| ≤ 128
   ```

   `γ_w` is a **constant** depending only on `wS(n)` (synthesis window,
   Annex I / §12.7) and `wR(n)` (pitch refinement window, Annex C / §12.18):
   ```
   γ_w = [ Σ_{n=−110}^{110} wR(n) ] · sqrt( (Σ_{n=−104}^{104} wS²(n))
                                           / (Σ_{n=−110}^{110} wR²(n)) )     (Eq. 121)
   ```
   Evaluated from the committed CSVs:
   ```
   Σ wR(n) for n=-110..110   = 110.019884
   Σ wR²(n) for n=-110..110  = 80.683642
   Σ wS²(n) for n=-104..104  = 143.340000
   γ_w                        = 146.643269
   ```
   Precompute once at decoder init; it's a fixed scalar.

   **⚠ Empirical calibration note:** direct use of γ_w = 146.643269
   produces unvoiced output roughly 150× louder than DVSI's reference
   PCM for the same bitstream. Empirical optimum on the `alert.bit`
   test case lies near γ_w ≈ 1.0 (see table below). The spec value
   is committed as authoritative; the mismatch is under investigation
   and documented in
   [`analysis/vocoder_decode_disambiguations.md §11`](../../analysis/vocoder_decode_disambiguations.md)
   with candidate causes (M̃_l scale from the Annex E quantizer
   interpretation, unvoiced-norm formula, or a different normalization
   convention between the spec and DVSI's implementation).
   ```
       γ_w   RMS error   SNR
       0.5    3587      -0.36 dB
       1.0    3587      -0.36 dB
       5.0    3593      -0.38 dB
      10.0    3613      -0.43 dB
      50.0    4230      -1.80 dB
     146.6    6724      -5.82 dB        (spec value)
   ```
   Use the spec value until the root cause is found — the mismatch
   may be compensating for a bug elsewhere in the pipeline, and
   "fitting" γ_w locally could mask it.

   **Independent chip measurement (2026-04-17, gap report 0001 §9.4):**
   A chip-vs-local comparison on a realistic silence→voiced-flat-500
   scenario (post blip25-mbe's quantizer-predictor fix) measured the
   voiced-amplitude chip/local ratio at **1.56×**. This is consistent
   with the historical "~1.7× too loud" memory note on voiced
   synthesis and with the broader unvoiced-scale story here. The
   single-harmonic synthetic probes attempted in the same gap report
   (§9.2 γ_w, §9.3 §1.10 bypass, §9.5 M̃_l scale) did **not** produce
   clean scalars because the AMBE-3000R applies proprietary post-
   synthesis gating that is not in BABA-A §1.10–§1.12 — see
   [`analysis/ambe3000_chip_oracle_caveats.md`](../../analysis/ambe3000_chip_oracle_caveats.md)
   for the three documented chip behaviors (amplitude-dependent mute,
   ±10000 internal cap, stationary-signal detector) and the revised
   probing methodology. The 1.56× figure remains the most trustworthy
   chip-side measurement of the voiced-amplitude gap to date; it is
   not a direct γ_w measurement but bounds the combined §1.10 + §1.12
   amplitude error.

4. **Inverse DFT** (Eq. 125):
   ```
   ũ_w(n) = (1/256) · Σ_{m=−128}^{127} Ũ_w(m) · e^{j2π·m·n/256}
            for −128 ≤ n ≤ 127
   ```

5. **Weighted overlap-add** with previous frame's unvoiced buffer (Eq. 126):
   ```
   s̃_uv(n) = [ wS(n)·ũ_w(n, −1) + wS(n−N)·ũ_w(n−N, 0) ]
             / [ wS²(n) + wS²(n−N) ]          for 0 ≤ n < N
   ```
   (wS is zero outside `−105..105`; ũ_w is zero outside `−128..127`.)

#### 1.12.2 Voiced Component (Eq. 127–141)

Per-harmonic sum:
```
s̃_v(n) = Σ_{l=1}^{max(L̃(−1), L̃(0))} 2 · s̃_{v,l}(n)     for 0 ≤ n < N  (Eq. 127)
```

Out-of-range spectral amplitudes from either frame are treated as zero
and unvoiced (Eq. 128–129):
```
M̄_l(0)  = 0  for l > L̃(0)
M̄_l(−1) = 0  for l > L̃(−1)
```

Per-harmonic rule for `s̃_{v,l}(n)` (four cases based on V/UV transitions):

| Previous | Current | l | pitch change | Formula |
|---------:|--------:|---|--------------|---------|
| unvoiced | unvoiced | — | — | `s̃_{v,l}(n) = 0`  (Eq. 130) |
| voiced | unvoiced | — | — | `wS(n) · M̄_l(−1) · cos(ω̃₀(−1)·n·l + φ_l(−1))`  (Eq. 131) |
| unvoiced | voiced | — | — | `wS(n−N) · M̄_l(0) · cos(ω̃₀(0)·(n−N)·l + φ_l(0))`  (Eq. 132) |
| voiced | voiced | l ≥ 8 **or** |Δω̃₀·l/ω̃₀(0)| ≥ 0.1 | — | sum of both previous terms (Eq. 133) |
| voiced | voiced | l < 8 **and** |Δω̃₀·l/ω̃₀(0)| < 0.1 | small | amplitude-and-phase ramp (Eq. 134) |

where `Δω̃₀ = ω̃₀(0) − ω̃₀(−1)`. The "small pitch change, low harmonic"
branch is the phase-tracked interpolation:

```
s̃_{v,l}(n) = a_l(n) · cos(θ_l(n))                               (Eq. 134)

a_l(n) = M̄_l(−1) + (n/N) · [M̄_l(0) − M̄_l(−1)]                  (Eq. 135)

θ_l(n) = φ_l(−1)
       + [ω̃₀(−1)·l + Δω_l(0)] · n
       + [ω̃₀(0)·l − ω̃₀(−1)·l] · n²/(2N)                         (Eq. 136)

Δφ_l(n) = φ_l(0) − φ_l(−1) − [ω̃₀(−1) + ω̃₀(0)] · l·N/2           (Eq. 137)

Δω_l(0) = (1/N) · [ Δφ_l(0) − 2π·⌊(Δφ_l(0) + π)/(2π)⌋ ]          (Eq. 138)
```

Phase-state update (every frame, for `1 ≤ l ≤ 56`, regardless of V/UV):

```
ψ_l(0) = ψ_l(−1) + [ω̃₀(−1) + ω̃₀(0)] · l·N/2                    (Eq. 139)

         ┌ ψ_l(0)                            for 1 ≤ l ≤ ⌊L̃/4⌋
φ_l(0) = ┤                                                      (Eq. 140)
         └ ψ_l(0) + L̃_uv(0)·ρ_l(0)/L̃(0) · l  for ⌊L̃/4⌋ < l ≤ max(L̃(−1), L̃(0))

ρ_l(0) = (2π / 53125) · u(l) − π                                (Eq. 141)
```

where `L̃_uv(0)` = count of unvoiced harmonics in the current frame, and
`u(l)` is drawn from the same shifted noise sequence used in the unvoiced
synthesizer (Eq. 117). `ρ_l(0)` is uniform in `[−π, π)`.

Initial state: `ψ_l(−1) = 0`, `φ_l(−1) = 0` for all l (Annex A §10).

### 1.13 Synthesizer Cross-Frame State Summary

Extending §1.9 with every piece of state needed by the complete
synthesis pipeline:

| State | Init | Updated by | Consumed by |
|-------|------|------------|-------------|
| `M̃_l(−1)` for l = 1..56 | 1.0 | §1.8.5 reconstruction | §1.8.5 next frame (prediction) |
| `L̃(−1)` | 30 | §1.3.1 per frame | §1.8.5, §1.12.2 |
| `ω̃₀(−1)` | 0.02985·π | §1.3.1 per frame | §1.12.2 voiced synthesis |
| `K̃(−1)` | 10 | §1.3.1 Eq. 48 | §1.11.3 (V/UV smoothing context) |
| `ṽ_k(−1)` | 0 for all k | §1.3.2 | §1.11.3 |
| `M̄_l(−1)` for l = 1..56 | 0 | §1.10, §1.11.3 | §1.12.2 voiced synthesis |
| `S_E(−1)` | 75000 | §1.10 Eq. 111 | §1.11.3 V_M, §1.10 next frame |
| `τ_M(−1)` | 20480 | §1.11.3 Eq. 115 | §1.11.3 next frame |
| `ε_R(−1)` (smoothed error rate) | 0 | FEC decode per frame | §1.11.1, §1.11.3 |
| `φ_l(−1)` for l = 1..56 | 0 | §1.12.2 Eq. 140 | §1.12.2 next frame |
| `ψ_l(−1)` for l = 1..56 | 0 | §1.12.2 Eq. 139 | §1.12.2 next frame |
| `ũ_w(n, −1)` for n = −128..127 | 0 | §1.12.1 Eq. 125 | §1.12.1 OLA next frame |
| noise sequence `u(n)` | u(−105) = 3147 | §1.12.1 Eq. 117 | §1.12.1 each frame |

**Frame repeat interaction** (§1.11.1):
- On repeat, Eq. 99–104 restore `ω̃₀, L̃, K̃, ṽ_k, M̃_l, M̄_l` from previous
  frame. **Do NOT restore `S_E, τ_M, ε_R, φ_l, ψ_l`** — those are
  per-frame running state and continue evolving.

**Frame mute interaction** (§1.11.2):
- Same state-preservation as repeat, but skip §1.12 entirely and emit
  silence/low-amplitude noise. Preserve everything listed above so
  re-acquisition after the mute period works.

**Cold start** (decoder reset):
- Use the init column of the table above exactly. Annex A §10 in the
  current impl spec has most of these; §1.13 here is the
  pipeline-eye-view cross-reference.

---

## 2. Half-Rate AMBE+2 Vocoder (Phase 2 TDMA)

### 2.1 Fundamental Parameters

| Parameter | Value |
|-----------|-------|
| Bit rate (total) | 3,600 bps |
| Voice parameter bits | 49 per frame |
| FEC bits | 23 per frame |
| Total bits per frame | 72 |
| Frame duration | 20 ms |
| Frames per second | 50 |
| Sample rate | 8,000 Hz |
| Samples per frame | 160 |
| Pitch levels | 120 (7-bit index) |
| Harmonics L | 9 to 56 |
| V/UV codebook entries | 32 (5-bit index) |

### 2.2 The 49 Information Bits -- Parameter Allocation

| Parameter | Symbol | Bits | Levels | Codebook | Description |
|-----------|--------|------|--------|----------|-------------|
| Pitch | b0 | 7 | 120 | Annex L | Fundamental frequency, maps to (L, omega_0) pair |
| V/UV | b1 | 5 | 32 | Annex M | Voiced/unvoiced decisions (codebook, not per-band) |
| Gain | b2 | 5 | 32 | Annex O | Differential gain quantizer |
| PRBA24 | b3 | 9 | 512 | Annex P | First-stage spectral VQ: G2, G3, G4 (3-element) |
| PRBA58 | b4 | 7 | 128 | Annex Q | Second-stage spectral VQ: G5, G6, G7, G8 (4-element) |
| HOC block 1 | b5 | 5 | 32 | Annex R | H1,1 H1,2 H1,3 H1,4 (4-element VQ) |
| HOC block 2 | b6 | 4 | 16 | Annex R | H2,1 H2,2 H2,3 H2,4 (4-element VQ) |
| HOC block 3 | b7 | 4 | 16 | Annex R | H3,1 H3,2 H3,3 H3,4 (4-element VQ) |
| HOC block 4 | b8 | 3 | 8 | Annex R | H4,1 H4,2 H4,3 H4,4 (4-element VQ) |
| **Total** | | **49** | | | |

### 2.3 Half-Rate Bit Prioritization

Source: BABA-A Section 14.1 "Bit Prioritization", pages 66–67. Tables 15–18
define the bit-by-bit mapping directly; no scan algorithm required.

Half-rate is conceptually simpler than full-rate: the 49 information bits
are deterministic mappings from the 9 quantizer values b̂₀..b̂₈ into 4 bit
vectors û₀..û₃. Vector widths:

```
û₀ = 12 bits     (protected by [24,12] extended Golay → ĉ₀ = 24 coded bits)
û₁ = 12 bits     (protected by [23,12] Golay + PN mod → ĉ₁ = 23 coded bits)
û₂ = 11 bits     (uncoded pass-through                → ĉ₂ = 11 bits)
û₃ = 14 bits     (uncoded pass-through                → ĉ₃ = 14 bits)
Grand total = 49 information bits, mapping to 24 + 23 + 11 + 14 = 72 channel bits
```

**Note on û₂ / û₃ widths and FEC:** The vector-widths match BABA-A §14.1
exactly (12/12/11/14 info bits). Only û₀ and û₁ are FEC-protected — see
BABA-A §14.2 Eq. 189–192 (page 68) and Figure 27 (page 66), which state
explicitly: `ĉ₀ = û₀·G_{24,12}`, `ĉ₁ = û₁·G_{23,12} + m̂₁`, `ĉ₂ = û₂`,
`ĉ₃ = û₃`. Earlier prose in this spec claimed û₂ also got [23,12] Golay —
that is wrong; only ĉ₀ and ĉ₁ carry FEC, and the 23 parity bits come
entirely from the two Golay codes. See the half-rate FEC layout note in
`analysis/halfrate_fec_layout.md` for the PDF-vs-impl-spec reconciliation.

#### 2.3.1 Construction of û₀ (Table 15 of BABA-A)

| Bit position in û₀ | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
|--------------------|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Source             |b̂₀(6)|b̂₀(5)|b̂₀(4)|b̂₀(3)|b̂₁(4)|b̂₁(3)|b̂₁(2)|b̂₁(1)|b̂₂(4)|b̂₂(3)|b̂₂(2)|b̂₂(1)|

MSBs of pitch (4 bits of the 7-bit b̂₀), V/UV (4 bits of the 5-bit b̂₁),
and gain (4 bits of the 5-bit b̂₂).

#### 2.3.2 Construction of û₁ (Table 16)

| Bit position in û₁ | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
|--------------------|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Source             |b̂₃(8)|b̂₃(7)|b̂₃(6)|b̂₃(5)|b̂₃(4)|b̂₃(3)|b̂₃(2)|b̂₃(1)|b̂₄(6)|b̂₄(5)|b̂₄(4)|b̂₄(3)|

MSBs of PRBA24 (8 of 9 bits) and PRBA58 (4 of 7 bits).

#### 2.3.3 Construction of û₂ (Table 17, 11 info bits)

| Bit position in û₂ | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
|--------------------|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Source             |b̂₅(4)|b̂₅(3)|b̂₅(2)|b̂₅(1)|b̂₆(3)|b̂₆(2)|b̂₆(1)|b̂₇(3)|b̂₇(2)|b̂₇(1)|b̂₈(2)|

MSBs of HOC1 (4 of 5), HOC2 (3 of 4), HOC3 (3 of 4), and HOC4 MSB only.

#### 2.3.4 Construction of û₃ (Table 18, 14 bits)

| Bit position in û₃ | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
|--------------------|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Source             |b̂₁(0)|b̂₂(0)|b̂₀(2)|b̂₀(1)|b̂₀(0)|b̂₃(0)|b̂₄(2)|b̂₄(1)|b̂₄(0)|b̂₅(0)|b̂₆(0)|b̂₇(0)|b̂₈(1)|b̂₈(0)|

All remaining LSBs. Every parameter bit lands in exactly one slot across
Tables 15–18 (verified: pitch 7, V/UV 5, gain 5, PRBA24 9, PRBA58 7, HOC1 5,
HOC2 4, HOC3 4, HOC4 3 → sum = 49 ✓).

#### 2.3.5 Consumer Signatures

```c
/* Half-rate bit prioritization is a deterministic table lookup, so the
 * implementation is most naturally expressed as an explicit (src_param,
 * src_bit) → (dst_vec, dst_bit) map of 49 entries. The inverse (decode)
 * walks the same map in the opposite direction. */

typedef struct {
    uint8_t src_param;   /* 0..8 for b̂₀..b̂₈                           */
    uint8_t src_bit;     /* 0..(width−1), LSB=0                         */
    uint8_t dst_vec;     /* 0..3 for û₀..û₃                             */
    uint8_t dst_bit;     /* 0..(width−1), LSB=0                         */
} ambe_bit_map_entry_t;

extern const ambe_bit_map_entry_t ambe_bit_map[49];  /* Tables 15..18 */

void ambe_pack(const uint16_t b[9], uint16_t u[4]);
void ambe_unpack(const uint16_t u[4], uint16_t b[9]);
```

The pre-computed map is available as
[`annex_tables/ambe_bit_prioritization.csv`](annex_tables/ambe_bit_prioritization.csv)
(49 rows, columns `src_param, src_bit, dst_vec, dst_bit`). The CSV was
verified during generation to cover every (src_param, src_bit) slot in
b̂₀..b̂₈ exactly once (coverage invariant PASS).

#### 2.3.6 V/UV Codebook Expansion (Per-Harmonic Decisions)

Source: BABA-A §13.2 "Voiced / Unvoiced Decision Encoding and Decoding",
page 59 (Equations 146–149).

Half-rate encodes V/UV decisions differently from full-rate. The encoder
stores an *index* b̂₁ ∈ [0, 31] selecting one of the 32 codebook vectors in
Annex M — each vector has exactly 8 binary entries `v(n) = (v_0, v_1, …, v_7)`,
regardless of L̃. The decoder then maps each harmonic l ∈ [1, L̃] to one of
those 8 codebook slots and extracts the bit.

**Per-harmonic reconstruction (Eq. 147 & 149):**

```
j_l = floor(l · 16 · ω̃₀ / (2π)),     clamped to [0, 7]     (Eq. 147)
ṽ_l = v_{j_l}(n)    where n = b̃₁                             (Eq. 149)
```

where v_{j_l}(n) is the j_l-th column of the selected Annex M row n. Note
that j_l depends on *both* the harmonic index l and the fundamental ω̃₀ —
higher harmonics and/or higher pitch shift a harmonic further into the
codebook's 8-bin logical band structure.

**Extraction note:** the pdftotext rendering of Eq. 147 drops the `l`
multiplicand (`⌊16·ω̂₀ / 2π⌋` with no l), which doesn't make sense
dimensionally (the result would be the same for every harmonic). The
correct form `⌊l · 16 · ω̂₀ / (2π)⌋` is the only interpretation that
produces the expected coverage of the 8-element codebook across a typical
frequency range (verified: at ω̃₀ = 2π/19.875 rad/sample and l=L̃=9,
j_9 = ⌊9·16/19.875⌋ = 7; at l=1, j_1 = 0 — i.e., full range of the codebook
is used).

**Silence frames:** when b̃₁ = 0 the frame is treated as silence and all
ṽ_l = 1 (per BABA-A §13.2 Eq. 145 via the all-voiced codebook entry
at n = 0).

```c
/* Expand half-rate V/UV codebook index into per-harmonic decisions.
 * n:     the 5-bit codebook index b̃₁ ∈ [0, 31]
 * ambe_vuv_codebook[n][0..7]: the 8-entry vector from Annex M (see §12.9)
 * omega_0: the decoded fundamental frequency (rad/sample)
 * L: the harmonic count derived from b̃₀ (Annex L; see §12.8)
 * v_harm: L entries written, indexed by l−1. */
void ambe_vuv_expand(uint8_t n, double omega_0, uint8_t L,
                    uint8_t v_harm[/* L */])
{
    for (uint8_t l = 1; l <= L; l++) {
        int j = (int)floor((double)l * 16.0 * omega_0 / (2.0 * M_PI));
        if (j < 0) j = 0;
        if (j > 7) j = 7;
        v_harm[l - 1] = ambe_vuv_codebook[n][j];
    }
}
```

### 2.4 FEC Encoding -- Half-Rate

Source: BABA-A §14.2 "Error Control Coding" Eq. 189–192 (page 68) and
Figure 27 "Half-Rate Code Vector Construction" (page 66).

Only û₀ and û₁ receive FEC. The four code vectors are:

```
ĉ₀ = û₀ · G_{24,12}            (Eq. 189) — 24 bits
ĉ₁ = û₁ · G_{23,12} ⊕ m̂₁       (Eq. 190) — 23 bits, PN-modulated
ĉ₂ = û₂                         (Eq. 191) — 11 bits, uncoded
ĉ₃ = û₃                         (Eq. 192) — 14 bits, uncoded
```

All operations are mod-2. The PN sequence m̂₁ (§2.5) is applied only to
ĉ₁, not to the other code vectors.

#### 2.4.1 [24,12] Extended Golay Code

Applied to u0 only. This is the standard [23,12] Golay code with one additional
overall parity bit appended, giving a [24,12] code with minimum distance 8.
Corrects up to 3 errors and detects 4.

```c
/* [24,12] Extended Golay encode: 12 info bits in, 24-bit codeword out.
 * Apply the systematic [23,12] Golay, then append an overall parity bit. */
uint32_t golay_24_12_encode(uint16_t info) {
    uint32_t golay23 = golay_23_12_encode(info);
    uint32_t parity  = (uint32_t)__builtin_parity(golay23);  /* GCC/Clang */
    return (golay23 << 1) | parity;
}

/* [24,12] Extended Golay decode. On success, writes the 12 info bits to
 * *info_bits and returns the number of bits corrected (0..3). Returns -1
 * if uncorrectable or if the overall parity check fails (4 errors detected).
 * Strategy: strip the overall parity bit, decode as [23,12], then verify
 * overall parity. */
int golay_24_12_decode(uint32_t codeword, uint16_t *info_bits);
```

#### 2.4.2 [23,12] Golay Code

Applied to **u1 only** (not u2 — u2 is uncoded per Eq. 191). Same code as
full-rate (see Section 1.5.1).

#### 2.4.3 Uncoded Vectors (ĉ₂, ĉ₃)

Per Eq. 191–192, û₂ (11 bits) and û₃ (14 bits) pass through with no FEC
and no PN modulation: `ĉ₂ = û₂`, `ĉ₃ = û₃`. This is the single largest
implementation gotcha for half-rate — see `analysis/halfrate_fec_layout.md`.

### 2.5 PN Sequence -- Half-Rate

Same linear congruential generator as full-rate, but only 24 values (indices 0..23):

```
p_r(0) = 16 * u0
p_r(n) = (173 * p_r(n-1) + 13849) mod 65536    for 1 <= n <= 23
```

The derived modulation vector `m̂₁` is applied **only to ĉ₁** (BABA-A
Eq. 190). ĉ₀, ĉ₂, and ĉ₃ are not PN-modulated.

### 2.6 Interleaving -- Half-Rate (Annex S)

The 72 FEC-encoded bits are interleaved across 36 dibit symbols. Annex S provides
the complete mapping. From the extraction, the first 6 symbols are:

| Symbol | Bit 1 (MSB) | Bit 0 (LSB) |
|--------|------------|------------|
| 0 | c0(23) | c0(5) |
| 1 | c1(10) | c2(3) |
| 2 | c0(22) | c0(4) |
| 3 | c1(9)  | c2(2) |
| 4 | c0(21) | c0(3) |
| 5 | c1(8)  | c2(1) |

The pattern continues for all 36 symbols. Note the interleaving distributes c0 bits
primarily to the MSB lane (strongest protection in QPSK) and interleaves c1, c2, c3
bits across both lanes.

```c
/* Deinterleave a 72-bit half-rate AMBE frame (36 dibits) into 4 code vectors.
 * dibits: 36 elements in 0..=3. c_out layout (per BABA-A §14.2 Eq. 189–192):
 *   c_out[0]: 24 bits ([24,12] extended Golay of û₀)
 *   c_out[1]: 23 bits ([23,12] Golay of û₁, PN-modulated)
 *   c_out[2]: 11 bits (uncoded û₂)
 *   c_out[3]: 14 bits (uncoded û₃)
 * Total: 24 + 23 + 11 + 14 = 72 bits.
 * Full table is in TIA-102.BABA-A Annex S and TIA-102.BBAC-1 Annex E. */
void deinterleave_ambe_halfrate(const uint8_t dibits[36], uint32_t c_out[4]);
```

### 2.7 Half-Rate Spectral Amplitude Enhancement

Source: BABA-A §15 "Half-Rate Spectral Amplitude Enhancement", page 72.

**The half-rate decoder uses the full-rate enhancement procedure verbatim.**
BABA-A §15 is a one-paragraph pointer back to §8 (Eq. 105–111) — same
formulas, same constants, same guardrail branches, same energy-preserving
rescale, same S_E recurrence with floor 10000.

See §1.10 of this spec for the complete algorithm and C reference. No
parameter changes apply when using it for half-rate.

**Prerequisite:** enhancement consumes the reconstructed `M̃_l` produced by
the dequantization pipeline. That pipeline (gain → PRBA → HOC → per-block
inverse DCT → log-magnitude prediction) is specified in §2.11–§2.13 below.
Run those first; then §2.7 applies.

### 2.8 Half-Rate Adaptive Smoothing, Frame Repeat / Mute

Source: BABA-A §14.5 "Error Estimation" (Eq. 196–197), §14.6 "Frame Repeats"
(Eq. 198–205), §14.7 "Frame Muting", pages 70–71.

The structural pattern matches full-rate §1.11 but uses **different trigger
thresholds and a different error-rate recurrence**. V/UV smoothing
(Eq. 112–116) and amplitude smoothing are also applied — same formulas as
full-rate §1.11.3, using the half-rate S_E and error parameters.

#### 2.8.1 Error Parameters (Eq. 196–197)

Count corrected bits per Golay codeword as `ε₀` (c̃₀ = [24,12] extended
Golay) and `ε₁` (c̃₁ = [23,12] Golay). The half-rate recurrence is:

```
ε_T = ε₀ + ε₁                                                      (Eq. 196)
ε_R(0) = 0.95 · ε_R(−1) + 0.001064 · ε_T                           (Eq. 197)
```

Initial `ε_R(−1) = 0`. Note the weight **0.001064** is specific to half-rate
(full-rate uses 0.05 in Eq. 77-like recurrences and a different derivation
for ε_R; half-rate's coefficient comes from 0.05/47 ≈ 0.001064 scaled by
the different frame size).

#### 2.8.2 Frame Repeat Trigger (Eq. 198–199)

Trigger a frame repeat if **any** hold:

```
b̃₀ ∈ [120, 123]                (erasure frame type — see §13.1)
ε₀ ≥ 4
ε₀ ≥ 2   AND   ε_T ≥ 6          (Eq. 198 + Eq. 199 joint)
```

On repeat, copy forward Eq. 200–205:

```
ω̃₀(0) = ω̃₀(−1)                                                   (Eq. 200)
L̃(0)  = L̃(−1)                                                    (Eq. 201)
K̃(0)  = K̃(−1)                                                    (Eq. 202)
ṽ_l(0) = ṽ_l(−1)   for 1 ≤ l ≤ L̃(−1)                              (Eq. 203)
M̃_l(0) = M̃_l(−1)  for 1 ≤ l ≤ L̃(−1)                              (Eq. 204)
M̄_l(0) = M̄_l(−1)  for 1 ≤ l ≤ L̃(−1)                              (Eq. 205)
```

Note half-rate copies `ṽ_l` (per-harmonic) directly — the codebook-based
V/UV expansion (§2.3.6 Eq. 149) produces per-harmonic values, so the
repeat copies those directly rather than per-band values as in full-rate
(Eq. 102).

#### 2.8.3 Frame Mute Trigger (§14.7)

Mute if **either** hold:

```
ε_R(0) > 0.096
4 consecutive voice frames have triggered frame repeat
```

On mute, run Eq. 200–205 to preserve state, bypass the synthesizer
(§2.10), and emit random low-amplitude noise (or silence) as `s̃(n)`.

#### 2.8.4 V/UV Smoothing

Runs the same §1.11.3 algorithm with half-rate's S_E (from §2.7) and
ε_R(0) from §2.8.1. V_M thresholds per Eq. 112 unchanged.

### 2.9 Half-Rate Speech Synthesis

Source: BABA-A §13.4 closing paragraph ("reconstructed spectral amplitudes
M̃_l are then used by the synthesis algorithm, as described in Chapter 11"),
page 66.

**The half-rate decoder uses the full-rate synthesis pipeline verbatim.**
Eq. 117–141 from full-rate §1.12 apply unchanged to half-rate, using:

- `ω̃₀`, `L̃`, `K̃` from §1.3.1 / §12.8 (half-rate pitch Annex L)
- `ṽ_l` from §2.3.6 Eq. 149 (not §1.3.2 — half-rate uses codebook expansion)
- `M̄_l` from §2.7 enhancement + §2.8.4 smoothing
- All cross-frame state per §1.13 (synthesizer state table applies to both
  rates; the only differences are in how parameters are *recovered*, not in
  how the synthesizer *uses* them)

See §1.12.1 (unvoiced) and §1.12.2 (voiced) for the complete algorithm.

### 2.10 Tone Frame Synthesis

Source: BABA-A §16 "Tone Frames", pages 73–77, Tables 19–20 and
Eq. 206–209.

Half-rate supports tone-frame transmission (DTMF, KNOX, single-frequency,
call progress, silence). When `b̃₀ ∈ [126, 127]` the frame is a Tone Frame,
parsed per Table 20 and synthesized using the parameters in
`annex_tables/annex_t_tone_params.csv` (§12.16). The tone-frame bit layout
is **different from voice-frame layout** (Tables 15–18 of §2.3 do not
apply); when parsing a half-rate frame, dispatch on `b̃₀` first, then
apply either the voice pipeline (§2.11–§2.13) or the tone pipeline (§2.10).

#### 2.10.1 Tone Frame Identification and Bit Field Layout (Table 20)

Source: BABA-A §16.2, page 74 text and Table 20 (page 75).

**Identification (per §16.2 opening paragraph, page 74):** the first 6 bits
of û₀ are always set to `63` (decimal) = `[1, 1, 1, 1, 1, 1]` for a tone
frame. Those 6 bits occupy û₀(11)..û₀(6), which per Table 15 of §2.3
corresponds to b̂₀(6..3) + b̂₁(4..3). Forcing those to `111111` places
b̂₀ in the range `120 ≤ b̂₀ ≤ 127` **and** forces the two MSBs of b̂₁ to `11`.
Combined with §13.1 Table 14, `b̂₀ ∈ {126, 127}` unambiguously identifies
a tone frame (120–125 are erasure/silence, not tone).

**Bit field layout (Table 20 of BABA-A, page 75):**

| Bit vector / range      | Content                            |
|-------------------------|------------------------------------|
| û₀(11, 10, 9, 8, 7, 6)  | `63` (fixed — tone-frame signature)|
| û₀(5, 4, 3, 2, 1, 0)    | `A_D(6, 5, 4, 3, 2, 1)` (amplitude MSBs) |
| û₁(11, 10, 9, 8, 7, 6, 5, 4) | `I_D(7, 6, 5, 4, 3, 2, 1, 0)` — tone ID, **copy 1 (full 8 bits)** |
| û₁(3, 2, 1, 0)          | `I_D(7, 6, 5, 4)` — tone ID, copy 2 (MSB nibble) |
| û₂(10, 9, 8, 7)         | `I_D(3, 2, 1, 0)` — tone ID, copy 2 (LSB nibble) |
| û₂(6, 5, 4, 3, 2, 1, 0) | `I_D(7, 6, 5, 4, 3, 2, 1)` — tone ID, copy 3 (top 7 bits) |
| û₃(13)                  | `I_D(0)` — tone ID, copy 3 (LSB) |
| û₃(12, 11, 10, 9, 8, 7, 6, 5) | `I_D(7, 6, 5, 4, 3, 2, 1, 0)` — tone ID, **copy 4 (full 8 bits)** |
| û₃(4)                   | `A_D(0)` — amplitude LSB |
| û₃(3, 2, 1, 0)          | `0000` (fixed) |

Totals check: `6 + 6 + 8 + 4 + 4 + 7 + 1 + 8 + 1 + 4 = 49 bits = 12 + 12 + 11 + 14` ✓.

**Redundancy:** the 8-bit tone index `I_D` is transmitted **4 times** —
copies 1 and 4 are contiguous 8-bit blocks; copies 2 and 3 are split
across bit-vector boundaries. The redundancy lets the decoder majority-vote
or discard invalid copies under channel errors. Copies 1 and 4 are the
most robust (each lives entirely within one prioritized-bits vector and
benefits from that vector's FEC or uncoded-but-Annex-S-interleaved
placement).

**Amplitude:** `A_D` is a 7-bit log-amplitude, `0 ≤ A_D ≤ 127`, with:
- `A_D = 127` → +3.17 dBm0 (max sinusoidal input level at the A-to-D)
- `A_D = 0` → −87.13 dBm0
- Step size: **0.711 dB** (≡ `0.03555 · 20 dB`; matches Eq. 209)

#### 2.10.2 Tone Frame Parsing (C Reference)

```c
/* Parse a half-rate Tone Frame. Call only when ba0 in [126, 127] and the
 * first 6 bits of u_hat[0] equal 0x3F (signature check). Returns 0 on
 * success, -1 if the signature or the fixed LSBs of û₃ don't match
 * (treat as erasure per §16.3 final paragraph).
 *
 * Inputs:
 *   u_hat[0..3]: prioritized bit vectors, widths 12/12/11/14, LSB = bit 0.
 *
 * Outputs:
 *   *tone_id:   8-bit I_D (0..255)
 *   *amplitude: 7-bit A_D (0..127)
 */
static inline uint8_t extract_bits(uint16_t v, uint8_t hi, uint8_t lo) {
    return (uint8_t)((v >> lo) & ((1u << (hi - lo + 1)) - 1u));
}

int p25_tone_frame_parse(const uint16_t u_hat[4],
                         uint8_t *tone_id,
                         uint8_t *amplitude)
{
    /* Signature: û₀(11..6) == 0x3F (63 decimal) */
    if (extract_bits(u_hat[0], 11, 6) != 0x3F)
        return -1;

    /* Fixed trailer: û₃(3..0) == 0 */
    if (extract_bits(u_hat[3], 3, 0) != 0)
        return -1;

    /* I_D copy 4 — û₃(12..5), 8 contiguous bits, uncoded — use as primary */
    uint8_t id = extract_bits(u_hat[3], 12, 5);

    /* Optional: majority-vote against copy 1 (û₁(11..4)) for robustness.
     * Copy 1 is inside ĉ₁ which carries [23,12] Golay + PN — it's the
     * most error-resilient of the four. Reference impls typically pick
     * copy 1 (or the first copy that passes the Annex-T valid-range
     * check) as primary; copy 4 is a good secondary. */
    /* uint8_t id_copy1 = extract_bits(u_hat[1], 11, 4); */

    /* A_D: 6 MSBs in û₀(5..0), LSB in û₃(4).
     * Per Table 20, û₀(5..0) carries A_D(6..1), so shift left by 1. */
    uint8_t ad_hi = extract_bits(u_hat[0], 5, 0);            /* A_D bits 6..1 */
    uint8_t ad_lo = extract_bits(u_hat[3], 4, 4);            /* A_D bit 0    */
    uint8_t ad    = (uint8_t)((ad_hi << 1) | ad_lo);         /* 7-bit A_D    */

    *tone_id   = id;
    *amplitude = ad;
    return 0;
}
```

**Validity check:** after extraction, test `*tone_id` against Table 19
reserved ranges (0–4, 123–127, 164–254). Invalid → erasure frame,
frame-repeat per §2.8.3 / BABA-A §16.3 final paragraph. `I_D = 255` →
silence (synthesize zero output regardless of `A_D`).

#### 2.10.3 Tone Synthesis via MBE Bridge (Eq. 206–209)

Source: BABA-A §17 Eq. 206–209, page 77. (Originally scoped under
parametric rate conversion; the same equations are the canonical way
to synthesize a tone frame as MBE parameters for the §1.12 synthesizer,
because §16.3 states the decoder replaces the Chapter 11 voice signal
with a tone signal for the current frame but does not inline a standalone
synthesizer — Eq. 206–209 is the only bit-exact specification of the
conversion from `(I_D, A_D)` to MBE parameters in the standard.)

**Step 1 — Annex T lookup (§12.16):**

```
(f_0, l_1, l_2) = ANNEX_T[I_D]
```

For single-frequency tones (IDs 5–122), `f_0` is the fundamental and
`l_1 = l_2` is the same harmonic index (one non-zero harmonic). For
DTMF (128–143), KNOX (144–159), and Call Progress (160–163),
`f_0 = gcd(f_1, f_2)` and `(l_1, l_2)` are the two harmonic indices
(two non-zero harmonics). For ID 255, the CSV lists `(f_0, l_1, l_2) =
(250, 0, 0)`; combined with `A_D → 0` override below, synthesizes
silence.

**Step 2 — MBE fundamental and harmonic count (Eq. 206–207):**

```
ω̃₀ = (2π / 8000) · f_0                                              (Eq. 206)
L̃  = ⌊ 3812.5 / f_0 ⌋                                               (Eq. 207)
```

Note the `8000` sample-rate assumption (the vocoder operates at 8 kHz
throughout) and the `3812.5 Hz` upper frequency cutoff.

**Step 3 — Voicing and spectral amplitudes (Eq. 208–209):**

```
ṽ_l = { 1   if l = l_1 or l = l_2
     { 0   otherwise                                                  (Eq. 208)

M̃_l = { 16384 · 10^{ 0.03555 · (A_D − 127) }   if l = l_1 or l = l_2
     { 0                                        otherwise              (Eq. 209)
```

All non-tone harmonics are silenced. The `16384 · 10^{0.03555(A_D − 127)}`
formula encodes the 0.711 dB/step amplitude scale (with `A_D = 127`
corresponding to the maximum spectral amplitude used by the MBE
synthesizer). For `I_D = 255` (silence), override `M̃_l = 0` for all `l`
regardless of `A_D`.

**Step 4 — MBE synthesis:** feed `(ω̃₀, L̃, ṽ_l, M̃_l)` into the standard
MBE synthesizer per §1.12 — voiced synthesis (§1.12.2) handles the
`ṽ_l = 1` harmonics, unvoiced synthesis (§1.12.1) processes the empty
unvoiced set. Cross-frame state (`ω̃₀(−1)`, `ψ_l(−1)`, etc.) is updated
normally per §1.13. Tone frames therefore plug into the same synthesis
pipeline as voice frames — no separate OLA sine generator is required
by the spec.

**Alternative direct synthesis.** Some implementations bypass the MBE
bridge and directly generate `s̃(n) = G · w_S(n) · [sin(2π·f_1·n/8000) +
sin(2π·f_2·n/8000)]` with `f_2` omitted for single-frequency tones and
`G` derived from `A_D`. This is permitted (§16.3 only constrains the
output, not the synthesis method), but it sits outside the normative
equations and will not bit-match a reference implementation that uses
Eq. 206–209. Prefer the MBE bridge unless there's a reason not to.

**Tone-frame FEC:** per §16.2 final paragraph (page 75), tone frames use
**the same FEC, PN modulation, and interleaving as voice frames** (§14.2
Eq. 189–192, §14.3 PN, §14.4 Annex S). The tone-specific layout applies
only *after* deinterleave and FEC decode have recovered `û₀..û₃`.

### 2.11 PRBA Decomposition (Gain + First Two DCT Coefficients per Block)

Source: BABA-A §13.4.1 Eq. 168 (gain) and §13.4.2 Eq. 169–178 (PRBA),
pages 63–64. Table 12 (page 57) gives the bit allocation consumed here:
b̃₂ (gain, 5 bits), b̃₃ (PRBA24, 9 bits), b̃₄ (PRBA58, 7 bits).

**Stage 1 — differential gain recovery (Eq. 168):**

```
Δ̃_γ = AMBE_GAIN_QUANTIZER[b̃₂]                 (Annex O, 32 entries, §12.11)
γ̃(0) = Δ̃_γ + 0.5 · γ̃(−1)                                           (Eq. 168)
```

γ̃(−1) is the reconstructed gain from the last **voice** frame (tone,
silence, and erasure frames do not update it); initial value 0.

**Stage 2 — transformed PRBA codebook lookup:**

```
G̃_1            = 0                           (discarded at encoder per §13.3.1)
G̃_2, G̃_3, G̃_4  = AMBE_PRBA24_CODEBOOK[b̃₃]    (Annex P, 512×3 entries, §12.12)
G̃_5..G̃_8       = AMBE_PRBA58_CODEBOOK[b̃₄]    (Annex Q, 128×4 entries, §12.13)
```

**Stage 3 — inverse 8-point DCT on the transformed PRBA vector (Eq. 169–170):**

```
R̃_i = Σ_{m=1}^{8} α(m) · G̃_m · cos[ π·(m−1)·(i − 0.5) / 8 ]
                                                    for 1 ≤ i ≤ 8    (Eq. 169)
α(1) = 1,  α(m) = 2  for m > 1                                       (Eq. 170)
```

This is a fixed 8-point DCT — the basis length is always 8, independent
of L̃. Matches the structure of §1.8.3's Eq. 69 (a fixed 6-point DCT once
the BABA-A typo is corrected per §1.8.3's Spec Bug note) and the encoder's
Eq. 60/61. The per-block `J̃_i`-length DCT appears only at the inner
spectral-residual reconstruction stage — §1.8.3's Eq. 73 (full-rate HOC
inverse DCT) and §2.13's Eq. 183 (half-rate HOC inverse DCT). The same
asymmetric-forward/inverse caveat noted in §1.8.3 applies: this is not
an orthonormal DCT-II / DCT-III pair.

**Stage 4 — pair-wise split into per-block (mean, first non-mean DCT coef)
(Eq. 171–178):**

The 8-element PRBA vector R̃ encodes both the block mean **and** the first
non-mean DCT coefficient of each of the four blocks, packed as a sum/diff
with √2 weighting on the non-mean element:

```
C̃_{1,1} = (R̃_1 + R̃_2) / 2                                         (Eq. 171)
C̃_{1,2} = (√2 / 4) · (R̃_1 − R̃_2)                                  (Eq. 172)
C̃_{2,1} = (R̃_3 + R̃_4) / 2                                         (Eq. 173)
C̃_{2,2} = (√2 / 4) · (R̃_3 − R̃_4)                                  (Eq. 174)
C̃_{3,1} = (R̃_5 + R̃_6) / 2                                         (Eq. 175)
C̃_{3,2} = (√2 / 4) · (R̃_5 − R̃_6)                                  (Eq. 176)
C̃_{4,1} = (R̃_7 + R̃_8) / 2                                         (Eq. 177)
C̃_{4,2} = (√2 / 4) · (R̃_7 − R̃_8)                                  (Eq. 178)
```

**Invertibility check:** Eq. 172 weight `√2/4 = 1/(2√2)` is consistent
with the encoder Eq. 159–160 weight of `√2`: plug
`R̂_{2i−1} = Ĉ_{i,1} + √2·Ĉ_{i,2}` and `R̂_{2i} = Ĉ_{i,1} − √2·Ĉ_{i,2}`
into Eq. 171–172 and confirm the identity maps round-trip.

At the end of §2.11 the decoder has populated `C̃_{i,k}` for **k = 1, 2**
only. DCT coefficients at k ≥ 3 come from the HOC codebook (§2.12).

### 2.12 HOC Placement

Source: BABA-A §13.4.3 "Decoding the Higher Order DCT Coefficients",
Eq. 179, page 64. Encoder side is §13.3.2, page 62.

**HOC codebook lookups (Annex R):**

```
H̃_{1, 1..4}  = AMBE_HOC_B5[b̃₅]   (5 bits, 32 entries × 4-vec)    (§12.14)
H̃_{2, 1..4}  = AMBE_HOC_B6[b̃₆]   (4 bits, 16 entries × 4-vec)
H̃_{3, 1..4}  = AMBE_HOC_B7[b̃₇]   (4 bits, 16 entries × 4-vec)
H̃_{4, 1..4}  = AMBE_HOC_B8[b̃₈]   (3 bits,  8 entries × 4-vec)
```

**Placement rule:**

```
C̃_{i, k} = H̃_{i, k−2}     for 3 ≤ k ≤ min(J̃_i, 6)
C̃_{i, k} = 0                for k ≥ 7, or k > J̃_i
```

This is the encoder-consistent reading (§13.3.2 defines
`H̄_{i,j} = Ĉ_{i, j+2}` for `1 ≤ j ≤ J_i − 2`, 4-entry codebook → HOC
fills `Ĉ_{i, 3..6}`). Confirmed against DVSI reference vectors at
current calibration level — see
[`analysis/vocoder_decode_disambiguations.md`](../../analysis/vocoder_decode_disambiguations.md)
§12 for the resolution log.

That is: HOC values fill DCT coefficients **k = 3..6** of each block
(coefficient k=2 comes from PRBA §2.11; k=1 is the block mean, also
from PRBA). Coefficients beyond k=6 — and any coefficients beyond
the block's actual length J̃_i — are zero-fill.

**Lossy by design (not a spec gap):** for L̃ near 56, Annex N gives
block lengths up to J̃_i = 17, so a block can carry up to 16 non-mean
DCT coefficients but the HOC codebook reconstructs at most 4 (plus 1
from PRBA, giving 5). The remaining 11+ are truncated to zero. This is
the lossy-compression point of the half-rate vocoder and is intentional;
do not treat it as a bug.

**Printed Eq. 179 bounds typo (for the record):**

The PDF prints Eq. 179 as `for 2 ≤ k ≤ J̃_i AND k ≤ 4` with
`C̃_{i,k} = H̃_{i, k−2}`. Taken literally, this would overwrite the
PRBA-derived `C̃_{i, 2}` and access out-of-range `H̃_{i, 0}` against
the 1-indexed 4-entry Annex R codebooks. The bounds are almost
certainly a draft-edit artifact: `2..4` should read `3..6`. Resolved
in favor of the encoder-consistent reading above; half-rate decode-PCM
under that reading correlates with DVSI `p25a.bit → p25a.pcm` reference
output at the current calibration level without contradiction. Full
rationale and test outcome are logged in
[`analysis/vocoder_decode_disambiguations.md`](../../analysis/vocoder_decode_disambiguations.md)
§12.

### 2.13 Per-Block Inverse DCT and Log-Magnitude Reconstruction

Source: BABA-A §13.4.3 Eq. 180–181 (per-block inverse DCT) and §13.4.4
Eq. 182–188 (log-magnitude reconstruction), pages 64–65.

**Per-block inverse DCT (Eq. 180–181):**

Once §2.11 + §2.12 have populated `C̃_{i, k}` for 1 ≤ k ≤ J̃_i (with
zero-fill for k > min(J̃_i, 6) and zero for any block with J̃_i < 1):

```
c̃_{i, j} = Σ_{k=1}^{J̃_i} α(k) · C̃_{i, k} · cos[ π·(k−1)·(j − 0.5) / J̃_i ]
                                                    for 1 ≤ j ≤ J̃_i   (Eq. 180)
α(1) = 1,  α(k) = 2  for k > 1                                        (Eq. 181)
```

This is **exactly §1.8.4 Eq. 73–74 with 4 blocks (i = 1..4) instead
of 6**. No coefficient changes. The same asymmetric-forward/inverse
caveat from §1.8.3 applies — see
[`analysis/vocoder_decode_disambiguations.md`](../../analysis/vocoder_decode_disambiguations.md)
§9 for the round-trip pairing.

Concatenate the four blocks into `T̃_l` for 1 ≤ l ≤ L̃, where block i
covers `l = Σ_{n=1}^{i−1} J̃_n + 1 .. Σ_{n=1}^{i} J̃_n` (J̃_i taken from
Annex N keyed on L̃ — see §12.10 / `annex_n_block_lengths.csv`).

**Log-magnitude prediction reconstruction (Eq. 182–187):**

```
k̃_l = (L̃(−1) / L̃(0)) · l                                           (Eq. 182)
δ̃_l = k̃_l − ⌊k̃_l⌋                                                  (Eq. 183)

Γ̃   = γ̃(0) − 0.5·log₂(L̃) − (1/L̃) · Σ_{λ=1}^{L̃} T̃_λ                (Eq. 184)

Λ̃_l(0) = T̃_l
       + 0.65·(1 − δ̃_l) · Λ̃_{⌊k̃_l⌋}(−1)
       + 0.65·δ̃_l       · Λ̃_{⌊k̃_l⌋+1}(−1)
       − (0.65 / L̃(0)) · Σ_{λ=1}^{L̃(0)} [
             (1 − δ̃_λ) · Λ̃_{⌊k̃_λ⌋}(−1)
           + δ̃_λ       · Λ̃_{⌊k̃_λ⌋+1}(−1) ]
       + Γ̃                                                           (Eq. 185)
```

Edge cases (Eq. 186–187):

```
Λ̃_0(−1)    = Λ̃_1(−1)
Λ̃_l(−1)    = Λ̃_{L̃(−1)}(−1)       for l > L̃(−1)
```

**Initialization (first frame):** `Λ̃_l(−1) = 1 for all l`,
`L̃(−1) = 15`. Note the different initial L̃ compared to full-rate
(30, §1.8.5) — half-rate uses 15.

**Structural note:** Eq. 185 is **§1.8.5 Eq. 77 with the addition of
the `+ Γ̃` intercept term from Eq. 184**. Full-rate carries the gain
γ̃ inside the dequantized C̃_{i,1} via §1.8.3's Eq. 68 substitution;
half-rate separates γ̃ out and applies it as a scalar offset in
Eq. 184, which is then added back to every Λ̃_l.

**Prediction-gain coefficient differs between rates.** Half-rate
Eq. 185 (BABA-A page 65) uses the literal constant `0.65` shown here.
Full-rate Eq. 77 uses a symbolic `ρ` defined by BABA-A Eq. 55 (page 25)
as a piecewise-linear function of `L̃(0)` ranging over `[0.4, 0.7]` —
**not** the constant 0.65. (Earlier drafts cross-referenced "Eq. 200
on page 61" — that was a mis-citation; Eq. 200–205 is the half-rate
frame-repeat copy-forward block, not the predictor.) See
[`analysis/vocoder_decode_disambiguations.md`](../../analysis/vocoder_decode_disambiguations.md)
§3 for the correction history; §1.8.5 of this spec carries the
corrected full-rate schedule.

**Final spectral amplitudes (Eq. 188):**

```
M̃_l = {  exp(0.693 · Λ̃_l)                  if ṽ_l = 1 (voiced)
      {  (0.2046 / √ω̃₀) · exp(0.693 · Λ̃_l)  otherwise (Eq. 188)
```

where `ṽ_l` is the per-harmonic V/UV decision from §2.3.6 Eq. 149 and
0.693 ≈ ln 2 (converting from log₂ domain to natural scale). **The
unvoiced branch's `(0.2046 / √ω̃₀)` scale factor is half-rate-specific**
and does not appear in full-rate's Eq. 79 — full-rate applies no
voicing-dependent rescale at this stage. Implementations that share
code between full-rate and half-rate must branch here.

Save `M̃_l` for §2.7 enhancement, §2.8 smoothing, and §2.9 synthesis.
Also save `Λ̃_l(0)` and `L̃(0)` as `Λ̃_l(−1)` and `L̃(−1)` for the next
voice frame (tone/silence/erasure frames do not update this state —
see §13.3 opening paragraph, page 60).

---

## 3. Voice Frame Placement in Air Interface Frames

### 3.1 FDMA: IMBE Frames in LDU1/LDU2

Each LDU (Logical Link Data Unit) carries **9 IMBE voice frames**, each 144 bits
(72 dibits) after FEC encoding.

- **LDU1** (DUID 0x5): carries IMBE frames 1-9 + Link Control (LC)
- **LDU2** (DUID 0xA): carries IMBE frames 10-18 + Encryption Sync (ES)
- **LDU duration:** 180 ms (9 x 20 ms)
- **LDU pair:** 360 ms, 18 voice frames total

#### 3.1.1 LDU1 Voice Frame Start Positions

| IMBE Frame | Approx Start Symbol | Approx End Symbol | Notes |
|------------|-------------------|------------------|-------|
| 1 | 57 | ~130 | After FS (48 bits) + NID |
| 2 | 131 | ~225 | |
| 3 | 226 | ~319 | |
| 4 | 320 | ~414 | |
| 5 | 415 | ~509 | |
| 6 | 510 | ~603 | |
| 7 | 604 | ~698 | |
| 8 | 699 | ~787 | |
| 9 | 788 | ~863 | Followed by LSD |

#### 3.1.2 LDU2 Voice Frame Start Positions

| IMBE Frame | Approx Start Symbol | Approx End Symbol |
|------------|-------------------|------------------|
| 10 | 921 | ~994 |
| 11 | 995 | ~1089 |
| 12 | 1090 | ~1183 |
| 13 | 1184 | ~1278 |
| 14 | 1279 | ~1373 |
| 15 | 1374 | ~1467 |
| 16 | 1468 | ~1562 |
| 17 | 1563 | ~1652 |
| 18 | 1653 | ~1727 |

#### 3.1.3 Interleaving Between Voice and Signaling

Within each LDU, voice frames and signaling (LC or ES) bits alternate:

```
[IMBE frame N] [Hamming(10,6,3) signaling words] [IMBE frame N+1]
```

The signaling words carry 6-bit fragments of the Link Control word (LDU1) or
Encryption Sync word (LDU2). Status symbols are inserted every 35th symbol.

**CRITICAL:** The exact bit-by-bit interleaving table for LDU1/LDU2 (Annex A.3/A.4
of TIA-102.BAAA-B) was NOT fully extracted from the PDF. These tables (864 entries
each for LDU1 and LDU2) must be programmatically extracted from the BAAA-B PDF
annex pages. See `P25_FDMA_Common_Air_Interface_Implementation_Spec.md` for the
BAAA-B extraction status.

#### 3.1.4 Voice Superframe Structure

```
HDU -> LDU1 -> LDU2 -> LDU1 -> LDU2 -> ... -> TDU/TDULC
       |<--- 360 ms --->|
       |<-180ms->|<-180ms->|
```

Each LDU pair = 360 ms = 18 IMBE voice frames. Repeats until call termination.

### 3.2 TDMA: AMBE Frames in 4V and 2V Bursts

In Phase 2 TDMA, half-rate AMBE frames are carried in voice traffic channel (VTCH)
bursts. Each 30 ms timeslot = 180 dibit symbols.

#### 3.2.1 4V Burst (4 Voice Frames)

A 4V burst carries **4 half-rate AMBE voice frames** (4 x 72 = 288 voice bits)
within a single 180-symbol timeslot:

```
[Guard/Ramp: 6 sym] [Pilot P1: 4 sym] [Voice 1: 36 sym] [DUID: 1 sym]
[Voice 2: 36 sym] [DUID: 1 sym] [ESS-B: 12 sym] [Voice 3: 36 sym]
[DUID-parity: 1 sym] [Voice 4: 36 sym] [DUID-parity: 1 sym]
[Pilot P2: 4 sym] [Guard: 6 sym]
```

The 4 voice frames each occupy 36 consecutive dibit symbols with interleaved
signaling (DUID, ESS-B) between them. From BBAC-1 Annex E Table E-1:

- **Voice 1:** symbols 10-45 (36 symbols, c0..c3 interleaved per Annex S)
- **Voice 2:** symbols 47-82 (36 symbols)
- **Voice 3:** symbols 96-131 (36 symbols, after ESS-B at symbols 84-95)
- **Voice 4:** symbols 133-168 (36 symbols)

Each voice frame's 36 dibits map to c0..c3 codeword bits using the identical
interleaving pattern (Annex S of BABA-A). The pattern repeats identically for each
of the 4 voice positions -- the bit-to-symbol mapping within each 36-symbol block
is the same.

#### 3.2.2 2V Burst (2 Voice Frames)

A 2V burst carries **2 half-rate AMBE voice frames** plus FACCH or SACCH data.
From BBAC-1 Annex E Table E-3:

- **Voice 1:** symbols 10-45 (36 symbols)
- **Voice 2:** symbols 47-82 (36 symbols)
- Remaining symbols carry data (FACCH/SACCH)

The voice frame interleaving within each 36-symbol block is identical to 4V.

#### 3.2.3 TDMA Voice Frame Extraction

```c
/* Extract 4 voice frame dibit-arrays from a 4V burst (180 symbols). */
void extract_4v_voice_frames(const uint8_t slot[180], uint8_t frames[4][36]) {
    memcpy(frames[0], &slot[10],  36);   /* Voice 1: symbols 10..45 */
    memcpy(frames[1], &slot[47],  36);   /* Voice 2: symbols 47..82 */
    memcpy(frames[2], &slot[96],  36);   /* Voice 3: symbols 96..131 */
    memcpy(frames[3], &slot[133], 36);   /* Voice 4: symbols 133..168 */
}

/* Extract 2 voice frame dibit-arrays from a 2V burst (180 symbols). */
void extract_2v_voice_frames(const uint8_t slot[180], uint8_t frames[2][36]) {
    memcpy(frames[0], &slot[10], 36);
    memcpy(frames[1], &slot[47], 36);
}
```

### 3.3 Timing Relationships

| Context | Voice Frames | Duration | Air Interface Unit |
|---------|-------------|----------|--------------------|
| FDMA LDU1 | 9 IMBE (144-bit) | 180 ms | 864 dibits |
| FDMA LDU2 | 9 IMBE (144-bit) | 180 ms | 864 dibits |
| FDMA LDU pair | 18 IMBE | 360 ms | 1728 dibits |
| TDMA 4V burst | 4 AMBE (72-bit) | 4 x 20 ms voice in 30 ms slot | 180 symbols |
| TDMA 2V burst | 2 AMBE (72-bit) | 2 x 20 ms voice in 30 ms slot | 180 symbols |
| TDMA superframe | Up to 20 AMBE per LCH | 360 ms (12 slots) | 5 VTCH slots/LCH |

---

## 4. FEC for Voice Frames -- Decode Pipeline

### 4.1 Full-Rate IMBE Decode Pipeline

```
Step 1: Deinterleave
  144 transmitted bits (72 dibits) -> c0[23], c1[23], c2[23], c3[23],
                                      c4[15], c5[15], c6[15], c7[7]

Step 2: Decode c0 (Golay)
  [23,12] Golay decode c0 -> u0 (12 info bits)
  Record error count e0

Step 3: Extract pitch, generate PN mask
  b0 = u0[bits 0..7]  (8-bit pitch index)
  Generate PN sequence: p_r(0) = 16 * u0, LCG mod 65536
  Compute masks m1..m7 from p_r values

Step 4: PN demodulate
  v_i = c_i XOR m_i    for i = 1..7

Step 5: FEC decode remaining vectors
  v1 -> [23,12] Golay -> u1, error count e1
  v2 -> [23,12] Golay -> u2, error count e2
  v3 -> [23,12] Golay -> u3, error count e3
  v4 -> [15,11] Hamming -> u4, error count e4
  v5 -> [15,11] Hamming -> u5, error count e5
  v6 -> [15,11] Hamming -> u6, error count e6
  u7 = v7 (uncoded, 7 bits)

Step 6: Error assessment
  e_T = e0 + e1 + e2 + e3 + e4 + e5 + e6
  e_R = e_R * alpha + (1 - alpha) * (e_T / max_errors)  // smoothed rate
  Frame repeat if: e0 >= 2 AND e_T >= 10 + 40 * e_R
  Frame mute if: e_R > 0.0875

Step 7: Reconstruct voice parameters
  From L (derived from b0), use bit allocation tables to unpack:
  b0 (pitch), v_k (V/UV), b2 (gain), b3..b_{L+2} (spectral)
  -> Pass to IMBE codec for synthesis
```

### 4.2 Half-Rate AMBE Decode Pipeline

```
Step 1: Deinterleave
  72 transmitted bits (36 dibits) -> c0[24], c1[23], c2[23], c3[variable]

Step 2: Decode c0 (Extended Golay)
  [24,12] Extended Golay decode c0 -> u0 (12 info bits)
  Record error count e0

Step 3: Extract pitch, generate PN mask
  b0 = u0[bits 0..6]  (7-bit pitch index, 0..119)
  Generate PN sequence: p_r(0) = 16 * u0, 24 values

Step 4: PN demodulate
  v_i = c_i XOR m_i    for i = 1..3

Step 5: FEC decode
  v1 -> [23,12] Golay -> u1, error count e1
  v2 -> [23,12] Golay -> u2, error count e2
  u3 = v3 (uncoded)

Step 6: Error assessment
  e_T = e0 + e1 + e2
  Frame repeat if: e0 >= 4 OR e_T >= 6
  Frame mute if: e_R > 0.096 or 4 consecutive invalid frames
  Invalid frame: b0 in [208..215], [220..255], or [216..219]

Step 7: Reconstruct voice parameters
  b0 (7 bits) -> Annex L pitch table -> (L, omega_0)
  b1 (5 bits) -> Annex M V/UV codebook -> 8-element V/UV vector
  b2 (5 bits) -> Annex O gain quantizer -> differential gain
  b3 (9 bits) -> Annex P PRBA24 VQ -> (G2, G3, G4)
  b4 (7 bits) -> Annex Q PRBA58 VQ -> (G5, G6, G7, G8)
  b5 (5 bits) -> Annex R HOC VQ -> (H1,1..H1,4)
  b6 (4 bits) -> Annex R HOC VQ -> (H2,1..H2,4)
  b7 (4 bits) -> Annex R HOC VQ -> (H3,1..H3,4)
  b8 (3 bits) -> Annex R HOC VQ -> (H4,1..H4,4)
  -> Pass to AMBE codec for synthesis
```

---

## 5. Tone Signaling

### 5.1 Half-Rate Tone Frames

The half-rate vocoder supports in-band tone signaling without a separate signaling
path. Tone frames are distinguished from voice frames by the tone ID byte I_D.

| Tone Type | I_D Range | Description |
|-----------|-----------|-------------|
| Reserved | 0-4 | Not used |
| Single-tone | 5-127 | Single-frequency tones at f0 = formula * I_D |
| DTMF | 128-143 | Two-frequency DTMF digits 0-9, A-D, *, # |
| KNOX | 144-163 | Proprietary tone set |
| Reserved | 164-254 | Not used |
| Silence | 255 | Silence frame indicator |

### 5.2 Tone Synthesis Parameters

Each tone entry maps to MBE synthesis parameters:

- **f0:** Fundamental frequency in Hz
- **l1, l2:** Spectral shaping indices for the two tone components

For single-frequency tones (I_D 5-122), the fundamental frequency is computed as:

| I_D Range | f0 Formula | l1=l2 |
|-----------|-----------|-------|
| 5-12 | 31.250 * I_D | 1 |
| 13-25 | 15.625 * I_D | 2 |
| 26-38 | 10.417 * I_D | 3 |
| 39-51 | 7.8125 * I_D | 4 |
| 52-64 | 6.2500 * I_D | 5 |
| 65-76 | 5.2803 * I_D | 6 |
| 77-89 | 4.4643 * I_D | 7 |
| 90-102 | 3.9063 * I_D | 8 |
| 103-115 | 3.4722 * I_D | 9 |
| 116-122 | 3.1250 * I_D | 10 |

### 5.3 DTMF Tone Mapping (Selected)

DTMF tones are entries 128-143 in Annex T, each with two-frequency MBE synthesis
parameters (f0, l1, l2) that produce the standard DTMF row/column frequencies.

Example: I_D=128 -> f0=78.5 Hz, l1=12, l2=17

### 5.4 Full-Rate Tone Handling

The full-rate vocoder does NOT define an explicit tone frame mechanism in BABA-A.
DTMF tones in Phase 1 FDMA are either:
- Encoded naturally by the IMBE vocoder (works for simple tones)
- Carried via in-band signaling in Link Control words
- Handled at the application layer

---

## 6. Silence Frames

### 6.1 Half-Rate Silence

Silence is indicated by tone ID **I_D = 255**:
- f0 = 250 Hz
- l1 = l2 = 0
- This produces a zero-energy MBE frame

When the decoder encounters I_D = 255, it should output silence (zero PCM samples)
or comfort noise, depending on implementation.

### 6.2 Full-Rate Silence

Full-rate silence is indicated implicitly through the adaptive smoothing mechanism:
- When the smoothed error rate e_R exceeds 0.0875, the frame is **muted** (output silence)
- During frame repeats, spectral amplitudes are progressively attenuated toward silence
- Explicit silence: all spectral amplitudes M_l near zero and all bands unvoiced

### 6.3 Frame Erasure/Muting Summary

| Condition | Full-Rate | Half-Rate |
|-----------|-----------|-----------|
| Frame repeat trigger | e0 >= 2 AND e_T >= 10 + 40*e_R | e0 >= 4 OR e_T >= 6 |
| Frame mute trigger | e_R > 0.0875 | e_R > 0.096 or 4 consecutive invalid |
| Repeat behavior | Attenuate previous M_l, fade to silence | Same principle |
| Mute behavior | Output silence | Output silence |

---

## 7. Quantization Tables

### 7.1 Tables Present in Extraction

**Full-Rate (complete):**
- Annex B: Analysis window wI(n), 301 values (n=−150..150) — see §12.6 below
- Annex C: Pitch refinement window wR(n), 221 values (n=−110..110) — see §12.18 below
- Annex D: FIR LPF coefficients, 21 values — inlined in §7.4 below
- Annex E: Gain quantizer (64 levels) — see §12.1 below
- Annex F: Gain bit allocation by L (9..56) — see §12.2 below
- Annex G: HOC bit allocation by L (9..56), variable width — see §12.3 below
- Annex H: Full-rate bit frame format / interleaving (72 symbols × 2 dibits) — see §12.4 below
- Annex I: Synthesis window wS(n), 211 values (n=−105..105) — see §12.7 below
- Annex J: Spectral block lengths J1..J6 by L — see §12.5 below

**Half-Rate (complete):**
- Annex L: Pitch quantization table (120 entries × 2 values: L, ω₀) — see §12.8 below
- Annex M: V/UV codebook (32 entries × 8 binary decisions) — see §12.9 below
- Annex N: Half-rate block lengths J1..J4 by L (48 rows) — see §12.10 below
- Annex O: Half-rate gain quantizer (32 levels) — see §12.11 below
- Annex P: PRBA24 VQ codebook (512 entries × 3 floats: G2, G3, G4) — see §12.12 below
- Annex Q: PRBA58 VQ codebook (128 entries × 4 floats: G5..G8) — see §12.13 below
- Annex R: HOC VQ codebooks (four sub-tables: 32+16+16+8 entries × 4 floats) — see §12.14 below
- Annex S: Half-rate interleaving (36 symbols × 4 columns) — see §12.15 below
- Annex T: Tone parameters (155 non-reserved entries × 3 values: f0, l1, l2) — see §12.16 below
- Bit prioritization maps (full-rate and half-rate, derived from §7.1 and §14.1) — see §12.17 below

### 7.2 Tables NOT Extracted

All normative numerical tables from TIA-102.BABA-A have been extracted as CSV files
in `annex_tables/`. No tables remain deferred.

**Note on Annex B size:** The implementation spec originally listed Annex B as 211 values.
The PDF Annex B (Initial Pitch Estimation Window) has 301 values spanning n=−150..150.
The 211-value window (n=−105..105) is Annex I (Synthesis Window). Both are extracted correctly.

### 7.3 Key Differences: IMBE vs AMBE Quantization

| Aspect | IMBE (Full-Rate) | AMBE+2 (Half-Rate) |
|--------|-------------------|---------------------|
| Pitch quantization | 8-bit scalar (256 levels) | 7-bit table lookup (120 levels, Annex L) |
| V/UV encoding | Per-band binary decisions (K bands) | 5-bit codebook (32 pre-defined patterns) |
| Spectral blocks | 6 blocks (J1..J6) | 4 blocks (J1..J4) |
| Gain quantizer | 6-bit scalar (64 levels) | 5-bit scalar (32 levels) |
| Spectral encoding | Scalar quantized DCT coefficients | Two-stage PRBA VQ + HOC VQ |
| HOC encoding | Scalar (bit-allocated per L) | Vector quantized (4-dim VQ per block) |
| FEC vectors | 8 (u0..u7) | 4 (u0..u3) |

### 7.4 Extracted FIR LPF Coefficients (Annex D)

These 21 coefficients were fully extracted and are used in pitch estimation:

```c
/* FIR low-pass filter for pitch autocorrelation (Annex D).
 * h_LPF(n), n = -10..10, symmetric. Index 0 = n = -10. */
static const double LPF_COEFFS[21] = {
    -0.002898, /* n = -10 */
    -0.002831, /* n = -9  */
     0.005666, /* n = -8  */
     0.016601, /* n = -7  */
     0.008800, /* n = -6  */
    -0.026955, /* n = -5  */
    -0.055990, /* n = -4  */
    -0.015116, /* n = -3  */
     0.118754, /* n = -2  */
     0.278990, /* n = -1  */
     0.351338, /* n =  0  */
     0.278990, /* n =  1  */
     0.118754, /* n =  2  */
    -0.015116, /* n =  3  */
    -0.055990, /* n =  4  */
    -0.026955, /* n =  5  */
     0.008800, /* n =  6  */
     0.016601, /* n =  7  */
     0.005666, /* n =  8  */
    -0.002831, /* n =  9  */
    -0.002898, /* n = 10  */
};
```

---

## 8. MBE Parameter Interface

The frame unpacking pipeline described above produces MBE model parameters that
are codec-agnostic. This section defines the parameter handoff boundary between
the frame unpacking layer and the speech synthesis layer.

### 8.1 Full-Rate: 88 Information Bits

After FEC decoding and PN demodulation, the 88 information bits are organized as
8 bit vectors (u0..u7). Packed MSB-first:

```
Byte packing (11 bytes, 88 bits total):
  u0[11..0], u1[11..0], u2[11..0], u3[11..0],
  u4[10..0], u5[10..0], u6[10..0], u7[6..0]
  MSB first within each vector.
```

From these bits, the MBE parameters are reconstructed:
1. b₀ (pitch index) → ω₀ and L via BABA-A Section 5
2. V/UV decisions from bit scan using L-dependent allocation (BABA-A Section 10)
3. Gain and spectral amplitude residuals via Annexes E-G tables
4. Dequantization → spectral amplitudes M̃_l for l = 1..L

### 8.2 Half-Rate: 49 Information Bits

The 49 bits are organized as 9 parameters (b0..b8). Packed MSB-first:

```
Byte packing (7 bytes, 49 bits total):
  b0[6..0], b1[4..0], b2[4..0], b3[8..0], b4[6..0],
  b5[4..0], b6[3..0], b7[3..0], b8[2..0]
  MSB first within each parameter.
```

From these, the MBE parameters are reconstructed:
1. b₀ → ω₀ and L via Annex L (120-entry pitch table)
2. b₁ → V/UV decisions via Annex M (32-entry codebook)
3. b₂ → differential gain via Annex O (32-level quantizer)
4. b₃, b₄ → PRBA spectral shape via Annexes P, Q (two-stage VQ)
5. b₅..b₈ → HOC coefficients via Annex R (four VQ codebooks)
6. Inverse DCT + prediction → spectral amplitudes M̃_l for l = 1..L

### 8.3 Synthesis Boundary

At this point the frame format is irrelevant. The synthesis engine receives:

| Parameter | Symbol | Description |
|-----------|--------|-------------|
| Fundamental frequency | ω₀ | Pitch in radians/sample |
| Harmonic count | L | Number of harmonics (9..56) |
| V/UV decisions | v_l | Binary voiced/unvoiced per harmonic |
| Spectral amplitudes | M̃_l | Magnitude for each harmonic l = 1..L |

These are standard MBE model parameters. The synthesis engine can implement:
- Baseline IMBE synthesis (BABA-A Sections 8, 9, 15) — sinusoidal OLA + noise shaping
- Improved MBE synthesis (AMBE+2-class) — better phase interpolation, enhanced
  spectral processing, improved V/UV transitions
- Any MBE-compatible reconstruction approach

See `analysis/vocoder_wire_vs_codec.md` for discussion of why the synthesis
engine is independent of the frame format.

### 8.4 DVSI Hardware Interface

For DVSI AMBE-3000 or AMBE-3003 chips, the interface uses a serial packet format:

- **Channel data packet** containing the FEC-decoded voice bits
- Packet format: `0x61` header, length, channel data, CMODE field
- CMODE selects full-rate (7200 bps) or half-rate (3600 bps) mode
- The chip handles parameter reconstruction and synthesis internally

---

## 9. Data Structures

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Full-rate IMBE voice frame (Phase 1 FDMA).
 * 88 information bits after FEC decoding and PN demodulation. */
typedef struct {
    /* The 8 decoded bit vectors (u0..u7), LSB-aligned in each uint16_t.
     *   u0..u3: 12 bits each
     *   u4..u6: 11 bits each  (c6 is sometimes shown as 10; see note in §1.2)
     *   u7:      7 bits       */
    uint16_t vectors[8];
    uint8_t  errors[8];       /* per-vector FEC error count */
    uint8_t  error_total;
    bool     repeat;          /* frame should be repeated (too many errors) */
    bool     mute;            /* frame should be muted (sustained errors)   */
} imbe_frame_t;

/* Half-rate AMBE+2 voice frame (Phase 2 TDMA).
 * 49 information bits after FEC decoding and PN demodulation. */
typedef struct {
    uint8_t  b0_pitch;    /* 7 bits  (0..119)  */
    uint8_t  b1_vuv;      /* 5 bits  (0..31)   */
    uint8_t  b2_gain;     /* 5 bits  (0..31)   */
    uint16_t b3_prba24;   /* 9 bits  (0..511)  */
    uint8_t  b4_prba58;   /* 7 bits  (0..127)  */
    uint8_t  b5_hoc1;     /* 5 bits  (0..31)   */
    uint8_t  b6_hoc2;     /* 4 bits  (0..15)   */
    uint8_t  b7_hoc3;     /* 4 bits  (0..15)   */
    uint8_t  b8_hoc4;     /* 3 bits  (0..7)    */

    uint8_t  errors[4];   /* e0..e3 */
    uint8_t  error_total;
    bool     repeat;
    bool     mute;

    /* Tone frame indicator: tone_valid == false means voice frame; otherwise
     * tone_id carries the tone ID (see ToneFrame / Annex T). */
    bool     tone_valid;
    uint8_t  tone_id;
} ambe_frame_t;

/* Half-rate tone frame parameters. */
typedef struct {
    uint8_t tone_id;       /* I_D (0..255) */
    float   f0_hz;         /* fundamental frequency */
    uint8_t l1;            /* spectral shaping index 1 */
    uint8_t l2;            /* spectral shaping index 2 */
    bool    is_silence;    /* true if I_D == 255 */
} tone_frame_t;

/* Raw FEC-encoded voice frame before decoding. */
typedef struct {
    /* c0..c3: 23 bits each (Golay-encoded)
     * c4..c6: 15 bits each (Hamming-encoded)
     * c7:      7 bits      (uncoded)           */
    uint32_t codewords[8];
} raw_imbe_codewords_t;

typedef struct {
    /* c0: 24 bits (extended Golay)
     * c1: 23 bits (Golay)
     * c2: 23 bits (Golay)
     * c3: variable (uncoded)                    */
    uint32_t codewords[4];
} raw_ambe_codewords_t;

/* Smoothed error tracking state for adaptive frame concealment. */
typedef struct {
    double  e_r;                /* exponentially smoothed error rate */
    uint8_t consecutive_bad;    /* count of consecutive invalid/errored frames */
    /* Previous frame's spectral amplitudes (for frame-repeat attenuation).
     * prev_frame is NULL when no previous frame is available. */
    double *prev_frame;
    size_t  prev_frame_len;
} error_tracker_t;

static inline void error_tracker_init(error_tracker_t *t) {
    t->e_r = 0.0;
    t->consecutive_bad = 0;
    t->prev_frame = NULL;
    t->prev_frame_len = 0;
}

/* Assess full-rate IMBE frame errors.
 * Sets *repeat/*mute per §6 (BABA-A error concealment rules). */
void error_tracker_assess_fullrate(error_tracker_t *t, uint8_t e0, uint8_t e_total,
                                   bool *repeat, bool *mute);

/* Assess half-rate AMBE frame errors. */
void error_tracker_assess_halfrate(error_tracker_t *t, uint8_t e0, uint8_t e_total,
                                   bool *repeat, bool *mute);
```

### 9.1 Bit Extraction Helpers

```c
/* Extract a single IMBE frame's 88 bits from an LDU bit stream.
 *
 * ldu_bits:         deinterleaved LDU bit stream (after status symbol removal)
 * frame_index:      which of the 9 frames to extract (0..=8)
 * position_table:   for each of the 9 frames, the 144 bit indices in the LDU
 *                   (sourced from TIA-102.BAAA-B Annex A)
 *
 * Returns a RawImbeCodewords via *out. */
void extract_imbe_from_ldu(
    const uint8_t *ldu_bits,
    size_t frame_index,
    const size_t position_table[9][144],
    raw_imbe_codewords_t *out)
{
    uint8_t bits[144];
    const size_t *positions = position_table[frame_index];
    for (size_t i = 0; i < 144; i++) bits[i] = ldu_bits[positions[i]];
    deinterleave_fullrate(bits, out);
}

/* Extract a single half-rate AMBE frame from a TDMA burst.
 *
 * burst_symbols: 180 dibit symbols of one TDMA timeslot
 * voice_index:   which voice frame (0..=3 for 4V, 0..=1 for 2V)
 * is_4v:         true for 4V burst (4 frames), false for 2V (2 frames) */
void extract_ambe_from_burst(
    const uint8_t burst_symbols[180],
    size_t voice_index,
    bool is_4v,
    raw_ambe_codewords_t *out)
{
    /* Voice frame start offsets (symbols) in a 4V/2V burst. */
    static const size_t starts_4v[4] = {10, 47, 96, 133};
    static const size_t starts_2v[2] = {10, 47};
    size_t start;
    if (is_4v) {
        /* assert(voice_index < 4); */
        start = starts_4v[voice_index];
    } else {
        /* assert(voice_index < 2); */
        start = starts_2v[voice_index];
    }

    uint8_t dibits[36];
    memcpy(dibits, &burst_symbols[start], 36);
    deinterleave_halfrate(dibits, out);
}

/* Pack an imbe_frame_t's 88 bits into an 11-byte array.
 * Bits are packed MSB-first: u0[11..0] | u1[11..0] | ... | u7[6..0]. */
void imbe_to_jmbe_bytes(const imbe_frame_t *frame, uint8_t out[11]) {
    static const uint8_t widths[8] = {12, 12, 12, 12, 11, 11, 11, 7};
    memset(out, 0, 11);
    size_t bit_pos = 0;
    for (size_t i = 0; i < 8; i++) {
        for (int b = (int)widths[i] - 1; b >= 0; b--) {
            if ((frame->vectors[i] >> b) & 1u)
                out[bit_pos / 8] |= (uint8_t)(1u << (7 - (bit_pos % 8)));
            bit_pos++;
        }
    }
}

/* Pack an ambe_frame_t's 49 bits into a 7-byte array.
 * Bits are packed MSB-first: b0[6..0] | b1[4..0] | ... | b8[2..0]. */
void ambe_to_jmbe_bytes(const ambe_frame_t *frame, uint8_t out[7]) {
    struct { uint16_t value; uint8_t width; } params[9] = {
        { frame->b0_pitch,  7 },
        { frame->b1_vuv,    5 },
        { frame->b2_gain,   5 },
        { frame->b3_prba24, 9 },
        { frame->b4_prba58, 7 },
        { frame->b5_hoc1,   5 },
        { frame->b6_hoc2,   4 },
        { frame->b7_hoc3,   4 },
        { frame->b8_hoc4,   3 },
    };
    memset(out, 0, 7);
    size_t bit_pos = 0;
    for (size_t i = 0; i < 9; i++) {
        for (int b = (int)params[i].width - 1; b >= 0; b--) {
            if ((params[i].value >> b) & 1u)
                out[bit_pos / 8] |= (uint8_t)(1u << (7 - (bit_pos % 8)));
            bit_pos++;
        }
    }
}

/* Full-rate deinterleave — applies inverse of BABA-A Annex H
 * (see annex_tables/annex_h_interleave.csv). */
void deinterleave_fullrate(const uint8_t bits[144], raw_imbe_codewords_t *out);

/* Half-rate deinterleave — applies inverse of BABA-A Annex S / BBAC-1 Annex E. */
void deinterleave_halfrate(const uint8_t dibits[36], raw_ambe_codewords_t *out);
```

---

## 10. Variable Initialization (Annex A)

These initial values MUST be set at decoder startup and call start:

```c
/* Decoder state initialization per Annex A. */
typedef struct {
    int     prev_pitch_period;       /* P_{-1}            */
    int     prev_pitch_period_2;     /* P_{-2}            */
    double  prev_omega_0;            /* omega_0(-1)       */
    double  prev_magnitudes[56];     /* M_l(-1) for l=1..L, plus headroom */
    uint8_t prev_l;                  /* L(-1)             */
    uint8_t prev_k;                  /* K(-1)             */
    double  error_rate;              /* e_R               */
    double  enhancement_energy;      /* S_E               */
    uint16_t noise_seed;             /* u(-105)           */
} decoder_state_t;

void decoder_state_init(decoder_state_t *s) {
    s->prev_pitch_period   = 100;                     /* P_{-1} = 100 */
    s->prev_pitch_period_2 = 100;                     /* P_{-2} = 100 */
    s->prev_omega_0        = 0.02985 * M_PI;          /* omega_0(-1) = 0.02985*pi */
    for (size_t i = 0; i < 56; i++) s->prev_magnitudes[i] = 1.0;  /* M_l(-1) = 1 */
    s->prev_l               = 30;                      /* L(-1) = 30  */
    s->prev_k               = 10;                      /* K(-1) = 10  */
    s->error_rate           = 0.0;                     /* e_R = 0     */
    s->enhancement_energy   = 75000.0;                 /* S_E = 75000 */
    s->noise_seed           = 3147;                    /* u(-105) = 3147 */
}
```

---

## 11. Implementation Notes

### 11.1 Common Pitfalls

1. **Bit ordering:** BABA-A numbers bits MSB-first within each vector. BAAA-B and
   BBAC-1 use the same convention throughout the P25 spec suite.

2. **PN demodulation before FEC decode:** c0 is decoded FIRST (without PN demod)
   to recover the pitch seed. Then PN masks are generated and applied to c1..c7
   BEFORE their FEC decoding.

3. **L-dependent bit allocation:** The full-rate vocoder's bit allocation changes
   for every value of L (9..56). The decoder must look up the correct allocation
   table AFTER recovering b0 (pitch) to know how to unpack the remaining parameters.

4. **Status symbol removal:** In FDMA, status symbols are inserted every 35th dibit
   in the transmitted stream. These must be stripped BEFORE IMBE frame extraction.
   The BAAA-B spec defines exactly which positions are status symbols.

5. **Extended Golay vs standard Golay:** Half-rate c0 uses [24,12] extended Golay
   (24 bits, distance 8), while full-rate c0..c3 use [23,12] standard Golay
   (23 bits, distance 7). Do not confuse them.

6. **Frame repeat attenuation:** When repeating a frame due to errors, the previous
   frame's spectral amplitudes must be attenuated (not simply replayed). The
   attenuation factor increases with consecutive repeats, fading to silence.

### 11.2 Reference Implementations

The following open-source projects implement portions of this spec and can be
referenced as working examples. Note that all use the baseline 1993-era IMBE
synthesis algorithm — none implement AMBE+2-class synthesis improvements.

| Implementation | Language | Full-Rate | Half-Rate | Notes |
|---------------|----------|-----------|-----------|-------|
| JMBE | Java | Yes | Yes | Frame unpacking + baseline IMBE synthesis |
| SDRTrunk | Java | Yes | Yes | P25 decoder using JMBE for vocoder |
| OP25 | C++/Python | Yes | Partial | GNU Radio-based P25 receiver |
| DSD | C | Yes | Yes | Multi-mode digital speech decoder |
| mbelib | C | Yes | Yes | Standalone MBE codec library |

### 11.3 Parametric Rate Conversion

BABA-A Section 17 defines conversion between full-rate and half-rate frames at the
parameter level (not PCM tandeming). This is used when bridging Phase 1 and Phase 2
systems:

- **Full-to-half:** Decode full-rate parameters, re-quantize to half-rate codebooks
- **Half-to-full:** Decode half-rate parameters, map to full-rate quantization

Frame type mapping:
| Input Frame | Output Frame |
|------------|-------------|
| Voice | Voice (re-quantized) |
| Erasure | Last good voice (attenuated) |
| Silence | Silence |
| Tone | Tone (parameter-mapped) |

---

## Appendix A: Quick Reference -- Decode Flow

### A.1 Full-Rate IMBE (from FDMA LDU)

```
LDU bit stream (1728 bits including status)
  |
  v
Strip status symbols -> 1680 information bits
  |
  v
Extract 9 IMBE frames at known positions (Annex A tables from BAAA-B)
  |
  v
For each 144-bit IMBE frame:
  Deinterleave (Annex H) -> c0[23], c1[23], c2[23], c3[23],
                             c4[15], c5[15], c6[15], c7[7]
  Golay decode c0 -> u0 (12 bits), compute b0 (pitch)
  Generate PN masks from b0
  PN demod c1..c7 -> v1..v7
  Golay decode v1..v3 -> u1..u3
  Hamming decode v4..v6 -> u4..u6
  u7 = v7
  Error check -> repeat/mute decision
  Dequantize u0..u7 -> MBE parameters (ω₀, L, v_l, M̃_l)
  |
  v
MBE synthesis (baseline IMBE or improved) -> PCM audio (160 samples, 16-bit, 8 kHz)
```

### A.2 Half-Rate AMBE (from TDMA 4V Burst)

```
TDMA timeslot (180 dibit symbols)
  |
  v
Identify burst type (4V, 2V, SACCH, FACCH) from DUID
  |
  v
Extract voice frame dibits at known positions:
  4V: symbols [10..45], [47..82], [96..131], [133..168]
  2V: symbols [10..45], [47..82]
  |
  v
For each 36-dibit AMBE frame:
  Deinterleave (Annex S) -> c0[24], c1[23], c2[23], c3[variable]
  Extended Golay decode c0 -> u0 (12 bits), extract b0 (pitch, 7 bits)
  Generate PN masks from u0
  PN demod c1..c3 -> v1..v3
  Golay decode v1..v2 -> u1..u2
  u3 = v3
  Reconstruct b0..b8 from u0..u3
  Error check -> repeat/mute decision
  Dequantize b0..b8 -> MBE parameters (ω₀, L, v_l, M̃_l) via Annexes L-R
  |
  v
MBE synthesis (baseline IMBE or improved) -> PCM audio (160 samples, 16-bit, 8 kHz)
```

---


## 12. Extracted Annex Tables

Source: TIA-102.BABA-A PDF, all normative annexes (pages 79–145).
Extracted programmatically from `pdftotext -layout` output with invariant verification.
OCR corrections applied: Annex H symbol 51 right-bit "c9(5)" → "c0(5)";
Annex S symbol 34 bit1 "c0(5)" → "c0(6)" (same class of typo; without fix c0(6)
would be absent and c0(5) duplicated across the 72-position coverage).

### 12.0 Format and Conventions

The table data is stored as CSV files alongside this specification, one file
per annex, in the `annex_tables/` directory:

```
standards/TIA-102.BABA-A/annex_tables/
├── annex_b_analysis_window.csv   (301 rows)   Annex B: wI(n), n=-150..150
├── annex_c_pitch_refinement_window.csv (221 rows) Annex C: wR(n), n=-110..110
├── annex_e_gain_quantizer.csv    (64 rows)    Annex E: full-rate gain levels
├── annex_f_gain_allocation.csv   (240 rows)   Annex F: gain bit alloc, 5 entries/L
├── annex_g_hoc_allocation.csv    (1272 rows)  Annex G: HOC bit alloc, L-6 entries/L
├── annex_h_interleave.csv        (72 rows)    Annex H: full-rate interleave
├── annex_i_synthesis_window.csv  (211 rows)   Annex I: wS(n), n=-105..105
├── annex_j_block_lengths.csv     (48 rows)    Annex J: block lengths J1..J6/L
├── annex_l_pitch_table.csv       (120 rows)   Annex L: half-rate pitch (L, omega_0)
├── annex_m_vuv_codebook.csv      (32 rows)    Annex M: V/UV pattern codebook
├── annex_n_block_lengths.csv     (48 rows)    Annex N: half-rate block lengths J1..J4
├── annex_o_gain_quantizer.csv    (32 rows)    Annex O: half-rate gain levels
├── annex_p_prba24_codebook.csv   (512 rows)   Annex P: PRBA24 VQ (G2, G3, G4)
├── annex_q_prba58_codebook.csv   (128 rows)   Annex Q: PRBA58 VQ (G5..G8)
├── annex_r_hoc_b5.csv            (32 rows)    Annex R: HOC block 1 (H1,1..H1,4)
├── annex_r_hoc_b6.csv            (16 rows)    Annex R: HOC block 2 (H2,1..H2,4)
├── annex_r_hoc_b7.csv            (16 rows)    Annex R: HOC block 3 (H3,1..H3,4)
├── annex_r_hoc_b8.csv            (8 rows)     Annex R: HOC block 4 (H4,1..H4,4)
├── annex_s_interleave.csv        (36 rows)    Annex S: half-rate interleave
└── annex_t_tone_params.csv       (155 rows)   Annex T: tone/DTMF/KNOX/silence params
```

Each CSV has a `#`-prefixed header block with the annex title, PDF page reference,
and a verification note, followed by a column-name row and the data rows.
Row counts above are data rows only.

CSVs are chosen over language-specific constant arrays so the spec remains
implementation-language neutral. Any standard CSV parser can load them; the
schemas below document the column meanings and typical consumer code shape
(shown in C).

### 12.1 Annex E — Gain Quantizer Levels

**Source:** BABA-A page 84.
**File:** [`annex_tables/annex_e_gain_quantizer.csv`](annex_tables/annex_e_gain_quantizer.csv)
**Schema:** `b2_index, level`

Six-bit non-uniform quantizer for the overall log-gain G̃_1 (64 levels,
range ≈ [−2.842, 8.696]). Encoder picks b̂_2 = argmin over `b2_index` of
|level[b2_index] − Ĝ_1|; decoder outputs G̃_1 = level[b̃_2].

```c
/* Typical consumer signature */
extern const float imbe_gain_levels[64];     /* loaded from annex_e_gain_quantizer.csv */

uint8_t imbe_gain_encode(float G1);          /* returns b2 in [0, 63] */
float   imbe_gain_decode(uint8_t b2);        /* returns G1 = imbe_gain_levels[b2] */
```

### 12.2 Annex F — Bit Allocation and Step Size for Transformed Gain

**Source:** BABA-A pages 85–88.
**File:** [`annex_tables/annex_f_gain_allocation.csv`](annex_tables/annex_f_gain_allocation.csv)
**Schema:** `L, m, B_m, Delta_m`

For each L̂ ∈ [9, 56] and each m ∈ {3, 4, 5, 6, 7} (quantizing DCT coefficients
b̂_3..b̂_7 of the transformed gain vector), one row gives:
- `B_m` — bit count for that coefficient (1..10)
- `Delta_m` — uniform quantizer step size (float)

Exactly 5 rows per L̂, 240 rows total. For a given L̂, rows appear in order
m = 3, 4, 5, 6, 7.

```c
/* Typical consumer signature */
typedef struct { uint8_t B_m; float delta_m; } imbe_gain_alloc_t;

/* alloc[L - 9][m - 3] is the (B_m, delta_m) pair for the requested L and m. */
extern const imbe_gain_alloc_t imbe_gain_allocation[48][5];
```

### 12.3 Annex G — Higher-Order DCT Coefficient Bit Allocation

**Source:** BABA-A pages 89–101.
**File:** [`annex_tables/annex_g_hoc_allocation.csv`](annex_tables/annex_g_hoc_allocation.csv)
**Schema:** `L, C_i, C_k, b_m, B_m`

Variable-length allocation per L̂. Each row identifies one HOC coefficient:
- `C_i`, `C_k` — the k-th DCT coefficient of block i (i ∈ [1, 6])
- `b_m` — voice-parameter element index (starts at 8, increments by 1)
- `B_m` — bit count (0..10; 0 means the coefficient is not transmitted)

Entry count per L̂ equals **L̂ − 6**, which matches sum(J̃_i) − 6 per Annex J
(each of the 6 DCT blocks contributes J̃_i − 1 HOCs since the block mean is
coded separately via Annex F). Counts range from 3 (L̂=9) to 50 (L̂=56).

The table is flattened row-major by L̂ then b_m, for 1272 rows total.

```c
/* Typical consumer signature */
typedef struct { uint8_t C_i, C_k, b_m, B_m; } imbe_hoc_alloc_t;

/* Pointer to the L̂-specific entry list, plus its length. */
const imbe_hoc_alloc_t *imbe_hoc_allocation_for(uint8_t L, size_t *out_count);
```

### 12.4 Annex H — Bit Frame Format (Full-Rate Interleaving)

**Source:** BABA-A page 102.
**File:** [`annex_tables/annex_h_interleave.csv`](annex_tables/annex_h_interleave.csv)
**Schema:** `symbol, bit1_vector, bit1_index, bit0_vector, bit0_index`

The 144-bit full-rate frame is transmitted as 72 dibit symbols. For each
symbol 0..71, `bit1_*` identifies the source of the symbol's MSB and `bit0_*`
the LSB. `*_vector` is 0..7 (which code vector c_i) and `*_index` is the bit
within that vector (c_0..c_3: 0..22, c_4..c_6: 0..14, c_7: 0..6).

**OCR correction applied:** At symbol 51, the PDF printed "c9(5)" for the
right-column (bit_0) position; this was corrected to "c0(5)" after verifying
that (a) c9 does not exist (only c0..c7 are defined) and (b) c0(5) was the
only missing bit in the exhaustive coverage check across all 72 symbols.

```c
/* Typical consumer signature */
typedef struct { uint8_t vec; uint8_t idx; } imbe_bit_src_t;
typedef struct { imbe_bit_src_t bit1, bit0; } imbe_symbol_t;

extern const imbe_symbol_t imbe_interleave[72];   /* loaded from annex_h_interleave.csv */
```

### 12.5 Annex J — Log Magnitude Prediction Residual Block Lengths

**Source:** BABA-A page 105.
**File:** [`annex_tables/annex_j_block_lengths.csv`](annex_tables/annex_j_block_lengths.csv)
**Schema:** `L, J1, J2, J3, J4, J5, J6`

For each L̂ ∈ [9, 56], six block lengths J̃_1..J̃_6 that partition the L̂
spectral-amplitude prediction residuals into 6 DCT blocks for HOC quantization.
Each row satisfies J̃_1 + J̃_2 + … + J̃_6 = L̂ exactly.

```c
/* Typical consumer signature */
extern const uint8_t imbe_block_lengths[48][6];   /* index 0 = L̂=9 */
```

### 12.6 Annex B — Initial Pitch Estimation Window

**Source:** BABA-A pages 79–80.
**File:** [`annex_tables/annex_b_analysis_window.csv`](annex_tables/annex_b_analysis_window.csv)
**Schema:** `n, wI_n`

FIR window `wI(n)` used to multiply the input speech before the initial pitch
autocorrelation (§3.1, Eq. 5). 301 coefficients indexed n = −150..150.
Symmetric around n = 0 (verified during extraction).

```c
/* Typical consumer signature */
extern const float imbe_analysis_window[301];   /* index 0 = n=−150 */
```

### 12.7 Annex I — Speech Synthesis Window

**Source:** BABA-A pages 103–104.
**File:** [`annex_tables/annex_i_synthesis_window.csv`](annex_tables/annex_i_synthesis_window.csv)
**Schema:** `n, wS_n`

FIR window `wS(n)` applied during voiced-speech synthesis overlap-add (§8).
211 coefficients indexed n = −105..105.

```c
/* Typical consumer signature */
extern const float imbe_synthesis_window[211];   /* index 0 = n=−105 */
```

### 12.8 Annex L — Half-Rate Fundamental Frequency Quantization Table

**Source:** BABA-A page 127.
**File:** [`annex_tables/annex_l_pitch_table.csv`](annex_tables/annex_l_pitch_table.csv)
**Schema:** `b0, L, omega_0`

7-bit half-rate pitch index (120 entries, b0 ∈ [0, 119]). Each row gives the
harmonic count L̃ and the reconstructed fundamental frequency for that
index. ω̃₀ is monotone decreasing in b0 (verified during extraction).
Replaces the analytical mapping used in full-rate IMBE (§1.3.1).

**Units — important:** the PDF's Annex L header reads `ω₀` with no explicit
unit annotation, but the stored values are **normalized frequency
(cycles/sample = f₀/fₛ)**, not radians per sample. That is, Annex L stores
`ω̃₀/(2π)`. Consumers using these values in equations that expect rad/sample
(Eq. 131–141, Eq. 147, Eq. 206, etc.) **must multiply by 2π** on lookup.

Confirmation: BABA-A §13.1 (page 58) explicitly states the voice pitch range
is `2π/123.125 ≤ ω̂₀ ≤ 2π/19.875` rad/sample — i.e. `[0.051, 0.316]`
rad/sample. The Annex L entries span `[0.008125, 0.049971]`, which matches
the stated range only after multiplication by 2π (giving `[0.051, 0.314]`).
Reading the entries verbatim as rad/sample yields `[10.3, 63.6] Hz` at
fs = 8 kHz, which is sub-voice and cannot be correct.

See `analysis/vocoder_decode_disambiguations.md` §13 for the full
reconciliation, including the matching `1/T_samples` pitch-period
interpretation and the cross-rate-converter regression that surfaced it.

```c
/* Typical consumer signature. Note the convention below returns ω̃₀ in
 * rad/sample — callers should apply the 2π multiplication at load time
 * (preferred) or consistently within every consumer formula. Mixing
 * conventions across the codebase is the failure mode this annotation
 * is meant to prevent. */
typedef struct { uint8_t L; float omega_0; } ambe_pitch_entry_t;
extern const ambe_pitch_entry_t ambe_pitch_table[120];  /* omega_0 in rad/sample */

/* Loader responsibility: multiply the CSV's omega_0 column by 2π before
 * populating ambe_pitch_table[], OR document that the table stores
 * cycles/sample and require every consumer site to multiply. The former
 * is recommended because it localizes the unit conversion to one place. */
```

### 12.9 Annex M — Half-Rate V/UV Codebook

**Source:** BABA-A page 128.
**File:** [`annex_tables/annex_m_vuv_codebook.csv`](annex_tables/annex_m_vuv_codebook.csv)
**Schema:** `b1, v0, v1, v2, v3, v4, v5, v6, v7`

5-bit half-rate V/UV vector quantizer (32 entries). Each row gives an 8-bit
V/UV decision pattern (one bit per frequency band). b1 = 0 is all-voiced;
b1 = 16 is all-unvoiced.

```c
/* Typical consumer signature */
extern const uint8_t ambe_vuv_codebook[32][8];   /* 1=voiced, 0=unvoiced */
```

### 12.10 Annex N — Half-Rate Block Lengths

**Source:** BABA-A page 129.
**File:** [`annex_tables/annex_n_block_lengths.csv`](annex_tables/annex_n_block_lengths.csv)
**Schema:** `L, J1, J2, J3, J4`

For each L̃ ∈ [9, 56], four block lengths J̃_1..J̃_4 partitioning the L̃
spectral-amplitude prediction residuals into 4 DCT blocks for half-rate HOC
quantization. Each row satisfies J̃_1 + J̃_2 + J̃_3 + J̃_4 = L̃ exactly
(half-rate uses 4 blocks; full-rate uses 6, see §12.5).

```c
/* Typical consumer signature */
extern const uint8_t ambe_block_lengths[48][4];   /* index 0 = L̃=9 */
```

### 12.11 Annex O — Half-Rate Gain Quantizer Levels

**Source:** BABA-A page 130.
**File:** [`annex_tables/annex_o_gain_quantizer.csv`](annex_tables/annex_o_gain_quantizer.csv)
**Schema:** `b2, gain_level`

5-bit half-rate gain quantizer (32 levels, b2 ∈ [0, 31]). Values are monotone
increasing in b2 (verified during extraction).

```c
/* Typical consumer signature */
extern const float ambe_gain_levels[32];
```

### 12.12 Annex P — PRBA24 Vector Quantizer

**Source:** BABA-A pages 131–136.
**File:** [`annex_tables/annex_p_prba24_codebook.csv`](annex_tables/annex_p_prba24_codebook.csv)
**Schema:** `b3, G2, G3, G4`

9-bit vector quantizer (512 entries) for the half-rate Prediction Residual
Block Average (PRBA) transform coefficients 2, 3, 4. Each row is a 3-tuple
codeword (G̃₂, G̃₃, G̃₄).

```c
/* Typical consumer signature */
typedef struct { float G2, G3, G4; } prba24_codeword_t;
extern const prba24_codeword_t ambe_prba24_codebook[512];
```

### 12.13 Annex Q — PRBA58 Vector Quantizer

**Source:** BABA-A pages 137–139.
**File:** [`annex_tables/annex_q_prba58_codebook.csv`](annex_tables/annex_q_prba58_codebook.csv)
**Schema:** `b4, G5, G6, G7, G8`

7-bit vector quantizer (128 entries) for PRBA coefficients 5–8. Each row is a
4-tuple codeword (G̃₅, G̃₆, G̃₇, G̃₈).

```c
/* Typical consumer signature */
typedef struct { float G5, G6, G7, G8; } prba58_codeword_t;
extern const prba58_codeword_t ambe_prba58_codebook[128];
```

### 12.14 Annex R — Higher-Order Coefficient VQ Codebooks

**Source:** BABA-A pages 140–141.
**Files:**
- [`annex_tables/annex_r_hoc_b5.csv`](annex_tables/annex_r_hoc_b5.csv) (32 entries, 5-bit index b5)
- [`annex_tables/annex_r_hoc_b6.csv`](annex_tables/annex_r_hoc_b6.csv) (16 entries, 4-bit index b6)
- [`annex_tables/annex_r_hoc_b7.csv`](annex_tables/annex_r_hoc_b7.csv) (16 entries, 4-bit index b7)
- [`annex_tables/annex_r_hoc_b8.csv`](annex_tables/annex_r_hoc_b8.csv) (8 entries, 3-bit index b8)

**Schema (all four):** `<b_index>, H_1, H_2, H_3, H_4`

Four separate vector quantizer codebooks, one per half-rate HOC block (blocks
1–4 from Annex N). Each row is a 4-dimensional codeword giving the DCT
coefficients H̃_{i,1..4} for that block.

```c
/* Typical consumer signature */
typedef struct { float H1, H2, H3, H4; } hoc_codeword_t;
extern const hoc_codeword_t ambe_hoc_codebook_b5[32];
extern const hoc_codeword_t ambe_hoc_codebook_b6[16];
extern const hoc_codeword_t ambe_hoc_codebook_b7[16];
extern const hoc_codeword_t ambe_hoc_codebook_b8[8];
```

### 12.15 Annex S — Half-Rate Bit Frame Format (Interleaving)

**Source:** BABA-A page 143.
**File:** [`annex_tables/annex_s_interleave.csv`](annex_tables/annex_s_interleave.csv)
**Schema:** `symbol, bit1_vector, bit1_index, bit0_vector, bit0_index`

The 72-bit half-rate frame is transmitted as 36 dibit symbols. For each
symbol 0..35, `bit1_*` identifies the source of the MSB and `bit0_*` the LSB.
Vector encoding:
- 0 = c0 (24 bits, extended [24,12] Golay)
- 1 = c1 (23 bits, [23,12] Golay)
- 2 = c2 (11 bits in table)
- 3 = c3 (14 bits in table)

Total bits = 24 + 23 + 11 + 14 = 72. The c2/c3 split reflects the PDF's own
labeling of the bit-stream segments and does not directly map to the
u2/u3 codeword boundaries.

**OCR correction applied:** Symbol 34 bit1 was printed as `c0(5)` in the PDF;
corrected to `c0(6)` after coverage check (the even-symbol bit1 lane walks
c0(23) down to c0(6) across symbols 0, 2, ..., 34; c0(5) is covered at
symbol 0 bit0, and c0(6) would otherwise be absent entirely). Same class of
typo as the Annex H `c9(5)` → `c0(5)` correction at full-rate symbol 51.
An equivalent table is also published in TIA-102.BBAC-1 Annex E.

```c
/* Typical consumer signature */
typedef struct { uint8_t vec; uint8_t idx; } ambe_bit_src_t;
typedef struct { ambe_bit_src_t bit1, bit0; } ambe_symbol_t;
extern const ambe_symbol_t ambe_interleave[36];
```

### 12.16 Annex T — Tone Frame Parameters

**Source:** BABA-A page 144.
**File:** [`annex_tables/annex_t_tone_params.csv`](annex_tables/annex_t_tone_params.csv)
**Schema:** `tone_id, f0_hz, l1, l2`

Tone-frame parameter table keyed by 8-bit tone ID (I_D). The table contains
155 non-reserved entries covering four ranges:

| ID range  | Category       | Generation rule |
|-----------|----------------|-----------------|
| 0–4       | reserved       | not in CSV      |
| 5–12      | single-freq    | f₀ = 31.250 · ID Hz, l₁ = l₂ = 1 |
| 13–25     | single-freq    | f₀ = 15.625 · ID Hz, l₁ = l₂ = 2 |
| 26–38     | single-freq    | f₀ = 10.417 · ID Hz, l₁ = l₂ = 3 |
| 39–51     | single-freq    | f₀ = 7.8125 · ID Hz, l₁ = l₂ = 4 |
| 52–64     | single-freq    | f₀ = 6.2500 · ID Hz, l₁ = l₂ = 5 |
| 65–76     | single-freq    | f₀ = 5.2803 · ID Hz, l₁ = l₂ = 6 |
| 77–89     | single-freq    | f₀ = 4.4643 · ID Hz, l₁ = l₂ = 7 |
| 90–102    | single-freq    | f₀ = 3.9063 · ID Hz, l₁ = l₂ = 8 |
| 103–115   | single-freq    | f₀ = 3.4722 · ID Hz, l₁ = l₂ = 9 |
| 116–122   | single-freq    | f₀ = 3.1250 · ID Hz, l₁ = l₂ = 10 |
| 123–127   | reserved       | not in CSV      |
| 128–143   | DTMF           | explicit values in CSV |
| 144–163   | KNOX           | explicit values in CSV |
| 164–254   | reserved       | not in CSV      |
| 255       | silence        | f₀ = 250, l₁ = l₂ = 0 |

```c
/* Typical consumer signature */
typedef struct { uint8_t tone_id; float f0_hz; uint8_t l1, l2; } tone_param_t;
extern const tone_param_t ambe_tone_params[155];  /* only non-reserved IDs */

/* Lookup helper (NULL if reserved): */
const tone_param_t *ambe_tone_lookup(uint8_t tone_id);
```

### 12.17 Bit Prioritization Maps (Full-Rate and Half-Rate)

**Source:** BABA-A §7.1 (full-rate, pages 33–35) and §14.1 Tables 15–18
(half-rate, pages 66–67). See §1.4 and §2.3 for the algorithms and table
descriptions; the CSVs below are the pre-computed mappings derived from
those sources.

**Files:**
- [`annex_tables/imbe_bit_prioritization.csv`](annex_tables/imbe_bit_prioritization.csv)
  — full-rate, 4224 rows (48 L values × 88 mappings per L)
- [`annex_tables/ambe_bit_prioritization.csv`](annex_tables/ambe_bit_prioritization.csv)
  — half-rate, 49 rows (deterministic, L-independent)

**Schema (both files):** `[L,] src_param, src_bit, dst_vec, dst_bit`
(the `L` column is present only in the full-rate file).

`src_param` identifies the voice quantizer value (0 = b̂₀ pitch, 1 = b̂₁
V/UV, 2 = b̂₂ gain, 3+ = spectral amplitudes or PRBA/HOC VQ indices
depending on mode). `src_bit` is the bit within that parameter (0 = LSB).
`dst_vec` ∈ [0, 7] for full-rate or [0, 3] for half-rate. `dst_bit` is the
bit within that output vector (0 = LSB).

**Invariants verified during generation** (generator aborts on failure):
- Full-rate: for each L, the 88-bit budget check `8 + K + 6 + (73-K) + 1 = 88`
  holds; every `(src_param, src_bit)` slot is covered exactly once; every
  `(dst_vec, dst_bit)` slot is covered exactly once.
- Half-rate: every `(src_param, src_bit)` in b̂₀..b̂₈ is covered exactly
  once; the 49 info bits map to 49 distinct `(dst_vec, dst_bit)` slots.

```c
/* Typical consumer signatures */
typedef struct {
    uint8_t src_param, src_bit, dst_vec, dst_bit;
} bit_map_entry_t;

/* Half-rate: fixed 49-entry map. */
extern const bit_map_entry_t ambe_bit_map[49];

/* Full-rate: indexed by L. Returns a pointer to the 88-entry array
 * for the requested L (valid L ∈ [9, 56]). */
const bit_map_entry_t *imbe_bit_map_for_L(uint8_t L);
```

### 12.18 Annex C — Pitch Refinement Window

**Source:** BABA-A pages 81–82.
**File:** [`annex_tables/annex_c_pitch_refinement_window.csv`](annex_tables/annex_c_pitch_refinement_window.csv)
**Schema:** `n, wR_n`

FIR window `wR(n)` used in pitch refinement spectral estimation (§3.2,
Eq. 29). 221 coefficients indexed n = −110..110. Symmetric around n = 0,
peak value `wR(0) = 1.0` (verified during extraction).

Also used by the decoder to compute the `γ_w` scaling constant in §1.12.1
(Eq. 121), which scales per-band unvoiced magnitudes during synthesis.

```c
/* Typical consumer signature */
extern const float imbe_pitch_refinement_window[221];   /* index 0 = n=−110 */
```
