# TIA-102.BABA-1: Related Resources and Context

## Status

**Active.** This document (TIA-102.BABA-1, July 2009) is an addendum to TIA-102.BABA and remains
normative for P25 Phase 2 Dual-Rate vocoder interoperability. It has not been superseded or
withdrawn. TIA-102.BABA was reaffirmed December 2008, and this addendum was finalized July 2009.
Both documents together define the mandatory Dual-Rate vocoder for APCO P25 Phase 2 systems.

## Standards Family

This document sits within the TIA-102 (Project 25) suite, specifically in the "B" (voice/audio)
layer of the P25 document tree.

### Standards Lineage

```
TIA-102 (APCO Project 25 Suite)
├── AAAA — Suite Overview
├── AAAB — Glossary
├── BAAA — Physical Layer (FDMA CAI)
├── BAAB — Physical Layer Measurements
├── BBAB — Physical Layer (Phase 2 TDMA)
│
├── BABA — P25 Full-Rate Vocoder (7200 bps, IMBE)
│            [Parent document — reaffirmed December 2008]
│
└── BABA-1 — P25 Half-Rate Vocoder Annex (THIS DOCUMENT)
              [Addendum — 3600 bps, IMBE Half-Rate]
              [Paired with BABA to form "Dual-Rate" vocoder]
              [Used in Phase 2 TDMA (BBAB) channel]
```

### Companion Documents

| Document | Title | Relationship |
|---|---|---|
| TIA-102.BABA | P25 Vocoder Description (Full-Rate) | Parent — speech model, synthesis defined there |
| TIA-102.BBAB | P25 Phase 2 Physical Layer (TDMA) | Defines radio channel carrying this vocoder |
| TIA-102.BAAA | P25 CAI (Phase 1 Physical Layer) | Full-Rate channel (uses BABA only) |
| TIA-102.BCAD/BCAE/BCAF | Vocoder Conformance Test Specs | Test procedures for BABA/BABA-1 compliance |
| TIA-102.CABC | Conventional Interoperability Tests | References vocoder interop requirements |

## Practical Context

### Deployment

This vocoder is deployed exclusively in APCO P25 Phase 2 systems. P25 Phase 2 uses TDMA (Time
Division Multiple Access) with two time slots per 12.5 kHz channel. Each time slot carries one
Half-Rate vocoder bitstream at 3600 bps, allowing two simultaneous voice calls in the spectrum
previously occupied by one Phase 1 call.

Real-world P25 Phase 2 infrastructure includes:
- Motorola Solutions ASTRO 25 Phase 2 systems (APX, XTS, SLR series)
- Harris (L3Harris) Unity XG-100P, BeOn systems
- Kenwood NX-series Phase 2 portables
- Hytera HP, PD series Phase 2 radios
- Tait TP series Phase 2 portables

The Half-Rate vocoder is mandatory for Phase 2 interoperability. Phase 2 radios must also
support Phase 1 Full-Rate (BABA) for backward compatibility.

### Codec Technology

The underlying technology is IMBE (Improved Multi-Band Excitation), developed by Digital Voice
Systems Inc. (DVSI). DVSI holds numerous patents on the technology. Any hardware or software
implementation requires a DVSI license. DVSI sells codec chips, software libraries, and VLSI
implementations directly to manufacturers.

The standard explicitly notes: "Errors or omissions which are discovered should be communicated
to DVSI" (Section 3), and DVSI "may occasionally update this document to correct such problems."
This reflects the unusual arrangement where the codec inventor authored the standard.

### Spectral Amplitude Enhancement

The decoder is required to perform spectral amplitude enhancement (Section 6), using the
procedure from Chapter 8 of TIA-102.BABA. This is a perceptual post-processing step that
improves perceived quality. It is mandatory at the decoder but not at the encoder.

### Tone Passthrough

A dedicated Tone Frame mechanism (Section 7) allows DTMF, KNOX, single-frequency, and call-
progress tones to pass through the vocoder with full fidelity. This is critical for public
safety applications where in-band signaling tones must survive the digital radio link.

## Key Online Resources

### Standards Bodies and Regulators

- **TIA Standards Store**: https://www.tiaonline.org/standards/catalog/
  (Purchase TIA-102.BABA and TIA-102.BABA-1)
- **APCO International P25 Standards**: https://www.apcointl.org/technology/p25/
- **FCC P25 CAP (Compliance Assessment Program)**:
  https://www.fcc.gov/public-safety-homeland-security/policy-and-licensing-division/p25
  (Equipment compliance database for P25 Phase 2 products)

### Technical References

- **IHS Markit / S&P Global (TIA document sales)**: https://store.accuristech.com/tia
- **DVSI (Digital Voice Systems Inc.)**: https://www.dvsinc.com
  (Patent holder; source of IMBE/AMBE codec chips and software)
- **OP25 project wiki**: https://osmocom.org/projects/op25/wiki
  (Technical documentation of P25 vocoder internals including Half-Rate)

### Phase 2 Background

- **NPSTC P25 Technology Interest Group**: https://www.npstc.org/p25TIG.jsp
  (Technical overview papers on Phase 2 deployment)

## Open-Source Implementations

The DVSI IMBE patent portfolio makes open-source implementation legally complex. Known projects:

### JMBE (Java Multi-Band Excitation)
- **Repository**: https://github.com/DSheirer/jmbe
- **Description**: Java implementation of IMBE (Full-Rate) and IMBE Half-Rate (AMBE+2)
  decoders. Used by SDRTrunk and similar P25 decoders. Operates under DVSI license requirement.
- **Relevance**: Implements the quantization tables and decoding pipeline specified in BABA-1.
  Key classes handle PRBA vector dequantization (Annexes E/F), Golay FEC (Section 5.2), and
  spectral amplitude reconstruction (Section 4.4).

### SDRTrunk
- **Repository**: https://github.com/DSheirer/sdrtrunk
- **Description**: Java SDR P25 decoder. Includes Phase 2 TDMA support and calls JMBE for
  vocoder decoding. Implements the deinterleaving (Annex H) and Golay error correction.
- **Relevance**: Full P25 Phase 2 stack including Half-Rate vocoder via JMBE.

### OP25 (osmocom)
- **Repository**: https://github.com/osmocom/op25
- **Description**: GNURadio-based P25 decoder. Python/C++ implementation.
- **Relevance**: Implements P25 Phase 2 framing. Vocoder decoding calls external IMBE library.

### gr-p25craft
- **Repository**: https://github.com/boatbod/op25 (fork)
- **Description**: GNURadio blocks for P25. Includes Phase 2 TDMA slot decoding.

### Noteworthy Limitation
No fully open-source implementation of the IMBE Half-Rate codec core (quantization/dequantization
math from Sections 4 and 5) exists without DVSI licensing, due to patent coverage. The tables
in this document (Annexes A–J) are normative constants that any compliant implementation must
embed exactly. Implementations typically use DVSI-supplied binary libraries for the actual codec
math, with the open-source layer handling framing, FEC, and interleaving.

## Document Revision History

| Date | Revision | Notes |
|---|---|---|
| 10 October 2007 | 1.0.0 | Initial Half-Rate Vocoder Addendum |
| 10 March 2008 | 1.0.1 | Revised addendum released |
| 28 March 2008 | 1.0.2 | Typographical corrections to Section 7.2 |
| 1 July 2008 | 1.0.3 | Scope revised to reference Dual-Rate vocoder |
| 16 April 2009 | 1.0.4 | Edits from TIA ballot comments |
| 27 April 2009 | 1.0.5 | Additional typographical edits from TIA ballot |
| 24 June 2009 | 1.0.6 | Final revision for publication |

## DVSI Patent References

U.S. Patents claimed by DVSI on the Half-Rate vocoder technology:
6,199,037 · 5,870,405 · 5,754,974 · 5,664,051 · 5,630,011 · 5,517,511 · 5,491,772 ·
5,247,579 · 5,226,108 · 5,226,084 · 5,216,747 · 5,081,681

Any use requires a separate written license from DVSI. DVSI has stated terms are available on
reasonable and nondiscriminatory (RAND) terms for APCO Project 25 equipment.
