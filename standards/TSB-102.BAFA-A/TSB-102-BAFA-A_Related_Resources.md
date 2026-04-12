# TSB-102.BAFA-A — Related Resources & Context

**Document:** TSB-102.BAFA-A (TIA/EIA/TSB102.BAFA-A)  
**Title:** Project 25 — Network Management Interface Overview  
**Approved:** July 1, 1999 | **Reaffirmed:** January 29, 2013

---

## Status

This document is **active** as of its reaffirmation on January 29, 2013. It is a TIA Telecommunications Systems Bulletin (TSB) — an informative document — not a normative TIA standard. TSBs are not subject to the same revision cycles as full standards; reaffirmation without change indicates the architectural content was considered still valid.

This bulletin is a **revision of TSB-102.BAFA** (the original APCO Network Management Interface document), which it supersedes. The document explicitly states in its revision history that it was proposed to supersede TSB-102.BAFA.

Notably, the normative MIB definitions, specific fault/performance object definitions, and detailed OMC-RF MIB specifications that this bulletin defers as "subject for further study" do not appear to have been published as separate TIA-102 standards in the public domain. The Network Management interface standardization within P25 has remained largely at the conceptual level described here, with vendors implementing proprietary OMC-RF solutions.

---

## Standards Family

This bulletin is part of the **TIA-102 Project 25 standards suite**, organized under the TR-8 committee (Mobile and Personal Private and Radio Standards), Subcommittee TR-8.19 (Wireline System Interfaces).

### Position in the TIA-102 Architecture

```
TIA-102 Project 25 Standards Suite
│
├── TSB-102-A  Project 25 System and Standards Definition [top-level overview]
│
├── Series A: Subscriber Equipment / Common Air Interface
│   └── (AAAA, AAAB, AAAC, AAAD, AABA, etc.)
│
├── Series B: Fixed Station / Infrastructure Interfaces
│   └── BAxx: RF Sub-system (RFSS) / Air Interface
│   └── BBxx: Fixed Station Internal Interfaces
│   └── BCxx: Conformance Testing
│   └── BDxx: (reserved)
│
└── BAFA family: Network Management Interface (En interface)
    ├── TSB-102.BAFA    [original APCO NMI document — superseded]
    └── TSB-102.BAFA-A  [this document — informative overview, 1999/2013]
        └── (normative OMC-RF MIB definitions deferred — not published)
```

### Key Companion Documents

| Document | Title | Relationship |
|----------|-------|--------------|
| TSB-102-A | Project 25 System and Standards Definition | Top-level P25 system overview; required reading per this bulletin |
| TSB-102.BAFA | Network Management Interface (original) | Predecessor; superseded by this document |
| TIA-102.BAAA-A | RF-Subsystem to RF-Subsystem Interface (RFSS-RFSS) | Defines G interface between RFSSs |
| TIA-102.BAAC | RF-Subsystem to Telephone Interconnect (Et interface) | Telephone interconnect interface |
| TIA-102.BACA | ISSI Voice and Mobility | Inter-RF-Subsystem Interface |

---

## Practical Context

### How the En Interface Is Used in Real Systems

In deployed P25 systems, the En interface concept describes how a **Network Management Center (NMC)** — typically a workstation or server running network management software — connects to and manages one or more **OMC-RF** (Operations and Maintenance Center - Radio Frequency) nodes, one per RF Sub-system (base site cluster).

In practice:
- Major P25 infrastructure vendors (Motorola Solutions, Harris/L3Harris, Kenwood, EF Johnson) implement proprietary OMC-RF software (e.g., Motorola's StarGate, L3Harris's OpenSky/Unity management systems).
- These proprietary systems expose network management functions via vendor-specific GUIs and APIs rather than through a standardized SNMP MIB as described in this bulletin.
- The SNMP-over-IP transport recommended here is widely used for general network element monitoring (via standard MIB-II objects for IP/link-layer statistics), but the P25-specific managed objects (base radio status, channel availability, call performance counters) have not been standardized in an OMC-RF MIB.
- Multi-vendor NMC interoperability via a standard En/SNMP interface has not been widely achieved in the field. Most large P25 systems use a single infrastructure vendor, or integrate through vendor-specific APIs.

### SNMP in P25 Infrastructure Context

The choice of SNMP (RFC 1157) in this bulletin was forward-looking for 1999. SNMP v1/v2c is widely supported in P25 infrastructure components for:
- Basic device health monitoring (CPU, memory, link status via MIB-II)
- Trap-based alerting for device failures

SNMP v3 (security enhancements) is now the recommended baseline in modern deployments, though the standard references only v1.

### X-Window Display Note

The bulletin's approach for Configuration, Accounting, and Security Management — making these available via X-window display from the OMC-RF platform rather than standardizing the interface — reflects 1990s Unix/workstation practice. Modern systems use web-based management consoles, REST APIs, or NETCONF/YANG. This aspect of the architecture is largely obsolete in current deployments.

---

## Key Online Resources

### TIA Standards

- **TIA Standards Store:** https://www.tiaonline.org/standards/catalog/
  (TSBs and standards available for purchase)
- **TIA TR-8 Committee information:** https://www.tiaonline.org/standards/technology/mobile-personal-private-and-radio-standards/

### Project 25 Technology Interest Group (PTIG)

- **PTIG (P25 Technology Interest Group):** https://www.project25.org/
  Industry group coordinating P25 implementation; publishes implementation guides and compliance documentation.

### APCO Project 25

- **APCO International P25 resources:** https://www.apcointl.org/technology/project-25/
  Original standards-development organization for P25; documents on P25 history and deployment.

### DHS SAFECOM / CISA

- **SAFECOM P25 resources:** https://www.cisa.gov/safecom/p25
  U.S. government resources for P25 procurement and interoperability, including compliance testing results.

### P25 Compliance Assessment Program (CAP)

- **P25 CAP:** https://www.dhs.gov/p25-cap
  DHS program for testing P25 equipment compliance; includes conformance test results for major vendors. Network Management conformance is not currently a tested area.

### SNMP / MIB References

- **RFC 1157 (SNMP v1):** https://www.rfc-editor.org/rfc/rfc1157
- **RFC 1213 (MIB-II):** https://www.rfc-editor.org/rfc/rfc1213
- **ITU M.3010 (TMN Principles):** Available from ITU-T (https://www.itu.int/rec/T-REC-M.3010)
- **ITU X.701 (OSI System Management):** Available from ITU-T (https://www.itu.int/rec/T-REC-X.701)

---

## Open-Source Implementations

### P25 Infrastructure / Protocol Projects

The En interface as defined in this bulletin (SNMP-based OMC-RF MIB) has no known open-source implementations because the normative MIB definitions were never published. However, the broader P25 protocol stack is implemented in:

- **OP25** (GNU Radio-based P25 receiver/transmitter):
  https://github.com/boatbod/op25
  Implements P25 air interface (CAI) protocols; no network management interface.

- **SDRTrunk** (Java-based P25 decoder):
  https://github.com/DSheirer/sdrtrunk
  Decodes P25 FDMA/TDMA trunking; monitors trunking control channels; no NMI implementation.

- **p25rx** and related GNU Radio blocks:
  Various forks implementing P25 physical and link layer; no NMI.

### SNMP Management Frameworks (General)

For implementing SNMP-based network management as described:

- **Net-SNMP:** https://github.com/net-snmp/net-snmp
  Widely-used SNMP agent and manager implementation (C); used as the basis for many embedded SNMP agents.

- **snmp-rs (Rust):** https://github.com/davedufresne/snmp-rs
  Rust SNMP client library.

- **rasn (Rust ASN.1):** https://github.com/XAMPPRocky/rasn
  Rust ASN.1 library relevant for SNMP PDU encoding/decoding.

---

## Standards Lineage

```
APCO Project 25 (initiated 1989)
│
└── TIA TR-8 Committee standardization (early 1990s)
    │
    └── TIA-102 standards suite
        │
        └── Network Management Interface family
            │
            ├── TSB-102.BAFA
            │   (original APCO NMI document, pre-1995)
            │   [based on CMIP; CMIP later removed]
            │   └── SUPERSEDED BY:
            │
            └── TSB-102.BAFA-A (July 1999)
                (revision: CMIP removed, SNMP confirmed)
                (reaffirmed: January 29, 2013)
                │
                └── Normative OMC-RF MIB specification
                    [deferred — "subject for further study"]
                    [not published as of reaffirmation]

Reference standards incorporated:
├── ITU M.3010 — TMN Principles (Manager/Agent/mediation model)
├── ITU X.701 — OSI System Management (Manager/Agent definitions)
├── RFC 1157  — SNMP v1 (transport protocol)
└── RFC 1213  — MIB-II (Internet standard MIB framework)
```

---

## Notes for Implementers

1. **No normative MIB exists.** This bulletin establishes the architecture but the OMC-RF MIB object definitions were explicitly deferred and never published as a TIA standard. Any implementation must define its own MIB schema.

2. **SNMP version.** The document references SNMPv1 (RFC 1157). Modern deployments should use SNMPv3 for authentication and privacy. SNMPv2c is widely supported as a minimum.

3. **Rust / open-source P25 note.** If implementing an OMC-RF agent in Rust, the `rasn` crate handles ASN.1/BER encoding needed for SNMP PDUs. The `net-snmp` C library can be wrapped via FFI. No Rust SNMP agent framework with full MIB registration exists at production quality as of 2024.

4. **Scope boundary.** The En interface standardizes only the NMC ↔ OMC-RF boundary. Everything below the OMC-RF (to individual base radios, base audio cards, controllers) is vendor-proprietary and not standardized by this or any other TIA-102 document.
