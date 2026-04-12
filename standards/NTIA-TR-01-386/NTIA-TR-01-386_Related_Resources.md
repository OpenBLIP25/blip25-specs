# NTIA-TR-01-386: Related Resources and Context

## Document Identity

| Field | Value |
|---|---|
| Title | Voice Quality Assessment of Vocoders in Tandem Configuration |
| Report Number | NTIA Report 01-386 |
| Authors | Christopher Redding, Nicholas DeMinco, Jeanne Lindner |
| Organization | Institute for Telecommunication Sciences (ITS), Boulder, CO |
| Agency | National Telecommunications and Information Administration (NTIA), U.S. Department of Commerce |
| Published | April 2001 |
| Sponsor | National Communications System (NCS), Office of Standards and Technology, DNRO 66008 |
| Copyright Status | **Public domain** -- U.S. government publication, no copyright restrictions |
| Type | Technical report (not a standard) |

## The ITS Lab and Authors

The Institute for Telecommunication Sciences (ITS) is the research and engineering laboratory of NTIA, located at 325 Broadway, Boulder, Colorado 80305. ITS has long been a center for objective voice quality measurement research. The APRE (Audio Play, Record, and Estimate) software tool and the Measuring Normalizing Block (MNB) algorithm used in this study were developed at ITS. Stephen Voran and Margaret Pinson of ITS developed the APRE tool; Voran's work on objective speech quality estimation (MNB technique) is foundational to ITU-T Recommendation P.861 and subsequent objective quality standards.

## Relationship to TIA-102.BABA-A Section 17 (Parametric Rate Conversion)

This report provides the empirical motivation for parametric rate conversion as specified in TIA-102.BABA-A (Vocoder Description for FDMA and TDMA) Section 17. The key finding -- that tandem vocoding (decode to PCM, re-encode) degrades MOS by approximately 0.5 points per stage -- demonstrates why Phase 1 (IMBE) to Phase 2 (AMBE+2) bridging must not use the naive decode-re-encode path.

Section 17 of BABA-A instead specifies direct parametric transcoding: converting IMBE vocoder parameters (pitch, voiced/unvoiced decisions, spectral magnitudes, error correction state) into AMBE+2 parameters without passing through the PCM audio domain. This preserves voice quality at near single-vocoder levels rather than suffering the tandem degradation quantified in TR-01-386.

The IMBE-IMBE tandem result (MOS 2.59 vs. single IMBE MOS 3.32) represents a 0.73 MOS point drop -- moving from "Fair" toward "Poor." An IMBE-to-AMBE+2 tandem would show comparable or worse degradation, since AMBE+2 is a different vocoder family. Parametric conversion avoids this entirely.

## Relationship to ISSI Bridging (TIA-102.BACA Series)

The Inter-RF Subsystem Interface (ISSI) specified in TIA-102.BACA and related documents carries vocoded speech between P25 RF subsystems. A core design principle of ISSI is to transport IMBE frames as parameters (in RTP packets) rather than decoding to audio at each hop. This report demonstrates why: each decode-re-encode cycle at an ISSI boundary would add roughly 0.5 MOS points of degradation. In a multi-hop network (e.g., console to RFSS A to RFSS B), tandem stages would compound, potentially pushing quality below acceptable levels.

The ISSI design philosophy -- pass vocoder parameters end-to-end, decode only at the final endpoint -- is directly validated by this report's findings.

## Connection to Vocoder Quality Analysis

This report is one of the few publicly available studies that measures the P25 IMBE vocoder's quality both standalone and in tandem with other codecs. Key reference data points:

- **Single IMBE**: Estimated MOS 3.32 (L(AD) = 0.579)
- **IMBE-IMBE tandem**: Estimated MOS 2.59 (L(AD) = 0.394)
- **IMBE-PCM tandem**: Estimated MOS 3.19 (L(AD) = 0.547)
- **IMBE-AMBE tandem**: Estimated MOS 2.63 (L(AD) = 0.408)

The AMBE vocoder tested here (4.8 kbps satellite variant) is related to but not identical to the AMBE+2 vocoder used in P25 Phase 2. However, both share the multi-band excitation architecture from DVSI, so the tandem behavior is expected to be similar.

## Practical Significance for P25 System Design

The report's findings translate into concrete system design rules:

1. **Avoid tandem vocoding wherever possible.** Every decode-re-encode cycle costs approximately 0.5 MOS points under ideal conditions; real-world degradation with channel errors will be worse.

2. **Use parametric conversion for Phase 1/Phase 2 bridging.** This is not optional -- it is the difference between "Fair" and "Poor" voice quality.

3. **Console patches and cross-system gateways are the highest-risk points.** Dispatch consoles that decode P25 IMBE to analog for monitoring and then re-encode for retransmission create exactly the tandem scenario measured here.

4. **ISSI links should carry vocoder parameters, not decoded audio.** Each ISSI hop that decodes and re-encodes compounds the degradation.

5. **PCM tandems are less damaging than vocoder-vocoder tandems** (MOS ~3.2 vs. ~2.6 for IMBE), but still measurably degrade quality. Even inserting a high-quality PCM link in the chain has a cost.

6. **Error-free channel results are best-case.** The report explicitly notes that degraded channel conditions (multipath, noise, interference) would further reduce tandem performance. Operational P25 systems with channel impairments will see worse results than reported here.

## Related Standards and References

| Document | Relationship |
|---|---|
| TIA-102.BABA-A | Vocoder description; Section 17 specifies parametric rate conversion motivated by this report's findings |
| TIA-102.BACA / BACA-A | ISSI specification; carries IMBE parameters to avoid tandem vocoding |
| TIA-102.BAAA-A | P25 Common Air Interface (FDMA); uses IMBE vocoder tested in this report |
| TIA-102.BAAB | P25 Phase 2 (TDMA); uses AMBE+2 vocoder (related to AMBE tested here) |
| ITU-T P.861 | MNB algorithm used for objective quality measurement in this report |
| ITU-T P.800 | Subjective MOS testing methodology referenced in this report |
| ITU-T G.711 | PCM standard used as quality reference benchmark |
| IEEE Std 297 | Harvard Sentences used as test speech material |
| NTIA Report 98-347 | Voran's earlier work on MNB objective quality estimation |

## Availability

As a U.S. government NTIA technical report, TR-01-386 is in the public domain and may be freely reproduced, distributed, and cited without restriction. It is available through the NTIA technical reports archive.
