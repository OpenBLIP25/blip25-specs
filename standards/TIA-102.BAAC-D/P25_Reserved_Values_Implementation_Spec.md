# P25 Common Air Interface Reserved Values — Implementation Specification

**Source:** TIA-102.BAAC-D (June 2017), "Project 25 Common Air Interface Reserved Values"
**Extracted:** 2026-04-12 from PDF with structural verification
**Classification:** MESSAGE_FORMAT
**Purpose:** Self-contained lookup tables and validation functions for every standardized
field value in the P25 Common Air Interface. This is the file a developer would
`use p25::reserved_values::*` from every module. No reference to the original PDF required.

---

## 1. Network Access Code (NAC) — 12 bits

The NAC is carried in the NID (Network ID) of every P25 FDMA and TDMA transmission.
It provides network segmentation — receivers compare the received NAC against their
programmed value to decide whether to process a frame.

Referenced by: TIA-102.BAAA-B (FDMA CAI), TSB-102.BBAA (TDMA CAI), TIA-102.AABB-B
(Trunking Control Channel Formats)

### 1.1 Standard Values

| NAC (hex) | NAC (decimal) | NAC (binary) | Meaning |
|-----------|---------------|--------------|---------|
| $293 | 659 | %001010010011 | Default NAC — factory default, safe for normal operation |
| $F7E | 3966 | %111101111110 | Receive any — receiver opens on any NAC (Monitor mode) |
| $F7F | 3967 | %111101111111 | Repeater any — Fixed Station receives and retransmits any NAC |

**Critical constraints:**
- $F7E and $F7F must NEVER be transmitted. They are receive-configuration-only values.
- Valid transmit NAC range: $000-$F7D, $F80-$FFE (all 12-bit values except $F7E, $F7F, $FFF).
- $FFF is not assigned by this spec but is excluded from normal use by convention.

### 1.2 Rust Implementation

```rust
/// Network Access Code — 12-bit network segmentation identifier.
/// Source: TIA-102.BAAC-D Section 2.1
/// Referenced by: TIA-102.BAAA-B (FDMA NID), TSB-102.BBAA (TDMA NID)
pub mod nac {
    /// Default NAC — factory default for all P25 equipment.
    pub const DEFAULT: u16 = 0x293;

    /// Receive-any NAC — causes receiver to open on any received NAC.
    /// MUST NOT be transmitted. Receive-side configuration only.
    pub const RECEIVE_ANY: u16 = 0xF7E;

    /// Repeater-any NAC — causes Fixed Station to receive and retransmit any NAC.
    /// MUST NOT be transmitted. Receive-side configuration only.
    pub const REPEATER_ANY: u16 = 0xF7F;

    /// Maximum valid 12-bit NAC value.
    pub const MAX: u16 = 0xFFF;

    /// Bit width of the NAC field.
    pub const BITS: u8 = 12;

    /// Returns a human-readable name for known NAC values.
    pub fn name(nac: u16) -> &'static str {
        match nac {
            DEFAULT => "Default",
            RECEIVE_ANY => "Receive Any (Monitor)",
            REPEATER_ANY => "Repeater Any (Fixed Station)",
            _ => "User-Defined",
        }
    }

    /// Returns true if the NAC is within the valid 12-bit range.
    #[inline]
    pub fn is_valid(nac: u16) -> bool {
        nac <= MAX
    }

    /// Returns true if this NAC value is safe to transmit.
    /// $F7E and $F7F are receive-only and must never be transmitted.
    #[inline]
    pub fn is_transmittable(nac: u16) -> bool {
        nac <= MAX && nac != RECEIVE_ANY && nac != REPEATER_ANY
    }

    /// Returns true if this is a special (non-user-assignable) NAC value.
    #[inline]
    pub fn is_special(nac: u16) -> bool {
        matches!(nac, DEFAULT | RECEIVE_ANY | REPEATER_ANY)
    }
}
```

---

## 2. Link Control Format (LCF) — 8 bits

The LCF is the first octet of every Link Control Word (LCW). It is the primary dispatch
key for decoding LC messages in voice frames (LDU1, ETDU).

The 8-bit LCF byte is structured as: P(1) | SF(1) | LCO(6) where P=Protected flag,
SF=Standard Format flag, LCO=Link Control Opcode. See TIA-102.AABF-D for full LCW parsing.

Referenced by: TIA-102.BAAA-B (FDMA CAI), TIA-102.AABF-D (LCW Formats), TIA-102.BBAD-A (TDMA MAC)

### 2.1 Standard LCF Values (from BAAC-D)

| LCF (hex) | P | SF | LCO (hex) | Meaning |
|-----------|---|-----|-----------|---------|
| $00 | 0 | 0 | $00 | Group Voice Channel User (Explicit MFID) |
| $03 | 0 | 0 | $03 | Unit-to-Unit Voice Channel User (Explicit MFID) |
| $80 | 1 | 0 | $00 | Encrypted Group Voice Channel User |
| $83 | 1 | 0 | $03 | Encrypted Unit-to-Unit Voice Channel User |

### 2.2 Extended LCF Values (from AABF-D, for completeness)

These are the full set of LCO values used across P25. The P bit (bit 7) adds $80 to
indicate the encrypted variant of any LCO.

| LCF (hex) | LCO | P | Meaning | Format |
|-----------|-----|---|---------|--------|
| $00 | 0 | 0 | Group Voice Channel User | Explicit |
| $02 | 2 | 0 | Group Voice Channel Update | Explicit |
| $03 | 3 | 0 | Unit-to-Unit Voice Channel User | Explicit |
| $04 | 4 | 0 | Group Voice Channel Update — Explicit | Explicit |
| $0F | 15 | 0 | Call Termination / Cancellation | Explicit |
| $10 | 16 | 0 | Group Affiliation Query | Explicit |
| $11 | 17 | 0 | Unit Registration Command | Explicit |
| $15 | 21 | 0 | System Service Broadcast | Explicit |
| $17 | 23 | 0 | Secondary Control Channel Broadcast | Explicit |
| $18 | 24 | 0 | Adjacent Site Status Broadcast | Explicit |
| $19 | 25 | 0 | RFSS Status Broadcast | Explicit |
| $1A | 26 | 0 | Network Status Broadcast | Explicit |
| $20 | 32 | 0 | Protection Parameter Broadcast | Explicit |
| $40 | 0 | 0 | Group Voice Channel User | Implicit (SF=1) |
| $42 | 2 | 0 | Group Voice Channel Update | Implicit (SF=1) |
| $43 | 3 | 0 | Unit-to-Unit Voice Channel User | Implicit (SF=1) |
| $44 | 4 | 0 | Group Voice Channel Update — Explicit | Implicit (SF=1) |
| $80 | 0 | 1 | Encrypted Group Voice Channel User | Explicit |
| $83 | 3 | 1 | Encrypted Unit-to-Unit Voice Channel User | Explicit |

### 2.3 Rust Implementation

```rust
/// Link Control Format — 8-bit dispatch key for LC messages.
/// Source: TIA-102.BAAC-D Section 2.2; full opcodes in TIA-102.AABF-D
/// Referenced by: TIA-102.BAAA-B (LDU1/ETDU), TIA-102.BBAD-A (TDMA MAC)
pub mod lcf {
    /// Bit width of the LCF field.
    pub const BITS: u8 = 8;

    // --- Standard LCF values from BAAC-D ---

    /// Group Voice Channel User (cleartext, explicit MFID)
    pub const GROUP_VOICE: u8 = 0x00;

    /// Unit-to-Unit Voice Channel User (cleartext, explicit MFID)
    pub const UNIT_TO_UNIT_VOICE: u8 = 0x03;

    /// Encrypted Group Voice Channel User
    pub const GROUP_VOICE_ENCRYPTED: u8 = 0x80;

    /// Encrypted Unit-to-Unit Voice Channel User
    pub const UNIT_TO_UNIT_VOICE_ENCRYPTED: u8 = 0x83;

    // --- Extended LCF values from AABF-D ---

    /// Group Voice Channel Update (explicit MFID)
    pub const GROUP_VOICE_UPDATE: u8 = 0x02;

    /// Group Voice Channel Update — Explicit
    pub const GROUP_VOICE_UPDATE_EXPLICIT: u8 = 0x04;

    /// Call Termination / Cancellation
    pub const CALL_TERMINATION: u8 = 0x0F;

    /// Group Affiliation Query
    pub const GROUP_AFFILIATION_QUERY: u8 = 0x10;

    /// Unit Registration Command
    pub const UNIT_REGISTRATION_CMD: u8 = 0x11;

    /// System Service Broadcast
    pub const SYSTEM_SERVICE_BROADCAST: u8 = 0x15;

    /// Secondary Control Channel Broadcast
    pub const SECONDARY_CONTROL_CHANNEL_BROADCAST: u8 = 0x17;

    /// Adjacent Site Status Broadcast
    pub const ADJACENT_SITE_STATUS_BROADCAST: u8 = 0x18;

    /// RFSS Status Broadcast
    pub const RFSS_STATUS_BROADCAST: u8 = 0x19;

    /// Network Status Broadcast
    pub const NETWORK_STATUS_BROADCAST: u8 = 0x1A;

    /// Protection Parameter Broadcast
    pub const PROTECTION_PARAMETER_BROADCAST: u8 = 0x20;

    // --- Implicit MFID variants (SF=1, bit 6 set) ---

    /// Group Voice Channel User (implicit MFID $00)
    pub const GROUP_VOICE_IMPLICIT: u8 = 0x40;

    /// Group Voice Channel Update (implicit MFID $00)
    pub const GROUP_VOICE_UPDATE_IMPLICIT: u8 = 0x42;

    /// Unit-to-Unit Voice Channel User (implicit MFID $00)
    pub const UNIT_TO_UNIT_VOICE_IMPLICIT: u8 = 0x43;

    /// Group Voice Channel Update — Explicit (implicit MFID $00)
    pub const GROUP_VOICE_UPDATE_EXPLICIT_IMPLICIT: u8 = 0x44;

    /// Extract the Protected flag (bit 7). 1 = encrypted payload.
    #[inline]
    pub fn is_protected(lcf: u8) -> bool {
        lcf & 0x80 != 0
    }

    /// Extract the Standard Format flag (bit 6). 1 = implicit MFID $00.
    #[inline]
    pub fn is_implicit_mfid(lcf: u8) -> bool {
        lcf & 0x40 != 0
    }

    /// Extract the Link Control Opcode (bits 5:0).
    #[inline]
    pub fn lco(lcf: u8) -> u8 {
        lcf & 0x3F
    }

    /// Returns a human-readable name for known LCF values.
    pub fn name(lcf: u8) -> &'static str {
        // Normalize: strip P and SF bits to get base LCO for naming
        let p = if lcf & 0x80 != 0 { "Encrypted " } else { "" };
        match lcf & 0x3F {
            0x00 => if lcf & 0x80 != 0 { "Encrypted Group Voice Channel User" }
                    else { "Group Voice Channel User" },
            0x02 => if lcf & 0x80 != 0 { "Encrypted Group Voice Channel Update" }
                    else { "Group Voice Channel Update" },
            0x03 => if lcf & 0x80 != 0 { "Encrypted Unit-to-Unit Voice Channel User" }
                    else { "Unit-to-Unit Voice Channel User" },
            0x04 => if lcf & 0x80 != 0 { "Encrypted Group Voice Channel Update (Explicit)" }
                    else { "Group Voice Channel Update (Explicit)" },
            0x0F => "Call Termination / Cancellation",
            0x10 => "Group Affiliation Query",
            0x11 => "Unit Registration Command",
            0x15 => "System Service Broadcast",
            0x17 => "Secondary Control Channel Broadcast",
            0x18 => "Adjacent Site Status Broadcast",
            0x19 => "RFSS Status Broadcast",
            0x1A => "Network Status Broadcast",
            0x20 => "Protection Parameter Broadcast",
            _ => "Unknown",
        }
    }
}
```

---

## 3. Manufacturer's ID (MFID) — 8 bits

The MFID identifies the standards body or vendor responsible for a message's opcode
space. It appears in explicit-format LC words (octet 1 when SF=0), in HDU frames,
and in trunking control channel messages.

Referenced by: TIA-102.BAAA-B (HDU), TIA-102.AABF-D (LCW Explicit format),
TIA-102.AABC-D (Trunking Messages), TIA-102.BBAD-A (TDMA MAC)

### 3.1 Standard and Known Vendor Values

| MFID (hex) | Decimal | Meaning | Notes |
|------------|---------|---------|-------|
| $00 | 0 | Standard (original P25) | All definitions before April 18, 2001 |
| $01 | 1 | Standard (secondary) | Added April 18, 2001 for opcode conflicts |
| $02-$08 | 2-8 | Reserved for future standard use | |
| $09 | 9 | ASELSAN Inc. | Turkish vendor |
| $10 | 16 | Relm / BK Radio | |
| $18 | 24 | ZETRON Inc. | |
| $20 | 32 | MA-COM Inc. | Now part of L3Harris |
| $28 | 40 | Datron | |
| $30 | 48 | Icom Inc. | |
| $34 | 52 | Cisco Systems | |
| $38 | 56 | EF Johnson / Kenwood Viking | Now part of JVC Kenwood |
| $40 | 64 | Thales/Racal | |
| $48 | 72 | Cycomm | |
| $50 | 80 | Midland | |
| $58 | 88 | Daniels Electronics | |
| $60 | 96 | Tait Electronics | |
| $68 | 104 | Uniden | |
| $70 | 112 | Midstate Communications | |
| $78 | 120 | Vertex Standard (Motorola subsidiary) | |
| $7C | 124 | Airbus DS (formerly Cassidian) | |
| $7E | 126 | Hytera (HYT) | |
| $90 | 144 | Motorola Solutions | Most common proprietary MFID |
| $A0 | 160 | Thales | |
| $A4 | 164 | Harris Corp. / L3Harris | Second most common |
| $B0 | 176 | Simoco | |
| $D8 | 216 | Tait Electronics (alternate) | |

Note: The full MFID registry is maintained by TIA TR-8.15 in a separate document [6].
The values above represent the most commonly encountered MFIDs in operational systems.
Unknown MFIDs should be passed through — they indicate vendor-proprietary content in
the LC payload.

### 3.2 Rust Implementation

```rust
/// Manufacturer's ID — 8-bit vendor/standards-body identifier.
/// Source: TIA-102.BAAC-D Section 2.3; full registry in TIA TR-8.15 MFID Assignments
/// Referenced by: TIA-102.BAAA-B (HDU), TIA-102.AABF-D (LCW), TIA-102.AABC-D (Trunking)
pub mod mfid {
    /// Bit width of the MFID field.
    pub const BITS: u8 = 8;

    /// Standard P25 — all definitions before April 18, 2001.
    pub const STANDARD: u8 = 0x00;

    /// Standard P25 (secondary) — for new definitions where $00 conflicts.
    /// Added April 18, 2001.
    pub const STANDARD_SECONDARY: u8 = 0x01;

    // --- Known vendor MFIDs (from TR-8.15 MFID Assignments) ---
    pub const ASELSAN: u8 = 0x09;
    pub const RELM_BK_RADIO: u8 = 0x10;
    pub const ZETRON: u8 = 0x18;
    pub const MACOM: u8 = 0x20;
    pub const DATRON: u8 = 0x28;
    pub const ICOM: u8 = 0x30;
    pub const CISCO: u8 = 0x34;
    pub const EF_JOHNSON: u8 = 0x38;
    pub const THALES_RACAL: u8 = 0x40;
    pub const CYCOMM: u8 = 0x48;
    pub const MIDLAND: u8 = 0x50;
    pub const DANIELS: u8 = 0x58;
    pub const TAIT: u8 = 0x60;
    pub const UNIDEN: u8 = 0x68;
    pub const MIDSTATE: u8 = 0x70;
    pub const VERTEX_STANDARD: u8 = 0x78;
    pub const AIRBUS_DS: u8 = 0x7C;
    pub const HYTERA: u8 = 0x7E;
    pub const MOTOROLA: u8 = 0x90;
    pub const THALES: u8 = 0xA0;
    pub const HARRIS: u8 = 0xA4;
    pub const SIMOCO: u8 = 0xB0;
    pub const TAIT_ALT: u8 = 0xD8;

    /// Returns true if this is a standard (non-vendor) MFID.
    #[inline]
    pub fn is_standard(mfid: u8) -> bool {
        mfid == STANDARD || mfid == STANDARD_SECONDARY
    }

    /// Returns true if this MFID is in the range reserved for future standards.
    #[inline]
    pub fn is_reserved_standard(mfid: u8) -> bool {
        mfid >= 0x02 && mfid <= 0x08
    }

    /// Returns a human-readable name for known MFID values.
    pub fn name(mfid: u8) -> &'static str {
        match mfid {
            STANDARD => "Standard (P25)",
            STANDARD_SECONDARY => "Standard (P25 Secondary)",
            ASELSAN => "ASELSAN Inc.",
            RELM_BK_RADIO => "Relm / BK Radio",
            ZETRON => "ZETRON Inc.",
            MACOM => "MA-COM Inc.",
            DATRON => "Datron",
            ICOM => "Icom Inc.",
            CISCO => "Cisco Systems",
            EF_JOHNSON => "EF Johnson / Kenwood Viking",
            THALES_RACAL => "Thales/Racal",
            CYCOMM => "Cycomm",
            MIDLAND => "Midland",
            DANIELS => "Daniels Electronics",
            TAIT => "Tait Electronics",
            UNIDEN => "Uniden",
            MIDSTATE => "Midstate Communications",
            VERTEX_STANDARD => "Vertex Standard",
            AIRBUS_DS => "Airbus DS (Cassidian)",
            HYTERA => "Hytera (HYT)",
            MOTOROLA => "Motorola Solutions",
            THALES => "Thales",
            HARRIS => "Harris Corp. / L3Harris",
            SIMOCO => "Simoco",
            TAIT_ALT => "Tait Electronics (alternate)",
            0x02..=0x08 => "Reserved (future standard)",
            _ => "Unknown Vendor",
        }
    }
}
```

---

## 4. Source ID / Destination ID — 24 bits each

Individual radio unit identifiers and group addresses. These fields appear in LC words,
HDU frames, trunking control channel messages, and data packet headers.

Referenced by: TIA-102.BAAA-B (HDU, PDU), TIA-102.AABF-D (LCW), TIA-102.AABC-D
(Trunking), TIA-102.BBAD-A (TDMA MAC), TIA-102.AABG (Conventional Control)

### 4.1 Address Ranges

| Range (hex) | Range (decimal) | Meaning |
|-------------|-----------------|---------|
| $000000 | 0 | No one — never assigned to a radio unit |
| $000001 - $98967F | 1 - 9,999,999 | Individual subscriber unit addresses |
| $989680 - $FEEEFF | 10,000,000 - 16,707,327 | Group/reserved addresses |
| $FEF001 - $FFEFFF | 16,707,585 - 16,773,119 | Conventional 16-bit TGID mapped to 24-bit |
| $FFEF00 - $FFFFFE | 16,773,120 - 16,777,214 | Group/reserved addresses (continued) |
| $FFFFFF | 16,777,215 | Everyone (broadcast) |

### 4.2 16-bit TGID to 24-bit Address Mapping

When a 16-bit TGID ($0001-$FFFF) must be carried in a 24-bit address field (for
Message Update and Status Update messages), it is mapped as follows:

```
24-bit address = $FEF000 + TGID

TGID $0001 -> $FEF001 (16,707,585)
TGID $0002 -> $FEF002 (16,707,586)
...
TGID $FFFF -> $FFEFFF (16,773,119)
```

### 4.3 Rust Implementation

```rust
/// Source ID / Destination ID — 24-bit radio unit and group addresses.
/// Source: TIA-102.BAAC-D Section 2.4
/// Referenced by: TIA-102.BAAA-B, TIA-102.AABF-D, TIA-102.AABC-D, TIA-102.BBAD-A
pub mod address {
    /// Bit width of the Source/Destination ID field.
    pub const BITS: u8 = 24;

    /// Maximum valid 24-bit address.
    pub const MAX: u32 = 0xFFFFFF;

    /// "No one" — never assigned to a radio unit.
    pub const NO_ONE: u32 = 0x000000;

    /// "Everyone" — broadcast address.
    pub const BROADCAST: u32 = 0xFFFFFF;

    /// Start of individual subscriber unit address range (inclusive).
    pub const INDIVIDUAL_MIN: u32 = 0x000001;

    /// End of individual subscriber unit address range (inclusive).
    /// Decimal 9,999,999.
    pub const INDIVIDUAL_MAX: u32 = 0x98967F;

    /// Start of group/reserved address range (inclusive).
    /// Decimal 10,000,000.
    pub const GROUP_MIN: u32 = 0x989680;

    /// End of group/reserved address range (inclusive).
    pub const GROUP_MAX: u32 = 0xFFFFFE;

    /// Base address for 16-bit TGID to 24-bit mapping.
    /// 24-bit address = TGID_MAP_BASE + tgid_16
    pub const TGID_MAP_BASE: u32 = 0xFEF000;

    /// Start of conventional TGID mapped range (TGID $0001).
    pub const TGID_MAP_MIN: u32 = 0xFEF001;

    /// End of conventional TGID mapped range (TGID $FFFF).
    pub const TGID_MAP_MAX: u32 = 0xFFEFFF;

    /// Returns true if the address is within the valid 24-bit range.
    #[inline]
    pub fn is_valid(addr: u32) -> bool {
        addr <= MAX
    }

    /// Returns true if this is an individual subscriber unit address.
    #[inline]
    pub fn is_individual(addr: u32) -> bool {
        addr >= INDIVIDUAL_MIN && addr <= INDIVIDUAL_MAX
    }

    /// Returns true if this is a group or reserved address.
    #[inline]
    pub fn is_group(addr: u32) -> bool {
        addr >= GROUP_MIN && addr <= GROUP_MAX
    }

    /// Returns true if this is a broadcast (everyone) address.
    #[inline]
    pub fn is_broadcast(addr: u32) -> bool {
        addr == BROADCAST
    }

    /// Returns true if this address falls in the conventional TGID mapped range.
    #[inline]
    pub fn is_conventional_tgid(addr: u32) -> bool {
        addr >= TGID_MAP_MIN && addr <= TGID_MAP_MAX
    }

    /// Converts a 16-bit TGID (1-65535) to its 24-bit address representation.
    /// Returns None if tgid is 0 (no one).
    #[inline]
    pub fn tgid_to_address(tgid: u16) -> Option<u32> {
        if tgid == 0 {
            None
        } else {
            Some(TGID_MAP_BASE + tgid as u32)
        }
    }

    /// Extracts the 16-bit TGID from a 24-bit conventional TGID address.
    /// Returns None if the address is not in the mapped TGID range.
    #[inline]
    pub fn address_to_tgid(addr: u32) -> Option<u16> {
        if addr >= TGID_MAP_MIN && addr <= TGID_MAP_MAX {
            Some((addr - TGID_MAP_BASE) as u16)
        } else {
            None
        }
    }

    /// Returns a human-readable classification of an address.
    pub fn classify(addr: u32) -> &'static str {
        match addr {
            NO_ONE => "No One",
            BROADCAST => "Broadcast (Everyone)",
            INDIVIDUAL_MIN..=INDIVIDUAL_MAX => "Individual Subscriber Unit",
            TGID_MAP_MIN..=TGID_MAP_MAX => "Conventional Talk Group (mapped TGID)",
            GROUP_MIN..=GROUP_MAX => "Group / Reserved",
            _ => "Invalid (exceeds 24 bits)",
        }
    }

    /// Returns true if this address is valid as a source address.
    /// Group addresses and broadcast should not be used as source
    /// unless anonymity is required.
    #[inline]
    pub fn is_valid_source(addr: u32) -> bool {
        addr >= INDIVIDUAL_MIN && addr <= INDIVIDUAL_MAX
    }
}
```

---

## 5. Talk Group ID (TGID) — 16 bits

The TGID partitions radio traffic into logical groups. It appears in HDU frames,
trunking control channel grants, and LC messages.

Referenced by: TIA-102.BAAA-B (HDU), TIA-102.AABF-D (LCW Group Call),
TIA-102.AABC-D (Trunking), TIA-102.BBAD-A (TDMA MAC)

### 5.1 Standard Values

| TGID (hex) | Decimal | Meaning |
|------------|---------|---------|
| $0000 | 0 | No one — no talk group assigned |
| $0001 | 1 | Default Talk Group — for systems without explicit partitioning |
| $FFFF | 65,535 | Everyone — all-call talk group |

All values $0002-$FFFE are available for user assignment.

### 5.2 Rust Implementation

```rust
/// Talk Group ID — 16-bit logical group identifier.
/// Source: TIA-102.BAAC-D Section 2.5
/// Referenced by: TIA-102.BAAA-B (HDU), TIA-102.AABF-D (LCW), TIA-102.AABC-D (Trunking)
pub mod tgid {
    /// Bit width of the TGID field.
    pub const BITS: u8 = 16;

    /// "No one" — no talk group assigned.
    pub const NO_ONE: u16 = 0x0000;

    /// Default Talk Group — factory default for systems without partitioning.
    pub const DEFAULT: u16 = 0x0001;

    /// "Everyone" — all-call / broadcast talk group.
    pub const EVERYONE: u16 = 0xFFFF;

    /// Returns a human-readable name for known TGID values.
    pub fn name(tgid: u16) -> &'static str {
        match tgid {
            NO_ONE => "No One",
            DEFAULT => "Default Talk Group",
            EVERYONE => "Everyone (All-Call)",
            _ => "User-Defined",
        }
    }

    /// Returns true if this is a special (non-user-assignable) TGID value.
    #[inline]
    pub fn is_special(tgid: u16) -> bool {
        matches!(tgid, NO_ONE | DEFAULT | EVERYONE)
    }

    /// Returns true if this is a user-assignable TGID ($0002-$FFFE).
    #[inline]
    pub fn is_user_assignable(tgid: u16) -> bool {
        tgid >= 0x0002 && tgid <= 0xFFFE
    }

    /// Returns true if this TGID represents a valid active talk group
    /// (anything other than "no one").
    #[inline]
    pub fn is_active(tgid: u16) -> bool {
        tgid != NO_ONE
    }
}
```

---

## 6. Message Indicator (MI) — 72 bits (9 octets)

The MI provides an initialization vector / synchronization value for voice encryption.
It is carried in HDU frames and LDU2 frames, and changes with each superframe to
maintain crypto sync.

Referenced by: TIA-102.BAAA-B (HDU, LDU2), TIA-102.AAAD-B (Block Encryption Protocol),
TIA-102.AABF-D (LCW encryption fields)

### 6.1 Standard Values

| MI (hex, 18 nibbles) | Meaning |
|----------------------|---------|
| $000000000000000000 | Unencrypted message (null MI). MUST NOT appear in encrypted messages. |

All other 72-bit values are valid for encrypted messages.

### 6.2 Rust Implementation

```rust
/// Message Indicator — 72-bit encryption initialization vector.
/// Source: TIA-102.BAAC-D Section 2.6
/// Referenced by: TIA-102.BAAA-B (HDU, LDU2), TIA-102.AAAD-B (Block Encryption)
pub mod mi {
    /// Bit width of the MI field.
    pub const BITS: u8 = 72;

    /// Byte length of the MI field.
    pub const BYTES: usize = 9;

    /// Null MI — indicates unencrypted message. Never used for encrypted messages.
    pub const UNENCRYPTED: [u8; 9] = [0x00; 9];

    /// Returns true if this MI indicates an unencrypted message (all zeros).
    #[inline]
    pub fn is_unencrypted(mi: &[u8; 9]) -> bool {
        mi.iter().all(|&b| b == 0)
    }

    /// Returns true if this MI is valid for an encrypted message.
    /// The null (all-zero) MI is explicitly prohibited in encrypted messages.
    #[inline]
    pub fn is_valid_for_encryption(mi: &[u8; 9]) -> bool {
        !is_unencrypted(mi)
    }

    /// Returns a human-readable description.
    pub fn describe(mi: &[u8; 9]) -> &'static str {
        if is_unencrypted(mi) {
            "Unencrypted (null MI)"
        } else {
            "Encrypted (non-null MI)"
        }
    }
}
```

---

## 7. Key ID (KID) — 16 bits

The KID identifies the encryption key in use. It is carried alongside the ALGID in
HDU frames and LDU2 frames.

Referenced by: TIA-102.BAAA-B (HDU, LDU2), TIA-102.AAAD-B (Block Encryption Protocol),
TIA-102.AABF-D (LCW)

### 7.1 Standard Values

| KID (hex) | Decimal | Meaning |
|-----------|---------|---------|
| $0000 | 0 | Unencrypted / Default — no key assigned |

All values $0001-$FFFF are valid for encrypted messages.

### 7.2 Rust Implementation

```rust
/// Key ID — 16-bit encryption key identifier.
/// Source: TIA-102.BAAC-D Section 2.7
/// Referenced by: TIA-102.BAAA-B (HDU, LDU2), TIA-102.AAAD-B (Block Encryption)
pub mod kid {
    /// Bit width of the KID field.
    pub const BITS: u8 = 16;

    /// Null/default KID — used for unencrypted messages or as default before
    /// key assignment.
    pub const UNENCRYPTED: u16 = 0x0000;

    /// Returns true if this KID indicates no encryption key is assigned.
    #[inline]
    pub fn is_unencrypted(kid: u16) -> bool {
        kid == UNENCRYPTED
    }

    /// Returns true if this KID value represents an active encryption key.
    #[inline]
    pub fn is_encrypted(kid: u16) -> bool {
        kid != UNENCRYPTED
    }

    /// Returns a human-readable description.
    pub fn describe(kid: u16) -> &'static str {
        if kid == UNENCRYPTED {
            "Unencrypted / Default"
        } else {
            "Encryption Key Active"
        }
    }
}
```

---

## 8. Algorithm ID (ALGID) — 8 bits

The ALGID identifies the encryption algorithm in use. It is carried alongside the KID
in HDU frames and LDU2 frames. This is the primary field for determining whether a
transmission is encrypted and which algorithm to use for decryption.

Referenced by: TIA-102.BAAA-B (HDU, LDU2), TIA-102.AAAD-B (Block Encryption Protocol),
TIA-102.AABF-D (LCW), TIA-102.AABC-D (Trunking)

### 8.1 Complete ALGID Table

#### Type 1 (classified) algorithms — $00-$04, $41

| ALGID | Decimal | Algorithm | Type | Notes |
|-------|---------|-----------|------|-------|
| $00 | 0 | ACCORDION 1.3 | Type 1 | Classified NSA algorithm |
| $01 | 1 | BATON (Auto Even) | Type 1 | Classified NSA algorithm |
| $02 | 2 | FIREFLY Type 1 | Type 1 | Classified NSA algorithm |
| $03 | 3 | MAYFLY Type 1 | Type 1 | Classified NSA algorithm |
| $04 | 4 | SAVILLE | Type 1 | Classified NSA algorithm |
| $41 | 65 | BATON (Auto Odd) | Type 1 | Classified NSA algorithm |

#### Standard Type 3/4 algorithms — $80-$85

| ALGID | Decimal | Algorithm | Type | Notes |
|-------|---------|-----------|------|-------|
| $80 | 128 | Unencrypted | N/A | No encryption algorithm — default for unencrypted equipment |
| $81 | 129 | DES-OFB | Type 3 | DEPRECATED for new equipment. 56-bit key, OFB mode |
| $83 | 131 | Triple DES (3-key) | Type 3 | DEPRECATED for new equipment. 168-bit key |
| $84 | 132 | AES-256 | Type 3 | 256-bit key. Current recommended algorithm |
| $85 | 133 | AES-128 | Type 3 | 128-bit key. Added in Revision C (2011) |

#### Known proprietary algorithms

| ALGID | Decimal | Algorithm | Vendor | Notes |
|-------|---------|-----------|--------|-------|
| $88 | 136 | RC4 | Motorola | Motorola proprietary stream cipher |
| $9F | 159 | DES-XL | Motorola | Motorola proprietary DES variant |
| $AA | 170 | ADP (Advanced Digital Privacy) | Motorola | Motorola proprietary voice privacy |

Note: Full proprietary ALGID registry is maintained in the ALGID Value Assignment Guide [7].

### 8.2 Rust Implementation

```rust
/// Algorithm ID — 8-bit encryption algorithm identifier.
/// Source: TIA-102.BAAC-D Section 2.8; extended registry in ALGID Value Assignment Guide
/// Referenced by: TIA-102.BAAA-B (HDU, LDU2), TIA-102.AAAD-B, TIA-102.AABF-D
pub mod algid {
    /// Bit width of the ALGID field.
    pub const BITS: u8 = 8;

    // --- Type 1 (classified) algorithms ---

    /// ACCORDION 1.3 — Type 1 classified NSA algorithm
    pub const ACCORDION: u8 = 0x00;

    /// BATON (Auto Even) — Type 1 classified NSA algorithm
    pub const BATON_AUTO_EVEN: u8 = 0x01;

    /// FIREFLY Type 1 — Type 1 classified NSA algorithm
    pub const FIREFLY: u8 = 0x02;

    /// MAYFLY Type 1 — Type 1 classified NSA algorithm
    pub const MAYFLY: u8 = 0x03;

    /// SAVILLE — Type 1 classified NSA algorithm
    pub const SAVILLE: u8 = 0x04;

    /// BATON (Auto Odd) — Type 1 classified NSA algorithm
    pub const BATON_AUTO_ODD: u8 = 0x41;

    // --- Standard Type 3/4 algorithms ---

    /// Unencrypted — no encryption algorithm. Default for clear equipment.
    pub const UNENCRYPTED: u8 = 0x80;

    /// DES-OFB — DEPRECATED. 56-bit DES in Output Feedback mode.
    pub const DES: u8 = 0x81;

    /// 3-key Triple DES — DEPRECATED. 168-bit effective key.
    pub const TRIPLE_DES: u8 = 0x83;

    /// AES-256 — Current recommended standard. 256-bit key.
    pub const AES_256: u8 = 0x84;

    /// AES-128 — 128-bit key. Added in BAAC-C (2011).
    pub const AES_128: u8 = 0x85;

    // --- Known proprietary algorithms ---

    /// RC4 — Motorola proprietary stream cipher
    pub const RC4: u8 = 0x88;

    /// DES-XL — Motorola proprietary DES variant
    pub const DES_XL: u8 = 0x9F;

    /// ADP (Advanced Digital Privacy) — Motorola proprietary voice privacy
    pub const ADP: u8 = 0xAA;

    /// Returns a human-readable name for known ALGID values.
    pub fn name(algid: u8) -> &'static str {
        match algid {
            ACCORDION => "ACCORDION 1.3",
            BATON_AUTO_EVEN => "BATON (Auto Even)",
            FIREFLY => "FIREFLY Type 1",
            MAYFLY => "MAYFLY Type 1",
            SAVILLE => "SAVILLE",
            BATON_AUTO_ODD => "BATON (Auto Odd)",
            UNENCRYPTED => "Unencrypted",
            DES => "DES-OFB (deprecated)",
            TRIPLE_DES => "Triple DES (deprecated)",
            AES_256 => "AES-256",
            AES_128 => "AES-128",
            RC4 => "RC4 (Motorola)",
            DES_XL => "DES-XL (Motorola)",
            ADP => "ADP (Motorola)",
            _ => "Unknown",
        }
    }

    /// Returns true if this ALGID indicates no encryption.
    #[inline]
    pub fn is_unencrypted(algid: u8) -> bool {
        algid == UNENCRYPTED
    }

    /// Returns true if this ALGID is a Type 1 (classified) algorithm.
    #[inline]
    pub fn is_type1(algid: u8) -> bool {
        matches!(algid, ACCORDION | BATON_AUTO_EVEN | FIREFLY | MAYFLY | SAVILLE | BATON_AUTO_ODD)
    }

    /// Returns true if this ALGID is a standard Type 3 algorithm.
    #[inline]
    pub fn is_type3(algid: u8) -> bool {
        matches!(algid, DES | TRIPLE_DES | AES_256 | AES_128)
    }

    /// Returns true if this ALGID is deprecated (DES or Triple DES).
    #[inline]
    pub fn is_deprecated(algid: u8) -> bool {
        matches!(algid, DES | TRIPLE_DES)
    }

    /// Returns true if this ALGID is a known proprietary algorithm.
    #[inline]
    pub fn is_proprietary(algid: u8) -> bool {
        matches!(algid, RC4 | DES_XL | ADP)
    }

    /// Returns the key size in bits for known algorithms, or None if unknown.
    pub fn key_bits(algid: u8) -> Option<u16> {
        match algid {
            DES => Some(56),
            TRIPLE_DES => Some(168),
            AES_256 => Some(256),
            AES_128 => Some(128),
            _ => None,
        }
    }

    /// Returns the algorithm classification category.
    pub fn classification(algid: u8) -> &'static str {
        match algid {
            UNENCRYPTED => "None",
            ACCORDION..=SAVILLE | BATON_AUTO_ODD => "Type 1 (Classified)",
            DES | TRIPLE_DES | AES_256 | AES_128 => "Type 3 (Unclassified)",
            RC4 | DES_XL | ADP => "Proprietary",
            _ => "Unknown",
        }
    }
}
```

---

## 9. Service Access Point (SAP) — 6 bits

The SAP identifies the data service type for packet data frames. It is the primary
demultiplexing field in the P25 data system, routing packets to the correct upper-layer
protocol handler.

Referenced by: TIA-102.BAAA-B (PDU header), TIA-102.AABC-D (Trunking data grants),
TIA-102.BBAD-A (TDMA data)

### 9.1 Complete SAP Table

| SAP (hex) | Decimal | Meaning | Status |
|-----------|---------|---------|--------|
| $00 | 0 | Unencrypted User Data | Active |
| $01 | 1 | Encrypted User Data | Active |
| $02 | 2 | Circuit Data | **Deprecated** (Revision D, withdrawn Jan 2010) |
| $03 | 3 | Circuit Data Control | **Deprecated** (Revision D, withdrawn Jan 2010) |
| $04 | 4 | Packet Data | Active |
| $05 | 5 | Address Resolution Protocol (ARP) | Active |
| $06 | 6 | SNDCP Packet Data Control | Active |
| $07-$0E | 7-14 | Reserved | |
| $0F | 15 | Packet Data Scan Preamble | Active |
| $10-$1C | 16-28 | Reserved | |
| $1D | 29 | Packet Data Encryption Support | Active |
| $1E | 30 | Reserved | |
| $1F | 31 | Extended Address (symmetric addressing) | Active |
| $20 | 32 | Registration and Authorization | Active |
| $21 | 33 | Channel Re-assignment | Active |
| $22 | 34 | System Configuration | Active |
| $23 | 35 | SU Loop-Back | Active |
| $24 | 36 | SU Statistics | Active |
| $25 | 37 | SU Out-of-Service | Active |
| $26 | 38 | SU Paging | Active |
| $27 | 39 | SU Configuration | Active |
| $28 | 40 | Unencrypted Key Management Message | Active |
| $29 | 41 | Encrypted Key Management Message | Active |
| $2A-$2F | 42-47 | Reserved | |
| $30 | 48 | Location Service | Active |
| $31-$3C | 49-60 | Reserved | |
| $3D | 61 | Trunking Control | Active |
| $3E | 62 | Reserved | |
| $3F | 63 | Protected Trunking Control | Active |

### 9.2 Rust Implementation

```rust
/// Service Access Point — 6-bit data service type identifier.
/// Source: TIA-102.BAAC-D Section 2.9
/// Referenced by: TIA-102.BAAA-B (PDU), TIA-102.AABC-D (Trunking), TIA-102.BBAD-A
pub mod sap {
    /// Bit width of the SAP field.
    pub const BITS: u8 = 6;

    /// Maximum valid 6-bit SAP value.
    pub const MAX: u8 = 0x3F;

    /// Unencrypted User Data
    pub const UNENCRYPTED_USER_DATA: u8 = 0x00;

    /// Encrypted User Data
    pub const ENCRYPTED_USER_DATA: u8 = 0x01;

    /// Circuit Data — DEPRECATED (specification withdrawn January 2010)
    pub const CIRCUIT_DATA: u8 = 0x02;

    /// Circuit Data Control — DEPRECATED (specification withdrawn January 2010)
    pub const CIRCUIT_DATA_CONTROL: u8 = 0x03;

    /// Packet Data
    pub const PACKET_DATA: u8 = 0x04;

    /// Address Resolution Protocol (ARP)
    pub const ARP: u8 = 0x05;

    /// SNDCP Packet Data Control
    pub const SNDCP_CONTROL: u8 = 0x06;

    /// Packet Data Scan Preamble
    pub const PACKET_DATA_PREAMBLE: u8 = 0x0F;

    /// Packet Data Encryption Support
    pub const PACKET_DATA_ENCRYPTION: u8 = 0x1D;

    /// Extended Address — for symmetric addressing
    pub const EXTENDED_ADDRESS: u8 = 0x1F;

    /// Registration and Authorization
    pub const REGISTRATION: u8 = 0x20;

    /// Channel Re-assignment
    pub const CHANNEL_REASSIGNMENT: u8 = 0x21;

    /// System Configuration
    pub const SYSTEM_CONFIG: u8 = 0x22;

    /// SU Loop-Back
    pub const LOOPBACK: u8 = 0x23;

    /// SU Statistics
    pub const STATISTICS: u8 = 0x24;

    /// SU Out-of-Service
    pub const OUT_OF_SERVICE: u8 = 0x25;

    /// SU Paging
    pub const PAGING: u8 = 0x26;

    /// SU Configuration
    pub const SU_CONFIG: u8 = 0x27;

    /// Unencrypted Key Management Message
    pub const KEY_MGMT_UNENCRYPTED: u8 = 0x28;

    /// Encrypted Key Management Message
    pub const KEY_MGMT_ENCRYPTED: u8 = 0x29;

    /// Location Service
    pub const LOCATION_SERVICE: u8 = 0x30;

    /// Trunking Control
    pub const TRUNKING_CONTROL: u8 = 0x3D;

    /// Protected Trunking Control
    pub const PROTECTED_TRUNKING_CONTROL: u8 = 0x3F;

    /// Returns true if the SAP is within the valid 6-bit range.
    #[inline]
    pub fn is_valid(sap: u8) -> bool {
        sap <= MAX
    }

    /// Returns true if this SAP value is deprecated.
    #[inline]
    pub fn is_deprecated(sap: u8) -> bool {
        matches!(sap, CIRCUIT_DATA | CIRCUIT_DATA_CONTROL)
    }

    /// Returns true if this SAP value is assigned (not reserved).
    pub fn is_assigned(sap: u8) -> bool {
        matches!(sap,
            UNENCRYPTED_USER_DATA | ENCRYPTED_USER_DATA |
            CIRCUIT_DATA | CIRCUIT_DATA_CONTROL |
            PACKET_DATA | ARP | SNDCP_CONTROL |
            PACKET_DATA_PREAMBLE | PACKET_DATA_ENCRYPTION |
            EXTENDED_ADDRESS |
            REGISTRATION | CHANNEL_REASSIGNMENT | SYSTEM_CONFIG |
            LOOPBACK | STATISTICS | OUT_OF_SERVICE | PAGING | SU_CONFIG |
            KEY_MGMT_UNENCRYPTED | KEY_MGMT_ENCRYPTED |
            LOCATION_SERVICE | TRUNKING_CONTROL | PROTECTED_TRUNKING_CONTROL
        )
    }

    /// Returns a human-readable name for known SAP values.
    pub fn name(sap: u8) -> &'static str {
        match sap {
            UNENCRYPTED_USER_DATA => "Unencrypted User Data",
            ENCRYPTED_USER_DATA => "Encrypted User Data",
            CIRCUIT_DATA => "Circuit Data (deprecated)",
            CIRCUIT_DATA_CONTROL => "Circuit Data Control (deprecated)",
            PACKET_DATA => "Packet Data",
            ARP => "Address Resolution Protocol (ARP)",
            SNDCP_CONTROL => "SNDCP Packet Data Control",
            PACKET_DATA_PREAMBLE => "Packet Data Scan Preamble",
            PACKET_DATA_ENCRYPTION => "Packet Data Encryption Support",
            EXTENDED_ADDRESS => "Extended Address (symmetric)",
            REGISTRATION => "Registration and Authorization",
            CHANNEL_REASSIGNMENT => "Channel Re-assignment",
            SYSTEM_CONFIG => "System Configuration",
            LOOPBACK => "SU Loop-Back",
            STATISTICS => "SU Statistics",
            OUT_OF_SERVICE => "SU Out-of-Service",
            PAGING => "SU Paging",
            SU_CONFIG => "SU Configuration",
            KEY_MGMT_UNENCRYPTED => "Unencrypted Key Management",
            KEY_MGMT_ENCRYPTED => "Encrypted Key Management",
            LOCATION_SERVICE => "Location Service",
            TRUNKING_CONTROL => "Trunking Control",
            PROTECTED_TRUNKING_CONTROL => "Protected Trunking Control",
            _ => "Reserved",
        }
    }
}
```

---

## 10. Service Options — 8 bits (bit field)

The Service Options octet appears in trunking control channel messages (TIA-102.AABC-D)
and certain LC words (TIA-102.AABF-D). It encodes call attributes as individual bit flags.

Referenced by: TIA-102.AABC-D (Trunking Messages), TIA-102.AABF-D (LCW),
TIA-102.BBAD-A (TDMA MAC)

### 10.1 Bit Field Layout

```
Bit:    7     6     5     4     3     2     1     0
      +-----+-----+-----+-----+-----+-----+-----+-----+
      |  E  |  P  |  D  |  M  |    Priority (4 bits)   |
      +-----+-----+-----+-----+-----+-----+-----+-----+
```

| Bit(s) | Field | Width | Meaning |
|--------|-------|-------|---------|
| [7] | E (Emergency) | 1 | 1 = Emergency call |
| [6] | P (Protected) | 1 | 1 = Encrypted/protected call |
| [5] | D (Duplex) | 1 | 1 = Full duplex; 0 = Half duplex |
| [4] | M (Mode) | 1 | 1 = Circuit mode; 0 = Packet mode |
| [3:0] | Priority | 4 | Call priority level (0-15, higher = more urgent) |

### 10.2 Rust Implementation

```rust
/// Service Options — 8-bit call attribute bit field.
/// Source: Derived from TIA-102.AABC-D and TIA-102.AABF-D service option fields
/// Referenced by: TIA-102.AABC-D (Trunking), TIA-102.AABF-D (LCW), TIA-102.BBAD-A
pub mod service_options {
    /// Bit width of the Service Options field.
    pub const BITS: u8 = 8;

    // --- Bit masks ---

    /// Emergency flag — bit 7. 1 = emergency call.
    pub const EMERGENCY_MASK: u8 = 0x80;

    /// Protected (encrypted) flag — bit 6. 1 = encrypted/protected call.
    pub const PROTECTED_MASK: u8 = 0x40;

    /// Duplex flag — bit 5. 1 = full duplex; 0 = half duplex.
    pub const DUPLEX_MASK: u8 = 0x20;

    /// Mode flag — bit 4. 1 = circuit mode; 0 = packet mode.
    pub const MODE_MASK: u8 = 0x10;

    /// Priority field — bits 3:0. Call priority level 0-15.
    pub const PRIORITY_MASK: u8 = 0x0F;

    /// Extract emergency flag.
    #[inline]
    pub fn is_emergency(opts: u8) -> bool {
        opts & EMERGENCY_MASK != 0
    }

    /// Extract protected (encrypted) flag.
    #[inline]
    pub fn is_protected(opts: u8) -> bool {
        opts & PROTECTED_MASK != 0
    }

    /// Extract duplex flag.
    #[inline]
    pub fn is_duplex(opts: u8) -> bool {
        opts & DUPLEX_MASK != 0
    }

    /// Extract mode flag. true = circuit mode, false = packet mode.
    #[inline]
    pub fn is_circuit_mode(opts: u8) -> bool {
        opts & MODE_MASK != 0
    }

    /// Extract priority level (0-15).
    #[inline]
    pub fn priority(opts: u8) -> u8 {
        opts & PRIORITY_MASK
    }

    /// Construct a Service Options byte from individual fields.
    #[inline]
    pub fn build(emergency: bool, protected: bool, duplex: bool, circuit_mode: bool, priority: u8) -> u8 {
        let mut opts: u8 = 0;
        if emergency { opts |= EMERGENCY_MASK; }
        if protected { opts |= PROTECTED_MASK; }
        if duplex { opts |= DUPLEX_MASK; }
        if circuit_mode { opts |= MODE_MASK; }
        opts |= priority & PRIORITY_MASK;
        opts
    }

    /// Returns a human-readable summary of the service options byte.
    pub fn describe(opts: u8) -> alloc::string::String {
        let mut parts = alloc::vec::Vec::new();
        if is_emergency(opts) { parts.push("EMERGENCY"); }
        if is_protected(opts) { parts.push("Protected"); }
        if is_duplex(opts) { parts.push("Full-Duplex"); } else { parts.push("Half-Duplex"); }
        if is_circuit_mode(opts) { parts.push("Circuit"); } else { parts.push("Packet"); }
        format!("[{}] Priority={}", parts.join(", "), priority(opts))
    }
}
```

---

## 11. Emergency Indicator (E) — 1 bit

The Emergency Indicator is embedded in the Link Control word of voice messages to
signal an emergency condition.

Referenced by: TIA-102.BAAA-B (FDMA CAI), TIA-102.AABF-D (LCW), TIA-102.BBAD-A (TDMA MAC)

### 11.1 Values

| E | Meaning |
|---|---------|
| 0 | Routine, non-emergency condition |
| 1 | Emergency condition |

### 11.2 Rust Implementation

```rust
/// Emergency Indicator — 1-bit flag in Link Control words.
/// Source: TIA-102.BAAC-D Section 2.10
/// Referenced by: TIA-102.BAAA-B, TIA-102.AABF-D (LCW), TIA-102.BBAD-A
pub mod emergency {
    /// Bit width of the Emergency Indicator field.
    pub const BITS: u8 = 1;

    /// Routine, non-emergency condition.
    pub const ROUTINE: u8 = 0;

    /// Emergency condition.
    pub const EMERGENCY: u8 = 1;

    /// Returns true if the emergency indicator is set.
    #[inline]
    pub fn is_emergency(e: u8) -> bool {
        e & 0x01 != 0
    }

    /// Returns a human-readable name.
    pub fn name(e: u8) -> &'static str {
        if e & 0x01 != 0 { "Emergency" } else { "Routine" }
    }
}
```

---

## 12. Data Unit ID (DUID) — 4 bits

The DUID identifies the type of data unit in each P25 FDMA frame. It is carried in
the NID (Network ID) codeword along with the NAC. The DUID determines how to parse
the remainder of the frame.

Referenced by: TIA-102.BAAA-B (FDMA CAI — NID structure), TIA-102.AABB-B (Trunking Formats)

### 12.1 Complete DUID Table

| DUID (binary) | Hex | Decimal | Data Unit | CAI | P bit |
|---------------|-----|---------|-----------|-----|-------|
| %0000 | $0 | 0 | Header Data Unit (HDU) | Yes | 0 |
| %0001 | $1 | 1 | Reserved | No | 1 |
| %0010 | $2 | 2 | Reserved | No | 1 |
| %0011 | $3 | 3 | Terminator without subsequent LC (TDU) | Yes | 0 |
| %0100 | $4 | 4 | Reserved | No | 0 |
| %0101 | $5 | 5 | Logical Link Data Unit 1 (LDU1) | Yes | 1 |
| %0110 | $6 | 6 | Reserved | No | 1 |
| %0111 | $7 | 7 | Trunking Signaling Data Unit (TSDU) | No* | 0 |
| %1000 | $8 | 8 | Reserved | No | 0 |
| %1001 | $9 | 9 | Reserved | No | 1 |
| %1010 | $A | 10 | Logical Link Data Unit 2 (LDU2) | Yes | 1 |
| %1011 | $B | 11 | Reserved | No | 0 |
| %1100 | $C | 12 | Packet Data Unit (PDU) | Yes | 0 |
| %1101 | $D | 13 | Reserved | No | 1 |
| %1110 | $E | 14 | Reserved | No | 1 |
| %1111 | $F | 15 | Terminator with subsequent LC (ETDU) | Yes | 0 |

*TSDU (%0111) is defined in BAAC-D but is not marked as a CAI data unit in TIA-102.BAAA-B.

P bit: the expected value of the 64th (last) parity bit in the NID codeword for this DUID.

### 12.2 Rust Implementation

```rust
/// Data Unit ID — 4-bit frame type identifier in the FDMA NID.
/// Source: TIA-102.BAAC-D Section 2.11
/// Referenced by: TIA-102.BAAA-B (FDMA NID), TIA-102.AABB-B (Trunking Formats)
pub mod duid {
    /// Bit width of the DUID field.
    pub const BITS: u8 = 4;

    /// Maximum valid 4-bit DUID value.
    pub const MAX: u8 = 0x0F;

    /// Header Data Unit — first frame of a voice call.
    /// Contains MI, MFID, ALGID, KID, TGID.
    pub const HDU: u8 = 0x0;

    /// Terminator without subsequent Link Control.
    /// Simple call termination, no trailing LC word.
    pub const TDU: u8 = 0x3;

    /// Logical Link Data Unit 1 — voice frame carrying Link Control word.
    /// Contains 9 IMBE voice codewords + 72-bit LC word.
    pub const LDU1: u8 = 0x5;

    /// Trunking Signaling Data Unit.
    /// Carries trunking control channel messages on a traffic channel.
    /// Defined in BAAC-D; not a CAI data unit per BAAA-B.
    pub const TSDU: u8 = 0x7;

    /// Logical Link Data Unit 2 — voice frame carrying encryption sync.
    /// Contains 9 IMBE voice codewords + MI, ALGID, KID.
    pub const LDU2: u8 = 0xA;

    /// Packet Data Unit — carries data packets.
    pub const PDU: u8 = 0xC;

    /// Terminator with subsequent Link Control (Expanded Terminator).
    /// Call termination with a trailing LC word.
    pub const ETDU: u8 = 0xF;

    /// Expected NID parity bit (64th bit) for each DUID value.
    /// Index by DUID value to get the expected P bit.
    pub const PARITY: [u8; 16] = [
        0, // $0 HDU
        1, // $1 Reserved
        1, // $2 Reserved
        0, // $3 TDU
        0, // $4 Reserved
        1, // $5 LDU1
        1, // $6 Reserved
        0, // $7 TSDU
        0, // $8 Reserved
        1, // $9 Reserved
        1, // $A LDU2
        0, // $B Reserved
        0, // $C PDU
        1, // $D Reserved
        1, // $E Reserved
        0, // $F ETDU
    ];

    /// Returns true if this DUID is used in the FDMA Common Air Interface.
    #[inline]
    pub fn is_cai(duid: u8) -> bool {
        matches!(duid, HDU | TDU | LDU1 | LDU2 | PDU | ETDU)
    }

    /// Returns true if this DUID is a valid 4-bit value.
    #[inline]
    pub fn is_valid(duid: u8) -> bool {
        duid <= MAX
    }

    /// Returns true if this DUID is reserved (not assigned to a data unit).
    #[inline]
    pub fn is_reserved(duid: u8) -> bool {
        duid <= MAX && !is_cai(duid) && duid != TSDU
    }

    /// Returns true if this DUID carries voice data (LDU1 or LDU2).
    #[inline]
    pub fn is_voice(duid: u8) -> bool {
        matches!(duid, LDU1 | LDU2)
    }

    /// Returns the expected NID parity bit for a given DUID.
    #[inline]
    pub fn expected_parity(duid: u8) -> u8 {
        PARITY[(duid & 0x0F) as usize]
    }

    /// Returns a human-readable name for known DUID values.
    pub fn name(duid: u8) -> &'static str {
        match duid {
            HDU => "Header Data Unit (HDU)",
            TDU => "Terminator without LC (TDU)",
            LDU1 => "Logical Link Data Unit 1 (LDU1)",
            TSDU => "Trunking Signaling Data Unit (TSDU)",
            LDU2 => "Logical Link Data Unit 2 (LDU2)",
            PDU => "Packet Data Unit (PDU)",
            ETDU => "Terminator with LC (ETDU)",
            0x1 | 0x2 | 0x4 | 0x6 | 0x8 | 0x9 | 0xB | 0xD | 0xE => "Reserved",
            _ => "Invalid",
        }
    }

    /// Returns the short abbreviation for known DUID values.
    pub fn abbreviation(duid: u8) -> &'static str {
        match duid {
            HDU => "HDU",
            TDU => "TDU",
            LDU1 => "LDU1",
            TSDU => "TSDU",
            LDU2 => "LDU2",
            PDU => "PDU",
            ETDU => "ETDU",
            _ => "???",
        }
    }
}
```

---

## 13. Channel Identifier — 4 bits

The Channel Identifier is a 4-bit index into a frequency band lookup table. It appears
in trunking control channel messages that specify channel numbers, allowing the receiver
to convert a logical channel number into an RF frequency.

Referenced by: TIA-102.AABC-D (Trunking Control Channel Messages — IDEN_UP, IDEN_UP_VU),
TIA-102.AABB-B (Trunking Control Channel Formats)

### 13.1 Structure

The Channel Identifier is NOT a fixed lookup table in BAAC-D. Instead, it is a 4-bit
index (values $0-$F, i.e. 0-15) that references a frequency band table broadcast
by the system via IDEN_UP (Identifier Update) messages on the control channel.

Each IDEN_UP message defines for a given Channel Identifier:
- Base frequency (Hz)
- Channel spacing (Hz)
- Transmit offset (Hz)
- Bandwidth

The actual RF frequency for a channel is computed as:
```
frequency = base_freq + (channel_spacing * channel_number)
```

### 13.2 Rust Implementation

```rust
/// Channel Identifier — 4-bit frequency band table index.
/// Source: TIA-102.BAAC-D (field definition); TIA-102.AABC-D (IDEN_UP messages)
/// Referenced by: TIA-102.AABC-D (Trunking Messages), TIA-102.AABB-B (Trunking Formats)
pub mod channel_id {
    /// Bit width of the Channel Identifier field.
    pub const BITS: u8 = 4;

    /// Maximum valid Channel Identifier value.
    pub const MAX: u8 = 0x0F;

    /// Number of possible channel identifier entries.
    pub const TABLE_SIZE: usize = 16;

    /// A frequency band table entry, populated from IDEN_UP control channel messages.
    #[derive(Clone, Debug, Default)]
    pub struct FrequencyBand {
        /// Channel Identifier (0-15)
        pub iden: u8,
        /// Base frequency in Hz
        pub base_freq_hz: u64,
        /// Channel spacing in Hz
        pub channel_spacing_hz: u32,
        /// Transmit offset in Hz (signed — can be negative for repeater input)
        pub tx_offset_hz: i64,
        /// Channel bandwidth in Hz
        pub bandwidth_hz: u32,
    }

    impl FrequencyBand {
        /// Compute the downlink (receive) frequency for a given channel number.
        pub fn rx_frequency_hz(&self, channel_number: u16) -> u64 {
            self.base_freq_hz + (self.channel_spacing_hz as u64 * channel_number as u64)
        }

        /// Compute the uplink (transmit) frequency for a given channel number.
        pub fn tx_frequency_hz(&self, channel_number: u16) -> u64 {
            let rx = self.rx_frequency_hz(channel_number);
            if self.tx_offset_hz >= 0 {
                rx + self.tx_offset_hz as u64
            } else {
                rx - (-self.tx_offset_hz) as u64
            }
        }
    }

    /// Frequency band lookup table — populated at runtime from IDEN_UP messages.
    /// Index by Channel Identifier (0-15). None = not yet received.
    pub type FrequencyTable = [Option<FrequencyBand>; TABLE_SIZE];

    /// Create an empty (uninitialized) frequency table.
    pub fn empty_table() -> FrequencyTable {
        [const { None }; TABLE_SIZE]
    }

    /// Returns true if the Channel Identifier is within the valid 4-bit range.
    #[inline]
    pub fn is_valid(iden: u8) -> bool {
        iden <= MAX
    }
}
```

---

## 14. Composite Encryption State Helper

Multiple fields must be checked together to determine the encryption state of a
transmission. This helper combines ALGID, KID, and MI into a single determination.

### 14.1 Rust Implementation

```rust
/// Composite encryption state — combines ALGID, KID, and MI for a definitive
/// encryption determination.
/// Source: Derived from TIA-102.BAAC-D Sections 2.6, 2.7, 2.8
pub mod encryption {
    use super::{algid, kid, mi};

    /// Encryption state of a P25 transmission.
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    pub enum EncryptionState {
        /// Not encrypted — ALGID=$80, KID=$0000, MI=all-zeros.
        Clear,
        /// Encrypted with a known standard algorithm.
        Encrypted {
            algid_value: u8,
            kid_value: u16,
        },
        /// Inconsistent fields — e.g. ALGID=$80 but non-zero MI.
        /// Indicates a possible decoding error or malformed frame.
        Inconsistent,
    }

    /// Determine the encryption state from the three encryption-related fields.
    pub fn determine(algid_value: u8, kid_value: u16, mi_value: &[u8; 9]) -> EncryptionState {
        let algid_clear = algid::is_unencrypted(algid_value);
        let mi_clear = mi::is_unencrypted(mi_value);

        if algid_clear && mi_clear {
            EncryptionState::Clear
        } else if !algid_clear && !mi_clear {
            EncryptionState::Encrypted {
                algid_value,
                kid_value,
            }
        } else {
            // ALGID says clear but MI is non-zero, or vice versa
            EncryptionState::Inconsistent
        }
    }

    /// Returns a human-readable summary of the encryption state.
    pub fn describe(algid_value: u8, kid_value: u16, mi_value: &[u8; 9]) -> alloc::string::String {
        match determine(algid_value, kid_value, mi_value) {
            EncryptionState::Clear => "Clear (unencrypted)".into(),
            EncryptionState::Encrypted { algid_value, kid_value } => {
                format!("Encrypted: {} (KID=0x{:04X})",
                    algid::name(algid_value), kid_value)
            }
            EncryptionState::Inconsistent => "INCONSISTENT encryption fields".into(),
        }
    }
}
```

---

## 15. Module-Level Re-exports and Validation

### 15.1 Rust Module Structure

```rust
//! P25 Common Air Interface Reserved Values
//!
//! Source: TIA-102.BAAC-D (June 2017)
//!
//! This module provides lookup tables, constants, and validation functions for
//! every standardized field value in the P25 Common Air Interface. Import with:
//!
//!     use p25::reserved_values::*;
//!
//! Or selectively:
//!
//!     use p25::reserved_values::{algid, nac, duid};

#![no_std]
extern crate alloc;

pub mod nac;         // Section 2:  Network Access Code (12 bits)
pub mod lcf;         // Section 3:  Link Control Format (8 bits)
pub mod mfid;        // Section 4:  Manufacturer's ID (8 bits)
pub mod address;     // Section 5:  Source ID / Destination ID (24 bits)
pub mod tgid;        // Section 6:  Talk Group ID (16 bits)
pub mod mi;          // Section 7:  Message Indicator (72 bits)
pub mod kid;         // Section 8:  Key ID (16 bits)
pub mod algid;       // Section 9:  Algorithm ID (8 bits)
pub mod sap;         // Section 10: Service Access Point (6 bits)
pub mod service_options; // Section 11: Service Options (8 bits)
pub mod emergency;   // Section 12: Emergency Indicator (1 bit)
pub mod duid;        // Section 13: Data Unit ID (4 bits)
pub mod channel_id;  // Section 14: Channel Identifier (4 bits)
pub mod encryption;  // Section 15: Composite encryption state helper
```

---

## Appendix A: Quick Reference — All Fields at a Glance

| Field | Bits | Module | Key Constants | Validation |
|-------|------|--------|---------------|------------|
| NAC | 12 | `nac` | DEFAULT=0x293, RECEIVE_ANY=0xF7E, REPEATER_ANY=0xF7F | `is_valid()`, `is_transmittable()` |
| LCF | 8 | `lcf` | GROUP_VOICE=0x00, UNIT_TO_UNIT=0x03, +0x80 encrypted | `is_protected()`, `lco()` |
| MFID | 8 | `mfid` | STANDARD=0x00, STANDARD_SECONDARY=0x01, MOTOROLA=0x90, HARRIS=0xA4 | `is_standard()` |
| Src/Dst ID | 24 | `address` | NO_ONE=0, BROADCAST=0xFFFFFF, INDIVIDUAL_MAX=0x98967F | `is_individual()`, `is_group()`, `is_broadcast()` |
| TGID | 16 | `tgid` | NO_ONE=0, DEFAULT=1, EVERYONE=0xFFFF | `is_active()`, `is_special()` |
| MI | 72 | `mi` | UNENCRYPTED=[0;9] | `is_unencrypted()`, `is_valid_for_encryption()` |
| KID | 16 | `kid` | UNENCRYPTED=0x0000 | `is_unencrypted()` |
| ALGID | 8 | `algid` | UNENCRYPTED=0x80, AES_256=0x84, AES_128=0x85, DES=0x81, TRIPLE_DES=0x83 | `is_unencrypted()`, `is_type1()`, `is_deprecated()` |
| SAP | 6 | `sap` | 23 assigned values, $00-$3F | `is_assigned()`, `is_deprecated()` |
| Service Opts | 8 | `service_options` | EMERGENCY_MASK=0x80, PROTECTED_MASK=0x40 | `is_emergency()`, `is_protected()`, `priority()` |
| Emergency | 1 | `emergency` | ROUTINE=0, EMERGENCY=1 | `is_emergency()` |
| DUID | 4 | `duid` | HDU=0, TDU=3, LDU1=5, LDU2=A, PDU=C, ETDU=F | `is_cai()`, `is_voice()`, `expected_parity()` |
| Channel ID | 4 | `channel_id` | Runtime table from IDEN_UP messages | `is_valid()`, `FrequencyBand` struct |

---

## Appendix B: Cross-Reference — Which Spec Uses Which Fields

| Specification | NAC | LCF | MFID | Src/Dst | TGID | MI | KID | ALGID | SAP | SvcOpt | E | DUID | ChID |
|---------------|-----|-----|------|---------|------|----|-----|-------|-----|--------|---|------|------|
| TIA-102.BAAA-B (FDMA CAI) | X | X | X | X | X | X | X | X | X | | X | X | |
| TIA-102.AABF-D (LCW) | | X | X | X | X | | | | | X | X | | |
| TIA-102.AABC-D (Trunking) | | | X | X | X | | | X | X | X | | | X |
| TIA-102.AABB-B (Trunking Fmt) | X | | | | | | | | | | | X | X |
| TIA-102.BBAD-A (TDMA MAC) | | X | X | X | X | X | X | X | X | X | X | | |
| TIA-102.AAAD-B (Encryption) | | | | | | X | X | X | | | | | |
| TIA-102.AABG (Conv. Control) | | | | X | X | | | | | | | | |

---

*End of implementation specification.*
*Source document: TIA-102.BAAC-D, ANSI/TIA-102.BAAC-D-2017, June 19, 2017.*
