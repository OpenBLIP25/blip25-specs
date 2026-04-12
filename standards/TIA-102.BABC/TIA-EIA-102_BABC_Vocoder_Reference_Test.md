# TIA/EIA STANDARD

# Project 25 — Vocoder Reference Test

**TIA/EIA-102.BABC**
(Upgrade and Revision of TIA/EIA/IS-102.BABC)

**APRIL 1999**

TELECOMMUNICATIONS INDUSTRY ASSOCIATION
Representing the telecommunications industry in association with the Electronic Industries Alliance

ANSI/TIA/EIA-102.BABC-1999
Approved: March 16, 1999

---

## Foreword

This Vocoder Standard TIA/EIA-102.BABC, titled Project 25 Vocoder Reference Test, has been developed with inputs from the Project 25 Interface Committee (APIC), the APIC Vocoder Task Group, and TIA Industry Members. This Standard will be maintained by the TR-8.4 Committee under the sponsorship of TIA TR-8.

This Standard describes a vocoder reference test for codecs meeting the Project 25 requirements. For system implementations, such as Conventional, Trunking, Encryption, etc. this document provides only the appropriate test references to ensure vocoder compatibility, rather than the requisite specification detail for such implementations.

For information on specific system implementations, as they are developed, the reader is referred to [1] for a Project 25 Overview, a General System Model, other appropriate standards, a Glossary, and a statement of Requirements.

The reader's attention is called to the possibility that compliance with this Standard may require the use of one or more inventions covered by patent rights.

By publication of this Standard, no position is taken with respect to the validity of those claims or any patent rights in connection therewith. The patent holders so far identified have, however, filed statements of willingness to grant licenses under those rights on reasonable and nondiscriminatory terms and conditions to applicants desiring to obtain such licenses. Details may be obtained from the publisher.

---

## Table of Contents

- 1 Scope
- 2 Overview
- 3 A25VCTS Operation Manual
  - 3.1 Run Loopback Test
  - 3.2 Run Type Approval Test
    - 3.2.1 Run All Decoder Tests
    - 3.2.2 Run Individual Decoder Tests
      - 3.2.2.1 Test 1 - Decoder Frequency Response
      - 3.2.2.2 Test 2 - Medium Pitch, High Amplitude
      - 3.2.2.3 Test 3 - Medium Pitch, Medium Amplitude
      - 3.2.2.4 Test 4 - Medium Pitch, Low Amplitude
      - 3.2.2.5 Test 5 - High Pitch
      - 3.2.2.6 Test 6 - Low Pitch
      - 3.2.2.7 Test 7 - Correctable Bit Errors
      - 3.2.2.8 Test 8 - Frame Repeat Errors
      - 3.2.2.9 Test 9 - Delay
      - 3.2.2.10 Test A - Idle Channel Noise
      - 3.2.2.11 Test B - Crosstalk
      - 3.2.2.12 Test C - Out-of-Band Frequency Response
    - 3.2.3 Run All Encoder Tests
    - 3.2.4 Run Individual Encoder Tests
      - 3.2.4.1 Test 1 - Encoder Frequency Response
      - 3.2.4.2 Test 2 - Medium Pitch, High Amplitude
      - 3.2.4.3 Test 3 - Medium Pitch, Medium Amplitude
      - 3.2.4.4 Test 4 - Medium Pitch, Low Amplitude
      - 3.2.4.5 Test 5 - High Pitch
      - 3.2.4.6 Test 6 - Low Pitch
      - 3.2.4.7 Test 7 - Voiced/Unvoiced Accuracy
      - 3.2.4.8 Test 8 - Out of Band Energy
      - 3.2.4.9 Test 9 - Delay
      - 3.2.4.10 Test A - Idle Channel Noise
      - 3.2.4.11 Test B - Crosstalk Test
      - 3.2.4.12 Test C - Out-of-Band Frequency Response
- 4 Test Computer Hardware and I/O Circuitry
  - 4.1 IBM PC Compatible Computer
  - 4.2 The DSP cards and the I/O Interface Circuitry
- 5 Test Computer Software
  - 5.1 Loopback Test
  - 5.2 Decoder Type Approval Tests
    - 5.2.1 Test 1 - Decoder Frequency Response
    - 5.2.2 Test 2-8 - Measure Decoder Distortion and Pitch
    - 5.2.3 Test 9 - Delay
    - 5.2.4 Test A - Idle Channel Noise
    - 5.2.5 Test B - Crosstalk
    - 5.2.6 Test C - Out-of-Band Frequency Response
  - 5.3 Encoder Type Approval Tests
    - 5.3.1 Test 1 - Encoder Frequency Response
    - 5.3.2 Test 2-8 - Measure Encoder Distortion and Pitch
    - 5.3.3 Test 9 - Delay
    - 5.3.4 Test A - Idle Channel Noise
    - 5.3.5 Test B - Crosstalk
    - 5.3.6 Test C - Out-of-Band Frequency Response
- 6 Objective Performance Requirements
  - 6.1 Loopback Test
  - 6.2 Decoder Type Approval Tests
  - 6.3 Encoder Type Approval Tests
- 7 References

---

## 1 Scope

This document specifies one method that may be employed to test that implementations of TIA/EIA-102.BABA compatible speech codecs meet recommended minimum performance requirements. This speech codec is the Improved Multi-Band Excitation (IMBE™) system described in [2]. The IMBE speech codec is used to digitally encode the speech signal at 4400 bps or decode the speech signal back to analog audio. Forward error correcting codes (FEC) are added to this to increase the signaling rate to 7200 bps. Most of the tests in this document are for the speech codec, and some of the tests verify the conformance of the FEC. All of the tests use an interface operating with the combined FEC/codec signal at 7200 bps.

The need for this procedure arises from the fact that implementations of speech coders do not have to be bit exact to achieve compatibility. Since individual designers implement algorithms in different manners, it is not possible to create a standard set of test vectors to assess conformance to a standard. This document describes a series of tests which are used to test conformance to specific aspects of the specification. These test do not necessarily ensure that the speech codecs operate satisfactorily under all possible input signals, and it is still a requirement of the manufacturer to ensure that his implementation operates in a consistent manner. The purpose of these tests is to test for minimum performance levels and the manufacturer should attempt to provide the highest performance possible.

The testing of the codecs described in this document is based upon quantitative tests with specific pass/fail criteria. The major purpose of these tests is not to assess the performance between one manufacturer's encoder and decoder, but to assess the performance between any pairing of manufacturer's encoders and decoders. This interoperability issue is of a critical nature to end users. The overall purpose of this standard is to provide a means for measuring performance levels while trying to ensure proper inter-operability with other manufacturer's equipment.

The issue of inter-operation can only be definitively answered by testing all combinations of encoder/decoder pairings. With the number of equipment manufacturers expected to supply equipment this may well be a prohibitive task. This standard takes the approach of defining a reference speech codec and then each manufacturer tests his implementation relative to the reference. Specific parameters identified by the developer of the speech codec are then compared against the reference. The speech codec is said to be in conformance once the objective performance requirements have been satisfied.

This document requires use of proprietary test equipment which may be purchased from Digital Voice Systems Incorporated. An alternative method of testing using Mean Opinion Scores (MOS) is described in [4].

---

## 2 Overview

The Project 25 Voice Codec Test Set (A25VCTS) is a complete system for testing and qualifying Project 25 voice codecs. The primary focus of this system is Type Approval Testing and Performance Verification Testing of prototype and production voice codecs. The A25VCTS also may be used for automated factory testing, voice codec development testing, and remote terminal commissioning. The A25VCTS is divided into two primary components. The first component is the Test Computer which interfaces to the unit under test. The Test Computer and the Test Computer Software is used to perform and supervise Type Approval Testing and Performance Verification Testing. The second component, the I/O Interface Circuitry, is used to provide the circuitry necessary for connecting to the unit-under-test. The combination of these two components provides an automated system for evaluating the performance of a voice codec and comparing this performance against the appropriate objective performance requirements.

The remainder of this document is divided into three sections. Section 3 contains the Operation Manual for the Project 25 Voice Codec Test Set (A25VCTS). This section describes the operation of the Test Software and the procedure that must be followed to test a voice codec. Sections 4 and 5 contain descriptions of the Test Computer Hardware and Software, respectively. Section 6 lists the Objective Performance Requirements that are tested for each prototype voice codec.

The Project 25 standard reference voice codec is the VC-20-PRJ25 voice codec module, which is available separately from DVSI. The VC-20-PRJ25 may be used in any of the tests described in this document to provide measurement data to compare to the unit-under-test. Such comparisons may be used for diagnostic purposes, but do not contribute to any of the pass/fail criteria of these tests. For a complete description of the VC-20-PRJ25 reference voice codec, the VC-20-PRJ25 Manual should be consulted [3].

---

## 3 A25VCTS Operation Manual

The Test Computer is powered up by turning on the power switch located at the rear of the unit. After power up, the Test Computer performs memory checks and loads operating software from the hard disk. After completing the startup procedures, the Project 25 voice codec test software automatically begins execution and displays the following menu:

```
MAIN MENU

1 - Run Loopback Test
2 - Run Type Approval Test
0 - Quit

Enter Item Number (0-2)
```

### 3.1 Run Loopback Test

In the Loopback Test, the test equipment should be configured as shown in Figure 3-1. The unit-under-test should be looped-back from its analog output to its analog input. During the Loopback Test the test computer transmits 7.2 kbps channel data to the unit under test. The voice codec under test decodes the received data and loops the data back to the encoder. The test computer records the data from the encoder. The encoder data is analyzed to estimate the following parameters:

> **Figure 3-1: Configuration for Loopback Test**
> *Block diagram showing: DSP Boards (2) inside the Test Computer, connected to I/O Interface Circuitry, which connects to the Unit Under Test via both a Channel I/F and a Speech I/F. The Speech I/F shows a loopback connection from the unit's output back to its input.*

**Frequency Response** — The combined decoder output and encoder input frequency response is computed by comparing the received encoder data with a stored reference. The computed frequency response is displayed as amplitude (dB) vs. frequency (0 - 4000 Hz). For digital loopback, this curve should be flat at 0 dB. The upper bound for the frequency response is displayed as a magenta curve. The lower bound for the frequency response is displayed as a red curve. The measured frequency response must be within these bounds to pass the frequency response test.

**Gain** — computed as the ratio between the measured energy and the reference energy.

**Spectral Distortion** — a measure of the dissimilarity between the measured spectrum and the reference spectrum expressed in dB. When the two spectra are very similar, a large negative number results. When the two spectra are dissimilar, a number near 0 dB results.

**Period Mean Deviation** — the average difference between the measured pitch period and the true pitch period in samples.

**Period Standard Deviation** — the standard deviation of the measured pitch period in samples.

**Maximum Deviation** — the maximum error of the measured pitch period from the true value in samples squared.

**Pitch Errors** — the percentage of frames that contain pitch errors large enough to alter the number of harmonics from the correct value.

**Unvoiced Errors** — the percentage of unvoiced harmonics that are declared voiced.

**Voiced Errors** — the percentage of voiced harmonics that are declared unvoiced.

The requirements for these parameters are specified in Section 6.1.

### 3.2 Run Type Approval Test

> **Figure 3-2: Configuration for Type Approval Tests**
> *Block diagram showing: DSP Boards (2) inside the Test Computer, connected to I/O Interface Circuitry, which connects to the Unit Under Test via both a Channel I/F and a Speech I/F. Unlike the loopback configuration, there is no loopback on the Speech I/F.*

When the Run Type Approval Tests option is selected, the test equipment should be configured as shown in Figure 3-2. The following menu is displayed:

```
TYPE APPROVAL TESTS

1 - Run All Decoder Tests
2 - Run Individual Decoder Tests
3 - Run All Encoder Tests
4 - Run Individual Encoder Tests
5 - Continuous Testing Until Failure
0 - Quit

Enter Item Number (0-4)
```

#### 3.2.1 Run All Decoder Tests

All of the decoder tests listed in Section 3.2.2 are executed sequentially. After all decoder tests are executed, a summary of the results of the decoder tests is displayed.

#### 3.2.2 Run Individual Decoder Tests

Before running these tests, the test equipment should be configured as shown in Figure 3-2. The performance requirements for the parameters measured in the decoder tests are given in Section 6.2. When this option is selected, the following menu is displayed:

```
DECODER TYPE APPROVAL TESTS

1 - Test 1 - Decoder Frequency Response
2 - Test 2 - Medium Pitch, High Amplitude
3 - Test 3 - Medium Pitch, Medium Amplitude
4 - Test 4 - Medium Pitch, Low Amplitude
5 - Test 5 - High Pitch
6 - Test 6 - Low Pitch
7 - Test 7 - Correctable Bit Errors
8 - Test 8 - Frame Repeat Errors
9 - Test 9 - Delay
A - Test A - Idle Channel Noise
B - Test B - Crosstalk
C - Test C - Out-of-Band Frequency Response
0 - Quit

Enter Item Number (0-C)
```

##### 3.2.2.1 Test 1 - Decoder Frequency Response

When this option is selected, the decoder frequency response and gain are measured and displayed on the screen. The measured frequency response in dB is shown as a yellow curve for the frequency range 0 - 4000 Hz. The minimum and maximum frequency response curves are also displayed in red and magenta. If the measured frequency response falls within the acceptable limits, the word "Pass" is displayed on the line containing the words "Frequency Response:", otherwise, the word "Fail" is displayed. The gain of the decoder under test is also displayed. This test must be run before running Decoder Tests 2-8.

##### 3.2.2.2 Test 2 - Medium Pitch, High Amplitude

When this option is selected, a sequence of bits corresponding to a medium pitch and a high amplitude is sent to the decoder. The decoder output waveform is recorded and analyzed. While the test computer is recording and analyzing data, the word "Working" is displayed in the center or upper right corner of the screen. The average spectrum of the decoder output is computed and compared with a stored reference spectrum that has been compensated by the decoder frequency response and gain measured in Test 1.

The measured spectrum is displayed in yellow as a function of frequency (0 - 8000 Hz). The compensated reference spectrum is displayed in magenta as a function of frequency (0 - 4000 Hz). The gain required to match the measured spectrum with the compensated reference spectrum is computed and displayed. The distortions in the voiced and unvoiced regions of the spectrum are computed and displayed. In band energy is computed as the sum of the energy in the range 0-4000 Hz. Out of band energy is computed as the sum of the energy in the range 4000-8000 Hz.

In the following screen, the pitch period of the decoder output is computed and displayed for a 1 second interval. The mean error between the measured pitch period and the reference pitch period is computed and displayed. The standard deviation and maximum deviation of the measured pitch period are computed and displayed.

The decoder output correlation at the measured pitch period is also computed and displayed for a 1 second interval. This is a measure of periodicity which is 1 for a perfectly periodic signal and approaches 0 for aperiodic signals.

##### 3.2.2.3 Test 3 - Medium Pitch, Medium Amplitude

When this option is selected, the tests described in Section 3.2.2.2 are executed using a sequence of bits corresponding to a medium pitch and a medium amplitude (approximately 10 dB below high amplitude).

##### 3.2.2.4 Test 4 - Medium Pitch, Low Amplitude

When this option is selected, the tests described in Section 3.2.2.2 are executed using a sequence of bits corresponding to a medium pitch and a low amplitude (approximately 20 dB below high amplitude).

##### 3.2.2.5 Test 5 - High Pitch

When this option is selected, the tests described in Section 3.2.2.2 are executed using a sequence of bits corresponding to a high pitch and a high amplitude.

##### 3.2.2.6 Test 6 - Low Pitch

When this option is selected, the tests described in Section 3.2.2.2 are executed using a sequence of bits corresponding to a low pitch and a high amplitude.

##### 3.2.2.7 Test 7 - Correctable Bit Errors

When this option is selected, the tests described in Section 3.2.2.2 are executed using a sequence of bits corresponding to a medium pitch and a high amplitude with correctable bit errors in all of the codes.

##### 3.2.2.8 Test 8 - Frame Repeat Errors

When this option is selected, the tests described in Section 3.2.2.2 are executed using a sequence of bits corresponding to a medium pitch and a high amplitude with sufficient errors to cause frame repeats.

##### 3.2.2.9 Test 9 - Delay

When this option is selected, a sequence of bits is sent to the decoder which produces a pulsed waveform. The delay between the end of the subframe containing the pulse and the beginning of the pulse in the decoder output is measured. The mean and standard deviation of the delay measurements are computed and displayed.

##### 3.2.2.10 Test A - Idle Channel Noise

When this option is selected, the amount of noise on an idle decoder channel is measured. A sequence of bits is sent to the decoder which is designated to force the decoder's analog output to zero. The analog output from the decoder is measured and analyzed to determine the resulting noise level. The single frequency maximum noise level is computed and compared to the threshold shown on the graph in magenta. The total noise level is computed as the sum of the energy from 0-8 kHz.

##### 3.2.2.11 Test B - Crosstalk

When this option is selected, the encoder-to-decoder crosstalk is measured. A sequence of bits is sent to the decoder which is designated to force the decoder's analog output to zero, while the encoder's analog input is exercised with a complex harmonic excitation signal. The amount of energy that is output from the decoder's analog output is measured and analyzed to determine the amount of crosstalk from the encoder side.

##### 3.2.2.12 Test C - Out-of-Band Frequency Response

When this option is selected, the out-of-band frequency response is measured. A sequence of bits that represents a complex excitation signal is sent to the decoder digital input. The decoder's analog output is measured and analyzed to determine the out-of-band frequency response of the decoder side. The maximum limit of the decoder spectrum is shown in magenta. Since energy cannot be generated by the decoder between 3800 and 4200 Hz, the limit is pushed down in the 4000-4200 Hz range to account for the lack of excitation energy in this region.

#### 3.2.3 Run All Encoder Tests

When this option is selected from the menu, all of the encoder tests listed in Section 3.2.4 are executed sequentially. After all encoder tests are executed, a summary of the results of the encoder tests is displayed.

#### 3.2.4 Run Individual Encoder Tests

Before running the encoder tests, the test equipment should be configured as shown in Figure 3-2. When this option is selected from the menu, the following menu is displayed:

```
ENCODER TYPE APPROVAL TESTS

1 - Test 1 - Encoder Frequency Response
2 - Test 2 - Medium Pitch, High Amplitude
3 - Test 3 - Medium Pitch, Medium Amplitude
4 - Test 4 - Medium Pitch, Low Amplitude
5 - Test 5 - High Pitch
6 - Test 6 - Low Pitch
7 - Test 7 - Voiced/Unvoiced Accuracy
8 - Test 8 - Out of Band Energy
9 - Test 9 - Delay
A - Test A - Idle Channel Noise
B - Test B - Crosstalk
C - Test C - Out-of-Band Frequency Response
0 - Quit

Enter Item Number (0-C)
```

##### 3.2.4.1 Test 1 - Encoder Frequency Response

When this option is selected, the encoder frequency response and gain are measured and displayed on the screen. The measured frequency response in dB is shown as a yellow curve for the frequency range 0 - 4000 Hz. The minimum and maximum frequency response curves are also displayed in red and magenta. If the measured frequency response falls within the acceptable limits, the word "Pass" is displayed on the line containing the words "Frequency Response:", otherwise, the word "Fail" is displayed. The gain of the encoder under test is also displayed. This test must be run before running Encoder Tests 2-8.

##### 3.2.4.2 Test 2 - Medium Pitch, High Amplitude

When this option is selected, an analog signal with medium pitch and high amplitude is sent to the encoder. The encoder output data is recorded and analyzed. While the test computer is recording and analyzing data, the word "Working" is displayed in the center or upper right corner of the screen. The average spectrum of the encoder output is computed and compared with a stored reference spectrum which has been compensated by the encoder frequency response and gain measured in Test 1. The measured and compensated reference spectra are displayed as yellow and magenta curves from 0 to 4000 Hz. The gain required to match the measured spectrum with the compensated reference spectrum is computed and displayed. The spectral distortion between the measured and reference spectrum is computed and displayed. The mean, standard, and maximum deviation of the measured pitch period from the true pitch period are computed and displayed.

##### 3.2.4.3 Test 3 - Medium Pitch, Medium Amplitude

When this option is selected, the tests described in Section 3.2.4.2 are executed using a signal with a medium pitch and a medium amplitude (10 dB below high amplitude).

##### 3.2.4.4 Test 4 - Medium Pitch, Low Amplitude

When this option is selected, the tests described in Section 3.2.4.2 are executed using a signal with a medium pitch and a low amplitude (20 dB below high amplitude).

##### 3.2.4.5 Test 5 - High Pitch

When this option is selected, the tests described in Section 3.2.4.2 are executed using a signal with a high pitch and a high amplitude.

##### 3.2.4.6 Test 6 - Low Pitch

When this option is selected, the tests described in Section 3.2.4.2 are executed using a signal with a low pitch and a high amplitude.

##### 3.2.4.7 Test 7 - Voiced/Unvoiced Accuracy

When this option is selected, the tests described in Section 3.2.4.2 are executed using a signal with 4 alternating bands of voiced and unvoiced harmonics. An additional test is executed for this signal to determine the accuracy of encoder voiced and unvoiced decisions. At the conclusion of this test, the percentage of unvoiced errors (unvoiced harmonics declared voiced) and voiced errors (voiced harmonics declared unvoiced) are displayed.

##### 3.2.4.8 Test 8 - Out of Band Energy

When this option is selected, the tests described in Section 3.2.4.2 are executed using a signal with a medium pitch and a high amplitude with additive wideband noise from 4100 to 8000 Hz. The energy of the out of band noise is approximately equal to the energy of the in band signal. This test is intended to expose insufficient A/D anti-aliasing filters.

##### 3.2.4.9 Test 9 - Delay

When this option is selected, a short pulse waveform is sent to the encoder. The delay between the pulse and the beginning of the subframe of data from the encoder containing the pulse is measured and displayed.

##### 3.2.4.10 Test A - Idle Channel Noise

When this option is selected, the channel noise on the encoder side is tested. Dual-frequency band analog signals are input to the encoder and the digital output signals are measured. In the first phase of the test, the lower frequency band contains a signal with enough amplitude to cause voice detection in the encoder, and the higher band consists of silence. The noise levels are then measured in the higher frequency band. In the second phase of the test, the lower frequency band consists of silence, and the higher band contains a signal with enough amplitude to cause voice detection in the encoder. The noise levels are then measured in the lower frequency band. The digital data outputs from both phases of the test are then analyzed to determine the resulting noise levels. The single frequency maximum noise level is computed and compared to the threshold shown on the graph in magenta. The total noise level is computed as the sum of the energy from 0 to 4 kHz.

##### 3.2.4.11 Test B - Crosstalk Test

When this option is selected, the decoder-to-encoder crosstalk is tested. Dual-frequency band analog signals are input to the encoder, while exercising the decoder's digital input with an encoded complex (harmonic) excitation signal. In the first phase of the test, the lower frequency band contains a signal with enough amplitude to cause voice detection in the encoder, and the higher band consists of silence. The crosstalk levels are then measured in the higher frequency band. In the second phase of the test, the lower frequency band of the encoder's analog input signal consists of silence, and the higher band contains a signal with enough amplitude to cause voice detection in the encoder. The crosstalk levels are then measured in the lower frequency band. The results are then analyzed to determine the amount of crosstalk generated from the decoder side onto the encoder side.

##### 3.2.4.12 Test C - Out-of-Band Frequency Response

When this option is selected, the encoder's out-of-band frequency response is measured and analyzed. Dual-frequency band analog signals are input to the encoder between the frequencies of 0-4 kHz, while noise is continually input between the frequencies of 4 kHz and 8 kHz. In the first phase of the test, the lower frequency part of the 0-4 kHz band contains a signal with enough amplitude to cause voice detection in the encoder, and the higher part of the 0-4 kHz band consists of silence. In the second phase of the test, the lower frequency band of the encoder's analog input signal consists of silence, and the higher band contains a signal with enough amplitude to cause voice detection in the encoder. The results are then analyzed to determine the out-of-band frequency response.

Since the encoder does not directly encode energy at frequencies above 4 kHz, the out-of-band frequency response is measured as the image of the 4-8 kHz band in the 0-4 kHz band due to sampling.

---

## 4 Test Computer Hardware and I/O Circuitry

The Test Computer is responsible for supervising objective testing of prototype Project 25 voice codecs. The Test Computer hardware has been selected to provide high performance and flexibility. The I/O circuitry is used to provide the appropriate channel and analog interface connections for the unit-under-test. Each test computer and I/O unit comprises the following items: IBM PC compatible computer, DSP32C cards in PC card slots, and I/O Interface Circuitry.

### 4.1 IBM PC Compatible Computer

DVSI recommends that at a minimum, the IBM-PC compatible computer contain a 33 MHz 486 processor, 4 MB memory, 50 MB free hard disk space, two free 16 bit ISA slots, 640x480 VGA graphics, a 3.25" floppy drive, and a keyboard.

More information on these components can be found in the manuals provided by the hardware manufacturers.

### 4.2 The DSP cards and the I/O Interface Circuitry

The DSP cards and the I/O interface circuitry are used for signal processing and analysis and for I/O control and interfacing. The DSP intensive tasks of the Test Computer Software are performed on the two DSP PC-based DSP boards in the computer. The I/O Circuitry interfaces one of the DSP serial ports to the digital channel interface of the unit-under test. It also interfaces the other DSP serial port to the analog interface of the unit-under test (please refer to Figures 3-1 and 3-2).

---

## 5 Test Computer Software

The Test Computer Software consists of components for the IBM PC compatible and the two DSP32C cards. Since the DSP32C cards are connected to the digital and analog interfaces and are faster at numeric processing than the PC, they are used for both data collection and analysis. The PC is used for control, file I/O, and display. The three major functions of the test computer software are the Loopback Test, the Decoder Type Approval Tests, and the Encoder Type Approval Tests. These are described in the following sections.

### 5.1 Loopback Test

In this test, the following steps are performed:

Set up DSP1 to digitally loop back A/D output to D/A input. This allows the test computer to perform the loop back of decoder output to encoder input if desired.

Set up DSP0 to repeatedly send a 144-bit subframe of data to the decoder. The decoder output must be looped back to the encoder input. After waiting for the encoder output to reach steady state, encoder data is collected and analyzed by DSP0. The gain is computed by dividing the measured energy in the band 494-2965 Hz by the reference energy in the same band. The reference signal is determined using a software simulation of the decoder and encoder. The spectral distortion is computed as:

```
         Σ (x[i] - G·y[i])²
D = 10·log₁₀ ─────────────────────
              Σ x[i]²
```

where x[i] are the reference harmonic magnitudes and y[i] are the measured harmonic magnitudes.

### 5.2 Decoder Type Approval Tests

The Decoder Type Approval Tests first measure the decoder frequency response and gain (Test 1). The measured frequency response and gain is then used to compensate the reference spectrum in Tests 2-8. The Decoder Delay is measured in Test 9.

#### 5.2.1 Test 1 - Decoder Frequency Response

The Decoder Frequency Response is measured using the following steps:

Set up DSP0 to repeatedly send a group of three 144 bit subframes to the decoder under test. These subframes produce a relatively flat, completely unvoiced spectrum with period 64.75 samples.

Set up DSP1 to wait until the decoder output has reached steady state and then digitize the decoder output at 16 kHz.

Use DSP1 to analyze the decoder output by computing the average of the magnitude squared of a 1024 point FFT of the windowed data. The windowed data is computed every 512 points by multiplying the data by a 1024 point Hamming window. The gain is computed by dividing the energy in the measured spectrum for the band 0 - 3000 Hz by the energy in the reference spectrum for the same band. The frequency response at each harmonic of the fundamental frequency (multiples of 123.55 Hz) is estimated by dividing the energy in the measured spectrum around the harmonic by the energy in the reference spectrum around the same harmonic. When samples of the decoder frequency response at other frequencies are required, they are generated by linear interpolation.

#### 5.2.2 Test 2-8 - Measure Decoder Distortion and Pitch

The Decoder Distortion and Pitch are measured using the following steps:

Set up DSP0 to repeatedly send a 144 bit subframe to the Decoder under test.

Set up DSP1 to record the analog output of the decoder under test.

Set up DSP1 to analyze the recorded decoder output. The gain (G) is computed to minimize the difference between the measured (y[i]) and reference (x[i]) spectrum according to:

```
      Σ y[i]²
G = ─────────
      Σ x[i]²
```

The voiced distortion is measured as:

```
           Σ v[i]·(x[i] - G·y[i])²
Dv = 10·log₁₀ ─────────────────────────
                Σ v[i]·x[i]²
```

where v[i] = 1 for voiced portions of the spectrum and v[i] = 0 for unvoiced portions.

The unvoiced distortion is measured as:

```
           Σ u[i]·(x[i] - G·y[i])²
Du = 10·log₁₀ ─────────────────────────
                Σ u[i]·x[i]²
```

where u[i] = 1 for unvoiced portions of the spectrum and u[i] = 0 for voiced portions.

Set up DSP1 to compute the pitch period of one second of the recorded decoder output. The pitch period is estimated every 20 ms by choosing the period (P) to minimize:

```
      159
E = Σ (x[4i] - x[4i + P])²
     i=0
```

where x[i] is the upsampled digitized data. The data is upsampled by inserting a zero between samples and low pass filtering in order to achieve .25 sample pitch period accuracy. The correlation (C) at the estimated pitch period is also computed as a measure of the periodicity of the waveform according to the following:

```
                    159
                  ( Σ x[4i]·x[4i + P] )²
                   i=0
C = ──────────────────────────────────────────
      159                 159
    ( Σ x[4i]² )  ·  ( Σ x[4i + P]² )
     i=0               i=0
```

The correlation is one for a perfectly periodic waveform and approaches zero for aperiodic waveforms.

#### 5.2.3 Test 9 - Delay

The Decoder Delay is measured using the following steps:

Setup DSP1 to enable it to start recording data after receiving a signal.

Setup DSP0 to start sending subframes to the decoder which consist primarily of a low level steady state. One subframe is sent with a large gain increase followed by one subframe with a large gain decrease. This produces a pulsed output from the decoder. This pulsed data is repeatedly sent to the decoder. As soon as the data starts to be transmitted to the decoder, DSP1 is signaled to start recording data.

Setup DSP1 to analyze the recorded decoder output data. The start and stop time of each decoder output pulse is computed and used to estimate the decoder delay from the end of the transmitted pulse subframe to the beginning of the decoder output pulse.

#### 5.2.4 Test A - Idle Channel Noise

The Decoder Idle Channel Noise is measured using the following steps:

Set up DSP0 to repeatedly send a 144 bit subframe to the Decoder under test forcing its output to zero.

Set up DSP1 to record the analog output of the decoder under test.

Use DSP1 to analyze the decoder output by computing the average of the magnitude squared of a 1024 point FFT of the windowed data. The windowed data is computed every 512 points by multiplying the data by a 1024 point Hamming window. The maximum and total energy are computed.

#### 5.2.5 Test B - Crosstalk

The Encoder to Decoder Crosstalk is measured using the following steps:

Set up DSP0 to repeatedly send a 144 bit subframe to the Decoder under test forcing its output to zero.

Set up DSP1 to repeatedly output a signal to the encoder analog input and record the analog output of the decoder under test.

Use DSP1 to analyze the decoder output by computing the average of the magnitude squared of a 1024 point FFT of the windowed data. The windowed data is computed every 512 points by multiplying the data by a 1024 point Hamming window. The maximum and total energy are computed.

#### 5.2.6 Test C - Out-of-Band Frequency Response

The Decoder Out-of-Band Frequency Response is measured using the following steps:

Set up DSP0 to repeatedly send a group of three 144 bit subframes to the decoder under test. These subframes produce a relatively flat, completely unvoiced spectrum.

Set up DSP1 to record the analog output of the decoder under test.

Use DSP1 to analyze the decoder output by computing the average of the magnitude squared of a 1024 point FFT of the windowed data. The windowed data is computed every 512 points by multiplying the data by a 1024 point Hamming window.

### 5.3 Encoder Type Approval Tests

The Encoder Type Approval Tests first measure the encoder frequency response and gain (Test 1). The measured frequency response and gain is then used to compensate the reference spectrum in Tests 2-8. The Encoder Delay is measured in Test 9.

#### 5.3.1 Test 1 - Encoder Frequency Response

The Encoder Frequency Response is measured using the following steps:

Set up DSP1 to repeatedly play a signal with period 64.75 samples through the D/A converter.

Set up DSP0 to wait until the encoder output has reached steady state and then record the encoder output data.

Use DSP1 to analyze the encoder output by computing the average of the harmonic magnitudes. The gain is computed by dividing the energy in the measured average magnitudes for the band 0 to 3000 Hz by the energy in the reference magnitudes for the same band. The frequency response at each harmonic of the fundamental frequency (multiples of 123.55 Hz) is estimated by dividing the average measured magnitude at the harmonic by the gain compensated reference magnitude. When samples of the encoder frequency response at other frequencies are required, they are generated by linear interpolation.

#### 5.3.2 Test 2-8 - Measure Encoder Distortion and Pitch

The Encoder Distortion and Pitch are measured using the following steps:

Set up DSP1 to repeatedly play a periodic signal through the D/A converter.

Set up DSP0 to record the encoder output data after waiting for the encoder to reach steady state.

Set up DSP1 to analyze the recorded encoder output. The gain (G) is computed to minimize the difference between the measured (y[i]) and reference (x[i]) magnitudes according to:

```
      Σ y[i]²
G = ─────────
      Σ x[i]²
```

The distortion (D) is measured as:

```
         Σ (x[i] - G·y[i])²
D = 10·log₁₀ ─────────────────────
              Σ x[i]²
```

The encoder pitch period is sent as part of the encoder data.

#### 5.3.3 Test 9 - Delay

The Encoder Delay is measured using the following steps:

Setup DSP1 to enable it to start sending a .125 ms pulse followed by silence after receiving a signal. The repetition period of the pulse is 240.125 ms so that it sequentially occupies each sample position in the 20 ms encoder input frame. This significantly improves the accuracy of the encoder delay measurement.

Setup DSP0 to start recording data from the encoder. When a frame pulse occurs, DSP1 is signaled to start and DSP0 starts recording data.

Setup DSP1 to analyze the recorded encoder output data. The time delay between the pulse and the start of the received encoder data containing the pulse is measured and averaged over all pulse periods.

#### 5.3.4 Test A - Idle Channel Noise

The Encoder Idle Channel Noise is measured using the following steps:

Set up DSP1 to repeatedly play a signal with period 64.75 samples and harmonics only in the lower portion of the 0-4 kHz band through the D/A converter.

Set up DSP0 to wait until the encoder output has reached steady state and then record the encoder output data.

Use DSP1 to analyze the encoder output by computing the average of the harmonic magnitudes for the upper portion of the band.

Set up DSP1 to repeatedly play a signal with period 64.75 samples and harmonics only in the upper portion of the 0-4 kHz band through the D/A converter.

Set up DSP0 to wait until the encoder output has reached steady state and then record the encoder output data.

Use DSP1 to analyze the encoder output by computing the average of the harmonic magnitudes for the lower portion of the band.

#### 5.3.5 Test B - Crosstalk

The Decoder to Encoder Crosstalk is measured using the following steps:

Set up DSP1 to repeatedly play a signal with period 64.75 samples and harmonics only in the lower portion of the 0-4 kHz band through the D/A converter.

Set up DSP0 to send bits repeatedly to decoder and record the encoder output data.

Use DSP1 to analyze the encoder output by computing the average of the harmonic magnitudes for the upper portion of the band.

Set up DSP1 to repeatedly play a signal with period 64.75 samples and harmonics only in the upper portion of the 0-4 kHz band through the D/A converter.

Set up DSP0 to send bits repeatedly to decoder and record the encoder output data.

Use DSP1 to analyze the encoder output by computing the average of the harmonic magnitudes for the lower portion of the band.

#### 5.3.6 Test C - Out-of-Band Frequency Response

The Encoder Out-of-Band Frequency Response is measured using the following steps:

Set up DSP1 to repeatedly play a signal with period 64.75 samples and harmonics only in the lower portion of the 0-4 kHz band with white noise in the 4-8 kHz band through the D/A converter.

Set up DSP0 to wait until the encoder output has reached steady state and then record the encoder output data.

Use DSP1 to analyze the encoder output by computing the average of the harmonic magnitudes for the upper portion of the band.

Set up DSP1 to repeatedly play a signal with period 64.75 samples and harmonics only in the upper portion of the 0-4 kHz band with white noise in the 4-8 kHz band through the D/A converter.

Set up DSP0 to wait until the encoder output has reached steady state and then record the encoder output data.

Use DSP1 to analyze the encoder output by computing the average of the harmonic magnitudes for the lower portion of the band.

---

## 6 Objective Performance Requirements

### 6.1 Loopback Test

Table 6-1 shows the objective performance requirements for the parameters measured during the Loopback Test. The frequency response bounds are shown in Table 6-2. Values at frequencies other than those shown are obtained by linear interpolation.

**Table 6-1: Loopback Test Performance Requirements**

| Parameter | Requirement |
|---|---|
| Frequency Response - H(f) | H_min(f) ≤ H(f) ≤ H_max(f) |
| Gain | -10 dB ≤ x ≤ 10 dB |
| Spectral Distortion | x ≤ -11 dB |
| Period: Mean Deviation | -0.5 Samples ≤ x ≤ 0.5 Samples |
| Period: Standard Deviation | x ≤ 0.25 Samples |
| Period: Maximum Deviation | x ≤ 2.0 Samples |
| Unvoiced Errors | x ≤ 35 % |
| Voiced Errors | x ≤ 10 % |

**Table 6-2: Frequency Response Bounds for Loopback Test**

| Frequency (f) Hz | H_min(f) dB | H_max(f) dB |
|---|---|---|
| 0 | -infinity | 2 |
| 100 | -infinity | 2 |
| 200 | -infinity | 2 |
| 300 | -2 | 2 |
| 400 | -2 | 2 |
| 500 | -2 | 2 |
| 600 | -2 | 2 |
| 700 | -2 | 2 |
| 800 | -2 | 2 |
| 900 | -2 | 2 |
| 1000 | -2 | 2 |
| 1100 | -2 | 2 |
| 1200 | -2 | 2 |
| 1300 | -2 | 2 |
| 1400 | -2 | 2 |
| 1500 | -2 | 2 |
| 1600 | -2 | 2 |
| 1700 | -2 | 2 |
| 1800 | -2 | 2 |
| 1900 | -2 | 2 |
| 2000 | -2 | 2 |
| 2100 | -2.2 | 2 |
| 2200 | -2.4 | 2 |
| 2300 | -2.6 | 2 |
| 2400 | -2.8 | 2 |
| 2500 | -3.0 | 2 |
| 2600 | -3.2 | 2 |
| 2700 | -3.4 | 2 |
| 2800 | -3.6 | 2 |
| 2900 | -3.8 | 2 |
| 3000 | -4 | 2 |
| 3100 | -5 | 2 |
| 3200 | -6 | 2 |
| 3300 | -7 | 2 |
| 3400 | -8 | 2 |
| 3500 | -infinity | 2 |
| 3600 | -infinity | 2 |
| 3700 | -infinity | 2 |
| 3800 | -infinity | 2 |
| 3900 | -infinity | 2 |
| 4000 | -infinity | 2 |

### 6.2 Decoder Type Approval Tests

Table 6-3 shows the objective performance requirements for the parameters measured during the Decoder Type Approval Tests. Several of the low pitch test (test 6) requirements are lower due to the increased harmonic phase variation in this test. The frequency response bounds are shown in Table 6-4. Values at frequencies other than those shown are obtained by linear interpolation.

**Table 6-3: Decoder Type Approval Tests Performance Requirements**

| Parameter | Requirement |
|---|---|
| Frequency Response - H(f) | H_min(f) ≤ H(f) ≤ H_max(f) |
| Gain - Test 1 | -7 dB ≤ x ≤ 1 dB |
| Gain - Test 2-8 | -0.75 dB ≤ x ≤ 0.75 dB |
| Voiced Distortion (not High Pitch) | x ≤ -20 dB |
| Voiced Distortion (High Pitch) | x ≤ -15 dB |
| Unvoiced Distortion (not Low Pitch) | x ≤ -20 dB |
| Unvoiced Distortion (Low Pitch) | x ≤ -15 dB |
| Out of Band Energy | x ≤ In Band Energy - 30 dB |
| Pitch Period (not Low Pitch): Mean Error | -0.25 Samples ≤ x ≤ 0.25 Samples |
| Pitch Period (not Low Pitch): Standard Deviation | x ≤ 0.25 Samples |
| Pitch Period (not Low Pitch): Maximum Error | x ≤ 0.5 Samples |
| Pitch Period (Low Pitch only): Mean Error | -0.5 Samples ≤ x ≤ 0.5 Samples |
| Pitch Period (Low Pitch only): Standard Deviation | x ≤ 0.5 Samples |
| Pitch Period (Low Pitch only): Maximum Error | x ≤ 1.0 Samples |
| Correlation (not Low Pitch): Mean | x ≥ 0.97 |
| Correlation (not Low Pitch): Standard Deviation | x ≤ 0.02 |
| Correlation (not Low Pitch): Minimum | x ≥ 0.95 |
| Correlation (Low Pitch only): Mean | x ≥ 0.9 |
| Correlation (Low Pitch only): Standard Deviation | x ≤ 0.1 |
| Correlation (Low Pitch only): Minimum | x ≥ 0.6 |
| Delay - Mean | x ≤ 40 ms |
| Delay - Std. Dev. | x ≤ 0.5 ms |
| Single Frequency Max. Noise Level | x ≤ -65 dBmO |
| Total Noise Level | x ≤ -60 dBmO |
| Single Frequency Max. Crosstalk Level | x ≤ -62 dBmO |
| Total Crosstalk Level | x ≤ -57 dBmO |

**Table 6-4: In-Band Frequency Response Bounds for Decoder and Encoder Tests**

| Frequency (f) Hz | H_min(f) dB | H_max(f) dB |
|---|---|---|
| 0 | -infinity | 2 |
| 100 | -infinity | 2 |
| 200 | -infinity | 2 |
| 300 | -1 | 2 |
| 400 | -1 | 2 |
| 500 | -1 | 2 |
| 600 | -1 | 2 |
| 700 | -1 | 2 |
| 800 | -1 | 2 |
| 900 | -1 | 2 |
| 1000 | -1 | 2 |
| 1100 | -1 | 2 |
| 1200 | -1 | 2 |
| 1300 | -1 | 2 |
| 1400 | -1 | 2 |
| 1500 | -1 | 2 |
| 1600 | -1 | 2 |
| 1700 | -1 | 2 |
| 1800 | -1 | 2 |
| 1900 | -1 | 2 |
| 2000 | -1 | 2 |
| 2100 | -1.1 | 2 |
| 2200 | -1.2 | 2 |
| 2300 | -1.3 | 2 |
| 2400 | -1.4 | 2 |
| 2500 | -1.5 | 2 |
| 2600 | -1.6 | 2 |
| 2700 | -1.7 | 2 |
| 2800 | -1.8 | 2 |
| 2900 | -1.9 | 2 |
| 3000 | -2 | 2 |
| 3100 | -2.5 | 2 |
| 3200 | -3 | 2 |
| 3300 | -3.5 | 2 |
| 3400 | -4 | 2 |
| 3500 | -infinity | 2 |
| 3600 | -infinity | 2 |
| 3700 | -infinity | 2 |
| 3800 | -infinity | 2 |
| 3900 | -infinity | 2 |
| 4000 | -infinity | 2 |

**Table 6-5: Out-of-Band Frequency Response Bounds for Decoder and Encoder Tests**

| Frequency (f) Hz | H_max(f) dB |
|---|---|
| 4000 | -11 |
| 4100 | -15 |
| 4200 | -19 |
| 4300 | -23 |
| 4400 | -27 |
| 4500 | -30 |
| 4600 | -32 |
| 4700 | -34 |
| 4800 | -35 |
| 4900 | -36 |
| 5000 | -37 |
| 5100 | -38 |
| 5200 | -39 |
| 5300 | -40 |
| 5400 | -41 |
| 5500 | -42 |
| 5600 | -43 |
| 5700 | -44 |
| 5800 | -45 |
| 5900 | -46 |
| 6000 | -47 |
| 6100 | -48 |
| 6200 | -49 |
| 6300 | -50 |
| 6400 | -50 |
| 6500 | -50 |
| 6600 | -50 |
| 6700 | -50 |
| 6800 | -50 |
| 6900 | -50 |
| 7000 | -50 |
| 7100 | -50 |
| 7200 | -50 |
| 7300 | -50 |
| 7400 | -50 |
| 7500 | -50 |
| 7600 | -50 |
| 7700 | -50 |
| 7800 | -50 |
| 7900 | -50 |
| 8000 | -50 |

### 6.3 Encoder Type Approval Tests

Table 6-6 shows the objective performance requirements for the parameters measured during the Encoder Type Approval Tests. The frequency response bounds are shown in Table 6-4. Values at frequencies other than those shown are obtained by linear interpolation.

**Table 6-6: Encoder Type Approval Tests Performance Requirements**

| Parameter | Requirement |
|---|---|
| Frequency Response - H(f) | H_min(f) ≤ H(f) ≤ H_max(f) |
| Gain - Test 1 | -7 dB ≤ x ≤ 1 dB |
| Gain - Test 2-8 | -1.5 dB ≤ x ≤ 1.5 dB |
| Spectral Distortion | x ≤ -20 dB |
| Pitch Period: Mean Deviation | -0.5 Samples ≤ x ≤ 0.5 Samples |
| Pitch Period: Standard Deviation | x ≤ 0.25 Samples |
| Pitch Period: Maximum Deviation | x ≤ 2.0 Samples |
| Unvoiced Errors | x ≤ 5 % |
| Voiced Errors | x ≤ 2 % |
| Delay | x ≤ 110 ms |
| Single Frequency Max. Noise Level | x ≤ -65 dBmO |
| Total Noise Level | x ≤ -60 dBmO |
| Single Frequency Max. Crosstalk Level | x ≤ -62 dBmO |
| Total Crosstalk Level | x ≤ -57 dBmO |

---

## 7 References

- [1] Project 25 System and Standard Family Document, TSB102, Telecommunications Industry Association.
- [2] Project 25 Vocoder Description, TIA/EIA-102.BABA, Telecommunications Industry Association.
- [3] VC-20-PRJ25 Operation Manual, Digital Voice Systems, Inc.
- [4] Vocoder Mean Opinion Score Conformance Test, IS-102.BABB, Telecommunications Industry Association.
