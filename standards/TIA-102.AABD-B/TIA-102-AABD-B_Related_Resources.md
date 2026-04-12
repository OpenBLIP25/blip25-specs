# TIA-102.AABD-B Related Resources & Context
# Project 25 FDMA — Common Air Interface — Trunking Control Channel Procedures
# Generated: 2026-04-12

---

## Status

**Active — with amendments.**

The base TIA-102.AABD-B (published November 5, 2014) is the current revision of
this standard, having superseded the original TSB-102.AABD (1997 Technical
Service Bulletin). The standard has been amended three times since the base
publication:

- **TIA-102.AABD-B-1** — December 13, 2023
- **TIA-102.AABD-B-2** — July 26, 2023 *(note: B-2 predates B-1 in publication date — this ordering may reflect internal TIA numbering conventions)*
- **TIA-102.AABD-B-3** — February 12, 2025 *(most current amendment)*

All three amendment files are present in the same project directory alongside the
2014 base document. The most current normative version is TIA-102.AABD-B-3
(February 2025). Users implementing this standard should apply all amendments on
top of the base B revision.

The standard is published by the Telecommunications Industry Association (TIA)
and is accessible through the TIA standards store at
https://store.accuristech.com/tia or https://tiaonline.org.

---

## Standards Family

This document is part of the TIA-102 P25 standards suite, which is organized
into sections by function. Within the suite, TIA-102.AABD-B is the behavioral
procedures document for the FDMA CAI trunking control channel.

### Immediate Companion Documents

These documents form the core set needed to implement a complete P25 trunked FDMA
system:

| Document | Title | Relationship |
|----------|-------|--------------|
| **TIA-102.AABC** | Trunking Messages and Formats | Message formats (opcodes, field layouts) for all ISPs and OSPs referenced in this document |
| **TIA-102.BAAA** | FDMA Common Air Interface Physical Layer | Physical and link layer that carries the messages |
| **TIA-102.AACE** | Security Services for Phase 1 | Authentication procedures referenced in Section 15 |
| **TIA-102.BAAC** | Manufacturer ID / Electronic Serial Number | Manufacturer IDs used in ESN and MFID assignments |
| **TIA-102.BAAD** | FDMA CAI — Link Layer | Link layer procedures for traffic channel (referenced in Section 16 Conventional Fallback) |
| **TIA-102.BBAC** | TDMA MAC Procedures | Phase 2 TDMA MAC; this document includes TDMA extensions for FDMA control channels coordinating TDMA traffic channels |

### Broader TIA-102 Suite Context

```
TIA-102 Suite
├── AA Series — FDMA Common Air Interface (CAI)
│   ├── AABC — Trunking Messages and Formats (ISPs, OSPs)
│   ├── AABD-B — THIS DOCUMENT — Trunking Control Channel Procedures
│   └── AACE — Security Services (authentication)
├── BA Series — FDMA Physical/Link Layer
│   ├── BAAA — Physical Layer (modulation, framing)
│   ├── BAAB — Physical Layer Supplement
│   ├── BAAC — Manufacturer IDs / ESN
│   ├── BAAD — Link Layer (LDU, conventional procedures)
│   └── BABA — Half-Rate Vocoder
├── BB Series — TDMA (Phase 2)
│   ├── BBAA — TDMA Physical Layer
│   └── BBAC — TDMA MAC Procedures
├── BC Series — Conformance
│   ├── BCAD — Protocol Conformance
│   ├── BCAE — SU Conformance
│   └── BCAF — Infrastructure Conformance
└── CA/CC Series — Measurement / Testing
    ├── CAAA — Transceiver Measurements
    └── CCAA — TDMA Measurements
```

### Predecessor

**TSB-102.AABD** (1997) — This was the original Technical Service Bulletin that
established the P25 trunking control channel procedures. The current TIA-102.AABD-B
supersedes it entirely. Much of the core architecture (addressing hierarchy, random
access, registration/affiliation, voice call procedures) was established in the
1997 document. The principal additions in TIA-102.AABD-B include:
- Composite Control Channel (CCC) procedures (Section 14, formerly Annex G)
- TDMA-related extensions (IDEN_UP_TDMA, SYNC_BCST)
- Enhanced Radio Unit Monitor (Section 10.6)
- Enhanced emergency procedures
- Section 15 link layer encryption content removed (subject of further study)
- Conventional Fallback refinements
- Priority Procedures formalized (Section 18)

---

## Practical Context

### Who Uses This Standard

This document governs the air interface behavior of every P25 trunked radio
system deployed in the United States and many other countries. P25 is the
dominant digital trunked radio standard for public safety:
- Law enforcement agencies (police departments, sheriff offices, federal law
  enforcement including FBI, DEA, DHS)
- Fire departments and EMS
- Military tactical communications
- Some utilities and transportation agencies

Virtually every major land mobile radio manufacturer implements this document:
- Motorola Solutions (APX, ASTRO 25 infrastructure)
- L3Harris (formerly Harris RF)
- Kenwood
- Hytera
- EF Johnson

### How It Is Used in Real Equipment

**Subscriber Unit (SU) Side:** Every P25 portable/mobile radio implements the SU
procedures in this document. When you press PTT on a P25 radio, it executes the
Section 7.1 Group Voice Call procedure (GRP_V_REQ → GRP_V_CH_GRANT → tune to
traffic channel). When you power up a P25 radio, it executes Sections 5 and 6
(control channel acquisition → registration → group affiliation). Radio Inhibit
(Section 13.3) is used by agencies to remotely brick a stolen or compromised
radio.

**Infrastructure (RFSS/FNE) Side:** The P25 site controller and repeater
infrastructure implements the RFSS-side procedures. Motorola's SmartZone,
Harris OpenSky/BeOn, and similar systems all implement these procedures for
their site controllers.

**Composite Control Channel:** CCC mode (Section 14) is widely deployed at
rural sites and state police installations that operate on single-channel or
two-channel sites. It allows the single channel to handle both control signaling
and voice traffic.

**Emergency Procedures:** Section 11 is critical for public safety. The Radio
Inhibit procedure (Section 13.3) is used after officer-involved shootings or
when a radio is reported stolen to remotely disable the unit. Emergency alarms
from Section 11.2 route to dispatch console screens immediately.

### APCO P25

The standard is published through TIA but the technology is also governed by
the APCO (Association of Public-Safety Communications Officials) Project 25
program. APCO P25 maintains its own website and compliance testing program:
- https://www.apcointl.org/technology/apco-project-25/
- The DHS PSCR (Public Safety Communications Research) program oversees
  P25 CAP (Compliance Assessment Program) testing

---

## Key Online Resources

### Official Standards

- **TIA Standards Store:** https://store.accuristech.com/tia
  — Purchase access to TIA-102.AABD-B and amendments (B-1, B-2, B-3)
- **TIA Online:** https://tiaonline.org
  — Standards overview, contact for TIA members
- **APCO P25 Technology Interest Group:**
  https://www.apcointl.org/technology/apco-project-25/
  — Non-technical overview, deployment guidance, compliance testing information
- **DHS PSCR (Public Safety Communications Research):**
  https://www.dhs.gov/science-and-technology/public-safety-communications-research
  — Research program overseeing P25 interoperability

### Technical References and Documentation

- **P25 Standards Suite Overview (PSCR):**
  https://www.dhs.gov/sites/default/files/publications/P25-Standards-Overview.pdf
  — Free overview document explaining the TIA-102 suite structure
- **NPSTC (National Public Safety Telecommunications Council):**
  https://www.npstc.org
  — P25 deployment guidance and interoperability recommendations
- **CISA P25 CAP (Compliance Assessment Program):**
  https://www.cisa.gov/safecom/p25
  — Compliance testing program for P25 equipment; includes test results for
  registered equipment

### Protocol Analysis Tools

- **Trunk Recorder:** https://github.com/robotastic/trunk-recorder
  — Open-source P25 trunk recorder; implements SU-side control channel
  monitoring and following procedures per this document
- **SDRTrunk:** https://github.com/DSheirer/sdrtrunk
  — SDR-based P25 signal decoder; implements control channel acquisition
  (Section 5), registration/affiliation monitoring (Section 6), and group
  voice call following (Section 7.1). Java source code in
  `src/main/java/io/github/dsheirer/module/decode/p25/` provides a working
  reference implementation of the SU-side procedures
- **OP25 (OpenMoko P25):** https://osmocom.org/projects/op25/wiki
  — GNU Radio-based P25 receiver; implements trunked monitoring procedures;
  Python source code provides reference implementation of control channel
  processing
- **Osmocom P25 (osmo-p25):** https://osmocom.org/projects/osmo-p25
  — Infrastructure-side P25 work within the Osmocom project

---

## Open-Source Implementations

The following open-source projects implement the procedures defined in this
document. None are complete or fully conformant implementations, but all provide
useful reference code for specific procedures:

### SDRTrunk
**URL:** https://github.com/DSheirer/sdrtrunk  
**Language:** Java  
**What it implements:**
- Control channel acquisition (Section 5) — `P25TrafficChannelManager`
- Slotted Aloha monitoring (Section 4) — passive monitor, not transmit
- Group affiliation monitoring (Section 6.8) — tracks affiliations from
  GRP_AFF_REQ/RSP
- Group voice call following (Section 7.1) — tunes to traffic channel on
  GRP_V_CH_GRANT
- Unit-to-unit call following (Section 7.3)
- Composite Control Channel detection (Section 14) — composite mode bit
- Emergency call indication (Section 11) — "E" bit detection
- Adjacent site broadcast processing (Section 12.6)
- Channel identifier update processing (Section 12.3)

### OP25
**URL:** https://github.com/boatbod/op25  
**Language:** Python + GNU Radio C++ blocks  
**What it implements:**
- Control channel monitoring and acquisition (Section 5)
- ISP/OSP message decode and state tracking
- Group voice call following with traffic channel tuning
- Trunked call logging and audio recording

### Trunk Recorder
**URL:** https://github.com/robotastic/trunk-recorder  
**Language:** C++  
**What it implements:**
- Multi-channel P25 trunked system monitoring
- Control channel acquisition and following
- Group voice call monitoring
- Selective recording based on talkgroup

### p25rx (various)
Several independent SDR-based P25 receivers exist on GitHub implementing
subsets of the SU-side monitoring procedures. Search GitHub for "p25 trunked"
or "p25 sdr" for current projects.

### No Known Open-Source P25 Infrastructure

As of April 2026, there is no known open-source P25 infrastructure (FNE/RFSS)
implementation that fully implements the RFSS-side procedures in this document.
The Osmocom project has partial work. This represents a significant gap in the
P25 open-source ecosystem.

---

## Standards Lineage — Document Hierarchy

```
TIA-102.AABD-B (this document, 2014) — Trunking Control Channel Procedures
│
├── Supersedes: TSB-102.AABD (1997)
│   └── Original P25 trunking control channel procedures bulletin
│
├── Amended by:
│   ├── TIA-102.AABD-B-2 (July 2023)
│   ├── TIA-102.AABD-B-1 (December 2023)
│   └── TIA-102.AABD-B-3 (February 2025) ← MOST CURRENT
│
├── Normative references (message formats and physical layer):
│   ├── TIA-102.AABC — ISP/OSP message formats and opcodes
│   ├── TIA-102.BAAA — FDMA physical layer
│   ├── TIA-102.BAAD — Link layer (traffic channel)
│   └── TIA-102.AACE — Authentication / security services
│
├── Normative references (addressing and identifiers):
│   └── TIA-102.BAAC — Manufacturer IDs / ESN
│
└── TDMA extensions reference:
    └── TIA-102.BBAC — TDMA MAC procedures (Phase 2)

Architecture context:
    TIA-102 Suite
    └── FDMA CAI (Phase 1)
        ├── TIA-102.AABC (messages)
        ├── TIA-102.AABD-B (procedures) ← THIS DOCUMENT
        ├── TIA-102.BAAA (physical)
        └── TIA-102.BAAD (link layer)
    └── TDMA CAI (Phase 2)
        ├── TIA-102.BBAA (physical)
        └── TIA-102.BBAC (MAC + procedures)

Note: AABD-B is primarily an FDMA document, but it includes procedure extensions
relevant to systems where FDMA control channels coordinate TDMA traffic channels
(IDEN_UP_TDMA messages, SYNC_BCST for TDMA traffic channel synchronization).
```

---

## Notes on This Extraction

- **Source:** The 2014 base "B" revision was processed for this extraction.
  Three subsequent amendments (B-1, B-2, B-3) were found in the same directory
  but not processed. For current normative content, the B-3 amendment should
  be applied.
- **Related Resources search was conducted from training knowledge (cutoff
  August 2025).** URLs should be verified for current availability.
- **ISP/OSP names** in Annexes B and C of this document provide the complete
  message vocabulary for the trunking control channel. These names (GRP_V_REQ,
  GRP_V_CH_GRANT, etc.) are used as the primary identifiers in all open-source
  P25 implementations and SDR decoder software.
