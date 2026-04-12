# AMBE-3000 Software Implementation — Objectives and Methodology

**Date:** 2026-04-13  
**Author:** Chance Lindsey  
**Status:** Active

---

## Goal

Produce a software implementation that is functionally equivalent to the
DVSI AMBE-3000 vocoder chip for P25 full-rate (7200 bps) and half-rate
(3600 bps) operation, including parametric rate conversion between the two.

The implementation will be validated by comparing its output against the
AMBE-3000 hardware (via USB-3000™) and DVSI's published test vectors.

---

## Why: The Cross-Standard Interoperability Problem

### The Tandeming Problem

Digital radio systems worldwide — P25, DMR, D-STAR, NXDN, dPMR — all use
DVSI's AMBE® vocoder technology at different bit rates. When these systems
need to interoperate (e.g., a P25 network bridging to a DMR network), the
standard approach is **tandeming**: decode the incoming bit stream to PCM
audio, then re-encode that PCM into the target codec format.

```
Tandeming (current state of the art in most implementations):

  P25 bits → FEC Decode → Vocoder Decode → PCM → Vocoder Encode → FEC Encode → DMR bits
                                            ^^^
                               Full synthesis + re-analysis
                               Additional processing & delay
                               Cumulative quality degradation
```

Each tandem stage introduces:
- **Quality loss** — the decode→PCM→encode round-trip is lossy; each pass
  degrades the speech further
- **Latency** — full synthesis and re-analysis adds processing delay
- **Intelligibility loss** — DVSI's own ABC-MRT testing (per ANSI/ASA S3.2)
  showed tandeming causes approximately **40% intelligibility loss in
  street noise conditions**, with MRT scores dropping below the 75%
  minimal intelligibility threshold established by NIST/ITS research [1][2]

In multi-hop scenarios (e.g., P25 → gateway → DMR → gateway → P25), the
degradation compounds with each tandem stage. The result can be severe —
transmissions with pops, cracks, hiss, and ultimately unintelligible audio.

### The Parametric Rate Conversion Solution

Because all of these standards use MBE-based vocoders, there is a better
approach: **parametric rate conversion** at the MBE model parameter level,
without ever synthesizing or re-analyzing PCM audio.

```
Parametric rate conversion:

  P25 bits → FEC Decode → Extract MBE Parameters → Re-quantize → FEC Encode → DMR bits
                                    ^^^
                          No PCM round-trip
                          Near-zero algorithmic delay
                          Minimal quality degradation
```

The parametric converter:
1. De-interleaves and FEC decodes the input bit stream
2. Extracts MBE model parameters (fundamental frequency, voicing
   decisions, spectral magnitudes, gain)
3. Re-quantizes these parameters to the target rate's codebook structure
4. FEC encodes and interleaves to produce the output bit stream

DVSI's own testing showed parametric rate conversion achieves a **12%
improvement in intelligibility** over tandeming, keeping MRT scores above
the critical threshold even in noise conditions [3].

Additionally, a **frame analyzer** can be integrated to detect corrupted
frames after FEC decode and replace them with erasure frames rather than
propagating bit errors through re-encoding. The downstream decoder then
performs a clean frame repeat instead of synthesizing corrupted parameters.

### The Amateur Radio Use Case

DVSI themselves identify amateur radio as a user community for this
technology [3]. Ham radio operators routinely need to bridge between:

- **P25** (7200 bps full-rate or 3600 bps half-rate) — public safety
  interoperability, ARES/RACES
- **DMR** (3600 bps) — the most widely deployed amateur digital voice mode
- **D-STAR** (3600 bps) — JARL/Icom amateur digital standard

Current amateur implementations (MMDVM, HBlink, xlxd reflectors) bridge
these modes via tandeming — DMR bits → PCM → P25 bits. This is the
"messy" approach: it works, but every bridge hop degrades the audio.

A software parametric rate converter would allow amateur gateway operators
to bridge P25 ↔ DMR ↔ D-STAR at the MBE parameter level, preserving
intelligibility across networks. This is a non-commercial, non-profit
application that directly benefits emergency communications
interoperability — the same mission that P25 was designed to serve.

### DVSI's Own Position

DVSI's 2021 white paper "Meeting the Demand of Expanding Digital Mobile
Radio Communication Networks" [3] explicitly advocates for parametric rate
conversion over tandeming and identifies these user communities:

> *"Mission critical wireless communication is not segregated solely to
> law enforcement and emergency first responders. Any industry that relies
> on digital wireless communication, from energy/utilities, to large
> university campuses or transportation systems, or manufacturing
> facilities and even amateur radio demand high quality voice and
> intelligibility."*

> *"When there is a need to expand or communicate between any of these
> systems, the best way to avoid the detrimental effect of encoder/decoder
> tandeming is to employ a parametric rate converter/repeater."*

The technical capability DVSI describes in that paper — parametric rate
conversion between any two AMBE® rates from 2000–9600 bps — is exactly
what this project aims to implement in software, using public
specifications and expired patents.

### References

[1] Letowski, T.R. & Scharine, A.A. (2017). *Correlational Analysis of
Speech Intelligibility Test and Metrics for Speech Transmission*
ARL-TR-8227. U.S. Army Research Laboratory.

[2] Institute for Telecommunication Sciences. *ABC-MRT16 and ABC-MRT*.
NIST/ITS Audio Quality Research.

[3] Digital Voice Systems, Inc. (2021). *Meeting the Demand of Expanding
Digital Mobile Radio Communication Networks*. DVSI white paper.

---

## Methodology: Black-Box Verification

The approach is black-box: the implementation is built from public
specifications and validated against hardware output, without access to
or reliance on DVSI's proprietary firmware or source code.

### Architecture

```
                    Same MBE frames (channel bits)
                           │
              ┌────────────┼────────────┐
              ▼                         ▼
    ┌──────────────────┐     ┌──────────────────┐
    │  Software Decoder │     │   AMBE-3000 Chip  │
    │  (TIA-102.BABA-A  │     │   (via USB-3000)  │
    │   + patent math)  │     │                   │
    └────────┬─────────┘     └────────┬──────────┘
             │                        │
          PCM output              PCM output
             │                        │
             └───────── compare ──────┘
                    (sample-level)
```

### Decoder Verification (channel bits → PCM)

1. Feed `.bit` test vector files to both the software decoder and the
   AMBE-3000 chip (via USB-3000 `usb3k_client -dec`)
2. Capture PCM output from both
3. Compare sample-by-sample — measure SNR, spectral distortion, and
   perceptual similarity
4. Iterate on synthesis algorithm until outputs converge
5. Repeat across all rate configurations (r0–r63) and P25 modes

### Encoder Verification (PCM → channel bits)

1. Feed `.pcm` test vector files to both the software encoder and the
   AMBE-3000 chip (via USB-3000 `usb3k_client -enc`)
2. Capture channel bit output from both
3. Compare bit-for-bit where possible; measure parameter-level deviation
   where bit-exact match is not achievable
4. Validate round-trip: software-encode → hardware-decode and
   hardware-encode → software-decode must both produce acceptable audio

### Rate Converter Verification (channel bits at rate A → channel bits at rate B)

1. Feed encoded frames at one rate to both the software rate converter
   and the AMBE-3000 chip in repeater mode (`PKT_RPT_MODE`)
2. Capture re-encoded frames at target rate from both
3. Compare parametrically — the MBE model parameters (pitch, voicing,
   spectral amplitudes) should match after rate conversion
4. Validate using `tv-rc` test vectors which cover 64 rate conversion
   configurations

### Pre-Computed Test Vectors (No Hardware Required)

DVSI provides pre-computed test vectors that represent the chip's
reference output at every supported rate. These enable initial
development and validation without the USB-3000 hardware:

| Vector Set | Configurations | Contents |
|------------|---------------|----------|
| `tv-std` | 62 rates (r0–r61) + P25/P25X with and without FEC | ~96 input files × 66 output directories |
| `tv-rc` | 64 rates (r0–r63) + D-STAR + P25 rate conversion | ~97 input files × 67 output directories |

Test methodology is defined in:
- `cmpstd.txt` — standard codec encode/decode tests
- `cmpp25.txt` — P25 full-rate with FEC (`-r 0x0558 0x086b 0x1030 0x0000 0x0000 0x0190`)
- `cmpp25a.txt` — P25 enhanced variant
- `cmpp25x.txt` — P25 extended variant
- `cmprc.txt` — rate conversion tests across all configurations

---

## Sources: Public Technical Documentation

### 1. TIA-102.BABA-A — Vocoder Description (2014)

The authoritative source for P25 vocoder wire format and baseline MBE
synthesis. Provides:
- IMBE frame structure (144 bits full-rate, 72 bits half-rate)
- FEC encoding (Golay and Hamming codes)
- Quantization tables for pitch, voicing, and spectral amplitudes
- Baseline MBE synthesis algorithm (Sections 8, 9, 15)
- Bit prioritization and interleaving

This standard defines **what goes over the air**. It is fully specified,
interoperable, and non-negotiable for any P25 implementation.

### 2. DVSI USB-3000 Manual and Software

Publicly downloadable from DVSI's website. Provides:
- AMBE-3000 packet protocol (control, speech, channel packets)
- All 64 rate table configurations
- Rate control word format (6 × 16-bit words)
- P25 rate control words: `0x0558 0x086b 0x1030 0x0000 0x0000 0x0190`
- Rate conversion architecture (repeater mode)
- Test vector format and validation methodology

### 3. Expired DVSI Patents

See [AMBE-3000_Patent_Reference.md](AMBE-3000_Patent_Reference.md) for
the complete patent analysis. Key patents covering the AMBE+2 algorithm
improvements are all expired and serve as public technical documentation
for the synthesis, quantization, and rate conversion techniques.

---

## Patent Landscape Summary

All relevant DVSI patents have expired. The table below covers the
complete chain from the original MBE work through AMBE+2 and the
voice transcoder:

| Patent | Title | Filed | Granted | Expired | Relevance |
|--------|-------|-------|---------|---------|-----------|
| US5054072 | MBE vocoder | ~1989 | 1991 | ~2009 | Original MBE codec |
| US5226084 | MBE quantization | ~1990 | 1993 | ~2010 | MBE parameter coding |
| US5491772 | IMBE vocoder | ~1993 | 1996 | ~2013 | Improved MBE (P25 Phase 1) |
| US5517511 | IMBE vocoder | ~1993 | 1996 | ~2013 | IMBE synthesis |
| US5701390 | MBE synthesis with regenerated phase | 1995-02-22 | 1997-12-23 | 2015-02-22 | **Phase regeneration algorithm** |
| US6199037 | Joint quantization of voicing and pitch | 1997-12-04 | 2001-03-06 | 2017-12-04 | **Subframe voicing/pitch quantization** |
| US7634399 | Voice transcoder | 2003-01-30 | 2009-12-15 | 2025-11-07 | **Parametric rate conversion** |
| US7957963 | Voice transcoder (continuation) | 2003-01-30 | 2011-06-07 | 2023-01-30 | Rate conversion (same as above) |
| US8315860 | Interoperable vocoder | 2002-11-13 | 2012-11-20 | 2022-11-13 | **Enhanced full-rate encoding** |
| US8359197 | Half-rate vocoder | ~2003 | 2013-01-22 | ~2023 | Half-rate AMBE+2 |
| US8595002 | Half-rate vocoder | 2003-04-01 | 2013-11-26 | 2023-04-01 | **Half-rate AMBE+2 (key patent)** |

### What the Patents Describe

Together, these patents document the complete algorithmic chain that
differentiates AMBE+2 from baseline IMBE:

**Synthesis improvements (decoder):**
- Phase regeneration from spectral envelope shape instead of random phase
  (US5701390) — eliminates "buzzy" artifacts
- Voicing-independent spectral magnitude computation — smoother
  representation, better quantization efficiency
- Per-harmonic voicing decisions instead of per-band (US8595002)
- Edge detection kernel for phase estimation from log-compressed
  spectral magnitudes

**Quantization improvements (encoder):**
- Joint quantization of voicing metrics across subframes (US6199037)
- Split vector quantization of spectral magnitudes with DCT-domain
  codebooks (US8595002)
- Prediction-residual coding with prior-frame interpolation
- Data-dependent scrambling for error resilience (US8595002)

**Rate conversion (transcoder):**
- Parametric transcoding at MBE model level — no PCM round-trip
  (US7634399)
- Spectral magnitude interpolation and resampling based on fundamental
  frequency ratios
- Voicing band normalization to fixed 8-band representation
- Prior-frame prediction with ρ=0.65 scaling factor

**Enhanced full-rate encoding (interoperable):**
- Three-state voicing model: voiced/unvoiced/pulsed (US8315860)
- Fundamental frequency field repurposing for voicing state encoding
  when no voiced bands exist
- Tone detection and spectral sidelobe suppression
- Noise suppression via spectral subtraction

---

## Implementation Order

| Phase | Deliverable | Primary Sources | Use Case |
|-------|-------------|-----------------|----------|
| 1 | Decoder (channel bits → PCM) | TIA-102.BABA-A, US5701390, US8595002 | SDR monitoring, audio playback |
| 2 | Encoder (PCM → channel bits) | TIA-102.BABA-A, US8595002, US6199037 | Transmit applications |
| 3 | Rate converter (bits → bits) | US7634399, US7957963 | **P25 ↔ DMR ↔ D-STAR bridging** |
| 4 | Enhanced interoperable mode | US8315860 | Improved full-rate quality |

The decoder is the highest-value target for individual users: it enables
monitoring, SDR applications, and audio playback from P25 captures.

The rate converter is the highest-value target for the amateur radio
community: it replaces the lossy tandem bridge approach (DMR → PCM → P25)
with parametric conversion at the MBE model level, preserving
intelligibility across cross-standard gateways.

---

## What This Is NOT

- This is not a copy of DVSI firmware or proprietary source code
- This is not derived from DVSI's licensed software libraries
- The DVSI host software (usb3k_client) is a USB communication layer,
  not the vocoder algorithm — the algorithm runs on the AMBE-3000 chip
- All algorithmic knowledge comes from TIA standards (public),
  expired patents (public domain), and black-box validation against
  published test vectors (public)
