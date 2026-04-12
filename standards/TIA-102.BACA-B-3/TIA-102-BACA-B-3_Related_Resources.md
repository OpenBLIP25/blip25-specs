# TIA-102.BACA-B-3 Related Resources and Context
# Project 25 ISSI Messages and Procedures — Addendum 3 — Interworking with an IWF
# Generated: April 2026

---

## Status

**Active.** This document was published April 2021 (approved February 2021, Version 1). It is an addendum to TIA-102.BACA-B (November 2012) and amends it by adding Annex J. It does not supersede the parent document; both remain in force together.

As of April 2026 no subsequent revision or superseding standard has been identified. The parent TIA-102.BACA-B remains the controlling document; a future consolidated revision of BACA-B is expected to incorporate the changes from this and other concurrent addendums (B-1 through B-3) into a single document. The addendum was published under TIA TR-8.19 (Wireline System Interfaces).

Companion addendums to the same parent:
- **TIA-102.BACA-B-1** (July 2013) — Group Emergency Cancellation procedures
- **TIA-102.BACA-B-2** (November 2016) — Error and omission corrections

---

## Standards Family

This document sits at the intersection of two standards families: the TIA-102 P25 ISSI suite and the evolving P25/3GPP MCPTT interworking work coordinated between TIA and ATIS.

### TIA-102 ISSI Document Hierarchy

```
TSB-102.BACC-B (ISSI Overview — umbrella)
│
├── TIA-102.BACA-B  Voice Services, Mobility Mgmt, RFSS Capability Polling
│   ├── TIA-102.BACA-B-1  Addendum 1: Group Emergency Cancellation (2013)
│   ├── TIA-102.BACA-B-2  Addendum 2: Error/Omission Corrections (2016)
│   └── TIA-102.BACA-B-3  Addendum 3: Interworking with an IWF (2021) ← THIS DOCUMENT
│
├── TIA-102.BACF    ISSI Packet Data (October 2019)
│   └── [Packet data mobility management; out of scope for BACA-B-3]
│
├── TIA-102.BACD    ISSI Supplementary Data Services
├── TIA-102.BACE    ISSI Packet Data Services (earlier revision)
├── TIA-102.BAAC    ISSI Reserved Values
└── TIA-102.BAAC-C  ISSI Reserved Values (revision C)
```

### Broader TIA-102 Suite Context

```
TIA-102 Project 25 Standards Suite
│
├── FDMA Air Interface (AAAA, AAAB, AAAC series)
│   └── Common Air Interface, Channel Access, Trunking
│
├── TDMA Air Interface (BBAA, BBAB, BBAC series)
│   └── Phase 2 TDMA physical/MAC layer
│
├── Wireline Interfaces (BACA, BACD, BACF series)  ← THIS FAMILY
│   ├── ISSI (Inter-RF Subsystem Interface) — G-interface
│   └── CSSI (Console Subsystem Interface) — Ec-interface
│
├── Conventional Interfaces (BAAA, BAAD series)
└── Conformance / Measurement (BCAA, CCAA series)
```

The ISSI G-interface is the IP-based protocol suite used to connect P25 RFSSs into wide-area networks and, as of this addendum, to connect P25 RFSSs to 3GPP Mission Critical systems via an IWF. The interface uses SIP (RFC 3261) for call signaling, RDP/SDP for session description, and RTP over UDP/IP for voice media. IPv4 with DSCP marking is used.

### 3GPP Interworking Standards Context

The IWF concept originates from 3GPP work:
- **3GPP TS 23.283** — Mission Critical Communication Interworking with Land Mobile Radio Systems — defines the IWF architecture and the IWF-x reference points
- **3GPP TS 29.379** — IWF signaling protocols
- **ATIS WTSC JLMRLTE** — Joint LMR-LTE (P25/3GPP) study group; produced the "Study of Interworking between P25 LMR and 3GPP Mission Critical Service" series (referenced in this document as [JLMRLTE 01R006], [JLMRLTE 04R002], [JLMRLTE 04R006], etc.)

The JLMRLTE study (ongoing through at least Phase 7, targeted December 2024) addresses vocoder transcoding, console mapping, unit roaming limitations, and multi-system topology scenarios that are explicitly deferred in the present addendum.

---

## Practical Context

### How This Document Is Used

This document is used by vendors building Interworking Functions (IWFs) that bridge 3GPP MCPTT systems to P25 ISSI networks. It tells the IWF implementer which behaviors from TIA-102.BACA-B apply unchanged and which behaviors must be adapted or excluded when the IWF acts as a peer RFSS.

Key implementation constraints defined here:
- The IWF presents itself to the TIA-102 RFSS as a standard G-interface RFSS peer
- Unit roaming (SU or UE physically moving between systems) is not supported — only group-level interworking applies
- Packet data is not supported via this interface (see TIA-102.BACF)
- Vocoder mode is negotiated per talk-spurt on the ISSI (Full Rate FDMA, Half Rate TDMA, or Native)
- The IWF must respond to SIP OPTIONS capability polling requests

### Real-World Deployment Landscape

Several commercial IWF products are deployed or in trials as of 2024–2026:

**Samsung/Etherstack LMR-IWF**: Partnership product meeting 3GPP TS 23.283 and P25 ISSI/CSSI/DFSI requirements. Deployed in AT&T FirstNet network. Supports prearranged group calls, emergency calls, and private (SU-to-SU) calls between LTE MCPTT and P25 LMR users.

**Valid8 IWF Emulator**: Demonstrated successful LTE MCX-to-P25 ISSI interworking including prearranged group calls, emergency group calls, and private calls in both directions.

**Catalyst Communications Technologies**: Multi-vendor modular IWF architecture using 3GPP MC standard messages with adapters for various LMR systems including P25.

**Motorola MC-Edge**: Intelligent gateway bridging P25, LTE, and other radio technologies.

### FirstNet Relevance

The AT&T FirstNet broadband network is the primary driver for P25/MCPTT interworking in the US public safety context. IWFs implementing this standard enable P25 agency dispatch consoles and mobile subscribers to communicate directly with LTE FirstNet devices using MCPTT — without requiring agencies to replace existing P25 infrastructure.

### DHS Science and Technology Context

DHS S&T published a report (February 2020) "Interworking Mission Critical Push-to-Talk between LTE and LMR" that describes the policy and technical motivation for this interworking work. That report preceded and informed several of the JLMRLTE study documents referenced in this addendum.

---

## Key Online Resources

- **TIA Standards Store** — Official source for purchasing TIA-102.BACA-B and its addendums:
  https://store.accuristech.com/standards/tia-tia-102-baca-b?product_id=2591499

- **Project 25 Technology Interest Group (PTIG)** — Industry body publishing P25 whitepapers and interoperability guidance:
  https://project25.org

- **PTIG Whitepaper: How to Extend Your P25 System for LTE Interworking** (October 2023) — Practical guide for agencies evaluating IWF interworking:
  https://project25.org/images/stories/ptig/PTIG_Whitepaper_-_How_to_Extend_your_P25_System_for_LTE_Interworking_October_2023_Web.pdf

- **DHS S&T Report: Interworking MCPTT between LTE and LMR** (February 2020):
  https://www.dhs.gov/sites/default/files/publications/interworking-mission-critical-push-talk-between-lte-and-lmr-report_02142020.pdf

- **Etherstack IWF Product** — Commercial IWF product description:
  https://www.etherstack.com/interworking-function/

- **Valid8 IWF Demonstration** — Description of successful MCX-to-P25 interworking demonstration:
  https://www.valid8.com/updates/valid8-performs-successful-lte-mcx-group-call-to-p25-lmr-via-iwf-emulator

- **Zetron: Comparing P25 and 3GPP MCPTT** — Technical comparison of the two standards families with interworking context:
  https://www.zetron.com/blog/comparing-project-25-and-3gpp-mcptt-capabilities/

- **GlobalSpec Standards Index** — Metadata and document history for TIA-102.BACA series:
  https://standards.globalspec.com/std/14376072/TIA-102.BACA

---

## Open-Source Implementations

No known open-source project implements the ISSI G-interface IWF interworking defined by this addendum as of April 2026. The open-source P25 ecosystem is focused on monitoring and decoding rather than full infrastructure implementation.

**SDRTrunk** (Java, Apache 2.0):
- Implements P25 Phase 1 and Phase 2 air interface decoding
- Includes ISSI-related message parsing (qualified identifiers, inter-RFSS call tracking)
- Does not implement the ISSI G-interface signaling stack or IWF procedures
- GitHub: https://github.com/Dsheirer/sdrtrunk

**OP25** (Python, GPLv2):
- P25 air interface decoder and monitor
- Group call tracking, trunking system monitoring
- No ISSI or IWF implementation
- GitHub: https://github.com/boatbod/op25

**Note for implementers**: A complete ISSI stack requires SIP/SDP/RTP over UDP/IP with P25-specific body parameters (X-TIA-P25-ISSI body type). No open-source stack implementing this is currently available. The P25 ISSI uses standard SIP (RFC 3261) transport, meaning commercial SIP stacks can be adapted, but the P25-specific SIP body parameters and call procedures require custom implementation against TIA-102.BACA-B and this addendum.

---

## Standards Lineage

```
APCO/NASTD/FED MOU (December 1993)
└── TIA TR-8 Project 25 Standards Committee
    └── TR-8.19 Wireline System Interfaces Subcommittee
        │
        ├── TIA-102.BACA (original)
        │   └── TIA-102.BACA-B (November 2012) — Major revision
        │       ├── Addendum B-1 (July 2013) — Group Emergency Cancellation
        │       ├── Addendum B-2 (November 2016) — Error/Omission Corrections
        │       └── Addendum B-3 (April 2021) — IWF Interworking ← THIS DOCUMENT
        │
        └── ATIS WTSC JLMRLTE (Joint LMR-LTE Study Group, 2019–present)
            └── Study of Interworking between P25 LMR and 3GPP MC Service
                └── Referenced informatively as [JLMRLTE 01R006], [04R002], [04R006], etc.
```

---

## Phase 3 Implementation Spec — Flagged for Follow-Up

**This document is classified PROTOCOL.** A follow-up Phase 3 pass should produce:

**`TIA-102-BACA-B-3_IWF_Protocol_Spec.md`** — State machine and procedures spec covering:
- IWF Home RFSS vs. Serving RFSS decision logic (when does IWF take each role?)
- Group call setup sequence diagrams (IWF-as-Home and IWF-as-Serving variants)
- SU-to-SU call setup/teardown sequences for the Calling-Home ↔ Called-Home segment
- Mobility management procedure flows (Serving Registration, Home Query, Deregistration)
- Group Emergency Cancellation propagation (IWF Home and Serving roles)
- Vocoder mode negotiation state machine (per-call-segment, per-talk-spurt basis)
- Confirmed call resource availability signaling sequence
- "Losing audio" handling when two TIA-102 talk spurts overlap
- SIP header exclusion table (which headers/params are excluded for IWF scenarios)
- RTP media fanout topology for IWF-as-MMF vs. IWF-as-SMF

This spec should be cross-referenced against TIA-102.BACA-B parent document sections (especially Figures 4, 5, 6, 8, 60, 61, 64, 65 and Tables 5–8, 10–12, 14–15, 25, 40, 43) which are normatively referenced but not reproduced in this addendum.
