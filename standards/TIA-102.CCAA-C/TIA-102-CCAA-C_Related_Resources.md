# TIA-102.CCAA-C Related Resources
## Two-Slot TDMA Transceiver Measurement Methods

---

## Document Status

| Field | Value |
|---|---|
| Document ID | TIA-102.CCAA-C |
| Full Title | Two-Slot Time Division Multiple Access Transceiver Measurement Methods |
| Revision | C (current) |
| Date | July 2022 |
| Publisher | Telecommunications Industry Association (TIA) |
| Committee | TR-8 Mobile and Personal Private Radio Standards |
| Status | Active standard |
| Purchase | https://store.accuristech.com/tia |
| TIA Standards | https://standards.tiaonline.org |

---

## TIA-102 Standards Family Context

TIA-102.CCAA-C belongs to the **Phase 2** (Two-Slot TDMA) sub-family of TIA-102 (Project 25). The TIA-102 numbering convention: the third letter group indicates the technology variant.

### Standards Lineage for This Document

```
TIA-102 Project 25 Family
├── Phase 1 (FDMA, C4FM)
│   ├── TIA-102.AAAA  - Common Air Interface (CAI)
│   ├── TIA-102.AAAD  - CAI Performance Specifications
│   └── TIA-102.AABD  - Transceiver Measurement Methods (Phase 1 equivalent)
│
└── Phase 2 (Two-Slot TDMA, H-CPM/H-DQPSK)
    ├── TIA-102.CAAA  - Two-Slot TDMA Common Air Interface
    │                  (defines modulation, frame structure, burst format)
    ├── TIA-102.CAAB  - Two-Slot TDMA Physical Layer Specification
    │                  (burst encoding, interleaving, superframe construction,
    │                   BER counting, scrambling, bit ordering - Annex E)
    ├── TIA-102.CAAC  - Two-Slot TDMA Transceiver Performance Specifications
    │                  (the pass/fail limits for all measurements defined here)
    ├── TIA-102.CCAA-C - Two-Slot TDMA Transceiver Measurement Methods  ← THIS DOCUMENT
    │                   (HOW to measure what CAAC specifies the limits for)
    └── TIA-102.CCAB  - Two-Slot TDMA Base Station Subsystem Measurement Methods
```

### Supporting Standards Referenced by CCAA-C

| Reference | Document | Relationship |
|---|---|---|
| [2] | TIA-102.CAAB | Burst/superframe structure; bit ordering (Annex E); ISCH format; vocoder frame construction (Clause 5); SACCH format (Clause 4.3) |
| [17] | TIA-102.BACA (or equivalent) | MAC layer; PDU formats; MAC_ACTIVE opcode; Group Voice Channel User messages; Service Options |
| [18] | TIA-102.AABF (or equivalent) | Scrambling sequence generator algorithm; WACN_ID, SYSTEM_ID, COLOR_CODE |
| [4] | TIA-102.BABA | IMBE vocoder; Service Options definitions |
| [6] | TIA-102.AABD | Phase 1 transceiver measurement methods (trunking timing tests referenced by Section 2.3) |
| [7] | ANSI/TIA-603 | Land mobile FM/PM communications equipment (audio, environmental) |

---

## Technical Context: What This Standard Tests

### The Measurement Architecture

TIA-102.CCAA-C forms one leg of a three-document tripod:

1. **TIA-102.CAAA** — Defines the air interface (modulation, frame structure, logical channels). This is "what the signal looks like."
2. **TIA-102.CAAC** — Defines the performance specifications ("the radio must achieve X"). This is "what the limits are."
3. **TIA-102.CCAA-C** — Defines how to set up the test and measure. This is "how to verify the limits."

Any P25 Phase 2 radio conformance test requires all three documents.

### H-CPM vs. H-DQPSK: Measurement Implications

The two Phase 2 modulations behave differently in tests:

| Aspect | H-CPM (Inbound) | H-DQPSK (Outbound) |
|---|---|---|
| Frequency deviation | Significant; required measurement | Dependent on symbol rate/filter; not directly measured |
| ISI from transmit filter | Significant (non-zero response at ±n symbol times) | Negligible (RECT filter approximation) |
| Modulation fidelity reference | Uses (reference − measured) with ideal H-CPM pulse filter | Uses RECT filter reference (no ISI correction needed) |
| Symbol rate accuracy | Measured via FM demodulator (ppm_error = (MBR_Hz/1500 − 1)×10⁶) | Verified implicitly by modulation fidelity |
| TDMA-specific tests | Off-slot power, power envelope, peak ACPR, time alignment | Not applicable (base station continuous TX) |

### Unique Phase 2 Measurements (Not in Phase 1 Standards)

These subclauses are specific to Two-Slot TDMA and have no Phase 1 equivalent:
- **2.2.12** Frequency Deviation for H-CPM
- **2.2.13** Modulation Fidelity (combined H-CPM/H-DQPSK)
- **2.2.14** Symbol Rate Accuracy
- **2.2.15** H-CPM Peak Adjacent Channel Power Ratio (during ramp)
- **2.2.16** H-CPM Off Slot Power (TDMA switching suppression)
- **2.2.17** H-CPM Logical Channel Power Envelope (ramp shape)
- **2.2.18** H-CPM Logical Channel Time Alignment (inbound burst timing)

### BER Counting: How P25 Phase 2 Measures Receiver Quality

Unlike Phase 1 (which counts all bits uniformly), Phase 2 BER counting is structured around the superframe:

**Outbound BER (per superframe per logical channel):**
- ISCH (Inter-Slot Signaling Channel) bits: 12 per superframe
- Voice channel (VCH) bits: 5 bursts
- SACCH bits: 2 per superframe
- Total countable bits per superframe: 2720 bits/SF/LCH

**Inbound BER (per ultraframe per logical channel):**
- 4 × 5 VCH bursts
- 3 × SACCH bursts
- Total countable bits per ultraframe: 7252 bits/UF/LCH

The faded sensitivity test requires a minimum 10-second measurement interval to adequately average over 30 Hz Rayleigh fading statistics.

---

## Implementation Notes for Open Source P25

### Time Alignment Implementation

Section 2.2.18 provides a complete autocorrelation-based time alignment measurement algorithm that is directly implementable in software. Key parameters:

- Capture rate: 192 kHz complex I/Q
- Autocorrelate with 609-sample (outbound) or 641-sample (inbound) reference waveform from Annex A (Tables A.6 / A.7)
- Outbound sync peak: located at boundary between timeslots 1 and 2
- Expected timing offsets: LCH 0 = 242.6667 ms, LCH 1 = 272.6667 ms from OB frame boundary

### Test Pattern Construction

All test patterns are fully specified in Annex A (normative). The assembly order is:
1. Start with voice information vectors (vxu_0..vxu_3) from Table A.3 or A.4
2. Apply Golay coding to get vxu_0_golay, vxu_1_golay
3. Assemble 18 coded voice frames (v1c..v18c) per superframe
4. Insert ISCH vectors (Table A.2) per TIA-102.CAAB Clause 5.1
5. Insert SACCH vectors (Table A.5) per TIA-102.CAAB Clause 5.6
6. Insert Data Unit ID, encryption fields (algid, kid, mi, parity_ess)
7. Apply scrambling per Annex A.1 vectors and [18] algorithm
8. Modulate per TIA-102.CAAB bit ordering (Annex E of CAAB)

Note: Calibration patterns are STTP patterns with every 20th bit inverted.

### Symbol Rate Formula

```
ppm_error = (MBR_Hz / 1500 - 1) * 1e6
```

Where MBR_Hz is the measured baseband rate in Hz from an FM demodulator. The divisor 1500 is because the FM demodulator output represents the symbol stream at 1500 Hz repetition rate for the HDTP pattern (which causes 4× symbol transitions per cycle at 6000 sym/sec, or 1500 Hz observed from the demodulator output).

### Modulation Fidelity Signal Model

```rust
// Pseudo-code for modulation fidelity calculation
for k in MIN..=MAX {
    let e_k = z_k / C1 - C0 / C1 - (s_k + i_k);
    sum_sq_error += e_k * e_k;
}
let rms_error_pct = (sum_sq_error / N).sqrt() / nominal_deviation * 100.0;
```

C0 and C1 are chosen to minimize sum_sq_error (least-squares fit). Symbol clock phase is also optimized.

---

## Related Open Source Projects

### Software Defined Radio Tools

| Project | Relevance | URL/Notes |
|---|---|---|
| op25 | P25 Phase 1 and Phase 2 receiver; GNU Radio based | https://github.com/boatbod/op25 |
| gr-p25rx | GNU Radio P25 Phase 2 demodulator blocks | Search PyPI / GitHub |
| liquid-dsp | DSP primitives (modems, filters) useful for P25 implementation | https://github.com/jgaeddert/liquid-dsp |
| GNU Radio | SDR framework; H-CPM/H-DQPSK modulator/demodulator blocks available | https://www.gnuradio.org |
| codec2 | Open vocoder work; relevant to IMBE/AMBE comparison | https://github.com/drowe67/codec2 |

### Protocol Analyzers

| Tool | Notes |
|---|---|
| DSD+ (Discriminator Software Decoder) | Windows P25 Phase 2 decoder |
| Unitrunker | P25 trunking site follower |
| SDR# + P25 plugin | Real-time P25 decoding |

### Test Equipment Software

The time alignment measurement (Section 2.2.18) is well-suited to implementation using:
- GNU Radio with USRP or similar SDR at ≥192 kHz sample rate
- NumPy/SciPy for autocorrelation of Annex A.6/A.7 reference waveforms
- The 641 I/Q samples of Table A.7 can be used directly as a matched filter

---

## P25 Phase 2 Standards Procurement

TIA standards are available for purchase from:
- TIA Store: https://store.accuristech.com/tia
- TIA Online: https://standards.tiaonline.org
- IHS Markit (now S&P Global): distributor

The P25 standards suite (TIA-102 family) requires several documents for a complete implementation. Minimum recommended set for Phase 2 transceiver work:
- TIA-102.CAAA (CAI) — air interface definition
- TIA-102.CAAB (physical layer) — burst/frame encoding
- TIA-102.CAAC (performance specs) — the limits
- TIA-102.CCAA-C (measurement methods) — this document

---

## FCC Regulatory Context

P25 Phase 2 equipment sold in the US must comply with:
- FCC Part 90 (Private Land Mobile Radio Services)
- FCC Part 15 (unintentional radiators)
- The TIA-102.CCAA-C measurement methods are used by FCC-recognized TCBs (Telecommunications Certification Bodies) to verify Part 90 compliance

Public safety radios operating on FirstNet/700 MHz spectrum must meet P25 CAP (Compliance Assessment Program) requirements, which reference this measurement standard.

---

## Document Family Tree (TIA-102 Phase 2 TDMA)

```
TIA-102.CAxx  = Two-Slot TDMA Technical Standards
  TIA-102.CAAA  = Common Air Interface
  TIA-102.CAAB  = Physical Layer
  TIA-102.CAAC  = Performance Specifications (limits)

TIA-102.CCxx  = Two-Slot TDMA Conformance / Measurement
  TIA-102.CCAA-C  = Transceiver Measurement Methods    ← this document
  TIA-102.CCAB    = Base Station Subsystem Measurement Methods

TIA-102.BAxx  = Vocoder Standards (apply to both Phase 1 and 2)
  TIA-102.BABA-A  = IMBE Vocoder (full-rate and half-rate)
  TIA-102.BABB    = Vocoder MOS Conformance Test
  TIA-102.BABG    = Enhanced Vocoder Methods of Measurement

TIA-102.AAxx  = Phase 1 FDMA Standards (shared infrastructure)
  TIA-102.AAAA    = Phase 1 Common Air Interface
  TIA-102.AABD    = Phase 1 Measurement Methods
  (trunking control channel shared with Phase 2)
```

---

## Notes on Using This Document with Automated Test Systems

When implementing automated test per this standard, note:

1. **Modulation fidelity tests** require a modulation fidelity analyzer with at least 164-symbol capture capability and TDMA trigger input (Section 2.2.13).

2. **Time alignment measurement** (2.2.18) is the most complex test, requiring simultaneous control channel signal generation and voice channel capture with autocorrelation processing. The reference waveforms in Annex A.6/A.7 are the key — they must be implemented at exactly 192 kHz.

3. **BER measurement** requires the TDMA test signal generator to produce complete superframes including ISCH and SACCH bursts per Annex A — not just raw voice bits. The full Annex A vector table must be implemented correctly.

4. **Faded sensitivity** (2.1.5) measurements at 30 Hz Doppler require long averaging (≥10 seconds) due to the slow fading rate. The faded channel simulator must be calibrated for CPDF F(P) = 1 − exp(−P/P_ave).

5. **Off-slot power and power envelope** (2.2.16, 2.2.17) measurements must use the Symmetrical Slot Test Mode (not the standard slot mode) to enable proper timing reference for the spectrum analyzer trigger.
