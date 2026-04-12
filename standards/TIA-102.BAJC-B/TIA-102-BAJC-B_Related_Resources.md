# TIA-102.BAJC-B — Related Resources and Project Context

## Document Identity

| Field | Value |
|-------|-------|
| Full title | ANSI/TIA-102.BAJC-B: Project 25 — Location Registration Reporting Protocol (LRRP) |
| Document ID | TIA-102.BAJC-B |
| Published | March 2019 |
| Supersedes | TIA-102.BAJC-A |
| Status in this project | Processed (Phase 1 + Phase 2 complete; Phase 3 pending) |
| Classification | protocol, location |
| Pages | 163 (184 including back matter) |

---

## Normative References (from Document)

| Ref | Document | Relevance |
|-----|----------|-----------|
| [1] | TIA-102.BAJC-A | Previous version of LRRP; superseded by this document |
| [2] | W3C EXI (Efficient XML Interchange) specification | Mandatory encoding for all LRRP messages |
| [3] | IETF RFC 791 | IPv4 addressing (suaddr-type IPV4, recipient-addr) |
| [4] | IETF RFC 2460 | IPv6 addressing (suaddr-type IPV6) |
| [9] | W3C XML Schema Parts 0, 1, 2 | XML schema format for Section 8 normative XSD |
| [11] | TIA-102.BAAA | P25 SUID addressing — defines APCO and LTD suaddr formats |
| [20] | ETSI TS 100 392-1 | TETRA subscriber addressing (ITSI format for TETRA suaddr) |

---

## Related TIA-102 Documents (P25 Ecosystem)

### Location Services Family
| Document | Title | Relationship |
|----------|-------|--------------|
| TIA-102.BAJC-A | LRRP previous version | Directly superseded by BAJC-B |
| TIA-102.BAJC (original) | Original LRRP spec | Ancestor |
| TIA-102.BAJB-A | P25 Location Services Tier 1 (XCMP-based) | Complementary — Tier 1 air interface location; BAJC-B is Tier 2 IP-based |
| TIA-102.BAJB-B | P25 Location Services Tier 2 | Likely defines higher-layer location service framework within which LRRP operates |

### P25 Tier 2 Infrastructure
| Document | Title | Relationship |
|----------|-------|--------------|
| TIA-102.BADA-1 / BADA-A | P25 ISSI and CSSI | Provides the IP inter-subsystem interconnect over which LRRP UDP runs |
| TIA-102.BACA-B | P25 ISSI Voice and Mobility | Context for SU mobility on Tier 2 networks |
| TIA-102.BAAA-A | P25 SUID and addressing | Defines APCO/LTD subscriber address formats used in suaddr element |

### Protocol Encoding
| Document | Title | Relationship |
|----------|-------|--------------|
| W3C EXI Spec | Efficient XML Interchange | Normative encoding reference — LRRP uses schema-informed EXI, bit-packed, strict mode |
| W3C XML Schema | XML Schema Definition Language | Normative reference for Section 8 XSD syntax |

---

## Phase 3 Implementation Specs — Flagged (Not Produced This Run)

Two implementation specs should be produced in a follow-up pass:

### 1. P25_LRRP_Protocol_State_Machine_Implementation_Spec.md
**What to cover:**
- State machine for each service role (Requester, Responder, Recipient)
- Immediate Location Service: Request → waiting → Report (or timeout)
- Triggered Location Service: Request → Answer → Reports → Stop-Request → Stop-Answer; error state transitions; what happens on REPORTING WILL STOP
- Unsolicited Report Service: event → send report; handle UNKNOWN SUBSCRIBER response
- Location Protocol Version Service: Request → negotiate version → Report; fallback on UNSUPPORTED VERSION
- Service Notification: Register → HOURS response; Re-Register; De-Register; retransmit timer logic (SN_SHORT/SN_LONG/SN_MAX)
- Tier 2 Profile management triggers

### 2. P25_LRRP_Message_Format_Implementation_Spec.md
**What to cover:**
- Complete message builder/parser for all 10 LRRP message types
- EXI encoding pipeline: XML → EXI compress (bit-packed, strict, schemaID=0x00) → strip header → UDP payload
- EXI decoding pipeline: UDP payload → reconstruct header → version check → decompress → XML
- All XML element types: serialization and deserialization with proper type unions (intOrFloat, uIntOrFloat, lrrpAngle, etc.)
- Angle encoding: lrrpAngle value = actual_degrees / 2; intAngle for even degrees, floatAngle for fractional
- suaddr serialization for each type (APCO 7 bytes, PLMN BCD nibbles, IPV4 4 bytes, IPV6 16 bytes, TETRA 6 bytes, LTD 3 bytes)
- recipient-addr format: `ipv4-udp:XXXXXXXX:YYYY` (uppercase hex, no separator between fields except colon)
- Result code enum and error response construction
- Multiple LRRP documents per UDP packet: packing as multiple EXI documents in single body

---

## Processing Status

| Phase | Status | Notes |
|-------|--------|-------|
| Phase 1 — Classification | Complete | protocol, location |
| Phase 2 — Full Text | Complete | TIA-102-BAJC-B_Full_Text.md |
| Phase 2 — Summary | Complete | TIA-102-BAJC-B_Summary.txt |
| Phase 2 — Related Resources | Complete | This file |
| Phase 3 — Protocol State Machine Spec | Pending | Flag for follow-up |
| Phase 3 — Message Format Spec | Pending | Flag for follow-up |

---

## Key Implementation Notes for Rust

- The EXI encoding is the critical path for any Rust implementation. Existing EXI libraries for Rust are sparse; may need to implement a schema-informed EXI encoder/decoder from scratch or port a C/C++ library (e.g., OpenEXI, EXIficient).
- The XML schema (Section 8) should be used directly as the reference for struct/enum definitions. Each complex type maps cleanly to a Rust struct; choice types map to enums.
- The lrrpAngle type requires careful handling: the stored value is half the actual degree value. Range check: value < 180 (not ≤ 180; the spec says "less than 180").
- The uIntOrFloat union is distinguished by the presence of a decimal point in the text representation — this is an XML-level distinction, not a binary one.
- UDP-only transport means a standard tokio/async-std UDP socket is the transport layer. No TCP fallback.
- Multiple LRRP documents per UDP packet requires a framing layer within the EXI body.
- Service notification registration state (HOURS counter) should be maintained by both SU and LSHS implementations.
