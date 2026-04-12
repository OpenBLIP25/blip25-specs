# TIA-102.BAJB-A Related Resources
## Project 25 Tier 1 Location Services Specification

---

## Status

**Active** — ANSI/TIA-102.BAJB-A-2014, approved November 5, 2014.

This is the current revision. It fully cancels and supersedes TIA-102.BAJB (February 2009). No further revision is known as of April 2026. The document was adopted by the APCO/NASTD/FED Project 25 Steering Committee on October 23, 2014 as part of the official P25 Standard.

The document is an ANSI-approved American National Standard, formulated under TIA TR-8 Mobile and Personal Private Radio Standards, TR-8.5 Subcommittee on Signaling and Data Transmission. Project number: ANSI/TIA-PN-102.BAJB-A.

---

## Standards Family

TIA-102.BAJB-A belongs to the **TIA-102 Location Services sub-suite** within the broader TIA-102 (Project 25) family. The location services documents are grouped under the BAJ designation:

- **TSB-102.BAJA-A** — Project 25 Location Services Overview (Feb 2010): Defines the two-tier framework. Normative context for BAJB-A.
- **TIA-102.BAJB-A** ← *This document* — Tier 1 Location Services Specification
- Additional Tier 2 location service documents are specified separately and handle infrastructure-routed, IP-addressed location services.

Within the broader TIA-102 suite, BAJB-A depends on:

- **TIA-102.BAAC-C** — P25 CAI Reserved Values (April 2011): Provides the SAP identifier for Location Service packets.
- **TIA-102.BAEA-B** — P25 Data Overview and Specification (June 2012): Defines Direct Data and Repeated Data configurations, the CAI Data Bearer Service, FSR, Um interface, and SU/FSR definitions used by BAJB-A.
- **TSB-102-B** — TIA-102 Documentation Suite Overview (June 2012): Top-level index of the full P25 standards family.

External dependency:
- **NMEA 0183, Version 3.01** (NMEA, January 2002): The application-layer protocol. Tier 1 Location Service transmits GPS position data in NMEA 0183 sentence format over P25 CAI data packets.

---

## Practical Context

Tier 1 Location Services as defined in this document are implemented in P25 portable and mobile radios with integrated GPS. The service was designed specifically for field-incident use cases: a supervisor's radio (running a mapping application as the LSHS) receives GPS location packets from patrol officers' radios (SUs with integrated GPS as LIS) over the P25 air interface, without requiring any infrastructure changes.

**Real-world equipment that implements Tier 1 Location Services:**

- Motorola APX Series portables and mobiles (APX 6000, 7000, 8000) — support P25 Phase 1 and Phase 2 data with GPS location reporting using NMEA over CAI data bearer.
- Harris (now L3Harris) XG-100P, XG-75 — P25 radios with GNSS and location reporting capability.
- Kenwood NX-5000 Series — P25 multi-protocol portables with GPS and P25 data support.
- BK Radio (Relm) portables with P25 Phase 1 data support.
- Tait Communications TM9300/TP9300 series — P25 radios supporting location services.

**Dispatch/LSHS software that consumes Tier 1 location data:**
- Motorola PremierOne CAD / WAVE PTX — can display GPS AVL from P25 radios.
- Zetron Acom — dispatch platform with P25 integration and AVL display.
- Generic AVL (Automatic Vehicle Location) mapping applications that accept NMEA 0183 data from P25 SUs via serial or Bluetooth bridge.

**Key operational constraint:** Tier 1 requires no changes to P25 infrastructure (repeaters, sites, controllers). The FSR transparently repeats the data packets. All configuration (trigger interval, destination SUID address) is programmed into the SU by the system administrator. This makes Tier 1 deployable on any P25 system that supports Direct Data or Repeated Data packet modes.

**Encryption:** Location packets can traverse encrypted channels. The transmission time budget accounts for encryption overhead (25 ms additional per packet at the 128-byte limit). The standard notes that encrypted channel reliability is worse than unencrypted due to additional processing requirements.

---

## Key Online Resources

- **TIA Standards Store (official):** https://www.tiaonline.org/standards/catalog/
  Purchase point for the official TIA-102.BAJB-A document.

- **IHS Markit / Accuristech (authorized reseller):** https://store.accuristech.com/tia
  Alternative purchase point listed in the document itself.

- **NMEA 0183 Standard:** https://www.nmea.org/content/STANDARDS/NMEA_0183_Standard
  The application-layer protocol used by Tier 1. Version 3.01 is referenced; current version may be newer.

- **APCO International P25 Technology Interest Group (PTIG):**
  https://www.apcointl.org/technology/p25/
  Hosts P25 documentation, compliance testing information, and industry liaison contacts.

- **P25 Technology Interest Group (PTIG) - Standards Documentation:**
  https://www.p25.com/
  Industry consortium site with P25 implementation guidance and conformance testing resources.

- **NIST P25 CAP (Compliance Assessment Program):**
  https://www.dhs.gov/science-and-technology/p25-cap
  DHS/SAFECOM program that tests P25 equipment compliance. Location services conformance is part of the test suite.

- **SAFECOM / DHS:**
  https://www.cisa.gov/safecom
  Federal guidance on P25 procurement including location services requirements for public safety agencies.

- **GPSd open source NMEA daemon (parses NMEA 0183 sentences):**
  https://gpsd.gitlab.io/gpsd/

---

## Open-Source Implementations

No open-source projects are known to implement the complete TIA-102.BAJB-A Tier 1 Location Service stack end-to-end. However, the following projects implement relevant components:

**P25 Protocol Libraries:**

- **OP25 (GNU Radio P25 implementation):**
  https://github.com/boatbod/op25
  Implements P25 Phase 1 and Phase 2 air interface including data channel decoding. Does not implement Tier 1 Location Service NMEA parsing but decodes the underlying data bearer packets that carry NMEA payloads.

- **p25rx / dsd-fme:**
  https://github.com/lwvmobile/dsd-fme
  P25 decoder that includes some data service decoding. Location service PDUs may be partially parsed.

- **Osmocom/OP25 related projects:**
  https://osmocom.org/projects/op25
  Community P25 tooling; data bearer support varies by project.

**NMEA 0183 Parsers (application-layer):**

- **gpsd:** https://gitlab.com/gpsd/gpsd
  The reference NMEA 0183 parser for Linux. Supports all sentence types used by Tier 1 (GGA, GLL, GSA, GSV, RMC, VTG). A complete Tier 1 LSHS could be built by piping P25 data bearer payloads into gpsd.

- **libnmea:** https://github.com/jacketizer/libnmea
  Lightweight C library for parsing NMEA sentences. Suitable for embedding in an SU firmware implementation.

- **nmeasim (Python NMEA simulator):** https://pypi.org/project/nmeasim/
  Useful for testing LSHS implementations without physical GPS hardware.

**Potential Implementation Path for Open Source Tier 1:**
A complete open-source Tier 1 implementation would require: (1) OP25 or similar for P25 air interface + data bearer decode/encode, (2) NMEA 0183 sentence construction/parsing (gpsd or libnmea), (3) SAP identifier filtering using the Location Service SAP value from TIA-102.BAAC-C, and (4) a mapping frontend (e.g., OpenLayers, Leaflet) as LSHS. No project is known to have assembled all four components.

---

## Standards Lineage

```
APCO/NASTD/FED P25 Standard (MOU 1992/1993)
└── TIA-102 Series (Project 25 Standards)
    │
    ├── TSB-102-B  ← Documentation Suite Overview
    │
    ├── TIA-102.BAAC-C  ← CAI Reserved Values (SAP identifiers)
    │
    ├── TIA-102.BAEA-B  ← P25 Data Overview & Specification
    │   (Direct Data, Repeated Data, CAI Data Bearer, Um, FSR)
    │
    └── TIA-102 Location Services Sub-Suite
        │
        ├── TSB-102.BAJA-A  ← Location Services Overview
        │   (Two-tier framework definition)
        │
        ├── TIA-102.BAJB    ← Tier 1 Location Services (Original, 2009)
        │   [SUPERSEDED]
        │
        └── TIA-102.BAJB-A  ← Tier 1 Location Services (THIS DOCUMENT)
            ANSI/TIA-102.BAJB-A-2014
            Approved: November 5, 2014
            │
            └── External dependency:
                NMEA 0183 v3.01 (NMEA, 2002)
                [Application-layer message format]

Tier 2 Location Services (separate documents, not superseded by this spec):
├── TIA-102.BAJx-x  ← Tier 2 infrastructure-routed location documents
    (IP addressing, fixed host routing, network-driven triggers)
```

**Document generation context:**
- Developed by: TIA TR-8.5 Subcommittee on Signaling and Data Transmission
- Based on materials from: APCO Project 25 Interface Committee (APIC) Data Task Group (DTG)
- Adopted by P25 Steering Committee: October 23, 2014
- ANSI approval: November 5, 2014
