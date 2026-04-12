# TIA-102.BAJB-B — Related Resources & Context
# Project 25 Tier 1 Location Services Specification

---

## Status

**Active.** ANSI/TIA-102.BAJB-B-2019, approved December 9, 2019. This is the third
edition of this document. It cancels and replaces TIA-102.BAJB-A (October 2014), which
itself replaced the original (February 2009). Revision B aligns with the updated Location
Services Overview (TIA/TSB-102.BAJA-B) and addresses errata and editorial issues from
Revision A. No superseding document has been published as of 2024.

---

## Standards Family

This document belongs to the TIA-102 Location Services cluster within the broader P25
(Project 25) suite. The Location Services documents form their own sub-family:

```
TIA-102 P25 Suite
└── Location Services Sub-suite
    ├── TIA/TSB-102.BAJA-B  — Location Services Overview (architecture, tier definitions)
    │   └── Tier 1 (simple SU-to-SU, no IP/FNE)
    │       └── TIA-102.BAJB-B  ← THIS DOCUMENT
    │           (Tier 1 specification: NMEA 0183 over CAI Data Bearer, Direct/Repeated Data)
    └── Tier 2 (IP addressing, trunked FNE delivery)
        ├── TIA-102.BAJC     — Tier 2 Location Services (network-managed)
        └── (related Tier 2 documents)
```

**Companion normative documents:**

| Document | Role |
|----------|------|
| TIA-102.BAAC-D | Provides the Location Service SAP identifier used to identify Tier 1 location data packets on the CAI |
| TIA-102.BAEA-C | Defines U_m interface, Direct Data and Repeated Data configurations, unconfirmed data packet format and header structure |
| TIA/TSB-102.BAJA-B | Location Services Overview — defines the two-tier framework and explains why LIS and LSHS interfaces are not standardized |
| NMEA 0183 v3.01 | NMEA standard for GPS sentence formats used as the Tier 1 application protocol |

**Related P25 data-layer documents:**

| Document | Role |
|----------|------|
| TIA-102.BAAA-A | P25 CAI Data Services Overview |
| TIA-102.BAEA series | P25 Data Overview and Specification (full data bearer service stack) |
| TIA-102.BAAC-D | CAI Reserved Values (SAP identifiers, opcodes) |
| TIA/TSB-102-C | TIA-102 Documentation Suite Overview |

---

## Practical Context

### Real-World Use

Tier 1 Location Services represent the simplest form of P25 GPS tracking, commonly
found in:

- **Portable radio GPS tracking** — Many P25 portables (e.g., Harris XG-100P, Motorola
  APX series, Kenwood NX-5000 series) include an embedded GPSR that periodically
  broadcasts NMEA sentences over the CAI data bearer. Nearby dispatch terminals or
  other radios receive and display these positions.

- **Incident command applications** — Field commanders using laptop or tablet-based
  software (the LSHS) can receive position reports from field personnel over conventional
  or direct channels without requiring trunked network infrastructure.

- **Direct Mode / Talk-Around** — Because Tier 1 works on Direct Data configuration
  (SU-to-SU without infrastructure), it is applicable to talk-around and off-network
  scenarios — useful for emergency or remote operations where repeater coverage is
  unavailable.

- **Conventional channel GPS** — Tier 1 works on conventional repeated channels via
  the Repeated Data configuration, making it accessible to agencies using P25
  conventional systems without trunked core infrastructure.

### Limitations Observed in Practice

- The 128-octet packet cap and unconfirmed delivery mean Tier 1 is inherently
  best-effort. In congested channels with many automatic updaters, significant packet
  loss is expected (the standard itself documents reliability tables showing this).

- Tier 1 provides no mechanism to request a position from a specific radio; triggering
  is SU-local. Network-initiated position queries require Tier 2.

- Tier 1 cannot route location data to a fixed dispatch server over a trunked network
  — that requires Tier 2 with FNE involvement and IP addressing.

### Relationship to P25 Phase 1 vs Phase 2

The CAI Data Bearer Service used by Tier 1 is defined in TIA-102.BAEA-C for both
P25 Phase 1 (FDMA/C4FM) and Phase 2 (TDMA/H-DQPSK) systems. The U_m
interface and unconfirmed data packet delivery mechanism apply to both phases,
so Tier 1 Location Services operate identically over Phase 1 and Phase 2 infrastructure.

---

## Key Online Resources

- **TIA Standards Store (Accuristech)**
  Primary source for purchasing the current standard:
  https://store.accuristech.com/tia

- **TIA Online (tiaonline.org)**
  Standards body home page; standards committee (TR-8.5) information:
  https://tiaonline.org

- **NMEA 0183 Standard**
  The application-layer protocol referenced by this document; purchase from NMEA:
  https://www.nmea.org/pub/0183

- **APCO Project 25 Technology Interest Group (PTIG)**
  P25 interoperability test results and compliance information relevant to location
  services: https://www.project25.org

- **DHS SAFECOM / CISA P25 Resources**
  Government-facing guidance on P25 procurement, including location-related
  interoperability: https://www.cisa.gov/safecom

- **RadioReference Wiki — P25 Data Services**
  Community documentation of P25 data services including location:
  https://wiki.radioreference.com/index.php/APCO_Project_25

---

## Open-Source Implementations

Several open-source P25 projects implement or interface with P25 data services,
including location:

- **OP25 (osmocom-analog / boatbod fork)**
  P25 software-defined radio implementation in Python/C++. Handles P25 CAI data
  bearer; limited location-specific decoding but parses unconfirmed data packets.
  https://github.com/boatbod/op25

- **SDRTrunk**
  Java-based P25 decoder and trunking controller. Includes decoding of P25 data
  packets over both Phase 1 and Phase 2. The data packet decoder handles the
  U_m unconfirmed packet format that Tier 1 location data uses. NMEA payload
  extraction has been reported in community forks.
  https://github.com/DSheirer/sdrtrunk

- **p25rx / DSD (Digital Speech Decoder)**
  C-based P25 decoder; primarily voice-focused but parses data frames that would
  carry location packets.
  https://github.com/szechyjs/dsd

- **JMBE** (not location-specific but part of the P25 ecosystem)
  https://github.com/DSheirer/jmbe

- **Trunk-Recorder**
  P25 recording tool with data channel support; community plugins have added
  GPS/location tracking features using the CAI data bearer.
  https://github.com/robotastic/trunk-recorder

*Note: None of the above projects explicitly cite TIA-102.BAJB-B in their
documentation as of 2024. The NMEA-over-U_m interface is relatively simple, and
implementations often derive behavior from the NMEA 0183 standard directly combined
with the generic P25 data packet format, without referencing the Tier 1 location spec
by name.*

---

## Standards Lineage

```
ANSI/TIA-102.BAJB-B-2019  (this document — Tier 1 Location Services)
    │
    ├── Cancels and replaces: TIA-102.BAJB-A (2014)
    │       └── Cancels and replaces: TIA-102.BAJB (Original, 2009)
    │
    ├── Normative dependencies:
    │   ├── TIA-102.BAAC-D  (CAI Reserved Values — SAP identifier for Location Service)
    │   ├── TIA-102.BAEA-C  (P25 Data Overview — U_m interface, Direct/Repeated Data)
    │   └── NMEA 0183 v3.01 (application-layer sentence format)
    │
    ├── Informative context:
    │   ├── TIA/TSB-102.BAJA-B  (Location Services Overview — tier framework)
    │   ├── TIA/TSB-102-C       (TIA-102 Documentation Suite Overview)
    │   └── TIA Style Manual
    │
    └── Tier 2 (separate documents, not superseded by this):
        └── TIA-102.BAJC series  (Tier 2 Location Services — IP/FNE based)

P25 Standards Hierarchy (Location Services context):
TIA-102 Suite
├── A-series: CAI Physical/Link Layer (BAAA, BAAB, BAAC, BAAD, BAAE…)
├── B-series: Trunking, Data, Supplementary Services
│   ├── BA-series: Data Services
│   │   ├── TIA-102.BAEA  — Data Overview and Specification (U_m bearer)
│   │   └── TIA-102.BAJ-series — Location Services
│   │       ├── TSB-102.BAJA-B  — Location Services Overview
│   │       ├── TIA-102.BAJB-B  — Tier 1 (THIS DOCUMENT)
│   │       └── TIA-102.BAJC    — Tier 2
│   └── BB-series: MAC, Physical Layer (TDMA)
├── C-series: Conformance
└── TSB-102 series: Technical Service Bulletins (overviews, guidance)
```

---

## Phase 3 Note

**This document is classified as PROTOCOL with MESSAGE_FORMAT elements.**

A Phase 3 implementation spec is warranted for a follow-up pass. The spec should cover:

1. **NMEA 0183 Parser Spec** — Sentence dispatch by message type (GGA/GLL/GSA/
   GSV/RMA/RMC/VTG), field extraction by comma-delimited position, checksum
   validation (XOR of chars between '$' and '*'), handling of empty/blank fields, and
   the proprietary $PP25 extension parsing (UID extraction, user data field sizing).

2. **U_m Unconfirmed Packet Framing** — How to construct the CAI unconfirmed
   data packet header block: SAP identifier (from BAAC-D, Location Service value),
   Data Header Offset = 0, 128-octet max payload, encapsulation of NMEA sentence(s).

3. **SU Triggering and Reporting State Machine** — Trigger detection, buffer
   management (batch reporting), freshness check logic (up to 4 retransmits of stale
   data, unlimited retransmits in response to query).

These specs are flagged for a follow-up run and should reference TIA-102.BAEA-C
for the unconfirmed packet header format details.
