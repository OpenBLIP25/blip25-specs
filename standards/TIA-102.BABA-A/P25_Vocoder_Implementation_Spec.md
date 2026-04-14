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

### 1.4 Bit Prioritization Scan Order

The encoder scans voice parameters b0 through b_{L+2} into vectors u0..u7 by
significance. The MSB of b0 (pitch) always goes into u0. The scan order is defined
algorithmically in BABA-A Section 10 and depends on L.

**For implementation:** The bit prioritization is invertible. The decoder reconstructs
b0..b_{L+2} from u0..u7 using the same scan tables. The L value is recovered from
b0 (pitch index) before the remaining parameters can be unpacked.

```
Decode order:
1. Golay-decode u0 -> extract b0 (8-bit pitch index)
2. From b0, compute omega_0 = 4*pi / (b0 + 39.5)  (Eq. 46)
3. Compute L = floor(0.9254 * floor(pi/omega_0 + 0.25))  (Eq. 47)
   Constrained to 9 <= L <= 56; see §1.3.1 for details.
4. Using L, look up bit allocation tables (Annexes F, G) to determine
   how many bits each parameter gets
5. Unpack u1..u7 using the scan order for this L value
```

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

/* Modulation vector descriptor: packed bits (LSB-first, element k at bit k)
 * and the vector's length in bits. */
typedef struct { uint32_t bits; uint8_t len; } imbe_mod_vec_t;

/* Compute the 8 modulation vectors m_0..m_7 for full-rate IMBE. */
void imbe_modulation_vectors(uint16_t u0, imbe_mod_vec_t mv[8]) {
    uint16_t pr[115];
    imbe_pn_sequence_fullrate(u0, pr);

    /* Inline helper: pack mask bits pr[start..start+len] into a u32. */
    #define MASK_RANGE(start, length) ({                                \
        uint32_t _bits = 0;                                             \
        for (int _k = 0; _k < (length); _k++)                           \
            _bits |= ((uint32_t)imbe_pn_mask_bit(pr[(start)+_k])) << _k;\
        _bits;                                                          \
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

The 49 voice bits are scanned into 4 bit vectors u0..u3:

| Vector | Information Bits | FEC Code | Encoded Bits | Protection Level |
|--------|-----------------|----------|-------------|-----------------|
| u0 | 12 | [24,12] Extended Golay | 24 | Strongest (MSBs of b0, b1, b2, b3) |
| u1 | 12 | [23,12] Golay | 23 | Strong |
| u2 | 12 | [23,12] Golay | 23 | Moderate |
| u3 | 13 | None (uncoded) | -- | None (carried in remaining bits) |

**Note on u3:** The 4th vector carries the least significant bits without FEC.
Total encoded = 24 + 23 + 23 = 70 coded bits. The remaining 2 bits of the 72-bit
frame carry the uncoded tail of u3. (Alternatively, some references show all 72 bits
allocated across 4 encoded vectors; the exact split is defined in the bit prioritization
algorithm, Section 16.7 of BABA-A.)

### 2.4 FEC Encoding -- Half-Rate

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

Applied to u1 and u2. Same code as full-rate (see Section 1.5.1).

### 2.5 PN Sequence -- Half-Rate

Same linear congruential generator as full-rate, but only 24 values (indices 0..23):

```
p_r(0) = 16 * u0
p_r(n) = (173 * p_r(n-1) + 13849) mod 65536    for 1 <= n <= 23
```

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
 * dibits: 36 elements in 0..=3. c_out layout:
 *   c_out[0]: 24 bits ([24,12] extended Golay)
 *   c_out[1]: 23 bits ([23,12] Golay)
 *   c_out[2]: 23 bits ([23,12] Golay)
 *   c_out[3]: variable (uncoded remainder)
 * Full table is in TIA-102.BABA-A Annex S and TIA-102.BBAC-1 Annex E. */
void deinterleave_ambe_halfrate(const uint8_t dibits[36], uint32_t c_out[4]);
```

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
- Annex E: Gain quantizer (64 levels) — see §12.1 below
- Annex F: Gain bit allocation by L (9..56) — see §12.2 below
- Annex G: HOC bit allocation by L (9..56), variable width — see §12.3 below
- Annex H: Full-rate bit frame format / interleaving (72 symbols × 2 dibits) — see §12.4 below
- Annex J: Spectral block lengths J1..J6 by L — see §12.5 below

**Half-Rate (partial — structural only):**
- Annex L: Pitch quantization (120 entries) -- selected values extracted:
  - b0=0: L=9, omega_0=0.049971
  - b0=40: L=17, omega_0=0.027122
  - b0=119: L=56, omega_0=0.008125
- Annex M: V/UV codebook (32 entries) -- selected entries extracted:
  - b1=0: all voiced [1,1,1,1,1,1,1,1]
  - b1=16: all unvoiced [0,0,0,0,0,0,0,0]
- Annex N: Half-rate block lengths J1..J4 -- selected entries extracted
- Annex O: Gain quantizer (32 levels) -- selected values extracted:
  - b2=0: -2.000000, b2=31: 6.874496
- Annex P: PRBA24 VQ (512 entries, 3-dimensional) -- first entry only:
  - b3=0: G2=0.526055, G3=-0.328567, G4=-0.304727
- Annex Q: PRBA58 VQ (128 entries, 4-dimensional) -- first entry only:
  - b4=0: G5=-0.103660, G6=0.094597, G7=-0.013149, G8=0.081501
- Annex R: HOC VQ tables -- first entry of b5 only:
  - b5=0: H1,1=0.264108, H1,2=0.045976, H1,3=-0.200999, H1,4=-0.122344

### 7.2 Tables NOT Extracted (Must Be Sourced Elsewhere)

These multi-page numerical tables have not yet been extracted; they're large
floating-point tables that need the same `pdftotext -layout` + parser approach
used for Annexes E/F/G/H/J:

| Table | Annex | Size | TIA Source |
|-------|-------|------|------------|
| Analysis window | B | 211 values | BABA-A Annex B |
| Synthesis window | I | 211 values | BABA-A Annex I |
| Half-rate pitch table | L | 120 entries x 2 values | BABA-A Annex L |
| Half-rate V/UV codebook | M | 32 entries x 8 bits | BABA-A Annex M |
| Half-rate PRBA24 VQ | P | 512 entries x 3 values | BABA-A Annex P |
| Half-rate PRBA58 VQ | Q | 128 entries x 4 values | BABA-A Annex Q |
| Half-rate HOC VQ tables | R | 32+16+16+8 entries x 4 values | BABA-A Annex R |
| Half-rate interleaving | S | 36 rows x 2 cols | BABA-A Annex S / BBAC-1 Annex E |
| Tone parameters | T | 256 entries x 3 values | BABA-A Annex T |
| FIR LPF coefficients | D | 21 values | Extracted (see Section 7.4) |
| Block lengths (full-rate) | J | 48 rows x 6 cols | BABA-A Annex J |
| Block lengths (half-rate) | N | 48 rows x 4 cols | BABA-A Annex N |

**Extraction note:** These tables span pages ~96-145 of the TIA-102.BABA-A PDF.
They are multi-page numerical tables that must be programmatically extracted
(not manually transcribed — see project guidelines on transcription error rates).
All tables are normatively specified in the TIA standard and are the authoritative
source for any implementation.

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


## 12. Extracted Annex Tables (Full-Rate)

Source: TIA-102.BABA-A PDF, Annexes E, F, G, H, J (pages 84–105).
Extracted programmatically from `pdftotext -layout` output with verification:
Annex J rows sum to L̂; Annex H covers all 144 bit positions exactly once
(one OCR fix: symbol 51 right-bit "c9(5)" → "c0(5)"); Annex F has 5 entries
per L̂; Annex G entry count equals L̂ − 6 for every L̂.

### 12.0 Format and Conventions

The table data is stored as CSV files alongside this specification, one file
per annex, in the `annex_tables/` directory:

```
standards/TIA-102.BABA-A/annex_tables/
├── annex_e_gain_quantizer.csv    (64 rows)
├── annex_f_gain_allocation.csv   (240 rows)
├── annex_g_hoc_allocation.csv    (1272 rows, flattened)
├── annex_h_interleave.csv        (72 rows)
└── annex_j_block_lengths.csv     (48 rows)
```

Each CSV has a three-line `#`-prefixed header with the annex title, PDF page
reference, and a one-line verification note, followed by a column-name row and
the data rows. Row counts above are data rows only.

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

