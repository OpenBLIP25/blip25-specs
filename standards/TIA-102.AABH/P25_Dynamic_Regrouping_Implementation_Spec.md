# P25 Dynamic Regrouping Messages and Procedures -- Implementation Specification

**Source:** TIA-102.AABH (November 2014), "Project 25 -- Dynamic Regrouping Messages and Procedures"
**Phase:** 3 -- Implementation-ready
**Classification:** MESSAGE_FORMAT + PROTOCOL
**Extracted:** 2026-04-12
**Purpose:** Self-contained spec for implementing P25 dynamic regrouping message parsing,
encoding, and state machine logic for both MFID90 (Motorola "Airlink Efficient") and MFIDA4
(Harris "Explicit Encryption") methods. Covers FDMA TSBK messages, Link Control words,
TDMA MAC messages, regrouping procedures, supergroup concepts, extended function commands
for individual regrouping, and all response types. No reference to the original PDF required.

**Cross-references:**
- TIA-102.AABC-E: Standard trunking ISP/OSP opcodes reused by MFIDA4 (GRP_V_REQ, GRP_V_CH_GRANT, etc.)
- TIA-102.AABF-D: Standard Link Control word formats reused on FDMA traffic channels
- TIA-102.BBAD-A: TDMA MAC messages for MFID90 regrouping (Section 3.4, Partition 0b10)
- TIA-102.AABD-B: Trunking procedures (affiliation, group voice call, emergency, state machines)
- SDRTrunk: `module/decode/p25/phase1/message/tsbk/motorola/osp/` and `isp/` packages
- OP25: `op25/gr-op25_repeater/lib/p25p1_fdma.cc` Motorola MFID90 TSBK handling

---

## 1. What Dynamic Regrouping Is

Dynamic regrouping temporarily reassigns radios to different talk groups without physical
reprogramming. Two distinct mechanisms exist:

### 1.1 Group Regrouping (Supergroups)

A dispatch console or system administrator merges multiple talk groups into a single
temporary composite called a **supergroup**, identified by a **Supergroup Working Group ID
(SP-WGID)**. The SP-WGID is drawn from a reserved pool of addresses not used for normal
talk group operation, so subscriber units (SUs) can distinguish supergroup assignments from
ordinary channel grants.

**Patch (two-way):** All SUs in the supergroup can transmit and receive on the SP-WGID.
The SU sends voice requests using the SP-WGID (MFID90) or its own WGID (MFIDA4).

**Simulselect (one-way, MFIDA4 only):** The console/RFSS transmits outbound to the
supergroup; SUs receive on the SP-WGID but continue to transmit on their original WGID.

### 1.2 Individual Regrouping (MFID90 Only)

Commands a specific SU to affiliate with a dynamically assigned talk group, replacing its
currently selected talk group for the duration of the regrouped state. The SU uses standard
GRP_V_REQ / GRP_V_CH_GRANT messages once affiliated to the dynamic group. Individual
regrouping survives power cycles.

### 1.3 Why Two Methods Exist

Two incompatible proprietary methods were already deployed before standardization:
- **MFIDA4 ($A4, Harris):** "Explicit Encryption" -- carries Algorithm ID and Key ID directly
  in the regrouping command. Supports both patch and simulselect.
- **MFID90 ($90, Motorola):** "Airlink Efficient" -- encryption is pre-configured; regrouping
  messages carry only a P-bit (encrypted/clear). Supports patch and individual regroup.
  Does NOT support simulselect. Does NOT support announcement group regrouping.

An RFSS may implement one or both. Subscriber manufacturers must implement both.

---

## 2. Supergroup Concept

### 2.1 SP-WGID Selection

The RFSS chooses the SP-WGID from a **reserved pool** of group addresses that are NOT
used as normal WGIDs. This is critical because MFIDA4 reuses standard GRP_V_CH_GRANT
messages -- the SU can only distinguish a supergroup grant from a normal grant by recognizing
the SP-WGID as belonging to the reserved pool.

### 2.2 Supergroup Sequence Number (SSN) -- MFIDA4 Only

A unique non-zero 5-bit value (bits 4:0 of the GRG_Options byte). Prevents SUs from
remaining regrouped after teardown when the SP-WGID is reused for a new supergroup.
When reusing an SP-WGID, the RFSS must assign a different SSN than previously used.

### 2.3 Timer T_grp_rgrp

| Parameter | Min | Default | Max |
|-----------|-----|---------|-----|
| T_grp_rgrp | 20s | 20s | unspecified |

- SU starts/restarts T_grp_rgrp on receiving a regrouping command containing its WGID
- Timer runs only while on the control channel; paused on traffic channel
- Expiration causes the SU to leave the Group Regrouped state
- The RFSS must repeat regrouping commands more often than T_grp_rgrp

### 2.4 Encryption Handling

| Method | Mechanism |
|--------|-----------|
| MFIDA4 | Algorithm ID + Key ID carried in GRG_EXENC_CMD (patch only; simulselect sets both to 0) |
| MFID90 | Pre-configured in infrastructure and SUs; only P-bit (encrypted/clear) in messages |

---

## 3. FDMA TSBK Messages -- MFID90 ($90) Opcodes

All MFID90 TSBK messages follow standard TSBK structure (12 bytes, CRC-16 in bytes 10-11).
Byte 1 is always $90 (Manufacturer ID). Byte 0 bit 6 is the P (Protected) bit for ISPs, and
byte 0 bit 7 is the LB (Last Block) bit.

### 3.1 MFID90 ISPs (Inbound Signaling Packets -- SU to RFSS)

#### 3.1.1 MOT_GRG_V_REQ -- Group Regroup Voice Request (Opcode 0x00)

SU requests a channel grant for a supergroup call.

```
Byte:  0         1         2         3         4         5-6       7-8-9     10-11
     +--------+---------+---------+---------+---------+---------+---------+---------+
     |LB|P|000000| $90  |res|P|res| reserved| reserved| SP-WGID | Src WUID| CRC-16 |
     +--------+---------+---------+---------+---------+---------+---------+---------+
```

| Byte(s) | Bits | Field | Description |
|---------|------|-------|-------------|
| 0 | 7 | LB | Last Block flag |
| 0 | 6 | P | Protected (ISP-level, usually 0) |
| 0 | 5:0 | Opcode | `0b000000` = 0x00 |
| 1 | 7:0 | MFID | `0x90` |
| 2 | 7:5 | reserved | Set to 0 |
| 2 | 4 | P (encryption) | `0` = clear request, `1` = encrypted request |
| 2 | 3:0 | reserved | Set to 0 |
| 3-4 | | reserved | Set to 0 |
| 5-6 | | SP-WGID | 16-bit Supergroup Working Group ID |
| 7-9 | | Source Address | 24-bit WUID of requesting SU |
| 10-11 | | CRC-16 | CCITT CRC-16 |

#### 3.1.2 MOT_EXT_FNCT_RSP -- Extended Function Response (Opcode 0x01)

SU acknowledges an Individual Regrouping command or cancellation.

```
Byte:  0         1         2         3         4-5-6-7   8-9       10-11
     +--------+---------+---------+---------+---------+---------+---------+
     |LB|P|000001| $90  |  Class  | Operand |Arguments| Src WUID| CRC-16 |
     +--------+---------+---------+---------+---------+---------+---------+
```

| Byte(s) | Bits | Field | Description |
|---------|------|-------|-------------|
| 0 | 5:0 | Opcode | `0b000001` = 0x01 |
| 1 | | MFID | `0x90` |
| 2 | 7:0 | Class | Function class (`0x02` = Individual Regrouping) |
| 3 | 7:0 | Operand | Function operand (see table below) |
| 4-7 | | Arguments | 32 bits, top-justified (24-bit data + 8 reserved). Content depends on class/operand |
| 8-9 | | Source Address | 24-bit WUID of responding SU (note: only 16 bits in bytes 8-9) |
| 10-11 | | CRC-16 | CCITT CRC-16 |

**Extended Function ISP Values:**

| Class | Operand | Arguments (24 bits, top-justified) | Description |
|-------|---------|-----------|-------------|
| `0x02` | `0x80` | New Dynamic Talkgroup (16-bit GID) | IR Command ACK |
| `0x02` | `0x81` | Old Dynamic Talkgroup (16-bit GID) | IR Cancel ACK |

### 3.2 MFID90 OSPs (Outbound Signaling Packets -- RFSS to SU)

#### 3.2.1 MOT_GRG_ADD_CMD -- Group Regroup Add Command (Opcode 0x00)

Programs up to 3 WGIDs into a supergroup. Sent continuously more often than T_grp_rgrp
for the duration of the group regroup.

```
Byte:  0         1         2-3       4-5       6-7       8-9       10-11
     +--------+---------+---------+---------+---------+---------+---------+
     |LB|P|000000| $90  | SP-WGID | WGID-1  | WGID-2  | WGID-3  | CRC-16 |
     +--------+---------+---------+---------+---------+---------+---------+
```

| Byte(s) | Field | Description |
|---------|-------|-------------|
| 0[5:0] | Opcode | `0b000000` = 0x00 |
| 1 | MFID | `0x90` |
| 2-3 | Supergroup Address | 16-bit SP-WGID being created/updated |
| 4-5 | WGID 1 | First talk group to add (16-bit) |
| 6-7 | WGID 2 | Second talk group to add (16-bit, 0x0000 if unused) |
| 8-9 | WGID 3 | Third talk group to add (16-bit, 0x0000 if unused) |
| 10-11 | CRC-16 | |

**Key behavior:** If more than 3 WGIDs need to be added, the RFSS sends multiple
MOT_GRG_ADD_CMD messages for the same SP-WGID. Each message refreshes the timer
only for SUs whose WGID appears in that specific message.

#### 3.2.2 MOT_GRG_DEL_CMD -- Group Regroup Delete Command (Opcode 0x01)

Removes up to 3 WGIDs from a supergroup, or dissolves the entire supergroup.

```
Byte:  0         1         2-3       4-5       6-7       8-9       10-11
     +--------+---------+---------+---------+---------+---------+---------+
     |LB|P|000001| $90  | SP-WGID | WGID-1  | WGID-2  | WGID-3  | CRC-16 |
     +--------+---------+---------+---------+---------+---------+---------+
```

| Byte(s) | Field | Description |
|---------|-------|-------------|
| 0[5:0] | Opcode | `0b000001` = 0x01 |
| 1 | MFID | `0x90` |
| 2-3 | Supergroup Address | 16-bit SP-WGID being modified/dissolved |
| 4-5 | WGID 1 | Talk group to remove. **If dissolving entire supergroup, set to SP-WGID** |
| 6-7 | WGID 2 | Talk group to remove (0x0000 if unused) |
| 8-9 | WGID 3 | Talk group to remove (0x0000 if unused) |
| 10-11 | CRC-16 | |

**Dissolve detection:** If WGID-1 equals the SP-WGID, the entire supergroup is being
dissolved. All SUs regrouped to this SP-WGID leave the Group Regrouped state.

#### 3.2.3 MOT_GRG_CN_GRANT -- Group Regroup Channel Grant (Opcode 0x02)

Initial channel grant for a supergroup voice call.

```
Byte:  0         1         2         3-4       5-6       7-8-9     10-11
     +--------+---------+---------+---------+---------+---------+---------+
     |LB|P|000010| $90  |res|P|res| Channel | SP-WGID | Src WUID| CRC-16 |
     +--------+---------+---------+---------+---------+---------+---------+
```

| Byte(s) | Bits | Field | Description |
|---------|------|-------|-------------|
| 0[5:0] | | Opcode | `0b000010` = 0x02 |
| 1 | | MFID | `0x90` |
| 2 | 7:5 | reserved | |
| 2 | 4 | P (encryption) | `0` = clear, `1` = encrypted grant |
| 2 | 3:0 | reserved | |
| 3-4 | | Channel | 16-bit channel identifier (4-bit IDEN + 12-bit channel number) |
| 5-6 | | SP-WGID | 16-bit Supergroup Address |
| 7-9 | | Source Address | 24-bit WUID of requesting unit |
| 10-11 | | CRC-16 | |

**Channel identifier decoding:** The 16-bit channel field encodes as `(IDEN << 12) | channel_number`.
The IDEN maps to a frequency band via IDEN_UP messages on the control channel. Actual
frequency = base_frequency + (channel_number * channel_spacing). See TIA-102.AABC-E
Section 2 for complete IDEN decoding.

#### 3.2.4 MOT_GRG_CN_GRANT_EXP -- Group Regroup Channel Grant, Explicit (Opcode 0x02, MBT)

Multi-block TSBK with separate transmit and receive channel identifiers for inter-site use.
Uses the Alternate MBT format (Format = 0b10111).

**Header Block (12 bytes):**

```
Byte:  0         1         2         3-4-5     6         7         8         9         10-11
     +--------+---------+---------+---------+---------+---------+---------+---------+---------+
     |0|AN|IO|Fmt| 1|1|SAP   | $90     | Src WUID  |1|BlkFol   |0|0|000010|res|P|res| rsv   | H-CRC |
     +--------+---------+---------+---------+---------+---------+---------+---------+---------+
```

| Field | Value |
|-------|-------|
| AN | `0` (unconfirmed) |
| IO | `1` (OSP) |
| Format | `0b10111` (Alternate) |
| SAP | `0b111101` (Trunking Control) |
| MFID | `0x90` |
| Source Address | 24-bit WUID |
| Blocks to Follow | `0b00000001` (1 data block follows) |
| Opcode | `0b000010` |
| P | `0`=clear, `1`=encrypted |

**Data Block (12 bytes):**

```
Byte:  0-1       2-3       4-5       6         7         8-9-10-11
     +---------+---------+---------+---------+---------+-----------+
     | Ch (T)  | Ch (R)  | SP-WGID | reserved| reserved| MBT CRC-32|
     +---------+---------+---------+---------+---------+-----------+
```

| Byte(s) | Field | Description |
|---------|-------|-------------|
| 0-1 | Channel (T) | 16-bit transmit channel (RFSS transmit = SU receive frequency) |
| 2-3 | Channel (R) | 16-bit receive channel (RFSS receive = SU transmit frequency) |
| 4-5 | SP-WGID | 16-bit Supergroup Address |
| 6-7 | reserved | |
| 8-11 | CRC-32 | Multi-block CRC over header + data |

#### 3.2.5 MOT_GRG_CN_GRANT_UPDT -- Group Regroup Channel Update (Opcode 0x03)

Carries two simultaneous supergroup channel assignments (A and B). Used as an ongoing
notification that a supergroup call is active on a particular channel.

```
Byte:  0         1         2-3       4-5       6-7       8-9       10-11
     +--------+---------+---------+---------+---------+---------+---------+
     |LB|P|000011| $90  | Chan-A  | SP-WGID-A| Chan-B | SP-WGID-B| CRC-16|
     +--------+---------+---------+---------+---------+---------+---------+
```

| Byte(s) | Field | Description |
|---------|-------|-------------|
| 0[5:0] | Opcode | `0b000011` = 0x03 |
| 1 | MFID | `0x90` |
| 2-3 | Channel A | 16-bit channel identifier for supergroup A |
| 4-5 | SP-WGID A | 16-bit Supergroup Address A |
| 6-7 | Channel B | 16-bit channel identifier for supergroup B |
| 8-9 | SP-WGID B | 16-bit Supergroup Address B |
| 10-11 | CRC-16 | |

**Note:** If only one supergroup update is needed, the B fields may be set to 0x0000.

#### 3.2.6 MOT_EXT_FNCT_CMD -- Extended Function Command (Opcode 0x04)

Sends Individual Regrouping commands (IR Command / IR Cancel) to a target SU.

```
Byte:  0         1         2         3         4-5-6-7   8-9       10-11
     +--------+---------+---------+---------+---------+---------+---------+
     |LB|P|000100| $90  |  Class  | Operand |Arguments|Tgt WUID | CRC-16 |
     +--------+---------+---------+---------+---------+---------+---------+
```

| Byte(s) | Field | Description |
|---------|-------|-------------|
| 0[5:0] | Opcode | `0b000100` = 0x04 |
| 1 | MFID | `0x90` |
| 2 | Class | `0x02` for Individual Regrouping |
| 3 | Operand | See table below |
| 4-7 | Arguments | 32 bits, top-justified (24-bit data + 8 reserved) |
| 8-9 | Target Address | 24-bit WUID of target SU (note: packed into 16 bits in standard TSBK) |
| 10-11 | CRC-16 | |

**Extended Function OSP Values:**

| Class | Operand | Arguments (24 bits, top-justified) | Description |
|-------|---------|-----------|-------------|
| `0x02` | `0x00` | Dynamic Talkgroup GID (16-bit, top-justified) | IR Command -- assign SU to dynamic group |
| `0x02` | `0x01` | Null (0x000000) | IR Cancel -- revert SU to original group |

**Note on Arguments field:** The Dynamic Talkgroup is the Group ID portion of the SGID.
The SU combines this with its pre-provisioned System ID to form the full SGID for affiliation.

#### 3.2.7 MOT_QUE_RSP -- Queued Response (Opcode 0x06)

Sent when the RFSS cannot immediately grant a supergroup channel request.

```
Byte:  0         1         2         3         4-5-6     7-8-9     10-11
     +--------+---------+---------+---------+---------+---------+---------+
     |LB|P|000110| $90  |AIV|0|SvcType| Reason |Add'l Info |Tgt WUID | CRC-16 |
     +--------+---------+---------+---------+---------+---------+---------+
```

| Byte(s) | Bits | Field | Description |
|---------|------|-------|-------------|
| 0[5:0] | | Opcode | `0b000110` = 0x06 |
| 1 | | MFID | `0x90` |
| 2 | 7 | AIV | Additional Information Valid: `1` = bytes 4-6 are valid |
| 2 | 6 | reserved | `0` |
| 2 | 5:0 | Service Type | Opcode of the original ISP request (MFID90) |
| 3 | 7:0 | Reason Code | Same codes as standard QUE_RSP |
| 4 | 7:0 | Add'l Info [0] | Reserved for supergroup requests |
| 5-6 | | Add'l Info [1:2] | Supergroup Address (SP-WGID) |
| 7-9 | | Target Address | 24-bit WUID of requesting unit |
| 10-11 | | CRC-16 | |

#### 3.2.8 MOT_DENY_RSP -- Deny Response (Opcode 0x07)

Sent when the RFSS denies a supergroup channel request.

```
Byte:  0         1         2         3         4-5-6     7-8-9     10-11
     +--------+---------+---------+---------+---------+---------+---------+
     |LB|P|000111| $90  |RC|STP|SvcType| Reason |Add'l Info |Tgt WUID | CRC-16 |
     +--------+---------+---------+---------+---------+---------+---------+
```

| Byte(s) | Bits | Field | Description |
|---------|------|-------|-------------|
| 0[5:0] | | Opcode | `0b000111` = 0x07 |
| 1 | | MFID | `0x90` |
| 2 | 7 | RC | Reason Code type: `0`=standard, `1`=MFID90-specific |
| 2 | 6 | STP | Service Type Prefix: `0`=MFID90 request, `1`=other MFID |
| 2 | 5:0 | Service Type | Opcode of the original ISP request |
| 3 | 7:0 | Reason Code | Deny reason (see table below) |
| 4-6 | | Additional Info | For supergroup: [4]=octet 2 of ISP, [5:6]=SP-WGID. For unit call: 24-bit target. If invalid: 0x000000 |
| 7-9 | | Target Address | 24-bit WUID of requesting unit |
| 10-11 | | CRC-16 | |

**MFID90 Deny Reason Codes (when RC=1):**

| Code | Meaning |
|------|---------|
| `0x00` | Secure (encrypted) request on a clear supergroup |
| `0x01` | Clear request on a secure (encrypted) supergroup |

When RC=0, standard DENY_RSP reason codes from TIA-102.AABC-E apply.

#### 3.2.9 MOT_ACK_RSP_FNE -- Acknowledge Response (Opcode 0x08)

FNE acknowledgment of an SU service request. Used to quiet SU retries while the RFSS
processes the request, and also as the final step of IR Command/Cancel exchanges.

```
Byte:  0         1         2         3         4-5-6     7-8-9     10-11
     +--------+---------+---------+---------+---------+---------+---------+
     |LB|P|001000| $90  |res|res|SvcType| reserved| Src Addr  |Tgt WUID | CRC-16 |
     +--------+---------+---------+---------+---------+---------+---------+
```

| Byte(s) | Bits | Field | Description |
|---------|------|-------|-------------|
| 0[5:0] | | Opcode | `0b001000` = 0x08 |
| 1 | | MFID | `0x90` |
| 2 | 7:6 | reserved | |
| 2 | 5:0 | Service Type | Opcode of the original ISP (must have been MFID90) |
| 3 | | reserved | |
| 4-6 | | Source Address | 24-bit: FNE address (pending) or application device WUID (app-level ACK) |
| 7-9 | | Target Address | 24-bit WUID of requesting unit |
| 10-11 | | CRC-16 | |

### 3.3 Complete MFID90 TSBK Opcode Table

| Opcode | Hex | Direction | Message | Alias |
|--------|-----|-----------|---------|-------|
| `000000` | 0x00 | ISP | Group Regroup Voice Request | MOT_GRG_V_REQ |
| `000001` | 0x01 | ISP | Extended Function Response | MOT_EXT_FNCT_RSP |
| `000000` | 0x00 | OSP | Group Regroup Add Command | MOT_GRG_ADD_CMD |
| `000001` | 0x01 | OSP | Group Regroup Delete Command | MOT_GRG_DEL_CMD |
| `000010` | 0x02 | OSP | Group Regroup Channel Grant | MOT_GRG_CN_GRANT |
| `000010` | 0x02 | OSP | Group Regroup Channel Grant (Explicit) | MOT_GRG_CN_GRANT_EXP |
| `000011` | 0x03 | OSP | Group Regroup Channel Update | MOT_GRG_CN_GRANT_UPDT |
| `000100` | 0x04 | OSP | Extended Function Command | MOT_EXT_FNCT_CMD |
| `000110` | 0x06 | OSP | Queued Response | MOT_QUE_RSP |
| `000111` | 0x07 | OSP | Deny Response | MOT_DENY_RSP |
| `001000` | 0x08 | OSP | Acknowledge Response (FNE) | MOT_ACK_RSP_FNE |

**Important:** ISP and OSP opcodes share the same numeric space but are distinguished by
context (inbound vs outbound). Opcode 0x00 is MOT_GRG_V_REQ (ISP) vs MOT_GRG_ADD_CMD (OSP).
Opcode 0x02 is shared between the single-block and multi-block (explicit) channel grant forms;
distinguish by the MBT header format indicator.

---

## 4. FDMA TSBK Messages -- MFIDA4 ($A4) Opcode

MFIDA4 defines exactly one proprietary TSBK message. All other messages reuse standard
ISPs/OSPs from TIA-102.AABC-E.

### 4.1 GRG_EXENC_CMD -- Group Regroup Explicit Encryption Command (Opcode 0x30)

```
Byte:  0         1         2         3-4       5-6       7         8-9       10-11
     +--------+---------+---------+---------+---------+---------+---------+---------+
     |LB|P|110000| $A4  |GRG_Opts | SP-WGID | Key ID  | AlgID/  |RegrpTgt | CRC-16 |
     |        |         |         |         |         | Target  |         |         |
     +--------+---------+---------+---------+---------+---------+---------+---------+
```

| Byte(s) | Bits | Field | Description |
|---------|------|-------|-------------|
| 0[5:0] | | Opcode | `0b110000` = 0x30 |
| 1 | | MFID | `0xA4` |
| 2 | 7 | T (Type) | `0`=Patch (two-way), `1`=Simulselect (one-way) |
| 2 | 6 | G (Group) | `0`=unit address (WUID), `1`=group address (WGID) |
| 2 | 5 | A (Activate) | `0`=deactivate SP-WGID, `1`=activate/associate |
| 2 | 4:0 | SSN | 5-bit Supergroup Sequence Number (non-zero when active) |
| 3-4 | | SP-WGID | 16-bit Supergroup Working Group ID |
| 5-6 | | Key ID | 16-bit Key ID (patch only; 0x0000 for simulselect or deactivation) |
| 7 | | Byte 7 | If G=1: Algorithm ID. If G=0: WUID bits 23:16 |
| 8-9 | | Regroup Target | If G=1: 16-bit WGID. If G=0: WUID bits 15:0 |
| 10-11 | | CRC-16 | |

**GRG_Options byte layout:**

```
Bit:    7     6     5     4     3     2     1     0
      +-----+-----+-----+-----+-----+-----+-----+-----+
      |  T  |  G  |  A  |          SSN (5 bits)         |
      +-----+-----+-----+-----+-----+-----+-----+-----+
```

### 4.2 MFIDA4 Reuses Standard Messages

For voice call procedures under MFIDA4, the following standard messages from TIA-102.AABC-E
are used with the Group Address set to the SP-WGID:

| Message | Direction | Standard Opcode | Used For |
|---------|-----------|-----------------|----------|
| GRP_V_REQ | ISP | 0x00 | SU requests call on its WGID (RFSS maps to supergroup) |
| GRP_V_CH_GRANT | OSP | 0x00 | Channel grant with Group Address = SP-WGID |
| GRP_V_CH_GRANT_UPDT | OSP | 0x02 | Channel update with Group Address = SP-WGID |
| GRP_V_CH_GRANT_UPDT_EXP | OSP | 0x03 (MBT) | Explicit channel update |
| QUE_RSP | OSP | 0x03 | Standard queued response |
| DENY_RSP | OSP | 0x27 | Standard deny response |
| ACK_RSP_FNE | OSP | 0x28 | Standard FNE acknowledge |

---

## 5. FDMA Link Control Words

Link Control words are carried on FDMA voice traffic channels in the Low-Speed Data
(LSD) stream. MFID90 defines two proprietary LCOs. MFIDA4 reuses standard LC formats
from TIA-102.AABF-D.

### 5.1 LC_MOT_GRG_V_CH_USR -- Group Regroup Voice Channel User (LCO 0x00)

Used both inbound and outbound during a supergroup call. 9 bytes total.

```
Byte:  0         1         2         3         4-5       6-7-8
     +--------+---------+---------+---------+---------+---------+
     |P|SF=0|000000| $90  |res|P|res|res...|S| SP-WGID | Src WUID|
     +--------+---------+---------+---------+---------+---------+
```

| Byte | Bits | Field | Description |
|------|------|-------|-------------|
| 0 | 7 | P (protect) | LC-level protection flag |
| 0 | 6 | SF | Standard Format = `0` (manufacturer-specific) |
| 0 | 5:0 | LCO | `0b000000` = 0x00 |
| 1 | | MFID | `0x90` |
| 2 | 4 | P (encryption) | `0`=clear, `1`=encrypted |
| 3 | 0 | S | Source ID Extension required: `1`=next LC is LC_SOURCE_ID_EXT |
| 4-5 | | SP-WGID | 16-bit Supergroup Address |
| 6-8 | | Source Address | 24-bit WUID of current transmitter |

### 5.2 LC_MOT_GRG_V_CH_UPDT -- Group Regroup Voice Channel Update (LCO 0x01)

Outbound only. Notifies SUs on the voice channel of a supergroup call on another channel.

```
Byte:  0         1         2         3-4       5-6       7-8
     +--------+---------+---------+---------+---------+---------+
     |P|SF=0|000001| $90  |res|P|res| SP-WGID |reserved | Channel |
     +--------+---------+---------+---------+---------+---------+
```

| Byte | Bits | Field | Description |
|------|------|-------|-------------|
| 0 | 5:0 | LCO | `0b000001` = 0x01 |
| 1 | | MFID | `0x90` |
| 2 | 4 | P (encryption) | `0`=clear, `1`=encrypted |
| 3-4 | | SP-WGID | 16-bit Supergroup Address |
| 5-6 | | reserved | |
| 7-8 | | Channel | 16-bit channel identifier |

---

## 6. TDMA MAC Messages -- MFID90

TDMA MAC messages are used on Phase 2 two-slot TDMA traffic channels. MFIDA4 reuses
standard MAC messages from TIA-102.BBAC; only MFID90 defines proprietary MAC messages.

Cross-reference: TIA-102.BBAD-A Section 3.4, Partition 0b10 (Manufacturer Specific).

### 6.1 TDMA MAC Message Table

All messages use B1=1, B2=0 (manufacturer-specific partition).

| MCO | Length | MFID | Message | TSBK Equivalent |
|-----|--------|------|---------|-----------------|
| `0b000000` | 8 | 0x90 | MAC_MOT_GRG_V_CH_USR (Abbreviated) | LC_MOT_GRG_V_CH_USR |
| `0b100000` | 16 | 0x90 | MAC_MOT_GRG_V_CH_USR_EXT (Extended) | LC_MOT_GRG_V_CH_USR + SUID |
| `0b000011` | 7 | 0x90 | MAC_MOT_GRG_V_CH_UPDT | LC_MOT_GRG_V_CH_UPDT |
| `0b000100` | 11 | 0x90 | MAC_MOT_EXT_FNCT_CMD | MOT_EXT_FNCT_CMD (TSBK) |

Additional TDMA MAC messages defined in TIA-102.BBAD-A but not in TIA-102.AABH:

| MCO | Length | MFID | Message | Notes |
|-----|--------|------|---------|-------|
| `0b000001` | var | 0x90 | MAC_MOT_GRG_ADD_CMD | TDMA equivalent of MOT_GRG_ADD_CMD |
| `0b001001` | var | 0x90 | MAC_MOT_GRG_DEL_CMD | TDMA equivalent of MOT_GRG_DEL_CMD |
| `0b100001` | 9 | 0x90 | MAC_MOT_GRG_V_REQ | TDMA equivalent of MOT_GRG_V_REQ |
| `0b100010` | 11 | 0x90 | MAC_MOT_EXT_FNCT_RSP | TDMA equivalent of MOT_EXT_FNCT_RSP |
| `0b100011` | 11 | 0x90 | MAC_MOT_GRG_CN_GRANT (Implicit) | TDMA equivalent of MOT_GRG_CN_GRANT |
| `0b100100` | 13 | 0x90 | MAC_MOT_GRG_CN_GRANT_EXP (Explicit) | TDMA equivalent of MOT_GRG_CN_GRANT_EXP |
| `0b100101` | 11 | 0x90 | MAC_MOT_GRG_CN_GRANT_UPDT | TDMA equivalent of MOT_GRG_CN_GRANT_UPDT |
| `0b100110` | 11 | 0x90 | MAC_MOT_QUE_RSP | TDMA equivalent of MOT_QUE_RSP |
| `0b100111` | 11 | 0x90 | MAC_MOT_DENY_RSP | TDMA equivalent of MOT_DENY_RSP |
| `0b101000` | 10 | 0x90 | MAC_MOT_ACK_RSP_FNE | TDMA equivalent of MOT_ACK_RSP_FNE |

### 6.2 MAC_MOT_GRG_V_CH_USR -- Abbreviated Format (MCO 0x00, 8 bytes)

```
Byte:  0         1         2         3-4       5-6-7
     +--------+---------+---------+---------+---------+
     |10|000000| $90    |res|P|res| SP-WGID | Src WUID|
     +--------+---------+---------+---------+---------+
```

Identical field layout to LC_MOT_GRG_V_CH_USR but without the S-bit (home system only).

### 6.3 MAC_MOT_GRG_V_CH_USR_EXT -- Extended Format (MCO 0x20, 16 bytes)

Used when roaming (not on home system). Includes full SUID identification.

```
Byte:  0         1         2         3         4-5       6-7-8     9-10-11-12  13-14-15
     +--------+---------+---------+---------+---------+---------+-----------+---------+
     |10|100000| $90    |res|Len  |res|P|res| SP-WGID | Src WUID| WACN+SysID| Src ID  |
     +--------+---------+---------+---------+---------+---------+-----------+---------+
```

| Byte(s) | Field | Description |
|---------|-------|-------------|
| 0[5:0] | MCO | `0b100000` = 0x20 |
| 1 | MFID | `0x90` |
| 2 | Length | `0b10000` = 16 (total message length) |
| 3 | P bit | Encryption state |
| 4-5 | SP-WGID | 16-bit Supergroup Address |
| 6-8 | Source Address | 24-bit WUID |
| 9-12 | WACN+System ID | 20-bit WACN (9[7:0], 10[7:0], 11[7:4]) + 12-bit System ID (11[3:0], 12[7:0]) |
| 13-15 | Source ID | 24-bit UID portion of full SUID |

### 6.4 MAC_MOT_GRG_V_CH_UPDT (MCO 0x03, 7 bytes)

```
Byte:  0         1         2         3-4       5-6
     +--------+---------+---------+---------+---------+
     |10|000011| $90    |res|P|res| SP-WGID | Channel |
     +--------+---------+---------+---------+---------+
```

### 6.5 MAC_MOT_EXT_FNCT_CMD (MCO 0x04, 11 bytes)

```
Byte:  0         1         2         3         4         5-6-7     8-9-10
     +--------+---------+---------+---------+---------+---------+---------+
     |10|000100| $90    |res|Len  |  Class  | Operand |Arguments|Tgt WUID |
     +--------+---------+---------+---------+---------+---------+---------+
```

Length = `0b01011` = 11. Same Class/Operand/Arguments encoding as TSBK MOT_EXT_FNCT_CMD.

---

## 7. Regrouping Procedures

### 7.1 MFID90 Group Regroup -- Add Flow

```
Console                  RFSS                    SUs (affiliated WGIDs)
   |                       |                            |
   |  Initiate Regroup     |                            |
   |  (WGIDs, SP-WGID)     |                            |
   |---------------------->|                            |
   |                       |  MOT_GRG_ADD_CMD           |
   |                       |  (SP-WGID, WGID1,2,3)     |
   |                       |--------------------------->|
   |                       |                            | SUs enter Group Regrouped state
   |                       |                            | Start T_grp_rgrp
   |                       |                            |
   |                       |  MOT_GRG_ADD_CMD (repeat)  |
   |                       |  < every T_grp_rgrp        |
   |                       |--------------------------->|
   |                       |                            | Restart T_grp_rgrp
```

**SU behavior on receiving MOT_GRG_ADD_CMD:**
1. If SP-WGID matches current regrouped state with same SP-WGID: restart T_grp_rgrp
   (only if SU's WGID appears in the message)
2. If SP-WGID matches but SU's WGID is not in this message: remain regrouped, timer
   continues (other WGIDs in the supergroup are being refreshed)
3. If different SP-WGID and SU's WGID is present: switch to new SP-WGID, restart timer
4. If SU's WGID not present in any active ADD: do nothing

### 7.2 MFID90 Group Regroup -- Delete Flow

```
Console                  RFSS                    SUs
   |                       |                       |
   |  Cancel Regroup       |                       |
   |---------------------->|                       |
   |                       |  MOT_GRG_DEL_CMD      |
   |                       |  (SP-WGID, WGIDs)     |
   |                       |---------------------->|
   |                       |                       | SUs leave Group Regrouped state
```

**SU leaves Group Regrouped state when:**
1. MOT_GRG_DEL_CMD contains the SP-WGID as a WGID (dissolve entire supergroup)
2. MOT_GRG_DEL_CMD contains the SU's affiliated WGID
3. T_grp_rgrp expires
4. SU startup / power cycle
5. Group affiliation changes
6. Moves to site with different WACN/System ID
7. Enters conventional fallback mode
8. Receives GRP_V_CH_GRANT or GRP_V_CH_GRANT_UPDT for its own WGID (not SP-WGID)

### 7.3 MFID90 Group Regroup -- Voice Call Flow

```
Source SU               RFSS                    Dest Supergroup SUs
   |                      |                            |
   | MOT_GRG_V_REQ        |                            |
   | (SP-WGID, P-bit)     |                            |
   |--------------------->|                            |
   |                      | MOT_GRG_CN_GRANT           |
   |<---------------------|                            |
   |                      |--------------------------->|
   |                      |                            |
   | (move to traffic ch) |                            | (move to traffic ch)
   |                      |                            |
   | LC_MOT_GRG_V_CH_USR  |                            |
   |--------------------->| LC_MOT_GRG_V_CH_USR        |
   |                      |--------------------------->|
   |                      | MOT_GRG_CN_GRANT_UPDT      |
   |                      | (on control channel)       |
   |                      |                            |
```

**Valid RFSS responses to MOT_GRG_V_REQ:**

| Response | Action |
|----------|--------|
| MOT_GRG_CN_GRANT | Channel granted -- SU proceeds to traffic channel |
| MOT_GRG_CN_GRANT_EXP | Channel granted with explicit Tx/Rx channels |
| MOT_QUE_RSP | Queued -- SU stops retries, shows busy |
| MOT_DENY_RSP | Denied -- SU returns to control channel |
| MOT_ACK_RSP_FNE | Acknowledged, pending -- SU waits for next OSP |
| MOT_GRG_DEL_CMD | SU missed delete -- leave regrouped state |

**Exception case:** If an SU missed the ADD and sends a standard GRP_V_REQ instead of
MOT_GRG_V_REQ, the RFSS responds with DENY_RSP + MOT_GRG_ADD_CMD to re-initiate
the regrouping.

### 7.4 MFID90 Individual Regroup -- Command Flow

```
RFSS                              Target SU
  |                                   |
  |  MOT_EXT_FNCT_CMD                |
  |  Class=0x02, Op=0x00             |
  |  Args=Dynamic Talkgroup GID      |
  |---------------------------------->|
  |                                   |
  |  MOT_EXT_FNCT_RSP                |
  |  Class=0x02, Op=0x80             |
  |  Args=New Dynamic Talkgroup GID  |
  |<----------------------------------|
  |                                   |
  |  MOT_ACK_RSP_FNE                 |
  |---------------------------------->|
  |                                   | SU enters Individually Regrouped state
  |                                   | SU affiliates to dynamic group
```

### 7.5 MFID90 Individual Regroup -- Cancel Flow

```
RFSS                              Target SU
  |                                   |
  |  MOT_EXT_FNCT_CMD                |
  |  Class=0x02, Op=0x01             |
  |  Args=0x000000 (null)            |
  |---------------------------------->|
  |                                   |
  |  MOT_EXT_FNCT_RSP                |
  |  Class=0x02, Op=0x81             |
  |  Args=Old Dynamic Talkgroup GID  |
  |<----------------------------------|
  |                                   |
  |  MOT_ACK_RSP_FNE                 |
  |---------------------------------->|
  |                                   | SU enters Normal state
  |                                   | SU affiliates to original group
```

### 7.6 MFID90 Emergency Handling

An SU in Emergency mode does NOT send MOT_GRG_V_REQ. Instead:
1. SU leaves Group Regrouped state
2. SU sends standard GRP_V_REQ with emergency bit set on its configured emergency WGID
3. RFSS removes the emergency talkgroup from the supergroup via MOT_GRG_DEL_CMD
4. Normal emergency group call procedures follow

This differs from MFIDA4, where emergencies are processed ON the supergroup.

### 7.7 MFIDA4 Group Regroup -- Activation Flow

```
RFSS                              SUs
  |                                   |
  |  GRG_EXENC_CMD                    |
  |  A=1, T=0, G=1, SSN=N            |
  |  SP-WGID, KeyID, AlgID, WGID     |
  |---------------------------------->| (one message per WGID in the supergroup)
  |                                   |
  |  (repeat continuously < T_grp_rgrp)|
  |---------------------------------->|
```

SU stores SP-WGID + SSN. If SP-WGID+SSN match existing entry: restart timer.
If SP-WGID matches but SSN differs and WGID is not SU's: delete old SP-WGID, leave
regrouped state (the SP-WGID was reused for a new supergroup).

### 7.8 MFIDA4 Group Regroup -- Cancellation Flow

```
RFSS                              SUs
  |                                   |
  |  GRG_EXENC_CMD                    |
  |  A=0, SP-WGID, SSN               |
  |  (AlgID, KeyID, Target = 0)      |
  |---------------------------------->|
  |                                   | SU removes SP-WGID, leaves regrouped state
```

---

## 8. Parser Pseudocode for MFID90 Message Dispatch

### 8.1 TSBK Dispatch (FDMA Control Channel)

```
function parse_tsbk(data: bytes[12]) -> Message:
    if not tsbk_crc_valid(data):
        return Error("CRC invalid")

    lb       = (data[0] >> 7) & 1
    p_header = (data[0] >> 6) & 1
    opcode   = data[0] & 0x3F
    mfid     = data[1]

    // Check for manufacturer-specific messages
    if mfid == 0x90:
        return parse_mfid90_tsbk(opcode, data, is_outbound)
    elif mfid == 0xA4:
        return parse_mfida4_tsbk(opcode, data)
    elif mfid == 0x00:
        return parse_standard_tsbk(opcode, data)
    else:
        return UnknownMfid(mfid, opcode, data)

function parse_mfid90_tsbk(opcode: u8, data: bytes[12], is_outbound: bool) -> Message:
    if is_outbound:
        // --- OSP dispatch ---
        match opcode:
            0x00 -> parse_mot_grg_add_cmd(data)
            0x01 -> parse_mot_grg_del_cmd(data)
            0x02 -> parse_mot_grg_cn_grant(data)     // Single-block form
            0x03 -> parse_mot_grg_cn_grant_updt(data)
            0x04 -> parse_mot_ext_fnct_cmd(data)
            0x06 -> parse_mot_que_rsp(data)
            0x07 -> parse_mot_deny_rsp(data)
            0x08 -> parse_mot_ack_rsp_fne(data)
            _    -> UnknownMfid90Osp(opcode, data)
    else:
        // --- ISP dispatch ---
        match opcode:
            0x00 -> parse_mot_grg_v_req(data)
            0x01 -> parse_mot_ext_fnct_rsp(data)
            _    -> UnknownMfid90Isp(opcode, data)

function parse_mfida4_tsbk(opcode: u8, data: bytes[12]) -> Message:
    match opcode:
        0x30 -> parse_grg_exenc_cmd(data)
        _    -> UnknownMfidA4(opcode, data)
```

### 8.2 Individual Parser Functions

```
function parse_mot_grg_add_cmd(data: bytes[12]) -> MotGrgAddCmd:
    return MotGrgAddCmd {
        supergroup: (data[2] << 8) | data[3],
        wgid_1:     (data[4] << 8) | data[5],
        wgid_2:     (data[6] << 8) | data[7],
        wgid_3:     (data[8] << 8) | data[9],
    }

function parse_mot_grg_del_cmd(data: bytes[12]) -> MotGrgDelCmd:
    return MotGrgDelCmd {
        supergroup: (data[2] << 8) | data[3],
        wgid_1:     (data[4] << 8) | data[5],
        wgid_2:     (data[6] << 8) | data[7],
        wgid_3:     (data[8] << 8) | data[9],
    }

function parse_mot_grg_cn_grant(data: bytes[12]) -> MotGrgCnGrant:
    return MotGrgCnGrant {
        encrypted:  (data[2] >> 4) & 1 == 1,
        channel:    (data[3] << 8) | data[4],
        supergroup: (data[5] << 8) | data[6],
        source:     (data[7] << 16) | (data[8] << 8) | data[9],
    }

function parse_mot_grg_cn_grant_updt(data: bytes[12]) -> MotGrgCnGrantUpdt:
    return MotGrgCnGrantUpdt {
        channel_a:    (data[2] << 8) | data[3],
        supergroup_a: (data[4] << 8) | data[5],
        channel_b:    (data[6] << 8) | data[7],
        supergroup_b: (data[8] << 8) | data[9],
    }

function parse_mot_grg_v_req(data: bytes[12]) -> MotGrgVReq:
    return MotGrgVReq {
        encrypted:  (data[2] >> 4) & 1 == 1,
        supergroup: (data[5] << 8) | data[6],
        source:     (data[7] << 16) | (data[8] << 8) | data[9],
    }

function parse_mot_ext_fnct_cmd(data: bytes[12]) -> MotExtFnctCmd:
    return MotExtFnctCmd {
        class:     data[2],
        operand:   data[3],
        arguments: (data[4] << 16) | (data[5] << 8) | data[6],
        target:    (data[7] << 16) | (data[8] << 8) | data[9],
    }

function parse_mot_ext_fnct_rsp(data: bytes[12]) -> MotExtFnctRsp:
    return MotExtFnctRsp {
        class:     data[2],
        operand:   data[3],
        arguments: (data[4] << 16) | (data[5] << 8) | data[6],
        source:    (data[7] << 16) | (data[8] << 8) | data[9],
    }

function parse_mot_deny_rsp(data: bytes[12]) -> MotDenyRsp:
    rc_bit       = (data[2] >> 7) & 1
    stp_bit      = (data[2] >> 6) & 1
    service_type = data[2] & 0x3F
    reason_code  = data[3]
    return MotDenyRsp {
        rc_mfid90:      rc_bit == 1,
        other_mfid:     stp_bit == 1,
        service_type:   service_type,
        reason_code:    reason_code,
        additional_info: (data[4] << 16) | (data[5] << 8) | data[6],
        target:         (data[7] << 16) | (data[8] << 8) | data[9],
    }

function parse_mot_que_rsp(data: bytes[12]) -> MotQueRsp:
    return MotQueRsp {
        aiv:            (data[2] >> 7) & 1 == 1,
        service_type:   data[2] & 0x3F,
        reason_code:    data[3],
        additional_info: (data[4] << 16) | (data[5] << 8) | data[6],
        target:         (data[7] << 16) | (data[8] << 8) | data[9],
    }

function parse_mot_ack_rsp_fne(data: bytes[12]) -> MotAckRspFne:
    return MotAckRspFne {
        service_type: data[2] & 0x3F,
        source:       (data[4] << 16) | (data[5] << 8) | data[6],
        target:       (data[7] << 16) | (data[8] << 8) | data[9],
    }

function parse_grg_exenc_cmd(data: bytes[12]) -> GrgExencCmd:
    grg_opts = data[2]
    t_bit    = (grg_opts >> 7) & 1
    g_bit    = (grg_opts >> 6) & 1
    a_bit    = (grg_opts >> 5) & 1
    ssn      = grg_opts & 0x1F
    sp_wgid  = (data[3] << 8) | data[4]
    key_id   = (data[5] << 8) | data[6]

    if g_bit == 1:
        // Group target
        algorithm_id = data[7]
        target_wgid  = (data[8] << 8) | data[9]
        target = GroupTarget { algorithm_id, wgid: target_wgid }
    else:
        // Individual target
        target_wuid = (data[7] << 16) | (data[8] << 8) | data[9]
        target = IndividualTarget { wuid: target_wuid }

    return GrgExencCmd {
        regroup_type: if t_bit == 0 then Patch else Simulselect,
        is_group:     g_bit == 1,
        activate:     a_bit == 1,
        ssn:          ssn,
        supergroup:   sp_wgid,
        key_id:       key_id,
        target:       target,
    }
```

### 8.3 TDMA MAC Dispatch

```
function parse_mfid90_mac(mco: u8, mfid: u8, data: bytes) -> Message:
    if mfid != 0x90:
        return UnknownMfid(mfid, mco, data)

    match mco:
        0x00 -> parse_mac_mot_grg_v_ch_usr(data)           // 8 bytes, abbreviated
        0x20 -> parse_mac_mot_grg_v_ch_usr_ext(data)       // 16 bytes, extended
        0x03 -> parse_mac_mot_grg_v_ch_updt(data)          // 7 bytes
        0x04 -> parse_mac_mot_ext_fnct_cmd(data)           // 11 bytes
        // Additional MCOs from BBAD-A:
        0x01 -> parse_mac_mot_grg_add_cmd(data)            // variable
        0x09 -> parse_mac_mot_grg_del_cmd(data)            // variable
        0x21 -> parse_mac_mot_grg_v_req(data)              // 9 bytes
        0x22 -> parse_mac_mot_ext_fnct_rsp(data)           // 11 bytes
        0x23 -> parse_mac_mot_grg_cn_grant(data)           // 11 bytes, implicit
        0x24 -> parse_mac_mot_grg_cn_grant_exp(data)       // 13 bytes, explicit
        0x25 -> parse_mac_mot_grg_cn_grant_updt(data)      // 11 bytes
        0x26 -> parse_mac_mot_que_rsp(data)                // 11 bytes
        0x27 -> parse_mac_mot_deny_rsp(data)               // 11 bytes
        0x28 -> parse_mac_mot_ack_rsp_fne(data)            // 10 bytes
        _    -> UnknownMfid90Mac(mco, data)
```

---

## 9. Rust Enum and Struct Definitions

```rust
use std::fmt;

// ─── Manufacturer IDs ────────────────────────────────────────────────────────

pub const MFID_STANDARD: u8 = 0x00;
pub const MFID_MOTOROLA: u8 = 0x90;
pub const MFID_HARRIS: u8   = 0xA4;

// ─── MFID90 TSBK Opcodes ────────────────────────────────────────────────────

/// MFID90 ISP opcodes (SU -> RFSS)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Mfid90IspOpcode {
    /// Group Regroup Voice Request
    MotGrgVReq     = 0x00,
    /// Extended Function Response (IR Command ACK / IR Cancel ACK)
    MotExtFnctRsp  = 0x01,
}

/// MFID90 OSP opcodes (RFSS -> SU)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Mfid90OspOpcode {
    /// Group Regroup Add Command
    MotGrgAddCmd       = 0x00,
    /// Group Regroup Delete Command
    MotGrgDelCmd       = 0x01,
    /// Group Regroup Channel Grant (also Explicit via MBT)
    MotGrgCnGrant      = 0x02,
    /// Group Regroup Channel Update
    MotGrgCnGrantUpdt  = 0x03,
    /// Extended Function Command (IR Command / IR Cancel)
    MotExtFnctCmd      = 0x04,
    /// Queued Response
    MotQueRsp          = 0x06,
    /// Deny Response
    MotDenyRsp         = 0x07,
    /// Acknowledge Response (FNE)
    MotAckRspFne       = 0x08,
}

// ─── MFID90 Extended Function Definitions ────────────────────────────────────

/// Extended Function class for Individual Regrouping
pub const EXT_FNCT_CLASS_IR: u8 = 0x02;

/// Extended Function operands
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum ExtFnctOperand {
    /// OSP: IR Command (assign to dynamic group)
    IrCommand       = 0x00,
    /// OSP: IR Cancel (revert to original group)
    IrCancel        = 0x01,
    /// ISP: IR Command ACK
    IrCommandAck    = 0x80,
    /// ISP: IR Cancel ACK
    IrCancelAck     = 0x81,
}

// ─── MFID90 Deny Reason Codes ───────────────────────────────────────────────

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Mfid90DenyReason {
    /// Encrypted request on a clear-only supergroup
    SecureOnClear = 0x00,
    /// Clear request on an encrypted-only supergroup
    ClearOnSecure = 0x01,
}

// ─── MFIDA4 Definitions ─────────────────────────────────────────────────────

/// MFIDA4 opcode
pub const MFIDA4_GRG_EXENC_CMD_OPCODE: u8 = 0x30;

/// Regroup type (T bit)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RegroupType {
    /// Two-way patch (T=0)
    Patch,
    /// One-way simulselect (T=1, MFIDA4 only)
    Simulselect,
}

/// Regroup target type in GRG_EXENC_CMD
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RegroupTarget {
    Group {
        algorithm_id: u8,
        wgid: u16,
    },
    Individual {
        wuid: u32, // 24-bit, stored in lower 24 bits
    },
}

// ─── Parsed MFID90 TSBK Messages ────────────────────────────────────────────

/// Group Regroup Add Command (OSP opcode 0x00, MFID 0x90)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MotGrgAddCmd {
    pub supergroup: u16,
    pub wgid_1: u16,
    pub wgid_2: u16,
    pub wgid_3: u16,
}

impl MotGrgAddCmd {
    pub fn from_tsbk(data: &[u8; 12]) -> Self {
        Self {
            supergroup: u16::from_be_bytes([data[2], data[3]]),
            wgid_1:     u16::from_be_bytes([data[4], data[5]]),
            wgid_2:     u16::from_be_bytes([data[6], data[7]]),
            wgid_3:     u16::from_be_bytes([data[8], data[9]]),
        }
    }

    /// Iterator over non-zero WGIDs in this command
    pub fn wgids(&self) -> impl Iterator<Item = u16> + '_ {
        [self.wgid_1, self.wgid_2, self.wgid_3]
            .into_iter()
            .filter(|&w| w != 0)
    }

    /// Check if the given WGID appears in this ADD command
    pub fn contains_wgid(&self, wgid: u16) -> bool {
        self.wgid_1 == wgid || self.wgid_2 == wgid || self.wgid_3 == wgid
    }
}

/// Group Regroup Delete Command (OSP opcode 0x01, MFID 0x90)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MotGrgDelCmd {
    pub supergroup: u16,
    pub wgid_1: u16,
    pub wgid_2: u16,
    pub wgid_3: u16,
}

impl MotGrgDelCmd {
    pub fn from_tsbk(data: &[u8; 12]) -> Self {
        Self {
            supergroup: u16::from_be_bytes([data[2], data[3]]),
            wgid_1:     u16::from_be_bytes([data[4], data[5]]),
            wgid_2:     u16::from_be_bytes([data[6], data[7]]),
            wgid_3:     u16::from_be_bytes([data[8], data[9]]),
        }
    }

    /// Returns true if this is a full supergroup dissolution
    /// (WGID-1 equals the SP-WGID)
    pub fn is_dissolve(&self) -> bool {
        self.wgid_1 == self.supergroup
    }

    /// Check if the given WGID is being removed
    pub fn removes_wgid(&self, wgid: u16) -> bool {
        self.wgid_1 == wgid || self.wgid_2 == wgid || self.wgid_3 == wgid
    }
}

/// Group Regroup Channel Grant (OSP opcode 0x02, MFID 0x90)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MotGrgCnGrant {
    pub encrypted: bool,
    pub channel: u16,
    pub supergroup: u16,
    pub source: u32, // 24-bit WUID
}

impl MotGrgCnGrant {
    pub fn from_tsbk(data: &[u8; 12]) -> Self {
        Self {
            encrypted:  (data[2] & 0x10) != 0,
            channel:    u16::from_be_bytes([data[3], data[4]]),
            supergroup: u16::from_be_bytes([data[5], data[6]]),
            source:     ((data[7] as u32) << 16) | ((data[8] as u32) << 8) | (data[9] as u32),
        }
    }

    /// Decode channel identifier into IDEN and channel number
    pub fn channel_iden(&self) -> u8 {
        (self.channel >> 12) as u8
    }

    pub fn channel_number(&self) -> u16 {
        self.channel & 0x0FFF
    }
}

/// Group Regroup Channel Grant -- Explicit (OSP opcode 0x02, MBT, MFID 0x90)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MotGrgCnGrantExp {
    pub encrypted: bool,
    pub channel_t: u16,  // RFSS transmit (SU receive)
    pub channel_r: u16,  // RFSS receive (SU transmit)
    pub supergroup: u16,
    pub source: u32,     // 24-bit WUID
}

/// Group Regroup Channel Update (OSP opcode 0x03, MFID 0x90)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MotGrgCnGrantUpdt {
    pub channel_a: u16,
    pub supergroup_a: u16,
    pub channel_b: u16,
    pub supergroup_b: u16,
}

impl MotGrgCnGrantUpdt {
    pub fn from_tsbk(data: &[u8; 12]) -> Self {
        Self {
            channel_a:    u16::from_be_bytes([data[2], data[3]]),
            supergroup_a: u16::from_be_bytes([data[4], data[5]]),
            channel_b:    u16::from_be_bytes([data[6], data[7]]),
            supergroup_b: u16::from_be_bytes([data[8], data[9]]),
        }
    }

    /// Returns the active supergroup/channel pairs (excludes zero entries)
    pub fn active_pairs(&self) -> Vec<(u16, u16)> {
        let mut pairs = Vec::new();
        if self.supergroup_a != 0 {
            pairs.push((self.supergroup_a, self.channel_a));
        }
        if self.supergroup_b != 0 {
            pairs.push((self.supergroup_b, self.channel_b));
        }
        pairs
    }
}

/// Group Regroup Voice Request (ISP opcode 0x00, MFID 0x90)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MotGrgVReq {
    pub encrypted: bool,
    pub supergroup: u16,
    pub source: u32, // 24-bit WUID
}

impl MotGrgVReq {
    pub fn from_tsbk(data: &[u8; 12]) -> Self {
        Self {
            encrypted:  (data[2] & 0x10) != 0,
            supergroup: u16::from_be_bytes([data[5], data[6]]),
            source:     ((data[7] as u32) << 16) | ((data[8] as u32) << 8) | (data[9] as u32),
        }
    }
}

/// Extended Function Command (OSP opcode 0x04, MFID 0x90)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MotExtFnctCmd {
    pub class: u8,
    pub operand: u8,
    pub arguments: u32, // 24-bit, top-justified in the original 32-bit field
    pub target: u32,    // 24-bit WUID
}

impl MotExtFnctCmd {
    pub fn from_tsbk(data: &[u8; 12]) -> Self {
        Self {
            class:     data[2],
            operand:   data[3],
            arguments: ((data[4] as u32) << 16) | ((data[5] as u32) << 8) | (data[6] as u32),
            target:    ((data[7] as u32) << 16) | ((data[8] as u32) << 8) | (data[9] as u32),
        }
    }

    /// Returns the IR operation if this is an Individual Regrouping command
    pub fn ir_operation(&self) -> Option<IrOperation> {
        if self.class != EXT_FNCT_CLASS_IR {
            return None;
        }
        match self.operand {
            0x00 => Some(IrOperation::Command {
                dynamic_group: (self.arguments >> 8) as u16,
            }),
            0x01 => Some(IrOperation::Cancel),
            _ => None,
        }
    }
}

/// Extended Function Response (ISP opcode 0x01, MFID 0x90)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MotExtFnctRsp {
    pub class: u8,
    pub operand: u8,
    pub arguments: u32, // 24-bit
    pub source: u32,    // 24-bit WUID
}

impl MotExtFnctRsp {
    pub fn from_tsbk(data: &[u8; 12]) -> Self {
        Self {
            class:     data[2],
            operand:   data[3],
            arguments: ((data[4] as u32) << 16) | ((data[5] as u32) << 8) | (data[6] as u32),
            source:    ((data[7] as u32) << 16) | ((data[8] as u32) << 8) | (data[9] as u32),
        }
    }

    /// Returns the IR acknowledgment if this is an IR response
    pub fn ir_ack(&self) -> Option<IrAck> {
        if self.class != EXT_FNCT_CLASS_IR {
            return None;
        }
        let talkgroup = (self.arguments >> 8) as u16;
        match self.operand {
            0x80 => Some(IrAck::CommandAck { new_dynamic_group: talkgroup }),
            0x81 => Some(IrAck::CancelAck { old_dynamic_group: talkgroup }),
            _ => None,
        }
    }
}

/// Queued Response (OSP opcode 0x06, MFID 0x90)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MotQueRsp {
    pub aiv: bool,
    pub service_type: u8,   // 6-bit ISP opcode
    pub reason_code: u8,
    pub supergroup: u16,    // valid when aiv=true (from additional_info bytes 5-6)
    pub target: u32,        // 24-bit WUID
}

impl MotQueRsp {
    pub fn from_tsbk(data: &[u8; 12]) -> Self {
        Self {
            aiv:          (data[2] & 0x80) != 0,
            service_type: data[2] & 0x3F,
            reason_code:  data[3],
            supergroup:   u16::from_be_bytes([data[5], data[6]]),
            target:       ((data[7] as u32) << 16) | ((data[8] as u32) << 8) | (data[9] as u32),
        }
    }
}

/// Deny Response (OSP opcode 0x07, MFID 0x90)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MotDenyRsp {
    /// true = MFID90-specific reason code; false = standard
    pub rc_mfid90: bool,
    /// true = request used a different MFID (not 0x90)
    pub other_mfid: bool,
    pub service_type: u8,     // 6-bit
    pub reason_code: u8,
    pub additional_info: u32, // 24-bit
    pub target: u32,          // 24-bit WUID
}

impl MotDenyRsp {
    pub fn from_tsbk(data: &[u8; 12]) -> Self {
        Self {
            rc_mfid90:       (data[2] & 0x80) != 0,
            other_mfid:      (data[2] & 0x40) != 0,
            service_type:    data[2] & 0x3F,
            reason_code:     data[3],
            additional_info: ((data[4] as u32) << 16) | ((data[5] as u32) << 8) | (data[6] as u32),
            target:          ((data[7] as u32) << 16) | ((data[8] as u32) << 8) | (data[9] as u32),
        }
    }

    /// Interpret the MFID90 deny reason, if applicable
    pub fn mfid90_reason(&self) -> Option<Mfid90DenyReason> {
        if !self.rc_mfid90 {
            return None;
        }
        match self.reason_code {
            0x00 => Some(Mfid90DenyReason::SecureOnClear),
            0x01 => Some(Mfid90DenyReason::ClearOnSecure),
            _ => None,
        }
    }
}

/// Acknowledge Response FNE (OSP opcode 0x08, MFID 0x90)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MotAckRspFne {
    pub service_type: u8,  // 6-bit
    pub source: u32,       // 24-bit (FNE address or application WUID)
    pub target: u32,       // 24-bit WUID
}

impl MotAckRspFne {
    pub fn from_tsbk(data: &[u8; 12]) -> Self {
        Self {
            service_type: data[2] & 0x3F,
            source:       ((data[4] as u32) << 16) | ((data[5] as u32) << 8) | (data[6] as u32),
            target:       ((data[7] as u32) << 16) | ((data[8] as u32) << 8) | (data[9] as u32),
        }
    }
}

// ─── MFIDA4 Parsed Message ──────────────────────────────────────────────────

/// Group Regroup Explicit Encryption Command (MFID 0xA4, opcode 0x30)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct GrgExencCmd {
    pub regroup_type: RegroupType,
    pub is_group: bool,
    pub activate: bool,
    pub ssn: u8,          // 5-bit Supergroup Sequence Number
    pub supergroup: u16,  // SP-WGID
    pub key_id: u16,
    pub target: RegroupTarget,
}

impl GrgExencCmd {
    pub fn from_tsbk(data: &[u8; 12]) -> Self {
        let grg_opts = data[2];
        let is_group = (grg_opts & 0x40) != 0;

        let target = if is_group {
            RegroupTarget::Group {
                algorithm_id: data[7],
                wgid: u16::from_be_bytes([data[8], data[9]]),
            }
        } else {
            RegroupTarget::Individual {
                wuid: ((data[7] as u32) << 16) | ((data[8] as u32) << 8) | (data[9] as u32),
            }
        };

        Self {
            regroup_type: if (grg_opts & 0x80) != 0 { RegroupType::Simulselect } else { RegroupType::Patch },
            is_group,
            activate: (grg_opts & 0x20) != 0,
            ssn: grg_opts & 0x1F,
            supergroup: u16::from_be_bytes([data[3], data[4]]),
            key_id: u16::from_be_bytes([data[5], data[6]]),
            target,
        }
    }
}

// ─── Individual Regrouping Types ─────────────────────────────────────────────

/// Decoded IR operation from Extended Function Command
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum IrOperation {
    /// Assign SU to a dynamic talk group
    Command { dynamic_group: u16 },
    /// Revert SU to its original talk group
    Cancel,
}

/// Decoded IR acknowledgment from Extended Function Response
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum IrAck {
    CommandAck { new_dynamic_group: u16 },
    CancelAck { old_dynamic_group: u16 },
}

// ─── Link Control Messages ──────────────────────────────────────────────────

/// MFID90 Group Regroup Voice Channel User LC (LCO 0x00)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct LcMotGrgVChUsr {
    pub encrypted: bool,
    pub source_ext_required: bool,  // S-bit
    pub supergroup: u16,
    pub source: u32,  // 24-bit WUID
}

impl LcMotGrgVChUsr {
    pub fn from_lc(data: &[u8; 9]) -> Self {
        Self {
            encrypted:           (data[2] & 0x10) != 0,
            source_ext_required: (data[3] & 0x01) != 0,
            supergroup:          u16::from_be_bytes([data[4], data[5]]),
            source:              ((data[6] as u32) << 16) | ((data[7] as u32) << 8) | (data[8] as u32),
        }
    }
}

/// MFID90 Group Regroup Voice Channel Update LC (LCO 0x01)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct LcMotGrgVChUpdt {
    pub encrypted: bool,
    pub supergroup: u16,
    pub channel: u16,
}

impl LcMotGrgVChUpdt {
    pub fn from_lc(data: &[u8; 9]) -> Self {
        Self {
            encrypted:  (data[2] & 0x10) != 0,
            supergroup: u16::from_be_bytes([data[3], data[4]]),
            channel:    u16::from_be_bytes([data[7], data[8]]),
        }
    }
}

// ─── TDMA MAC Messages ──────────────────────────────────────────────────────

/// MFID90 MAC opcodes (MCO values, TDMA traffic channel)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Mfid90MacOpcode {
    GrgVChUsr            = 0x00,  // 8 bytes, abbreviated
    GrgAddCmd            = 0x01,  // variable
    GrgVChUpdt           = 0x03,  // 7 bytes
    ExtFnctCmd           = 0x04,  // 11 bytes
    GrgDelCmd            = 0x09,  // variable
    GrgVChUsrExt         = 0x20,  // 16 bytes, extended
    GrgVReq              = 0x21,  // 9 bytes
    ExtFnctRsp           = 0x22,  // 11 bytes
    GrgCnGrantImplicit   = 0x23,  // 11 bytes
    GrgCnGrantExplicit   = 0x24,  // 13 bytes
    GrgCnGrantUpdt       = 0x25,  // 11 bytes
    QueRsp               = 0x26,  // 11 bytes
    DenyRsp              = 0x27,  // 11 bytes
    AckRspFne            = 0x28,  // 10 bytes
}

/// Unified enum for all parsed MFID90 messages across TSBK, LC, and MAC
#[derive(Debug, Clone)]
pub enum DynamicRegroupMessage {
    // MFID90 TSBK / MAC
    GrgAddCmd(MotGrgAddCmd),
    GrgDelCmd(MotGrgDelCmd),
    GrgCnGrant(MotGrgCnGrant),
    GrgCnGrantExp(MotGrgCnGrantExp),
    GrgCnGrantUpdt(MotGrgCnGrantUpdt),
    GrgVReq(MotGrgVReq),
    ExtFnctCmd(MotExtFnctCmd),
    ExtFnctRsp(MotExtFnctRsp),
    QueRsp(MotQueRsp),
    DenyRsp(MotDenyRsp),
    AckRspFne(MotAckRspFne),

    // MFID90 Link Control / MAC traffic
    GrgVChUsr(LcMotGrgVChUsr),
    GrgVChUpdt(LcMotGrgVChUpdt),

    // MFIDA4 TSBK
    GrgExencCmd(GrgExencCmd),
}

// ─── Supergroup State Tracking ──────────────────────────────────────────────

/// Tracks the regrouping state for an SU (receiver-side state machine)
#[derive(Debug, Clone)]
pub struct RegroupState {
    /// Current group regrouped state (if any)
    pub group_regroup: Option<GroupRegroupEntry>,
    /// Current individual regroup state (if any)
    pub individual_regroup: Option<IndividualRegroupEntry>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct GroupRegroupEntry {
    pub supergroup: u16,       // SP-WGID
    pub original_wgid: u16,    // The SU's affiliated WGID
    pub ssn: Option<u8>,       // SSN (MFIDA4 only, 5-bit)
    pub method: RegroupMethod,
    pub regroup_type: RegroupType,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RegroupMethod {
    Mfid90,
    Mfida4,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct IndividualRegroupEntry {
    pub dynamic_group: u16,    // Dynamically assigned talk group GID
    pub original_wgid: u16,    // WGID before regrouping
}

impl Default for RegroupState {
    fn default() -> Self {
        Self {
            group_regroup: None,
            individual_regroup: None,
        }
    }
}
```

---

## 10. SDRTrunk / OP25 Cross-Reference

### 10.1 SDRTrunk

SDRTrunk implements MFID90 TSBK parsing in its Motorola-specific message packages:

| Java Class | This Spec Section |
|------------|-------------------|
| `MotorolaGroupRegroupAddCommand` | 3.2.1 (MOT_GRG_ADD_CMD) |
| `MotorolaGroupRegroupDeleteCommand` | 3.2.2 (MOT_GRG_DEL_CMD) |
| `MotorolaGroupRegroupChannelGrant` | 3.2.3 (MOT_GRG_CN_GRANT) |
| `MotorolaGroupRegroupChannelGrantUpdate` | 3.2.5 (MOT_GRG_CN_GRANT_UPDT) |
| `MotorolaGroupRegroupVoiceRequest` | 3.1.1 (MOT_GRG_V_REQ) |
| `MotorolaExtendedFunctionCommand` | 3.2.6 (MOT_EXT_FNCT_CMD) |
| `MotorolaQueuedResponse` | 3.2.7 (MOT_QUE_RSP) |
| `MotorolaDenyResponse` | 3.2.8 (MOT_DENY_RSP) |
| `MotorolaAcknowledgeResponse` | 3.2.9 (MOT_ACK_RSP_FNE) |

SDRTrunk uses MFID90 supergroup messages to maintain a patch group mapping that shows
the original talk group names alongside the supergroup ID in its channel display.

### 10.2 OP25

OP25 handles MFID90 TSBKs in `p25p1_fdma.cc` with a vendor-specific dispatch on MFID:
- MFID 0x90 messages are decoded for logging/display
- Supergroup channel grants trigger the same frequency-tracking logic as standard grants
- The GRG_ADD_CMD and GRG_DEL_CMD are used to maintain a supergroup-to-WGID mapping

### 10.3 Implementation Notes from Open-Source Projects

1. **Opcode collision:** ISP and OSP opcode 0x00 are different messages. Implementations
   must track whether a TSBK is inbound or outbound (typically inferred from the channel
   direction or the OSP/ISP context in the control channel slot).

2. **Supergroup pool:** In practice, Motorola systems use SP-WGIDs in a range that does
   not overlap with regular talk groups. Common ranges observed: 0xFCxx-0xFFxx or
   system-specific reserved ranges. Implementations should not hard-code these ranges
   but instead track ADD/DEL commands to build the supergroup map dynamically.

3. **Channel grant handling:** A MOT_GRG_CN_GRANT should be treated identically to a
   standard GRP_V_CH_GRANT for frequency-tracking purposes. The channel field uses the
   same IDEN + channel number encoding.

4. **Extended function dispatch:** Class 0x02 is Individual Regrouping. Other class values
   exist for non-regrouping Motorola extended functions (radio check, inhibit, etc.) and
   should not be confused with regrouping operations.

---

## 11. FDMA/TDMA Message Equivalence Quick Reference

| Function | FDMA TSBK (Opcode) | FDMA LC (LCO) | TDMA MAC (MCO) |
|----------|-------------------|----------------|-----------------|
| Group Regroup Add | OSP 0x00, MFID90 | -- | 0x01, MFID90 |
| Group Regroup Delete | OSP 0x01, MFID90 | -- | 0x09, MFID90 |
| Group Regroup Channel Grant | OSP 0x02, MFID90 | -- | 0x23, MFID90 |
| Group Regroup Channel Grant (Explicit) | OSP 0x02, MFID90 (MBT) | -- | 0x24, MFID90 |
| Group Regroup Channel Update | OSP 0x03, MFID90 | -- | 0x25, MFID90 |
| Group Regroup Voice Request | ISP 0x00, MFID90 | -- | 0x21, MFID90 |
| Extended Function Command | OSP 0x04, MFID90 | -- | 0x04, MFID90 |
| Extended Function Response | ISP 0x01, MFID90 | -- | 0x22, MFID90 |
| Queued Response | OSP 0x06, MFID90 | -- | 0x26, MFID90 |
| Deny Response | OSP 0x07, MFID90 | -- | 0x27, MFID90 |
| Acknowledge Response | OSP 0x08, MFID90 | -- | 0x28, MFID90 |
| Group Regroup Voice Channel User | -- | LCO 0x00, MFID90 | MCO 0x00 (abbrev), 0x20 (ext), MFID90 |
| Group Regroup Voice Channel Update | -- | LCO 0x01, MFID90 | MCO 0x03, MFID90 |
| Group Regroup Explicit Encryption | OSP 0x30, MFIDA4 | -- | -- (standard MACs used) |
