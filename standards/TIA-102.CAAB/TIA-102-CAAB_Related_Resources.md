# TIA-102.CAAB-E: Related Resources and Context

Document: TIA-102.CAAB-E — Land Mobile Radio Transceiver Performance Recommendations,
Digital Radio Technology, C4FM/CQPSK Modulation  
Published: September 2021  
Researched: 2026-04-12

---

## Status

**Active.** TIA-102.CAAB-E (September 2021) is the current revision. The document
supersedes TIA-102.CAAB-D (February 2013). A subsequent revision,
TIA-102.CAAB-E-2, was published December 2024 — this later version exists in the
same directory and likely incorporates additional clarifications or limit updates;
it has not been processed here. An erratum (TR-8.1 TIA-102.CAAB-E-1) was
published November 2022.

The CAAB-E version introduced Broadband Strong Signal Intermodulation Rejection
(SSIR) limits as its primary technical addition over CAAB-D, reflecting real-world
interference problems in densely populated spectrum environments, particularly
the 700 MHz band.

No evidence of pending retirement or absorption into another document. The CAAB
series is expected to continue tracking FCC rules updates and technology evolution.

---

## Standards Family

This document sits in the TIA-102 "CA" sub-family: FDMA transceiver measurement
and performance standards.

```
TIA-102 (Project 25 Suite)
├── BAAA — FDMA Common Air Interface (physical + data link)
│   └── Referenced normatively as [1] in this document
├── CA — Transceiver Measurement & Performance (FDMA/C4FM/CQPSK)
│   ├── CAAA — C4FM or CQPSK Digital Transceiver Methods of Measurement
│   │   └── Referenced normatively as [2] — defines HOW to measure
│   └── CAAB — C4FM/CQPSK Performance Recommendations  ← THIS DOCUMENT
│       └── Defines WHAT the pass/fail limits are
├── CCAA — TDMA Digital Transceiver Methods of Measurement
├── CCAB — TDMA Performance Recommendations
│       └── Parallel document for TDMA/QPSK equipment
└── BCAD/BCAE/BCAF — Conformance testing suites
```

Companion documents in the same TR-8.1 scope:
- **TIA-102.CAAA-F** — The measurement methods document paired with this one
- **TIA-603-E** — Land Mobile FM or PM Communications Equipment (analog baseline;
  Class B performance is calibrated to match this legacy analog standard)
- **MIL-STD-810E** — Environmental test methods (normative reference for Class A
  "other environmental" tests in section 3.4.10)
- **MIL-STD-167-1(SHIPS)** — Marine vibration (normative for marine installs)

---

## Practical Context

### Role in P25 Certification

P25 equipment manufacturers must demonstrate compliance with this document (and
its companion CAAA methods) to receive APCO P25 Compliance Assessment Program
(CAP) certification. The CAP was established by DHS SAFECOM and is administered
by the PSCR (Public Safety Communications Research) program at NIST. Many state
and federal procurement specifications require P25 CAP certification.

### FCC Type Acceptance

For equipment operating in the 700 MHz band (Bands 13 and 14), FCC Parts 27
and 90 incorporate specific limits that are referenced normatively in this
document. The document explicitly cites FCC rule sections throughout (§27.50,
§27.53, §90.210, §90.541, §90.543) and states that if the FCC updates those
rules, the FCC rule supersedes this document's citation until CAAB is revised.

### Class A vs. Class B in Practice

Class B performance matches legacy 12.5 kHz analog radios. Class A is required
for more demanding applications:
- Public safety deployments where interference rejection is critical
- 700 MHz band equipment (tighter spectrum, more challenging receiver demands)
- Equipment certified against MIL-STD-810E environmental requirements (Class A
  mandatory for this additional suite of tests)

Most new P25 public safety equipment targets Class A performance.

### Simulcast Systems

The CAAB-C revision (2010) added limits for "standard simulcast" modulation,
and these propagate through CAAB-D and CAAB-E. Simulcast operation increases
delay spread requirements (80 µs vs. 50 µs for C4FM) and has special RFSS
timing limits (RFSS idle-to-busy ≤30 ms, RFSS throughput ≤100 ms for simple
repeaters). Time-to-grant limits in section 3.3.4 show significantly higher
values for simulcast systems (up to 524 ms) versus non-simulcast (374 ms),
reflecting the additional RFSS processing overhead.

### Strong Signal Intermodulation (SSIR) — New in CAAB-E

The SSIR limits added in CAAB-E address real-world degradation of receiver
performance when strong nearby interferers are present — a growing problem as
700 MHz public safety networks coexist with LTE (FirstNet) equipment in adjacent
spectrum. The limits are frequency-band-specific because the interference
environment differs:
- 800 MHz/700 MHz Band 14: moderate limits (55-75 dB range)
- 700 MHz Band 13: tighter limits (70-90 dB) reflecting this band's proximity
  to FirstNet deployment
- 900 MHz: looser limits (25-65 dB) reflecting more relaxed spectrum environment

SSIR is tested with unwanted signal powers from -40 dBm down to -10 dBm; limits
relax as the interferer gets stronger, acknowledging physical receiver constraints.

---

## Key Online Resources

### Standards Bodies and Official Sources

- **TIA Standards online store**: https://standards.globalspec.com (search TIA-102.CAAB)
  Purchase required; TIA members may access directly via TIAonline.org

- **APCO Project 25 Technology Interest Group (PTIG)**:
  https://www.apcointl.org/technology/project-25/
  Maintains overview documents describing where CAAB fits in the P25 ecosystem.

- **DHS SAFECOM P25 Compliance Assessment Program**:
  https://www.cisa.gov/safecom/p25-cap
  Lists certified equipment. Equipment listed under P25 CAP has been tested
  against CAAB limits using CAAA methods.

- **NIST PSCR (Public Safety Communications Research)**:
  https://www.nist.gov/ctl/pscr
  Publishes test reports and technical notes on P25 transceiver performance.
  Some reports explicitly reference CAAB performance limits in their analysis.

- **FCC Knowledge Database (KDB)**:
  https://www.fcc.gov/kdb
  Guidance on FCC rule citations used in this document (Parts 27, 90, 15).

### Technical References

- **NPSTC (National Public Safety Telecommunications Council)**:
  https://www.npstc.org
  Policy and standards guidance for P25 adoption; references CAAB indirectly
  through certification requirements.

- **PSCR P25 Test Artifacts**:
  https://www.pscr.gov
  Technical test reports for P25 equipment; uses CAAA methods and CAAB limits.

---

## Open-Source Implementations

This document defines performance limits, not protocols or algorithms. There
is no "implementation" to open-source. However, the following projects test
or validate P25 equipment performance in ways that relate to the limits here:

- **OP25** (https://github.com/boatbod/op25) — Open-source P25 receiver.
  Receiver sensitivity and BER performance of the SDR front-end can be benchmarked
  against CAAB Class B reference sensitivity limits (-113 dBm) in practice.
  Co-channel and adjacent channel rejection characteristics are observable through
  OP25's C4FM demodulator performance in real-world deployments.

- **SDRTrunk** (https://github.com/DSheirer/sdrtrunk) — P25 Phase 1 and Phase 2
  decoder. SDR-based P25 receivers inherently demonstrate CAAB limits in practice;
  the BER floor, delay spread capability, and adjacent channel rejection are
  observable performance characteristics, though SDRTrunk is not certified to CAAB.

- **JMBE** (https://github.com/DSheirer/jmbe) — IMBE/AMBE vocoder library.
  Vocoder throughput delay contributes to the receiver throughput delay budget;
  JMBE's decode latency is a component of the 125 ms receiver throughput delay
  budget defined in section 3.1.18.

**Note:** No open-source project implements or certifies against CAAB limits
directly, since certification requires calibrated lab equipment and formal
test procedures per TIA-102.CAAA. SDR-based receivers are substantially below
Class B sensitivity limits due to noise figure constraints of consumer SDR hardware.

---

## Standards Lineage

```
TIA-603-E (Analog FM Land Mobile)
    └── Defines Class B performance baseline (CAAB Class B ≈ TIA-603 12.5 kHz)

TIA-102.BAAA-B (FDMA Common Air Interface)
    └── Defines C4FM/CQPSK physical layer; CAAB limits protect this interface

TIA-102.CAAA-F (Measurement Methods)
    └── Companion to CAAB; defines test procedures for all limits herein

TIA-102.CAAB (Nov 2000) — Initial publication
    └── TIA-102.CAAB-A (Sep 2002) — Added 700 MHz limits
        └── ANSI/TIA-102.CAAB-B (Jul 2004) — FCC power line limits
            └── ANSI/TIA-102.CAAB-C (Jan 2010) — Simulcast modulation limits
                └── ANSI/TIA-102.CAAB-D (Feb 2013) — Blocking rejection,
                    |                                   offset ACR revision
                    └── TIA-102.CAAB-E (Jun/Sep 2021) ← CURRENT PROCESSED VERSION
                        └── TR-8.1 TIA-102.CAAB-E-1 (Nov 2022) — Erratum
                            └── TIA-102.CAAB-E-2 (Dec 2024) — Latest revision

Parallel track (TDMA equipment):
TIA-102.CCAA — TDMA measurement methods
TIA-102.CCAB — TDMA performance recommendations
    └── Parallel to CAAA/CAAB but for TDMA/H-DQPSK modulation
```

---

## Notes for Processing

- The -E-2 revision (December 2024) is present in the same directory. It may
  contain updated SSIR limits or additional FCC rule references. A follow-up
  processing pass of -E-2 is recommended to capture any changes.
- The erratum TR-8.1_TIA-102.CAAB-E-1 (November 2022) is also present in the
  directory and should be reviewed for corrections to the -E tables.
- No Phase 3 implementation spec is warranted for this document (MEASUREMENT
  classification). All limits are defined here as pass/fail thresholds only;
  implementation of measurement procedures is in TIA-102.CAAA.
