# P25 Conventional Procedures -- Implementation Specification

**Source:** TIA-102.BAAD-B (July 2015), "Project 25 Conventional Procedures"
**Phase:** 3 -- Implementation-ready
**Classification:** PROTOCOL
**Extracted:** 2026-04-12
**Purpose:** Self-contained behavioral spec for implementing P25 conventional (non-trunked)
operation -- voice call setup/teardown, supplementary data, channel access, emergency
procedures, and packet data on conventional channels. This is the behavioral companion to
TIA-102.AABG (conventional control messages) and TIA-102.BAAA-B (FDMA physical layer).
No reference to the original PDF required for implementation.

**Companion specs (cross-referenced throughout):**
- **BAAA-B** -- FDMA physical layer: frame sync, NID, HDU/LDU/TDU/TSDU framing
- **AABF-D** -- Link Control Word formats: LC_GRP_V_CH_USR, LC_UU_V_CH_USR, LC_TELE_INT_V_CH_USR, LC_CALL_TRM_CAN
- **AABG** -- Conventional control TSBK messages: EMRG_ALRM_REQ, CALL_ALRT_REQ, EXT_FNCT_CMD, etc.
- **AABC-E** -- TSBK wire formats (shared between trunking and conventional)
- **BAAC-D** -- Reserved values: NAC, UID, TGID ranges
- **AAAD-B** -- Block encryption protocol
- **BAED-A** -- Packet data LLC procedures (separated from BAAD in Revision B)
- **BABA-A** -- IMBE vocoder (88-bit voice frame)
- **SDRTrunk** / **OP25** -- Open-source reference implementations for conventional mode

---

## Table of Contents

1. [Conventional vs. Trunked Operation](#1-conventional-vs-trunked-operation)
2. [Voice Call Procedures](#2-voice-call-procedures)
3. [Emergency Procedures on Conventional Channels](#3-emergency-procedures-on-conventional-channels)
4. [Data Procedures on Conventional](#4-data-procedures-on-conventional)
5. [Console Operation](#5-console-operation)
6. [Channel Access Procedures](#6-channel-access-procedures)
7. [TSBK Signaling on Conventional](#7-tsbk-signaling-on-conventional)
8. [State Machines for SU Conventional Operation](#8-state-machines-for-su-conventional-operation)
9. [Timing Parameters](#9-timing-parameters)
10. [Parser Pseudocode for Conventional Call Tracking](#10-parser-pseudocode-for-conventional-call-tracking)
11. [Rust Struct Definitions](#11-rust-struct-definitions)

---

## 1. Conventional vs. Trunked Operation

### 1.1 Fundamental Architectural Difference

| Attribute | Trunked (AABD-B) | Conventional (BAAD-B) |
|-----------|-------------------|-----------------------|
| Control channel | Dedicated outbound CC with broadcast rotation | **None** -- no dedicated control channel |
| Channel assignment | FNE grants traffic channels via OSP (e.g., GRP_V_CH_GRANT) | **None** -- SUs are pre-programmed to a fixed frequency |
| Channel access | Slotted Aloha (inbound CC), granted (traffic) | **Carrier-sense / polite access** -- listen before talk |
| Registration | GRP_AFF_REQ / U_REG_REQ to RFSS | **Placeholder only** -- Section 5 of source is unpopulated |
| Signaling transport | TSBK on dedicated CC | TSBK on the **same channel** used for voice/data |
| Repeater role | Channel-switching infrastructure | **Transparent relay** -- receives and retransmits |

### 1.2 Channel Configurations

There are two fundamental modes:

**Direct Mode (talk-around):**
```
SU_A ----[RF]----> SU_B
  (simplex, same frequency TX and RX)
```

**Repeated Mode:**
```
SU_A --[inbound RF]--> Fixed_Station --[outbound RF]--> SU_B
  (half-duplex SUs, full-duplex FS)
```

Repeaters further subdivide into:

- **Simple Configuration:** Transparent relay. The FS receives, optionally error-corrects,
  and retransmits. No FNE connection. The FS may substitute LC information but must echo
  the source SU's LC at least every NTrafficLC (default 3) LC messages.

- **Fixed Network Configuration:** The FS connects to FNE infrastructure (consoles, other
  FNE elements). Voice is simultaneously repeated to air AND routed to the FNE. Data packets
  are routed asymmetrically (to FNE) or symmetrically (back to air), per BAED-A addressing.

### 1.3 Addressing

| Address Type | Width | Range | Notes |
|---|---|---|---|
| Unit ID (UID) | 24 bits | 1 -- 9,999,999 ($000001 -- $98967F) | Individual SU address |
| Talk Group ID (TGID) | 16 bits | $0001 -- $FFFF | Group address |
| All Call TGID | 16 bits | $FFFF | Every SU is a member |
| Null TGID | 16 bits | $0000 | Used only in unit-to-unit Header Word |
| General FNE Address | 24 bits | $FFFFFC | Generic FNE source/destination for supplementary data |
| 24-bit TGID mapping | 24 bits | $FEF001 -- $FFEFFF | Mapped from 16-bit: $FEF001 = TGID $0001 |

**Mapping formula:** `tgid_24bit = 0xFEF000 + tgid_16bit`

### 1.4 Network Access Code (NAC)

The NAC is a 12-bit field in the NID codeword (see BAAA-B Section 3) that provides co-channel
interference protection.

| NAC Value | Meaning |
|---|---|
| $001 -- $F7D | Operator-assigned values |
| $293 | Default NAC |
| $F7E | Receiver opens on ANY NAC (receive-only, never transmitted) |
| $F7F | Repeater passes through received NAC unchanged (never transmitted by SU) |

**Rules:**
- SUs shall NEVER transmit NAC $F7E or $F7F
- A repeater may use different NAC values for receive and transmit
- If assigned a single NAC, the repeater uses it for both directions
- If receive NAC is unreserved, only accept and repeat transmissions matching that NAC

---

## 2. Voice Call Procedures

### 2.1 Voice Data Unit Sequencing (BAAA-B Cross-Reference)

A P25 FDMA voice transmission consists of a sequence of data units transmitted continuously
from PTT-assert to PTT-release. The frame structure per BAAA-B:

```
[Preamble] -> [HDU] -> [LDU1] -> [LDU2] -> [LDU1] -> [LDU2] -> ... -> [TDU]
                                                                          or
                                                                        [TDULC]
```

| Data Unit | Duration | Contents | Conventional Role |
|---|---|---|---|
| HDU (Header Data Unit) | 648 bits / 67.5 ms | MI, MFID, ALGID, KID, TGID | Establishes call: encryption params + talk group |
| LDU1 (Logical Data Unit 1) | 1728 bits / 180 ms | 9 IMBE voice frames + LC word | Carries voice + link control (LC_GRP_V_CH_USR etc.) |
| LDU2 (Logical Data Unit 2) | 1728 bits / 180 ms | 9 IMBE voice frames + ES (Encryption Sync) | Carries voice + encryption sync (MI, ALGID, KID) |
| TDU (Simple Terminator) | 288 bits / 30 ms | NID only, no payload | SU sends this to terminate inbound |
| TDULC (Terminator w/ LC) | 432 bits / 45 ms | LC word (e.g., LC_CALL_TRM_CAN) | FNE may send this (ETDU/CTC) on outbound to terminate |

**Superframe timing:** One LDU1+LDU2 pair = 360 ms. A typical short PTT = HDU + 1 LDU pair
+ TDU = approximately 427.5 ms minimum.

### 2.2 Voice Call Types

#### 2.2.1 Routine Group Call

**Originating SU actions:**
1. Assert PTT -- begin channel access procedure (Section 6)
2. Transmit HDU with:
   - TGID = selected talk group ($0001 -- $FFFF)
   - ALGID/KID/MI per encryption config (or clear: ALGID=$80, KID=$0000)
   - MFID per BAAA-B rules
3. Transmit continuous LDU1/LDU2 pairs:
   - LDU1 LC field = `LC_GRP_V_CH_USR` (AABF-D opcode $00):
     - Emergency bit (E) = 0 for routine
     - Service Options as configured
     - Group Address = TGID (16-bit, in 24-bit LC field use 24-bit mapped form)
     - Source Address = SU UID
   - LDU2 ES field = encryption sync (MI, ALGID, KID)
4. On PTT release, transmit 1--3 Simple TDU(s)

**FNE response (repeat mode):**
- Repeat inbound voice to outbound, substituting transmit NAC
- Set outbound status symbols to BUSY while receiving inbound
- May substitute LC but must echo source SU's LC_GRP_V_CH_USR at least every NTrafficLC (3) LC slots
- May convert Simple TDU to TDULC (LC_CALL_TRM_CAN) on outbound path

**Receiving SU actions -- squelch modes:**

| Mode | NAC Check | TGID Check | Encryption Check |
|---|---|---|---|
| Monitor | No | No | No (may mute if cannot decrypt) |
| Normal Squelch | Yes (match or $F7E) | No | Yes (must be able to decrypt) |
| Selective Squelch | Yes (match or $F7E) | Yes (match or $FFFF) | Yes (must be able to decrypt) |

**Hang time:** After receiving a call, the SU may retain the received TGID for a configurable
hang time window. If the user keys PTT during hang time, the SU transmits on the received
TGID (useful when monitoring multiple groups or responding to All Call). It is recommended
that SUs configured with hang time do NOT respond to All Call ($FFFF) with an All Call
transmission.

#### 2.2.2 Unit-to-Unit Call

Identical to group call except:
- Header Word TGID = $0000 (null)
- LDU1 LC = `LC_UU_V_CH_USR` (AABF-D opcode $04):
  - Destination Address = target UID (24-bit)
  - Source Address = originating UID
  - Service Options field is entirely Reserved (no emergency bit)
- No conventional emergency unit-to-unit call exists

**Receiving SU squelch:**

| Mode | NAC Check | Destination UID Check | Encryption Check |
|---|---|---|---|
| Monitor | No | No | No |
| Normal Squelch | Yes | No | Yes |
| Selective Squelch | Yes | Yes (match own UID) | Yes |

#### 2.2.3 All Call

- Group call with TGID = $FFFF
- Every SU is a member of $FFFF
- Typically FNE-initiated (SUs should not originate All Call)
- SUs shall unmute if other squelch criteria (NAC) are met

#### 2.2.4 Unaddressed Call Mode

For operators who do not want TGID-based squelch (analog FM equivalent):
- Set TGID = $0001 (recommended default)
- SUs operate in Normal Squelch mode (NAC check only, no TGID filtering)

#### 2.2.5 PSTN Interconnect Calls

**SU-to-PSTN (outbound):**
1. SU sends `TELE_INT_DIAL_REQ` (explicit) or `TELE_INT_PSTN_REQ` (implicit) via TSBK
   channel access (Section 6.2)
2. FNE may respond with: `ACK_RSP_FNE` (processing), `QUE_RSP` (busy), `DENY_RSP` (rejected)
3. During call: LDU1 LC = `LC_TELE_INT_V_CH_USR` (AABF-D opcode $06)
4. Termination: SU sends `CAN_SRV_REQ` or pre-programmed disconnect code; FNE sends TDULC
   (LC_CALL_TRM_CAN / ETDU/CTC) with target = SU UID

**PSTN-to-Group:**
- FNE receives incoming call, provides audible alert
- Voice transmitted on outbound with LC_GRP_V_CH_USR
- Only FNE or telephone network can terminate

**PSTN-to-Unit:**
- FNE may send `TELE_INT_ANS_REQ` to target SU for availability check
- Target SU responds with `TELE_INT_ANS_RSP` (PROCEED or DENY)
- Timer: recommended 1 second wait for response; if expired, end call
- During call: LC_TELE_INT_V_CH_USR in LDU1

### 2.3 Call Termination Rules

**Inbound (SU to FNE):**
- SU transmits 1--3 Simple TDU(s) at end of transmission
- A transmit timer may exist in the SU to limit maximum transmit duration

**Outbound (FNE to SUs):**
- FNE may send one or more Simple TDU or TDULC (LC_CALL_TRM_CAN / ETDU/CTC)
- ETDU/CTC target address:
  - Group calls: $FFFFFF
  - Unit-to-unit calls: $FFFFFF or one of the individual UIDs (source or target)
- FNE may convert received Simple TDU to ETDU/CTC on the repeat path

**FNE-initiated termination triggers:**
1. Non-radio call participant (console, PSTN) signals call end
2. No activity on channel for TNoAct (default 3 seconds)
3. Simple TDU received from transmitting SU

**Receiving SU:** Upon receiving either Simple TDU or ETDU/CTC, consider the active
transmission complete.

### 2.4 Encryption Constraints (Table 2 from Source)

Voice, LSD (Low Speed Data), and LC fields may be independently clear or encrypted, but
only these combinations are legal:

| Voice | LSD | LC | Allowed? | LC Format |
|---|---|---|---|---|
| Clear | Clear | Clear | YES | Unencrypted |
| Encrypted | Encrypted | Clear | YES | Unencrypted |
| Encrypted | Encrypted | Encrypted | YES | Encrypted |
| All other combinations | | | **NOT ALLOWED** | |

Rule: If voice is encrypted, LSD must also be encrypted. LC may optionally also be encrypted.
If voice is clear, everything must be clear.

---

## 3. Emergency Procedures on Conventional Channels

### 3.1 Emergency Operating Modes

An SU entering emergency mode may operate in one of three combinations:

| Mode | Emergency Alarm TSBK | Emergency Bit in Voice LC | Description |
|---|---|---|---|
| Emergency Call Only | No | Yes (E=1 in LC_GRP_V_CH_USR) | Voice-only emergency indication |
| Emergency Alarm Only | Yes (EMRG_ALRM_REQ) | No (E=0) | Silent alarm to dispatch |
| Emergency Alarm + Call | Yes (EMRG_ALRM_REQ) | Yes (E=1) | Both alarm and voice indication |

### 3.2 Emergency Alarm Procedure

**TSBK message:** `EMRG_ALRM_REQ` (AABG ISP opcode $27)

**Channel access:** Combined impolite + polite (Section 6.2.2 below):
- Typical: 5 impolite attempts followed by 15 polite attempts
- Impolite: transmit regardless of channel busy state
- Polite: check channel idle before transmitting

**Preamble for impolite emergency:** Recommended high dotting pattern ($5F) lasting
longer than one LDU (possibly 2+ LDUs) to enable RF capture at the receiver.

**Sequence:**
```
SU                              FNE
 |                               |
 |-- EMRG_ALRM_REQ ------------>|  (impolite attempt 1..N)
 |          ...                  |
 |-- EMRG_ALRM_REQ ------------>|  (polite attempts)
 |                               |
 |<--------- ACK_RSP_FNE -------|  (acknowledgement)
 |                               |
```

**FNE actions on receipt:**
- Send `ACK_RSP_FNE` to SU (may wait for console delivery confirmation)
- May transparently repeat the EMRG_ALRM_REQ on outbound (repeat mode)
- Notify consoles and wireline monitoring equipment
- Begin tracking the emergency condition for this SU

**SU actions on ACK:**
- Stop retries
- May notify user of success
- May activate internal emergency state: subsequent group voice transmissions sent with E=1

### 3.3 Emergency Alarm Cancellation

The SU cancels its own emergency via `CAN_SRV_REQ` (AABG ISP opcode $23):

**Required field encoding:**
- AIV = 1 (Additional Information Valid)
- Service Type = $27 (Emergency Alarm Request opcode)
- Reason Code = $00
- Additional Information octet 4 = %1xxx xxxx (emergency bit = true, echoing original)
- Additional Information octets 5-6 = 16-bit group address
- Source Address = SU UID

**FNE response:** ACK_RSP_FNE; notify consoles of cancellation.

**Critical:** The FNE cannot cancel the SU's internal emergency state. Only the user can
clear the SU's emergency mode (typically by holding the emergency switch for an extended
period). The FNE cancellation only clears the FNE's tracking of the condition.

### 3.4 Emergency Group Voice Call

Identical to routine group call (Section 2.2.1) except:
- Emergency bit (E) = 1 in LC_GRP_V_CH_USR Service Options field
- Channel access may be impolite (configurable)
- Receiving SU must indicate emergency mode to user
- Emergency state persists until user clears it -- not clearable by the LMR system

**No conventional emergency unit-to-unit call exists.** The Service Options field in
LC_UU_V_CH_USR is entirely Reserved.

---

## 4. Data Procedures on Conventional

### 4.1 Packet Data

Packet data on conventional channels follows the procedures in TIA-102.BAED-A (Packet Data
LLC) with channel access defined in Section 6.3 below. Key points:

- All packet data transmissions are **polite** (listen before talk)
- Uses the six-state CSMA machine (Section 6.3)
- Data packets are FDMA data units per BAAA-B
- NAC qualification: receiver accepts if NAC matches or receiver NAC = $F7E

**Routing in Fixed Network configuration:**
- **Asymmetric addressing:** Inbound data routed to FNE for processing (not repeated to air)
- **Symmetric addressing:** Inbound data may be repeated to outbound path
- FNE-originated data packets transmitted on outbound

**Direct mode limitation:** Idle status symbols are not available. Polite access relies
solely on carrier-sense (no-carrier detection).

### 4.2 Supplementary Data (TSBK)

Supplementary data messages use the TSBK format defined in AABG and are transmitted on the
same channel as voice. Ten services are defined (Section 7 below). Channel access follows
the TSBK state machines in Section 6.2.

---

## 5. Console Operation

### 5.1 Console Wireline Priority

In repeated LMR systems with wireline consoles, a console and SU may transmit simultaneously.
The console is given priority and captures the repeat path:

```
Console audio -> FNE -> outbound path (priority)
SU audio ------> FNE -> wireline only (not repeated)
```

Result: Monitoring consoles hear both the transmitting console's audio AND the inbound SU's
audio simultaneously. SUs on the air hear only the console audio. This is also called
"Console Priority" or "Console Takeover."

### 5.2 FNE-Sourced Transmissions

The FNE (typically at console request) can originate all voice call types:
- Group call: same procedures as SU-originated (Section 2.2.1), FNE takes originating SU role
- Emergency group call: same as SU emergency (Section 3.4)
- Unit-to-unit call: same as SU-originated (Section 2.2.2)
- All Call: FNE may initiate (SUs should not)

The FNE can also originate supplementary data:
- Non-emergency TSBK requests (polite access per Section 6.2.1 principles)
- TSBK acknowledgements (not polite -- send when ready)
- The FNE does NOT originate Emergency Alarm (EMRG_ALRM_REQ)

### 5.3 Repeat Mode Operation

In repeat mode, the FNE:
1. Receives inbound voice/data/TSBK on the receive frequency
2. Validates NAC
3. Retransmits on the outbound frequency with:
   - Substituted transmit NAC (unless $F7F configured)
   - Status symbols reflecting inbound channel state (Busy/Idle)
   - May substitute LC content but must echo source SU's LC every NTrafficLC messages
4. Simultaneously routes to wireline infrastructure (consoles) in Fixed Network configuration

### 5.4 Multi-Site Conventional

When conventional channels span multiple sites:
- Each repeater site has its own NAC(s)
- SUs select NAC based on proximity (see BAAD-B Figure 2)
- Multiple repeaters can share a frequency using different inbound NACs but same outbound NAC
- Console dispatch can span sites via wireline FNE interconnection
- In multi-RFSS configurations, message names may be transformed at RFSS boundaries
  (e.g., CALL_ALRT_REQ -> CALL_ALRT per AABG Section 1.3)

### 5.5 Status Symbols and AFC

**Status symbols** are 2-bit fields embedded in voice/data transmissions at microslot
boundaries (every 7.5 ms per BAAA-B):

| Value | Meaning | Who Asserts |
|---|---|---|
| Busy | Inbound channel has activity | Repeater/FNE on outbound |
| Idle | Inbound channel has no activity | Repeater/FNE on outbound |
| Unknown | Cannot indicate inbound state (repeater mode) | SU inbound to repeater |
| Unknown/Direct | Cannot indicate inbound state (direct mode) | SU in direct/talk-around |

**Slotted status:** The repeater asserts Busy or Idle only at slot boundaries (every N
microslots). Intervening microslot boundaries carry Unknown. Simplest case: N=1.

**AFC (Automatic Frequency Control):** SUs may lock their reference oscillator to the
repeater's carrier frequency by detecting Busy/Idle (which only repeaters transmit).
AFC unlocks when the repeater stops transmitting. This is critical in 769--805 MHz band
operation per FCC regulations.

---

## 6. Channel Access Procedures

### 6.1 Voice Channel Access

**Polite access (default for voice):**
1. Check inbound channel busy status (via status symbols or carrier sense)
2. If Idle: transmit voice
3. If Busy or Unknown: inform user of failure to transmit

**Impolite access (bypasses busy check):**
- Configured for emergency calls
- Required for direct mode (no status symbols available from repeater)

**Configuration options:** SU may be configured to always check, never check, or check
based on service type.

```
State: CHECK_IF_SLOT_IDLE
  |
  |-- Slot Idle -----------> TRANSMIT_VOICE -> Done
  |
  |-- Slot Busy/Unknown ---> INFORM_USER_FAILURE -> Done
```

### 6.2 TSBK Channel Access

Three transmission categories with different access rules:

#### 6.2.1 Non-Emergency SD Requests (Polite)

```
State Machine: NON_EMERG_TSBK_TX

INITIAL:
  on TSBK_REQUEST:
    polite_retries_remaining = NPRetry  (default 5)
    -> CHECK_IF_SLOT_IDLE

CHECK_IF_SLOT_IDLE:
  if channel_idle:
    transmit TSBK
    start timer TAck (default 200 ms)
    -> WAITING_FOR_ACK
  elif polite_retries_remaining > 0:
    start timer TNextAttempt (default 200 ms)
    -> WAITING_FOR_ACK
  else:
    -> DONE (failure)

WAITING_FOR_ACK:
  on ACK_RSP_FNE received:
    -> DONE (success)
  on timer expired:
    if polite_retries_remaining > 0:
      polite_retries_remaining -= 1
      start timer THoldoff (random, see Section 9)
      -> WAITING_FOR_HOLDOFF
    else:
      -> DONE (failure)

WAITING_FOR_HOLDOFF:
  on ACK_RSP_FNE received:
    -> DONE (success)
  on THoldoff expired:
    -> CHECK_IF_SLOT_IDLE
```

Maximum total attempts = NPRetry + 1.

#### 6.2.2 Emergency Alarm SD Requests (Impolite + Polite)

```
State Machine: EMERG_ALARM_TSBK_TX

INITIAL:
  impolite_retries = NIRetry  (default 0, typical 5 for emergency)
  polite_retries = NPRetry    (default 5, typical 15 for emergency)
  transmit TSBK immediately (impolite)
  start timer TAck
  -> WAITING_FOR_ACK

WAITING_FOR_ACK:
  on ACK_RSP_FNE received:
    -> DONE (success)
  on timer expired:
    if impolite_retries > 0:
      impolite_retries -= 1
      start timer THoldoff (short, fixed)
      -> WAITING_IMPOLITE_HOLDOFF
    elif polite_retries > 0:
      start timer THoldoff (random)
      -> WAITING_POLITE_HOLDOFF
    else:
      -> DONE (failure)

WAITING_IMPOLITE_HOLDOFF:
  on ACK_RSP_FNE received:
    -> DONE (success)
  on timer expired:
    transmit TSBK immediately (impolite)
    start timer TAck
    -> WAITING_FOR_ACK

WAITING_POLITE_HOLDOFF:
  on ACK_RSP_FNE received:
    -> DONE (success)
  on THoldoff expired:
    polite_retries -= 1
    -> CHECK_IF_SLOT_IDLE

CHECK_IF_SLOT_IDLE:
  if channel_idle:
    transmit TSBK
    start timer TAck
    -> WAITING_FOR_ACK
  elif polite_retries > 0:
    start timer TNextAttempt
    -> WAITING_FOR_ACK
  else:
    -> DONE (failure)
  on ACK_RSP_FNE received:
    -> DONE (success)
```

#### 6.2.3 SD Acknowledgements (Single Polite Attempt)

```
State Machine: SD_ACK_TX

INITIAL:
  on ACK_TO_SEND:
    -> CHECK_IF_SLOT_IDLE

CHECK_IF_SLOT_IDLE:
  if channel_idle:
    transmit TSBK
    -> DONE
  else:
    -> DONE (rely on sender retry)
    OR hold ack and retry when idle (implementation choice)
```

No retries for acknowledgements. If channel is busy, either drop the ack (sender will
retry the original message) or queue it for later transmission when idle.

### 6.3 Packet Data Channel Access (CSMA)

Six-state machine for polite packet data access:

```
                         ┌──────────────┐
                    ┌───>│ RECEIVE_IDLE │<────────────────────────────────┐
                    │    └──────┬───────┘                                 │
                    │           │ new pkt + carrier < FSSP                │
                    │           v                                         │
                    │    ┌──────────────────┐     FSSP timeout           │
                    │    │ FS_AND_NAC_SEEK  │──────────────────┐         │
                    │    └──────┬───────────┘                  │         │
                    │           │ found FS+NAC                 │         │
                    │           v                              │         │
                    │    ┌──────────────────────┐              │         │
                    │    │ WAIT_RANDOM_T_SHORT  │              │         │
                    │    └──────┬───────────────┘              │         │
                    │           │ delay over                   │         │
                    │           v                              v         │
  lost FS/NAC ┌────┴───────────────────────────┐    ┌────────────────┐  │
  ────────────│ WAIT_FOR_END_OF_MICRO_SLOT     │    │ TRANSMIT_PKT   │──┘
              │  IDLE -> TX                    │───>│                │
              │  BUSY -> BACKOFF               │    └────────────────┘
              │  UNKNOWN -> stay               │              ^
              └────────────┬───────────────────┘              │
                           │ BUSY                             │
                           v                                  │
              ┌────────────────────────┐   delay over         │
              │ RANDOM_BACKOFF_DELAY   │──────────────────────┘
              └────────────────────────┘     (back to WAIT_FOR_END...)
```

**State details:**

1. **RECEIVE_IDLE:** Default state. If new packet to send:
   - No carrier detected -> TRANSMIT_PACKET
   - Carrier detected >= FSSP (728 ms) -> TRANSMIT_PACKET
   - Carrier detected < FSSP -> FS_AND_NAC_SEEK

2. **FS_AND_NAC_SEEK:** Try to locate frame sync on outbound channel:
   - Found FS + NAC match -> WAIT_RANDOM_T_SHORT
   - FSSP timeout with no FS found -> TRANSMIT_PACKET

3. **WAIT_RANDOM_T_SHORT:** Delay uniform random [0, T_SHORT] (0--50 ms) to reduce collision
   potential, then -> WAIT_FOR_END_OF_MICRO_SLOT

4. **WAIT_FOR_END_OF_MICRO_SLOT:** Wait for next microslot boundary, check status symbol:
   - IDLE -> TRANSMIT_PACKET
   - BUSY -> RANDOM_BACKOFF_DELAY
   - UNKNOWN -> remain (wait for next boundary)
   - Lost sync -> FS_AND_NAC_SEEK

5. **RANDOM_BACKOFF_DELAY:** Delay uniform random:
   - Data packets: [0, T_LONG] (0--500 ms)
   - Response packets: [0, T_ACK] (0--250 ms) -- gives responses priority
   - Then -> WAIT_FOR_END_OF_MICRO_SLOT

6. **TRANSMIT_PACKET:** Transmit the packet, then -> RECEIVE_IDLE

---

## 7. TSBK Signaling on Conventional

### 7.1 Message Inventory (AABG Cross-Reference)

All conventional supplementary data uses TSBK-format messages. Per AABG, these are the same
wire formats as trunking TSBKs (AABC-E) but the ISP/OSP distinction does not strictly apply
in conventional mode.

**Inbound Signaling Packets (ISP) -- SU originated:**

| Alias | Opcode | Service |
|---|---|---|
| EMRG_ALRM_REQ | $27 | Emergency Alarm Request |
| CALL_ALRT_REQ | $1F | Call Alert Request |
| CAN_SRV_REQ | $23 | Cancel Service Request |
| EXT_FNCT_CMD | $24 | Extended Function Command (bidirectional) |
| MSG_UPDT_REQ | $1C | Message Update Request |
| STS_UPDT_REQ | $18 | Status Update Request |
| STS_Q_REQ | $1A | Status Query Request |
| STS_Q_RSP | $19 | Status Query Response (bidirectional) |
| ACK_RSP_FNE | $20 | Acknowledgement Response (bidirectional) |
| RAD_MON_CMD | $1D | Radio Unit Monitor Command (bidirectional) |
| TELE_INT_DIAL_REQ | $15 | Telephone Interconnect Dial Request (explicit) |
| TELE_INT_PSTN_REQ | $16 | Telephone Interconnect PSTN Request (implicit) |
| TELE_INT_ANS_RSP | $17 | Telephone Interconnect Answer Response |

**Outbound Signaling Packets (OSP) -- FNE originated:**

| Alias | Opcode | Service |
|---|---|---|
| ACK_RSP_FNE | $20 | Acknowledgement Response |
| CALL_ALRT | $1F | Call Alert |
| DENY_RSP | $27 | Deny Response |
| QUE_RSP | $29 | Queue Response |
| EXT_FNCT_CMD | $24 | Extended Function Command |
| MSG_UPDT | $1C | Message Update |
| STS_Q | $1A | Status Query |
| STS_Q_RSP | $19 | Status Query Response |
| STS_UPDT | $18 | Status Update |
| RAD_MON_CMD | $1D | Radio Unit Monitor Command |
| TELE_INT_ANS_REQ | $17 | Telephone Interconnect Answer Request |

**Key AABG insight:** Many ISP/OSP pairs are bit-for-bit identical in non-extended (abbreviated)
TSBK format (same opcode, same field layout):
- CALL_ALRT_REQ / CALL_ALRT ($1F)
- STS_Q_REQ / STS_Q ($1A)
- MSG_UPDT_REQ / MSG_UPDT ($1C)
- STS_UPDT_REQ / STS_UPDT ($18)

A conventional receiver must be prepared to process ANY applicable TSBK opcode regardless of
whether it was classified as ISP or OSP.

### 7.2 Supplementary Data Services

#### 7.2.1 Emergency Alarm

See Section 3 above.

#### 7.2.2 Call Alert

Point-to-point alerting to indicate identity to another SU:

```
Source SU/FNE -> CALL_ALRT_REQ/CALL_ALRT -> Target SU
Target SU -> ACK_RSP_FNE -> Source SU/FNE
```

- SU-sourced: CALL_ALRT_REQ with polite retries (NPRetry)
- FNE-sourced: CALL_ALRT with polite retries (NPRetry)
- Target responds with ACK_RSP_FNE per SD ack procedure (Section 6.2.3)
- FNE may transparently repeat both request and ack in repeat mode

#### 7.2.3 Radio Check

Verifies unit presence on the channel:

```
FNE/SU -> EXT_FNCT_CMD (Radio Check) -> Target SU
Target SU -> EXT_FNCT_CMD (Radio Check ACK) -> FNE/SU
```

- Uses EXT_FNCT_CMD with Radio Check extended function value
- Target SU must respond if idle; one-shot response, no retries from target
- If no response after NPRetry attempts, consider SU not present
- SUs shall NOT send Radio Check to the General FNE Address ($FFFFFC)

#### 7.2.4 Radio Inhibit / Uninhibit

**Inhibit** (EXT_FNCT_CMD with Radio Inhibit value):
- Inhibited SU: appears powered off to user, blanks display, ignores all input
- Only processes: EXT_FNCT_CMD responses and OTAR commands
- State persists through power cycling
- Only cleared by Radio Uninhibit or field reprogramming
- SUs should NOT originate Radio Inhibit

**Uninhibit** (EXT_FNCT_CMD with Radio Uninhibit value):
- Cancels inhibit condition, returns to normal operation
- SU acknowledges even if not currently inhibited
- SUs should NOT originate Radio Uninhibit

#### 7.2.5 Message Update

Short coded messages between SU/FNE:
- Target may be UID or GID
- When targeting a group, FNE acknowledges on behalf of the group (prevents simultaneous acks)
- General FNE Address ($FFFFFC) may be used as destination
- SU may display message codes or lookup in a code table

#### 7.2.6 Status Update

User/unit status reporting:
- SU reports status via STS_UPDT_REQ
- FNE acknowledges; may forward status to other FNEs
- FNE may originate STS_UPDT based on inter-FNE input (consoles do NOT originate)
- When targeting a group, FNE acknowledges on behalf of the group

#### 7.2.7 Status Query

Query another SU's current status:
- FNE-sourced: STS_Q -> target responds with STS_Q_RSP
- SU-sourced: STS_Q_REQ -> target responds with STS_Q_RSP
- Consoles cannot be queried for their status

#### 7.2.8 Radio Unit Monitor

Force an SU to transmit for remote listening:
- RAD_MON_CMD includes a Transmit Multiplier value
- Target SU transmits for duration = Transmit Multiplier x programmed transmit time
- If FNE-sourced and group transmission started, uses SU's currently selected group
- If SU-sourced, target starts unit-to-unit transmission to source UID
- Duration = 0 (Transmit Multiplier = 0 or programmed time = 0) means no transmission

#### 7.2.9 Alert Tones

Defined as future work -- not implemented.

---

## 8. State Machines for SU Conventional Operation

### 8.1 Top-Level SU State Machine

```
                    ┌──────────┐
         ┌─────────│  POWER_ON│
         │         └────┬─────┘
         │              │ initialization complete
         │              v
         │         ┌──────────────┐
         │    ┌───>│    IDLE      │<──────────────────────────────┐
         │    │    └──┬───┬───┬──┘                                │
         │    │       │   │   │                                   │
         │    │  PTT  │   │   │ TSBK service request              │
         │    │       │   │   v                                   │
         │    │       │   │ ┌──────────────────┐                  │
         │    │       │   │ │ SD_TRANSACTION   │──── done ────────┤
         │    │       │   │ └──────────────────┘                  │
         │    │       │   │                                       │
         │    │       │   │ incoming voice detected                │
         │    │       │   v                                       │
         │    │       │ ┌──────────────────┐                      │
         │    │       │ │ RECEIVING_VOICE  │──── TDU/TDULC ───────┤
         │    │       │ └──────────────────┘                      │
         │    │       │                                           │
         │    │       v                                           │
         │    │  ┌──────────────────────┐                         │
         │    │  │ CHANNEL_ACCESS_CHECK │                         │
         │    │  └──┬──────────────────┘                         │
         │    │     │                                             │
         │    │     ├── channel busy -> IDLE (inform user)        │
         │    │     │                                             │
         │    │     └── channel idle                              │
         │    │         v                                         │
         │    │    ┌──────────────────┐                           │
         │    │    │ TRANSMITTING_VOICE│──── PTT release ─────────┤
         │    │    └──────────────────┘    (send TDU)             │
         │    │                                                   │
         │    │  ┌──────────────────┐                             │
         │    └──│  INHIBITED       │  (only EXT_FNCT_CMD/OTAR)  │
         │       └──────────────────┘──── uninhibit ──────────────┘
         │
         └── (inhibit persists through power cycle)
```

### 8.2 SU Receiving Voice Sub-State Machine

```
RECEIVING_VOICE:
  entry:
    lock_to_incoming_signal()
    decode NID -> check NAC

  DECODE_HDU:
    extract MI, ALGID, KID, TGID, MFID
    check squelch conditions (Section 2.2)
    if squelch passes: unmute audio, set call_active = true
    -> DECODE_LDU

  DECODE_LDU:
    on LDU1:
      extract LC word (LC_GRP_V_CH_USR / LC_UU_V_CH_USR / LC_TELE_INT_V_CH_USR)
      update call metadata (source_uid, tgid, emergency flag)
      decode 9 IMBE voice frames -> audio
    on LDU2:
      extract encryption sync (MI, ALGID, KID)
      decode 9 IMBE voice frames -> audio
    on Simple TDU or TDULC:
      mute audio
      if TDULC: extract LC_CALL_TRM_CAN, process termination
      set call_active = false
      start hang_timer (if configured)
      -> IDLE
    on signal lost:
      mute audio
      set call_active = false
      -> IDLE
```

### 8.3 SU Transmitting Voice Sub-State Machine

```
TRANSMITTING_VOICE:
  entry:
    ramp_up_transmitter()
    optional: send preamble (up to 30 ms, $5F dotting pattern)

  SEND_HDU:
    construct HDU: MI, ALGID, KID, TGID, MFID
    transmit HDU
    -> SEND_LDU_PAIR

  SEND_LDU_PAIR:
    construct LDU1:
      encode 9 IMBE voice frames from microphone
      embed LC word (LC_GRP_V_CH_USR with current call params)
      embed status symbols = Unknown (repeater mode) or Unknown/Direct (direct mode)
    transmit LDU1

    construct LDU2:
      encode 9 IMBE voice frames
      embed encryption sync (MI, ALGID, KID)
      embed status symbols = Unknown or Unknown/Direct
    transmit LDU2

    if PTT still asserted AND transmit_timer not expired:
      -> SEND_LDU_PAIR (next pair)
    else:
      -> SEND_TDU

  SEND_TDU:
    transmit 1-3 Simple TDU(s)
    ramp_down_transmitter()
    -> IDLE
```

### 8.4 FNE/Repeater State Machine

```
FNE_CHANNEL:
  IDLE:
    transmit outbound with status = Idle
    on inbound Frame Sync + NAC match:
      -> RECEIVING_INBOUND

  RECEIVING_INBOUND:
    set outbound status = Busy (at next slot boundary)
    decode NID, determine data unit type:
      voice: begin repeat to outbound + route to wireline
      TSBK: process supplementary data, generate response if needed
      packet data: route per addressing mode (asymmetric/symmetric)
    on Simple TDU received:
      may convert to TDULC (LC_CALL_TRM_CAN) on outbound
      -> TERMINATING

    on TNoAct timer expired (no decodable data units):
      transmit ETDU/CTC on outbound
      -> IDLE

    on console wireline priority assertion:
      route console audio to outbound (priority)
      continue routing inbound SU audio to wireline only

  TERMINATING:
    transmit termination message(s) on outbound (TDU or TDULC)
    set outbound status = Idle
    -> IDLE
```

---

## 9. Timing Parameters

### 9.1 Complete Timer Reference

| Parameter | Default | Min | Max | Applies To | Description |
|---|---|---|---|---|---|
| TNoAct | 3 s | 0 s | unspecified | FNE | No-activity timeout: FNE tears down channel if no decodable data units for this duration. May differ by call type. |
| TAck | 200 ms | 150 ms | 300 ms | SU | Time to wait for expected acknowledgement before preparing retry |
| TNextAttempt | 200 ms | 150 ms | 300 ms | SU | Time to wait before starting holdoff after polite access blocked |
| THoldoff | random | 50 ms | 1000 ms | SU | Random backoff before TSBK retry. Formula: `50ms * (1 + M % N)` where M=random, N=configurable 1--20 (default 4). Yields values: 50, 100, 150, 200 ms when N=4. |
| NPRetry | 5 | 1 | 15 | SU/FNE | Number of polite retries for supplementary data |
| NIRetry | 0 | 0 | 5 | SU | Number of impolite retries for emergency alarm |
| NMax | 5 | 1 | 20 | SU/FNE | Maximum total supplementary data transmissions |
| NTrafficLC | 3 | 1 | 3 | FNE | FNE must echo source SU's LC at least every NTrafficLC LC messages |
| FSSP | 728 ms | 0 | unspecified | SU | Frame Sync Seek Period for packet data access. Use 938 ms for unconfirmed packets with up to 512 octets. |
| T_ACK | 250 ms | -- | -- | SU | Packet data backoff after BUSY (response packets) |
| T_LONG | 500 ms | -- | -- | SU | Packet data backoff after BUSY (data packets) |
| T_SHORT | 50 ms | -- | -- | SU | Random delay after finding FS in packet data access |

### 9.2 PTT-Related Timing

| Event | Timing | Notes |
|---|---|---|
| Transmitter preamble | up to 30 ms | High dotting pattern $5F during power ramp |
| Emergency preamble | 1--2+ LDU durations (360--720+ ms) | Long preamble for RF capture during impolite access |
| Minimum voice transmission | ~427.5 ms | HDU (67.5) + LDU1 (180) + LDU2 (180) + TDU (~30) -- but LDU pair is not mandatory |
| Hang time | Implementation-specific | Time to retain received TGID for response; not specified by standard |
| PSTN availability check timeout | ~1 s (recommended) | FNE waits for TELE_INT_ANS_RSP |
| Microslot | 7.5 ms | Status symbol boundary per BAAA-B |
| LDU duration | 180 ms | 1728 bits at 9600 bps |
| LDU pair (superframe) | 360 ms | LDU1 + LDU2 |

### 9.3 THoldoff Calculation

```
THoldoff = 50ms * (1 + M % N)

Where:
  M = uniformly distributed random integer
  N = configurable parameter, range 1..20, default 4

With N=4, possible values: {50, 100, 150, 200} ms
With N=20, possible values: {50, 100, 150, ..., 1000} ms (20 values)
```

---

## 10. Parser Pseudocode for Conventional Call Tracking

### 10.1 Conventional Channel Monitor

This pseudocode tracks voice calls and supplementary data on a conventional channel,
suitable for SDR receiver applications (comparable to SDRTrunk's conventional channel
processing or OP25's rx.py conventional mode).

```python
class ConventionalChannelMonitor:
    """
    Tracks P25 conventional channel activity.
    Cross-ref: SDRTrunk P25TrafficChannelManager (conventional mode),
               OP25 rx.py conventional decoder.
    """

    def __init__(self, channel_nac: int, rx_tgids: set[int] | None = None):
        self.channel_nac = channel_nac        # configured NAC ($293 default)
        self.rx_tgids = rx_tgids              # None = monitor mode (all)
        self.current_call: CallState | None = None
        self.emergency_units: set[int] = set()  # UIDs with active emergency
        self.inhibited_units: set[int] = set()
        self.no_activity_timer = Timer(3.0)   # TNoAct default 3s

    def on_frame_sync_detected(self, nid: NID):
        """Called when frame sync + NID decoded from RF."""
        if not self.nac_matches(nid.nac):
            return  # co-channel interference, ignore

        self.no_activity_timer.reset()

        match nid.duid:
            case DUID.HDU:
                self.on_hdu(nid)
            case DUID.LDU1:
                self.on_ldu1(nid)
            case DUID.LDU2:
                self.on_ldu2(nid)
            case DUID.TDU:
                self.on_tdu()
            case DUID.TDULC:
                self.on_tdulc(nid)
            case DUID.TSDU:
                self.on_tsdu(nid)
            case DUID.PDU:
                self.on_pdu(nid)

    def nac_matches(self, received_nac: int) -> bool:
        """NAC qualification per BAAD-B Section 2.5."""
        if self.channel_nac == 0xF7E:
            return True  # open on any NAC
        return received_nac == self.channel_nac

    def on_hdu(self, nid: NID):
        """New voice call starting -- decode HDU."""
        hdu = decode_hdu(nid)
        # HDU fields: MI, MFID, ALGID, KID, TGID
        self.current_call = CallState(
            tgid=hdu.tgid,
            algid=hdu.algid,
            kid=hdu.kid,
            mi=hdu.mi,
            encrypted=(hdu.algid != 0x80),
            start_time=now(),
        )

    def on_ldu1(self, nid: NID):
        """Voice frame with Link Control."""
        ldu1 = decode_ldu1(nid)
        lc = ldu1.link_control

        match lc.opcode:
            case LC_GRP_V_CH_USR:  # $00 -- group voice
                self._update_group_call(lc)
            case LC_UU_V_CH_USR:   # $04 -- unit-to-unit voice
                self._update_uu_call(lc)
            case LC_TELE_INT_V_CH_USR:  # $06 -- PSTN interconnect
                self._update_pstn_call(lc)

        # Decode 9 IMBE voice frames -> audio output
        if self.current_call and self.squelch_passes(self.current_call):
            audio = decode_imbe_frames(ldu1.voice_frames)
            output_audio(audio)

    def on_ldu2(self, nid: NID):
        """Voice frame with Encryption Sync."""
        ldu2 = decode_ldu2(nid)
        if self.current_call:
            self.current_call.mi = ldu2.mi
            self.current_call.algid = ldu2.algid
            self.current_call.kid = ldu2.kid

        if self.current_call and self.squelch_passes(self.current_call):
            audio = decode_imbe_frames(ldu2.voice_frames)
            output_audio(audio)

    def on_tdu(self):
        """Simple Terminator -- call ended."""
        if self.current_call:
            self.current_call.end_time = now()
            emit_call_event(self.current_call, "TERMINATED_TDU")
            self.current_call = None

    def on_tdulc(self, nid: NID):
        """Terminator with Link Control (ETDU/CTC from FNE)."""
        tdulc = decode_tdulc(nid)
        lc = tdulc.link_control
        if lc.opcode == LC_CALL_TRM_CAN:
            target = lc.target_address
            # $FFFFFF = group call termination
            # specific UID = unit call termination
        if self.current_call:
            self.current_call.end_time = now()
            emit_call_event(self.current_call, "TERMINATED_ETDU_CTC")
            self.current_call = None

    def on_tsdu(self, nid: NID):
        """TSBK Signaling Data Unit -- supplementary data."""
        tsdu = decode_tsdu(nid)
        for tsbk in tsdu.tsbk_blocks:
            self._process_tsbk(tsbk)

    def on_pdu(self, nid: NID):
        """Packet Data Unit -- route to packet data handler."""
        pdu = decode_pdu(nid)
        emit_data_event(pdu)

    def on_no_activity_timeout(self):
        """TNoAct expired -- FNE would tear down, monitor should too."""
        if self.current_call:
            self.current_call.end_time = now()
            emit_call_event(self.current_call, "TERMINATED_TIMEOUT")
            self.current_call = None

    # -- Internal helpers --

    def _update_group_call(self, lc):
        if self.current_call is None:
            self.current_call = CallState(
                call_type=CallType.GROUP,
                start_time=now(),
            )
        call = self.current_call
        call.call_type = CallType.GROUP
        call.source_uid = lc.source_address
        call.tgid = lc.group_address
        call.emergency = lc.emergency_bit
        if call.emergency:
            self.emergency_units.add(call.source_uid)

    def _update_uu_call(self, lc):
        if self.current_call is None:
            self.current_call = CallState(
                call_type=CallType.UNIT_TO_UNIT,
                start_time=now(),
            )
        call = self.current_call
        call.call_type = CallType.UNIT_TO_UNIT
        call.source_uid = lc.source_address
        call.destination_uid = lc.destination_address

    def _update_pstn_call(self, lc):
        if self.current_call is None:
            self.current_call = CallState(
                call_type=CallType.PSTN,
                start_time=now(),
            )
        call = self.current_call
        call.call_type = CallType.PSTN
        call.source_uid = lc.source_address

    def _process_tsbk(self, tsbk: TSBK):
        """Process a conventional TSBK per AABG message set."""
        match tsbk.opcode:
            case 0x27:  # EMRG_ALRM_REQ
                self.emergency_units.add(tsbk.source_address)
                emit_event("EMERGENCY_ALARM",
                           source=tsbk.source_address,
                           group=tsbk.group_address)

            case 0x20:  # ACK_RSP_FNE
                emit_event("ACK", source=tsbk.source_address,
                           target=tsbk.target_address,
                           service_type=tsbk.service_type)

            case 0x1F:  # CALL_ALRT_REQ / CALL_ALRT
                emit_event("CALL_ALERT",
                           source=tsbk.source_address,
                           target=tsbk.target_address)

            case 0x24:  # EXT_FNCT_CMD
                self._process_ext_fnct(tsbk)

            case 0x1C:  # MSG_UPDT_REQ / MSG_UPDT
                emit_event("MESSAGE_UPDATE",
                           source=tsbk.source_address,
                           target=tsbk.target_address,
                           message=tsbk.message_value)

            case 0x18:  # STS_UPDT_REQ / STS_UPDT
                emit_event("STATUS_UPDATE",
                           source=tsbk.source_address,
                           target=tsbk.target_address,
                           user_status=tsbk.user_status,
                           unit_status=tsbk.unit_status)

            case 0x1A:  # STS_Q_REQ / STS_Q
                emit_event("STATUS_QUERY",
                           source=tsbk.source_address,
                           target=tsbk.target_address)

            case 0x19:  # STS_Q_RSP
                emit_event("STATUS_QUERY_RESPONSE",
                           source=tsbk.source_address,
                           target=tsbk.target_address,
                           user_status=tsbk.user_status,
                           unit_status=tsbk.unit_status)

            case 0x1D:  # RAD_MON_CMD
                emit_event("RADIO_MONITOR",
                           source=tsbk.source_address,
                           target=tsbk.target_address,
                           tx_multiplier=tsbk.tx_multiplier)

            case 0x23:  # CAN_SRV_REQ
                self._process_cancel_service(tsbk)

            case 0x15:  # TELE_INT_DIAL_REQ
                emit_event("PSTN_DIAL_REQUEST",
                           source=tsbk.source_address,
                           digits=tsbk.digit_string)

            case 0x16:  # TELE_INT_PSTN_REQ
                emit_event("PSTN_IMPLICIT_REQUEST",
                           source=tsbk.source_address,
                           service_code=tsbk.service_code)

    def _process_ext_fnct(self, tsbk: TSBK):
        """Decode extended function subtype."""
        match tsbk.extended_function:
            case ExtFunc.RADIO_CHECK:
                emit_event("RADIO_CHECK",
                           source=tsbk.source_address,
                           target=tsbk.target_address)
            case ExtFunc.RADIO_CHECK_ACK:
                emit_event("RADIO_CHECK_ACK",
                           source=tsbk.source_address,
                           target=tsbk.target_address)
            case ExtFunc.RADIO_INHIBIT:
                self.inhibited_units.add(tsbk.target_address)
                emit_event("RADIO_INHIBIT",
                           target=tsbk.target_address)
            case ExtFunc.RADIO_INHIBIT_ACK:
                emit_event("RADIO_INHIBIT_ACK",
                           source=tsbk.source_address)
            case ExtFunc.RADIO_UNINHIBIT:
                self.inhibited_units.discard(tsbk.target_address)
                emit_event("RADIO_UNINHIBIT",
                           target=tsbk.target_address)
            case ExtFunc.RADIO_UNINHIBIT_ACK:
                emit_event("RADIO_UNINHIBIT_ACK",
                           source=tsbk.source_address)

    def _process_cancel_service(self, tsbk: TSBK):
        """Handle CAN_SRV_REQ -- may cancel emergency or PSTN."""
        if tsbk.aiv and tsbk.service_type == 0x27:
            # Emergency alarm cancellation
            self.emergency_units.discard(tsbk.source_address)
            emit_event("EMERGENCY_CANCEL",
                       source=tsbk.source_address,
                       group=tsbk.additional_info_group)
        else:
            emit_event("CANCEL_SERVICE",
                       source=tsbk.source_address,
                       service_type=tsbk.service_type)

    def squelch_passes(self, call: CallState) -> bool:
        """Apply configured squelch mode."""
        if self.rx_tgids is None:
            return True  # monitor mode
        if call.call_type == CallType.GROUP:
            return call.tgid in self.rx_tgids or call.tgid == 0xFFFF
        if call.call_type == CallType.UNIT_TO_UNIT:
            return True  # selective squelch handled elsewhere
        return True
```

### 10.2 Status Symbol Decoder (for Channel Busy Detection)

```python
class StatusSymbolDecoder:
    """
    Decodes status symbols from the FDMA bitstream to determine
    inbound channel state. Used by SUs for polite channel access.
    Cross-ref: BAAA-B status symbol positions in LDU/TDU frames.
    """

    BUSY    = 0b01   # Repeater asserts: inbound has activity
    IDLE    = 0b10   # Repeater asserts: inbound is idle
    UNKNOWN = 0b00   # SU asserts (repeater mode)
    UNKNOWN_DIRECT = 0b11  # SU asserts (direct mode)

    def __init__(self, slot_size_microslots: int = 1):
        self.slot_size = slot_size_microslots
        self.microslot_count = 0
        self.last_slot_boundary_status = None
        self.source_is_repeater = False

    def on_status_symbol(self, ss: int):
        """Called every microslot (7.5 ms)."""
        self.microslot_count += 1

        if self.microslot_count % self.slot_size == 0:
            # Slot boundary -- this status symbol is authoritative
            self.last_slot_boundary_status = ss
            if ss == self.BUSY or ss == self.IDLE:
                self.source_is_repeater = True  # only repeaters assert Busy/Idle

    def channel_is_idle(self) -> bool:
        """Returns True if the channel appears idle for polite access."""
        return self.last_slot_boundary_status == self.IDLE

    def channel_is_busy(self) -> bool:
        return self.last_slot_boundary_status == self.BUSY

    def can_use_for_afc(self) -> bool:
        """AFC lock is possible only when receiving from a repeater."""
        return self.source_is_repeater
```

---

## 11. Rust Struct Definitions

### 11.1 Core Types

```rust
use std::time::Instant;

/// Network Access Code (12-bit).
/// Cross-ref: BAAC-D reserved values, BAAA-B NID field.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Nac(pub u16);

impl Nac {
    pub const DEFAULT: Nac = Nac(0x293);
    pub const OPEN_ON_ANY: Nac = Nac(0xF7E);
    pub const REPEATER_PASSTHROUGH: Nac = Nac(0xF7F);

    /// Returns true if `received` NAC matches this configured NAC.
    pub fn matches(&self, received: Nac) -> bool {
        if *self == Self::OPEN_ON_ANY {
            return true;
        }
        *self == received
    }

    /// SUs must never transmit these reserved NAC values.
    pub fn is_valid_for_su_tx(&self) -> bool {
        *self != Self::OPEN_ON_ANY && *self != Self::REPEATER_PASSTHROUGH
    }
}

/// 24-bit Unit ID.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct UnitId(pub u32);

impl UnitId {
    pub const GENERAL_FNE: UnitId = UnitId(0xFFFFFC);
    pub const ALL_UNITS: UnitId = UnitId(0xFFFFFF);

    /// Valid SU UID range: 1 to 9,999,999.
    pub fn is_valid_su(&self) -> bool {
        self.0 >= 1 && self.0 <= 9_999_999
    }
}

/// 16-bit Talk Group ID.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct TalkGroupId(pub u16);

impl TalkGroupId {
    pub const NULL: TalkGroupId = TalkGroupId(0x0000);
    pub const ALL_CALL: TalkGroupId = TalkGroupId(0xFFFF);
    pub const DEFAULT: TalkGroupId = TalkGroupId(0x0001);

    /// Map 16-bit TGID to 24-bit address space per BAAD-B Section 5.1.
    /// $FEF001 = TGID $0001, $FFEFFF = TGID $FFFF.
    pub fn to_24bit_address(&self) -> u32 {
        0xFEF000u32 + self.0 as u32
    }

    pub fn from_24bit_address(addr: u32) -> Option<TalkGroupId> {
        if addr >= 0xFEF001 && addr <= 0xFFEFFF {
            Some(TalkGroupId((addr - 0xFEF000) as u16))
        } else {
            None
        }
    }
}

/// Voice call types on conventional channels.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ConventionalCallType {
    /// Group voice call (LC_GRP_V_CH_USR, AABF-D opcode $00).
    GroupVoice,
    /// Unit-to-unit voice call (LC_UU_V_CH_USR, AABF-D opcode $04).
    UnitToUnit,
    /// PSTN interconnect call (LC_TELE_INT_V_CH_USR, AABF-D opcode $06).
    PstnInterconnect,
    /// All Call -- group voice with TGID $FFFF.
    AllCall,
}

/// Receiver squelch mode.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SquelchMode {
    /// Unmute on any recognizable voice signal.
    Monitor,
    /// Unmute on correct NAC match.
    NormalSquelch,
    /// Unmute on correct NAC + matching TGID/UID.
    SelectiveSquelch,
}

/// Status symbols as transmitted in FDMA data units.
/// Cross-ref: BAAA-B, BAAD-B Section 2.6.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum StatusSymbol {
    /// SU transmitting inbound to repeater.
    Unknown = 0b00,
    /// Repeater: inbound channel has activity.
    Busy = 0b01,
    /// Repeater: inbound channel is idle.
    Idle = 0b10,
    /// SU transmitting in direct (talk-around) mode.
    UnknownDirect = 0b11,
}

impl StatusSymbol {
    /// Only repeaters/FNE transmit Busy or Idle.
    pub fn is_from_repeater(&self) -> bool {
        matches!(self, StatusSymbol::Busy | StatusSymbol::Idle)
    }
}
```

### 11.2 Call State

```rust
/// Active conventional voice call state.
/// One instance per monitored conventional channel.
#[derive(Debug, Clone)]
pub struct ConventionalCallState {
    /// Type of call determined from Link Control opcode.
    pub call_type: ConventionalCallType,

    /// Source unit ID (24-bit) from LC word.
    pub source_uid: UnitId,

    /// Talk group ID (16-bit) from HDU or LC word.
    /// NULL ($0000) for unit-to-unit calls.
    pub tgid: TalkGroupId,

    /// Destination UID for unit-to-unit or PSTN calls.
    pub destination_uid: Option<UnitId>,

    /// Emergency flag from Service Options bit 7 in LC_GRP_V_CH_USR.
    /// Always false for unit-to-unit (no emergency U2U in conventional).
    pub emergency: bool,

    /// Encryption state from HDU / LDU2 encryption sync.
    pub encryption: EncryptionState,

    /// When the call started (first HDU or LDU1 detected).
    pub start_time: Instant,

    /// When the call ended (TDU/TDULC received), None if still active.
    pub end_time: Option<Instant>,

    /// Count of LDU pairs received (for duration tracking).
    pub ldu_pair_count: u32,

    /// NAC observed on this call.
    pub nac: Nac,
}

/// Encryption parameters extracted from HDU and LDU2.
#[derive(Debug, Clone, Copy)]
pub struct EncryptionState {
    /// Algorithm ID ($80 = unencrypted).
    pub algid: u8,
    /// Key ID.
    pub kid: u16,
    /// Message Indicator (72-bit nonce).
    pub mi: [u8; 9],
}

impl EncryptionState {
    pub const CLEAR: EncryptionState = EncryptionState {
        algid: 0x80,
        kid: 0x0000,
        mi: [0; 9],
    };

    pub fn is_encrypted(&self) -> bool {
        self.algid != 0x80
    }
}
```

### 11.3 Channel Access State Machines

```rust
/// TSBK channel access state for non-emergency supplementary data.
/// Cross-ref: BAAD-B Section 4.2.1, Figure 9.
#[derive(Debug)]
pub struct TsbkNonEmergAccessState {
    pub state: TsbkAccessPhase,
    pub polite_retries_remaining: u8,
    pub tsbk_payload: Vec<u8>,
    pub timer_deadline: Option<Instant>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TsbkAccessPhase {
    /// Check channel busy state.
    CheckIfSlotIdle,
    /// Waiting for ACK or timer expiry.
    WaitingForAckOrNextAttempt,
    /// Random holdoff before retry.
    WaitingForHoldoff,
    /// Transaction complete.
    Done,
}

/// TSBK channel access state for emergency alarm.
/// Cross-ref: BAAD-B Section 4.2.2, Figure 10.
#[derive(Debug)]
pub struct TsbkEmergAccessState {
    pub state: EmergAccessPhase,
    pub impolite_retries_remaining: u8,
    pub polite_retries_remaining: u8,
    pub tsbk_payload: Vec<u8>,
    pub timer_deadline: Option<Instant>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EmergAccessPhase {
    /// Waiting for ACK after transmission.
    WaitingForAck,
    /// Impolite holdoff (short, fixed).
    WaitingImpoliteHoldoff,
    /// Polite holdoff (random THoldoff).
    WaitingPoliteHoldoff,
    /// Check channel idle for polite attempt.
    CheckIfSlotIdle,
    /// Transaction complete.
    Done,
}

/// Packet data channel access state machine (six states).
/// Cross-ref: BAAD-B Section 7.1, Figure 12.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PacketDataAccessState {
    /// Default state, monitoring channel.
    ReceiveIdle,
    /// Receiving a valid frame sync + NAC matched transmission.
    ReceiveFsAndNac,
    /// Trying to locate frame sync on outbound channel.
    FsAndNacSeek,
    /// Random delay [0, T_SHORT] to reduce collision.
    WaitRandomTShort,
    /// Waiting for microslot boundary to check status.
    WaitForEndOfMicroSlot,
    /// Random backoff after BUSY detection.
    RandomBackoffDelay,
    /// Transmitting the packet.
    TransmitPacket,
}
```

### 11.4 Conventional Channel Configuration

```rust
/// Complete configuration for a conventional channel.
#[derive(Debug, Clone)]
pub struct ConventionalChannelConfig {
    /// Receive frequency in Hz.
    pub rx_freq_hz: u64,
    /// Transmit frequency in Hz (same as RX for direct mode).
    pub tx_freq_hz: u64,
    /// Configured receive NAC.
    pub rx_nac: Nac,
    /// Configured transmit NAC (SU must not use $F7E or $F7F).
    pub tx_nac: Nac,
    /// Selected talk group for transmit.
    pub tx_tgid: TalkGroupId,
    /// Talk groups to receive (selective squelch). Empty = monitor all.
    pub rx_tgids: Vec<TalkGroupId>,
    /// Squelch mode.
    pub squelch_mode: SquelchMode,
    /// Channel mode.
    pub mode: ConventionalMode,
    /// Encryption configuration.
    pub encryption: Option<EncryptionState>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ConventionalMode {
    /// Direct / talk-around (simplex, SU to SU).
    Direct,
    /// Through repeater (half-duplex SU, full-duplex FS).
    Repeated,
}
```

### 11.5 Timing Configuration

```rust
use std::time::Duration;

/// All configurable timing parameters from BAAD-B Section 10.
#[derive(Debug, Clone)]
pub struct ConventionalTimingConfig {
    /// TNoAct: FNE no-activity timeout. Default 3s.
    pub t_no_act: Duration,
    /// TAck: SU wait for expected ack. Default 200ms.
    pub t_ack: Duration,
    /// TNextAttempt: SU wait after polite block. Default 200ms.
    pub t_next_attempt: Duration,
    /// THoldoff N parameter for random backoff. Default 4, range 1..=20.
    pub t_holdoff_n: u8,
    /// NPRetry: polite retries. Default 5, range 1..=15.
    pub np_retry: u8,
    /// NIRetry: impolite retries. Default 0, range 0..=5.
    pub ni_retry: u8,
    /// NMax: max supplementary data transmissions. Default 5, range 1..=20.
    pub n_max: u8,
    /// NTrafficLC: FNE must echo source LC every N LC slots. Default 3, range 1..=3.
    pub n_traffic_lc: u8,
    /// FSSP: Frame Sync Seek Period for packet data. Default 728ms.
    pub fssp: Duration,
    /// T_ACK: packet data backoff for responses. 250ms.
    pub t_ack_data: Duration,
    /// T_LONG: packet data backoff for data packets. 500ms.
    pub t_long: Duration,
    /// T_SHORT: packet data random delay. 50ms.
    pub t_short: Duration,
}

impl Default for ConventionalTimingConfig {
    fn default() -> Self {
        Self {
            t_no_act: Duration::from_secs(3),
            t_ack: Duration::from_millis(200),
            t_next_attempt: Duration::from_millis(200),
            t_holdoff_n: 4,
            np_retry: 5,
            ni_retry: 0,
            n_max: 5,
            n_traffic_lc: 3,
            fssp: Duration::from_millis(728),
            t_ack_data: Duration::from_millis(250),
            t_long: Duration::from_millis(500),
            t_short: Duration::from_millis(50),
        }
    }
}

impl ConventionalTimingConfig {
    /// Calculate random THoldoff per BAAD-B formula: 50ms * (1 + M % N).
    pub fn random_t_holdoff(&self) -> Duration {
        let m: u32 = rand::random();
        let n = self.t_holdoff_n as u32;
        let ticks = 1 + (m % n);
        Duration::from_millis(50 * ticks as u64)
    }
}
```

### 11.6 Supplementary Data Event Types

```rust
/// Events emitted by the conventional channel monitor when processing TSBKs.
/// Cross-ref: AABG message inventory.
#[derive(Debug, Clone)]
pub enum ConventionalEvent {
    /// Voice call started.
    VoiceCallStart {
        call_type: ConventionalCallType,
        source: UnitId,
        tgid: TalkGroupId,
        destination: Option<UnitId>,
        emergency: bool,
        nac: Nac,
    },
    /// Voice call ended.
    VoiceCallEnd {
        call_type: ConventionalCallType,
        source: UnitId,
        tgid: TalkGroupId,
        duration_ms: u64,
        termination: TerminationReason,
    },
    /// Emergency alarm received (EMRG_ALRM_REQ, opcode $27).
    EmergencyAlarm {
        source: UnitId,
        group: TalkGroupId,
    },
    /// Emergency alarm cancelled (CAN_SRV_REQ with service_type=$27).
    EmergencyCancel {
        source: UnitId,
        group: TalkGroupId,
    },
    /// Call alert (CALL_ALRT_REQ/CALL_ALRT, opcode $1F).
    CallAlert {
        source: UnitId,
        target: UnitId,
    },
    /// Acknowledgement (ACK_RSP_FNE, opcode $20).
    Acknowledgement {
        source: UnitId,
        target: UnitId,
        service_type: u8,
    },
    /// Radio check (EXT_FNCT_CMD, Radio Check subtype).
    RadioCheck {
        source: UnitId,
        target: UnitId,
        is_ack: bool,
    },
    /// Radio inhibit/uninhibit (EXT_FNCT_CMD subtypes).
    RadioInhibit {
        target: UnitId,
        inhibit: bool, // true = inhibit, false = uninhibit
        is_ack: bool,
    },
    /// Message update (MSG_UPDT_REQ/MSG_UPDT, opcode $1C).
    MessageUpdate {
        source: UnitId,
        target: UnitId, // may be UID or mapped 24-bit TGID
        message: u16,
    },
    /// Status update (STS_UPDT_REQ/STS_UPDT, opcode $18).
    StatusUpdate {
        source: UnitId,
        target: UnitId,
        user_status: u8,
        unit_status: u8,
    },
    /// Status query (STS_Q_REQ/STS_Q, opcode $1A).
    StatusQuery {
        source: UnitId,
        target: UnitId,
    },
    /// Status query response (STS_Q_RSP, opcode $19).
    StatusQueryResponse {
        source: UnitId,
        target: UnitId,
        user_status: u8,
        unit_status: u8,
    },
    /// Radio unit monitor (RAD_MON_CMD, opcode $1D).
    RadioMonitor {
        source: UnitId,
        target: UnitId,
        tx_multiplier: u8,
    },
    /// PSTN interconnect request.
    PstnRequest {
        source: UnitId,
        explicit_dial: bool,
    },
    /// Packet data received.
    PacketData {
        source: UnitId,
        destination: UnitId,
    },
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TerminationReason {
    /// Simple TDU received from transmitting SU.
    SimpleTdu,
    /// ETDU/CTC (TDULC with LC_CALL_TRM_CAN) from FNE.
    ExpandedTduCtc,
    /// TNoAct timeout (no activity).
    NoActivityTimeout,
    /// Signal lost (no frame sync).
    SignalLost,
}
```

---

## Appendix A: SDRTrunk / OP25 Conventional Mode Cross-Reference

### SDRTrunk

SDRTrunk handles conventional P25 channels through its `P25TrafficChannelManager` and
`P25DecoderState` classes. In conventional mode:
- No control channel is tracked -- the decoder processes voice and TSBK on a single frequency
- Call start is detected from HDU or first LDU1 with valid LC
- Call end is detected from TDU, TDULC, or timeout
- TSBK messages are decoded using the same `TSBKMessage` parser as trunking
- Status symbols are not heavily used (SDRTrunk is receive-only, does not need polite access)

### OP25

OP25's `rx.py` conventional mode:
- Demodulates C4FM on a single frequency
- Decodes NID to identify DUID (data unit ID)
- Voice frames decoded through IMBE vocoder
- TSBK blocks decoded using same format as trunking messages
- Supports NAC filtering
- No channel access logic needed (receive-only)

### Implementation Notes for Receiver-Only Applications

For SDR receiver applications (no transmit capability), the following sections of BAAD-B
can be simplified or omitted:

| Section | Transmit-Side | Receive-Side |
|---|---|---|
| Channel access (Sec 4, 7.1) | Full state machine needed | Not needed -- always receiving |
| Status symbols (Sec 2.6) | Must embed Unknown/Unknown-Direct | Decode for AFC lock detection |
| Voice TX sequencing (Sec 6) | HDU->LDU->TDU generation | HDU->LDU->TDU parsing |
| TSBK TX retries (Sec 4.2) | Full retry state machine | Not needed |
| TNoAct (Sec 2.2) | FNE must implement | Monitor may implement for call timeout |
| NAC substitution (Sec 2.5) | Repeater swaps NAC | Check match only |
| Encryption (Sec 9) | Encrypt outbound | Decrypt inbound (if keys available) |

---

## Appendix B: Conventional vs. Trunking Procedure Mapping

For implementers who already have a trunking stack (per AABD-B), here is the mapping:

| Trunking Concept | Conventional Equivalent |
|---|---|
| Control channel | Does not exist -- TSBK on voice channel |
| GRP_V_CH_GRANT (channel assignment) | Does not exist -- SU is pre-configured |
| Slotted Aloha inbound access | Carrier-sense polite access |
| Registration (U_REG_REQ) | Not defined (placeholder in BAAD-B) |
| Group affiliation (GRP_AFF_REQ) | Not defined -- TGID pre-programmed |
| Channel grant queuing | Does not exist |
| Site roaming / adjacent site | Multi-site via NAC selection |
| RFSS/WACN addressing | Not applicable (single-channel addressing) |
| Authentication (LLA) | Not applicable to conventional per BAAD-B |
| TDMA traffic channels | Not applicable -- FDMA only for conventional |

---

*End of implementation specification.*
