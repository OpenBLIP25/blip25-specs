# TSB-102-BAFA (APCO) — Related Resources & Context
# APCO Project 25 Network Management Interface
# Generated: 2026-04-13

---

## Status

**Original document:** TSB102.BAFA, December 1994 — superseded.

This was an early-stage Technical Systems Bulletin published under the original APCO/EIA TSB-102 numbering scheme before the TIA-102 suite was fully restructured. It was intentionally marked as preliminary, with several referenced companion documents listed as [TBD] at publication.

**Successor document:** TSB102.BAFA-A (July 1999) — "P25 Network Management Interface Overview" — published under the restructured TIA-102 series numbering. This 1999 revision updated the interface description as the P25 standard matured through the late 1990s.

**Current status:** The NMI area in P25 has continued to evolve. The broader TIA-102.BAF\* sub-series covers infrastructure interfaces and network management. Modern P25 systems typically use SNMP-based management with vendor-defined MIBs, as GOSIP/CMIP was phased out of U.S. government procurement by the late 1990s (GOSIP was withdrawn as a mandatory procurement requirement around 1994-1995, shortly after this document was published).

The P25 Network Management Interface as such is not a single fully standardized MIB — vendors have generally implemented proprietary SNMP MIBs for their P25 infrastructure, with interoperability achieved at the application/manager layer rather than through a common ASN.1 module as this document envisioned.

---

## Standards Family

This document is part of the TIA-102 (Project 25) suite, under the infrastructure/network management sub-family:

```
TIA/EIA TSB-102 (1993) — P25 System and Standards Definition (root document)
└── TSB102.BAFA (1994) — Network Management Interface [this document]
    └── TSB102.BAFA-A (1999) — NMI Overview (successor)
```

**Companion documents in the P25 suite relevant to infrastructure interfaces:**

| Document | Topic |
|---|---|
| TSB-102 / TIA-102.BAAA | P25 System and Standards Definition |
| TIA-102.BAAA-A | Common Air Interface (FDMA) |
| TIA-102.BABA | P25 IMBE Half-Rate Vocoder |
| TIA-102.BAAC | Reserved Values and Guidelines |
| TIA-102.BAEA series | Console Subsystem Interface (CSSI) |
| TIA-102.BAEB series | Fixed Station Interface (FSI) |
| TIA-102.BACA series | Inter-RF Subsystem Interface (ISSI) |
| TIA-102.BAEG series | Data Gateway Interface |
| TIA-102.BBAD | Conventional Channel Gateway (CCGW) |

The NMI ("En interface" in P25 terminology) sits alongside these peer-level infrastructure interfaces rather than on the radio channel path.

---

## Practical Context

The problem this document addresses is real and enduring: public safety radio infrastructure from different vendors must be manageable from a common network operations center (NOC). In P25 systems, the fixed infrastructure includes base stations (repeaters), site controllers, RF switches (zone controllers), and gateways — all of which need to be monitored for faults, performance, and configuration.

**What actually happened in practice:**

The vision of a common, publicly registered ASN.1 MIB for P25 RF subsystems — which this document called for — was never fully realized in a standardized form. Instead:

1. **SNMP won, GOSIP lost.** Within a year of this document's publication, GOSIP was de-mandated in U.S. federal procurement. All practical P25 network management implementations use SNMP, not CMIP.

2. **Vendor-proprietary MIBs dominate.** Major P25 infrastructure vendors (Motorola Solutions, Harris/L3Harris, Kenwood, Tait) implement their own SNMP MIBs for element management. There is no common P25 MIB in wide use.

3. **Element Management Systems (EMS).** Vendors typically supply their own EMS platforms that northbound into generic NOC tools (SolarWinds, HP OpenView/NNMi, etc.) via SNMP traps and polls. The "En interface" exists functionally but not as an open, standardized ASN.1 module.

4. **P25 ISSI and CSSI have more active standardization.** The network management interface received less standardization energy than voice interoperability interfaces, which became the higher priority for P25 interoperability efforts through the 2000s and 2010s.

**FCAPS management in P25 systems today:**

- **Configuration:** Managed via vendor EMS (often web-based or proprietary client)
- **Fault:** SNMP traps to NOC; some vendors implement syslog as well
- **Performance:** SNMP counters polled by NOC tools; channel loading, error rates
- **Accounting:** Typically via call logging at site/zone controller level, exported to CAD/logging systems
- **Security:** Role-based access via vendor EMS; RADIUS/LDAP integration in newer systems

---

## Key Online Resources

- **TIA standards store (GlobalSpec listing):** [TIA TSB-102.BAFA at GlobalSpec](https://standards.globalspec.com/std/1581900/TIA%20TSB-102.BAFA) — confirms document existence and provides metadata; full text behind paywall.
- **APCO International — Project 25:** [https://www.apcointl.org/technology/interoperability/project-25/](https://www.apcointl.org/technology/interoperability/project-25/) — official APCO P25 overview and program information.
- **P25 Technology Interest Group (PTIG):** [https://project25.org](https://project25.org) — PTIG publishes P25 standards, test procedures, and user needs documents.
- **CISA P25 Program:** [https://www.cisa.gov/project-25](https://www.cisa.gov/project-25) — federal government P25 compliance testing and procurement guidance.
- **RadioReference Wiki — APCO Project 25:** [https://wiki.radioreference.com/index.php/APCO_Project_25](https://wiki.radioreference.com/index.php/APCO_Project_25) — community-maintained P25 technical reference.
- **RFC 1157 (SNMP):** [https://datatracker.ietf.org/doc/html/rfc1157](https://datatracker.ietf.org/doc/html/rfc1157) — the SNMP protocol referenced by this document.
- **RFC 1158 (MIB-II):** [https://datatracker.ietf.org/doc/html/rfc1158](https://datatracker.ietf.org/doc/html/rfc1158) — the MIB-II standard referenced for Internet-based network element management.
- **Internet Archive — TIA-102 series text dump:** [https://archive.org/stream/TIA-102_Series_Documents/TIA_TSB-102-A_APCO-25_System_And_Standards_Definition_djvu.txt](https://archive.org/stream/TIA-102_Series_Documents/TIA_TSB-102-A_APCO-25_System_And_Standards_Definition_djvu.txt) — partial archive of early P25 documents.

---

## Open-Source Implementations

There are no known open-source implementations that directly implement the TSB102.BAFA NMI specification as written. The absence of a published ASN.1 MIB module means there is nothing concrete to implement.

However, these open-source P25 projects engage with P25 infrastructure management indirectly:

- **OP25** ([https://github.com/boatbod/op25](https://github.com/boatbod/op25)) — GNU Radio-based P25 receiver; includes monitoring/decoding of P25 traffic but is not an infrastructure NMS.
- **SDRTrunk** ([https://github.com/DSheirer/sdrtrunk](https://github.com/DSheirer/sdrtrunk)) — Java-based P25 decoder/monitor; passive monitoring only.
- **p25toolkit** — various community tools for P25 analysis; none implement an NMI.

No open-source project has implemented a P25-specific SNMP MIB or CMIP schema. This remains an open gap — a complete, open ASN.1 MIB module for P25 infrastructure managed objects has never been published, which is exactly what this document said needed to be done in 1994.

---

## Standards Lineage

```
APCO/NASTD/FED Project 25 Statement of Requirements (pre-1993)
└── TIA/EIA TSB-102 (1993) — P25 System and Standards Definition
    │   [Root document; establishes system model including RFG, ES, En interface]
    │
    ├── TSB102.BAFA (Dec 1994) — Network Management Interface [this document]
    │   └── TSB102.BAFA-A (Jul 1999) — NMI Overview (successor/update)
    │
    ├── TSB102.BAAA → TIA-102.BAAA-A — Common Air Interface (CAI/FDMA)
    ├── TSB102.BAEA series — Console Subsystem Interface (CSSI)
    ├── TSB102.BAEB series — Fixed Station Interface (FSI)
    ├── TSB102.BACA series — Inter-RF Subsystem Interface (ISSI)
    └── [... other TIA-102.BA* and TIA-102.BB* series documents]

External dependencies (as cited):
    ├── RFC 1157 — SNMP
    ├── RFC 1158 — MIB-II
    ├── FIPS PUB 146-1 — GOSIP
    └── ISO/IEC 9595:1990 — CMIS (Common Management Information Service)
```

---

## Notes on Document Age

This document predates the modern P25 landscape by three decades. Key contextual points:

- GOSIP was de-mandated by the U.S. OMB in a 1994 memorandum, the same year this document was published. The GOSIP/CMIP path it offers was already becoming moot.
- MIB-II (RFC 1158) was itself superseded by RFC 1213 (also called MIB-II), then by SNMPv2 and SNMPv3 frameworks through the 1990s.
- The XMP (X Open Management Protocol) mentioned as a possible bridge between Internet and GOSIP management was never widely adopted and faded as GOSIP lost traction.
- SMP (cited as the "more robust, security-focused extension of SNMP") evolved into SNMPv2 and eventually SNMPv3, which addressed the security gaps noted.

Sources:
- [TIA TSB-102.BAFA at GlobalSpec](https://standards.globalspec.com/std/1581900/TIA%20TSB-102.BAFA)
- [APCO International — Project 25](https://www.apcointl.org/technology/interoperability/project-25/)
- [APCO Project 25 — RadioReference Wiki](https://wiki.radioreference.com/index.php/APCO_Project_25)
- [RFC 1157 — SNMP](https://datatracker.ietf.org/doc/html/rfc1157)
