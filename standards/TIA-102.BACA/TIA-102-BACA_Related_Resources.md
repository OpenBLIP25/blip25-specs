# TIA-102.BACA-B Related Resources and Implementation Context

**Document:** TIA-102.BACA-B — P25 ISSI Messages and Procedures for Voice and Mobility  
**Published:** November 2012  
**Interface:** ISSI (Inter-RF Subsystem Interface)

---

## Standards Dependencies

### Normative References (Required for Implementation)

| Reference | Document | Role |
|-----------|----------|------|
| RFC 3261 | SIP: Session Initiation Protocol | Base SIP protocol; all SIP behavior |
| RFC 3264 | An Offer/Answer Model with SDP | Vocoder mode negotiation |
| RFC 3550 | RTP: A Transport Protocol for Real-Time Applications | Audio transport |
| RFC 3551 | RTP Profile for Audio and Video | RTP profile baseline |
| TIA-102.AAAB | P25 FDMA Common Air Interface | Air interface frame formats |
| TIA-102.AAAC | IMBE Vocoder | Full Rate vocoder codec |
| TIA-102.AACA | OTAR Messages and Procedures | Console OTAR/key management |
| TIA-102.BAEB | Data Host Interface (Ed) | Console data host interface |
| TIA-102.BACD | Supplementary Data Transport | Console as Serving RFSS for data |

### Addenda to TIA-102.BACA-B

| Addendum | Title | Notes |
|----------|-------|-------|
| TIA-102.BACA-B-1 | ISSI Addendum 1 | Corrections/additions to base spec |
| TIA-102.BACA-B-2 | ISSI Addendum 2 | Corrections/additions to base spec |
| TIA-102.BACA-B-3 | ISSI — Interworking with IWF | Adds IWF (Interworking Function) support (2021) |

### Related ISSI/CSSI Documents

| Document | Title | Notes |
|----------|-------|-------|
| TIA-102.BACE | ISSI Messages — Conventional Operations | Conventional (non-trunked) ISSI |
| TIA-102.BACF | ISSI Messages — Packet Data Services | Data over ISSI |
| TIA-102.BAAA | Console Subsystem Interface Overview | CSSI architecture overview |
| TIA-102.BAAC | CSSI Messages and Procedures | Console interface procedures |

---

## Protocol Stack

```
Application Layer:
  P25 Call Control (UCCF, GCCF, MMF, SMF, RCPF)
  X-TIA-P25-ISSI header / X-TIA-P25-SNDCP header

Signaling:
  SIP/2.0 (RFC 3261)
  SDP (RFC 4566 / RFC 3264 offer/answer)

Audio Transport:
  RTP (RFC 3550)
  P25 Payload Types: PT 100 (FullRate), PT 101 (HalfRate), PT 102 (Native)

Network:
  UDP/IP
```

---

## Key Data Structures for Implementation

### SIP Address Space

```
P25 SIP URI: sip:[radical-name@]rfss-id.system-id.wacn-id.p25dr

Field widths:
  wacn-id:   5 hex digits (20 bits)
  system-id: 3 hex digits (12 bits)
  rfss-id:   2 hex digits (8 bits)

Example: sip:99999111aaaaaa@22.222.99999.p25dr
  WACN    = 99999
  System  = 111
  RFSS    = 22 (serving)
  SUID    = 99999111aaaaaa (radical name)
```

### RTP Packet Structure

```
[RTP Header (12 bytes minimum, RFC 3550)]
  Version=2, Padding, Extension, CC, Marker, PT, Sequence, Timestamp, SSRC

[P25 Block(s)]
  Block Type (7 bits) | Reserved (1 bit) | Block Length (8 bits) | Data...

Block Types:
  0  = Full Rate Voice (IMBE frame)
  1  = Packet Type (control metadata)
  5  = Full Rate ISSI Header Word
  11 = RF PTT Control Word
  15 = Console PTT Control Word
  33 = Half Rate ISSI Header Word
  34 = Half Rate Voice (AMBE+2 frame)
  35 = TalkSpurt Vocoder Mode
  63-127 = Manufacturer Specific
```

### PTT Floor Control State Machine

```
States: IDLE | REQUESTING | GRANTED | ACTIVE | RELEASING

Transitions (Serving RFSS perspective):
  IDLE      --[SU keys up]--> TX_REQUEST(0) sent --> REQUESTING
  REQUESTING --[TX_GRANT(1)]--> GRANTED
  REQUESTING --[TX_DENY(2)] --> IDLE
  REQUESTING --[PTT_WAIT(6)]--> WAITING (back to REQUESTING on timer)
  GRANTED   --[PTT_START(8)]--> ACTIVE (begin transmitting audio)
  ACTIVE    --[SU unkeys]   --> TX_END (implicit via RTP end) --> RELEASING
  ACTIVE    --[PTT_END(9)]  --> IDLE
  * --[HEARTBEAT_QUERY(10)] --> send heartbeat response
```

### SSRC Assignment (Fixed Values)

```
Group Calls:
  SSRC=1  Home RFSS → Serving RFSS (downlink audio)
  SSRC=2  Serving RFSS → Home RFSS (uplink audio)

SU-to-SU Calls:
  SSRC=16  Calling Serving → Calling Home
  SSRC=14  Calling Home → Calling Serving
  SSRC=13  Called Home → Called Serving
  SSRC=11  Called Serving → Called Home
  SSRC=12  Cross-segment (Calling Serving → Called Serving via Calling Home path)
```

### Vocoder Mode Negotiation Logic

```
Preference order (highest first):
  Native (PT 102) > HalfRate (PT 101) > FullRate (PT 100)

SDP offer must list modes in preference order (highest PT first in m-line).

Accept first common mode between offer and answer.
If no common mode: respond 606 Not Acceptable.

Rate conversion:
  Serving RFSS: can convert between air-interface mode and ISSI mode
  Called Home RFSS: can convert between call segments
  RC capability advertised in X-TIA-P25-ISSI header parameters
```

---

## Call Topology Reference

### SU-to-SU: 4-RFSS Case

```
Calling SU --- [Calling Serving RFSS]
                       |
                  SIP Segment 1
                  RTP (SSRC 16/14)
                       |
               [Calling Home RFSS]
                       |
                  SIP Segment 2
                       |
               [Called Home RFSS]
                       |
                  SIP Segment 3
                  RTP (SSRC 13/11)
                       |
               [Called Serving RFSS] --- Called SU
```

### Group Call: Star Topology

```
[Serving RFSS-1] --- SIP Segment (SSRC 2/1) --\
[Serving RFSS-2] --- SIP Segment (SSRC 2/1) ----> [Home RFSS] (GCCF + MMF)
[Serving RFSS-3] --- SIP Segment (SSRC 2/1) --/
```

---

## Implementation Checklist

### SIP Stack Requirements

- [ ] RFC 3261 full compliance (transaction layer, dialog layer, UA behavior)
- [ ] Via header generation with unique branch parameters (z9hG4bK prefix)
- [ ] Route header processing for RFSS role identification
- [ ] X-TIA-P25-ISSI header parsing and generation (40+ parameters)
- [ ] X-TIA-P25-SNDCP header parsing and generation (21 parameters)
- [ ] SDP offer/answer with P25 payload types (PT 100, 101, 102)
- [ ] Re-INVITE handling for group calls (not for SU-to-SU)
- [ ] All SIP methods: INVITE, CANCEL, BYE, ACK, REGISTER, OPTIONS
- [ ] All response codes in Tables 45, 46, 47, and Annex A Table 57
- [ ] GEN_ prefix generic responses with P25 condition codes

### RTP/Media Requirements

- [ ] RFC 3550 RTP implementation
- [ ] Fixed SSRC values per role (Section 6 SSRC table)
- [ ] P25 block type parsing (all 8 defined types)
- [ ] Full Rate frame format (18 types, $C2-$73 range)
- [ ] Half Rate frame format (6 types, $01-$06 range)
- [ ] PTT floor control protocol (11 packet types)
- [ ] Heartbeat mechanism (Theartbeat=10s)
- [ ] Talkspurt boundary detection (TEndLoss=200ms, TFirstPacketTime=500ms)

### Call Control Requirements

- [ ] UCCF: SU-to-SU originating and destination state machines
- [ ] GCCF: Group call originating and destination state machines
- [ ] MMF: Group audio aggregation and mixing (Home RFSS)
- [ ] SMF: Group audio handling (Serving RFSS)
- [ ] RCPF: OPTIONS-based capability polling
- [ ] In-call roaming (c-icr=1 flag, BYE/REGISTER/INVITE sequence)
- [ ] Colliding call resolution (higher SUID hex wins)
- [ ] Confirmed group call (RF resource availability, Tgchstartconfirmed=5s)
- [ ] All timers from Annex B Table 58

### Mobility Requirements

- [ ] REGISTER (unit registration): Expires != 0
- [ ] REGISTER (query): specific query form
- [ ] REGISTER (deregister): Expires=0
- [ ] Nine mobility object classes

### Console Support Requirements (Annex I)

- [ ] Console subsystem as RFSS type
- [ ] Group registration for dispatch groups
- [ ] Console priority over mobile transmissions
- [ ] Console floor takeover from lower-priority console
- [ ] Transmission source type indication (mobile vs. console)
- [ ] Speaker mute for adjacent consoles (feedback prevention)
- [ ] Duplicate audio detection in patch groups

---

## Common Implementation Pitfalls

1. **SSRC values are fixed, not random**: Unlike standard RTP, P25 ISSI uses fixed SSRC values by role. Generating random SSRCs (per RFC 3550 default) will cause interoperability failures.

2. **Re-INVITE is group-call only**: SU-to-SU calls must never use Re-INVITE. Vocoder mode negotiation for SU-to-SU requires CANCEL + new INVITE if the initial offer is declined.

3. **Three independent SIP dialogs for SU-to-SU**: Each of the three call segments has its own Call-ID, CSeq space, and dialog state. They are NOT the same SIP dialog.

4. **Route header tag identifies role, not routing**: The TIA-P25-U2Uorig/U2Udest/GroupCall tags in the Route header are role identifiers used by the receiving RFSS (see Table 44), not standard SIP routing headers.

5. **Floor control is in-band RTP**: PTT packet types are carried inside RTP packets (Block Type 11 = RF PTT Control Word, Block Type 15 = Console PTT Control Word), not in separate SIP messages.

6. **In-call roaming requires atomic state transfer**: When ICR occurs, the Home must atomically switch from the old to new Serving RFSS call segment. The BYE to old Serving, REGISTER(Expires=0) to old Serving, and new INVITE to new Serving must be coordinated to prevent audio gaps.

7. **Confirmed group calls block on RF confirmation**: The c-rf-resource parameter triggers a blocking wait (up to Tgchstartconfirmed=5s) at the Home RFSS for RF channel confirmations. Missing this wait will cause confirmed calls to be granted without RF resource verification.

---

## Rust Implementation Notes

Target crate: `p25_issi`

Suggested module structure:
```
p25_issi/
  src/
    sip/          # SIP message construction and parsing
      headers.rs  # X-TIA-P25-ISSI, X-TIA-P25-SNDCP, Route tag parsing
      methods.rs  # INVITE, REGISTER, OPTIONS, BYE, CANCEL, ACK builders
      response.rs # Response code enumerations (Tables 45-47, Annex A)
    rtp/          # RTP packet handling
      blocks.rs   # P25 block type definitions and serialization
      ssrc.rs     # Fixed SSRC value constants by role
      ptt.rs      # PTT floor control packet types (11 types)
      codec.rs    # Payload type constants (PT 100, 101, 102)
    call/         # Call control state machines
      uu.rs       # SU-to-SU call (UCCF): originating + destination FSMs
      group.rs    # Group call (GCCF + MMF + SMF): originating + destination FSMs
      roaming.rs  # In-call roaming procedures
    mobility/     # Unit registration (REGISTER variants)
      register.rs # Register, Query, Deregister message builders
    polling/      # RFSS capability polling (RCPF)
      options.rs  # OPTIONS request/response with retry logic
    timers.rs     # All timer constants from Annex B Table 58
    address.rs    # P25 SIP URI construction (wacn.system.rfss.p25dr format)
```

Key Rust types to define:
```rust
// P25 SIP address components
pub struct P25Address {
    pub wacn: u32,     // 20 bits
    pub system_id: u16, // 12 bits
    pub rfss_id: u8,   // 8 bits
    pub suid: Option<u64>, // subscriber unit ID
    pub tgid: Option<u32>, // talk group ID
}

// SSRC role enumeration
pub enum SsrcRole {
    GroupServing,  // = 2
    GroupHome,     // = 1
    UuCallServing, // values 11-16 by direction
}

// PTT packet type
pub enum PttPacketType {
    TxRequest = 0,
    TxGrant = 1,
    TxDeny = 2,
    TxProgress = 3,
    TxMute = 4,
    TxUnmute = 5,
    PttWait = 6,
    PttTransmit = 7,
    PttStart = 8,
    PttEnd = 9,
    HeartbeatQuery = 10,
}

// RTP block type
pub enum BlockType {
    FullRateVoice = 0,
    PacketType = 1,
    FullRateIssiHeader = 5,
    RfPttControl = 11,
    ConsolePttControl = 15,
    HalfRateIssiHeader = 33,
    HalfRateVoice = 34,
    TalkspurtVocoderMode = 35,
}
```

---

## Version History

| Version | Date | Key Changes |
|---------|------|-------------|
| TIA-102.BACA (original) | ~2005 | Initial ISSI voice/mobility spec |
| TIA-102.BACA-A | ~2008 | First revision |
| TIA-102.BACA-B | 2012-11 | This document; comprehensive revision |
| TIA-102.BACA-B-1 | Post-2012 | Addendum 1 corrections |
| TIA-102.BACA-B-2 | Post-2012 | Addendum 2 corrections |
| TIA-102.BACA-B-3 | 2021-01 | Addendum 3: IWF interworking |
| TIA-102.BACA-C | (draft at time of publication) | Next major revision |
