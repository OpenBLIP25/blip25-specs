# TIA-102.BAEE-B Related Resources & Context
# Project 25 Radio Management Protocols
# Generated: 2026-04-13

---

## Status

**Superseded.** This document (approved May 7, 2010) was superseded by **TIA-102.BAEE-C**
(published December 2015, reaffirmed October 2015). BAEE-C is the current active revision.
The BAEE-B version remains widely referenced in existing equipment documentation and
Federal procurement specifications, since deployed P25 systems were certified against earlier
revisions.

The TIA-102 series as a whole remains active as of 2025 under the APCO Project 25
Technology Interest Group (PTIG) and TIA TR-8.5 Subcommittee on Signaling and Data
Transmission.

**Standards Proposal origin:** No. 3-4631-RV2 under TIA TR-8 Mobile and Personal Private
Radio Standards, TR-8.5 Subcommittee on Signaling and Data Transmission.

---

## Standards Family

This document is part of the **TIA-102.BAE** data services sub-series within the larger
TIA-102 Project 25 suite:

```
TIA-102 (Project 25 Digital Radio)
└── TIA-102.B (Subscriber Equipment)
    └── TIA-102.BA (Data Services)
        ├── TIA-102.BAAA-A  FDMA Common Air Interface
        ├── TIA-102.BAEA-A  P25 Data Overview
        ├── TIA-102.BAEB-A  Packet Data Specification (IP/PPP/SLIP transport)
        └── TIA-102.BAEE    Radio Management Protocols (RCP & SNMP)
            ├── TIA-102.BAEE-2000  (original)
            ├── TIA-102.BAEE-1-2002 (addendum: PPP/USB support)
            ├── TIA-102.BAEE-A (2004) — consolidated + SNMP addition
            ├── TIA-102.BAEE-B (2010) — THIS DOCUMENT; 5-year review revisions
            └── TIA-102.BAEE-C (2015) — current active version
```

**Companion documents directly referenced:**
- **TIA-102.BAAA-A** — P25 FDMA Common Air Interface; defines the CAI structure, MFID
  values, Logical Link IDs, and maximum packet sizes (2028 octets) that RCP parameters
  reference.
- **TIA-102.BAEA-A** — P25 Data Overview; defines the "A" Reference Point interface
  architecture, MDP/MRC roles, and bearer service capabilities that this standard manages.
- **TIA-102.BAEB-A** — P25 Packet Data Specification; defines the PPP/SLIP/USB transport
  layer beneath the UDP/IP that carries both RCP and SNMP.
- **TSB102-A** — APCO P25 System and Standards Definition; the umbrella systems definition.

**Normative IETF RFCs:**
- RFC 1155/1157/1212/1213 (SNMPv1 framework)
- RFC 2578/2579/2580 (SMIv2)
- RFC 3412–3418 (SNMPv3 framework operations)

---

## Practical Context

The "A" Reference Point interface defined in TIA-102.BAEA-A sits between a **Mobile Data
Peripheral (MDP)** — typically a laptop, Mobile Data Terminal (MDT), or embedded computer
— and the **Mobile Radio Controller (MRC)**, the radio subsystem. The physical connection
is a serial (RS-232) or USB link. The MDP runs IP/PPP or IP/SLIP over this link.

RCP was the original management protocol (ca. 1995) and is the simpler choice for embedded
implementations. It is a lightweight, purpose-built binary protocol with a fixed set of 7
request operators and 3 report events. It requires no third-party software stack.

SNMP was added (in revision A/2003) to support interoperability with commercial network
management systems (NMS), element managers, and off-the-shelf SNMP tools. An implementor
that already runs an IP/UDP stack and wants to use standard NMS software would choose SNMP.
The two are functionally equivalent (see Table 4-1 of the standard).

**Real-world usage:** This protocol pair appears in P25 subscriber radio systems where the
MRC (the radio) is controlled by an attached MDP (computer or intelligent peripheral). The
most common deployment scenario is a vehicle installation where a laptop MDT controls a
mobile radio. The radio powers up, sends a POWER_UP report ($50), the MDT responds with a
GET_INFO request, and normal data service begins. Encryption can be enabled/disabled and RF
power mode changed without reconfiguring the radio through its front panel.

**Key implementation detail for embedded systems:** The MTU field in the INFO_BLOCK
(maximum 2028 octets, 2-octet field) is critical. The MDP must use this value to fragment
IP datagrams to fit within a single CAI frame. Failure to honor the MRC_MTU causes
fragmentation failures at the CAI layer.

**Configuration persistence model:**
- Volatile (RAM): changed by SET_CONFG / `rmpConfigStatusMode = Modified`
- Non-volatile (NVM): loaded at power-up; written by SAVE_DFLT / `rmpConfigStatusMode = Stored`
- Restore from NVM: RST_DFLT / `rmpConfigStatusMode = Default`

**Note on DATA_DELIVERY:** The Confirmed/Unconfirmed option is marked "future support" in
configuration tables. This means the field is defined and encoded, but operational
confirmed data delivery at the CAI layer was not yet mandated as of this revision.

---

## Key Online Resources

- **APCO Project 25 Technology Interest Group (PTIG)**
  https://www.project25.org/
  Official P25 standards body. Publishes the current list of approved TIA-102 standards.

- **TIA Standards Approved List (Feb 2023)**
  https://www.project25.org/images/stories/ptig/P25_SC_23-02-001-R1_P25_TIA_Standards_Approved_16Feb2023.pdf
  Lists all currently approved P25 TIA standards including BAEE-C as the active version.

- **GlobalSpec TIA-102.BAEE listing**
  https://standards.globalspec.com/std/9977374/tia-102-baee
  Aggregates version history for the BAEE series.

- **APCO International P25 Resource Page**
  https://www.apcointl.org/technology/interoperability/project-25/
  Background on P25 interoperability and standards adoption.

- **TIA Online Standards Catalog**
  https://standards.tiaonline.org/
  Official TIA standards purchase and status page.

- **RadioReference P25 Wiki**
  https://wiki.radioreference.com/index.php/APCO_Project_25
  Community resource covering P25 system types, trunking, and protocol background.

---

## Open-Source Implementations

No known open-source project has published a complete implementation of the RCP or SNMP
MIB defined in this document. The "A" interface (MDP↔MRC over serial/USB) is a proprietary
embedded hardware link rather than an over-the-air or network protocol, making it less
accessible to SDR-based open-source projects that focus on the air interface.

The following projects implement related P25 layers but do not appear to implement RCP:

- **SDRTrunk** (Java, cross-platform)
  https://github.com/DSheirer/sdrtrunk
  Implements P25 Phase 1 & 2 trunking decode, ISSI/CSSI, talker alias, GPS. Operates
  entirely on the air interface side; does not implement the A-interface management protocols.

- **OP25** (GNU Radio, Python/C++)
  https://github.com/boatbod/op25
  P25 Phase 1 & 2 receiver. Air-interface focused; no RCP implementation.

- **p25.rs** (Rust)
  https://github.com/kchmck/p25.rs
  P25 CAI protocol implementation in Rust. CAI/air-interface layer; no RCP.

- **Trunk Recorder** (C++)
  https://github.com/robotastic/trunk-recorder
  Records calls from trunked P25 systems. No RCP.

**For implementors:** The RCP protocol is simple enough that a from-scratch Rust or C
implementation is straightforward. The complete wire format is defined in Section 3 of this
document. Key implementation notes:
  - UDP socket bound to port 469 (RCP) or standard SNMP ports
  - Single UDP datagram per SDU
  - 6-octet common header + variable data
  - SDU Tag for request/response correlation (16-bit arbitrary, $FFFF for reports)
  - No fragmentation at the RCP layer; MTU negotiated via GET_INFO

---

## Standards Lineage

```
ANSI/TIA/EIA-102.BAEE-2000  (original Radio Control Protocol)
    |
    +-- ANSI/TIA-102.BAEE-1-2002  (Addendum: PPP/USB support)
    |
ANSI/TIA-102.BAEE-A (Sep 2004)
    Consolidated + SNMP addition
    SNMP OID 17405 registered with IANA
    |
ANSI/TIA-102.BAEE-B (May 2010)   <-- THIS DOCUMENT
    Revision B: 5-year review, editorial, clarifications
    Standards Proposal 3-4631-RV2
    |
TIA-102.BAEE-C (Dec 2015)
    Current active version
```

**IANA Registration:** Private Enterprise Number 17405 assigned to "Telecommunications
Industry Association TR8dot5" — registered during Issue F (July 2003) of the standards
development process.

---

## Phase 3 Implementation Spec — Flag for Follow-Up

Based on the Phase 1 classification (PROTOCOL + MESSAGE_FORMAT), a Phase 3 implementation
spec should be produced. Recommended follow-up output:

**`TIA-102-BAEE-B_RCP_Message_Parsing_Spec.md`** — should cover:
- Complete opcode dispatch table (Type byte → class → operator byte → handler)
- Byte-level format for all 7 request SDUs, 8 response SDUs, 3 report SDUs
- SDU Tag management (correlation map, $FFFF handling)
- Parser pseudocode for RCP framing (UDP receive → SDU dispatch)
- State behavior: multithreaded request/response tracking
- Transaction timeout handling (not specified in the standard — implementation choice)
- SNMP MIB OID cross-reference table

This spec was NOT produced in this run per the processing instructions. Flag for a
follow-up pass.
