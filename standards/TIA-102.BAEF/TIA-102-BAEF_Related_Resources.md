# TIA-102-BAEF Related Resources and Context
# Project 25 — Packet Data Host Network Interface
# Generated: 2026-04-13

---

## Status

**Active.** ANSI/TIA-102.BAEF-2013 was approved November 26, 2013 and adopted by
the APCO/NASTD/FED Project 25 Steering Committee on November 1, 2013. No
subsequent revision has been identified as of 2026. This document is the first
and only edition.

This document supersedes the E_d Interface-specific content previously embedded
in TIA-102.BAEB-A. It does not supersede BAEB-A in its entirety — it only
extracts and replaces the Packet Data Host Network Interface (E_d) material.
BAEB-A continued to exist as a separate document covering other packet data
interface content.

No patents were identified by TIA in connection with this document at publication.

---

## Standards Family

This document is part of the **TIA-102 BAE-series** (Packet Data subsystem),
within the broader TIA-102 Project 25 standard suite.

### E_d Interface in the P25 Data Architecture

P25 defines several standardized interfaces between network elements. The E_d
Interface is the point where the Fixed Network Equipment (FNE) — the P25
infrastructure — connects to the Data Host Network (DHN), which is the operator's
external IP network hosting data applications. This document specifies only the
physical/link/network layer stack at that interface.

P25 Data interface designations (from TIA-102.BAEA-B):
- **E_d** — FNE to Data Host Network (this document)
- **E_u** — RF Subsystem to FNE (subscriber-side data)
- **E_b** — FNE to Billing System
- **E_c** — FNE to Network Management
- **E_h** — FNE to Home Location Register / Authentication Center

### Key Related Documents

**Normative parent:**
- **TIA-102.BAEA-B** (Project 25 Data Overview and Specification, June 2012) —
  defines the overall P25 Packet Data architecture, including the E_d Interface
  concept, DHN, FNE, and Local Policy terms. This document is the required
  prerequisite reading for TIA-102.BAEF.

**Predecessor (partially superseded):**
- **TIA-102.BAEB-A** (Project 25 Packet Data, 2006 era) — the original combined
  packet data document from which E_d content was extracted into BAEF.

**BAE-series sibling documents (packet data interfaces):**
- TIA-102.BAEC — Packet Data Air Interface (E_u, subscriber-side)
- TIA-102.BAED — Packet Data Network Interface (other FNE interfaces)
- TIA-102.BAEE — Packet Data Authentication/Security

**Referenced external standards (normative):**
- IEEE 802.2-1998 — Logical Link Control (LLC)
- IEEE 802.3-2000 — CSMA/CD Ethernet Physical and MAC Layer
- RFC 791 — Internet Protocol (IPv4)
- RFC 894 — IP Datagrams over Ethernet Networks
- RFC 1042 — IP Datagrams over IEEE 802 Networks

**Referenced external standards (informative):**
- RFC 792 — Internet Control Message Protocol (ICMP)
- RFC 826 — Ethernet Address Resolution Protocol (ARP)

---

## Standards Lineage

```
TIA-102 (Project 25 Standards Suite)
└── TIA-102.B (Infrastructure/Network)
    └── TIA-102.BA (Packet Data Subsystem)
        └── TIA-102.BAE (Packet Data Interfaces)
            ├── TIA-102.BAEA-B  ← Data Overview & Specification (parent)
            ├── TIA-102.BAEB-A  ← Combined predecessor (partially superseded)
            ├── TIA-102.BAEC    ← Air Interface (Eu)
            ├── TIA-102.BAED    ← Network Interfaces
            ├── TIA-102.BAEE    ← Authentication/Security
            └── TIA-102.BAEF    ← THIS DOCUMENT: Host Network Interface (Ed)
                                   (cancels Ed content from BAEB-A)
```

---

## Practical Context

### Role in P25 Systems

In a deployed P25 system, the E_d Interface is the Ethernet attachment point
between the P25 data gateway or zone controller (FNE side) and the agency's IP
LAN/WAN (DHN side). Examples of DHN-hosted data applications include:

- Computer-Aided Dispatch (CAD) systems
- Automatic Vehicle Location (AVL) systems
- License plate reader databases
- Messaging/text applications for P25 subscribers
- Telemetry and sensor data collection systems

Because this document specifies standard IPv4 over Ethernet or IEEE 802.3, the
E_d Interface can be implemented using any commodity Ethernet hardware and
standard IP networking — no P25-specific hardware or drivers are required at the
physical/network level. The P25-specific protocol complexity is located elsewhere
in the stack (the air interface, SNDCP layer, FNE data gateway logic).

### Implementation Implications

This document is intentionally minimal. Its normative effect is:
1. The FNE must present an Ethernet or IEEE 802.3 interface to the DHN.
2. IP must be version 4 (IPv4), using RFC 894 or RFC 1042 encapsulation.
3. IP address assignment is left to local policy (ARP is informative).
4. The FNE may use ICMP to return error signals to IP sources (informative).
5. TCP and UDP are intended transport protocols above IPv4 (informative).

Any IPv6, VLAN tagging, QoS marking, IPsec, or other network-layer behavior is
outside the scope of this document and governed by local agency policy.

### Vendor and Agency Use

Commercial P25 vendors (Motorola Solutions, L3Harris, Kenwood, etc.) implement
the E_d Interface as a standard Ethernet port on their FNE/zone controller
hardware. The interface is configured by the agency network administrator using
standard IP networking practices. Procurement specifications for P25 data systems
will reference TIA-102.BAEF (or its predecessor BAEB-A) when specifying data host
network connectivity requirements.

The minimalism of this document is a deliberate design choice: by deferring all
address binding, transport selection, and application-layer behavior to local
policy, the standard avoids mandating a specific network topology or data
application architecture, allowing agencies maximum flexibility in integrating
P25 data into their existing network infrastructure.

---

## Key Online Resources

- **TIA Standards Catalog** — Official source for purchasing the standard:
  https://www.tiaonline.org/standards/

- **Accuristech/IHS Markit (formerly TechStreet)** — Distributor for TIA standards:
  https://store.accuristech.com/tia/

- **PSWN / SAFECOM P25 Resources** — US DHS resources on P25 procurement and
  standards compliance:
  https://www.cisa.gov/safecom/p25

- **P25 CAP (Compliance Assessment Program)** — NIST/DHS program for P25
  interoperability testing (data capability testing references BAE-series specs):
  https://www.dhs.gov/science-and-technology/p25-cap

- **TIA TR-8.5 Subcommittee** — The TIA committee responsible for this document
  (Signaling and Data Transmission):
  https://www.tiaonline.org/

- **RFC 791 (IPv4)**: https://www.rfc-editor.org/rfc/rfc791
- **RFC 894 (IP over Ethernet)**: https://www.rfc-editor.org/rfc/rfc894
- **RFC 1042 (IP over IEEE 802)**: https://www.rfc-editor.org/rfc/rfc1042
- **RFC 792 (ICMP)**: https://www.rfc-editor.org/rfc/rfc792
- **RFC 826 (ARP)**: https://www.rfc-editor.org/rfc/rfc826

---

## Open-Source Implementations

No public open-source projects specifically targeting the E_d Interface
(TIA-102.BAEF) were identified. This is expected given the document's
narrow and minimal scope — it specifies standard IPv4-over-Ethernet, which
is implemented by every network stack and requires no P25-specific code.

The P25-specific logic that sits above or below the E_d Interface is found in
some open-source tools, but none implement a full FNE data gateway:

- **op25** (https://github.com/boatbod/op25) — P25 SDR receiver; focuses on
  air interface decoding. No E_d or DHN functionality.
- **SDRTrunk** (https://github.com/DSheirer/sdrtrunk) — P25 decoder/scanner.
  Air interface focus; no data gateway implementation.
- **dvmhost** (https://github.com/DVMProject/dvmhost) — Mixed-mode P25/DMR
  repeater. No packet data subsystem support identified.

The gap in open-source P25 data gateway implementations reflects the general
state of the ecosystem: the voice and control channel are well-represented in
open-source software, but the packet data subsystem (SNDCP, E_d interface,
FNE data gateway) has no known public implementation. This is a significant
opportunity for open-source P25 development.

---

## Notes on Document Scope Boundaries

This document explicitly does not cover:
- The P25 air interface (covered in TIA-102.BAEA-B and TIA-102.BAAC-series)
- SNDCP (Subnetwork Dependent Convergence Protocol) — the P25 data adaptation
  layer between the air interface and IP (covered in TIA-102.BAEA-B)
- The FNE internal architecture or data routing logic
- Security (encryption/authentication) at the E_d Interface
- IPv6 support
- Any data application protocol formats

For implementers building a P25 FNE data gateway: the E_d Interface is simply
a standard Ethernet/IPv4 port. The complexity is in the P25 protocol layers
that translate between the P25 air interface data and the IPv4 datagrams
delivered across the E_d Interface to the DHN.
