# TIA-102.CAAA-F — Related Resources and Context

## Document Identity

| Field           | Value                                                          |
|-----------------|----------------------------------------------------------------|
| Document Number | TIA-102.CAAA-F                                                 |
| Title           | Digital C4FM/CQPSK Transceiver Measurement Methods            |
| Revision        | F                                                              |
| Date            | September 14, 2021                                             |
| Status          | Current (active standard)                                      |
| Supersedes      | TIA-102.CAAA-E                                                 |
| Classification  | MEASUREMENT                                                    |
| Committee       | TIA TR-8 (Mobile and Personal Private Radio Standards)         |

---

## Role in the TIA-102 Standards Architecture

TIA-102.CAAA-F occupies a specific niche in the P25 standards family: it is the
**test methods companion** to the performance specifications. The relationship is:

```
TIA-102.CAAB-B  ←→  TIA-102.CAAA-F
(What the limits ARE)    (How to MEASURE compliance)
```

Every parameter in TIA-102.CAAB-B (sensitivity, ACR, ACPR, attack times, etc.)
has a corresponding measurement procedure defined here. The two documents are
inseparable for compliance testing.

---

## Standards Lineage (Revision History)

| Revision | Notes                                              |
|----------|----------------------------------------------------|
| CAAA     | Original version                                   |
| CAAA-A   | First revision                                     |
| CAAA-B   | —                                                  |
| CAAA-C   | —                                                  |
| CAAA-D   | —                                                  |
| CAAA-E   | Preceding revision (still referenced as [R1])      |
| **CAAA-F** | **Current (2021). Added broadband LTE IMR test, updated radiated spurious methods, GNSS-band measurement** |

Key changes in revision F:
- Section 2.1.14: Added Broadband Strong Signal Intermodulation Rejection measurement
  for LTE band interference (700/800/900 MHz spectrum neighbors)
- Section 2.2.6: Updated radiated spurious EIRP measurement methods including
  GNSS band (1.559–1.610 GHz) test for 700 MHz band equipment
- Section 2.2.6.4: Added calculated EIRP method for external-antenna equipment
- Alignment with TIA-102.CAAB-B current revision

---

## Normative References (Documents This Standard Depends On)

| Ref  | Document           | Title                                          | Role                                    |
|------|--------------------|------------------------------------------------|-----------------------------------------|
| [R1] | TIA-102.CAAA-E     | Previous revision of this document             | Historical predecessor                  |
| [R2] | TIA-102.AABB-E     | P25 FDMA Common Air Interface                  | Defines the air interface being tested; receiver/transmitter specs for Phase 1 |
| [R3] | TIA-102.BAAB-B     | P25 TDMA Common Air Interface                  | TDMA CAI; defines Phase 2 air interface parameters |
| [R4] | TIA-102.BBAB       | P25 TDMA Physical Layer                        | Phase 2 physical layer (TDMA framing, modulation) |
| [R5] | TIA-102.CAAB-B     | Digital C4FM/CQPSK Transceiver Performance Specs | The companion limits document; defines pass/fail thresholds |

---

## Documents That Reference TIA-102.CAAA-F

Any TIA-102 conformance testing document or P25 compliance test report will
cite CAAA-F as the measurement authority. It is referenced by:

- **TIA-102.CAAB-B** (performance specs) — for all measurement methods
- **P25 CAP (Compliance Assessment Program)** documents — testing labs use CAAA-F
  as the normative test procedure
- Radio manufacturer test reports for P25 certification

---

## Related TIA-102 Documents by Category

### Phase 1 (FDMA) Foundation
- **TIA-102.AABB-E** — FDMA Common Air Interface (the primary CAI spec; Section 9.6
  defines the integrate-and-dump filter referenced in modulation fidelity)
- **TIA-102.AABC** — FDMA Subscriber Unit Sub-system Interface
- **TIA-102.AABF** — FDMA Data Sub-system Interface

### Phase 2 (TDMA) Foundation
- **TIA-102.BAAB-B** — TDMA Common Air Interface
- **TIA-102.BBAB** — TDMA Physical Layer
- **TIA-102.BAAC** — TDMA Subscriber Unit Sub-system Interface

### Performance/Measurement Family
- **TIA-102.CAAA-F** — THIS DOCUMENT (measurement methods)
- **TIA-102.CAAB-B** — Transceiver Performance Specifications (limits)
- **TIA-102.CAAC** — Vocoder Performance Requirements

### Trunking
- **TIA-102.AABD-D** — FDMA Trunking Control Channel Formats (defines ISP/OSP/TSBK
  structure that the Annex A software implements)
- **TIA-102.AABF** — Trunking system architecture

### Security
- **TIA-102.AACE** — Message Encryption (AES, DES algorithms)
- **TIA-102.AACF** — Digital Radio Over-the-Air Rekeying (OTAR)

---

## Open-Source P25 Implementations

The measurement methods and reference software in TIA-102.CAAA-F are directly
relevant to these open-source projects:

### Software-Defined Radio Implementations
- **OP25** (https://github.com/boatbod/op25) — GNU Radio-based P25 Phase 1 & 2
  receiver. Implements the same C4FM demodulation, BER counting, and frame sync
  detection procedures described in this document.
- **gr-p25** — GNU Radio P25 blocks; implements C4FM/CQPSK demodulation.
- **DSD (Digital Speech Decoder)** — Decodes P25, DMR, and other digital voice
  protocols. P25 vocoder and frame structure matches what is tested here.

### Decoder Frameworks
- **Unitrunker** — Windows trunking decoder; interprets TSBK packets (the same
  packets built by the Annex A software).
- **SDRTrunk** — Java-based P25 decoder; implements the full trunking control
  channel decoder including ISP/OSP parsing.
- **trunk-recorder** — GNU Radio-based P25 recorder.

### Test Vector Generation
The Annex A software (bch.c, crc.c, trellis.c, buildtsb.c, etc.) is the normative
reference implementation for generating P25 trunking signaling test vectors. An
open-source Rust implementation of this chain would be:
- `bch_64_encode()` — BCH(64,16,23) encoder
- `crc_ccitt()`, `crc_9()`, `crc_32()` — three CRC variants
- `trellis_3_4_encode()`, `trellis_1_2_encode()` — trellis encoders
- Frame packing with status symbol insertion (every 36 dibits)

### Compliance Testing Tools
- **P25RX** (various) — BER test tools that implement the receiver measurement
  procedures from this document
- **P25 Test Suite** — Lab automation software implementing Section 2.1 and 2.2
  measurement sequences

---

## Key Technical Values for Implementation

These values from TIA-102.CAAA-F are essential for any P25 implementation:

| Parameter              | Value                           | Source      |
|------------------------|---------------------------------|-------------|
| Standard BER           | 5% (1/20 bits)                  | §1.4.6      |
| Rayleigh Doppler (std) | 30 Hz                           | §1.5.2      |
| C4FM high deviation    | ±1.8 kHz                        | §2.2.15, Table 10 |
| C4FM low deviation     | ±0.6 kHz                        | §2.2.15, Table 10 |
| Symbol rate            | 4800 sym/s                      | §2.2.17     |
| Channel spacing        | 12.5 kHz                        | §2.1.3      |
| Frame sync word        | +3,+3,+3,+3,+3,-3,+3,+3,-3,+3,+3,-3,-3,-3,+3,-3,+3,-3,+3,-3,-3,-3,-3,-3 | §2.3.1.2 |
| Status symbol interval | Every 36 dibits (i%36==35)      | Annex A     |
| IDLE status symbol     | 0x3                             | Annex A     |
| BCH code               | (64,16,23) primitive            | Annex A     |
| CRC-CCITT polynomial   | G_16 = x^12+x^5+1 = 0x1021     | Annex A     |
| CRC-9 polynomial       | G_9 = x^6+x^4+x^3+1 = 0x49    | Annex A     |
| CRC-32 polynomial      | 0x04c11db7                      | Annex A     |
| NID structure          | BCH over (NAC<<4 | DUID)        | Annex A     |
| Microslot structure    | 150 microslots × 6 integers of 12 bits | Annex A |
| Frame dibits           | 24 FS + 32 NID + 98/block       | Annex A     |
| Modulation fidelity capture | ≥144-bit segment           | §2.2.16.2   |
| Symbol rate ppm formula | (MBR_Hz/1200 − 1) × 10^6      | §2.2.17     |

---

## Relationship to P25 Compliance Assessment Program (CAP)

The P25 CAP (administered by DHS/SAFECOM) requires that equipment be tested by
an accredited laboratory using the procedures in TIA-102.CAAA-F. CAP test reports
cite specific section numbers from this document. Equipment that passes CAP testing
in all required categories listed in TIA-102.CAAB-B is listed on the P25 CAP
Approved Products List (APL).

Public safety agencies (police, fire, EMS) frequently require CAP-listed equipment
in procurement specifications.

---

## Processor Note for This Project

TIA-102.CAAA-F is foundational for understanding how to validate a P25 implementation:
- The standard BER test (§2.1.1–2.1.2) defines what "correct demodulation" means
- The modulation fidelity methods (§2.2.16) define the quality metrics for any
  C4FM/CQPSK transmitter implementation
- The Annex A code is the normative reference for P25 trunking frame generation;
  porting it to Rust would produce a verified TSBK/data frame generator
- The timing measurements (§2.2.12–2.2.20, §2.3) define latency constraints that
  must be met by any infrastructure implementation

Phase 3 implementation specs are not warranted for this MEASUREMENT document,
but the Annex A software and the key technical values table above directly support
spec-driven development of P25 radio implementations.
