# TIA-102.CABA — Related Resources and Standards Context

## Document Identity

| Field | Value |
|-------|-------|
| Document ID | TIA-102.CABA |
| Full Title | Project 25 Standards for Conventional Voice Operation — Interoperability Test Procedures |
| Classification | CONFORMANCE — Interoperability test procedures |
| Phase 3 Spec | Not applicable (no implementation specification warranted) |
| Normative for | Verifying interoperability of P25 conventional (FDMA) equipment |

---

## Standards Family Context

TIA-102.CABA sits within the P25 Suite II (TIA-102) standards family as the conformance
document for conventional voice operation. It tests the protocols defined in the core air
interface and supplementary data standards.

### Direct Normative References (Tested by this Document)

| Standard | Title | Relationship |
|----------|-------|--------------|
| **TIA-102.BAAB** | Project 25 FDMA Common Air Interface | Primary air interface spec — all RF framing, NAC, TGID, and voice channel behavior tested here is defined there |
| **TIA-102.AAAD** | IMBE Vocoder | Vocoder encoding used in all voice tests |
| **TIA-102.BABA** | Project 25 FDMA IMBE Phase 2 Vocoder | Phase 2 vocoder; tested where supported |
| **TIA-102.AABF** | Project 25 Security | Encryption (DES/AES) — defines key management and encryption framing tested in §2.4.13 and §2.6.6 |
| **TIA-102.BAAC** | Link Control Word Formats and Messages | LC word formats for voice headers, group call, U-U call, emergency |
| **TIA-102.BAAD** | Trunking Control Channel Messages | Defines supplementary data message formats (call alert, radio check, message update, status update, status query, radio unit monitor, inhibit/uninhibit, emergency alarm) |

### Related Conformance Documents

| Standard | Title | Relationship |
|----------|-------|--------------|
| TIA-102.CAAA | P25 Conventional Data Interoperability Test Procedures | Companion conformance document for data services on conventional channels |
| TIA-102.CAAB | P25 Trunking Interoperability Test Procedures | Trunking equivalent of this document |
| TIA-102.CAAC | P25 ISSI Interoperability Test Procedures | Inter-RF-Subsystem Interface conformance |

### Protocol Specifications for Features Tested

| Feature Tested | Defining Standard |
|----------------|-------------------|
| NAC filtering ($293, $300, $F7F, $F7E) | TIA-102.BAAB §CAI RF layer |
| Selective/Normal/Monitor squelch | TIA-102.BAAB |
| Group call (TGID) | TIA-102.BAAB, TIA-102.BAAC |
| Unit-to-unit call | TIA-102.BAAB, TIA-102.BAAC |
| Emergency call and alert | TIA-102.BAAB, TIA-102.BAAC, TIA-102.BAAD |
| Call alert | TIA-102.BAAD |
| Radio check | TIA-102.BAAD |
| Message update | TIA-102.BAAD |
| Status update / status query | TIA-102.BAAD |
| Radio unit monitor | TIA-102.BAAD |
| Radio inhibit / uninhibit | TIA-102.BAAD |
| All call (DMC) | TIA-102.BAAC, TIA-102.BAAD |
| Encryption (DES/AES) | TIA-102.AABF |
| Co-channel interference suppression | TIA-102.BAAB |
| IMBE voice encoding | TIA-102.AAAD, TIA-102.BABA |

---

## P25 CAI (TIA-102.BAAB) Key Parameters Referenced

The following parameters from TIA-102.BAAB are central to understanding the test configurations:

### Network Access Code (NAC)
- 12-bit field in the Network ID (NID) symbol of every P25 frame
- Provides RF channel filtering analogous to CTCSS/DCS on analog systems
- Special values: $F7F = receive any NAC, retransmit as-received; $F7E = receive any NAC, retransmit with configured TX NAC
- Default test value in this document: $293
- Rejection test value: $300

### Talk Group ID (TGID)
- 16-bit identifier in the Link Control word
- Used by selective squelch to accept or reject a received transmission
- Set to 1 for most test configurations in this document

### Squelch Modes
- **Normal**: unsquelch on matching NAC, any TGID
- **Selective**: unsquelch on matching NAC AND matching TGID
- **Monitor**: unsquelch on any received P25 signal regardless of NAC or TGID

---

## P25 FDMA vs. TDMA Context

TIA-102.CABA tests **FDMA (Phase 1)** conventional operation only. The P25 Phase 2 TDMA
standards (TIA-102.BBAB and related) have separate conformance documents. IMBE vocoder
(Phase 1) is the primary voice codec; AMBE+2 is used in TDMA. Phase 2 vocoder testing
(TIA-102.BABA) is conditional in this document.

---

## Open Source P25 Implementations

These open source projects implement the P25 CAI and could benefit from conformance testing
against TIA-102.CABA procedures:

| Project | Language | Scope | Notes |
|---------|----------|-------|-------|
| [OP25](https://github.com/boatbod/op25) | Python/C++ | P25 receiver (FDMA + TDMA) | GNU Radio based; implements CAI decoding |
| [DSD (Digital Speech Decoder)](https://github.com/szechyjs/dsd) | C | P25 Phase 1 decoder | Decodes IMBE vocoder; NAC-aware |
| [p25rx](https://github.com/tkuester/p25rx) | Rust | P25 receiver | Rust implementation; relevant for Rust project |
| [MMDVM](https://github.com/g4klx/MMDVM) | C++ | Multi-mode digital voice modem | Includes P25 FDMA repeater (FNE) mode |
| [MMDVM_HS](https://github.com/juribeparada/MMDVM_HS) | C++ | Hotspot implementation | FNE equivalent for hotspot hardware |
| [dvmproject/dvmhost](https://github.com/DVMProject/dvmhost) | C++ | Full P25 repeater/controller | Open source FNE — directly relevant to §2.4 tests |
| [imbe_vocoder](https://github.com/szechyjs/dsd) | C | IMBE vocoder | Referenced in vocoder conformance context |

---

## Test Configuration Quick Reference

### Default Test Parameters (from §1 Base Configuration)

| Parameter | Value |
|-----------|-------|
| Test NAC | $293 |
| Alternate NAC | $300 |
| Any-NAC (as-received TX) | $F7F |
| Any-NAC (fixed TX NAC) | $F7E |
| Default TGID | 1 |
| SU UID range | 00001–00008 |
| Direct channel 1 | 293 1 (NAC $293) |
| Direct channel 2 | 293 2 (NAC $293) |
| Rejection channel | 300 1 (NAC $300) |
| Default encryption key (group 1) | $0001 |
| Alternate encryption key (group 2) | $0002 |

### SU Count Requirements

| Assessment Type | SUs Required |
|----------------|--------------|
| Single model against single FNE | SU1–SU4 (4 SUs minimum) |
| Two-model interoperability (full) | SU1–SU8 (8 SUs) |
| DMC tests | 2 SUs of one model (passed §2.2/§2.3) + DMC |

---

## Relationship to P25 Certification Program

TIA-102.CABA procedures are the basis for P25 Compliance Assessment Program (P25 CAP)
testing conducted by PSCR (Public Safety Communications Research) and DHS-funded test
laboratories. Manufacturers submit equipment to these labs for conformance testing before
claiming P25 interoperability certification. The test cases here map directly to the P25 CAP
Compliance Assessment Bulletin (CAB) for conventional voice operation.

Reference: DHS/SAFECOM P25 CAP — https://www.dhs.gov/safecom/p25

---

## Dependency Map

```
TIA-102.CABA (this document — conformance)
    |
    |-- Tests TIA-102.BAAB (CAI) — NAC, framing, voice channel, squelch
    |-- Tests TIA-102.AAAD / TIA-102.BABA — IMBE vocoder
    |-- Tests TIA-102.BAAC — link control word formats
    |-- Tests TIA-102.BAAD — supplementary data, inhibit, emergency
    |-- Tests TIA-102.AABF — encryption (DES/AES)
    |
    Companion conformance docs:
    |-- TIA-102.CAAA — Conventional data interop tests
    |-- TIA-102.CAAB — Trunking interop tests
    |-- TIA-102.CAAC — ISSI interop tests
```
