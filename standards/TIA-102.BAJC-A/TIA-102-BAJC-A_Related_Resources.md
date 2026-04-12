# TIA-102.BAJC-A Related Resources
## Location Registration and Reporting Protocol (LRRP) for P25

---

## Standards Lineage

### Direct Predecessor / Companion Documents

| Document | Title | Relationship |
|----------|-------|--------------|
| TIA-102.BAJC | Location Registration and Reporting Protocol (base) | Parent specification; BAJC-A is a revision/amendment |
| TIA-102.BACE | EXI Document Format for P25 Data Applications | Normative reference [2]; defines EXI rules used by LRRP |
| TIA-102.BAAA-A | P25 Data Overview | System context for P25 data layer |
| TIA-102.BABA | P25 Tier 2 Data Transport | Transport layer LRRP operates over |

### P25 Data Application Suite (TIA-102.BA series)

| Document | Title | Relationship |
|----------|-------|--------------|
| TIA-102.BAAB | P25 UDP/IP Mapping | Transport mapping; LRRP uses UDP/IP [reference in document] |
| TIA-102.BACA-B | P25 ISSI Voice and Mobility | Same data infrastructure; inter-system location awareness |
| TIA-102.BAEA | P25 Data Gateway | Data bridging context |
| TIA-102.BAEB | P25 Short Data Services | Alternate data bearer |
| TIA-102.BAEC | P25 Packet Data Services | Core data transport context |
| TIA-102.BAJB | P25 Location Trigger Services | Companion; location triggering from network side |

### P25 Identity and Addressing

| Document | Title | Relationship |
|----------|-------|--------------|
| TIA-102.BAAA | P25 Common Air Interface (data) | APCO SUID long/short formats referenced in suaddr element |
| TIA-102.AABA | P25 Common Air Interface (voice/CAI) | WACN/system ID structure |

### Security / OTAR

| Document | Title | Relationship |
|----------|-------|--------------|
| TIA-102.AACA | P25 OTAR Messages and Procedures | Security key management; SU identity management relevant to location authorization |

---

## External Standards Referenced

### EXI (Efficient XML Interchange)

| Standard | Description | Relevance |
|----------|-------------|-----------|
| W3C EXI Specification | Efficient XML Interchange Format | Normative [reference 9]; defines the EXI compression algorithm used for LRRP messages. Schema-informed mode, bit-packed alignment, strict option. |
| W3C XML Schema Part 1 | XML Schema Structures | Framework for the LRRP XSD schema (Section 8) |
| W3C XML Schema Part 2 | XML Schema Datatypes | xs:decimal, xs:dateTime, xs:hexBinary, xs:unsignedInt, xs:float types used throughout |

### IP Networking

| Standard | Description | Relevance |
|----------|-------------|-----------|
| IETF RFC 791 | Internet Protocol (IPv4) | Normative [reference 3]; IPv4 addressing in suaddr and recipient-addr |
| IETF RFC 2460 | Internet Protocol Version 6 (IPv6) | Normative [reference 4]; IPv6 addressing in suaddr |

### Geodesy

| Standard | Description | Relevance |
|----------|-------------|-----------|
| WGS-84 | World Geodetic System 1984 | Mandatory datum for all LRRP coordinates; all lat/long/altitude values |
| ETRS89 | European Terrestrial Reference System | Regional WGS-84 equivalent |

### Interoperability Standards (referenced in suaddr)

| Standard | Description | Relevance |
|----------|-------------|-----------|
| TETRA ITSI | Terrestrial Trunked Radio Individual Subscriber Identity | suaddr-type="TETRA": 6-byte format, 24-bit country/network + 24-bit SU |
| ETSI EN 300 392 | TETRA Voice+Data | TETRA system context for multi-standard interop |
| ITU E.164 / PLMN | Phone number encoding | suaddr-type="PLMN": BCD nibble encoding per Table 8 |

---

## Open Source Implementations and Projects

### P25 Software-Defined Radio

| Project | URL | Relevance |
|---------|-----|-----------|
| OP25 | https://github.com/boatbod/op25 | Open-source P25 SDR; handles P25 data including location services. Reference for protocol-level testing. |
| gr-p25 | Part of OP25 / GNU Radio | GNU Radio P25 blocks; data channel decoding |
| SDR Trunk | https://github.com/DSheirer/sdrtrunk | Java-based P25 decoder with data channel support |

### EXI Libraries

| Library | Language | URL |
|---------|----------|-----|
| EXIficient | Java | Open-source EXI codec; schema-informed mode supported |
| OpenEXI | Java | W3C EXI reference implementation |
| exip | C | Lightweight C EXI library; suitable for embedded SU implementation |

### XML / Schema Tools

| Tool | Relevance |
|------|-----------|
| xmllint | Validate LRRP XML against Section 8 schema |
| xsv | XML Schema Validator for conformance testing |
| jaxb / serde + quick-xml | Code generation from XSD for Java/Rust implementations |

---

## Technical Context and Background

### Location Services in LMR vs. Cellular

LRRP was designed specifically for Land Mobile Radio systems with these constraints not present in cellular location protocols:

- **No persistent IP connection**: P25 SUs are not always reachable via IP; the protocol uses UDP with explicit SU registration (Section 10 Service Notification) to track reachability.
- **Bandwidth constraints**: LRRP XML is compressed with EXI to minimize air interface usage. The EXI header stripping (Section 9.2) removes the 4-byte cookie and verbose options for further savings.
- **Multi-system addressing**: suaddr supports APCO (P25), TETRA, PLMN, LTD, and IP addressing to enable cross-system location sharing.
- **SU autonomy**: Unlike cellular, P25 SUs have significant autonomy; Unsolicited Location Reports allow SU-initiated event reporting (emergency, PTT, power cycle, DMO/TMO transitions).

### Relationship to 3GPP Location Services

| 3GPP Concept | LRRP Equivalent |
|-------------|-----------------|
| LCS Client | Requester / LSHS |
| Target UE | SU / MDP |
| GMLC | LSHS |
| LPP (LTE Positioning Protocol) | LRRP (P25 equivalent) |
| OMA MLP | Not used; LRRP is proprietary XML |
| SUPL (Secure User Plane Location) | Loosely analogous to LRRP Triggered service |

### Relationship to APCO Standards

| APCO Standard | Title | Relevance |
|---------------|-------|-----------|
| APCO ANS 1.106.1 | Automated Vehicle Location for CAD | End-use context for LRRP location data |
| APCO ANS 2.101.1 | CAD-to-CAD | Location data sharing across dispatch centers |
| APCO ANS 1.104.1 | Computer Aided Dispatch | Consumer of LRRP location information |

---

## Implementation Considerations for P25 LRRP

### EXI Schema Version Management

The `schemaID` in the EXI header is the critical versioning mechanism. Current value is **0x00**. When a future version of LRRP modifies the schema:
- The schemaID SHALL be incremented
- The result-code 0x08 (UNSUPPORTED VERSION) exchange allows SUs and LSHS to negotiate to the highest mutually supported version
- Implementations must store their supported schemaID list and negotiate downward

### Trigger Lifecycle Management

Triggered location services require careful state management:
- Each `request-id` identifies an active trigger session
- Stop-Request must reference the same request-id as the original Request
- result-code 0x15 (NO SUCH REQUEST) is returned if no matching active trigger
- result-code 0x11 (REPORTING WILL STOP) is returned when a triggered session self-terminates (e.g., stop-time reached)
- Multiple concurrent triggers per SU are possible; each with a distinct request-id

### Service Notification Retry Logic

The Service Notification retry parameters (from Section 10.1):
- SN_SHORT (suggested 30 seconds): time to wait for LSHS response before retransmission
- SN_LONG (suggested 5 minutes): wait before restarting the whole registration sequence after SN_MAX failures
- SN_MAX (suggested 5): maximum retransmission attempts before waiting SN_LONG

These are **local policy** values, not mandated by the standard.

### Default Location Response Behavior

When no `query-info` element is present, the Responder SHALL return latitude/longitude in WGS-84. This is also the fallback when neither request-impl-spec nor require-impl-spec is specified (LRRP-DF-QI-001 and LRRP-DF-QI-003).

### Angle Encoding Pitfall

lrrpAngle values are in **units of 2 degrees**, not degrees:
- An intAngle value of 90 represents 180° (south), not 90° (east)
- An intAngle value of 45 represents 90° (east)
- intAngle cannot represent odd degree values
- floatAngle allows: 45.5 = 91°
- Maximum representable angle: just under 360° (value must be < 180)

---

## Key Tables Quick Reference

### Table 1 — Application IDs
| Range | Use |
|-------|-----|
| 0x00–0x3F | Implementation-defined |
| 0x41 | Service Notification |

### Table 4 — Trigger Condition Data (impl-spec-tdata)
| 0x00 | BATTERY | 0x01 | STATUS | 0x02 | STATUS-TETRA |
| 0x03 | STATUS-APCO | 0x04 | STATUS-LTD | 0x40+ | Impl-defined |

### Table 5 — Reserved Request IDs (Unsolicited Reports)
0x00=Power ON, 0x01=Power OFF, 0x02=Emergency, 0x03=PTT,
0x04=Status, 0x05=TIM ON, 0x06=TIM OFF, 0x07=System accessible,
0x08=Direct Mode, 0x09=Enter service, 0x0A=Leave service,
0x0B=Cell reselect, 0x0C=Low battery, 0x0D=Car Kit ON,
0x0E=Car Kit OFF, 0x0F=Coverage loss, 0x10=Coverage recovery,
0x2D=Predefined movement, 0x2E=Predefined text msg, 0x2F=Periodic

### Table 7 — Key Result Codes
0x00=SUCCESS, 0x08=UNSUPPORTED VERSION, 0x0A=SYNTAX ERROR,
0x11=REPORTING WILL STOP, 0x12=TIME EXPIRED, 0x15=NO SUCH REQUEST,
0x51=POSITION METHOD FAILURE, 0x200+=GPS errors, 0x202+=impl-specific

### Table 9 — Trigger Condition Codes
0x00=Power ON, 0x01=Power OFF, 0x02=Emergency, 0x03=PTT,
0x04=Status (+ STATUS tdata), 0x0C=Low battery (+ BATTERY tdata),
0x40+=Implementation-defined
