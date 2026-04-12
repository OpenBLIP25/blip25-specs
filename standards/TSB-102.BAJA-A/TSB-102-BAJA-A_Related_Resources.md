# TSB-102.BAJA-A — Related Resources & Context

**Document:** TSB-102.BAJA-A, Project 25 Location Services Overview
**Published:** February 2010
**Issuing body:** TIA TR-8.5 Signaling and Data Transmission Subcommittee

---

## Status

**Active as of 2026.** TSB-102.BAJA-A (Issue A, rev. 1, February 2010) is the current edition. It superseded TSB-102.BAJA (Issue O, first published 2009). No subsequent revision has been identified. As a Telecommunications Systems Bulletin (TSB), it is informative rather than normative — it provides architectural guidance for the normative Tier 1 and Tier 2 location service specifications. It is available for purchase from TIA at https://www.tiaonline.org/standards and through IHS/Accuristech at https://store.accuristech.com/tia.

---

## Standards Family

This document is part of the TIA-102 (Project 25 / APCO P25) suite. It specifically heads the P25 Location Services sub-suite:

### P25 Location Services Document Suite

| Document | Title | Role |
|---|---|---|
| **TSB-102.BAJA-A** | P25 Location Services Overview | Architecture overview (this document) |
| TIA-102.BAJB | P25 Tier 1 Location Service (Dedicated SAP) | Normative Tier 1 spec — NMEA 0183 over SAP 0x30 |
| TIA-102.BAJC | P25 Tier 2 Location Service (LRRP) | Normative Tier 2 spec — LRRP protocol |

### Foundational P25 Standards Referenced

| Document | Content |
|---|---|
| TIA-102.BAAA-A | FDMA Common Air Interface (CAI) |
| TIA-102.BAAC-B | Common Air Interface Reserved Values (SAP assignments) |
| TIA-102.BAEB-A | P25 Packet Data Specification (SNDCP, SCEP, Ed, A interfaces) |
| TIA-102.BACF | P25 Inter-RF Subsystem Interface for Packet Data (ISSI/G interface) |
| TIA-102.BAHA | Fixed Station Interface (noted as lacking data support at time of publication) |
| TIA-102 (base) | APCO P25 System and Standards Definition |

### External Protocol Standards

| Document | Content |
|---|---|
| NMEA 0183 v3.01 (Jan 2002) | GPS sentence formats used by Tier 1 |
| RFC 768 (UDP) | Transport protocol for Tier 2 |
| RFC 791 (IP) | Network protocol for Tier 2 |
| W3C EXI (Sep 2008) | Efficient XML Interchange — Tier 2 air interface compression |
| OMA LIF TS 101 v3.0.0 | Mobile Location Protocol — LRRP ancestor |
| JSR-179 (J2ME Location API) | Java Location API — LRRP ancestor |

---

## Standards Lineage

```
TIA-102 (P25 System and Standards Definition)
└── TIA-102.BA__ (Air Interface / Packet Data Layer)
    ├── TIA-102.BAAA-A  FDMA Common Air Interface
    ├── TIA-102.BAAC-B  CAI Reserved Values
    ├── TIA-102.BAEB-A  Packet Data (SNDCP, SCEP, Ed, A interfaces)
    └── TIA-102.BACF    ISSI Packet Data (G interface)
└── TIA-102.BAJ__ (Location Services)
    ├── TSB-102.BAJA    Location Services Overview (Issue O, 2009)
    │   └── TSB-102.BAJA-A  Location Services Overview [THIS DOCUMENT] (Issue A, 2010)
    ├── TIA-102.BAJB    Tier 1 Location Service (Dedicated SAP / NMEA 0183)
    └── TIA-102.BAJC    Tier 2 Location Service (LRRP)
        └── [Uses] W3C EXI compression over air interface

External standards feeding into LRRP:
    OMA/LIF Mobile Location Protocol (LIF TS 101)
    WAP Forum Location Standard
    Java Community Process JSR-179 (J2ME Location API)
```

---

## Practical Context

### Real-World Deployment

P25 Location Services are deployed across public safety radio networks in North America and are used by law enforcement, fire, EMS, and emergency management agencies. The primary consumer-facing manifestation is **Automatic Vehicle Location (AVL)** mapping at dispatch centers, which shows dispatcher consoles the real-time positions of all units in the field.

Practical deployment breaks down along the two tiers:

**Tier 1 (Dedicated SAP / NMEA 0183)** is the simpler path. SUs equipped with internal GPS broadcast NMEA GGA sentences on SAP 0x30. A second radio (or the same radio in a DMR gateway role) receives these and forwards them to mapping software running on the dispatcher's laptop. This tier is favored in conventional systems — volunteer fire, smaller agencies, and field incident command — where the LSHS runs on portable hardware and no fixed infrastructure is available or needed.

**Tier 2 (LRRP / UDP/IP)** is the enterprise path. It requires a trunked or IP-capable conventional infrastructure with SNDCP or SCEP data services active. Location data is routed as IP datagrams through the RFSS to a Fixed Data Host on the agency's customer data network, where CAD (Computer Aided Dispatch) or standalone AVL software receives and displays it. Major CAD vendors (TriTech, Hexagon/Intergraph, Tyler Technologies) and standalone AVL products (Motorola PremierOne CAD, various third-party mapping platforms) support the LRRP-over-UDP/IP interface.

The MDP path (A interface) is used in vehicle installations where a laptop or MDT (Mobile Data Terminal) serves as both GPS source and mapping display. The MDP connects via USB/RS-232 to the radio and handles UDP/IP encapsulation.

### Triggering in Practice

Agencies most commonly enable Periodic triggering (typically 30–60 second intervals for routine tracking) and Emergency triggering (immediate burst on emergency button activation). PTT triggering is popular in conventional systems as it provides near-real-time position updates without dedicated air time. Distance Change triggering is used where bandwidth is constrained and periodic traffic would be excessive.

### Interoperability Caveat

A key practical limitation noted in this document remains relevant: **Tier 1 (Dedicated SAP) and Tier 2 (SCEP) units cannot interoperate on the same conventional channel** even though both support Direct and Repeated modes. Agencies deploying mixed fleets must standardize on a single air interface method or deploy separate channels per method.

### Group Location Services

Group-oriented location (sending position to all members of a talkgroup simultaneously) was explicitly out of scope at publication and remained so through the 2010 revision. This is a known gap; implementations that provide talkgroup AVL typically do so through LSHS-side fan-out rather than air-interface multicast.

---

## Key Online Resources

- **TIA Standards catalog** — https://www.tiaonline.org/standards (search "TSB-102.BAJA")
- **IHS/Accuristech TIA store** — https://store.accuristech.com/tia
- **APCO International P25 resources** — https://www.apcointl.org/technology/p25/
- **PTIG (P25 Technology Interest Group)** — https://www.p25technologyinterestgroup.org/ — publishes P25 ISSI/CSSI/AVL implementation guides and interoperability test results
- **DHS SAFECOM P25 CAP** — https://www.cisa.gov/safecom/p25 — the DHS Compliance Assessment Program tests P25 equipment including location services; test summaries publicly available
- **NMEA 0183 standard** — https://www.nmea.org/content/STANDARDS/NMEA_0183_Standard (purchase required)
- **W3C EXI specification** — https://www.w3.org/TR/exi/ (free)
- **OMA LIF Mobile Location Protocol (LIF TS 101)** — archived at https://www.openmobilealliance.org/tech/affiliates/lif/lifindex.html

---

## Open-Source Implementations

P25 Location Services are partially implemented across several open-source P25 projects. None implement the full LRRP Tier 2 specification as a standalone library, but the following are relevant:

### SDRTrunk
- **Repository:** https://github.com/DSheirer/sdrtrunk
- **Relevance:** SDRTrunk decodes P25 FDMA and TDMA signals including packet data channels. It can decode SNDCP and display decoded location payloads from Tier 1 (NMEA 0183 over SAP 0x30). Tier 2 LRRP parsing is partial. The codebase in `src/main/java/io/github/dsheirer/module/decode/p25/` handles CAI, SNDCP, and SCEP decoding.

### OP25
- **Repository:** https://github.com/boatbod/op25
- **Relevance:** GNU Radio-based P25 receiver. Decodes packet data channels including SNDCP. Location data appearing on SAP 0x30 or SNDCP channels can be extracted. Limited LRRP decoding. Active development community.
- **Related:** https://osmocom.org/projects/op25

### p25-avl (community projects)
- Several community GitHub repositories have experimented with parsing LRRP datagrams received from RFSS UDP feeds. Search GitHub for "p25 avl lrrp" for current state. These are typically single-author proof-of-concept tools.

### Wireshark P25 Dissector
- Wireshark includes P25 (APCO-25) protocol dissectors that can decode CAI, SNDCP, and SCEP. Limited support for LRRP payload dissection. Useful for debugging location service implementations.
- https://www.wireshark.org/

### OpenMHz / Trunk Recorder
- **Repository:** https://github.com/robotastic/trunk-recorder
- Not specifically focused on location services, but captures P25 trunked traffic including data channels where SNDCP location datagrams may appear.

---

## Implementation Notes for Developers

The following observations are intended to aid implementers working from this architectural overview toward actual implementation:

1. **SAP 0x30 is the entry point for Tier 1.** The SAP is defined in TIA-102.BAAC-B. NMEA 0183 GGA sentences are the expected payload. An SU need only format a GGA sentence and encapsulate it in a Um data packet with SAP 0x30 to be Tier 1 compliant.

2. **LRRP compression is mandatory on-air.** The EXI schema for LRRP is defined alongside TIA-102.BAJC. The Fixed Data Host / MDP boundary is where compression/decompression happens — the LSHS API always sees uncompressed LRRP/XML. Implementers of FDH or MDP software must handle EXI encode/decode; the radio and RFSS pass the compressed bytes transparently.

3. **UDP port is the protocol discriminator.** Both RFSS→FDH (Ed interface) and SU→MDP (A interface) use the UDP port number to identify the location information protocol. This means a single FDH IP address can serve multiple services; location data is identified by its assigned UDP port, not by IP address alone.

4. **SCEP vs. SNDCP selection.** On Conventional systems, SCEP is the more common choice; on Trunked systems, SNDCP is the only option. SNDCP supports both confirmed (SN-Data) and unconfirmed (SN-UData) delivery — unconfirmed is typically used for location to avoid the overhead of acknowledgment on a chatty periodic trigger.

5. **Freshness logic.** The SU is required to track whether buffered location data is stale (e.g., GPS fix lost). Stale data may be retransmitted up to 4 times on autonomous triggers; it is always acceptable to retransmit stale data in response to a Host Request. This needs to be handled in SU firmware.

6. **Roaming and ISSI.** For roaming SUs on Trunked systems, the Visited RFSS routes location IP packets over the G (ISSI) interface to the Home RFSS, which then forwards them to the FDH. No application-layer changes are needed — the routing is transparent to LRRP. This means Tier 2 location works across ISSI-connected P25 systems with no additional protocol work.
