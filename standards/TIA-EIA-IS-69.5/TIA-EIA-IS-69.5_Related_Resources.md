# TIA/EIA/IS-69.5 -- Related Resources and Context

## Document Status

- **Title:** Enhanced Digital Access Communications System IMBE Implementation
- **Document Number:** TIA/EIA/IS-69.5
- **Type:** TIA/EIA Interim Standard
- **Published:** April 2000
- **TIA Project:** No. 4223
- **Subcommittee:** TR-8.4 (Vocoders)
- **Pages:** 117 (59 pages body + annexes A through K + references)
- **Status:** Published as Interim Standard. TIA/EIA Interim Standards were required to be reviewed annually and either advanced to full standard or cancelled within three years. As part of the EDACS family, which is now a legacy system, this document is effectively historical/inactive. No superseding revision is known.

## EDACS System History

EDACS (Enhanced Digital Access Communications System) was a proprietary trunked radio system developed by Ericsson/GE Mobile Communications in the late 1980s. Key milestones:

- **Late 1980s:** EDACS deployed as a digital trunked radio system competing with Motorola's SmartNet/SmartZone. Originally analog FM voice with digital trunking control.
- **1990s:** Digital voice capability added via IMBE vocoder (the subject of this document). EDACS systems were widely deployed for public safety, utilities, and commercial users.
- **Acquisition chain:** Ericsson/GE -> Ericsson -> M/A-COM -> Harris Corporation -> L3Harris Technologies.
- **Relationship to ProVoice:** The EDACS digital voice mode described in IS-69.5 is closely related to what later became known commercially as "ProVoice" by M/A-COM/Harris. ProVoice is the proprietary digital voice protocol used on EDACS trunked systems.
- **Current status:** EDACS is a legacy system. Many EDACS deployments have migrated or are migrating to P25. However, EDACS systems remain operational in some jurisdictions and the ProVoice digital voice mode continues to be encountered.

The EDACS family of TIA documents includes:
- **TSB69** -- EDACS System and Shell Standard (overview, glossary, requirements)
- **TIA/EIA/IS-69.5** -- IMBE Vocoder Implementation (this document)
- Additional IS-69.x documents covering other EDACS subsystems

## Relationship to P25/TIA-102 Suite

EDACS and Project 25 are **separate, non-interoperable trunking systems** that share a common vocoder technology:

### What They Share
- **Same IMBE speech model:** Both use the DVSI Improved Multi-Band Excitation algorithm based on the MBE speech model (Griffin and Lim, 1988).
- **Same core parameters:** 8 kHz sampling, 20 ms frames, 160 samples/frame, pitch range 21-122 samples, 9-56 harmonics, same analysis-by-synthesis approach.
- **Same 88 voice parameter bits per frame:** Fundamental frequency (8 bits), V/UV decisions (variable), spectral amplitudes (variable), totaling 88 bits of speech model data.
- **Same standards body:** Both developed under TIA, with vocoder work in the TR-8.4 subcommittee. The same chairman (Jim Holthaus) oversaw both efforts.
- **Same DVSI patent portfolio:** Both require DVSI licensing for the IMBE algorithm.

### Where They Differ
- **Gross bit rate:** EDACS = 7.1 kbps (142 bits/frame); P25 Full-Rate = 7.2 kbps (144 bits/frame).
- **FEC structure:** EDACS uses 7 code vectors with [19,7] Extended Golay + [24,12] Extended Golay + two [23,12] Golay + two [15,11] Hamming + one uncoded. P25 uses a different arrangement of Golay and Hamming codes across its 144-bit frame.
- **Bit prioritization:** Different priority scanning orders for the same 88 voice bits.
- **Interleaving:** Different interleave tables tailored to each system's air interface.
- **Bit modulation:** Both use pseudo-random modulation keyed off the highest-priority code vector, but the specific derivation differs.
- **Air interface:** Completely different trunking protocols, channel structures, and signaling. EDACS uses a dedicated control channel with frequency-hopping voice channels; P25 Phase 1 uses a common air interface defined in TIA-102.BAAA.

### Practical Implication
Because both systems encode the same 88 bits of MBE model parameters, **parameter-level transcoding** between EDACS and P25 is theoretically possible without tandem vocoder degradation. The MBE parameters (fundamental frequency, V/UV decisions, spectral amplitudes) could be extracted from one system's bitstream and re-encoded for the other. This is analogous to the parametric rate conversion between P25 Full-Rate and Half-Rate described in TIA-102.BABA.

## Relevance to Vocoder Implementation Work

### Why This Document Matters

1. **Alternative specification of the same algorithm:** TIA/EIA/IS-69.5 specifies the same IMBE vocoder algorithm as TIA-102.BABA but was written independently for the EDACS context. Comparing the two documents provides cross-validation of the IMBE algorithm specification and can clarify ambiguities in either document.

2. **Complete functional description:** The document explicitly states it describes "the essential operations that are necessary and sufficient to implement this voice coding algorithm." It includes 12 detailed flow charts (Annex K) covering every major subsystem, which serve as pseudocode for implementation.

3. **Detailed encoding example:** Section 10 provides a worked parameter encoding example for a complete frame (L=16, K=6), showing the step-by-step quantization, DCT, bit allocation, and bit vector construction. This is invaluable for validating an implementation.

4. **Tabulated reference data:** All window coefficients (Annexes B, C, I), filter taps (Annex D), quantizer levels (Annex E), bit allocation tables (Annexes F, G), and interleave tables (Annex H) are provided with full numerical precision. While the EDACS-specific FEC and interleaving differ from P25, the core vocoder tables (windows, quantizers, DCT parameters) are the same.

5. **Speech synthesis details:** Section 11 provides a complete specification of the voiced and unvoiced synthesis algorithms, including the Weighted Overlap Add method for unvoiced speech, phase tracking equations, and amplitude interpolation for voiced speech. These synthesis details complement TIA-102.BABA.

### Key Differences from TIA-102.BABA to Note for Implementation
- The EDACS document specifies 142 bits/frame; P25 specifies 144 bits/frame. The extra 2 bits in P25 go to the FEC layer.
- The EDACS FEC uses a [19,7] Extended Golay for the highest-priority vector (u_0 = 7 bits), while P25 uses different code lengths.
- The bit prioritization order differs, so implementing the EDACS bit manipulation layer directly would not produce P25-compatible output.
- The core speech analysis (Section 5), parameter encoding/decoding (Section 6), spectral amplitude enhancement (Section 8), adaptive smoothing (Section 9), and speech synthesis (Section 11) should be functionally equivalent between EDACS and P25.

## DVSI/IMBE Licensing Context

Digital Voice Systems, Inc. (DVSI) holds patent rights on the IMBE voice coding algorithm. The document's DVSI notice (page iii) states:

> "DVSI claims certain rights, including patent rights, in the Improved Multi-Band Excitation (IMBE) voice coding algorithm described in this document and elsewhere. Any use of this technology requires written license from DVSI."

Key licensing facts:
- **IMBE is a trademark of DVSI.**
- DVSI is willing to grant royalty-bearing licenses for EDACS use under standard terms.
- DVSI acknowledges MIT and the Rome Air Development Center (USAF) for early MBE development.
- DVSI granted TIA a free irrevocable license to incorporate the vocoder description into TIA standards publications (copyright notice dated September 8, 1998).
- The same DVSI licensing requirement applies to P25 IMBE implementations under TIA-102.BABA.
- DVSI's address at time of publication: One Van deGraff Drive, Burlington, MA 01803.

The patent notice in the foreword also states: "The patent holders so far identified have, however, filed statements of willingness to grant licenses under those rights on reasonable and nondiscriminatory terms and conditions."

## References Cited in the Document

The document references 11 works, including foundational MBE/IMBE papers:
1. Almeida & Silva -- Variable Frequency Synthesis (ICASSP 1984)
2. Campbell et al. -- 4800 bps Voice Coding Standard (Proc. Mil. Speech Tech. 1989)
3. Griffin & Lim -- Multiband Excitation Vocoder (IEEE Trans. ASSP, 1988) -- the foundational MBE paper
4. Griffin & Lim -- Signal Estimation from Modified Short-Time Fourier Transform (IEEE Trans. ASSP, 1984)
5. Hardwick & Lim -- 4800 bps IMBE Speech Coder (IEEE Workshop 1989)
6. Hardwick -- 4.8 Kbps MBE Speech Coder (MIT S.M. Thesis, 1988)
7. Jayant & Noll -- Digital Coding of Waveforms (Prentice-Hall, 1984)
8. Levesque & Michelson -- Error-Control Techniques for Digital Communication (Wiley, 1985)
9. Lin & Costello -- Error Control Coding (Prentice-Hall, 1983)
10. Press et al. -- Numerical Recipes in C (Cambridge, 1988)
11. Oppenheim & Schafer -- Discrete Time Signal Processing (Prentice-Hall, 1989)

## Related TIA-102 (P25) Documents

| Document | Relationship |
|----------|-------------|
| TIA-102.BABA | P25 Full-Rate IMBE vocoder -- same core algorithm, different FEC/framing |
| TIA-102.BABA-1 | P25 Half-Rate AMBE+2 vocoder addendum -- 3600 bps variant |
| TIA-102.BAAA | P25 FDMA Common Air Interface -- defines how P25 carries IMBE frames |
| TIA-102.BABA-A | P25 vocoder description, revision A |
