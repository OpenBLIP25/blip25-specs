# P25 FDMA Trunking Control Channel Procedures -- Implementation Specification

**Source:** TIA-102.AABD-B (November 2014), amendments B-1 (2023-12), B-2 (2023-07), B-3 (2025-02)
**Phase:** 3 -- Implementation-ready
**Classification:** PROTOCOL
**Extracted:** 2026-04-12
**Purpose:** Self-contained behavioral spec for implementing both SU-side and FNE/RFSS-side
P25 FDMA trunking state machines, procedures, and timers. This is the behavioral companion to
TIA-102.AABC-E (message formats) and TIA-102.AABB-B (vocoder/codec formats). No reference
to the original PDF required for implementation.

**Companion specs (cross-referenced throughout):**
- **AABC-E** -- All ISP/OSP message opcodes and field layouts
- **BAAA-B** -- FDMA physical layer (modulation, NID, frame sync)
- **AACE** -- Authentication algorithms and key management
- **BAAD** -- Link layer (LDU structure, traffic channel framing)
- **BBAC** -- TDMA MAC procedures (Phase 2 traffic channels)

---

## Table of Contents

1. [Control Channel Management](#1-control-channel-management)
2. [SU Registration](#2-su-registration)
3. [Group Affiliation](#3-group-affiliation)
4. [Voice Call Setup](#4-voice-call-setup)
5. [Channel Grant Procedures](#5-channel-grant-procedures)
6. [Call Maintenance](#6-call-maintenance)
7. [Emergency Procedures](#7-emergency-procedures)
8. [Authentication](#8-authentication)
9. [Roaming](#9-roaming)
10. [Data Services](#10-data-services)
11. [Timer Definitions](#11-timer-definitions)
12. [State Machines](#12-state-machines)

---

## 1. Control Channel Management

### 1.1 Overview

The FNE manages a dedicated outbound control channel that continuously broadcasts signaling
packets (OSPs) to all SUs. The control channel carries three classes of traffic:

1. **Broadcast messages** -- periodic system information (no explicit address)
2. **Directed messages** -- responses to SU requests, addressed to specific WUIDs
3. **Grant messages** -- channel assignments for voice/data calls

The inbound control channel uses Slotted Aloha random access (Section 4 of source).

### 1.2 Outbound Message Scheduling

The FNE must interleave broadcast and directed messages on the outbound control channel.
Each TSBK occupies one time slot on the outbound. The scheduling algorithm must satisfy:

**Hard constraints:**
- Every broadcast type must appear at least once per T_BCST (3 seconds max, default 3 sec)
- NET_STS_BCST interval <= T_BCST
- RFSS_STS_BCST interval <= T_BCST
- IDEN_UP (one per channel identifier) interval <= T_BCST
- ADJ_STS_BCST (one per adjacent site) interval <= T_ASB_UPDT (default unspecified, max 10 sec)
- SYNC_BCST interval <= T_SYNC_BCST (max 10 sec) when TDMA present

**Soft constraints:**
- Directed messages (responses to ISPs) should be sent within T_RFSS_RSP (500 ms) of receipt
- Grant messages take priority over broadcast rotation
- Emergency grants take priority over all other directed messages

### 1.3 Broadcast Message Rotation

**AABC-E messages used:** `NET_STS_BCST` (OSP 0x3B), `RFSS_STS_BCST` (OSP 0x3A),
`IDEN_UP` (OSP 0x34), `IDEN_UP_VU` (OSP 0x35), `IDEN_UP_TDMA` (OSP 0x33),
`ADJ_STS_BCST` (OSP 0x3C), `SCCB` (OSP 0x39), `SYS_SRV_BCST` (OSP 0x38),
`SYNC_BCST` (OSP 0x37)

Recommended rotation order (one cycle):

```
Slot 1:  NET_STS_BCST           -- network identity
Slot 2:  RFSS_STS_BCST          -- site identity, trunking mode (R bit)
Slot 3:  IDEN_UP[0]             -- first channel identifier
Slot 4:  ADJ_STS_BCST[0]       -- first adjacent site
Slot 5:  <directed if pending>  -- responses, grants
Slot 6:  IDEN_UP[1]             -- next channel identifier
Slot 7:  ADJ_STS_BCST[1]       -- next adjacent site
Slot 8:  SYS_SRV_BCST          -- service availability
Slot 9:  SCCB                   -- secondary control channels
Slot 10: <directed if pending>
... repeat, cycling through all IDEN_UP[n] and ADJ_STS_BCST[n]
```

The rotation must be interruptible: any pending directed message (grant, deny, registration
response) preempts the next scheduled broadcast slot.

### 1.4 Pseudocode: FNE Outbound Scheduler

```python
class ControlChannelScheduler:
    def __init__(self, site_config):
        self.broadcast_queue = deque()   # rotating broadcast OSPs
        self.directed_queue = PriorityQueue()  # directed OSPs by priority
        self.last_broadcast_times = {}   # message_type -> timestamp
        self.T_BCST = 3.0  # seconds
        self.T_ASB_UPDT = 10.0

    def next_osp(self) -> TSBK:
        """Select next OSP to transmit on outbound control channel."""
        now = time.monotonic()

        # Priority 1: Emergency grants
        if self.directed_queue.has_emergency():
            return self.directed_queue.pop_emergency()

        # Priority 2: Any directed message older than T_RFSS_RSP/2
        if self.directed_queue.has_urgent(threshold=0.250):
            return self.directed_queue.pop()

        # Priority 3: Overdue broadcasts (any broadcast past T_BCST)
        overdue = self.get_overdue_broadcast(now)
        if overdue:
            return overdue

        # Priority 4: Remaining directed messages
        if not self.directed_queue.empty():
            return self.directed_queue.pop()

        # Priority 5: Next broadcast in rotation
        return self.next_broadcast_rotation(now)

    def get_overdue_broadcast(self, now) -> Optional[TSBK]:
        for msg_type in [NET_STS_BCST, RFSS_STS_BCST]:
            if now - self.last_broadcast_times.get(msg_type, 0) >= self.T_BCST:
                self.last_broadcast_times[msg_type] = now
                return self.build_broadcast(msg_type)
        return None
```

### 1.5 Control Channel Acquisition (SU Side)

**AABC-E messages used:** All broadcast OSPs (decoded passively)

The SU must acquire and maintain lock on the outbound control channel. Signal quality is
continuously monitored.

**Control channel loss detection:**
- T_ctrl = 300 ms (default) of no valid NID decoded => channel lost
- Minimum T_ctrl = 150 ms

**Hunt procedure on loss:**

| Step | Action | Channels Scanned |
|------|--------|-----------------|
| 1. Short Hunt | Scan prioritized short list | Last known CC, adjacent sites from ADJ_STS_BCST, programmed scan list |
| 2. Extended Hunt | Scan full channel list | All programmable frequencies |
| 3. Fallback Search | Scan for LC_CONV_FALLBACK | Same channels as short hunt plus configured fallback channels |
| 4. Loop | Return to Extended Hunt | Repeat until control channel found |

On each candidate channel, the SU listens for a valid NID. If NID is acquired, the SU
verifies the NAC matches, then waits for NET_STS_BCST and RFSS_STS_BCST to confirm
system identity before committing to the channel.

### 1.6 SDRTrunk Cross-Reference

SDRTrunk implements control channel acquisition in `P25TrafficChannelManager.java`.
The class monitors the outbound control channel, parses all broadcast OSPs, and maintains
channel identifier tables and adjacent site lists. When a GRP_V_CH_GRANT is received,
it spawns a traffic channel decoder on the granted frequency.

Key SDRTrunk classes:
- `P25P1DecoderState` -- main state machine for control channel monitoring
- `P25TrafficChannelManager` -- manages traffic channel following
- `P25P1MessageProcessor` -- dispatches parsed TSBKs to handlers

### 1.7 OP25 Cross-Reference

OP25 implements control channel monitoring in `trunking.py` (Python) with GNU Radio C++
blocks for physical layer demodulation. The `rx.py` module manages the trunking state
machine including control channel lock, channel grant following, and broadcast message
tracking.

---

## 2. SU Registration

### 2.1 Overview

Registration associates a SU's identity (SUID) with the serving RFSS and assigns a
Working Unit ID (WUID). Registration is mandatory before any service request.

**AABC-E messages used:**
- `U_REG_REQ` (ISP 0x2C) -- SU requests registration
- `U_REG_RSP` (OSP 0x2C) -- RFSS accepts/denies registration
- `U_REG_CMD` (OSP 0x2D) -- RFSS commands re-registration
- `LOC_REG_REQ` (ISP 0x0F) -- SU requests location registration
- `LOC_REG_RSP` (OSP 0x1B) -- RFSS responds to location registration
- `U_DE_REG_REQ` (ISP 0x2B) -- SU requests de-registration
- `U_DE_REG_ACK` (OSP 0x2B) -- RFSS acknowledges de-registration

### 2.2 Conditions Requiring Registration

| Condition | Registration Type | Notes |
|-----------|-------------------|-------|
| Power-on | Full (U_REG_REQ) | Always required |
| System change (different WACN/System/RFSS) | Full (U_REG_REQ) | New WUID assigned |
| WUID expired (T_wuid_validity elapsed) | Full (U_REG_REQ) | Default 4 hours |
| RFSS command (U_REG_CMD received) | Full (U_REG_REQ) | RFSS forces re-register |
| After conventional fallback | Full (U_REG_REQ) | Registration state stale |
| Move to new RFSS within same system | Location (LOC_REG_REQ) | WUID retained |

### 2.3 Full Registration Flow

```
SU                              RFSS
|                                 |
|--- U_REG_REQ (SUID, ESN) ----->|  [Slotted Aloha access per Sec 4]
|                                 |  [RFSS validates SUID, assigns WUID]
|<-- U_REG_RSP (WUID, accept) ---|  [or DENY_RSP / U_REG_CMD]
|                                 |
|--- GRP_AFF_REQ (WGID) -------->|  [SU immediately affiliates]
|<-- GRP_AFF_RSP (accept) -------|
|                                 |
```

### 2.4 WUID Assignment Rules

| SU Type | WUID Assignment |
|---------|----------------|
| Home SU (same WACN + System) | WUID = lower 24 bits of SUID (Unit ID) |
| Roaming SU (different WACN or System) | WUID assigned from reserved pool |
| Emergency temporary access | WUID from reserved pool, limited services |

**Special WUIDs (never assigned to SUs):**

| WUID | Name | Purpose |
|------|------|---------|
| $000000 | No Unit | Placeholder |
| $FFFFFC | FNE Radio Dispatch | Radio control/dispatch operator |
| $FFFFFD | System Default | FNE call processing |
| $FFFFFE | Registration Default | Unregistered SU identity |
| $FFFFFF | ALL UNIT | Broadcast to all SUs |

### 2.5 Location Registration

Location registration is a lightweight update when a SU moves to a different RFSS site
within the same system. The WUID is retained.

```
SU                              RFSS (new site)
|                                 |
|--- LOC_REG_REQ (WUID) -------->|
|<-- LOC_REG_RSP (accept) -------|
|                                 |
```

### 2.6 De-Registration

Sent on power-off or system departure:

```
SU                              RFSS
|                                 |
|--- U_DE_REG_REQ (WUID) ------->|
|<-- U_DE_REG_ACK ---------------|
|                                 |
```

### 2.7 Power-On Sequence (Complete)

```python
def su_power_on_sequence():
    # 1. Control channel acquisition (Section 5)
    cc = acquire_control_channel()  # Short Hunt -> Extended Hunt
    if not cc:
        enter_fallback_search()     # Section 16
        return

    # 2. Verify system identity
    wait_for(NET_STS_BCST)
    wait_for(RFSS_STS_BCST)
    store_system_params()           # WACN, System ID, RFSS ID, Site ID

    # 3. Collect channel identifiers
    collect_iden_up_messages()       # IDEN_UP, IDEN_UP_VU, IDEN_UP_TDMA

    # 4. Full registration
    result = slotted_aloha_send(U_REG_REQ, suid=MY_SUID, esn=MY_ESN)
    if result == U_REG_RSP:
        store_wuid(result.wuid)
        start_timer(T_wuid_validity)
    elif result == DENY_RSP:
        log_denial(result.reason)
        return
    elif result == U_REG_CMD:
        # RFSS wants re-registration with different params
        handle_reg_command(result)
        return

    # 5. Group affiliation
    result = slotted_aloha_send(GRP_AFF_REQ, wgid=SELECTED_TALKGROUP)
    if result == GRP_AFF_RSP:
        store_affiliation(result)

    # 6. Enter idle monitoring
    enter_idle_state()
```

### 2.8 FNE Registration Processing

```python
def fne_process_registration(u_reg_req):
    suid = u_reg_req.suid
    esn = u_reg_req.esn
    emergency = u_reg_req.service_options.emergency

    # Validate SU
    if not validate_suid(suid):
        if emergency:
            # May grant temporary access per Section 11.3.5
            wuid = assign_temp_wuid(suid)
            send_osp(U_REG_RSP(wuid=wuid, status=ACCEPTED))
            notify_consoles_emergency(suid)
            return
        send_osp(DENY_RSP(reason=INVALID_SU))
        return

    # Check if authentication required
    if site_requires_auth():
        initiate_authentication(suid)  # Section 8 / [AACE]

    # Assign WUID
    if is_home_su(suid):
        wuid = suid & 0xFFFFFF  # lower 24 bits
    else:
        wuid = allocate_roaming_wuid(suid)

    # Register in database
    register_su(suid, wuid, rfss_id=MY_RFSS_ID, site_id=MY_SITE_ID)
    start_timer(T_wuid_validity, su=suid)

    # Respond
    send_osp(U_REG_RSP(wuid=wuid, status=ACCEPTED))

    if emergency:
        notify_consoles_emergency(suid)
```

---

## 3. Group Affiliation

### 3.1 Overview

Group affiliation tells the RFSS which talkgroup(s) a SU is monitoring, so the RFSS
can route group calls correctly. Without affiliation, the SU will not receive
GRP_V_CH_GRANT messages for group calls.

**AABC-E messages used:**
- `GRP_AFF_REQ` (ISP 0x04) -- SU requests affiliation
- `GRP_AFF_RSP` (OSP 0x28) -- RFSS accepts/denies affiliation
- `GRP_AFF_Q` (OSP 0x29) -- RFSS queries SU affiliation
- `GRP_AFF_Q_RSP` (ISP 0x05) -- SU responds to affiliation query

### 3.2 Affiliation Flow

```
SU                              RFSS
|                                 |
|--- GRP_AFF_REQ (WGID) -------->|  [WGID = talkgroup to join]
|<-- GRP_AFF_RSP (accept/deny) --|
|                                 |
```

### 3.3 Conditions Triggering Affiliation

| Condition | Notes |
|-----------|-------|
| After successful registration | Immediate affiliation to selected talkgroup |
| User selects new talkgroup | User rotates channel selector |
| Dynamic regrouping command | RFSS reassigns SU to different group |
| After returning from traffic channel | Re-affirm affiliation |
| Periodically per RFSS config | Keep-alive affiliation |
| Enhanced Radio Unit Monitor (group call) | Temporary affiliation to monitor group |

### 3.4 Affiliation Query

The RFSS may query any SU to confirm it is still affiliated:

```
RFSS                            SU
|                                 |
|--- GRP_AFF_Q (to WUID) ------->|
|<-- GRP_AFF_Q_RSP (affiliations)|
|                                 |
```

### 3.5 Group Addressing

| WGID | Type | Description |
|------|------|-------------|
| $0000 | Null group | No group; used by inhibited SUs |
| $0001-$FFFE | Assignable | Normal talkgroups (65,534 values) |
| $FFFF | ALL group | ALL SYSTEM call; all SUs respond |

### 3.6 FNE Affiliation Table

The FNE maintains an affiliation table mapping WUIDs to WGIDs:

```python
class AffiliationTable:
    def __init__(self):
        self.affiliations = {}  # wuid -> set(wgid)
        self.group_members = defaultdict(set)  # wgid -> set(wuid)

    def affiliate(self, wuid, wgid):
        self.affiliations.setdefault(wuid, set()).add(wgid)
        self.group_members[wgid].add(wuid)

    def get_group_members(self, wgid):
        """Return all WUIDs affiliated with this talkgroup."""
        if wgid == 0xFFFF:  # ALL group
            return set(self.affiliations.keys())
        return self.group_members.get(wgid, set())

    def deaffiliate(self, wuid, wgid=None):
        if wgid:
            self.affiliations.get(wuid, set()).discard(wgid)
            self.group_members.get(wgid, set()).discard(wuid)
        else:
            # Remove all affiliations (de-registration)
            for g in self.affiliations.pop(wuid, set()):
                self.group_members.get(g, set()).discard(wuid)
```

---

## 4. Voice Call Setup

### 4.1 Group Voice Call (Section 7.1)

The most common P25 call type. Any affiliated SU can initiate a group call.

**AABC-E messages used:**
- `GRP_V_REQ` (ISP 0x08) -- SU requests group voice
- `GRP_V_CH_GRANT` (OSP 0x00) -- RFSS grants traffic channel
- `GRP_V_GRANT_UPDT` (OSP 0x02) -- implicit grant update (late entry)
- `GRP_V_GRANT_UPDT_EXP` (OSP 0x03) -- explicit grant update
- `QUE_RSP` (OSP 0x03) -- RFSS queues request
- `DENY_RSP` (OSP 0x27) -- RFSS denies request
- `CAN_SRV_REQ` (ISP 0x1F) -- SU cancels call

**Full message sequence:**

```
SOURCE SU           RFSS                    DESTINATION GROUP (all affiliated SUs)
    |                 |                              |
    |--GRP_V_REQ----->|                              |  INBOUND CC
    |                 |  [validate SU, check affil]  |
    |                 |  [assign traffic channel]    |
    |<-GRP_V_CH_GRANT-|---GRP_V_CH_GRANT----------->|  OUTBOUND CC (to all)
    |                 |                              |
    |  [tune to TC    |                              |  [affiliated SUs tune
    |   within T_rec] |                              |   to traffic channel]
    |                 |                              |
    |===VOICE TRAFFIC=|=============VOICE===========>|  TRAFFIC CHANNEL
    |                 |                              |
    |                 |---GRP_V_GRANT_UPDT---------->|  CC (during hangtime,
    |                 |---GRP_V_GRANT_UPDT---------->|   for late entry)
    |                 |                              |
    |--CAN_SRV_REQ--->|  [or T_noact expires]        |
    |                 |---CALL TERMINATION----------->|  TC
    |  [return to CC] |                              |  [return to CC]
```

**Queued call variant:**

```
SOURCE SU           RFSS
    |                 |
    |--GRP_V_REQ----->|
    |<-QUE_RSP--------|  [no channel available; SU queued]
    |                 |  [channel becomes available or lower-priority call preempted]
    |<-GRP_V_CH_GRANT-|  [grant from queue]
```

### 4.2 Announcement Group Call (Section 7.2)

Identical to group voice call but addressed to an Announcement Group (AG).
The AG logically contains N_AG member groups (default 15). All SUs affiliated
with any member group receive the call.

**AABC-E messages:** Same as group voice (GRP_V_REQ, GRP_V_CH_GRANT) but the
WGID field contains the AG address.

### 4.3 Unit-to-Unit Voice Call (Section 7.3)

Private call between two specific SUs. Optionally includes an availability check.

**AABC-E messages used:**
- `UU_V_REQ` (ISP 0x09) -- SU requests U2U call
- `UU_ANS_REQ` (OSP 0x1A) -- RFSS queries destination availability
- `UU_ANS_RSP` (ISP 0x1A) -- destination responds (Wait/Proceed/Deny)
- `UU_V_CH_GRANT` (OSP 0x04) -- RFSS grants traffic channel
- `UU_V_CH_GRANT_UPDT` (OSP 0x05) -- grant update
- `QUE_RSP` (OSP 0x03) -- queued
- `DENY_RSP` (OSP 0x27) -- denied
- `CAN_SRV_REQ` (ISP 0x1F) -- cancel

**Full message sequence (with availability check):**

```
SOURCE SU           RFSS                    DESTINATION SU
    |                 |                          |
    |--UU_V_REQ------>|                          |
    |                 |---UU_ANS_REQ------------>|  [optional avail check]
    |                 |<--UU_ANS_RSP(Wait)-------|  [optional wait]
    |                 |<--UU_ANS_RSP(Proceed)----|
    |<-ACK_RSP_FNE----|                          |  [optional ack to source]
    |<-UU_V_CH_GRANT--|---UU_V_CH_GRANT--------->|
    |                 |                          |
    |===VOICE=========|===========VOICE=========>|  TRAFFIC CHANNEL
    |                 |                          |
    |--CAN_SRV_REQ--->|                          |  [either party can cancel]
    |                 |---CALL TERMINATION------->|
```

**Destination deny variant:**

```
SOURCE SU           RFSS                    DESTINATION SU
    |                 |                          |
    |--UU_V_REQ------>|                          |
    |                 |---UU_ANS_REQ------------>|
    |                 |<--UU_ANS_RSP(Deny)-------|
    |<-DENY_RSP-------|                          |
```

### 4.4 Telephone Interconnect -- Outbound (Section 7.4)

SU calls a PSTN number.

**AABC-E messages used:**
- `TELE_INT_DIAL_REQ` (ISP 0x11) -- explicit dialing with digits
- `TELE_INT_PSTN_REQ` (ISP 0x12) -- implicit/pre-programmed number
- `TELE_INT_V_CH_GRANT` (OSP 0x14) -- channel grant
- `TELE_INT_CH_GRANT_UPDT` (OSP 0x15) -- grant update
- `QUE_RSP` / `DENY_RSP` -- queue/deny

```
SOURCE SU           RFSS                    PSTN
    |                 |                       |
    |--TELE_INT_DIAL_REQ-->|                  |
    |                 |---Call Setup---------->|
    |<-QUE_RSP--------|<--Call Proceeding------|  [optional queue]
    |<-TELE_INT_V_CH_GRANT-|<--Alerting------|
    |                 |<--Answer--------------|
    |===VOICE=========|========VOICE=========>|  TRAFFIC CHANNEL
    |--CAN_SRV_REQ--->|---Release------------>|
```

### 4.5 Telephone Interconnect -- Inbound to Group (Section 7.5)

PSTN caller reaches a talkgroup. No ISP from SU required.

```
PSTN                RFSS                    DESTINATION GROUP
    |                 |                          |
    |--Call Setup---->|                          |
    |<--Alerting------|                          |
    |<--Connect-------|---GRP_V_CH_GRANT-------->|  OUTBOUND CC
    |--Connect Ack--->|                          |
    |===VOICE=========|===========VOICE=========>|  TRAFFIC CHANNEL
    |--Disconnect---->|---CALL TERMINATION------->|
```

### 4.6 Telephone Interconnect -- Inbound to Unit (Section 7.6)

PSTN caller reaches a specific SU.

**AABC-E messages used:**
- `TELE_INT_ANS_REQ` (OSP 0x18) -- RFSS queries SU
- `TELE_INT_ANS_RSP` (ISP 0x18) -- SU responds (Proceed/Deny)
- `TELE_INT_V_CH_GRANT` (OSP 0x14) -- channel grant

```
PSTN                RFSS                    DESTINATION SU
    |                 |                          |
    |--Call Setup---->|---TELE_INT_ANS_REQ------>|
    |<--Alerting------|<--TELE_INT_ANS_RSP(Proceed)--|
    |<--Connect-------|---TELE_INT_V_CH_GRANT--->|
    |--Connect Ack--->|                          |
    |===VOICE=========|===========VOICE=========>|  TRAFFIC CHANNEL
    |--Disconnect---->|---CALL TERMINATION------->|
```

### 4.7 Pseudocode: FNE Group Voice Call Processing

```python
def fne_process_grp_v_req(grp_v_req):
    source_wuid = grp_v_req.source_address
    wgid = grp_v_req.group_address
    svc_opts = grp_v_req.service_options
    emergency = svc_opts.emergency
    priority = svc_opts.priority

    # Validate source SU
    if not is_registered(source_wuid):
        send_osp(DENY_RSP(target=source_wuid, reason=NOT_REGISTERED))
        return

    # Validate group affiliation
    if not is_affiliated(source_wuid, wgid):
        send_osp(DENY_RSP(target=source_wuid, reason=NOT_AFFILIATED))
        return

    # Calculate effective priority
    calc_priority = calculate_priority(
        request_priority=priority,
        group_priority=get_group_priority(wgid),
        emergency=emergency
    )

    # Try to assign a traffic channel
    channel = allocate_traffic_channel()

    if channel:
        # Grant immediately
        grant = GRP_V_CH_GRANT(
            channel=channel.identifier,
            group_address=wgid,
            source_address=source_wuid,
            service_options=svc_opts
        )
        send_osp(grant)  # Goes to ALL SUs on outbound CC
        setup_traffic_channel(channel, wgid, source_wuid, svc_opts)
    else:
        # No channel available
        if emergency:
            # Try preemption
            preempted = preempt_lowest_priority_call(calc_priority)
            if preempted:
                channel = preempted.channel
                grant = GRP_V_CH_GRANT(
                    channel=channel.identifier,
                    group_address=wgid,
                    source_address=source_wuid,
                    service_options=svc_opts
                )
                send_osp(grant)
                setup_traffic_channel(channel, wgid, source_wuid, svc_opts)
                return

        # Queue the request
        queue_service_request(grp_v_req, calc_priority)
        send_osp(QUE_RSP(target=source_wuid, service_type=GROUP_VOICE))

        if emergency:
            notify_consoles_emergency(source_wuid, wgid)
```

---

## 5. Channel Grant Procedures

### 5.1 Implicit vs. Explicit Channel Grants

P25 uses two formats for communicating traffic channel assignments:

**Implicit (abbreviated) grant:**
- Uses a Channel Identifier (4 bits) + Channel Number (12 bits) = 16-bit Channel field
- The SU must already know the channel parameters from IDEN_UP broadcasts
- Used by: `GRP_V_CH_GRANT`, `UU_V_CH_GRANT`, `TELE_INT_V_CH_GRANT`

**Explicit grant:**
- Includes transmit and receive frequencies directly (or full channel parameters)
- Used when SU may not have the channel identifier in its table
- Used by: `GRP_V_GRANT_UPDT_EXP`

### 5.2 Channel Identifier Resolution

Each IDEN_UP message defines parameters for one Channel Identifier (0-15):

| Field | Bits | Description |
|-------|------|-------------|
| Identifier | 4 | Channel identifier value (0x0-0xF) |
| BW | 9 | Channel bandwidth (kHz) |
| Base Frequency | 32 | Base frequency of the channel band (Hz) |
| Transmit Offset | 16 | Signed offset for SU transmit frequency (Hz) |
| Channel Spacing | 10 | Channel spacing (Hz) |

### 5.3 Frequency Calculation

Given a 16-bit channel field from a grant message:

```
Channel Field: [Identifier(4 bits)][Channel Number(12 bits)]
```

**Downlink (RFSS to SU) frequency:**
```
freq_rx = Base_Frequency + (Channel_Spacing * Channel_Number)
```

**Uplink (SU to RFSS) frequency:**
```
freq_tx = freq_rx + Transmit_Offset
```

### 5.4 Pseudocode: Channel Frequency Resolution

```python
class ChannelIdentifierTable:
    def __init__(self):
        self.identifiers = {}  # id -> ChannelParams

    def update_from_iden_up(self, iden_up):
        """Process an IDEN_UP OSP and store channel parameters."""
        params = ChannelParams(
            identifier=iden_up.identifier,
            bw_khz=iden_up.bandwidth,
            base_freq_hz=iden_up.base_frequency * 5,  # 5 Hz steps
            tx_offset_hz=iden_up.transmit_offset * 250000,  # 250 kHz steps (signed)
            spacing_hz=iden_up.channel_spacing * 125  # 125 Hz steps
        )
        self.identifiers[params.identifier] = params

    def resolve_channel(self, channel_field: int) -> tuple:
        """Resolve a 16-bit channel field to (rx_freq_hz, tx_freq_hz).

        Args:
            channel_field: 16-bit value [Identifier(4):ChannelNumber(12)]

        Returns:
            (downlink_freq_hz, uplink_freq_hz) or None if identifier unknown
        """
        iden = (channel_field >> 12) & 0xF
        chan_num = channel_field & 0xFFF

        params = self.identifiers.get(iden)
        if params is None:
            return None  # Must wait for IDEN_UP

        rx_freq = params.base_freq_hz + (params.spacing_hz * chan_num)
        tx_freq = rx_freq + params.tx_offset_hz
        return (rx_freq, tx_freq)
```

### 5.5 TDMA Channel Identifier (IDEN_UP_TDMA)

For TDMA traffic channels, IDEN_UP_TDMA provides additional parameters:

| Field | Description |
|-------|-------------|
| Slots | Number of TDMA slots per channel (1 or 2) |
| Channel Type | FDMA or TDMA indicator |

When the channel type indicates TDMA, the SU must use Phase 2 TDMA procedures
(TIA-102.BBAC) on the traffic channel rather than Phase 1 FDMA.

### 5.6 SDRTrunk Cross-Reference

SDRTrunk resolves channel identifiers in `P25P1DecoderState.java` which maintains
a `ChannelIdentifier` map. When a `GRP_V_CH_GRANT` is received, the channel field
is resolved against this map to determine the actual frequency for the traffic channel
decoder. If the identifier is unknown, SDRTrunk logs a warning and requests the
identifier via `IDEN_UP_REQ`.

---

## 6. Call Maintenance

### 6.1 Voice Channel Supervision

Once a traffic channel is active, the FNE must maintain it:

**FNE responsibilities:**
- Transmit Link Control Words (LCWs) on traffic channel at least once per T_TRAFFIC_LC
  superframes (default 3, max 3). LCW types: `LC_GRP_V_CH_USR` for group calls,
  `LC_UU_V_CH_USR` for U2U calls.
- Monitor inbound traffic channel for voice activity
- Maintain hangtime timer for message trunking
- Send Grant Update messages on control channel during hangtime (for late entry)

**SU responsibilities:**
- Monitor outbound traffic channel for LCWs (verify call identity)
- Return to control channel if T_noact expires (no decodable message for 3 sec)
- Return to control channel on Call Termination data unit
- Return to control channel if assigned to different group's traffic channel

### 6.2 Trunking Methods

| Method | R bit | Behavior |
|--------|-------|----------|
| Message Trunking (Repeater) | R=1 | Channel held during hangtime between PTT presses; Grant Updates sent on CC for late entry |
| Transmission Trunking (Direct) | R=0 | Channel released after each PTT; re-granted for next transmission |

The R bit is communicated in RFSS_STS_BCST. CCC sites always use Direct (R=0).

### 6.3 Hangtime

During message trunking, after the transmitting SU releases PTT:

1. FNE starts hangtime timer (T_hangtime, implementation-defined, typically 2-4 seconds)
2. FNE continues transmitting on traffic channel (idle carrier with LCWs)
3. FNE sends GRP_V_GRANT_UPDT on control channel (enables late entry)
4. If another SU keys up within hangtime, channel is reused without new grant
5. If hangtime expires with no activity, FNE releases traffic channel

### 6.4 Call Preemption

When the RFSS needs to free a traffic channel for a higher-priority call:

**Unforced preemption:**
- RFSS sends Call Termination on the traffic channel
- SUs return to control channel normally
- Used for: queue service, load balancing

**Forced preemption (emergency):**
- RFSS immediately terminates the traffic channel
- "Ruthless" preemption -- no negotiation
- Used for: emergency calls preempting normal calls

**Preemption priority hierarchy:**

| Priority (highest first) | Call Type |
|--------------------------|-----------|
| 1 | Emergency group call (E bit set, priority %111) |
| 2 | Emergency U2U call |
| 3 | Priority %111 non-emergency |
| 4 | Priority %110 |
| 5 | Priority %101 |
| 6 | Priority %100 |
| 7 | Priority %011 |
| 8 | Priority %010 |
| 9 | Priority %001 |
| 10 | Priority %000 (lowest) |

### 6.5 Grant Update Messages

During message trunking hangtime, the FNE sends updates on the control channel:

| Message | When Used | Purpose |
|---------|-----------|---------|
| `GRP_V_GRANT_UPDT` | Hangtime, same system | Late entry; implicit channel |
| `GRP_V_GRANT_UPDT_EXP` | Hangtime, cross-system | Late entry; explicit frequencies, includes E bit |
| `UU_V_CH_GRANT_UPDT` | U2U hangtime | Late entry for U2U calls |
| `TELE_INT_CH_GRANT_UPDT` | Tele interconnect | Late entry for telephone calls |

---

## 7. Emergency Procedures

### 7.1 Overview

Emergency is indicated by the "E" bit (bit 7) in the Service Options field.
Two distinct concepts:

- **Emergency Alarm** -- notification only (no voice call)
- **Emergency Call** -- voice call with emergency priority

### 7.2 Emergency Alarm Flow

**AABC-E messages used:**
- `EMRG_ALRM_REQ` (ISP 0x1C) -- SU sends alarm
- `ACK_RSP_FNE` (OSP 0x20) -- RFSS acknowledges
- `CAN_SRV_REQ` (ISP 0x1F) -- SU cancels alarm

```
SU                              RFSS                    CONSOLE
|                                 |                        |
|--- EMRG_ALRM_REQ ------------->|                        |
|<-- ACK_RSP_FNE -----------------|---Emergency Alert----->|
|                                 |                        |
... (later) ...
|--- CAN_SRV_REQ (cancel alarm)->|                        |
|<-- ACK_RSP_FNE -----------------|---Emergency Clear----->|
```

**Cancellation CAN_SRV_REQ fields:**
- AIV = 1
- Service Type = Emergency Alarm Request opcode (%100111)
- Reason Code = $00
- Additional Info: octet 4 bit 7 = 1 (emergency bit echoed), octets 5-6 = 16-bit group address

### 7.3 Emergency Group Voice Call

Standard group voice call with E bit = true in Service Options of GRP_V_REQ:

```
SU                              RFSS                    GROUP
|                                 |                        |
|--- GRP_V_REQ (E=1) ----------->|                        |
|                                 |  [emergency priority]  |
|                                 |  [may preempt calls]   |
|<-- GRP_V_CH_GRANT (E=1) -------|---GRP_V_CH_GRANT(E=1)->|
|                                 |---Console Alert------->|
|===VOICE (emergency)============>|========================>|
```

**FNE emergency call behavior:**
- If channel available: grant immediately with E=1 in response
- If no channel: queue with emergency priority, may ruthlessly preempt lower-priority call
- Notify dispatch consoles of emergency declaration
- E bit reflected in all traffic channel LCWs (LC_GRP_V_CH_USR)

### 7.4 Emergency During Registration/Affiliation

A SU in emergency mode (E=1) may:
- Register at sites previously marked "site access denied" (Section 11.3.5)
- Affiliate to groups not normally valid at this site (Section 11.3.6)
- Receive temporary access limited to emergency services only

### 7.5 Emergency Cancellation

**Two cancellation methods:**

| Method | Initiated By | Mechanism |
|--------|-------------|-----------|
| Explicit | SU or console | CAN_SRV_REQ with Service Type = Group Call, Additional Info echoing emergency |
| Timer-based | FNE | FNE monitors for E=1 in SU inbound ISPs/LCWs; after FNE timer expires with no E=1 activity, emergency state cleared |

**Important:** A group call emergency cancellation does NOT cancel the SU's internal
emergency state. The SU controls its own internal emergency condition.

### 7.6 Unit-to-Unit Emergency Call

- UU_V_REQ with E=1 in Service Options
- Always granted in the indicated mode
- FNE does NOT maintain emergency state for U2U calls (unlike group calls)
- E bit reflected in call setup messages

### 7.7 Pre-Programmed Emergency Group

A SU may be pre-programmed with an emergency-specific talkgroup different from
its selected talkgroup. When emergency is activated:

1. SU may affiliate to pre-programmed emergency group first
2. If emergency group is not home on current system, SU must affiliate before requesting
3. If SU sends alarm without affiliating first and FNE requires affiliation, FNE denies
   and sends U_REG_CMD to force registration/affiliation

---

## 8. Authentication

### 8.1 Overview

Authentication is optional. When enabled, it verifies SU identity before granting service.
Procedures are defined in [AACE] (TIA-102.AACE); this section covers the trunking control
channel message flows.

**AABC-E messages used:**
- `AUTH_DMD` (OSP 0x2E) -- RFSS demands authentication
- `AUTH_RESP` (ISP 0x0C) -- SU responds (non-mutual)
- `AUTH_RESP_M` (ISP 0x0D) -- SU responds (mutual authentication)
- `AUTH_FNE_RESP` (OSP 0x2F) -- RFSS provides its authentication result (mutual)
- `AUTH_FNE_RST` (ISP 0x0E) -- SU confirms FNE result (mutual)
- `AUTH_SU_DMD` (ISP 0x0B) -- SU initiates authentication

### 8.2 Non-Mutual Authentication

FNE authenticates SU identity (one-way):

```
RFSS                            SU
|                                 |
|--- AUTH_DMD (RAND, RS) -------->|  [RFSS sends random challenge]
|<-- AUTH_RESP (RES) -------------|  [SU computes response using auth key]
|                                 |
|  [RFSS validates RES]           |
|  [if valid: continue service]   |
|  [if invalid: DENY_RSP]        |
```

The RFSS may send AUTH_DMD:
- In response to U_REG_REQ (during registration)
- Whenever the SU is idle on the control channel
- Periodically per RFSS policy

Timeout: T_AUTH = 30 seconds (default). If SU does not respond within T_AUTH,
RFSS may deny service.

### 8.3 Mutual Authentication

Both RFSS and SU authenticate each other:

```
RFSS                            SU
|                                 |
|--- AUTH_DMD (RAND, RS) -------->|  [RFSS challenges SU]
|<-- AUTH_RESP_M (RES1, RAND2) ---|  [SU responds + challenges RFSS]
|                                 |
|  [RFSS validates RES1]          |
|                                 |
|--- AUTH_FNE_RESP (RES2) ------->|  [RFSS responds to SU challenge]
|<-- AUTH_FNE_RST (accept/deny) --|  [SU validates RES2]
|                                 |
```

### 8.4 SU-Initiated Authentication

SU may initiate authentication to verify the RFSS identity:

```
SU                              RFSS
|                                 |
|--- AUTH_SU_DMD (RAND) --------->|  [SU challenges RFSS]
|<-- AUTH_FNE_RESP (RES) ---------|  [RFSS responds]
|                                 |
|  [SU validates RES]             |
|  [if valid: trust RFSS]         |
|  [if invalid: may leave site]   |
```

---

## 9. Roaming

### 9.1 Overview

Roaming allows a SU to operate on systems outside its home WACN/System. The SU
maintains a roaming address stack of authorized WACN+System pairs.

**AABC-E messages used:**
- `ROAM_ADDR_CMD` (OSP 0x2A) -- RFSS manages SU roaming stack
- `ROAM_ADDR_REQ` (ISP 0x16) -- SU reads another SU's stack
- `ROAM_ADDR_RSP` (ISP 0x17) -- SU returns stack contents
- `ROAM_ADDR_UPDT` (OSP 0x29) -- RFSS relays stack contents to requesting SU

### 9.2 Roaming Address Stack Operations

The RFSS manages the SU's roaming stack via ROAM_ADDR_CMD:

| Stack Operation | Action |
|----------------|--------|
| Clear | Erase all erasable entries (read-only entries preserved) |
| Write | Add WACN+System to stack; if full, delete oldest |
| Delete | Remove specific WACN+System from stack |
| Read | SU returns all stack entries via ROAM_ADDR_RSP |

### 9.3 Stack Write Flow

```
RFSS                            SU
|                                 |
|--- ROAM_ADDR_CMD (Write, ------>|
|    WACN=$12345, Sys=$001)       |
|<-- ACK_RSP_U -------------------|
|                                 |
```

The RFSS should wait >= 2 * T_RFSS_RSP before the next stack operation.

### 9.4 Stack Read Flow

```
RFSS                            SU
|                                 |
|--- ROAM_ADDR_CMD (Read) ------->|
|<-- ACK_RSP_U -------------------|
|<-- ROAM_ADDR_RSP (entry 1, LM=0)|  [Home WACN+Sys first]
|--- ACK_RSP_FNE ---------------->|
|<-- ROAM_ADDR_RSP (entry 2, LM=0)|
|--- ACK_RSP_FNE ---------------->|
|<-- ROAM_ADDR_RSP (entry N, LM=1)|  [LM=1 = last message]
|--- ACK_RSP_FNE ---------------->|
```

### 9.5 SU Reading Another SU's Stack

```
SOURCE SU           RFSS                    TARGET SU
    |                 |                          |
    |--ROAM_ADDR_REQ->|                          |
    |<-ACK_RSP_FNE----|                          |
    |                 |---ROAM_ADDR_CMD(Read)---->|
    |                 |<--ACK_RSP_U--------------|
    |                 |<--ROAM_ADDR_RSP----------|
    |                 |---ACK_RSP_FNE----------->|
    |<-ROAM_ADDR_UPDT-|                          |
    |--ACK_RSP_U----->|                          |
    |<-ROAM_ADDR_UPDT(LM=1)--|                   |
    |--ACK_RSP_U----->|                          |
```

### 9.6 Inter-RFSS Roaming Procedure

When a SU moves to a different RFSS (possibly different WACN/System):

1. SU loses control channel on old site (T_ctrl expires)
2. SU performs Short Hunt / Extended Hunt
3. SU acquires new control channel, decodes NET_STS_BCST and RFSS_STS_BCST
4. SU detects different WACN/System/RFSS
5. SU performs full registration (U_REG_REQ) on new RFSS
6. New RFSS assigns WUID from roaming pool (cannot use home WUID)
7. SU affiliates to desired talkgroup
8. New RFSS coordinates with home system via ISSI/CSSI (infrastructure side, outside scope of this spec)

### 9.7 Wildcard Roaming

- WACN = $FFFFF in roaming stack: SU may roam to any network
- System = $FFF in roaming stack: SU may roam to any system within the specified WACN

---

## 10. Data Services

### 10.1 SNDCP Data Channel Request/Grant

Packet data uses dedicated data traffic channels. The access model is similar to
voice channel grants.

**AABC-E messages used:**
- SNDCP Data Channel Announcement (OSP, from AABC section 5.2.7)
- Standard grant/deny/queue messages for data channel assignment

### 10.2 Data Channel Access State Machine

```
                 Tune to
                 Traffic Channel
                      |
                      v
          +--> Frame Sync Seek <--Frame sync search--+
          |          |                               |
    Frame |    Frame sync / monitor                  |
    sync  |          v                               | Return to
    lost/ |     Idle/RX --TC assignment diff unit--> | Control
    Resync|      ^   |                               | Channel
          |      |   | Frame sync search timeout
   Packet |      |   |
   ready  |      v   v
   to     +-> Wait for Status
   transmit      |
                 | Status = IDLE
                 v
           Transmit Packet
                 |
                 | Packet Transmitted
                 v
              (back to Idle/RX)
```

**States:**

| State | Description |
|-------|-------------|
| Frame Sync Seek | Acquiring sync on data traffic channel; timeout = T_data_sync (180 ms) |
| Idle/RX | Monitoring and receiving packets; ready to transmit |
| Wait for Status | Waiting for IDLE status symbol before transmitting |
| Transmit Packet | Sending packet data |

**Exit conditions from Idle/RX (return to control channel):**
- Call Termination received
- Voice activity detected on data channel
- Excessive bit errors or invalid NACs
- T_noact expires

### 10.3 Data Page

The RFSS can page a SU to join a data channel using standard grant mechanisms.
The SU tunes to the data traffic channel and enters the data channel access
state machine above.

### 10.4 Supplementary Data Services (Control Channel)

These services use control channel messaging only (no traffic channel):

| Service | ISP | OSP | Description |
|---------|-----|-----|-------------|
| Call Alert | `CALL_ALRT_REQ` | `CALL_ALRT` | Page/alert a SU |
| Message Update | `MSG_UPDT_REQ` | `MSG_UPDT` | Short pre-programmed message |
| Status Update | `STS_UPDT_REQ` | `STS_UPDT` | Machine-readable status code |
| Status Query | `STS_Q_REQ` | `STS_Q` | Request status from SU |

All follow the pattern: Source SU -> ISP -> RFSS -> OSP -> Target SU -> ACK_RSP_U -> RFSS -> ACK_RSP_FNE -> Source SU.

---

## 11. Timer Definitions

### 11.1 Complete Timer Table

| Timer | Default | Min | Max | Section | Purpose |
|-------|---------|-----|-----|---------|---------|
| **T_SLOT** | 37.5 ms | 37.5 ms | U | 4 | Slotted Aloha inbound slot duration |
| **T_RFSS_RSP** | 500 ms | U | U | 4.1.3 | Max time for RFSS to respond to ISP |
| **T_retry** | Random | 0 | N_random * T_SLOT | 4.1.3 | Random backoff before retry |
| **T_voice_sync** | 180 ms | U | U | 4.2 | Voice frame sync acquisition timeout on TC |
| **T_data_sync** | 180 ms | U | U | 4.2 | Data frame sync acquisition timeout on TC |
| **T_ctrl** | 300 ms | 150 ms | U | 5.6 | No valid NID timeout before CC declared lost |
| **T_rec** | 35 ms | 10 ms | 50 ms | 3.3 | SU tune time to traffic channel |
| **T_noact** | 3 sec | 0 sec | U | 3.2 | Traffic channel inactivity timeout |
| **T_BCST** | 3 sec | 1 sec | 3 sec | 12 | Max interval between broadcast updates |
| **T_TRAFFIC_LC** | 3 SF | 1 SF | 3 SF | 7.1.5 | LCW frequency on traffic channel (superframes) |
| **T_ACK** | 4 sec | 1 sec | 10 sec | 10 | Wait for target unit ACK (supplementary data) |
| **T_wuid_validity** | 4 hours | 4 hours | U | 6.6 | WUID validity period before re-registration |
| **T_AUTH** | 30 sec | U | U | [AACE] | Authentication response timeout |
| **T_ADJ_SITE** | 150 ms | U | U | 3.6.1 | Adjacent site scanning window |
| **T_SYNC_BCST** | U | U | 10 sec | 12.8 | TDMA sync broadcast interval |
| **T_CCC** | 3 SF | U | 9 SF | 14 | CCC RFSS Status Broadcast LCW interval |
| **T_Conv_Fallback_LC** | 10 sec | U | 10 sec | 16 | Max interval between LC_CONV_FALLBACK |
| **T_ASB_UPDT** | U | U | 10 sec | 12.6 | Adjacent status broadcast interval |

### 11.2 Counter Definitions

| Counter | Default | Min | Max | Section | Purpose |
|---------|---------|-----|-----|---------|---------|
| **N_max** | 5 | 1 | 5 | 4 | Max transmissions per access attempt |
| **N_retry** | N_max-1 = 4 | 0 | 4 | 4.1.3 | Max retry attempts |
| **N_random** | 5 | -- | -- | 4.1.3 | Random window size (slots) |
| **N_AG** | 15 | 1 | U | 7.2 | Max member groups per announcement group |
| **N_adj_asb_acc** | 50 | U | U | 12.6 | ASB messages before non-adjacent sites invalidated |
| **N_Conv_Fallback_LC** | 2 | 2 | U | 16 | Missed fallback LCWs before exiting fallback |

### 11.3 Timer Usage by Procedure

| Procedure | Timers Used | Counters Used |
|-----------|-------------|---------------|
| Slotted Aloha access | T_SLOT, T_RFSS_RSP, T_retry | N_max, N_retry, N_random |
| Control channel acquisition | T_ctrl | -- |
| Traffic channel tune | T_rec, T_voice_sync, T_data_sync | -- |
| Traffic channel supervision | T_noact, T_TRAFFIC_LC | -- |
| Registration | T_wuid_validity | -- |
| Supplementary data | T_ACK | -- |
| Authentication | T_AUTH | -- |
| Broadcast rotation | T_BCST, T_ASB_UPDT, T_SYNC_BCST | -- |
| Conventional fallback | T_Conv_Fallback_LC | N_Conv_Fallback_LC |
| CCC mode | T_CCC | -- |

---

## 12. State Machines

### 12.1 SU Master State Machine

The top-level SU state machine governs the overall operating mode:

| Current State | Event | Action | Next State |
|---------------|-------|--------|------------|
| **OFF** | Power on | Begin CC acquisition | CC_HUNT |
| **CC_HUNT** | CC acquired (valid NID + NAC) | Store params, check reg needed | CC_ACQUIRED |
| **CC_HUNT** | Hunt timeout, fallback configured | Search for fallback channel | FALLBACK_SEARCH |
| **CC_ACQUIRED** | System identity confirmed (NET_STS_BCST + RFSS_STS_BCST) | Check registration requirements | REGISTERING |
| **CC_ACQUIRED** | Already registered, WUID valid | Enter idle | IDLE |
| **REGISTERING** | Send U_REG_REQ | Wait for response | REG_WAIT |
| **REG_WAIT** | U_REG_RSP (accept) received | Store WUID, start T_wuid_validity | AFFILIATING |
| **REG_WAIT** | DENY_RSP received | Log denial | CC_HUNT or OFF |
| **REG_WAIT** | U_REG_CMD received | Re-register with new params | REGISTERING |
| **REG_WAIT** | T_RFSS_RSP * N_max expired | Retry exhausted | CC_HUNT |
| **AFFILIATING** | Send GRP_AFF_REQ | Wait for response | AFF_WAIT |
| **AFF_WAIT** | GRP_AFF_RSP (accept) | Store affiliation | IDLE |
| **AFF_WAIT** | GRP_AFF_RSP (deny) | Log denial | IDLE (no group) |
| **AFF_WAIT** | Timeout | Retry or idle | IDLE |
| **IDLE** | GRP_V_CH_GRANT for my group | Tune to TC | ON_TRAFFIC_CH |
| **IDLE** | UU_V_CH_GRANT for my WUID | Tune to TC | ON_TRAFFIC_CH |
| **IDLE** | TELE_INT_V_CH_GRANT for my WUID | Tune to TC | ON_TRAFFIC_CH |
| **IDLE** | User PTT (group call) | Send GRP_V_REQ | CALL_REQUEST |
| **IDLE** | User PTT (U2U call) | Send UU_V_REQ | CALL_REQUEST |
| **IDLE** | User emergency | Send EMRG_ALRM_REQ and/or GRP_V_REQ(E=1) | EMERGENCY |
| **IDLE** | T_wuid_validity expired | Re-register | REGISTERING |
| **IDLE** | T_ctrl expired (CC lost) | Begin hunt | CC_HUNT |
| **IDLE** | EXT_FNCT_CMD (Radio Inhibit) | ACK, go inhibited | INHIBITED |
| **IDLE** | EXT_FNCT_CMD (Radio Detach) | ACK, re-register | REGISTERING |
| **IDLE** | AUTH_DMD received | Send AUTH_RESP | AUTH_WAIT |
| **CALL_REQUEST** | GRP_V_CH_GRANT received | Tune to TC | ON_TRAFFIC_CH |
| **CALL_REQUEST** | QUE_RSP received | Wait for grant | QUEUED |
| **CALL_REQUEST** | DENY_RSP received | Return to idle | IDLE |
| **CALL_REQUEST** | T_RFSS_RSP * N_max expired | Retry exhausted | IDLE |
| **QUEUED** | GRP_V_CH_GRANT received | Tune to TC | ON_TRAFFIC_CH |
| **QUEUED** | DENY_RSP received | Return to idle | IDLE |
| **QUEUED** | Timeout | Return to idle | IDLE |
| **ON_TRAFFIC_CH** | Call Termination DU received | Return to CC | IDLE |
| **ON_TRAFFIC_CH** | T_noact expired | Return to CC | IDLE |
| **ON_TRAFFIC_CH** | CAN_SRV_REQ sent (user end call) | Return to CC | IDLE |
| **INHIBITED** | EXT_FNCT_CMD (Radio Uninhibit) | ACK, resume | IDLE |
| **INHIBITED** | Power cycle | Remain inhibited | INHIBITED |
| **FALLBACK_SEARCH** | Fallback channel found | Enter conventional mode | FALLBACK_OP |
| **FALLBACK_SEARCH** | No fallback found | Return to extended hunt | CC_HUNT |
| **FALLBACK_OP** | Control channel detected | Begin CC acquisition | CC_HUNT |
| **FALLBACK_OP** | N_Conv_Fallback_LC misses | Exit fallback | CC_HUNT |
| **EMERGENCY** | Emergency acknowledged | Continue with E=1 service requests | IDLE (with E flag) |
| **AUTH_WAIT** | AUTH result (pass) | Continue | IDLE |
| **AUTH_WAIT** | AUTH result (fail) or T_AUTH | May be denied service | IDLE |

### 12.2 SU Slotted Aloha Access State Machine

Controls the random access procedure for sending any ISP on the inbound control channel.

| Current State | Event | Action | Next State |
|---------------|-------|--------|------------|
| **IDLE** | ISP to send | Select random slot [0, N_random-1] | WAIT_SLOT |
| **WAIT_SLOT** | Slot timer expires | Transmit ISP | WAIT_RSP |
| **WAIT_RSP** | Valid response received (within T_RFSS_RSP) | Deliver response to caller | IDLE |
| **WAIT_RSP** | T_RFSS_RSP expires, retries < N_retry | Select random T_retry | RETRY_WAIT |
| **WAIT_RSP** | T_RFSS_RSP expires, retries >= N_retry | Report failure | IDLE |
| **RETRY_WAIT** | T_retry expires | Retransmit ISP, increment retry count | WAIT_RSP |

```python
def slotted_aloha_send(isp, max_retries=4):
    """Send an ISP using Slotted Aloha random access.

    Returns the response OSP or None on failure.
    """
    T_SLOT = 0.0375  # 37.5 ms
    N_RANDOM = 5
    T_RFSS_RSP = 0.500  # 500 ms

    for attempt in range(max_retries + 1):
        # Select random slot
        slot = random.randint(0, N_RANDOM - 1)
        wait(slot * T_SLOT)

        # Transmit ISP
        transmit(isp)

        # Wait for response
        response = wait_for_response(timeout=T_RFSS_RSP)
        if response is not None:
            return response

        # Random backoff before retry
        if attempt < max_retries:
            t_retry = random.random() * N_RANDOM * T_SLOT
            wait(t_retry)

    return None  # Access attempt failed
```

### 12.3 FNE Group Voice Call State Machine

Controls the FNE-side processing of a group voice call from request through termination.

| Current State | Event | Action | Next State |
|---------------|-------|--------|------------|
| **IDLE** | GRP_V_REQ received | Validate SU, check group | VALIDATING |
| **VALIDATING** | Valid + channel available | Send GRP_V_CH_GRANT, setup TC | ACTIVE |
| **VALIDATING** | Valid + no channel + emergency | Attempt preemption | PREEMPTING |
| **VALIDATING** | Valid + no channel | Send QUE_RSP | QUEUED |
| **VALIDATING** | Invalid SU or group | Send DENY_RSP | IDLE |
| **PREEMPTING** | Preemption succeeded | Send GRP_V_CH_GRANT | ACTIVE |
| **PREEMPTING** | Preemption failed | Send QUE_RSP | QUEUED |
| **QUEUED** | Channel becomes available | Send GRP_V_CH_GRANT | ACTIVE |
| **QUEUED** | Queue timeout | Send DENY_RSP | IDLE |
| **QUEUED** | CAN_SRV_REQ from source | Remove from queue | IDLE |
| **ACTIVE** | Voice activity on TC | Reset hangtime timer | ACTIVE |
| **ACTIVE** | PTT released (message trunking) | Start hangtime timer, send Grant Updates | HANGTIME |
| **ACTIVE** | PTT released (transmission trunking) | Release TC | IDLE |
| **ACTIVE** | CAN_SRV_REQ received | Send Call Termination on TC | TERMINATING |
| **HANGTIME** | New PTT within hangtime | Cancel hangtime, resume voice | ACTIVE |
| **HANGTIME** | Hangtime expires | Send Call Termination on TC | TERMINATING |
| **HANGTIME** | Higher-priority preemption request | Release TC | TERMINATING |
| **TERMINATING** | TC released | Free channel resources | IDLE |

### 12.4 SU Traffic Channel State Machine

Controls SU behavior on a traffic channel after receiving a grant.

| Current State | Event | Action | Next State |
|---------------|-------|--------|------------|
| **TUNE** | Grant received | Tune receiver within T_rec | SYNC_SEEK |
| **SYNC_SEEK** | Frame sync acquired (within T_voice_sync) | Begin monitoring | RECEIVING |
| **SYNC_SEEK** | T_voice_sync / T_data_sync expires | Return to CC | RETURN_CC |
| **RECEIVING** | LCW matches my group/call | Confirm on correct channel | RECEIVING |
| **RECEIVING** | User PTT pressed | Begin transmitting voice | TRANSMITTING |
| **RECEIVING** | Call Termination DU | Return to CC | RETURN_CC |
| **RECEIVING** | T_noact expired | Return to CC | RETURN_CC |
| **RECEIVING** | Different group assignment | May return to CC | RETURN_CC |
| **TRANSMITTING** | User PTT released | Stop transmitting | RECEIVING |
| **TRANSMITTING** | Call Termination DU | Return to CC | RETURN_CC |
| **RETURN_CC** | Tune to CC | Resume CC monitoring | (exit to master SM) |

### 12.5 SU Registration State Machine

| Current State | Event | Action | Next State |
|---------------|-------|--------|------------|
| **UNREGISTERED** | Power on / system change / WUID expired | Prepare U_REG_REQ | SENDING_REQ |
| **SENDING_REQ** | Send U_REG_REQ via Slotted Aloha | Start T_RFSS_RSP timer | WAIT_RSP |
| **WAIT_RSP** | U_REG_RSP (accepted) | Store WUID, start T_wuid_validity | REGISTERED |
| **WAIT_RSP** | DENY_RSP | Log denial | UNREGISTERED |
| **WAIT_RSP** | U_REG_CMD | Extract new params | SENDING_REQ |
| **WAIT_RSP** | Timeout, retries available | Retry with Slotted Aloha backoff | SENDING_REQ |
| **WAIT_RSP** | Timeout, retries exhausted | Give up | UNREGISTERED |
| **REGISTERED** | T_wuid_validity expires | Need re-registration | SENDING_REQ |
| **REGISTERED** | U_REG_CMD received | RFSS forces re-reg | SENDING_REQ |
| **REGISTERED** | System change detected | New system | UNREGISTERED |
| **REGISTERED** | Move to new RFSS (same system) | Send LOC_REG_REQ | LOC_REG_WAIT |
| **REGISTERED** | Power off | Send U_DE_REG_REQ | DEREGISTERING |
| **LOC_REG_WAIT** | LOC_REG_RSP (accepted) | Update RFSS/site | REGISTERED |
| **LOC_REG_WAIT** | DENY_RSP | May need full re-reg | SENDING_REQ |
| **LOC_REG_WAIT** | Timeout | Retry or full re-reg | SENDING_REQ |
| **DEREGISTERING** | U_DE_REG_ACK received | Shutdown | UNREGISTERED |
| **DEREGISTERING** | Timeout | Shutdown anyway | UNREGISTERED |

### 12.6 Conventional Fallback State Machine

| Current State | Event | Action | Next State |
|---------------|-------|--------|------------|
| **CC_HUNT_FAILED** | Extended hunt exhausted, fallback configured | Scan for fallback channels | FALLBACK_SEARCH |
| **FALLBACK_SEARCH** | LC_CONV_FALLBACK found, Fallback CH ID matches | Use this channel | FALLBACK_ACTIVE |
| **FALLBACK_SEARCH** | LC_CONV_FALLBACK found, Fallback CH ID=0 (wildcard) | Remember, keep scanning | FALLBACK_SEARCH |
| **FALLBACK_SEARCH** | Scan exhausted, wildcard found | Use wildcard channel | FALLBACK_ACTIVE |
| **FALLBACK_SEARCH** | Scan exhausted, no match | Return to extended hunt | CC_HUNT |
| **FALLBACK_ACTIVE** | Operating conventional, monitoring LC_CONV_FALLBACK | Normal conventional repeater mode | FALLBACK_ACTIVE |
| **FALLBACK_ACTIVE** | N_Conv_Fallback_LC missed (2 consecutive) | Exit fallback, start CC hunt | CC_HUNT |
| **FALLBACK_ACTIVE** | Control channel detected on this channel | Exit fallback | CC_HUNT |
| **FALLBACK_ACTIVE** | User requests scan | May leave to search | CC_HUNT |

---

## 13. Composite Control Channel (CCC) Considerations

### 13.1 CCC State Transitions

CCC sites operate the control channel in two mutually exclusive modes:

| CCC State | CC Behavior | TC Behavior |
|-----------|-------------|-------------|
| Control Channel State | Normal CC: broadcasts + directed messages | N/A -- channel is CC |
| Traffic Channel State | No OSPs transmitted | Normal TC: voice/data + LCWs |

**Detection:** Composite mode bit in RFSS_STS_BCST (System Service Class, bit 0)
and LC_RFSS_STS_BCST on traffic channel.

### 13.2 CCC-Specific Rules

- Single-channel CCC: NO update messages (GRP_V_GRANT_UPDT) since CC becomes TC
- Assignment messages may be repeated to reduce missed grants
- Only Direct trunking method (R=0) supported
- Local RF request queuing not supported in single-channel mode
- SUs must NOT send ISPs while CCC is in Traffic Channel State
- Late entry only possible via traffic channel LCWs (not CC grant updates)

### 13.3 SU Behavior on CCC

When SU detects CCC mode:
1. Expect periods where CC is unavailable (TC state)
2. Do not interpret TC signaling as CC loss
3. Wait for CCC to return to CC state before sending registration/affiliation
4. May late-enter calls via traffic channel LCWs
5. Confirm CC is present before transmitting service requests

---

## 14. Extended Functions

### 14.1 Radio Check / Inhibit / Uninhibit / Detach

All extended functions use EXT_FNCT_CMD (OSP) and EXT_FNCT_RSP (ISP):

**AABC-E messages used:**
- `EXT_FNCT_CMD` (OSP 0x24) -- RFSS commands extended function
- `EXT_FNCT_RSP` (ISP 0x24) -- SU responds

| Function | Effect | Persistence |
|----------|--------|-------------|
| Radio Check | Verify SU is active | Transient |
| Radio Inhibit | Disable SU (appears powered off) | Survives power cycle |
| Radio Uninhibit | Restore inhibited SU | Permanent until re-inhibited |
| Radio Detach | Cancel registration | Until SU re-registers |

**Radio Inhibit flow:**

```
RFSS                            SU
|                                 |
|--- EXT_FNCT_CMD (Inhibit) ----->|
|<-- EXT_FNCT_RSP (ACK) ---------|  [SU goes dark]
|--- ACK_RSP_FNE ---------------->|  [confirms inhibit active]
|                                 |
```

**Inhibited SU behavior:**
- Blank all displays, no audio/visual indication
- Ignore all user input
- Still responds to: system broadcasts, EXT_FNCT_CMD, registration messages, OTAR
- Survives power cycling (stored in NVM)
- Only cleared by Radio Uninhibit or field reprogramming

---

## 15. SDRTrunk and OP25 State Machine Cross-Reference

### 15.1 SDRTrunk Architecture

SDRTrunk (Java, https://github.com/DSheirer/sdrtrunk) implements passive SU-side monitoring.
Key state machine mappings:

| This Spec Section | SDRTrunk Class | Notes |
|-------------------|----------------|-------|
| 12.1 SU Master SM | `P25P1DecoderState` | Main decoder state; transitions between CC monitoring and TC following |
| 12.4 TC SM | `P25TrafficChannelManager` | Manages TC decoder instances; spawns/tears down on grant/termination |
| 5 CC Acquisition | `P25P1DecoderState.processMessage()` | Processes NET_STS_BCST, RFSS_STS_BCST for system identity |
| 5.2 Channel ID Resolution | `P25P1DecoderState` ChannelIdentifier map | Maintains IDEN_UP table for frequency resolution |
| 7.1 Group Voice | `P25TrafficChannelManager.processChannelGrant()` | On GRP_V_CH_GRANT, creates TC decoder at resolved frequency |
| 12.6 ADJ_STS_BCST | `P25P1DecoderState` adjacent site tracking | Stores adjacent site info for display |

SDRTrunk does NOT implement:
- ISP transmission (it is receive-only)
- Registration or affiliation (passive monitoring)
- Authentication
- CAN_SRV_REQ or any call termination initiation

### 15.2 OP25 Architecture

OP25 (Python + GNU Radio, https://github.com/boatbod/op25) implements passive monitoring:

| This Spec Section | OP25 Module | Notes |
|-------------------|-------------|-------|
| 12.1 SU Master SM | `trunking.py` | Main trunking state machine |
| 5 CC Acquisition | `rx.py` | Frequency lock and NID/NAC validation |
| 7.1 Group Voice | `trunking.py` voice_grant handler | Tunes to TC on grant, returns to CC on termination |
| 5.2 Channel ID | `trunking.py` channel ID table | Resolves channel fields to frequencies |
| 12 Broadcast processing | `trunking.py` TSBK dispatch | Processes all broadcast OSPs |

OP25 does NOT implement:
- ISP transmission
- Registration/affiliation
- Authentication
- FNE-side procedures

### 15.3 Implementation Gap Analysis

Neither SDRTrunk nor OP25 implements the FNE (infrastructure) side. The following
procedures from this spec have no known open-source implementation as of April 2026:

- FNE outbound message scheduling (Section 1.2-1.4 of this spec)
- FNE registration processing and WUID assignment
- FNE call grant/queue/preemption logic
- FNE emergency processing
- FNE authentication demand
- FNE roaming address stack management
- CCC state transitions (FNE side)
- Traffic channel supervision and hangtime management

---

## 16. Gaps and Missing Information

### 16.1 Content Confirmed Present in Extraction

The 91K full text extraction covers all major sections (1-18, Annexes A-H) of
TIA-102.AABD-B base revision. The following are well-represented:

- Random access procedures (Section 4) -- complete
- Control channel acquisition (Section 5) -- mostly complete; signal quality monitoring parameters manufacturer-set
- Registration procedures (Section 6) -- complete
- Voice service procedures (Section 7) -- complete
- Data service procedures (Section 8) -- summary level
- Supplementary data (Sections 9-10) -- complete
- Emergency procedures (Section 11) -- complete
- System status procedures (Section 12) -- complete
- Extended functions (Section 13) -- complete
- CCC procedures (Section 14) -- complete
- Protection/authentication (Section 15) -- abbreviated (link layer content deleted)
- Conventional fallback (Section 16) -- complete
- Parameter values (Section 17) -- complete table
- Priority procedures (Section 18) -- complete
- Annexes A-H -- mostly complete

### 16.2 Known Gaps

| Area | What Is Missing | Impact |
|------|----------------|--------|
| **Diagrams (Figures 5-1 through 7-x)** | Original spec contains state machine diagrams as figures; extraction has text descriptions but may miss transition nuances captured only in figures | State machine tables in Section 12 are reconstructed from text; some edge cases may differ from normative figures |
| **Section 8 (Data Services)** | Only summary-level detail; SNDCP specifics are in AABC-E | Data channel grant/access procedures are partial |
| **Section 15 (Protection)** | Link layer encryption deleted from standard ("under development") | No impact -- this content is intentionally absent from the normative standard |
| **Section 13.6 (Alert Tones)** | "Under development" in the standard itself | No implementation possible |
| **Amendments B-1, B-2, B-3** | Not incorporated in extraction (base 2014 revision only) | May miss refinements added 2023-2025; implementers should review amendments |
| **ISSI/CSSI inter-system** | Infrastructure-to-infrastructure roaming coordination | Outside scope of this air interface spec; defined in TIA-102.BACA |
| **Exact authentication algorithms** | Defined in [AACE], not in this document | Authentication message flows are specified here; crypto algorithms in AACE |
| **Hangtime duration** | Not specified in standard (implementation-defined) | FNE vendors choose hangtime; typical values 2-4 seconds |
| **Queue timeout** | Not specified in standard (implementation-defined) | FNE vendors choose queue timeout |
| **Dynamic regrouping details** | Section 7.x references [AABC] for format details | Message format in AABC-E; procedure is "affiliate to commanded group" |

### 16.3 Amendment Notes

The base 2014 revision has three amendments that may modify procedures:
- **B-1 (Dec 2023)** and **B-2 (Jul 2023)**: Likely contain refinements to emergency procedures, CCC, and enhanced radio unit monitor based on TIA committee work
- **B-3 (Feb 2025)**: Most current; may incorporate lessons learned from field deployments

Implementers should obtain and apply all three amendments for a fully current implementation.

---

## 17. Quick Reference: ISP/OSP Message Cross-Reference by Procedure

| Procedure | ISPs (SU -> RFSS) | OSPs (RFSS -> SU) |
|-----------|-------------------|-------------------|
| **Registration** | U_REG_REQ | U_REG_RSP, U_REG_CMD, DENY_RSP |
| **Location Registration** | LOC_REG_REQ | LOC_REG_RSP, DENY_RSP |
| **De-Registration** | U_DE_REG_REQ | U_DE_REG_ACK |
| **Group Affiliation** | GRP_AFF_REQ | GRP_AFF_RSP, DENY_RSP |
| **Affiliation Query** | GRP_AFF_Q_RSP | GRP_AFF_Q |
| **Group Voice Call** | GRP_V_REQ, CAN_SRV_REQ | GRP_V_CH_GRANT, GRP_V_GRANT_UPDT, GRP_V_GRANT_UPDT_EXP, QUE_RSP, DENY_RSP |
| **U2U Voice Call** | UU_V_REQ, UU_ANS_RSP, CAN_SRV_REQ | UU_ANS_REQ, UU_V_CH_GRANT, UU_V_CH_GRANT_UPDT, QUE_RSP, DENY_RSP |
| **Tele Interconnect (Out)** | TELE_INT_DIAL_REQ, TELE_INT_PSTN_REQ, CAN_SRV_REQ | TELE_INT_V_CH_GRANT, TELE_INT_CH_GRANT_UPDT, QUE_RSP, DENY_RSP |
| **Tele Interconnect (In)** | TELE_INT_ANS_RSP | TELE_INT_ANS_REQ, TELE_INT_V_CH_GRANT |
| **Emergency Alarm** | EMRG_ALRM_REQ, CAN_SRV_REQ | ACK_RSP_FNE, DENY_RSP |
| **Call Alert** | CALL_ALRT_REQ, ACK_RSP_U | CALL_ALRT, ACK_RSP_FNE, DENY_RSP |
| **Message Update** | MSG_UPDT_REQ, ACK_RSP_U | MSG_UPDT, ACK_RSP_FNE, DENY_RSP |
| **Status Update** | STS_UPDT_REQ, ACK_RSP_U | STS_UPDT, ACK_RSP_FNE, DENY_RSP |
| **Status Query** | STS_Q_REQ, STS_Q_RSP | STS_Q, STS_UPDT, ACK_RSP_FNE, DENY_RSP |
| **Radio Unit Monitor** | RAD_MON_REQ, ACK_RSP_U | RAD_MON_CMD, ACK_RSP_FNE, DENY_RSP |
| **Enhanced Radio Monitor** | RAD_MON_ENH_REQ, ACK_RSP_U | RAD_MON_ENH_CMD, ACK_RSP_FNE, DENY_RSP |
| **Authentication (FNE-init)** | AUTH_RESP, AUTH_RESP_M, AUTH_FNE_RST | AUTH_DMD, AUTH_FNE_RESP |
| **Authentication (SU-init)** | AUTH_SU_DMD | AUTH_FNE_RESP |
| **Roaming** | ROAM_ADDR_REQ, ROAM_ADDR_RSP, ACK_RSP_U | ROAM_ADDR_CMD, ROAM_ADDR_UPDT, ACK_RSP_FNE |
| **Extended Functions** | EXT_FNCT_RSP | EXT_FNCT_CMD, ACK_RSP_FNE |
| **Channel ID Request** | IDEN_UP_REQ | IDEN_UP, IDEN_UP_VU, IDEN_UP_TDMA, ACK_RSP_FNE, DENY_RSP |
| **Broadcasts (periodic)** | -- | NET_STS_BCST, RFSS_STS_BCST, IDEN_UP, IDEN_UP_TDMA, ADJ_STS_BCST, SCCB, SYS_SRV_BCST, SYNC_BCST |
