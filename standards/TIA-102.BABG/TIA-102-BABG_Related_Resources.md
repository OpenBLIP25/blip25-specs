# TIA-102.BABG Related Resources and Context

## Document Status

**Active.** Published March 2010. Approved by the Project 25 Steering Committee as part of the
Project 25 Standard. Developed under the cognizance of TIA TR-8 Mobile and Personal Private
Radio Standards, TR-8.4 Subcommittee on Vocoders, with inputs from the TIA Project 25
Interface Committee (APIC) and the APIC Vocoder Task Group. Originated from Standards Proposal
No. 3-0379.

This is a TIA Standard but is **not ANSI-accredited** (no "-X" suffix in the document baseline
identifier). The document went through five draft revisions (v0.0 through v0.32, September
2008 to October 2009) before final publication.

The document can be purchased from TIA's standards store:
https://store.accuristech.com/tia

---

## Standards Family Context

This document sits within the **TIA-102 vocoder measurement** sub-family, defining the quality
bar for the "enhanced" MBE vocoders used in P25 Phase 1 (full-rate) and Phase 2 (half-rate)
systems.

### Document Hierarchy and Lineage

```
TIA-102 (Project 25 Standard)
└── Vocoder Suite
    ├── TIA-102.BABA-A     P25 Vocoder Description (Full-Rate and Half-Rate)
    │                       [Normative reference 4.1.1 — BABG's foundational spec]
    │
    │   1999-era testing (original IMBE baseline):
    │
    ├── TIA-102.BABC       P25 Vocoder Reference Test (1999, expired/withdrawn)
    │                       [Predecessor — objective tests for 1993 IMBE baseline]
    │
    ├── TIA-102.BABB       P25 Vocoder MOS Conformance Test (1999, status uncertain)
    │                       [Predecessor — subjective MOS test for original IMBE]
    │                       [TIA website redirects BABB to BBAB; may be a catalog error]
    │
    │   2007-2008 enhanced vocoder MOS experiments (established quality levels):
    │
    ├── TSB-102.BABE       Full-Rate Vocoder Evaluation MOS Test (2007, WITHDRAWN)
    │                       [Informative ref 4.2.3 — established full-rate MOS levels]
    │                       [Source of test sentence pairs and some noise files]
    │
    ├── TSB-102.BABF       Half-Rate Vocoder MOS Test Plan for Phase 2 (2008, WITHDRAWN)
    │                       [Informative ref 4.2.4 — established half-rate MOS levels]
    │                       [Source of additional noise files: police siren, fire siren, etc.]
    │       │                                │
    │       └──────────┬─────────────────────┘
    │                  ▼
    │   2010 — subjective MOS results codified into objective PESQ thresholds:
    │
    └── TIA-102.BABG  <--- THIS DOCUMENT
                        Enhanced Vocoder Methods of Measurement for Performance
                        [Successor to BABC/BABB for "enhanced" vocoders]
                        [Covers both full-rate AND half-rate in one document]
                        [Converts BABE/BABF MOS results into automatable PESQ/LQO]
                        [Ships with the actual test vectors from BABE/BABF]
```

**Lineage note:** TSB-102.BABE and TSB-102.BABF were the subjective listening
tests (MOS panels) that established the quality levels the enhanced vocoder
must achieve. BABG then codified those subjective results into objective,
automatable PESQ/LQO thresholds and bundled the test vectors (speech
sentence pairs and noise environments) from those experiments. Once BABG
was published in 2010, BABE and BABF were withdrawn — their purpose was
served. BABG is the single document that carries forward the quality
requirements for both full-rate and half-rate enhanced vocoders.

The 1999-era documents (BABC, BABB) tested the original IMBE baseline
vocoder. BABG Section 1.2 explicitly states that baseline vocoder "cannot
pass" the enhanced tests. The 1999 documents are historically interesting
but not actionable for modern P25 vocoder implementation.

### Companion Documents

| Document | Role |
|----------|------|
| TIA-102.BABA | Vocoder description with half-rate annex -- the codec algorithm BABG measures against |
| TIA-102.BABC | Earlier vocoder reference test (expired) -- BABG effectively replaces this for enhanced vocoders |
| TIA-102.BABB | Earlier MOS conformance test (status uncertain; TIA website redirects to BBAB) -- BABG replaces this role for enhanced vocoders |
| TSB-102.BABE | Full-rate MOS test (2007, **withdrawn**) -- established the performance levels BABG's thresholds are derived from; superseded by BABG |
| TSB-102.BABF | Half-rate MOS test plan for Phase 2 (2008, **withdrawn**) -- established half-rate performance levels; superseded by BABG |
| NTIA TR 08-453 | "Intelligibility of Selected Radio Systems in the Presence of Fireground Noise: Test Plan and Results" (2008) -- source of 6 fireground noise recordings used in BABG |
| ITU-T P.862 | PESQ algorithm (**deleted Jan 2024**, replaced by P.863 POLQA) -- BABG's normative quality metric. See note below on deletion impact. |
| ITU-T P.862.1 | PESQ-to-MOS-LQO mapping (**deleted Jan 2024**) -- provides the LQO conversion formula used in BABG equation 1.3 |
| ITU-T P.863 | POLQA -- active replacement for P.862. Supports narrowband via Appendix III but BABG thresholds are not calibrated for POLQA scores |
| ITU-T P.56 | Objective measurement of active speech level -- how BABG measures ASL |
| TSB-102-A | APCO P25 System and Standards Definition (1995) |
| APCO P25 SoR | APCO Project 25 Statement of Requirements (October 17, 2008) |

### TDMA Testing Suite

Per Motorola's July 2013 white paper on P25 TDMA, BABG is categorized as a TDMA testing
document alongside:

| Document | Focus |
|----------|-------|
| TIA-102.CCAA | Phase 2 TDMA Conformance Tests |
| TIA-102.CCAB | Phase 2 TDMA Interoperability Tests |
| TIA-102.BCAD | Vocoder Conformance Test Procedures |
| TIA-102.BCAF | Vocoder Conformance Test Vectors |
| TIA-102.BABG | Enhanced Vocoder Performance Measurement (this document) |

---

## Key Technical Context

### Scope and Purpose

BABG defines **methods of measurement** (Clause 2) and **performance specifications** (Clause 3)
for the P25 enhanced vocoder. It covers both the full-rate mode (Phase 1 interoperability) and
half-rate mode (Phase 2 interoperability). Tests apply to radios and fixed infrastructure
devices (consoles) alike.

### The "Enhanced" vs. "Baseline" Distinction

Section 1.2 explicitly states that the **baseline vocoder defined in prior versions of
TIA-102.BABA for Phase 1 is outside the scope** of these tests. "In general, the baseline
vocoder cannot pass the tests defined here." This is the 1993-era IMBE vocoder. The "enhanced"
vocoder refers to the improved MBE codec selected by the P25 Steering Committee on
27-April-2007 for the TDMA Two-Slot Air Interface.

### Test Architecture

The tests use a **standard reference vocoder** -- PC-executable software (not hardware) -- as
the known-good half of each measurement. The encoder and decoder are tested independently:

- **Encoder test**: Candidate encoder output is fed to the standard reference decoder. Quality
  is measured at the decoder output.
- **Decoder test**: Standard reference encoder output is fed to the candidate decoder. Quality
  is measured at the decoder output.

This architecture directly verifies interoperability: a candidate encoder must produce output
that the reference decoder handles well, and vice versa.

### PESQ/MOS-LQO Scoring

Quality is measured using **ITU-T P.862 PESQ** (Perceptual Evaluation of Speech Quality),
converted to **MOS-LQO** (Listener Quality Objective) via ITU-T P.862.1:

```
LQO = 0.999 + 4 / (1 + exp(4.6607 - 1.4945 * PESQ))
```

LQO range is approximately 1.0 to 4.55. The minimum LQO threshold for most noise conditions
is **2.0** (Tables 3-1 and 3-2), with fog nozzle noise at **1.8** (the hardest environment).

### IMPORTANT: ITU-T P.862 Deleted — Replaced by P.863 (POLQA)

**As of January 5, 2024**, ITU-T P.862, P.862.1, P.862.2, and P.862.3 were all
**deleted** by the ITU. They have been replaced by **ITU-T P.863** (POLQA —
Perceptual Objective Listening Quality Assessment), active since 2011 with the
current version being P.863 (03/18).

This creates a tension for BABG compliance:

1. **BABG normatively references P.862 and P.862.1** — the LQO conversion
   formula (equation 1.3) and the performance thresholds in Tables 3-1/3-2
   are calibrated against P.862 PESQ scores, not P.863 POLQA scores.

2. **The PESQ and POLQA scales are not directly comparable** — they use
   different perceptual models and produce different raw scores for the same
   audio. A POLQA score cannot be substituted into BABG's LQO formula.

3. **BABG has not been revised** since its original publication in March 2010.
   TIA would need to update BABG to reference P.863 and recalibrate the
   performance thresholds — but no such update has been published.

**Practical implications for implementers:**

- **For BABG conformance testing as written**: Use the P.862 PESQ algorithm.
  Reference C implementations were distributed by ITU before deletion and
  remain available. The algorithm does not stop working because the standard
  was withdrawn — BABG specifies the algorithm, not the ITU's maintenance
  status. This is the correct approach for validating against BABG's
  published thresholds.

- **For new/future P25 vocoder qualification**: The P25 Steering Committee
  may eventually update BABG to reference P.863. Until then, P.862 PESQ
  remains the normative measurement algorithm per BABG.

- **P.863 (POLQA) supports narrowband (8 kHz)** via Appendix III, so it
  is technically capable of measuring P25 vocoder output. However, the
  quality thresholds would need to be re-established on the POLQA scale.

This is a known issue in the telecom standards world — many active standards
still reference P.862 despite its deletion. The algorithm is well-understood,
widely implemented, and the reference implementation source code circulates
freely.

### Test Vectors and Noise Environments

BABG ships with actual test data files ("TIA-102.BABG Extra Files"):

**32 standard test sentence pairs** (WAV, 48 kHz, 8 seconds each, -28 dBov ASL):
- Sourced from TSB-102.BABE MOS tests

**15 standard test noise environments** (WAV, 40 seconds each, -43 dBov):

| Noise | Source | Description |
|-------|--------|-------------|
| Pink Noise | Synthetic | Gaussian, -3 dB/octave, 100 Hz to 20 kHz |
| Babble | TSB-102.BABE | Crowd noise |
| Car | TSB-102.BABE | Inside moving vehicle |
| Street | TSB-102.BABE | Stationary near moving cars |
| Police Siren | TSB-102.BABF | Inside police car at 60 mph with siren |
| Fire Siren | TSB-102.BABF | Inside fire truck at 30 mph with siren |
| Fire Truck w/o Siren | TSB-102.BABF | Inside fire truck at 60 mph, no siren |
| Helicopter | TSB-102.BABF | Inside helicopter in flight |
| Boat | TSB-102.BABF | Coast Guard boat, outboard motor, 30 knots |
| Fire Truck Pump Panel | NTIA TR 08-453 | In front of fire truck water pump |
| Dual PASS Alarm | NTIA TR 08-453 | Two simultaneous Personal Alert Safety System alarms |
| Low Air Alarm | NTIA TR 08-453 | SCBA mask low-air buzzer |
| Fog Nozzle | NTIA TR 08-453 | Fire hose fog nozzle |
| Rotary Saw | NTIA TR 08-453 | Gas-powered rotary saw cutting metal |
| Chainsaw | NTIA TR 08-453 | Gas-powered chainsaw cutting wood |

The noise environments are specifically **public safety operational scenarios** -- this is
not a generic codec quality test but one tailored to the conditions P25 radios actually face.
The 6 fireground noises from NTIA TR 08-453 were added specifically because of concerns about
firefighter communications intelligibility.

### File Formats

| Format | Extension | Sample Rate | Description |
|--------|-----------|-------------|-------------|
| WAV | .WAV | 48 kHz | 16-bit linear, monaural, RIFF header (44 bytes) |
| PCM | .PCM | 8 kHz | 16-bit linear, monaural, headerless raw |
| DAT | .DAT | N/A | Encoded vocoder frames: 144 octets/frame (full-rate) or 72 octets/frame (half-rate), 20 ms per frame |

### Performance Specifications Summary

**Encoder** (Section 3.1):
- Audio sensitivity: -22 dBm0 +1/-2 dB (full-rate and half-rate)
- Noise reduction: Per-noise limits in Table 3-1 (all LQO >= 2.0 except fog nozzle >= 1.8)

**Decoder** (Section 3.2):
- Audio output level: -22 dBm0 +/- 1 dB (full-rate and half-rate)
- Residual noise: Per-noise limits in Table 3-2 (identical thresholds to encoder Table 3-1)

### Software Requirements

- All floating point arithmetic must use **IEEE 754 double precision** (64-bit) minimum
- Active Speech Level computed per **ITU-T P.56**
- Standard hex interpolation (8 kHz to 48 kHz) and decimation (48 kHz to 8 kHz) algorithms
  are specified with exact FIR filter coefficients (Tables 1-2 through 1-4)

---

## Relationship to AMBE+2/DVSI

BABG **never uses the term "AMBE+2"** anywhere in the document. It consistently says "enhanced
MBE vocoder" and "enhanced vocoder." This is deliberate and consistent with TIA's approach of
decoupling the quality standard from any specific vendor implementation. DVSI's AMBE+2 is one
implementation that meets these requirements, but the standard is written so that any enhanced
MBE vocoder achieving the specified PESQ/MOS-LQO scores with the provided test vectors would
comply.

The standard reference vocoder is described as "PC executable software" that "shall obtain the
performance demonstrated by the MOS tests" -- it is a software artifact, not tied to any
specific hardware codec chip.

---

## Practical Significance

This is the document that makes **clean-room enhanced vocoder implementation feasible**. It
provides:

1. **Measurable, automatable quality targets** -- PESQ scores can be computed by software,
   no human listening panels required
2. **Provided test vectors** -- 32 speech files and 15 noise environments ship with the
   standard, so any implementer can self-test
3. **Independent encoder/decoder testing** -- an encoder implementation can be validated
   without a matching decoder, and vice versa
4. **Vendor-neutral language** -- no proprietary algorithm names, no DVSI-specific terminology
5. **Public safety noise realism** -- the noise environments represent actual operational
   conditions, not laboratory abstractions

For a clean-room vocoder implementation effort, BABG provides the acceptance criteria:
implement the algorithm from TIA-102.BABA, then verify it meets BABG's thresholds using the
provided test data. This separates "what the codec must do" (BABA) from "how good it must be"
(BABG).

---

## Normative References (from Section 4.1)

| Ref | Document | Description |
|-----|----------|-------------|
| 4.1.1 | TIA-102.BABA | P25 Vocoder Description; with Annex for Half Rate Vocoder |
| 4.1.2 | ITU-T P.56 | Objective measurement of active speech level |
| 4.1.3 | ITU-T P.862 | PESQ -- Perceptual evaluation of speech quality |
| 4.1.4 | ITU-T P.862.1 | Mapping function for transforming P.862 raw result scores to MOS-LQO |
| 4.1.5 | ITU-T P.862.3 | Application guide for objective quality measurement based on P.862 |
| 4.1.6 | IEEE 754 | Standard for Binary Floating Point Arithmetic |

## Informative References (from Section 4.2)

| Ref | Document | Description |
|-----|----------|-------------|
| 4.2.1 | APCO P25 SoR | APCO Project 25 Statement of Requirements; October 17, 2008 |
| 4.2.2 | TSB-102-A | APCO Project 25 System and Standards Definition; November 1995 |
| 4.2.3 | TSB-102.BABE | Project 25 Vocoder Evaluation Mean Opinion Score Test; 2007 |
| 4.2.4 | TSB-102.BABF | Experiment 3 MOS Test Plan for Vocoder Technology for Project 25 Phase 2; 2008 |
| 4.2.5 | NTIA TR 08-453 | Intelligibility of Selected Radio Systems in the Presence of Fireground Noise: Test Plan and Results; 2008 |

---

## Project Cross-References

- [`analysis/vocoder_wire_vs_codec.md`](../analysis/vocoder_wire_vs_codec.md) -- Analysis of
  vocoder bitstream formats on the wire vs. codec-internal representations
- [`analysis/vocoder_missing_specs.md`](../analysis/vocoder_missing_specs.md) -- Tracking of
  vocoder-related specs that were previously missing or needed; BABG was listed as highest
  priority

---

## Document Revision History (from Section 4.4)

| Date | Version | Notes |
|------|---------|-------|
| 19-September-2008 | 0.0 | First draft for circulation |
| 25-September-2008 | 0.1 | Second draft with performance specs |
| 11-December-2008 | 0.2 | Third draft to address comments |
| 29-April-2009 | 0.3 | Fourth draft to address comments in Vocoder TG |
| 16-October-2009 | 0.32 | Address comments from TR8.4 letter ballot |
| March 2010 | 1.0 (published) | Final TIA Standard |
