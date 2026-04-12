# NTIA TR-08-453 Related Resources and Context

## Document Status

**U.S. Government Publication -- Public Domain.** This is an NTIA Technical Report,
not a TIA standard. As a work of the U.S. federal government (National
Telecommunications and Information Administration, U.S. Department of Commerce),
it is not subject to copyright and is freely available for any use.

- **Report Number:** NTIA Technical Report TR-08-453
- **Title:** Intelligibility of Selected Radio Systems in the Presence of Fireground
  Noise: Test Plan and Results
- **Authors:** David J. Atkinson, Andrew A. Catellier (Institute for Telecommunication
  Sciences, NTIA, U.S. Department of Commerce, Boulder, CO 80305)
- **Published:** June 2008
- **Funded by:** Department of Homeland Security Office of Interoperability and
  Compatibility, National Institute of Standards and Technology Office of Law
  Enforcement Standards, and the Federal Partnership for Interoperable
  Communications
- **Conducted at:** Institute for Telecommunication Sciences (ITS), Boulder, Colorado,
  under the supervision of J.R. Bratcher

---

## The Institute for Telecommunication Sciences (ITS)

ITS is the research and engineering laboratory of NTIA within the U.S. Department
of Commerce. It is the federal government's primary resource for
telecommunications testing, including speech quality and intelligibility
measurement. ITS developed the test plan for this study in conjunction with the
Testing Subcommittee of the IAFC Digital Problem Working Group (DPWG).

ITS maintains expertise in:
- Modified Rhyme Test (MRT) methodology per ANSI S3.2
- Subjective and objective speech quality evaluation (MOS, PESQ)
- Acoustic measurement in controlled laboratory environments
- Land mobile radio system performance characterization

The same ITS laboratory and methodology were used for the MOS experiments in
TSB-102.BABE (full-rate, 2007) and TSB-102.BABF (half-rate, 2008).

---

## Relationship to TIA-102.BABG

NTIA TR-08-453 is referenced in TIA-102.BABG as **informative reference [4.2.5]**.

### Six Fireground Noise Recordings

The primary concrete contribution of TR-08-453 to the P25 standard suite is
six fireground noise recordings that BABG adopted as standard test environments
for enhanced vocoder performance measurement:

| Noise Environment | Description | SNR in TR-08-453 |
|---|---|---|
| Fire Truck Pump Panel | Operating fire truck water pump | 4 dB |
| Dual PASS Alarm | Two simultaneous Personal Alert Safety System alarms | -2 dB (+ mask) |
| Low Air Alarm | SCBA mask low-air buzzer | 15 dB (+ mask) |
| Fog Nozzle | 2.5-inch fire hose with fog nozzle | 9 dB (+ mask) |
| Rotary Saw | Gas-powered rotary saw cutting metal garage door | 4 dB (+ mask) |
| Chainsaw | Gas-powered chainsaw cutting wood roof | 5 dB (+ mask) |

These recordings were originally captured at 48 kHz sample rate at a fire agency
training facility (City of Boise, Idaho and Town of Plainfield, Indiana).

In BABG, these six noises join noise environments sourced from TSB-102.BABE
(police siren, fire siren, highway, crowd noise, factory) and TSB-102.BABF
(fire truck without siren, helicopter, boat) to form a comprehensive test suite
of 15+ noise conditions that enhanced P25 vocoders must be evaluated against.

### How BABG Uses These Recordings

BABG converts the subjective listening test results from TR-08-453 (and from
BABE/BABF) into objective, automatable PESQ/LQO thresholds. Rather than
requiring a 30-person listening panel for each vocoder conformance test, BABG
ships the actual noise WAV files and defines PESQ score thresholds that a
conformant enhanced vocoder must achieve when processing speech corrupted by
these noises.

---

## Relationship to TSB-102.BABF

TSB-102.BABF ("Experiment 3 MOS Test Plan for Vocoder Technology for Project 25
Phase 2," 2008, now withdrawn) is the half-rate vocoder MOS test that was
conducted contemporaneously with TR-08-453. Both studies:

- Were conducted at ITS Boulder in the 2007-2008 timeframe
- Used noise environments relevant to public safety operations
- Fed their noise recordings into TIA-102.BABG's test vector suite
- Were withdrawn/superseded once BABG codified their results into objective
  thresholds (BABG published March 2010)

BABF focused on half-rate (AMBE+2) vocoder quality using MOS scoring, while
TR-08-453 focused on full-rate IMBE intelligibility using MRT scoring. The two
studies complement each other: BABF provides subjective quality ratings, while
TR-08-453 provides intelligibility measurements under extreme conditions.

BABF contributed different noise types (fire truck in motion, helicopter, boat)
while TR-08-453 contributed the fireground-specific noises listed above.

---

## Connection to Vocoder Analysis in This Project

### vocoder_wire_vs_codec.md

The TR-08-453 findings reinforce the central thesis of our
`analysis/vocoder_wire_vs_codec.md` analysis: the IMBE frame format (the wire
protocol) is distinct from the MBE speech synthesis algorithm (the codec). The
study tested two different implementations of the full-rate IMBE codec --
"P25 Full Rate" (QFA, the 1992 baseline with manufacturer speech processing)
and "P25 Enhanced Full Rate" (QFB, with post-1992 algorithmic improvements) --
both transmitting identical IMBE frames on the wire. The Enhanced Full Rate
system consistently performed equal to or better than the baseline, demonstrating
that vocoder algorithm improvements are possible without changing the air
interface format.

Specifically, the P25 Enhanced Full Rate system achieved:
- Statistically equivalent performance in 8 of 9 environments
- Statistically better performance in 1 environment (amplified mask)
- Never statistically worse than the baseline

This is empirical evidence that the synthesis algorithm matters independently of
the frame format -- exactly the point made in `vocoder_wire_vs_codec.md`.

### vocoder_missing_specs.md

TR-08-453 helps fill in the picture described in `analysis/vocoder_missing_specs.md`
regarding the gap between the original IMBE vocoder (TIA-102.BABA, 1992) and
the "enhanced" vocoders that P25 eventually required. The study was conducted
during the transition period (2008) when the P25 community was actively developing
enhanced vocoder requirements. The QFB "Enhanced Full Rate" codec tested in
TR-08-453 represents the class of enhanced vocoders that BABG would later set
objective performance thresholds for.

The report also references the Digital Voice Systems Incorporated (DVSI)
implementation of IMBE on the VC 20 Project 25 hardware card, connecting the
testing to the specific vendor implementation that dominated P25 vocoder
deployment.

---

## Practical Significance

### Real-World Fireground Conditions

This study is important because it measured vocoder performance under conditions
that actual firefighters face -- not laboratory-idealized noise. The noise
recordings were selected by a panel of fire practitioners from the IAFC and
recorded at actual fire training facilities. The SNR levels were calibrated to
match measured field conditions.

Key practical findings:

1. **SCBA mask alone degrades P25 digital intelligibility by 26-33 percentage
   points** (from ~80% to 52-59%), while analog FM drops only ~10 points. This
   means firefighters wearing SCBA masks will experience significantly worse
   digital radio intelligibility even in quiet environments.

2. **Four of nine fireground environments rendered ALL radio systems
   unintelligible** (below 10%). This includes rotary saw, chainsaw, fog nozzle,
   and low air alarm with mask. These are common fireground conditions.

3. **PASS alarms severely degrade vocoder performance.** The tonal characteristics
   of PASS alarms confuse the IMBE vocoder's pitch tracking, causing a 58% vs
   15-21% intelligibility gap between analog and digital systems. This is a
   safety-critical finding since PASS alarms activate when a firefighter is down.

4. **The Enhanced Full Rate vocoder offered modest but measurable improvement**
   over the baseline, particularly in the pump panel and amplified mask
   environments. This validated the direction of enhanced vocoder development.

### Listener Population

The 30 listeners were active fire service practitioners -- not laboratory
technicians. This was deliberate: the test measured intelligibility for the
actual user population. Listeners were required to have audiometrically normal
hearing, be naive with respect to communication technology, and represent mixed
sex, age, and discipline.

---

## References in BABG

| BABG Ref | Document | Role |
|---|---|---|
| 4.2.5 | NTIA TR 08-453 | Source of 6 fireground noise WAV files used as test vectors |

---

## Equipment and Organizations Involved

**Fire agencies providing noise recordings:**
- City of Boise, Idaho
- Town of Plainfield, Indiana

**Listener participant agencies:** Boise, Fairfax (VA), Plainfield (IN),
Littleton (CO), Plainfield (IL), Coeur d'Alene (ID), Philadelphia (PA),
Riverside (OH), Englewood (OH), Huber Heights (OH)

**Equipment providers:** Scott Health and Safety (SCBA), Motorola (radios),
Draeger Safety, EF Johnson (radios), International Safety Instruments

---

## Key Standards and Methods Referenced

| Reference | Description |
|---|---|
| TIA-102.BABA | P25 Vocoder Description -- defines the IMBE codec under test |
| ANSI S3.2 | Method for Measuring the Intelligibility of Speech over Communication Systems (MRT method) |
| NFPA 1981-2007 | Standard on Open-Circuit Self-Contained Breathing Apparatus -- requires MRT for SCBA evaluation |
| ITU-T P.56 | Objective measurement of active speech level |
| ITU-T P.800 | Methods for subjective determination of transmission quality |
