# P25 Phase 2 TDMA MAC Layer Messages — Implementation Specification

**Source:** TIA-102.BBAD-A (November 2019), "Project 25 TDMA Medium Access Control Layer Messages"  
**Extracted:** 2026-04-12 from PDF with structural verification  
**Purpose:** Self-contained spec for implementing a complete TDMA MAC message parser.
No reference to the original PDF required.

---

## 1. PDU Structure Reference

### 1.1 MAC PDU Header

Every MAC PDU begins with a single-octet header:

```
Bit:    7     6     5     4     3     2     1     0
      +-----+-----+-----+-----+-----+-----+-----+-----+
      | Opcode[2:0]       | Offset[2:0]       | Rsv | P |
      +-----+-----+-----+-----+-----+-----+-----+-----+

  Opcode  = bits [7:5] — MAC PDU type
  Offset  = bits [4:2] — message offset indicator (slot count)
  Reserved = bit [1]
  Protected (P) = bit [0] — 1 = payload is encrypted
```

### 1.2 Opcode Values (Table 9)

| Opcode (binary) | Opcode (hex) | Name          | Description                              |
|------------------|-------------|---------------|------------------------------------------|
| `%000`           | `0x00`      | MAC_SIGNAL    | Signalling PDU — carries MAC messages    |
| `%001`           | `0x01`      | MAC_PTT       | Push-to-talk — voice channel grant start |
| `%010`           | `0x02`      | MAC_END_PTT   | End push-to-talk — voice release         |
| `%011`           | `0x03`      | MAC_IDLE      | Idle — no traffic, carries MAC messages  |
| `%100`           | `0x04`      | MAC_ACTIVE    | Active — voice active, carries messages  |
| `%110`           | `0x06`      | MAC_HANGTIME  | Hang time — post-voice, carries messages |

**Note:** Opcode `%101` and `%111` are reserved/undefined.

MAC_SIGNAL, MAC_IDLE, MAC_ACTIVE, and MAC_HANGTIME all carry MAC signalling messages
in their payload. MAC_PTT and MAC_END_PTT carry fixed-format voice channel control data.

### 1.3 PDU Types and Sizes

| PDU Acronym | Full Name                          | Direction | Channel | Total Size   | Payload Octets | Trailer                        |
|-------------|------------------------------------|-----------|---------|--------------|----------------|--------------------------------|
| OECI        | Outbound Explicit Channel Intf.    | Outbound  | LCCH    | 22.5 octets  | Octets 1-18   | 12-bit ColorCode + 16-bit CRC-16 |
| IECI        | Inbound Explicit Channel Intf.     | Inbound   | LCCH    | 17 octets    | Octets 1-14   | 16-bit CRC-16                  |
| I-OEMI      | Interstitial Outbound EMI          | Outbound  | VCH     | 22.5 octets  | Octets 1-20   | 12-bit CRC-12                  |
| S-OEMI      | Superframe Outbound EMI            | Outbound  | VCH     | 19.5 octets  | Octets 1-17   | 12-bit CRC-12                  |
| IEMI        | Inbound EMI                        | Inbound   | VCH     | 19.5 octets  | Octets 1-17   | 12-bit CRC-12                  |

Payload capacity summary for message parsing:
- **OECI:** 18 octets of MAC messages (outbound LCCH)
- **IECI:** 14 octets of MAC messages (inbound LCCH)
- **I-OEMI:** 20 octets of MAC messages (outbound VCH, no sync)
- **S-OEMI:** 17 octets of MAC messages (outbound VCH, with sync)
- **IEMI:** 17 octets of MAC messages (inbound VCH, with sync)

---

## 2. MCO Partitioning

The first octet of every MAC message (after the PDU header) contains the **Message Control Octet (MCO) qualifier** bits B1 and B2, plus the 6-bit MCO value:

```
Bit:    7     6     5     4     3     2     1     0
      +-----+-----+-----+-----+-----+-----+-----+-----+
      | B1  | B2  | MCO[5:0]                            |
      +-----+-----+-----+-----+-----+-----+-----+-----+
```

The B1/B2 partition determines the message namespace:

| B1 | B2 | Partition                                                        |
|----|----|------------------------------------------------------------------|
| 0  | 0  | Unique TDMA CAI message (no FDMA equivalent)                     |
| 0  | 1  | Derived from FDMA CAI OSP/ISP abbreviated format                 |
| 1  | 0  | Manufacturer specific message (MFID in 2nd octet)               |
| 1  | 1  | Derived from FDMA CAI OSP/ISP extended or explicit format        |

**Direction disambiguation:** For B1=0/B2=1 messages, the same MCO value may map to
different messages depending on direction. Use the burst type to determine direction:
- IECI or IEMI burst = **inbound (ISP)**
- OECI, I-OEMI, or S-OEMI burst = **outbound (OSP)**

---

## 3. Complete Opcode Dispatch Table

### 3.1 Partition 0b00 — Unique TDMA Messages

These messages exist only in the TDMA CAI and have no FDMA equivalent.

```rust
// B1=0, B2=0 — Unique TDMA messages
// (mco, length, name)
// Length 0 = Variable
const TDMA_UNIQUE: &[(u8, u8, &str)] = &[
    (0b000000,  0, "Null Information"),                              // 7.1
    (0b000001,  7, "Group Voice Channel User - Abbreviated"),        // 7.2
    (0b100001, 14, "Group Voice Channel User - Extended"),           // 7.3
    (0b000010,  8, "Unit to Unit Voice Channel User - Abbreviated"), // 7.4
    (0b100010, 15, "Unit to Unit Voice Channel User - Extended"),    // 7.5
    (0b000011,  7, "Telephone Interconnect Voice Channel User"),     // 7.6
    (0b000101, 16, "Group Voice Channel Grant Update Multiple - Implicit"),  // 7.7
    (0b100101, 15, "Group Voice Channel Grant Update Multiple - Explicit"),  // 7.8
    (0b001000,  0, "Null Avoid Zero Bias Information"),              // 7.9
    (0b010000,  0, "Continuation Fragment"),                         // 7.10
    (0b010001,  0, "Indirect Group Paging without Priority"),        // 7.11
    (0b010010,  0, "Individual Paging with Priority"),               // 7.12
    (0b110000,  5, "Power Control Signal Quality"),                  // 7.13
    (0b110001,  7, "MAC_Release"),                                   // 7.14
];
```

### 3.2 Partition 0b01 — Abbreviated (FDMA-Derived) OSP Messages

Outbound Signalling Packets (direction = outbound).

```rust
// B1=0, B2=1, Direction=Outbound (OSP)
// (mco, length, name)
const ABBREVIATED_OSP: &[(u8, u8, &str)] = &[
    (0b000000,  9, "Group Voice Channel Grant - Implicit"),
    (0b000010,  9, "Group Voice Channel Grant Update - Implicit"),
    (0b000100,  9, "Unit to Unit Voice Service Channel Grant - Abbreviated"),
    (0b000101,  8, "Unit to Unit Answer Request - Abbreviated"),
    (0b000110,  9, "Unit to Unit Voice Channel Grant Update - Abbreviated"),
    (0b001000, 10, "Telephone Interconnect Voice Channel Grant - Implicit"),
    (0b001001, 10, "Telephone Interconnect Voice Channel Grant Update - Implicit"),
    (0b001010,  9, "Telephone Interconnect Answer Request"),
    (0b001100, 10, "Radio Unit Monitor Command - Abbreviated"),
    (0b010100,  9, "SNDCP Data Channel Grant"),
    (0b010101,  7, "SNDCP Data Page Request"),
    (0b011000, 10, "Status Update - Abbreviated"),
    (0b011010,  7, "Status Query - Abbreviated"),
    (0b011100, 10, "Message Update - Abbreviated"),
    (0b011101,  8, "Radio Unit Monitor Command - Obsolete"),
    (0b011110, 14, "Radio Unit Monitor Enhanced Command - Abbreviated"),
    (0b011111,  7, "Call Alert - Abbreviated"),
    (0b100000,  9, "Acknowledge Response FNE - Abbreviated"),
    (0b100001,  9, "Queued Response"),
    (0b100100,  9, "Extended Function Command - Abbreviated"),
    (0b100111,  9, "Deny Response"),
    (0b101000, 10, "Group Affiliation Response - Abbreviated"),
    (0b101001,  9, "Secondary Control Channel Broadcast - Implicit"),
    (0b101010,  7, "Group Affiliation Query - Abbreviated"),
    (0b101011, 10, "Location Registration Response"),
    (0b101100, 10, "Unit Registration Response - Abbreviated"),
    (0b101101,  7, "Unit Registration Command"),
    (0b101111,  9, "Unit Deregistration Acknowledge"),
    (0b110000,  9, "Synchronization Broadcast"),
    (0b110001, 29, "Authentication Demand"),
    (0b110010,  9, "Authentication FNE Response - Abbreviated"),
    (0b110011,  9, "Identifier Update for TDMA - Abbreviated"),
    (0b110100,  9, "Identifier Update for VHF/UHF Bands"),
    (0b110101,  9, "Time and Date Announcement"),
    (0b110110, 10, "Roaming Address Command"),
    (0b110111, 13, "Roaming Address Update"),
    (0b111000,  9, "System Service Broadcast"),
    (0b111001,  9, "Secondary Control Channel Broadcast - Implicit"),
    (0b111010,  9, "RFSS Status Broadcast - Implicit"),
    (0b111011, 11, "Network Status Broadcast - Implicit"),
    (0b111100,  9, "Adjacent Status Broadcast - Implicit"),
    (0b111101,  9, "Identifier Update"),
];
```

### 3.3 Partition 0b01 — Abbreviated (FDMA-Derived) ISP Messages

Inbound Signalling Packets (direction = inbound).

```rust
// B1=0, B2=1, Direction=Inbound (ISP)
// (mco, length, name)
const ABBREVIATED_ISP: &[(u8, u8, &str)] = &[
    (0b000001,  7, "Group Voice Service Request"),
    (0b000100,  8, "Unit to Unit Voice Service Request - Abbreviated"),
    (0b000101, 10, "Unit to Unit Voice Service Answer Response - Abbreviated"),
    (0b001000,  0, "Telephone Interconnect Request - Explicit Dialing"),  // Variable
    (0b001001,  7, "Telephone Interconnect Request - Implicit Dialing"),
    (0b001010,  7, "Telephone Interconnect Answer Response"),
    (0b010010,  8, "SNDCP Data Channel Request"),
    (0b010011,  9, "SNDCP Data Page Response"),
    (0b010100,  9, "SNDCP Reconnect Request"),
    (0b011000, 10, "Status Update - Abbreviated"),
    (0b011001, 10, "Status Query Response - Abbreviated"),
    (0b011010,  8, "Status Query Request - Abbreviated"),
    (0b011100, 10, "Message Update - Abbreviated"),
    (0b011101, 10, "Radio Unit Monitor Request - Abbreviated"),
    (0b011110, 20, "Radio Unit Monitor Enhanced Command - Abbreviated"),
    (0b011111,  8, "Call Alert Request - Abbreviated"),
    (0b100000,  9, "Acknowledge Response Unit - Abbreviated"),
    (0b100011, 10, "Cancel Service Request - Abbreviated"),
    (0b100100,  9, "Extended Function Response - Abbreviated"),
    (0b100111,  9, "Emergency Alarm Request"),
    (0b101000,  9, "Group Affiliation Request - Abbreviated"),
    (0b101010, 10, "Group Affiliation Query Response"),
    (0b101100, 10, "Unit Registration Request"),
    (0b101101,  9, "Location Registration Request - Abbreviated"),
    (0b101111,  9, "Unit Deregistration Request"),
    (0b110010,  6, "Identifier Update Request"),
    (0b110110,  8, "Roaming Address Request - Abbreviated"),
    (0b110111,  0, "Roaming Address Response"),  // Variable
    (0b111000, 10, "Authentication Response - Abbreviated"),
    (0b111001, 27, "Authentication Response Mutual"),
    (0b111010,  6, "Authentication FNE Result - Abbreviated"),
    (0b111011,  5, "Authentication SU Demand - Abbreviated"),
];
```

### 3.4 Partition 0b10 — Manufacturer Specific (MFID=0x90)

Manufacturer-specific messages. The second octet contains the MFID (Manufacturer ID).
Known messages use MFID=0x90 (Motorola) and MFID=0xA4 (Harris).

```rust
// B1=1, B2=0 — Manufacturer Specific
// (mco, length, mfid, name)
// Length 0 = Variable
const MANUFACTURER_SPECIFIC: &[(u8, u8, u8, &str)] = &[
    (0b000000,  8, 0x90, "MFID90 Group Regroup Voice Channel User - Abbreviated"),
    (0b100000, 16, 0x90, "MFID90 Group Regroup Voice Channel User - Extended"),
    (0b000001,  0, 0x90, "MFID90 Group Regroup Add Command"),           // Variable
    (0b000011,  7, 0x90, "MFID90 Group Regroup Voice Channel Update"),
    (0b000100, 11, 0x90, "MFID90 Extended Function Command"),
    (0b001001,  0, 0x90, "MFID90 Group Regroup Delete Command"),        // Variable
    (0b100001,  9, 0x90, "MFID90 Group Regroup Voice Request"),
    (0b100010, 11, 0x90, "MFID90 Extended Function Response"),
    (0b100011, 11, 0x90, "MFID90 Group Regroup Channel Grant - Implicit"),
    (0b100100, 13, 0x90, "MFID90 Group Regroup Channel Grant - Explicit"),
    (0b100101, 11, 0x90, "MFID90 Group Regroup Channel Update"),
    (0b100110, 11, 0x90, "MFID90 Queued Response"),
    (0b100111, 11, 0x90, "MFID90 Deny Response"),
    (0b101000, 10, 0x90, "MFID90 Acknowledge Response"),
    (0b110000,  0, 0xA4, "MFIDA4 Group Regroup Explicit Encryption Command"),  // Variable
];

// Any undefined MCO with B1=1, B2=0 is treated as:
// Length = Variable (determined by length octet in 2nd byte after MFID)
// Name = "Manufacturer Specific"
```

### 3.5 Partition 0b11 — Extended/Explicit (FDMA-Derived) OSP Messages

```rust
// B1=1, B2=1, Direction=Outbound (OSP)
// (mco, length, name)
const EXTENDED_OSP: &[(u8, u8, &str)] = &[
    (0b000000, 11, "Group Voice Channel Grant - Explicit"),
    (0b000011,  8, "Group Voice Channel Grant Update - Explicit"),
    (0b000100, 15, "Unit to Unit Voice Service Channel Grant - Extended VCH"),
    (0b000101, 12, "Unit to Unit Answer Request - Extended"),
    (0b000110, 15, "Unit to Unit Voice Channel Grant Update - Extended VCH"),
    (0b000111, 32, "Unit to Unit Voice Channel Grant Update - Extended LCCH"),
    (0b001000, 12, "Telephone Interconnect Voice Channel Grant - Explicit"),
    (0b001001, 12, "Telephone Interconnect Voice Channel Grant Update - Explicit"),
    (0b001011, 27, "Call Alert - Extended LCCH"),
    (0b001101, 29, "Radio Unit Monitor Command - Extended LCCH"),
    (0b001110, 29, "Message Update - Extended LCCH"),
    (0b001111, 32, "Unit to Unit Voice Service Channel Grant - Extended LCCH"),
    (0b010110,  9, "SNDCP Data Channel Announcement - Explicit"),
    (0b011000, 14, "Status Update - Extended VCH"),
    (0b011001, 29, "Status Update - Extended LCCH"),
    (0b011010, 11, "Status Query - Extended VCH"),
    (0b011011, 27, "Status Query - Extended LCCH"),
    (0b011100, 14, "Message Update - Extended VCH"),
    (0b011110, 40, "Radio Unit Monitor Enhanced Command - Extended"),
    (0b011111, 11, "Call Alert - Extended VCH"),
    (0b100000, 28, "Acknowledge Response FNE - Extended"),
    (0b100100, 17, "Extended Function Command - Extended VCH"),
    (0b100101, 14, "Extended Function Command - Extended LCCH"),
    (0b101000, 16, "Group Affiliation Response - Extended"),
    (0b101001,  8, "Secondary Control Channel Broadcast - Explicit"),
    (0b101010, 11, "Group Affiliation Query - Extended"),
    (0b101100, 13, "Unit Registration Response - Extended"),
    (0b110010, 16, "Authentication FNE Response - Extended"),
    (0b110011, 14, "Identifier Update for TDMA - Extended"),
    (0b111010, 11, "RFSS Status Broadcast - Explicit"),
    (0b111011, 13, "Network Status Broadcast - Explicit"),
    (0b111100, 11, "Adjacent Status Broadcast - Explicit"),
    (0b111110, 15, "Adjacent Status Broadcast - Extended - Explicit"),
];
```

### 3.6 Partition 0b11 — Extended/Explicit (FDMA-Derived) ISP Messages

```rust
// B1=1, B2=1, Direction=Inbound (ISP)
// (mco, length, name)
const EXTENDED_ISP: &[(u8, u8, &str)] = &[
    (0b000100, 12, "Unit to Unit Voice Service Request - Extended"),
    (0b000101, 14, "Unit to Unit Answer Response - Extended"),
    (0b011000, 14, "Status Update - Extended VCH"),
    (0b011001, 14, "Status Query Response - Extended"),
    (0b011010, 12, "Status Query Request - Extended"),
    (0b011100, 14, "Message Update - Extended VCH"),
    (0b011101, 14, "Radio Unit Monitor Request - Extended"),
    (0b011110, 28, "Radio Unit Monitor Enhanced Command - Extended"),
    (0b011111, 12, "Call Alert Request - Extended VCH"),
    (0b100000, 13, "Acknowledge Response Unit - Extended"),
    (0b100011, 14, "Cancel Service Request - Extended"),
    (0b100100, 14, "Extended Function Response - Extended VCH"),
    (0b101000, 12, "Group Affiliation Request - Extended"),
    (0b101101, 19, "Location Registration Request - Extended"),
    (0b110010, 13, "Authentication FNE Result - Extended"),
    (0b110110, 12, "Roaming Address Request - Extended"),
    (0b111000, 22, "Authentication Response - Extended"),
    (0b111011, 12, "Authentication SU Demand - Extended"),
];
```

### 3.7 Unified Dispatch Key

For code implementation, the full dispatch key is a 4-tuple:

```rust
struct MacMessageKey {
    b1: u8,         // 1 bit
    b2: u8,         // 1 bit
    mco: u8,        // 6 bits
    direction: Direction,  // Inbound or Outbound
}

enum Direction {
    Outbound,  // OECI, I-OEMI, S-OEMI bursts
    Inbound,   // IECI, IEMI bursts
}

// Extract from first message octet:
fn decode_mco_byte(byte: u8) -> (u8, u8, u8) {
    let b1  = (byte >> 7) & 0x01;
    let b2  = (byte >> 6) & 0x01;
    let mco = byte & 0x3F;
    (b1, b2, mco)
}
```

---

## 4. Fixed-Content PDU Formats

### 4.1 MAC_PTT PDU

Used on voice channel to indicate push-to-talk. Contains encryption parameters and
source/group identification.

**IEMI and S-OEMI format (19.5 octets total, 17 octets payload):**

```
Octet 0:      PDU Header (Opcode=%001, Offset, Rsv, P)
Octets 1-9:   MI[71:0]            — 72 bits (9 octets) Message Indicator
Octet 10:     ALGID[7:0]          — 8 bits Algorithm ID
Octets 11-12: KeyID[15:0]         — 16 bits Key ID
Octets 13-15: SourceAddr[23:0]    — 24 bits Source Address
Octets 16-17: GroupAddr[15:0]     — 16 bits Group Address
Bits 144-155: CRC-12              — 12 bits (1.5 octets)
```

**I-OEMI format (22.5 octets total, 20 octets payload):**

```
Octet 0:      PDU Header (Opcode=%001, Offset, Rsv, P)
Octets 1-9:   MI[71:0]            — 72 bits Message Indicator
Octet 10:     ALGID[7:0]          — 8 bits Algorithm ID
Octets 11-12: KeyID[15:0]         — 16 bits Key ID
Octets 13-15: SourceAddr[23:0]    — 24 bits Source Address
Octets 16-17: GroupAddr[15:0]     — 16 bits Group Address
Octets 18-20: Reserved[23:0]      — 24 bits (set to 0)
Bits 168-179: CRC-12              — 12 bits (1.5 octets)
```

### 4.2 MAC_END_PTT PDU

Used to signal end of voice transmission.

**IEMI and S-OEMI format (19.5 octets total, 17 octets payload):**

```
Octet 0:      PDU Header (Opcode=%010, Offset, Rsv, P)
Bits 8-11:    Reserved[3:0]       — 4 bits
Bits 12-23:   ColorCode[11:0]     — 12 bits
Bits 24-103:  Reserved[79:0]      — 80 bits (10 octets)
Octets 13-15: SourceAddr[23:0]    — 24 bits Source Address
Octets 16-17: GroupAddr[15:0]     — 16 bits Group Address
Bits 144-155: CRC-12              — 12 bits
```

**I-OEMI format (22.5 octets total, 20 octets payload):**

```
Octet 0:      PDU Header (Opcode=%010, Offset, Rsv, P)
Bits 8-11:    Reserved[3:0]       — 4 bits
Bits 12-23:   ColorCode[11:0]     — 12 bits
Bits 24-103:  Reserved[79:0]      — 80 bits
Octets 13-15: SourceAddr[23:0]    — 24 bits Source Address
Octets 16-17: GroupAddr[15:0]     — 16 bits Group Address
Octets 18-20: Reserved[23:0]      — 24 bits
Bits 168-179: CRC-12              — 12 bits
```

---

## 5. Message Length Determination Logic

Four methods are used to determine MAC message length. They must be applied in priority order:

### Method 1: Fixed Length from Dispatch Table

Most messages have a fixed length specified in the dispatch tables (Section 3). If the
table entry has a non-zero length, use it directly.

### Method 2: Variable via Field Parsing

These messages contain internal structure that determines their length:

- **Individual Paging with Priority (B1=0, B2=0, MCO=%010010):** Length depends on
  the number of paged targets. Parse the count field to determine total octets.
- **Indirect Group Paging without Priority (B1=0, B2=0, MCO=%010001):** Similar —
  variable number of group addresses.
- **Telephone Interconnect Request - Explicit Dialing (B1=0, B2=1, MCO=%001000, ISP):**
  Contains variable-length dialed digit string.

### Method 3: Fill Remaining PDU Space

These messages expand to fill whatever space remains in the PDU after all preceding
messages have been parsed:

- **Null Information (B1=0, B2=0, MCO=%000000):** Padding to fill remaining payload.
  Content is arbitrary (typically zeroes).
- **Null Avoid Zero Bias Information (B1=0, B2=0, MCO=%001000):** Padding to fill
  remaining payload. Each octet is `0x88` (avoid zero bias pattern).

These messages are always the **last** message in a PDU. When encountered, consume all
remaining payload bytes.

### Method 4: Length Octet (Post-Publication Messages)

Messages added after the initial BBAD publication encode their length in the **second
octet** of the message. The length value is in the low 6 bits (bits [5:0]) of that octet.

- **Continuation Fragment (B1=0, B2=0, MCO=%010000):** Second octet bits [5:0] = length.

```rust
fn determine_message_length(
    b1: u8, b2: u8, mco: u8, direction: Direction,
    raw: &[u8], offset: usize, pdu_payload_len: usize
) -> usize {
    // Method 3: Fill-type messages
    if b1 == 0 && b2 == 0 && (mco == 0b000000 || mco == 0b001000) {
        return pdu_payload_len - offset;  // consume remainder
    }

    // Method 4: Length-octet messages
    if b1 == 0 && b2 == 0 && mco == 0b010000 {
        return (raw[offset + 1] & 0x3F) as usize;
    }

    // Method 2: Variable-length messages (parse internal fields)
    if b1 == 0 && b2 == 0 && (mco == 0b010001 || mco == 0b010010) {
        return parse_variable_paging_length(raw, offset);
    }
    if b1 == 0 && b2 == 1 && mco == 0b001000 && direction == Direction::Inbound {
        return parse_explicit_dialing_length(raw, offset);
    }

    // Method 1: Fixed length from dispatch table
    return lookup_fixed_length(b1, b2, mco, direction);
}
```

---

## 6. CRC Specifications

### 6.1 CRC-12 (VCH PDUs: I-OEMI, S-OEMI, IEMI)

Used to protect voice channel MAC PDUs.

```
Generator polynomial:
  g(x) = x^12 + x^11 + x^7 + x^4 + x^2 + x + 1

Binary:   1_1000_1001_0111
Hex:      0x1897
Bit mask: 0x897 (12-bit remainder)
```

**Computation:**
1. Initialize the CRC register to all zeros: `crc = 0x000`
2. Process all PDU bits preceding the CRC field, MSB first
3. For each bit: if MSB of register is 1, shift left and XOR with `0x897`; else shift left
4. **Invert** the final result: `crc = crc ^ 0xFFF`

```rust
fn crc12(data: &[u8], num_bits: usize) -> u16 {
    let poly: u16 = 0x0897;
    let mut crc: u16 = 0x0000;

    for i in 0..num_bits {
        let byte_idx = i / 8;
        let bit_idx = 7 - (i % 8);
        let bit = ((data[byte_idx] >> bit_idx) & 1) as u16;

        let feedback = (crc >> 11) & 1;
        crc = (crc << 1) & 0x0FFF;
        crc |= bit;  // shift data bit in

        // Actually: standard CRC division approach
        // Re-derive properly:
        if feedback ^ bit == 1 {
            // This needs care. Use the standard approach:
        }
    }

    // Cleaner implementation using standard bit-at-a-time CRC:
    crc = 0x000;
    for i in 0..num_bits {
        let byte_idx = i / 8;
        let bit_idx = 7 - (i % 8);
        let data_bit = ((data[byte_idx] >> bit_idx) & 1) as u16;

        let msb = (crc >> 11) & 1;
        crc = ((crc << 1) | data_bit) & 0x0FFF;
        if msb == 1 {
            crc ^= poly;
        }
    }

    crc ^ 0x0FFF  // Invert result
}
```

**Practical implementation (byte-at-a-time with standard CRC division):**

```rust
fn crc12_compute(data_bits: &[u8]) -> u16 {
    // data_bits: array of 0/1 values, MSB first
    let poly: u16 = 0x0897;
    let mut remainder: u16 = 0x0000;

    for &bit in data_bits {
        let feedback = ((remainder >> 11) ^ (bit as u16)) & 1;
        remainder = (remainder << 1) & 0x0FFF;
        if feedback == 1 {
            remainder ^= poly;
        }
    }

    remainder ^ 0x0FFF  // INVERT the final CRC
}
```

### 6.2 CRC-16 (LCCH PDUs: OECI, IECI)

Used to protect logical control channel MAC PDUs.

```
Generator polynomial:
  G(x) = x^16 + x^12 + x^5 + 1

Binary:   1_0001_0000_0010_0001
Hex:      0x11021
Bit mask: 0x1021 (CRC-CCITT)

Inversion polynomial:
  I(x) = x^15 + x^14 + x^13 + ... + x + 1
  I = 0xFFFF (all ones, 16 bits)
```

**Computation:**
1. Form the message polynomial M(x) from all PDU bits before the CRC field
2. Compute `R(x) = (x^16 * M(x)) mod G(x)` — standard CRC-CCITT division
3. Apply inversion: `F(x) = (R(x) + I(x)) mod 2` — i.e., XOR with `0xFFFF`

```rust
fn crc16_ccitt(data_bits: &[u8]) -> u16 {
    let poly: u16 = 0x1021;
    let mut crc: u16 = 0x0000;  // initial value = 0

    for &bit in data_bits {
        let feedback = ((crc >> 15) ^ (bit as u16)) & 1;
        crc = (crc << 1) & 0xFFFF;
        if feedback == 1 {
            crc ^= poly;
        }
    }

    crc ^ 0xFFFF  // Apply inversion polynomial
}
```

### 6.3 CRC Validation

```rust
fn validate_pdu_crc(burst_type: BurstType, raw_bytes: &[u8]) -> bool {
    match burst_type {
        // VCH PDUs use CRC-12
        BurstType::IOEMI | BurstType::SOEMI | BurstType::IEMI => {
            let total_bits = raw_bytes.len() * 8;  // includes half-octet
            let crc_bits = 12;
            let data_bits = total_bits - crc_bits;
            let computed = crc12_compute(&to_bit_array(raw_bytes)[..data_bits]);
            let received = extract_bits(raw_bytes, data_bits, 12) as u16;
            computed == received
        }
        // LCCH PDUs use CRC-16
        BurstType::OECI => {
            // OECI: 22.5 octets = 180 bits
            // Trailer: 12-bit ColorCode + 16-bit CRC-16
            // CRC covers: octets 0 through end of ColorCode
            let data_bits = 180 - 16;  // everything except CRC-16
            let computed = crc16_ccitt(&to_bit_array(raw_bytes)[..data_bits]);
            let received = extract_bits(raw_bytes, data_bits, 16) as u16;
            computed == received
        }
        BurstType::IECI => {
            // IECI: 17 octets = 136 bits
            // Trailer: 16-bit CRC-16
            let data_bits = 136 - 16;
            let computed = crc16_ccitt(&to_bit_array(raw_bytes)[..data_bits]);
            let received = extract_bits(raw_bytes, data_bits, 16) as u16;
            computed == received
        }
    }
}
```

---

## 7. Parser Pseudocode

### 7.1 Top-Level PDU Dispatcher

```rust
enum BurstType { OECI, IECI, IOEMI, SOEMI, IEMI }

fn direction_from_burst(burst_type: BurstType) -> Direction {
    match burst_type {
        BurstType::OECI | BurstType::IOEMI | BurstType::SOEMI => Direction::Outbound,
        BurstType::IECI | BurstType::IEMI => Direction::Inbound,
    }
}

fn payload_length(burst_type: BurstType) -> usize {
    match burst_type {
        BurstType::OECI  => 18,
        BurstType::IECI  => 14,
        BurstType::IOEMI => 20,
        BurstType::SOEMI => 17,
        BurstType::IEMI  => 17,
    }
}

fn parse_mac_pdu(burst_type: BurstType, raw_bytes: &[u8]) -> MacPdu {
    // Step 1: Validate CRC
    if !validate_pdu_crc(burst_type, raw_bytes) {
        return MacPdu::CrcError;
    }

    // Step 2: Parse header
    let header = raw_bytes[0];
    let opcode   = (header >> 5) & 0x07;
    let offset   = (header >> 2) & 0x07;
    let reserved = (header >> 1) & 0x01;
    let protected = header & 0x01;

    // Step 3: Dispatch on opcode
    match opcode {
        0b000 => {  // MAC_SIGNAL
            parse_mac_signal(burst_type, raw_bytes, protected != 0)
        }
        0b001 => {  // MAC_PTT
            parse_mac_ptt(burst_type, raw_bytes)
        }
        0b010 => {  // MAC_END_PTT
            parse_mac_end_ptt(burst_type, raw_bytes)
        }
        0b011 => {  // MAC_IDLE — carries signalling messages
            parse_mac_signal(burst_type, raw_bytes, protected != 0)
        }
        0b100 => {  // MAC_ACTIVE — carries signalling messages
            parse_mac_signal(burst_type, raw_bytes, protected != 0)
        }
        0b110 => {  // MAC_HANGTIME — carries signalling messages
            parse_mac_signal(burst_type, raw_bytes, protected != 0)
        }
        _ => MacPdu::UnknownOpcode(opcode),
    }
}
```

### 7.2 MAC Signal Parser (Sequential Message Extraction)

MAC_SIGNAL, MAC_IDLE, MAC_ACTIVE, and MAC_HANGTIME PDUs contain one or more
MAC messages packed sequentially into the payload area. Messages are parsed
left-to-right until the payload is exhausted.

```rust
fn parse_mac_signal(burst_type: BurstType, raw_bytes: &[u8], is_protected: bool) -> MacPdu {
    let direction = direction_from_burst(burst_type);
    let payload_len = payload_length(burst_type);
    let payload = &raw_bytes[1..1 + payload_len];  // skip PDU header

    let mut messages = Vec::new();
    let mut pos = 0;

    while pos < payload_len {
        // Read the MCO qualifier byte
        let mco_byte = payload[pos];
        let (b1, b2, mco) = decode_mco_byte(mco_byte);

        // Determine message length
        let msg_len = determine_message_length(
            b1, b2, mco, direction,
            payload, pos, payload_len
        );

        if msg_len == 0 || pos + msg_len > payload_len {
            break;  // malformed or end of payload
        }

        // Extract message bytes
        let msg_bytes = &payload[pos..pos + msg_len];

        // Parse the specific message
        let message = parse_mac_message(b1, b2, mco, direction, msg_bytes);
        messages.push(message);

        // Null and NullAZB fill to end — stop after them
        if b1 == 0 && b2 == 0 && (mco == 0b000000 || mco == 0b001000) {
            break;
        }

        pos += msg_len;
    }

    MacPdu::Signal { messages, is_protected }
}
```

### 7.3 MAC_PTT Parser

```rust
fn parse_mac_ptt(burst_type: BurstType, raw_bytes: &[u8]) -> MacPdu {
    let payload = &raw_bytes[1..];  // skip PDU header

    let mi       = &payload[0..9];                         // 9 octets, 72 bits
    let algid    = payload[9];                              // 1 octet
    let key_id   = u16::from_be_bytes([payload[10], payload[11]]);  // 2 octets
    let source   = u32::from_be_bytes([0, payload[12], payload[13], payload[14]]);  // 3 octets
    let group    = u16::from_be_bytes([payload[15], payload[16]]);  // 2 octets

    // I-OEMI has 3 additional reserved octets (payload[17..20])

    MacPdu::Ptt {
        mi: mi.to_vec(),
        algid,
        key_id,
        source_addr: source,
        group_addr: group,
    }
}
```

### 7.4 MAC_END_PTT Parser

```rust
fn parse_mac_end_ptt(burst_type: BurstType, raw_bytes: &[u8]) -> MacPdu {
    let payload = &raw_bytes[1..];

    // First 4 bits: reserved
    // Next 12 bits: Color Code
    let color_code = (((payload[0] & 0x0F) as u16) << 8) | (payload[1] as u16);

    // 80 bits reserved (octets 2-11)

    let source = u32::from_be_bytes([0, payload[12], payload[13], payload[14]]);
    let group  = u16::from_be_bytes([payload[15], payload[16]]);

    // I-OEMI has 3 additional reserved octets (payload[17..20])

    MacPdu::EndPtt {
        color_code,
        source_addr: source,
        group_addr: group,
    }
}
```

### 7.5 Message-Level Dispatch

```rust
fn parse_mac_message(b1: u8, b2: u8, mco: u8, dir: Direction, bytes: &[u8]) -> MacMessage {
    match (b1, b2) {
        (0, 0) => parse_tdma_unique(mco, bytes),
        (0, 1) => match dir {
            Direction::Outbound => parse_abbreviated_osp(mco, bytes),
            Direction::Inbound  => parse_abbreviated_isp(mco, bytes),
        },
        (1, 0) => parse_manufacturer_specific(mco, bytes),
        (1, 1) => match dir {
            Direction::Outbound => parse_extended_osp(mco, bytes),
            Direction::Inbound  => parse_extended_isp(mco, bytes),
        },
        _ => unreachable!(),
    }
}
```

---

## 8. Key Composite Field Definitions

### 8.1 SUID — Source Unit Full ID

```
Total: 56 bits (7 octets)

+--------------------+------------+----------------+
| WACN_ID [19:0]     | System_ID  | Unit_ID        |
| 20 bits             | [11:0]     | [23:0]         |
|                     | 12 bits    | 24 bits        |
+--------------------+------------+----------------+
Bits:  55..36           35..24       23..0
```

```rust
struct SUID {
    wacn_id: u32,    // 20 bits
    system_id: u16,  // 12 bits
    unit_id: u32,    // 24 bits
}

fn decode_suid(bytes: &[u8]) -> SUID {
    // 7 octets = 56 bits, big-endian
    let raw = u64::from_be_bytes([0, bytes[0], bytes[1], bytes[2],
                                  bytes[3], bytes[4], bytes[5], bytes[6]]);
    SUID {
        wacn_id:   ((raw >> 36) & 0xFFFFF) as u32,
        system_id: ((raw >> 24) & 0xFFF) as u16,
        unit_id:   (raw & 0xFFFFFF) as u32,
    }
}
```

### 8.2 SGID — Source Group Full ID

```
Total: 48 bits (6 octets)

+--------------------+------------+----------------+
| WACN_ID [19:0]     | System_ID  | Group_ID       |
| 20 bits             | [11:0]     | [15:0]         |
|                     | 12 bits    | 16 bits        |
+--------------------+------------+----------------+
Bits:  47..28           27..16       15..0
```

```rust
struct SGID {
    wacn_id: u32,    // 20 bits
    system_id: u16,  // 12 bits
    group_id: u16,   // 16 bits
}

fn decode_sgid(bytes: &[u8]) -> SGID {
    let raw = u64::from_be_bytes([0, 0, bytes[0], bytes[1],
                                  bytes[2], bytes[3], bytes[4], bytes[5]]);
    SGID {
        wacn_id:   ((raw >> 28) & 0xFFFFF) as u32,
        system_id: ((raw >> 16) & 0xFFF) as u16,
        group_id:  (raw & 0xFFFF) as u16,
    }
}
```

### 8.3 Avoid Zero Bias Pattern

Used by Null Avoid Zero Bias Information messages. Every octet in the fill region
is set to `0x88`:

```
Binary pattern per octet: 1000_1000 = 0x88
```

This ensures no long runs of zeros that could confuse symbol timing recovery.

### 8.4 Service Type Field

The Service Type field in various response messages (Queued Response, Deny Response,
etc.) contains the MCO value from the B1=0/B2=1 partition that identifies which
service is being referenced. For example, if a Deny Response references a Group Voice
Channel Grant, the Service Type field would contain `0b000000` (the MCO for Group Voice
Channel Grant in the abbreviated partition).

### 8.5 Channel Identifier Fields

Channel parameters are encoded as composite fields:

```
Implicit Channel (4 octets):
  Identifier[3:0] | Channel Number[11:0] | Identifier[3:0] | Channel Number[11:0]
  = Transmit Channel + Receive Channel

Explicit Channel (8 octets):
  Transmit Identifier[3:0] | Transmit Channel Number[11:0] |
  Transmit Frequency Band Offset[14:0] | ... (similar for receive)
```

The Identifier field references an Identifier Update message that provides the
base frequency and channel spacing for that identifier.

---

## 9. Annex A Opcode Mapping (ISP and OSP)

### 9.1 FDMA ISP to TDMA MAC Message Mapping (Table 14)

This table maps FDMA CAI Inbound Signalling Packet opcodes to their TDMA MAC equivalents.

```rust
// (fdma_opcode_hex, tdma_b1b2, tdma_mco_hex, name)
const FDMA_ISP_TO_TDMA: &[(u8, u8, u8, &str)] = &[
    // FDMA Abbreviated ISP -> TDMA B1=0,B2=1 (0b01)
    (0x01, 0b01, 0x01, "Group Voice Service Request"),
    (0x04, 0b01, 0x04, "Unit to Unit Voice Service Request - Abbreviated"),
    (0x05, 0b01, 0x05, "Unit to Unit Voice Service Answer Response - Abbreviated"),
    (0x08, 0b01, 0x08, "Telephone Interconnect Request - Explicit Dialing"),
    (0x09, 0b01, 0x09, "Telephone Interconnect Request - Implicit Dialing"),
    (0x0A, 0b01, 0x0A, "Telephone Interconnect Answer Response"),
    (0x12, 0b01, 0x12, "SNDCP Data Channel Request"),
    (0x13, 0b01, 0x13, "SNDCP Data Page Response"),
    (0x14, 0b01, 0x14, "SNDCP Reconnect Request"),
    (0x18, 0b01, 0x18, "Status Update - Abbreviated"),
    (0x19, 0b01, 0x19, "Status Query Response - Abbreviated"),
    (0x1A, 0b01, 0x1A, "Status Query Request - Abbreviated"),
    (0x1C, 0b01, 0x1C, "Message Update - Abbreviated"),
    (0x1D, 0b01, 0x1D, "Radio Unit Monitor Request - Abbreviated"),
    (0x1E, 0b01, 0x1E, "Radio Unit Monitor Enhanced Command - Abbreviated"),
    (0x1F, 0b01, 0x1F, "Call Alert Request - Abbreviated"),
    (0x20, 0b01, 0x20, "Acknowledge Response Unit - Abbreviated"),
    (0x23, 0b01, 0x23, "Cancel Service Request - Abbreviated"),
    (0x24, 0b01, 0x24, "Extended Function Response - Abbreviated"),
    (0x27, 0b01, 0x27, "Emergency Alarm Request"),
    (0x28, 0b01, 0x28, "Group Affiliation Request - Abbreviated"),
    (0x2A, 0b01, 0x2A, "Group Affiliation Query Response"),
    (0x2C, 0b01, 0x2C, "Unit Registration Request"),
    (0x2D, 0b01, 0x2D, "Location Registration Request - Abbreviated"),
    (0x2F, 0b01, 0x2F, "Unit Deregistration Request"),
    (0x32, 0b01, 0x32, "Identifier Update Request"),
    (0x36, 0b01, 0x36, "Roaming Address Request - Abbreviated"),
    (0x37, 0b01, 0x37, "Roaming Address Response"),
    (0x38, 0b01, 0x38, "Authentication Response - Abbreviated"),
    (0x39, 0b01, 0x39, "Authentication Response Mutual"),
    (0x3A, 0b01, 0x3A, "Authentication FNE Result - Abbreviated"),
    (0x3B, 0b01, 0x3B, "Authentication SU Demand - Abbreviated"),

    // FDMA Extended ISP -> TDMA B1=1,B2=1 (0b11)
    (0x04, 0b11, 0x04, "Unit to Unit Voice Service Request - Extended"),
    (0x05, 0b11, 0x05, "Unit to Unit Answer Response - Extended"),
    (0x18, 0b11, 0x18, "Status Update - Extended VCH"),
    (0x19, 0b11, 0x19, "Status Query Response - Extended"),
    (0x1A, 0b11, 0x1A, "Status Query Request - Extended"),
    (0x1C, 0b11, 0x1C, "Message Update - Extended VCH"),
    (0x1D, 0b11, 0x1D, "Radio Unit Monitor Request - Extended"),
    (0x1E, 0b11, 0x1E, "Radio Unit Monitor Enhanced Command - Extended"),
    (0x1F, 0b11, 0x1F, "Call Alert Request - Extended VCH"),
    (0x20, 0b11, 0x20, "Acknowledge Response Unit - Extended"),
    (0x23, 0b11, 0x23, "Cancel Service Request - Extended"),
    (0x24, 0b11, 0x24, "Extended Function Response - Extended VCH"),
    (0x28, 0b11, 0x28, "Group Affiliation Request - Extended"),
    (0x2D, 0b11, 0x2D, "Location Registration Request - Extended"),
    (0x32, 0b11, 0x32, "Authentication FNE Result - Extended"),
    (0x36, 0b11, 0x36, "Roaming Address Request - Extended"),
    (0x38, 0b11, 0x38, "Authentication Response - Extended"),
    (0x3B, 0b11, 0x3B, "Authentication SU Demand - Extended"),
];
```

### 9.2 FDMA OSP to TDMA MAC Message Mapping (Table 15)

```rust
// (fdma_opcode_hex, tdma_b1b2, tdma_mco_hex, name)
const FDMA_OSP_TO_TDMA: &[(u8, u8, u8, &str)] = &[
    // FDMA Abbreviated OSP -> TDMA B1=0,B2=1 (0b01)
    (0x00, 0b01, 0x00, "Group Voice Channel Grant - Implicit"),
    (0x02, 0b01, 0x02, "Group Voice Channel Grant Update - Implicit"),
    (0x04, 0b01, 0x04, "Unit to Unit Voice Service Channel Grant - Abbreviated"),
    (0x05, 0b01, 0x05, "Unit to Unit Answer Request - Abbreviated"),
    (0x06, 0b01, 0x06, "Unit to Unit Voice Channel Grant Update - Abbreviated"),
    (0x08, 0b01, 0x08, "Telephone Interconnect Voice Channel Grant - Implicit"),
    (0x09, 0b01, 0x09, "Telephone Interconnect Voice Channel Grant Update - Implicit"),
    (0x0A, 0b01, 0x0A, "Telephone Interconnect Answer Request"),
    (0x0C, 0b01, 0x0C, "Radio Unit Monitor Command - Abbreviated"),
    (0x14, 0b01, 0x14, "SNDCP Data Channel Grant"),
    (0x15, 0b01, 0x15, "SNDCP Data Page Request"),
    (0x18, 0b01, 0x18, "Status Update - Abbreviated"),
    (0x1A, 0b01, 0x1A, "Status Query - Abbreviated"),
    (0x1C, 0b01, 0x1C, "Message Update - Abbreviated"),
    (0x1D, 0b01, 0x1D, "Radio Unit Monitor Command - Obsolete"),
    (0x1E, 0b01, 0x1E, "Radio Unit Monitor Enhanced Command - Abbreviated"),
    (0x1F, 0b01, 0x1F, "Call Alert - Abbreviated"),
    (0x20, 0b01, 0x20, "Acknowledge Response FNE - Abbreviated"),
    (0x21, 0b01, 0x21, "Queued Response"),
    (0x24, 0b01, 0x24, "Extended Function Command - Abbreviated"),
    (0x27, 0b01, 0x27, "Deny Response"),
    (0x28, 0b01, 0x28, "Group Affiliation Response - Abbreviated"),
    (0x29, 0b01, 0x29, "Secondary Control Channel Broadcast - Implicit"),
    (0x2A, 0b01, 0x2A, "Group Affiliation Query - Abbreviated"),
    (0x2B, 0b01, 0x2B, "Location Registration Response"),
    (0x2C, 0b01, 0x2C, "Unit Registration Response - Abbreviated"),
    (0x2D, 0b01, 0x2D, "Unit Registration Command"),
    (0x2F, 0b01, 0x2F, "Unit Deregistration Acknowledge"),
    (0x30, 0b01, 0x30, "Synchronization Broadcast"),
    (0x31, 0b01, 0x31, "Authentication Demand"),
    (0x32, 0b01, 0x32, "Authentication FNE Response - Abbreviated"),
    (0x33, 0b01, 0x33, "Identifier Update for TDMA - Abbreviated"),
    (0x34, 0b01, 0x34, "Identifier Update for VHF/UHF Bands"),
    (0x35, 0b01, 0x35, "Time and Date Announcement"),
    (0x36, 0b01, 0x36, "Roaming Address Command"),
    (0x37, 0b01, 0x37, "Roaming Address Update"),
    (0x38, 0b01, 0x38, "System Service Broadcast"),
    (0x39, 0b01, 0x39, "Secondary Control Channel Broadcast - Implicit"),
    (0x3A, 0b01, 0x3A, "RFSS Status Broadcast - Implicit"),
    (0x3B, 0b01, 0x3B, "Network Status Broadcast - Implicit"),
    (0x3C, 0b01, 0x3C, "Adjacent Status Broadcast - Implicit"),
    (0x3D, 0b01, 0x3D, "Identifier Update"),

    // FDMA Extended/Explicit OSP -> TDMA B1=1,B2=1 (0b11)
    (0x00, 0b11, 0x00, "Group Voice Channel Grant - Explicit"),
    (0x03, 0b11, 0x03, "Group Voice Channel Grant Update - Explicit"),
    (0x04, 0b11, 0x04, "Unit to Unit Voice Service Channel Grant - Extended VCH"),
    (0x05, 0b11, 0x05, "Unit to Unit Answer Request - Extended"),
    (0x06, 0b11, 0x06, "Unit to Unit Voice Channel Grant Update - Extended VCH"),
    (0x07, 0b11, 0x07, "Unit to Unit Voice Channel Grant Update - Extended LCCH"),
    (0x08, 0b11, 0x08, "Telephone Interconnect Voice Channel Grant - Explicit"),
    (0x09, 0b11, 0x09, "Telephone Interconnect Voice Channel Grant Update - Explicit"),
    (0x0B, 0b11, 0x0B, "Call Alert - Extended LCCH"),
    (0x0D, 0b11, 0x0D, "Radio Unit Monitor Command - Extended LCCH"),
    (0x0E, 0b11, 0x0E, "Message Update - Extended LCCH"),
    (0x0F, 0b11, 0x0F, "Unit to Unit Voice Service Channel Grant - Extended LCCH"),
    (0x16, 0b11, 0x16, "SNDCP Data Channel Announcement - Explicit"),
    (0x18, 0b11, 0x18, "Status Update - Extended VCH"),
    (0x19, 0b11, 0x19, "Status Update - Extended LCCH"),
    (0x1A, 0b11, 0x1A, "Status Query - Extended VCH"),
    (0x1B, 0b11, 0x1B, "Status Query - Extended LCCH"),
    (0x1C, 0b11, 0x1C, "Message Update - Extended VCH"),
    (0x1E, 0b11, 0x1E, "Radio Unit Monitor Enhanced Command - Extended"),
    (0x1F, 0b11, 0x1F, "Call Alert - Extended VCH"),
    (0x20, 0b11, 0x20, "Acknowledge Response FNE - Extended"),
    (0x24, 0b11, 0x24, "Extended Function Command - Extended VCH"),
    (0x25, 0b11, 0x25, "Extended Function Command - Extended LCCH"),
    (0x28, 0b11, 0x28, "Group Affiliation Response - Extended"),
    (0x29, 0b11, 0x29, "Secondary Control Channel Broadcast - Explicit"),
    (0x2A, 0b11, 0x2A, "Group Affiliation Query - Extended"),
    (0x2C, 0b11, 0x2C, "Unit Registration Response - Extended"),
    (0x32, 0b11, 0x32, "Authentication FNE Response - Extended"),
    (0x33, 0b11, 0x33, "Identifier Update for TDMA - Extended"),
    (0x3A, 0b11, 0x3A, "RFSS Status Broadcast - Explicit"),
    (0x3B, 0b11, 0x3B, "Network Status Broadcast - Explicit"),
    (0x3C, 0b11, 0x3C, "Adjacent Status Broadcast - Explicit"),
    (0x3E, 0b11, 0x3E, "Adjacent Status Broadcast - Extended - Explicit"),
];
```

### 9.3 Mapping Rule

The MCO value in TDMA directly corresponds to the 6-bit opcode in FDMA for the
B1=0/B2=1 (abbreviated) and B1=1/B2=1 (extended/explicit) partitions. This is by
design: the TDMA MAC layer preserves the FDMA opcode numbering to simplify cross-mode
implementations.

For manufacturer-specific messages (B1=1, B2=0), the mapping is determined by the MFID
value in the second octet, and MCO values are manufacturer-defined.

---

## 10. Implementation Notes

### 10.1 Rust Enum Design

```rust
/// MAC PDU opcode (3 bits)
#[repr(u8)]
enum MacOpcode {
    MacSignal   = 0b000,
    MacPtt      = 0b001,
    MacEndPtt   = 0b010,
    MacIdle     = 0b011,
    MacActive   = 0b100,
    MacHangtime = 0b110,
}

/// B1/B2 partition
#[repr(u8)]
enum MacPartition {
    TdmaUnique       = 0b00,
    FdmaAbbreviated  = 0b01,
    ManufacturerSpec = 0b10,
    FdmaExtended     = 0b11,
}

/// Fully qualified message identifier
struct MacMessageId {
    partition: MacPartition,
    mco: u8,            // 6 bits
    direction: Direction,
    mfid: Option<u8>,   // Only for ManufacturerSpec
}
```

### 10.2 SDRTrunk / OP25 Cross-Reference

**SDRTrunk** (Java) implements TDMA MAC parsing in:
- `MacOpcode.java` — enum of all MAC opcodes with B1/B2/MCO encoding
- `MacMessage.java` — base class for MAC message parsing
- `MacMessageFactory.java` — dispatch logic using opcode + direction

Key patterns from SDRTrunk:
- Uses a flat enum with integer opcode values combining B1, B2, and MCO
- Direction is determined from the channel type (traffic vs control, inbound vs outbound)
- Variable-length messages are handled by checking remaining bytes in the PDU

**OP25** (C++/Python) implements TDMA MAC parsing in:
- `tdma_msg.py` / `p25_tdma.cc` — MAC PDU parsing
- Uses the burst type to determine direction and payload size
- CRC validation is performed before message dispatch

### 10.3 Multi-Message PDU Handling

A single MAC_SIGNAL/MAC_IDLE/MAC_ACTIVE/MAC_HANGTIME PDU can contain **multiple**
MAC messages packed sequentially. The parser must:

1. Start at the first payload octet (after PDU header)
2. Read B1/B2/MCO from the first byte of the message
3. Determine message length (Section 5)
4. Extract and parse the message
5. Advance the position by the message length
6. Repeat until:
   - Position reaches the end of the payload area, OR
   - A Null Information or Null Avoid Zero Bias message is encountered (these fill
     the remainder and are always last)

### 10.4 Protected (Encrypted) PDU Handling

When the Protected bit (P) in the PDU header is set to 1, the MAC message payload
is encrypted. The parser should:
1. Still parse the PDU header (it is not encrypted)
2. Note that the payload cannot be parsed without the encryption key
3. Store the raw encrypted payload for later decryption if keys become available

### 10.5 Color Code Validation (OECI)

For outbound LCCH (OECI) PDUs, the 12-bit Color Code in the trailer should be
validated against the expected system Color Code before processing the messages.
This provides an additional layer of filtering beyond CRC validation.

### 10.6 Offset Field Usage

The Offset field (bits [4:2] of the PDU header) indicates the timeslot offset for
the message content. For MAC_SIGNAL PDUs, this is typically 0. For MAC_ACTIVE and
MAC_HANGTIME, it indicates which slot the voice traffic occupies relative to the
current burst position.

---

## 11. Quick Reference: Payload Extraction by Burst Type

```
Burst Type    Direction    Payload Start    Payload End    CRC Type    CRC Position
----------    ---------    -------------    -----------    --------    ------------
OECI          Outbound     Octet 1          Octet 18       CRC-16      Bits 164-179
IECI          Inbound      Octet 1          Octet 14       CRC-16      Bits 120-135
I-OEMI        Outbound     Octet 1          Octet 20       CRC-12      Bits 168-179
S-OEMI        Outbound     Octet 1          Octet 17       CRC-12      Bits 144-155
IEMI          Inbound      Octet 1          Octet 17       CRC-12      Bits 144-155
```

All octet numbering is 0-based from the start of the PDU (octet 0 = PDU header).
CRC is computed over all bits from octet 0 through the end of the payload area
(or through the Color Code field for OECI).
