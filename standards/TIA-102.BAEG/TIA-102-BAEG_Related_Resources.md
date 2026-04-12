# TIA-102-BAEG Related Resources and Context

**Document:** TIA-102.BAEG — Project 25 Mobile Data Peripheral Interface  
**ANSI Designation:** ANSI/TIA-102.BAEG-2013  
**Approved:** May 10, 2013  

---

## Status

**Active.** This document was published May 2013 as the first (and to date only) edition. It was adopted by the APCO/NASTD/FED Project 25 Steering Committee on April 25, 2013 as part of the Project 25 Standard. No superseding document has been identified; TIA's published catalog does not list a revision as of 2026.

This document **cancels and replaces** the A Interface (Mobile Data Peripheral Interface) material that was previously split between:
- TIA-102.BAEA-A (Data Overview and Specification, earlier revision)
- TIA-102.BAEB-A (Data Host Packet Interface, earlier revision)

It consolidates that material into a single standalone specification while those parent documents were separately revised (BAEA was updated to BAEA-B in June 2012). This document does not supersede BAEA-B or BAEB-A entirely — only the A Interface portions of those earlier revisions.

---

## Standards Family

This document belongs to the TIA-102 "BA" packet data subcategory within the broader Project 25 suite.

### Standards Lineage (ASCII Tree)

```
TIA-102 (Project 25 Standard Suite)
└── BA — Packet Data
    ├── BAEA-B  — Data Overview and Specification (2012)
    │              [defines A Interface, U_m Interface, data architecture]
    ├── BAEB-B  — Data Host Packet Interface
    │              [Uh interface: SU to host network stack]
    ├── BAEC    — Data Performance Objectives
    ├── BAED    — Packet Data Interoperability
    ├── BAEE-B  — Radio Management Protocols (2010)
    │              [radio management signaling referenced by §2.5.2.2]
    ├── BAEF    — Gateway Interface
    ├── BAEG    ← THIS DOCUMENT — Mobile Data Peripheral Interface (2013)
    │              [A Interface: SU ↔ MDP physical+data link+network protocols]
    └── (additional BA documents)

TIA-102.BAEG references externally:
    ├── TIA-232-F    — RS-232 serial interface standard
    ├── USB 2.0      — USB-IF, April 2000
    ├── USB CDC 1.2  — USB Class Definitions for Communications Devices
    ├── RFC-791      — IPv4
    ├── RFC-792      — ICMP
    ├── RFC-1055     — SLIP
    ├── RFC-1661/1662/2153 — PPP core
    ├── RFC-1334     — PPP PAP authentication
    ├── RFC-1994     — PPP CHAP authentication
    └── RFC-2131     — DHCP
```

### Companion Documents in the P25 Data Suite

| Document         | Title                                         | Relationship to BAEG                                      |
|------------------|-----------------------------------------------|-----------------------------------------------------------|
| TIA-102.BAEA-B   | Data Overview and Specification               | Normative parent; defines A Interface concept and Local Policy |
| TIA-102.BAEE-B   | Radio Management Protocols                    | Normative; defines radio mgmt protocols carried over A Interface (§2.5.2.2) |
| TIA-102.BAEB-B   | Data Host Packet Interface                    | Sibling; defines U_h interface (SU to host network, not MDP peripheral) |
| TIA-102.BAEF     | Gateway Interface                             | Sibling; defines P25 data gateway to external networks    |

---

## Practical Context

### What the A Interface Is

The A Interface is the short-distance wired connection between a P25 radio (Subscriber Unit, SU) and a Mobile Data Peripheral (MDP). In operational deployments this is typically:

- A laptop computer or ruggedized mobile data terminal (MDT) mounted in a police cruiser or fire apparatus, connected to a P25 mobile radio via a short serial or USB cable
- A tablet or handheld computer connected to a P25 portable radio
- A vehicle modem or data gateway connected to a vehicle-mounted P25 radio

The SU acts as a wireless modem: the MDP sends and receives IPv4 packets over the A Interface, and the SU carries that IP traffic over the P25 air interface (U_m) to the network. The SU can operate as either an IP host (source/destination) or an IP router (relay between A Interface and U_m).

### Why This Interface Exists

Public safety mobile data applications — computer-aided dispatch (CAD) queries, license plate checks, mapping, forms submission, messaging — require a standardized method for a data peripheral to attach to a P25 radio. Without a standard, each radio vendor would implement a proprietary interface, preventing interoperable pairing of MDPs and radios from different manufacturers.

By specifying that the A Interface uses IPv4 over SLIP or PPP over TIA-232 or USB, this document ensures that any P25-compliant SU can connect to any compliant MDP using commodity operating system networking stacks. No P25-specific protocol is required at or below the network layer on the A Interface.

### Deployment Considerations

- **SLIP** is simpler to implement (no negotiation) but lacks error detection and dynamic addressing. It is suitable for dedicated point-to-point connections with static IP configuration. Its lack of authentication makes it appropriate only in physically secured cable connections.
- **PPP** is more robust: CRC error detection, LCP negotiation of MTU and compression, and optional PAP/CHAP authentication. PPP with DHCP-based address allocation (NCP) is appropriate when IP address management is needed across multiple radio/MDP pairings.
- **USB** is now the more common physical medium in modern deployments (post-2010), replacing traditional DB-9 RS-232 connectors on laptops. The CDC (Communications Device Class) USB profile enables the OS to enumerate the radio as a network interface without vendor drivers on most platforms.
- The **Echo Reply enable/disable** requirement (§2.5.2.1.3) is notable: radios on active channels may be pinged by network management tools, and the ability to disable ICMP echo is a standard hardening option.
- **IP fragmentation** behavior is flagged: if the SU does not perform IP fragmentation and a datagram exceeds the link MTU, it shall generate an ICMP Parameter Problem message. Implementers should set MTU appropriately at the MDP.

---

## Key Online Resources

- **TIA Standards Catalog** — Official source for purchasing this document:  
  https://www.tiaonline.org/standards/catalog/

- **P25 Technology Interest Group (PTIG)** — Industry group promoting P25 interoperability; publishes implementation guidance and conformance test documentation:  
  https://www.p25tig.org/

- **APCO International P25 Resources** — Hosts P25 CAP (Compliance Assessment Program) documents and test results relevant to the data interfaces:  
  https://www.apcointl.org/technology/p25/

- **DHS SAFECOM P25 Program** — U.S. Department of Homeland Security program office for P25; publishes procurement guidance and compatibility documentation:  
  https://www.cisa.gov/safecom/p25

- **IETF RFC Editor** — Authoritative source for all IETF RFCs referenced normatively:  
  https://www.rfc-editor.org/

- **USB Implementers Forum (USB-IF)** — Source for USB 2.0 and CDC class specifications referenced normatively:  
  https://www.usb.org/documents

---

## Open-Source Implementations

Because the A Interface uses standard protocols (SLIP/PPP/USB CDC over IPv4), it does not require P25-specific software. The relevant open-source implementations are of the underlying protocols, not P25-specific:

### P25 Software-Defined Radio Projects (SU-side emulation)

- **OP25** — Open-source P25 receiver/transceiver built on GNU Radio. Implements the P25 air interface but does not implement the A Interface (treats the host as the MDP directly).  
  https://github.com/boatbod/op25

- **SDRTrunk** — Java-based P25 decoder. Decodes trunking and data channels; does not implement the MDP peripheral interface.  
  https://github.com/DSheirer/sdrtrunk

### A Interface Protocol Libraries (Standard IETF/OS Components)

The A Interface protocols are handled by standard OS networking:
- **Linux pppd** — PPP daemon implementing RFC-1661/1662/2153, PAP/CHAP, and IP address negotiation. Standard on all Linux-based MDP platforms.  
  https://github.com/paulusmack/ppp

- **Linux SLIP driver** — `slattach` utility and kernel SLIP/CSLIP driver (net/slip/). Standard in Linux kernel.

- **Linux USB CDC-NCM/ECM drivers** — USB network class drivers enabling the SU to present as a network interface to the Linux MDP without vendor drivers. Present in mainline Linux kernel.

No public open-source project was identified that specifically implements the P25 A Interface as defined in this document (i.e., a software SU that exposes a compliant A Interface to an attached MDP). Commercial implementations exist in Motorola, Harris/L3Harris, Kenwood, and Tait P25 radio product lines.

---

## Standards Lineage Notes

This document's material history:
1. **TIA-102.BAEA** (original) — included A Interface material as part of the broader data overview
2. **TIA-102.BAEA-A** — revised data overview; still included A Interface content
3. **TIA-102.BAEB-A** — data host interface; also included A Interface content
4. **TIA-102.BAEG** (2013) — extracted A Interface into standalone document; cancelled A Interface sections of BAEA-A and BAEB-A
5. **TIA-102.BAEA-B** (2012) — concurrent revision of data overview; BAEG references BAEA-B normatively

The A Interface specification has been stable since consolidation. The underlying technologies (USB 2.0, PPP, SLIP) are mature IETF/industry standards with no pending changes relevant to the A Interface.
