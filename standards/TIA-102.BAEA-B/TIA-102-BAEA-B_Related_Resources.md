# TIA-102.BAEA-B — Related Resources and Context

**Document:** Project 25 Data Overview and Specification  
**TIA Number:** TIA-102.BAEA-B (ANSI/TIA-102.BAEA-B-2012)  
**Approved:** June 22, 2012  

---

## Status

**Active.** This document is the current revision (Revision B) of TIA-102.BAEA. It cancels and replaces TIA-102.BAEA-A (2004). There is no known subsequent revision as of 2026.

Key status notes:
- TIA-102.BAEC (Circuit Data Bearer Service), which this document previously referenced as a companion, was **officially rescinded in January 2010**. Revision B was produced partly to remove all references to circuit data.
- Several documents referenced normatively in this revision (BAEB, BAED, BAEF, BAEG) were still in TR-8.5 draft form at publication. Most have since been published as approved TIA standards.
- The BAEH (IP Data Security Service) document was listed as a future work item and was not published within the BAEA suite as of the knowledge cutoff.
- This document was adopted by the APCO/NASTD/FED Project 25 Steering Committee on January 27, 2012.

---

## Standards Family

This document is the root of the **TIA-102 BAEA Packet Data Service document suite**, within the broader TIA-102 (Project 25) standards hierarchy.

### TIA-102 BAEA Packet Data Suite

| Designator | Title | Notes |
|------------|-------|-------|
| TIA-102.BAEA-B | **Data Overview and Specification** | This document |
| TIA-102.BAEB | IP Data Bearer Service Specification | SCEP, SNDCP, TMS, CMS protocols |
| TIA-102.BAEC | (Withdrawn) | Circuit Data Bearer Service — rescinded Jan 2010 |
| TIA-102.BAED | Packet Data Logical Link Control Procedures | LLC layer for packet data |
| TIA-102.BAEE-B | Radio Management Protocols | MDP management over A Interface |
| TIA-102.BAEF | Packet Data Host Network Interface | E_d Interface (IPv4 over Ethernet/802.3) |
| TIA-102.BAEG | Mobile Data Peripheral Interface | A Interface physical layer (USB/PPP/SLIP) |
| TIA-102.BAEH | IP Data Security Service | Future work item; not yet published |

### Foundation Documents Referenced

| Document | Title |
|----------|-------|
| TIA-102.BAAA-A | Project 25 FDMA Common Air Interface (PHY/MAC layer) |
| TIA-102.BAAD-1 | CAI Conventional Channels Addendum 1 — Packet Data Registration/OTAR |
| TIA-102.AAAD-A | Block Encryption Protocol |
| TIA-102.AABA-B | Trunking Overview |
| TIA-102.BAEE-B | Radio Management Protocols |

### Related ISSI/Wireline Interfaces

| Document | Title |
|----------|-------|
| TSB-102.BACC-B | Project 25 Inter-RF Subsystem Interface Overview |
| TIA-102.BACD-B | ISSI Messages and Procedures for Supplementary Data Services |
| TIA-102.BACE | ISSI Messages and Procedures for Conventional Operation |
| TIA-102.BAHA | Fixed Station Interface Messages and Procedures |
| TSB-102.BAGA | Console Subsystem Interface Overview |

### Related Application/Service Documents

| Document | Title |
|----------|-------|
| TIA-102.AACA-A | Over-the-Air Rekeying (OTAR) Messages and Procedures |
| TSB-102.BAJA-A | Project 25 Location Services Overview |
| TSB-102.BBAA | Project 25 Two-Slot TDMA Overview |

---

## Standards Lineage

```
TIA-102 (Project 25) Standards Suite
│
├── Voice / CAI
│   ├── TIA-102.BAAA-A  FDMA Common Air Interface (PHY/MAC)
│   ├── TIA-102.BABA    Vocoder (IMBE/AMBE)
│   └── TSB-102.BBAA    Two-Slot TDMA Overview
│
├── Trunking
│   ├── TIA-102.AABA-B  Trunking Overview
│   ├── TIA-102.AABC-C  Trunking Control Channel Messages
│   └── TIA-102.AABD-A  Trunking Procedures
│
├── Conventional
│   ├── TIA-102.AABG    Conventional Control Messages
│   └── TIA-102.BAAD-A  Conventional Procedures
│
├── Packet Data  ◄── THIS SUITE
│   ├── TIA-102.BAEA-B  Data Overview and Specification  ← THIS DOC
│   ├── TIA-102.BAEB    IP Data Bearer Service Specification
│   ├── TIA-102.BAED    Packet Data LLC Procedures
│   ├── TIA-102.BAEE-B  Radio Management Protocols
│   ├── TIA-102.BAEF    Packet Data Host Network Interface
│   ├── TIA-102.BAEG    Mobile Data Peripheral Interface
│   └── TIA-102.BAEH    IP Data Security Service (future)
│
├── Security
│   ├── TIA-102.AAAD-A  Block Encryption Protocol
│   └── TIA-102.AACA-A  OTAR Messages and Procedures
│
├── Location Services
│   └── TSB-102.BAJA-A  Location Services Overview
│
└── Inter-System Interfaces
    ├── TSB-102.BACC-B  ISSI Overview
    ├── TIA-102.BACD-B  ISSI Supplementary Data
    └── TSB-102.BAGA    Console Subsystem Interface Overview
```

Document lineage for this document specifically:
```
TIA/EIA-102.BAEA (Interim Standard, 02/1995)
    └── TIA/EIA-102.BAEA (Original, 02/2000)
            └── TIA-102.BAEA-A (Revision A, 05/2004)
                    └── TIA-102.BAEA-B (Revision B, 06/2012) ← CURRENT
```

---

## Practical Context

### Where This Document Fits in Equipment Design

This document is the **architectural reference** that engineers consult when designing or certifying P25 packet data capability. Its primary audiences are:

1. **Subscriber unit (SU) manufacturers** — to understand which bearer services and configurations must/may be supported and which protocol stacks are required on the U_m air interface.
2. **Fixed Network Equipment (FNE/RFSS) vendors** — to understand which configurations their infrastructure must support and how to implement the CMS, TMS, SNDCP, and SCEP protocol elements.
3. **Mobile Data Peripheral (MDP) manufacturers** — to understand the A Interface (serial, via USB or TIA-232) and what IPv4 encapsulation protocols are required.
4. **Test and certification lab developers** — to understand the matrix of bearer service / configuration combinations (Table 11) against which conformance testing is structured.

### Real-World Usage

P25 packet data as defined by this suite is deployed in several forms:

- **Location Services (AVL/GPS)**: The most common P25 data application. Tier 1 uses CAI Data Bearer Service (direct or repeated). Tier 2 uses IP Data Bearer Service and requires SCEP on the air interface. Most modern P25 portables and mobiles support at minimum Direct Data IP with SCEP.

- **OTAR (Over-the-Air Rekeying)**: Data Link Dependent OTAR uses CAI Data Bearer Service on conventional channels. Data Link Independent OTAR uses IP Data Bearer Service. This is a critical security operation for key management in encrypted P25 deployments.

- **CAD/Dispatch Integration**: P25 packet data enables Computer-Aided Dispatch (CAD) data from dispatch consoles to be delivered to mobile units over the same radio infrastructure used for voice.

- **Trunked Data Channels**: The TMS/SNDCP trunked configuration enables dedicated packet data channels to be allocated on trunked systems, allowing IP data delivery without tying up voice traffic channels.

- **Conventional Data Registration**: The CMS protocol (Static/Dynamic Packet Data Registration, Location Tracking) over conventional channels allows FNEs to track which SUs are currently registered for packet data service and their scan/channel status.

### Addressing Modes

This document formally distinguishes two addressing modes:
- **Symmetric** (Direct/Repeated Data): Both endpoints are SUs; addresses are equal peers.
- **Asymmetric** (FNE configurations): SU-to-FNE, where the FNE acts as an infrastructure gateway with a different addressing role.

This distinction drives different protocol behaviors in BAEB (SCEP vs. SNDCP selection, address binding methods) and affects how OTAR and location services route data.

---

## Key Online Resources

- **TIA Online Standards Catalog**: https://standards.tiaonline.org/  
  (Primary source for purchasing TIA-102 standards; BAEA-B is available for purchase.)

- **Project 25 Technology Interest Group (PTIG)**: https://www.project25.org/  
  (Maintains the list of approved P25 TIA standards and Project 25 Steering Committee documents.)

- **P25 Approved Standards List (2022)**: https://project25.org/images/stories/ptig/P25_SC_22-04-003_Approved_P25_TIA_Standards_Approved_4052022.pdf  
  (Official list confirming BAEA-B active status.)

- **GlobalSpec TIA-102.BAEA listing**: https://standards.globalspec.com/std/9977410/TIA-102.BAEA  
  (Third-party standards aggregator with document metadata.)

- **P25 Standards Document Suite Reference (QSL.net archive)**: https://www.qsl.net/kb9mwr/projects/dv/apco25/P25_Standards.pdf  
  (Historical reference showing the BAEA suite evolution.)

- **Motorola Solutions P25 TDMA White Paper**: https://www.motorolasolutions.com/content/dam/msi/docs/business/_documents/white_paper/_static_files/p25_tdma_standard_white_paper_final.pdf  
  (Vendor context for the broader P25 data architecture, including trunked data.)

---

## Open-Source Implementations

No open-source projects are known to implement the full TIA-102 BAEA packet data stack (SCEP, SNDCP, LLC, TMS, CMS) as a standalone library. However, the following projects implement adjacent or related P25 capabilities:

### OP25 (GNU Radio-based P25 decoder)
- **Repository**: https://github.com/boatbod/op25 (primary maintained fork)
- **Relevance**: Decodes P25 FDMA PHY/MAC and LLC layers (BAAA/BAED stack base). Does not implement the SCEP/SNDCP/IP data path, but parsing the underlying LLC frames is the foundation for packet data. The OP25 project has some exposure to data PDUs in its frame parsing.
- **Osmocom OP25 wiki**: https://projects.osmocom.org/projects/op25/wiki/DecoderPage

### SDRTrunk (Java-based P25 trunking decoder)
- **Repository**: https://github.com/DSheirer/sdrtrunk
- **Relevance**: Implements P25 trunking control channel decoding (AABC/AABD) and traffic channel reassembly. Does not implement the full data bearer service stack, but TMS functions (Unit Registration, SNDCP Channel Management) appear on the trunking control channel that SDRTrunk decodes.

### gr-p25 (GNU Radio P25 blocks)
- **Repository**: https://github.com/gnuradio/gr-p25 (historical; largely superseded by OP25)
- **Relevance**: Early P25 PHY/MAC work that predates the BAEA packet data suite being widely implemented in SDR tools.

### Limitations of Open-Source Coverage
The SCEP and SNDCP protocol elements specified in BAEB (which BAEA references) are not publicly implemented in any known open-source project. SNDCP is closely related to the GSM/GPRS SNDCP specification (3GPP TS 04.65), and a GSM SNDCP implementation could serve as a reference starting point, though P25 SNDCP has P25-specific context management procedures.

The A Interface (IPv4 over PPP/USB or SLIP/USB per BAEG) is straightforward to implement using standard Linux PPP/SLIP drivers; no P25-specific code is needed for the A Interface physical and link layers.

---

## Phase 3 Note

This document is classified **OVERVIEW**. No implementation spec is required. The document contains no algorithms, message format tables, or state machine definitions — it is a pure architectural reference that directs implementors to BAEB, BAED, BAEF, and BAEG for those details.

**Recommended follow-up documents for implementation specs** (if not already processed):
- **TIA-102.BAEB** — IP Data Bearer Service Specification: MESSAGE_FORMAT + PROTOCOL; would produce SCEP parsing spec, SNDCP context management procedures, TMS message formats.
- **TIA-102.BAED** — Packet Data LLC Procedures: PROTOCOL; would produce LLC state machine and PDU format spec.
- **TIA-102.BAEG** — Mobile Data Peripheral Interface: MESSAGE_FORMAT; would produce A Interface serial framing spec.
- **TIA-102.BAEF** — Packet Data Host Network Interface: PROTOCOL/MESSAGE_FORMAT; would produce E_d Interface spec.
