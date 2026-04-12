# P25 Trunking Control Channel Formats -- Implementation Specification

**Source:** TIA-102.AABB-B (Revision B, April 2011)
**Classification:** MESSAGE_FORMAT
**Phase:** 3 -- Implementation-ready
**Extracted:** 2026-04-12
**Purpose:** Self-contained spec for implementing P25 FDMA trunking control channel
framing, TSBK extraction from raw FDMA frames, multi-block trunking (MBT) packet
reassembly, inbound/outbound channel structure, and slotted ALOHA access. This is the
transport layer that carries AABC-E message content. No reference to the original PDF
required.

**Cross-references:**
- TIA-102.BAAA-B -- FDMA Common Air Interface: physical layer, frame sync, NID, trellis
  coding, CRC polynomials. The TSDU (DUID=0x07) and PDU (DUID=0x0C) frame types carry
  the control channel packets defined here.
- TIA-102.AABC-E -- Trunking Control Channel Messages: opcode definitions, argument
  field layouts, and per-message semantics that ride inside the TSBK and MBT structures
  defined here.
- TIA-102.AABD-A -- Trunking Procedures: call setup, registration, and slotted ALOHA
  behavioral procedures that use these packet formats.
- TIA-102.BAAC-A -- Reserved Values: MFID assignments, NAC reserved values.
- TIA-102.BBAC-A / BBAD-A -- TDMA Phase 2: LCCH (Logical Control Channel) carries
  MAC messages in TDMA slots, analogous to FDMA TSDU carrying TSBKs.
- SDRTrunk (Java) -- `io.github.dsheirer.module.decode.p25.phase1.message.tsbk`
- OP25 (C++) -- `op25/gr-op25_repeater/lib/p25p1_fdma.cc`, `rx_tsbk.cc`

---

## 1. Control Channel MAC PDU Structure

### 1.1 Fundamental Parameters

| Parameter | Value |
|-----------|-------|
| Control channel data rate | 9,600 bps |
| Symbol rate | 4,800 symbols/sec |
| Bits per symbol (dibit) | 2 |
| Microslot duration | 7.5 ms |
| Microslot size | 70 bits (information) + 2 bits (status symbol) = 72 bits |
| TSBK information block | 12 octets = 96 bits |
| TSBK after rate-1/2 trellis coding | 196 bits (98 dibits) |
| Frame Sync (FS) | 48 bits |
| Network Identifier (NID) | 64 bits |
| Preamble total (FS + NID) | 112 bits |

```rust
/// Fundamental control channel constants
pub const CONTROL_CHANNEL_BPS: u32 = 9600;
pub const SYMBOL_RATE: u32 = 4800;
pub const MICROSLOT_DURATION_US: u32 = 7500;    // 7.5 ms
pub const MICROSLOT_INFO_BITS: u32 = 70;
pub const STATUS_SYMBOL_BITS: u32 = 2;
pub const MICROSLOT_TOTAL_BITS: u32 = 72;       // 70 + 2 SS
pub const FRAME_SYNC_BITS: u32 = 48;
pub const NID_BITS: u32 = 64;
pub const PREAMBLE_BITS: u32 = FRAME_SYNC_BITS + NID_BITS; // 112
pub const TSBK_INFO_OCTETS: usize = 12;
pub const TSBK_INFO_BITS: u32 = 96;
pub const TSBK_CODED_BITS: u32 = 196;           // After rate-1/2 trellis
pub const TSBK_CODED_DIBITS: u32 = 98;
```

### 1.2 Two Packet Families

The control channel carries two packet families, distinguished by the DUID in the NID:

| Packet Family | DUID | Hex | Description |
|---------------|------|-----|-------------|
| Single-Block (TSBK) | `0111` | `0x7` | 1-3 TSBKs per OSP, 1 TSBK per ISP |
| Multi-Block (PDU) | `1100` | `0xC` | Header block + 1-3 data blocks |

```rust
/// DUID values for control channel packet types
pub const DUID_TSBK: u8 = 0x07;   // Single-block: Trunking Signaling Data Unit (TSDU)
pub const DUID_PDU: u8 = 0x0C;    // Multi-block: Packet Data Unit

/// NOTE: BAAA-B defines DUID_PDU as 0x0C for both TSDU (TSBK payloads) and
/// generic PDU (data payloads). The two are distinguished by the first block's
/// content: TSBK blocks have LB/P/Opcode in octet 0; PDU header blocks have
/// AN/IO/Format in octet 0. AABB-B uses DUID $7 for TSBK and $C for multi-block.
/// In practice some implementations use DUID 0x07 for TSDU.
///
/// SDRTrunk maps: DUID 0x07 -> TSBKMessage, DUID 0x0C -> PDUMessage
/// OP25 maps: duid == 7 -> process_tsbk(), duid == 12 -> process_pdu()
```

**Implementation note on DUID ambiguity:** AABB-B clause 4.2 explicitly states DUID `$7`
for single-block and `$C` for multi-block. However, BAAA-B only formally defines DUID
values 0x0, 0x3, 0x5, 0xA, 0xC, 0xF. The value 0x7 is used in practice but is technically
in the "reserved" range of BAAA-B. Both SDRTrunk and OP25 handle DUID 0x7 as TSBK.

---

## 2. Outbound Control Channel Format (FNE to SU)

### 2.1 Continuous Outbound Stream

The outbound control channel is a continuous transmission from the FNE (Fixed Network
Equipment). On a dedicated control channel, OSPs are transmitted back-to-back with no
gaps. The FNE dynamically selects single, double, or triple TSBK formats to maximize
throughput.

### 2.2 Single TSBK OSP -- 37.5 ms (5 microslots)

```
Bit:    0                48              112             308   318  360
        |--- FS (48) ----|--- NID (64) ---|--- TSBK (196) ---| SS |null|
                                                             (10) (42)
Microslots:  |----1----|----2----|----3----|----4----|----5----|
             7.5ms each
```

| Component | Bits | Dibits |
|-----------|------|--------|
| Frame Sync | 48 | 24 |
| NID | 64 | 32 |
| TSBK (trellis coded) | 196 | 98 |
| Status Symbols | 10 | 5 |
| Null padding | 42 | 21 |
| **Total** | **360** | **180** |

### 2.3 Double TSBK OSP -- 60 ms (8 microslots)

```
Bit:    0         48       112        308        504   520   576
        |-- FS ---|-- NID --|-- TSBK --|-- TSBK2 -| SS |null|
        (48)      (64)      (196)      (196)     (16)  (56)
Microslots:  |--1--|--2--|--3--|--4--|--5--|--6--|--7--|--8--|
```

| Component | Bits |
|-----------|------|
| Frame Sync | 48 |
| NID | 64 |
| TSBK 1 (trellis coded) | 196 |
| TSBK 2 (trellis coded) | 196 |
| Status Symbols | 16 |
| Null padding | 56 |
| **Total** | **576** |

### 2.4 Triple TSBK OSP -- 75 ms (10 microslots)

```
Bit:    0       48     112       308       504       700  720
        |--FS--|--NID--|--TSBK--|--TSBK2--|--TSBK3--| SS |
        (48)   (64)    (196)    (196)     (196)     (20)
Microslots:  |--1--|--2--|--3--|--4--|--5--|--6--|--7--|--8--|--9--|--10-|
```

| Component | Bits |
|-----------|------|
| Frame Sync | 48 |
| NID | 64 |
| TSBK 1 (trellis coded) | 196 |
| TSBK 2 (trellis coded) | 196 |
| TSBK 3 (trellis coded) | 196 |
| Status Symbols | 20 |
| **Total** | **720** |

Triple TSBK is the most efficient format (3 TSBKs in 10 microslots vs. 3 single-TSBK
OSPs requiring 15 microslots).

```rust
/// Outbound OSP format variants
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OspFormat {
    SingleTsbk,   // 5 microslots, 37.5 ms, 1 TSBK
    DoubleTsbk,   // 8 microslots, 60.0 ms, 2 TSBKs
    TripleTsbk,   // 10 microslots, 75.0 ms, 3 TSBKs
}

impl OspFormat {
    pub const fn microslots(self) -> u32 {
        match self {
            OspFormat::SingleTsbk => 5,
            OspFormat::DoubleTsbk => 8,
            OspFormat::TripleTsbk => 10,
        }
    }

    pub const fn duration_ms(self) -> f32 {
        self.microslots() as f32 * 7.5
    }

    pub const fn tsbk_count(self) -> usize {
        match self {
            OspFormat::SingleTsbk => 1,
            OspFormat::DoubleTsbk => 2,
            OspFormat::TripleTsbk => 3,
        }
    }

    pub const fn total_bits(self) -> u32 {
        self.microslots() * MICROSLOT_TOTAL_BITS
    }
}
```

### 2.5 How the SU Determines OSP Format

The subscriber unit determines how many TSBKs are in the current OSP by:

1. Detecting Frame Sync to identify OSP start
2. Decoding NID to get DUID (must be 0x07 for TSBK)
3. Decoding the first TSBK and checking its LB (Last Block) bit
   - LB=1: single TSBK OSP, expect next FS immediately
   - LB=0: at least one more TSBK follows, decode next 196-bit block
4. If second TSBK has LB=1: double TSBK OSP
5. If second TSBK has LB=0: third TSBK follows (triple TSBK OSP)

Each TSBK in a multi-TSBK OSP can independently be protected or nonprotected (the P
bit is per-TSBK).

---

## 3. Inbound Control Channel Format (SU to FNE)

### 3.1 ISP Structure -- 37.5 ms (5 microslots)

```
Bit:    0        48       112             308  316  360
        |--FS --|--NID --|--- TSBK (196) --| SS |null|
        (48)    (64)     (196)            (8)  (44)
Microslots:  |----1----|----2----|----3----|----4----|----5----|
```

| Component | Bits |
|-----------|------|
| Frame Sync | 48 |
| NID | 64 |
| TSBK (trellis coded) | 196 |
| Status Symbols | 8 |
| Null padding (not transmitted) | 44 |
| **Total slot** | **360** |
| Required transmit time | 316 bits = 32.9 ms |

**Key constraint:** An ISP carries exactly ONE TSBK. No multi-TSBK ISP is permitted.
The LB bit in an ISP TSBK is always set to 1.

The 44-bit guard time absorbs TONT + TOFFT + propagation delay:
```
360 (slot) - 196 (TSBK) - 64 (NID) - 48 (FS) - 8 (SS) = 44 guard bits
```

### 3.2 Slotted ALOHA Access Procedure

Inbound channel access uses Slotted ALOHA:

1. **Slot boundary detection:** The SU monitors outbound Status Symbols. A status value
   of `%11` (IDLE) marks the start of an inbound slot boundary.
2. **Intra-slot spacing:** Status symbols with value `%10` (UNKNOWN) occur at microslot
   boundaries within a slot; these do NOT mark slot starts.
3. **Slot size:** A multiple of the 7.5 ms microslot. For a standard ISP, the slot is
   5 microslots (37.5 ms).
4. **Transmission:** After PTT assertion, the SU waits for the next IDLE status symbol
   on the outbound channel, then transmits its ISP aligned to that slot boundary.
5. **Collision:** If two SUs transmit in the same slot, the RFSS receives corrupted data.
   The SU retries after a random backoff (per AABD-A procedures).

### 3.3 Status Symbol Codes

**Outbound (OSP) Status Symbols:**

| Value | Binary | Meaning |
|-------|--------|---------|
| 0 | `%00` | Unknown -- not used for OSPs |
| 1 | `%01` | Busy -- inbound channel not available |
| 2 | `%10` | Unknown -- intra-slot microslot boundary |
| 3 | `%11` | IDLE -- marks start of inbound slot |

**Inbound (ISP) Status Symbols:**

| Value | Binary | Meaning |
|-------|--------|---------|
| 0 | `%00` | Unknown -- not used for ISPs |
| 1 | `%01` | Busy -- not used for ISPs |
| 2 | `%10` | Unknown -- standard ISP status |
| 3 | `%11` | Idle -- not used for ISPs |

```rust
/// Outbound status symbol values
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OutboundStatusSymbol {
    Unknown00   = 0b00,   // Not used for OSPs
    Busy        = 0b01,   // Inbound control channel not available
    IntraSlot   = 0b10,   // Microslot boundary within a slot
    Idle        = 0b11,   // Start of inbound slot boundary
}

impl OutboundStatusSymbol {
    pub fn from_dibit(val: u8) -> Self {
        match val & 0x03 {
            0b00 => Self::Unknown00,
            0b01 => Self::Busy,
            0b10 => Self::IntraSlot,
            0b11 => Self::Idle,
            _ => unreachable!(),
        }
    }

    /// Returns true if this marks an inbound slot boundary
    pub fn is_slot_boundary(self) -> bool {
        matches!(self, Self::Idle)
    }
}
```

**Important:** Status symbols are ALWAYS transmitted in nonprotected form, regardless
of whether the control channel is operating in protected mode.

### 3.4 Multi-Block ISP Formats

For multi-block inbound messages, the slot size must accommodate the larger packet:

| Format | Microslots | Duration | Transmit time |
|--------|------------|----------|---------------|
| 1 data block ISP | 10 | 75.0 ms | 54.2 ms |
| 2 data block ISP | 15 | 112.5 ms | 75.0 ms |
| 3 data block ISP | 15 | 112.5 ms | 96.0 ms |

---

## 4. TSBK Framing Within the Data Unit

### 4.1 The 12-Octet TSBK Structure

```
Octet   Bit 7   Bit 6   Bits 5-0
  0   |  LB   |   P   |  Opcode (6 bits)  |  <- Always nonprotected
  1   |     Manufacturer's ID (MFID)       |  <- Always nonprotected
  2   |                                     |
  3   |                                     |
  4   |                                     |
  5   |          Arguments (8 octets)       |  <- May be protected
  6   |                                     |
  7   |                                     |
  8   |                                     |
  9   |                                     |
 10   |          TSBK CRC-16               |  <- Always nonprotected
 11   |                                     |
```

### 4.2 Field Definitions

| Field | Location | Size | Description |
|-------|----------|------|-------------|
| LB | Octet 0, bit 7 | 1 bit | Last Block: `1` = final TSBK in this OSP; `0` = more follow |
| P | Octet 0, bit 6 | 1 bit | Protected: `1` = encrypted; `0` = plaintext |
| Opcode | Octet 0, bits 5:0 | 6 bits | Message type (see AABC-E for definitions) |
| MFID | Octet 1 | 8 bits | Manufacturer's ID: `0x00` or `0x01` = standard P25 |
| Arguments | Octets 2-9 | 64 bits | Message-specific; includes Logical Link ID in octets 7-9 |
| LLID | Octets 7-9 | 24 bits | Logical Link ID: destination SU (OSP) or source SU (ISP) |
| CRC | Octets 10-11 | 16 bits | CCITT CRC-16 per BAAA-B subclause 6.2 |

### 4.3 LB Bit Semantics

The LB bit controls multi-TSBK packing in outbound OSPs:

| Scenario | TSBK 1 LB | TSBK 2 LB | TSBK 3 LB |
|----------|-----------|-----------|-----------|
| Single TSBK OSP | 1 | -- | -- |
| Double TSBK OSP | 0 | 1 | -- |
| Triple TSBK OSP | 0 | 0 | 1 |
| ISP (always single) | 1 | -- | -- |

### 4.4 Protection Model

When P=1 (protected TSBK):

| Field | Protected? | Rationale |
|-------|-----------|-----------|
| Octet 0 bits 7-6 (LB, P) | NO | SU must detect block boundaries and protection state |
| Octet 0 bits 5-0 (Opcode) | YES | Conceals message type from unauthorized listeners |
| Octet 1 (MFID) | NO | Required to select encryption key set |
| Octets 2-9 (Arguments) | YES | Conceals operational data |
| Octets 10-11 (CRC) | NO | Any SU can validate block integrity without decrypting |

The CRC is computed AFTER encryption is applied to the protected fields. This means
non-crypto-capable radios can still validate the block was received correctly (for
control channel synchronization), even though they cannot read the message content.

```rust
/// Extract the always-nonprotected fields from a TSBK without decryption.
/// Useful for non-crypto SUs to maintain control channel lock.
pub fn tsbk_unprotected_fields(tsbk: &[u8; 12]) -> TsbkUnprotected {
    TsbkUnprotected {
        last_block: (tsbk[0] >> 7) & 1 == 1,
        protected: (tsbk[0] >> 6) & 1 == 1,
        mfid: tsbk[1],
        crc: ((tsbk[10] as u16) << 8) | tsbk[11] as u16,
    }
}

pub struct TsbkUnprotected {
    pub last_block: bool,
    pub protected: bool,
    pub mfid: u8,
    pub crc: u16,
}
```

### 4.5 CRC-16 Specification

```
Polynomial: x^16 + x^12 + x^5 + 1  (0x1021, CCITT)
Initial value: 0xFFFF
Input: Octets 0-9 of the TSBK (80 bits), processed MSB first
Output: 16-bit CRC stored MSB-first in octets 10-11
No final XOR
Reference: BAAA-B subclause 6.2
```

```rust
/// Compute CCITT CRC-16 for a TSBK.
/// Input: 10 octets (octets 0..9).
/// Output: 16-bit CRC to compare against octets 10..11.
pub fn tsbk_crc16(data: &[u8]) -> u16 {
    assert!(data.len() == 10);
    let mut crc: u16 = 0xFFFF;
    for &byte in data {
        crc ^= (byte as u16) << 8;
        for _ in 0..8 {
            if crc & 0x8000 != 0 {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    crc & 0xFFFF
}

/// Validate a complete 12-byte TSBK.
pub fn tsbk_crc_valid(tsbk: &[u8; 12]) -> bool {
    let computed = tsbk_crc16(&tsbk[0..10]);
    let stored = ((tsbk[10] as u16) << 8) | tsbk[11] as u16;
    computed == stored
}
```

### 4.6 Trellis Coding per TSBK Block

Each 12-octet TSBK (96 information bits = 48 dibits) is encoded using the rate-1/2
trellis code defined in BAAA-B clause 7:

```
Input:  48 dibits (96 bits) + 1 flush dibit (00) = 49 input symbols
Output: 98 dibits = 196 bits per coded block
Rate:   1/2 (each input dibit produces 2 output dibits via constellation mapping)
```

The trellis encoder state machine and constellation point mapping are defined in the
BAAA-B Implementation Spec (Section 10). Each TSBK block is independently trellis-coded
with encoder state reset to 0 at the start of each block.

---

## 5. Multi-Block Trunking (MBT)

### 5.1 Overview

Multi-block trunking packets carry control channel messages that exceed the 10-octet
argument capacity of a single TSBK. The packet uses the Unconfirmed Data Packet structure
from BAAA-B with a specialized header block.

| Property | Value |
|----------|-------|
| NID DUID | `0x0C` (PDU) |
| Maximum blocks | 4 (1 header + 3 data) |
| Maximum octets | 44 |
| Block size | 12 octets each |
| Maximum outbound latency | ~135 ms |

### 5.2 Header Block Format -- Standard (Format `%10101` = 0x15)

```
Octet   Bit 7   Bit 6   Bit 5   Bits 4-0
  0   |   0   |   AN  |  I/O  |  Format (%10101)  |  <- nonprotected
  1   |   1   |   1   |  SAP Identifier (6 bits)   |  <- nonprotected
  2   |         Manufacturer's ID (MFID)            |  <- nonprotected
  3   |                                              |
  4   |         Logical Link ID (24 bits)            |  <- optionally protected
  5   |                                              |
  6   |   1   |     Blocks to Follow (7 bits)        |  <- nonprotected
  7   |   0   | 0 | 0 |  Pad Octet Count (5 bits)   |  <- nonprotected
  8   |         Reserved (set to 0)                  |  <- nonprotected
  9   |   0   | 0 |   Data Header Offset (6 bits)    |  <- nonprotected
 10   |         Header CRC-16                        |
 11   |                                              |  <- nonprotected
```

### 5.3 Header Block Format -- Alternative (Format `%10111` = 0x17)

```
Octet   Bit 7   Bit 6   Bit 5   Bits 4-0
  0   |   0   |   AN  |  I/O  |  Format (%10111)  |  <- nonprotected
  1   |   1   |   1   |  SAP Identifier (6 bits)   |  <- nonprotected
  2   |         Manufacturer's ID (MFID)            |  <- nonprotected
  3   |                                              |
  4   |         Logical Link ID (24 bits)            |  <- optionally protected
  5   |                                              |
  6   |   1   |     Blocks to Follow (7 bits)        |  <- nonprotected
  7   |   0   |   0   |  Opcode (6 bits)             |  <- nonprotected*
  8   |    Defined by Trunking Messages (AABC-E)     |  <- nonprotected*
  9   |    Defined by Trunking Messages (AABC-E)     |  <- nonprotected*
 10   |         Header CRC-16                        |
 11   |                                              |  <- nonprotected
```

*Note: In protected mode, the alternative format's Opcode (octet 7 bits 5:0) and
octets 8-9 are also encrypted, in addition to the LLID (octets 3-5).

### 5.4 Header Field Definitions

| Field | Location | Value/Description |
|-------|----------|-------------------|
| Octet 0 bit 7 | Fixed | Always `0` |
| AN | Octet 0 bit 6 | ARQ/Non-ARQ: `1` for MBT |
| I/O | Octet 0 bit 5 | Direction: `0` = ISP (SU->FNE), `1` = OSP (FNE->SU) |
| Format | Octet 0 bits 4:0 | `0b10101` (0x15) = standard MBT; `0b10111` (0x17) = alternative MBT |
| SAP bits 7:6 | Octet 1 bits 7:6 | Always `11` |
| SAP ID | Octet 1 bits 5:0 | `61` (0x3D) = Trunked Control; `63` (0x3F) = Protected Trunked Control |
| MFID | Octet 2 | `0x00` or `0x01` = standard P25 |
| LLID | Octets 3-5 | 24-bit address (source for ISP, destination for OSP) |
| Octet 6 bit 7 | Fixed | Always `1` |
| Blocks to Follow | Octet 6 bits 6:0 | Number of data blocks following header (1-3) |
| Header CRC | Octets 10-11 | CRC-16 over octets 0-9 |

```rust
/// MBT header format values
pub const MBT_FORMAT_STANDARD: u8 = 0x15;     // %10101
pub const MBT_FORMAT_ALTERNATIVE: u8 = 0x17;  // %10111

/// Service Access Point identifiers for trunking control
pub const SAP_TRUNKED_CONTROL: u8 = 61;       // 0x3D
pub const SAP_PROTECTED_TRUNKED_CONTROL: u8 = 63; // 0x3F

/// SAP high bits (octet 1 bits 7:6) are always 0b11 for trunking
pub const SAP_HIGH_BITS: u8 = 0xC0;
```

### 5.5 Data Block Structure

Each data block is 12 octets. The last data block contains a 4-octet CRC spanning
all data block content (excluding the header block CRC):

```
Data Block 1..N-1:
  Octets 0-11: Message data (12 octets each)

Last Data Block:
  Octets 0-7:  Message data (8 octets)
  Octets 8-11: 32-bit Packet CRC (per BAAA-B subclause 6.3)
```

Each data block is independently trellis-coded at rate 1/2 (same as TSBK blocks).

### 5.6 Multi-Block Timing

**Outbound (OSP):**

| Blocks (header + data) | Microslots | Duration |
|------------------------|------------|----------|
| 1 + 1 = 2 blocks | 8 | 60.0 ms |
| 1 + 2 = 3 blocks | 10 | 75.0 ms |
| 1 + 3 = 4 blocks | 13 | 97.5 ms |

**Inbound (ISP):**

| Blocks (header + data) | Microslots | Duration | Transmit time |
|------------------------|------------|----------|---------------|
| 1 + 1 = 2 blocks | 10 | 75.0 ms | 54.2 ms |
| 1 + 2 = 3 blocks | 15 | 112.5 ms | 75.0 ms |
| 1 + 3 = 4 blocks | 15 | 112.5 ms | 96.0 ms |

```rust
/// Multi-block OSP timing
pub const MBT_OSP_1_DATA_MICROSLOTS: u32 = 8;   // 60 ms
pub const MBT_OSP_2_DATA_MICROSLOTS: u32 = 10;  // 75 ms
pub const MBT_OSP_3_DATA_MICROSLOTS: u32 = 13;  // 97.5 ms

/// Multi-block ISP timing
pub const MBT_ISP_1_DATA_MICROSLOTS: u32 = 10;  // 75 ms
pub const MBT_ISP_2_DATA_MICROSLOTS: u32 = 15;  // 112.5 ms
pub const MBT_ISP_3_DATA_MICROSLOTS: u32 = 15;  // 112.5 ms

/// Maximum outbound latency for the largest MBT packet
pub const MBT_MAX_LATENCY_MS: f32 = 135.0;
```

### 5.7 Protected Multi-Block Packets

| Component | Standard Format | Alternative Format |
|-----------|----------------|-------------------|
| Header octets 0-2 (format, SAP, MFID) | Nonprotected | Nonprotected |
| Header octets 3-5 (LLID) | PROTECTED | PROTECTED |
| Header octet 6 (blocks-to-follow) | Nonprotected | Nonprotected |
| Header octets 7-9 | Nonprotected (pad/rsv/offset) | PROTECTED (opcode, msg fields) |
| Header octets 10-11 (CRC) | Nonprotected (computed after encryption) | Same |
| Data block content | PROTECTED | PROTECTED |
| Last data block CRC (4 octets) | Nonprotected (computed after encryption) | Same |

---

## 6. Control Channel Acquisition

### 6.1 SU Acquisition Procedure

When a subscriber unit powers on or loses control channel lock, it must find and
synchronize to a control channel:

```
PROCEDURE acquire_control_channel():
    1. SCAN designated control channel frequencies (from preprogrammed list
       or last-known frequency)

    2. For each candidate frequency:
       a. Search for 48-bit Frame Sync pattern (0x5575F5FF7FFF)
          - Allow up to 6 bit errors (Hamming distance threshold)
          - SDRTrunk: P25P1FrameSync.java, threshold = 6
          - OP25: p25_frame_assembler_impl.cc, uses same pattern

       b. Once FS detected, read next 64 bits as NID
          - BCH(63,16,23) decode to extract NAC and DUID
          - If BCH decode fails -> return to FS search

       c. Validate NAC against expected Network Access Code
          - NAC is 12 bits, identifies the P25 system/site
          - Special values: 0x293 = default, 0xF7E = receive-any,
            0xF7F = repeater-any (per BAAC-A)

       d. Verify DUID indicates control channel traffic:
          - DUID 0x07 (TSBK) or DUID 0x0C (PDU)

       e. Decode first TSBK block via trellis decoding
          - Validate CRC-16
          - If CRC valid -> control channel acquired

    3. Once acquired:
       - Store NAC for all subsequent ISP transmissions
       - Monitor outbound stream for Status Symbols to determine
         inbound slot timing
       - Begin processing OSP messages (per AABC-E opcode dispatch)
```

### 6.2 NAC Handling

```rust
/// Network Access Code extracted from NID
pub const NAC_DEFAULT: u16 = 0x293;
pub const NAC_RECEIVE_ANY: u16 = 0xF7E;
pub const NAC_REPEATER_ANY: u16 = 0xF7F;

/// Validate NAC for control channel acquisition
pub fn nac_acceptable(received_nac: u16, expected_nac: u16) -> bool {
    if expected_nac == NAC_RECEIVE_ANY {
        return true;  // Accept any NAC
    }
    received_nac == expected_nac
}
```

### 6.3 Control Channel Modes

| Mode | Description |
|------|-------------|
| **Dedicated** | Channel used exclusively for control signaling; continuous OSP stream |
| **Composite** | Channel alternates between control and traffic bearer; control not available during traffic use |
| **Primary** | Main control channel for a site; used for normal transactions |
| **Secondary** | Additional control channel(s); may be dedicated or composite |

At least one 9600 bps control channel is required per RFSS. The primary control channel
is always designated. Subscriber units should monitor the primary and fall back to
secondary if the primary fails (per AABD-A procedures).

---

## 7. Data Header Formats

### 7.1 Format Field Values

The Format field (octet 0 bits 4:0) in the header block determines the packet type:

| Format Value | Binary | Packet Type |
|-------------|--------|-------------|
| 0x15 | `%10101` | Standard Multi-Block Trunking Control |
| 0x17 | `%10111` | Alternative Multi-Block Trunking Control |

These are distinct from other BAAA-B unconfirmed data packet formats:

| Format Value | Binary | Packet Type (from BAAA-B) |
|-------------|--------|---------------------------|
| 0x15 | `%10101` | Reserved in BAAA-B; used by AABB-B for MBT standard |
| 0x17 | `%10111` | Reserved in BAAA-B; used by AABB-B for MBT alternative |

### 7.2 Standard vs. Alternative Header Selection

| Use Case | Format | Rationale |
|----------|--------|-----------|
| Messages needing pad count, data header offset | Standard (0x15) | Compatible with generic unconfirmed data packet parsing |
| Messages where opcode must appear in header | Alternative (0x17) | Allows message routing without reading data blocks; more compact for common MBT messages |

The alternative format is preferred by most real-world implementations (SDRTrunk calls
this "AMBTC" -- Alternative Multi-Block Trunking Control).

```rust
/// Determine MBT header variant from the format field
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MbtHeaderVariant {
    Standard,     // Format 0x15 -- pad count, reserved, data header offset
    Alternative,  // Format 0x17 -- opcode, message-specific in octets 7-9
}

impl MbtHeaderVariant {
    pub fn from_format(format: u8) -> Option<Self> {
        match format & 0x1F {
            0x15 => Some(Self::Standard),
            0x17 => Some(Self::Alternative),
            _ => None,
        }
    }
}
```

---

## 8. FEC and Interleaving Specific to the Control Channel

### 8.1 Rate 1/2 Trellis Code

All control channel information blocks (TSBKs, MBT header blocks, MBT data blocks)
use the rate-1/2 trellis code defined in BAAA-B clause 7.

| Parameter | Value |
|-----------|-------|
| Encoder input | 48 dibits (96 bits) per block |
| Flush symbol | 1 dibit (00) appended |
| Encoder output | 98 dibits (196 bits) per block |
| FSM states | 4 |
| State transition table | BAAA-B Table 7-2 (see BAAA-B Implementation Spec Section 10.2) |
| Constellation mapping | 16-point mapping to dibit pairs (BAAA-B Table 7-4) |

### 8.2 Trellis Encoding/Decoding Pipeline

```
ENCODE (transmitter):
  1. Take 12-octet block (96 bits)
  2. Split into 48 dibits (MSB first within each byte)
  3. Initialize encoder state to 0
  4. For each of 48 dibits:
     a. Look up constellation point: TRELLIS_1_2[state][input_dibit]
     b. Map constellation point to dibit pair: CONSTELLATION[point]
     c. Emit 2 output dibits
     d. Advance state: state = input_dibit
  5. Emit flush: TRELLIS_1_2[state][0] -> 2 output dibits
  6. Result: 98 dibits = 196 bits

DECODE (receiver):
  1. Receive 98 dibits (196 bits)
  2. Map dibit pairs back to constellation points
  3. Viterbi decode through the 4-state trellis -> 49 dibits
  4. Discard flush dibit -> 48 dibits = 96 bits = 12 octets
  5. Verify CRC (TSBK CRC-16 or header/data block CRC)
```

### 8.3 Status Symbol Insertion/Stripping

Status symbols are inserted every 70 bits (35 dibits). The first SS appears after the
first 70 bits of the packet. SS are not part of the information content and must be
stripped before trellis decoding.

```
Transmitted bit stream:
  [70 info bits] [2 SS bits] [70 info bits] [2 SS bits] [70 info bits] ...

SS stripping:
  Every 36th raw dibit (position 35, 71, 107, ...) is a status symbol.
```

```rust
/// Strip status symbols from raw dibit stream.
/// Returns (info_dibits, status_symbols).
pub fn strip_status_symbols(raw: &[u8]) -> (Vec<u8>, Vec<u8>) {
    let mut info = Vec::new();
    let mut ss = Vec::new();
    for (i, &dibit) in raw.iter().enumerate() {
        if (i + 1) % 36 == 0 {
            ss.push(dibit);
        } else {
            info.push(dibit);
        }
    }
    (info, ss)
}
```

### 8.4 CRC Summary

| CRC Type | Polynomial | Init | Size | Applied To |
|----------|-----------|------|------|------------|
| TSBK CRC-16 | 0x1021 (CCITT) | 0xFFFF | 16 bits | Octets 0-9 of each TSBK (BAAA-B 6.2) |
| Header CRC-16 | 0x1021 (CCITT) | 0xFFFF | 16 bits | Octets 0-9 of MBT header block |
| Data Block CRC-32 | BAAA-B 6.3 | -- | 32 bits | All data block content (last 4 octets of last data block) |
| NID BCH | BCH(63,16,23) | -- | 47 parity + 1 parity | 16-bit NID info (NAC + DUID) |

---

## 9. Timing

### 9.1 Control Channel Data Rate and TSBK Delivery Rate

| Metric | Value |
|--------|-------|
| Raw channel rate | 9,600 bps |
| Microslot period | 7.5 ms (72 raw bits per microslot) |
| Triple TSBK OSP period | 75 ms (10 microslots) |
| TSBKs per triple OSP | 3 |
| Maximum TSBK throughput | 3 TSBKs / 75 ms = **40 TSBKs/sec** |
| Single TSBK OSP period | 37.5 ms (5 microslots) |
| Minimum TSBK throughput (single mode) | 1 TSBK / 37.5 ms = **26.7 TSBKs/sec** |
| ISP slot period | 37.5 ms (5 microslots) |
| ISP throughput (per slot) | 1 TSBK / 37.5 ms = **26.7 ISPs/sec max** |

### 9.2 Airtime Budget

```
Per microslot:  70 info bits + 2 SS bits = 72 bits
                72 bits / 9600 bps = 7.5 ms

Triple TSBK OSP breakdown:
  FS:    48 bits
  NID:   64 bits
  TSBK1: 196 bits (coded)
  TSBK2: 196 bits (coded)
  TSBK3: 196 bits (coded)
  SS:    20 bits (10 status symbols across 10 microslots)
  Total: 720 bits = 10 microslots

  Information bits delivered: 3 * 96 = 288 bits in 75 ms
  Effective info rate: 288 / 0.075 = 3,840 bps (of 9,600 raw)
  Overhead: FS + NID + SS + trellis expansion = 60% overhead
```

### 9.3 Mixed TSBK/MBT Throughput Impact

When the FNE must send an MBT packet, it reduces TSBK throughput:

| Scenario | Time consumed | TSBKs in same time (triple mode) |
|----------|--------------|----------------------------------|
| Single MBT (2 blocks) | 60 ms | 2.4 TSBKs equivalent |
| Double MBT (3 blocks) | 75 ms | 3.0 TSBKs equivalent |
| Triple MBT (4 blocks) | 97.5 ms | 3.9 TSBKs equivalent |

The FNE balances MBT messages against pending TSBK traffic and inbound ISP demand.

### 9.4 Timing Constants

```rust
/// Timing constants for control channel implementation
pub const MICROSLOT_MS: f32 = 7.5;

/// OSP timing
pub const SINGLE_TSBK_OSP_MS: f32 = 37.5;
pub const DOUBLE_TSBK_OSP_MS: f32 = 60.0;
pub const TRIPLE_TSBK_OSP_MS: f32 = 75.0;

/// ISP timing
pub const ISP_SLOT_MS: f32 = 37.5;          // Standard 5-microslot slot
pub const ISP_TRANSMIT_MS: f32 = 32.9;      // Actual transmit time
pub const ISP_GUARD_BITS: u32 = 44;         // For TONT + TOFFT + PD

/// Maximum TSBK throughput (triple mode, outbound only)
pub const MAX_TSBK_PER_SEC: f32 = 40.0;     // 3 / 0.075

/// Maximum MBT outbound latency
pub const MAX_MBT_LATENCY_MS: f32 = 135.0;  // 4-block MBT worst case
```

---

## 10. Parser Pseudocode for Extracting TSBKs from Raw FDMA Frames

### 10.1 Top-Level Control Channel Frame Parser

```
FUNCTION parse_control_channel_stream(raw_dibits: Stream<Dibit>) -> Stream<ControlMessage>:

    STATE: sync_state = SEARCHING

    LOOP:
        CASE sync_state:

        SEARCHING:
            // Slide through dibits looking for 48-bit Frame Sync
            window: u64 = 0
            FOR each dibit in raw_dibits:
                window = ((window << 2) | dibit) & 0xFFFFFFFFFFFF
                IF hamming_distance(window, 0x5575F5FF7FFF) <= 6:
                    sync_state = NID_DECODE
                    BREAK

        NID_DECODE:
            // Read 32 dibits (64 bits) for NID
            nid_bits = read_dibits(raw_dibits, 32)  // Note: may span SS boundaries
            nid_raw = assemble_64bit(nid_bits)
            (nac, duid) = bch_decode_nid(nid_raw)

            IF bch_decode_failed:
                sync_state = SEARCHING
                CONTINUE

            IF duid == 0x07:    // TSBK (single-block)
                sync_state = TSBK_DECODE
            ELIF duid == 0x0C:  // PDU (multi-block)
                sync_state = MBT_DECODE
            ELSE:
                sync_state = SEARCHING  // Not a control channel DUID
                CONTINUE

        TSBK_DECODE:
            // Decode TSBKs (1-3 per OSP)
            tsbk_count = 0
            LOOP:
                // Read 98 dibits (196 bits) -- one trellis-coded block
                // Must account for SS insertion every 35 info dibits
                coded_dibits = read_info_dibits(raw_dibits, 98)
                decoded_block = viterbi_decode_rate_half(coded_dibits)  // -> 12 octets

                IF NOT tsbk_crc_valid(decoded_block):
                    EMIT error("TSBK CRC failure")
                    sync_state = SEARCHING
                    BREAK

                tsbk = parse_tsbk(decoded_block)
                EMIT ControlMessage::Tsbk(nac, tsbk)
                tsbk_count += 1

                IF tsbk.last_block OR tsbk_count >= 3:
                    sync_state = SEARCHING  // Look for next FS
                    BREAK

        MBT_DECODE:
            // Read and decode header block
            coded_header = read_info_dibits(raw_dibits, 98)
            header_bytes = viterbi_decode_rate_half(coded_header)

            IF NOT header_crc_valid(header_bytes):
                EMIT error("MBT header CRC failure")
                sync_state = SEARCHING
                CONTINUE

            mbt_header = parse_mbt_header(header_bytes)
            blocks_to_follow = mbt_header.blocks_to_follow

            IF blocks_to_follow < 1 OR blocks_to_follow > 3:
                EMIT error("Invalid blocks_to_follow")
                sync_state = SEARCHING
                CONTINUE

            // Read data blocks
            data_blocks = []
            FOR i in 0..blocks_to_follow:
                coded_data = read_info_dibits(raw_dibits, 98)
                data_bytes = viterbi_decode_rate_half(coded_data)
                data_blocks.append(data_bytes)

            // Validate packet CRC-32 across data blocks
            IF NOT mbt_data_crc_valid(data_blocks):
                EMIT error("MBT data CRC failure")
                sync_state = SEARCHING
                CONTINUE

            EMIT ControlMessage::Mbt(nac, mbt_header, data_blocks)
            sync_state = SEARCHING

    RETURN
```

### 10.2 Status Symbol Aware Reading

```
FUNCTION read_info_dibits(stream: Stream<Dibit>, count: usize) -> Vec<Dibit>:
    // Read 'count' information dibits, skipping status symbols.
    // Status symbols occur every 35 info dibits (70 info bits).
    // In the raw stream, SS appears at position 35, 71, 107, ... (0-indexed).

    info = []
    raw_position = current_stream_position_within_microslot
    WHILE info.len() < count:
        dibit = stream.next()
        raw_position += 1
        IF raw_position % 36 == 0:
            // This is a status symbol; record but don't include in info
            record_status_symbol(dibit)
        ELSE:
            info.append(dibit)
    RETURN info
```

### 10.3 TSBK Extraction from Decoded Block

```rust
/// Parse a decoded 12-byte TSBK into structured fields.
pub fn parse_tsbk(block: &[u8; 12]) -> Tsbk {
    let octet0 = block[0];
    Tsbk {
        last_block: (octet0 >> 7) & 1 == 1,
        protected:  (octet0 >> 6) & 1 == 1,
        opcode:     octet0 & 0x3F,
        mfid:       block[1],
        arguments:  [block[2], block[3], block[4], block[5],
                     block[6], block[7], block[8], block[9]],
        llid:       ((block[7] as u32) << 16)
                  | ((block[8] as u32) << 8)
                  | (block[9] as u32),
        crc:        ((block[10] as u16) << 8) | block[11] as u16,
    }
}
```

### 10.4 MBT Header Extraction

```rust
/// Parse a decoded 12-byte MBT header block.
pub fn parse_mbt_header(block: &[u8; 12]) -> MbtHeader {
    let format = block[0] & 0x1F;
    let io_bit = (block[0] >> 5) & 1;
    let sap_id = block[1] & 0x3F;
    let mfid = block[2];
    let llid = ((block[3] as u32) << 16)
             | ((block[4] as u32) << 8)
             | (block[5] as u32);
    let blocks_to_follow = block[6] & 0x7F;

    let (opcode, msg_specific) = if format == MBT_FORMAT_ALTERNATIVE {
        (block[7] & 0x3F, [block[8], block[9]])
    } else {
        // Standard format: opcode is in the data blocks, not the header
        let pad_count = block[7] & 0x1F;
        let data_header_offset = block[9] & 0x3F;
        (0u8, [pad_count, data_header_offset])
    };

    MbtHeader {
        direction: if io_bit == 0 { Direction::Isp } else { Direction::Osp },
        format: MbtHeaderVariant::from_format(format).unwrap(),
        sap_id,
        mfid,
        llid,
        blocks_to_follow,
        opcode,
        msg_specific,
    }
}
```

---

## 11. Rust Struct Definitions

### 11.1 Core Types

```rust
use std::fmt;

/// Direction of control channel message
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Direction {
    Isp,  // Inbound: SU -> FNE
    Osp,  // Outbound: FNE -> SU
}

/// A single Trunking Signaling Block (12 octets decoded)
#[derive(Debug, Clone)]
pub struct Tsbk {
    /// Last Block flag: true = final TSBK in this OSP
    pub last_block: bool,
    /// Protected flag: true = encrypted content
    pub protected: bool,
    /// 6-bit opcode (message type per AABC-E)
    pub opcode: u8,
    /// Manufacturer's ID (0x00/0x01 = standard P25)
    pub mfid: u8,
    /// 8 octets of message-specific arguments (octets 2-9)
    pub arguments: [u8; 8],
    /// 24-bit Logical Link ID (extracted from arguments octets 5-7,
    /// i.e., block octets 7-9)
    pub llid: u32,
    /// CRC-16 as received (octets 10-11)
    pub crc: u16,
}

impl Tsbk {
    /// Raw 12-byte block for re-encoding or forwarding
    pub fn to_bytes(&self) -> [u8; 12] {
        let mut out = [0u8; 12];
        out[0] = ((self.last_block as u8) << 7)
               | ((self.protected as u8) << 6)
               | (self.opcode & 0x3F);
        out[1] = self.mfid;
        out[2..10].copy_from_slice(&self.arguments);
        out[10] = (self.crc >> 8) as u8;
        out[11] = self.crc as u8;
        out
    }

    /// Returns true if this is a standard P25 message (not manufacturer-specific)
    pub fn is_standard(&self) -> bool {
        self.mfid == 0x00 || self.mfid == 0x01
    }
}

/// MBT header block (decoded)
#[derive(Debug, Clone)]
pub struct MbtHeader {
    pub direction: Direction,
    pub format: MbtHeaderVariant,
    pub sap_id: u8,
    pub mfid: u8,
    pub llid: u32,
    pub blocks_to_follow: u8,
    /// 6-bit opcode (alternative format only; 0 for standard format)
    pub opcode: u8,
    /// Alternative format: [octet8, octet9] message-specific
    /// Standard format: [pad_count, data_header_offset]
    pub msg_specific: [u8; 2],
}

/// Complete multi-block trunking packet
#[derive(Debug, Clone)]
pub struct MbtPacket {
    pub header: MbtHeader,
    /// 1-3 data blocks, each 12 octets
    pub data_blocks: Vec<[u8; 12]>,
}

impl MbtPacket {
    /// Total message octets (excluding headers and CRCs)
    pub fn payload_octets(&self) -> usize {
        let block_count = self.data_blocks.len();
        if block_count == 0 { return 0; }
        // Last block has 4-octet CRC -> 8 payload octets
        // Other blocks have 12 payload octets each
        (block_count - 1) * 12 + 8
    }
}

/// MBT header variant
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MbtHeaderVariant {
    Standard,     // Format 0x15
    Alternative,  // Format 0x17
}

impl MbtHeaderVariant {
    pub fn from_format(format: u8) -> Option<Self> {
        match format & 0x1F {
            0x15 => Some(Self::Standard),
            0x17 => Some(Self::Alternative),
            _ => None,
        }
    }

    pub fn format_value(self) -> u8 {
        match self {
            Self::Standard => 0x15,
            Self::Alternative => 0x17,
        }
    }
}

/// Unified control channel message
#[derive(Debug, Clone)]
pub enum ControlMessage {
    /// Single-block TSBK message
    Tsbk {
        nac: u16,
        tsbk: Tsbk,
    },
    /// Multi-block trunking packet
    Mbt {
        nac: u16,
        packet: MbtPacket,
    },
}
```

### 11.2 OSP Burst (Outbound Signaling Packet)

```rust
/// An outbound signaling packet containing 1-3 TSBKs
#[derive(Debug, Clone)]
pub struct OspBurst {
    pub nac: u16,
    pub duid: u8,
    pub tsbks: Vec<Tsbk>,  // 1-3 TSBKs
}

impl OspBurst {
    pub fn format(&self) -> OspFormat {
        match self.tsbks.len() {
            1 => OspFormat::SingleTsbk,
            2 => OspFormat::DoubleTsbk,
            3 => OspFormat::TripleTsbk,
            _ => panic!("Invalid TSBK count"),
        }
    }

    pub fn duration_ms(&self) -> f32 {
        self.format().duration_ms()
    }

    pub fn microslots(&self) -> u32 {
        self.format().microslots()
    }
}
```

### 11.3 Status Symbol Tracking

```rust
/// Tracks outbound status symbols for inbound slot timing
pub struct SlotTimingTracker {
    /// Microslot counter within current outbound OSP
    pub microslot_count: u32,
    /// Last observed status symbol
    pub last_ss: OutboundStatusSymbol,
    /// True when inbound channel is available for transmission
    pub inbound_available: bool,
}

impl SlotTimingTracker {
    pub fn new() -> Self {
        Self {
            microslot_count: 0,
            last_ss: OutboundStatusSymbol::Unknown00,
            inbound_available: false,
        }
    }

    /// Process a status symbol from the outbound stream
    pub fn update(&mut self, ss: OutboundStatusSymbol) {
        self.microslot_count += 1;
        self.last_ss = ss;
        match ss {
            OutboundStatusSymbol::Idle => {
                // Slot boundary -- inbound transmission may begin
                self.inbound_available = true;
                self.microslot_count = 0;
            }
            OutboundStatusSymbol::Busy => {
                // Inbound channel not available
                self.inbound_available = false;
            }
            OutboundStatusSymbol::IntraSlot => {
                // Intra-slot boundary -- do not transmit
            }
            OutboundStatusSymbol::Unknown00 => {}
        }
    }
}
```

### 11.4 Control Channel State Machine

```rust
/// Control channel receiver state machine
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ControlChannelState {
    /// Scanning for Frame Sync pattern
    Searching,
    /// Frame Sync found, decoding NID
    NidDecode,
    /// Decoding TSBK block(s)
    TsbkDecode { blocks_decoded: u8 },
    /// Decoding MBT header
    MbtHeaderDecode,
    /// Decoding MBT data blocks
    MbtDataDecode { blocks_remaining: u8 },
}

/// Top-level control channel receiver
pub struct ControlChannelReceiver {
    pub state: ControlChannelState,
    pub nac: u16,
    pub slot_tracker: SlotTimingTracker,
    /// Frame sync correlation window (48 bits)
    sync_window: u64,
    /// Accumulated status symbols for inbound timing
    status_symbols: Vec<OutboundStatusSymbol>,
}

impl ControlChannelReceiver {
    pub fn new(expected_nac: u16) -> Self {
        Self {
            state: ControlChannelState::Searching,
            nac: expected_nac,
            slot_tracker: SlotTimingTracker::new(),
            sync_window: 0,
            status_symbols: Vec::new(),
        }
    }
}
```

---

## 12. Cross-Reference: FDMA Control Channel vs. TDMA Control Channel

### 12.1 Architectural Mapping

| FDMA (Phase 1) | TDMA (Phase 2) | Standard |
|----------------|----------------|----------|
| TSDU (DUID 0x07) | LCCH (Logical Control Channel) | AABB-B / BBAC-A |
| TSBK (12 octets) | MAC PDU (variable) | AABB-B / BBAD-A |
| OSP (outbound signaling packet) | OECI (Outbound Explicit Channel Info) burst | AABB-B / BBAC-A |
| ISP (inbound signaling packet) | IECI (Inbound w/ Sync) burst | AABB-B / BBAC-A |
| LB/P/Opcode header (8 bits) | B1/B2/MCO header (8 bits) | AABB-B / BBAD-A |
| MFID (octet 1) | MFID in MAC PDU | AABB-B / BBAD-A |
| Status Symbols (ALOHA) | SACCH/FACCH slot access | AABB-B / BBAC-A |
| Rate 1/2 trellis | TDMA-specific FEC | BAAA-B / BBAB |
| 9,600 bps on 12.5 kHz | 12,000 bps on 12.5 kHz (2 slots) | BAAA-B / BBAB |

### 12.2 TSBK Opcode to MAC MCO Mapping

TDMA MAC messages use a partition system (B1/B2 qualifier bits). When B1=0, B2=1
(Partition 0b01), the 6-bit MCO value equals the FDMA TSBK opcode directly. This
means the same opcode value (e.g., 0x00 for Group Voice Channel Grant) maps to the
same message in both FDMA and TDMA, just in different framing.

See the AABC-E Implementation Spec Section 9 for the complete cross-reference table.

### 12.3 SDRTrunk Code Mapping

| Component | FDMA (AABB-B) | TDMA (BBAC-A / BBAD-A) |
|-----------|---------------|------------------------|
| Frame sync | `P25P1FrameSync.java` | `P25P2SyncPattern.java` |
| NID decode | `NID.java` | N/A (TDMA uses SACCH) |
| TSBK parse | `TSBKMessage.java` | `MacMessage.java` |
| Trellis decode | `TrellisCodec.java` | `AMBE_HALF_RATE.java` |
| Control channel | `P25P1DecoderState.java` | `P25P2DecoderState.java` |
| TSBK dispatch | `TSBKMessageFactory.java` | `MacMessageFactory.java` |

### 12.4 OP25 Code Mapping

| Component | FDMA (AABB-B) | TDMA |
|-----------|---------------|------|
| Frame processing | `p25p1_fdma.cc` | `p25p2_tdma.cc` |
| TSBK decode | `rx_tsbk.cc` | `rx_mac.cc` |
| Trellis decode | `rs.cc` / inline | Inline |
| CRC validation | `crc16()` in `p25_crypt.cc` | Per-burst CRC |
| Frequency resolve | `iden_map[]` | Same identifier table |

---

## 13. Implementation Notes

### 13.1 Common Pitfalls

1. **DUID ambiguity:** AABB-B specifies DUID `$7` for TSBK but BAAA-B does not formally
   define 0x7 as a DUID. Implementations must handle both 0x07 (TSBK) and 0x0C (PDU)
   when the first block could be either a TSBK or an MBT header. Distinguish them by
   examining octet 0: TSBK has LB/P/Opcode; MBT header has 0/AN/IO/Format.

2. **Status symbol timing:** SS insertion/stripping must account for the position within
   the overall outbound stream, not just within a single OSP. The SS cadence is tied to
   the microslot grid which is continuous across OSPs.

3. **Protected field boundaries:** When P=1, only bits 7-6 of octet 0 and octet 1 (MFID)
   are readable. The opcode itself (bits 5:0 of octet 0) IS encrypted. Implementations
   that attempt to dispatch on opcode before decryption will get garbage values.

4. **Multi-block CRC scope:** The header block has its own CRC-16 (octets 10-11). The
   data blocks share a single 32-bit CRC in the last 4 octets of the last data block.
   These are independent CRCs -- the header CRC does not cover data blocks.

5. **Triple TSBK detection:** An SU must check the LB bit of EACH TSBK sequentially.
   There is no header field indicating the total count upfront. If a CRC failure occurs
   on any TSBK, the remainder of that OSP should be discarded.

### 13.2 Recommended Test Vectors

Implementations should validate against:
- Known TSBK CRC-16 values from captured control channel traffic
- SDRTrunk test fixtures in `src/test/java/.../p25/phase1/message/tsbk/`
- OP25 sample captures in `op25/gr-op25_repeater/data/`

### 13.3 Performance Considerations

For real-time control channel monitoring:
- Frame sync correlation: O(1) per dibit using sliding window XOR + popcount
- Trellis decode: 98 dibits x 4 states = ~400 operations per TSBK (Viterbi)
- CRC-16: 10 bytes x 8 iterations = 80 operations per TSBK
- Total per TSBK: sub-microsecond on modern hardware
- At 40 TSBKs/sec max throughput, CPU load is negligible
