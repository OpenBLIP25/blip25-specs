# P25 Conventional Control Messages -- Implementation Specification

**Source:** TIA-102.AABG (April 2009), "Project 25 -- Conventional Control Messages"
**Phase:** 3 -- Implementation-ready
**Classification:** MESSAGE_FORMAT
**Extracted:** 2026-04-12
**Purpose:** Self-contained spec for implementing a P25 conventional (non-trunked) supplementary
data message parser/encoder. Covers the subset of TSBK messages applicable to conventional
channels, addressing conventions unique to conventional mode, and the relationship to the
trunking control channel messages defined in TIA-102.AABC-E.

**Key dependency:** All wire formats are defined in TIA-102.AABC-B/E. This document does NOT
define any new wire formats -- it identifies which trunking messages apply to conventional
systems and specifies the addressing conventions for the conventional context.

---

## 1. Conventional vs. Trunked Signaling -- Architectural Differences

### 1.1 No Dedicated Control Channel

In trunked P25 systems, a dedicated control channel continuously broadcasts system information
(RFSS_STS_BCST, NET_STS_BCST, ADJ_STS_BCST, IDEN_UP, etc.) and carries inbound/outbound
signaling packets (ISP/OSP). A centralized controller assigns traffic channels on demand.

Conventional P25 systems have **no dedicated control channel** and **no centralized controller**.
Instead:

- Supplementary data messages are transmitted as TSBK blocks embedded in the voice channel
  signaling (interleaved with voice frames, or sent during channel idle periods).
- There is no channel assignment -- subscribers operate on pre-configured frequencies.
- Channel access is contention-based (listen-before-talk / carrier sense).

### 1.2 No ISP/OSP Distinction

In trunking, messages are classified as:
- **ISP (Inbound Signaling Packet):** SU -> FNE (subscriber to infrastructure)
- **OSP (Outbound Signaling Packet):** FNE -> SU (infrastructure to subscriber)

In conventional mode, this distinction **does not apply** because of:
- **Talk-around (direct mode):** A mobile SU can transmit directly to another mobile SU
  without any infrastructure. A "request" message sent by one SU is received directly by
  another SU.
- **Repeat mode:** A fixed station repeater rebroadcasts messages without converting them
  from "request" to "command" form.

Consequence: A conventional radio must be prepared to receive any applicable TSBK opcode
regardless of whether it was originally classified as ISP or OSP in the trunking spec.

### 1.3 Identical Message Pairs

Because of the above, several message pairs that have different names in the trunking spec
are **bit-for-bit identical** in their non-extended (abbreviated TSBK) CAI format:

| ISP Name (mobile-originated) | OSP Name (FNE-originated) | Identical in non-extended form? |
|------------------------------|---------------------------|-------------------------------|
| `CALL_ALRT_REQ` (ISP 0x1F)  | `CALL_ALRT` (OSP 0x1F)   | YES -- same opcode, same layout |
| `STS_Q_REQ` (ISP 0x1A)      | `STS_Q` (OSP 0x1A)       | YES -- same opcode, same layout |
| `MSG_UPDT_REQ` (ISP 0x1C)   | `MSG_UPDT` (OSP 0x1C)    | YES -- same opcode, same layout |
| `STS_UPDT_REQ` (ISP 0x18)   | `STS_UPDT` (OSP 0x18)    | YES -- same opcode, same layout |

Note: The extended (MBT) forms DO differ because the ISP and OSP extended formats carry
different SUID fields. For conventional systems spanning multiple RFSSes, the extended
format distinction matters.

### 1.4 Message Routing in Conventional Systems

```
Direct Mode (talk-around):
  SU_A --[RF]--> SU_B          (no infrastructure)

Repeat Mode (single RFSS):
  SU_A --[RF]--> Fixed_Station --[RF]--> SU_B
                 (repeats verbatim, same message name)

Multi-RFSS Repeat:
  SU_A --[RF]--> Fixed_Station_1 --[RFSS_1]--[wire]--[RFSS_2]--> Fixed_Station_2 --[RF]--> SU_B
                 (RFSS may re-name: CALL_ALRT_REQ -> CALL_ALRT when crossing RFSS boundary)

Console-originated:
  Wireline_Console --[wire]--> RFSS --> Fixed_Station --[RF]--> SU
                   (uses FNE-originated message names: CALL_ALRT, STS_Q, MSG_UPDT, etc.)
```

### 1.5 What Conventional Does NOT Have

The following trunking features are **explicitly excluded** from conventional:

- **Radio Detach** -- defined in AABC-B and AABD-A, not a conventional feature
- **ACK_RSP_U** (ISP 0x20) -- subscriber-sourced ACK is not defined for conventional use
- **RAD_MON_REQ** (ISP 0x1D) -- only RAD_MON_CMD (OSP 0x1D) is used; format differs
- **Channel grants** -- no GRP_V_CH_GRANT, UU_V_CH_GRANT, etc. (no controller to assign channels)
- **Registration** -- no U_REG_REQ/RSP, LOC_REG_REQ/RSP, GRP_AFF_REQ/RSP
- **System broadcasts** -- no RFSS_STS_BCST, NET_STS_BCST, ADJ_STS_BCST, IDEN_UP, SYNC_BCST
- **SNDCP data channel** operations

### 1.6 Wireline Console Restrictions

A wireline console (fixed console device attached to the RFSS):
- SHALL NOT send Status Update (`STS_UPDT_REQ`)
- SHALL NOT send Emergency Alarm (`EMRG_ALRM_REQ`)
- SHALL NOT respond to Radio Unit Monitor (`RAD_MON_CMD`)
- SHALL NOT respond to Status Request (`STS_Q`/`STS_Q_REQ`)

---

## 2. Conventional Message Inventory

### 2.1 Complete Conventional Message Table

All messages use the standard 12-octet TSBK format from TIA-102.AABC-B/E. The TSBK
structure (LB, P, Opcode, MFID, Payload, CRC-16) is identical -- see the AABC-E
Implementation Spec Section 1 for the wire format.

```rust
/// Complete list of TSBK opcodes used in P25 conventional mode.
/// Each entry: (opcode, conventional_alias, trunking_alias, description, section_in_aabc_b)
///
/// NOTE: In conventional mode, the ISP/OSP distinction does not apply. Many opcodes
/// appear in both ISP and OSP tables with the same value. For conventional parsing,
/// treat all as a single flat opcode space.
pub const CONVENTIONAL_OPCODES: &[(u8, &str, &str, &str)] = &[
    // Emergency
    (0x27, "EMRG_ALRM_REQ",     "EMRG_ALRM_REQ",     "Emergency Alarm Request"),
    // Call Alert
    (0x1F, "CALL_ALRT",          "CALL_ALRT_REQ/CALL_ALRT", "Call Alert (mobile or FNE originated)"),
    // Status
    (0x18, "STS_UPDT",           "STS_UPDT_REQ/STS_UPDT",   "Status Update (mobile or FNE originated)"),
    (0x1A, "STS_Q",              "STS_Q_REQ/STS_Q",         "Status Query (mobile or FNE originated)"),
    (0x19, "STS_Q_RSP",          "STS_Q_RSP",               "Status Query Response"),
    // Message (Short Message)
    (0x1C, "MSG_UPDT",           "MSG_UPDT_REQ/MSG_UPDT",   "Message Update (mobile or FNE originated)"),
    // Radio Check / Inhibit / Uninhibit
    (0x24, "EXT_FNCT_CMD",       "EXT_FNCT_CMD",            "Extended Function Command (Radio Check, Inhibit, Uninhibit)"),
    // Radio Unit Monitor
    (0x1D, "RAD_MON_CMD",        "RAD_MON_CMD",             "Radio Unit Monitor Command"),
    // Acknowledge
    (0x20, "ACK_RSP_FNE",        "ACK_RSP_FNE",             "Acknowledge Response (FNE format, used by mobile AND FNE)"),
    // Cancel Service
    (0x23, "CAN_SRV_REQ",        "CAN_SRV_REQ",             "Cancel Service Request"),
    // Telephone Interconnect
    (0x08, "TELE_INT_DIAL_REQ",  "TELE_INT_DIAL_REQ",       "Telephone Interconnect Explicit Dial Request"),
    (0x09, "TELE_INT_PSTN_REQ",  "TELE_INT_PSTN_REQ",       "Telephone Interconnect PSTN Request (Implicit Dial)"),
    (0x0A, "TEL_INT_ANS_REQ",    "TELE_INT_ANS_REQ",        "Telephone Interconnect Answer Request"),
    // NOTE: TEL_INT_ANS_RSP uses ISP opcode 0x0A per AABC-B section 4.1.6
    // Deny / Queue
    (0x27, "DENY_RSP",           "DENY_RSP",                "Deny Response (shares opcode 0x27 in OSP space)"),
    (0x21, "QUE_RSP",            "QUE_RSP",                 "Queued Response"),
];
```

**IMPORTANT OPCODE NOTE:** In the trunking spec, ISP and OSP have separate 6-bit opcode
spaces (the direction bit in the MBT header distinguishes them). In conventional abbreviated
TSBK mode, there is no direction bit. The opcode 0x27 is `EMRG_ALRM_REQ` in ISP context
and `DENY_RSP` in OSP context -- these have **different field layouts**. A conventional
parser must disambiguate based on context (see Section 7).

### 2.2 Conventional Feature Categories

| Feature | Opcodes Used | Standard Option in [102A]? |
|---------|-------------|---------------------------|
| Emergency Alarm | 0x27 (EMRG_ALRM_REQ), 0x20 (ACK_RSP_FNE), 0x23 (CAN_SRV_REQ) | Not specifically mentioned |
| Call Alert | 0x1F (CALL_ALRT/CALL_ALRT_REQ), 0x20 (ACK_RSP_FNE) | Standard Option |
| Radio Check | 0x24 (EXT_FNCT_CMD) | Not specifically mentioned |
| Radio Inhibit/Uninhibit | 0x24 (EXT_FNCT_CMD) | Standard Option |
| Status Update | 0x18 (STS_UPDT/STS_UPDT_REQ), 0x20 (ACK_RSP_FNE) | Not specifically mentioned |
| Status Query | 0x1A (STS_Q/STS_Q_REQ), 0x19 (STS_Q_RSP) | Not specifically mentioned |
| Short Message | 0x1C (MSG_UPDT/MSG_UPDT_REQ), 0x20 (ACK_RSP_FNE) | Standard Option |
| Telephone Interconnect Dial | 0x08, 0x09, 0x20, 0x21, 0x27 | Standard Option |
| Telephone Interconnect Incoming | 0x0A (TEL_INT_ANS_REQ/RSP) | Standard Option |
| Telephone Call Clear Down | 0x23 (CAN_SRV_REQ), LC_CALL_TERM_CAN (LCW) | Standard Option |
| Radio Unit Monitor | 0x1D (RAD_MON_CMD), 0x20 (ACK_RSP_FNE) | Standard Option |

---

## 3. Conventional Message Field Layouts

All field layouts below use the standard 12-octet TSBK format. Octets 0-1 are the
TSBK header (LB|P|Opcode + MFID), octets 2-9 are message-specific payload, octets
10-11 are CRC-16.

### 3.1 EMRG_ALRM_REQ (0x27) -- Emergency Alarm Request

```
Oct 0: LB|P|100111    Oct 1: MFID (0x00)
Oct 2: SI-1 (Special Information 1)
Oct 3: SI-2 (Special Information 2)
Oct 4: reserved
Oct 5: Group Address[23:16]
Oct 6: Group Address[15:8]
Oct 7: Group Address[7:0]
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
Oct 10-11: CRC-16
```

**Conventional addressing:**
- Source Address (octets 7-9, 24-bit): Originator UID
- Group Address (octets 4-5, mapped as octets 5-7 in payload): Target group

SI-1 bit flags:
```
Bit 7: AC   -- Additional Codes present in SI-2
Bit 6: IC6  -- Interoperable Condition code 6
Bit 5: IC5  -- Interoperable Condition code 5
Bit 4: IC4  -- Interoperable Condition code 4
Bit 3: IC3  -- Interoperable Condition code 3
Bit 2: ASE  -- Accessory Sensed Emergency
Bit 1: VSE  -- Vehicle Sensed Emergency
Bit 0: MD   -- Man-Down condition
```

SI-2 values (when AC=1): 0x00 = default, 0x01 = Vest Pierced, 0x02-0x7F reserved,
0x80-0xFF user-defined.

ACK: Destination responds with ACK_RSP_FNE (0x20).
Cancel: Originator may send CAN_SRV_REQ (0x23) to cancel.

### 3.2 CALL_ALRT / CALL_ALRT_REQ (0x1F) -- Call Alert

```
Oct 0: LB|P|011111    Oct 1: MFID (0x00)
Oct 2: reserved (0x00)
Oct 3: reserved (0x00)
Oct 4: Target Address[23:16]
Oct 5: Target Address[15:8]
Oct 6: Target Address[7:0]
Oct 7: reserved (0x00)
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
Oct 10-11: CRC-16
```

**Conventional addressing:**
- Source Address (octets 7-9): Originator UID
- Target Address (octets 4-6): Destination UID

NOTE: Mobile-originated = CALL_ALRT_REQ, FNE-originated = CALL_ALRT. Identical format.

ACK: Destination responds with ACK_RSP_FNE (0x20).

### 3.3 STS_UPDT / STS_UPDT_REQ (0x18) -- Status Update

```
Oct 0: LB|P|011000    Oct 1: MFID (0x00)
Oct 2: Status[15:8]
Oct 3: Status[7:0]
Oct 4: Target Address[23:16]
Oct 5: Target Address[15:8]
Oct 6: Target Address[7:0]
Oct 7: reserved (0x00)
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
Oct 10-11: CRC-16
```

**Conventional addressing:**
- Source Address (octets 7-9): Originator UID
- Target Address (octets 4-6): Destination UID
- Status (octets 2-3): 16-bit status value

ACK: Destination responds with ACK_RSP_FNE (0x20).

### 3.4 STS_Q / STS_Q_REQ (0x1A) -- Status Query

```
Oct 0: LB|P|011010    Oct 1: MFID (0x00)
Oct 2: reserved (0x00)
Oct 3: reserved (0x00)
Oct 4: Target Address[23:16]
Oct 5: Target Address[15:8]
Oct 6: Target Address[7:0]
Oct 7: reserved (0x00)
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
Oct 10-11: CRC-16
```

**Conventional addressing:**
- Source Address (octets 7-9): Originator UID
- Target Address (octets 4-6): Destination UID

Response: Destination responds with STS_Q_RSP (0x19).

### 3.5 STS_Q_RSP (0x19) -- Status Query Response

```
Oct 0: LB|P|011001    Oct 1: MFID (0x00)
Oct 2: Status[15:8]
Oct 3: Status[7:0]
Oct 4: Target Address[23:16]
Oct 5: Target Address[15:8]
Oct 6: Target Address[7:0]
Oct 7: reserved (0x00)
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
Oct 10-11: CRC-16
```

**Conventional addressing:**
- Source Address (octets 7-9): Destination device (responding unit)
- Target Address (octets 4-6): Originator device (unit that sent the query)
- Status (octets 2-3): 16-bit status value of the responding unit

### 3.6 MSG_UPDT / MSG_UPDT_REQ (0x1C) -- Message Update (Short Message)

```
Oct 0: LB|P|011100    Oct 1: MFID (0x00)
Oct 2: Message[15:8]
Oct 3: Message[7:0]
Oct 4: Target Address[23:16]
Oct 5: Target Address[15:8]
Oct 6: Target Address[7:0]
Oct 7: reserved (0x00)
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
Oct 10-11: CRC-16
```

**Conventional addressing:**
- Source Address (octets 7-9): Originator UID
- Target Address (octets 4-6): Destination UID
- Message (octets 2-3): 16-bit pre-defined message code

ACK: Destination responds with ACK_RSP_FNE (0x20).

### 3.7 EXT_FNCT_CMD (0x24) -- Extended Function Command

Used for Radio Check, Radio Inhibit, and Radio Uninhibit.

```
Oct 0: LB|P|100100    Oct 1: MFID (0x00)
Oct 2: reserved (0x00)
Oct 3: reserved (0x00)
Oct 4: Extended Function[23:16]
Oct 5: Extended Function[15:8]
Oct 6: Extended Function[7:0]
Oct 7: reserved (0x00)
Oct 8: Target Address[23:16]
Oct 9: Target Address[7:0]
Oct 10-11: CRC-16
```

**Conventional addressing (note reversed octet positions vs. most messages):**
- Initial command: Source Address (octets 4-6) = Originator, Target Address (octets 7-9) = Destination
- ACK response: Source Address (octets 4-6) = Destination, Target Address (octets 7-9) = Originator

The Extended Function field (24 bits) encodes the specific function:

```rust
/// Extended Function class values (upper 16 bits of 24-bit Extended Function field)
pub const EXT_FNCT_RADIO_CHECK:     u16 = 0x0000;
pub const EXT_FNCT_RADIO_INHIBIT:   u16 = 0x0004; // NOTE: verify against AABC-B Section 6.2.6
pub const EXT_FNCT_RADIO_UNINHIBIT: u16 = 0x0005;
pub const EXT_FNCT_RADIO_CHECK_ACK: u16 = 0x0080;
pub const EXT_FNCT_RADIO_INHIBIT_ACK:   u16 = 0x0084;
pub const EXT_FNCT_RADIO_UNINHIBIT_ACK: u16 = 0x0085;
```

**NOTE:** The exact bit-packing of the Extended Function class within the 24-bit field
is defined in TIA-102.AABC-B Section 6.2.6. The values above are derived from SDRTrunk
and OP25 implementations and should be verified against the standard. The PDF extraction
did not include the detailed Extended Function encoding from AABC-B.

### 3.8 RAD_MON_CMD (0x1D) -- Radio Unit Monitor Command

```
Oct 0: LB|P|011101    Oct 1: MFID (0x00)
Oct 2: TX Time (8-bit, seconds; 0x00 = don't key)
Oct 3: SM(7) | reserved(6:2) | TX Mult(1:0)
Oct 4: Target Address[23:16]
Oct 5: Target Address[15:8]
Oct 6: Target Address[7:0]
Oct 7: reserved (0x00)
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
Oct 10-11: CRC-16
```

**Conventional addressing:**
- Source Address (octets 4-6): Originator (FNE or mobile console)
- Target Address (octets 7-9): Destination (unit to be monitored)

SM = Silent Monitor flag (1 = monitored unit does not indicate it is being monitored).
TX Mult = Transmit time multiplier (0-3).

**NOTE:** RAD_MON_REQ (ISP 0x1D) is NOT used in conventional mode. Only RAD_MON_CMD
(OSP 0x1D) is used, but in conventional mode it may be sourced by a mobile console
device in addition to the FNE.

ACK: Destination responds with ACK_RSP_FNE (0x20).

### 3.9 ACK_RSP_FNE (0x20) -- Acknowledge Response

```
Oct 0: LB|P|100000    Oct 1: MFID (0x00)
Oct 2: AIV(7) | EX(6) | Service Type(5:0)
Oct 3: Additional Info[31:24]
Oct 4: Additional Info[23:16]
Oct 5: Additional Info[15:8]
Oct 6: Additional Info[7:0]
Oct 7: reserved (0x00)
Oct 8: Target Address[23:16]
Oct 9: Target Address[7:0]
Oct 10-11: CRC-16
```

**CRITICAL CONVENTIONAL RULE:** In conventional mode:
- AIV (Additional Information Valid) SHALL be set to `1`
- EX (Extension) SHALL be cleared to `0`
- This enables symmetric addressing: Source Address in octets 4-6, Target Address in octets 7-9

**Conventional addressing (with AIV=1, EX=0):**
- Source Address (octets 4-6): Destination device (the acknowledging unit)
- Target Address (octets 7-9): Originator device (the unit being acknowledged)

Service Type (6 bits) echoes the opcode of the message being acknowledged.

In trunking, ACK_RSP_FNE is FNE-originated only. In conventional mode, it is used by
**both mobile SUs and the FNE** as the universal acknowledgment. The trunking subscriber
ACK (ACK_RSP_U, ISP 0x20) is NOT used in conventional systems.

### 3.10 CAN_SRV_REQ (0x23) -- Cancel Service Request

```
Oct 0: LB|P|100011    Oct 1: MFID (0x00)
Oct 2: AIV(7) | 0(6) | Service Type(5:0)
Oct 3: Reason Code
Oct 4: Additional Info[23:16]
Oct 5: Additional Info[15:8]
Oct 6: Additional Info[7:0]
Oct 7: reserved (0x00)
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
Oct 10-11: CRC-16
```

**Conventional addressing:**
- Source Address (octets 7-9): Originator UID

Used for canceling Emergency Alarm and Telephone Call Clear Down.

### 3.11 TELE_INT_DIAL_REQ (0x08) -- Telephone Interconnect Explicit Dial

Extended format only (MBT with 1-2 data blocks). Carries up to 34 DTMF digits at 4 bits each.

**Conventional addressing:**
- Source Address (octets 3-5 of MBT header): Originator UID

### 3.12 TELE_INT_PSTN_REQ (0x09) -- Telephone Interconnect PSTN Request

```
Oct 0: LB|P|001001    Oct 1: MFID (0x00)
Oct 2: Service Options
Oct 3-7: reserved
Oct 8: Source Address[23:16]
Oct 9: Source Address[7:0]
Oct 10-11: CRC-16
```

**Conventional addressing:**
- Source Address (octets 7-9): Originator UID

### 3.13 TEL_INT_ANS_REQ (0x0A) -- Telephone Interconnect Answer Request

```
Oct 0: LB|P|001010    Oct 1: MFID (0x00)
Oct 2-7: reserved / call info
Oct 8: Recipient Address[23:16]
Oct 9: Recipient Address[7:0]
Oct 10-11: CRC-16
```

**Conventional addressing:**
- Target Address (octets 7-9): Recipient UID

### 3.14 QUE_RSP (0x21) -- Queued Response

```
Oct 0: LB|P|100001    Oct 1: MFID (0x00)
Oct 2: AIV(7) | 0(6) | Service Type(5:0)
Oct 3: Reason Code
Oct 4: Additional Info[23:16]
Oct 5: Additional Info[15:8]
Oct 6: Additional Info[7:0]
Oct 7: reserved (0x00)
Oct 8: Target Address[23:16]
Oct 9: Target Address[7:0]
Oct 10-11: CRC-16
```

Used in telephone interconnect scenarios when the request cannot be immediately served.

### 3.15 DENY_RSP (0x27) -- Deny Response

```
Oct 0: LB|P|100111    Oct 1: MFID (0x00)
Oct 2: AIV(7) | 0(6) | Service Type(5:0)
Oct 3: Reason Code
Oct 4: Additional Info[23:16]
Oct 5: Additional Info[15:8]
Oct 6: Additional Info[7:0]
Oct 7: reserved (0x00)
Oct 8: Target Address[23:16]
Oct 9: Target Address[7:0]
Oct 10-11: CRC-16
```

**OPCODE COLLISION:** DENY_RSP shares opcode 0x27 with EMRG_ALRM_REQ. In trunking,
these are distinguished by ISP (0x27 = EMRG_ALRM_REQ) vs. OSP (0x27 = DENY_RSP).
In conventional mode without a direction indicator, disambiguation requires inspecting
the MFID and payload structure -- see Section 7.

### 3.16 LC_CALL_TERM_CAN -- Telephone Call Clear Down (Link Control Word)

This is NOT a TSBK message. It is carried in the Expanded TDU (Terminator Data Unit)
using the Link Control Word format defined in TIA-102.BAAC-A Section 7.3.8.

**Conventional addressing:**
- SUID (octets 6-8 of LCW): Unit ID being cleared

---

## 4. Conventional Addressing Summary Table

This table maps the originator/destination device roles to specific TSBK octet positions
for each conventional message.

```
Convention:
  "Originator" = device that initiates the transaction
  "Destination" = device that is the target of the transaction
  Octet numbers refer to TSBK payload octets (0-based from TSBK start)
```

| Message | Source Addr Octets | Source Role | Target Addr Octets | Target Role |
|---------|--------------------|-------------|--------------------|----|
| EMRG_ALRM_REQ (0x27) | 7-9 (24-bit) | Originator | 4-5 (Group, 16-bit*) | Group |
| ACK_RSP_FNE (0x20) | 4-6 (24-bit) | Destination (ack-er) | 7-9 (24-bit) | Originator |
| CAN_SRV_REQ (0x23) | 7-9 (24-bit) | Originator | -- | -- |
| CALL_ALRT (0x1F) | 7-9 (24-bit) | Originator | 4-6 (24-bit) | Destination |
| STS_UPDT (0x18) | 7-9 (24-bit) | Originator | 4-6 (24-bit) | Destination |
| STS_Q (0x1A) | 7-9 (24-bit) | Originator | 4-6 (24-bit) | Destination |
| STS_Q_RSP (0x19) | 7-9 (24-bit) | Destination (responder) | 4-6 (24-bit) | Originator |
| MSG_UPDT (0x1C) | 7-9 (24-bit) | Originator | 4-6 (24-bit) | Destination |
| EXT_FNCT_CMD (0x24) initial | 4-6 (24-bit) | Originator | 7-9 (24-bit) | Destination |
| EXT_FNCT_CMD (0x24) ack | 4-6 (24-bit) | Destination | 7-9 (24-bit) | Originator |
| RAD_MON_CMD (0x1D) | 4-6 (24-bit) | Originator | 7-9 (24-bit) | Destination |
| TELE_INT_DIAL_REQ (0x08) | 3-5 (MBT hdr) | Originator | -- | -- |
| TELE_INT_PSTN_REQ (0x09) | 7-9 (24-bit) | Originator | -- | -- |
| TEL_INT_ANS_REQ (0x0A) | -- | -- | 7-9 (24-bit) | Recipient |
| QUE_RSP (0x21) | -- | -- | 7-9 (24-bit) | Originator |
| DENY_RSP (0x27) | -- | -- | 7-9 (24-bit) | Originator |

*Note: EMRG_ALRM_REQ carries a Group Address in octets 5-7 (24-bit) per the AABC-B layout;
Table 2-2 in AABG references octets 4-5 which may refer to 16-bit Group ID in abbreviated form.
Verify against actual field extraction from the AABC-B ISP 0x27 layout.

---

## 5. Conventional Channel Management

### 5.1 No Dedicated Channel Management Messages

TIA-102.AABG does **not** define any conventional-specific channel management messages
(no channel markers, no busy/idle status messages). Channel management in conventional
P25 is handled at the physical/link layer level:

- **Busy/Idle detection:** Done via carrier sense (RF energy detection) at the physical layer,
  not via signaling messages. The radio monitors the channel for RF activity.
- **Channel markers:** Not defined in AABG. The concept of "channel markers" exists in
  some vendor implementations but is not part of this standard.
- **PTT (Push-to-Talk) signaling:** Handled by the voice frame structure in TIA-102.BAAA
  (HDU, LDU1, LDU2, TDU sequence) rather than by control messages.

### 5.2 Voice Channel Signaling Structure (from TIA-102.BAAA / TIA-102.BAAD)

On a conventional channel, the TSBK supplementary data messages are transmitted:
- During channel idle periods (no voice traffic)
- In the signaling slots within the voice superframe structure
- As standalone data bursts

The physical layer framing is defined in TIA-102.BAAD (Conventional CAI Operational
Description), not in AABG.

### 5.3 Group Call Setup on Conventional Channels

Conventional group calls do NOT use control-channel signaling for setup. The procedure is:

1. Originating SU checks channel busy/idle status (carrier sense)
2. If idle, SU keys up and transmits HDU (Header Data Unit) containing:
   - ALGID (Algorithm ID) and KID (Key ID) for encryption
   - TGID (Talk Group ID)
   - Source Unit ID (via Link Control Word in LDU1)
3. Voice frames (LDU1/LDU2) follow, carrying Link Control Words with:
   - Group Voice Channel User (LC opcode) identifying source and group
4. TDU (Terminator Data Unit) ends the transmission

No TSBK-level group call setup messages are used. The `GRP_V_REQ` (ISP 0x00) and
`GRP_V_CH_GRANT` (OSP 0x00) are trunking-only -- they request/grant a traffic channel
from the controller, which does not exist in conventional systems.

### 5.4 Emergency Signaling on Conventional

Emergency on conventional uses the TSBK `EMRG_ALRM_REQ` (0x27) message:

1. SU in emergency sends `EMRG_ALRM_REQ` with:
   - Source Address = emergency unit's UID
   - Group Address = emergency talk group
   - SI-1/SI-2 = emergency condition codes (Man-Down, Vest Pierced, etc.)
2. If a repeater/RFSS is present, it rebroadcasts the alarm
3. Destination (dispatcher/console) sends `ACK_RSP_FNE` (0x20) to acknowledge
4. Emergency unit may cancel with `CAN_SRV_REQ` (0x23)

In direct mode (talk-around), the emergency alarm is received directly by nearby units.
There is no guarantee of delivery without infrastructure.

---

## 6. Relationship to AABC-E Trunking Messages

### 6.1 Shared Opcodes (Same Wire Format)

These opcodes have identical wire formats in both trunking and conventional contexts:

| Opcode | ISP Name | OSP Name | Conventional Name | Wire Format |
|--------|----------|----------|-------------------|-------------|
| 0x18 | STS_UPDT_REQ | STS_UPDT | STS_UPDT | Identical (non-extended) |
| 0x19 | STS_Q_RSP | -- | STS_Q_RSP | Same |
| 0x1A | STS_Q_REQ | STS_Q | STS_Q | Identical (non-extended) |
| 0x1C | MSG_UPDT_REQ | MSG_UPDT | MSG_UPDT | Identical (non-extended) |
| 0x1F | CALL_ALRT_REQ | CALL_ALRT | CALL_ALRT | Identical (non-extended) |
| 0x20 | ACK_RSP_U | ACK_RSP_FNE | ACK_RSP_FNE | **Different** -- conventional uses FNE format only |
| 0x23 | CAN_SRV_REQ | -- | CAN_SRV_REQ | Same |
| 0x24 | EXT_FNCT_RSP | EXT_FNCT_CMD | EXT_FNCT_CMD | **Different** -- conventional uses CMD format |
| 0x27 | EMRG_ALRM_REQ | DENY_RSP | Both (context-dependent) | **Different layouts, same opcode** |

### 6.2 Conventional-Only Behavioral Differences

1. **ACK_RSP_FNE is bidirectional:** In trunking, only the FNE sends ACK_RSP_FNE. In
   conventional, both mobile SUs and FNE use this format. ACK_RSP_U is never used.

2. **EXT_FNCT_CMD is bidirectional:** The same format serves as both initial command and
   acknowledgment response. The Extended Function field value indicates whether it is a
   command or an ACK (see Section 3.7).

3. **RAD_MON_CMD from mobile consoles:** In trunking, only the FNE can issue RAD_MON_CMD.
   In conventional, a mobile console device may also originate this command.

### 6.3 Trunking-Only Messages (NOT Used in Conventional)

```rust
/// Opcodes that are trunking-only and SHALL NOT appear on conventional channels.
/// A conventional parser should log/ignore these if encountered.
pub const TRUNKING_ONLY_OPCODES: &[u8] = &[
    // Voice/Data channel grants (require controller)
    0x00, // GRP_V_CH_GRANT (OSP) / GRP_V_REQ (ISP)
    0x02, // GRP_V_CH_GRANT_UPDT
    0x03, // GRP_V_CH_GRANT_UPDT_EXP
    0x04, // UU_V_CH_GRANT / UU_V_REQ
    0x05, // UU_ANS_REQ / UU_ANS_RSP
    0x06, // UU_V_CH_GRANT_UPDT
    // Data channel (requires controller)
    0x10, // IND_DATA_CH_GRANT / IND_DATA_REQ
    0x11, // GRP_DATA_CH_GRANT / GRP_DATA_REQ
    0x12, // GRP_DATA_CH_ANN / SN_DATA_CHN_REQ
    0x13, // GRP_DATA_CH_ANN_EXP / SN_DATA_PAGE_RES
    0x14, // SN_DATA_CHN_GNT / SN_REC_REQ
    0x15, // SN_DATA_PAGE_REQ
    0x16, // SN_DATA_CHN_ANN_EXP
    // Registration (requires controller)
    0x28, // GRP_AFF_RSP / GRP_AFF_REQ
    0x29, // SCCB_EXP / GRP_AFF_Q_RSP
    0x2A, // GRP_AFF_Q
    0x2B, // LOC_REG_RSP / U_DE_REG_REQ
    0x2C, // U_REG_RSP / U_REG_REQ
    0x2D, // U_REG_CMD / LOC_REG_REQ
    0x2F, // U_DE_REG_ACK
    // System broadcasts (control channel only)
    0x30, // SYNC_BCST / P_PARM_REQ
    0x33, // IDEN_UP_TDMA
    0x34, // IDEN_UP_VU
    0x35, // TIME_DATE_ANN
    0x38, // SYS_SRV_BCST
    0x39, // SCCB
    0x3A, // RFSS_STS_BCST
    0x3B, // NET_STS_BCST
    0x3C, // ADJ_STS_BCST
    0x3D, // IDEN_UP
    0x3E, // ADJ_STS_BCST_UNC
    // Roaming (requires controller)
    0x36, // ROAM_ADDR_CMD / ROAM_ADDR_REQ
    0x37, // ROAM_ADDR_UPDT / ROAM_ADDR_RSP
    // Authentication (requires controller infrastructure)
    0x31, // AUTH_DMD
    0x32, // AUTH_FNE_RESP / IDEN_UP_REQ
];
```

---

## 7. Parser Pseudocode for Conventional Message Dispatch

### 7.1 Disambiguation Strategy

The core challenge in conventional mode parsing is that:
1. There is no ISP/OSP direction indicator in abbreviated TSBK
2. Opcode 0x27 is shared between EMRG_ALRM_REQ and DENY_RSP

Disambiguation for opcode 0x27:
- **EMRG_ALRM_REQ:** Octets 2-3 are SI-1/SI-2 (emergency condition codes), octet 4 is
  reserved, octets 5-7 are Group Address, octets 8-9 are Source Address
- **DENY_RSP:** Octet 2 has AIV|0|ServiceType, octet 3 is Reason Code, octets 4-6 are
  Additional Info, octets 8-9 are Target Address

Heuristic: Check octet 2 bit 6 -- in DENY_RSP this is always 0 (fixed). In EMRG_ALRM_REQ
this is SI-1 bit 6 (IC6 condition code) which is usually 0 but could be 1. A more reliable
approach: if the receiving unit is infrastructure (FNE/repeater), opcode 0x27 is likely
EMRG_ALRM_REQ from a subscriber. If the receiving unit is a subscriber, it could be either.
In practice, DENY_RSP is only sent by infrastructure, so:

- **If receiver is a subscriber:** 0x27 from infrastructure = DENY_RSP; 0x27 from another
  SU = EMRG_ALRM_REQ
- **If receiver is infrastructure:** 0x27 = EMRG_ALRM_REQ (subscribers don't send DENY_RSP)

### 7.2 Pseudocode

```
FUNCTION parse_conventional_tsbk(tsbk: [u8; 12]) -> ConventionalMessage:
    // Step 1: Validate CRC
    IF NOT tsbk_crc_valid(tsbk):
        RETURN Error("CRC mismatch")

    // Step 2: Extract header
    opcode = tsbk[0] & 0x3F
    mfid   = tsbk[1]

    // Step 3: Check for manufacturer-specific
    IF mfid != 0x00:
        RETURN ManufacturerSpecific(mfid, opcode, tsbk[2..10])

    // Step 4: Dispatch on opcode
    MATCH opcode:
        0x27 =>
            // Disambiguate EMRG_ALRM_REQ vs DENY_RSP
            // Check AIV bit (octet 2, bit 7) -- DENY_RSP typically has AIV set
            aiv = (tsbk[2] >> 7) & 0x01
            service_type = tsbk[2] & 0x3F
            IF aiv == 1 AND service_type != 0:
                // Likely DENY_RSP (AIV=1 with a service type echoed)
                RETURN parse_deny_rsp(tsbk)
            ELSE:
                // Likely EMRG_ALRM_REQ (SI-1 in octet 2)
                RETURN parse_emrg_alrm_req(tsbk)

        0x1F => RETURN parse_call_alrt(tsbk)

        0x18 => RETURN parse_sts_updt(tsbk)

        0x1A => RETURN parse_sts_q(tsbk)

        0x19 => RETURN parse_sts_q_rsp(tsbk)

        0x1C => RETURN parse_msg_updt(tsbk)

        0x24 =>
            // EXT_FNCT_CMD -- could be initial command or ACK
            ext_func = (tsbk[4] as u32) << 16
                     | (tsbk[5] as u32) << 8
                     | (tsbk[6] as u32)
            RETURN parse_ext_fnct_cmd(tsbk, ext_func)

        0x1D => RETURN parse_rad_mon_cmd(tsbk)

        0x20 => RETURN parse_ack_rsp_fne(tsbk)

        0x23 => RETURN parse_can_srv_req(tsbk)

        0x09 => RETURN parse_tele_int_pstn_req(tsbk)

        0x0A => RETURN parse_tel_int_ans(tsbk)

        0x21 => RETURN parse_que_rsp(tsbk)

        _ =>
            IF opcode IN TRUNKING_ONLY_OPCODES:
                LOG_WARNING("Trunking-only opcode 0x{opcode:02X} on conventional channel")
            RETURN Unknown(opcode, tsbk[2..10])


FUNCTION parse_emrg_alrm_req(tsbk: [u8; 12]) -> ConventionalMessage:
    si1 = tsbk[2]
    si2 = tsbk[3]
    group_addr = (tsbk[5] as u32) << 16 | (tsbk[6] as u32) << 8 | (tsbk[7] as u32)
    source_addr = (tsbk[7] as u32) << 16 | (tsbk[8] as u32) << 8 | (tsbk[9] as u32)
    // NOTE: Source address spans octets 7-9 per AABC-B layout
    // Group address spans octets 5-7. There is overlap at octet 7 --
    // verify against actual AABC-B bit-level extraction.
    RETURN EmergencyAlarm {
        man_down:    (si1 & 0x01) != 0,
        veh_sensed:  (si1 & 0x02) != 0,
        acc_sensed:  (si1 & 0x04) != 0,
        vest_pierced: si2 == 0x01,
        group_addr,
        source_addr,
    }


FUNCTION parse_ack_rsp_fne(tsbk: [u8; 12]) -> ConventionalMessage:
    aiv = (tsbk[2] >> 7) & 0x01
    ex  = (tsbk[2] >> 6) & 0x01
    service_type = tsbk[2] & 0x3F
    // In conventional: AIV SHALL be 1, EX SHALL be 0
    source_addr = (tsbk[4] as u32) << 16 | (tsbk[5] as u32) << 8 | (tsbk[6] as u32)
    target_addr = (tsbk[7] as u32) << 16 | (tsbk[8] as u32) << 8 | (tsbk[9] as u32)
    RETURN Acknowledge {
        aiv, ex,
        service_type,
        source_addr,  // the acknowledging unit
        target_addr,  // the unit being acknowledged
    }
```

### 7.3 MBT Handling for Telephone Interconnect

```
FUNCTION parse_conventional_mbt(header: [u8; 12], data_blocks: Vec<[u8; 12]>)
    -> ConventionalMessage:

    // Validate header CRC
    IF NOT tsbk_crc_valid(header):
        RETURN Error("MBT header CRC mismatch")

    opcode = header[7] & 0x3F
    source_addr = (header[3] as u32) << 16 | (header[4] as u32) << 8 | (header[5] as u32)
    blocks_to_follow = header[6] & 0x7F

    IF blocks_to_follow != data_blocks.len():
        RETURN Error("Block count mismatch")

    MATCH opcode:
        0x08 =>  // TELE_INT_DIAL_REQ
            // Extract DTMF digits from data blocks (4 bits per digit)
            digits = extract_dtmf_digits(data_blocks)
            RETURN TelephoneInterconnectDial {
                source_addr,
                digits,
            }

        _ => RETURN Unknown(opcode)
```

---

## 8. Rust Implementation

### 8.1 Conventional Message Enum

```rust
/// A parsed P25 conventional control message.
/// All messages use the TSBK wire format from TIA-102.AABC-B/E.
#[derive(Debug, Clone)]
pub enum ConventionalMessage {
    /// Emergency Alarm (opcode 0x27 as EMRG_ALRM_REQ)
    EmergencyAlarm {
        source_addr: u32,       // 24-bit originator UID
        group_addr: u32,        // 24-bit talk group
        si1: EmergencyInfo1,
        si2: u8,                // SI-2 code (0x00=default, 0x01=vest pierced, etc.)
    },

    /// Call Alert (opcode 0x1F)
    CallAlert {
        source_addr: u32,       // 24-bit originator UID
        target_addr: u32,       // 24-bit destination UID
    },

    /// Status Update (opcode 0x18)
    StatusUpdate {
        source_addr: u32,       // 24-bit originator UID
        target_addr: u32,       // 24-bit destination UID
        status: u16,            // 16-bit status value
    },

    /// Status Query (opcode 0x1A)
    StatusQuery {
        source_addr: u32,       // 24-bit originator UID
        target_addr: u32,       // 24-bit destination UID
    },

    /// Status Query Response (opcode 0x19)
    StatusQueryResponse {
        source_addr: u32,       // 24-bit responder UID (= destination from query)
        target_addr: u32,       // 24-bit query originator UID
        status: u16,            // 16-bit status value
    },

    /// Message Update / Short Message (opcode 0x1C)
    MessageUpdate {
        source_addr: u32,       // 24-bit originator UID
        target_addr: u32,       // 24-bit destination UID
        message: u16,           // 16-bit pre-defined message code
    },

    /// Extended Function Command (opcode 0x24)
    /// Used for Radio Check, Inhibit, Uninhibit, and their ACKs
    ExtendedFunction {
        source_addr: u32,       // 24-bit (octets 4-6)
        target_addr: u32,       // 24-bit (octets 7-9)
        function: ExtendedFunctionType,
    },

    /// Radio Unit Monitor Command (opcode 0x1D)
    RadioMonitor {
        source_addr: u32,       // 24-bit originator (FNE or mobile console)
        target_addr: u32,       // 24-bit destination (unit to monitor)
        tx_time_secs: u8,       // transmit time in seconds (0 = don't key)
        silent_monitor: bool,   // true = silent (monitored unit doesn't indicate)
        tx_multiplier: u8,      // 2-bit (0-3)
    },

    /// Acknowledge Response (opcode 0x20, ACK_RSP_FNE format)
    Acknowledge {
        source_addr: u32,       // 24-bit acknowledging unit (octets 4-6 when AIV=1,EX=0)
        target_addr: u32,       // 24-bit acknowledged unit (octets 7-9)
        service_type: u8,       // 6-bit: echoes opcode of acknowledged message
        aiv: bool,              // Additional Information Valid (SHALL be true in conventional)
    },

    /// Cancel Service Request (opcode 0x23)
    CancelService {
        source_addr: u32,       // 24-bit originator UID
        service_type: u8,       // 6-bit: service being canceled
        reason_code: u8,        // 8-bit reason
    },

    /// Deny Response (opcode 0x27 as DENY_RSP)
    DenyResponse {
        target_addr: u32,       // 24-bit: unit being denied
        service_type: u8,       // 6-bit: service denied
        reason_code: u8,        // 8-bit reason
        additional_info: u32,   // 24-bit additional information
    },

    /// Queued Response (opcode 0x21)
    QueuedResponse {
        target_addr: u32,       // 24-bit: unit being queued
        service_type: u8,       // 6-bit: queued service
        reason_code: u8,        // 8-bit reason
        additional_info: u32,   // 24-bit additional information
    },

    /// Telephone Interconnect PSTN Request (opcode 0x09)
    TelephoneInterconnectPstn {
        source_addr: u32,       // 24-bit originator UID
        service_options: u8,    // 8-bit service options
    },

    /// Telephone Interconnect Explicit Dial (opcode 0x08, MBT only)
    TelephoneInterconnectDial {
        source_addr: u32,       // 24-bit originator UID
        digits: Vec<u8>,        // DTMF digits (4 bits each, 0-9, A-D, *, #)
    },

    /// Telephone Interconnect Answer (opcode 0x0A)
    TelephoneInterconnectAnswer {
        recipient_addr: u32,    // 24-bit recipient UID
    },

    /// Manufacturer-specific message (MFID != 0x00)
    ManufacturerSpecific {
        mfid: u8,
        opcode: u8,
        payload: [u8; 8],       // octets 2-9
    },

    /// Unknown or trunking-only opcode received on conventional channel
    Unknown {
        opcode: u8,
        payload: [u8; 8],
    },
}
```

### 8.2 Supporting Types

```rust
/// Emergency Alarm Special Information 1 (SI-1) bit flags
#[derive(Debug, Clone, Copy)]
pub struct EmergencyInfo1 {
    pub man_down: bool,           // Bit 0: Man-Down condition
    pub vehicle_sensed: bool,     // Bit 1: Vehicle Sensed Emergency
    pub accessory_sensed: bool,   // Bit 2: Accessory Sensed Emergency
    pub ic3: bool,                // Bit 3: Interoperable Condition 3
    pub ic4: bool,                // Bit 4: Interoperable Condition 4
    pub ic5: bool,                // Bit 5: Interoperable Condition 5
    pub ic6: bool,                // Bit 6: Interoperable Condition 6
    pub additional_codes: bool,   // Bit 7: Additional Codes present in SI-2
}

impl EmergencyInfo1 {
    pub fn from_byte(b: u8) -> Self {
        EmergencyInfo1 {
            man_down:          (b & 0x01) != 0,
            vehicle_sensed:    (b & 0x02) != 0,
            accessory_sensed:  (b & 0x04) != 0,
            ic3:               (b & 0x08) != 0,
            ic4:               (b & 0x10) != 0,
            ic5:               (b & 0x20) != 0,
            ic6:               (b & 0x40) != 0,
            additional_codes:  (b & 0x80) != 0,
        }
    }

    pub fn to_byte(&self) -> u8 {
        (if self.man_down         { 0x01 } else { 0 })
        | (if self.vehicle_sensed { 0x02 } else { 0 })
        | (if self.accessory_sensed { 0x04 } else { 0 })
        | (if self.ic3            { 0x08 } else { 0 })
        | (if self.ic4            { 0x10 } else { 0 })
        | (if self.ic5            { 0x20 } else { 0 })
        | (if self.ic6            { 0x40 } else { 0 })
        | (if self.additional_codes { 0x80 } else { 0 })
    }
}

/// Extended Function types used in EXT_FNCT_CMD (opcode 0x24)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ExtendedFunctionType {
    RadioCheck,
    RadioCheckAck,
    RadioInhibit,
    RadioInhibitAck,
    RadioUninhibit,
    RadioUninhibitAck,
    Unknown(u32),   // 24-bit extended function value
}

impl ExtendedFunctionType {
    /// Decode the 24-bit extended function field.
    ///
    /// NOTE: These values are cross-referenced from SDRTrunk and OP25 implementations.
    /// The exact encoding is defined in TIA-102.AABC-B Section 6.2.6 which was not
    /// fully extracted from the PDF. Values should be verified.
    pub fn from_u24(val: u32) -> Self {
        // The extended function class is typically in the upper 16 bits
        let class = (val >> 8) & 0xFFFF;
        match class {
            0x0000 => ExtendedFunctionType::RadioCheck,
            0x0080 => ExtendedFunctionType::RadioCheckAck,
            0x0004 => ExtendedFunctionType::RadioInhibit,
            0x0084 => ExtendedFunctionType::RadioInhibitAck,
            0x0005 => ExtendedFunctionType::RadioUninhibit,
            0x0085 => ExtendedFunctionType::RadioUninhibitAck,
            _ => ExtendedFunctionType::Unknown(val),
        }
    }

    pub fn to_u24(&self) -> u32 {
        match self {
            ExtendedFunctionType::RadioCheck       => 0x000000,
            ExtendedFunctionType::RadioCheckAck    => 0x008000,
            ExtendedFunctionType::RadioInhibit     => 0x000400,
            ExtendedFunctionType::RadioInhibitAck  => 0x008400,
            ExtendedFunctionType::RadioUninhibit   => 0x000500,
            ExtendedFunctionType::RadioUninhibitAck => 0x008500,
            ExtendedFunctionType::Unknown(v)       => *v,
        }
    }

    pub fn is_ack(&self) -> bool {
        matches!(self,
            ExtendedFunctionType::RadioCheckAck |
            ExtendedFunctionType::RadioInhibitAck |
            ExtendedFunctionType::RadioUninhibitAck
        )
    }
}
```

### 8.3 TSBK Parser for Conventional Mode

```rust
/// Parse a 12-byte TSBK received on a P25 conventional channel.
///
/// This function handles the conventional message subset defined in TIA-102.AABG.
/// It uses the TSBK wire format from TIA-102.AABC-B/E but ignores the ISP/OSP
/// distinction since conventional mode does not use it.
pub fn parse_conventional_tsbk(tsbk: &[u8; 12]) -> Result<ConventionalMessage, &'static str> {
    // Validate CRC-16
    if !tsbk_crc_valid(tsbk) {
        return Err("CRC-16 validation failed");
    }

    let opcode = tsbk[0] & 0x3F;
    let mfid = tsbk[1];

    // Manufacturer-specific messages
    if mfid != 0x00 {
        let mut payload = [0u8; 8];
        payload.copy_from_slice(&tsbk[2..10]);
        return Ok(ConventionalMessage::ManufacturerSpecific {
            mfid,
            opcode,
            payload,
        });
    }

    match opcode {
        // 0x27: EMRG_ALRM_REQ or DENY_RSP -- disambiguate
        0x27 => {
            let aiv = (tsbk[2] >> 7) & 0x01;
            let service_type = tsbk[2] & 0x3F;
            if aiv == 1 && service_type != 0 {
                // DENY_RSP: AIV=1 with echoed service type
                Ok(ConventionalMessage::DenyResponse {
                    target_addr: extract_addr_8_9(tsbk),
                    service_type,
                    reason_code: tsbk[3],
                    additional_info: extract_addr_4_6(tsbk),
                })
            } else {
                // EMRG_ALRM_REQ
                Ok(ConventionalMessage::EmergencyAlarm {
                    source_addr: extract_addr_7_9(tsbk),
                    group_addr: extract_addr_5_7(tsbk),
                    si1: EmergencyInfo1::from_byte(tsbk[2]),
                    si2: tsbk[3],
                })
            }
        }

        // 0x1F: Call Alert
        0x1F => Ok(ConventionalMessage::CallAlert {
            source_addr: extract_addr_7_9(tsbk),
            target_addr: extract_addr_4_6(tsbk),
        }),

        // 0x18: Status Update
        0x18 => Ok(ConventionalMessage::StatusUpdate {
            source_addr: extract_addr_7_9(tsbk),
            target_addr: extract_addr_4_6(tsbk),
            status: ((tsbk[2] as u16) << 8) | (tsbk[3] as u16),
        }),

        // 0x1A: Status Query
        0x1A => Ok(ConventionalMessage::StatusQuery {
            source_addr: extract_addr_7_9(tsbk),
            target_addr: extract_addr_4_6(tsbk),
        }),

        // 0x19: Status Query Response
        0x19 => Ok(ConventionalMessage::StatusQueryResponse {
            source_addr: extract_addr_7_9(tsbk),
            target_addr: extract_addr_4_6(tsbk),
            status: ((tsbk[2] as u16) << 8) | (tsbk[3] as u16),
        }),

        // 0x1C: Message Update
        0x1C => Ok(ConventionalMessage::MessageUpdate {
            source_addr: extract_addr_7_9(tsbk),
            target_addr: extract_addr_4_6(tsbk),
            message: ((tsbk[2] as u16) << 8) | (tsbk[3] as u16),
        }),

        // 0x24: Extended Function Command
        0x24 => {
            let ext_func_val = ((tsbk[4] as u32) << 16)
                | ((tsbk[5] as u32) << 8)
                | (tsbk[6] as u32);
            Ok(ConventionalMessage::ExtendedFunction {
                source_addr: extract_addr_4_6(tsbk),
                target_addr: extract_addr_7_9(tsbk),
                function: ExtendedFunctionType::from_u24(ext_func_val),
            })
        }

        // 0x1D: Radio Unit Monitor Command
        0x1D => Ok(ConventionalMessage::RadioMonitor {
            source_addr: extract_addr_4_6(tsbk),
            target_addr: extract_addr_7_9(tsbk),
            tx_time_secs: tsbk[2],
            silent_monitor: (tsbk[3] & 0x80) != 0,
            tx_multiplier: tsbk[3] & 0x03,
        }),

        // 0x20: Acknowledge Response (ACK_RSP_FNE format)
        0x20 => {
            let aiv = (tsbk[2] >> 7) & 0x01 != 0;
            let service_type = tsbk[2] & 0x3F;
            // In conventional: AIV=1, EX=0 -> source in octets 4-6, target in 7-9
            Ok(ConventionalMessage::Acknowledge {
                source_addr: extract_addr_4_6(tsbk),  // AIV=1,EX=0: acknowledging unit
                target_addr: extract_addr_7_9(tsbk),  // unit being acknowledged
                service_type,
                aiv,
            })
        }

        // 0x23: Cancel Service Request
        0x23 => Ok(ConventionalMessage::CancelService {
            source_addr: extract_addr_7_9(tsbk),
            service_type: tsbk[2] & 0x3F,
            reason_code: tsbk[3],
        }),

        // 0x09: Telephone Interconnect PSTN Request
        0x09 => Ok(ConventionalMessage::TelephoneInterconnectPstn {
            source_addr: extract_addr_7_9(tsbk),
            service_options: tsbk[2],
        }),

        // 0x0A: Telephone Interconnect Answer
        0x0A => Ok(ConventionalMessage::TelephoneInterconnectAnswer {
            recipient_addr: extract_addr_7_9(tsbk),
        }),

        // 0x21: Queued Response
        0x21 => Ok(ConventionalMessage::QueuedResponse {
            target_addr: extract_addr_8_9(tsbk),
            service_type: tsbk[2] & 0x3F,
            reason_code: tsbk[3],
            additional_info: extract_addr_4_6(tsbk),
        }),

        // Unknown or trunking-only
        _ => {
            let mut payload = [0u8; 8];
            payload.copy_from_slice(&tsbk[2..10]);
            Ok(ConventionalMessage::Unknown { opcode, payload })
        }
    }
}

// --- Address extraction helpers ---

/// Extract 24-bit address from TSBK octets 4-6
fn extract_addr_4_6(tsbk: &[u8; 12]) -> u32 {
    ((tsbk[4] as u32) << 16) | ((tsbk[5] as u32) << 8) | (tsbk[6] as u32)
}

/// Extract 24-bit address from TSBK octets 5-7
fn extract_addr_5_7(tsbk: &[u8; 12]) -> u32 {
    ((tsbk[5] as u32) << 16) | ((tsbk[6] as u32) << 8) | (tsbk[7] as u32)
}

/// Extract 24-bit address from TSBK octets 7-9
fn extract_addr_7_9(tsbk: &[u8; 12]) -> u32 {
    ((tsbk[7] as u32) << 16) | ((tsbk[8] as u32) << 8) | (tsbk[9] as u32)
}

/// Extract 16-bit address from TSBK octets 8-9 (used in some abbreviated forms)
fn extract_addr_8_9(tsbk: &[u8; 12]) -> u32 {
    ((tsbk[8] as u32) << 8) | (tsbk[9] as u32)
}
```

### 8.4 Message Encoder

```rust
/// Encode a conventional message into a 12-byte TSBK.
/// Sets LB=1 (last block), P=0, MFID=0x00.
pub fn encode_conventional_tsbk(msg: &ConventionalMessage) -> Result<[u8; 12], &'static str> {
    let mut tsbk = [0u8; 12];
    tsbk[0] = 0x80; // LB=1, P=0 (will OR in opcode below)
    tsbk[1] = 0x00; // MFID = standard

    match msg {
        ConventionalMessage::EmergencyAlarm { source_addr, group_addr, si1, si2 } => {
            tsbk[0] |= 0x27;
            tsbk[2] = si1.to_byte();
            tsbk[3] = *si2;
            // Group address in octets 5-7
            tsbk[5] = ((group_addr >> 16) & 0xFF) as u8;
            tsbk[6] = ((group_addr >> 8) & 0xFF) as u8;
            tsbk[7] = (group_addr & 0xFF) as u8;
            // Source address in octets 7-9
            // NOTE: octet 7 overlap -- source occupies 7-9, group occupies 5-7
            // The actual AABC-B layout must be consulted for exact bit boundaries
            tsbk[7] = ((source_addr >> 16) & 0xFF) as u8;
            tsbk[8] = ((source_addr >> 8) & 0xFF) as u8;
            tsbk[9] = (source_addr & 0xFF) as u8;
        }

        ConventionalMessage::CallAlert { source_addr, target_addr } => {
            tsbk[0] |= 0x1F;
            pack_addr_4_6(&mut tsbk, *target_addr);
            pack_addr_7_9(&mut tsbk, *source_addr);
        }

        ConventionalMessage::StatusUpdate { source_addr, target_addr, status } => {
            tsbk[0] |= 0x18;
            tsbk[2] = (status >> 8) as u8;
            tsbk[3] = *status as u8;
            pack_addr_4_6(&mut tsbk, *target_addr);
            pack_addr_7_9(&mut tsbk, *source_addr);
        }

        ConventionalMessage::StatusQuery { source_addr, target_addr } => {
            tsbk[0] |= 0x1A;
            pack_addr_4_6(&mut tsbk, *target_addr);
            pack_addr_7_9(&mut tsbk, *source_addr);
        }

        ConventionalMessage::StatusQueryResponse { source_addr, target_addr, status } => {
            tsbk[0] |= 0x19;
            tsbk[2] = (status >> 8) as u8;
            tsbk[3] = *status as u8;
            pack_addr_4_6(&mut tsbk, *target_addr);
            pack_addr_7_9(&mut tsbk, *source_addr);
        }

        ConventionalMessage::MessageUpdate { source_addr, target_addr, message } => {
            tsbk[0] |= 0x1C;
            tsbk[2] = (message >> 8) as u8;
            tsbk[3] = *message as u8;
            pack_addr_4_6(&mut tsbk, *target_addr);
            pack_addr_7_9(&mut tsbk, *source_addr);
        }

        ConventionalMessage::ExtendedFunction { source_addr, target_addr, function } => {
            tsbk[0] |= 0x24;
            let val = function.to_u24();
            tsbk[4] = ((val >> 16) & 0xFF) as u8;
            tsbk[5] = ((val >> 8) & 0xFF) as u8;
            tsbk[6] = (val & 0xFF) as u8;
            // NOTE: EXT_FNCT_CMD has source in 4-6 and target in 7-9
            // but octets 4-6 are used for extended function value
            // The source address is the originator, packed elsewhere
            // Verify against AABC-B Section 6.2.6 for exact layout
            pack_addr_4_6(&mut tsbk, *source_addr);
            pack_addr_7_9(&mut tsbk, *target_addr);
        }

        ConventionalMessage::RadioMonitor {
            source_addr, target_addr, tx_time_secs, silent_monitor, tx_multiplier
        } => {
            tsbk[0] |= 0x1D;
            tsbk[2] = *tx_time_secs;
            tsbk[3] = if *silent_monitor { 0x80 } else { 0x00 } | (tx_multiplier & 0x03);
            pack_addr_4_6(&mut tsbk, *source_addr);
            pack_addr_7_9(&mut tsbk, *target_addr);
        }

        ConventionalMessage::Acknowledge { source_addr, target_addr, service_type, aiv } => {
            tsbk[0] |= 0x20;
            tsbk[2] = if *aiv { 0x80 } else { 0x00 } | (service_type & 0x3F);
            // EX = 0 for conventional (bit 6 of octet 2 already 0)
            pack_addr_4_6(&mut tsbk, *source_addr);
            pack_addr_7_9(&mut tsbk, *target_addr);
        }

        ConventionalMessage::CancelService { source_addr, service_type, reason_code } => {
            tsbk[0] |= 0x23;
            tsbk[2] = service_type & 0x3F;
            tsbk[3] = *reason_code;
            pack_addr_7_9(&mut tsbk, *source_addr);
        }

        _ => return Err("Unsupported message type for encoding"),
    }

    // Compute and store CRC-16
    let crc = tsbk_crc16(&tsbk[0..10].try_into().unwrap());
    tsbk[10] = (crc >> 8) as u8;
    tsbk[11] = (crc & 0xFF) as u8;

    Ok(tsbk)
}

fn pack_addr_4_6(tsbk: &mut [u8; 12], addr: u32) {
    tsbk[4] = ((addr >> 16) & 0xFF) as u8;
    tsbk[5] = ((addr >> 8) & 0xFF) as u8;
    tsbk[6] = (addr & 0xFF) as u8;
}

fn pack_addr_7_9(tsbk: &mut [u8; 12], addr: u32) {
    tsbk[7] = ((addr >> 16) & 0xFF) as u8;
    tsbk[8] = ((addr >> 8) & 0xFF) as u8;
    tsbk[9] = (addr & 0xFF) as u8;
}
```

### 8.5 Conventional Message Constants

```rust
/// Conventional-applicable TSBK opcodes as constants.
/// In conventional mode, these are not separated into ISP/OSP.
pub mod conventional_opcode {
    pub const TELE_INT_DIAL_REQ: u8  = 0x08;
    pub const TELE_INT_PSTN_REQ: u8  = 0x09;
    pub const TEL_INT_ANS: u8        = 0x0A;
    pub const STS_UPDT: u8           = 0x18;
    pub const STS_Q_RSP: u8          = 0x19;
    pub const STS_Q: u8              = 0x1A;
    pub const MSG_UPDT: u8           = 0x1C;
    pub const RAD_MON_CMD: u8        = 0x1D;
    pub const CALL_ALRT: u8          = 0x1F;
    pub const ACK_RSP_FNE: u8        = 0x20;
    pub const QUE_RSP: u8            = 0x21;
    pub const CAN_SRV_REQ: u8        = 0x23;
    pub const EXT_FNCT_CMD: u8       = 0x24;
    pub const EMRG_ALRM_REQ: u8     = 0x27;  // Also DENY_RSP in trunking OSP context
}

/// Check if a TSBK opcode is valid for conventional mode
pub fn is_conventional_opcode(opcode: u8) -> bool {
    matches!(opcode,
        0x08 | 0x09 | 0x0A |
        0x18 | 0x19 | 0x1A | 0x1C | 0x1D | 0x1F |
        0x20 | 0x21 | 0x23 | 0x24 | 0x27
    )
}
```

---

## 9. Cross-Reference to Open-Source Implementations

### 9.1 SDRTrunk

SDRTrunk implements conventional P25 message decoding in its `io.github.dsheirer.module.decode.p25`
package. Key reference points:

- **TSBKMessage.java** -- Base TSBK parser, handles CRC validation and opcode dispatch.
  The ISP/OSP distinction is maintained in the class hierarchy but both paths converge
  for conventional channel processing.
- **P25P1DecoderC4FM.java** / **P25P1DecoderLSM.java** -- Conventional channel decoders
  that process TSBK messages embedded in the voice channel. These do NOT expect a
  dedicated control channel stream.
- **Extended Function handling:** SDRTrunk decodes `EXT_FNCT_CMD` extended function values
  for Radio Check (0x0000), Inhibit (0x007F), Uninhibit (0x007E), and their ACKs.

  **NOTE:** SDRTrunk's extended function values may differ from the constants in Section 3.7
  above. Cross-reference with the actual SDRTrunk source for production use:
  `src/main/java/io/github/dsheirer/module/decode/p25/phase1/message/tsbk/standard/osp/ExtendedFunctionCommand.java`

- **Conventional channel state tracking:** SDRTrunk maintains channel state for conventional
  channels without relying on control channel broadcasts. It tracks active calls via
  HDU/LDU/TDU frame detection rather than channel grant messages.

### 9.2 OP25

OP25 implements conventional P25 in its `op25/gr-op25_repeater` GNURadio block:

- **p25_frame_assembler_impl.cc** -- Frame assembly for both trunking and conventional.
  In conventional mode, TSBK messages are extracted from the data unit stream (not from
  a dedicated control channel).
- **p25p1_fdma.cc** -- FDMA demodulator that handles conventional channel framing.
- **tsbk.py** / **tsbk.cc** -- TSBK message decoder. OP25 uses a flat opcode lookup
  without ISP/OSP separation for conventional mode, consistent with the AABG specification.

### 9.3 Implementation Notes from Open-Source

1. **CRC validation is critical:** Both SDRTrunk and OP25 validate TSBK CRC-16 before
   processing. On noisy conventional channels (especially talk-around with no repeater),
   CRC failures are common.

2. **No control channel state:** Conventional parsers cannot rely on having received
   IDEN_UP, NET_STS_BCST, or RFSS_STS_BCST. The parser must work without frequency
   table or system identity information.

3. **Emergency alarm priority:** Both implementations prioritize emergency alarm detection
   and display. EMRG_ALRM_REQ (0x27) is the most operationally critical conventional message.

4. **Opcode 0x27 disambiguation:** SDRTrunk handles this by context -- if the message is
   received from the control channel (trunking), it applies ISP/OSP classification. If
   received on a traffic/conventional channel, it defaults to EMRG_ALRM_REQ interpretation
   since DENY_RSP is a controller response that would not normally appear on a conventional
   channel without FNE infrastructure.

---

## 10. Extraction Completeness Assessment

### 10.1 What Was Successfully Extracted

The full text extraction from TIA-102.AABG captured:
- Complete document text including all sections (Foreword, Introduction, Scope, Overview,
  References, Definitions, Acronyms, Revision History)
- Table 2-1 (Conventional Supplementary Data Features) -- complete
- Table 2-2 (Addressing for Conventional Features) -- complete
- Figure descriptions (Figure 2-1 system diagram, Figure 2-2 originator/destination)
- All normative text about message naming, addressing conventions, and restrictions

### 10.2 What Is Missing or Incomplete

1. **No wire format diagrams:** TIA-102.AABG deliberately does not reproduce message wire
   formats -- it references TIA-102.AABC-B for all formats. The wire format details in this
   implementation spec are sourced from the AABC-E Implementation Spec.

2. **Extended Function field encoding:** The exact bit-level encoding of the Extended Function
   field (Radio Check, Inhibit, Uninhibit class values) is defined in TIA-102.AABC-B
   Section 6.2.6, not in AABG. The values in this spec are cross-referenced from SDRTrunk
   and OP25 and should be verified against the AABC-B standard.

3. **Telephone Interconnect detailed formats:** TELE_INT_DIAL_REQ (MBT format with DTMF
   digit encoding) and TEL_INT_ANS_RSP are defined in TIA-102.AABC-B Sections 4.1.4-4.2.7.
   The full MBT data block layouts for digit packing were not extracted.

4. **Reason codes:** The Deny Response and Cancel Service reason codes are defined in
   TIA-102.AABC-B Annexes A-C. These were not extracted from AABG (which does not
   reproduce them).

5. **EMRG_ALRM_REQ octet overlap:** The exact bit boundaries between Group Address
   (octets 5-7) and Source Address (octets 7-9) in the emergency alarm message need
   verification against the AABC-B ISP 0x27 layout. The extraction shows potential
   overlap at octet 7.

6. **LC_CALL_TERM_CAN format:** The Link Control Word format for telephone call clear
   down is defined in TIA-102.BAAC-A Section 7.3.8, not in AABG. Only the reference
   is noted.

### 10.3 Impact on Implementation

The missing items are all sourced from OTHER TIA-102 documents, not from AABG itself.
AABG is a mapping/applicability document -- it identifies which trunking messages apply
to conventional and defines addressing rules. All wire formats come from AABC-B/E.
The AABC-E Implementation Spec already covers the wire formats; this spec adds the
conventional-specific context, addressing rules, and disambiguation logic.
