# P25 Phase 2 TDMA MAC Layer -- Supplementary Implementation Specification

**Source:** TIA-102.BBAC-A (Revision A, November 2019)  
**Phase:** 4 -- Verified and uplifted  
**Supplements:** BBAC-1 specs for Scrambling, MAC Message Parsing, and Burst Bit Tables  
**Extracted:** 2026-04-12; Phase 4 uplift: 2026-04-13  
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

```c
/* Outbound slot-to-LCH assignment */
static const uint8_t OUTBOUND_LCH[12] = {
    0, /* slot 0 */
    1, /* slot 1 */
    0, /* slot 2 */
    1, /* slot 3 */
    0, /* slot 4 */
    1, /* slot 5 */
    0, /* slot 6 */
    1, /* slot 7 */
    0, /* slot 8 */
    1, /* slot 9 */
    1, /* slot 10  ** inverted ** */
    0, /* slot 11  ** inverted ** */
};

/* Inbound slot-to-LCH assignment (inverse of outbound) */
static const uint8_t INBOUND_LCH[12] = {
    1, /* slot 0 */
    0, /* slot 1 */
    1, /* slot 2 */
    0, /* slot 3 */
    1, /* slot 4 */
    0, /* slot 5 */
    1, /* slot 6 */
    0, /* slot 7 */
    1, /* slot 8 */
    0, /* slot 9 */
    0, /* slot 10  ** inverted ** */
    1, /* slot 11  ** inverted ** */
};
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

```c
/* Returns true if the transmitting SU should use the inbound SACCH
 * in this superframe of the ultraframe. */
static inline bool su_should_tx_sacch(uint8_t ultraframe_count) {
    return ultraframe_count < 3;  /* TX on SF 0,1,2; listen on SF 3 */
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

```c
static const int8_t S_ISCH_SYNC[20] = {
    3, 3, 3, -3, 3, 3, -3, 3, 3, 3,   /* S(19)..S(10) */
    3, -3, -3, -3, 3, -3, -3, -3, -3, -3  /* S(9)..S(0) */
};
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

```c
typedef enum {
    LCH_TYPE_VCH  = 0x00,
    LCH_TYPE_DCH  = 0x01,
    LCH_TYPE_LCCH = 0x03,
} lch_type_t;
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

From PDF Table 15 (page 28). Each row is a 40-bit value; MSB (leftmost column) = bit 39.

**Phase 4 correction:** Two rows in the earlier draft had transcription errors.
Row 0 had bits 36 and 35 swapped (`1001 0...` vs PDF `1000 1...`).
Row 4 had bit 26 set instead of bit 27 (`0100...` vs PDF `1000...` in the nibble at positions 27-24).
Both corrections verified by re-extracting directly from the pdftotext output and
confirmed by full minimum-distance check: the corrected matrix achieves d=16 as specified.

```c
/* I-ISCH (40,9,16) generator matrix -- 9 rows of 40 bits each.
 * Each value fits in the low 40 bits of a uint64_t.
 * Bit 39 (MSB) = first transmitted column.
 * Source: TIA-102.BBAC-A PDF Table 15, page 28.
 * Verified: GF(2) rank=9; col 1 all-zero (per PDF NOTE); min distance=16. */
static const uint64_t I_ISCH_GEN[9] = {
    0x8816CE36D7ULL,  /* row 0: 1000 1000 0000 0101 1011 0011 1000 1101 1011 0101 11 */
    0x201DFD4F64ULL,  /* row 1 */
    0x100F4B1758ULL,  /* row 2 */
    0x0C00DED18EULL,  /* row 3 */
    0x020807F7FFULL,  /* row 4: 0000 0010 0000 1000 0000 0111 1111 0111 1111 1111 11 */
    0x09048D9B72ULL,  /* row 5 */
    0x009DA3A171ULL,  /* row 6 */
    0x0058CBAA4EULL,  /* row 7 */
    0x00343D8597ULL,  /* row 8 */
};
```

Bulk table (binary and hex per row) is in
`annex_tables/annex_5_iisch_gen_matrix.csv`.

#### Codeword Offset Vector C0

From PDF Equation (3):

`C0 = %0001 1000 0100 0010 0010 1001 1101 0100 0110 0001`

```c
/* I-ISCH coset offset vector C0 (40 bits).
 * Source: TIA-102.BBAC-A PDF Section 5.5.2 Equation 3, page 28. */
static const uint64_t I_ISCH_C0 = 0x184229D461ULL;
```

#### Encoding

```c
/* Encode a 9-bit I-ISCH information word into a 40-bit coset codeword. */
static uint64_t encode_i_isch(uint16_t info_9bits) {
    uint64_t cw = 0;
    for (int row = 0; row < 9; row++) {
        if ((info_9bits >> (8 - row)) & 1) {
            cw ^= I_ISCH_GEN[row];
        }
    }
    return cw ^ I_ISCH_C0;  /* 40-bit coset codeword */
}
```

#### Decoding

The I-ISCH uses a (40,9,16) code with minimum distance 16, meaning it can correct up to
7 bit errors. Practical decode approaches:

1. **Lookup table:** Only 512 valid codewords (2^9). XOR received 40 bits with C0, then
   find the nearest codeword by Hamming distance.
2. **Syndrome decode:** Compute syndrome, use a precomputed table.

```c
/* Decode I-ISCH by exhaustive search (512 candidates -- feasible at runtime).
 * Returns decoded 9-bit info in *info_out and error count in *errors_out.
 * Returns false if uncorrectable (best distance > 7). */
static bool decode_i_isch(uint64_t received, uint16_t *info_out, uint32_t *errors_out) {
    uint64_t adjusted = received ^ I_ISCH_C0;
    uint16_t best_info = 0;
    uint32_t best_dist = 41;
    for (uint16_t cand = 0; cand < 512; cand++) {
        uint64_t cw = 0;
        for (int row = 0; row < 9; row++) {
            if ((cand >> (8 - row)) & 1) cw ^= I_ISCH_GEN[row];
        }
        uint32_t dist = (uint32_t)__builtin_popcountll(adjusted ^ cw);
        if (dist < best_dist) { best_dist = dist; best_info = cand; }
    }
    if (info_out)   *info_out   = best_info;
    if (errors_out) *errors_out = best_dist;
    return best_dist <= 7;
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

```c
/* (8,4,4) DUID generator matrix.
 * Rows correspond to information bits 3,2,1,0.
 * Columns: [I3 I2 I1 I0 | P3 P2 P1 P0]
 * Source: TIA-102.BBAC-A PDF Table 12, page 24.
 * Verified: systematic 4x4 identity; parity sub-matrix rank=4; min distance=4. */
static const uint8_t DUID_GEN[4] = {
    0x8D,  /* 1000 1101 -- info bit 3 */
    0x4B,  /* 0100 1011 -- info bit 2 */
    0x2E,  /* 0010 1110 -- info bit 1 */
    0x17,  /* 0001 0111 -- info bit 0 */
};
```

### 3.3 DUID Value Table

```c
typedef enum {
    DUID_VTCH_4V               = 0x0,  /* VTCH 4V voice burst */
    DUID_SACCH_SCRAMBLED       = 0x3,  /* SACCH with scrambling */
    DUID_LCCH_SCRAMBLED        = 0x4,  /* LCCH with scrambling */
    DUID_VTCH_2V               = 0x6,  /* VTCH 2V voice burst */
    DUID_FACCH_SCRAMBLED       = 0x9,  /* FACCH with scrambling */
    DUID_SACCH_UNSCRAMBLED     = 0xC,  /* SACCH without scrambling */
    DUID_LCCH_UNSCRAMBLED      = 0xD,  /* LCCH without scrambling */
    DUID_FACCH_UNSCRAMBLED     = 0xF,  /* FACCH without scrambling */
} duid_value_t;
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

```c
static inline bool is_signaling_scrambled(uint8_t duid_info) {
    return (duid_info & 0x0C) < 0x08;
}
```

### 3.4 DUID Encoding

```c
static uint8_t encode_duid(uint8_t info) {
    uint8_t cw = 0;
    for (int i = 0; i < 4; i++) {
        if ((info >> (3 - i)) & 1) cw ^= DUID_GEN[i];
    }
    return cw;
}
```

### 3.5 DUID Decoding

Only 8 valid codewords out of 256. Use a hard-decision minimum-distance decoder or
a soft-decision variant.

```c
/* Precomputed DUID codewords for all 16 info values (8 defined + 8 reserved).
 * Generated from DUID_GEN[] via encode_duid(info) for info=0..15.
 * Verified: min Hamming distance among the 8 defined codewords = 4 (d=4 code). */
static const uint8_t DUID_CODEWORDS[16] = {
    0x00, 0x17, 0x2E, 0x39, 0x4B, 0x5C, 0x65, 0x72,
    0x8D, 0x9A, 0xA3, 0xB4, 0xC6, 0xD1, 0xE8, 0xFF,
};

/* Hard-decision minimum-distance DUID decoder.
 * Returns decoded 4-bit info in *info_out; error count in *errors_out. */
static void decode_duid(uint8_t received, uint8_t *info_out, uint32_t *errors_out) {
    uint8_t best = 0;
    uint32_t best_dist = 9;
    for (int info = 0; info < 16; info++) {
        uint32_t dist = (uint32_t)__builtin_popcount(received ^ DUID_CODEWORDS[info]);
        if (dist < best_dist) { best_dist = dist; best = (uint8_t)info; }
    }
    if (info_out)   *info_out   = best;
    if (errors_out) *errors_out = best_dist;
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

```c
#define INBOUND_SYNC_PAYLOAD_BITS     276  /* IECI, IEMI -- 46 hexbits */
#define INBOUND_NOSYNC_PAYLOAD_BITS   312  /* 4V, 2V -- 52 hexbits (voice, not RS-encoded) */
#define OUTBOUND_SYNC_PAYLOAD_BITS    270  /* S-OEMI -- 45 hexbits */
#define OUTBOUND_NOSYNC_PAYLOAD_BITS  312  /* OECI, I-OEMI -- 52 hexbits */
```

---

## 5. Synchronization Sequences

### 5.1 Three Sync Sequences

All values are signed dibit symbols, transmitted in order from S(N-1) down to S(0).

```c
/* Inbound sync (IECI & IEMI): 22 symbols, S(21)..S(0), transmitted top-to-bottom.
 * Source: TIA-102.BBAC-A PDF Table 11, page 23. Verified all symbols in {+3,-3}. */
static const int8_t SYNC_INBOUND[22] = {
     3,  3, -3, -3,  3,  3,  3, -3,  3, -3,
    -3,  3,  3,  3,  3, -3,  3, -3, -3, -3,
    -3, -3,
};

/* Outbound sync with sync burst (S-OEMI): 21 symbols, S(20)..S(0).
 * Source: TIA-102.BBAC-A PDF Table 11, page 23. */
static const int8_t SYNC_OUTBOUND_S[21] = {
    -3, -3, -3, -3,  3,  3,  3,  3,  3, -3,
     3, -3,  3, -3, -3,  3, -3, -3, -3,  3,
     3,
];

/* S-ISCH sync: 20 symbols, S(19)..S(0).
 * Source: TIA-102.BBAC-A PDF Table 11, page 23. */
static const int8_t SYNC_S_ISCH[20] = {
     3,  3,  3, -3,  3,  3, -3,  3,  3,  3,
     3, -3, -3, -3,  3, -3, -3, -3, -3, -3,
};
```

### 5.2 Pilot Sequences

```c
/* P1: at start of inbound burst without sync.
 * P2: at end of every inbound burst.
 * Source: TIA-102.BBAC-A PDF Section 5.2, page 23. */
static const int8_t PILOT_P1[4] = {  1, -1,  1, -1 };
static const int8_t PILOT_P2[4] = { -1,  1, -1,  1 };
```

---

## 6. Reed-Solomon FEC Parameters

### 6.1 Mother Code

All signaling and ESS encoding derives from a single (63,35,29) RS code over GF(64).

```c
#define RS_MOTHER_N  63  /* codeword length (hexbits) */
#define RS_MOTHER_K  35  /* information length (hexbits) */
#define RS_MOTHER_D  29  /* minimum distance */
#define RS_MOTHER_R  28  /* redundancy = N - K */

/* GF(64) characteristic polynomial: c(x) = x^6 + x + 1 = 0x43 */
#define GF64_CHAR_POLY  0x43
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

Full table in `annex_tables/annex_4_rs_derived_codes.csv`.

Derivation formula: `K' = 35 - S`, `R' = 28 - U`, `N' = K' + R'`, verified for all six
applications against PDF Table 16 (page 29).

```c
typedef struct {
    int shortened;     /* S: MSB hexbits set to zero for encoding */
    int punctured;     /* U: LSB parity hexbits removed from codeword */
    int n_prime;       /* transmitted codeword length (hexbits) */
    int k_prime;       /* information length (hexbits) */
    int r_prime;       /* transmitted parity length (hexbits) */
    int payload_bits;  /* k_prime * 6 */
    int encoded_bits;  /* n_prime * 6 */
} rs_derived_t;

/* Source: TIA-102.BBAC-A PDF Table 16, page 29.
 * Invariant: K'=35-S, R'=28-U, N'=K'+R', payload=K'*6, encoded=N'*6. */
static const rs_derived_t RS_IECI   = { 12, 5,  46, 23, 23, 138, 276 };
static const rs_derived_t RS_OECI   = {  5, 6,  52, 30, 22, 180, 312 };
static const rs_derived_t RS_I_OEMI = {  5, 6,  52, 30, 22, 180, 312 };
static const rs_derived_t RS_IEMI   = {  9, 8,  46, 26, 20, 156, 276 };
static const rs_derived_t RS_S_OEMI = {  9, 9,  45, 26, 19, 156, 270 };
static const rs_derived_t RS_ESS    = { 19, 0,  44, 16, 28,  96, 264 };
```

### 6.3 GF(64) Arithmetic

Characteristic polynomial: `c(x) = x^6 + x + 1`, giving `alpha^6 = alpha + 1`.

Full table in `annex_tables/annex_d1_gf64_exp_log.csv`.

```c
/* GF(64) exponential table: GF64_EXP[e] = alpha^e, for e in 0..63.
 * Values are 6-bit field elements in polynomial form (octal notation matches spec).
 * Source: TIA-102.BBAC-A PDF Table 28, Annex D, pages 60-61.
 * Verified: EXP[63]==EXP[0]==1; 63 distinct values; round-trip with LOG table;
 *           alpha^6=0x03 (=alpha+1); alpha^7=0x06 (=alpha^2+alpha). */
static const uint8_t GF64_EXP[64] = {
    01, 02, 04, 010, 020, 040, 03, 06,
    014, 030, 060, 043, 05, 012, 024, 050,
    023, 046, 017, 036, 074, 073, 065, 051,
    021, 042, 07, 016, 034, 070, 063, 045,
    011, 022, 044, 013, 026, 054, 033, 066,
    057, 035, 072, 067, 055, 031, 062, 047,
    015, 032, 064, 053, 025, 052, 027, 056,
    037, 076, 077, 075, 071, 061, 041, 01,
};

/* GF(64) logarithm table: GF64_LOG[b] = e such that alpha^e = b.
 * GF64_LOG[0] = 255 (undefined sentinel -- zero element has no logarithm).
 * Indices are decimal; row comments show corresponding octal notation. */
static const uint8_t GF64_LOG[64] = {
    255,  0,  1,  6,  2, 12,  7, 26,   /* b = 00-07 octal */
      3, 32, 13, 35,  8, 48, 27, 18,   /* b = 10-17 octal */
      4, 24, 33, 16, 14, 52, 36, 54,   /* b = 20-27 octal */
      9, 45, 49, 38, 28, 41, 19, 56,   /* b = 30-37 octal */
      5, 62, 25, 11, 34, 31, 17, 47,   /* b = 40-47 octal */
     15, 23, 53, 51, 37, 44, 55, 40,   /* b = 50-57 octal */
     10, 61, 46, 30, 50, 22, 39, 43,   /* b = 60-67 octal */
     29, 60, 42, 21, 20, 59, 57, 58,   /* b = 70-77 octal */
};
```

### 6.4 RS Generator Polynomial

```
g(x) = x^28 + 26*x^27 + 55*x^26 + 65*x^25 + 12*x^24 + 51*x^23 + 67*x^22 +
       43*x^21 + 12*x^20 + 26*x^19 + 35*x^18 + 27*x^17 + 15*x^16 +
       75*x^15 + 55*x^14 + 42*x^13 + 67*x^12 + 50*x^11 + 45*x^10 +
       56*x^9  + 61*x^8  + 42*x^7  + 51*x^6  + 11*x^5  + 53*x^4  +
       07*x^3  + 24*x^2  + 13*x    + 34
```

All coefficients are octal field elements (6-bit GF(64) values).

Full table in `annex_tables/annex_d2_rs_gen_poly.csv`.

```c
/* RS(63,35,29) generator polynomial coefficients in GF(64).
 * RS_GEN_POLY[i] = coefficient of x^i; RS_GEN_POLY[0] = constant term.
 * Values in octal to match spec notation (TIA-102.BBAC-A Annex D Eq. 5, page 64).
 * Verified: leading coefficient RS_GEN_POLY[28]=01 (=1); constant term != 0;
 *           g(alpha^e) = 0 for all e = 1..28 over GF(64). */
static const uint8_t RS_GEN_POLY[29] = {
    034, 013, 024, 07, 053, 011, 051, 042,
    061, 056, 045, 050, 067, 042, 055, 075,
    015, 027, 035, 026, 012, 043, 067, 051,
    012, 065, 055, 026, 01,   /* x^28 coefficient = 1 */
};
```

### 6.5 RS(63,35,29) Generator Matrix G

The full 35x63 generator matrix G in systematic form [I_35 | P] is in
`annex_tables/annex_d3_rs_gen_matrix.csv` (35 rows x 63 columns of GF(64) decimal values).

Verified invariants (Phase 4, source: PDF Table 29, pages 62-64):
- Top-left 35x35 block = identity matrix
- All 35 rows are valid RS(63,35,29) codewords: Horner evaluation at alpha^1..alpha^28 = 0

### 6.6 Decoding Recommendation

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

```c
/* What each 4V burst's 24-bit ESS-B field carries:
 * Burst 1: ALGID[7:0] (8 bits) + KID[15:0] (16 bits)
 * Burst 2: MI[71:48]  (24 bits)
 * Burst 3: MI[47:24]  (24 bits)
 * Burst 4: MI[23:0]   (24 bits) */
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

```c
#define IECI_MAC_PAYLOAD_BITS    136  /* + 2 reserved = 138 total = 23 hexbits */
#define OECI_MAC_PAYLOAD_BITS    180  /* = 30 hexbits */
#define I_OEMI_MAC_PAYLOAD_BITS  180  /* = 30 hexbits (same as OECI) */
#define IEMI_MAC_PAYLOAD_BITS    156  /* = 26 hexbits */
#define S_OEMI_MAC_PAYLOAD_BITS  156  /* = 26 hexbits */
```

In bytes:

```c
#define IECI_MAC_PAYLOAD_BYTES    17  /* 136 bits = 17 bytes */
#define OECI_MAC_PAYLOAD_BYTES    22  /* 180 bits = 22 full bytes + 4 bits */
#define I_OEMI_MAC_PAYLOAD_BYTES  22  /* same as OECI */
#define IEMI_MAC_PAYLOAD_BYTES    19  /* 156 bits = 19 full bytes + 4 bits */
#define S_OEMI_MAC_PAYLOAD_BYTES  19  /* same as IEMI */
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

```c
#define VOICE_FRAMES_PER_CYCLE   18
#define VOICE_BURSTS_PER_CYCLE    5
#define VOICE_FRAME_DURATION_MS  20   /* milliseconds */
#define VOICE_CYCLE_DURATION_MS  360  /* 18 * 20 ms */
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

```c
typedef struct {
    bool    emergency;    /* bit 7: E flag */
    bool    encrypted;    /* bit 6: P flag */
    bool    duplex;       /* bit 5: D flag */
    bool    circuit_mode; /* bit 4: M flag */
    uint8_t priority;     /* bits 2-0: 0-7 */
} service_options_t;

static service_options_t service_options_decode(uint8_t b) {
    service_options_t so;
    so.emergency    = (b >> 7) & 1;
    so.encrypted    = (b >> 6) & 1;
    so.duplex       = (b >> 5) & 1;
    so.circuit_mode = (b >> 4) & 1;
    so.priority     = b & 0x07;
    return so;
}

static uint8_t service_options_encode(const service_options_t *so) {
    return ((uint8_t)so->emergency    << 7)
         | ((uint8_t)so->encrypted    << 6)
         | ((uint8_t)so->duplex       << 5)
         | ((uint8_t)so->circuit_mode << 4)
         | (so->priority & 0x07);
}
```

**NOTE:** The full Service Options field definition is specified in TIA-102.BBAD-A and
TIA-102.AABC-E. The above is the commonly implemented subset based on the BBAC-1 VCU
message format. Bit 3 may carry additional meaning in some message contexts per BBAD-A.

---

## 12. FDMA-to-TDMA Synchronization

### 12.1 SYNC_BCST TSBK Alignment

For sites with FDMA control channels synchronized to TDMA traffic channels:

```c
/* Compute superframe and ultraframe marks from SYNC_BCST TSBK fields.
 *
 * microslot = 7.5 ms (FDMA CCH timing unit)
 * superframe = 48 microslots = 360 ms
 * ultraframe = 192 microslots = 1440 ms
 */
static void compute_tdma_marks(uint16_t minutes, uint16_t micro_slots,
                                uint16_t *superframe_mark, uint16_t *ultraframe_mark) {
    uint32_t total = (uint32_t)minutes * 8000 + micro_slots;
    if (superframe_mark)  *superframe_mark  = (uint16_t)(total % 48);
    if (ultraframe_mark)  *ultraframe_mark  = (uint16_t)(total % 192);
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

## 13. Phase 4 Verification Status

Phase 4 uplift completed 2026-04-13. All previously deferred items resolved or
reclassified.

### 13.1 Verified (Phase 4)

- [x] Superframe/ultraframe structure and slot assignments
- [x] DUID generator matrix -- GF(2) rank=4, min distance=4 (PDF Table 12, page 24)
- [x] Synchronization sequences -- all three matched PDF Table 11, page 23
- [x] Pilot sequences
- [x] I-ISCH information field format and LCH Type table
- [x] I-ISCH generator matrix -- **two rows corrected** (row 0 bits 36/35 swapped;
      row 4 bit 26 vs 27); verified GF(2) rank=9 and min distance=16
      (PDF Table 15, page 28; see `annex_tables/annex_5_iisch_gen_matrix.csv`)
- [x] I-ISCH codeword offset vector C0 = 0x184229D461 (PDF Eq. 3, page 28)
- [x] RS mother code parameters and derived code table -- all arithmetic verified
      K'=35-S, R'=28-U, N'=K'+R' (PDF Table 16, page 29;
      see `annex_tables/annex_4_rs_derived_codes.csv`)
- [x] GF(64) exponential and logarithm tables -- round-trip verified for all 63
      non-zero elements; char poly alpha^6=alpha+1 verified
      (PDF Table 28, page 61; see `annex_tables/annex_d1_gf64_exp_log.csv`)
- [x] RS generator polynomial -- roots alpha^1..alpha^28 verified over GF(64)
      (PDF Eq. 5, page 64; see `annex_tables/annex_d2_rs_gen_poly.csv`)
- [x] RS(63,35,29) generator matrix -- all 35 rows verified as valid codewords
      via Horner evaluation at alpha^1..alpha^28; top-left 35x35 = identity
      (PDF Table 29, pages 62-64; see `annex_tables/annex_d3_rs_gen_matrix.csv`)
- [x] ESS structure and distribution across bursts
- [x] ESS-B content mapping per 4V burst
- [x] Burst type summary table
- [x] Voice burst sequencing pattern
- [x] DUID sequencing for voice bursts
- [x] All inline code blocks converted from Rust to C

### 13.2 Requires BBAD-A (Separate Document -- out of scope for this spec)

- [ ] MAC_PTT PDU byte-level structure (defined in TIA-102.BBAD-A [8])
- [ ] MAC_END_PTT PDU byte-level structure (defined in TIA-102.BBAD-A [8])
- [ ] Full Network Status Broadcast field layout (in BBAD-A section 8.3.1.25)
- [ ] Complete Service Options semantics for all message contexts
- [ ] MAC_ACTIVE, MAC_IDLE, MAC_HANGTIME PDU wrapper structures
- [ ] All signaling procedures (TIA-102.BBAE [9])

### 13.3 Remaining Open Items

- [ ] Annex A/B burst bit location tables (BBAC-A has them as text descriptions of
      full 180-symbol tables; the BBAC-1 Annex E tables provide the VCH burst tables
      but LCCH burst tables IECI/OECI were not extracted into CSV form)

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

*Phase 3 extracted from TIA-102.BBAC-A full text (2026-04-12). Phase 4 uplift (2026-04-13)
corrected two I-ISCH generator matrix rows (verified min distance=16), extracted and
verified all tables programmatically from pdftotext output, converted all code to C,
and created CSV annex tables. Cross-validate against SDRTrunk and OP25 implementations.*
