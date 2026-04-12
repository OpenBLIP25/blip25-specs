# P25 Link Control Word (LCW) Formats and Messages — Implementation Specification

**Source:** TIA-102.AABF-D (February 2015), "Project 25 Link Control Word Formats and Messages"
**Extracted:** 2026-04-12 from PDF with structural verification
**Classification:** MESSAGE_FORMAT
**Purpose:** Self-contained spec for implementing a complete FDMA Link Control Word parser
and encoder. No reference to the original PDF required.

---

## 1. LCW Structure and Embedding

### 1.1 Fundamental Format

A Link Control Word (LCW) is exactly **72 bits (9 octets)** of information. It is the
primary mechanism for carrying call metadata (talker identity, group/unit addressing,
encryption status, system topology) alongside voice data in P25 systems.

```
Bit positions (MSB-first per octet):

         Bit 7   6   5   4   3   2   1   0
Octet 0 | P  | SF |       LCO[5:0]        |   ← Link Control Format (LCF)
Octet 1 |          Payload byte 1          |
Octet 2 |          Payload byte 2          |
Octet 3 |          Payload byte 3          |
Octet 4 |          Payload byte 4          |
Octet 5 |          Payload byte 5          |
Octet 6 |          Payload byte 6          |
Octet 7 |          Payload byte 7          |
Octet 8 |          Payload byte 8          |
```

**Total: 72 information bits.** Error correction (Reed-Solomon + shortened Hamming or
extended Golay) is applied externally per TIA-102.BAAA and is NOT part of this spec.

### 1.2 LCF (Link Control Format) — Octet 0

The first octet is always the LCF byte, containing three sub-fields:

```
Bit:    7     6     5     4     3     2     1     0
      +-----+-----+-----+-----+-----+-----+-----+-----+
      |  P  | SF  |           LCO[5:0]                  |
      +-----+-----+-----+-----+-----+-----+-----+-----+
```

| Field | Bits | Width | Description |
|-------|------|-------|-------------|
| P     | [7]  | 1 bit | **Protected flag.** 1 = payload is encrypted; 0 = cleartext |
| SF    | [6]  | 1 bit | **MFID format.** 1 = Implicit (standard MFID $00 assumed); 0 = Explicit (MFID in octet 1) |
| LCO   | [5:0]| 6 bits| **Link Control Opcode.** Values 0-63 select the message type |

**Full LCF byte value** (combined P + SF + LCO) is the primary dispatch key.
The "LCF value" referenced in scanner software (e.g., SDRTrunk, OP25) is this entire
8-bit octet 0.

### 1.3 Implicit vs. Explicit MFID Formats

**Implicit MFID (SF=1):** Standard P25 MFID $00 is assumed. Octets 1-8 are the full
message payload. When P=1, octets 1-8 are encrypted.

```
Octet 0: LCF (P | SF=1 | LCO)
Octet 1-8: Message payload (8 octets)
           When P=1: octets 1-8 encrypted
```

**Explicit MFID (SF=0):** Octet 1 carries the MFID value. Octets 2-8 are
manufacturer-defined content. When P=1, octets 2-8 are encrypted.

```
Octet 0: LCF (P | SF=0 | LCO)
Octet 1: MFID (Manufacturer ID)
Octet 2-8: Manufacturer-defined payload (7 octets)
           When P=1: octets 2-8 encrypted
```

Standard MFID values: $00 (default P25), $01 (reserved for future standard expansion).
Non-standard MFIDs (e.g., $90 = Motorola, $A4 = Harris) identify proprietary LC words.

### 1.4 FDMA Embedding — HDU, LDU1, and ETDU Frames

In FDMA (Phase 1, 9600 bps C4FM), LCWs are carried in specific voice superframe
positions defined by TIA-102.BAAA:

| Frame Type | Role | LCW Content |
|------------|------|-------------|
| **HDU** (Header Data Unit) | First frame of voice transmission | Contains MI (Message Indicator), MFID, ALGID, KeyID, TGID — NOT an LCW per se, but overlapping fields |
| **LDU1** (Logical Link Data Unit 1) | Voice frame carrying LC | The 72-bit LCW is distributed across the LDU1 frame in designated bit positions (after RS error correction) |
| **LDU2** (Logical Link Data Unit 2) | Voice frame carrying encryption sync | Carries MI, ALGID, KeyID — NOT an LCW |
| **ETDU** (Expanded Terminator Data Unit) | Call termination frame | Carries an LCW (typically LCO=15 Call Termination) |
| **TDU** (Terminator Data Unit) | Simple terminator | No LCW (just NID) |

**LDU1 LC extraction:** The 72 LC bits are spread across the LDU1 frame in 6 groups of
12 bits each, interleaved with IMBE voice codewords. After Reed-Solomon (24,12,13)
decoding, the 72 information bits are recovered as 9 contiguous octets.

**ETDU LC extraction:** Similar RS-coded LC bits embedded in the terminator frame.

A typical FDMA voice call produces this sequence:
```
HDU → LDU1 → LDU2 → LDU1 → LDU2 → ... → TDU/ETDU
       ↑LC            ↑LC                    ↑LC (if ETDU)
```

Each LDU1 in the stream carries the same or updated LCW. The LC word announces the
active call parameters for the duration of the voice transmission.

### 1.5 TDMA Embedding (Phase 2)

In Phase 2 TDMA (TIA-102.BBAD-A), the LCW concept is replaced by MAC messages in
the EMI (Embedded Message Indicator) slots. The TDMA equivalents of LCW call metadata
are carried by specific MAC message types:

| FDMA LCW (AABF-D) | TDMA MAC Message (BBAD-A) | Partition |
|--------------------|---------------------------|-----------|
| LCO 0: Group Voice Channel User | Group Voice Channel User - Abbreviated (MCO=0b000001, 7 bytes) | B1=0,B2=0 (Unique TDMA) |
| LCO 0: Group Voice Channel User | Group Voice Channel User - Extended (MCO=0b100001, 14 bytes) | B1=0,B2=0 (Unique TDMA) |
| LCO 3: Unit-to-Unit Voice Channel User | Unit to Unit Voice Channel User - Abbreviated (MCO=0b000010, 8 bytes) | B1=0,B2=0 |
| LCO 3: Unit-to-Unit Voice Channel User | Unit to Unit Voice Channel User - Extended (MCO=0b100010, 15 bytes) | B1=0,B2=0 |
| LCO 6: Telephone Interconnect VCU | Telephone Interconnect Voice Channel User (MCO=0b000011, 7 bytes) | B1=0,B2=0 |
| LCO 15: Call Termination | MAC_Release (MCO=0b110001, 7 bytes) | B1=0,B2=0 |

In TDMA, the MAC_PTT and MAC_END_PTT PDUs also carry Source Address and Group Address
in a fixed format (see TIA-102.BBAD-A Section 4.1/4.2). These provide the same call
metadata as LCO 0/3 LCWs but in a TDMA-specific encoding.

**Key difference:** FDMA LCWs use a 72-bit fixed format with RS error coding. TDMA MAC
messages use variable-length formats within EMI bursts with CRC-12 protection. The
information content (source, group, service options) is semantically equivalent.

---

## 2. Complete LCO Dispatch Table

All 64 possible LCO values. The "LCF" column shows the full octet 0 value when P=0.
When P=1, add $80 to these values.

```rust
/// LCO dispatch table — all 64 opcode values
/// (lco, sf, alias, full_name, length_note)
///
/// SF: 0 = Explicit MFID (octet 1 = MFID), 1 = Implicit MFID ($00)
/// When SF=0: payload in octets 2-8 (7 bytes)
/// When SF=1: payload in octets 1-8 (8 bytes)

const LCO_DISPATCH: &[(u8, u8, &str, &str)] = &[
    // === Voice Call User Messages ===
    (0x00, 0, "LC_GRP_V_CH_USR",           "Group Voice Channel User"),                   // 7.3.1
    (0x01, 0, "RESERVED",                   "Reserved"),
    (0x02, 1, "LC_GRP_V_CH_UPDT",          "Group Voice Channel Update"),                 // 7.3.2
    (0x03, 0, "LC_UU_V_CH_USR",            "Unit to Unit Voice Channel User"),             // 7.3.3
    (0x04, 1, "LC_GRP_CH_UPDT_EXP",        "Group Voice Channel Update - Explicit"),       // 7.3.4
    (0x05, 1, "LC_UU_ANS_REQ",             "Unit to Unit Answer Request"),                 // 7.3.5
    (0x06, 1, "LC_TELE_INT_V_CH_USR",      "Telephone Interconnect Voice Channel User"),   // 7.3.6
    (0x07, 1, "LC_TELE_INT_ANS_REQ",       "Telephone Interconnect Answer Request"),       // 7.3.7
    (0x08, 0, "RESERVED",                   "Reserved"),
    (0x09, 1, "LC_SOURCE_ID_EXT",           "Source ID Extension"),                         // 7.3.32
    (0x0A, 1, "LC_UU_V_CH_USR_EXT",        "Unit-to-Unit Voice Channel User - Extended"),  // 7.3.29
    (0x0B, 0, "RESERVED",                   "Reserved"),
    (0x0C, 0, "RESERVED",                   "Reserved"),
    (0x0D, 0, "RESERVED",                   "Reserved"),
    (0x0E, 0, "RESERVED",                   "Reserved"),

    // === Call Lifecycle ===
    (0x0F, 1, "LC_CALL_TRM_CAN",           "Call Termination/Cancellation"),               // 7.3.8

    // === System Queries and Commands ===
    (0x10, 1, "LC_GRP_AFF_Q",              "Group Affiliation Query"),                     // 7.3.9
    (0x11, 1, "LC_U_REG_CMD",              "Unit Registration Command"),                   // 7.3.10
    (0x12, 0, "RESERVED_OBSOLETE",          "Unit Authentication Command (OBSOLETE)"),      // 7.3.11
    (0x13, 1, "LC_STS_Q",                  "Status Query"),                                // 7.3.12
    (0x14, 1, "LC_STS_UPDT",               "Status Update"),                               // 7.3.18
    (0x15, 1, "LC_MSG_UPDT",               "Message Update"),                              // 7.3.19
    (0x16, 1, "LC_CALL_ALRT",              "Call Alert"),                                  // 7.3.20
    (0x17, 1, "LC_EXT_FNCT_CMD",           "Extended Function Command"),                   // 7.3.21
    (0x18, 1, "LC_CH_ID_UPDT",             "Channel Identifier Update"),                   // 7.3.22
    (0x19, 1, "LC_CH_ID_UPDT_VU",          "Channel Identifier Update VU"),                // 7.3.26
    (0x1A, 1, "LC_STS_UPDT_SRC_RQRD",     "Status Update - Source ID Required"),           // 7.3.30
    (0x1B, 1, "LC_MSG_UPDT_SRC_RQRD",     "Message Update - Source ID Required"),          // 7.3.31
    (0x1C, 1, "LC_EXT_FNCT_CMD_SRC_RQRD", "Extended Function Command - Source ID Required"), // 7.3.33
    (0x1D, 0, "RESERVED",                   "Reserved"),
    (0x1E, 0, "RESERVED",                   "Reserved"),
    (0x1F, 0, "RESERVED",                   "Reserved"),

    // === Network Identity Broadcasts ===
    (0x20, 1, "LC_SYS_SRV_BCST",           "System Service Broadcast"),                   // 7.3.13
    (0x21, 1, "LC_SCCB",                   "Secondary Control Channel Broadcast"),         // 7.3.14
    (0x22, 1, "LC_ADJ_STS_BCST",           "Adjacent Site Status Broadcast"),              // 7.3.15
    (0x23, 1, "LC_RFSS_STS_BCST",          "RFSS Status Broadcast"),                      // 7.3.16
    (0x24, 1, "LC_NET_STS_BCST",           "Network Status Broadcast"),                   // 7.3.17
    (0x25, 0, "RESERVED_OBSOLETE",          "Protection Parameter Broadcast (OBSOLETE)"),   // 7.3.23
    (0x26, 1, "LC_SCCB_EXP",               "Secondary Control Channel Broadcast - Explicit"), // 7.3.24
    (0x27, 1, "LC_ADJ_STS_BCST_EXP",       "Adjacent Site Status Broadcast - Explicit"),   // 7.3.25
    (0x28, 1, "LC_RFSS_STS_BCST_EXP",      "RFSS Status Broadcast - Explicit"),            // 7.3.27
    (0x29, 1, "LC_NET_STS_BCST_EXP",       "Network Status Broadcast - Explicit"),         // 7.3.28
    (0x2A, 0, "LC_CONV_FALLBACK",           "Conventional Fallback Indication"),            // 7.3.34
    // 0x2B - 0x3F: Reserved
    (0x2B, 0, "RESERVED",                   "Reserved"),
    (0x2C, 0, "RESERVED",                   "Reserved"),
    (0x2D, 0, "RESERVED",                   "Reserved"),
    (0x2E, 0, "RESERVED",                   "Reserved"),
    (0x2F, 0, "RESERVED",                   "Reserved"),
    (0x30, 0, "RESERVED",                   "Reserved"),
    (0x31, 0, "RESERVED",                   "Reserved"),
    (0x32, 0, "RESERVED",                   "Reserved"),
    (0x33, 0, "RESERVED",                   "Reserved"),
    (0x34, 0, "RESERVED",                   "Reserved"),
    (0x35, 0, "RESERVED",                   "Reserved"),
    (0x36, 0, "RESERVED",                   "Reserved"),
    (0x37, 0, "RESERVED",                   "Reserved"),
    (0x38, 0, "RESERVED",                   "Reserved"),
    (0x39, 0, "RESERVED",                   "Reserved"),
    (0x3A, 0, "RESERVED",                   "Reserved"),
    (0x3B, 0, "RESERVED",                   "Reserved"),
    (0x3C, 0, "RESERVED",                   "Reserved"),
    (0x3D, 0, "RESERVED",                   "Reserved"),
    (0x3E, 0, "RESERVED",                   "Reserved"),
    (0x3F, 0, "RESERVED",                   "Reserved"),
];
```

### 2.1 LCO Usage Matrix

| LCO | Name | Conv Out | Conv In | Trunk Out | Trunk In |
|-----|------|:--------:|:-------:|:---------:|:--------:|
| 0   | Group Voice Channel User        | x | x | x | x |
| 2   | Group Voice Channel Update       |   |   | x |   |
| 3   | Unit-to-Unit Voice Channel User  | x | x | x | x |
| 4   | Group Voice Ch Update - Explicit |   |   | x |   |
| 5   | Unit-to-Unit Answer Request      |   |   | x |   |
| 6   | Telephone Interconnect VCU       | x | x | x | x |
| 7   | Telephone Interconnect Ans Req   |   |   | x |   |
| 9   | Source ID Extension              |   |   | x | x |
| 10  | Unit-to-Unit VCU - Extended      |   |   | x | x |
| 15  | Call Termination/Cancellation    | x | x | x | x |
| 16  | Group Affiliation Query          |   |   | x |   |
| 17  | Unit Registration Command        |   |   | x |   |
| 19  | Status Query                     |   |   | x | x |
| 20  | Status Update                    |   |   | x |   |
| 21  | Message Update                   |   |   | x |   |
| 22  | Call Alert                       |   |   | x | x |
| 23  | Extended Function Command        |   |   | x | x |
| 24  | Channel Identifier Update        |   |   | x | x |
| 25  | Channel Identifier Update VU     |   |   | x |   |
| 26  | Status Update - Src ID Req       |   |   | x |   |
| 27  | Message Update - Src ID Req      |   |   | x |   |
| 28  | Ext Function Cmd - Src ID Req    |   |   | x |   |
| 32  | System Service Broadcast         |   |   | x | x |
| 33  | Secondary CC Broadcast           |   |   | x |   |
| 34  | Adjacent Site Status Broadcast   |   |   | x | x |
| 35  | RFSS Status Broadcast            |   |   | x | x |
| 36  | Network Status Broadcast         |   |   | x |   |
| 38  | Secondary CC Broadcast - Explicit|   |   | x |   |
| 39  | Adjacent Site Status - Explicit  |   |   | x |   |
| 40  | RFSS Status Broadcast - Explicit |   |   | x |   |
| 41  | Network Status Broadcast - Exp   |   |   | x |   |
| 42  | Conventional Fallback            |   |   | x | x |

---

## 3. Standard LC Format Detailed Field Layouts

### 3.1 LCO $00 — Group Voice Channel User (Unencrypted)

The most common LC word in P25 systems. Identifies the talker and group on the
current voice channel. Used on both conventional and trunked, inbound and outbound.

**SF=0 (Explicit MFID format). Full LCF byte = $00 (clear) or $80 (encrypted).**

```
Octet   Bits      Field               Width    Description
------  --------  ------------------  -------  ------------------------------------
  0     [7]       P (Protected)       1 bit    0=clear, 1=encrypted (octets 1-8)
  0     [6]       SF                  1 bit    0 (Explicit MFID)
  0     [5:0]     LCO                 6 bits   $00 = %000000
  1     [7:0]     MFID                8 bits   Manufacturer ID ($00 = standard P25)
  2     [7:0]     Service Options     8 bits   See Section 3.5
  3     [7:1]     Reserved            7 bits   Set to 0 on transmit, ignored on receive
  3     [0]       S (Src ID Ext Req)  1 bit    1=next LC is Source ID Extension (LCO 9)
  4     [7:0]     Group Address[15:8] 8 bits   MSB of 16-bit Group Address
  5     [7:0]     Group Address[7:0]  8 bits   LSB of 16-bit Group Address
  6     [7:0]     Source Address[23:16] 8 bits MSB of 24-bit Source Address (WUID)
  7     [7:0]     Source Address[15:8] 8 bits  Middle byte of Source Address
  8     [7:0]     Source Address[7:0] 8 bits   LSB of Source Address
```

**Bit-level extraction (from 72-bit LCW vector, bit 0 = MSB of octet 0):**

```
Bits  [0]      = P
Bits  [1]      = SF (= 0)
Bits  [2:7]    = LCO (= 0b000000)
Bits  [8:15]   = MFID
Bits  [16:23]  = Service Options
Bits  [24:30]  = Reserved
Bit   [31]     = S flag
Bits  [32:47]  = Group Address (16 bits)
Bits  [48:71]  = Source Address (24 bits)
```

### 3.2 LCO $03 — Unit-to-Unit Voice Channel User (Unencrypted)

Identifies talker and target for unit-to-unit (private) voice calls.
Used on conventional and trunked, inbound and outbound.

**SF=0 (Explicit MFID format). Full LCF byte = $03 (clear) or $83 (encrypted).**

```
Octet   Bits      Field               Width    Description
------  --------  ------------------  -------  ------------------------------------
  0     [7]       P (Protected)       1 bit    0=clear, 1=encrypted
  0     [6]       SF                  1 bit    0 (Explicit MFID)
  0     [5:0]     LCO                 6 bits   $03 = %000011
  1     [7:0]     MFID                8 bits   Manufacturer ID ($00 = standard P25)
  2     [7:0]     Service Options     8 bits   See Section 3.5
  3     [7:0]     Target Address[23:16] 8 bits MSB of 24-bit Target Address
  4     [7:0]     Target Address[15:8] 8 bits  Middle byte of Target Address
  5     [7:0]     Target Address[7:0] 8 bits   LSB of Target Address
  6     [7:0]     Source Address[23:16] 8 bits MSB of 24-bit Source Address (WUID)
  7     [7:0]     Source Address[15:8] 8 bits  Middle byte of Source Address
  8     [7:0]     Source Address[7:0] 8 bits   LSB of Source Address
```

**Bit-level extraction:**

```
Bits  [0]      = P
Bits  [1]      = SF (= 0)
Bits  [2:7]    = LCO (= 0b000011)
Bits  [8:15]   = MFID
Bits  [16:23]  = Service Options
Bits  [24:47]  = Target Address (24 bits)
Bits  [48:71]  = Source Address (24 bits)
```

### 3.3 LCO $00 with P=1 — Group Voice Channel User (Encrypted)

**Full LCF byte = $80.** Same structure as Section 3.1 but P=1 indicates octets 1-8
are encrypted. After decryption, the field layout is identical to LCO $00 clear.

```
Octet 0:  LCF = $80 (P=1, SF=0, LCO=$00)
Octet 1-8: Encrypted payload
           After decryption → same layout as Section 3.1 octets 1-8
```

**Implementation note:** When P=1, octets 1-8 must be decrypted before field extraction.
The encryption parameters (ALGID, KeyID, MI) come from the HDU or LDU2 frames (FDMA)
or from the MAC_PTT PDU (TDMA). In SDRTrunk and OP25, encrypted LCWs are typically
logged with the encrypted payload hex and a flag indicating encryption is active.

### 3.4 LCO $03 with P=1 — Unit-to-Unit Voice Channel User (Encrypted)

**Full LCF byte = $83.** Same structure as Section 3.2 but P=1 indicates octets 1-8
are encrypted.

```
Octet 0:  LCF = $83 (P=1, SF=0, LCO=$03)
Octet 1-8: Encrypted payload
           After decryption → same layout as Section 3.2 octets 1-8
```

### 3.5 Service Options Field (8 bits)

Used in LCO 0, 3, 4, 5, 6, 10. This byte appears in the same position for all
voice channel user messages.

```
Bit:    7     6     5     4     3     2     1     0
      +-----+-----+-----+-----+-----+-----+-----+-----+
      |  E  |  P  |  D  |  M  |  R  | Priority[2:0]   |
      +-----+-----+-----+-----+-----+-----+-----+-----+
```

| Bit | Field | Description |
|-----|-------|-------------|
| [7] | E - Emergency | 0=normal, 1=emergency (special processing required) |
| [6] | P - Protected | 0=unprotected mode, 1=protected/encrypted resources |
| [5] | D - Duplex | 0=half duplex, 1=full duplex |
| [4] | M - Mode | 0=circuit mode, 1=packet mode |
| [3] | R - Reserved | Set to 0, ignored on receive |
| [2:0] | Priority Level | 3-bit priority (0-7), per TIA-102.AABC |

**Constants:**

```rust
const SERVICE_OPT_EMERGENCY: u8    = 0x80;  // bit 7
const SERVICE_OPT_PROTECTED: u8    = 0x40;  // bit 6
const SERVICE_OPT_DUPLEX: u8       = 0x20;  // bit 5
const SERVICE_OPT_PACKET_MODE: u8  = 0x10;  // bit 4
const SERVICE_OPT_RESERVED: u8     = 0x08;  // bit 3
const SERVICE_OPT_PRIORITY_MASK: u8 = 0x07; // bits 2:0
```

---

## 4. All Other LC Format Field Layouts

### 4.1 LCO $02 — Group Voice Channel Update (SF=1, Implicit MFID)

Notifies listeners of other concurrent group calls on the trunked system.
Two channel/group pairs per LC word.

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($42)               8 bits   P=0, SF=1, LCO=$02
  1     Channel-A[15:8]         8 bits   ─┐ 16-bit Channel number A
  2     Channel-A[7:0]          8 bits   ─┘   (4-bit Ident + 12-bit ChNum)
  3     Group Address-A[15:8]   8 bits   ─┐ 16-bit Group Address A
  4     Group Address-A[7:0]    8 bits   ─┘
  5     Channel-B[15:8]         8 bits   ─┐ 16-bit Channel number B
  6     Channel-B[7:0]          8 bits   ─┘
  7     Group Address-B[15:8]   8 bits   ─┐ 16-bit Group Address B
  8     Group Address-B[7:0]    8 bits   ─┘
```

### 4.2 LCO $04 — Group Voice Channel Update - Explicit (SF=1)

Explicit transmit/receive channel pair variant. Trunked outbound only.

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($44)               8 bits   P=0, SF=1, LCO=$04
  1     Reserved                8 bits
  2     Service Options         8 bits   See Section 3.5
  3     Group Address[15:8]     8 bits   ─┐
  4     Group Address[7:0]      8 bits   ─┘ 16-bit Group Address
  5     Channel-T[15:8]         8 bits   ─┐ 16-bit Transmit Channel
  6     Channel-T[7:0]          8 bits   ─┘
  7     Channel-R[15:8]         8 bits   ─┐ 16-bit Receive Channel
  8     Channel-R[7:0]          8 bits   ─┘
```

### 4.3 LCO $05 — Unit-to-Unit Answer Request (SF=1)

Trunked outbound only. Looking for target unit for private call.

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($45)               8 bits
  1     Service Options         8 bits   See Section 3.5
  2     Reserved[7:1] | S[0]    8 bits   S=Source ID Extension Required
  3     Target Address[23:16]   8 bits   ─┐
  4     Target Address[15:8]    8 bits    │ 24-bit Target Address
  5     Target Address[7:0]     8 bits   ─┘
  6     Source Address[23:16]   8 bits   ─┐
  7     Source Address[15:8]    8 bits    │ 24-bit Source Address
  8     Source Address[7:0]     8 bits   ─┘
```

### 4.4 LCO $06 — Telephone Interconnect Voice Channel User (SF=1)

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($46)               8 bits
  1     Reserved                8 bits
  2     Service Options         8 bits
  3     Reserved                8 bits
  4     Call Timer[15:8]        8 bits   ─┐ 16-bit timer (units = 100ms)
  5     Call Timer[7:0]         8 bits   ─┘ $0000=unlimited, $0001-$FFFF=0.1s-6553.6s
  6     Source/Target Addr[23:16] 8 bits ─┐
  7     Source/Target Addr[15:8]  8 bits  │ 24-bit address (inbound=source, outbound=target)
  8     Source/Target Addr[7:0]   8 bits ─┘
```

### 4.5 LCO $07 — Telephone Interconnect Answer Request (SF=1)

Trunked outbound only. Carries up to 10 BCD PSTN digits.

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($47)               8 bits
  1     Digit 1[7:4] | Digit 2[3:0]   8 bits   BCD nibble pairs
  2     Digit 3[7:4] | Digit 4[3:0]   8 bits
  3     Digit 5[7:4] | Digit 6[3:0]   8 bits
  4     Digit 7[7:4] | Digit 8[3:0]   8 bits
  5     Digit 9[7:4] | Digit 10[3:0]  8 bits   (NOTE: PDF rendering ambiguous here)
  6     Target Address[23:16]   8 bits  ─┐
  7     Target Address[15:8]    8 bits   │ 24-bit Target Address
  8     Target Address[7:0]     8 bits  ─┘
```

> **EXTRACTION CAVEAT:** The PDF scan for octet 5 is ambiguous — the digit numbering
> may have a rendering artifact. Logically, 5 nibble-pair octets carry digits 1-10.
> Flag for manual verification against a clean copy of AABF-D.

### 4.6 LCO $09 — Source ID Extension (SF=1)

Follows any LC word with S=1. Carries full SUID (WACN + System + Unit) for
cross-system subscriber identification.

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($49)               8 bits
  1     Reserved                8 bits
  2     Network ID[19:12]       8 bits   ─┐
  3     Network ID[11:4]        8 bits    │ 20-bit WACN
  4     Network ID[3:0] | SysID[11:8]  8 bits ─┘ + top 4 bits of 12-bit System ID
  5     System ID[7:0]          8 bits      bottom 8 bits of System ID
  6     Source ID[23:16]        8 bits   ─┐
  7     Source ID[15:8]         8 bits    │ 24-bit Unit ID
  8     Source ID[7:0]          8 bits   ─┘
```

**Full SUID reconstruction:** WACN (20 bits) + System ID (12 bits) + Unit ID (24 bits) = 56-bit SUID.

### 4.7 LCO $0A — Unit-to-Unit Voice Channel User - Extended (SF=1)

Extended form used when source or destination is from a different system/network.
Always has S=1, always followed by Source ID Extension (LCO $09).

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($4A)               8 bits
  1     Reserved[7:1] | S[0]    8 bits   S always = 1
  2     Service Options         8 bits   See Section 3.5
  3     Target Address[23:16]   8 bits   ─┐
  4     Target Address[15:8]    8 bits    │ 24-bit Target Address
  5     Target Address[7:0]     8 bits   ─┘
  6     Source ID[23:16]        8 bits   ─┐
  7     Source ID[15:8]         8 bits    │ 24-bit Source Unit ID
  8     Source ID[7:0]          8 bits   ─┘
```

### 4.8 LCO $0F — Call Termination/Cancellation (SF=1)

Carried in ETDU frame to signal channel release.

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($4F)               8 bits
  1     Reserved                8 bits
  2     Reserved                8 bits
  3     Reserved                8 bits
  4     Reserved                8 bits
  5     Reserved                8 bits
  6     Src/Tgt Address[23:16]  8 bits   ─┐ Inbound = source, outbound = target
  7     Src/Tgt Address[15:8]   8 bits    │ 24-bit address
  8     Src/Tgt Address[7:0]    8 bits   ─┘
```

### 4.9 LCO $10 — Group Affiliation Query (SF=1)

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($50)               8 bits
  1     Reserved                8 bits
  2     Reserved                8 bits
  3     Target Address[23:16]   8 bits   ─┐
  4     Target Address[15:8]    8 bits    │ 24-bit Target Address
  5     Target Address[7:0]     8 bits   ─┘
  6     Source Address[23:16]   8 bits   ─┐
  7     Source Address[15:8]    8 bits    │ 24-bit Source Address
  8     Source Address[7:0]     8 bits   ─┘
```

### 4.10 LCO $11 — Unit Registration Command (SF=1)

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($51)               8 bits
  1     Network ID[19:12]       8 bits   ─┐
  2     Network ID[11:4]        8 bits    │ 20-bit WACN
  3     Network ID[3:0] | SysID[11:8]  8 bits ─┘
  4     System ID[7:0]          8 bits
  5     Reserved                8 bits
  6     Target ID[23:16]        8 bits   ─┐
  7     Target ID[15:8]         8 bits    │ 24-bit Target Unit ID
  8     Target ID[7:0]          8 bits   ─┘   (NOTE: extraction shows Reserved in octet 8)
```

> **EXTRACTION NOTE:** The PDF extraction for LCO $11 shows octets 5-8 as
> "Reserved / Target ID / Reserved" with some ambiguity in exact byte boundaries.
> The layout above reflects the logical structure but may need verification.

### 4.11 LCO $13 — Status Query (SF=1)

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($53)               8 bits
  1     Reserved[7:1] | S[0]    8 bits   S=Source ID Extension Required
  2     Reserved                8 bits
  3     Target Address[23:16]   8 bits   ─┐
  4     Target Address[15:8]    8 bits    │ 24-bit Target Address
  5     Target Address[7:0]     8 bits   ─┘
  6     Source Address[23:16]   8 bits   ─┐
  7     Source Address[15:8]    8 bits    │ 24-bit Source Address
  8     Source Address[7:0]     8 bits   ─┘
```

### 4.12 LCO $14 — Status Update (SF=1)

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($54)               8 bits
  1     Status[15:8]            8 bits   ─┐ 16-bit Status (user + unit subfields)
  2     Status[7:0]             8 bits   ─┘
  3     Target Address[23:16]   8 bits   ─┐
  4     Target Address[15:8]    8 bits    │ 24-bit Target Address
  5     Target Address[7:0]     8 bits   ─┘
  6     Source Address[23:16]   8 bits   ─┐
  7     Source Address[15:8]    8 bits    │ 24-bit Source Address
  8     Source Address[7:0]     8 bits   ─┘
```

### 4.13 LCO $15 — Message Update (SF=1)

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($55)               8 bits
  1     Message[15:8]           8 bits   ─┐ 16-bit Message value
  2     Message[7:0]            8 bits   ─┘
  3     Target Address[23:16]   8 bits   ─┐
  4     Target Address[15:8]    8 bits    │
  5     Target Address[7:0]     8 bits   ─┘
  6     Source Address[23:16]   8 bits   ─┐
  7     Source Address[15:8]    8 bits    │
  8     Source Address[7:0]     8 bits   ─┘
```

### 4.14 LCO $16 — Call Alert (SF=1)

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($56)               8 bits
  1     Reserved[7:1] | S[0]    8 bits   S=Source ID Extension Required
  2     Reserved                8 bits
  3     Target Address[23:16]   8 bits   ─┐
  4     Target Address[15:8]    8 bits    │
  5     Target Address[7:0]     8 bits   ─┘
  6     Source Address[23:16]   8 bits   ─┐
  7     Source Address[15:8]    8 bits    │
  8     Source Address[7:0]     8 bits   ─┘
```

### 4.15 LCO $17 — Extended Function Command (SF=1)

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($57)               8 bits
  1     Extended Function[23:16] 8 bits  ─┐ 24-bit Extended Function
  2     Extended Function[15:8]  8 bits   │ (Class + Operand + Arguments)
  3     Extended Function[7:0]   8 bits  ─┘
  4     Reserved                 8 bits
  5     Reserved                 8 bits
  6     Target Address[23:16]    8 bits  ─┐
  7     Target Address[15:8]     8 bits   │
  8     Target Address[7:0]      8 bits  ─┘
```

> **NOTE:** The extraction shows octets 1-3 as one block and octets 4-5 as another.
> The exact boundary between Extended Function and Reserved fields within octets 1-5
> may differ from the 3+2 split shown here. The 24-bit Extended Function field contains
> Class, Operand, and Arguments subfields defined in TIA-102.AABC.

### 4.16 LCO $18 — Channel Identifier Update (SF=1)

Describes frequency band plan for a channel identifier. Trunked outbound.

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($58)               8 bits
  1     Identifier[7:4] | BW[11:8]   8 bits   4-bit Channel Ident + top 4 bits of BW
  2     BW[7:0]                      8 bits   Bottom 8 bits of Bandwidth (total 12 bits)
  3     Transmit Offset[13:8]        8 bits   ─┐ NOTE: exact bit widths per AABC
  4     Channel Spacing[9:8] | ...   8 bits    │
  5     Channel Spacing[7:0]         8 bits   ─┘
  6     Base Frequency[23:16]        8 bits   ─┐
  7     Base Frequency[15:8]         8 bits    │ 24-bit Base Frequency
  8     Base Frequency[7:0]          8 bits   ─┘
```

> **EXTRACTION CAVEAT:** The sub-field bit boundaries within octets 1-5 (Identifier,
> BW, Transmit Offset, Channel Spacing) require cross-reference with TIA-102.AABC
> for exact widths. The extraction captures the octet-level layout but the intra-octet
> boundaries are partially ambiguous from the PDF rendering.

### 4.17 LCO $19 — Channel Identifier Update VU (SF=1)

Same structure as LCO $18 but with VU (VHF/UHF) band plan parameters.

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($59)               8 bits
  1     Identifier[7:4] | BW_VU[11:8]  8 bits
  2     BW_VU[7:0]                     8 bits
  3     Transmit Offset VU             8 bits
  4     Channel Spacing[?:8]           8 bits
  5     Channel Spacing[7:0]           8 bits
  6     Base Frequency[23:16]          8 bits
  7     Base Frequency[15:8]           8 bits
  8     Base Frequency[7:0]            8 bits
```

### 4.18 LCO $1A — Status Update - Source ID Required (SF=1)

Same as LCO $14 but uses Source ID (not Source Address). Must be followed by LCO $09.

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($5A)               8 bits
  1     Status[15:8]            8 bits
  2     Status[7:0]             8 bits
  3     Target Address[23:16]   8 bits
  4     Target Address[15:8]    8 bits
  5     Target Address[7:0]     8 bits
  6     Source ID[23:16]        8 bits
  7     Source ID[15:8]         8 bits
  8     Source ID[7:0]          8 bits
```

### 4.19 LCO $1B — Message Update - Source ID Required (SF=1)

Same as LCO $15 but uses Source ID. Must be followed by LCO $09.

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($5B)               8 bits
  1     Message[15:8]           8 bits
  2     Message[7:0]            8 bits
  3     Target Address[23:16]   8 bits
  4     Target Address[15:8]    8 bits
  5     Target Address[7:0]     8 bits
  6     Source ID[23:16]        8 bits
  7     Source ID[15:8]         8 bits
  8     Source ID[7:0]          8 bits
```

### 4.20 LCO $1C — Extended Function Command - Source ID Required (SF=1)

Same as LCO $17 but must be followed by LCO $09.

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($5C)               8 bits
  1     Extended Function[23:16] 8 bits
  2     Extended Function[15:8]  8 bits
  3     Extended Function[7:0]   8 bits
  4     Reserved                 8 bits
  5     Reserved                 8 bits
  6     Target ID[23:16]         8 bits
  7     Target ID[15:8]          8 bits
  8     Target ID[7:0]           8 bits
```

### 4.21 LCO $20 — System Service Broadcast (SF=1)

Announces current system services on the primary control channel.

```
Octet   Field                       Width
------  --------------------------  ------
  0     LCF ($60)                   8 bits
  1     T_wuid_validity             8 bits   WUID validity timer
  2     Reserved[7:3] | Req Priority Lvl[2:0]  8 bits
  3     Reserved                    8 bits
  4     Sys Services Available[23:16] 8 bits ─┐
  5     Sys Services Available[15:8]  8 bits  │ 24-bit service availability bitmap
  6     Sys Services Available[7:0]   8 bits ─┘  (NOTE: extraction ambiguous on exact split)
  7     Sys Services Supported[15:8]  8 bits ─┐ services the site is equipped to handle
  8     Sys Services Supported[7:0]   8 bits ─┘
```

> **EXTRACTION CAVEAT:** The boundary between "Available" and "Supported" across
> octets 4-8 may not be exactly as shown. The original document shows octets 4-5 as
> "System Services Available" and octets 6-8 as "System Services Supported" (24 bits
> each), but that would require 6 octets for two 24-bit fields which exceeds the
> available space. Cross-reference with TIA-102.AABC for exact bit assignments.

### 4.22 LCO $21 — Secondary Control Channel Broadcast (SF=1)

Two secondary control channel entries per LC word.

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($61)               8 bits
  1     RFSS ID                 8 bits   RF Sub-System ID
  2     Site ID                 8 bits
  3     Channel-A[15:8]         8 bits   ─┐ 16-bit Channel A
  4     Channel-A[7:0]          8 bits   ─┘
  5     Sys Service Class-A     8 bits
  6     Channel-B[15:8]         8 bits   ─┐ 16-bit Channel B
  7     Channel-B[7:0]          8 bits   ─┘
  8     Sys Service Class-B     8 bits
```

### 4.23 LCO $22 — Adjacent Site Status Broadcast (SF=1)

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($62)               8 bits
  1     LRA                     8 bits   Location Registration Area
  2     C|F|V|A|SysID[11:8]     8 bits   4 flag bits + top 4 of System ID
  3     System ID[7:0]          8 bits
  4     RFSS ID                 8 bits
  5     Site ID                 8 bits
  6     Channel[15:8]           8 bits   ─┐ 16-bit Channel
  7     Channel[7:0]            8 bits   ─┘
  8     Sys Service Class       8 bits
```

Flag bits in octet 2:
- C (bit 7): Conventional channel (1=conventional, 0=trunked)
- F (bit 6): Site failure condition
- V (bit 5): Valid site
- A (bit 4): Active (site currently active)

### 4.24 LCO $23 — RFSS Status Broadcast (SF=1)

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($63)               8 bits
  1     LRA                     8 bits
  2     Reserved[7:4] | SysID[11:8]  8 bits
  3     System ID[7:0]          8 bits
  4     RFSS ID                 8 bits
  5     Site ID                 8 bits
  6     Channel[15:8]           8 bits   ─┐ Primary control channel
  7     Channel[7:0]            8 bits   ─┘
  8     Sys Service Class       8 bits
```

### 4.25 LCO $24 — Network Status Broadcast (SF=1)

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($64)               8 bits
  1     Reserved                8 bits
  2     Reserved                8 bits
  3     Network ID[19:12]       8 bits   ─┐
  4     Network ID[11:4] | SysID[11:8]  8 bits  │ 20-bit WACN + top of SysID
                                        ─┘
  5     System ID[7:0]          8 bits
  6     Channel[15:8]           8 bits   ─┐ Primary control channel
  7     Channel[7:0]            8 bits   ─┘
  8     Sys Service Class       8 bits
```

### 4.26 LCO $26 — Secondary CC Broadcast - Explicit (SF=1)

One secondary control channel entry with explicit T/R channels.

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($66)               8 bits
  1     RFSS ID                 8 bits
  2     Site ID                 8 bits
  3     Channel-T[15:8]         8 bits   ─┐ Transmit channel
  4     Channel-T[7:0]          8 bits   ─┘
  5     Channel-R[15:8]         8 bits   ─┐ Receive channel
  6     Channel-R[7:0]          8 bits   ─┘
  7     Sys Service Class       8 bits
  8     Reserved                8 bits
```

### 4.27 LCO $27 — Adjacent Site Status Broadcast - Explicit (SF=1)

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($67)               8 bits
  1     LRA                     8 bits
  2     Channel-T[15:8]         8 bits   ─┐ Transmit channel
  3     Channel-T[7:0]          8 bits   ─┘
  4     RFSS ID                 8 bits
  5     Site ID                 8 bits
  6     Channel-R[15:8]         8 bits   ─┐ Receive channel
  7     Channel-R[7:0]          8 bits   ─┘
  8     C|F|V|A|Reserved[3:0]   8 bits   Same flag bits as LCO $22
```

### 4.28 LCO $28 — RFSS Status Broadcast - Explicit (SF=1)

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($68)               8 bits
  1     LRA                     8 bits
  2     Channel-R[15:8]         8 bits   ─┐ Receive channel
  3     Channel-R[7:0]          8 bits   ─┘
  4     RFSS ID                 8 bits
  5     Site ID                 8 bits
  6     Channel-T[15:8]         8 bits   ─┐ Transmit channel
  7     Channel-T[7:0]          8 bits   ─┘
  8     Sys Service Class       8 bits
```

### 4.29 LCO $29 — Network Status Broadcast - Explicit (SF=1)

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($69)               8 bits
  1     Network ID[19:12]       8 bits   ─┐
  2     Network ID[11:4]        8 bits    │ 20-bit WACN
  3     Network ID[3:0] | SysID[11:8]  8 bits ─┘
  4     System ID[7:0]          8 bits
  5     Channel-T[15:8]         8 bits   ─┐ Transmit channel
  6     Channel-T[7:0]          8 bits   ─┘
  7     Channel-R[15:8]         8 bits   ─┐ Receive channel
  8     Channel-R[7:0]          8 bits   ─┘
```

### 4.30 LCO $2A — Conventional Fallback Indication (SF=0, MFID=$00)

```
Octet   Field                   Width
------  ----------------------  ------
  0     LCF ($2A)               8 bits   P=0, SF=0, LCO=$2A
  1     MFID ($00)              8 bits
  2     T|N|Reserved[5:0]       8 bits   T=Alert Tone Sync, N=Failsoft Network
  3     Fallback CH ID 1        8 bits   ─┐
  4     Fallback CH ID 2        8 bits    │
  5     Fallback CH ID 3        8 bits    │ Up to 6 fallback channel IDs
  6     Fallback CH ID 4        8 bits    │ (repeat if fewer than 6 needed)
  7     Fallback CH ID 5        8 bits    │
  8     Fallback CH ID 6        8 bits   ─┘
```

---

## 5. Extended LC Formats

### 5.1 Definition

AABF-D does not define a separate "Extended LC" format in the way BBAD-A defines
Extended MAC messages. The extension mechanism in AABF-D is the **Source ID Extension
(LCO $09)** which acts as a follow-on LC word.

When any LC message sets its **S flag** to 1, the next LC word transmitted in the
LDU1 sequence shall be LCO $09 (Source ID Extension). This two-LC-word pair provides
the equivalent of an "extended" identification.

Messages that can trigger Source ID Extension:
- LCO $00 (Group Voice Channel User) — S flag in octet 3, bit 0
- LCO $05 (Unit-to-Unit Answer Request) — S flag in octet 2, bit 0
- LCO $0A (Unit-to-Unit VCU Extended) — S always = 1
- LCO $13 (Status Query) — S flag in octet 1, bit 0
- LCO $16 (Call Alert) — S flag in octet 1, bit 0
- LCO $1A (Status Update - Src ID Req) — implicit, always followed by LCO $09
- LCO $1B (Message Update - Src ID Req) — implicit, always followed by LCO $09
- LCO $1C (Ext Function Cmd - Src ID Req) — implicit, always followed by LCO $09

### 5.2 Manufacturer-Specific Extended LC

When SF=0 and MFID is a non-standard value (not $00 or $01), the LC word carries
manufacturer-proprietary content. Known examples from scanner software:

| MFID | Manufacturer | Known LC Extensions |
|------|-------------|---------------------|
| $90  | Motorola    | Group Regroup (Motorola Patch), GPS Location |
| $A4  | Harris      | Group Regroup Explicit Encryption |

These are documented in SDRTrunk source code (see Section 8 cross-references) but
are not part of the TIA-102.AABF-D standard.

---

## 6. Subscriber Unit Address Space

24-bit address values used in Source Address, Target Address, and Source/Target ID fields:

```rust
const ADDR_NO_UNIT: u32         = 0x000000;  // No unit / placeholder
// 0x000001 - 0xFFFFFC = Assignable subscriber unit addresses
const ADDR_FNE_DISPATCH: u32    = 0xFFFFFD;  // Reserved for FNE dispatch functions
const ADDR_SYSTEM_DEFAULT: u32  = 0xFFFFFE;  // FNE call processing (registration, mobility)
const ADDR_ALL_UNITS: u32       = 0xFFFFFF;  // Broadcast to ALL subscriber units

// Registration Default address noted in spec — may overlap with System Default
// due to possible typo in original document. Implementations typically treat
// $FFFFFE as both "Registration Default" and "System Default".
```

**Group Address:** 16-bit value. $0000 = no group. $0001-$FFFF = assignable.
Group addresses are unique within a System.

---

## 7. Channel Field Encoding

The 16-bit Channel field used in LCO 2, 4, 22, 23, 24, 33, 34, 35, 36, 38-41:

```
Bit:   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0
     +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
     | Channel Identifier[3:0]     |          Channel Number[11:0]                   |
     +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
```

- **Channel Identifier** (4 bits, [15:12]): Selects a band plan / frequency table
  defined by Channel Identifier Update (LCO $18/$19)
- **Channel Number** (12 bits, [11:0]): Index into the frequency table for that identifier

Actual frequency = Base Frequency + (Channel Number * Channel Spacing) +/- Transmit Offset.

---

## 8. LCW Parser Pseudocode

```
function parse_lcw(data: [u8; 9]) -> LinkControlWord:
    // Step 1: Extract LCF (octet 0)
    lcf = data[0]
    p_flag   = (lcf >> 7) & 0x01       // Protected
    sf_flag  = (lcf >> 6) & 0x01       // Implicit/Explicit MFID
    lco      = lcf & 0x3F              // Link Control Opcode

    // Step 2: Handle encryption
    if p_flag == 1:
        // Octets 1-8 (SF=1) or 2-8 (SF=0) are encrypted
        // Cannot parse payload without decryption keys
        // Log as encrypted LC with LCO value
        return EncryptedLC { lco, raw: data }

    // Step 3: Determine MFID
    if sf_flag == 1:
        mfid = 0x00  // Implicit standard MFID
        payload_start = 1
    else:
        mfid = data[1]
        payload_start = 2
        if mfid != 0x00 and mfid != 0x01:
            // Non-standard manufacturer — cannot parse further
            return ManufacturerLC { lco, mfid, raw: data }

    // Step 4: Dispatch on LCO
    match lco:
        0x00:  // Group Voice Channel User
            service_options = data[payload_start]
            s_flag = data[payload_start + 1] & 0x01
            group_addr = (data[payload_start + 2] as u16) << 8
                       | (data[payload_start + 3] as u16)
            source_addr = (data[payload_start + 4] as u32) << 16
                        | (data[payload_start + 5] as u32) << 8
                        | (data[payload_start + 6] as u32)
            emergency = (service_options >> 7) & 0x01
            protected = (service_options >> 6) & 0x01
            priority  = service_options & 0x07
            return GroupVoiceChannelUser {
                mfid, service_options, emergency, protected,
                priority, s_flag, group_addr, source_addr
            }

        0x03:  // Unit-to-Unit Voice Channel User
            service_options = data[payload_start]
            target_addr = (data[payload_start + 1] as u32) << 16
                        | (data[payload_start + 2] as u32) << 8
                        | (data[payload_start + 3] as u32)
            source_addr = (data[payload_start + 4] as u32) << 16
                        | (data[payload_start + 5] as u32) << 8
                        | (data[payload_start + 6] as u32)
            return UnitToUnitVoiceChannelUser {
                mfid, service_options, target_addr, source_addr
            }

        0x02:  // Group Voice Channel Update
            ch_a = (data[1] as u16) << 8 | (data[2] as u16)
            grp_a = (data[3] as u16) << 8 | (data[4] as u16)
            ch_b = (data[5] as u16) << 8 | (data[6] as u16)
            grp_b = (data[7] as u16) << 8 | (data[8] as u16)
            return GroupVoiceChannelUpdate { ch_a, grp_a, ch_b, grp_b }

        0x09:  // Source ID Extension
            wacn = (data[2] as u32) << 12
                 | (data[3] as u32) << 4
                 | (data[4] as u32) >> 4
            sys_id = ((data[4] & 0x0F) as u16) << 8
                   | (data[5] as u16)
            unit_id = (data[6] as u32) << 16
                    | (data[7] as u32) << 8
                    | (data[8] as u32)
            return SourceIdExtension { wacn, sys_id, unit_id }

        0x0F:  // Call Termination
            addr = (data[6] as u32) << 16
                 | (data[7] as u32) << 8
                 | (data[8] as u32)
            return CallTermination { address: addr }

        0x20:  // System Service Broadcast
            t_wuid = data[1]
            req_priority = data[2] & 0x07
            // Remaining octets carry service bitmaps
            return SystemServiceBroadcast { t_wuid, req_priority, raw: data }

        0x22..=0x24:  // Adjacent/RFSS/Network Status
            return parse_status_broadcast(lco, data)

        _:
            if lco >= 0x2B:
                return Reserved { lco }
            return UnhandledLC { lco, raw: data }
```

---

## 9. Rust Type Definitions

```rust
/// Link Control Opcode values — all 64 possible values
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LinkControlOpcode {
    GroupVoiceChannelUser           = 0x00,
    // 0x01 reserved
    GroupVoiceChannelUpdate         = 0x02,
    UnitToUnitVoiceChannelUser      = 0x03,
    GroupVoiceChannelUpdateExplicit = 0x04,
    UnitToUnitAnswerRequest         = 0x05,
    TelephoneInterconnectVCU        = 0x06,
    TelephoneInterconnectAnsReq     = 0x07,
    // 0x08 reserved
    SourceIdExtension               = 0x09,
    UnitToUnitVCUExtended           = 0x0A,
    // 0x0B-0x0E reserved
    CallTermination                 = 0x0F,
    GroupAffiliationQuery           = 0x10,
    UnitRegistrationCommand         = 0x11,
    // 0x12 reserved (obsolete Unit Auth Cmd)
    StatusQuery                     = 0x13,
    StatusUpdate                    = 0x14,
    MessageUpdate                   = 0x15,
    CallAlert                       = 0x16,
    ExtendedFunctionCommand         = 0x17,
    ChannelIdentifierUpdate         = 0x18,
    ChannelIdentifierUpdateVU       = 0x19,
    StatusUpdateSrcIdReq            = 0x1A,
    MessageUpdateSrcIdReq           = 0x1B,
    ExtFunctionCmdSrcIdReq          = 0x1C,
    // 0x1D-0x1F reserved
    SystemServiceBroadcast          = 0x20,
    SecondaryControlChannelBcast    = 0x21,
    AdjacentSiteStatusBroadcast     = 0x22,
    RfssStatusBroadcast             = 0x23,
    NetworkStatusBroadcast          = 0x24,
    // 0x25 reserved (obsolete Protection Param Bcast)
    SecondaryControlChannelBcastExp = 0x26,
    AdjacentSiteStatusBcastExp      = 0x27,
    RfssStatusBroadcastExplicit     = 0x28,
    NetworkStatusBroadcastExplicit  = 0x29,
    ConventionalFallback            = 0x2A,
    // 0x2B-0x3F reserved
}

impl LinkControlOpcode {
    pub fn from_u8(val: u8) -> Option<Self> {
        match val {
            0x00 => Some(Self::GroupVoiceChannelUser),
            0x02 => Some(Self::GroupVoiceChannelUpdate),
            0x03 => Some(Self::UnitToUnitVoiceChannelUser),
            0x04 => Some(Self::GroupVoiceChannelUpdateExplicit),
            0x05 => Some(Self::UnitToUnitAnswerRequest),
            0x06 => Some(Self::TelephoneInterconnectVCU),
            0x07 => Some(Self::TelephoneInterconnectAnsReq),
            0x09 => Some(Self::SourceIdExtension),
            0x0A => Some(Self::UnitToUnitVCUExtended),
            0x0F => Some(Self::CallTermination),
            0x10 => Some(Self::GroupAffiliationQuery),
            0x11 => Some(Self::UnitRegistrationCommand),
            0x13 => Some(Self::StatusQuery),
            0x14 => Some(Self::StatusUpdate),
            0x15 => Some(Self::MessageUpdate),
            0x16 => Some(Self::CallAlert),
            0x17 => Some(Self::ExtendedFunctionCommand),
            0x18 => Some(Self::ChannelIdentifierUpdate),
            0x19 => Some(Self::ChannelIdentifierUpdateVU),
            0x1A => Some(Self::StatusUpdateSrcIdReq),
            0x1B => Some(Self::MessageUpdateSrcIdReq),
            0x1C => Some(Self::ExtFunctionCmdSrcIdReq),
            0x20 => Some(Self::SystemServiceBroadcast),
            0x21 => Some(Self::SecondaryControlChannelBcast),
            0x22 => Some(Self::AdjacentSiteStatusBroadcast),
            0x23 => Some(Self::RfssStatusBroadcast),
            0x24 => Some(Self::NetworkStatusBroadcast),
            0x26 => Some(Self::SecondaryControlChannelBcastExp),
            0x27 => Some(Self::AdjacentSiteStatusBcastExp),
            0x28 => Some(Self::RfssStatusBroadcastExplicit),
            0x29 => Some(Self::NetworkStatusBroadcastExplicit),
            0x2A => Some(Self::ConventionalFallback),
            _ => None,
        }
    }
}

/// Service Options byte — carried in voice channel user LC messages
#[derive(Debug, Clone, Copy)]
pub struct ServiceOptions {
    pub emergency: bool,       // bit 7 — emergency status
    pub protected: bool,       // bit 6 — encrypted resources
    pub duplex: bool,          // bit 5 — full duplex capable
    pub packet_mode: bool,     // bit 4 — packet (vs circuit) mode
    pub priority: u8,          // bits 2:0 — priority level 0-7
}

impl ServiceOptions {
    pub fn from_u8(val: u8) -> Self {
        Self {
            emergency: (val & 0x80) != 0,
            protected: (val & 0x40) != 0,
            duplex: (val & 0x20) != 0,
            packet_mode: (val & 0x10) != 0,
            priority: val & 0x07,
        }
    }

    pub fn to_u8(&self) -> u8 {
        let mut val = self.priority & 0x07;
        if self.emergency { val |= 0x80; }
        if self.protected { val |= 0x40; }
        if self.duplex { val |= 0x20; }
        if self.packet_mode { val |= 0x10; }
        val
    }
}

/// Link Control Format header — octet 0 of every LCW
#[derive(Debug, Clone, Copy)]
pub struct LinkControlFormat {
    pub protected: bool,              // P flag (bit 7)
    pub implicit_mfid: bool,          // SF flag (bit 6) — true = implicit MFID $00
    pub opcode: u8,                   // LCO (bits 5:0)
    pub raw: u8,                      // Full LCF byte
}

impl LinkControlFormat {
    pub fn from_u8(val: u8) -> Self {
        Self {
            protected: (val & 0x80) != 0,
            implicit_mfid: (val & 0x40) != 0,
            opcode: val & 0x3F,
            raw: val,
        }
    }
}

/// Parsed Link Control Word — the 72-bit LC payload after error correction
#[derive(Debug, Clone)]
pub enum LinkControlWord {
    /// LCO 0x00: Group Voice Channel User
    GroupVoiceChannelUser {
        lcf: LinkControlFormat,
        mfid: u8,
        service_options: ServiceOptions,
        source_id_extension_required: bool,  // S flag
        group_address: u16,                   // 16-bit TGID
        source_address: u32,                  // 24-bit WUID
    },

    /// LCO 0x02: Group Voice Channel Update (two channel/group pairs)
    GroupVoiceChannelUpdate {
        lcf: LinkControlFormat,
        channel_a: u16,
        group_address_a: u16,
        channel_b: u16,
        group_address_b: u16,
    },

    /// LCO 0x03: Unit-to-Unit Voice Channel User
    UnitToUnitVoiceChannelUser {
        lcf: LinkControlFormat,
        mfid: u8,
        service_options: ServiceOptions,
        target_address: u32,                  // 24-bit target WUID
        source_address: u32,                  // 24-bit source WUID
    },

    /// LCO 0x04: Group Voice Channel Update - Explicit
    GroupVoiceChannelUpdateExplicit {
        lcf: LinkControlFormat,
        service_options: ServiceOptions,
        group_address: u16,
        channel_t: u16,                       // Transmit channel
        channel_r: u16,                       // Receive channel
    },

    /// LCO 0x05: Unit-to-Unit Answer Request
    UnitToUnitAnswerRequest {
        lcf: LinkControlFormat,
        service_options: ServiceOptions,
        source_id_extension_required: bool,
        target_address: u32,
        source_address: u32,
    },

    /// LCO 0x06: Telephone Interconnect Voice Channel User
    TelephoneInterconnectVCU {
        lcf: LinkControlFormat,
        service_options: ServiceOptions,
        call_timer: u16,                      // 100ms units, 0=unlimited
        address: u32,                         // source (inbound) or target (outbound)
    },

    /// LCO 0x07: Telephone Interconnect Answer Request
    TelephoneInterconnectAnswerRequest {
        lcf: LinkControlFormat,
        digits: [u8; 10],                     // BCD digits 1-10
        target_address: u32,
    },

    /// LCO 0x09: Source ID Extension
    SourceIdExtension {
        lcf: LinkControlFormat,
        wacn: u32,                            // 20-bit WACN (Network ID)
        system_id: u16,                       // 12-bit System ID
        source_id: u32,                       // 24-bit Unit ID
    },

    /// LCO 0x0A: Unit-to-Unit Voice Channel User - Extended
    UnitToUnitVCUExtended {
        lcf: LinkControlFormat,
        service_options: ServiceOptions,
        source_id_extension_required: bool,   // Always true
        target_address: u32,
        source_id: u32,
    },

    /// LCO 0x0F: Call Termination/Cancellation
    CallTermination {
        lcf: LinkControlFormat,
        address: u32,                         // source (inbound) or target (outbound)
    },

    /// LCO 0x10: Group Affiliation Query
    GroupAffiliationQuery {
        lcf: LinkControlFormat,
        target_address: u32,
        source_address: u32,
    },

    /// LCO 0x11: Unit Registration Command
    UnitRegistrationCommand {
        lcf: LinkControlFormat,
        wacn: u32,                            // 20-bit Network ID
        system_id: u16,                       // 12-bit System ID
        target_id: u32,                       // 24-bit target
    },

    /// LCO 0x13: Status Query
    StatusQuery {
        lcf: LinkControlFormat,
        source_id_extension_required: bool,
        target_address: u32,
        source_address: u32,
    },

    /// LCO 0x14: Status Update
    StatusUpdate {
        lcf: LinkControlFormat,
        status: u16,
        target_address: u32,
        source_address: u32,
    },

    /// LCO 0x15: Message Update
    MessageUpdate {
        lcf: LinkControlFormat,
        message: u16,
        target_address: u32,
        source_address: u32,
    },

    /// LCO 0x16: Call Alert
    CallAlert {
        lcf: LinkControlFormat,
        source_id_extension_required: bool,
        target_address: u32,
        source_address: u32,
    },

    /// LCO 0x17: Extended Function Command
    ExtendedFunctionCommand {
        lcf: LinkControlFormat,
        extended_function: u32,               // 24-bit (Class + Operand + Args)
        target_address: u32,
    },

    /// LCO 0x18: Channel Identifier Update
    ChannelIdentifierUpdate {
        lcf: LinkControlFormat,
        identifier: u8,                       // 4-bit channel identifier
        bandwidth: u16,                       // bandwidth value
        transmit_offset: u16,                 // TX offset
        channel_spacing: u16,                 // channel spacing
        base_frequency: u32,                  // 24-bit base frequency
    },

    /// LCO 0x19: Channel Identifier Update VU
    ChannelIdentifierUpdateVU {
        lcf: LinkControlFormat,
        identifier: u8,
        bandwidth_vu: u16,
        transmit_offset_vu: u16,
        channel_spacing: u16,
        base_frequency: u32,
    },

    /// LCO 0x1A: Status Update - Source ID Required
    StatusUpdateSrcIdReq {
        lcf: LinkControlFormat,
        status: u16,
        target_address: u32,
        source_id: u32,
    },

    /// LCO 0x1B: Message Update - Source ID Required
    MessageUpdateSrcIdReq {
        lcf: LinkControlFormat,
        message: u16,
        target_address: u32,
        source_id: u32,
    },

    /// LCO 0x1C: Extended Function Command - Source ID Required
    ExtFunctionCmdSrcIdReq {
        lcf: LinkControlFormat,
        extended_function: u32,
        target_id: u32,
    },

    /// LCO 0x20: System Service Broadcast
    SystemServiceBroadcast {
        lcf: LinkControlFormat,
        t_wuid_validity: u8,
        request_priority_level: u8,           // 3-bit
        services_available: u32,              // 24-bit bitmap (approximate)
        services_supported: u32,              // 24-bit bitmap (approximate)
    },

    /// LCO 0x21: Secondary Control Channel Broadcast
    SecondaryControlChannelBroadcast {
        lcf: LinkControlFormat,
        rfss_id: u8,
        site_id: u8,
        channel_a: u16,
        service_class_a: u8,
        channel_b: u16,
        service_class_b: u8,
    },

    /// LCO 0x22: Adjacent Site Status Broadcast
    AdjacentSiteStatusBroadcast {
        lcf: LinkControlFormat,
        lra: u8,
        conventional: bool,                   // C flag
        site_failure: bool,                   // F flag
        valid: bool,                          // V flag
        active: bool,                         // A flag
        system_id: u16,                       // 12-bit
        rfss_id: u8,
        site_id: u8,
        channel: u16,
        service_class: u8,
    },

    /// LCO 0x23: RFSS Status Broadcast
    RfssStatusBroadcast {
        lcf: LinkControlFormat,
        lra: u8,
        system_id: u16,                       // 12-bit
        rfss_id: u8,
        site_id: u8,
        channel: u16,                         // Primary control channel
        service_class: u8,
    },

    /// LCO 0x24: Network Status Broadcast
    NetworkStatusBroadcast {
        lcf: LinkControlFormat,
        wacn: u32,                            // 20-bit Network ID
        system_id: u16,                       // 12-bit
        channel: u16,                         // Primary control channel
        service_class: u8,
    },

    /// LCO 0x26: Secondary CC Broadcast - Explicit
    SecondaryControlChannelBcastExplicit {
        lcf: LinkControlFormat,
        rfss_id: u8,
        site_id: u8,
        channel_t: u16,
        channel_r: u16,
        service_class: u8,
    },

    /// LCO 0x27: Adjacent Site Status Broadcast - Explicit
    AdjacentSiteStatusBcastExplicit {
        lcf: LinkControlFormat,
        lra: u8,
        channel_t: u16,
        rfss_id: u8,
        site_id: u8,
        channel_r: u16,
        conventional: bool,
        site_failure: bool,
        valid: bool,
        active: bool,
    },

    /// LCO 0x28: RFSS Status Broadcast - Explicit
    RfssStatusBroadcastExplicit {
        lcf: LinkControlFormat,
        lra: u8,
        channel_r: u16,
        rfss_id: u8,
        site_id: u8,
        channel_t: u16,
        service_class: u8,
    },

    /// LCO 0x29: Network Status Broadcast - Explicit
    NetworkStatusBroadcastExplicit {
        lcf: LinkControlFormat,
        wacn: u32,                            // 20-bit
        system_id: u16,                       // 12-bit
        channel_t: u16,
        channel_r: u16,
    },

    /// LCO 0x2A: Conventional Fallback
    ConventionalFallback {
        lcf: LinkControlFormat,
        mfid: u8,
        alert_tone: bool,                     // T flag
        failsoft_network: bool,               // N flag
        fallback_channel_ids: [u8; 6],
    },

    /// Encrypted LC — cannot parse payload
    Encrypted {
        lcf: LinkControlFormat,
        raw_payload: [u8; 8],                 // octets 1-8
    },

    /// Manufacturer-specific LC (non-standard MFID)
    ManufacturerSpecific {
        lcf: LinkControlFormat,
        mfid: u8,
        payload: [u8; 7],                     // octets 2-8
    },

    /// Reserved or unrecognized LCO
    Reserved {
        lcf: LinkControlFormat,
        raw: [u8; 9],
    },
}
```

---

## 10. LCF Full-Byte Quick Reference

The full LCF byte (octet 0) values seen in practice. This table is what SDRTrunk
and OP25 use for primary dispatch.

**Unencrypted (P=0):**

| LCF Byte | P | SF | LCO | Message |
|----------|---|-----|-----|---------|
| `$00`    | 0 | 0   | $00 | Group Voice Channel User (explicit MFID) |
| `$03`    | 0 | 0   | $03 | Unit-to-Unit Voice Channel User (explicit MFID) |
| `$2A`    | 0 | 0   | $2A | Conventional Fallback (explicit MFID=$00) |
| `$42`    | 0 | 1   | $02 | Group Voice Channel Update |
| `$44`    | 0 | 1   | $04 | Group Voice Ch Update - Explicit |
| `$45`    | 0 | 1   | $05 | Unit-to-Unit Answer Request |
| `$46`    | 0 | 1   | $06 | Telephone Interconnect VCU |
| `$47`    | 0 | 1   | $07 | Telephone Interconnect Answer Req |
| `$49`    | 0 | 1   | $09 | Source ID Extension |
| `$4A`    | 0 | 1   | $0A | Unit-to-Unit VCU - Extended |
| `$4F`    | 0 | 1   | $0F | Call Termination/Cancellation |
| `$50`    | 0 | 1   | $10 | Group Affiliation Query |
| `$51`    | 0 | 1   | $11 | Unit Registration Command |
| `$53`    | 0 | 1   | $13 | Status Query |
| `$54`    | 0 | 1   | $14 | Status Update |
| `$55`    | 0 | 1   | $15 | Message Update |
| `$56`    | 0 | 1   | $16 | Call Alert |
| `$57`    | 0 | 1   | $17 | Extended Function Command |
| `$58`    | 0 | 1   | $18 | Channel Identifier Update |
| `$59`    | 0 | 1   | $19 | Channel Identifier Update VU |
| `$5A`    | 0 | 1   | $1A | Status Update - Src ID Req |
| `$5B`    | 0 | 1   | $1B | Message Update - Src ID Req |
| `$5C`    | 0 | 1   | $1C | Ext Function Cmd - Src ID Req |
| `$60`    | 0 | 1   | $20 | System Service Broadcast |
| `$61`    | 0 | 1   | $21 | Secondary CC Broadcast |
| `$62`    | 0 | 1   | $22 | Adjacent Site Status Broadcast |
| `$63`    | 0 | 1   | $23 | RFSS Status Broadcast |
| `$64`    | 0 | 1   | $24 | Network Status Broadcast |
| `$66`    | 0 | 1   | $26 | Secondary CC Broadcast - Explicit |
| `$67`    | 0 | 1   | $27 | Adjacent Site Status Bcast - Explicit |
| `$68`    | 0 | 1   | $28 | RFSS Status Broadcast - Explicit |
| `$69`    | 0 | 1   | $29 | Network Status Broadcast - Explicit |

**Encrypted (P=1):** Add `$80` to the unencrypted LCF values above.

| LCF Byte | Message |
|----------|---------|
| `$80`    | Group Voice Channel User (encrypted) |
| `$83`    | Unit-to-Unit Voice Channel User (encrypted) |
| `$C2`    | Group Voice Channel Update (encrypted) |
| etc.     | Same pattern for all LCO values |

---

## 11. SDRTrunk and OP25 Cross-References

### 11.1 SDRTrunk

SDRTrunk parses FDMA LCWs in its P25 decoder module. Key source files:

- **LinkControlWord.java** / **LinkControlWordFactory.java**: Factory-dispatches on the
  full LCF byte (octet 0) to create typed LC message objects.
- **GroupVoiceChannelUser.java**: Extracts MFID, Service Options, Group Address, Source
  Address from the 72-bit LC payload. Emergency flag is extracted from Service Options bit 7.
- **UnitToUnitVoiceChannelUser.java**: Similar extraction for private calls.
- **LCMessageFilter**: Filters LCW types for display.

SDRTrunk treats the full LCF byte as the primary dispatch value, matching this spec's
Section 10 table. It handles Motorola MFID $90 LC words (Group Regroup/Patch) as
manufacturer-specific extensions outside the standard dispatch.

### 11.2 OP25

OP25 parses LCWs in its `p25_frame_assembler` and `p25p1_fdma` modules:

- **p25p1_fdma.cc**: Extracts the 72-bit LC from LDU1 frames after RS decoding.
  The LCF byte is used for dispatch. LCO 0 (Group Voice) and LCO 3 (Unit-to-Unit)
  are the primary handlers that emit talkgroup/source metadata.
- **tsbk.cc** (related): Shares field extraction logic with LC words since many
  TSBK (Trunking Signalling Block) fields have the same layout as LC fields.

Both projects confirm that the most frequently encountered LC words in practice are:
1. LCO $00 (Group Voice Channel User) — on virtually every group voice call
2. LCO $02 (Group Voice Channel Update) — on trunked systems during active calls
3. LCO $0F (Call Termination) — in ETDU frames
4. LCO $23/$24 (RFSS/Network Status Broadcast) — system identity announcements

### 11.3 TDMA Relationship Summary

For implementers handling both Phase 1 (FDMA) and Phase 2 (TDMA):

| FDMA LC (this spec) | TDMA MAC (BBAD-A) | Notes |
|---------------------|-------------------|-------|
| LCO $00 Group VCU   | B1B2=00 MCO=0x01 (Abbreviated, 7 bytes) | Same semantic: group + source + service opts |
| LCO $00 Group VCU   | B1B2=00 MCO=0x21 (Extended, 14 bytes) | Extended includes WACN/SysID |
| LCO $03 Unit-Unit VCU | B1B2=00 MCO=0x02 (Abbreviated, 8 bytes) | Same semantic: target + source |
| LCO $03 Unit-Unit VCU | B1B2=00 MCO=0x22 (Extended, 15 bytes) | Extended includes WACN/SysID |
| LCO $06 Tel Intercon | B1B2=00 MCO=0x03 (7 bytes) | |
| LCO $0F Call Term    | B1B2=00 MCO=0x31 MAC_Release (7 bytes) | Different opcode, same function |
| MAC_PTT PDU (N/A)    | Opcode=%001 PDU | Carries MI+ALGID+KeyID+Source+Group — combines HDU+LCO$00 |
| MAC_END_PTT PDU (N/A)| Opcode=%010 PDU | Carries Source+Group like a termination LC |

A unified implementation should map both FDMA LC words and TDMA MAC VCU messages
to a common internal representation (e.g., `CallMetadata { group, source, emergency,
encrypted, ... }`).

---

## 12. Extraction Gaps and Caveats

The following items could not be fully verified from the PDF extraction and may need
manual verification against a clean copy of TIA-102.AABF-D:

1. **LCO $07 (Telephone Interconnect Answer Request):** Digit numbering in octet 5
   is ambiguous in the PDF — rendering artifact may obscure digit 9 vs digit 8.

2. **LCO $11 (Unit Registration Command):** The exact byte boundary between Network ID,
   System ID, Target ID, and Reserved fields in octets 5-8 needs verification.

3. **LCO $20 (System Service Broadcast):** The split between "Services Available" (24 bits)
   and "Services Supported" (24 bits) across octets 3-8 is ambiguous — the 48 total bits
   do not cleanly fit 5 octets. The extraction may have missed a boundary detail or the
   fields may have different widths than described in the summary. Cross-reference with
   TIA-102.AABC for exact bit assignments.

4. **LCO $18/$19 (Channel Identifier Update / VU):** The intra-octet bit boundaries for
   Identifier, BW, Transmit Offset, and Channel Spacing fields require cross-reference
   with TIA-102.AABC/AABB for exact widths. The extraction captures octet-level layout only.

5. **Subscriber Unit Address table:** The original document may have overlapping entries
   for $FFFFFE (Registration Default vs System Default). Extracted as shown in the PDF
   with a note about possible typo.

6. **ETDU frame format details:** The LC embedding in ETDU frames is defined in TIA-102.BAAA,
   not AABF-D. This spec covers the LC content only, not the frame-level encoding.

7. **Encryption details:** The encryption algorithm applied to LC payload octets (when P=1)
   is defined in TIA-102.AAAD (Block Encryption Protocol). AABF-D only specifies which
   octets are encrypted, not how.
