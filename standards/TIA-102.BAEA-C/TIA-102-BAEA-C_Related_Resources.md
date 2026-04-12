# TIA-102.BAEA-C — Related Resources and Context

## Status

**Active.** ANSI/TIA-102.BAEA-C-2015 was approved December 9, 2015 and adopted by the Project 25 Steering Committee on October 29, 2015. It is the current revision of the Data Overview and Specification document. It cancels and replaces TIA-102.BAEA-B (February 2012).

Revision history:
- Interim Standard, February 1995 (as TIA/EIA-PN-3373)
- Original (TIA/EIA-102.BAEA), February 2000
- Revision A — May 2004 (added USB/PPP A interface addendum)
- Revision B — February 2012 (comprehensive restructure; removed circuit data bearer service)
- **Revision C — December 2015 (current; errata corrections, TR-8.5 alignment)**

No superseding document has been published as of this writing. The BAEA designator in the TIA-102 suite is the permanent home for the Packet Data overview; it is not expected to be replaced by a different designator.

---

## Standards Family

This document sits at the top of the TIA-102 Packet Data sub-suite within the broader TIA-102 (Project 25) standards family.

### TIA-102 Packet Data Document Suite (BAEA–BAEJ)

| Designator | Title | Status |
|-----------|-------|--------|
| **BAEA-C** | **Data Overview and Specification** *(this document)* | Active |
| BAEB-B | IP Data Bearer Service Specification | Active |
| BAEC | *(withdrawn)* | Withdrawn |
| BAED | Packet Data Logical Link Control Procedures | Active |
| BAEE-C | Radio Management Protocols | Active |
| BAEF | Packet Data Host Network Interface | Active |
| BAEG | Mobile Data Peripheral Interface | Active |
| BAEH | IP Data Security Service Specification | Future (reserved) |
| BAEI | *(not used)* | N/A |
| BAEJ | Conventional Management Service Specification for Packet Data | Active |

### Key Upstream Dependencies

| Document | Role |
|---------|------|
| TSB-102.B | TIA-102 Documentation Suite Overview — defines the Open System Interface Model |
| TIA-102.BAAA-A | FDMA Common Air Interface — defines PHY/MAC layer used by all FDMA packet data |
| TIA-102.AAAD-A | Block Encryption Protocol — used for encrypted Um Packet Data |
| TIA-102.AABA-B | Trunking Overview — context for Trunked FNE Data TMS functions |

### Supplementary Services Cross-References

| Document | Title |
|---------|-------|
| TIA-102.AABC-D | Trunking Control Channel Messages |
| TIA-102.AABD-B | Trunking Procedures |
| TIA-102.AABG | Conventional Control Messages |
| TIA-102.BAAD-B | Conventional Procedures |
| TIA-102.BACD-B | ISSI Messages and Procedures for Supplementary Data |
| TIA-102.BACE | ISSI Messages and Procedures for Conventional Operation |

### ISSI/FSSI/CSSI Wireline Interfaces

| Document | Role |
|---------|------|
| TSB-102.BACC-B | Inter-RF Subsystem Interface Overview (G interface) |
| TIA-102.BAHA | Fixed Station Interface Messages and Procedures (Ef interface) |
| TSB-102.BAGA | Console Subsystem Interface Overview (Ec interface) |

---

## Standards Lineage (ASCII Tree)

```
TIA-102 (Project 25 Standards Suite)
│
├── TSB-102.B  ← Documentation Suite Overview / Open System Interface Model
│
├── TIA-102.BAAA-A  ← FDMA Common Air Interface (PHY/MAC)
│
└── [Packet Data Sub-Suite]
    │
    ├── TIA-102.BAEA-C  ← DATA OVERVIEW & SPECIFICATION (this document)
    │       ↓ delegates to:
    ├── TIA-102.BAEB-B  ← IP Data Bearer Service (SCEP, SNDCP)
    ├── TIA-102.BAED    ← Logical Link Control Procedures
    ├── TIA-102.BAEE-C  ← Radio Management Protocols (A interface mgmt)
    ├── TIA-102.BAEF    ← Packet Data Host Network Interface (Ed)
    ├── TIA-102.BAEG    ← Mobile Data Peripheral Interface (A)
    ├── TIA-102.BAEJ    ← Conventional Management Service for Packet Data
    └── TIA-102.BAEH    ← IP Data Security Service (future)
```

---

## Practical Context

### Role in Real-World P25 Systems

This document defines the "what and how" of P25 data at the architectural level. In practice, every P25 radio that supports data — whether it's a portable radio connecting to a CAD/dispatch system, an AVL (Automatic Vehicle Location) unit, or a radio performing Over-the-Air Rekeying (OTAR) — operates within the framework established here.

The four packet data configurations correspond to real deployment scenarios:

- **Direct Data** is used in peer-to-peer scenarios where two radios exchange data directly over the air, most commonly for Tier 1 Location Services (GPS position reports using the CAI bearer).
- **Repeated Data** extends the range of Direct Data by routing through a conventional fixed station repeater. The repeater operates as a pure PHY/MAC relay — it has no awareness of the LLC or IP layers above it.
- **Conventional FNE Data** is the workhorse configuration for conventional systems. The radio connects to an FNE (typically a site controller or network infrastructure node) over a conventional channel. Used for Tier 2 Location, OTAR, and application data.
- **Trunked FNE Data** is the equivalent for trunked systems. The radio uses SNDCP over LLC, with TMS signaling on the trunked control channel for channel assignment and registration. This is the configuration used in large trunked P25 systems for IP data (MDT/AVL applications).

The CMS (Conventional Management Service) layer is specific to conventional channels and handles registration, channel scanning, and encryption key metadata. SNDCP handles context management (IP address assignment, authentication) for both conventional and trunked FNE scenarios.

### Equipment Implementations

P25 vendors that implement the packet data stack include Motorola Solutions (APX and ASTRO series), Harris (now L3Harris, XL and BeOn series), Kenwood, and others. Infrastructure vendors implementing the FNE/network side include Motorola Solutions (SmartZone, MCC7500), Zetron, and others. The data standards are required for P25 Phase 1 compliant equipment.

### Application-Level Use Cases Referenced in This Document

- **Tier 1 Location Service** (TSB-102.BAJA-A): Uses CAI Data Bearer Service over Direct Data or Repeated Data configurations. GPS position in CAI format.
- **Tier 2 Location Service** (TSB-102.BAJA-A): Uses IP Data Bearer Service. IPv4 datagrams carrying location data.
- **Data Link Dependent OTAR** (TIA-102.AACA-A): Uses CAI Data Bearer Service over Conventional FNE Data. Rekeying material transported as CAI packets.
- **Data Link Independent OTAR** (TIA-102.AACA-A): Uses IP Data Bearer Service over Conventional or Trunked FNE Data. Rekeying over IP.

---

## Key Online Resources

- **TIA Standards Catalog** (purchase/status): https://www.tiaonline.org/standards/catalog/
- **IHS Markit / Accuristech** (purchase): https://store.accuristech.com/tia
- **P25 Technology Interest Group (P25 TTIG)**: https://www.project25.org/ — Industry consortium documentation on P25 data implementations
- **PTIG Compliance Assessment Program (CAP)**: Tests P25 interoperability including packet data features; results published at https://www.project25.org/index.php/capsite
- **NPSTC (National Public Safety Telecommunications Council)**: https://www.npstc.org/ — Publishes P25 deployment guides referencing data services
- **DHS SAFECOM Program**: https://www.cisa.gov/safecom — Federal guidance on P25 systems including data capabilities
- **APCO International**: https://www.apcointl.org/ — Original requestor of the P25 standard; publishes technical resources

---

## Open-Source Implementations

The P25 packet data stack is partially or fully implemented in several open-source projects:

### OP25 (GNU Radio-based P25 decoder/transceiver)
- **Repository**: https://github.com/boatbod/op25
- **Relevance**: Implements P25 FDMA PHY/MAC (BAAA), LLC (BAED), and partial packet data handling. The LLC layer implementation is directly relevant to the protocol stack defined in this document. Does not implement SCEP or SNDCP at the IP bearer level.

### SDRTrunk
- **Repository**: https://github.com/DSheirer/sdrtrunk
- **Relevance**: Java-based P25 decoder that decodes trunked and conventional P25 channels including data traffic. Handles the MAC/LLC layers. Relevant to understanding how Conventional FNE Data and Trunked FNE Data frames appear on the air.

### dsd (Digital Speech Decoder)
- **Repository**: https://github.com/szechyjs/dsd
- **Relevance**: Decodes P25 Phase 1 frames including data frames at the PHY/MAC level. Does not implement the full packet data stack.

### p25lib (various)
- Multiple independent implementations exist in Python and C for research purposes; search GitHub for "p25 packet data" or "p25 LLC".

### Note on SCEP and SNDCP
No complete open-source implementation of SCEP (TIA-102.BAEB-B) or SNDCP (also defined in BAEB-B) is known as of this writing. These layers would need to be implemented to fully support the IP Data Bearer Service. The BAEB-B document specifies the detailed procedures.

---

## Related Academic and Technical Literature

- NIST SP 800-137A and FIPS references apply to P25 encryption (AES DES OTAR) used in conjunction with encrypted packet data
- DHS Science and Technology Directorate has published assessments of P25 security including data service vulnerabilities (2011 report on P25 security)
- Various APCO Annual Conference proceedings include papers on P25 data service deployment experiences
