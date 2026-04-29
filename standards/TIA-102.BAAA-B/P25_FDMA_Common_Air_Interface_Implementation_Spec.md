# P25 Phase 1 FDMA Common Air Interface -- Implementation Specification

**Source:** TIA-102.BAAA-B (Revision B, June 2017)  
**Classification:** ALGORITHM + MESSAGE_FORMAT + PROTOCOL  
**Phase:** 3 -- Implementation-ready  
**Extracted:** 2026-04-12  
**Purpose:** Self-contained spec for implementing P25 Phase 1 FDMA signal decoding:
modulation, frame sync, NID, all voice/data frame types, FEC codes, interleaving,
and complete superframe structure. No reference to the original PDF required.

**Cross-references:**
- TIA-102.BBAB / BBAC-A / BBAD-A -- Phase 2 TDMA specs (see Section 14 for FDMA-to-TDMA mapping)
- TIA-102.AABC-E -- Trunking signaling (TSBK) message opcodes carried in TSDU frames
- TIA-102.AABF-D -- Link Control word formats carried in LDU1 and TDULC
- TIA-102.BABA -- IMBE vocoder (88-bit voice frame format)
- SDRTrunk (Java) and OP25 (C++/Python) -- open-source reference implementations

---

## 1. C4FM Modulation

### 1.1 Fundamental Parameters

| Parameter | Value |
|-----------|-------|
| Channel bandwidth | 12.5 kHz |
| Bit rate | 9,600 bps |
| Symbol rate | 4,800 symbols/sec |
| Symbol period | 208.333 us (1/4800) |
| Bits per symbol | 2 (dibit) |
| Modulation | pi/4 DQPSK |
| Transmitter variants | C4FM (constant envelope FM), CQPSK (linear IQ) |

### 1.2 Dibit-to-Symbol Mapping

| Information Bits [b1 b0] | Symbol Value | C4FM Deviation | CQPSK Phase Change |
|--------------------------|--------------|----------------|-------------------|
| `01` | +3 | +1800 Hz | +135 deg |
| `00` | +1 | +600 Hz  | +45 deg  |
| `10` | -1 | -600 Hz  | -45 deg  |
| `11` | -3 | -1800 Hz | -135 deg |

```c
/* Map a raw dibit (2 bits, MSB=b1 first) to a quaternary symbol.
 * Input: dibit in 0..3 where layout is [b1, b0]. */
static const int8_t DIBIT_TO_SYMBOL[4] = {
     1,   /* 0x0 (00) -> +1 */
     3,   /* 0x1 (01) -> +3 */
    -1,   /* 0x2 (10) -> -1 */
    -3,   /* 0x3 (11) -> -3 */
};

/* Inverse: symbol to dibit.
 * Indexed as (symbol + 3): -3->0, -1->2, +1->4, +3->6.
 * Unused entries (odd offsets) are set to 0xFF (invalid). */
static const uint8_t SYMBOL_TO_DIBIT[7] = {
    0x03, 0xFF, 0x02, 0xFF, 0x00, 0xFF, 0x01,
};

/* C4FM deviation in Hz for each dibit value.
 * Index by dibit: 0x0->+600, 0x1->+1800, 0x2->-600, 0x3->-1800 */
static const float C4FM_DEVIATION_HZ[4] = { 600.0f, 1800.0f, -600.0f, -1800.0f };
```

### 1.3 Pulse Shaping -- Nyquist Raised Cosine Filter

The symbol stream (impulses scaled by symbol value) is filtered through a raised cosine
filter before modulation:

```
|H(f)| = 1.0                             for |f| <= 1920 Hz
|H(f)| = 0.5 + 0.5 * cos(2*pi*f / 1920) for 1920 < |f| <= 2880 Hz
|H(f)| = 0.0                             for |f| > 2880 Hz
```

Rolloff factor alpha = (2880 - 1920) / (2880 + 1920) = 0.2. This is effectively
alpha = 0.2 relative to the symbol rate of 4800 sym/s (Nyquist bandwidth = 2400 Hz,
excess bandwidth = 480 Hz on each side -- but the spec defines the filter shape directly).

### 1.4 C4FM Shaping Filter

After the Nyquist filter, a shaping filter compensates for the integrate-and-dump
receiver matched filter:

```
|P(f)| = (pi*f/4800) / sin(pi*f/4800)    for |f| < 2880 Hz
```

Combined C4FM modulator chain: `Dibits -> H(f) -> P(f) -> FM modulator -> RF output`

### 1.5 CQPSK Modulation (Repeater Output / LSM)

CQPSK (Compatible Quadrature Phase Shift Keying) is a linear IQ modulation used as an
alternative transmitter type, typically for repeater output. It produces a narrower
spectrum than C4FM. Both can be received by the same QPSK demodulator.

The CQPSK modulator uses a state machine with 8 phase states (0-7). Each input dibit
produces an (I, Q) pair from a 5-level set {-1.0, -0.7071, 0.0, +0.7071, +1.0}.

```c
/* CQPSK lookup entry: next phase state, I level, Q level. */
typedef struct { uint8_t next_state; int8_t i_level; int8_t q_level; } CqpskEntry;

/* CQPSK lookup table: cqpsk_table[current_phase_state][input_dibit]
 * Phase states 0-7; input dibits: 0x0=00, 0x1=01, 0x2=10, 0x3=11.
 * I/Q levels encoded as int8_t: -4=-1.0, -3=-0.7071, 0=0.0, 3=+0.7071, 4=+1.0.
 * Actual float value: level / 4.0. Use 1.0/sqrt(2.0) for the 0.7071 cases in DSP. */
static const CqpskEntry CQPSK_TABLE[8][4] = {
    /* State 0 */ {{1,  3,  3}, {3, -3,  3}, {7,  3, -3}, {5, -3, -3}},
    /* State 1 */ {{2,  0,  4}, {4, -4,  0}, {0,  4,  0}, {6,  0, -4}},
    /* State 2 */ {{3, -3,  3}, {5, -3, -3}, {1,  3,  3}, {7,  3, -3}},
    /* State 3 */ {{4, -4,  0}, {6,  0, -4}, {2,  0,  4}, {0,  4,  0}},
    /* State 4 */ {{5, -3, -3}, {7,  3, -3}, {3, -3,  3}, {1,  3,  3}},
    /* State 5 */ {{6,  0, -4}, {0,  4,  0}, {4, -4,  0}, {2,  0,  4}},
    /* State 6 */ {{7,  3, -3}, {1,  3,  3}, {5, -3, -3}, {3, -3,  3}},
    /* State 7 */ {{0,  4,  0}, {2,  0,  4}, {6,  0, -4}, {4, -4,  0}},
};
```

### 1.6 QPSK Demodulator

A single QPSK demodulator receives both C4FM and CQPSK signals:
```
RF -> FM Discriminator -> D(f) Integrate-and-Dump Filter -> Clock Recovery -> Dibits
```

Integrate-and-dump filter: `|D(f)| = sin(pi*f/4800) / (pi*f/4800)` for |f| < 2880 Hz.

Clock recovery uses a stochastic gradient algorithm (not specified by the standard).

---

## 2. Frame Sync (FS)

### 2.1 Frame Sync Pattern

Every data unit begins with a 48-bit (24-dibit) Frame Sync word. This is the primary
mechanism for detecting the start of any P25 transmission.

**Hex representation:** `0x5575F5FF77FF`

The pattern is derived from a 24-bit base value `0x04CF5F` where each bit is expanded:
- bit `1` -> dibit `11`
- bit `0` -> dibit `01`

```
Base (24 bits):   0000 0100 1100 1111 0101 1111
Expanded (48 bits):
  0101 0101  0111 0101  1111 0101  1111 1111  0111 0111  1111 1111
    55          75         F5         FF         77         FF
```

The 48-bit frame sync as a 6-byte hex value is: **`0x5575F5FF77FF`**

Bytes 5-6 of the base (`01 01 1111`) expand to `0111 0111 1111 1111` = `0x77FF`,
not `0x7FFF`. The `0x77` (not `0x7F`) at byte 5 is confirmed in PDF Table 15 (page 43).

```c
/* 48-bit frame sync pattern, MSB first (first transmitted bit is bit 47). */
/* PDF: Table 15 -- Frame Sync Word Sequence, page 43. */
/* Verification: expand 24-bit base 0x04CF5F bit-by-bit; 0->01, 1->11: PASS */
#define FRAME_SYNC      UINT64_C(0x5575F5FF77FF)
#define FRAME_SYNC_MASK UINT64_C(0x0000FFFFFFFFFFFF)  /* 48-bit mask */

/* Maximum Hamming distance for frame sync detection. */
/* OP25 uses threshold of 4-6 bit errors; SDRTrunk uses 6. */
#define FRAME_SYNC_MAX_ERRORS 6

/* Detect frame sync in a 48-bit sliding window of received bits. */
static bool detect_frame_sync(uint64_t window) {
    uint64_t diff = (window ^ FRAME_SYNC) & FRAME_SYNC_MASK;
    return __builtin_popcountll(diff) <= FRAME_SYNC_MAX_ERRORS;
}
```

**SDRTrunk cross-ref:** `P25P1FrameSync.java` uses the 48-bit pattern `0x5575F5FF77FF`.
**OP25 cross-ref:** `p25_frame_assembler_impl.cc` searches for `0x5575F5FF77FF`.

---

## 3. Network Identifier (NID)

### 3.1 NID Structure

The NID is a 64-bit field transmitted immediately after Frame Sync in every data unit.
It carries 16 bits of information protected by a (63,16,23) BCH code plus 1 parity bit.

```
Bit positions (transmitted order, MSB first):
  [63..52] = NAC (Network Access Code, 12 bits)
  [51..48] = DUID (Data Unit Identifier, 4 bits)
  [47..1]  = BCH parity (47 bits from BCH(63,16,23))
  [0]      = Overall parity bit P (even parity over all 64 bits)
```

```c
/* Extract NAC from a 64-bit NID value (bits 63..52). */
static uint16_t nid_nac(uint64_t nid) {
    return (uint16_t)((nid >> 52) & 0xFFFu);
}

/* Extract DUID from a 64-bit NID value (bits 51..48). */
static uint8_t nid_duid(uint64_t nid) {
    return (uint8_t)((nid >> 48) & 0xFu);
}
```

### 3.2 Data Unit Identifier (DUID) Values

| DUID (binary) | DUID (hex) | Parity (P) | Data Unit |
|----------------|-----------|------------|-----------|
| `0000` | `0x0` | 0 | HDU -- Header Data Unit |
| `0011` | `0x3` | 0 | TDU -- Terminator Data Unit (no LC) |
| `0101` | `0x5` | 1 | LDU1 -- Logical Link Data Unit 1 |
| `0111` | `0x7` | 0 | TSBK -- Single-block Trunking Signaling (per AABB-B §4.2; P bit value confirmed by MMDVMHost cross-reference audit, see `analysis/p25_nid_implementation_audit.md`) |
| `1010` | `0xA` | 1 | LDU2 -- Logical Link Data Unit 2 |
| `1100` | `0xC` | 0 | PDU -- Multi-block Packet Data Unit (data PDU or MBT) |
| `1111` | `0xF` | 0 | TDULC -- Terminator with Link Control |

**Note on DUID `0x7` (TSBK / TSDU):** BAAA-B does not list DUID `0x7` in its own
table, but TIA-102.AABB-B §4.2 (Trunking Control Channel Formats) normatively
assigns DUID `$7` to single-block TSBK packets (Trunking Signaling Data Units).
DUID `$C` in AABB-B is reserved for multi-block PDUs (MBT and data PDUs).

A conformant FDMA receiver therefore recognizes **seven** DUID values, not six:

| DUID | Defined by | Payload |
|------|-----------|---------|
| `0x0` / HDU | BAAA-B | Voice-call header |
| `0x3` / TDU | BAAA-B | Bare voice-call terminator |
| `0x5` / LDU1 | BAAA-B | Voice superframe half 1 |
| `0x7` / TSBK | **AABB-B §4.2** | 1–3 TSBKs (single-block trunking) |
| `0xA` / LDU2 | BAAA-B | Voice superframe half 2 |
| `0xC` / PDU | BAAA-B (+ AABB-B §5 for MBT) | Header block + data blocks (data PDU or MBT) |
| `0xF` / TDULC | BAAA-B | Voice terminator with Link Control |

**TSBK vs. header-block discriminator:** Under DUID `0xC`, the first trellis-
coded block is **always** a PDU header block (§5.2) — never a TSBK. TSBKs only
appear under DUID `0x7`. A receiver can therefore dispatch unambiguously:

- DUID `0x7` → 1–3 consecutive TSBKs, terminated by the `LB=1` flag in the last
  one (see AABB-B §4.2 for LB/P/Opcode layout).
- DUID `0xC` → single header block (§5.7.2 Format dispatch), followed by 1–3
  data blocks determined by the header's Blocks-to-Follow field.

The remaining 9 DUID values (`0x1`, `0x2`, `0x4`, `0x6`, `0x8`, `0x9`, `0xB`,
`0xD`, `0xE`) are reserved for future P25 standards work and should be treated
as unknown/drop by current receivers. DUID `0x9` in particular has appeared in
some legacy captures but is not defined in any current TIA-102 document.

```c
/* Data Unit Identifier values recognized by a conformant FDMA receiver. */
#define DUID_HDU   0x0u  /* Header Data Unit */
#define DUID_TDU   0x3u  /* Terminator (no LC) */
#define DUID_LDU1  0x5u  /* Logical Data Unit 1 */
#define DUID_TSBK  0x7u  /* Single-block TSBK / TSDU (per TIA-102.AABB-B §4.2) */
#define DUID_LDU2  0xAu  /* Logical Data Unit 2 */
#define DUID_PDU   0xCu  /* Multi-block PDU (header + data blocks; data PDU or MBT) */
#define DUID_TDULC 0xFu  /* Terminator with Link Control */

/* Validate a raw 4-bit DUID nibble. Returns 1 if known, 0 if reserved. */
static int duid_is_valid(uint8_t val) {
    val &= 0xFu;
    return (val == DUID_HDU  || val == DUID_TDU   || val == DUID_LDU1 ||
            val == DUID_TSBK || val == DUID_LDU2  || val == DUID_PDU  ||
            val == DUID_TDULC);
}
```

### 3.3 BCH(63,16,23) Encoding/Decoding

The BCH code is generated by the polynomial (degree 47):

```
g(x) = octal 6331 1413 6723 5453
     = binary: 110 011 011 001 001 100 001 011 110 111 010 011 101 100 101 011
```

This is a 48-bit polynomial (degree 47). In hex: `0x6649_0BDC_B52B` (approximate --
verify from generator matrix below).

**Generator matrix** (systematic form, 16 rows x 64 columns, octal notation):

| Row | Identity (16 bits) | Parity (48 bits, octal) |
|-----|-------------------|------------------------|
| 1  | `10 0000` | `6331 1413 6723 5452` |
| 2  | `04 0000` | `5265 5216 1472 3276` |
| 3  | `02 0000` | `4603 7114 6116 4164` |
| 4  | `01 0000` | `2301 7446 3047 2072` |
| 5  | `00 4000` | `7271 6230 7300 0466` |
| 6  | `00 2000` | `5605 6507 5263 5660` |
| 7  | `00 1000` | `2702 7243 6531 6730` |
| 8  | `00 0400` | `1341 3521 7254 7354` |
| 9  | `00 0200` | `0560 5650 7526 3566` |
| 10 | `00 0100` | `6141 3337 5170 4220` |
| 11 | `00 0040` | `3060 5557 6474 2110` |
| 12 | `00 0020` | `1430 2667 7236 1044` |
| 13 | `00 0010` | `0614 1333 7517 0422` |
| 14 | `00 0004` | `6037 1146 1164 1642` |
| 15 | `00 0002` | `5326 5070 6351 5373` |
| 16 | `00 0001` | `4662 3027 5647 3127` |

The 64th bit (P) is an even parity bit over all 63 BCH code bits. This extends the
minimum distance from 23 to 24 for even-weight error patterns.

```c
/*
 * BCH(63,16,23)+P NID generator matrix.
 * Each row is a 64-bit value: [16-bit info | 47-bit BCH parity | 1-bit overall parity].
 * Converted from PDF Table 19 (page 45) octal values to hex.
 * Row i encodes information bit (15-i), where bit 15 = NAC[11] (MSB).
 *
 * Verification (annex_tables/bch_nid_generator_matrix.csv):
 *   16 unique rows: PASS
 *   Identity portion = I_16: PASS
 *   Generator poly constant term = 1 (monic): PASS
 *
 * Usage: NID = XOR of GENERATOR_NID[i] for each set bit i in the 16-bit info word.
 *   info word = [NAC(11)..NAC(0) | DUID(3)..DUID(0)] (MSB first, bit 15 = NAC[11])
 */
static const uint64_t GENERATOR_NID[16] = {
    0x8000cd930bdd3b2aULL,  /* row  1: NAC[11] (bit 15) */
    0x4000ab5a8e33a6beULL,  /* row  2: NAC[10] (bit 14) */
    0x2000983e4cc4e874ULL,  /* row  3: NAC[9]  (bit 13) */
    0x10004c1f2662743aULL,  /* row  4: NAC[8]  (bit 12) */
    0x0800eb9c98ec0136ULL,  /* row  5: NAC[7]  (bit 11) */
    0x0400b85d47ab3bb0ULL,  /* row  6: NAC[6]  (bit 10) */
    0x02005c2ea3d59dd8ULL,  /* row  7: NAC[5]  (bit  9) */
    0x01002e1751eaceecULL,  /* row  8: NAC[4]  (bit  8) */
    0x0080170ba8f56776ULL,  /* row  9: NAC[3]  (bit  7) */
    0x0040c616dfa78890ULL,  /* row 10: NAC[2]  (bit  6) */
    0x0020630b6fd3c448ULL,  /* row 11: NAC[1]  (bit  5) */
    0x00103185b7e9e224ULL,  /* row 12: NAC[0]  (bit  4) */
    0x000818c2dbf4f112ULL,  /* row 13: DUID[3] (bit  3) */
    0x0004c1f2662743a2ULL,  /* row 14: DUID[2] (bit  2) */
    0x0002ad6a38ce9afbULL,  /* row 15: DUID[1] (bit  1) */
    0x00019b2617ba7657ULL,  /* row 16: DUID[0] (bit  0) */
};
/* Cross-reference: SDRTrunk BchCode_63_16_23.java, OP25 bch.cc. */
/* For production decoding, use syndrome-based correction (t=11 error correction). */
/* See also: annex_tables/bch_nid_generator_matrix.csv for the full source table. */
```

**Decoding approach:** For a received 64-bit NID:
1. Check overall parity (bit 0). If odd weight, at least one error.
2. Compute syndrome from the 63-bit BCH portion.
3. If syndrome is zero, no errors in BCH portion.
4. Otherwise, use error-trapping or lookup table to correct up to 11 errors
   (the code has d_min = 23, so t = 11 error correction capability).
5. Extract NAC (bits 63..52) and DUID (bits 51..48) from corrected word.
6. If the correction algorithm cannot converge to a valid codeword (more than
   `t = 11` errors, or a miscorrection into another coset leader), the NID
   cannot be reliably decoded at the algebraic layer. The standard is silent
   on what to do next — see §3.3.1.

The interop contract is at the **codeword** level: any decoder that produces
the correct `(NAC, DUID)` pair from a received word that lies within the
t=11 correction sphere of a transmitted codeword satisfies TIA-102.BAAA-B
§7.5.2. BAAA-B does not mandate a specific decoding algorithm (Berlekamp-
Massey, Peterson–Gorenstein–Zierler, syndrome-table lookup, Chase-II
over soft bits, etc. are all conformant).

**SDRTrunk cross-ref:** `NID.java` performs BCH decode, then dispatches on DUID.
**OP25 cross-ref:** `p25p1_fdma.cc` function `process_NID()`.

#### 3.3.1 Post-failure Recovery (informative)

BAAA-B §7.5.2 specifies only the code — generator polynomial, generator
matrix, and the 64th overall-parity bit. It does not prescribe a decoding
algorithm and does not describe what a receiver should do when correction
fails. This is a deliberate separation: algebraic error-control coding is a
well-studied general topic, and the standard treats the decoder as
implementer-chosen.

In practice, every production FDMA decoder layers *something* on top of the
nominal t=11 syndrome decoder to recover NIDs that would otherwise be
dropped. None of these patterns is normative, but all three are widely
deployed and all produce the same `(NAC, DUID)` output when they succeed,
so they are *interop-safe*: a receiver using any combination of these
strategies is bit-compatible with a receiver using none.

**Pattern A — NAC-forced retry (SDRTrunk, OP25, others).** A receiver that
is already tracking the expected NAC (from prior frames in the same call or
from a control-channel scan) overwrites the received NAC field with the
tracked NAC and re-runs the t=11 decoder. This recovers NIDs whose residual
errors, once the NAC field is corrected by hypothesis, fall back inside
the correction sphere.

```c
/* Informative: SDRTrunk-style NAC-hint retry. Not normative. */
int decode_nid_with_nac_hint(uint64_t received, uint16_t tracked_nac,
                             uint16_t *out_nac, uint8_t *out_duid) {
    if (bch_decode(received, out_nac, out_duid) == BCH_OK) return BCH_OK;
    if (tracked_nac == 0) return BCH_FAIL;  /* no hint available */

    /* Force the tracked NAC into bits 63..52, re-run correction. */
    uint64_t forced = (received & ~((uint64_t)0xFFFu << 52))
                    | ((uint64_t)(tracked_nac & 0xFFFu) << 52);
    return bch_decode(forced, out_nac, out_duid);
}
```

**Pattern B — Chase-II soft decoding.** A receiver with access to per-bit
demod confidence flips the `p` least-reliable bits exhaustively (2^p
combinations), runs the t=11 decoder for each, and picks the candidate
with the smallest `(BCH corrections + Chase flips)` total (or weighted
soft-distance). Typical `p` values are 3–6; the payoff grows sharply with
`p` but so does the compute cost. This approach can tolerate up to roughly
`t + p` hard errors when the high-error positions coincide with the
low-confidence bits.

**Pattern C — Matched-codeword enumeration.** A receiver that has locked
onto a specific NAC may enumerate all 7 valid `(NAC_tracked, DUID)`
codewords (one per known DUID) and pick the one with minimum hard- or
soft-distance to the received word. This is equivalent to maximum-
likelihood decoding over a tiny codebook and is cheap because there are
only seven candidates. It is most useful during initial lock or on a
fading channel where a small bump in recovery rate matters more than
cycles.

By BCH linearity, the pairwise minimum distance among any 7 codewords
sharing a fixed NAC is `≥ d_min = 23` (the XOR of any two such
codewords is itself a non-zero BCH codeword, hence weight ≥ 23). So
hard-decision Hamming-nearest-codeword lookup over the 7-entry table
reaches the **same `t = 11` correction floor** as the algebraic decoder,
with no Galois-field arithmetic — just `popcount(received XOR entry)`
seven times. This makes Pattern C usable as the *only* decoder in a
known-NAC system (transceiver / repeater); MMDVMHost (`P25NID.cpp`)
takes this approach, with `MAX_NID_ERRS = 5` as a conservative
threshold against cross-NAC false positives. For a passive multi-NAC
scanner, Pattern C is appropriate as a fast path layered above an
algebraic decoder used for first-sight NAC discovery. See
`analysis/p25_nid_implementation_audit.md` for the full third-party
audit and threshold-selection analysis.

**Interop consequences.** All three patterns either succeed (and produce a
codeword that also satisfies the nominal t=11 decoder) or fail (and the
receiver drops the NID as uncorrectable). None of them produces NID output
a conformant transmitter could not have emitted. A receiver that
implements none of them is also conformant — it just recovers fewer
marginal NIDs.

**When to skip recovery.** On a clean channel with SNR well above the
BER/SER knee, recovery patterns rarely fire and cost nothing. On a
channel saturated with false frame syncs (for example, a CQPSK receiver
with unresolved phase ambiguity), aggressive NID recovery can *amplify*
false-positive rates by turning random noise into plausible-looking NIDs.
Implementers should gate recovery on upstream lock quality (frame-sync
confidence, symbol-timing lock, NAC consistency across adjacent frames)
rather than always running it.

---

## 4. Status Symbols

### 4.1 Insertion Pattern

Status symbols are 2-bit values inserted after every 70 information bits (35 dibits)
throughout each data unit. They are NOT part of the information content and must be
stripped before processing.

| Status Symbol | Meaning | Transmitter |
|---------------|---------|-------------|
| `01` | Inbound channel busy | Fixed station only |
| `00` | Unknown (talk-around) | Subscriber only |
| `10` | Unknown (general use) | Either |
| `11` | Inbound channel idle | Fixed station only |

### 4.2 Status Symbol Positions

Within any data unit, status symbols appear at dibit positions:

```
SS position = 35 + 36*k    for k = 0, 1, 2, ...
```

That is: after dibit 34, insert SS; after dibit 70 (35+36-1), insert SS; etc.
The first 35 dibits (70 bits) are transmitted, then 1 SS dibit, then 35 info dibits,
then 1 SS dibit, and so on.

**Per-frame status symbol counts:**

| Frame Type | Info Bits | Status Symbols | Total Bits |
|------------|-----------|----------------|------------|
| HDU | 770 | 11 | 792 |
| LDU1 | 1680 | 24 | 1728 |
| LDU2 | 1680 | 24 | 1728 |
| TDU | 140 | 2 | 144 |
| TDULC | 408 | 6 | 420 |
| PDU | variable | variable | variable |

```c
/* Strip status symbols from a raw dibit stream.
 * raw:        input dibit array (includes status symbols)
 * raw_len:    number of input dibits
 * info_out:   caller-allocated output buffer for information dibits
 * status_out: caller-allocated output buffer for status dibits
 * Returns: number of information dibits written to info_out.
 *
 * SS positions in the raw stream: indices 35, 71, 107, ...
 * (every 36th dibit starting at index 35). */
static size_t strip_status_symbols(const uint8_t *raw, size_t raw_len,
                                   uint8_t *info_out, uint8_t *status_out) {
    size_t info_count = 0, status_count = 0;
    for (size_t i = 0; i < raw_len; i++) {
        if (i >= 35 && (i - 35) % 36 == 0) {
            if (status_out) status_out[status_count] = raw[i];
            status_count++;
        } else {
            if (info_out) info_out[info_count] = raw[i];
            info_count++;
        }
    }
    return info_count;
}
```

**SDRTrunk cross-ref:** `P25P1DataUnitAssembler.java` strips SS before frame decode.
**OP25 cross-ref:** Status bits removed in `p25p1_fdma.cc` during frame assembly.

---

## 5. Frame Types -- Complete Structure

### 5.1 Header Data Unit (HDU)

The HDU is transmitted at the start of every voice call. It carries encryption and
talk group information needed before voice decoding begins.

**Timing:** 82.5 ms at 9600 bps

| Field | Bits | Description |
|-------|------|-------------|
| Frame Sync (FS) | 48 | Fixed pattern 0x5575F5FF7FFF |
| Network ID (NID) | 64 | NAC + DUID(0x0) + BCH parity + P |
| Header Code Word | 648 | MI + MFID + ALGID + KID + TGID + FEC |
| Null fill | 10 | Padding zeros |
| **Subtotal (info)** | **770** | Before status expansion |
| Status Symbols | 22 (11 x 2) | Interleaved per Section 4 |
| **Total** | **792** | 396 dibits |

**Header Code Word payload (before FEC encoding):**

| Field | Bits | Description |
|-------|------|-------------|
| MI (Message Indicator) | 72 | Encryption initialization vector |
| MFID (Manufacturer ID) | 8 | 0x00 = standard, others per AABC |
| ALGID (Algorithm ID) | 8 | 0x80 = unencrypted, others per AABC |
| KID (Key ID) | 16 | Encryption key identifier |
| TGID (Talk Group ID) | 16 | Destination talk group |
| **Total payload** | **120** | = 20 six-bit hex symbols for RS |

**FEC encoding of HDU:**
1. The 120 payload bits are grouped into 20 six-bit symbols.
2. **Outer code:** RS(36,20,17) over GF(2^6) adds 16 parity symbols -> 36 symbols (216 bits).
3. The 216 bits are regrouped into 36 six-bit words, then each 6-bit word is encoded with:
4. **Inner code:** Golay(18,6,8) shortened code -> each 6-bit word becomes 18 bits.
5. 36 words x 18 bits = 648 bits = Header Code Word.

Decoding (receiver): Golay decode each 18-bit word -> 6-bit symbols -> RS(36,20,17) decode -> extract MI/MFID/ALGID/KID/TGID.

```c
typedef struct {
    uint16_t nac;       /* 12-bit Network Access Code */
    uint8_t  mi[9];     /* 72-bit Message Indicator */
    uint8_t  mfid;      /* Manufacturer ID */
    uint8_t  algid;     /* Algorithm ID (0x80 = unencrypted) */
    uint16_t kid;       /* Key ID */
    uint16_t tgid;      /* Talk Group ID */
} HeaderDataUnit;
```

### 5.2 Logical Link Data Unit 1 (LDU1)

LDU1 carries 9 IMBE voice frames (frames 1-9) plus Link Control (LC) information.

**Timing:** 180 ms (9 x 20 ms voice frames)

| Field | Bits | Description |
|-------|------|-------------|
| Frame Sync (FS) | 48 | Fixed pattern |
| Network ID (NID) | 64 | NAC + DUID(0x5) + BCH + P |
| 9 x IMBE voice frames | 9 x 144 = 1296 | FEC-encoded voice (see Section 6) |
| Link Control (LC) | 240 | LC word + Hamming/RS FEC |
| Low Speed Data (LSD) | 32 | 2 octets + cyclic(16,8,5) FEC |
| **Subtotal (info)** | **1680** | Before status expansion |
| Status Symbols | 48 (24 x 2) | |
| **Total** | **1728** | 864 dibits |

**Link Control in LDU1:**
- 72 information bits (LC_format 8 + MFID 8 + LC_information 56)
- Encoded with Hamming(10,6,3) inner code: 72 bits / 6 = 12 six-bit words -> 12 x 10 = 120 bits
- Plus RS(24,16,9) outer code over GF(2^6): 16 info symbols + 8 parity symbols = 24 symbols
- Total: 24 symbols x 10 bits/symbol = 240 bits

The 240 LC bits are distributed across the LDU1 frame between voice frames (see Section 6
for exact positions).

**Low Speed Data (LSD):**
- 2 octets (LSD1, LSD2), each protected by cyclic(16,8,5)
- 2 x 16 = 32 bits total, placed after the 9th IMBE frame

```c
typedef struct {
    uint16_t nac;
    uint8_t  voice_frames[9][11]; /* 9 x 88 IMBE information bits (packed, 11 bytes each) */
    uint8_t  lc_format;
    uint8_t  mfid;
    uint8_t  lc_info[7];          /* 56-bit LC information field */
    uint8_t  lsd[2];              /* Low Speed Data octets */
} LogicalDataUnit1;
```

### 5.3 Logical Link Data Unit 2 (LDU2)

LDU2 carries 9 IMBE voice frames (frames 10-18) plus Encryption Synchronization (ES).

**Timing:** 180 ms (9 x 20 ms voice frames)

| Field | Bits | Description |
|-------|------|-------------|
| Frame Sync (FS) | 48 | Fixed pattern |
| Network ID (NID) | 64 | NAC + DUID(0xA) + BCH + P |
| 9 x IMBE voice frames | 9 x 144 = 1296 | FEC-encoded voice |
| Encryption Sync (ES) | 240 | MI + ALGID + KID + Hamming/RS FEC |
| Low Speed Data (LSD) | 32 | 2 octets + cyclic(16,8,5) FEC |
| **Subtotal (info)** | **1680** | Before status expansion |
| Status Symbols | 48 (24 x 2) | |
| **Total** | **1728** | 864 dibits |

**Encryption Synchronization in LDU2:**
- 96 information bits (MI 72 + ALGID 8 + KID 16)
- Encoded as 16 six-bit symbols (but only 12 info + RS parity)
- Wait -- the ES uses RS(24,12,13) with 12 information symbols and 12 parity symbols:
  - 12 info symbols x 6 bits = 72 bits ... that is only MI.
  - Actually: MI (72 bits = 12 six-bit symbols) + ALGID (8 bits) + KID (16 bits) = 96 bits = 16 six-bit symbols
  - RS(24,16,9) gives 8 parity symbols -> 24 symbols total, each Hamming(10,6,3) encoded
  - 24 x 10 = 240 bits

**Correction per the spec:** The ES field uses the same structure as the LC field:
- 16 six-bit info symbols (96 bits: MI + ALGID + KID)
- RS(24,16,9) outer code: 16 info + 8 parity = 24 symbols
- Hamming(10,6,3) inner code on each symbol: 24 x 10 = 240 bits

**However**, the summary text says ES uses RS(24,12,13). Let me reconcile: The spec defines
G_ES as RS(24,12,13) -- this would mean 12 info symbols (72 bits) + 12 parity symbols.
The ALGID and KID (24 bits = 4 symbols) may be encoded separately or the info content
may be structured differently. Per SDRTrunk/OP25 implementations:

- **OP25 implementation:** LDU2 ES carries MI (72 bits) protected by RS(24,12,13)
  and ALGID+KID (24 bits) carried in a separate field with Hamming protection only.
- **SDRTrunk:** Treats ES as MI(72) + ALGID(8) + KID(16) with combined FEC.

The 240-bit ES block in LDU2 contains:
- 24 Hamming(10,6,3) encoded words = 240 bits
- Of those 24 six-bit symbols: 12 info (MI) + 12 RS parity per RS(24,12,13), PLUS
  ALGID and KID carried in additional Hamming words interleaved in the structure

```c
typedef struct {
    uint16_t nac;
    uint8_t  voice_frames[9][11]; /* 9 x 88 IMBE information bits (packed, 11 bytes each) */
    uint8_t  mi[9];               /* 72-bit Message Indicator */
    uint8_t  algid;               /* Algorithm ID */
    uint16_t kid;                 /* Key ID */
    uint8_t  lsd[2];              /* Low Speed Data octets */
} LogicalDataUnit2;
```

### 5.4 Terminator Data Unit (TDU)

Simple terminator marking the end of a voice transmission.

| Field | Bits | Description |
|-------|------|-------------|
| Frame Sync (FS) | 48 | Fixed pattern |
| Network ID (NID) | 64 | NAC + DUID(0x3) + BCH + P |
| Null padding | 28 | Zeros |
| **Subtotal (info)** | **140** | |
| Status Symbols | 4 (2 x 2) | |
| **Total** | **144** | 72 dibits |

No payload data. Used for simple end-of-call signaling.

```c
typedef struct {
    uint16_t nac;
    /* No payload -- end-of-call marker only. */
} TerminatorDataUnit;
```

### 5.5 Terminator with Link Control (TDULC)

Terminator carrying a complete Link Control word, used at end of voice to deliver
final LC information (e.g., call duration, GPS location).

| Field | Bits | Description |
|-------|------|-------------|
| Frame Sync (FS) | 48 | Fixed pattern |
| Network ID (NID) | 64 | NAC + DUID(0xF) + BCH + P |
| LC Code Word | 288 | LC + Golay/RS FEC |
| Null padding | 20 | Zeros |
| **Subtotal (info)** | **420** | Wait - recheck: 48+64+288+20 = 420; with 6 SS = 432 |
| Status Symbols | 12 (6 x 2) | |
| **Total** | **432** | See note |

**Correction from spec:** The spec says "Expanded with 6 status symbols" on the TDULC.
Let me recalculate: 48 + 64 + 288 + 20 = 420 info bits. With 6 status symbols (12 bits):
420 + 12 = 432 bits = 216 dibits. However, the summary says TDU+LC is ~440 bits.
The exact count depends on status symbol placement.

**LC Code Word FEC for TDULC:**
- 72 information bits (LC_format + MFID + LC_information)
- Grouped into 12 six-bit symbols
- **Outer code:** RS(24,12,13) over GF(2^6): 12 info + 12 parity = 24 symbols
- **Inner code:** Golay(24,12,8) extended Golay: each 12-bit pair (two 6-bit symbols
  concatenated) -> 24 bits
- 12 Golay code words x 24 bits = 288 bits

```c
typedef struct {
    uint16_t nac;
    uint8_t  lc_format;
    uint8_t  mfid;
    uint8_t  lc_info[7]; /* 56-bit LC information field */
} TerminatorWithLc;
```

### 5.6 Trunking Signaling Data Unit (TSDU)

The TSDU carries Trunking Signaling Block (TSBK) messages. Per **TIA-102.AABB-B
§4.2**, the TSDU wire format uses **DUID = `0x7`** — *not* DUID = `0xC`.
BAAA-B §7.5.1 lists six DUIDs and leaves the remaining ten "reserved for use in
trunking or other systems"; AABB-B §4.2 is the normative assignment for two of
them (`$7` for single-block TSBK, `$C` for multi-block PDU). See §3.2 for the
full seven-DUID table. The TSBK *message* opcodes are defined in TIA-102.AABC-E.

A TSDU contains 1-3 TSBK blocks, each 12 octets (96 bits) before FEC. Each TSBK
is individually trellis-coded using the rate-1/2 trellis code (same as
unconfirmed data blocks).

| Field | Bits | Description |
|-------|------|-------------|
| Frame Sync (FS) | 48 | |
| Network ID (NID) | 64 | DUID = `0x7` (per AABB-B §4.2) |
| TSBK block(s) | 196 each | Trellis-coded (98 dibits each); 1, 2, or 3 blocks |
| Null fill | variable | To next SS boundary |
| Status Symbols | variable | |

Each TSBK before trellis coding: 12 octets = 96 bits. After rate-1/2 trellis:
48 dibits input → 98 dibits output → 196 bits per TSBK.

The last TSBK in a TSDU has its "Last Block" (LB) flag set (octet 0 bit 7). Up
to 3 TSBKs can be concatenated in a single TSDU. See AABB-B §5 (Figures 5-1 and
5-2) for the single/double/triple-TSBK slot layouts.

**Discriminator vs. DUID `0xC`:** A receiver that has decoded the NID dispatches
on DUID *before* looking at block contents. DUID `0x7` → always TSDU with 1–3
TSBKs. DUID `0xC` → always a multi-block PDU header (data PDU, Response, or MBT
per §5.7). The two framings never overlap at the NID layer, so the
TSBK/header-block discriminator question of §3.2 is resolved by DUID alone.

**Cross-ref:** See TIA-102.AABB-B Implementation Spec §4 for TSBK framing and
Status Symbol placement, and TIA-102.AABC-E Implementation Spec for the
complete TSBK opcode table.

### 5.7 Packet Data Unit (PDU)

Variable-length data unit for non-voice FDMA traffic carried under **DUID `0xC`**:
user data packets (Confirmed, Unconfirmed, Response) and multi-block trunking
(MBT Standard and Alternative). All share DUID `0xC` and the same wire-level
framing — header block + 1..N data blocks; a 5-bit **Format** field in the
header block's octet 0 selects the subtype.

Single-block TSBK traffic is *not* a PDU subtype — it uses its own DUID `0x7`
(TSDU; see §5.6). Under DUID `0xC`, the first trellis-coded block is always a
PDU header block, never a TSBK.

#### 5.7.1 PDU Frame Structure

| Field | Bits | Description |
|-------|------|-------------|
| Frame Sync (FS) | 48 | |
| Network ID (NID) | 64 | DUID = 0xC |
| Header Block | 196 | Trellis-coded (rate 1/2), 12 info octets |
| Data Block 1..N | 196 each | Trellis-coded (rate 1/2 or 3/4 per subtype) |
| Null fill | variable | Pad to next SS boundary |
| Status Symbols | variable | 1 SS dibit per 35 info dibits |

**Trellis input per block:**
- Rate ½: 12 octets = 96 bits = 48 dibits in → 98 dibits (196 bits) out.
- Rate ¾: 18 octets = 144 bits = 48 tribits in → 98 dibits (196 bits) out.

The header block is *always* rate ½. Data blocks follow the rate chosen by the subtype
dispatch table below. See Section 10 for the trellis FSM and constellation mapping.

#### 5.7.2 Format Field (Subtype Dispatch)

Every PDU header block and every TSBK begins with an 8-bit field in octet 0:

```
 Bit 7  Bit 6  Bit 5   Bits 4..0
 ┌────┬───────┬─────┬─────────────┐
 │  0 │  A/N  │ I/O │   Format    │  <- non-TSBK (header blocks)
 └────┴───────┴─────┴─────────────┘
 ┌────┬───────┬─────────────────────┐
 │ LB │   P   │    Opcode (6 bits)  │  <- TSBK payload (no Format; dispatched by LB/P/Opcode)
 └────┴───────┴─────────────────────┘
```

- **A/N** (octet 0 bit 6): ARQ/Non-ARQ. `1` = confirmation desired (confirmed);
  `0` = no confirmation (unconfirmed or Response).
- **I/O** (octet 0 bit 5): Outbound (`1` = FNE→SU) vs. inbound (`0` = SU→FNE).
- **Format** (octet 0 bits 4:0): 5-bit subtype selector.

**Subtype dispatch table:**

| Format | Binary | Name | Data block rate | A/N | Defined in |
|--------|--------|------|-----------------|-----|------------|
| `0x03` | `%00011` | Response Packet | ½ | 0 | BAAA-B §5.5 |
| `0x15` | `%10101` | Unconfirmed Data Packet | ½ | 0 | BAAA-B §5.6 |
| `0x15` | `%10101` | MBT Standard (Unconfirmed Data + trunking SAP) | ½ | 1 | AABB-B §5 |
| `0x16` | `%10110` | Confirmed Data Packet | ¾ | 1 | BAAA-B §5.4 |
| `0x17` | `%10111` | Alternative MBT (AMBT) | ½ | 1 | AABB-B §5 |

All other Format values (0..2, 4..14, 16..20, 24..31) are reserved.

**Implementation note:** Format `0x15` serves dual duty — it is both the Unconfirmed
Data Packet header *and* the Standard Multi-Block Trunking header. The two are
distinguished by the SAP ID in octet 1 (trunking SAPs are `0x3D`/`0x3F`; data SAPs
are everything else) and by the A/N bit. AABB-B §5 layers MBT-specific field
semantics onto the generic Unconfirmed Data Packet structure.

#### 5.7.3 Unconfirmed Data Packet (Format `0x15`)

Header block (12 octets before trellis coding):

```
O\B  | 7    6    5    4    3    2    1    0
-----+----------------------------------------
  0  | 0   A/N  I/O   1    0    1    0    1    Format = %10101
  1  | 1    1         SAP Identifier (6 bits)
  2  |           Manufacturer's ID
  3  |         Logical Link ID (24 bits,
  4  |              octets 3-5)
  5  |
  6  | 1              Blocks to Follow (7 bits)
  7  | 0    0    0    Pad Octet Count (5 bits)
  8  |              Reserved (set to 0)
  9  | 0    0        Data Header Offset (6 bits)
 10  |           Header CRC (CRC-CCITT-16)
 11  |
```

**Data blocks:** 12 octets user data each, rate ½ trellis. No per-block CRC and no
serial number. The final 4 octets of the last data block carry a 32-bit Packet CRC
covering all user data + pad. See Section 13 for CRC polynomials.

**Payload capacity:** `user_octets = 12 × BTF − 4 − pad_octets`.

**Pad octet placement:** BAED-A §5.4 recommends `P_MAXU = 11` as the maximum pad
octet count for Unconfirmed packets. Since the last data block has
`12 − 4 = 8` octets of non-CRC payload area, pads beyond 8 cascade into the
second-to-last data block (BAAA-B §5.4.3 convention: greedy fill from the end).

*Worked example — 14 octets of user data, pad_count = 10 (BTF = 2):*

```
Block 1 (12 octets):  [U0 U1 U2 U3 U4 U5 U6 U7 U8 U9 P0 P1]
                       └─── user data (10 octets) ────┘└pad┘
Block 2 (12 octets):  [U10 U11 U12 U13 P2 P3 P4 P5 P6 P7 |  CRC-32  |]
                       └─ user (4) ┘└── pad (6) ──┘└─── CRC-32 (4) ─┘
```

Total = 2×12 = 24 octets = 14 user + 10 pad + 4 CRC. Pad fills 2 octets at the
end of block 1's payload area and 6 octets at the end of block 2's user-data
area (just before the CRC-32). Pad octet value is conventionally `0x00`.

#### 5.7.4 Confirmed Data Packet (Format `0x16`)

Header block adds a re-sync bit, sequence number N(S), and Fragment Sequence Number
Field (FSNF):

```
O\B  | 7    6    5    4    3    2    1    0
-----+----------------------------------------
  0  | 0   A/N  I/O   1    0    1    1    0    Format = %10110
  1  | 1    1         SAP Identifier
  2  |           Manufacturer's ID
  3  |         Logical Link ID (octets 3-5)
  4  |
  5  |
  6  |FMF             Blocks to Follow (7 bits)
  7  | 0    0    0    Pad Octet Count (5 bits)
  8  |Syn        N(S)              FSNF
  9  | 0    0        Data Header Offset (6 bits)
 10  |           Header CRC
 11  |
```

- **FMF** (octet 6 bit 7): Full Message Flag. `1` = header carries every data block
  of the fragment; `0` = header accompanies a selective retry (only the blocks in
  the retry list are present). The Revision-B definition of FMF corrects an earlier
  error that could cause a retransmitted packet to be silently discarded when the
  receiver missed the original header transmission.
- **Syn** (octet 8 bit 7): Re-synchronize sequence numbers. Receiver accepts the
  packet's N(S) and FSNF as authoritative, resetting its V(R). Should only be
  asserted on registration messages, not normal data.
- **N(S):** Sequence number mod 8. Receiver tracks V(R) = last valid N(S) accepted;
  accepts packets where N(S) ∈ {V(R), V(R)+1 mod 8}.
- **FSNF** (octet 8 bits 3:0): 4-bit field = `[LIC:1 | FSN:3]` for fragmentation
  across multiple packets. See BAED-A Implementation Spec §4.

**Data blocks:** 18 octets each, rate ¾ trellis. Each block carries a 7-bit serial
number + 9-bit CRC-9 + 16 octets of user data (last block reserves 4 of those for
the 32-bit Packet CRC). See §5.7.3 payload formula adjusted for 16-octet blocks:
`user_octets = 16 × BTF − 4 − pad_octets`.

See §13.2 for CRC-9 polynomial (`x^9 + x^6 + x^4 + x^3 + 1`).

**Pad octet placement:** BAED-A §5.4 recommends `P_MAXC = 15` as the maximum pad
octet count for Confirmed packets. Last data block has 12 octets of non-CRC
payload area (`16 − 4`); pads beyond 12 cascade into the second-to-last data
block (BAAA-B §5.4.3). Each block's serial number and CRC-9 are computed over
the 16 user-data octets as-padded, then the Packet CRC-32 is computed over all
user-data (including pad) across all blocks.

*Worked example — 20 octets of user data, pad_count = 8 (BTF = 2):*

```
Block 1 (18 octets): [SN1 CRC9_1] [U0 U1 U2 ... U15]
                                   └── user data (16 octets) ──┘
Block 2 (18 octets): [SN2 CRC9_2] [U16 U17 U18 U19 P0 P1 P2 P3 P4 P5 P6 P7 | CRC-32 |]
                                   └─ user (4) ─┘└─── pad (8) ───┘└── CRC-32 (4) ──┘
```

Total user-area bytes = 2×16 = 32 = 20 user + 8 pad + 4 CRC. All 8 pad octets
fit in the last block (pad_count ≤ 12), so the second-to-last block carries no
pad. If pad_count were 14, last block would hold 12 pad (filling its non-CRC
area) and the second-to-last block would take the remaining 2 pad octets at the
end of its user-data area.

#### 5.7.5 Response Packet (Format `0x03`)

Sent by a confirmed-data receiver back to the sender to ACK, NACK, or request
selective retry of specific blocks:

```
O\B  | 7    6    5    4    3    2    1    0
-----+----------------------------------------
  0  | 0    0   I/O   0    0    0    1    1    Format = %00011
  1  | Class      Type            Status
  2  |           Manufacturer's ID
  3  |           Destination LLID (octets 3-5)
  4  |
  5  |
  6  | X              Blocks to Follow
  7  |
  8  |           Source LLID (when X=0)
  9  |
 10  |           Header CRC
 11  |
```

- **X** (octet 6 bit 7): `1` = response to an asymmetric confirmed packet (Source
  LLID is null); `0` = response to an Enhanced Address confirmed packet (Source
  LLID carries the responder's address).
- **Blocks to Follow:** Number of data blocks carrying selective-retry flag bitmaps
  (SACK only; ACK/NACK responses have BTF = 0).

**Class / Type / Status dispatch (BAAA-B Table 10):**

| Class | Type | Status | Meaning |
|-------|------|--------|---------|
| `%00` | `%001` | N(R) | ACK — all blocks received correctly |
| `%01` | `%000` | N(R)* | NACK — illegal format (N(R) may be meaningless) |
| `%01` | `%001` | N(R) | NACK — packet CRC-32 failure |
| `%01` | `%010` | N(R) | NACK — memory full |
| `%01` | `%011` | FSN | NACK — out-of-sequence fragment |
| `%01` | `%100` | N(R) | NACK — undeliverable |
| `%01` | `%101` | V(R) | NACK — out of sequence *(deprecated in Revision B)* |
| `%01` | `%110` | N(R) | NACK — invalid user, disallowed by system |
| `%10` | `%000` | N(R) | SACK — selective retry needed for some blocks |

N(R) echoes the N(S) of the confirmed packet being acknowledged.

**SACK data block format** (one 12-octet block covers up to 64 block flags; two
blocks cover the full 127-block maximum). Flag bit `fN = 1` means block N was
received correctly; `fN = 0` means block N must be retransmitted. Unused flag bits
(beyond the actual packet's BTF) are set to 1.

*Single-block SACK* (for Confirmed packets with ≤ 64 data blocks; Response header
BTF = 1):

```
O\B  | 7    6    5    4    3    2    1    0
-----+----------------------------------------
  0  | f7   f6   f5   f4   f3   f2   f1   f0
  1  | f15  f14  f13  f12  f11  f10  f9   f8
  ...
  7  | f63  f62  f61  f60  f59  f58  f57  f56
  8  |
  9  |           Packet CRC-32
 10  |
 11  |
```

*Two-block SACK* (for Confirmed packets with 65–127 data blocks; Response header
BTF = 2). First SACK block carries `f0..f63` and has no CRC. Second SACK block
carries `f64..f127` in the same octet-0..7 layout, with the Packet CRC-32 in
octets 8–11. The CRC-32 covers both SACK data blocks' flag areas (octets 0–7 of
block 1 concatenated with octets 0–7 of block 2 = 16 octets of flag data) per
BAAA-B §5.3.3. The unused flag `f127` is always set to 1 because the maximum
packet size is 127 blocks (BTF field is 7 bits but value 0 is not a valid packet).

```
Block 1 (octets 0-11):
O\B  | 7    6    5    4    3    2    1    0
-----+----------------------------------------
  0  | f7   f6   f5   f4   f3   f2   f1   f0
  1  | f15  f14  f13  f12  f11  f10  f9   f8
  ...
  7  | f63  f62  f61  f60  f59  f58  f57  f56
  8  |           (padded to 12 octets;
  9  |            last 4 octets are user-data
 10  |            flag continuation is in
 11  |            the next block)

Block 2 (octets 0-11):
O\B  | 7    6    5    4    3    2    1    0
-----+----------------------------------------
  0  | f71  f70  f69  f68  f67  f66  f65  f64
  1  | f79  f78  f77  f76  f75  f74  f73  f72
  ...
  7  | f127 f126 f125 f124 f123 f122 f121 f120
  8  |
  9  |           Packet CRC-32
 10  |
 11  |
```

Some implementations simplify by capping SACK responses at 64 blocks per Response
and requiring the sender to track retry state across multiple Response packets.
This avoids the two-block layout entirely at the cost of more round-trips for
long packets. Real-world P25 data traffic rarely produces packets with more than
64 blocks, so the two-block SACK is uncommon in practice.

#### 5.7.6 Enhanced Addressing (Symmetric, Formats `0x15` / `0x16` with EA SAP)

Direct Data and Repeated Data configurations require both source and destination
addresses on every packet. The mechanism reuses the Unconfirmed (`0x15`) or Confirmed
(`0x16`) Format values but sets the SAP field to the **EA SAP** sentinel
(`0x1F` per BAAC-D). This signals the receiver that the first data block is not
user payload but a **Second Header** carrying the source LLID and the real SAP.

```
HEADER BLOCK           DATA BLOCK 1 (Second Header)       DATA BLOCK 2..N
┌──────────────┐      ┌──────────────────────────┐       ┌──────────────┐
│ Dest LLID    │      │ Source LLID              │       │ User data    │
│ SAP = $1F    │ ───► │ SAP = <real SAP>         │ ───►  │ ...          │
│ Header CRC   │      │ Header CRC 2             │       │              │
└──────────────┘      └──────────────────────────┘       └──────────────┘
```

For Unconfirmed Enhanced Addressing the Second Header is a plain 12-octet block
(mirrors the Unconfirmed header layout). For Confirmed Enhanced Addressing the
Second Header is an 18-octet confirmed data block with serial number + CRC-9 and
the source LLID + real SAP in its data area.

#### 5.7.7 Payload-Family Lens (How This Maps to TSDU, MBT, Data)

Because Format `0x15` is shared between user data and Standard MBT, implementers
often find it clearer to dispatch on **payload family** first (by examining SAP
and the Format/TSBK-header choice) before dispatching on subtype:

| Payload family | DUID | Header selector | Payload spec |
|----------------|------|------------------|--------------|
| TSDU (1–3 TSBKs) | `0x7` (per AABB-B §4.2) | Each block is a TSBK: octet 0 = `LB/P/Opcode`, no PDU header | TIA-102.AABC-E |
| MBT Standard (trunking) | `0xC` | Format = `0x15`, SAP = `0x3D`/`0x3F` | TIA-102.AABB-B §5, AABC-E for message content |
| MBT Alternative (AMBT) | `0xC` | Format = `0x17`, SAP = `0x3D`/`0x3F` | TIA-102.AABB-B §5 |
| Unconfirmed data | `0xC` | Format = `0x15`, SAP ≠ trunking | TIA-102.BAED-A (LLC), BAEB-C (SNDCP) |
| Confirmed data | `0xC` | Format = `0x16` | TIA-102.BAED-A (LLC), BAEB-C (SNDCP) |
| Response | `0xC` | Format = `0x03` | BAAA-B §5.5 (this spec) |

See also the consolidated cross-document view in
`analysis/fdma_pdu_frame.md`, which walks through end-to-end packet flow with
worked byte-level examples.

**Cross-references:**
- TSBK framing (12-octet block, CRC-16, 1–3 per TSDU): AABB-B Implementation Spec §4
- MBT Standard (`0x15`) and Alternative (`0x17`) headers: AABB-B Implementation Spec §5
- Full PDU header field semantics + SAP routing + fragmentation: BAED-A Implementation Spec §2–4
- SNDCP IP encapsulation layered on data PDUs: BAEB-C Implementation Spec
- Trellis coding (rate ½ and rate ¾ FSMs): this spec §10
- CRC polynomials (Header CRC-CCITT-16, CRC-9, Packet CRC-32): this spec §13

---

## 6. IMBE Voice Frame Placement

### 6.1 IMBE Frame Structure

Each IMBE voice frame contains 88 information bits (code words c0 through c7) which are
FEC-encoded to 144 bits for transmission. The IMBE codec (TIA-102.BABA) produces
88 bits per 20 ms frame.

| IMBE Code Word | Bits | Protection |
|----------------|------|------------|
| c0 | 12 | Most protected (Hamming + Golay) |
| c1 | 12 | Hamming(15,11,3) variants |
| c2 | 12 | |
| c3 | 12 | |
| c4 | 11 | |
| c5 | 11 | |
| c6 | 10 | |
| c7 | 8 | Least protected |
| **Total** | **88** | |

After FEC encoding with inner Hamming/Golay codes: 144 bits per voice frame.

### 6.2 Voice Frames per LDU

Each LDU contains 9 IMBE voice frames:
- LDU1: IMBE frames 1-9
- LDU2: IMBE frames 10-18

9 frames x 144 bits = 1296 voice bits per LDU.

### 6.3 Voice Frame Start Positions in LDU1

After stripping status symbols, the approximate dibit positions of IMBE frames within
LDU1 (864 total dibits including SS):

| IMBE Frame | Start Symbol | End Symbol | Notes |
|------------|-------------|------------|-------|
| 1 | 57 | ~130 | After FS + NID |
| 2 | 131 | ~225 | |
| 3 | 226 | ~319 | |
| 4 | 320 | ~414 | |
| 5 | 415 | ~509 | |
| 6 | 510 | ~603 | |
| 7 | 604 | ~698 | |
| 8 | 699 | ~787 | |
| 9 | 788 | ~863 | Followed by LSD |

**Note on symbol positions:** These are dibit indices in the TRANSMITTED stream (including
status symbols). The exact boundaries depend on SS insertion. Between voice frames,
LC/ES signaling bits and RS parity bits are interleaved.

### 6.4 Voice Frame Start Positions in LDU2

| IMBE Frame | Start Symbol | End Symbol |
|------------|-------------|------------|
| 10 | 921 | ~994 |
| 11 | 995 | ~1089 |
| 12 | 1090 | ~1183 |
| 13 | 1184 | ~1278 |
| 14 | 1279 | ~1373 |
| 15 | 1374 | ~1467 |
| 16 | 1468 | ~1562 |
| 17 | 1563 | ~1652 |
| 18 | 1653 | ~1727 |

### 6.5 Interleaving of Voice and Signaling Bits

Within each LDU, voice bits and signaling (LC/ES) bits are interleaved in a specific
pattern. The general structure between each pair of IMBE frames:

```
[IMBE frame N] [signaling bits] [IMBE frame N+1]
```

The signaling bits carry portions of the LC word (LDU1) or ES word (LDU2) encoded
with Hamming(10,6,3). Each 10-bit Hamming word carries 6 information bits.

The exact bit-by-bit interleaving pattern for voice frames within LDU1 and LDU2 is
defined in Annex A of TIA-102.BAAA-B. The complete symbol-by-symbol tables have been
extracted and are available as CSV files:

- **HDU (396 symbols):** `annex_tables/annex_a2_hdu_transmit_bit_order.csv`
- **LDU1 (864 symbols):** `annex_tables/annex_a3_ldu1_transmit_bit_order.csv`
- **LDU2 (864 symbols):** `annex_tables/annex_a4_ldu2_transmit_bit_order.csv`

Each CSV maps symbol (dibit) index to source field and bit names (e.g., `c_0(11)` for
IMBE codeword c0 bit 11). Symbol indices are consecutive with no gaps (verified).

**For implementation:** Use the bit position tables from these CSVs, or the corresponding
arrays in SDRTrunk or OP25:
- **SDRTrunk:** `P25P1Message.java` has hardcoded bit position arrays for extracting
  each IMBE frame from the deinterleaved LDU.
- **OP25:** `imbe_decoder.cc` contains the voice frame extraction indices.

---

## 7. Superframe Structure

### 7.1 Voice Superframe

A voice call consists of a superframe:

```
HDU -> LDU1 -> LDU2 -> LDU1 -> LDU2 -> ... -> TDU (or TDULC)
         |<--- 360 ms --->|<--- 360 ms --->|
         |<- 180ms->|<-180ms->|
```

| Unit | Duration | Dibits | Bits (with SS) |
|------|----------|--------|----------------|
| HDU | 82.5 ms | 396 | 792 |
| LDU1 | 180 ms | 864 | 1728 |
| LDU2 | 180 ms | 864 | 1728 |
| TDU | 15 ms | 72 | 144 |
| TDULC | ~45 ms | ~216 | ~432 |

An LDU pair (LDU1 + LDU2) forms a 360 ms voice superframe containing 18 IMBE voice
frames (20 ms each). The superframe repeats until the call ends.

### 7.2 Timing Relationships

```
Symbol rate:     4800 sym/s
Bit rate:        9600 bps
IMBE frame:      20 ms (= 96 symbols = 192 bits, but only 144 bits are voice+FEC;
                        the rest is signaling interleaved around the frame)
LDU duration:    180 ms = 864 symbols = 1728 bits (including SS)
LDU pair:        360 ms
Voice frames/s:  50 (9 per 180 ms LDU = 50/sec from two LDUs/360ms... actually 9+9=18/360ms = 50/sec)
```

### 7.3 Data Superframe

Data transmissions use PDU frames independently -- no superframe structure.

```
[PDU] ... [PDU]    (each starts with FS + NID)
```

TSDU transmissions also stand alone -- each TSDU (DUID `0x7`) carries 1–3 TSBK
blocks directly after the NID, with no PDU header block (see §5.6).

---

## 8. FEC Codes -- Complete Reference

### 8.1 Cyclic Code (16,8,5)

**Used for:** Low Speed Data (LSD) octets in LDU1 and LDU2.

Generator polynomial: `g(x) = x^8 + x^7 + x^6 + x^5 + 1`

```c
/* Cyclic(16,8,5) generator polynomial: x^8 + x^7 + x^6 + x^5 + 1 = 0x01E1 */
#define CYCLIC_16_8_GENERATOR 0x01E1u

/* Generator matrix in systematic form [I_8 | P_8].
 * Each row: 8-bit identity (high byte) | 8-bit parity (low byte) = 16-bit code word. */
static const uint16_t CYCLIC_16_8_GENERATOR_MATRIX[8] = {
    0x804E, /* row 1 */
    0x4027, /* row 2 */
    0x208F, /* row 3 */
    0x10DB, /* row 4 */
    0x08F1, /* row 5 */
    0x04E4, /* row 6 */
    0x0272, /* row 7 */
    0x0139, /* row 8 */
};

/* Encode one LSD octet with cyclic(16,8,5). */
static uint16_t cyclic_16_8_encode(uint8_t data) {
    uint16_t codeword = 0;
    for (int i = 0; i < 8; i++) {
        if ((data >> (7 - i)) & 1u) {
            codeword ^= CYCLIC_16_8_GENERATOR_MATRIX[i];
        }
    }
    return codeword;
}
```

### 8.2 Golay Codes

#### 8.2.1 Golay(24,12,8) -- Extended

**Used for:** TDULC inner code (pairs of RS symbols).

Generator polynomial (for the (23,12,7) standard code from which this is extended):
```
g(x) = x^11 + x^10 + x^6 + x^5 + x^4 + x^2 + 1
Octal: 6165
```

The (24,12,8) extended code appends an even parity bit to the (23,12,7) code.

```c
/* Golay(24,12,8) generator matrix (systematic form).
 * Each row: [12-bit identity | 12-bit parity] packed into low 24 bits of uint32_t.
 * Octal from spec converted to hex.
 * NOTE: Octal-to-binary conversion of the parity columns must be verified
 * bit-by-bit. Cross-reference with SDRTrunk Golay24.java or OP25 golay24.cc. */
static const uint32_t GOLAY_24_12_GENERATOR[12] = {
    0x800C75u, /* row  1: identity 0x800, parity octal 6165 */
    0x40063Bu, /* row  2: 2000 3073 */
    0x200F68u, /* row  3: 1000 7550 -- verify */
    0x1007B4u, /* row  4: 0400 3664 */
    0x0803DAu, /* row  5: 0200 1732 */
    0x040D99u, /* row  6: 0100 6631 */
    0x0206CDu, /* row  7: 0040 3315 */
    0x010367u, /* row  8: 0020 1547 */
    0x008DC6u, /* row  9: 0010 6706 */
    0x004A97u, /* row 10: 0004 5227 */
    0x00293Eu, /* row 11: 0002 4476 */
    0x0018EBu, /* row 12: 0001 4353 */
};
```

#### 8.2.2 Golay(18,6,8) -- Shortened

**Used for:** HDU inner code (each RS symbol).

Shortened from the (24,12,8) by deleting the leftmost 6 info columns.

```c
/* Golay(18,6,8) generator matrix (systematic form).
 * Each row: [6-bit identity | 12-bit parity] = 18 bits, packed into low 18 bits of uint32_t.
 * These are rows 7-12 of the (24,12,8) matrix with the left 6 identity columns removed.
 * Same caveat as GOLAY_24_12_GENERATOR -- verify against reference implementation. */
static const uint32_t GOLAY_18_6_GENERATOR[6] = {
    0x206CDu, /* row 1: octal 40 3315 */
    0x10367u, /* row 2: octal 20 1547 */
    0x08DC6u, /* row 3: octal 10 6706 */
    0x04A97u, /* row 4: octal 04 5227 */
    0x0293Eu, /* row 5: octal 02 4476 */
    0x018EBu, /* row 6: octal 01 4353 */
};
```

**Implementation shortcut.** Implementations that already include
Golay(24,12,8) for TDULC need not maintain a separate (18,6,8) generator
or decoder. Encode by zero-padding the 6 info bits into a 12-bit word
(high 6 bits zero) and passing to the (24,12,8) encoder; the low 18
bits of the 24-bit output are the (18,6,8) codeword. Decode by
zero-extending received 18 bits to 24 and running the (24,12,8)
decoder; the low 6 bits of the recovered 12-bit info word are the
(18,6,8) information. This is what MMDVMHost's `P25Data.cpp` does
(`encodeHeaderGolay` / `decodeHeaderGolay`). The `GOLAY_18_6_GENERATOR`
table above is therefore optional — useful only if a project ships
without a (24,12,8) implementation. See
`analysis/p25_hdu_ldu_lsd_implementation_audit.md`.

### 8.3 Hamming Codes

#### 8.3.1 Hamming(10,6,3) -- Shortened

**Used for:** Inner code for LC words in LDU1, ES words in LDU2.

Generator polynomial: `g(x) = x^4 + x + 1` (octal: 23)

```c
/* Hamming(10,6,3) generator matrix (systematic form).
 * Each row: [6-bit identity | 4-bit parity] = 10 bits.
 * Row 5 parity 0011 confirmed by MMDVMHost cross-reference audit:
 * Hamming.cpp encode1063 implements parity equations
 *   d[6] = d[0]^d[1]^d[2]^d[5]
 *   d[7] = d[0]^d[1]^d[3]^d[5]
 *   d[8] = d[0]^d[2]^d[3]^d[4]
 *   d[9] = d[1]^d[2]^d[3]^d[4]
 * which match these generator rows column-by-column. See
 * analysis/p25_hdu_ldu_lsd_implementation_audit.md. */
static const uint16_t HAMMING_10_6_GENERATOR[6] = {
    0x020Eu, /* row 1: 10 0000 1110 */
    0x010Du, /* row 2: 01 0000 1101 */
    0x008Bu, /* row 3: 00 1000 1011 */
    0x0047u, /* row 4: 00 0100 0111 */
    0x0023u, /* row 5: 00 0010 0011 */
    0x001Cu, /* row 6: 00 0001 1100 */
};

/* Encode a 6-bit value with Hamming(10,6,3). */
static uint16_t hamming_10_6_encode(uint8_t data) {
    uint16_t codeword = 0;
    for (int i = 0; i < 6; i++) {
        if ((data >> (5 - i)) & 1u) {
            codeword ^= HAMMING_10_6_GENERATOR[i];
        }
    }
    return codeword;
}

/* Decode Hamming(10,6,3): correct single bit errors.
 * Returns 6-bit data. Sets *error_detected if syndrome != 0.
 * Syndrome = parity-check matrix H applied to received word.
 * Syndrome points to the error bit position if non-zero. */
static uint8_t hamming_10_6_decode(uint16_t received, bool *error_detected) {
    /* Compute 4-bit syndrome: s = H * received^T over GF(2). */
    /* H rows: 1110011000, 1101010100, 1011100010, 0111100001 */
    /* (matching HAMMING_10_6_GENERATOR parity columns transposed) */
    uint8_t syndrome = 0;
    /* ... (syndrome-based single-error correction; t=1) ... */
    *error_detected = (syndrome != 0);
    return (uint8_t)((received >> 4) & 0x3Fu);
}
```

#### 8.3.2 Hamming(15,11,3) -- Standard

**Used for:** Inner code for IMBE voice data sub-words.

Same generator polynomial: `g(x) = x^4 + x + 1`

```c
/* Hamming(15,11,3) generator matrix (systematic form).
 * Each row: [11-bit identity | 4-bit parity] = 15 bits. */
static const uint16_t HAMMING_15_11_GENERATOR[11] = {
    0x400Fu, /* row  1 */
    0x200Eu, /* row  2 */
    0x100Du, /* row  3 */
    0x080Cu, /* row  4 */
    0x040Bu, /* row  5 */
    0x020Au, /* row  6 */
    0x0109u, /* row  7 */
    0x0087u, /* row  8 */
    0x0046u, /* row  9 */
    0x0025u, /* row 10 */
    0x0013u, /* row 11 */
};
```

### 8.4 Reed-Solomon Codes over GF(2^6)

All RS codes operate over GF(2^6) with primitive polynomial `p(x) = x^6 + x + 1`.
This generates a field of 64 elements (including 0). Each symbol is 6 bits.

#### 8.4.1 GF(2^6) Arithmetic Tables

```c
/* Primitive polynomial for GF(2^6): x^6 + x + 1 = 0b1000011 = 0x43. */
#define GF64_PRIMITIVE 0x43u

/*
 * Exponential table: GF64_EXP[i] = alpha^i mod p(x), for i = 0..63.
 * alpha^63 == alpha^0 == 1 (field order is 63).
 * Programmatically generated from primitive polynomial x^6 + x + 1.
 * Verification: EXP[LOG[b]] == b for b=1..63 (PASS); LOG[EXP[i]] == i for i=0..62 (PASS).
 * See annex_tables/gf64_lookup_tables.csv for the full verified table.
 * OCR errors in spec Table 6 corrected: rows e=8..15 had 0x15/0x33/0x3B (wrong);
 *   correct values are 0x30=48 for e=10 (was 0x15=21), etc. Use this table.
 */
static const uint8_t GF64_EXP[64] = {
    /* e=0..7   */ 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x03, 0x06,
    /* e=8..15  */ 0x0c, 0x18, 0x30, 0x23, 0x05, 0x0a, 0x14, 0x28,
    /* e=16..23 */ 0x13, 0x26, 0x0f, 0x1e, 0x3c, 0x3b, 0x35, 0x29,
    /* e=24..31 */ 0x11, 0x22, 0x07, 0x0e, 0x1c, 0x38, 0x33, 0x25,
    /* e=32..39 */ 0x09, 0x12, 0x24, 0x0b, 0x16, 0x2c, 0x1b, 0x36,
    /* e=40..47 */ 0x2f, 0x1d, 0x3a, 0x37, 0x2d, 0x19, 0x32, 0x27,
    /* e=48..55 */ 0x0d, 0x1a, 0x34, 0x2b, 0x15, 0x2a, 0x17, 0x2e,
    /* e=56..63 */ 0x1f, 0x3e, 0x3f, 0x3d, 0x39, 0x31, 0x21, 0x01,
};

/*
 * Logarithm table: GF64_LOG[b] = e where alpha^e = b, for b = 1..63.
 * GF64_LOG[0] = 0xFF (undefined; 0 has no discrete logarithm).
 */
static const uint8_t GF64_LOG[64] = {
    /*  0 */ 0xFF,
    /*  1 */   0,  1,  6,  2, 12,  7, 26,  3, 32, 13, 35,  8, 48, 27, 18,
    /* 16 */   4, 24, 33, 16, 14, 52, 36, 54,  9, 45, 49, 38, 28, 41, 19, 56,
    /* 32 */   5, 62, 25, 11, 34, 31, 17, 47, 15, 23, 53, 51, 37, 44, 55, 40,
    /* 48 */  10, 61, 46, 30, 50, 22, 39, 43, 29, 60, 42, 21, 20, 59, 57, 58,
};

/* GF(2^6) multiplication using EXP/LOG tables. */
static uint8_t gf64_mul(uint8_t a, uint8_t b) {
    if (a == 0 || b == 0) return 0;
    return GF64_EXP[(GF64_LOG[a] + GF64_LOG[b]) % 63];
}

/* GF(2^6) addition is XOR. */
static uint8_t gf64_add(uint8_t a, uint8_t b) { return a ^ b; }
```

#### 8.4.2 RS(36,20,17) -- G_HDR (HDU)

**Used for:** Header Data Unit outer code.

- 20 information symbols (120 bits): MI + MFID + ALGID + KID + TGID
- 16 parity symbols (96 bits)
- Minimum distance: 17 -> corrects up to 8 symbol errors

Generator polynomial (coefficients in octal over GF(2^6)):
```
g_HDR(x) = 60 + 73x + 46x^2 + 51x^3 + 73x^4 + 05x^5 + 42x^6 + 64x^7
          + 33x^8 + 22x^9 + 27x^10 + 21x^11 + 23x^12 + 02x^13
          + 35x^14 + 34x^15 + x^16
```

```c
/* RS(36,20,17) generator polynomial coefficients (GF(2^6) elements).
 * g(x) = g[0] + g[1]*x + ... + g[16]*x^16. g[16] = 1 (monic).
 * Coefficients converted from octal (spec notation) to hex. */
static const uint8_t RS_36_20_GENERATOR[17] = {
    0x30, 0x3B, 0x26, 0x29, 0x3B, 0x05, 0x22, 0x34,
    0x1B, 0x12, 0x17, 0x11, 0x13, 0x02, 0x1D, 0x1C,
    0x01, /* x^16 coefficient = 1 (monic) */
};
```

#### 8.4.3 The Two RS(24, ·, ·) Codes — G_LC and G_ES

P25 defines two distinct Reed–Solomon codes that share codeword length
24 but differ in dimension and minimum distance. These are easy to
confuse because both are tabulated as "RS(24,…)" generator matrices
in the standard. The MMDVMHost cross-reference audit
(`analysis/p25_hdu_ldu_lsd_implementation_audit.md`) confirms the
correct assignments:

- **G_LC = RS(24,12,13):** 12 info symbols, 12 parity symbols, d_min = 13,
  degree-12 generator. Used for: **LDU1 LC code word** and **TDULC LC
  code word**. The LC content (1 LCF byte + 1 MFID byte + 7 info bytes
  = 72 bits = 12 six-bit symbols) fills the 12 info symbols exactly.
- **G_ES = RS(24,16,9):** 16 info symbols, 8 parity symbols, d_min = 9,
  degree-8 generator. Used for: **LDU2 Encryption Sync** (9-byte MI +
  ALGID + 2-byte KID = 12 bytes ≈ 16 six-bit symbols).

(Earlier revisions of this spec reversed these assignments — LDU1 LC
was incorrectly listed as RS(24,16,9). MMDVMHost's `P25Data.cpp` calls
`decode241213` in `decodeLDU1` and `decode24169` in `decodeLDU2`,
which is the correct mapping.)

Generator polynomial for RS(24,12,13) (labeled G_LC in spec, degree 12):
```
g_LC(x) = 50 + 41x + 02x^2 + 74x^3 + 11x^4 + 60x^5 + 34x^6 + 71x^7
         + 03x^8 + 55x^9 + 05x^10 + 71x^11 + x^12
```

```c
/* RS(24,12,13) generator polynomial (G_LC / TDULC).
 * g(x) = g[0] + g[1]*x + ... + g[12]*x^12. g[12] = 1 (monic).
 * Coefficients converted from octal (spec notation) to hex. */
static const uint8_t RS_24_12_GENERATOR[13] = {
    0x28, 0x21, 0x02, 0x3C, 0x09, 0x30, 0x1C, 0x39,
    0x03, 0x2D, 0x05, 0x39, 0x01,
};
```

Generator polynomial for RS(24,16,9) (labeled G_ES in spec, degree 8):
```
g_ES(x) = 26 + 06x + 24x^2 + 57x^3 + 60x^4 + 45x^5 + 75x^6 + 67x^7 + x^8
```

```c
/* RS(24,16,9) generator polynomial (G_ES / LDU1 LC and LDU2 ES).
 * g(x) = g[0] + g[1]*x + ... + g[8]*x^8. g[8] = 1 (monic).
 * Coefficients converted from octal (spec notation) to hex. */
static const uint8_t RS_24_16_GENERATOR[9] = {
    0x16, 0x06, 0x14, 0x2F, 0x30, 0x25, 0x3D, 0x37,
    0x01,
};
```

#### 8.4.4 RS Generator Matrices

The full generator matrices for all three RS codes are provided in the spec in octal
notation (Tables 7, 8, 9 in the full text extraction). These are systematic matrices:
left portion is identity, right portion is parity. Each entry is a GF(2^6) element
in octal.

Either form yields identical codewords. Polynomial division (LFSR-style
remainder) and matrix multiplication (`info × [I | P]` over GF(2⁶))
both implement the systematic encoder. Choose by platform: matrix form
maps directly to the spec's tabulated generator matrices and is easier
to spot-check entry-by-entry; polynomial form needs less memory and
favors platforms with cheap shift-register loops over GF mults.
MMDVMHost picks the matrix form (`RS634717.cpp` `ENCODE_MATRIX_362017`,
`ENCODE_MATRIX_24169`, `ENCODE_MATRIX_241213`); SDRTrunk and OP25 tend
toward polynomial form. Both are conformant.

```c
/* Generic RS encoder over GF(2^6) using the generator polynomial.
 * info:     k information symbols (GF(2^6) elements)
 * k:        number of information symbols
 * gen_poly: generator polynomial coefficients, length r+1, gen_poly[r]=1 (monic)
 * n:        total code length (n = k + r)
 * codeword: caller-allocated output buffer of n symbols;
 *           on return, codeword[0..k-1] = info, codeword[k..n-1] = parity.
 *
 * Algorithm: systematic encoding by polynomial division. */
static void rs_encode(const uint8_t *info, size_t k,
                      const uint8_t *gen_poly, size_t n,
                      uint8_t *codeword) {
    size_t r = n - k;
    /* Copy info symbols into the high-order positions. */
    for (size_t i = 0; i < k; i++) codeword[i] = info[i];
    for (size_t i = k; i < n; i++) codeword[i] = 0;
    /* Polynomial division over GF(2^6). */
    for (size_t i = 0; i < k; i++) {
        if (codeword[i] != 0) {
            uint8_t factor = codeword[i];
            for (size_t j = 0; j <= r; j++) {
                codeword[i + j] = gf64_add(codeword[i + j],
                                           gf64_mul(factor, gen_poly[r - j]));
            }
        }
    }
    /* Parity symbols now occupy codeword[k..n-1]. */
}
```

### 8.5 BCH(63,16,23) -- NID Code

See Section 3.3 above for the full generator matrix.

- 16 information bits (NAC + DUID)
- 47 BCH parity bits
- 1 overall parity bit
- Total: 64 bits
- Error correction capability: up to 11 bit errors (t = (23-1)/2 = 11)

Generator polynomial (degree 47):
```
g(x) = octal 6331 1413 6723 5453
```

---

## 9. Interleaving

### 9.1 Trellis Code Interleaving (Data Blocks)

After trellis encoding, the 98-dibit output is reordered using a fixed permutation table.
This interleaving spreads burst errors across the trellis code word.

The same interleave table is used for both rate-1/2 and rate-3/4 trellis codes.

```c
/* Trellis interleave table: output_position -> input_position.
 * interleaved[out] = encoded[TRELLIS_INTERLEAVE[out]] */
static const uint8_t TRELLIS_INTERLEAVE[98] = {
     0,  1,  8,  9, 16, 17, 24, 25, 32, 33, 40, 41, 48, 49, 56, 57,
    64, 65, 72, 73, 80, 81, 88, 89, 96, 97,
     2,  3, 10, 11, 18, 19, 26, 27, 34, 35, 42, 43, 50, 51, 58, 59,
    66, 67, 74, 75, 82, 83, 90, 91,
     4,  5, 12, 13, 20, 21, 28, 29, 36, 37, 44, 45, 52, 53, 60, 61,
    68, 69, 76, 77, 84, 85, 92, 93,
     6,  7, 14, 15, 22, 23, 30, 31, 38, 39, 46, 47, 54, 55, 62, 63,
    70, 71, 78, 79, 86, 87, 94, 95,
};

/* Trellis deinterleave table: input_position -> output_position.
 * Inverse permutation of TRELLIS_INTERLEAVE, pre-computed.
 * decoded[TRELLIS_DEINTERLEAVE[i]] = received[i] */
static const uint8_t TRELLIS_DEINTERLEAVE[98] = {
     0,  1, 26, 27, 50, 51, 74, 75,  2,  3, 28, 29, 52, 53, 76, 77,
     4,  5, 30, 31, 54, 55, 78, 79,  6,  7, 32, 33, 56, 57, 80, 81,
     8,  9, 34, 35, 58, 59, 82, 83, 10, 11, 36, 37, 60, 61, 84, 85,
    12, 13, 38, 39, 62, 63, 86, 87, 14, 15, 40, 41, 64, 65, 88, 89,
    16, 17, 42, 43, 66, 67, 90, 91, 18, 19, 44, 45, 68, 69, 92, 93,
    20, 21, 46, 47, 70, 71, 94, 95, 22, 23, 48, 49, 72, 73, 96, 97,
    24, 25,
};
```

### 9.2 Voice Frame Interleaving (HDU, LDU1, LDU2)

Voice data units use a different interleaving scheme specific to each frame type.
The IMBE voice bits, Hamming/Golay FEC bits, and signaling bits are placed at specific
positions defined in Annex A of BAAA-B.

The complete bit-position tables for HDU, LDU1, and LDU2 interleaving have been
extracted from PDF Annex A and are stored as CSVs (verified: complete symbol coverage):

- **HDU (396 symbols):** `annex_tables/annex_a2_hdu_transmit_bit_order.csv`
- **LDU1 (864 symbols):** `annex_tables/annex_a3_ldu1_transmit_bit_order.csv`
- **LDU2 (864 symbols):** `annex_tables/annex_a4_ldu2_transmit_bit_order.csv`

The general structure within each frame:

**HDU interleaving:**
- 36 Golay(18,6,8) code words are arranged in a specific order within the 648-bit
  header code word field, interleaved with FS, NID, and status symbols.

**LDU1/LDU2 interleaving:**
- 9 IMBE voice frames (each 144 FEC-encoded bits) placed at fixed positions.
- Between voice frames, 10-bit Hamming words carrying LC/ES data are inserted.
- RS parity symbols are distributed throughout.
- LSD code words appear after the 9th voice frame.

**Cross-reference for pre-computed bit-index arrays:**
- **OP25:** `p25p1_fdma.cc` contains `imbe_ldu_index_table[]` arrays
- **SDRTrunk:** `P25P1MessageProcessor.java` has position arrays for each field

### 9.3 Items Out of Scope for This Spec

The following are correctly deferred and not included here:

1. **Annex A.5/A.6: TDU and TDULC transmit bit order tables** — absent from the
   83-page edition of TIA-102.BAAA-B. TDU has only null padding; TDULC structure
   follows directly from the LC code word definition in Section 5.5.
2. **IMBE voice frame internal interleaving** — defined in TIA-102.BABA (IMBE vocoder
   spec), not in BAAA-B. See the BABA-A Implementation Spec for the 88-bit frame layout.

---

## 10. Trellis Coding (Data Blocks)

### 10.1 Overview

Data blocks (PDU header, PDU data blocks, TSDU/TSBK) are protected by a trellis code
before transmission. Two rates are defined:

| Parameter | Rate 1/2 | Rate 3/4 |
|-----------|----------|----------|
| Input unit | Dibit (2 bits) | Tribit (3 bits) |
| Input symbols per block | 48 | 48 |
| Input bits per block | 96 | 144 |
| Output dibits per block | 98 | 98 |
| Output bits per block | 196 | 196 |
| Used by | Header blocks, unconfirmed data | Confirmed data blocks |
| FSM states | 4 | 8 |

Input is 48 symbols + 1 flush symbol (dibit 00 or tribit 000) = 49 inputs -> 98 outputs.

### 10.2 Rate 1/2 State Transition Table

```c
/* Rate 1/2 trellis encoder: TRELLIS_1_2[state][input_dibit] = constellation_point */
static const uint8_t TRELLIS_1_2[4][4] = {
    { 0, 15, 12,  3}, /* state 0 */
    { 4, 11,  8,  7}, /* state 1 */
    {13,  2,  1, 14}, /* state 2 */
    { 9,  6,  5, 10}, /* state 3 */
};
```

### 10.3 Rate 3/4 State Transition Table

```c
/* Rate 3/4 trellis encoder: TRELLIS_3_4[state][input_tribit] = constellation_point */
static const uint8_t TRELLIS_3_4[8][8] = {
    { 0,  8,  4, 12,  2, 10,  6, 14}, /* state 0 */
    { 4, 12,  2, 10,  6, 14,  0,  8}, /* state 1 */
    { 1,  9,  5, 13,  3, 11,  7, 15}, /* state 2 */
    { 5, 13,  3, 11,  7, 15,  1,  9}, /* state 3 */
    { 3, 11,  7, 15,  1,  9,  5, 13}, /* state 4 */
    { 7, 15,  1,  9,  5, 13,  3, 11}, /* state 5 */
    { 2, 10,  6, 14,  0,  8,  4, 12}, /* state 6 */
    { 6, 14,  0,  8,  4, 12,  2, 10}, /* state 7 */
};
```

### 10.4 Constellation Point to Dibit Pair Mapping

Each constellation point (0-15) maps to a pair of dibits for transmission:

```c
/* Constellation point to (dibit_0, dibit_1) symbol pair.
 * Symbol values are in {-3, -1, +1, +3}. */
typedef struct { int8_t d0; int8_t d1; } ConstellationPoint;

static const ConstellationPoint CONSTELLATION[16] = {
    { 1, -1}, /* 0  */
    {-1, -1}, /* 1  */
    { 3, -3}, /* 2  */
    {-3, -3}, /* 3  */
    {-3, -1}, /* 4  */
    { 3, -1}, /* 5  */
    {-1, -3}, /* 6  */
    { 1, -3}, /* 7  */
    {-3,  3}, /* 8  */
    { 3,  3}, /* 9  */
    {-1,  1}, /* 10 */
    { 1,  1}, /* 11 */
    { 1,  3}, /* 12 */
    {-1,  3}, /* 13 */
    { 3,  1}, /* 14 */
    {-3,  1}, /* 15 */
};
```

### 10.5 Trellis Encoder

```c
typedef struct { uint8_t state; } TrellisEncoder;

/* Initialize the trellis encoder. */
static void trellis_encoder_init(TrellisEncoder *enc) { enc->state = 0; }

/* Encode a block of data at rate 1/2.
 * data:   12 bytes (96 bits = 48 dibits)
 * output: 98 dibits (before interleaving); caller-allocated uint8_t[98]
 *
 * Each input dibit is processed MSB-first within each byte.
 * Next state = current input dibit (2-state trellis).
 * A flush dibit 0x0 (00) is appended at the end. */
static void trellis_encode_rate_half(TrellisEncoder *enc,
                                     const uint8_t data[12],
                                     uint8_t output[98]) {
    enc->state = 0;
    size_t out_idx = 0;
    for (int b = 0; b < 12; b++) {
        for (int shift = 6; shift >= 0; shift -= 2) {
            uint8_t input_dibit = (data[b] >> shift) & 0x03u;
            uint8_t point = TRELLIS_1_2[enc->state][input_dibit];
            output[out_idx]     = (uint8_t)SYMBOL_TO_DIBIT[(CONSTELLATION[point].d0 + 3)];
            output[out_idx + 1] = (uint8_t)SYMBOL_TO_DIBIT[(CONSTELLATION[point].d1 + 3)];
            out_idx += 2;
            enc->state = input_dibit; /* next state = current input */
        }
    }
    /* Flush with dibit 0x0 (00). */
    uint8_t point = TRELLIS_1_2[enc->state][0];
    output[96] = (uint8_t)SYMBOL_TO_DIBIT[(CONSTELLATION[point].d0 + 3)];
    output[97] = (uint8_t)SYMBOL_TO_DIBIT[(CONSTELLATION[point].d1 + 3)];
}
```

**Trellis decoding** is not specified in the standard — only the
encoder. Two valid decoder choices appear in production implementations:

1. **Viterbi (maximum-likelihood).** SDRTrunk's `TrellisCodec.java`. Keeps
   `|states|` parallel paths, computes path metrics, runs traceback at
   the end. Naturally accepts soft-decision input for ~2–3 dB of
   receiver-sensitivity gain. `O(n · |states|²)` ops with
   `|states| × n` memory.
2. **Sequential search with bounded backtracking.** MMDVMHost's
   `P25Trellis.cpp` (`fixCode12` / `fixCode34`). Walks the trellis state-
   by-state, accepting any received point that is a valid edge from
   the current state. On a stall, tries all 16 substitutions for the
   failing position, picks the one that reaches furthest, advances,
   and iterates up to 20 times; backtracks one position once. No path
   metrics, no soft input. Dramatically simpler to implement; constant
   memory; returns a clean "uncorrectable" signal when correction
   fails. On clean input, recovers the same bits as Viterbi.

For a passive SDR receiver with soft demod output, Viterbi is the
better choice — soft-decision gain translates directly into more
decoded calls at coverage edges. For a first-cut implementation or a
firmware/embedded context, sequential backtracking ships faster behind
the same `decode(input) -> Result<Payload>` interface, and both
implementations are bit-compatible on clean input. See
`analysis/p25_trellis_implementation_audit.md` for the full comparison
including a recommended test-oracle harness pattern.

---

## 11. Parser Pseudocode

### 11.1 Top-Level Frame Processing Pipeline

```
LOOP forever:
    1. RECEIVE raw dibit stream from demodulator
    
    2. FRAME_SYNC_SEARCH:
       - Correlate incoming dibits against 48-bit FS pattern (0x5575F5FF7FFF)
       - Allow up to FRAME_SYNC_MAX_ERRORS (6) bit errors
       - When found, mark start of data unit
    
    3. NID_DECODE:
       - Read next 64 bits (32 dibits) after FS
       - BCH(63,16,23) decode to extract NAC and DUID
       - If BCH decode fails, return to FRAME_SYNC_SEARCH
       - Validate NAC against expected network
    
    4. DISPATCH on DUID:
       CASE HDU (0x0):
           - Read 648 + 10 info bits (with SS stripped)
           - For each of 36 eighteen-bit words: Golay(18,6,8) decode -> 6-bit symbol
           - RS(36,20,17) decode 36 symbols -> 20 info symbols
           - Extract MI, MFID, ALGID, KID, TGID
           - Store for current voice call
           
       CASE LDU1 (0x5):
           - Read 1680 info bits (with SS stripped)
           - Extract 9 IMBE frames from known positions
           - For each IMBE frame: Hamming inner decode, then send 88 bits to vocoder
           - Extract LC Hamming words -> RS(24,16,9) decode -> LC_format, MFID, LC_info
           - Extract LSD: cyclic(16,8,5) decode -> 2 octets
           - Output 9 decoded voice frames
           
       CASE LDU2 (0xA):
           - Read 1680 info bits (with SS stripped)
           - Extract 9 IMBE frames (same positions as LDU1)
           - For each IMBE frame: Hamming inner decode -> vocoder
           - Extract ES Hamming words -> RS decode -> MI, ALGID, KID
           - Extract LSD
           - Output 9 decoded voice frames
           
       CASE TDU (0x3):
           - Read 28 null bits (with SS stripped)
           - Signal end of voice call
           
       CASE TDULC (0xF):
           - Read 288 + 20 info bits (with SS stripped)
           - For each of 12 twenty-four-bit words: Golay(24,12,8) decode -> 12-bit pair
           - Group into 24 six-bit RS symbols
           - RS(24,12,13) decode -> 12 info symbols -> LC word
           - Signal end of voice call with LC data
           
       CASE TSBK (0x7):  /* TSDU -- per AABB-B §4.2 */
           - Read 196-bit trellis-coded block
           - Deinterleave (98 dibits), Viterbi decode (rate 1/2) -> 12-byte TSBK
           - CRC-16 verify (CRC-CCITT, see §13)
           - Parse: octet 0 = LB | P | Opcode(6); dispatch via TIA-102.AABC-E
           - If LB=0 in this block, loop for the next TSBK (up to 3 per TSDU)

       CASE PDU (0xC):   /* Multi-block PDU (data, Response, or MBT) */
           - Read 196-bit trellis-coded header block
           - Deinterleave (98 dibits), Viterbi decode (rate 1/2) -> 12-byte header
           - Header CRC-CCITT-16 verify
           - Parse header: octet 0 = 0 | A/N | I/O | Format(5); SAP, LLID,
             blocks_to_follow, etc. (§5.7.2 dispatch table)
           - Read `blocks_to_follow` additional trellis-coded data blocks
             (rate chosen by Format subtype; see §5.7)
           - CRC verify per-block CRC-9 (confirmed) and/or packet CRC-32
    
    5. OUTPUT decoded frame to upper layer
    
    6. TIMING: After processing, expect next FS at:
       - After HDU: next FS in ~180ms (LDU1 follows)
       - After LDU1: next FS in ~180ms (LDU2 follows)
       - After LDU2: next FS in ~180ms (LDU1 follows)
       - After TDU/TDULC: return to FRAME_SYNC_SEARCH
```

### 11.2 Status Symbol Stripping

```
FUNCTION strip_status_symbols(raw_dibits) -> info_dibits:
    info = []
    ss_values = []
    count = 0
    FOR each dibit in raw_dibits:
        count += 1
        IF count == 36:
            ss_values.append(dibit)  // This is a status symbol
            count = 0
        ELSE:
            info.append(dibit)
    RETURN (info, ss_values)
```

Note: The first SS appears after 35 info dibits (i.e., at raw position 35, counting
from 0). Subsequent SS appear every 36 raw dibits.

---

## 12. Core Type Definitions

### 12.1 Core Types and Structs

```c
/* Data Unit Identifier -- use the DUID_* defines from Section 3.2. */

/* Network Identifier (decoded). */
typedef struct {
    uint16_t nac;  /* 12-bit Network Access Code */
    uint8_t  duid; /* Data Unit ID (one of DUID_* values) */
} Nid;

/* Decoded voice call state. */
typedef struct {
    uint16_t nac;
    uint8_t  mi[9];      /* 72-bit Message Indicator */
    uint8_t  mfid;       /* Manufacturer ID */
    uint8_t  algid;      /* Algorithm ID (0x80 = clear) */
    uint16_t kid;        /* Key ID */
    uint16_t tgid;       /* Talk Group ID */
    uint8_t  lc_format;  /* Link Control format */
    uint8_t  lc_info[7]; /* LC information (56 bits) */
} VoiceCallState;

/* A decoded P25 frame is a tagged union: tag is a DUID_* value.
 * The payload union holds the appropriate frame struct. */
typedef struct {
    uint8_t duid; /* DUID_HDU, DUID_LDU1, DUID_LDU2, DUID_TDU, DUID_TDULC, DUID_PDU */
    union {
        HeaderDataUnit     hdu;
        LogicalDataUnit1   ldu1;
        LogicalDataUnit2   ldu2;
        TerminatorDataUnit tdu;
        TerminatorWithLc   tdulc;
        /* PDU/TSDU: variable-length; handle via pointer in actual implementation. */
    } frame;
} P25Frame;

/* Status symbol values (2-bit dibit). */
#define STATUS_BUSY       0x01u /* 01 -- inbound channel busy */
#define STATUS_UNKNOWN00  0x00u /* 00 -- talk-around / unknown */
#define STATUS_UNKNOWN10  0x02u /* 10 -- general use / unknown */
#define STATUS_IDLE       0x03u /* 11 -- inbound channel idle */

/* Frame sync detection state (sliding 48-bit window). */
typedef struct {
    uint64_t window;    /* Sliding window of last 48 received bits */
    uint64_t bit_count; /* Total bits fed since reset */
} FrameSyncDetector;

static void frame_sync_detector_init(FrameSyncDetector *d) {
    d->window = 0;
    d->bit_count = 0;
}

/* Feed one bit; returns 1 if frame sync detected within error threshold. */
static int frame_sync_detector_feed(FrameSyncDetector *d, uint8_t bit) {
    d->window = ((d->window << 1) | (bit & 1u)) & FRAME_SYNC_MASK;
    d->bit_count++;
    uint64_t diff = (d->window ^ FRAME_SYNC) & FRAME_SYNC_MASK;
    return __builtin_popcountll(diff) <= FRAME_SYNC_MAX_ERRORS;
}
```

### 12.2 FEC Parameter Constants

```c
/* All FEC code parameters in one place. */

/* Cyclic(16,8,5) for LSD */
#define FEC_CYCLIC_N         16u
#define FEC_CYCLIC_K          8u
#define FEC_CYCLIC_D          5u
#define FEC_CYCLIC_GEN_POLY  0x01E1u  /* x^8 + x^7 + x^6 + x^5 + 1 */

/* Golay(18,6,8) shortened for HDU */
#define FEC_GOLAY_SHORT_N    18u
#define FEC_GOLAY_SHORT_K     6u
#define FEC_GOLAY_SHORT_D     8u

/* Golay(24,12,8) extended for TDULC */
#define FEC_GOLAY_EXT_N      24u
#define FEC_GOLAY_EXT_K      12u
#define FEC_GOLAY_EXT_D       8u
#define FEC_GOLAY_GEN_POLY   0x0C75u  /* x^11 + x^10 + x^6 + x^5 + x^4 + x^2 + 1 */

/* Hamming(10,6,3) for LDU LC/ES */
#define FEC_HAMMING_SHORT_N  10u
#define FEC_HAMMING_SHORT_K   6u
#define FEC_HAMMING_SHORT_D   3u

/* Hamming(15,11,3) for IMBE voice */
#define FEC_HAMMING_STD_N    15u
#define FEC_HAMMING_STD_K    11u
#define FEC_HAMMING_STD_D     3u
#define FEC_HAMMING_GEN_POLY 0x13u   /* x^4 + x + 1 */

/* RS(36,20,17) over GF(2^6) for HDU */
#define FEC_RS_HDR_N         36u
#define FEC_RS_HDR_K         20u
#define FEC_RS_HDR_D         17u
#define FEC_RS_HDR_T          8u  /* error correction capability */

/* RS(24,12,13) over GF(2^6) for TDULC / LDU2 ES */
#define FEC_RS_LC_N          24u
#define FEC_RS_LC_K          12u
#define FEC_RS_LC_D          13u
#define FEC_RS_LC_T           6u

/* RS(24,16,9) over GF(2^6) for LDU1 LC / LDU2 ES */
#define FEC_RS_ES_N          24u
#define FEC_RS_ES_K          16u
#define FEC_RS_ES_D           9u
#define FEC_RS_ES_T           4u

/* BCH(63,16,23)+P for NID */
#define FEC_BCH_N            64u  /* includes overall parity bit */
#define FEC_BCH_K            16u
#define FEC_BCH_D            23u
#define FEC_BCH_T            11u

/* GF(2^6) primitive polynomial: x^6 + x + 1 */
#define FEC_GF64_PRIM_POLY   0x43u   /* 1_000_011 */
#define FEC_GF64_FIELD_ORDER 63u
```

### 12.3 Frame Size Constants

```c
/* Frame size constants -- all sizes in BITS (not dibits) unless noted. */

#define FS_FRAME_SYNC_BITS         48u
#define FS_NID_BITS                64u
#define FS_STATUS_SYMBOL_BITS       2u
#define FS_STATUS_SYMBOL_PERIOD    70u  /* info bits between consecutive SS */

/* HDU */
#define FS_HDU_HEADER_CODEWORD_BITS 648u
#define FS_HDU_NULL_BITS            10u
#define FS_HDU_INFO_BITS           770u  /* FS + NID + header + nulls */
#define FS_HDU_STATUS_SYMBOLS       11u
#define FS_HDU_TOTAL_BITS          792u
#define FS_HDU_DIBITS              396u

/* LDU1 / LDU2 */
#define FS_LDU_INFO_BITS          1680u
#define FS_LDU_STATUS_SYMBOLS       24u
#define FS_LDU_TOTAL_BITS         1728u
#define FS_LDU_DIBITS              864u
#define FS_IMBE_FRAMES_PER_LDU       9u
#define FS_IMBE_BITS_PER_FRAME      88u  /* information bits */
#define FS_IMBE_FEC_BITS_PER_FRAME 144u  /* after inner FEC encoding */

/* TDU */
#define FS_TDU_NULL_BITS            28u
#define FS_TDU_INFO_BITS           140u
#define FS_TDU_STATUS_SYMBOLS        2u
#define FS_TDU_TOTAL_BITS          144u

/* TDULC */
#define FS_TDULC_LC_CODEWORD_BITS  288u
#define FS_TDULC_NULL_BITS          20u
#define FS_TDULC_STATUS_SYMBOLS      6u

/* PDU / TSDU (per trellis block) */
#define FS_TRELLIS_OUTPUT_DIBITS    98u
#define FS_TRELLIS_OUTPUT_BITS     196u

/* IMBE / LDU timing (milliseconds) */
#define FS_IMBE_FRAME_DURATION_MS   20.0
#define FS_LDU_DURATION_MS         180.0
#define FS_HDU_DURATION_MS          82.5
#define FS_LDU_PAIR_DURATION_MS    360.0
```

---

## 13. CRC Polynomials (Data Packets)

### 13.0 BAAA-B CRC Convention (All Three PDU CRCs)

BAAA-B §5.2 / §5.3.3 / §5.4.2 define all three PDU CRCs with the same formula:

```
F(x) = (x^n · M(x)  mod  G(x))  +  I(x)      (mod 2, GF(2))
```

where `n` is the CRC width (16, 9, or 32), `G(x)` is the generator polynomial,
and `I(x) = x^(n-1) + x^(n-2) + ... + x + 1` is the all-ones inversion polynomial
of the same width.

**There is no register "initial fill"** in the BAAA-B formula — it is a plain
polynomial division of the `x^n`-augmented message, followed by a final XOR with
all-ones. Expressed algorithmically:

```
// MSB-first direct CRC, new bit XOR'd into feedback.
// Equivalent to augmenting the message with n trailing zero bits.
reg = 0                                // register init is zero, NOT all-ones
for each message bit b (MSB-first):
    fb = ((reg >> (n-1)) & 1) XOR b
    reg = (reg << 1) AND ((1 << n) - 1)
    if fb: reg ^= G
wire_crc = reg XOR ((1 << n) - 1)      // final XOR with I(x)
```

The same algorithm reproduces every row of `annex_tables/pdu_crc_test_vectors.csv`
across all three CRC widths (verified 2026-04-21 — see §13.5).

**Watch-out.** Some CRC-CCITT-16 library code frames the same math differently:
"init = 0xFFFF, no XOR-out" instead of "init = 0, XOR-out = 0xFFFF". For
CRC-CCITT-16 these two framings produce identical wire values because of the
`x^16` length asymmetry, which is why swapping them works on-air even though
they are not algorithmically equivalent for all widths. Do **not** carry that
framing over to CRC-9 or CRC-32 blind-copy — those only reproduce under the
`init = 0, XOR-out = I(x)` formulation above. `op25` and SDRTrunk's
`CRCP25.correctCCITT80()` also implement the `init = 0, XOR-out = 0xFFFF`
framing (residual check `== 0 || == 0xFFFF`).

### 13.1 Header CRC (CRC-CCITT, 16-bit)

```c
/* CRC-CCITT for PDU header blocks.
 * G(x) = x^16 + x^12 + x^5 + 1 */
#define CRC_CCITT_POLY   0x1021u   /* 16-bit polynomial (bit 16 implicit) */
#define CRC_CCITT_INIT   0x0000u   /* register init per BAAA-B §5.2 */
#define CRC_CCITT_XOROUT 0xFFFFu   /* I(x) — XOR with raw remainder on output */
```

### 13.2 CRC-9 (Confirmed Data Blocks)

```c
/* CRC-9 for confirmed data blocks.
 * G(x) = x^9 + x^6 + x^4 + x^3 + 1 */
#define CRC9_POLY   0x059u  /* 9-bit polynomial (bit 9 implicit): 001011001 */
#define CRC9_INIT   0x000u  /* register init per BAAA-B §5.4.2 */
#define CRC9_XOROUT 0x1FFu  /* I(x) — XOR with raw remainder on output */
```

Message for CRC-9 is 7-bit DBSN (MSB-first) concatenated with 128 bits of
user-data, totaling 135 bits. Transmitted CRC-9 field follows (9 bits).

### 13.3 Packet CRC (CRC-32)

```c
/* CRC-32 for packet integrity.
 * G(x) = x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10
 *              + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1 */
#define CRC32_POLY   0x04C11DB7u  /* 32-bit polynomial (bit 32 implicit) */
#define CRC32_INIT   0x00000000u  /* register init per BAAA-B §5.3.3 */
#define CRC32_XOROUT 0xFFFFFFFFu  /* I(x) — XOR with raw remainder on output */
```

### 13.4 Wire-Format CRC (Post-Inversion) and Receiver Residual

All three PDU CRCs use the BAAA-B inversion polynomial `I(x) = x^(n-1) + ... + 1`
(all-ones of width n) as XOR-out. The raw polynomial remainder is XORed with
`I(x)` before being placed on the wire:

```
wire_crc = raw_remainder XOR ((1 << crc_width) - 1)
```

Equivalently, the receiver computes the remainder over the received message AND
the CRC field; the expected result is the all-ones pattern of CRC width (e.g.
`0xFFFF` for CRC-16). This matches SDRTrunk's `CRCP25.correctCCITT80()` accept
condition `residual == 0 || residual == 0xFFFF` and blip25's
`crc9_residual(block) ∈ {0, 0x1FF}` pattern.

### 13.5 Test Vectors

Canonical (input → expected wire CRC) pairs for all three PDU CRCs are in
`annex_tables/pdu_crc_test_vectors.csv` (header columns:
`xor_out_hex`, `wire_crc_hex` = post-inversion, `raw_remainder_hex` =
pre-inversion register state).

All 15 rows reproduce bit-exactly under the §13.0 convention (init=0,
MSB-first, direct feedback-XOR, final XOR with `xor_out_hex`). Verification:

```python
# Reproduce all CRC-9 rows (serial in low 7 bits of a byte, MSB-first; 128-bit data).
POLY, XOROUT, W = 0x059, 0x1FF, 9
def crc(bits, poly=POLY, w=W):
    reg = 0
    for b in bits:
        fb = ((reg >> (w-1)) & 1) ^ (b & 1)
        reg = (reg << 1) & ((1 << w) - 1)
        if fb: reg ^= poly
    return reg
# Row 12: serial=0x05, canonical 16-byte pattern
msg = [int(b) for b in f"{0x05 & 0x7F:07b}"] + \
      [int(b) for h in "0011223344556677889900AABBCCDDEE" for b in f"{int(h,16):04b}"]
assert crc(msg) == 0x180            # raw_remainder_hex
assert crc(msg) ^ XOROUT == 0x07F   # wire_crc_hex
```

Run a new CRC implementation against these vectors before exposing it on-air —
P25's MSB-first, init=0, final-XOR-with-I(x) convention differs from the
LSB-first, init=all-ones convention in most consumer modem / file-integrity CRC
libraries, and the bugs are silent.

---

## 14. FDMA-to-TDMA Cross-Reference

### 14.1 Conceptual Mapping

P25 Phase 2 TDMA (TIA-102.BBAC-A, BBAD-A) carries equivalent information to Phase 1
FDMA but in a time-slotted structure. Key correspondences:

| FDMA (BAAA-B) | TDMA (BBAC-A / BBAD-A) | Notes |
|----------------|----------------------|-------|
| HDU (DUID 0x0) | No direct equivalent | Call setup via SACCH/FACCH |
| LDU1 (DUID 0x5) | VTCH voice burst | 2 AMBE+2 frames per slot, LC via SACCH |
| LDU2 (DUID 0xA) | VTCH voice burst | ES via SACCH |
| TDU (DUID 0x3) | SACCH TDULC | End-of-call via SACCH |
| TDULC (DUID 0xF) | SACCH with LC | LC on termination |
| TSDU/TSBK (DUID 0x7) | FACCH signaling burst | MAC messages (BBAD-A opcodes) |
| PDU data / MBT (DUID 0xC) | DCH data burst | Data channel operation |
| NID (NAC+DUID) | DUID in ISCH field | TDMA uses ISCH for burst identification |
| Frame Sync 48-bit | Sync pattern per burst type | TDMA has per-slot sync |
| Status Symbols | Not used in TDMA | TDMA uses ISCH for status |
| IMBE vocoder | AMBE+2 vocoder | Different codec in Phase 2 |
| 12.5 kHz channel | 12.5 kHz, 2 slots | TDMA doubles capacity |

### 14.2 Message Format Mapping

TSBK messages (AABC-E) are used in both FDMA and TDMA:
- **FDMA:** Carried in TSDU (DUID 0x7, per AABB-B §4.2), trellis-coded rate 1/2
- **TDMA:** Carried in FACCH burst, with TDMA-specific FEC (RS + Golay)

Link Control words (AABF-D) are used in both:
- **FDMA:** Carried in LDU1 (interleaved with voice) and TDULC
- **TDMA:** Carried in SACCH (Slow Associated Control Channel)

The MAC message opcodes are shared between FDMA and TDMA (see TIA-102.BBAD-A
Implementation Spec for the complete opcode table). Some opcodes are TDMA-only
(e.g., power control, timing advance).

---

## 15. Extracted Data and Out-of-Scope Items

### 15.1 Extracted Annex Tables

All normative Annex A transmit bit order tables have been extracted from the PDF
and stored as CSV files in `annex_tables/`:

1. **HDU Transmit Bit Order (Annex A.2):** 396 entries, symbols 0..395.
   `annex_tables/annex_a2_hdu_transmit_bit_order.csv`

2. **LDU1 Transmit Bit Order (Annex A.3):** 864 entries, symbols 0..863.
   `annex_tables/annex_a3_ldu1_transmit_bit_order.csv`

3. **LDU2 Transmit Bit Order (Annex A.4):** 864 entries, symbols 864..1727.
   `annex_tables/annex_a4_ldu2_transmit_bit_order.csv`

4. **GF(2^6) EXP/LOG Tables** (programmatically generated from x^6+x+1):
   `annex_tables/gf64_lookup_tables.csv`

5. **BCH NID Generator Matrix** (from PDF Table 19, page 45):
   `annex_tables/bch_nid_generator_matrix.csv`

### 15.2 Items Out of Scope

The following are correctly excluded from this spec:

1. **TDU/TDULC transmit bit order (Annex A.5/A.6):** Absent from the 83-page edition.
   TDU is entirely null padding after NID; TDULC structure follows from Section 5.5.

2. **IMBE voice frame internal bit ordering:** Defined in TIA-102.BABA (IMBE vocoder
   spec), not in BAAA-B. See the BABA-A Implementation Spec.

### 15.3 GF(2^6) Table Generation Code

The pre-computed tables in Section 8.4.1 can be verified at runtime:

```c
/* Generate GF(2^6) exponential and logarithm tables.
 * Pre-computed results: GF64_EXP and GF64_LOG (Section 8.4.1).
 * exp_out and log_out must each be caller-allocated uint8_t[64]. */
static void generate_gf64_tables(uint8_t exp_out[64], uint8_t log_out[64]) {
    uint8_t val = 1;
    for (uint8_t i = 0; i < 63; i++) {
        exp_out[i] = val;
        log_out[val] = i;
        val = (uint8_t)(val << 1);
        if (val & 0x40u) {
            val ^= 0x43u; /* reduce mod x^6 + x + 1: clear bit 6, XOR with x+1 */
            val &= 0x3Fu;
        }
    }
    exp_out[63] = 1; /* alpha^63 = alpha^0 = 1 */
    log_out[0]  = 0xFF; /* log(0) undefined */
}
/* Pre-computed tables: see annex_tables/gf64_lookup_tables.csv and Section 8.4.1. */
```

```c
/* Generate GF(2^6) exponential and logarithm tables.
 * The pre-computed results are in GF64_EXP and GF64_LOG (Section 8.4.1).
 * This function can be used to verify or regenerate those tables at runtime.
 * exp_out and log_out must each be caller-allocated uint8_t[64]. */
static void generate_gf64_tables(uint8_t exp_out[64], uint8_t log_out[64]) {
    uint8_t val = 1;
    for (uint8_t i = 0; i < 63; i++) {
        exp_out[i] = val;
        log_out[val] = i;
        val = (uint8_t)(val << 1);
        if (val & 0x40u) {
            val ^= 0x43u; /* reduce mod x^6 + x + 1: clear bit 6, XOR with x+1 */
            val &= 0x3Fu;
        }
    }
    exp_out[63] = 1; /* alpha^63 = alpha^0 = 1 */
    log_out[0]  = 0xFF; /* log(0) undefined */
}
/* Pre-computed tables: see annex_tables/gf64_lookup_tables.csv and Section 8.4.1. */
```

### 15.4 Open-Source Reference Implementations

- **SDRTrunk (Java):** `P25P1DataUnitDetector.java` (frame sync), `P25P1Message*.java`
  (bit position arrays), `Golay18.java`, `Golay24.java`, `Hamming10.java`,
  `ReedSolomon_63_47_17.java`, `BchCode_63_16_23.java`, `TrellisCodec.java`.

- **OP25 (C++/Python):** `p25p1_fdma.cc` (full FDMA pipeline), `bch.cc`, `golay2087.cc`,
  `rs.cc`, `imbe_decoder.cc` (IMBE frame extraction from LDU).

---

*End of P25 FDMA Common Air Interface Implementation Specification*
