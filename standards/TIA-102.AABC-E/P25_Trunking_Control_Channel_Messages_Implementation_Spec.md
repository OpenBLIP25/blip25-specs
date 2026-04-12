# P25 FDMA Trunking Control Channel Messages -- Implementation Specification

**Source:** TIA-102.AABC-E (April 2019), "Project 25 -- Trunking Control Channel Messages"
**Phase:** 3 -- Implementation-ready
**Classification:** MESSAGE_FORMAT + PROTOCOL
**Extracted:** 2026-04-12
**Purpose:** Self-contained spec for implementing a complete P25 FDMA trunking control channel
message parser/encoder. Covers TSBK structure, all ISP/OSP opcodes, MBT multi-block format,
channel identifier encoding, and cross-reference to TDMA MAC messages (TIA-102.BBAD-A).
No reference to the original PDF required.

---

## 1. TSBK (Trunking Signaling Block) Structure

### 1.1 Single-Block Format

Every TSBK is exactly 12 octets (96 bits):

```
Byte:  0         1         2         3         4         5         6         7         8         9        10        11
     +----+----+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+
     |LB|P|Opcode(5:0)| MFID   |  Message-Specific Payload (octets 2-9)                                | CRC-16 |
     +----+----+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+
```

Bit-level layout of Octet 0:

```
Bit:    7     6     5     4     3     2     1     0
      +-----+-----+-----+-----+-----+-----+-----+-----+
      | LB  |  P  | Opcode[5:0]                         |
      +-----+-----+-----+-----+-----+-----+-----+-----+
```

### 1.2 Field Definitions

| Field  | Bits    | Description |
|--------|---------|-------------|
| LB     | Octet 0, bit 7 | Last Block. `1` = last TSBK in a multi-TSBK burst; `0` = more TSBKs follow |
| P      | Octet 0, bit 6 | Protected. `1` = associated voice traffic channel is encrypted |
| Opcode | Octet 0, bits 5:0 | 6-bit message type identifier (0x00-0x3F) |
| MFID   | Octet 1 | Manufacturer's ID. `0x00` = standard TIA message |
| Payload | Octets 2-9 | 8 octets (64 bits) of message-specific data |
| CRC    | Octets 10-11 | CCITT CRC-16 computed over octets 0-9 |

### 1.3 CRC-16 Calculation

The CRC uses the CCITT polynomial:

```
Polynomial: x^16 + x^12 + x^5 + 1  (0x1021)
Initial value: 0xFFFF
Input: octets 0-9 (80 bits)
Output: 16-bit CRC stored MSB-first in octets 10-11
No final XOR (standard CCITT)
```

```rust
/// Compute CCITT CRC-16 for a TSBK (octets 0..9)
fn tsbk_crc16(data: &[u8; 10]) -> u16 {
    let mut crc: u16 = 0xFFFF;
    for &byte in data.iter() {
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

/// Validate a complete 12-byte TSBK
fn tsbk_crc_valid(tsbk: &[u8; 12]) -> bool {
    let computed = tsbk_crc16(tsbk[0..10].try_into().unwrap());
    let stored = ((tsbk[10] as u16) << 8) | (tsbk[11] as u16);
    computed == stored
}
```

### 1.4 Rust TSBK Header Extraction

```rust
/// Parsed TSBK header fields
#[derive(Debug, Clone, Copy)]
pub struct TsbkHeader {
    pub last_block: bool,
    pub protected: bool,
    pub opcode: u8,       // 6-bit, 0x00..0x3F
    pub mfid: u8,
}

impl TsbkHeader {
    pub fn from_bytes(data: &[u8; 12]) -> Self {
        TsbkHeader {
            last_block: (data[0] & 0x80) != 0,
            protected:  (data[0] & 0x40) != 0,
            opcode:      data[0] & 0x3F,
            mfid:        data[1],
        }
    }

    /// True if this is a standard TIA-defined message (not manufacturer-specific)
    pub fn is_standard(&self) -> bool {
        self.mfid == 0x00
    }
}
```

---

## 2. MBT (Multiple Block Trunking) Structure

### 2.1 Overview

MBT is used when a single TSBK cannot carry the full message (e.g., roaming with full
56-bit SUIDs). An MBT packet consists of a 12-octet Header Block followed by 1-3
Data Blocks of 12 octets each.

### 2.2 Header Block Format

```
Byte:  0             1                2      3-5          6              7            8-9       10-11
     +-+--+--+-----+--+--+----------+------+------------+--+-----------+--+--+-------+---------+-------+
     |0|AN|IO|Fmt  |1 |1 |SAP ID    | MFID | Addr(23:0) |1 |BlkFollow  |0 |0 |Opcode | Msg-Spc | H-CRC |
     +-+--+--+-----+--+--+----------+------+------------+--+-----------+--+--+-------+---------+-------+
```

| Field | Location | Description |
|-------|----------|-------------|
| Bit 7 of Oct 0 | Oct 0[7] | Always `0` |
| AN | Oct 0[6] | Always `1` for MBT headers |
| IO | Oct 0[5] | Direction: `0` = ISP (SU->FNE), `1` = OSP (FNE->SU) |
| Format | Oct 0[4:0] | `0b10111` (0x17) = Alternate MBT Control (AMBTC) |
| SAP ID | Oct 1[5:0] | Service Access Point; Oct 1 bits 7:6 = `11` for trunking |
| MFID | Oct 2 | Manufacturer's ID (`0x00` = standard) |
| Address | Oct 3-5 | 24-bit Source Address (ISP) or Target Address (OSP) |
| Blocks to Follow | Oct 6[6:0] | Count of Data Blocks after header (1-3); Oct 6[7] = `1` |
| Opcode | Oct 7[5:0] | 6-bit message opcode; Oct 7 bits 7:6 = `00` |
| Message-Specific | Oct 8-9 | Additional fields |
| Header CRC | Oct 10-11 | CRC-16 over octets 0-9 |

### 2.3 Data Block Format

```
Byte:  0-9                   10-11
     +----------------------+--------+
     | Message-Specific     | CRC    |
     +----------------------+--------+
```

The last Data Block carries either a Packet CRC (single data block) or Multiple Block CRC
(multi data block) in octets 10-11.

### 2.4 Abbreviated vs. Extended Selection Rule

| Condition | Format |
|-----------|--------|
| Source and target share same Home system, source is at Home | **Abbreviated (TSBK)** -- 24-bit WUIDs |
| Source or target is roaming, or different Home systems | **Extended (MBT)** -- full 56-bit SUIDs |

SUID = WACN ID (20 bits) + System ID (12 bits) + Unit ID (24 bits) = 56 bits total.

```rust
/// Full P25 Subscriber Unit ID
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Suid {
    pub wacn_id: u32,    // 20 bits
    pub system_id: u16,  // 12 bits
    pub unit_id: u32,    // 24 bits
}

impl Suid {
    /// Pack into a u64 (bits 55:36 = WACN, 35:24 = SysID, 23:0 = UnitID)
    pub fn to_u64(&self) -> u64 {
        ((self.wacn_id as u64 & 0xFFFFF) << 36)
            | ((self.system_id as u64 & 0xFFF) << 24)
            | (self.unit_id as u64 & 0xFFFFFF)
    }
}
```

---

## 3. Common Field Definitions

### 3.1 Channel Field (16 bits)

The Channel field is the critical link between a channel grant and actual RF parameters.

```
Bits 15:12  Channel Identifier (4-bit) -- references an IDEN_UP broadcast entry
Bits 11:0   Channel Number (12-bit)    -- offset index within the identifier table
```

```rust
/// Decode a 16-bit channel field into identifier and number
#[derive(Debug, Clone, Copy)]
pub struct ChannelField {
    pub identifier: u8,    // 4-bit (0-15)
    pub number: u16,       // 12-bit (0-4095)
}

impl ChannelField {
    pub fn from_u16(val: u16) -> Self {
        ChannelField {
            identifier: ((val >> 12) & 0x0F) as u8,
            number: val & 0x0FFF,
        }
    }

    pub fn to_u16(&self) -> u16 {
        ((self.identifier as u16 & 0x0F) << 12) | (self.number & 0x0FFF)
    }
}
```

### 3.2 Service Options (8 bits)

Used in voice/data service requests and grants (typically Octet 2).

```
Bit 7: Emergency       (E)  -- 1 = emergency call
Bit 6: Protected       (P)  -- 1 = encrypted channel requested
Bit 5: Duplex          (D)  -- 1 = full-duplex mode
Bit 4: Packet Mode     (M)  -- 1 = packet data mode
Bit 3: Reserved
Bit 2: Reserved
Bits 1:0: Priority     (0 = lowest, 3 = highest)
```

```rust
#[derive(Debug, Clone, Copy)]
pub struct ServiceOptions {
    pub emergency: bool,
    pub protected: bool,
    pub duplex: bool,
    pub packet_mode: bool,
    pub priority: u8,   // 0-3
}

impl ServiceOptions {
    pub fn from_byte(b: u8) -> Self {
        ServiceOptions {
            emergency:   (b & 0x80) != 0,
            protected:   (b & 0x40) != 0,
            duplex:      (b & 0x20) != 0,
            packet_mode: (b & 0x10) != 0,
            priority:     b & 0x03,
        }
    }

    pub fn to_byte(&self) -> u8 {
        (if self.emergency   { 0x80 } else { 0 })
        | (if self.protected { 0x40 } else { 0 })
        | (if self.duplex    { 0x20 } else { 0 })
        | (if self.packet_mode { 0x10 } else { 0 })
        | (self.priority & 0x03)
    }
}
```

### 3.3 Address Types

| Type | Width | Usage |
|------|-------|-------|
| Source Address (abbreviated) | 24-bit | WUID assigned at registration |
| Target Address (abbreviated) | 24-bit | WUID of destination unit |
| Group Address (abbreviated) | 24-bit | WGID (Working Group ID) |
| SUID (extended) | 56-bit | WACN(20) + SysID(12) + UnitID(24) |

**Common extraction positions in TSBK messages:**
- Source Address: typically octets 8-9 (16-bit abbreviated in some messages) or octets 7-9 (24-bit)
- Target Address: typically octets 4-6 (24-bit)
- Group Address: typically octets 5-7 (24-bit)

NOTE: Some messages carry only 16 bits of source address in octets 8-9. This is an artifact
of the extraction; the full 24-bit source address spans octets 7-9 in most messages. The
bit-level layouts in Section 4/5 should be treated as authoritative.

### 3.4 Capabilities Field (8 bits)

```
Bit 7: Reserved (or E in some messages)
Bit 6: V     -- Valid (1 = capabilities field is valid)
Bit 5: 2HR   -- Half-rate 2-slot TDMA capable
Bit 4: 4HR   -- Half-rate 4-slot TDMA capable
Bit 3: HF    -- Half-rate FDMA capable
Bit 2: 8LVL  -- H-D8PSK TDMA capable
Bits 1:0: Reserved
```

```rust
#[derive(Debug, Clone, Copy)]
pub struct Capabilities {
    pub valid: bool,
    pub tdma_2slot: bool,
    pub tdma_4slot: bool,
    pub fdma_halfrate: bool,
    pub tdma_8psk: bool,
}

impl Capabilities {
    pub fn from_byte(b: u8) -> Self {
        Capabilities {
            valid:         (b & 0x40) != 0,
            tdma_2slot:    (b & 0x20) != 0,
            tdma_4slot:    (b & 0x10) != 0,
            fdma_halfrate: (b & 0x08) != 0,
            tdma_8psk:     (b & 0x04) != 0,
        }
    }
}
```

### 3.5 Channel Type (4 bits)

```rust
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum ChannelType {
    Fdma12_5kHzHalfRate  = 0x0,  // FDMA 12.5 kHz, 1-slot half-rate
    Fdma12_5kHzFullRate  = 0x1,  // FDMA 12.5 kHz, 1-slot full-rate
    Fdma6_25kHzHalfRate  = 0x2,  // FDMA 6.25 kHz, 1-slot half-rate
    Tdma12_5kHz2Slot     = 0x3,  // TDMA 12.5 kHz, 2-slot half-rate
    Tdma25kHz4Slot       = 0x4,  // TDMA 25 kHz, 4-slot half-rate
    Tdma12_5kHz8psk      = 0x5,  // TDMA 12.5 kHz, 2-slot H-D8PSK
    // 0x6-0xF reserved
}
```

### 3.6 System Service Class (8 bits)

Describes services available at a site. Used in ADJ_STS_BCST, NET_STS_BCST, SCCB, etc.

### 3.7 LRA (Location Registration Area)

8-bit value identifying a geographic region for location registration. Used in LOC_REG_REQ
and RFSS_STS_BCST.

### 3.8 Transmit Offset Encoding

**FDMA (9-bit):**
```
Bit 8: Sign (0 = negative/low, 1 = positive/high)
Bits 7:0: Magnitude
Offset_Hz = Sign * Magnitude * 250_000
```

**VHF/UHF FDMA (14-bit):**
```
Bit 13: Sign (0 = negative/low, 1 = positive/high)
Bits 12:0: Magnitude
Offset_Hz = Sign * Magnitude * Channel_Spacing
```

**TDMA (14-bit):** Same encoding as VHF/UHF.

---

## 4. Complete ISP Opcode Dispatch Table (SU -> FNE)

### 4.1 Code-Ready Const Array

```rust
/// ISP (Inbound Signaling Packet) opcodes -- SU to FNE
/// (opcode, alias, description, obsolete)
pub const ISP_OPCODES: &[(u8, &str, &str, bool)] = &[
    // Voice Service
    (0x00, "GRP_V_REQ",          "Group Voice Service Request",                     false),
    (0x01, "RESERVED",           "Reserved",                                        false),
    (0x02, "RESERVED",           "Reserved",                                        false),
    (0x03, "RESERVED",           "Reserved",                                        false),
    (0x04, "UU_V_REQ",           "Unit-to-Unit Voice Service Request",              false),
    (0x05, "UU_ANS_RSP",         "Unit-to-Unit Answer Response",                    false),
    (0x06, "RESERVED",           "Reserved",                                        false),
    (0x07, "RESERVED",           "Reserved",                                        false),
    (0x08, "TELE_INT_DIAL_REQ",  "Telephone Interconnect Explicit Dial Request",    false),
    (0x09, "TELE_INT_PSTN_REQ",  "Telephone Interconnect PSTN Request",             false),
    (0x0A, "TELE_INT_ANS_RSP",   "Telephone Interconnect Answer Response",          false),
    (0x0B, "RESERVED",           "Reserved",                                        false),
    (0x0C, "RESERVED",           "Reserved",                                        false),
    (0x0D, "RESERVED",           "Reserved",                                        false),
    (0x0E, "RESERVED",           "Reserved",                                        false),
    (0x0F, "RESERVED",           "Reserved",                                        false),
    // Data Service
    (0x10, "IND_DATA_REQ",       "Individual Data Service Request",                 true),  // OBSOLETE
    (0x11, "GRP_DATA_REQ",       "Group Data Service Request",                      true),  // OBSOLETE
    (0x12, "SN_DATA_CHN_REQ",    "SNDCP Data Channel Request",                     false),
    (0x13, "SN_DATA_PAGE_RES",   "SNDCP Data Page Response",                       false),
    (0x14, "SN_REC_REQ",         "SNDCP Reconnect Request",                        false),
    (0x15, "RESERVED",           "Reserved",                                        false),
    (0x16, "RESERVED",           "Reserved",                                        false),
    (0x17, "RESERVED",           "Reserved",                                        false),
    // Control and Status
    (0x18, "STS_UPDT_REQ",       "Status Update Request",                          false),
    (0x19, "STS_Q_RSP",          "Status Query Response",                           false),
    (0x1A, "STS_Q_REQ",          "Status Query Request",                            false),
    (0x1B, "RESERVED",           "Reserved",                                        false),
    (0x1C, "MSG_UPDT_REQ",       "Message Update Request",                          false),
    (0x1D, "RAD_MON_REQ",        "Radio Unit Monitor Request",                      false),
    (0x1E, "RAD_MON_ENH_REQ",    "Radio Unit Monitor Enhanced Request",             false),
    (0x1F, "CALL_ALRT_REQ",      "Call Alert Request",                              false),
    (0x20, "ACK_RSP_U",          "Acknowledge Response - Unit",                     false),
    (0x21, "RESERVED",           "Reserved",                                        false),
    (0x22, "RESERVED",           "Reserved",                                        false),
    (0x23, "CAN_SRV_REQ",        "Cancel Service Request",                          false),
    (0x24, "EXT_FNCT_RSP",       "Extended Function Response",                      false),
    (0x25, "RESERVED",           "Reserved",                                        false),
    (0x26, "RESERVED",           "Reserved",                                        false),
    (0x27, "EMRG_ALRM_REQ",     "Emergency Alarm Request",                         false),
    (0x28, "GRP_AFF_REQ",        "Group Affiliation Request",                       false),
    (0x29, "GRP_AFF_Q_RSP",      "Group Affiliation Query Response",                false),
    (0x2A, "RESERVED",           "Reserved",                                        false),
    (0x2B, "U_DE_REG_REQ",       "De-Registration Request",                         false),
    (0x2C, "U_REG_REQ",          "Unit Registration Request",                       false),
    (0x2D, "LOC_REG_REQ",        "Location Registration Request",                   false),
    (0x2E, "RESERVED",           "Reserved",                                        false),
    (0x2F, "RESERVED",           "Reserved",                                        false),
    (0x30, "P_PARM_REQ",         "Protection Parameter Request",                    true),  // OBSOLETE
    (0x31, "RESERVED",           "Reserved",                                        false),
    (0x32, "IDEN_UP_REQ",        "Identifier Update Request",                       false),
    (0x33, "RESERVED",           "Reserved",                                        false),
    (0x34, "RESERVED",           "Reserved",                                        false),
    (0x35, "RESERVED",           "Reserved",                                        false),
    (0x36, "ROAM_ADDR_REQ",      "Roaming Address Request",                         false),
    (0x37, "ROAM_ADDR_RSP",      "Roaming Address Response",                        false),
    // Authentication
    (0x38, "AUTH_RESP",           "Authentication Response",                          false),
    (0x39, "AUTH_RESP_M",         "Authentication Response Mutual",                  false),
    (0x3A, "AUTH_FNE_RST",        "Authentication FNE Result",                       false),
    (0x3B, "AUTH_SU_DMD",         "Authentication SU Demand",                        false),
    (0x3C, "RESERVED",           "Reserved",                                        false),
    (0x3D, "RESERVED",           "Reserved",                                        false),
    (0x3E, "RESERVED",           "Reserved",                                        false),
    (0x3F, "RESERVED",           "Reserved",                                        false),
];
```

### 4.2 Rust Enum

```rust
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(u8)]
pub enum IspOpcode {
    GrpVReq         = 0x00,
    UuVReq          = 0x04,
    UuAnsRsp        = 0x05,
    TeleIntDialReq  = 0x08,
    TeleIntPstnReq  = 0x09,
    TeleIntAnsRsp   = 0x0A,
    IndDataReq      = 0x10,  // OBSOLETE
    GrpDataReq      = 0x11,  // OBSOLETE
    SnDataChnReq    = 0x12,
    SnDataPageRes   = 0x13,
    SnRecReq        = 0x14,
    StsUpdtReq      = 0x18,
    StsQRsp         = 0x19,
    StsQReq         = 0x1A,
    MsgUpdtReq      = 0x1C,
    RadMonReq       = 0x1D,
    RadMonEnhReq    = 0x1E,
    CallAlrtReq     = 0x1F,
    AckRspU         = 0x20,
    CanSrvReq       = 0x23,
    ExtFnctRsp      = 0x24,
    EmrgAlrmReq     = 0x27,
    GrpAffReq       = 0x28,
    GrpAffQRsp      = 0x29,
    UDeRegReq       = 0x2B,
    URegReq         = 0x2C,
    LocRegReq       = 0x2D,
    PParmReq        = 0x30,  // OBSOLETE
    IdenUpReq       = 0x32,
    RoamAddrReq     = 0x36,
    RoamAddrRsp     = 0x37,
    AuthResp        = 0x38,
    AuthRespM       = 0x39,
    AuthFneRst      = 0x3A,
    AuthSuDmd       = 0x3B,
}

impl IspOpcode {
    pub fn from_u8(val: u8) -> Option<Self> {
        match val {
            0x00 => Some(Self::GrpVReq),
            0x04 => Some(Self::UuVReq),
            0x05 => Some(Self::UuAnsRsp),
            0x08 => Some(Self::TeleIntDialReq),
            0x09 => Some(Self::TeleIntPstnReq),
            0x0A => Some(Self::TeleIntAnsRsp),
            0x10 => Some(Self::IndDataReq),
            0x11 => Some(Self::GrpDataReq),
            0x12 => Some(Self::SnDataChnReq),
            0x13 => Some(Self::SnDataPageRes),
            0x14 => Some(Self::SnRecReq),
            0x18 => Some(Self::StsUpdtReq),
            0x19 => Some(Self::StsQRsp),
            0x1A => Some(Self::StsQReq),
            0x1C => Some(Self::MsgUpdtReq),
            0x1D => Some(Self::RadMonReq),
            0x1E => Some(Self::RadMonEnhReq),
            0x1F => Some(Self::CallAlrtReq),
            0x20 => Some(Self::AckRspU),
            0x23 => Some(Self::CanSrvReq),
            0x24 => Some(Self::ExtFnctRsp),
            0x27 => Some(Self::EmrgAlrmReq),
            0x28 => Some(Self::GrpAffReq),
            0x29 => Some(Self::GrpAffQRsp),
            0x2B => Some(Self::UDeRegReq),
            0x2C => Some(Self::URegReq),
            0x2D => Some(Self::LocRegReq),
            0x30 => Some(Self::PParmReq),
            0x32 => Some(Self::IdenUpReq),
            0x36 => Some(Self::RoamAddrReq),
            0x37 => Some(Self::RoamAddrRsp),
            0x38 => Some(Self::AuthResp),
            0x39 => Some(Self::AuthRespM),
            0x3A => Some(Self::AuthFneRst),
            0x3B => Some(Self::AuthSuDmd),
            _    => None,
        }
    }

    pub fn is_obsolete(&self) -> bool {
        matches!(self, Self::IndDataReq | Self::GrpDataReq | Self::PParmReq)
    }
}
```

### 4.3 ISP Message Field Layouts (Abbreviated TSBK)

#### GRP_V_REQ (0x00) -- Group Voice Service Request

```
Oct 0: LB|P|000000    Oct 1: MFID
Oct 2: Service Options
Oct 3: reserved
Oct 4: reserved
Oct 5: Group Address[23:16]
Oct 6: Group Address[15:8]
Oct 7: Group Address[7:0]
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]     (NOTE: 16-bit abbreviated in some readings)
```

#### UU_V_REQ (0x04) -- Unit-to-Unit Voice Service Request

```
Oct 0: LB|P|000100    Oct 1: MFID
Oct 2: Service Options
Oct 3: reserved
Oct 4: Target Address[23:16]
Oct 5: Target Address[15:8]
Oct 6: Target Address[7:0]
Oct 7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

Extended (MBT, Blocks=1): Header carries Source Address + Opcode. Data Block carries
target WACN ID (20-bit), System ID (12-bit), Target Unit ID (24-bit).

#### UU_ANS_RSP (0x05) -- Unit-to-Unit Answer Response

```
Oct 0: LB|P|000101    Oct 1: MFID
Oct 2: Service Options
Oct 3: reserved
Oct 4: Target Address[23:16]
Oct 5: Target Address[15:8]
Oct 6: Target Address[7:0]
Oct 7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### TELE_INT_DIAL_REQ (0x08) -- Telephone Interconnect Explicit Dial

Extended format only (MBT, 1-2 data blocks). Carries up to 34 DTMF digits at 4 bits each.

#### TELE_INT_PSTN_REQ (0x09) -- Telephone Interconnect PSTN Request

```
Oct 0: LB|P|001001    Oct 1: MFID
Oct 2: Service Options
Oct 3-7: reserved
Oct 8-9: Source Address (abbreviated)
```

#### SN_DATA_CHN_REQ (0x12) -- SNDCP Data Channel Request

```
Oct 0: LB|P|010010    Oct 1: MFID
Oct 2: Service Options
Oct 3-7: reserved / data service info
Oct 8-9: Source Address (abbreviated)
```

#### STS_UPDT_REQ (0x18) -- Status Update Request

```
Oct 0: LB|P|011000    Oct 1: MFID
Oct 2: Status[15:8]
Oct 3: Status[7:0]
Oct 4: Target ID[23:16]
Oct 5: Target ID[15:8]
Oct 6: Target ID[7:0]
Oct 7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### STS_Q_RSP (0x19) -- Status Query Response

```
Oct 0: LB|P|011001    Oct 1: MFID
Oct 2: Status[15:8]
Oct 3: Status[7:0]
Oct 4: Target Address[23:16]
Oct 5: Target Address[15:8]
Oct 6: Target Address[7:0]
Oct 7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### STS_Q_REQ (0x1A) -- Status Query Request

```
Oct 0: LB|P|011010    Oct 1: MFID
Oct 2-3: reserved
Oct 4: Target ID[23:16]
Oct 5: Target ID[15:8]
Oct 6: Target ID[7:0]
Oct 7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### MSG_UPDT_REQ (0x1C) -- Message Update Request

```
Oct 0: LB|P|011100    Oct 1: MFID
Oct 2: Message[15:8]
Oct 3: Message[7:0]
Oct 4: Target ID[23:16]
Oct 5: Target ID[15:8]
Oct 6: Target ID[7:0]
Oct 7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### RAD_MON_REQ (0x1D) -- Radio Unit Monitor Request

```
Oct 0: LB|P|011101    Oct 1: MFID
Oct 2: TX Time (8-bit, seconds; 0 = don't key)
Oct 3: SM(7) | reserved(6:2) | TX Mult(1:0)
Oct 4: Target ID[23:16]
Oct 5: Target ID[15:8]
Oct 6: Target ID[7:0]
Oct 7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

SM = 0 non-silent, 1 silent. TX Mult = 2-bit multiplier (0-3).

#### RAD_MON_ENH_REQ (0x1E) -- Radio Unit Monitor Enhanced Request

MBT only (Blocks=1). Header: Opcode + Group ID. Data Block: Target ID (24-bit),
SM|TG|reserved, TX Time, Key ID (16-bit), ALGID (8-bit; 0x80 = unencrypted).

#### CALL_ALRT_REQ (0x1F) -- Call Alert Request

```
Oct 0: LB|P|011111    Oct 1: MFID
Oct 2-3: reserved
Oct 4: Target Address[23:16]
Oct 5: Target Address[15:8]
Oct 6: Target Address[7:0]
Oct 7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### ACK_RSP_U (0x20) -- Acknowledge Response - Unit

```
Oct 0: LB|P|100000    Oct 1: MFID
Oct 2: AIV(7) | 0(6) | Service Type(5:0)
Oct 3-6: Additional Information
Oct 7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### CAN_SRV_REQ (0x23) -- Cancel Service Request

```
Oct 0: LB|P|100011    Oct 1: MFID
Oct 2: AIV(7) | 0(6) | Service Type(5:0)
Oct 3: Reason Code (Annex A)
Oct 4: Additional Info[23:16]   (if Group: Call Options)
Oct 5: Additional Info[15:8]    (if Group: Group Addr high)
Oct 6: Additional Info[7:0]     (if Group: Group Addr low)
Oct 7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### EXT_FNCT_RSP (0x24) -- Extended Function Response

```
Oct 0: LB|P|100100    Oct 1: MFID
Oct 2-3: reserved
Oct 4: Extended Function[23:16]
Oct 5: Extended Function[15:8]
Oct 6: Extended Function[7:0]
Oct 7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### EMRG_ALRM_REQ (0x27) -- Emergency Alarm Request

```
Oct 0: LB|P|100111    Oct 1: MFID
Oct 2: SI-1 (Special Information 1)
Oct 3: SI-2 (Special Information 2)
Oct 4: reserved
Oct 5: Group Address[23:16]
Oct 6: Group Address[15:8]
Oct 7: Group Address[7:0]
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

SI-1 bit flags:
```
Bit 0: MD   -- Man-Down condition
Bit 1: VSE  -- Vehicle Sensed Emergency
Bit 2: ASE  -- Accessory Sensed Emergency
Bits 3-6: IC3-IC6 -- Interoperable Condition codes
Bit 7: AC   -- Additional Codes present in SI-2
```

SI-2 values: 0x00 = default, 0x01 = Vest Pierced, 0x02-0x7F = reserved, 0x80-0xFF = user-defined.

#### GRP_AFF_REQ (0x28) -- Group Affiliation Request

```
Oct 0: LB|P|101000    Oct 1: MFID
Oct 2: E(7) | reserved(6:0)
Oct 3: reserved(7:4) | System ID[11:8]
Oct 4: System ID[7:0]
Oct 5: Group ID[23:16]
Oct 6: Group ID[15:8]
Oct 7: Group ID[7:0]
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### GRP_AFF_Q_RSP (0x29) -- Group Affiliation Query Response

```
Oct 0: LB|P|101001    Oct 1: MFID
Oct 2: E(7) | reserved(6:0)
Oct 3: Announcement Group Address[15:8]
Oct 4: Announcement Group Address[7:0]
Oct 5: Group Address[23:16]
Oct 6: Group Address[15:8]
Oct 7: Group Address[7:0]
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### U_DE_REG_REQ (0x2B) -- De-Registration Request

```
Oct 0: LB|P|101011    Oct 1: MFID
Oct 2: reserved
Oct 3: reserved
Oct 4: WACN ID[19:12]
Oct 5: WACN ID[11:4]  (upper nibble) | WACN ID[3:0] (lower nibble -- see note)
Oct 6: System ID[7:0]
Oct 7: reserved
Oct 8: Source ID[23:16]
Oct 9: Source ID[7:0]
```

#### U_REG_REQ (0x2C) -- Unit Registration Request

```
Oct 0: LB|P|101100    Oct 1: MFID
Oct 2: E(7) | Capabilities(6:0)
Oct 3: reserved
Oct 4: WACN ID[19:12]
Oct 5: WACN ID[11:8] (upper nibble)  -- lower nibble part of System ID or reserved
Oct 6: System ID[7:0]
Oct 7: reserved
Oct 8: Source ID[23:16]  (assigned WUID or registration default)
Oct 9: Source ID[7:0]
```

#### LOC_REG_REQ (0x2D) -- Location Registration Request

```
Oct 0: LB|P|101101    Oct 1: MFID
Oct 2: E(7) | Capabilities(6:0)
Oct 3: reserved
Oct 4: LRA (8-bit Location Registration Area)
Oct 5: Group Address[23:16]
Oct 6: Group Address[15:8]
Oct 7: Group Address[7:0]
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

Extended (MBT, Blocks=2): Header + 2 Data Blocks. Adds WACN ID, System ID, Unit ID,
Previous LRA, E|Capabilities.

#### IDEN_UP_REQ (0x32) -- Identifier Update Request

```
Oct 0: LB|P|110010    Oct 1: MFID
Oct 2: Flags(7:4) | Identifier(3:0)
Oct 3-7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

Flags bit 7 = 1: request all identifiers. Flags bit 7 = 0: request specific identifier only.

#### ROAM_ADDR_REQ (0x36) -- Roaming Address Request

```
Oct 0: LB|P|110110    Oct 1: MFID
Oct 2-3: reserved
Oct 4: Target Address[23:16]
Oct 5: Target Address[15:8]
Oct 6: Target Address[7:0]
Oct 7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### ROAM_ADDR_RSP (0x37) -- Roaming Address Response

```
Oct 0: LB|P|110111    Oct 1: MFID
Oct 2: LM(7) | reserved(6:4) | MSN(3:0)
Oct 3: WACN ID[19:12]  (from stack entry)
Oct 4: WACN ID[11:4]
Oct 5: WACN ID[3:0] | System ID[11:8]
Oct 6: System ID[7:0]
Oct 7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

LM = Last Message flag (1 on final response). MSN = Message Sequence Number (0-15).
Extended formats carry 2/4/8 stack entries using MBT with 1/2/3 data blocks.

#### AUTH_RESP (0x38) -- Authentication Response

```
Oct 0: LB|P|111000    Oct 1: MFID
Oct 2: reserved(7:1) | S(0)
Oct 3: RES1[31:24]
Oct 4: RES1[23:16]
Oct 5: RES1[15:8]
Oct 6: RES1[7:0]
Oct 7: reserved
Oct 8: Source ID[23:16]
Oct 9: Source ID[7:0]
```

S = 1: standalone authentication. RES1 = 32-bit Response 1 per TIA-102.AACE.

#### AUTH_RESP_M (0x39) -- Authentication Response Mutual

Extended format only (MBT, Blocks=2). Carries RES1 (32-bit) + RAND2 (40-bit) + S flag.

#### AUTH_FNE_RST (0x3A) -- Authentication FNE Result

```
Oct 0: LB|P|111010    Oct 1: MFID
Oct 2: R2(7) | reserved(6:1) | S(0)
Oct 3-7: reserved
Oct 8: Source ID[23:16]
Oct 9: Source ID[7:0]
```

R2 = 1: FNE passed authentication. S = 1: standalone.

#### AUTH_SU_DMD (0x3B) -- Authentication SU Demand

```
Oct 0: LB|P|111011    Oct 1: MFID
Oct 2-7: reserved
Oct 8: Source ID[23:16]
Oct 9: Source ID[7:0]
```

---

## 5. Complete OSP Opcode Dispatch Table (FNE -> SU)

### 5.1 Code-Ready Const Array

```rust
/// OSP (Outbound Signaling Packet) opcodes -- FNE to SU
/// (opcode, alias, description, obsolete)
pub const OSP_OPCODES: &[(u8, &str, &str, bool)] = &[
    // Voice Service
    (0x00, "GRP_V_CH_GRANT",          "Group Voice Channel Grant",                       false),
    (0x01, "RESERVED",                 "Reserved",                                        false),
    (0x02, "GRP_V_CH_GRANT_UPDT",     "Group Voice Channel Grant Update",                false),
    (0x03, "GRP_V_CH_GRANT_UPDT_EXP", "Group Voice Channel Grant Update - Explicit",     false),
    (0x04, "UU_V_CH_GRANT",           "Unit-to-Unit Voice Channel Grant",                false),
    (0x05, "UU_ANS_REQ",              "Unit-to-Unit Answer Request",                     false),
    (0x06, "UU_V_CH_GRANT_UPDT",      "Unit-to-Unit Voice Channel Grant Update",         false),
    (0x07, "RESERVED",                 "Reserved",                                        false),
    (0x08, "TELE_INT_CH_GRANT",       "Telephone Interconnect Voice Channel Grant",      false),
    (0x09, "TELE_INT_CH_GRANT_UPDT",  "Telephone Interconnect Voice CH Grant Update",    false),
    (0x0A, "TELE_INT_ANS_REQ",        "Telephone Interconnect Answer Request",           false),
    (0x0B, "RESERVED",                 "Reserved",                                        false),
    (0x0C, "RESERVED",                 "Reserved",                                        false),
    (0x0D, "RESERVED",                 "Reserved",                                        false),
    (0x0E, "RESERVED",                 "Reserved",                                        false),
    (0x0F, "RESERVED",                 "Reserved",                                        false),
    // Data Service
    (0x10, "IND_DATA_CH_GRANT",        "Individual Data Channel Grant",                   true),  // OBSOLETE
    (0x11, "GRP_DATA_CH_GRANT",        "Group Data Channel Grant",                        true),  // OBSOLETE
    (0x12, "GRP_DATA_CH_ANN",          "Group Data Channel Announcement",                 true),  // OBSOLETE
    (0x13, "GRP_DATA_CH_ANN_EXP",      "Group Data Channel Announcement Explicit",        true),  // OBSOLETE
    (0x14, "SN_DATA_CHN_GNT",          "SNDCP Data Channel Grant",                       false),
    (0x15, "SN_DATA_PAGE_REQ",         "SNDCP Data Page Request",                        false),
    (0x16, "SN_DATA_CHN_ANN_EXP",      "SNDCP Data Channel Announcement - Explicit",     false),
    (0x17, "RESERVED",                 "Reserved",                                        false),
    // Supplementary Services
    (0x18, "STS_UPDT",                 "Status Update",                                   false),
    (0x19, "RESERVED",                 "Reserved",                                        false),
    (0x1A, "STS_Q",                    "Status Query",                                    false),
    (0x1B, "RESERVED",                 "Reserved",                                        false),
    (0x1C, "MSG_UPDT",                 "Message Update",                                  false),
    (0x1D, "RAD_MON_CMD",              "Radio Unit Monitor Command",                      false),
    (0x1E, "RAD_MON_ENH_CMD",          "Radio Unit Monitor Enhanced Command",             false),
    (0x1F, "CALL_ALRT",               "Call Alert",                                      false),
    // Response and Control
    (0x20, "ACK_RSP_FNE",              "Acknowledge Response - FNE",                      false),
    (0x21, "QUE_RSP",                  "Queued Response",                                 false),
    (0x22, "RESERVED",                 "Reserved",                                        false),
    (0x23, "RESERVED",                 "Reserved",                                        false),
    (0x24, "EXT_FNCT_CMD",             "Extended Function Command",                       false),
    (0x25, "RESERVED",                 "Reserved",                                        false),
    (0x26, "RESERVED",                 "Reserved",                                        false),
    (0x27, "DENY_RSP",                 "Deny Response",                                   false),
    (0x28, "GRP_AFF_RSP",              "Group Affiliation Response",                      false),
    (0x29, "SCCB_EXP",                 "Secondary Control Channel Broadcast - Explicit",  false),
    (0x2A, "GRP_AFF_Q",               "Group Affiliation Query",                          false),
    (0x2B, "LOC_REG_RSP",              "Location Registration Response",                  false),
    (0x2C, "U_REG_RSP",               "Unit Registration Response",                       false),
    (0x2D, "U_REG_CMD",               "Unit Registration Command",                        false),
    (0x2E, "AUTH_CMD",                 "Authentication Command",                           true),  // OBSOLETE
    (0x2F, "U_DE_REG_ACK",             "De-Registration Acknowledge",                     false),
    // Synchronization and Authentication
    (0x30, "SYNC_BCST",               "Synchronization Broadcast",                        false),
    (0x31, "AUTH_DMD",                  "Authentication Demand",                           false),
    (0x32, "AUTH_FNE_RESP",             "Authentication FNE Response",                     false),
    (0x33, "IDEN_UP_TDMA",             "Identifier Update for TDMA",                      false),
    (0x34, "IDEN_UP_VU",               "Identifier Update for VHF/UHF Bands",             false),
    (0x35, "TIME_DATE_ANN",            "Time and Date Announcement",                      false),
    (0x36, "ROAM_ADDR_CMD",            "Roaming Address Command",                         false),
    (0x37, "ROAM_ADDR_UPDT",           "Roaming Address Update",                          false),
    // System Broadcasts
    (0x38, "SYS_SRV_BCST",             "System Service Broadcast",                       false),
    (0x39, "SCCB",                     "Secondary Control Channel Broadcast",              false),
    (0x3A, "RFSS_STS_BCST",            "RFSS Status Broadcast",                           false),
    (0x3B, "NET_STS_BCST",             "Network Status Broadcast",                        false),
    (0x3C, "ADJ_STS_BCST",             "Adjacent Status Broadcast",                       false),
    (0x3D, "IDEN_UP",                  "Identifier Update",                                false),
    (0x3E, "ADJ_STS_BCST_UNC",         "Adjacent Status Broadcast Uncoordinated",         false),
    (0x3F, "RESERVED",                 "Reserved",                                        false),
];
```

### 5.2 Rust Enum

```rust
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(u8)]
pub enum OspOpcode {
    GrpVChGrant         = 0x00,
    GrpVChGrantUpdt     = 0x02,
    GrpVChGrantUpdtExp  = 0x03,
    UuVChGrant          = 0x04,
    UuAnsReq            = 0x05,
    UuVChGrantUpdt      = 0x06,
    TeleIntChGrant      = 0x08,
    TeleIntChGrantUpdt  = 0x09,
    TeleIntAnsReq       = 0x0A,
    IndDataChGrant      = 0x10,  // OBSOLETE
    GrpDataChGrant      = 0x11,  // OBSOLETE
    GrpDataChAnn        = 0x12,  // OBSOLETE
    GrpDataChAnnExp     = 0x13,  // OBSOLETE
    SnDataChnGnt        = 0x14,
    SnDataPageReq       = 0x15,
    SnDataChnAnnExp     = 0x16,
    StsUpdt             = 0x18,
    StsQ                = 0x1A,
    MsgUpdt             = 0x1C,
    RadMonCmd           = 0x1D,
    RadMonEnhCmd        = 0x1E,
    CallAlrt            = 0x1F,
    AckRspFne           = 0x20,
    QueRsp              = 0x21,
    ExtFnctCmd          = 0x24,
    DenyRsp             = 0x27,
    GrpAffRsp           = 0x28,
    SccbExp             = 0x29,
    GrpAffQ             = 0x2A,
    LocRegRsp           = 0x2B,
    URegRsp             = 0x2C,
    URegCmd             = 0x2D,
    AuthCmd             = 0x2E,  // OBSOLETE
    UDeRegAck           = 0x2F,
    SyncBcst            = 0x30,
    AuthDmd             = 0x31,
    AuthFneResp         = 0x32,
    IdenUpTdma          = 0x33,
    IdenUpVu            = 0x34,
    TimeDateAnn         = 0x35,
    RoamAddrCmd         = 0x36,
    RoamAddrUpdt        = 0x37,
    SysSrvBcst          = 0x38,
    Sccb                = 0x39,
    RfssStsBoast        = 0x3A,
    NetStsBcst          = 0x3B,
    AdjStsBcst          = 0x3C,
    IdenUp              = 0x3D,
    AdjStsBcstUnc       = 0x3E,
}

impl OspOpcode {
    pub fn from_u8(val: u8) -> Option<Self> {
        match val {
            0x00 => Some(Self::GrpVChGrant),
            0x02 => Some(Self::GrpVChGrantUpdt),
            0x03 => Some(Self::GrpVChGrantUpdtExp),
            0x04 => Some(Self::UuVChGrant),
            0x05 => Some(Self::UuAnsReq),
            0x06 => Some(Self::UuVChGrantUpdt),
            0x08 => Some(Self::TeleIntChGrant),
            0x09 => Some(Self::TeleIntChGrantUpdt),
            0x0A => Some(Self::TeleIntAnsReq),
            0x10 => Some(Self::IndDataChGrant),
            0x11 => Some(Self::GrpDataChGrant),
            0x12 => Some(Self::GrpDataChAnn),
            0x13 => Some(Self::GrpDataChAnnExp),
            0x14 => Some(Self::SnDataChnGnt),
            0x15 => Some(Self::SnDataPageReq),
            0x16 => Some(Self::SnDataChnAnnExp),
            0x18 => Some(Self::StsUpdt),
            0x1A => Some(Self::StsQ),
            0x1C => Some(Self::MsgUpdt),
            0x1D => Some(Self::RadMonCmd),
            0x1E => Some(Self::RadMonEnhCmd),
            0x1F => Some(Self::CallAlrt),
            0x20 => Some(Self::AckRspFne),
            0x21 => Some(Self::QueRsp),
            0x24 => Some(Self::ExtFnctCmd),
            0x27 => Some(Self::DenyRsp),
            0x28 => Some(Self::GrpAffRsp),
            0x29 => Some(Self::SccbExp),
            0x2A => Some(Self::GrpAffQ),
            0x2B => Some(Self::LocRegRsp),
            0x2C => Some(Self::URegRsp),
            0x2D => Some(Self::URegCmd),
            0x2E => Some(Self::AuthCmd),
            0x2F => Some(Self::UDeRegAck),
            0x30 => Some(Self::SyncBcst),
            0x31 => Some(Self::AuthDmd),
            0x32 => Some(Self::AuthFneResp),
            0x33 => Some(Self::IdenUpTdma),
            0x34 => Some(Self::IdenUpVu),
            0x35 => Some(Self::TimeDateAnn),
            0x36 => Some(Self::RoamAddrCmd),
            0x37 => Some(Self::RoamAddrUpdt),
            0x38 => Some(Self::SysSrvBcst),
            0x39 => Some(Self::Sccb),
            0x3A => Some(Self::RfssStsBoast),
            0x3B => Some(Self::NetStsBcst),
            0x3C => Some(Self::AdjStsBcst),
            0x3D => Some(Self::IdenUp),
            0x3E => Some(Self::AdjStsBcstUnc),
            _    => None,
        }
    }

    pub fn is_obsolete(&self) -> bool {
        matches!(self,
            Self::IndDataChGrant | Self::GrpDataChGrant |
            Self::GrpDataChAnn | Self::GrpDataChAnnExp | Self::AuthCmd
        )
    }

    /// True if this is a system broadcast (cycled continuously on control channel)
    pub fn is_broadcast(&self) -> bool {
        matches!(self,
            Self::SyncBcst | Self::IdenUpTdma | Self::IdenUpVu |
            Self::TimeDateAnn | Self::SysSrvBcst | Self::Sccb |
            Self::RfssStsBoast | Self::NetStsBcst |
            Self::AdjStsBcst | Self::IdenUp | Self::AdjStsBcstUnc
        )
    }
}
```

### 5.3 OSP Message Field Layouts (Abbreviated TSBK)

#### GRP_V_CH_GRANT (0x00) -- Group Voice Channel Grant

```
Oct 0: LB|P|000000    Oct 1: MFID
Oct 2: Service Options
Oct 3: reserved
Oct 4: Channel(T)[15:8]
Oct 5: Channel(T)[7:0]
Oct 6: reserved (or Group Address high in some variants)
Oct 7: reserved (or Group Address low)
Oct 8: Group Address[15:8]
Oct 9: Group Address[7:0]
```

Channel(T) = 16-bit: `Identifier[15:12] | Number[11:0]`

Extended (MBT): Adds separate Channel(T) and Channel(R) for explicit T/R assignment.

#### GRP_V_CH_GRANT_UPDT (0x02) -- Group Voice Channel Grant Update

```
Oct 0: LB|P|000010    Oct 1: MFID
Oct 2: Service Options
Oct 3: Channel A(T)[15:8]
Oct 4: Channel A(T)[7:0]
Oct 5: Group Address A[15:8]
Oct 6: Group Address A[7:0]
Oct 7: Channel B(T)[15:8]
Oct 8: Channel B(T)[7:0]
Oct 9: Group Address B[7:0]
```

Carries two simultaneous channel-grant updates (A and B) in a single TSBK.

#### GRP_V_CH_GRANT_UPDT_EXP (0x03) -- Group Voice Channel Grant Update - Explicit

Explicit form with separate T and R channel fields per assignment.

#### UU_V_CH_GRANT (0x04) -- Unit-to-Unit Voice Channel Grant

```
Oct 0: LB|P|000100    Oct 1: MFID
Oct 2: Service Options
Oct 3: Channel(T)[15:8]
Oct 4: Channel(T)[7:0]
Oct 5: Target Address[23:16]
Oct 6: Target Address[15:8]
Oct 7: Target Address[7:0]
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

Extended (MBT, 3-block): Full SUIDs for both source and target.

#### UU_ANS_REQ (0x05) -- Unit-to-Unit Answer Request

FNE forwards a UU voice request to the target SU.

#### UU_V_CH_GRANT_UPDT (0x06) -- Unit-to-Unit Voice Channel Grant Update

Updates a UU voice channel grant.

#### TELE_INT_CH_GRANT (0x08) -- Telephone Interconnect Voice Channel Grant

FNE grants voice channel for telephone interconnect call.

#### TELE_INT_CH_GRANT_UPDT (0x09) -- Telephone Interconnect Voice CH Grant Update

Updates a telephone interconnect voice channel grant.

#### SN_DATA_CHN_GNT (0x14) -- SNDCP Data Channel Grant

FNE grants a SNDCP data channel.

#### ACK_RSP_FNE (0x20) -- Acknowledge Response - FNE

```
Oct 0: LB|P|100000    Oct 1: MFID
Oct 2: AIV(7) | EX(6) | Service Type(5:0)
Oct 3: Additional Info[31:24]
Oct 4: Additional Info[23:16]
Oct 5: Additional Info[15:8]
Oct 6: Additional Info[7:0]
Oct 7: reserved
Oct 8: Target Address[23:16]
Oct 9: Target Address[7:0]
```

AIV/EX interpretation:
- AIV=0: octets 3-6 unused
- AIV=1, EX=1: octets 3-6 = WACN ID + System ID of target
- AIV=1, EX=0: octets 3-4 reserved, octets 5-6 = Source Address (abbreviated)

#### QUE_RSP (0x21) -- Queued Response

```
Oct 0: LB|P|100001    Oct 1: MFID
Oct 2: AIV(7) | 0(6) | Service Type(5:0)
Oct 3: Reason Code (Annex C)
Oct 4: Additional Info[23:16]
Oct 5: Additional Info[15:8]
Oct 6: Additional Info[7:0]
Oct 7: reserved
Oct 8: Target Address[23:16]
Oct 9: Target Address[7:0]
```

#### DENY_RSP (0x27) -- Deny Response

```
Oct 0: LB|P|100111    Oct 1: MFID
Oct 2: AIV(7) | 0(6) | Service Type(5:0)
Oct 3: Reason Code (Annex B)
Oct 4: Additional Info[23:16]  (if Group: Call Options)
Oct 5: Additional Info[15:8]   (if Group: Group Addr high)
Oct 6: Additional Info[7:0]    (if Group: Group Addr low)
Oct 7: reserved
Oct 8: Target Address[23:16]
Oct 9: Target Address[7:0]
```

#### EXT_FNCT_CMD (0x24) -- Extended Function Command

```
Oct 0: LB|P|100100    Oct 1: MFID
Oct 2-3: reserved
Oct 4: Extended Function[23:16]
Oct 5: Extended Function[15:8]
Oct 6: Extended Function[7:0]
Oct 7: reserved
Oct 8: Target Address[23:16]
Oct 9: Target Address[7:0]
```

#### GRP_AFF_RSP (0x28) -- Group Affiliation Response

```
Oct 0: LB|P|101000    Oct 1: MFID
Oct 2: reserved(7:2) | AA(1:0)  (Affiliation Announcement bits)
Oct 3: Announcement Group Address[15:8]
Oct 4: Announcement Group Address[7:0]
Oct 5: Group Address[23:16]
Oct 6: Group Address[15:8]
Oct 7: Group Address[7:0]
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### GRP_AFF_Q (0x2A) -- Group Affiliation Query

```
Oct 0: LB|P|101010    Oct 1: MFID
Oct 2-7: reserved
Oct 8: Target Address[23:16]
Oct 9: Target Address[7:0]
```

#### CALL_ALRT (0x1F) -- Call Alert

```
Oct 0: LB|P|011111    Oct 1: MFID
Oct 2-3: reserved
Oct 4: Target Address[23:16]
Oct 5: Target Address[15:8]
Oct 6: Target Address[7:0]
Oct 7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### U_REG_RSP (0x2C) -- Unit Registration Response

```
Oct 0: LB|P|101100    Oct 1: MFID
Oct 2: reserved(7:2) | RC(1:0)  -- Response Code
Oct 3: WACN ID[19:12]
Oct 4: WACN ID[11:4]
Oct 5: WACN ID[3:0] | System ID[11:8]
Oct 6: System ID[7:0]  (or Source ID high)
Oct 7: Source ID[7:0]   (source who registered)
Oct 8: Assigned Source Address[23:16]
Oct 9: Assigned Source Address[7:0]
```

Response Codes: `00` = accepted, `01` = accepted (used default), `10` = denied, `11` = denied (partial).

#### U_REG_CMD (0x2D) -- Unit Registration Command

```
Oct 0: LB|P|101101    Oct 1: MFID
Oct 2: WACN ID[19:12]
Oct 3: WACN ID[11:4]
Oct 4: WACN ID[3:0] | System ID[11:8]
Oct 5: System ID[7:0]
Oct 6-7: reserved
Oct 8: Target Address[23:16]
Oct 9: Target Address[7:0]
```

#### U_DE_REG_ACK (0x2F) -- De-Registration Acknowledge

```
Oct 0: LB|P|101111    Oct 1: MFID
Oct 2: WACN ID[19:12]
Oct 3: WACN ID[11:4]
Oct 4: WACN ID[3:0] | System ID[11:8]
Oct 5: System ID[7:0]
Oct 6-7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### LOC_REG_RSP (0x2B) -- Location Registration Response

```
Oct 0: LB|P|101011    Oct 1: MFID
Oct 2: reserved(7:2) | RC(1:0)
Oct 3-4: reserved
Oct 5: Group Address[23:16]
Oct 6: Group Address[15:8]
Oct 7: Group Address[7:0]
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
```

#### RFSS_STS_BCST (0x3A) -- RFSS Status Broadcast

```
Oct 0: LB|P|111010    Oct 1: MFID
Oct 2: LRA (8-bit)
Oct 3: reserved(7:5) | A(4) | System ID[11:8]
Oct 4: System ID[7:0]
Oct 5: RF Sub-system ID (8-bit)
Oct 6: Site ID (8-bit)
Oct 7: reserved
Oct 8: Channel[15:8]
Oct 9: Channel[7:0]
```

A = 1: active network connection.

#### NET_STS_BCST (0x3B) -- Network Status Broadcast

```
Oct 0: LB|P|111011    Oct 1: MFID
Oct 2: LRA (8-bit)
Oct 3: WACN ID[19:12]
Oct 4: WACN ID[11:4]
Oct 5: WACN ID[3:0] | System ID[11:8]
Oct 6: Channel[15:8]
Oct 7: Channel[7:0]
Oct 8: System Service Class
Oct 9: reserved
```

NOTE: Octet 5 packs the bottom 4 bits of WACN ID and the top 4 bits of System ID.

#### ADJ_STS_BCST (0x3C) -- Adjacent Status Broadcast

```
Oct 0: LB|P|111100    Oct 1: MFID
Oct 2: LRA (8-bit, adjacent site's LRA)
Oct 3: C(7) | F(6) | V(5) | A(4) | System ID[11:8]
Oct 4: System ID[7:0]
Oct 5: RF Sub-system ID
Oct 6: Site ID
Oct 7: reserved
Oct 8: Channel[15:8]
Oct 9: System Service Class
```

Flags: C = conventional, F = failure, V = valid, A = active network.

#### ADJ_STS_BCST_UNC (0x3E) -- Adjacent Status Broadcast Uncoordinated

Same structure as ADJ_STS_BCST but for sites not sharing WACN coordination.

#### SYS_SRV_BCST (0x38) -- System Service Broadcast

```
Oct 0: LB|P|111000    Oct 1: MFID
Oct 2: reserved
Oct 3: System Service Class (8-bit)
Oct 4: WACN ID[19:12]
Oct 5: WACN ID[11:4]
Oct 6: WACN ID[3:0] | System ID[11:8]
Oct 7: System ID[7:0]  (continued)
Oct 8: reserved
Oct 9: System Service Class (extended)
```

#### SCCB (0x39) -- Secondary Control Channel Broadcast

```
Oct 0: LB|P|111001    Oct 1: MFID
Oct 2: RF Sub-system ID
Oct 3: RFSS | Site (combined field)
Oct 4: Channel(T)[15:8]
Oct 5: Channel(T)[7:0]
Oct 6: Channel(R)[15:8]  (or reserved)
Oct 7: Channel(R)[7:0]
Oct 8: System Service Class
Oct 9: reserved
```

#### IDEN_UP (0x3D) -- Identifier Update (FDMA)

```
Oct 0: LB|P|111101    Oct 1: MFID
Oct 2: Identifier(7:4) | reserved(3:0)
Oct 3: Channel Spacing + Transmit Offset (high)
Oct 4: Transmit Offset (low)
Oct 5: Channel Spacing[15:8]
Oct 6: Channel Spacing[7:0]
Oct 7: Base Frequency[31:24]  (resolution = 5 Hz)
Oct 8: Base Frequency[23:16]
Oct 9: Base Frequency[7:0]
```

NOTE: Base Frequency uses octets 7-9 (24 bits in the extraction); the full 32 bits
may use the boundary bits from octets 6-7. See Section 6 for frequency calculation.

#### IDEN_UP_VU (0x34) -- Identifier Update for VHF/UHF Bands

```
Oct 0: LB|P|110100    Oct 1: MFID
Oct 2: Identifier(7:4) | Channel Spacing(3:0) MSB
Oct 3: Channel Spacing (continued)
Oct 4: Transmit Offset VU[13:8]
Oct 5: Transmit Offset VU[7:0]
Oct 6: Base Frequency[31:24]   (resolution = 5 Hz)
Oct 7: Base Frequency[23:16]
Oct 8: Base Frequency[15:8]
Oct 9: Base Frequency[7:0]
```

#### IDEN_UP_TDMA (0x33) -- Identifier Update for TDMA

```
Oct 0: LB|P|110011    Oct 1: MFID
Oct 2: Identifier(7:4) | Channel Type(3:0)
Oct 3: Slots per Frame + Transmit Offset TDMA (high)
Oct 4: Transmit Offset TDMA (low)
Oct 5: Channel Spacing[15:8]
Oct 6: Channel Spacing[7:0]
Oct 7: Base Frequency[31:24]
Oct 8: Base Frequency[23:16]
Oct 9: Base Frequency[7:0]
```

#### SYNC_BCST (0x30) -- Synchronization Broadcast

```
Oct 0: LB|P|110000    Oct 1: MFID
Oct 2: US(7) | IST(6) | MMU(5) | MC(4) | reserved(3:0)
Oct 3: Local Time Offset (8-bit signed, units = 30 min)
Oct 4: Year (7-bit, offset from 2000)
Oct 5: Month (4-bit)
Oct 6: Day (5-bit)
Oct 7: Hours (5-bit, UTC)
Oct 8: Minutes (6-bit, UTC)
Oct 9: Micro-Slots (6-bit)
```

Flags: US = UTC synchronized, IST = In Sync with Time, MMU = Micro-slot valid, MC = Multi-Channel sync.

#### AUTH_DMD (0x31) -- Authentication Demand

```
Oct 0: LB|P|110001    Oct 1: MFID
Oct 2: reserved(7:1) | S(0)
Oct 3: RAND1[31:24]
Oct 4: RAND1[23:16]
Oct 5: RAND1[15:8]
Oct 6: RAND1[7:0]
Oct 7: reserved
Oct 8: Target Address[23:16]
Oct 9: Target Address[7:0]
```

S = standalone flag. RAND1 = 32-bit random challenge.

#### AUTH_FNE_RESP (0x32) -- Authentication FNE Response

```
Oct 0: LB|P|110010    Oct 1: MFID
Oct 2: reserved
Oct 3: RES2[31:24]
Oct 4: RES2[23:16]
Oct 5: RES2[15:8]
Oct 6: RES2[7:0]
Oct 7: reserved
Oct 8: Target Address[23:16]
Oct 9: Target Address[7:0]
```

RES2 = FNE's 32-bit response to RAND2 challenge during mutual authentication.

#### TIME_DATE_ANN (0x35) -- Time and Date Announcement

Similar to SYNC_BCST but without micro-slot synchronization fields.

#### ROAM_ADDR_CMD (0x36) -- Roaming Address Command

FNE commands SU to update roaming address stack.

#### ROAM_ADDR_UPDT (0x37) -- Roaming Address Update

FNE updates SU's roaming address stack entry.

---

## 6. Channel Identifier Encoding -- Frequency Calculation

### 6.1 Overview

Every channel grant carries a 16-bit Channel field. The 4-bit Identifier portion references
a parameter set broadcast via IDEN_UP, IDEN_UP_VU, or IDEN_UP_TDMA. The 12-bit Channel
Number is the offset within that parameter set.

### 6.2 Frequency Calculation

```
Receive Frequency = Base_Frequency + (Channel_Number * Channel_Spacing)
Transmit Frequency = Receive_Frequency + Transmit_Offset
```

Where:
- `Base_Frequency` = value from IDEN_UP * 5 Hz (32-bit value, resolution = 5 Hz)
- `Channel_Spacing` = spacing in Hz from IDEN_UP
- `Transmit_Offset` = signed offset (see encoding below)
- `Channel_Number` = 12-bit value from the channel grant (bits 11:0 of Channel field)

### 6.3 Identifier Table Structure

```rust
/// Channel identifier parameters from IDEN_UP / IDEN_UP_VU / IDEN_UP_TDMA
#[derive(Debug, Clone)]
pub struct ChannelIdentifier {
    pub id: u8,                    // 4-bit identifier (0-15)
    pub channel_type: ChannelType, // FDMA/TDMA variant (only for TDMA iden)
    pub base_frequency_hz: u64,    // Base frequency in Hz
    pub channel_spacing_hz: u32,   // Channel spacing in Hz
    pub transmit_offset_hz: i64,   // Signed transmit offset in Hz
    pub slots_per_frame: u8,       // TDMA only: slots per frame
}

/// Channel identifier table -- up to 16 entries (4-bit ID)
pub struct ChannelIdentifierTable {
    entries: [Option<ChannelIdentifier>; 16],
}

impl ChannelIdentifierTable {
    pub fn new() -> Self {
        ChannelIdentifierTable {
            entries: Default::default(),
        }
    }

    /// Update entry from an IDEN_UP / IDEN_UP_VU / IDEN_UP_TDMA broadcast
    pub fn update(&mut self, ident: ChannelIdentifier) {
        if (ident.id as usize) < 16 {
            self.entries[ident.id as usize] = Some(ident);
        }
    }

    /// Resolve a 16-bit channel field to transmit and receive frequencies
    pub fn resolve(&self, channel_field: u16) -> Option<(u64, u64)> {
        let cf = ChannelField::from_u16(channel_field);
        let entry = self.entries.get(cf.identifier as usize)?.as_ref()?;
        let rx_freq = entry.base_frequency_hz
            + (cf.number as u64) * (entry.channel_spacing_hz as u64);
        let tx_freq = (rx_freq as i64 + entry.transmit_offset_hz) as u64;
        Some((rx_freq, tx_freq))
    }
}
```

### 6.4 IDEN_UP Parsing Examples

```rust
/// Parse IDEN_UP (FDMA) -- OSP 0x3D
fn parse_iden_up(tsbk: &[u8; 12]) -> ChannelIdentifier {
    let identifier = (tsbk[2] >> 4) & 0x0F;

    // Transmit offset: 9-bit (bit 8 = sign, bits 7:0 = magnitude)
    // Packed across octets 3-4 along with channel spacing
    // Simplified: sign in high bit, magnitude * 250 kHz
    let tx_offset_sign: i64 = if (tsbk[3] & 0x80) != 0 { 1 } else { -1 };
    let tx_offset_mag = ((tsbk[3] as u32 & 0x01) << 8 | tsbk[4] as u32) as i64;
    let transmit_offset_hz = tx_offset_sign * tx_offset_mag * 250_000;

    // Channel spacing: 10-bit, units = 125 Hz (typical)
    let channel_spacing_raw = ((tsbk[5] as u32) << 8) | (tsbk[6] as u32);
    let channel_spacing_hz = channel_spacing_raw * 125;  // 125 Hz resolution typical

    // Base frequency: 32-bit, resolution = 5 Hz
    let base_freq_raw = ((tsbk[7] as u64) << 24)
        | ((tsbk[8] as u64) << 16)
        | ((tsbk[9] as u64) << 8);  // NOTE: only 24 bits available in extraction
    let base_frequency_hz = base_freq_raw * 5;

    ChannelIdentifier {
        id: identifier,
        channel_type: ChannelType::Fdma12_5kHzHalfRate, // default for FDMA IDEN_UP
        base_frequency_hz,
        channel_spacing_hz,
        transmit_offset_hz,
        slots_per_frame: 1,
    }
}

/// Parse IDEN_UP_TDMA -- OSP 0x33
fn parse_iden_up_tdma(tsbk: &[u8; 12]) -> ChannelIdentifier {
    let identifier = (tsbk[2] >> 4) & 0x0F;
    let channel_type_raw = tsbk[2] & 0x0F;

    // Transmit offset TDMA: 14-bit
    let tx_offset_sign: i64 = if (tsbk[3] & 0x20) != 0 { 1 } else { -1 };
    let tx_offset_mag = (((tsbk[3] as u32 & 0x1F) << 8) | tsbk[4] as u32) as i64;

    let channel_spacing_raw = ((tsbk[5] as u32) << 8) | (tsbk[6] as u32);
    let channel_spacing_hz = channel_spacing_raw * 125;

    let transmit_offset_hz = tx_offset_sign * tx_offset_mag * (channel_spacing_hz as i64);

    let base_freq_raw = ((tsbk[7] as u64) << 16)
        | ((tsbk[8] as u64) << 8)
        | (tsbk[9] as u64);
    let base_frequency_hz = base_freq_raw * 5;

    ChannelIdentifier {
        id: identifier,
        channel_type: match channel_type_raw {
            0x3 => ChannelType::Tdma12_5kHz2Slot,
            0x4 => ChannelType::Tdma25kHz4Slot,
            0x5 => ChannelType::Tdma12_5kHz8psk,
            _   => ChannelType::Fdma12_5kHzHalfRate, // fallback
        },
        base_frequency_hz,
        channel_spacing_hz,
        transmit_offset_hz,
        slots_per_frame: if channel_type_raw == 0x4 { 4 } else { 2 },
    }
}
```

---

## 7. Multi-Block Messages (MBT)

### 7.1 Messages That Require MBT

The following messages are always or conditionally MBT:

| Message | When MBT | Blocks |
|---------|----------|--------|
| TELE_INT_DIAL_REQ (ISP 0x08) | Always (carries DTMF digits) | 1-2 |
| AUTH_RESP_M (ISP 0x39) | Always (carries RES1 + RAND2) | 2 |
| RAD_MON_ENH_REQ (ISP 0x1E) | Always (abbreviated uses MBT) | 1-2 |
| RAD_MON_ENH_CMD (OSP 0x1E) | Always (abbreviated uses MBT) | 1-2 |
| UU_V_REQ (ISP 0x04) | When roaming/cross-system | 1 |
| UU_ANS_RSP (ISP 0x05) | When roaming | 1 |
| UU_V_CH_GRANT (OSP 0x04) | When roaming/cross-system | 2 (3-block total) |
| UU_ANS_REQ (OSP 0x05) | When roaming | 1-2 |
| GRP_V_CH_GRANT (OSP 0x00) | When explicit T/R channels needed | 1 |
| GRP_AFF_REQ (ISP 0x28) | When roaming | 1 |
| LOC_REG_REQ (ISP 0x2D) | When roaming | 2 |
| STS_UPDT_REQ (ISP 0x18) | When roaming | 1 |
| STS_Q_REQ (ISP 0x1A) | When roaming | 1 |
| STS_Q_RSP (ISP 0x19) | When roaming | 1 |
| MSG_UPDT_REQ (ISP 0x1C) | When roaming | 1 |
| RAD_MON_REQ (ISP 0x1D) | When roaming | 1 |
| CAN_SRV_REQ (ISP 0x23) | When UU cross-system | 1 |
| EXT_FNCT_RSP (ISP 0x24) | When roaming | 1 |
| ROAM_ADDR_RSP (ISP 0x37) | Multiple stack entries | 1-3 |
| AUTH_RESP (ISP 0x38) | When roaming | 2 |
| AUTH_FNE_RST (ISP 0x3A) | When roaming | 1 |
| AUTH_SU_DMD (ISP 0x3B) | When roaming | 1 |
| ACK_RSP_FNE (OSP 0x20) | When extended addressing needed | 2 |
| CALL_ALRT (OSP 0x1F) | When roaming | 2 |
| EXT_FNCT_CMD (OSP 0x24) | When roaming | 1 |
| ADJ_STS_BCST (OSP 0x3C) | Cross-WACN / VHF/UHF | 1 |

### 7.2 MBT Parsing Pseudocode

```rust
/// Parse an MBT Header Block
fn parse_mbt_header(header: &[u8; 12]) -> Option<MbtHeader> {
    let an = (header[0] >> 6) & 0x01;
    if an != 1 { return None; }  // Must be 1 for MBT

    let io = (header[0] >> 5) & 0x01;  // 0=ISP, 1=OSP
    let format = header[0] & 0x1F;
    if format != 0x17 { return None; }  // 0b10111 = AMBTC

    let mfid = header[2];
    let address = ((header[3] as u32) << 16)
        | ((header[4] as u32) << 8)
        | (header[5] as u32);
    let blocks_to_follow = header[6] & 0x7F;
    let opcode = header[7] & 0x3F;

    Some(MbtHeader {
        direction: if io == 0 { Direction::Isp } else { Direction::Osp },
        mfid,
        address,
        blocks_to_follow,
        opcode,
        msg_specific: [header[8], header[9]],
    })
}
```

---

## 8. Parser Pseudocode for TSBK Dispatch

### 8.1 Top-Level TSBK Parser

```rust
/// Direction of a TSBK on the control channel
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Direction {
    Isp,  // Inbound: SU -> FNE (on ISCH/inbound slot)
    Osp,  // Outbound: FNE -> SU (on broadcast/outbound slot)
}

/// Parsed TSBK message
#[derive(Debug)]
pub enum TsbkMessage {
    // Voice Service
    GrpVReq { service_opts: ServiceOptions, group_addr: u32, source: u32 },
    GrpVChGrant { service_opts: ServiceOptions, channel: u16, group_addr: u32 },
    GrpVChGrantUpdt { ch_a: u16, group_a: u16, ch_b: u16, group_b: u16 },
    UuVReq { service_opts: ServiceOptions, target: u32, source: u32 },
    UuVChGrant { service_opts: ServiceOptions, channel: u16, target: u32, source: u32 },

    // Registration / Affiliation
    URegReq { capabilities: Capabilities, wacn_id: u32, system_id: u16, source: u32 },
    URegRsp { response_code: u8, wacn_id: u32, system_id: u16, source: u32, assigned: u32 },
    LocRegReq { capabilities: Capabilities, lra: u8, group_addr: u32, source: u32 },
    LocRegRsp { response_code: u8, group_addr: u32, source: u32 },
    GrpAffReq { emergency: bool, system_id: u16, group_id: u32, source: u32 },
    GrpAffRsp { aff_announce: u8, ann_group: u16, group_addr: u32, source: u32 },

    // System Broadcasts
    NetStsBcst { lra: u8, wacn_id: u32, system_id: u16, channel: u16, svc_class: u8 },
    RfssStsBoast { lra: u8, active: bool, system_id: u16, rfss_id: u8, site_id: u8, channel: u16 },
    AdjStsBcst { lra: u8, cfva: u8, system_id: u16, rfss_id: u8, site_id: u8, channel: u16, svc_class: u8 },
    SysSrvBcst { svc_class: u8, wacn_id: u32, system_id: u16 },
    IdenUp { identifier: u8, base_freq: u64, spacing: u32, offset: i64 },
    IdenUpTdma { identifier: u8, channel_type: u8, base_freq: u64, spacing: u32, offset: i64 },
    SyncBcst { flags: u8, time_offset: i8, year: u16, month: u8, day: u8, hours: u8, minutes: u8, micro_slots: u8 },

    // Auth
    AuthDmd { standalone: bool, rand1: u32, target: u32 },
    AuthResp { standalone: bool, res1: u32, source: u32 },

    // Control
    AckRspFne { aiv: bool, ex: bool, service_type: u8, info: u32, target: u32 },
    DenyRsp { aiv: bool, service_type: u8, reason: u8, info: u32, target: u32 },
    QueRsp { aiv: bool, service_type: u8, reason: u8, info: u32, target: u32 },
    EmrgAlrmReq { si1: u8, si2: u8, group_addr: u32, source: u32 },

    // Catch-all
    Unknown { opcode: u8, direction: Direction, payload: [u8; 8] },
}

/// Main TSBK dispatch function
pub fn parse_tsbk(data: &[u8; 12], direction: Direction) -> Option<TsbkMessage> {
    // Step 1: Validate CRC
    if !tsbk_crc_valid(data) {
        return None;  // CRC failure -- discard
    }

    // Step 2: Extract header
    let header = TsbkHeader::from_bytes(data);

    // Step 3: Check for manufacturer-specific
    if header.mfid != 0x00 {
        // Manufacturer-specific message -- dispatch by MFID
        // Common: 0x90 = Motorola, 0xA4 = Harris
        return Some(TsbkMessage::Unknown {
            opcode: header.opcode,
            direction,
            payload: data[2..10].try_into().unwrap(),
        });
    }

    // Step 4: Dispatch by direction and opcode
    match (direction, header.opcode) {
        // === ISP Messages ===
        (Direction::Isp, 0x00) => {
            let service_opts = ServiceOptions::from_byte(data[2]);
            let group_addr = ((data[5] as u32) << 16)
                | ((data[6] as u32) << 8)
                | (data[7] as u32);
            let source = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::GrpVReq { service_opts, group_addr, source })
        }

        (Direction::Isp, 0x04) => {
            let service_opts = ServiceOptions::from_byte(data[2]);
            let target = ((data[4] as u32) << 16)
                | ((data[5] as u32) << 8)
                | (data[6] as u32);
            let source = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::UuVReq { service_opts, target, source })
        }

        (Direction::Isp, 0x27) => {
            let si1 = data[2];
            let si2 = data[3];
            let group_addr = ((data[5] as u32) << 16)
                | ((data[6] as u32) << 8)
                | (data[7] as u32);
            let source = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::EmrgAlrmReq { si1, si2, group_addr, source })
        }

        (Direction::Isp, 0x28) => {
            let emergency = (data[2] & 0x80) != 0;
            let system_id = (((data[3] as u16) & 0x0F) << 8) | (data[4] as u16);
            let group_id = ((data[5] as u32) << 16)
                | ((data[6] as u32) << 8)
                | (data[7] as u32);
            let source = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::GrpAffReq { emergency, system_id, group_id, source })
        }

        (Direction::Isp, 0x2C) => {
            let capabilities = Capabilities::from_byte(data[2]);
            let wacn_id = ((data[4] as u32) << 12) | ((data[5] as u32) << 4);
            let system_id = ((data[5] as u16 & 0x0F) << 8) | (data[6] as u16);
            let source = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::URegReq { capabilities, wacn_id, system_id, source })
        }

        (Direction::Isp, 0x2D) => {
            let capabilities = Capabilities::from_byte(data[2]);
            let lra = data[4];
            let group_addr = ((data[5] as u32) << 16)
                | ((data[6] as u32) << 8)
                | (data[7] as u32);
            let source = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::LocRegReq { capabilities, lra, group_addr, source })
        }

        (Direction::Isp, 0x38) => {
            let standalone = (data[2] & 0x01) != 0;
            let res1 = ((data[3] as u32) << 24) | ((data[4] as u32) << 16)
                | ((data[5] as u32) << 8) | (data[6] as u32);
            let source = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::AuthResp { standalone, res1, source })
        }

        // === OSP Messages ===
        (Direction::Osp, 0x00) => {
            let service_opts = ServiceOptions::from_byte(data[2]);
            let channel = ((data[4] as u16) << 8) | (data[5] as u16);
            let group_addr = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::GrpVChGrant { service_opts, channel, group_addr })
        }

        (Direction::Osp, 0x02) => {
            let ch_a = ((data[3] as u16) << 8) | (data[4] as u16);
            let group_a = ((data[5] as u16) << 8) | (data[6] as u16);
            let ch_b = ((data[7] as u16) << 8) | (data[8] as u16);
            let group_b = data[9] as u16;
            Some(TsbkMessage::GrpVChGrantUpdt { ch_a, group_a, ch_b, group_b })
        }

        (Direction::Osp, 0x04) => {
            let service_opts = ServiceOptions::from_byte(data[2]);
            let channel = ((data[3] as u16) << 8) | (data[4] as u16);
            let target = ((data[5] as u32) << 16)
                | ((data[6] as u32) << 8)
                | (data[7] as u32);
            let source = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::UuVChGrant { service_opts, channel, target, source })
        }

        (Direction::Osp, 0x20) => {
            let aiv = (data[2] & 0x80) != 0;
            let ex = (data[2] & 0x40) != 0;
            let service_type = data[2] & 0x3F;
            let info = ((data[3] as u32) << 24) | ((data[4] as u32) << 16)
                | ((data[5] as u32) << 8) | (data[6] as u32);
            let target = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::AckRspFne { aiv, ex, service_type, info, target })
        }

        (Direction::Osp, 0x21) => {
            let aiv = (data[2] & 0x80) != 0;
            let service_type = data[2] & 0x3F;
            let reason = data[3];
            let info = ((data[4] as u32) << 16)
                | ((data[5] as u32) << 8)
                | (data[6] as u32);
            let target = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::QueRsp { aiv, service_type, reason, info, target })
        }

        (Direction::Osp, 0x27) => {
            let aiv = (data[2] & 0x80) != 0;
            let service_type = data[2] & 0x3F;
            let reason = data[3];
            let info = ((data[4] as u32) << 16)
                | ((data[5] as u32) << 8)
                | (data[6] as u32);
            let target = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::DenyRsp { aiv, service_type, reason, info, target })
        }

        (Direction::Osp, 0x28) => {
            let aff_announce = data[2] & 0x03;
            let ann_group = ((data[3] as u16) << 8) | (data[4] as u16);
            let group_addr = ((data[5] as u32) << 16)
                | ((data[6] as u32) << 8)
                | (data[7] as u32);
            let source = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::GrpAffRsp { aff_announce, ann_group, group_addr, source })
        }

        (Direction::Osp, 0x2B) => {
            let response_code = data[2] & 0x03;
            let group_addr = ((data[5] as u32) << 16)
                | ((data[6] as u32) << 8)
                | (data[7] as u32);
            let source = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::LocRegRsp { response_code, group_addr, source })
        }

        (Direction::Osp, 0x2C) => {
            let response_code = data[2] & 0x03;
            let wacn_id = ((data[3] as u32) << 12) | ((data[4] as u32) << 4)
                | ((data[5] as u32) >> 4);
            let system_id = (((data[5] as u16) & 0x0F) << 8) | (data[6] as u16);
            let source = ((data[7] as u32) << 8) | (data[8] as u32);
            let assigned = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::URegRsp { response_code, wacn_id, system_id, source, assigned })
        }

        (Direction::Osp, 0x30) => {
            let flags = data[2];
            let time_offset = data[3] as i8;
            let year = 2000 + (data[4] & 0x7F) as u16;
            let month = data[5] & 0x0F;
            let day = data[6] & 0x1F;
            let hours = data[7] & 0x1F;
            let minutes = data[8] & 0x3F;
            let micro_slots = data[9] & 0x3F;
            Some(TsbkMessage::SyncBcst {
                flags, time_offset, year, month, day, hours, minutes, micro_slots,
            })
        }

        (Direction::Osp, 0x31) => {
            let standalone = (data[2] & 0x01) != 0;
            let rand1 = ((data[3] as u32) << 24) | ((data[4] as u32) << 16)
                | ((data[5] as u32) << 8) | (data[6] as u32);
            let target = ((data[8] as u32) << 8) | (data[9] as u32);
            Some(TsbkMessage::AuthDmd { standalone, rand1, target })
        }

        (Direction::Osp, 0x38) => {
            let svc_class = data[3];
            let wacn_id = ((data[4] as u32) << 12) | ((data[5] as u32) << 4)
                | ((data[6] as u32) >> 4);
            let system_id = (((data[6] as u16) & 0x0F) << 8) | (data[7] as u16);
            Some(TsbkMessage::SysSrvBcst { svc_class, wacn_id, system_id })
        }

        (Direction::Osp, 0x3A) => {
            let lra = data[2];
            let active = (data[3] & 0x10) != 0;
            let system_id = (((data[3] as u16) & 0x0F) << 8) | (data[4] as u16);
            let rfss_id = data[5];
            let site_id = data[6];
            let channel = ((data[8] as u16) << 8) | (data[9] as u16);
            Some(TsbkMessage::RfssStsBoast {
                lra, active, system_id, rfss_id, site_id, channel,
            })
        }

        (Direction::Osp, 0x3B) => {
            let lra = data[2];
            let wacn_id = ((data[3] as u32) << 12) | ((data[4] as u32) << 4)
                | ((data[5] as u32) >> 4);
            let system_id = (((data[5] as u16) & 0x0F) << 8);
            let channel = ((data[6] as u16) << 8) | (data[7] as u16);
            let svc_class = data[8];
            Some(TsbkMessage::NetStsBcst { lra, wacn_id, system_id, channel, svc_class })
        }

        (Direction::Osp, 0x3C) => {
            let lra = data[2];
            let cfva = (data[3] >> 4) & 0x0F;
            let system_id = (((data[3] as u16) & 0x0F) << 8) | (data[4] as u16);
            let rfss_id = data[5];
            let site_id = data[6];
            let channel = ((data[8] as u16) << 8) | (data[9] as u16);
            let svc_class = data[9];  // NOTE: may overlap -- see extraction notes
            Some(TsbkMessage::AdjStsBcst {
                lra, cfva, system_id, rfss_id, site_id, channel, svc_class,
            })
        }

        (Direction::Osp, 0x3D) => {
            let identifier = (data[2] >> 4) & 0x0F;
            // Simplified -- full parsing in Section 6
            Some(TsbkMessage::IdenUp {
                identifier,
                base_freq: 0,
                spacing: 0,
                offset: 0,
            })
        }

        // Default: unknown/unhandled opcode
        _ => Some(TsbkMessage::Unknown {
            opcode: header.opcode,
            direction,
            payload: data[2..10].try_into().unwrap(),
        }),
    }
}
```

### 8.2 SDRTrunk Cross-Reference

SDRTrunk (Java) parses TSBKs in `io.github.dsheirer.module.decode.p25.phase1.message.tsbk`.
Key classes:

| SDRTrunk Class | AABC-E Opcode | Direction |
|----------------|---------------|-----------|
| `OSPGroupVoiceChannelGrant` | OSP 0x00 | Outbound |
| `OSPGroupVoiceChannelGrantUpdate` | OSP 0x02 | Outbound |
| `OSPUnitToUnitVoiceChannelGrant` | OSP 0x04 | Outbound |
| `OSPNetworkStatusBroadcast` | OSP 0x3B | Outbound |
| `OSPRFSSStatusBroadcast` | OSP 0x3A | Outbound |
| `OSPAdjacentStatusBroadcast` | OSP 0x3C | Outbound |
| `OSPIdentifierUpdate` | OSP 0x3D | Outbound |
| `OSPIdentifierUpdateTDMA` | OSP 0x33 | Outbound |
| `OSPIdentifierUpdateVUHF` | OSP 0x34 | Outbound |
| `OSPSystemServiceBroadcast` | OSP 0x38 | Outbound |
| `OSPSecondaryControlChannelBroadcast` | OSP 0x39 | Outbound |
| `OSPAcknowledgeResponse` | OSP 0x20 | Outbound |
| `OSPDenyResponse` | OSP 0x27 | Outbound |
| `OSPQueuedResponse` | OSP 0x21 | Outbound |
| `OSPUnitRegistrationResponse` | OSP 0x2C | Outbound |
| `OSPGroupAffiliationResponse` | OSP 0x28 | Outbound |
| `ISPGroupVoiceServiceRequest` | ISP 0x00 | Inbound |
| `ISPUnitToUnitVoiceServiceRequest` | ISP 0x04 | Inbound |
| `ISPUnitRegistrationRequest` | ISP 0x2C | Inbound |
| `ISPGroupAffiliationRequest` | ISP 0x28 | Inbound |
| `ISPEmergencyAlarmRequest` | ISP 0x27 | Inbound |

### 8.3 OP25 Cross-Reference

OP25 (C++) parses TSBKs in `op25/gr-op25_repeater/lib/p25p1_fdma.cc` and
`op25/gr-op25_repeater/lib/rx_tsbk.cc`. Key opcode handling:

- `rx_tsbk()` dispatches on `opcode = tsbk[0] & 0x3f` after CRC validation
- Outbound opcodes matched via case statements covering 0x00 (grp voice grant),
  0x02 (grant update), 0x3B (net status), 0x3A (rfss status), 0x3C (adj status),
  0x3D (iden update), etc.
- Channel frequency resolution done via `iden_map` lookup table keyed by 4-bit identifier

---

## 9. FDMA TSBK to TDMA MAC Opcode Cross-Reference

### 9.1 Mapping Principle

TDMA MAC messages in TIA-102.BBAD-A use a partition system (B1/B2 qualifier bits) to
indicate their relationship to FDMA TSBK messages:

- **B1=0, B2=1 (Partition 0b01):** Abbreviated format -- MCO value equals the FDMA TSBK
  opcode. Direction (ISP vs. OSP) disambiguated by burst type.
- **B1=1, B2=1 (Partition 0b11):** Extended/Explicit format -- MCO value equals the FDMA
  TSBK opcode. These carry full SUIDs or explicit T/R channel fields.
- **B1=0, B2=0 (Partition 0b00):** Unique TDMA messages with no FDMA equivalent.

### 9.2 Complete Cross-Reference Table -- OSP Messages

The MCO (6-bit) in TDMA partition 0b01/0b11 maps directly to the FDMA TSBK opcode:

| FDMA Opcode | FDMA Alias | TDMA 0b01 (Abbreviated) | TDMA 0b11 (Extended/Explicit) |
|-------------|-----------|------------------------|------------------------------|
| 0x00 | GRP_V_CH_GRANT | Group Voice Channel Grant - Implicit (9 oct) | Group Voice Channel Grant - Explicit (11 oct) |
| 0x02 | GRP_V_CH_GRANT_UPDT | Group Voice Channel Grant Update - Implicit (9 oct) | -- |
| 0x03 | GRP_V_CH_GRANT_UPDT_EXP | -- | Group Voice Channel Grant Update - Explicit (8 oct) |
| 0x04 | UU_V_CH_GRANT | UU Voice Service CH Grant - Abbreviated (9 oct) | UU Voice Service CH Grant - Extended VCH (15 oct) / Extended LCCH (32 oct) |
| 0x05 | UU_ANS_REQ | UU Answer Request - Abbreviated (8 oct) | UU Answer Request - Extended (12 oct) |
| 0x06 | UU_V_CH_GRANT_UPDT | UU Voice CH Grant Update - Abbreviated (9 oct) | UU Voice CH Grant Update - Extended VCH (15 oct) / Extended LCCH (32 oct) |
| 0x08 | TELE_INT_CH_GRANT | TELE INT Voice CH Grant - Implicit (10 oct) | TELE INT Voice CH Grant - Explicit (12 oct) |
| 0x09 | TELE_INT_CH_GRANT_UPDT | TELE INT Voice CH Grant Update - Implicit (10 oct) | TELE INT Voice CH Grant Update - Explicit (12 oct) |
| 0x0A | TELE_INT_ANS_REQ | TELE INT Answer Request (9 oct) | -- |
| 0x14 | SN_DATA_CHN_GNT | SNDCP Data Channel Grant (9 oct) | -- |
| 0x15 | SN_DATA_PAGE_REQ | SNDCP Data Page Request (7 oct) | -- |
| 0x16 | SN_DATA_CHN_ANN_EXP | -- | SNDCP Data Channel Announcement - Explicit (9 oct) |
| 0x18 | STS_UPDT | Status Update - Abbreviated (10 oct) | Status Update - Extended VCH (14 oct) / Extended LCCH (29 oct) |
| 0x1A | STS_Q | Status Query - Abbreviated (7 oct) | Status Query - Extended VCH (11 oct) / Extended LCCH (27 oct) |
| 0x1C | MSG_UPDT | Message Update - Abbreviated (10 oct) | Message Update - Extended VCH (14 oct) / Extended LCCH (29 oct) |
| 0x1D | RAD_MON_CMD | Radio Unit Monitor Command - Obsolete (8 oct) | Radio Unit Monitor Command - Extended LCCH (29 oct) |
| 0x1E | RAD_MON_ENH_CMD | Radio Unit Monitor Enhanced Cmd - Abbreviated (14 oct) | Radio Unit Monitor Enhanced Cmd - Extended (40 oct) |
| 0x1F | CALL_ALRT | Call Alert - Abbreviated (7 oct) | Call Alert - Extended VCH (11 oct) / Extended LCCH (27 oct) |
| 0x20 | ACK_RSP_FNE | Acknowledge Response FNE - Abbreviated (9 oct) | Acknowledge Response FNE - Extended (28 oct) |
| 0x21 | QUE_RSP | Queued Response (9 oct) | -- |
| 0x24 | EXT_FNCT_CMD | Extended Function Cmd - Abbreviated (9 oct) | Extended Function Cmd - Extended VCH (17 oct) / Extended LCCH (14 oct) |
| 0x27 | DENY_RSP | Deny Response (9 oct) | -- |
| 0x28 | GRP_AFF_RSP | Group Affiliation Response - Abbreviated (10 oct) | Group Affiliation Response - Extended (16 oct) |
| 0x29 | SCCB/SCCB_EXP | SCCB - Implicit (9 oct) | SCCB - Explicit (8 oct) |
| 0x2A | GRP_AFF_Q | Group Affiliation Query - Abbreviated (7 oct) | Group Affiliation Query - Extended (11 oct) |
| 0x2B | LOC_REG_RSP | Location Registration Response (10 oct) | -- |
| 0x2C | U_REG_RSP | Unit Registration Response - Abbreviated (10 oct) | Unit Registration Response - Extended (13 oct) |
| 0x2D | U_REG_CMD | Unit Registration Command (7 oct) | -- |
| 0x2F | U_DE_REG_ACK | Unit Deregistration Acknowledge (9 oct) | -- |
| 0x30 | SYNC_BCST | Synchronization Broadcast (9 oct) | -- |
| 0x31 | AUTH_DMD | Authentication Demand (29 oct) | -- |
| 0x32 | AUTH_FNE_RESP | Authentication FNE Response - Abbreviated (9 oct) | Authentication FNE Response - Extended (16 oct) |
| 0x33 | IDEN_UP_TDMA | IDEN Update TDMA - Abbreviated (9 oct) | IDEN Update TDMA - Extended (14 oct) |
| 0x34 | IDEN_UP_VU | IDEN Update VHF/UHF (9 oct) | -- |
| 0x35 | TIME_DATE_ANN | Time and Date Announcement (9 oct) | -- |
| 0x36 | ROAM_ADDR_CMD | Roaming Address Command (10 oct) | -- |
| 0x37 | ROAM_ADDR_UPDT | Roaming Address Update (13 oct) | -- |
| 0x38 | SYS_SRV_BCST | System Service Broadcast (9 oct) | -- |
| 0x39 | SCCB | SCCB - Implicit (9 oct) | -- |
| 0x3A | RFSS_STS_BCST | RFSS Status Broadcast - Implicit (9 oct) | RFSS Status Broadcast - Explicit (11 oct) |
| 0x3B | NET_STS_BCST | Network Status Broadcast - Implicit (11 oct) | Network Status Broadcast - Explicit (13 oct) |
| 0x3C | ADJ_STS_BCST | Adjacent Status Broadcast - Implicit (9 oct) | Adjacent Status Broadcast - Explicit (11 oct) |
| 0x3D | IDEN_UP | Identifier Update (9 oct) | -- |
| 0x3E | ADJ_STS_BCST_UNC | -- | Adjacent Status Broadcast - Extended Explicit (15 oct) |

### 9.3 Complete Cross-Reference Table -- ISP Messages

| FDMA Opcode | FDMA Alias | TDMA 0b01 (Abbreviated) | TDMA 0b11 (Extended) |
|-------------|-----------|------------------------|---------------------|
| 0x00 | GRP_V_REQ | Group Voice Service Request (7 oct) [MCO=0x01] | -- |
| 0x04 | UU_V_REQ | UU Voice Service Request - Abbreviated (8 oct) | UU Voice Service Request - Extended (12 oct) |
| 0x05 | UU_ANS_RSP | UU Answer Response - Abbreviated (10 oct) | UU Answer Response - Extended (14 oct) |
| 0x08 | TELE_INT_DIAL_REQ | TELE INT Request - Explicit Dialing (variable) | -- |
| 0x09 | TELE_INT_PSTN_REQ | TELE INT Request - Implicit Dialing (7 oct) | -- |
| 0x0A | TELE_INT_ANS_RSP | TELE INT Answer Response (7 oct) | -- |
| 0x12 | SN_DATA_CHN_REQ | SNDCP Data Channel Request (8 oct) | -- |
| 0x13 | SN_DATA_PAGE_RES | SNDCP Data Page Response (9 oct) | -- |
| 0x14 | SN_REC_REQ | SNDCP Reconnect Request (9 oct) | -- |
| 0x18 | STS_UPDT_REQ | Status Update - Abbreviated (10 oct) | Status Update - Extended VCH (14 oct) |
| 0x19 | STS_Q_RSP | Status Query Response - Abbreviated (10 oct) | Status Query Response - Extended (14 oct) |
| 0x1A | STS_Q_REQ | Status Query Request - Abbreviated (8 oct) | Status Query Request - Extended (12 oct) |
| 0x1C | MSG_UPDT_REQ | Message Update - Abbreviated (10 oct) | Message Update - Extended VCH (14 oct) |
| 0x1D | RAD_MON_REQ | Radio Unit Monitor Request - Abbreviated (10 oct) | Radio Unit Monitor Request - Extended (14 oct) |
| 0x1E | RAD_MON_ENH_REQ | Radio Unit Monitor Enhanced - Abbreviated (20 oct) | Radio Unit Monitor Enhanced - Extended (28 oct) |
| 0x1F | CALL_ALRT_REQ | Call Alert Request - Abbreviated (8 oct) | Call Alert Request - Extended VCH (12 oct) |
| 0x20 | ACK_RSP_U | Acknowledge Response Unit - Abbreviated (9 oct) | Acknowledge Response Unit - Extended (13 oct) |
| 0x23 | CAN_SRV_REQ | Cancel Service Request - Abbreviated (10 oct) | Cancel Service Request - Extended (14 oct) |
| 0x24 | EXT_FNCT_RSP | Extended Function Response - Abbreviated (9 oct) | Extended Function Response - Extended VCH (14 oct) |
| 0x27 | EMRG_ALRM_REQ | Emergency Alarm Request (9 oct) | -- |
| 0x28 | GRP_AFF_REQ | Group Affiliation Request - Abbreviated (9 oct) | Group Affiliation Request - Extended (12 oct) |
| 0x29 | GRP_AFF_Q_RSP | -- [uses MCO 0x2A in TDMA] | -- |
| 0x2B | U_DE_REG_REQ | Unit Deregistration Request (9 oct) [MCO=0x2F] | -- |
| 0x2C | U_REG_REQ | Unit Registration Request (10 oct) | -- |
| 0x2D | LOC_REG_REQ | Location Registration Request - Abbreviated (9 oct) | Location Registration Request - Extended (19 oct) |
| 0x32 | IDEN_UP_REQ | Identifier Update Request (6 oct) | -- |
| 0x36 | ROAM_ADDR_REQ | Roaming Address Request - Abbreviated (8 oct) | Roaming Address Request - Extended (12 oct) |
| 0x37 | ROAM_ADDR_RSP | Roaming Address Response (variable) | -- |
| 0x38 | AUTH_RESP | Authentication Response - Abbreviated (10 oct) | Authentication Response - Extended (22 oct) |
| 0x39 | AUTH_RESP_M | Authentication Response Mutual (27 oct) | -- |
| 0x3A | AUTH_FNE_RST | Authentication FNE Result - Abbreviated (6 oct) | Authentication FNE Result - Extended (13 oct) |
| 0x3B | AUTH_SU_DMD | Authentication SU Demand - Abbreviated (5 oct) | Authentication SU Demand - Extended (12 oct) |

NOTE on ISP MCO values: In TDMA partition 0b01, the ISP MCO for GRP_V_REQ is 0x01
(not 0x00), and GRP_AFF_Q_RSP uses MCO 0x2A, and U_DE_REG_REQ uses MCO 0x2F. These
differ from the FDMA opcode because partition 0b01 ISP reuses the same MCO namespace
as partition 0b01 OSP, requiring deconfliction. The direction is determined by burst type.

### 9.4 Unique TDMA Messages (No FDMA Equivalent)

These exist only in TDMA (BBAD-A partition 0b00):

| MCO | Name | Size |
|-----|------|------|
| 0x00 | Null Information | variable |
| 0x01 | Group Voice Channel User - Abbreviated | 7 oct |
| 0x21 | Group Voice Channel User - Extended | 14 oct |
| 0x02 | Unit to Unit Voice Channel User - Abbreviated | 8 oct |
| 0x22 | Unit to Unit Voice Channel User - Extended | 15 oct |
| 0x03 | Telephone Interconnect Voice Channel User | 7 oct |
| 0x05 | Group Voice Channel Grant Update Multiple - Implicit | 16 oct |
| 0x25 | Group Voice Channel Grant Update Multiple - Explicit | 15 oct |
| 0x08 | Null Avoid Zero Bias Information | variable |
| 0x10 | Continuation Fragment | variable |
| 0x11 | Indirect Group Paging without Priority | variable |
| 0x12 | Individual Paging with Priority | variable |
| 0x30 | Power Control Signal Quality | 5 oct |
| 0x31 | MAC_Release | 7 oct |

---

## 10. Reason Code Tables

### 10.1 Cancel Reason Codes (Annex A)

```rust
pub const CANCEL_REASON_CODES: &[(u8, &str)] = &[
    (0x00, "No reason code"),
    (0x10, "Terminate queued condition"),
    (0x20, "Terminate resource assignment"),
    // 0x80-0xFF: User or system definable
];
```

### 10.2 Deny Response Reason Codes (Annex B)

```rust
pub const DENY_REASON_CODES: &[(u8, &str)] = &[
    (0x10, "Requesting unit not valid"),
    (0x11, "Requesting unit not authorized"),
    (0x12, "Requesting unit not registered"),
    (0x20, "Target unit not valid"),
    (0x21, "Target not authorized"),
    (0x22, "SU failed authentication"),
    (0x23, "FNE failed authentication"),
    (0x24, "SU could not be authenticated"),
    (0x25, "Target not registered"),
    (0x26, "Target Home RFSS unknown"),
    (0x27, "Target currently unavailable"),
    (0x2F, "Target refused call"),
    (0x30, "Target group not valid"),
    (0x31, "Target group not authorized"),
    (0x40, "Invalid dialing"),
    (0x41, "Telephone number not authorized"),
    (0x42, "PSTN address not valid"),
    (0x50, "Call time-out"),
    (0x51, "Landline terminated call"),
    (0x52, "SU terminated call"),
    (0x53, "No network resources"),
    (0x54, "No RF resources"),
    (0x55, "Service already being initiated"),
    (0x5F, "Call pre-empted"),
    (0x60, "Site access denial"),
    (0xF0, "Call options not valid"),
    (0xF1, "Protection service option not valid"),
    (0xF2, "Duplex service option not valid"),
    (0xF3, "Circuit or packet mode not valid"),
    (0xFF, "System does not support this service"),
];
```

### 10.3 Queued Response Reason Codes (Annex C)

```rust
pub const QUEUED_REASON_CODES: &[(u8, &str)] = &[
    (0x10, "Requesting unit active in another service"),
    (0x20, "Target unit active in another service"),
    (0x2F, "Target unit has queued this call"),
    (0x30, "Target group currently active"),
    (0x40, "Channel resources not available"),
    (0x41, "Telephone resources not available"),
    (0x42, "Data resources not available"),
    (0x50, "Superseding service currently active"),
];
```

---

## 11. Authentication Framework

### 11.1 Transaction Flow

```
Non-Mutual Authentication:
  FNE ──AUTH_DMD(RAND1)──> SU
  FNE <──AUTH_RESP(RES1)── SU

Mutual Authentication:
  FNE ──AUTH_DMD(RAND1)──────────────> SU
  FNE <──AUTH_RESP_M(RES1, RAND2)──── SU
  FNE ──AUTH_FNE_RESP(RES2)─────────> SU
  FNE <──AUTH_FNE_RST(R2=pass/fail)── SU

SU-Initiated Authentication:
  FNE <──AUTH_SU_DMD──────── SU       (SU demands FNE start auth)
  FNE ──AUTH_DMD(RAND1)───> SU       (FNE begins normal flow)
  ...
```

### 11.2 Key Fields

- RAND1: 32-bit random challenge from FNE (in AUTH_DMD)
- RES1: 32-bit response from SU (computed per TIA-102.AACE)
- RAND2: 40-bit random challenge from SU (in AUTH_RESP_M, MBT only)
- RES2: 32-bit response from FNE (in AUTH_FNE_RESP)
- R2: 1-bit pass/fail result (in AUTH_FNE_RST)
- S: 1-bit standalone flag (indicates auth not tied to a service request)

---

## 12. Extraction Completeness Notes

The source extraction covers the full document structure. The following areas have
complete coverage:

- All ISP and OSP opcodes from Annex D (Tables D-1 and D-2) -- complete
- TSBK and MBT frame format (Section 2) -- complete
- Common field definitions (Section 3) -- complete
- Voice service messages (Section 4) -- complete for all active messages
- Data service messages (Section 5) -- complete; obsolete messages noted
- Control and status messages (Section 6) -- complete for all listed messages
- Cancel/Deny/Queued reason codes (Annexes A-C) -- complete
- Authentication framework -- complete

**Potential gaps noted during extraction:**

1. Some OSP messages in Section 6.2 (STS_UPDT, STS_Q, MSG_UPDT, TIME_DATE_ANN,
   ROAM_ADDR_CMD, ROAM_ADDR_UPDT) have abbreviated descriptions without full
   octet-level field layouts in the extraction. The layouts should mirror their ISP
   counterparts with source/target roles swapped.

2. The exact bit packing of Base Frequency in IDEN_UP messages may span a partial
   nibble boundary. The extraction shows octets 7-9 (24 bits) but the spec defines
   a 32-bit field; 8 bits may be packed with channel spacing in octets 5-6.
   Implementors should verify against SDRTrunk's `IdentifierUpdate.java` which has
   the definitive field extraction.

3. The SCCB_EXP (OSP 0x29) explicit format field layout is not detailed in the
   extraction. It follows the same pattern as SCCB but with explicit T/R channel
   fields separated.

4. Telephone Interconnect messages (TELE_INT_ANS_RSP ISP 0x0A, TELE_INT_ANS_REQ OSP
   0x0A) lack detailed field layouts in the extraction. They follow the same pattern
   as UU_ANS_RSP/REQ.

---

## 13. Quick-Reference: Most Common Messages in the Wild

For decoder implementors, these are the messages seen most frequently on active P25
trunked systems, in approximate order of frequency:

1. **IDEN_UP / IDEN_UP_VU / IDEN_UP_TDMA** (0x3D/0x34/0x33) -- Broadcast continuously
2. **NET_STS_BCST** (0x3B) -- Broadcast continuously
3. **RFSS_STS_BCST** (0x3A) -- Broadcast continuously
4. **ADJ_STS_BCST** (0x3C) -- Broadcast continuously (one per adjacent site)
5. **SYS_SRV_BCST** (0x38) -- Broadcast continuously
6. **SCCB** (0x39) -- Broadcast continuously (if secondary CCs exist)
7. **GRP_V_CH_GRANT** (0x00) -- Every group voice call
8. **GRP_V_CH_GRANT_UPDT** (0x02) -- During active calls
9. **GRP_AFF_RSP** (0x28) -- Radio affiliations
10. **ACK_RSP_FNE** (0x20) -- General acknowledgments
11. **U_REG_RSP** (0x2C) -- Registration responses
12. **DENY_RSP** (0x27) / **QUE_RSP** (0x21) -- Denied/queued requests
