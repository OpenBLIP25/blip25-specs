# TIA-102.BACE — Related Resources and Context

**Document:** TIA-102.BACE — Inter RF Subsystem Interface (ISSI) for Conventional
Operation: Messages and Procedures  
**Generated:** 2026-04-13

---

## Status

**Active.** This document was approved in June 2008 and reaffirmed in January 2013.
No superseding document is known; reaffirmation indicates the document remains
normative without change. It is listed as an active P25 TIA standard in the Project
25 Technology Interest Group (PTIG) approved standards list.

This document is part of a set of conventional ISSI specifications. It does not
stand alone: it requires TIA-102.BACA for shared definitions, TIA-102.BAHA for the
CAR-to-station interface, and TIA-102.BAAA-A for air interface details.

---

## Standards Family

This document belongs to the TIA-102 B-series (Infrastructure) within the
TIA-102 (Project 25) suite.

### Standards Lineage (ASCII Tree)

```
TIA-102 (Project 25 Standards Suite)
├── A-Series: Common Air Interface (CAI)
│   ├── TIA-102.BAAA-A  Common Air Interface (CAI) — P25 Phase 1
│   ├── TIA-102.AABF-A  Link Control Word Formats
│   ├── TIA-102.AABC-B  Trunking Control Channel (TSBK) Messages
│   └── TIA-102.AABG    Short Data Messages
│
├── B-Series: Infrastructure Interfaces
│   ├── TIA-102.BACA    Inter RF Subsystem Interface — Trunking (ISSI trunking)
│   │                   [parent/sibling specification; shares block type 5]
│   ├── TIA-102.BACE ◄── THIS DOCUMENT
│   │                   Inter RF Subsystem Interface — Conventional Operation (Ec)
│   ├── TIA-102.BAHA    Fixed Station Interface (FSI)
│   │                   [CAR-to-fixed-station protocol; defines SSRC assignment]
│   ├── TIA-102.BAAD-B  Conventional Procedures (call procedures on CAI)
│   └── TIA-102.BBAD    Console/Subscriber Interoperability (CSSI — trunking)
│
└── C-Series: Measurement and Conformance
    └── (various measurement and conformance test specs)
```

### Position in the P25 Architecture

The conventional ISSI (Ec, this document) interconnects:

```
┌─────────────────────┐         Ec (TIA-102.BACE)        ┌──────────────────────┐
│  Console Subsystem  │ ◄──── IP / RTP / UDP ──────────► │  Conventional Access  │
│  (dispatch console) │                                   │  Radio (CAR)          │
└─────────────────────┘                                   └──────────┬───────────┘
                                                                     │
                                                          FSI (TIA-102.BAHA)
                                                                     │
                                                          ┌──────────▼───────────┐
                                                          │  Fixed Station (FS)  │
                                                          │  (base station/      │
                                                          │   repeater)          │
                                                          └──────────────────────┘
```

The Console Subsystem connects to the CAR once per fixed station it wishes to
monitor or control. Multiple Console Subsystems may connect to the same CAR for
the same fixed station simultaneously.

### Companion Documents

| Document          | Relationship                                                    |
|-------------------|-----------------------------------------------------------------|
| TIA-102.BACA      | Trunking ISSI; parent spec from which Ec is derived. Block type 5 (Conventional Header Word) is a direct copy of the BACA ISSI Header Word. |
| TIA-102.BAHA      | FSI; the CAR-to-fixed-station protocol. Defines the SSRC identifier the CAR uses, and controls the CAR's interaction with the fixed station hardware. |
| TIA-102.BAAA-A    | P25 Common Air Interface. Defines IMBE frame structure, link control code words (RS encoding), encryption sync words, low speed data, voice header format. |
| TIA-102.AABF-A    | Link Control Word formats. Defines LCO values and payloads carried in Block Type 19 (Conventional Link Control Word). LCO=0 (Group Voice Channel User) and LCO=3 (Unit to Unit Voice Channel User) are typical conventional uses. |
| TIA-102.AABC-B    | TSBK (Trunking Signaling Block) messages. Defines Single Block Control message opcodes and payloads referenced by Block Type 24. |
| TIA-102.AABG      | Short data messages. Additional SBC message definitions used in Block Type 24. |
| TIA-102.BAAD-B    | Conventional Procedures specification. Describes call procedures on the CAI for conventional channels; relevant context for CAR behavior at the RF interface. |
| RFC 3550          | RTP: A Transport Protocol for Real-Time Applications. The transport layer for all Ec messages. |
| RFC 3551          | RTP Profile for Audio and Video Conferences with Minimal Control. Defines PCMU (μ-law) payload type used for PCM audio blocks. |
| ITU-T G.711       | PCM of voice frequencies. μ-law encoding used by PCM Audio block (Block Type 0, E=0). |

---

## Practical Context

### Real-World Use

This specification is implemented in P25 dispatch console systems and in the
infrastructure equipment (CARs) that interface those consoles to conventional
P25 repeater networks. Practical deployment scenarios include:

- **Multi-position dispatch centers:** Multiple dispatcher workstations simultaneously
  monitoring the same conventional channel. Each dispatcher's console subsystem
  maintains its own Ec connection to the CAR serving the fixed station. The CAR
  routes received audio (from field radios or SU transmissions) to all connected
  console positions simultaneously.

- **Console-to-radio transmission:** Dispatcher clicks a PTT button; the console
  sends a PTT Request with priority. The CAR arbitrates, keys the transmitter via
  FSI, and grants permission. The dispatcher's audio flows as RTP IMBE (for digital
  channels) or PCM (for analog channels) packets to the CAR, which converts and
  transmits over the air.

- **Mixed analog/digital infrastructure:** When the fixed station's FSI connection
  is analog (E&M or Tone Remote Control) but the channel is digital, or vice versa,
  the CAR performs the audio format conversion. PCM Audio blocks (μ-law, 160 samples/
  20 ms) handle the analog bridge case.

- **Voter systems:** Wide-area receive sites (voter receivers) connected to a CAR
  allow a dispatcher to hear the best-quality signal from multiple sites. The Voter
  Command and Voter Status Update block types (25 and 26) allow dispatchers to
  manually SELECT or DISABLE specific receiver sites, and to receive real-time
  signal quality reports (GOOD_P25, GOOD_FM, BAD_P25, BAD_FM, etc.).

- **Station control:** Console subsystems can remotely change channel, squelch,
  repeat mode, protected mode, and RF mode of TRC-controlled or DFSI-connected
  fixed stations via Block Types 21-23.

- **Emergency signaling:** Single Block Control (Block Type 24) carries conventional
  supplementary data messages including Emergency Alarm, allowing emergency
  activations from SUs to be relayed from the fixed station through the CAR to the
  console subsystem.

### Vendor Implementations

Conventional ISSI has been implemented and tested by multiple P25 manufacturers
participating in the Project 25 Compliance Assessment Program (CAP) and voluntary
interoperability demonstrations:

- **Motorola Solutions** — ASTRO 25 conventional infrastructure; APX-series CARs
  and dispatch console systems (PremierOne, CenterPoint)
- **Harris (now L3Harris)** — Unity XG-series infrastructure
- **Tait Communications** — TaitNet P25 conventional infrastructure
- **Cassidian (now Airbus DS)** — THR series infrastructure
- **E.F. Johnson** — ATLAS P25 infrastructure

Interoperability testing for conventional ISSI is coordinated through the PTIG
(Project 25 Technology Interest Group) and the DHS/FEMA SAFECOM program. The P25
CAP does not currently include a specific Conventional ISSI test suite (as of the
reaffirmation date), but voluntary interoperability demonstrations between vendors
have been conducted.

---

## Key Online Resources

- **Project 25 Technology Interest Group (PTIG)** — official P25 standards body;
  approved standards list confirms BACE as active:
  https://project25.org/

- **PTIG Non-CAP ISSI/CSSI Interoperability Testing** — interoperability testing
  context:
  https://project25.org/index.php/compliance-assessment/p25-non-cap-issi-cssi-interoperability-testing

- **TIA Online** — official TIA standards store (purchase):
  https://standards.accuristech.com/tia or https://www.tiaonline.org/

- **RadioReference P25 Wiki** — overview of P25 system types including conventional:
  https://wiki.radioreference.com/index.php/APCO_Project_25

- **TIA-102 Archive** (Internet Archive collection):
  https://archive.org/details/TIA-102_Series_Documents

- **TIA-102.BACA listing** (trunking ISSI, companion spec):
  https://standards.globalspec.com/std/1558641/tia-102-baca

- **TIA-102.BAHA listing** (FSI, companion spec):
  https://standards.globalspec.com/std/10163296/tia-102-baha

- **Urgent Communications — ISSI/CSSI overview:**
  https://urgentcomm.com/collections/why-you-should-look-into-p25s-issi-and-cssi/

---

## Open-Source Implementations

No known open-source project implements the full Ec (Conventional ISSI, TIA-102.BACE)
stack as of the knowledge cutoff. The interface sits between infrastructure components
(CAR and Console Subsystem) rather than between subscriber radios and infrastructure,
so it is not directly relevant to SDR receiver or scanner software.

Related open-source projects that work with conventional P25 at adjacent layers:

- **OP25** (https://github.com/osmocom/op25) — P25 Phase 1 CAI decoder/encoder.
  Handles conventional P25 audio decoding, NAC filtering, link control parsing
  (the air interface layer that feeds into the CAR side of Ec). Does not implement
  Ec itself.

- **SDRTrunk** (https://github.com/DSheirer/sdrtrunk) — P25 Phase 1 and Phase 2
  decoder with full ISSI roaming support, talker alias, GPS. Like OP25, operates
  at the CAI layer (radio-to-air), not at the CAR-to-console IP layer. Full ISSI
  qualified identifier support present.

- **DVSwitch / MMDVM Bridge** — P25 gateway software for amateur radio linking.
  Operates above the codec layer and does not implement Ec.

- **p25gateway / AnalogBridge** — amateur P25 linking tools. Audio transport only;
  no Ec message handling.

For an open-source Ec implementation, the relevant components would be:
1. An RTP stack (e.g., using `rtp` crate in Rust or libsrtp in C)
2. The P25 block parser defined in this document (Sections 3.1–3.2)
3. The call control and transmission control state machines (Sections 4–5)
4. Integration with an FSI implementation (TIA-102.BAHA) on the CAR side

The IMBE codec (JMBE / AMBE hardware) would be needed for voice encoding/decoding
but is handled separately from the Ec protocol itself.

---

## Phase 3 Flag — Implementation Spec Needed

Based on Phase 1 classification (PROTOCOL + MESSAGE_FORMAT), this document warrants
two Phase 3 implementation specs in a follow-up pass:

1. **TIA-102-BACE_Message_Parsing_Spec.md** — Block type dispatch table, byte-level
   format diagrams for all 21+ block types, field extraction logic, RTP packet
   construction rules, ACK/NAK code handling.

2. **TIA-102-BACE_State_Machine_Spec.md** — All 8 call control state machines plus
   4 transmission control state machines. State tables with events, guards, and
   actions. Timer parameter table. PTT arbitration decision tree. TSN management
   algorithm.

These were not produced in this run per the instructions. Flag for follow-up.
