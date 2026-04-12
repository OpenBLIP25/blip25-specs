# TSB-88.3-C Related Resources and Context

## Document Status

| Field | Value |
|-------|-------|
| Document | TIA TSB-88.3-C |
| PN Reference | PN3-4744.3-RV3 (Draft Issue K) |
| Full Title | Wireless Communications Systems - Performance in Noise and Interference - Limited Situations, Part 3: Performance Verification |
| Date | January 2008 |
| Committee | TIA TR-8.18 |
| Type | Telecommunications Systems Bulletin (informative) |
| Supersedes | TSB-88-B (Section 8 and Annex E) |
| Scope | Frequencies below 1 GHz |

TSB documents are informative (advisory) rather than normative (mandatory). They represent recommended best practices developed by industry consensus through TIA engineering committees.

## TSB-88 Series Overview

The TSB-88-C series is a three-part bulletin providing a unified methodology for coverage prediction, performance modeling, and empirical verification of land mobile radio systems. The series is technology-neutral, covering analog FM, P25 digital (C4FM/CQPSK), and wideband data modulations across VHF, UHF, 700 MHz, 800 MHz, and 900 MHz bands.

### Part 1: TSB-88.1-C -- Technology Independent Performance Modeling
Defines the Channel Performance Criterion (CPC), Delivered Audio Quality (DAQ) scale, and the framework for computing Cf/N and Cf/(I+N) thresholds for various modulation types. Contains the reference tables (Annex A) that map DAQ levels to specific C/N and BER values for each modulation -- data that TSB-88.3-C references extensively for acceptance test thresholds.

### Part 2: TSB-88.2-C -- Propagation Modeling, Including Noise
Covers terrain databases, propagation loss models, environmental RF noise data, and the mathematical framework for predicting coverage contours and service area reliability. Provides the theoretical predictions that TSB-88.3-C's verification methods are designed to validate.

### Part 3: TSB-88.3-C -- Performance Verification (this document)
Provides the empirical test methodologies for Coverage Acceptance Test Plans (CATP), local conformance measurements, and interference identification. Bridges the gap between prediction (Parts 1 and 2) and real-world deployed performance.

## Revision History

| Version | Date | Key Changes |
|---------|------|-------------|
| TSB-88 Issue O | January 1998 | Original release (single document) |
| TSB-88 Issue B | September 2004 | Major update: consolidated annexes, new terrain data, NLCD info |
| TSB-88 Issue B (errata) | April 2005 | Clarified default ENBW for analog FM in VHF/UHF |
| TSB-88.3 Issue C | ~2008 | Split into three documents; new CATP types for voice, data, and local median; new IM measurement application (Annex C); new intercept point and IM impact section; corrected confidence level/interval errors from TSB-88B |

## Relationship to P25 Coverage Planning

TSB-88.3-C is the primary industry reference for validating P25 system coverage. The relationship operates at several levels:

**Acceptance Testing**: When a P25 system is deployed (particularly for public safety), the customer and integrator use TSB-88.3-C methodology to confirm the system meets its coverage specification. The Voice CATP (V-CATP) uses BER% as the objective criterion for digital systems, with RSSI providing supplementary data for interference identification.

**Channel Performance Criterion**: The CPC ties directly to P25's performance parameters. For P25 C4FM, the document references TIA-102.CAAA for standard BER (5%) and reference sensitivity measurements. The faded Cf/N values from TSB-88.1-C's Annex A tables determine the pass/fail threshold at each test tile.

**DAQ Scale**: The DAQ scale provides a technology-neutral quality metric that allows comparison between analog FM and P25 digital voice. For public safety, DAQ 3.4 is the recommended target -- defined as speech that is understandable with repetition only rarely required. This is the same quality target used in P25 system procurement specifications.

**Interference Analysis**: The intermodulation and interference identification methods are directly applicable to P25 base station sites, particularly those using receiver multicouplers. The document's treatment of cascaded noise figure, IP3, and the symbolic method for IM analysis is essential for P25 site engineering where multiple narrowband channels share antenna infrastructure.

**Wideband Considerations**: The document explicitly addresses the reduced IMR seen in wideband data systems compared to P25 narrowband, quantifying a 12.8 dB reduction for SAM 50 kHz wideband versus P25 narrowband with identical IP3 values. This is critical for frequency coordination at sites hosting both P25 and wideband systems.

## Referenced TIA/P25 Standards

| Document | Description | Role in TSB-88.3-C |
|----------|-------------|---------------------|
| TIA-102.CAAA | Digital C4FM/CQPSK Transceiver Measurement Methods | Defines standard BER, reference sensitivity, IMR measurement for P25 |
| TIA-102.CAAB | Digital C4FM/CQPSK Performance Recommendations | P25 transceiver performance parameters |
| TIA-603-C | Land Mobile FM/PM Equipment Measurement | Analog measurement methods (SINAD, sensitivity) |
| TIA/EIA-845 | Signal Strength Measurement | Calibration and RSSI measurement methodology |
| TSB-102-A | P25 overview document | General P25 reference |
| TSB-88.1-C | Technology Independent Performance Modeling | CPC definitions, DAQ tables, Cf/N thresholds |
| TSB-88.2-C | Propagation Modeling | Propagation models, noise data, coverage prediction |
| TIA-902.CAAB-A / TIA-902.CBAB | SAM Wideband Data standards | Referenced for wideband IM comparison |
| TIA-905.CAAA / TIA-905.CAAB | TDMA transceiver standards | Referenced for 2-slot TDMA measurements |
| TSB-902 | Wideband overview | SAM system reference |

## Connection to NTIA TR-08-453 (Fireground Noise)

While TSB-88.3-C does not directly reference NTIA TR-08-453, the two documents address complementary aspects of the same problem. TSB-88.3-C establishes the RF-level performance metrics (Cf/N, Cf/(I+N), BER) that determine whether a signal is deliverable, while NTIA TR-08-453 examines how vocoder performance degrades in the presence of high ambient acoustic noise at the transmitting end (as encountered on firegrounds).

The connection is through the DAQ scale: TSB-88.3-C uses DAQ as the pass/fail quality criterion for coverage acceptance, and the DAQ rating inherently reflects vocoder behavior. A system that achieves DAQ 3.4 at the RF level (adequate Cf/N per TSB-88.1-C tables) may still deliver poor intelligibility if the source audio is severely corrupted by fireground noise before it reaches the vocoder. NTIA TR-08-453's findings about IMBE/AMBE vocoder performance in high-noise acoustic environments thus represent a degradation factor that sits "above" the RF performance layer that TSB-88 addresses.

## Connection to TIA-102.BABG (Enhanced Vocoder Measurement)

TIA-102.BABG defines measurement methods for P25 vocoders including the AMBE+2 enhanced vocoder. TSB-88.3-C's DAQ-based acceptance criteria depend on the vocoder's performance characteristics as measured by TIA-102.BABG methods. The Cf/N thresholds in TSB-88.1-C's Annex A tables that TSB-88.3-C uses for pass/fail criteria were derived from vocoder performance testing. Any vocoder performance improvement (such as from IMBE to AMBE+2) would potentially alter the required Cf/N for a given DAQ level, which propagates directly into TSB-88.3-C acceptance test thresholds.

## Practical Significance for System Design

### For System Integrators
TSB-88.3-C provides the contractual framework for proving a system meets its coverage specification. Annexes A and B present fill-in-the-blank user choice forms for Voice CATP and Data CATP respectively, allowing procurement specifications to reference specific test parameters (DAQ target, reliability percentage, confidence level, sampling error allowance, pass/fail criterion).

### For Frequency Coordinators
The interference identification methodology (Section 5.11) and intermodulation analysis (Section 5.9) provide standardized tools for resolving interference disputes, particularly important in the post-800 MHz rebanding environment. The local median testing procedure with its three-test progression (squelch test, I+N measurement, C-only measurement) was specifically developed for this purpose.

### For RF Engineers
The receiver multicoupler analysis methodology (cascaded noise figure, symbolic method, IP3 calculations) provides the tools to evaluate the tradeoff between sensitivity improvement and intermodulation degradation at base station sites. The comparison between narrowband and wideband IM performance is essential for sites hosting multiple system types.

### For Public Safety
The recommended reliability targets (97% service area, 99% confidence level) and DAQ 3.4 quality criterion represent the public safety community's consensus requirements. The building penetration loss data and in-building test methodologies address the critical requirement for indoor portable coverage.

## Key Technical References (from Bibliography)

- Lee, Wm. C.Y. -- Lee's Method for local median measurement (50 subsamples over 40 wavelengths)
- Hata, Masaharu -- Empirical propagation loss formula
- APCO "Best Practices Guide" -- Avoiding interference between public safety and commercial systems at 800 MHz
- Davidson, Allen et al. -- Building penetration loss measurements at 900 and 1500 MHz (basis for generalized building loss curves)
- Sangers, Richard C. -- Intercept point and undesired responses (foundational IMR theory)
