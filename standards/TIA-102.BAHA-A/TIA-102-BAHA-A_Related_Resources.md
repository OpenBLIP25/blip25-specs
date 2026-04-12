# TIA-102.BAHA-A — Related Resources

## Document Identity and Lineage

| Field | Value |
|-------|-------|
| Standard | TIA-102.BAHA-A |
| Full title | Project 25 Fixed Station Interface — Messages and Procedures |
| Date | June 29, 2017 |
| Supersedes | TIA-102.BAHA (original, ~2001-era) |
| Committee | TIA TR-8.25 (P25 standards working group) |
| ANSI accreditation | Yes (TIA is ANSI-accredited) |
| Purchase | AccurisTech (store.accuristech.com/tia) |

The original TIA-102.BAHA defined Protocol Version 1 only. The -A revision
(2017) is the major update adding Protocol Version 2 and the Data Conveyance
service. As of 2026 there is no known -B revision; BAHA-A remains the current
published version.

---

## Relationship to Other TIA-102 Documents

### Normative Dependencies (documents this standard references)

| Document | Title | Relationship |
|----------|-------|-------------|
| TIA-102.BAAA-A | Common Air Interface (CAI) | CAI voice frame formats, data block types, NID/NAC/DUID definitions |
| TIA-102.BABA | Conventional Channel Access | LDU structure, super frame, link control, low speed data, encryption sync |
| TIA-102.AABF-C | Link Control Word Formats | LCF definitions for PT=2 decoded LCW block |
| TIA-102.BAAC-B | System and Standards Overview | P25 system architecture |
| TIA-102.BACA-B | ISSI Voice and Mobility Procedures | Block types 1, 11, 15 reserved here, defined there |
| TIA-102.BACE | Console Subsystem Interface | Block types 16-30 reserved here, defined there |
| ITU-T G.711 | PCM audio coding | μ-law encoding for analog transparent voice |
| RFC 1889 / RFC 3550 | RTP | Voice Conveyance transport |
| RFC 3551 | RTP Audio/Video Profile | PCMU block format for analog transparent voice |
| RFC 768 | UDP | Control and Data Conveyance transport |
| RFC 791 | IPv4 | Network layer |
| RFC 2474 | DSCP | QoS marking (EF for voice, AF41 for control) |

### Documents That Reference This Standard

- **TIA-102.BACE** (Console Subsystem Interface): Uses the same DFSI RTP
  payload structure as BAHA-A; extends block types 16-30 for console-specific
  purposes. Console implementations must implement both BAHA-A and BACE.
- **TIA-102.BACA-B** (ISSI): Defines block types 1, 11, 15 which are reserved
  in BAHA-A's payload type space.
- DHS SAFECOM interoperability field assessments reference BAHA-A compliance
  as a requirement for P25 conventional infrastructure procurement.

---

## Implementations and Open-Source Projects

### op25 (GNU Radio P25 receiver)

- Repository: https://github.com/boatbod/op25
- Language: Python/C++ (GNU Radio blocks)
- DFSI support: op25 includes DFSI client/server capability. The DFSI
  implementation handles the RTP payload format defined in BAHA-A including
  voice block parsing (PT=0 CAI Voice frames, SoS/EoS, Receiver Reports).
- Key files: `op25/gr-op25-nachos/src/lib/` contains DFSI framing code
- Status: Active community project; the most widely used open-source P25
  implementation

### p25rx / p25craft (various)

- Various Python scripts implementing portions of the DFSI protocol exist in
  the p25 hobbyist community, primarily targeting PV1 compatibility
- MMDVM-based repeater controllers (e.g., DVMProject/dvmhost) implement FSI
  for P25 conventional fixed station connection

### DVMProject / dvmhost

- Repository: https://github.com/DVMProject/dvmhost
- Language: C++
- Implements DFSI (Digital Fixed Station Interface) for P25 conventional
  operation; aims at full PV1 compatibility
- Used in digital voice repeater deployments as an open alternative to
  commercial RFSS equipment

### RCFW (Radio Control Firmware, various vendors)

Commercial implementations (not open source) are widespread:
- **Motorola**: ASTRO 25 conventional infrastructure uses BAHA-A DFSI
- **Harris/L3Harris**: BeOn and older XL platforms
- **Kenwood**: NX-D series conventional RFSS implementations
- **AVTEC**: Scout console system with documented DFSI capability (AVTEC
  published a DFSI integration guide for connecting fixed stations)
- **Catalyst Communications**: P25 infrastructure using BAHA-A DFSI

---

## Field Deployment Context

### Conventional vs. Trunked

BAHA-A governs **conventional** (non-trunked) P25 fixed station connectivity.
In a conventional P25 system, each channel operates independently with no
trunking controller. The CFSH (host) is typically a console or site controller
that manages one or more conventional channels.

### RFSS Architecture

In a trunked P25 system, the CFSH function exists within the RFSS. The BAHA-A
interface is used at the conventional sublayer; trunked signaling is layered
above it. This means the DFSI protocol defined here is used even in trunked
deployments for the radio-to-site-controller link.

### Interoperability and DHS/SAFECOM

DHS P25 CAP (Compliance Assessment Program) tests P25 equipment for standard
conformance. The FSI is part of the assessed interface set. The DHS field
assessment methodology for conventional P25 includes testing DFSI connectivity
between CFSS and CFSH from different vendors. Published DHS field assessments
of agency P25 systems reference BAHA-A as the applicable FSI standard.

---

## Key Protocol Characteristics for Implementation

### What makes DFSI unusual compared to SIP/RTP

1. **Shared RTP payload type space**: Control messages, voice blocks, and data
   blocks all live within the same PT=100 RTP payload, differentiated by
   block PT values within the BAHA-A-defined payload header — not by separate
   RTP sessions per media type.

2. **Voice blocks are P25 CAI frames, not decoded audio**: PT=0 carries raw
   IMBE vocoder vectors (U0-U7) from the Common Air Interface, not decoded
   PCM. Decoding requires an IMBE vocoder (licensed from DVSI or equivalent).

3. **Compact framing**: Each 20 ms P25 voice frame (one IMBE frame = one CAI
   voice block = 14 bytes) is far smaller than a typical RTP audio frame.
   Multiple IMBE blocks may be combined in one RTP packet (up to 3).

4. **Protocol version negotiation at connect time**: PV1 and PV2 have different
   block type sets and different SoS/EoS semantics. The FSC_CONNECT/FSC_ACK
   exchange negotiates the version in use.

5. **Reliable NID delivery**: The SoS repeat-until-acked mechanism (PT=14
   Stream Acknowledge) is unusual for RTP — it imposes a mini-reliability
   protocol on top of UDP specifically for the NAC and squelch code.

### IMBE Vocoder Licensing

The IMBE vocoder used in P25 is patented by DVSI (Digital Voice Systems Inc.).
Commercial implementations require a license. Open-source implementations
(op25) include IMBE decode capability for receive-only interoperability
research. Implementing a transmitting DFSI endpoint that re-encodes to IMBE
requires a DVSI license agreement.

---

## Historical Notes

The original TIA-102.BAHA defined the initial DFSI for conventional P25 with
Protocol Version 1: UDP control (port 50006) + RTP voice. The core block type
set (PT=0 through PT=14 in PV1) originated in that document.

The 2017 -A revision was driven by demand for:
1. **Packet data transport**: P25 conventional data services require the CFSS
   to relay packet data frames to/from the CFSH without voice preemption issues
2. **Improved receiver/transmitter management**: Moving voter control out of
   the voice stream into the control plane (FSC_RCV_CNTRL, FSC_TX_CNTRL)
3. **CTCSS/CDCSS code delivery**: PT=8 Analog SoS to carry squelch codes
   across the FSI without relying on station local configuration
4. **Richer voice metadata**: PT=2-5 decoded blocks for LCW, encryption sync,
   LSD, and header word to enable CFSH-side logging and routing decisions
   without CFSH needing to decode raw CAI frames

---

## Searching and RAG Notes

Related search terms: DFSI, CFSI, "fixed station interface", "P25 FSI",
"FSC_CONNECT", "voice conveyance", "data conveyance", IMBEFRAME, "Block PT",
"voter control", "receiver report", "start of stream", CFSS, CFSH,
"TIA-102.BAHA", port 50006, "protocol version 2"
