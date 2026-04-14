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

```rust
use std::f64::consts::PI;

/// Encode a refined fundamental frequency omega_0 (rad/sample) into the
/// 8-bit pitch index b_0. Caller must ensure omega_0 is within the
/// pitch estimator's range (2*pi/123.125 to 2*pi/19.875).
fn pitch_encode(omega_0: f64) -> u8 {
    let b0 = ((4.0 * PI / omega_0) - 39.0).floor() as i32;
    debug_assert!((0..=207).contains(&b0), "omega_0 out of range");
    b0 as u8
}

/// Decode the 8-bit pitch index b_0 to the reconstructed fundamental
/// frequency omega_0 (rad/sample). Returns None for reserved values (208-255).
fn pitch_decode(b0: u8) -> Option<f64> {
    if b0 > 207 {
        return None;  // reserved
    }
    Some(4.0 * PI / (b0 as f64 + 39.5))
}

/// Compute the number of harmonics L from omega_0, per Eq. 31/47.
/// Result is always in 9..=56 for valid omega_0.
fn harmonics_count(omega_0: f64) -> u8 {
    let inner = (PI / omega_0 + 0.25).floor();
    let l = (0.9254 * inner).floor() as u8;
    debug_assert!((9..=56).contains(&l));
    l
}

/// Compute the number of V/UV bands K from L, per Eq. 48.
fn vuv_band_count(l: u8) -> u8 {
    if l <= 36 { (l + 2) / 3 } else { 12 }
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

```rust
/// [23,12] Golay generator matrix (systematic form).
/// Each row is 23 bits: 12 identity bits followed by 11 parity bits.
/// Stored as u32 with MSB = bit 22 (identity column 0), LSB = bit 0 (parity x¹⁰).
const GOLAY_23_12_GEN: [u32; 12] = [
    0b100000000000_11000111010,  // Row 0
    0b010000000000_01100011101,  // Row 1
    0b001000000000_11110110100,  // Row 2
    0b000100000000_01111011010,  // Row 3
    0b000010000000_00111101101,  // Row 4
    0b000001000000_11011001100,  // Row 5
    0b000000100000_01101100110,  // Row 6
    0b000000010000_00110110011,  // Row 7
    0b000000001000_11011100011,  // Row 8
    0b000000000100_10101001011,  // Row 9
    0b000000000010_10010011111,  // Row 10
    0b000000000001_10001110101,  // Row 11
];

/// Golay decode: correct up to 3 errors in a 23-bit code word.
/// Returns (corrected_12_info_bits, error_count).
fn golay_23_12_decode(codeword: u32) -> (u16, u8) {
    // Standard syndrome-based Golay decoding.
    // Compute syndrome = codeword * H^T (11-bit syndrome).
    // Look up syndrome in 2048-entry table for error pattern.
    // If weight(error_pattern) <= 3, correct and return.
    // Otherwise, flag uncorrectable.
    todo!("Use standard Golay decode algorithm")
}
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

```rust
/// [15,11] Hamming generator matrix parity columns.
/// For each of 11 info bits, the 4 parity bits to append.
const HAMMING_15_11_PARITY: [u8; 11] = [
    0b1111, // row 0
    0b1110, // row 1
    0b1101, // row 2
    0b1100, // row 3
    0b1011, // row 4
    0b1010, // row 5
    0b1001, // row 6
    0b0111, // row 7
    0b0110, // row 8
    0b0101, // row 9
    0b0011, // row 10
];

/// Hamming decode: correct up to 1 error in a 15-bit code word.
/// Returns (corrected_11_info_bits, error_count).
fn hamming_15_11_decode(codeword: u16) -> (u16, u8) {
    // Compute 4-bit syndrome.
    // If syndrome != 0, flip the indicated bit.
    // Extract 11 information bits.
    todo!("Standard Hamming decode")
}
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

```rust
/// Generate the 115-element PN sequence p_r(0..=114) for full-rate IMBE
/// bit modulation. Seed is the 12-bit Golay info word u0 (range [0, 4095]).
fn imbe_pn_sequence_fullrate(u0: u16) -> [u16; 115] {
    debug_assert!(u0 < 4096);
    let mut pr = [0u16; 115];
    pr[0] = (16u32 * u0 as u32) as u16;  // p_r(0) = 16·u0, fits in u16 since u0 ≤ 4095
    for n in 1..115 {
        pr[n] = ((173u32 * pr[n - 1] as u32 + 13849) & 0xFFFF) as u16;
    }
    pr
}

/// Extract one mask bit from p_r(n): bit 15 of the 16-bit value.
#[inline]
fn pn_mask_bit(pr_n: u16) -> u8 {
    (pr_n >> 15) as u8
}

/// Compute the 8 modulation vectors m̂_0..m̂_7 for full-rate IMBE.
/// Each vector is returned as (bits, length). Bits are packed LSB-first:
/// vector element k is at bit position k.
fn imbe_modulation_vectors(u0: u16) -> [(u32, u8); 8] {
    let pr = imbe_pn_sequence_fullrate(u0);
    let mask_range = |start: usize, len: usize| -> u32 {
        let mut bits = 0u32;
        for k in 0..len {
            bits |= (pn_mask_bit(pr[start + k]) as u32) << k;
        }
        bits
    };
    [
        (0,                            23),  // m_0 (Eq. 86)
        (mask_range(1, 23),            23),  // m_1 (Eq. 87)
        (mask_range(24, 23),           23),  // m_2 (Eq. 88)
        (mask_range(47, 23),           23),  // m_3 (Eq. 89)
        (mask_range(70, 15),           15),  // m_4 (Eq. 90)
        (mask_range(85, 15),           15),  // m_5 (Eq. 91)
        (mask_range(100, 15),          15),  // m_6 (Eq. 92)
        (0,                             7),  // m_7 (Eq. 93)
    ]
}
```

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

```rust
/// Deinterleave a 144-bit IMBE frame (72 dibits) into 8 code word vectors.
/// Returns [c0, c1, c2, c3, c4, c5, c6, c7].
///
/// The full interleaving table (Annex H, 72 rows) must be extracted from
/// TIA-102.BABA-A Annex H (pages ~130-131 of the PDF).
fn deinterleave_imbe_fullrate(dibits: &[u8; 72]) -> [u32; 8] {
    // Each dibit contributes bit_1 (MSB) and bit_0 (LSB).
    // The Annex H table maps (symbol_index, bit_position) -> (vector_index, bit_index).
    // After deinterleaving:
    //   c0: 23 bits, c1: 23 bits, c2: 23 bits, c3: 23 bits (Golay-encoded)
    //   c4: 15 bits, c5: 15 bits, c6: 15 bits (Hamming-encoded)
    //   c7: 7 bits (uncoded)
    todo!("Apply Annex H inverse mapping")
}
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

```rust
/// [24,12] Extended Golay encode.
/// Takes 12 information bits, returns 24-bit code word.
fn golay_24_12_encode(info: u16) -> u32 {
    let golay23 = golay_23_12_encode(info);  // 23-bit systematic Golay
    let parity = (golay23.count_ones() % 2) as u32;  // overall parity
    (golay23 << 1) | parity  // append parity as LSB
}

/// [24,12] Extended Golay decode.
/// Returns (corrected_12_info_bits, error_count).
fn golay_24_12_decode(codeword: u32) -> (u16, u8) {
    // Strip the overall parity bit, decode as [23,12] Golay,
    // then verify overall parity for 4-error detection.
    todo!("Extended Golay decode")
}
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

```rust
/// Deinterleave a 72-bit half-rate AMBE frame (36 dibits) into 4 code word vectors.
/// Returns [c0, c1, c2, c3].
///
/// The Annex S table is partially reproduced above. Full table in
/// TIA-102.BABA-A Annex S and TIA-102.BBAC-1 Annex E.
fn deinterleave_ambe_halfrate(dibits: &[u8; 36]) -> [u32; 4] {
    // c0: 24 bits ([24,12] extended Golay)
    // c1: 23 bits ([23,12] Golay)
    // c2: 23 bits ([23,12] Golay)
    // c3: variable (uncoded remainder)
    todo!("Apply Annex S inverse mapping")
}
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

```rust
/// Extract voice frame dibits from a 4V burst.
/// Input: 180 dibit symbols (one TDMA timeslot).
/// Output: 4 arrays of 36 dibits each.
fn extract_4v_voice_frames(slot: &[u8; 180]) -> [[u8; 36]; 4] {
    let mut frames = [[0u8; 36]; 4];
    // Voice 1: symbols 10..45
    frames[0].copy_from_slice(&slot[10..46]);
    // Voice 2: symbols 47..82
    frames[1].copy_from_slice(&slot[47..83]);
    // Voice 3: symbols 96..131
    frames[2].copy_from_slice(&slot[96..132]);
    // Voice 4: symbols 133..168
    frames[3].copy_from_slice(&slot[133..169]);
    frames
}

/// Extract voice frame dibits from a 2V burst.
/// Input: 180 dibit symbols.
/// Output: 2 arrays of 36 dibits each.
fn extract_2v_voice_frames(slot: &[u8; 180]) -> [[u8; 36]; 2] {
    let mut frames = [[0u8; 36]; 2];
    frames[0].copy_from_slice(&slot[10..46]);
    frames[1].copy_from_slice(&slot[47..83]);
    frames
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

```rust
/// FIR low-pass filter for pitch autocorrelation (Annex D).
/// h_LPF(n), n = -10..10, symmetric.
const LPF_COEFFS: [f64; 21] = [
    -0.002898, // n = -10
    -0.002831, // n = -9
     0.005666, // n = -8
     0.016601, // n = -7
     0.008800, // n = -6
    -0.026955, // n = -5
    -0.055990, // n = -4
    -0.015116, // n = -3
     0.118754, // n = -2
     0.278990, // n = -1
     0.351338, // n = 0
     0.278990, // n = 1
     0.118754, // n = 2
    -0.015116, // n = 3
    -0.055990, // n = 4
    -0.026955, // n = 5
     0.008800, // n = 6
     0.016601, // n = 7
     0.005666, // n = 8
    -0.002831, // n = 9
    -0.002898, // n = 10
];
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

## 9. Rust Struct Definitions

```rust
/// Full-rate IMBE voice frame (Phase 1 FDMA).
/// 88 information bits after FEC decoding and PN demodulation.
#[derive(Clone, Debug)]
pub struct ImbeFrame {
    /// The 8 decoded bit vectors (u0..u7).
    /// u0: 12 bits (MSB-aligned in u16)
    /// u1: 12 bits
    /// u2: 12 bits
    /// u3: 12 bits
    /// u4: 11 bits
    /// u5: 11 bits
    /// u6: 11 bits (note: some references show 10 for c6 -- see note below)
    /// u7: 7 bits
    pub vectors: [u16; 8],

    /// Error counts per vector from FEC decoding.
    pub errors: [u8; 8],

    /// Total error count across all vectors.
    pub error_total: u8,

    /// True if this frame should be repeated (too many errors).
    pub repeat: bool,

    /// True if this frame should be muted (sustained errors).
    pub mute: bool,
}

/// Half-rate AMBE+2 voice frame (Phase 2 TDMA).
/// 49 information bits after FEC decoding and PN demodulation.
#[derive(Clone, Debug)]
pub struct AmbeFrame {
    /// Decoded voice parameters.
    pub b0_pitch: u8,       // 7 bits (0..119)
    pub b1_vuv: u8,         // 5 bits (0..31)
    pub b2_gain: u8,        // 5 bits (0..31)
    pub b3_prba24: u16,     // 9 bits (0..511)
    pub b4_prba58: u8,      // 7 bits (0..127)
    pub b5_hoc1: u8,        // 5 bits (0..31)
    pub b6_hoc2: u8,        // 4 bits (0..15)
    pub b7_hoc3: u8,        // 4 bits (0..15)
    pub b8_hoc4: u8,        // 3 bits (0..7)

    /// Error counts per vector from FEC decoding.
    pub errors: [u8; 4],    // e0..e3

    /// Total error count.
    pub error_total: u8,

    /// Frame status.
    pub repeat: bool,
    pub mute: bool,

    /// Tone frame indicator (None = voice, Some(id) = tone).
    pub tone_id: Option<u8>,
}

/// Half-rate tone frame parameters.
#[derive(Clone, Debug)]
pub struct ToneFrame {
    pub tone_id: u8,        // I_D (0..255)
    pub f0_hz: f32,         // Fundamental frequency
    pub l1: u8,             // Spectral shaping index 1
    pub l2: u8,             // Spectral shaping index 2
    pub is_silence: bool,   // true if I_D == 255
}

/// Raw FEC-encoded voice frame before decoding.
/// Used as intermediate representation during extraction from air interface.
#[derive(Clone, Debug)]
pub struct RawImbeCodewords {
    /// 8 FEC-encoded code words.
    /// c0..c3: 23 bits each (Golay-encoded)
    /// c4..c6: 15 bits each (Hamming-encoded)
    /// c7: 7 bits (uncoded)
    pub codewords: [u32; 8],
}

#[derive(Clone, Debug)]
pub struct RawAmbeCodewords {
    /// 4 FEC-encoded code words.
    /// c0: 24 bits (extended Golay)
    /// c1: 23 bits (Golay)
    /// c2: 23 bits (Golay)
    /// c3: variable (uncoded)
    pub codewords: [u32; 4],
}

/// Smoothed error tracking state for adaptive smoothing.
#[derive(Clone, Debug)]
pub struct ErrorTracker {
    /// Exponentially smoothed error rate.
    pub e_r: f64,
    /// Count of consecutive invalid/errored frames.
    pub consecutive_bad: u8,
    /// Previous frame's spectral amplitudes (for frame repeat attenuation).
    pub prev_frame: Option<Vec<f64>>,
}

impl ErrorTracker {
    pub fn new() -> Self {
        Self {
            e_r: 0.0,
            consecutive_bad: 0,
            prev_frame: None,
        }
    }

    /// Assess full-rate IMBE frame errors.
    /// Returns (should_repeat, should_mute).
    pub fn assess_fullrate(&mut self, e0: u8, e_total: u8) -> (bool, bool) {
        let repeat = e0 >= 2 && (e_total as f64) >= 10.0 + 40.0 * self.e_r;
        let mute = self.e_r > 0.0875;
        // Update e_r (exponential smoothing -- exact alpha from BABA-A)
        (repeat, mute)
    }

    /// Assess half-rate AMBE frame errors.
    /// Returns (should_repeat, should_mute).
    pub fn assess_halfrate(&mut self, e0: u8, e_total: u8) -> (bool, bool) {
        let repeat = e0 >= 4 || e_total >= 6;
        let mute = self.e_r > 0.096 || self.consecutive_bad >= 4;
        (repeat, mute)
    }
}
```

### 9.1 Bit Extraction Helpers

```rust
/// Extract a single IMBE frame's 88 bits from an LDU bit stream.
///
/// `ldu_bits`: The deinterleaved LDU bit stream (after status symbol removal).
/// `frame_index`: Which of the 9 frames to extract (0-based, 0..8).
///
/// NOTE: The exact bit positions require the Annex A tables from TIA-102.BAAA-B.
/// These 864-entry tables must be programmatically extracted from the BAAA-B PDF.
pub fn extract_imbe_from_ldu(
    ldu_bits: &[u8],
    frame_index: usize,
    // Position table: for each of the 9 frames, the 144 bit indices in the LDU
    position_table: &[[usize; 144]; 9],
) -> RawImbeCodewords {
    let positions = &position_table[frame_index];
    let mut bits = [0u8; 144];
    for (i, &pos) in positions.iter().enumerate() {
        bits[i] = ldu_bits[pos];
    }
    // Now partition 144 bits into 8 code words using Annex H deinterleave
    deinterleave_fullrate(&bits)
}

/// Extract a single half-rate AMBE frame from a TDMA burst.
///
/// `burst_symbols`: 180 dibit symbols of one TDMA timeslot.
/// `voice_index`: Which voice frame (0-based).
/// `is_4v`: true for 4V burst (4 frames), false for 2V (2 frames).
pub fn extract_ambe_from_burst(
    burst_symbols: &[u8; 180],
    voice_index: usize,
    is_4v: bool,
) -> RawAmbeCodewords {
    let start = match (is_4v, voice_index) {
        (_, 0) => 10,
        (_, 1) => 47,
        (true, 2) => 96,
        (true, 3) => 133,
        _ => panic!("Invalid voice_index for burst type"),
    };

    let mut dibits = [0u8; 36];
    dibits.copy_from_slice(&burst_symbols[start..start + 36]);

    deinterleave_halfrate(&dibits)
}

/// Pack an ImbeFrame's 88 bits into an 11-byte array.
/// Bits are packed MSB-first: u0[11]..u0[0], u1[11]..u1[0], ..., u7[6]..u7[0].
pub fn imbe_to_jmbe_bytes(frame: &ImbeFrame) -> [u8; 11] {
    let mut output = [0u8; 11];
    let mut bit_pos = 0usize;
    let widths = [12, 12, 12, 12, 11, 11, 11, 7];

    for (i, &width) in widths.iter().enumerate() {
        for b in (0..width).rev() {
            let byte_idx = bit_pos / 8;
            let bit_idx = 7 - (bit_pos % 8);
            if (frame.vectors[i] >> b) & 1 == 1 {
                output[byte_idx] |= 1 << bit_idx;
            }
            bit_pos += 1;
        }
    }
    output
}

/// Pack an AmbeFrame's 49 bits into a 7-byte array.
/// Bits are packed MSB-first: b0[6..0], b1[4..0], ..., b8[2..0].
pub fn ambe_to_jmbe_bytes(frame: &AmbeFrame) -> [u8; 7] {
    let mut output = [0u8; 7];
    let mut bit_pos = 0usize;

    let params: [(u16, usize); 9] = [
        (frame.b0_pitch as u16, 7),
        (frame.b1_vuv as u16, 5),
        (frame.b2_gain as u16, 5),
        (frame.b3_prba24, 9),
        (frame.b4_prba58 as u16, 7),
        (frame.b5_hoc1 as u16, 5),
        (frame.b6_hoc2 as u16, 4),
        (frame.b7_hoc3 as u16, 4),
        (frame.b8_hoc4 as u16, 3),
    ];

    for &(value, width) in &params {
        for b in (0..width).rev() {
            let byte_idx = bit_pos / 8;
            let bit_idx = 7 - (bit_pos % 8);
            if (value >> b) & 1 == 1 {
                output[byte_idx] |= 1 << bit_idx;
            }
            bit_pos += 1;
        }
    }
    output
}

/// Placeholder for full-rate deinterleaving (requires Annex H table).
fn deinterleave_fullrate(bits: &[u8; 144]) -> RawImbeCodewords {
    // Apply inverse of BABA-A Annex H interleaving table (72 rows x 2 cols).
    todo!("Implement with Annex H table from TIA-102.BABA-A")
}

/// Placeholder for half-rate deinterleaving (requires Annex S table).
fn deinterleave_halfrate(dibits: &[u8; 36]) -> RawAmbeCodewords {
    // Apply inverse of BABA-A Annex S interleaving table (36 rows x 2 cols).
    // Also available in BBAC-1 Annex E.
    todo!("Implement with Annex S table from TIA-102.BABA-A")
}
```

---

## 10. Variable Initialization (Annex A)

These initial values MUST be set at decoder startup and call start:

```rust
/// Decoder state initialization per Annex A.
pub fn init_decoder_state() -> DecoderState {
    DecoderState {
        prev_pitch_period: 100,        // P_{-1} = 100
        prev_pitch_period_2: 100,      // P_{-2} = 100
        prev_omega_0: 0.02985 * std::f64::consts::PI,  // omega_0(-1) = 0.02985*pi
        prev_magnitudes: vec![1.0; 56], // M_l(-1) = 1 for all l
        prev_l: 30,                     // L(-1) = 30
        prev_k: 10,                     // K(-1) = 10
        error_rate: 0.0,               // e_R = 0.0
        enhancement_energy: 75000.0,   // S_E = 75000
        noise_seed: 3147,              // u(-105) = 3147
    }
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

