# TIA-102.CCAB-B Related Resources

## Document Status

| Field | Value |
|-------|-------|
| Document ID | TIA-102.CCAB-B |
| Title | Two-Slot Time Division Multiple Access Transceiver Performance Recommendations |
| Published | July 2022 |
| Status | Active |
| Supersedes | TIA-102.CCAB-A (March 2014) |
| Developed by | TIA TR-8.1 Equipment Measurement Procedures Subcommittee |
| Classification | MEASUREMENT — defines pass/fail limits; no implementation spec applicable |

---

## Standards Family Position

TIA-102.CCAB-B sits in the P25 Phase 2 (Two-Slot TDMA) equipment performance branch.
The document tree for this area:

```
P25 Phase 2 Overview
└── TSB-102.BBAA          System overview bulletin

Physical/MAC Layer
├── TIA-102.BBAB          Physical layer (H-CPM/H-DQPSK waveform, 6 ksps, 12.5 kHz)
└── TIA-102.BBAC-A        MAC layer (frame structure, burst types, slot assignment)

Equipment Performance (this branch)
├── TIA-102.CCAA-C        Measurement procedures (HOW to test)
└── TIA-102.CCAB-B        Performance limits (WHAT the thresholds are)  ← this document
```

CCAA-C and CCAB-B are inseparable companions: CCAB-B provides the pass/fail numbers
and CCAA-C provides the test setups and procedures. Neither is usable without the other
for compliance testing.

### Revision Lineage

| Revision | Date | Key Addition |
|----------|------|--------------|
| TIA-102.CCAB | October 2011 | Initial publication: baseline receiver/transmitter/environmental limits |
| TIA-102.CCAB-A | March 2014 | Added blocking rejection performance recommendations |
| TIA-102.CCAB-B | July 2022 | Added Broadband SSIR recommendations (700/800/900 MHz) |

---

## Normative References (from document Section 2)

| Ref | Document | Relevance |
|-----|----------|-----------|
| [1] | TIA-102.CCAA-C | Measurement methods companion — normative |
| [2] | MIL-STD-810H | Environmental test methods (temperature, humidity, vibration, shock) |
| [3] | TIA-603-E | Land Mobile FM/PM communications equipment standard |
| [4] | 47 CFR Parts 22, 24, 27, 90 | FCC regulations — supersede where more stringent |
| [5] | TIA-102.BBAB | P25 Phase 2 physical layer |
| [6] | TIA-102.BBAC-A | P25 Phase 2 MAC layer |
| [7] | TIA-102.CCAA | Earlier measurement procedures (superseded by CCAA-C) |
| [8] | TSB-102.BBAA | P25 Phase 2 system overview bulletin |

---

## Practical Context for P25 Implementers

### What CCAB-B Governs

CCAB-B limits apply to any subscriber unit (portable or mobile) or base station
transceiver operating on P25 Phase 2 Two-Slot TDMA below 1 GHz. Compliance is
verified under the P25 Compliance Assessment Program (CAP), administered jointly by
DHS and the P25 Technology Interest Group (PTIG).

Equipment sold for public safety use must pass CCAB-B limits (and CCAA-C test
procedures) to be listed on the P25 CAP compliance database. Agencies referencing
P25 CAP in their procurement specifications can use this database to vet equipment.

### Class A vs. Class B

Class A limits are more stringent — better sensitivity, higher rejection ratios — and
are appropriate for:
- Subscriber units expected to operate in high-RF-density urban environments
- Infrastructure receivers at shared tower sites
- Trunked system subscriber units at dense talk sites with multiple co-located TX

Class B limits are comparable to analog 12.5 kHz FM performance and are a lower-cost
interoperability floor. Agencies may specify Class A in procurement if their operational
environment warrants it.

### SSIR and Multi-Band Infrastructure

The major -B revision addition (Broadband SSIR) addresses a real failure mode in P25
deployments: subscriber units at simulcast talk sites receive high-power transmissions
from multiple bands simultaneously (e.g., 700 + 800 MHz from co-located infrastructure).
Without adequate SSIR, the broadband front-end generates intermodulation products that
desensitize or block the desired receive channel. The SSIR limits in CCAB-B were added
after field reports of this problem in deployed systems.

### Relationship to FCC Part 90

47 CFR Part 90.210 and related sections establish minimum RF performance standards for
land mobile equipment. Where Part 90 requirements are more stringent than CCAB-B Class A
or B limits, the FCC requirements control. CCAB-B explicitly notes this throughout
(e.g., Section 6.4.5 on conducted spurious emissions).

### No Phase 3 Spec Applicable

CCAB-B is a measurement/performance-limits document. It contains no algorithms,
protocol state machines, encoding schemes, or packet formats that would be implemented
in software or firmware. A Rust implementation of a P25 Phase 2 radio stack would
implement TIA-102.BBAB (waveform) and TIA-102.BBAC-A (MAC), not CCAB-B. CCAB-B is
relevant at integration/test time: use it to define acceptance test criteria for
transmitter and receiver subsystems.

---

## Open-Source P25 Implementations

These projects implement P25 Phase 2 and are relevant context for the waveform and
MAC layers that CCAB-B tests:

- **op25** (GNU Radio-based P25 receiver/transmitter)
  - Implements Phase 2 TDMA receive including H-DQPSK demodulation
  - Relevant to understanding what receiver characteristics CCAB-B is testing

- **dsd** / **dsd-fme** (Digital Speech Decoder)
  - Phase 2 TDMA decode support
  - Limited to receive path; no transmit implementation

- **p25rx** and related SDR tools
  - Various RTL-SDR/HackRF-based P25 Phase 2 monitoring tools

For a Rust P25 stack, the waveform spec (TIA-102.BBAB) and MAC spec (TIA-102.BBAC-A)
are the primary implementation targets. CCAB-B defines the performance envelope that
a compliant implementation should fall within when measured in hardware.

---

## P25 Program Resources

- **TIA TR-8 Standards**: Available through the TIA Standards Store
  (tiaonline.org) — active members have access; published documents available for purchase
- **P25 CAP Compliance Database**: Maintained by DHS/CISA; lists equipment tested
  against P25 standards including CCAB-B limits
- **PTIG (P25 Technology Interest Group)**: ptig.org — publishes implementation
  guidance, liaison documents, and CAP program information
- **APCO International**: apco911.org — standards body co-developing P25; publishes
  Project 25 system user guidelines

---

## Key Parameters Quick Reference

| Parameter | Class A | Class B | Units |
|-----------|---------|---------|-------|
| Reference Sensitivity (H-CPM) | -116 | -113 | dBm |
| Co-Channel Rejection | +3 | +6 | dB |
| Adjacent Channel Rejection | 60 | 50 | dB |
| Intermodulation Rejection | 75 | 70 | dB |
| Blocking Rejection (in-band) | 90 | 80 | dB |
| Blocking Rejection (out-of-band) | 90 | 90 | dB |
| Spurious Response Rejection | 90 | 80 | dB |
| Max Portable TX Power | 3 | 3 | W |
| Max Mobile TX Power | 35 | 35 | W |
| Min TX Power | 1 | 1 | W |
| Frequency Error | ±200 | ±200 | Hz |
| Channel Bandwidth (99% power, H-CPM) | 11.25 | 11.25 | kHz |
| Channel Bandwidth (99% power, H-DQPSK) | 12.0 | 12.0 | kHz |
| Operating Frequency Range | ≤1 GHz | ≤1 GHz | — |
