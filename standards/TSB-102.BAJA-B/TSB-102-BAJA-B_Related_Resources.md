# TSB-102-BAJA-B Related Resources and Context

**Document:** TSB-102.BAJA-B — Project 25 Location Services Overview  
**Published:** November 2017 (Revision B)  
**Researched:** April 2026

---

## Status

**Active.** This document is the current (third) edition of the Location Services Overview bulletin, having cancelled and replaced TSB-102.BAJA-A (February 2010). No supersession has been published as of the research date.

This is a non-normative overview bulletin (TSB = Telecommunications Systems Bulletin), not a normative ANSI/TIA standard. It carries no mandatory requirements of its own; its role is architectural orientation for the normative Location Services specifications it introduces.

**Note on companion documents:** The normative specifications referenced within this bulletin have been revised since its 2017 publication:
- TIA-102.BAJB-A (Tier 1 Location Services, November 2014) has been superseded by **TIA-102.BAJB-B** (December 2019, ANSI approved, active).
- TIA-102.BAJC-A (Tier 2 Location Services, April 2015) has been superseded by **TIA-102.BAJC-B** (March 2019, ANSI approved, active).

The architectural framework described in this bulletin remains consistent with those updated specifications; the overview itself has not required a corresponding revision.

---

## Standards Family

This document is part of the **TIA-102 Location Services sub-suite** within the broader TIA-102 (Project 25) standards family.

### Location Services Document Suite

| Document | Title | Role |
|----------|-------|------|
| TSB-102.BAJA-B (this document) | Location Services Overview | Non-normative architecture overview |
| TIA-102.BAJB-B | Tier 1 Location Services Specification | Normative: NMEA 0183 over CAI, Direct/Repeated Data |
| TIA-102.BAJC-B | Tier 2 Location Services Specification | Normative: LRRP/UDP/IP, all four data configurations |

### Key Dependencies (as cited within this document)

| Reference | Document | Purpose |
|-----------|----------|---------|
| [1] | TIA-102.BAEA-C | Data Overview and Specification — defines the four packet data configurations (Direct, Repeated, Conventional FNE, Trunked FNE) and the IP/CAI Data Bearer Services |
| [4] | TSB-102-C | TIA-102 Documentation Suite Overview — defines CAI, SU, and other foundational terms |

### Standards Lineage (ASCII Tree)

```
TIA-102 (Project 25 Standards Suite)
└── Data Services Sub-Suite
    ├── TIA-102.BAEA-C  Data Overview and Specification
    │   (defines packet data configurations: Direct, Repeated, Conv FNE, Trunked FNE)
    │
    └── Location Services Sub-Suite
        ├── TSB-102.BAJA-B  Location Services Overview [this document]
        │   (non-normative; supersedes TSB-102.BAJA-A)
        ├── TIA-102.BAJB-B  Tier 1 Location Services Specification
        │   (NMEA 0183 over CAI Data Bearer Service; Direct/Repeated Data only)
        │   └── supersedes TIA-102.BAJB-A (Nov 2014)
        └── TIA-102.BAJC-B  Tier 2 Location Services Specification
            (LRRP/XML/UDP/IP; all four data configurations; compression required)
            └── supersedes TIA-102.BAJC-A (Apr 2015)
```

---

## Practical Context

### Use in Real-World Systems

P25 Location Services are widely deployed across public safety agencies in North America. The architectural model described in this document maps directly onto real-world products:

**Tier 1 (NMEA 0183 over CAI):** Used in field incident command scenarios where portable radios exchange position using the radio's built-in GPS and the direct data channel. No infrastructure investment required beyond the radios themselves. Many P25 portables from manufacturers such as Motorola, Harris (now L3Harris), Kenwood, and BK Radio implement Tier 1 location natively. The LSHS in Tier 1 is typically a ruggedized laptop or tablet running commercial mapping software.

**Tier 2 (LRRP over IP Data Bearer):** Used in agency-wide dispatch and tracking deployments. The FDH role is typically fulfilled by a server running CAD (Computer Aided Dispatch) integration middleware. The LSHS is often integrated into the CAD system itself or a standalone mapping application such as those from Esri (ArcGIS), Motorola PremierOne, or similar. Compression is handled transparently within the radio and infrastructure stack.

**MDP configurations** are less common in modern deployments, as most current-generation P25 portables and mobiles incorporate GPS hardware directly. Legacy deployments where the GPS was an external serial device connected to the radio's accessory port correspond to the MDP-as-LIS-relay topology described in this document.

**Agency trigger configuration** — the ability to enable/disable each triggering condition per agency — is a key operational requirement. Agencies commonly configure emergency-mode automatic reporting (continuous retransmit without PTT) and timed periodic reporting during planned operations, while disabling PTT-based reporting during normal operations to avoid channel congestion.

### Relationship to Broader P25 Data Services

Location Services ride on top of the packet data infrastructure defined in TIA-102.BAEA-C. An agency must have P25 data services operational before location services can function. In trunked systems, this means the Site Controller and RFSS must support the IP Data Bearer Service. Many early P25 systems deployed voice-only infrastructure and added data capability later; location services are therefore often a second-phase deployment in agency migration projects.

---

## Key Online Resources

- **TIA Standards Catalog** — Search for TSB-102.BAJA, TIA-102.BAJB, TIA-102.BAJC:  
  https://www.tiaonline.org/standards/catalog/

- **Accuris/IHS standards purchase** (TSB-102.BAJA-B):  
  https://store.accuristech.com/tia

- **GlobalSpec listing** (TSB-102.BAJA):  
  https://standards.globalspec.com/std/10193926/tia-tsb-102-baja

- **GlobalSpec listing** (TIA-102.BAJB-B, Tier 1, current):  
  https://standards.globalspec.com/std/14215338/TIA-102.BAJB

- **GlobalSpec listing** (TIA-102.BAJC-B, Tier 2, current):  
  https://standards.globalspec.com/std/13225702/TIA-102.BAJC

- **Project 25 Technology Interest Group (PTIG) Capabilities Guide v1.7** — covers location services capabilities from a user perspective:  
  https://project25.org/images/stories/ptig/docs/PTIG_P25Capabilities_Guide_v1.7.pdf

- **CISA SAFECOM Project 25 Overview**:  
  https://www.cisa.gov/safecom/project-25

- **NIST PSCR Location-Based Services R&D Roadmap** — government research context for public safety location:  
  https://www.nist.gov/publications/public-safety-communications-research-pscr-program-location-based-services-rd-roadmap

- **NMEA 0183 specification** (used by Tier 1 for location formatting):  
  https://www.nmea.org/nmea-0183.html

---

## Open-Source Implementations

LRRP (Tier 2) has received open-source implementation attention primarily because it carries actual GPS coordinates decodable from the radio channel — making it useful for passive monitoring of P25 traffic.

### SDRTrunk
The primary open-source P25 decoder with LRRP support. Decodes and plots GPS positions from P25 LRRP packets captured via SDR.
- GitHub: https://github.com/DSheirer/sdrtrunk
- LRRP GPS plotting issue tracker: https://github.com/DSheirer/sdrtrunk/issues/2254

### DSD-FME
P25 decoder that generates LRRP output files compatible with QGIS for mapping decoded location reports.
- GitHub: https://github.com/lwvmobile/dsd-fme

### Pocket25
Mobile P25 radio decoder with GPS/LRRP decoding capability.
- GitHub: https://github.com/SarahRoseLives/Pocket25

### OP25
The GNU Radio-based P25 decoder. Has basic data channel support but LRRP decoding is limited compared to SDRTrunk.
- GitHub: https://github.com/osmocom/op25

**Note:** None of these projects implement the full P25 Location Services stack (SU-side triggering, LSHS application, FDH routing). They implement the receive/decode side only. The normative Tier 1 (NMEA 0183 framing) and Tier 2 (LRRP protocol + compression) specifications for a full SU or FDH implementation are defined in TIA-102.BAJB-B and TIA-102.BAJC-B respectively.

---

## Implementation Notes for Developers

- **Tier 1** requires only NMEA 0183 sentence parsing (specifically GPGGA for position data). The CAI Data Bearer Service Service Access Point used for Tier 1 is defined in TIA-102.BAJB. No IP stack required in the SU for Tier 1.

- **Tier 2** requires: UDP/IP stack in the SU, LRRP (XML-based protocol) implementation, and a compression codec. The compression method for Tier 2 is specified in TIA-102.BAJC-B. XML verbosity makes compression non-optional for reasonable over-the-air efficiency.

- **LIS-to-SU and SU-to-LSHS interfaces are deliberately not standardized.** Implementers have freedom to use any protocol for these local interfaces. Common approaches include NMEA 0183 serial input for GPS receivers (LIS to SU) and TCP sockets or REST APIs for LSHS integration.

- **Agency trigger configuration** must be supported: the standard requires that all triggering options be individually enable/disable configurable per agency. Hard-coding triggers would be non-conformant.

- **Group-oriented location services** are explicitly out of scope. Implementations should not expect standardized behavior for talkgroup-based location tracking.
