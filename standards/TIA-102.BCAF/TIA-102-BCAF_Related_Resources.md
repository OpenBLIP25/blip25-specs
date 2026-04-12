# TIA-102.BCAF — Related Resources and Context

**Document**: TIA-102.BCAF — Trunked TDMA Voice Channel Conformance Profiles
**Published**: August 2012 (Issue O, balloted 7/18/12)
**Issuing Body**: TIA TR-8.12 Subcommittee on Two-Slot TDMA (sponsored by APIC TDMA Task Group)

---

## Document Lineage and Position in the P25 Standard Family

TIA-102.BCAF sits at the intersection of the TDMA (Phase 2) and conformance testing branches of the P25 standard family. It is one of several BCAF-adjacent conformance documents:

```
P25 TDMA (Phase 2) Standards Tree:

Physical Layer:
  TIA-102.BBAB — Two-Slot TDMA Physical Layer Protocol Specification (2009)

MAC Layer:
  TIA-102.BBAC — Project 25 Phase 2 Two-Slot TDMA MAC Layer Description (2010)

Trunking Control:
  TIA-102.AABC-C — Trunking Control Channel Messages (2009)
  TIA-102.AABD-A — Trunking Procedures (2008)

Conformance (TDMA-specific):
  TIA-102.BCAD — Phase 2 TDMA Trunked Voice CAI Conformance Specification [under construction at publication]
  TIA-102.BCAE — Phase 2 TDMA Trunked Voice Services Message and Procedures Conformance Specification
  TIA-102.BCAF — Trunked TDMA Voice Channel Conformance Profiles [THIS DOCUMENT, 2012]

Interoperability:
  TIA-102.CABC-B — Interoperability Testing for Voice Operation in Trunked Systems (2010)

Conformance (P25 general / Phase 1):
  TIA-102.BAAB-B — Project 25 Common Air Interface Conformance Test (2005)
  TIA-102.CAEC — Conformance Profile Basic Trunked Operation [under construction at publication]
  TIA-102.CAED — Conformance Profile Level 4 Advanced Trunked Operation [under construction at publication]
```

The "AF" suffix in BCAF indicates it is in the "Conformance" (CF) group within the BC (Phase 2 TDMA) document cluster.

---

## Relationship to Key Normative Standards

### TIA-102.BBAC (MAC Layer Description)
The foundational TDMA MAC layer specification. BCAF references BBAC for:
- DUID field definitions and burst type encoding
- ESS (Encryption Synchronization Signaling) structure — ESS-A and ESS-B
- MAC PDU formats for FACCH and SACCH
- Voice superframe definition (4Va/4Vb/4Vc/4Vd/2V)
- Offset field semantics in MAC_PTT PDUs
- MAC message types: MAC_ACTIVE, MAC_PTT, MAC_END_PTT, MAC_IDLE, MAC_HANGTIME, MAC_RELEASE

### TIA-102.AABD-A (Trunking Procedures)
Defines the behavioral trunking procedures that the BCAF CPs are designed to test conformance against. BCAF translates AABD behavioral requirements into specific test sequences with exact bit-level signal parameters.

### TIA-102.AABC-C (Trunking Control Channel Messages)
Defines the control channel message formats (TSBKs) used in the BCAF procedures:
- GRP_V_REQ, GRP_V_CH_GRANT, ACK_RSP_FNE
- UU_V_REQ, UU_V_CH_GRANT, UU_V_CH_GRANT_EXP
- CAN_SRV_REQ
- RFSS_STS_BCST (with R-bit for message/transmission trunking indication)

### TIA-102.BCAE (Message and Procedures Conformance Specification)
BCAE provides the definitions for roles (Originating SU, Requesting SU) that BCAF imports. BCAF Section 2.1 explicitly copies definitions from BCAE.

---

## Project 25 Phase 2 TDMA Context

### What is P25 Phase 2?
Project 25 Phase 2 (also called TDMA Phase 2) extends the original P25 FDMA (Phase 1) system to use Time Division Multiple Access on the traffic channel. While Phase 1 uses a single-slot 12.5 kHz FDMA channel, Phase 2 uses two timeslots on a single 12.5 kHz channel (two-slot TDMA), effectively doubling spectral efficiency for voice traffic.

Key characteristics:
- Same 12.5 kHz channel bandwidth as Phase 1
- TDMA with 2 timeslots per channel (Um2 interface)
- Modulation: H-CPM (Harmonically Companded Phase Modulation) or H-DQPSK on voice channels
- Control channel: Still uses FDMA/C4FM or CQPSK (backward-compatible with Phase 1 control channel)
- Two-slot voice traffic enables 2x channel reuse efficiency
- Same P25 control channel architecture (TSBK, RFSS, WACN, System ID, Site ID, NAC)

### Standards Development History
- Phase 2 TDMA was developed by the APIC TDMA Task Group under TIA TR-8.12 Subcommittee
- Initial Phase 2 specs (BBAB, BBAC) published 2009–2010
- TIA-102.BCAF published August 2012 as Issue O (first publication)
- As of 2012 publication, companion documents BCAD and CAEC/CAED were still under construction

---

## Conformance Testing Context

### Purpose of Conformance Profiles (CPs)
CPs are structured test scripts for verifying that equipment produces the correct CAI (Common Air Interface) signal sequences. They define:
1. Equipment Under Test (EUT) role
2. Pre-conditions (state before test)
3. Procedure (required signal sequence with exact PDU parameters)
4. Post-conditions (state after test)
5. Signaling Parameters (bit-level field values)

CPs are not interoperability tests. They verify that each individual component (SU or RFSS Site) produces conformant signals in isolation — the test environment emulates the other party.

### Relation to APCO P25 Compliance Assessment Program (CAP)
APCO administers the Compliance Assessment Program (CAP) for P25 equipment. CAP testing includes:
- Laboratory testing of P25 CAI conformance (for Phase 1, based on BAAB conformance tests)
- Interoperability testing (CABC)
- For Phase 2 TDMA: conformance testing based on BCAD/BCAE/BCAF documents

At the time of BCAF publication (2012), the full Phase 2 CAP testing framework was still being developed (BCAD was under construction).

### Test Roles Defined
- **Originating SU**: Initiates a voice service (primary talker role)
- **Destination SU**: Receives a voice service (listener role)
- **Interrupting SU**: Preempts an ongoing call (preemption scenarios)
- **RFSS Site**: Infrastructure reference point for the CAI

---

## Deployment Status and Industry Adoption

### P25 Phase 2 Deployments (as of 2024–2025)
P25 Phase 2 TDMA systems have been deployed in major U.S. public safety networks:
- **Harris Corporation** (now L3Harris): BeOn, XG-75, and XL-185 radios support Phase 2
- **Motorola Solutions**: APX series (APX 6000, APX 8000, APX NEXT) support Phase 2 TDMA
- **Kenwood**: NX-5000 series supports Phase 2
- Major U.S. deployments: Harris County TX, New York City (NYPD/FDNY networks), Los Angeles County, various state-wide systems

### Transition Trajectory
P25 Phase 2 adoption has been gradual due to:
- Significant installed base of Phase 1 infrastructure
- Phase 2 control channels remain backward-compatible (Phase 1 SUs can operate on Phase 2 systems with reduced capacity)
- Interoperability requirements between agencies using different generations
- BCAF conformance testing ensures equipment from different manufacturers interoperates correctly on Phase 2 traffic channels

---

## Related Open Source Projects

### GNU Radio / gr-p25
- **gr-p25**: GNU Radio blocks for P25 decoding; supports Phase 1 well; Phase 2 TDMA support exists in forks (notably **gr-op25-arm** / **OP25**)
- **OP25**: Active open-source P25 receiver project; supports both Phase 1 FDMA and Phase 2 TDMA decoding
- OP25 decodes TDMA traffic channels including MAC PDUs documented in BCAF (MAC_PTT, MAC_END_PTT, MAC_ACTIVE, etc.)

### JMBE (Java Multi-Band Excitation)
- Open source IMBE/AMBE vocoder library used in conjunction with P25 decoders; relevant for the voice bursts described in BCAF superframe sequences

### Trunk Recorder
- Open source P25 trunked radio recording software; uses BCAF-documented call lifecycle concepts (call start, transmission, termination, continuation) to track and record trunked calls

### SDRTrunk
- GUI-based P25 trunked radio decoder for Software Defined Radio; implements Phase 2 TDMA decoding including the MAC layer messages documented in BCAF

---

## Standards Adjacent to BCAF

### TIA-102.BAAD-A — Conventional Procedures (Feb 2010)
Covers conventional (non-trunked) P25 procedures. Some MAC layer structures are shared.

### TIA-102.CABC-B — Interoperability Testing (Oct 2010)
The interoperability test suite that BCAF is designed to supplement. Where BCAF tests individual equipment conformance in isolation, CABC tests multi-vendor interoperability.

### APCO ANS 1.102.1 / P25 System Statement
APCO's system-level requirements for P25 networks, including Phase 2 trunking behavior. Behavioral requirements that BCAF tests conformance against ultimately trace back to these system-level requirements.

---

## Procurement and Access

TIA standards are copyrighted documents. TIA-102.BCAF is available for purchase from:
- **TIA Standards Store**: store.accuristech.com/tia
- **IHS Markit** (now S&P Global): ihsmarkit.com
- Typical pricing: $100–$300 USD for individual standards

The document contains "Limited Use Only" watermarks on each page, indicating distribution is restricted to TIA member licensees.

---

## Key Distinguishing Features of This Document

1. **Bit-exact signaling parameters**: Unlike behavioral specs, BCAF provides exact field values (DUID bits, opcode bits, offset values, MCO codes) for each message in each procedure, making it directly usable for protocol implementation verification.

2. **MAC_RELEASE C/A bit semantics**: BCAF explicitly defines the two MAC_RELEASE modes — C/A=%0 (leave voice channel, used in polite/impolite preemption) vs C/A=%1 (stay on voice channel, used in forced dekey). This distinction is critical for correct preemption behavior.

3. **Dual-stream PSTN procedures**: CPs 25–28 cover the PSTN to Group scenario where an SU can talk while the RFSS simultaneously maintains a PSTN audio stream outbound — a technically complex dual-stream MAC state machine unique to P25 interconnect.

4. **SACCH timing alignment rules**: The FxA alternate flows capture the inherent timing ambiguity when a SACCH slot falls between consecutive FACCH flows — a subtlety critical to correct MAC layer implementation.

5. **Voice Channel grant via MAC message (CP 26 Alt B)**: The voice-channel-based grant using Group Voice Service Req / Group Voice Channel Grant MAC messages embedded in MAC_ACTIVE PDUs (rather than TSBKs on the control channel) is unique to PSTN-to-Group continuation scenarios and documented in detail only in BCAF.
