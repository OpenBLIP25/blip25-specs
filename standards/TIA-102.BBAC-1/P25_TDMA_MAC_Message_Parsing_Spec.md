# P25 Phase 2 TDMA MAC Message Parsing — Implementation Specification

**Source:** TIA-102.BBAC-1, Sections 8.3, 8.4.2, Annex B  
**Extracted:** 2026-04-12 from PDF, cross-referenced with rendered pages  
**Purpose:** Self-contained spec for parsing MAC PDU messages on TDMA voice/signaling channels.

---

## 1. MAC PDU Structure

A MAC PDU is a fixed-length Protocol Data Unit carried on the FACCH or SACCH logical channels.
Each PDU contains one or more MAC messages packed sequentially, plus an Offset field and
padding (Null Information Message) if needed.

### 1.1 PDU Types

| PDU Type | Description | Context |
|----------|-------------|---------|
| MAC_ACTIVE | Active call — carries VCU message + optional others | FACCH/SACCH during call |
| MAC_IDLE | No active call on this slot | FACCH/SACCH when unassigned |
| MAC_PTT | Push-to-talk initiation | FACCH (call setup) |
| MAC_END_PTT | End of voice transmission | FACCH (call teardown) |
| MAC_HANGTIME | Between voice transmissions | FACCH/SACCH during hangtime |

### 1.2 PDU Offset Field

Every MAC PDU contains a 3-bit Offset field indicating the position of the next 4V burst
relative to the current signaling burst.

```
Offset  Meaning
------  -------
%000    First 4V is in the next non-SACCH burst on this slot
%001    First 4V is in the 2nd non-SACCH burst
%010    First 4V is in the 3rd non-SACCH burst
%011    First 4V is in the 4th non-SACCH burst
%100    First 4V is in the 5th non-SACCH burst
%101    Inbound: Reserved
        Outbound: First 4V is in the 6th non-SACCH burst
%110    Inbound: Random Access SACCH use
        Outbound: Reserved
%111    No voice framing or unknown voice framing
```

`%111` is used during call setup (before voice arrives), during hangtime, and
in the MAC_END_PTT (no voice follows).

---

## 2. Message Identification

### 2.1 First Byte Structure

Every MAC message begins with a single byte containing three fields:

```
Byte 1:  [ B1 | B2 | MCO(5) | MCO(4) | MCO(3) | MCO(2) | MCO(1) | MCO(0) ]
          bit7  bit6  bit5     bit4     bit3     bit2     bit1     bit0
```

- **B1** (bit 7): Block indicator 1
- **B2** (bit 6): Block indicator 2
- **MCO** (bits 5-0): MAC Channel Opcode (6 bits, values 0-63)

### 2.2 Message Type Determination

The combination of B1, B2, and MCO uniquely identifies the message type:

| B1 | B2 | Meaning |
|----|-----|---------|
| 0  | 0   | Voice channel message (abbreviated) — MCO defines specific message |
| 0  | 0   | Voice channel message (extended) if MCO bit 5 = 1 |
| 0  | 1   | OSP-derived message (abbreviated) — MCO maps to OSP opcode |
| 1  | 1   | OSP-derived message (extended) — same MCO, longer format |
| 1  | 0   | Manufacturer message — MCO defined by manufacturer |

**Abbreviated vs Extended:** For messages with both forms, the abbreviated format uses
B1=0 and the extended format uses B1=1. Extended formats add a Source SUID field
for cross-system identification. MCO bit 5 serves as the extended indicator for
voice channel messages (B1=0, B2=0).

### 2.3 Pseudocode — Message Type Dispatch

```
function identify_message(first_byte) -> MessageType:
    b1  = (first_byte >> 7) & 1
    b2  = (first_byte >> 6) & 1
    mco = first_byte & 0x3F

    if b1 == 1 and b2 == 0:
        return ManufacturerMessage(mco)

    if b1 == 0 and b2 == 0:
        return lookup_voice_channel_message(mco)  // Table in Section 3

    if b1 == 0 and b2 == 1:
        return lookup_osp_abbreviated(mco)         // Table in Section 3

    if b1 == 1 and b2 == 1:
        return lookup_osp_extended(mco)            // Table in Section 3
```

---

## 3. Complete Message Opcode Table

This is the authoritative message identification table from TIA-102.BBAC-1 Table 8-2.

### 3.1 Voice Channel Messages (B1=0, B2=0)

| MCO (hex) | MCO (bin) | Length | Message |
|-----------|-----------|--------|---------|
| 0x00 | %000000 | Variable | Null Information Message |
| 0x01 | %000001 | 7 | Group Voice Channel User — Abbreviated |
| 0x21 | %100001 | 14 | Group Voice Channel User — Extended |
| 0x02 | %000010 | 8 | Unit to Unit Voice Channel User — Abbreviated |
| 0x22 | %100010 | 15 | Unit to Unit Voice Channel User — Extended |
| 0x03 | %000011 | 7 | Telephone Interconnect Voice Channel User |
| 0x05 | %000101 | 16 | Group Voice Channel Grant Update Multiple |
| 0x25 | %100101 | 15 | Group Voice Channel Grant Update Multiple — Explicit |
| 0x12 | %010010 | Variable | Individual Paging Message with Priority |
| 0x11 | %010001 | Variable | Indirect Group Paging Message without Priority |
| 0x30 | %110000 | 5 | Power Control Signal Quality |
| 0x31 | %110001 | 7 | MAC_Release |

### 3.2 OSP-Derived Messages — Abbreviated (B1=0, B2=1)

| MCO (hex) | MCO (bin) | Length | Message |
|-----------|-----------|--------|---------|
| 0x00 | %000000 | 9 | Group Voice Channel Grant — Abbreviated |
| 0x01 | %000001 | 7 | Group Voice Service Request |
| 0x02 | %000010 | 9 | Group Voice Channel Grant Update |
| 0x04 | %000100 | 5 | Unit to Unit Voice Request — Abbreviated |
| 0x04 | %000100 | 9 | Unit to Unit Voice Channel Grant — Abbreviated |
| 0x05 | %000101 | 8 | Unit to Unit Answer Request — Abbreviated |
| 0x06 | %000110 | 9 | Unit to Unit Voice Channel Grant Update — Abbreviated |
| 0x0A | %001010 | 9 | Telephone Interconnect Answer Request |
| 0x0C | %001100 | 10 | Radio Unit Monitor Command — Abbreviated |
| 0x14 | %010100 | 9 | SNDCP Data Channel Grant |
| 0x15 | %010101 | 7 | SNDCP Data Page Request |
| 0x18 | %011000 | 10 | Status Update — Abbreviated |
| 0x1A | %011010 | 7 | Status Query — Abbreviated |
| 0x1C | %011100 | 10 | Message Update — Abbreviated |
| 0x1D | %011101 | 8 | Radio Unit Monitor Command — **OBSOLETE** |
| 0x1E | %011110 | 14 | Radio Unit Monitor Enhanced Command — Abbreviated |
| 0x1F | %011111 | 7 | Call Alert — Abbreviated |
| 0x20 | %100000 | 9 | ACK Response — Abbreviated |
| 0x21 | %100001 | 9 | Queued Response |
| 0x24 | %100100 | 9 | Extended Function Command — Abbreviated |
| 0x27 | %100111 | 9 | Deny Response |
| 0x2A | %101010 | 7 | Group Affiliation Query — Abbreviated |
| 0x2D | %101101 | 7 | Unit Registration Command — Abbreviated |
| 0x33 | %110011 | 9 | Identifier Update for TDMA |
| 0x34 | %110100 | 9 | Identifier Update for VHF/UHF Bands |
| 0x35 | %110101 | 9 | Time and Date Announcement |
| 0x38 | %111000 | 9 | System Service Broadcast |
| 0x39 | %111001 | 9 | Secondary Control Channel Broadcast |
| 0x3A | %111010 | 9 | RFSS Status Broadcast — Abbreviated |
| 0x3B | %111011 | 11 | Network Status Broadcast — Abbreviated |
| 0x3C | %111100 | 9 | Adjacent Status Broadcast — Abbreviated |
| 0x3D | %111101 | 9 | Identifier Update |

### 3.3 OSP-Derived Messages — Extended (B1=1, B2=1)

| MCO (hex) | MCO (bin) | Length | Message |
|-----------|-----------|--------|---------|
| 0x00 | %000000 | 11 | Group Voice Channel Grant — Explicit |
| 0x03 | %000011 | 8 | Group Voice Channel Grant Update — Explicit |
| 0x04 | %000100 | 16 | Unit to Unit Voice Request — Extended |
| 0x04 | %000100 | 15 | Unit to Unit Voice Channel Grant — Extended |
| 0x05 | %000101 | 12 | Unit to Unit Answer Request — Extended |
| 0x06 | %000110 | 15 | Unit to Unit Voice Channel Grant Update — Extended |
| 0x0C | %001100 | 14 | Radio Unit Monitor Command — Extended |
| 0x16 | %010110 | 9 | SNDCP Data Channel Announcement — Explicit |
| 0x18 | %011000 | 14 | Status Update — Extended |
| 0x1A | %011010 | 11 | Status Query — Extended |
| 0x1C | %011100 | 14 | Message Update — Extended |
| 0x1F | %011111 | 11 | Call Alert — Extended |
| 0x24 | %100100 | 14 | Extended Function Command — Extended |
| 0x29 | %101001 | 8 | Secondary Control Channel Broadcast — Explicit |
| 0x2A | %101010 | 11 | Group Affiliation Query — Extended |
| 0x3A | %111010 | 11 | RFSS Status Broadcast — Extended |
| 0x3B | %111011 | 13 | Network Status Broadcast — Extended |
| 0x3C | %111100 | 11 | Adjacent Status Broadcast — Extended |

### 3.4 Manufacturer Messages (B1=1, B2=0)

MCO values are manufacturer-defined. Length is variable, determined by the Length field
in byte 3.

---

## 4. Message Length Determination

Four methods, checked in order:

### Method 1: Fixed Length from Opcode
Most messages have a fixed length determined by the B1/B2/MCO combination.
Use the tables in Section 3.

### Method 2: Variable — Parse Additional Fields
Two messages determine length by parsing internal fields:
- **Individual Paging Message with Priority** (MCO 0x12) — parse to determine count
- **Indirect Group Paging Message without Priority** (MCO 0x11) — parse to determine count

### Method 3: Variable — Fills Remaining PDU Space
Two messages expand to fill remaining PDU space:
- **Null Information Message** (MCO 0x00, B1=0, B2=0) — always last in PDU
- **Manufacturer Message** (B1=1, B2=0) — uses Length field in byte 3

### Method 4: Explicit Length Octet
All messages added after the initial spec publication contain a length field in the
**least significant 6 bits of byte 2** (the byte immediately after the B1/B2/MCO byte).

Messages with explicit length octets (from this addendum):
- Status Update (abbreviated/extended)
- Message Update (abbreviated/extended)
- Radio Unit Monitor Command (abbreviated/extended)
- Extended Function Command — Extended
- Manufacturer Message (length in byte 3)

### Pseudocode — Length Determination

```
function get_message_length(pdu_bytes, offset) -> usize:
    first_byte = pdu_bytes[offset]
    b1  = (first_byte >> 7) & 1
    b2  = (first_byte >> 6) & 1
    mco = first_byte & 0x3F

    // Check fixed-length lookup first
    if let Some(len) = FIXED_LENGTH_TABLE.get((b1, b2, mco)):
        return len

    // Null Information Message — fills rest of PDU
    if b1 == 0 and b2 == 0 and mco == 0x00:
        return pdu_remaining_bytes(offset)

    // Manufacturer Message — length in byte 3
    if b1 == 1 and b2 == 0:
        return pdu_bytes[offset + 2] & 0x3F

    // Messages with explicit length octet
    return pdu_bytes[offset + 1] & 0x3F
```

### PDU Message Ordering Rules

1. Standard messages come first
2. Manufacturer messages come after standard messages
3. **Null Information Message is always last** in the PDU
4. Unknown messages with a length octet can be skipped by older implementations

---

## 5. Message Format Definitions

All byte offsets are 1-indexed per the spec convention. Bit numbering: bit 7 = MSB, bit 0 = LSB.

### 5.1 Group Voice Channel User — Abbreviated (8.3.1.2)

```
Byte  Bits 7-6    Bits 5-0
----  ---------   --------
 1    B1=0 B2=0   MCO = 0x01 (%000001)
 2    Service Options [8 bits]
 3    ┐
 4    │ Group Address [16 bits]
 5    ┘
 6    ┐
 7    │ Source Address [24 bits] (WUID of talker)
 8    ┘
```
Length: 7 bytes. Not new in this addendum — included for completeness.

### 5.2 Group Voice Channel User — Extended (8.3.1.2)

```
Byte  Bits 7-6    Bits 5-0
----  ---------   --------
 1    B1=0 B2=0   MCO = 0x21 (%100001)
 2    Service Options [8 bits]
 3    ┐
 4    │ Group Address [16 bits]
 5    ┘
 6    ┐
 7    │ Source Address [24 bits]
 8    ┘
 9    ┐
10    │
11    │ Source SUID [48 bits] — full WACN+SysID+UnitID
12    │
13    │
14    ┘
```
Length: 14 bytes.

### 5.3 Unit to Unit Voice Channel User — Abbreviated (8.3.1.3)

```
Byte  Bits 7-6    Bits 5-0
----  ---------   --------
 1    B1=0 B2=0   MCO = 0x02 (%000010)
 2    Service Options [8 bits]
 3    ┐
 4    │ Target Address [24 bits]
 5    ┘
 6    ┐
 7    │ Source Address [24 bits]
 8    ┘
```
Length: 8 bytes.

### 5.4 Unit to Unit Voice Channel User — Extended (8.3.1.3)

```
Byte  Bits 7-6    Bits 5-0
----  ---------   --------
 1    B1=0 B2=0   MCO = 0x22 (%100010)
 2    Service Options [8 bits]
 3    ┐
 4    │ Target Address [24 bits]
 5    ┘
 6    ┐
 7    │ Source Address [24 bits]
 8    ┘
 9    ┐
10    │
11    │ Source SUID [48 bits]
12    │
13    │
14    │
15    ┘
```
Length: 15 bytes. If Target Address is unknown, it shall be 0.

### 5.5 MAC_Release (8.3.1.41)

```
Byte  Bits 7-6    Bits 5-0
----  ---------   --------
 1    B1=0 B2=0   MCO = 0x31 (%110001)
 2    [8 bits — includes U/F and C/A flags]
 3    ┐
 4    │ Group Address / reserved [16 bits]
 5    ┘
 6    ┐
 7    │ Source/Target Address [24 bits]
 8    ┘
```
Length: 7 bytes.

Key flags in byte 2:
- **U/F** (Unforced/Forced): 0 = SU may continue, 1 = SU shall cease transmitting
- **C/A** (Call/Audio preemption): 0 = call preemption (leave channel), 1 = audio preemption (stay on channel)

### 5.6 Radio Unit Monitor Enhanced Command — Abbreviated (8.3.1.12)

```
Byte  Bits 7-6    Bits 5-0
----  ---------   --------
 1    B1=0 B2=1   MCO = 0x1E (%011110)
 2    ┐
 3    │ Target Address [24 bits]
 4    ┘
 5    ┐
 6    │ Talkgroup ID [16 bits]
 7    ┘
 8    ┐
 9    │ Source Address [16 bits]  (Note: 16 bits, not 24)
10    SM | TG | Reserved [8 bits]
11    ┐
12    │ TX Time [16 bits]
13    Key ID [8 bits]
14    Alg ID [8 bits]
```
Length: 14 bytes.

Fields:
- **SM**: Stealth Mode flag
- **TG**: Talkgroup call flag (vs unit-to-unit)
- **TX Time**: Transmission duration
- **Key ID**: Encryption key identifier
- **Alg ID**: Encryption algorithm identifier

### 5.7 Extended Function Command — Abbreviated (8.3.1.21)

```
Byte  Bits 7-6    Bits 5-0
----  ---------   --------
 1    B1=0 B2=1   MCO = 0x24 (%100100)
 2    ┐
 3    │
 4    │ Extended Function [40 bits]
 5    │
 6    ┘
 7    ┐
 8    │ Target Address [24 bits]
 9    ┘
```
Length: 9 bytes.

### 5.8 Extended Function Command — Extended (8.3.1.21)

```
Byte  Bits 7-6    Bits 5-0
----  ---------   --------
 1    B1=1 B2=1   MCO = 0x24 (%100100)
 2    RES[2] | Length=17 [6 bits]
 3    ┐
 4    │
 5    │ Extended Function [40 bits]
 6    │
 7    ┘
 8    ┐
 9    │ Target Address [24 bits]
10    ┘
11    ┐
12    │
13    │ Source SUID [48 bits]
14    │
15    │
16    │
17    ┘
```
Length: 14 bytes per Table 8-2 (17 per internal length field — **verify against base spec**).

### 5.9 Radio Unit Monitor Command — OBSOLETE (8.3.1.34)

```
Byte  Bits 7-6    Bits 5-1        Bit 0
----  ---------   -----------     ------
 1    B1=0 B2=1   MCO = 0x1D (%011101)
 2    Reserved [6 bits]           Tx Mult [2 bits]
 3    ┐
 4    │ Source Address [24 bits]
 5    ┘
 6    ┐
 7    │ Target Address [16 bits]
 8    ┘
```
Length: 8 bytes. **OBSOLETE** — replaced by 8.3.1.44. Opcode reserved; do not reuse.

### 5.10 Manufacturer Message (8.3.1.37)

```
Byte  Bits 7-6    Bits 5-0
----  ---------   --------
 1    B1=1 B2=0   MCO (manufacturer-defined)
 2    Manufacturer's ID [8 bits]
 3    RES[2] | Length [6 bits]
 4+   Manufacturer Dependent Information [variable]
```
Length: Variable, determined by Length field in byte 3.

### 5.11 Status Update — Abbreviated (8.3.1.42)

```
Byte  Bits 7-6    Bits 5-0
----  ---------   --------
 1    B1=0 B2=1   MCO = 0x18 (%011000)
 2    RES[2] | Length=10 [6 bits]
 3    ┐
 4    │ Status [24 bits]
 5    ┘
 6    ┐
 7    │ Target Address [24 bits]
 8    ┘
 9    ┐
10    │ Source Address [16 bits]
```
Length: 10 bytes. Echo of a subscriber's status update destined for another subscriber.

### 5.12 Status Update — Extended (8.3.1.42)

```
Byte  Bits 7-6    Bits 5-0
----  ---------   --------
 1    B1=1 B2=1   MCO = 0x18 (%011000)
 2    RES[2] | Length=14 [6 bits]
 3    ┐
 4    │ Status [24 bits]
 5    ┘
 6    ┐
 7    │ Target Address [24 bits]
 8    ┘
 9    ┐
10    │
11    │ Source SUID [40 bits]
12    │
13    │
14    ┘
```
Length: 14 bytes.

### 5.13 Message Update — Abbreviated (8.3.1.43)

```
Byte  Bits 7-6    Bits 5-0
----  ---------   --------
 1    B1=0 B2=1   MCO = 0x1C (%011100)
 2    RES[2] | Length=10 [6 bits]
 3    ┐
 4    │ Message [24 bits]
 5    ┘
 6    ┐
 7    │ Target Address [24 bits]
 8    ┘
 9    ┐
10    │ Source Address [16 bits]
```
Length: 10 bytes. Echo of a subscriber's short data message.

### 5.14 Message Update — Extended (8.3.1.43)

```
Byte  Bits 7-6    Bits 5-0
----  ---------   --------
 1    B1=1 B2=1   MCO = 0x1C (%011100)
 2    RES[2] | Length=14 [6 bits]
 3    ┐
 4    │ Message [24 bits]
 5    ┘
 6    ┐
 7    │ Target Address [24 bits]
 8    ┘
 9    ┐
10    │
11    │ Source SUID [40 bits]
12    │
13    │
14    ┘
```
Length: 14 bytes.

### 5.15 Radio Unit Monitor Command — Abbreviated (8.3.1.44)

Replacement for the obsolete 8.3.1.34 version.

```
Byte  Bits 7-6    Bits 5-0
----  ---------   --------
 1    B1=0 B2=1   MCO = 0x0C (%001100)
 2    RES[2] | Length=10 [6 bits]
 3    TX Time [8 bits]
 4    SM[1] | Reserved[5] | Tx Mult[2]
 5    ┐
 6    │ Target Address [24 bits]
 7    ┘
 8    ┐
 9    │ Source Address [16 bits]
10    ┘
```
Length: 10 bytes.

### 5.16 Radio Unit Monitor Command — Extended (8.3.1.44)

```
Byte  Bits 7-6    Bits 5-0
----  ---------   --------
 1    B1=1 B2=1   MCO = 0x0C (%001100)
 2    RES[2] | Length=14 [6 bits]
 3    TX Time [8 bits]
 4    SM[1] | Reserved[5] | Tx Mult[2]
 5    ┐
 6    │ Target Address [24 bits]
 7    ┘
 8    ┐
 9    │
10    │ Source SUID [40 bits]
11    │
12    │
13    │
14    ┘
```
Length: 14 bytes.

---

## 6. OSP Opcode Reference (Annex B)

Complete mapping of all 64 six-bit MCO opcodes to their OSP (Outbound Signaling Packet)
aliases. Use this when the message originates on the control channel and is echoed on the VCH.

```
MCO    Description                                    Alias                    VCH Sub
-----  ---------------------------------------------  ----------------------  -------
0x00   Group Voice Channel Grant                      GRP_V_CH_GRANT          8.3.1.7
0x01   Reserved                                       --                      --
0x02   Group Voice Channel Grant Update               GRP_V_CH_GRANT_UPDT     8.3.1.8
0x03   Group Voice Channel Grant Update - Explicit    GRP_V_CH_GRANT_UPDT_EXP 8.3.1.9
0x04   Unit To Unit Voice Channel Grant               UU_V_CH_GRANT           8.3.1.10
0x05   Unit To Unit Answer Request                    UU_ANS_REQ              8.3.1.11
0x06   Unit To Unit Voice Channel Grant Update        UU_V_CH_GRANT_UPDT      8.3.1.14
0x07   Reserved                                       --                      --
0x08   Telephone Interconnect Voice Channel Grant      TELE_INT_CH_GRANT       --
0x09   Telephone Interconnect Voice Ch Grant Update    TELE_INT_CH_GRANT_UPDT  --
0x0A   Telephone Interconnect Answer Request          TELE_INT_ANS_REQ        8.3.1.13
0x0B   Reserved                                       --                      --
0x0C   Reserved (now: Radio Unit Monitor Cmd)         --                      8.3.1.44
0x0D   Reserved                                       --                      --
0x0E   Reserved                                       --                      --
0x0F   Reserved                                       --                      --
0x10   Obsolete                                        --                      --
0x11   Obsolete                                        --                      --
0x12   Obsolete                                        --                      --
0x13   Obsolete                                        --                      --
0x14   SNDCP Data Channel Grant                       SN-DATA_CHN_GNT         8.3.1.16
0x15   SNDCP Data Page Request                        SN-DATA_PAGE_REQ        8.3.1.17
0x16   SNDCP Data Channel Announcement - Explicit     SN-DATA_CHN_ANN_EXP    8.3.1.18
0x17   Reserved                                       --                      --
0x18   Status Update                                   STS_UPDT                8.3.1.42
0x19   Reserved                                       --                      --
0x1A   Status Query                                    STS_Q                   8.3.1.29
0x1B   Reserved                                       --                      --
0x1C   Message Update                                  MSG_UPDT                8.3.1.43
0x1D   Radio Unit Monitor Command (OBSOLETE)          RAD_MON_CMD             8.3.1.34
0x1E   Radio Unit Monitor Enhanced Command            --                      8.3.1.12
0x1F   Call Alert                                      CALL_ALRT               8.3.1.20
0x20   Acknowledge Response - FNE                     ACK_RSP_FNE             --
0x21   Queued Response                                 QUE_RSP                 --
0x22   Reserved                                       --                      --
0x23   Reserved                                       --                      --
0x24   Extended Function Command                       EXT_FNCT_CMD            8.3.1.21
0x25   Reserved                                       --                      --
0x26   Reserved                                       --                      --
0x27   Deny Response                                   DENY_RSP                --
0x28   Group Affiliation Response                     GRP_AFF_RSP             --
0x29   Secondary Control Channel Broadcast - Explicit SCCB_EXP                8.3.1.28
0x2A   Group Affiliation Query                        GRP_AFF_Q               8.3.1.22
0x2B   Location Registration Response                 LOC_REG_RSP             --
0x2C   Unit Registration Response                     U_REG_RSP               --
0x2D   Unit Registration Command                      U_REG_CMD               8.3.1.33
0x2E   Authentication Command                         AUTH_CMD                --
0x2F   De-Registration Acknowledge                    U_DE_REG_ACK            --
0x30   Synchronization Broadcast                      SYNC_BCST               --
0x31   Authentication Demand                           AUTH_DMD                --
0x32   Authentication FNE Response                    AUTH_FNE_RESP           --
0x33   Identifier Update for TDMA                     IDEN_UP_TDMA            8.3.1.36
0x34   Identifier Update for VHF/UHF Bands            IDEN_UP_VU              8.3.1.35
0x35   Time and Date Announcement                     TIME_DATE_ANN           --
0x36   Roaming Address Command                        ROAM_ADDR_CMD           --
0x37   Roaming Address Update                         ROAM_ADDR_UPDT          --
0x38   System Service Broadcast                       SYS_SRV_BCST            8.3.1.32
0x39   Secondary Control Channel Broadcast            SCCB                    8.3.1.28
0x3A   RFSS Status Broadcast                          RFSS_STS_BCST           8.3.1.27
0x3B   Network Status Broadcast                       NET_STS_BCST            8.3.1.25
0x3C   Adjacent Status Broadcast                      ADJ_STS_BCST            8.3.1.19
0x3D   Identifier Update                              IDEN_UP                 8.3.1.23
0x3E   Protection Parameter Broadcast                 P_PARM_BCST             --
0x3F   Protection Parameter Update                    P_PARM_UPDT             --
```

---

## 7. PDU Parser Pseudocode

```
function parse_mac_pdu(pdu: &[u8]) -> Vec<MacMessage>:
    messages = []
    offset = 0
    
    while offset < pdu.len():
        first_byte = pdu[offset]
        b1  = (first_byte >> 7) & 1
        b2  = (first_byte >> 6) & 1
        mco = first_byte & 0x3F
        
        // Determine message length
        msg_len = get_message_length(pdu, offset)
        if msg_len == 0 or offset + msg_len > pdu.len():
            break  // Invalid or end of PDU
        
        // Extract message bytes
        msg_bytes = pdu[offset .. offset + msg_len]
        
        // Parse based on type
        match (b1, b2, mco):
            // Voice Channel User messages — most common on VCH
            (0, 0, 0x00) => messages.push(NullInfo)  // End of useful data
                            break
            (0, 0, 0x01) => messages.push(parse_grp_vch_user_abbrev(msg_bytes))
            (0, 0, 0x21) => messages.push(parse_grp_vch_user_extended(msg_bytes))
            (0, 0, 0x02) => messages.push(parse_uu_vch_user_abbrev(msg_bytes))
            (0, 0, 0x22) => messages.push(parse_uu_vch_user_extended(msg_bytes))
            (0, 0, 0x03) => messages.push(parse_tel_vch_user(msg_bytes))
            (0, 0, 0x31) => messages.push(parse_mac_release(msg_bytes))
            
            // OSP-derived messages on VCH
            (0, 1, 0x18) => messages.push(parse_status_update_abbrev(msg_bytes))
            (1, 1, 0x18) => messages.push(parse_status_update_extended(msg_bytes))
            (0, 1, 0x1C) => messages.push(parse_message_update_abbrev(msg_bytes))
            (1, 1, 0x1C) => messages.push(parse_message_update_extended(msg_bytes))
            (0, 1, 0x0C) => messages.push(parse_rad_mon_cmd_abbrev(msg_bytes))
            (1, 1, 0x0C) => messages.push(parse_rad_mon_cmd_extended(msg_bytes))
            (0, 1, 0x24) => messages.push(parse_ext_func_cmd_abbrev(msg_bytes))
            (1, 1, 0x24) => messages.push(parse_ext_func_cmd_extended(msg_bytes))
            (0, 1, 0x3B) => messages.push(parse_net_status_broadcast_abbrev(msg_bytes))
            (1, 1, 0x3B) => messages.push(parse_net_status_broadcast_extended(msg_bytes))
            
            // Manufacturer
            (1, 0, _)    => messages.push(parse_manufacturer(msg_bytes))
            
            // Unknown with length octet — skip gracefully
            _             => messages.push(UnknownMessage(b1, b2, mco, msg_bytes))
        
        offset += msg_len
    
    return messages


// Example: Parse the most common message on a voice channel
function parse_grp_vch_user_abbrev(bytes: &[u8]) -> GroupVoiceChannelUser:
    service_options = bytes[1]
    group_address   = (bytes[2] as u16) << 8 | bytes[3] as u16
    source_address  = (bytes[4] as u32) << 16
                    | (bytes[5] as u32) << 8
                    | bytes[6] as u32
    
    emergency  = (service_options >> 7) & 1
    encrypted  = (service_options >> 6) & 1
    duplex     = (service_options >> 5) & 1
    mode       = (service_options >> 4) & 1
    priority   = service_options & 0x07
    
    return GroupVoiceChannelUser {
        group_address,
        source_address,
        emergency: emergency == 1,
        encrypted: encrypted == 1,
        priority,
    }


function parse_uu_vch_user_abbrev(bytes: &[u8]) -> UnitToUnitVoiceChannelUser:
    service_options = bytes[1]
    target_address  = (bytes[2] as u32) << 16
                    | (bytes[3] as u32) << 8
                    | bytes[4] as u32
    source_address  = (bytes[5] as u32) << 16
                    | (bytes[6] as u32) << 8
                    | bytes[7] as u32
    
    return UnitToUnitVoiceChannelUser {
        target_address,
        source_address,
        service_options,
    }
```

---

## 8. Implementation Notes

### 8.1 Network Status Broadcast — Critical for Scrambling

The Network Status Broadcast message (MCO 0x3B) carries WACN ID, System ID, and
Color Code needed for the scrambling seed. This message is:
- **Never scrambled** (it provides the descrambling parameters)
- Available in both abbreviated (11 bytes) and extended (13 bytes) forms
- Detailed format is in the base spec (8.3.1.25), not modified by this addendum

### 8.2 Obsolescence Handling

MCO 0x1D (Radio Unit Monitor Command) is marked **OBSOLETE**. The opcode is
permanently reserved and must not be reused. Implementations should:
- Recognize the opcode to skip the 8-byte message
- Not generate this message
- Log it if received (indicates old infrastructure)

The replacement is MCO 0x0C (Radio Unit Monitor Command, 8.3.1.44) with TX Time,
SM flag, and explicit length octet.

### 8.3 Abbreviated vs Extended Selection

- **Abbreviated** forms are used when source and destination are within the same WACN/System
- **Extended** forms are required when the source SUID cannot be derived from the
  local WACN + System ID + the 24-bit working unit ID (i.e., roaming subscribers)
- Receivers **must accept both forms** for any message that has both

### 8.4 Forward Compatibility

All messages added after the initial spec publication include a length octet in byte 2.
Unknown messages encountered by older implementations can be skipped using:
```
skip_length = pdu[offset + 1] & 0x3F
```
This is fundamental to the P25 extensibility model.

### 8.5 Cross-Validation References

- **SDRTrunk:** `io.github.dsheirer.module.decode.p25.phase2.message` package
- **OP25:** `op25/gr-op25_repeater/lib/p25p2_tdma.cc` — MAC PDU parsing functions

---

*Extracted from TIA-102.BBAC-1 Sections 8.3, 8.4.2, and Annex B.
Message formats verified against rendered PDF pages.*
