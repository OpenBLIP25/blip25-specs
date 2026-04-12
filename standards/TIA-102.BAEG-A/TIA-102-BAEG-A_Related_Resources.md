# TIA-102-BAEG-A Related Resources & Context
## Project 25 Mobile Data Peripheral Interface

---

## Status

**Active.** This document is ANSI/TIA-102.BAEG-A-2019, approved March 8, 2019. It supersedes TIA-102.BAEG (the original 2013 edition). It was adopted by the Project 25 Steering Committee as part of the P25 Standard following their October 18, 2018 vote.

No successor or replacement has been issued as of early 2026. This document is considered current in the P25 Standard suite.

---

## Standards Family

This document is part of the **TIA-102 Series B — Packet Data** standards group, specifically the **BAE** cluster covering P25 data interfaces:

```
TIA-102 (Project 25 Standard)
└── Series B: Radio Subsystem
    └── BA: Packet Data
        ├── BAEA-C  Data Overview and Specification (architecture, defines A/Um/Host interfaces)
        ├── BAEB     [absorbed into BAEA-C; original conventional IP data procedures]
        ├── BAEC     Conventional Data Services
        ├── BAED     Trunked Packet Data Services
        ├── BAEE-C  Radio Management Protocols (payload carried over A Interface)
        ├── BAEF-A  Packet Data Host Network Interface (B Interface / host side)
        ├── BAEG-A  Mobile Data Peripheral Interface (A Interface) ← THIS DOCUMENT
        └── BAEA-B  [earlier edition, superseded]
```

**Key companion documents:**

| Document | Title | Relationship |
|----------|-------|--------------|
| TIA-102.BAEA-C | P25 Data Overview and Specification | Normative parent; defines "A Interface" term, overall architecture |
| TIA-102.BAEE-C | P25 Radio Management Protocols | Defines radio management payload carried over the A Interface per §2.5.2.2 |
| TIA-102.BAEF-A | P25 Packet Data Host Network Interface | Complementary: defines the network/host-facing "B Interface" of the SU |
| TIA-232-F | Serial interface standard | Physical layer for TIA-232 combinations |
| USB Rev 3.1 | USB specification | Physical layer for USB combinations |
| RFC 1055 | SLIP | Point-to-point framing for SLIP combinations |
| RFC 1661/1662/2153 | PPP | Point-to-point framing for PPP combinations |
| RFC 2131 + 1533 | DHCP | Dynamic IP address binding for RNDIS/USB combination |
| RNDIS v5.0 | Microsoft RNDIS specification | Point-to-point layer for RNDIS/USB combination |

---

## Practical Context

### What Is the A Interface in Real Deployments?

In a public safety vehicle, a P25 subscriber radio (SU) connects to a mobile data computer (MDC) — typically a ruggedized laptop or tablet mounted in the vehicle — via the A Interface. The MDC runs Computer-Aided Dispatch (CAD) clients, license plate query tools (e.g., NCIC), mapping software, and digital forms. The A Interface is what turns the P25 radio into a mobile broadband gateway for these applications.

### Physical Reality

In practice the A Interface connection is almost always either:
1. **TIA-232 serial cable** — The traditional connection, present in virtually all legacy P25 radios. TIA-232 support over 9-pin or manufacturer-specific accessory connectors has been standard since the original P25 design. Typical throughput is 9600 baud to 115.2 kbps — adequate for legacy CAD but too slow for image-heavy applications.
2. **USB** — Increasingly common on modern P25 radios. The radio appears as a USB CDC-NCM or RNDIS device to the MDC's operating system, enabling much higher throughput. Motorola, Harris (L3Harris), Kenwood, and Tait all ship radios with USB data interfaces on their later-generation P25 equipment.
3. **Ethernet** — Present on fixed/base station deployments or higher-end vehicle-mounted radio equipment. Adding Ethernet in Revision A reflects the trend toward IP-native P25 infrastructure.

### Role in P25 Data Architecture

The A Interface sits on the "user side" of the SU. On the "network side," the SU transmits P25 packet data over the air interface (U_m) using the protocols defined in the BA series. The SU thus bridges the MDC's IPv4 traffic onto the P25 air interface, functioning as both a router (for forwarding MDC traffic) and a host (for radio management traffic to/from itself).

### RNDIS vs CDC-ECM on USB

The addition of RNDIS in Revision A reflects Microsoft Windows driver ecosystem reality. RNDIS is Microsoft's proprietary protocol for networking over USB, natively supported by Windows without additional drivers. Linux and macOS support RNDIS via the `rndis_host` kernel driver. CDC-ECM (USB Communications Device Class - Ethernet Control Model) is the standards-based alternative but historically had less consistent Windows support. By standardizing RNDIS, this document simplifies deployment in law enforcement environments that are overwhelmingly Windows-based.

### DHCP Restriction on RNDIS/USB

The document mandates that for RNDIS/USB, the SU acts as DHCP server and must support "dynamic allocation" only — not "automatic" (which would remember the same address permanently) or "manual" (admin-assigned). This avoids address conflicts in scenarios where multiple radios are used with different MDCs over time.

---

## Key Online Resources

- **TIA Standards Catalog** (official source for purchasing):  
  https://www.tiaonline.org/standards/catalog/

- **APCO P25 Technology Interest Group** — industry forum covering P25 implementation:  
  https://www.apcointl.org/technology/p25/

- **PTIG (P25 Technology Interest Group) Compliance Assessment Program (CAP)**:  
  https://www.dhs.gov/safecom/p25  
  The DHS SAFECOM program tests P25 interoperability, including data interface compliance.

- **CISA SAFECOM P25 CAP Data Sheets** — published compliance results for radio equipment including data interface testing:  
  https://www.dhs.gov/safecom/p25-compliance-assessment-program

- **RFC 1055 (SLIP)**: https://www.rfc-editor.org/rfc/rfc1055  
- **RFC 1661 (PPP)**: https://www.rfc-editor.org/rfc/rfc1661  
- **RFC 2131 (DHCP)**: https://www.rfc-editor.org/rfc/rfc2131  
- **Microsoft RNDIS specification**: https://docs.microsoft.com/en-us/windows-hardware/drivers/network/remote-ndis--rndis-2

---

## Open-Source Implementations

The A Interface involves only well-established standard protocols (SLIP, PPP, RNDIS, Ethernet, IP, DHCP) on the host/MDP side. The P25-specific aspect is only the radio management payload content defined in TIA-102.BAEE-C. Consequently, implementation of the MDP side of this interface does not require P25-specific open-source code; it uses standard OS networking stacks.

**Relevant open-source projects touching this interface:**

| Project | URL | Relevance |
|---------|-----|-----------|
| **OP25** | https://github.com/boatbod/op25 | GNU Radio-based P25 software radio. Primarily a receiver; some data channel decode capability. MDP-side interface not applicable to a pure SDR implementation. |
| **SDRTrunk** | https://github.com/DSheirer/sdrtrunk | Java-based P25 decoder. Handles data channel decoding (SNDCP/SNF layers). Does not implement the A Interface directly (acts as a passive decoder, not an SU). |
| **GNU Radio P25 blocks** | https://github.com/osmocom/op25 | Osmocom fork of OP25; similar scope. |
| **Linux rndis_host driver** | https://github.com/torvalds/linux/blob/master/drivers/net/usb/rndis_host.c | Host-side RNDIS driver in the Linux kernel. This is the MDP-side implementation for the RNDIS/USB combination on Linux. |
| **Linux cdc_eem / cdc_ncm** | Linux kernel USB CDC drivers | Alternative USB networking drivers; the SU may expose these classes per §2.1.2. |

**Note:** No open-source projects are known to implement a complete software-defined P25 SU with a standards-compliant A Interface. Real-world A Interface implementations are embedded in commercial P25 radios (Motorola APX series, L3Harris Unity XG series, Kenwood NX-5000 series, Tait TP9000 series, etc.).

---

## Standards Lineage

```
P25 Standard (Project 25 Steering Committee / TIA TR-8)
│
├── TIA-102 Series (overall P25 standard)
│   │
│   └── Series B (Radio Subsystem)
│       │
│       └── BA (Packet Data)
│           │
│           ├── BAEA-C (Data Overview & Specification)  ← defines A Interface concept
│           │
│           ├── BAEG (original, 2013)
│           │   └── derived from BAEA-A + BAEB-A material
│           │
│           └── BAEG-A (this document, 2019)
│               └── adds RNDIS/USB and Ethernet combinations
│
└── Referenced external standards:
    ├── TIA-232-F (serial physical layer)
    ├── USB Rev 3.1 (USB physical layer)
    ├── RFC 1055 (SLIP)
    ├── RFC 1661/1662/2153 (PPP)
    ├── RFC 791/792 (IPv4/ICMP)
    ├── RFC 2131 + 1533 (DHCP)
    └── RNDIS v5.0 (Microsoft)
```

---

## Phase 3 Implementation Spec Notes

**Classification: PROTOCOL + OVERVIEW**

This document does not require a Phase 3 implementation spec in the traditional sense. It defines no novel algorithms, no P25-specific message formats, and no complex state machines. All protocol behavior is inherited from the referenced IETF RFCs and external specifications.

**However, the following implementation notes are useful for a P25 SU software stack:**

1. **RNDIS/USB DHCP server**: The SU must implement a minimal DHCP server that: (a) issues a single dynamic lease, (b) includes Subnet Mask, Router Option, DHCP Message Type, Server Identifier, and Parameter Request List options, (c) ignores htype/hlen/sname/file fields, and (d) sets hops and siaddr to zero. This is simple enough to implement inline; no Phase 3 spec is flagged.

2. **ICMP Echo Reply enable/disable**: The SU's ICMP Echo Reply response must be configurable (on/off). This is a simple config flag affecting the IP stack.

3. **Radio Management IP header**: ToS must be forced to 0x00 for all radio management datagrams; the Protocol field must match the transport specified in TIA-102.BAEE-C. Implementation detail is in BAEE-C, not here.

**Flagged for follow-up:** A Phase 3 spec for the radio management payload (TIA-102.BAEE-C) would be the natural companion implementation spec for this document.
