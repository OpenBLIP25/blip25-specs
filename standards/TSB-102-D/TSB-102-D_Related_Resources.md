# TSB-102-D: Project 25 — TIA-102 Documentation Suite Overview
## Related Resources and Context

---

## Status

**Active.** TSB-102-D is the current revision of the TIA-102 Documentation Suite Overview, published March 2020. It supersedes TSB-102-C and all earlier revisions (TSB-102, TSB-102-A, TSB-102-B). As an informative TSB rather than a normative TIA Standard, it is not directly "superseded" in the same sense as a protocol document — it is periodically revised as the suite evolves. The "D" suffix indicates this is the fourth major revision of the original document.

The document was formulated under TIA Project No. TIA-PN-102-D, under the cognizance of the TIA TR-8 Mobile and Personal Private Radio Standards, TR-8.10 Subcommittee on Trunking and Conventional Control.

As of the 2026 processing date, this is the most recent published version. Any updates would be tracked at the TIA publication catalog.

---

## Standards Family

TSB-102-D is the root-level overview of the entire TIA-102 (Project 25) standards suite. The TIA-102 series is organized into four document categories:

- **Overview (***-102.A___ and ***-102.B___)** — Informative high-level descriptions
- **Service (***-102.A___)** — Normative protocol and service definitions (messages, formats, procedures)
- **System (***-102.B___)** — Normative interface and system definitions (physical layer, MAC, ISSI, etc.)
- **Test (***-102.C___)** — Conformance, interoperability, performance, and compliance assessment

The direct peer documents (other suite overviews) are:

| Document | Topic |
|----------|-------|
| TSB-102.BBAA | Two-Slot TDMA Overview |
| TIA-102.AAAB | Security Services Overview |
| TIA-102.AABA | Trunking Overview |
| TSB-102.BACC | Inter-RF Subsystem Interface Overview |
| TSB-102.BADA | Telephone Interconnect Overview |
| TSB-102.BAGA | Console Subsystem Interface Overview |
| TSB-102.BAFA | Network Management Interface Overview |
| TSB-102.BAJA | Location Services Overview |
| TSB-102.CBAA | Product Compliance Assessment Overview |

The foundational normative documents (what gets implemented) are:

| Document | Topic |
|----------|-------|
| TIA-102.BAAA | FDMA Common Air Interface |
| TIA-102.BBAB | Phase 2 TDMA Physical Layer |
| TIA-102.BBAC | Phase 2 TDMA MAC Layer |
| TIA-102.AAAD | Block Encryption Protocol (DES/AES) |
| TIA-102.AABB/AABC/AABD | Trunking Control Channel Formats, Messages, Procedures |
| TIA-102.BACA | ISSI Voice/Mobility/RFSS Capability Polling |
| TIA-102.AACA | OTAR Messages and Procedures |

---

## Practical Context

Project 25 (P25) is the dominant interoperability standard for public safety Land Mobile Radio in North America. It is used by federal agencies (FBI, DHS, Border Patrol), state police, county sheriffs, municipal police and fire departments, and military installations. P25-compliant equipment is required for federally-funded public safety communications systems in the United States.

TSB-102-D serves several practical roles:

**For system integrators and procurement:** This document is the starting point for understanding what capabilities a P25 system is supposed to have. Tables 2-1 through 2-10 tell integrators which services are available on which interfaces (CAI vs. ISSI vs. DFSI), and whether a given service requires TDMA (Phase 2) hardware.

**For manufacturers:** The document catalog in Section 4 tells manufacturers which specific TIA-102 documents they must conform to in order to claim compliance with specific P25 features. The Recommended Compliance Assessment Tests (RCAT) TSBs (TSB-102.CBB series) further specify which test documents apply to which product categories.

**For open-source P25 developers:** This document is the map for the implementation territory. Projects like SDRTrunk, OP25, and Unitrunker implement P25 reception and decoding; this document explains the full scope of what they are (and are not) implementing. Notably:
  - The FDMA CAI (Um interface) is what SDR-based P25 scanners receive
  - The ISSI (G interface) is infrastructure-to-infrastructure and is not visible over the air
  - Phase 2 (TDMA, Um2) requires H-CPM/H-DQPSK demodulation, which is distinct from Phase 1 FDMA
  - Encryption (TIA-102.AAAD) is a separate layer applied on top of the CAI bearer

**For P25 Phase 2 (TDMA) context:** This document is one of the first places that makes clear the Phase 2 CAI (Um2, Two-Slot TDMA) is a parallel air interface, not a replacement for Phase 1. Both interfaces are defined in the same suite and can coexist. Phase 2 adds Transmitting Subscriber Forced Preemption, which Phase 1 FDMA cannot support (the FNE can only preempt outbound audio to receiving SUs, not signal the transmitting SU to stop).

**Interoperability model:** The document explains that P25 interoperability is achieved at the interfaces, not inside the RFSS. Two vendors' RFSSs can connect via the ISSI regardless of internal implementation differences. A vendor's subscriber unit must conform to the CAI but the internal workings of its FNE are unspecified.

---

## Key Online Resources

- **TIA Online (official standards publisher):**
  https://www.tiaonline.org/standards/catalog/
  The official TIA publication catalog lists current versions of all TIA-102 documents.

- **APCO International — Project 25:**
  https://www.apcointl.org/technology/project-25/
  APCO is the primary user organization behind P25. Contains user-facing information on P25 deployment, compliance assessment, and the Steering Committee.

- **P25 Steering Committee (PTIG):**
  https://www.ptig.org/
  The Project 25 Technology Interest Group. Maintains information on P25 implementations, conformance assessment programs, and industry liaison. Their Compliance Assessment Program (CAP) directly references the RCAT TSBs cataloged in this document.

- **NIST P25 Compliance Assessment Program:**
  https://www.dhs.gov/science-and-technology/safecom-p25-cap
  DHS/SAFECOM administers the P25 CAP, which certifies equipment against TIA-102 standards. Uses the conformance and interoperability test results referenced in this document.

- **SAFECOM / DHS:**
  https://www.cisa.gov/safecom
  U.S. federal public safety communications coordination. P25 compliance requirements for federal grant programs are coordinated through SAFECOM.

- **Accuris Tech (TIA standards distributor):**
  https://store.accuristech.com/tia
  Commercial source for purchasing individual TIA-102 documents referenced in this catalog.

- **IHS Markit / S&P Global (standards archive):**
  Secondary source for TIA standards. Many of the documents referenced in the catalog can be purchased here.

---

## Open-Source Implementations

The following open-source projects implement portions of the P25 suite cataloged by this document:

**SDRTrunk** (Java, receives and decodes P25 Phase 1 and Phase 2 over-the-air)
- https://github.com/DSheirer/sdrtrunk
- Implements CAI (Um) decoding for Phase 1 (FDMA/C4FM/CQPSK) and Phase 2 (TDMA/H-CPM)
- Decodes trunking control channel messages (TIA-102.AABC) and trunking procedures (TIA-102.AABD)
- Decodes link control words (TIA-102.AABF)
- Does NOT implement ISSI (G interface), encryption (TIA-102.AAAD active decryption), or OTAR

**OP25** (Python/GNU Radio, P25 Phase 1 and 2 receiver)
- https://github.com/boatbod/op25
- Implements FDMA CAI demodulation and decoding
- Handles trunked and conventional operation
- Phase 2 support via H-DQPSK demodulation

**Unitrunker** (Windows, closed-source scanner control with open P25 protocol parsing)
- Not open source, but widely deployed; implements CAI control channel parsing per TIA-102.AABC/AABD

**JMBE** (Java IMBE/AMBE vocoder)
- https://github.com/DSheirer/jmbe
- Implements the IMBE vocoder format described in TIA-102.BABA (Vocoder Description)
- Required for audio decoding; the vocoder specification documents the IMBE codec parameters

**p25lib** (various forks, C/C++)
- Various GitHub forks providing P25 CAI framing and message parsing
- Example: https://github.com/DSD-dev (DSD/DSD+ decode P25 Phase 1 audio)

**Note on encryption:** The Block Encryption Protocol (TIA-102.AAAD) implements AES/DES encryption on voice frames. Legal open-source implementations of P25 decryption exist but require the actual key — key recovery from encrypted P25 traffic is not supported by any public tool.

---

## Standards Lineage

```
TIA (Telecommunications Industry Association)
└── TIA TR-8 (Mobile and Personal Private Radio Standards)
    └── TR-8.10 (Trunking and Conventional Control Subcommittee)
        └── TIA-102 Series (Project 25 / P25 Standards Suite)
            │
            ├── TSB-102-D  ← THIS DOCUMENT (Suite Overview, rev. D, 2020)
            │   Supersedes: TSB-102-C, TSB-102-B, TSB-102-A, TSB-102 (original)
            │
            ├── OVERVIEW CATEGORY (***-102.A___, ***-102.B___)
            │   ├── TIA-102.AAAB  Security Services Overview
            │   ├── TIA-102.AABA  Trunking Overview
            │   ├── TSB-102.BACC  ISSI Overview
            │   ├── TSB-102.BADA  Telephone Interconnect Overview
            │   ├── TSB-102.BAGA  CSSI Overview
            │   ├── TSB-102.BAFA  Network Management Interface Overview
            │   ├── TSB-102.BAJA  Location Services Overview
            │   ├── TSB-102.BBAA  Two-Slot TDMA Overview
            │   └── TSB-102.CBAA  Product Compliance Assessment Overview
            │
            ├── SERVICE CATEGORY (***-102.A___)
            │   ├── TIA-102.AAAD  Block Encryption Protocol (DES/AES)
            │   ├── TIA-102.AABB  Trunking Control Channel Formats
            │   ├── TIA-102.AABC  Trunking Control Channel Messages
            │   ├── TIA-102.AABD  Trunking Procedures
            │   ├── TIA-102.AABF  Link Control Word Formats and Messages
            │   ├── TIA-102.AABG  Conventional Control Messages
            │   ├── TIA-102.AABH  Dynamic Regrouping Messages and Procedures
            │   ├── TIA-102.AACA  OTAR Messages and Procedures
            │   ├── TIA-102.AACD  Key Fill Device Interface Protocol
            │   └── TIA-102.AACE  Link Layer Authentication
            │
            ├── SYSTEM CATEGORY (***-102.B___)
            │   ├── TIA-102.BAAA  FDMA Common Air Interface (CAI) — Phase 1
            │   ├── TIA-102.BAAC  CAI Reserved Values
            │   ├── TIA-102.BAAD  Conventional Procedures
            │   ├── TIA-102.BABA  Vocoder Description (IMBE full-rate + half-rate)
            │   ├── TIA-102.BACA  ISSI Voice/Mobility/RFSS Capability Polling
            │   ├── TIA-102.BACD  ISSI Supplementary Data
            │   ├── TIA-102.BACE  ISSI Conventional Operation
            │   ├── TIA-102.BACF  ISSI Packet Data
            │   ├── TIA-102.BAEA  Data Overview and Specification
            │   ├── TIA-102.BAEB  IP Data Bearer Service
            │   ├── TIA-102.BAED  Packet Data LLC Procedures
            │   ├── TIA-102.BAEE  Radio Management Protocols
            │   ├── TIA-102.BAEF  Packet Data Host Network Interface (Ed)
            │   ├── TIA-102.BAEG  Mobile Data Peripheral Interface (A)
            │   ├── TIA-102.BAEJ  Conventional Management Service for Packet Data
            │   ├── TIA-102.BAHA  Fixed Station Interface Messages and Procedures (Ef)
            │   ├── TIA-102.BAJB  Tier 1 Location Services
            │   ├── TIA-102.BAJC  Tier 2 Location Services
            │   ├── TIA-102.BAJD  TCP/UDP Port Number Assignments
            │   ├── TIA-102.BAKA  KMF to KMF Interface (IKI)
            │   ├── TIA-102.BBAB  Phase 2 TDMA Physical Layer — Two-Slot TDMA CAI
            │   ├── TIA-102.BBAC  Phase 2 TDMA MAC Layer Specification
            │   ├── TIA-102.BBAD  Phase 2 TDMA MAC Layer Messages
            │   └── TIA-102.BBAE  Phase 2 TDMA MAC Layer Procedures
            │
            ├── TEST CATEGORY — CONFORMANCE (***-102.C___, ***-102.B___)
            │   ├── TIA-102.AAAC  DES Encryption Conformance
            │   ├── TIA-102.AACC  OTAR Conformance
            │   ├── TIA-102.BAAB  CAI Conformance
            │   ├── TIA-102.BABB  Vocoder MOS Conformance
            │   ├── TIA-102.BCAD  Phase 2 TDMA CAI Conformance
            │   ├── TIA-102.BCAE  Phase 2 TDMA Messages/Procedures Conformance
            │   ├── TIA-102.BCAF  TDMA Voice Channel Conformance Profiles
            │   ├── TIA-102.CAEA  Conformance Profile L1 — Basic Conventional
            │   ├── TIA-102.CAEB  Conformance Profile L2 — Advanced Conventional
            │   ├── TIA-102.CAEC  Conformance Profile — Basic Trunked
            │   ├── TIA-102.CAED  Conformance Profiles — Advanced Trunked
            │   └── TIA-102.CADA  Fixed Station Interface Conformance
            │
            ├── TEST CATEGORY — MEASUREMENT AND PERFORMANCE
            │   ├── TIA-102.CAAA  C4FM/CQPSK Transceiver Measurement Methods
            │   ├── TIA-102.CAAB  C4FM/CQPSK Transceiver Performance Recommendations
            │   ├── TIA-102.CCAA  TDMA Transceiver Measurement Methods
            │   ├── TIA-102.CCAB  TDMA Transceiver Performance Recommendations
            │   ├── TIA-102.BABG  Enhanced Vocoder Measurement Methods
            │   ├── TIA-102.CACA  ISSI Measurement Methods for Voice Services
            │   └── TIA-102.CACB  ISSI Performance Recommendations for Voice Services
            │
            ├── TEST CATEGORY — INTEROPERABILITY
            │   ├── TIA-102.CABA  Interoperability — Conventional Voice
            │   ├── TIA-102.CABB  Interoperability — OTAR
            │   ├── TIA-102.CABC  Interoperability — Trunked Voice
            │   ├── TIA-102.CACC  ISSI Conformance Test Procedures
            │   └── TIA-102.CACD  ISSI Interoperability — Trunked Voice
            │
            └── COMPLIANCE ASSESSMENT (TSB-102.CB series)
                ├── TSB-102.CBAB  SDoC Template
                ├── TSB-102.CBAC  Test Report Guidelines — Transceiver Performance
                ├── TSB-102.CBAF  Test Report Guidelines — Trunking Interoperability
                ├── TSB-102.CBBA  RCAT — Conventional Mode Subscriber
                ├── TSB-102.CBBC  RCAT — Conventional Mode Fixed Station
                ├── TSB-102.CBBE  RCAT — Conventional Operation
                ├── TSB-102.CBBF  RCAT — Trunking Mode Subscriber
                ├── TSB-102.CBBH  RCAT — Trunking Mode Fixed Station
                ├── TSB-102.CBBJ  RCAT — Trunking Operation
                ├── TSB-102.CBBK  RCAT — Trunking ISSI
                └── TSB-102.CBBL  RCAT — Two-Slot TDMA Trunking Voice Channel

Related TIA Series:
├── TSB-88.1–88.5    LMR Performance in Noise and Interference
├── TSB-133, 150, 159  RF Exposure (FCC Part 90)
├── TIA-4950           Hazardous Locations (intrinsic safety)
├── TIA-5045           Numeric ID for Conventional Analog Operation
├── TSB-4973.000       Broadband Data Protocol Standards Overview
├── TIA-4973.201       Mission Critical PTT Requirements
└── TIA-4973.211       Mission Critical Priority and QoS Control Service
```

---

## Phase 3 Notes

**No Phase 3 implementation spec is needed.** This document is classified as OVERVIEW. It defines no algorithms, message formats, protocol state machines, or vocoder parameters that require implementation. All normative implementation content lives in the TIA-102 documents cataloged within this document.

If implementation specs are desired for documents referenced herein, the appropriate Phase 3 work is scoped per those documents — not this one.
