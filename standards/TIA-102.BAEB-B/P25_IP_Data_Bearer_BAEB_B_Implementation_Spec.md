# P25 IP Data Bearer — TIA-102.BAEB-B Implementation Spec

**Source:** TIA-102.BAEB-B (2014-09), *Packet Data Specifications for
Project 25 Data Bearer Services*. Supersedes TIA-102.BAEB-A.
Superseded in turn by TIA-102.BAEB-C (2019-12) for the SNDCP-V3
detail; BAEB-B remains the authoritative reference for **SCEP**,
which was not carried into BAEB-C.

**Companion to:**
`standards/TIA-102.BAEB-C/P25_IP_Data_Bearer_Implementation_Spec.md`
(SNDCP-V3-centric consolidated spec). This BAEB-B derivation focuses
on the content BAEB-C doesn't cover — SCEP, SNDCP V1/V2 forms, and
the state machines — and provides the authoritative PDU figures
(Figures 16–28) that parsers can code against at byte granularity.

**Scope of this derived work:**
- §1 — What BAEB-B adds beyond BAEB-C (SCEP + state machines)
- §2 — SCEP: semantics, ARP, datagram conveyance, zero-wrapper
  invariant
- §3 — SNDCP version lineage (V1 → V2 → V3) and WAI version space
- §4 — TDS / CDS state machines, timers, and the registration →
  context-activation → Ready-state path
- §5 — SNDCP PDU types and field semantics (Tables 1, 23, 25–43)
- §6 — Context-management PDU byte layouts (Figures 16–25)
- §7 — Data payload PDU byte layouts (Figures 26–28)
- §8 — Modifiable Context Accept fields (Annex A)
- §9 — Reconciliation with the observed Motorola trunked traffic
- §10 — Cross-reference map

**Pipeline artifacts:**
- `standards/TIA-102.BAEB-B/TIA-102-BAEB-B_Full_Text.md` — clean-room
  extraction (copyrighted, git-ignored).
- `annex_tables/sndcp_reject_codes.csv` — reject code tables (V1/V2
  and V3).
- `annex_tables/sndcp_field_definitions.csv` — field encoding
  (PDU type, NSAPI, NAT, DSUT, MTU, IPHC/HCOMP, SNDCP version).

---

## 1. What BAEB-B Has That BAEB-C Doesn't

BAEB-C (2019) is organized around SNDCP-V3 in the trunked FNE-data
configuration. It does **not** reproduce the SCEP chapter or the
four-configuration taxonomy. As a result:

| Content | BAEB-B (2014) | BAEB-C (2019) |
|---|---|---|
| SCEP (Simple Convergence Encapsulation Protocol) | **Defined in §4** | Not reproduced |
| SCEP ARP wire format | **Defined in §4.2** | Not reproduced |
| Four-configuration taxonomy (Direct / Repeated / Conv-FNE / Trunked-FNE) | **§1** | Trunked-only |
| SNDCP V1 / V2 PDU formats | **Figures 16–20** | V3-only |
| TDS and CDS state machines | **§5.2 / §5.3** | Not reproduced in detail |
| PCO/CHAP encoding for V2 | **§6.2.13, Table 29** | V3-only (Table 30) |
| Timer encoding (Ready, Standby) | **§5.4, Tables 23/25** | References only |

A conformant implementer working with pre-V3 deployments, any
conventional-mode data capture, or anything involving SCEP
(including Direct Data, Repeated Data, and Conventional-FNE SCEP
captures) **must** read BAEB-B, not BAEB-C.

## 2. SCEP — Simple Convergence Encapsulation Protocol

### 2.1 What SCEP actually is

SCEP is a **TIA-102 standard convergence protocol**, not a Motorola-
proprietary extension. It is the only IP convergence protocol
defined for Direct Data and Repeated Data, and one of two options
(alongside SNDCP) for Conventional FNE Data. Reference:
TIA-102.BAEB-B §4, p. 11.

Name note: Motorola's conventional-mode documentation uses the
acronym SCEP with a slightly different expansion — "Simple CAI
Encapsulation Protocol" — which has caused confusion in at least one
field-reverse-engineering attempt. The TIA doc's official name is
**Simple *Convergence* Encapsulation Protocol**; Motorola's "Simple
CAI" gloss refers to the same protocol.

### 2.2 Zero-wrapper invariant

Per BAEB-B §4.1 and §6.4 mapping rules, a SCEP-conveyed IP datagram
is placed directly into a P25 CAI logical message. **No SCEP PDU
header is prepended.** The carrying CAI logical message uses:

- **SAP** = Packet Data
- **Data Header Offset** = 0 (indicating user payload begins at
  Octet 0 of the first data block)

So the bytes from the BAEB-A/B/C block-reassembly boundary straight
to the IPv4 header `0x45` are, in the pure-SCEP case, **zero extra
bytes**. Anything observed between those boundaries on a Motorola
conventional capture (13-byte AAAD encryption header, CAI-ID for
broadcast steering, or a registration-form discriminator) is coming
from a layer *other than SCEP* — usually AAAD crypto or Motorola's
registration framing — not from a SCEP PDU wrapper.

This resolves the main framing error that motivated gap 0017: there
is no SCEP-specific byte layout to reverse-engineer because SCEP
doesn't add bytes.

### 2.3 SCEP IP Address Binding

Two mechanisms (BAEB-B §4.1):

1. **Provisioned** — IP addresses pre-configured on SU and FNE.
2. **Dynamic via SCEP ARP** — SU broadcasts an ARP Request; the peer
   resolves the LLID↔IPv4 binding.

In Conventional FNE Data, the FNE may also be provisioned with
per-SU IP bindings and can learn new bindings during Packet Data
Registration.

### 2.4 SCEP ARP wire format

SCEP ARP (BAEB-B §4.2) is an adaptation of Ethernet ARP (RFC 826)
for P25, substituting 3-octet P25 LLIDs for 6-octet Ethernet MACs.

**22-octet message layout:**

```
Octet  Length  Field                      Value
-------------------------------------------------------------------
  0      2     Hardware Type              0x0021  (ARP-TYPE_CAI = 33)
  2      2     Protocol Type              0x0800  (IPv4)
  4      1     Hardware Address Length    0x03    (LLID size)
  5      1     Protocol Address Length    0x04    (IPv4)
  6      2     Operation                  0x0001 Request / 0x0002 Reply
  8      3     Sender Hardware Address    Sender LLID
 11      4     Sender Protocol Address    Sender IPv4
 15      3     Target Hardware Address    0xFFFFFF (Request broadcast)
                                          or responder LLID (Reply)
 18      4     Target Protocol Address    Target IPv4
```

- Delivery: Unconfirmed CAI delivery.
- Addressing: symmetric — Request/Reply use the same LLID-field
  conventions.
- Broadcast LLID for requests is the CAI "Designates Everyone"
  value (`0xFFFFFF`).

This is the likely source of the "CAI ID in a header transmitted
with each datagram" behavior cited in Motorola
`MN003252A01-A` §2.4 (p. 42) — the binding is established via ARP,
not carried in every data PDU. A passive decoder that sees SCEP ARP
exchanges can log the LLID↔IPv4 mapping for later unicast flows.

### 2.5 SCEP datagram conveyance paths

Per BAEB-B §4.3:

- **Conventional FNE inbound:** SU → FNE → DH. SU encapsulates IP
  using SCEP (i.e., places IP in CAI payload directly), FNE
  receives and routes.
- **Conventional FNE outbound:** DH → FNE → SU. FNE encapsulates
  and transmits to SU.
- **Direct Data:** SU → SU or SU → MDP. No FNE involved.
- **Repeated Data:** SU → SU or SU → MDP via a repeater. Repeater
  is transparent to SCEP.

MTU: BAEB-B §4.3 says "IPv4 datagrams of at least 512 octets per
the maximum CAI fragment length." This is consistent with
Motorola's 512-byte outbound fragment size and the 499-byte
encrypted variant (512 − 13-byte AAAD encryption header).

## 3. SNDCP Version Lineage

| Version | Added | Key semantics |
|---|---|---|
| **V1** (BAEB-A) | baseline | RFC 1144 Van Jacobsen TCP/IP header compression. Static or dynamic IP. |
| **V2** (BAEB-A-1) | APN, PCO/CHAP auth, DHCP | DHN selection via APN. PPP/CHAP tunneled in PCO for FNE-side authentication. |
| **V3** (BAEB-B) | RFC 2507, MSO, Version Supported | RFC 2507 IP header compression (replaces earlier UDP/IP scheme that was never defined). Manufacturer-Specific Options. Expanded reject-code set. Version-negotiation fallback. |

**SNDCP version field encoding** (4 bits, Request PDU Octet 0 lower
nibble):

| Value | Meaning |
|---|---|
| 0b0001 (1) | TIA-102 SNDCP V1 |
| 0b0010 (2) | TIA-102 SNDCP V2 |
| 0b0011 (3) | TIA-102 SNDCP V3 |
| 0b1000 (8) | WAI SNDCP V1 |
| 9–15 | WAI reserved |

WAI (Wideband Air Interface) SNDCP runs in a parallel version-code
range (8–15). An SU supporting both TIA-102 and WAI advertises both
in the Version Supported field.

## 4. State Machines and Timers

### 4.1 TDS (Trunked Data Service) states — SU side

Six states per BAEB-B Table 6:

| State | Entry trigger | Exit trigger |
|---|---|---|
| Closed | Initial; SU deregistered | SU registers via TMS |
| Idle | SU registered; no active context | PDCH grant (SN-DATA_CHN_GNT) |
| Ready* | PDCH grant received; awaiting first context activation | First context activated → Standby; Ready* Timer expires → Idle |
| Standby | ≥ 1 context active; not on PDCH; Standby Timer running | PDCH grant → Ready; Standby Timer expires → Idle |
| Ready | ≥ 1 context active; on PDCH | Ready Timer expires → Standby; Site Unacceptable → Ready Roaming |
| Ready Roaming | Roaming-zone Ready state | Resumed on original zone → Ready |

FNE side has the same five states minus Ready Roaming (BAEB-B Table
7 — 5 states: Closed, Idle, Ready*, Standby, Ready).

### 4.2 CDS (Conventional Data Service) states

5 states per BAEB-B Tables 8 / 9, no Ready Roaming. Closed↔Idle
transition driven by **SU LLID allocation/revocation** (rather than
TMS registration as in TDS). DAC field not used on CDS.

### 4.3 Timer values

| Timer | Default | Encoding | Used in |
|---|---|---|---|
| Ready* | 10 s | fixed | §5.2.3 context-activation waiting window |
| Activation Wait | 60 s | fixed | SU wait for FNE response |
| Ready | negotiated per-SU | Table 23 (4 bits, 16 values "Not Allowed" → "Always in Ready") | §5.2.4.2 Ready state duration |
| Standby | negotiated per-SU | Table 25 (4 bits, 16 values "Not Allowed" → "Always in Standby") | §5.2.4.1 Standby state duration |

The 4-bit encoded timer values map to specific durations per Tables
23 and 25. Two "corner" codepoints are reserved: one indicates the
SU is not permitted to enter that state; one indicates the state is
entered permanently (no timeout).

### 4.4 Context activation message flow (TDS, BAEB-B §5.2)

1. FNE sends **SN-DATA_CHN_GNT** (trunking grant) → SU enters
   **Ready\***.
2. SU sends **SN-Context Activation Request** (confirmed CAI
   delivery, SAP = SNDCP Packet Data Control, Data Header Offset = 0).
3. FNE responds with **Accept** or **Reject**.
4. On Accept: SU latches negotiated parameters (timers, MTU,
   compression, IP address, DAC) and enters **Standby**.
5. If no further request before Ready* Timer expires, SU returns to
   **Idle**.

An FNE-initiated activation path (V2+) uses the **SN-Context
Activation Command** — FNE solicits the SU to activate a specific
NSAPI, typically to pre-establish a data session.

## 5. SNDCP PDU Types and Fields

### 5.1 PDU Type dispatch (Octet 0 upper nibble, 4 bits)

Per BAEB-B Table 1:

| Value | PDU | Direction | Octet 0 lower nibble |
|---|---|---|---|
| 0 | SN-Context Activation Request | SU → FNE | SNDCP Version |
| 0 | SN-Context Activation Accept | FNE → SU | NSAPI |
| 1 | SN-Context Activation Command | FNE → SU | — |
| 2 | SN-Context Deactivation Request | bidirectional | NSAPI |
| 3 | SN-Context Activation Reject | FNE → SU | NSAPI |
| 4 | SN-UData | bidirectional | NSAPI |
| 5 | SN-Data | bidirectional | NSAPI |
| 6–15 | Reserved | — | — |

**Critical parser note:** PDU Type 0 is shared by Request and
Accept. Direction disambiguates (SU→FNE = Request; FNE→SU = Accept),
and the Octet 0 lower nibble has different meaning in each case
(SNDCP Version vs. NSAPI). A passive decoder that lacks definitive
direction must infer it from the carrying Confirmed Data Packet's
source/destination LLIDs.

### 5.2 Field semantics — the frequently-cited ones

**NSAPI (4 bits):** Network Service Access Point Identifier.

| Value | Meaning |
|---|---|
| 0 | Reserved for control signaling |
| 1–14 | Dynamic user-data contexts |
| 15 | Reserved |

NSAPI 1 is the only value all FNE implementations are **required**
to support. An SU seeking maximum interoperability should constrain
itself to NSAPI 1.

**NAT (4 bits):** Network Address Type. Controls IP assignment and
(in V3) whether the IPv4 Address field is present.

| Value | Meaning | IPv4 field present in V3? |
|---|---|---|
| 0 | No static address; FNE assigns dynamically | No |
| 1 | Static address; SU provides in PDU | Yes |

In V1/V2 PDUs, the IPv4 Address field is always present
(fixed-length formats). In V3, it is conditional on NAT value —
**a V3 parser must read NAT before knowing the offset of any
subsequent field**.

**DSUT (4 bits):** Data Subscriber Unit Type.

| Value | Meaning |
|---|---|
| 0 | Trunked Data Only |
| 1 | Alternate Trunked Voice and Data |
| 2 | Conventional Data Only |
| 3 | Alternate Conventional Voice and Data |
| 4 | Trunked and Conventional Data Only |
| 5 | Alternate Trunked and Conventional Voice and Data |
| 6–15 | Reserved |

**MTU (4 bits):** Maximum Transmission Unit.

| Value | MTU (octets) |
|---|---|
| 0 | Reserved |
| 1 | 296 |
| 2 | 510 |
| 3 | 1020 |
| 4 | 1500 |
| 5–15 | Reserved |

**UDPC (4 bits):** UDP compression support (0 = not supported,
1 = supported, 2–15 = reserved).

### 5.3 Reject codes

See `annex_tables/sndcp_reject_codes.csv` for the combined V1/V2 and
V3 tables. Key differences:

- V1/V2 (Table 26): codes 0–12.
- V3 (Table 27): codes 0–12 carry over, plus **14 Temporary
  Rejection**, **20 User Authentication Failed**, **21 Activation
  Rejected by External Network**, **22 APN Incorrect or Unknown**,
  **23 SNDCP Version Not Supported, Version Supported Field
  Included**.

When Reject Code = 23, the FNE appends a **Version Supported** field
(16 bits) to the Reject PDU so the SU can retry at a compatible
version. Bit encoding in Table 43:

| Bit | Supported version |
|---|---|
| 0 | Reserved |
| 1 | TIA-102 SNDCP V1 |
| 2 | TIA-102 SNDCP V2 |
| 3 | TIA-102 SNDCP V3 |
| 4–7 | TIA reserved |
| 8 | WAI SNDCP V1 |
| 9–15 | WAI reserved |

### 5.4 PCO / CHAP encoding (V2+)

PCO (Protocol Configuration Options) tunnels PPP protocol options
inside the SNDCP context-activation exchange — primarily to carry
CHAP authentication between SU and an external authentication
function reached via the FNE. Length byte indicates absence/presence:

| PCO Length | Meaning |
|---|---|
| 0x00 | PCO Value field absent |
| 0x01–0xFF | PCO Value field present, length in octets |

V2 (Table 29) permits Configuration Protocol = 0 (PPP) or 1 (DHCP);
PPP Protocol Identity values include 0xC023 (PAP, deprecated),
0xC223 (CHAP), 0xC021 (LCP, deprecated), 0x8021 (IPCP, deprecated).

V3 (Table 30) restricts Configuration Protocol to PPP (value 0) and
Protocol Identity to CHAP (0xC223). Earlier PAP/LCP/IPCP codepoints
are no longer supported.

The tunneled PPP information is a PPP packet stripped of Protocol
and Padding octets per RFC 1661.

### 5.5 APN encoding (V2+)

APN identifies the Data Host Network (and optionally an SNDCP
endpoint within the FNE). Encoding uses an APN Length byte:

| APN Length | Meaning |
|---|---|
| 0x00 | APN field absent |
| 0x01 | One-octet APN Index follows (compact form) |
| 0x02–0xFF | APN string of that length follows (2–255 octets) |

APN strings are DNS-style label sequences:

- **Network ID** — one or more DNS labels, **not** terminated with
  ".cpds".
- **Operator ID** — exactly three DNS labels ending with ".cpds".
- When both are present, they are joined by "." (Network ID first,
  Operator ID last).

The mapping of compact APN indices to actual strings is a local FNE
policy decision. How an SU discovers the desired APN from its MDP
application is outside the scope of BAEB-B.

### 5.6 RFC 2507 sub-fields (V3)

When HCOMP bit 2 is set (indicating RFC 2507 header compression is
being negotiated), six sub-fields accompany the negotiation in the
Request/Accept PDUs:

| Field | Size | Meaning |
|---|---|---|
| VJ_TCP_CSS | 8 bits | Number of VJ TCP/IP compression state slots (range 0–255) |
| TCP_CSS | 8 bits | RFC 2507 TCP compression state slots (0–255) |
| NON_TCP_CSS | 16 bits | RFC 2507 non-TCP compression state slots (0–65535) |
| MAX_INT_FHD | 8 bits | Max compressed non-TCP headers between full headers: value × 16 (0 = disabled) |
| MAX_TME_FHD | 8 bits | Max time between full headers: value × 5 seconds (0 = disabled) |
| MAX_HDR_SZE | 8 bits | Max header size: 0 = not significant, 1–59 reserved, 60–255 = octet count |

Per BAEB-B §6.2.15.5: the Maximum Context Identifier for TCP and
non-TCP is **not** negotiated (RFC 2507 default of 15 is used); and
the RFC 2507 **reordering-of-streams option is not supported**.

Motorola's trunked Feature Guide tunables
*"Max Number of Compressed Headers Between Full Headers"* and
*"Max Time Between Full Headers"* map directly onto
`MAX_INT_FHD × 16` and `MAX_TME_FHD × 5 s` respectively.

### 5.7 MSO — Manufacturer Specific Options (V3)

Per §6.2.16, Tables 41–42:

| Sub-field | Size | Value |
|---|---|---|
| MSO Length | 8 bits | 0 = absent, 1–255 = length of Value |
| MFID | 8 bits | Manufacturer's Identifier (TIA reserved-values registry) |
| MSO Value | variable | Manufacturer-defined |

Intended to let a vendor add proprietary context options for its own
SUs while remaining interoperable with other-vendor SUs that ignore
the field. The MFID lets the receiver dispatch to the correct
parser. MFID values come from TIA-102.BAAC-D reserved-values tables.

## 6. Context-Management PDU Byte Layouts

All PDUs in this subsection use: **Confirmed CAI Data Packet
Delivery**, **SAP = SNDCP Packet Data Control**, **Data Header
Offset = 0**.

### 6.1 SN-Context Activation Request V1 (Figure 16, 10 octets fixed)

```
Octet  Bits 7–4      Bits 3–0
  0    PDU Type (0)  SNDCP Version (%0001)
  1    NSAPI         NAT
  2–5  IPv4 Address (always present in V1)
  6    DSUT          UDPC
  7    IPHC (8 bits)
  8    TCPSS         UDPSS
  9    Reserved      MDPCO
```

### 6.2 SN-Context Activation Request V2 (Figure 17, variable)

```
Octet  Bits 7–4      Bits 3–0
  0    PDU Type (0)  SNDCP Version (%0010)
  1    NSAPI         NAT
  2–5  IPv4 Address (always present in V2)
  6    DSUT          UDPC
  7    IPHC (8 bits)
  8    TCPSS         UDPSS
  9    Reserved      MDPCO
  10   APN Length (8 bits)
  11–W APN Value (0–255 octets, conditional)
  X    PCO Length (8 bits)
  Y–Z  PCO Value (0–255 octets, conditional)
```

### 6.3 SN-Context Activation Request V3 (Figure 18, variable)

```
Octet               Bits 7–4       Bits 3–0
  0                 PDU Type (0)   SNDCP Version (%0011)
  1                 NSAPI          NAT
  [if NAT=1: Octets 2–5 = IPv4 Address (32 bits)]
  next              DSUT           UDPC
  +1                HCOMP (8 bits)
  +2                VJ_TCP_CSS (8 bits)
  +3                TCP_CSS (8 bits)
  +4..+5            NON_TCP_CSS (16 bits)
  +6                MAX_INT_FHD (8 bits)
  +7                MAX_TME_FHD (8 bits)
  +8                MAX_HDR_SZE (8 bits)
  +9                Reserved       MDPCO
  +10               APN Length (8 bits)
  +11..APN_end      APN Value (conditional)
  APN_end+1         PCO Length (8 bits)
  PCO begin..end    PCO Value (conditional)
  MSO offset        MSO Length (8 bits)
  MSO+1..end        MSO Value (conditional)
```

**Parsing rule for V3:** read Octet 1 lower nibble (NAT) first. If
NAT = 1, skip 4 octets for IPv4 Address before reading DSUT. Subsequent
offsets shift by 4 based on this decision.

### 6.4 SN-Context Activation Accept V1 (Figure 19, 13 octets fixed)

```
Octet  Bits 7–4      Bits 3–0
  0    PDU Type (0)  NSAPI
  1    PDUPM         Ready
  2    Standby       NAT
  3–6  IPv4 Address
  7    IPHC
  8    TCPSS         UDPSS
  9    MTU           UDPC
  10   Reserved      MDPCO
  11–12 Data Access Control (16 bits)
```

### 6.5 SN-Context Activation Accept V2 (Figure 20, variable)

```
Octet    Bits 7–4      Bits 3–0
  0      PDU Type (0)  NSAPI
  1      PDUPM         Ready
  2      Standby       NAT
  3–6    IPv4 Address (always present)
  7      IPHC
  8      TCPSS         UDPSS
  9      MTU           UDPC
  10     Reserved      MDPCO
  11–12  Data Access Control (16 bits)
  13     APN Length
  14–W   APN Value (conditional)
  X      PCO Length
  Y–Z    PCO Value (conditional)
```

### 6.6 SN-Context Activation Accept V3 (Figure 21, variable)

```
Octet                 Bits 7–4      Bits 3–0
  0                   PDU Type (0)  NSAPI
  1                   PDUPM         Ready
  2                   Standby       NAT
  [if NAT=1: Octets 3–6 = IPv4 Address (32 bits)]
  next                HCOMP (8 bits)
  +1                  VJ_TCP_CSS
  +2                  TCP_CSS
  +3..+4              NON_TCP_CSS (16 bits)
  +5                  MAX_INT_FHD
  +6                  MAX_TME_FHD
  +7                  MAX_HDR_SZE
  +8                  MTU           UDPC
  +9                  Reserved      MDPCO
  +10..+11            Data Access Control (16 bits)
  +12                 APN Length
  +13..APN_end        APN Value (conditional)
  APN_end+1           PCO Length
  PCO begin..end      PCO Value (conditional)
  MSO offset          MSO Length
  MSO+1..end          MSO Value (conditional)
```

### 6.7 SN-Context Activation Reject (Figures 22–23)

```
V1 / V2 (2 octets fixed):
Octet  Bits 7–4      Bits 3–0
  0    PDU Type (3)  NSAPI
  1    Reject Code (8 bits)

V3 (2 or 4 octets):
Octet  Bits 7–4      Bits 3–0
  0    PDU Type (3)  NSAPI
  1    Reject Code (8 bits)
  2–3  Version Supported (16 bits, present only when Reject Code = 23)
```

### 6.8 SN-Context Deactivation Request (Figure 24, 2 octets, all versions)

```
Octet  Bits 7–4      Bits 3–0
  0    PDU Type (2)  NSAPI
  1    Deactivation Type (8 bits)
```

Deactivation Type = 0 → deactivate all NSAPIs; = 1 → deactivate the
NSAPI in this PDU; 2–255 reserved.

### 6.9 SN-Context Activation Command V2/V3 (Figure 25, variable)

```
Octet  Bits 7–4      Bits 3–0
  0    PDU Type (1)  SNDCP Version
  1    NSAPI         NAT
  2    APN Length
  3–X  APN Value (conditional)
  Y–Z  IPv4 Address (32 bits)
```

FNE-initiated activation — FNE tells SU to bring up a specific
context with a suggested APN and IP.

## 7. Data Payload PDU Byte Layouts

All PDUs in this subsection use: **SAP = Unencrypted User Data**
(or Encrypted User Data per AACA/AAAD), **Data Header Offset = 2**
— meaning the IP datagram begins 2 octets into the first data block
after the 2-byte SNDCP header.

### 7.1 SN-Data (Confirmed) — Figure 26, all SNDCP versions

```
Octet  Bits 7–4      Bits 3–0
  0    PDU Type (5)  NSAPI
  1    PCOMP         DCOMP
  2–N  Network PDU (IP datagram)
```

### 7.2 SN-UData (Unconfirmed) V1 / V2 — Figure 27

```
Octet  Bits 7–4      Bits 3–0
  0    PDU Type (4)  NSAPI
  1    Reserved      DCOMP      <-- Reserved nibble MUST be zero
  2–N  Network PDU (IP datagram)
```

**Note the V1/V2 vs V3 asymmetry:** in V1/V2 SN-UData, Octet 1
upper nibble is **Reserved** (shall be zero), while in V3 SN-UData
(below) it is reused as **PCOMP**. This is one of the rare
SNDCP-version-visible differences in the on-wire data PDU.

### 7.3 SN-UData (Unconfirmed) V3 — Figure 28

```
Octet  Bits 7–4      Bits 3–0
  0    PDU Type (4)  NSAPI
  1    PCOMP         DCOMP
  2–N  Network PDU (IP datagram)
```

### 7.4 PCOMP / DCOMP semantics — watch the revision

**In BAEB-B §6.4:** both PCOMP and DCOMP are described as reserved
for future use (no codepoints assigned). PCOMP is "Payload
Compression Identifier"; DCOMP is "Data Compression Identifier".

**In BAEB-C (later revision):** PCOMP was repurposed as the
**header-compression identifier** — PCOMP = 0 (no compression),
1–2 (RFC 1144), 3–6 (RFC 2507). DCOMP remained reserved.

A passive decoder that sees a non-zero PCOMP value on trunked
traffic is looking at BAEB-C-era semantics. A non-zero DCOMP value
is non-normative in both revisions.

## 8. Annex A — Fields Modifiable by Unsolicited Accept

Per BAEB-B Annex A, the FNE may send an **unsolicited** SN-Context
Activation Accept while a context is active to update certain
negotiated parameters. Most fields are **not** modifiable — the
standardized modifiable set is small:

| Field | Modifiable |
|---|---|
| Ready | Yes (TDS §5.2.4.2, CDS §5.3.4.2) |
| Standby | Yes (TDS §5.2.4.1, CDS §5.3.4.1) |
| DAC | TDS only; No for V1/V2, Yes for V3+ (§5.2.7.1.1). Not used on CDS. |
| MSO | Defined by the manufacturer (vendor-specific behavior) |
| All other fields | Not modifiable by unsolicited Accept |

A parser that sees an incoming unsolicited Accept must treat it as a
timer-and/or-DAC update, not a full context re-establishment — the
NSAPI, IP address, compression state, and MTU all persist from the
original Accept.

## 9. Reconciliation with Observed Motorola Trunked Traffic

This section maps BAEB-B byte semantics onto the bytes observed in
the Sachse capture (see
`analysis/motorola_sndcp_npdu_preamble.md`).

**Implementer-confirmed layering on Motorola trunked captures:**

```
[ CAI reassembled payload ]
   [ LLC user-plane bytes   ]   <-- starts with e.g. 0x40 0x06 ...
   [ SNDCP header (2 bytes) ]   <-- sits at ip_offset - 2
   [ IPv4 datagram          ]   <-- starts at 0x45 (ip_offset)
```

**Earlier drafts of this spec tried to decode the initial
`0x40 0x06 ...` bytes as the start of the SNDCP header under
Figures 26 / 27 / 28. That was mis-scoping.** Those bytes belong
to the LLC user-plane layer (TIA-102.BAED-A), which sits ahead of
SNDCP in the wire layering. The implementer-side passive decoder
(`blip25-data`) reports that when the SNDCP header is anchored at
`ip_offset - 2` — i.e., the two octets immediately preceding the
IPv4 `0x45` version nibble — it decodes cleanly under standard
BAEB-B / BAEB-C Figure 26 semantics:

- Octet `ip_offset − 2` = `PDU Type | NSAPI`. PDU Type 4 (SN-UData)
  or 5 (SN-Data); NSAPI in the normal user-context range (1–14),
  not 0.
- Octet `ip_offset − 1` = `PCOMP | DCOMP`. Both nibbles 0 on the
  observed traffic (no header compression, no payload compression).

So on Motorola trunked unconfirmed flows, the on-wire SNDCP header
is standards-conformant. There is no reserved-DCOMP anomaly and no
NSAPI-0-on-data-flow anomaly; the earlier apparent anomalies were
artifacts of reading LLC bytes as SNDCP.

**What the LLC bytes carry (open question):** BAED-A §2 describes
the user-plane sequence / window / ACK/SACK mechanics but doesn't
pin down the exact byte offsets within the first data block. That's
recorded as the leading open item in
`analysis/motorola_sndcp_npdu_preamble.md` §8.

**For the conventional case** (not observed in Sachse; applies to
any true conventional Motorola IV&D capture):

- There is **no SNDCP header**. SAP = Packet Data; Data Header
  Offset = 0. The IP datagram begins at Octet 0 of the reassembled
  payload.
- Any "extra bytes" between the block boundary and `0x45` come
  from TIA-102.AAAD encryption (13 bytes) or Motorola registration
  framing — not SCEP itself.

## 10. Cross-References

- `standards/TIA-102.BAEB-C/P25_IP_Data_Bearer_Implementation_Spec.md`
  — SNDCP-V3 consolidated spec; complement to this BAEB-B
  derivation.
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md`
  — LLC layer that sits between BAEB-*blocks and SNDCP on trunked
  IV&D (explains the "extra bytes" observed in gap 0013 / 0018).
- `analysis/motorola_conventional_scep_vs_trunked_sndcp.md` —
  architectural witness; updated with the BAEB-B correction that
  SCEP is TIA-standard, not Motorola-proprietary.
- `analysis/motorola_sndcp_npdu_preamble.md` — observed Motorola
  trunked preamble bytes and the LLC + SNDCP layering.
- `analysis/fdma_pdu_frame.md` §4.3 — trunked-IV&D data PDU
  example, consistent with this spec's §7.
- `standards/TIA-102.AABF-D` / `TIA-102.AABC-E` — SAP value
  assignments used by the SNDCP Packet Data Control and Packet
  Data / User Data SAPs referenced throughout §6 / §7.
- `standards/TIA-102.AACA` — over-the-air rekeying; encryption for
  SNDCP data PDUs uses the Encrypted User Data SAP.
- `standards/TIA-102.AAAD-B/P25_Block_Encryption_Protocol_Implementation_Spec.md`
  — 13-byte encryption header on encrypted conventional data.
- `standards/TIA-102.BAJD-A/P25_Port_Number_Assignments_Implementation_Spec.md`
  — UDP port assignments for the applications that ride above
  this IP data stack.
- RFC 1144 (Van Jacobsen TCP/IP header compression), RFC 1661
  (PPP), RFC 1994 (CHAP), RFC 2507 (IP header compression) —
  normative RFCs referenced by SNDCP.

## 11. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- SCEP protocol — TIA-102.BAEB-B §4, pp. 11–16.
- SCEP ARP message format — §4.2.
- SNDCP state machines — §5.2 (TDS), §5.3 (CDS), pp. 20–45
  (Tables 6–9 for state × event matrices).
- Timers — §5.4, Tables 23 and 25.
- PDU type table — §6.1, Table 1.
- Field definitions — §6.2, Tables 12–43.
- Context management PDU mapping — §6.3, Figures 16–25.
- Data payload PDU mapping — §6.4, Figures 26–28.
- Modifiable fields — Annex A.
