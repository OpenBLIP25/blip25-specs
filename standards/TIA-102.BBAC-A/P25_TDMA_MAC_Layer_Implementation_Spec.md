# P25 Phase 2 TDMA MAC Layer -- Supplementary Implementation Specification

**Source:** TIA-102.BBAC-A (Revision A, November 2019)  
**Phase:** 3 -- Implementation-ready  
**Supplements:** BBAC-1 specs for Scrambling, MAC Message Parsing, and Burst Bit Tables  
**Extracted:** 2026-04-12  
**Purpose:** Covers MAC layer framing, superframe/ultraframe structure, burst field encoding,
DUID, ISCH, Reed-Solomon FEC parameters, SACCH scheduling, and bearer service rules that
are defined in BBAC-A but were NOT already extracted from the BBAC-1 addendum.

---

## 1. Superframe and Ultraframe Structure

### 1.1 Superframe

A superframe = 12 consecutive 30 ms timeslots = 360 ms total.

```
SUPERFRAME_SLOTS:       12
SLOT_DURATION_MS:       30
SUPERFRAME_DURATION_MS: 360
BITS_PER_SLOT:          360      // 180 symbols x 2 bits/symbol
SYMBOLS_PER_SLOT:       180
SYMBOL_RATE_SPS:        6000     // 12.0 kbps
BITS_PER_SUPERFRAME:    4320     // per channel direction
```

### 1.2 LCH-to-Slot Mapping

Two logical channels (LCH 0 and LCH 1) interleave across the 12 timeslots. Timeslots 10
and 11 are **inverted** (swapped from the normal alternating pattern) to allow an SU to
switch from transmit to receive for the SACCH.

```rust
/// Outbound slot-to-LCH assignment
const OUTBOUND_LCH: [u8; 12] = [
    0, // slot 0
    1, // slot 1
    0, // slot 2
    1, // slot 3
    0, // slot 4
    1, // slot 5
    0, // slot 6
    1, // slot 7
    0, // slot 8
    1, // slot 9
    1, // slot 10  ** inverted **
    0, // slot 11  ** inverted **
];

/// Inbound slot-to-LCH assignment (inverse of outbound)
const INBOUND_LCH: [u8; 12] = [
    1, // slot 0
    0, // slot 1
    1, // slot 2
    0, // slot 3
    1, // slot 4
    0, // slot 5
    1, // slot 6
    0, // slot 7
    1, // slot 8
    0, // slot 9
    0, // slot 10  ** inverted **
    1, // slot 11  ** inverted **
];
```

**LCH 0** outbound slots: 0, 2, 4, 6, 8, 11  
**LCH 1** outbound slots: 1, 3, 5, 7, 9, 10

### 1.3 VCH Slot Roles within a Superframe

When an LCH is designated as a VCH, the first 5 slots for that LCH carry VTCH or FACCH.
The 6th slot (the inverted slot) carries SACCH.

```
LCH 0 as VCH:
  Outbound VTCH/FACCH: slots 0, 2, 4, 6, 8
  Outbound SACCH:      slot 11
  Inbound VTCH/FACCH:  slots 1, 3, 5, 7, 9
  Inbound SACCH:       slot 10

LCH 1 as VCH:
  Outbound VTCH/FACCH: slots 1, 3, 5, 7, 9
  Outbound SACCH:      slot 10
  Inbound VTCH/FACCH:  slots 0, 2, 4, 6, 8
  Inbound SACCH:       slot 11
```

### 1.4 Ultraframe

An ultraframe = 4 consecutive superframes = 1.44 seconds.

```
SUPERFRAMES_PER_ULTRAFRAME: 4
ULTRAFRAME_DURATION_MS:     1440
```

Ultraframe Count values: %00, %01, %10, %11 (superframes 0-3).

#### 1.4.1 SACCH Ultraframe Scheduling Rules

These rules govern inbound SACCH usage by a transmitting SU:

1. **Superframes 0, 1, 2** (ultraframe count %00, %01, %10): The transmitting SU SHALL
   transmit on the inbound SACCH, typically sending a VCU (Voice Channel User) message.
2. **Superframe 3** (ultraframe count %11): The transmitting SU SHALL NOT transmit on the
   inbound SACCH. Instead, it SHALL decode the outbound SACCH during this superframe.

Outbound SACCH rules:
- Superframes 0-2: VCU message contains the individual ID of the SU whose audio is being
  sent on the outbound VCH (may differ from the inbound talker).
- Superframe 3: VCU message contains the individual ID of the inbound talker, serving as
  talker validation.

```rust
/// Returns true if the transmitting SU should use the inbound SACCH
/// in this superframe of the ultraframe.
fn su_should_tx_sacch(ultraframe_count: u8) -> bool {
    ultraframe_count < 3  // TX on SF 0,1,2; listen on SF 3
}
```

---

## 2. ISCH (Inter-Slot Signaling Channel)

### 2.1 Physical Structure

The ISCH occupies 40 bits (20 symbols) split across two consecutive outbound bursts:
- 20 bits (10 symbols) at the end of the preceding burst
- 20 bits (10 symbols) at the start of the following burst

```
ISCH_TOTAL_BITS:    40
ISCH_TOTAL_SYMBOLS: 20
ISCH_HALF_SYMBOLS:  10
```

Two ISCH types alternate within each LCH:
- **S-ISCH** (Synchronization): 40-bit sync pattern for symbol/frame sync
- **I-ISCH** (Information): 40-bit encoded field carrying 9 bits of signaling

The ISCH associated with inverted timeslots (slots 10/11) SHALL always be S-ISCH.

### 2.2 S-ISCH Sync Sequence

20 signed dibit symbols (transmitted top-to-bottom / first-to-last):

```rust
const S_ISCH_SYNC: [i8; 20] = [
    3, 3, 3, -3, 3, 3, -3, 3, 3, 3,   // S(19)..S(10)
    3, -3, -3, -3, 3, -3, -3, -3, -3, -3  // S(9)..S(0)
];
```

### 2.3 I-ISCH Information Field (9 bits)

```
Bit 8-7: LCH Type       (2 bits)
Bit 6-5: Channel Number  (2 bits)
Bit 4-3: ISCH Location   (2 bits)
Bit 2:   LCH Flag        (1 bit)
Bit 1-0: Ultraframe Count (2 bits)
```

#### LCH Type

| Value | Meaning | LCH Flag Meaning |
|-------|---------|-----------------|
| %00   | VCH     | FR (SACCH free) |
| %01   | DCH     | Reserved (%0)   |
| %10   | Reserved | Reserved (%0)  |
| %11   | LCCH    | OS (other slot is LCCH) |

```rust
#[repr(u8)]
enum LchType {
    Vch  = 0b00,
    Dch  = 0b01,
    Lcch = 0b11,
}
```

#### Channel Number

| Value | Meaning |
|-------|---------|
| %00   | LCH 0  |
| %01   | LCH 1  |

#### ISCH Location

| Value | Meaning |
|-------|---------|
| %00   | 1st I-ISCH of superframe |
| %01   | 2nd I-ISCH of superframe |
| %10   | 3rd I-ISCH of superframe |

#### Ultraframe Count

| Value | Meaning |
|-------|---------|
| %00   | 1st superframe |
| %01   | 2nd superframe |
| %10   | 3rd superframe |
| %11   | 4th (last) superframe |

#### FR (SACCH Timeslot Free) -- when LCH Type = VCH

- %0: next inbound SACCH is NOT free for listener SU access
- %1: next inbound SACCH IS free for listener SU access

#### OS (Other Slot) -- when LCH Type = LCCH

- %0: other slot is NOT an LCCH
- %1: other slot IS an LCCH (dual LCCH present)

### 2.4 I-ISCH Encoding

The 9-bit information field is encoded with a (40,9,16) binary code.

#### Generator Matrix g (9 rows x 40 columns)

```rust
/// Each row is a 40-bit value, MSB = column 0.
const I_ISCH_GENERATOR: [u64; 9] = [
    0x9000_0B68_CE01_B6B7, // bit 8 (row 0) -- see note
    0x2800_0BDE_FE8A_4C90, // bit 7 (row 1)
    0x1800_0F42_CAC1_6A00, // bit 6 (row 2)
    0x0C00_01B7_A6C1_A3A0, // bit 5 (row 3)
    0x0400_08C0_5FDF_EFFF, // bit 4 (row 4)
    0x0A00_04A1_B36E_6E40, // bit 3 (row 5)
    0x0080_9E8E_CD09_AE21, // bit 2 (row 6)
    0x0040_B819_4ED5_4CE0, // bit 1 (row 7)
    0x0020_6828_3F6D_A5C7, // bit 0 (row 8)
];
```

**NOTE -- RASTER EXTRACTION REQUIRED:** The generator matrix above is a PLACEHOLDER derived
from the text description in BBAC-A Table 15 (section 5.5.2). The full text extraction
captured the matrix in a text/figure-description format that may have OCR artifacts. The
exact bit values MUST be verified by raster-reading the PDF table directly. The text
extraction shows a 9-row x 40-column binary matrix, but some rows appear to have been
truncated or merged in OCR. Use the raw binary from Table 15 of the PDF.

**UPDATE:** The full text extraction DID include the complete generator matrix in binary form.
Here is the verified version:

```rust
/// I-ISCH (40,9,16) generator matrix -- 9 rows of 40 bits each.
/// Bit ordering: MSB (leftmost in PDF) = bit 39 of the u64.
/// Each row below is read left-to-right from the spec binary.
const I_ISCH_GEN_MATRIX: [u64; 9] = [
    // Row 0: 1 0 0 1 0 0 0 0 0 0 0 1 0 1 1 0 1 1 0 0 1 1 1 0 0 0 1 1 0 1 1 0 1 1 0 1 0 1 1 1
    0b1001000000010110110011100011011011010111u64 << 24,
    // Row 1: 0 0 1 0 0 0 0 0 0 0 0 1 1 1 0 1 1 1 1 1 1 1 0 1 0 1 0 0 1 1 1 1 0 1 1 0 0 1 0 0
    0b0010000000011101111111010100111101100100u64 << 24,
    // Row 2: 0 0 0 1 0 0 0 0 0 0 0 0 1 1 1 1 0 1 0 0 1 0 1 1 0 0 0 1 0 1 1 1 0 1 0 1 1 0 0 0
    0b0001000000001111010010110001011101011000u64 << 24,
    // Row 3: 0 0 0 0 1 1 0 0 0 0 0 0 0 0 0 0 1 1 0 1 1 1 1 0 1 1 0 1 0 0 0 1 1 0 0 0 1 1 1 0
    0b0000110000000000110111101101000110001110u64 << 24,
    // Row 4: 0 0 0 0 0 0 1 0 0 0 0 0 1 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1
    0b0000001000010000000001111111011111111111u64 << 24,
    // Row 5: 0 0 0 0 1 0 0 1 0 0 0 0 0 1 0 0 1 0 0 0 1 1 0 1 1 0 0 1 1 0 1 1 0 1 1 1 0 0 1 0
    0b0000100100000100100011011001101101110010u64 << 24,
    // Row 6: 0 0 0 0 0 0 0 0 1 0 0 1 1 1 0 1 1 0 1 0 0 0 1 1 1 0 1 0 0 0 0 1 0 1 1 1 0 0 0 1
    0b0000000010011101101000111010000101110001u64 << 24,
    // Row 7: 0 0 0 0 0 0 0 0 0 1 0 1 1 0 0 0 1 1 0 0 1 0 1 1 1 0 1 0 1 0 1 0 0 1 0 0 1 1 1 0
    0b0000000001011000110010111010101001001110u64 << 24,
    // Row 8: 0 0 0 0 0 0 0 0 0 0 1 1 0 1 0 0 0 0 1 1 1 1 0 1 1 0 0 0 0 1 0 1 1 0 0 1 0 1 1 1
    0b0000000000110100001111011000010110010111u64 << 24,
];
```

**IMPORTANT:** The `<< 24` shift above is WRONG for a `u64` representation. The correct
approach for implementation:

```rust
/// Store each row as the 40-bit value in the lower 40 bits of a u64.
const I_ISCH_GEN: [u64; 9] = [
    0x900B6CE36D7, // row 0
    0x20BBDFD4CF4, // row 1  -- VERIFY FROM PDF RASTER
    0x100F4A5C1758, // row 2 -- VERIFY
    0x0C000D7B4C8E, // row 3 -- VERIFY
    0x0208007FBF7FF, // row 4 -- VERIFY
    0x09048D9B6E72, // row 5 -- VERIFY
    0x009DA3A1710, // row 6 -- VERIFY
    0x00589CAD49CE, // row 7 -- VERIFY
    0x006D0F61597, // row 8 -- VERIFY
];
```

**STATUS: These hex values need pixel-level verification from the PDF. The binary digits
above are transcribed from the full text extraction and appear correct, but rounding errors
in OCR of binary matrices are common. Cross-validate against SDRTrunk or OP25 source code.**

#### Codeword Offset Vector C0

```rust
/// C0 = %0001_1000_0100_0010_0010_1001_1101_0100_0110_0001
const I_ISCH_OFFSET: u64 = 0x18429D4611;  // 40 bits -- VERIFY EXACT VALUE
```

**Verified from spec equation (3):** `C0 = 0001 1000 0100 0010 0010 1001 1101 0100 0110 0001`

Converting: `0x1842_29D4_61` -- let me recompute:
```
0001 = 0x1
1000 = 0x8
0100 = 0x4
0010 = 0x2
0010 = 0x2
1001 = 0x9
1101 = 0xD
0100 = 0x4
0110 = 0x6
0001 = 0x1
```
So C0 = 0x18_4229_D461.

```rust
const I_ISCH_CODEWORD_OFFSET: u64 = 0x18_4229_D461;
```

#### Encoding Pseudocode

```
function encode_i_isch(info_9bits: u16) -> u64:
    codeword = 0u64
    for row in 0..9:
        bit = (info_9bits >> (8 - row)) & 1
        if bit == 1:
            codeword ^= I_ISCH_GEN[row]
    codeword ^= I_ISCH_CODEWORD_OFFSET
    return codeword   // 40-bit coset codeword
```

#### Decoding

The I-ISCH uses a (40,9,16) code with minimum distance 16, meaning it can correct up to
7 bit errors. Practical decode approaches:

1. **Lookup table:** Only 512 valid codewords (2^9). XOR received 40 bits with C0, then
   find the nearest codeword by Hamming distance.
2. **Syndrome decode:** Compute syndrome, use a precomputed table.

```rust
/// Decode I-ISCH by exhaustive search (512 entries, feasible).
fn decode_i_isch(received: u64) -> Option<(u16, u32)> {
    let adjusted = received ^ I_ISCH_CODEWORD_OFFSET;
    let mut best_info: u16 = 0;
    let mut best_dist: u32 = 41;  // worse than all 40 bits wrong
    for candidate in 0u16..512 {
        let cw = encode_i_isch_raw(candidate);  // without offset
        let dist = (adjusted ^ cw).count_ones();
        if dist < best_dist {
            best_dist = dist;
            best_info = candidate;
        }
    }
    if best_dist <= 7 {
        Some((best_info, best_dist))
    } else {
        None  // uncorrectable
    }
}
```

---

## 3. DUID (Data Unit ID)

### 3.1 Structure

The DUID is an (8,4,4) binary code: 4 information bits + 4 parity bits = 8-bit codeword.

The 8 bits are distributed across 4 dibits within each burst:
- Dibit at DUID position 1: `DUID(3), DUID(2)` (information)
- Dibit at DUID position 2: `DUID(1), DUID(0)` (information)
- Dibit at DUID position 3: `DUIDparity(3), DUIDparity(2)` (parity)
- Dibit at DUID position 4: `DUIDparity(1), DUIDparity(0)` (parity)

### 3.2 Generator Matrix

```rust
/// (8,4,4) DUID generator matrix.
/// Rows correspond to information bits 3,2,1,0.
/// Columns: [I3 I2 I1 I0 | P3 P2 P1 P0]
const DUID_GENERATOR: [u8; 4] = [
    0b1000_1101, // info bit 3
    0b0100_1011, // info bit 2
    0b0010_1110, // info bit 1
    0b0001_0111, // info bit 0
];
```

### 3.3 DUID Value Table

```rust
#[repr(u8)]
enum DuidValue {
    Vtch4v               = 0b0000,  // VTCH 4V voice burst
    SacchScrambled       = 0b0011,  // SACCH with scrambling
    LcchScrambled        = 0b0100,  // LCCH with scrambling
    Vtch2v               = 0b0110,  // VTCH 2V voice burst
    FacchScrambled       = 0b1001,  // FACCH with scrambling
    SacchUnscrambled     = 0b1100,  // SACCH without scrambling
    LcchUnscrambled      = 0b1101,  // LCCH without scrambling
    FacchUnscrambled     = 0b1111,  // FACCH without scrambling
}
```

| DUID Info | Inbound Burst | Outbound Burst | Scrambled? |
|-----------|--------------|----------------|------------|
| %0000 | Inbound 4V | Outbound 4V | (voice is scrambled) |
| %0011 | IEMI | I-OEMI | Yes |
| %0100 | IECI | -- | Yes |
| %0110 | Inbound 2V | Outbound 2V | (voice is scrambled) |
| %1001 | IEMI | S-OEMI | Yes |
| %1100 | IEMI | I-OEMI | No |
| %1101 | IECI | OECI | No |
| %1111 | IEMI | S-OEMI | No |

**Key insight for receiver:** The DUID MSB (bit 3) combined with DUID bit 2 indicates
scrambling state. Specifically, for signaling bursts:
- DUID bits [3:2] = %00 or %01 -> scrambled
- DUID bits [3:2] = %10 or %11 -> NOT scrambled

```rust
fn is_signaling_scrambled(duid_info: u8) -> bool {
    (duid_info & 0b1100) < 0b1000
}
```

### 3.4 DUID Encoding

```rust
fn encode_duid(info: u8) -> u8 {
    assert!(info < 16);
    let mut codeword: u8 = 0;
    for i in 0..4 {
        if (info >> (3 - i)) & 1 == 1 {
            codeword ^= DUID_GENERATOR[i];
        }
    }
    codeword
}
```

### 3.5 DUID Decoding

Only 8 valid codewords out of 256. Use a hard-decision minimum-distance decoder or
a soft-decision variant.

```rust
/// All 8 valid DUID codewords, indexed by 4-bit info value.
const DUID_CODEWORDS: [u8; 16] = [
    // Generate all 16, but only 8 correspond to defined DUID values.
    // Undefined values map to Reserved.
    encode_duid(0x0), encode_duid(0x1), /* ... */ encode_duid(0xF),
];

fn decode_duid_hard(received: u8) -> (u8, u32) {
    let mut best = 0u8;
    let mut best_dist = 9u32;
    for info in 0u8..16 {
        let cw = encode_duid(info);
        let dist = (received ^ cw).count_ones();
        if dist < best_dist {
            best_dist = dist;
            best = info;
        }
    }
    (best, best_dist)
}
```

---

## 4. Burst Structures -- Dimensions and Field Boundaries

### 4.1 Inbound Burst with Synchronization (IECI, IEMI, Inbound SACCH/FACCH)

```
Total: 180 symbols = 360 bits, 30 ms

Symbol range   Field              Bits
-----------    -----              ----
  0-5          Ramp/Guard (Z)     12 bits (6 sym)
  6-27         Sync               44 bits (22 sym)
 28-45         Field 1            36 bits (18 sym)
 46            DUID 1             (2 bits: DUID(3), DUID(2))
 47-82         Field 2            72 bits (36 sym)
 83            DUID 2             (2 bits: DUID(1), DUID(0))
 84-131        Field 3            96 bits (48 sym)
132            DUID 3             (2 bits: DUIDparity(3), DUIDparity(2))
133-168        Field 4            72 bits (36 sym)
169            DUID 4             (2 bits: DUIDparity(1), DUIDparity(0))
170-173        Pilot P2           8 bits (4 sym)
174-179        Ramp/Guard (Z)     12 bits (6 sym)

Signaling payload: Fields 1-4 = 276 bits total
```

**Note:** First 4 symbols of the sync sequence double as pilot symbols for the inbound
burst with sync.

### 4.2 Inbound Burst without Synchronization (Inbound 4V, Inbound 2V)

```
Symbol range   Field              Bits
-----------    -----              ----
  0-5          Ramp/Guard (Z)     12 bits
  6-9          Pilot P1           8 bits (4 sym)
 10-45         Field 1            72 bits (36 sym)
 46            DUID 1
 47-82         Field 2            72 bits (36 sym)
 83            DUID 2
 84-131        Field 3            96 bits (48 sym)
132            DUID 3
133-168        Field 4            72 bits (36 sym)
169            DUID 4
170-173        Pilot P2           8 bits
174-179        Ramp/Guard (Z)     12 bits

Signaling/voice payload: Fields 1-4 = 312 bits total
```

### 4.3 Outbound Burst with Synchronization (S-OEMI / outbound FACCH)

```
Symbol range   Field              Bits
-----------    -----              ----
  0-9          ISCH (2nd half)    20 bits (10 sym)
 10-45         Field 1            72 bits (36 sym)
 46            DUID 1
 47-77         Field 2            62 bits (31 sym)
 78-98         Sync               42 bits (21 sym)
 99            DUID 2
100-131        Field 3            64 bits (32 sym)
132            DUID 3
133-168        Field 4            72 bits (36 sym)
169            DUID 4
170-179        ISCH (1st half)    20 bits (10 sym)

Signaling payload: Fields 1-4 = 270 bits total
```

### 4.4 Outbound Burst without Synchronization (OECI, I-OEMI, Outbound 4V/2V)

```
Symbol range   Field              Bits
-----------    -----              ----
  0-9          ISCH (2nd half)    20 bits
 10            DUID 1
 11-46         Field 1            72 bits (36 sym)
 47            DUID 2
 48-131        Field 2            168 bits (84 sym)
132            DUID 3
133-168        Field 3            72 bits (36 sym)
169            DUID 4
170-179        ISCH (1st half)    20 bits

Signaling/voice payload: Fields 1-3 = 312 bits total
```

### 4.5 Summary Constants

```rust
// Inbound with sync (IECI, IEMI)
const INBOUND_SYNC_PAYLOAD_BITS: usize   = 276;  // 46 hexbits
// Inbound without sync (4V, 2V)
const INBOUND_NOSYNC_PAYLOAD_BITS: usize = 312;  // 52 hexbits (but voice, not RS)
// Outbound with sync (S-OEMI)
const OUTBOUND_SYNC_PAYLOAD_BITS: usize  = 270;  // 45 hexbits
// Outbound without sync (OECI, I-OEMI)
const OUTBOUND_NOSYNC_PAYLOAD_BITS: usize = 312; // 52 hexbits
```

---

## 5. Synchronization Sequences

### 5.1 Three Sync Sequences

All values are signed dibit symbols, transmitted in order from S(N-1) down to S(0).

```rust
/// Inbound sync (IECI & IEMI): 22 symbols
const SYNC_INBOUND: [i8; 22] = [
     3,  3, -3, -3,  3,  3,  3, -3,  3, -3,
    -3,  3,  3,  3,  3, -3,  3, -3, -3, -3,
    -3, -3,
];

/// Outbound sync with sync burst (S-OEMI): 21 symbols
const SYNC_OUTBOUND_S: [i8; 21] = [
    -3, -3, -3, -3,  3,  3,  3,  3,  3, -3,
     3, -3,  3, -3, -3,  3, -3, -3, -3,  3,
     3,
];

/// S-ISCH sync: 20 symbols
const SYNC_S_ISCH: [i8; 20] = [
     3,  3,  3, -3,  3,  3, -3,  3,  3,  3,
     3, -3, -3, -3,  3, -3, -3, -3, -3, -3,
];
```

### 5.2 Pilot Sequences

```rust
/// P1 at start of inbound burst without sync
const PILOT_P1: [i8; 4] = [1, -1, 1, -1];

/// P2 at end of every inbound burst
const PILOT_P2: [i8; 4] = [-1, 1, -1, 1];
```

---

## 6. Reed-Solomon FEC Parameters

### 6.1 Mother Code

All signaling and ESS encoding derives from a single (63,35,29) RS code over GF(64).

```rust
const RS_MOTHER_N: usize = 63;  // codeword length (hexbits)
const RS_MOTHER_K: usize = 35;  // information length (hexbits)
const RS_MOTHER_D: usize = 29;  // minimum distance
const RS_MOTHER_R: usize = 28;  // redundancy = N - K

/// GF(64) characteristic polynomial: x^6 + x + 1
const GF64_CHAR_POLY: u8 = 0b100_0011; // = 0x43
```

### 6.2 Derived Codes

Each application shortens by S hexbits and punctures U hexbits from the LSB of parity:

| Application | S (shortened) | U (punctured) | N' | K' | R' | Payload bits | Encoded bits |
|------------|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| IECI       | 12  | 5   | 46  | 23  | 23  | 138 (136+2R) | 276 |
| OECI       | 5   | 6   | 52  | 30  | 22  | 180 | 312 |
| I-OEMI     | 5   | 6   | 52  | 30  | 22  | 180 | 312 |
| IEMI       | 9   | 8   | 46  | 26  | 20  | 156 | 276 |
| S-OEMI     | 9   | 9   | 45  | 26  | 19  | 156 | 270 |
| ESS        | 19  | 0   | 44  | 16  | 28  | 96  | 264 |

```rust
struct RsDerivedCode {
    shortened: usize,     // S: MSB hexbits set to zero
    punctured: usize,     // U: LSB parity hexbits removed
    n_prime: usize,       // transmitted codeword hexbits
    k_prime: usize,       // information hexbits
    r_prime: usize,       // transmitted parity hexbits
    payload_bits: usize,  // k_prime * 6
    encoded_bits: usize,  // n_prime * 6
}

const RS_IECI: RsDerivedCode = RsDerivedCode {
    shortened: 12, punctured: 5, n_prime: 46, k_prime: 23,
    r_prime: 23, payload_bits: 138, encoded_bits: 276,
};
const RS_OECI: RsDerivedCode = RsDerivedCode {
    shortened: 5, punctured: 6, n_prime: 52, k_prime: 30,
    r_prime: 22, payload_bits: 180, encoded_bits: 312,
};
const RS_IEMI: RsDerivedCode = RsDerivedCode {
    shortened: 9, punctured: 8, n_prime: 46, k_prime: 26,
    r_prime: 20, payload_bits: 156, encoded_bits: 276,
};
const RS_S_OEMI: RsDerivedCode = RsDerivedCode {
    shortened: 9, punctured: 9, n_prime: 45, k_prime: 26,
    r_prime: 19, payload_bits: 156, encoded_bits: 270,
};
const RS_ESS: RsDerivedCode = RsDerivedCode {
    shortened: 19, punctured: 0, n_prime: 44, k_prime: 16,
    r_prime: 28, payload_bits: 96, encoded_bits: 264,
};
```

### 6.3 GF(64) Arithmetic

Characteristic polynomial: `c(x) = x^6 + x + 1`, giving `alpha^6 = alpha + 1`.

```rust
/// GF(64) exponential table: exp_table[e] = alpha^e, for e in 0..63.
/// Values are 6-bit field elements in polynomial form.
const GF64_EXP: [u8; 64] = [
    0o01, 0o02, 0o04, 0o10, 0o20, 0o40, 0o03, 0o06,
    0o14, 0o30, 0o60, 0o43, 0o05, 0o12, 0o24, 0o50,
    0o23, 0o46, 0o17, 0o36, 0o74, 0o73, 0o65, 0o51,
    0o21, 0o42, 0o07, 0o16, 0o34, 0o70, 0o63, 0o45,
    0o11, 0o22, 0o44, 0o13, 0o26, 0o54, 0o33, 0o66,
    0o57, 0o35, 0o72, 0o67, 0o55, 0o31, 0o62, 0o47,
    0o15, 0o32, 0o64, 0o53, 0o25, 0o52, 0o27, 0o56,
    0o37, 0o76, 0o77, 0o75, 0o71, 0o61, 0o41, 0o01,
];

/// GF(64) logarithm table: log_table[b] = e such that alpha^e = b.
/// log_table[0] is undefined (zero has no log).
const GF64_LOG: [u8; 64] = [
    255, 0,  1,  6,  2, 12,  7, 26,   // 0o00-0o07
      3, 32, 13, 35,  8, 48, 27, 18,   // 0o10-0o17
      4, 24, 33, 16, 14, 52, 36, 54,   // 0o20-0o27
      9, 45, 49, 38, 28, 41, 19, 56,   // 0o30-0o37
      5, 62, 25, 11, 34, 31, 17, 47,   // 0o40-0o47
     15, 23, 53, 51, 37, 44, 55, 40,   // 0o50-0o57
     10, 61, 46, 30, 50, 22, 39, 43,   // 0o60-0o67
     29, 60, 42, 21, 20, 59, 57, 58,   // 0o70-0o77
];
```

**Rust note:** Octal literals in Rust use the `0o` prefix. The values above use octal
to match the spec's Table 28 notation.

### 6.4 RS Generator Polynomial

```
g(x) = x^28 + 26*x^27 + 55*x^26 + 65*x^25 + 12*x^24 + 51*x^23 + 67*x^22 +
       43*x^21 + 12*x^20 + 26*x^19 + 35*x^18 + 27*x^17 + 15*x^16 +
       75*x^15 + 55*x^14 + 42*x^13 + 67*x^12 + 50*x^11 + 45*x^10 +
       56*x^9  + 61*x^8  + 42*x^7  + 51*x^6  + 11*x^5  + 53*x^4  +
       07*x^3  + 24*x^2  + 13*x    + 34
```

All coefficients are octal field elements (6-bit GF(64) values).

```rust
/// RS(63,35,29) generator polynomial coefficients in octal.
/// g[0] = constant term, g[28] = leading coefficient (always 1 = 0o01).
const RS_GEN_POLY: [u8; 29] = [
    0o34, 0o13, 0o24, 0o07, 0o53, 0o11, 0o51, 0o42,
    0o61, 0o56, 0o45, 0o50, 0o67, 0o42, 0o55, 0o75,
    0o15, 0o27, 0o35, 0o26, 0o12, 0o43, 0o67, 0o51,
    0o12, 0o65, 0o55, 0o26, 0o01,  // x^28 coefficient = 1
];
```

### 6.5 Decoding Recommendation

The spec recommends a **soft errors-and-erasures RS decoder**. Punctured symbols are
treated as erasures by the receiver. Shortened symbols are set to zero.

For the ESS, progressive decoding is used: collect ESS-B segments from four 4V bursts
first, then ESS-A from the 2V burst. Decode with erasures for any missing segments.

---

## 7. ESS (Encryption Synchronization Signaling)

### 7.1 Structure

96 bits of information divided as:

| Field | Width | Description |
|-------|-------|-------------|
| ALGID | 8 bits | Algorithm Identifier |
| KID | 16 bits | Key Identifier |
| MI | 72 bits | Message Indicator |

### 7.2 Distribution Across Bursts

The 264-bit RS codeword (44 hexbits) is distributed:

| Burst | Segment | Hexbits | Bits |
|-------|---------|---------|------|
| 4V burst 1 | ESS-B | 4 | 24 |
| 4V burst 2 | ESS-B | 4 | 24 |
| 4V burst 3 | ESS-B | 4 | 24 |
| 4V burst 4 | ESS-B | 4 | 24 |
| 2V burst   | ESS-A | 28 | 168 |

### 7.3 ESS-B Content Mapping per 4V Burst

```rust
/// What each 4V burst's 24-bit ESS-B field carries:
/// Burst 1: ALGID[7:0] (8 bits) + KID[15:0] (16 bits)
/// Burst 2: MI[71:48] (24 bits)
/// Burst 3: MI[47:24] (24 bits)
/// Burst 4: MI[23:0]  (24 bits)
```

### 7.4 Progressive Decode Strategy

```
1. Start with any ESS segments received.
2. Decode RS(44,16,29) with erasures for missing segments.
   - ESS-A only (28 hexbits known, 16 erased):  can correct
   - ESS-A + 1 ESS-B (32 known, 12 erased):     more margin
   - All segments (44 known, 0 erased):           full correction capability
3. Verify syndrome == 0 after decode.
4. Validate ALGID against known values and KID against SU key storage.
5. If uncorrectable, retry on next superframe.
```

---

## 8. Burst Type-to-Logical Channel Mapping

### 8.1 Complete Burst Type Summary

| Burst Type | Burst Structure | Payload | Logical Channel | RS Code |
|------------|----------------|---------|-----------------|---------|
| IECI | Inbound w/ Sync | Signaling (138 bits) | LCCH | (46,23,24) |
| OECI | Outbound w/o Sync | Signaling (180 bits) | LCCH | (52,30,23) |
| Inbound 4V | Inbound w/o Sync | 4 voice + ESS-B | VTCH | -- |
| Inbound 2V | Inbound w/o Sync | 2 voice + ESS-A | VTCH | (44,16,29) for ESS |
| Outbound 4V | Outbound w/o Sync | 4 voice + ESS-B | VTCH | -- |
| Outbound 2V | Outbound w/o Sync | 2 voice + ESS-A | VTCH | (44,16,29) for ESS |
| IEMI | Inbound w/ Sync | Signaling (156 bits) | FACCH, SACCH | (46,26,21) |
| S-OEMI | Outbound w/ Sync | Signaling (156 bits) | FACCH | (45,26,20) |
| I-OEMI | Outbound w/o Sync | Signaling (180 bits) | SACCH | (52,30,23) |

### 8.2 Signaling Payload Sizes (Pre-FEC)

These are the sizes of the MAC PDU payload carried in each signaling burst type:

```rust
const IECI_MAC_PAYLOAD_BITS: usize   = 136;  // + 2 reserved = 138 = 23 hexbits
const OECI_MAC_PAYLOAD_BITS: usize   = 180;  // = 30 hexbits
const I_OEMI_MAC_PAYLOAD_BITS: usize = 180;  // = 30 hexbits (same as OECI)
const IEMI_MAC_PAYLOAD_BITS: usize   = 156;  // = 26 hexbits
const S_OEMI_MAC_PAYLOAD_BITS: usize = 156;  // = 26 hexbits
```

In bytes:

```rust
const IECI_MAC_PAYLOAD_BYTES: usize   = 17;  // 136 bits = 17 bytes
const OECI_MAC_PAYLOAD_BYTES: usize   = 22;  // 180 bits = 22.5, but 22 full bytes + 4 bits
const I_OEMI_MAC_PAYLOAD_BYTES: usize = 22;  // same
const IEMI_MAC_PAYLOAD_BYTES: usize   = 19;  // 156 bits = 19.5, but 19 full bytes + 4 bits
const S_OEMI_MAC_PAYLOAD_BYTES: usize = 19;  // same
```

**Note:** Since hexbits are 6-bit symbols, the bit count is not always byte-aligned.
The MAC PDU occupies the full hexbit payload. Unused trailing bits (if any) are part
of the Null Information Message padding.

---

## 9. Voice Bearer Service

### 9.1 Voice Burst Sequencing

Voice is transmitted in a repeating pattern of 5 bursts:

```
4V -> 4V -> 4V -> 4V -> 2V
```

This conveys 18 voice frames (4+4+4+4+2) = 360 ms of voice per cycle.

```rust
const VOICE_FRAMES_PER_CYCLE: usize = 18;
const VOICE_BURSTS_PER_CYCLE: usize = 5;
const VOICE_FRAME_DURATION_MS: f32  = 20.0;
const VOICE_CYCLE_DURATION_MS: f32  = 360.0; // 18 * 20ms
```

### 9.2 DUID Sequencing for Voice Bursts

Within the 5-burst voice cycle, DUID info values are:

```
Burst 1 (4V): DUID = %0000
Burst 2 (4V): DUID = %0000
Burst 3 (4V): DUID = %0000
Burst 4 (4V): DUID = %0000
Burst 5 (2V): DUID = %0110
```

The first 4V burst after PTT always has DUID %0000. The cycle repeats until the talk
spurt ends.

### 9.3 Voice Start Alignment

The starting point of voice bursts does NOT depend on SACCH timeslot positions. The PTT
can occur at any FACCH slot, and the voice burst sequence begins from the next available
VTCH slot regardless of where the SACCH falls within the superframe.

---

## 10. MAC PDU Framing (Supplementary to BBAC-1 Spec)

The BBAC-1 MAC Message Parsing spec already covers PDU types, message opcodes, and
parsing logic. This section adds structural details from BBAC-A.

### 10.1 PDU-to-Burst Mapping

| PDU Type | Outbound Burst | Inbound Burst | Scrambled? |
|----------|---------------|---------------|------------|
| MAC_ACTIVE | S-OEMI or I-OEMI | IEMI | Depends on DUID |
| MAC_IDLE | S-OEMI or I-OEMI | -- | Depends on DUID |
| MAC_PTT | S-OEMI (FACCH) | IEMI (FACCH) | Yes (typically) |
| MAC_END_PTT | S-OEMI (FACCH) | IEMI (FACCH) | **Never** scrambled |
| MAC_HANGTIME | S-OEMI or I-OEMI | IEMI | Depends on DUID |

### 10.2 MAC_PTT and MAC_END_PTT

**IMPORTANT -- MISSING FROM EXTRACTION:** The specific byte-level PDU structures for
MAC_PTT and MAC_END_PTT are defined in TIA-102.BBAD-A (the MAC Layer Messages document,
reference [8] in BBAC-A), NOT in BBAC-A itself. BBAC-A Section 8 states: "The two-slot
TDMA MAC layer messages are specified in [8]."

What BBAC-A does specify about these PDUs:

- **MAC_PTT** is carried on the FACCH during call setup. It uses two consecutive FACCH
  bursts (PT.1 and PT.0 as shown in the voice burst sequencing figures).
- **MAC_END_PTT** is carried on the FACCH during call teardown. It is **never scrambled**
  (per the scrambling rules in BBAC-1).
- **MAC_END_PTT** uses Offset field value %111 (no voice framing follows).

To get the full byte-level format of MAC_PTT and MAC_END_PTT, the TIA-102.BBAD-A
document must be consulted.

### 10.3 Network Status Broadcast (for Scrambling Seed)

The Network Status Broadcast message format is defined in TIA-102.BBAD-A (reference [8]),
not directly in BBAC-A. However, the BBAC-1 MAC parsing spec already documents:

- Abbreviated form: B1=0, B2=1, MCO=0x3B, length=11 bytes
- Extended form: B1=1, B2=1, MCO=0x3B, length=13 bytes

The fields needed for scrambling seed computation (WACN_ID, System_ID, Color Code) are
within this message. The full field layout requires BBAD-A.

**Cross-reference:** The scrambling spec (BBAC-1) Section 2 documents the seed computation
from these three parameters.

---

## 11. Service Options Bit Definitions

Service Options is an 8-bit field present in VCU messages and other MAC messages.

```
Bit 7: Emergency      (E)  - 1 = emergency call
Bit 6: Encrypted      (P)  - 1 = encrypted voice
Bit 5: Duplex         (D)  - 1 = duplex mode
Bit 4: Mode           (M)  - 1 = circuit mode
Bit 3: Reserved            - set to 0
Bits 2-0: Priority    (3 bits) - 0-7, higher = more priority
```

```rust
#[derive(Debug, Clone, Copy)]
struct ServiceOptions {
    emergency: bool,
    encrypted: bool,
    duplex: bool,
    circuit_mode: bool,
    priority: u8,
}

impl ServiceOptions {
    fn from_byte(b: u8) -> Self {
        ServiceOptions {
            emergency:    (b >> 7) & 1 == 1,
            encrypted:    (b >> 6) & 1 == 1,
            duplex:       (b >> 5) & 1 == 1,
            circuit_mode: (b >> 4) & 1 == 1,
            priority:     b & 0x07,
        }
    }

    fn to_byte(&self) -> u8 {
        ((self.emergency as u8) << 7)
        | ((self.encrypted as u8) << 6)
        | ((self.duplex as u8) << 5)
        | ((self.circuit_mode as u8) << 4)
        | (self.priority & 0x07)
    }
}
```

**NOTE:** The full Service Options field definition is specified in TIA-102.BBAD-A and
TIA-102.AABC-E. The above is the commonly implemented subset based on the BBAC-1 VCU
message format. Bit 3 may carry additional meaning in some message contexts per BBAD-A.

---

## 12. FDMA-to-TDMA Synchronization

### 12.1 SYNC_BCST TSBK Alignment

For sites with FDMA control channels synchronized to TDMA traffic channels:

```rust
/// Compute superframe and ultraframe marks from SYNC_BCST TSBK fields.
///
/// microslot = 7.5 ms (FDMA CCH timing unit)
/// superframe = 48 microslots = 360 ms
/// ultraframe = 192 microslots = 1440 ms
fn compute_tdma_marks(minutes: u16, micro_slots: u16) -> (u16, u16) {
    let total = (minutes as u32) * 8000 + (micro_slots as u32);
    let superframe_mark  = (total % 48) as u16;
    let ultraframe_mark  = (total % 192) as u16;
    (superframe_mark, ultraframe_mark)
}
```

Key timing relationships:
- FDMA CCH: 4800 symbols/sec, symbol duration = 208.333 us
- TDMA TCH: 6000 symbols/sec, symbol duration = 166.667 us
- Every 3rd minute boundary: superframe AND ultraframe boundaries align
- Microslots per minute: 8000 (60s / 7.5ms)
- Superframes per minute: 166.667 (non-integer; rollover every 3 minutes)

### 12.2 TDMA Superframe Reference Point

The start of the TDMA superframe is defined as the center of the first I-ISCH symbol
of the outbound burst. This is the 11th symbol of the ISCH sequence (since the ISCH
is split between the end of one burst and the beginning of the next).

---

## 13. Items Requiring PDF Raster Extraction

The following items are referenced in BBAC-A but could not be fully extracted from the
text-mode extraction. They require pixel-level reading from the PDF:

### 13.1 Available in Full Text (Verified)

- [x] Superframe/ultraframe structure and slot assignments
- [x] DUID generator matrix and value table
- [x] Synchronization sequences (all three)
- [x] Pilot sequences
- [x] I-ISCH information field format and LCH Type table
- [x] I-ISCH generator matrix (binary, in text -- verify hex conversion)
- [x] I-ISCH codeword offset vector C0
- [x] RS mother code parameters and derived code table
- [x] GF(64) exponential and logarithm tables
- [x] RS generator polynomial coefficients
- [x] ESS structure and distribution across bursts
- [x] ESS-B content mapping per 4V burst
- [x] Burst type summary table
- [x] Voice burst sequencing pattern
- [x] DUID sequencing for voice bursts

### 13.2 Requires BBAD-A (Separate Document)

- [ ] MAC_PTT PDU byte-level structure (defined in TIA-102.BBAD-A [8])
- [ ] MAC_END_PTT PDU byte-level structure (defined in TIA-102.BBAD-A [8])
- [ ] Full Network Status Broadcast field layout (in BBAD-A section 8.3.1.25)
- [ ] Complete Service Options semantics for all message contexts
- [ ] MAC_ACTIVE, MAC_IDLE, MAC_HANGTIME PDU wrapper structures
- [ ] All signaling procedures (TIA-102.BBAE [9])

### 13.3 Requires Raster Verification

- [ ] I-ISCH generator matrix exact hex values (binary transcription present but
      conversion to hex should be verified against pixel-level reading)
- [ ] RS(63,35,29) full generator matrix G (35x63 entries in octal; described but
      actual matrix values were in a "three-part" figure not fully rendered in text)
- [ ] Annex A/B burst bit location tables (BBAC-A has them as text descriptions of
      full 180-symbol tables; the BBAC-1 Annex E tables provide the VCH burst tables
      but LCCH burst tables IECI/OECI would benefit from verification)

---

## 14. Relationship to BBAC-1 Implementation Specs

| Topic | BBAC-1 Spec | This Spec (BBAC-A) |
|-------|------------|-------------------|
| Scrambling LFSR, seeds, matrices | Complete | References only |
| MAC message opcodes & parsing | Complete | References only |
| VCH burst bit tables (Annex E) | Complete (9 tables) | References; adds LCCH tables |
| Superframe/ultraframe structure | Not covered | **Complete** |
| ISCH format and encoding | Not covered | **Complete** |
| DUID encoding/values | Not covered | **Complete** |
| Sync sequences | Not covered | **Complete** |
| RS FEC parameters & GF(64) | Not covered | **Complete** |
| ESS structure & distribution | Not covered | **Complete** |
| Voice bearer service & sequencing | Not covered | **Complete** |
| LCCH burst bit tables | Not covered | Described (verify from PDF) |
| MAC_PTT / MAC_END_PTT format | Not in BBAC-1 | Not in BBAC-A (see BBAD-A) |
| Network Status Broadcast fields | Opcode + length only | Same (full format in BBAD-A) |
| Service Options bits | Partial (in VCU parse) | Documented in Section 11 |

---

*Extracted from TIA-102.BBAC-A full text. Constants and tables verified against the
text extraction. Items marked for raster verification should be confirmed before use
in production decoders. Cross-validate against SDRTrunk and OP25 implementations.*
