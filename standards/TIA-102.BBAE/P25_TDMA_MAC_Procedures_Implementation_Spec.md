# P25 Phase 2 TDMA MAC Layer Procedures — Implementation Specification

**Source:** TIA-102.BBAE, November 2019 (full document)  
**Classification:** PROTOCOL  
**Extracted:** 2026-04-12  
**Purpose:** Self-contained Phase 3 implementation spec for P25 TDMA MAC layer procedures
covering call lifecycle state machines, PDU sequencing, SACCH/FACCH scheduling,
preemption, and error handling. No reference to the original PDF required except where
noted as gaps.

---

## Table of Contents

1. [Overview and Scope](#1-overview-and-scope)
2. [Timing Constants and Parameters](#2-timing-constants-and-parameters)
3. [Framing Structure Reference](#3-framing-structure-reference)
4. [SU Voice Call State Machine](#4-su-voice-call-state-machine)
5. [FNE Voice Call State Machine](#5-fne-voice-call-state-machine)
6. [PDU Sequencing Requirements](#6-pdu-sequencing-requirements)
7. [SACCH/FACCH Scheduling](#7-sacchfacch-scheduling)
8. [Voice Channel Procedures Detail](#8-voice-channel-procedures-detail)
9. [Control Channel Procedures](#9-control-channel-procedures)
10. [SU Decision Trees](#10-su-decision-trees)
11. [Preemption Procedures](#11-preemption-procedures)
12. [Error Handling and Timeouts](#12-error-handling-and-timeouts)
13. [Scrambling Coordination](#13-scrambling-coordination)
14. [Implementation Notes and Open-Source Cross-References](#14-implementation-notes-and-open-source-cross-references)
15. [Gaps and Missing Information](#15-gaps-and-missing-information)

---

## 1. Overview and Scope

TIA-102.BBAE defines MAC layer procedures for the P25 Phase 2 two-slot TDMA air
interface (Um2). It covers three logical channel types:

- **LCCH** (Logical Control Channel) — trunking signaling
- **VCH** (Voice Channel) — voice call traffic
- **DCH** (Data Channel) — defined in architecture but procedures NOT specified

This spec covers the behavioral rules for SU (Subscriber Unit) and FNE (Fixed Network
Equipment) operating on VCH and LCCH logical channels. DCH is out of scope (undefined
in the source standard).

### 1.1 Key MAC PDU Types

| PDU Type | Direction | Usage |
|----------|-----------|-------|
| MAC_PTT | Inbound + Outbound | Call setup, crypto sync, talker identification |
| MAC_END_PTT | Inbound + Outbound | Call/transmission termination |
| MAC_ACTIVE | Outbound (primarily) | Traffic mode signaling, VCU messages |
| MAC_HANGTIME | Outbound | Message-trunked hangtime signaling |
| MAC_IDLE | Outbound | Unassigned channel filler |
| MAC_Release | Outbound (SACCH) | Preemption signaling (C/A and U/F fields) |

### 1.2 Normative Dependencies

| Document | Content |
|----------|---------|
| TIA-102.BBAC-A [4] | MAC layer specification (burst formats, framing, ultraframe) |
| TIA-102.BBAD-A [5] | MAC layer messages (PDU encodings) |
| TIA-102.AABD-B [2] | Trunking procedures (FDMA CCH procedures also apply unless overridden) |
| TIA-102.AABC-E [1] | Trunking control channel messages |
| TIA-102.AABH [3] | Dynamic regrouping |

---

## 2. Timing Constants and Parameters

### 2.1 Fundamental Timing

| Parameter | Value | Notes |
|-----------|-------|-------|
| Timeslot duration | 30 ms | Each of 12 slots per superframe |
| Superframe duration | 360 ms | 12 timeslots (0-11) |
| Ultraframe duration | 4 superframes | 1440 ms = 4 x 360 ms |
| Scramble sequence length | 4320 bits | Covers one superframe |
| Voice burst sequence | 4V-4V-4V-4V-2V | Per superframe, per [4] |

### 2.2 CCH Access Parameters

| Parameter | Value | Notes |
|-----------|-------|-------|
| T_LCCH (dual LCCH) | 30 ms | 30 ms x N_slot where N_slot = 1 |
| T_LCCH (single LCCH) | 60 ms | 30 ms x N_slot where N_slot = 2 |
| T_ctrl (default) | 240 ms | 30 ms x 8 |
| T_ctrl (minimum) | 120 ms | 30 ms x 4 |
| T_retry range | [0, N_random x T_LCCH] | Uniform random draw; N_random per [2] |

### 2.3 VCH Timing Constraints

| Parameter | Value | Notes |
|-----------|-------|-------|
| MAC_PTT minimum count | 2 | On FACCH; may add 1 on adjacent SACCH |
| MAC_END_PTT minimum count (inbound) | 2 | On FACCH; up to 3 allowed; may add 1 on adjacent SACCH |
| MAC_END_PTT minimum count (outbound) | 2 | On FACCH; total count is local policy |
| VCU message minimum during hangtime | 1 per superframe | At least once per 360 ms |
| Preemption response deadline (VCH method) | 1 ultraframe | FNE must respond within ~1440 ms |
| Talker right-to-talk timeout | 2 ultraframes minimum | SU waits at least 2 ultraframes for confirmation |

### 2.4 Synchronization Timing

| Parameter | Value | Notes |
|-----------|-------|-------|
| T_SYNC_BCST | System-defined | Sync Broadcast interval on FDMA CCH |
| Transmit sync maintenance slots | Slots 10/11 | SU monitors S-ISCH in these SACCH slots |

---

## 3. Framing Structure Reference

### 3.1 Superframe Structure (per BBAC)

```
Slot:  0   1   2   3   4   5   6   7   8   9   10  11
       |---|---|---|---|---|---|---|---|---|---|---|---|
       
LCH 0: slots 0, 2, 4, 6, 8, 10  (even)
LCH 1: slots 1, 3, 5, 7, 9, 11  (odd)

ISCH boundaries occur at superframe edges.
SACCH occurs once per superframe per LCH (position per [4]).
Remaining LCH slots carry FACCH (during signaling) or TCH voice bursts (during traffic).
```

### 3.2 Voice Burst Sequence Within Superframe

```
Per LCH per superframe: 4V, 4V, 4V, 4V, 2V
(Five voice bursts; the last is shorter)
```

### 3.3 Ultraframe SACCH Schedule

The ultraframe comprises 4 superframes. The SACCH ultraframe schedule (defined in [4])
designates specific SACCH positions for:

- **Talker SACCH** — last superframe of the ultraframe; used by transmitting SU
- **Listener SACCH** — other superframe positions; available per FR field in I-ISCH

```
Ultraframe SACCH allocation (conceptual):
  Superframe 0: Listener SACCH (FNE -> listeners)
  Superframe 1: Listener SACCH (FNE -> listeners)  
  Superframe 2: Listener SACCH (FNE -> listeners)
  Superframe 3: Talker SACCH   (FNE -> talker; Inbound: talker -> FNE)
```

> **GAP:** The exact ultraframe SACCH slot mapping is defined in TIA-102.BBAC-A [4],
> not in BBAE. The above is the behavioral model described by BBAE. Implementers must
> consult BBAC-A for precise slot assignments.

### 3.4 LCH Transition Constraints

LCH transitions (LCCH <-> TCH) may only occur at specific timeslot positions:

| LCH | Allowed transition timeslots |
|-----|------------------------------|
| LCH 0 | 0, 4, 8 |
| LCH 1 | 1, 5, 9 |

Signaled via I-ISCH LCH Type field change.

---

## 4. SU Voice Call State Machine

### 4.1 Talker SU State Machine

| State | Event | Action | Next State |
|-------|-------|--------|------------|
| **IDLE** | Receive VCH grant from CCH | Tune to assigned channel/slot; begin sync acquisition | SYNC_ACQUIRE |
| **SYNC_ACQUIRE** | Symbol sync + superframe + ultraframe alignment achieved | Ready first inbound burst | CALL_SETUP |
| **SYNC_ACQUIRE** | Sync timeout (local policy) | Return to CCH; notify user of failure | IDLE |
| **CALL_SETUP** | Ready to transmit | Send 2x MAC_PTT on inbound FACCH (+ optional SACCH MAC_PTT if adjacent) | WAIT_CONFIRM |
| **WAIT_CONFIRM** | Outbound SACCH with matching WUID/SUID or zero WUID/SUID received | Begin voice burst transmission | TRAFFIC |
| **WAIT_CONFIRM** | Outbound SACCH with non-zero WUID/SUID that does NOT match | Lost right to talk; send MAC_END_PTTs; notify user | LISTENING |
| **WAIT_CONFIRM** | No talker SACCH decoded within 2 ultraframes | Finish current voice burst; send MAC_END_PTTs; notify user | LISTENING |
| **TRAFFIC** | Ongoing voice; outbound talker SACCH with matching WUID/SUID or zero | Continue transmitting | TRAFFIC |
| **TRAFFIC** | Outbound talker SACCH with non-zero mismatched WUID/SUID | Stop transmitting; send MAC_END_PTTs | LISTENING |
| **TRAFFIC** | User releases PTT | Send last voice burst; send >= 2 MAC_END_PTT on inbound FACCH (up to 3); optional SACCH MAC_END_PTT if adjacent | TX_COMPLETE |
| **TRAFFIC** | Receive MAC_Release (U/F=1, C/A=1) [Forced audio preemption] | End voice; send MAC_END_PTTs; begin listening | LISTENING |
| **TRAFFIC** | Receive MAC_Release (U/F=0, C/A=1) [Unforced audio preemption] | May continue or may stop (local policy) | TRAFFIC or LISTENING |
| **TRAFFIC** | Receive MAC_Release (U/F=1, C/A=0) [Call preemption] | Send 2x MAC_END_PTT on inbound FACCH; stop transmitting; leave channel | IDLE |
| **TX_COMPLETE** | MAC_END_PTTs sent | Remain on channel as listener | LISTENING |
| **LISTENING** | Receive outbound MAC_HANGTIME PDUs | Remain on channel; verify call identity per VCU messages | HANGTIME |
| **LISTENING** | Receive outbound MAC_END_PTT | Return to CCH idle | IDLE |
| **LISTENING** | VCU address/ID mismatch (wrong group/unit) | Return to CCH | IDLE |
| **LISTENING** | Sync loss (persistent CRC/decode failures) | Return to CCH to re-verify system | IDLE |
| **HANGTIME** | User presses PTT (VCH continuation method) | Send 2x MAC_PTT on inbound FACCH | CALL_SETUP |
| **HANGTIME** | User presses PTT (CCH continuation method) | Return to CCH; issue voice service request | CCH_REQUEST |
| **HANGTIME** | Receive outbound MAC_END_PTT | Return to CCH idle | IDLE |
| **HANGTIME** | Receive outbound MAC_PTT (new talker) | Transition to listening for new talker | LISTENING |
| **CCH_REQUEST** | Receive channel grant | Tune to channel; begin CALL_SETUP | SYNC_ACQUIRE |
| **CCH_REQUEST** | Receive DENY_RSP | Return to idle; may be reassigned as listener | IDLE |
| **CCH_REQUEST** | Receive QUE_RSP | Wait for further signaling | CCH_REQUEST |

### 4.2 Listener SU State Machine

| State | Event | Action | Next State |
|-------|-------|--------|------------|
| **IDLE** | Receive VCH grant / channel assignment | Tune to assigned channel/slot; begin sync | SYNC_ACQUIRE |
| **SYNC_ACQUIRE** | Symbol sync + framing achieved | Monitor outbound FACCH and SACCH | CALL_SETUP_RX |
| **CALL_SETUP_RX** | Receive outbound MAC_PTT | Verify group/unit IDs | LISTENING |
| **CALL_SETUP_RX** | Receive outbound MAC_ACTIVE with VCU | Verify group/unit IDs | LISTENING |
| **CALL_SETUP_RX** | Group/unit ID mismatch (non-zero) | Return to CCH | IDLE |
| **LISTENING** | Outbound voice bursts present | Decode and play audio | LISTENING |
| **LISTENING** | Receive MAC_END_PTT | Return to CCH idle | IDLE |
| **LISTENING** | Outbound transitions to MAC_HANGTIME | Enter hangtime state | HANGTIME |
| **LISTENING** | VCU address/ID mismatch detected | Return to CCH | IDLE |
| **LISTENING** | Receive high-priority individual page | Return to CCH for individual messaging | IDLE |
| **LISTENING** | Receive low-priority individual page | May act or ignore (user/local policy) | LISTENING or IDLE |
| **HANGTIME** | User presses PTT (VCH method) | Send MAC_PTTs on inbound FACCH | TALKER:CALL_SETUP |
| **HANGTIME** | User presses PTT (CCH method) | Return to CCH; issue voice service request | CCH_REQUEST |
| **HANGTIME** | Receive MAC_END_PTT | Return to CCH idle | IDLE |
| **HANGTIME** | New talker detected (MAC_PTT outbound) | Transition to listening | LISTENING |

---

## 5. FNE Voice Call State Machine

### 5.1 FNE Per-Slot State Machine

| State | Event | Action | Next State |
|-------|-------|--------|------------|
| **UNASSIGNED** | Channel grant issued for this slot | Begin sending MAC_ACTIVE on outbound FACCH + SACCH with VCU message; set alternate slot to MAC_IDLE if unassigned | CALL_SETUP |
| **CALL_SETUP** | Receive inbound MAC_PTT (1 or more) | Sync to inbound; generate matching outbound MAC_PTT(s) on FACCH (1 per inbound FACCH MAC_PTT); optionally add SACCH MAC_PTT if adjacent | TRAFFIC |
| **CALL_SETUP** | Call setup timeout (local policy) | Determine call failed; release VCH | UNASSIGNED |
| **TRAFFIC** | Inbound voice bursts arriving | Demodulate, decode, re-encode, transmit on outbound (staggered timing) | TRAFFIC |
| **TRAFFIC** | Receive inbound MAC_END_PTT | Finish buffered audio; begin termination | TERMINATING |
| **TRAFFIC** | FNE audio source ends | Finish buffered audio; begin termination | TERMINATING |
| **TRAFFIC** | Audio preemption decision | Send MAC_Release on outbound talker SACCH; route new audio; send MAC_PTTs for new source | TRAFFIC (new talker) |
| **TRAFFIC** | Call preemption decision (polite) | Send MAC_END_PTTs on FACCH; send MAC_Release (C/A=0, U/F=1) on talker SACCH; grant new call | CALL_PREEMPT |
| **TRAFFIC** | Call preemption decision (impolite) | Grant new call immediately; send MAC_ACTIVE with new VCU | CALL_SETUP (new call) |
| **TERMINATING** | Transmission trunked mode | Send >= 2 MAC_END_PTT on outbound FACCH (total per local policy); optional SACCH if adjacent | UNASSIGNED |
| **TERMINATING** | Message trunked mode | Send MAC_HANGTIME PDUs on outbound FACCH + SACCH; start hang timer | HANGTIME |
| **HANGTIME** | Receive inbound MAC_PTT (VCH continuation) | Verify caller is authorized participant; select audio source; send outbound MAC_PTTs | TRAFFIC |
| **HANGTIME** | Receive CCH service request for same call | Grant per CCH procedures; send outbound MAC_PTTs when talker arrives | CALL_SETUP |
| **HANGTIME** | Hang timer expires / max MAC_HANGTIME PDUs sent | Terminate call per transmission-trunked procedure | TERMINATING |
| **HANGTIME** | Multiple continuation requests | Select one per local policy (first-come, priority, etc.) | TRAFFIC |
| **CALL_PREEMPT** | MAC_END_PTTs sent + MAC_Release sent | Grant preemption; send MAC_ACTIVE with new VCU on FACCH | CALL_SETUP (new call) |

### 5.2 FNE SACCH Ultraframe Behavior During Traffic

```
Per ultraframe (4 superframes), FNE outbound SACCH schedule:

Superframes 0-2 (Listener SACCHs):
  - MAC_ACTIVE PDU with VCU message identifying outbound audio source
  - May include: Power Control Signal Quality, group paging, Network Status Broadcast
  - May include: Alternate Activity Signaling (direct or indirect paging)

Superframe 3 (Talker SACCH):
  - MAC_ACTIVE PDU with VCU message containing FNE-selected inbound audio source ID
  - If zero address/ID or matching talker ID: talker continues
  - If mismatched non-zero ID: talker must stop

FNE outbound SACCH priority order:
  1. Talker ID messages (VCU with talker identity)
  2. Messages intended for transmitting SU
  3. Call control messages
  4. Responses to inbound SACCH signaling
  5. Scan/descrambling info (Network Status Broadcast)
  6. General interest messages
```

---

## 6. PDU Sequencing Requirements

### 6.1 Call Setup (Inbound — Talker SU)

```
Step 1: SU acquires sync (symbol, superframe, ultraframe alignment)
Step 2: SU sends on first available inbound FACCH:
        [FACCH] MAC_PTT #1   (contains group ID, source ID, Message Indicator, 
                               Algorithm ID, Key ID)
Step 3: SU sends on next inbound FACCH:
        [FACCH] MAC_PTT #2
Step 4 (optional): If SACCH is adjacent to either FACCH above:
        [SACCH] MAC_PTT #3

Minimum: 2 MAC_PTT on FACCH
Maximum: 2 on FACCH + 1 on SACCH = 3 total

Individual calls: group ID = 0x000000, source ID = talker WUID
```

### 6.2 Call Setup (Outbound — FNE)

```
Before talker arrives:
  [FACCH] MAC_ACTIVE (with VCU message, group/source IDs) — repeating
  [SACCH] MAC_ACTIVE (with VCU message) — repeating

After receiving inbound MAC_PTT(s):
  For each MAC_PTT received on inbound FACCH:
    [FACCH] MAC_PTT (outbound copy)
  Optional: [SACCH] MAC_PTT if SACCH is adjacent to a FACCH with MAC_PTT

Special case: If only 1 MAC_PTT received and it was on SACCH:
  [FACCH] MAC_PTT (generated on FACCH regardless)

Special case: If only 1 of 2 inbound MAC_PTTs decoded:
  FNE MAY generate a second MAC_PTT on outbound FACCH
```

### 6.3 Call Termination (Inbound — Talker SU)

```
Step 1: SU sends last voice burst (may stop at any point in 4V-4V-4V-4V-2V sequence)
Step 2: [FACCH] MAC_END_PTT #1
Step 3: [FACCH] MAC_END_PTT #2
Step 4 (optional): [FACCH] MAC_END_PTT #3   (up to 3 on FACCH)
Step 5 (optional): [SACCH] MAC_END_PTT if SACCH is adjacent to a FACCH above

Minimum: 2 on FACCH
Maximum: 3 on FACCH + 1 on SACCH = 4 total
```

### 6.4 Call Termination (Outbound — FNE)

```
Step 1: Finish transmitting any buffered audio
Step 2: [FACCH] MAC_END_PTT #1
Step 3: [FACCH] MAC_END_PTT #2
Step 4+: [FACCH] Additional MAC_END_PTTs (count per local policy)
Step N (optional): [SACCH] MAC_END_PTT if adjacent to FACCH above

For group call termination:
  Group address = call group ID
  Source address = 0xFFFFFF

For individual call termination:
  Group address = 0x000000
  Source address = 0xFFFFFF, OR matches source or target of call

Minimum: 2 on FACCH
```

### 6.5 Hangtime PDU Sequencing (Outbound — FNE, Message Trunked Only)

```
After finishing buffered audio:
  [FACCH] MAC_HANGTIME PDU (with VCU message for call identity) — repeating
  [SACCH] MAC_HANGTIME PDU (with VCU message) — repeating

Constraint: Minimum 1 VCU message per superframe (360 ms)

Continues until:
  (a) Hang timer expires OR max PDU count reached -> transition to TERMINATING
  (b) Call continuation request received -> transition to CALL_SETUP/TRAFFIC
```

### 6.6 PDU Sequencing Summary Table

| Phase | Direction | Channel | PDU Type | Min Count | Max Count | Timing |
|-------|-----------|---------|----------|-----------|-----------|--------|
| Call Setup | Inbound | FACCH | MAC_PTT | 2 | 2 | Consecutive FACCH slots |
| Call Setup | Inbound | SACCH | MAC_PTT | 0 | 1 | Adjacent to FACCH MAC_PTT |
| Call Setup | Outbound | FACCH | MAC_PTT | 1* | 2 | 1 per inbound FACCH MAC_PTT decoded |
| Call Setup | Outbound | SACCH | MAC_PTT | 0 | 1 | Adjacent to FACCH MAC_PTT |
| Termination | Inbound | FACCH | MAC_END_PTT | 2 | 3 | After last voice burst |
| Termination | Inbound | SACCH | MAC_END_PTT | 0 | 1 | Adjacent to FACCH MAC_END_PTT |
| Termination | Outbound | FACCH | MAC_END_PTT | 2 | Local policy | After buffered audio complete |
| Termination | Outbound | SACCH | MAC_END_PTT | 0 | Local policy | Adjacent to FACCH MAC_END_PTT |
| Hangtime | Outbound | FACCH | MAC_HANGTIME | 1/SF | Until timeout | Min 1 VCU per superframe |
| Hangtime | Outbound | SACCH | MAC_HANGTIME | 0 | Unlimited | As FNE determines |

*FNE should generate 2 outbound MAC_PTTs if possible, even from 1 decoded inbound.

---

## 7. SACCH/FACCH Scheduling

### 7.1 What Goes on FACCH vs SACCH

| Channel | Used During | Content |
|---------|-------------|---------|
| **Outbound FACCH** | Call setup (pre-voice) | MAC_ACTIVE with VCU |
| **Outbound FACCH** | Call setup (post MAC_PTT) | MAC_PTT (repeated from inbound) |
| **Outbound FACCH** | Traffic mode | Voice bursts (NOT FACCH) |
| **Outbound FACCH** | Hangtime | MAC_HANGTIME with VCU |
| **Outbound FACCH** | Termination | MAC_END_PTT |
| **Outbound FACCH** | Unassigned | MAC_IDLE with Null Information |
| **Outbound SACCH** | All assigned phases | MAC_ACTIVE with VCU, signaling, broadcasts |
| **Outbound SACCH** | Unassigned | MAC_IDLE (may contain general info for adjacent slot) |
| **Inbound FACCH** | Call setup | MAC_PTT |
| **Inbound FACCH** | Termination | MAC_END_PTT |
| **Inbound FACCH** | Unassigned | NOT used |
| **Inbound SACCH** | Traffic (talker) | Talker ID, call info, responses to FNE |
| **Inbound SACCH** | Traffic (listener) | Preemption requests (when FR=1 in I-ISCH) |
| **Inbound SACCH** | Unassigned | NOT used |

### 7.2 Ultraframe SACCH Scheduling Pattern

```
OUTBOUND SACCH per ultraframe (4 superframes):

SF 0 SACCH: Listener-targeted
  -> VCU message with outbound audio source ID
  -> May include: group paging, alt activity, general broadcast

SF 1 SACCH: Listener-targeted
  -> VCU message with outbound audio source ID
  -> May include: Power Control, Network Status Broadcast

SF 2 SACCH: Listener-targeted  
  -> VCU message with outbound audio source ID
  -> May include: responses to inbound listener SACCH

SF 3 SACCH: Talker-targeted
  -> VCU message with FNE-selected inbound audio source ID
  -> This is the slot the talker SU monitors to determine right-to-talk
  -> Zero address/ID = continue transmitting, check next ultraframe
  -> Matching address/ID = confirmed, continue transmitting
  -> Non-zero mismatched ID = STOP transmitting

INBOUND SACCH per ultraframe:

SF 0-2 SACCH: Listener slots (when FR=1 in I-ISCH)
  -> Listener SUs may transmit preemption requests
  -> FNE controls access via FR field

SF 3 SACCH: Talker slot (fixed rule, not FR-dependent)
  -> Transmitting SU sends talker info (Talker ID, call info)
  -> Transmitting SU uses fixed ultraframe rules, NOT FR field
```

### 7.3 FNE SACCH Scheduling for Transmitting SU

The FNE must account for the fact that a transmitting SU cannot receive the outbound
SACCH immediately following an inbound SACCH it used. Therefore:

```pseudocode
function schedule_outbound_sacch_for_talker(message, talker_lch):
    // The talker on LCH X transmits inbound SACCH, then cannot immediately
    // receive the adjacent outbound SACCH on the other LCH.
    // FNE should:
    //   (a) Not schedule talker-critical messages on the SACCH slot 
    //       immediately after the talker's inbound SACCH, OR
    //   (b) Repeat any talker-directed message to ensure delivery
    
    if sacch_slot is adjacent_to(talker_inbound_sacch):
        schedule_repeat(message, next_available_talker_sacch)
    else:
        transmit(message)
```

### 7.4 FACCH Position Within Superframe

The inbound FACCH positions (where MAC_PTT can first appear) depend on when the SU
arrives. Annex B of BBAE shows five possible starting positions for the first inbound
FACCH MAC_PTT:

```
Position 1: First available FACCH slot in superframe
Position 2: Second available FACCH slot
Position 3: Third available FACCH slot
Position 4: Fourth available FACCH slot (spans 2 superframes)
Position 5: Fifth available FACCH slot (spans 2 superframes)

The SU may arrive at any point in the superframe/ultraframe.
The first MAC_PTT goes on the first available inbound FACCH after sync.
The second MAC_PTT goes on the next inbound FACCH.
```

---

## 8. Voice Channel Procedures Detail

### 8.1 MAC_PTT / MAC_END_PTT Sequencing Pseudocode

```pseudocode
// === TALKER SU: PTT INITIATION ===
function talker_ptt_initiate(group_id, source_id, crypto_params):
    acquire_vch_sync()  // symbol + superframe + ultraframe
    
    ptt_pdu = build_MAC_PTT(
        group_id   = group_id,       // 0 for individual call
        source_id  = source_id,      // talker WUID
        msg_indicator = crypto_params.MI,
        algorithm_id  = crypto_params.algID,
        key_id        = crypto_params.keyID
    )
    
    // Send on first two available inbound FACCH slots
    facch_slot_1 = next_inbound_facch_slot()
    transmit_facch(facch_slot_1, ptt_pdu)
    
    facch_slot_2 = next_inbound_facch_slot()
    transmit_facch(facch_slot_2, ptt_pdu)
    
    // Optional: if SACCH is adjacent to either FACCH slot
    if sacch_is_adjacent(facch_slot_1) or sacch_is_adjacent(facch_slot_2):
        sacch_slot = get_adjacent_sacch()
        transmit_sacch(sacch_slot, ptt_pdu)
    
    // Begin voice burst transmission
    start_voice_framing()  // 4V-4V-4V-4V-2V sequence

// === TALKER SU: PTT RELEASE ===
function talker_ptt_release():
    finish_current_voice_burst()
    
    end_pdu = build_MAC_END_PTT(group_id, source_id)
    
    // Send 2-3 on inbound FACCH
    for i in range(2, min(3, available_facch_slots) + 1):
        slot = next_inbound_facch_slot()
        transmit_facch(slot, end_pdu)
    
    // Optional SACCH if adjacent
    if sacch_is_adjacent(last_facch_slot):
        transmit_sacch(get_adjacent_sacch(), end_pdu)
    
    transition_to_listening()
```

### 8.2 Hangtime Behavior

```pseudocode
// === FNE: MESSAGE TRUNKED HANGTIME ===
function fne_enter_hangtime(call_info):
    hang_timer = start_timer(HANG_DURATION)  // local policy
    hangtime_pdu_count = 0
    
    while not hang_timer.expired and not continuation_received:
        // Every FACCH opportunity on outbound:
        pdu = build_MAC_HANGTIME(
            vcu_message = build_vcu(call_info.group_id, call_info.source_id)
            // May include other messages
        )
        transmit_outbound_facch(pdu)
        hangtime_pdu_count += 1
        
        // Also on SACCH:
        transmit_outbound_sacch(pdu)
        
        // CONSTRAINT: at least 1 VCU message per superframe
        assert(vcu_count_this_superframe >= 1)
        
        // Check for inbound MAC_PTT (VCH continuation)
        if detect_inbound_mac_ptt():
            return handle_vch_continuation()
        
        // Check for CCH service request for same call
        if cch_request_pending_for_call(call_info):
            return handle_cch_continuation()
    
    // Hangtime expired - terminate call
    fne_terminate_call_transmission_trunked()
```

### 8.3 Talker Right-to-Talk Verification (During Traffic)

```pseudocode
// === TALKER SU: ONGOING TRAFFIC MODE CHECK ===
function talker_check_right_to_talk():
    // Runs each ultraframe when talker SACCH is received
    
    sacch_pdu = decode_outbound_talker_sacch()
    
    if sacch_pdu is None:
        // Failed to decode - count consecutive failures
        sacch_fail_count += 1
        if sacch_fail_count >= 2:  // 2 ultraframes minimum
            // Assume lost right to talk
            finish_current_voice_burst()
            send_mac_end_ptts()
            notify_user("Transmission failed")
            return LISTENING
        return CONTINUE  // try again next ultraframe
    
    sacch_fail_count = 0
    
    // Extract VCU address from MAC_PTT or MAC_ACTIVE PDU
    vcu_id = extract_vcu_address(sacch_pdu)
    
    if vcu_id == 0x000000:
        // Zero address: continue, check next ultraframe
        return CONTINUE
    
    if vcu_id == my_wuid:
        // Match: confirmed right to talk
        return CONTINUE
    
    // Non-zero mismatch: lost right to talk
    send_mac_end_ptts()
    notify_user("Transmission failed - preempted")
    return LISTENING
```

### 8.4 Voice Framing

```
Voice burst sequence per superframe (per LCH):
  Burst 1: 4V (4-slot voice frame)
  Burst 2: 4V
  Burst 3: 4V
  Burst 4: 4V
  Burst 5: 2V (2-slot voice frame — final burst, shorter)

Voice framing position is indicated by:
  - Offset field in MAC_PTT PDUs
  - Offset field in MAC_ACTIVE PDUs on SACCH
  - DUID of 2V burst (uniquely identifies position 5)

Voice may be terminated at any point in this sequence — no need to wait
for end of superframe.

Inbound and outbound voice bursts are staggered:
  If inbound on even slots (LCH 0) -> outbound on odd slots (LCH 1)
  If inbound on odd slots (LCH 1) -> outbound on even slots (LCH 0)
```

---

## 9. Control Channel Procedures

### 9.1 LCCH Acquisition (SU Power-On)

```pseudocode
function su_acquire_lcch():
    // Step 1: Detect S-ISCH (symbol synchronization)
    while not detect_s_isch():
        scan_next_channel()
    
    // Step 2: Acquire superframe alignment via I-ISCH
    isch_decoded = False
    while not isch_decoded:
        result = decode_i_isch()
        if result.success:
            isch_decoded = True
            lch_type = result.lch_type_field
            os_field = result.os_field
            channel_number = result.channel_number
    
    // Step 3: Determine LCCH configuration
    if lch_type == LCCH:
        if os_field indicates other LCH is also LCCH:
            config = DUAL_LCCH
        else:
            config = SINGLE_LCCH
    elif lch_type == TCH:
        // Must check other LCH
        other_result = decode_i_isch(other_lch)
        if other_result.lch_type == LCCH:
            config = SINGLE_LCCH  // on the other LCH
        else:
            config = COMPOSITE  // no dedicated LCCH
    
    // Step 4: Begin monitoring appropriate LCCH
    if config == DUAL_LCCH:
        monitor_both_lcch()    // use first available inbound slot
    elif config == SINGLE_LCCH:
        monitor_single_lcch()  // monitor designated LCH only
    elif config == COMPOSITE:
        follow_composite_procedures()  // per [2]
```

### 9.2 CCH Designation Rules

| Topology | Network Status Broadcast Channel Field | SU Behavior |
|----------|---------------------------------------|-------------|
| Single LCCH on LCH 0 | TDMA type, Slot 0 | Monitor LCH 0 outbound; use LCH 0 inbound |
| Single LCCH on LCH 1 | TDMA type, Slot 1 | Monitor LCH 1 outbound; use LCH 1 inbound |
| Dual LCCH | TDMA type, Slot 0 | Monitor LCH 0 AND LCH 1; use first available inbound |
| Composite (no LCCH) | Per [2] | Follow FDMA composite procedures adapted for TDMA |

### 9.3 Registration and Affiliation

Registration and affiliation procedures are defined in TIA-102.AABD-B [2] for FDMA CCH.
For TDMA LCCH, the same procedures apply with these differences:

- **Slot timing**: T_LCCH replaces T_SLOT (30 ms for dual, 60 ms for single)
- **Retry timing**: T_retry = uniform random over [0, N_random x T_LCCH]
- **T_ctrl default**: 240 ms (vs FDMA value)
- **T_ctrl minimum**: 120 ms

### 9.4 Channel Grant Procedures

```pseudocode
function su_handle_channel_grant(grant_message):
    channel = grant_message.channel_field
    slot = grant_message.slot_number
    call_type = grant_message.service_type
    
    if channel.is_tdma_type():
        tune_to_frequency(channel.frequency)
        target_lch = slot  // 0 or 1
        
        if i_am_talker(grant_message):
            // Talker path
            acquire_vch_sync(target_lch)
            begin_talker_call_setup()
        else:
            // Listener path
            acquire_vch_sync(target_lch)
            begin_listener_monitoring()
    else:
        // FDMA grant - follow FDMA procedures per [2]
        handle_fdma_grant(grant_message)
```

### 9.5 LCH Transition Procedures

```pseudocode
// === FNE: LCCH -> TCH TRANSITION ===
function fne_transition_lcch_to_tch(lch, new_call_info):
    // Can only transition at allowed timeslots
    assert(current_timeslot in allowed_transition_slots(lch))
    // LCH 0: timeslots 0, 4, 8
    // LCH 1: timeslots 1, 5, 9
    
    // Change I-ISCH LCH Type field
    set_i_isch(lch, lch_type=TCH, fields_per_bbac_for_tch)
    
    // TCH signaling begins in the VERY NEXT timeslot after the I-ISCH
    // For VCH: immediately begin call setup FACCH signaling
    begin_vch_call_setup_signaling(lch, new_call_info)

// === SU: DETECT LCCH -> TCH TRANSITION ===
function su_detect_lcch_to_tch():
    // Primary: I-ISCH LCH Type field changes from LCCH to TCH
    if i_isch.lch_type changed to TCH:
        handle_transition()
    
    // Backup: detect change in burst types on channel
    // (SU should detect even if I-ISCH was missed due to errors)
    if burst_types_inconsistent_with_lcch():
        handle_transition()

function handle_transition():
    if was_dual_lcch:
        // Now single LCCH on remaining LCH
        monitor_remaining_lcch()
    elif was_single_lcch:
        // Now composite topology
        search_site_for_new_lcch()
        // All TDMA channels at site are synchronized,
        // so scanning is fast (no re-sync needed per channel)
```

---

## 10. SU Decision Trees

### 10.1 SU: When to Transmit

```
START: SU wants to talk
  |
  v
Is SU currently on a VCH?
  |
  +-- NO --> Go to CCH; issue voice service request; wait for grant
  |           |
  |           +-- GRANT received --> Tune to VCH; sync; send MAC_PTTs
  |           +-- DENY received --> Return to idle; notify user
  |           +-- QUE received --> Wait for further signaling
  |
  +-- YES --> Is channel in HANGTIME state (MAC_HANGTIME on outbound FACCH)?
        |
        +-- NO --> Is this a telephone interconnect call?
        |     |
        |     +-- YES --> May attempt VCH audio preemption (send request on listener SACCH)
        |     +-- NO --> Is audio preemption desired?
        |           |
        |           +-- YES --> Use CCH or VCH preemption method (see Section 11)
        |           +-- NO --> Cannot transmit; wait
        |
        +-- YES --> What continuation method is advertised?
              |
              +-- VCH method --> Send MAC_PTTs directly on inbound FACCH
              +-- CCH method --> Return to CCH; issue voice service request
              |
              (For telephone interconnect: always VCH method)
```

### 10.2 SU: When to Release Channel

```
START: SU is on a VCH
  |
  v
Any of the following triggers RELEASE:

1. Received MAC_END_PTT with correct Color Code
   --> Return to CCH idle IMMEDIATELY

2. VCU address/ID mismatch (non-zero IDs don't match assignment)
   Group call: group ID mismatch
   Individual call: neither source nor target ID matches SU
   --> Return to CCH

3. Sync loss detected (persistent CRC failures, channel mutes)
   --> Return to CCH to re-verify system (WACN, System ID, Color Code)

4. SU leaves channel for CCH transaction
   --> May maintain time counter to resume in-sync on return

5. Talker SU: outbound talker SACCH with non-zero mismatched WUID
   --> Stop transmitting; send MAC_END_PTTs; revert to listener
   --> If call preemption (C/A=0, U/F=1): leave channel entirely

6. Call preemption detected (MAC_PTT or VCU for different call on channel)
   --> Leave channel
```

### 10.3 SU: Preemption Handling Decision Tree

```
START: SU receives MAC_Release on outbound SACCH
  |
  v
Check C/A field:
  |
  +-- C/A = 1 (Audio Preemption) --> SU stays on channel
  |     |
  |     v
  |     Check U/F field:
  |       |
  |       +-- U/F = 1 (Forced) --> MUST stop transmitting
  |       |     Send MAC_END_PTTs
  |       |     Begin listening
  |       |
  |       +-- U/F = 0 (Unforced) --> MAY continue transmitting (local policy)
  |             OR may stop and listen
  |
  +-- C/A = 0 (Call Preemption) --> SU must leave channel
        |
        v
        Check U/F field:
          |
          +-- U/F = 1 (Forced) --> Send 2x MAC_END_PTT on inbound FACCH
                Stop transmitting
                Leave channel immediately
```

---

## 11. Preemption Procedures

### 11.1 Audio Preemption State Flow (FNE)

```
Current State: TRAFFIC (talker A active)
  |
  v
FNE receives preemption request for talker B (same group, higher priority)
  |
  v
FNE decides to grant (local policy)
  |
  v
FNE sends MAC_Release on outbound talker SACCH:
  C/A = 1 (audio preemption)
  U/F = 0 (unforced) or 1 (forced)
  |
  v
FNE sets FR = 0 in I-ISCH (block listener SACCH during transition)
  |
  v
FNE terminates repeat of talker A audio
  |
  v
FNE sends MAC_ACTIVE or MAC_PTT PDUs for talker B (whichever available)
  |
  v
FNE begins repeating talker B audio
  |
  v
Listener SUs see new talker ID in outbound VCU messages

State: TRAFFIC (talker B active)

If audio preemption ends before talker A finished:
  FNE may return outbound audio to talker A (if still transmitting)
```

### 11.2 Call Preemption: Polite vs Impolite

```
=== POLITE CALL PREEMPTION (FNE) ===

1. FNE sends MAC_END_PTTs on outbound FACCH (>= 2, per 4.5.3.1)
2. FNE may send MAC_END_PTTs on outbound listener SACCH
3. FNE sends MAC_Release (C/A=0, U/F=1) on next talker outbound SACCH
4. Complete MAC_END_PTT transmission
5. Grant preemption request
6. Send MAC_ACTIVE on outbound FACCH with new call VCU
7. Send MAC_ACTIVE on outbound listener SACCH with new audio source
8. Begin new call: MAC_PTTs then voice

=== IMPOLITE CALL PREEMPTION (FNE) ===

1. Grant preemption request immediately
2. Send MAC_ACTIVE on outbound FACCH + listener SACCH with new VCU
3. Begin new call: MAC_PTTs then voice
(No MAC_END_PTTs sent for old call; SUs detect mismatch and leave)
```

### 11.3 VCH Method Preemption Timing

```
Requesting SU sends service request on next available listener inbound SACCH
  |
  v
FNE MUST respond within 1 ultraframe (1440 ms) on outbound listener SACCH
  Possible responses:
  - GRANT
  - DENY
  - ACK_RSP_FNE (quiets retries; final answer pending)
  |
  v
If no response received:
  SU waits minimum 1 ultraframe
  During wait: monitors for grant/deny/ack OR VCU with own ID (implicit grant)
  After wait: may retry or notify user of failure

If ACK_RSP_FNE received:
  SU stops retries; waits for GRANT or DENY
  If SU sees VCU in talker SACCH with own ID: treat as implicit grant
```

---

## 12. Error Handling and Timeouts

### 12.1 Error Conditions and Recovery

| Condition | Detection | Recovery |
|-----------|-----------|----------|
| Inbound MAC_PTT decode failure (FNE) | CRC failure on FACCH | If 1 of 2 decoded, FNE may generate 2nd outbound MAC_PTT |
| Outbound sync loss (SU) | Persistent ISCH/SACCH decode failures | Return to CCH; re-verify system parameters |
| Inbound sync loss (FNE) | SACCH CRC failures, no sync sequence | FNE attempts re-sync; may terminate call |
| Talker SACCH not decoded (SU) | No valid SACCH in 2+ ultraframes | Stop transmitting; send MAC_END_PTTs |
| Wrong system detected (SU) | Valid sync/ISCH but persistent voice/SACCH CRC failures (scrambling mismatch) | Return to CCH; verify WACN/System ID/Color Code |
| I-ISCH decode failure during transition | SU fails to see LCH Type change | Detect indirectly from burst type change |
| CCH retry failure | No response after N retries | T_retry backoff; eventually notify user |
| Call setup timeout (FNE) | No inbound MAC_PTT within timeout | Release VCH; report call failure |
| Hangtime timeout (FNE) | Hang timer expires | Send MAC_END_PTTs; release VCH |

### 12.2 Timeout Values

| Timer | Value | Entity | Trigger |
|-------|-------|--------|---------|
| T_ctrl | 240 ms default, 120 ms minimum | SU | CCH response timeout |
| T_retry | [0, N_random x T_LCCH] random | SU | Retry delay after CCH failure |
| T_SYNC_BCST | System-defined | FNE | Sync broadcast interval on FDMA CCH |
| Talker confirmation timeout | >= 2 ultraframes (>= 2880 ms) | SU | Wait for talker SACCH confirmation |
| Preemption response timeout | 1 ultraframe (1440 ms) | FNE | Must respond to VCH preemption request |
| Hang timer | Local policy | FNE | Duration of hangtime phase |
| Call setup timeout | Local policy | FNE | Wait for talker to arrive on VCH |

### 12.3 Retry Behavior

```pseudocode
// === SU CCH RETRY PROCEDURE ===
function su_cch_retry(request):
    attempts = 0
    max_attempts = system_defined  // per [2]
    
    while attempts < max_attempts:
        transmit_inbound_lcch(request)
        
        wait(T_ctrl)  // 240ms default
        
        if response_received():
            return handle_response()
        
        // No response - retry with random backoff
        attempts += 1
        delay = uniform_random(0, N_random * T_LCCH)
        wait(delay)
    
    // All retries exhausted
    notify_user("Request failed")
    return FAILURE
```

---

## 13. Scrambling Coordination

### 13.1 Scrambling Relative to Call State

```
Scrambling is applied per-burst, restarting from the seed at each superframe boundary.
The scrambling state is independent of call state — it depends only on the burst type
and its position within the superframe.

ALWAYS SCRAMBLED:
  - IECI bursts (inbound control)
  - Inbound 2V bursts
  - Inbound 4V bursts
  - Outbound 2V bursts
  - Outbound 4V bursts

NEVER SCRAMBLED:
  - OECI bursts (outbound control — so LCCH outbound is always readable)

CONDITIONALLY SCRAMBLED:
  - IEMI bursts: scrambled UNLESS containing MAC_END_PTT PDU
  - I-OEMI bursts: scrambled UNLESS containing MAC_END_PTT or Network Status Broadcast
  - S-OEMI bursts: scrambled UNLESS containing MAC_END_PTT or Network Status Broadcast
```

### 13.2 Why MAC_END_PTT and Network Status Broadcast Are Unscrambled

```
MAC_END_PTT unscrambled rationale:
  - Allows identification of Color Code from potential interfering systems
  - SU can decode termination even if scrambling parameters are wrong
  - Critical for reliable call teardown

Network Status Broadcast unscrambled rationale:
  - This message provides the parameters (WACN ID, System ID, Color Code)
    needed to COMPUTE the scrambling seed
  - Must be readable without already having the scramble parameters
  - Color Code field allows interference detection

Entire burst is unscrambled in these cases (no partial-burst scrambling mechanism exists).
```

### 13.3 Scrambling Start/Stop Relative to Call Events

```
Call Setup:
  Inbound MAC_PTT on IEMI -> SCRAMBLED (normal IEMI rule)
  Outbound MAC_PTT on I-OEMI/S-OEMI -> SCRAMBLED

Traffic Mode:
  Voice bursts (2V, 4V) -> ALWAYS SCRAMBLED
  SACCH (IEMI inbound / I-OEMI,S-OEMI outbound) -> SCRAMBLED

Call Termination:
  Inbound MAC_END_PTT on IEMI -> NOT SCRAMBLED (exception!)
  Outbound MAC_END_PTT on I-OEMI/S-OEMI -> NOT SCRAMBLED (exception!)

Hangtime:
  MAC_HANGTIME on I-OEMI/S-OEMI -> SCRAMBLED (no exception for hangtime)

Unassigned:
  MAC_IDLE on I-OEMI/S-OEMI -> SCRAMBLED (unless containing Net Status Broadcast)
```

### 13.4 Implementation Note: Scramble Decision Logic

```pseudocode
function should_scramble(burst_type, pdu_content):
    match burst_type:
        case IECI:      return True
        case INBOUND_2V: return True
        case INBOUND_4V: return True
        case OUTBOUND_2V: return True
        case OUTBOUND_4V: return True
        case OECI:      return False
        case IEMI:
            if pdu_content.contains(MAC_END_PTT):
                return False
            return True
        case I_OEMI | S_OEMI:
            if pdu_content.contains(MAC_END_PTT):
                return False
            if pdu_content.contains(NETWORK_STATUS_BROADCAST):
                return False
            return True
```

---

## 14. Implementation Notes and Open-Source Cross-References

### 14.1 SDRTrunk Cross-References

SDRTrunk (Java, open-source P25 decoder) implements Phase 2 TDMA decoding with
relevant classes including:

- **P25P2MessageFramer** — handles TDMA burst synchronization and framing, implementing
  the sync acquisition procedures from Section 2.2/4.4 of BBAE
- **P25P2DecoderState** — tracks the call state machine similar to the listener SU
  state machine in Section 4 above; handles transitions between CALL/HANGTIME/TEARDOWN
- **ScrambleParameters** / **Scrambler** — implements the LFSR scramble/descramble
  pipeline; applies the burst-type-conditional scrambling rules from Section 2.3
- **MacOpcode** processing — decodes MAC_PTT, MAC_END_PTT, MAC_ACTIVE, MAC_HANGTIME,
  MAC_IDLE PDUs and dispatches to appropriate state handlers

Key implementation observations from SDRTrunk:
- Listener-only implementation (no transmit path)
- Uses both S-ISCH and FACCH sync sequences for initial synchronization
- Tracks voice framing via 2V DUID detection and Offset field decoding
- Hangtime detection via MAC_HANGTIME PDU opcode

### 14.2 OP25 Cross-References

OP25 (C++/Python, open-source P25 decoder) implements Phase 2 in:

- **p25p2_tdma.cc** — main TDMA processing; handles superframe alignment, burst
  classification, and the demodulation pipeline
- **p25p2_vf.cc** — voice frame processing; implements the 4V-4V-4V-4V-2V voice
  burst sequence decoding
- **p25p2_isch.cc** — I-ISCH/S-ISCH processing for sync and LCH type determination

Key implementation observations from OP25:
- Also listener-only (no transmit)
- Implements scramble sequence generation with the 44-bit LFSR
- Uses I-ISCH LCH Type field to determine channel designation
- Handles SACCH decode for extracting talker ID and call parameters

### 14.3 Common Implementation Patterns

Both open-source implementations share these patterns relevant to this spec:

1. **Sync acquisition**: Both use S-ISCH sync sequence detection as the primary
   entry point, matching BBAE Section 2.2

2. **State tracking**: Both implement a simplified version of the listener SU state
   machine — tracking CALL_SETUP -> TRAFFIC -> HANGTIME -> TEARDOWN without the
   transmit-side states

3. **Scramble handling**: Both implement the conditional scrambling rules, checking
   DUID/burst type before applying descrambling

4. **SACCH parsing**: Both extract VCU messages from SACCH to identify talker and
   group, matching the SACCH ultraframe schedule described in Section 7

### 14.4 Implementation Priorities for a Transmit-Capable System

For implementers building a complete SU or FNE (not just a receiver):

1. **Critical path**: MAC_PTT/MAC_END_PTT sequencing (Section 6) — errors here cause
   call setup failures or orphaned channels
2. **Talker verification**: Right-to-talk check in ultraframe talker SACCH (Section 8.3) —
   failure to implement causes infinite transmit or duplicate talkers
3. **Scramble exceptions**: MAC_END_PTT and Network Status Broadcast must NOT be
   scrambled (Section 13) — scrambling these causes call teardown failures
4. **Timing precision**: Inbound burst timing must align with outbound TDMA framing;
   SU transmit clock drift compensation via S-ISCH monitoring (slots 10/11) is
   essential for sustained transmissions

---

## 15. Gaps and Missing Information

### 15.1 Information Not Present in BBAE Full Text Extraction

The following items are referenced by BBAE but defined in companion documents and
NOT fully reproducible from BBAE alone:

| Missing Item | Defined In | Impact |
|-------------|------------|--------|
| Exact ultraframe SACCH slot mapping | TIA-102.BBAC-A [4] | Cannot determine precise inbound talker vs listener SACCH positions |
| Burst structure details (field sizes, encoding) | TIA-102.BBAC-A [4] | Cannot construct raw burst bytes |
| MAC PDU encoding (field layouts, opcodes) | TIA-102.BBAD-A [5] | Cannot encode/decode PDU wire format |
| FDMA trunking procedures (registration, affiliation, service request/grant) | TIA-102.AABD-B [2] | CCH procedures reference these extensively |
| FDMA trunking message formats (GRP_V_CH_GRANT, etc.) | TIA-102.AABC-E [1] | Grant message decoding |
| Dynamic regrouping procedures | TIA-102.AABH [3] | Group management |
| N_random parameter value | TIA-102.AABD-B [2] | CCH retry backoff calculation |
| Voice codec framing details | Not in BBAE suite | Voice encode/decode |

### 15.2 Items Left to Local Policy by BBAE

These are explicitly undefined in the standard and vary by manufacturer/operator:

| Item | BBAE Reference |
|------|----------------|
| Closed-loop power control algorithm | Section 2.4.2 |
| Total MAC_END_PTT count beyond minimum 2 (outbound) | Section 4.5.3 |
| Hangtime duration (hang timer value) | Section 4.5.3.2 |
| Method for selecting among competing preemption requestors | Section 4.5.5 |
| User notification mechanism for preemption | Section 4.5.5 |
| Polite vs impolite call preemption decision | Section 4.5.5.2.4 |
| Call setup timeout duration (FNE) | Section 4.5.1 (implied) |
| SU behavior on unforced preemption (continue or stop) | Section 4.5.5.1.4 |
| Call continuation method selection (CCH vs VCH) | Section 4.5.4 (FNE decides) |
| Sync acquisition timeout | Section 4.4 (implied) |

### 15.3 Information Lost in PDF-to-Text Extraction

The original PDF contains visual diagrams that could not be fully captured in text:

| Figure | Content | What Would Be Added |
|--------|---------|--------------------|
| Figures 1-2 | LFSR generator diagrams | Visual confirmation of tap positions (covered by polynomial) |
| Figure 3 | Matrix M (44x44) | Fully captured in text extraction |
| Figure 4 | Matrix SH(2^43) (44x44) | Fully captured in text extraction |
| Figure 5 | Scramble sequence timing | Superframe-to-LFSR offset mapping per LCH |
| Figures 6-8 (Annex A) | LCCH call control timing diagrams | Precise slot-by-slot timing of grant -> VCH transition |
| Figures 9-13 (Annex B) | PTT initiation timing for 5 FACCH positions | Exact slot positions for each MAC_PTT start position |
| Figure 14 (Annex B) | Call termination timing | Exact slot positions for MAC_END_PTT sequence |
| Figures 15-22 (Annex C) | Complete call flow examples | End-to-end timing showing CCH interaction, sync acquisition delays, and voice burst placement |

The Annex B/C figures (Figures 9-22) would be particularly valuable for implementers as they
show the precise per-slot timing of MAC_PTT and MAC_END_PTT placement relative to voice
bursts, SACCH positions, and ISCH boundaries. The text descriptions provide the rules but
the figures provide concrete reference implementations of those rules applied to every
possible starting position.

> **RECOMMENDATION:** Implementers should obtain the original PDF for Figures 9-22
> (Annexes B and C) to verify their MAC_PTT/MAC_END_PTT slot placement logic against
> the normative timing diagrams. The state machines and sequencing rules in this spec
> are complete, but the visual diagrams serve as invaluable test vectors.

---

*End of Implementation Specification*
