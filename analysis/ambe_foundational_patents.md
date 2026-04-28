# DVSI Foundational MBE Patents — Public-Domain Implementation References

**Category:** Vocoder / Patent-Sourced Algorithmic Reference
**Date:** 2026-04-27
**Scope:** Three foundational DVSI patents that pre-date AMBE+2 and have been
in the public domain since 2010–2016. All three are positive references for
clean-room MBE / IMBE / AMBE-compatible implementation work — the patent
specifications contain explicit algorithmic disclosure that can be copied,
implemented, and built upon without infringing any active DVSI patent.

This document complements `DVSI/AMBE-3000/AMBE-3000_Patent_Reference.md` §11.
Where the §11 entry catalogs these patents as bibliographic references, this
file extracts the actual technical content from the specifications.

---

## Why This File Exists

The DVSI patent reference identified that three pre-2003 DVSI patents
(US 5,081,681, US 5,664,051, US 5,870,405) form the **mathematical
foundation** of MBE-family speech coding and are now public domain.
Reading the patents themselves reveals algorithmic content that is:

1. **Implementation-quality** — the specifications include closed-form
   equations, parity matrices, pseudo-random generators, and threshold
   tables that can be directly transcribed into code
2. **P25-direct** — particularly US 5,870,405, which explicitly references
   "APCO/NASTD/Fed Project 25 Vocoder Description" with appendix
   citations, and includes the complete IMBE FEC scheme used in BABA-A
3. **Free to use** — all three patents have expired

The most important finding from this analysis: **US 5,870,405 is the
patent that defines the P25 Phase 1 IMBE error control scheme in
implementation-ready detail**. It is the divisional of US 5,517,511
(Hardwick '511) and contains the parity matrices, bit prioritization
tables, modulation pseudo-random recurrence, and frame error/repeat/mute
logic that BABA-A specifies for Phase 1 voice frames.

---

## US 5,081,681 — Method and Apparatus for Phase Synthesis (1992)

### Bibliographic

- **Inventors:** John C. Hardwick (Cambridge), Jae S. Lim (Winchester)
- **Assignee:** Digital Voice Systems, Inc., Cambridge MA
- **Filed:** 1989-11-30
- **Granted:** 1992-01-14
- **Reexamination Certificate (B1) issued:** 1995-08-15
- **Expired:** ~2010 (17-year term from grant under pre-URAA rules)
- **Application:** 444,042
- **22 claims, 1 drawing sheet**

### Why This Patent Matters

This is the **original DVSI phase synthesis patent**, predating both
AMBE+2 and the spectral-envelope-derived phase regeneration of US5701390
(see `DVSI/AMBE-3000/AMBE-3000_Patent_Reference.md` §1). The patent
introduces phase recreation from fundamental frequency + V/UV
information + a random component, scaled by the unvoiced fraction.

This is the simpler approach an MBE decoder can implement before adding
the more sophisticated US5701390 spectral-envelope phase regeneration.
For low-complexity implementations, US 5,081,681's approach alone may
be sufficient.

### Block Diagram (FIG. 1)

```
Speech s(t) → A/D (10) → Speech Analysis (12) → Phase Synthesis (14) → Speech Synthesis (16) → D/A (18) → Synthetic Speech ŝ(t)
                                                       ↑                       ↑
                                                  Random Signal       A_k(t), V_k(t), ω(t), Θ_k(t)
```

The phase synthesizer (14) takes V_k(t) and ω(t), generates intermediate
phase φ_k(t), combines with random component r_k(t) to produce harmonic
phase Θ_k(t), which is then fed to speech synthesis (16) along with
harmonic magnitudes A_k(t) and other parameters.

### Notation

| Symbol | Meaning |
|---|---|
| A_k(t) | kth harmonic magnitude at time t |
| V_k(t) | voicing/unvoicing for kth harmonic (1 = voiced, 0 = unvoiced) |
| ω(t) | fundamental angular frequency in rad/sec |
| Θ_k(t) | phase for kth harmonic in radians |
| φ_k(t) | intermediate phase (without random component) |
| r_k(t) | random phase component |
| u_k(t) | white random signal, uniformly distributed in [-π, π] |
| α(t) | scaling factor (fraction of unvoiced harmonics) |
| N(t) | total number of harmonics of interest |
| P(t) | number of voiced harmonics |

### Key Equations

**Equation 1 — Intermediate phase from fundamental:**

```
φ_k(t₁) = φ_k(t₀) + ∫[τ=t₀ to t₁] k·ω(τ) dτ
```

**Equation 2 — Linear interpolation of fundamental frequency:**

```
ω(t) = ω(t₀) + (ω(t₁) - ω(t₀)) · (t - t₀)/(t₁ - t₀),  for t₀ ≤ t ≤ t₁
```

**Equation 3 — Closed-form intermediate phase using linear interpolation:**

```
φ_k(t₁) = φ_k(t₀) + k · ((ω(t₀) + ω(t₁))/2) · (t₁ - t₀)
```

This is the trapezoidal-rule integration of equation 1 with linearly-
interpolated fundamental — directly implementable in code.

**Equation 4 — Phase = intermediate + random:**

```
Θ_k(t) = φ_k(t) + r_k(t)
```

**Equation 5 — Random component:**

```
r_k(t) = α(t) · u_k(t)
```

where u_k(t) is white random uniform on [-π, π].

**Equations 6–8 — Voiced fraction and scaling:**

```
P(t) = Σ_{k=1}^{N(t)} V_k(t)               (Eq 6 — count of voiced harmonics)
V_k(t) ∈ {0, 1}                            (Eq 7)
α(t) = (N(t) - P(t)) / N(t)                (Eq 8 — fraction unvoiced)
```

**Implementation pseudocode:**

```c
// Per-harmonic phase recreation (US 5,081,681 Eqs 3-8)
double phi[L+1];          // intermediate phase, persistent across frames
double omega_prev, omega_curr;  // fundamental from previous and current frame

void synthesize_harmonic_phases(double *theta_out,
                                 const int *V,         // V[k] = 0 or 1
                                 int L) {
    // Count voiced harmonics
    int P = 0;
    for (int k = 1; k <= L; k++) P += V[k];
    double alpha = (double)(L - P) / (double)L;

    double dt = FRAME_DURATION;  // e.g., 20ms
    double omega_avg = 0.5 * (omega_prev + omega_curr);

    for (int k = 1; k <= L; k++) {
        // Eq 3: intermediate phase update
        phi[k] += k * omega_avg * dt;
        phi[k] = fmod(phi[k], 2.0 * M_PI);

        // Eq 5: random component (only if voiced; unvoiced uses different path)
        double u = uniform_random(-M_PI, M_PI);
        double r = alpha * u;

        // Eq 4: total phase
        theta_out[k] = phi[k] + r;
    }

    omega_prev = omega_curr;
}
```

**Equations 9–13 — Voiced speech synthesis with smooth phase:**

The patent specifies that voiced speech is synthesized as:

```
ŝ_v(t) = Σ_{k=1}^{N(t)} Â_k(t) · cos(Θ̂_k(t))         (Eq 9)
Θ̂_k(t) = ∫[τ=t₀ to t] ω̂_k(τ) dτ + Θ̂_k(t₀)            (Eq 10)
```

with smoothness constraints:

```
Θ̂_k(t_i) = Θ_k(t_i)                  (Eq 11 — phase matches at sample points)
dΘ̂_k(t)/dt|_{t=t_i} = k·ω(t_i)       (Eq 12 — frequency matches at sample points)
Â_k(t_i) = A_k(t_i)                  (Eq 13 — amplitude matches at sample points)
```

Â_k(t) and Θ̂_k(t) are typically chosen as **low-order polynomials** (e.g.,
linear amplitude interpolation, second-order phase polynomial) that
satisfy the smoothness conditions.

### Reexamination Outcome (1995)

US 5,081,681 was reexamined twice and emerged with claims **1, 6, 12-16,
18-22 patentable as amended; claims 2-5, 7-10 patentable as dependents
of amended claims; claims 11 and 17 cancelled.**

The amended claim 1 narrowed from "synthesizing speech" generally to
"synthesizing voiced and unvoiced frequency components coexisting at the
same time instants, wherein voiced frequency components are synthesized
from harmonic phase signal Θ_k(t)" — i.e., the patent was narrowed to
specifically cover **separated voiced/unvoiced synthesis paths with
distinct techniques for each**.

The amendment is documented in the B1 reexamination certificate (Aug 15,
1995). For implementation purposes, the algorithmic content described
above is unchanged — the reexamination affected only claim scope, not
disclosure.

### Implementation Notes

- The intermediate phase φ_k(t) is **persistent across frames** — the
  decoder must maintain it as state. Reset to zero at decoder
  initialization.
- The random component r_k(t) should be **larger when more harmonics are
  unvoiced** (per Eq 5 + Eq 8). When all harmonics are voiced (α=0),
  there is no random jitter and the phase is fully deterministic.
- The Eq 10 phase polynomial Θ̂_k(t) is what produces smooth voiced speech
  between frame samples. This is typically a 2nd-order polynomial
  (quadratic in t) that meets the boundary conditions in Eq 11–12.

---

## US 5,664,051 — Speech Decoder with Random Phase (1997)

### Bibliographic

- **Inventors:** Hardwick + Lim (DVSI, Burlington MA)
- **Filed:** 1994-06-23 (continuation of 814 / 1993, abandoned;
  continuation of 587,250 / 1990, abandoned)
- **Granted:** 1997-09-02
- **Terminal Disclaimer:** "The term of this patent shall not extend
  beyond the expiration date of Pat. No. 5,081,681" — tied to '681's
  ~2010 expiration
- **Application:** 265,492
- **12 claims, 1 drawing sheet (identical to '681's FIG. 1)**

### Relationship to US 5,081,681

US 5,664,051 has the **same disclosure** as US 5,081,681 (same drawing,
same equations, same notation). It was filed as a continuation in the
same family. The technical content overlaps almost completely.

The difference is in **what's claimed**:

- **US 5,081,681** claims the **phase synthesis algorithm** (recreation
  of phase from fundamental + random)
- **US 5,664,051** claims the **decoder architecture and bit-allocation
  insight**: don't encode phase at all in the bitstream; save those bits
  for other parameters

### The Bit-Allocation Insight

The US 5,664,051 specification adds (over the '681 spec) the
explicit reasoning for why phase recreation is valuable:

> "We have discovered that a great improvement in the quality of
> synthesized speech, in speech coding applications, can be achieved by
> not encoding the phase of harmonics in voiced portions of the
> speech, and instead synthesizing an artificial phase for the harmonics
> at the receiver. By not encoding this harmonic phase information, the
> bits that would have been consumed in representing the phase are
> available for improving the quality of the other components of the
> encoded speech (e.g., pitch, harmonic magnitudes). In synthesizing the
> artificial phase, the phases and frequencies of the harmonics within
> the segments are taken into account. In addition, a random phase
> component, or jitter, is added to introduce randomness in the phase.
> More jitter is used for speech segments in which a greater fraction of
> the frequency bands are unvoiced. Quite unexpectedly, the random
> jitter improves the quality of the synthesized speech, avoiding the
> buzzy, artificial quality that can result when phase is artificially
> synthesized."

This is the IMBE design philosophy in claim form: **phase is
reconstructed at the decoder, freeing the bitstream's bit budget for
fundamental + V/UV + magnitudes.** All MBE-family vocoders
(IMBE, AMBE, AMBE+, AMBE+2) follow this pattern.

### Claim 1 (Verbatim, Granted)

> "A speech decoder apparatus for synthesizing a speech signal from a
> digitized speech bit stream of the type produced by processing speech
> with a speech encoder, said apparatus comprising:
> - an analyzer for processing said digitized speech bit stream to
>   generate an angular frequency and magnitude for each of a plurality
>   of sinusoidal voiced frequency components representing the speech
>   processed by the speech encoder, said analyzer generating said
>   angular frequencies and magnitudes over a sequence of times;
> - a random signal generator for generating a time sequence of random
>   phase components;
> - a phase synthesizer for generating a time sequence of synthesized
>   phases for at least some of said sinusoidal voiced frequency
>   components, said synthesized phases being generated from said
>   angular frequencies and random phase components;
> - a first synthesizer for synthesizing the voiced frequency components
>   of speech from said time sequences of angular frequencies,
>   magnitudes, and synthesized phases; and
> - a second synthesizer for synthesizing unvoiced frequency components
>   representing the speech processed by the speech encoder, **using a
>   technique different from the technique used for synthesizing the
>   voiced frequency components**;
> - wherein the speech signal is synthesized by combining synthesized
>   voiced and unvoiced frequency components coexisting at the same time
>   instants."

### Notable Dependent Claims

- **Claim 4:** synthesis is MBE (multi-band excitation), bitstream
  encoded with an MBE speech encoder
- **Claim 6:** bitstream encoded with a sinusoidal transform coder
- **Claims 11–12:** method/apparatus variants

### INMARSAT M Reference

The specification ends with: *"A specific example of a speech synthesis
method that utilizes the invention is shown in the INMARSAT Standard M
Voice Codec Definition Manual available from INMARSAT."*

This confirms US 5,664,051 covers the INMARSAT-M satellite vocoder
(also IMBE-family). Same architectural pattern as P25 IMBE.

---

## US 5,870,405 — Digital Transmission of Acoustic Signals Over a Noisy Communication Channel (1999) — **THE P25 PHASE 1 IMBE FEC PATENT**

### Bibliographic

- **Inventors:** John C. Hardwick (Somerville), Jae S. Lim (Winchester)
- **Assignee:** Digital Voice Systems Inc, Burlington Mass
- **Filed:** 1996-03-04 (**divisional of US 5,517,511**, filed
  1992-11-30)
- **Granted:** 1999-02-09
- **Expired:** ~2016-02-09 (17-year term from grant under pre-URAA;
  20-year-from-priority would be 2012-11-30)
- **Application:** 610,184
- **14 claims, 18 drawing sheets**
- **Certificate of Correction issued 2000-09-26** (numerous typo fixes)

### Why This Is the Most Important Patent in This Document

US 5,870,405 is the patent on the complete IMBE error control scheme
for P25 Phase 1. The specification:

1. **Explicitly references "APCO/NASTD/Fed Project 25 Vocoder
   Description"** with citations to Appendices E, F, G, and H of the
   APCO/NASTD/P25 specification (the original IMBE annex tables)
2. **Includes the complete Golay [23,12] and Hamming [15,11] parity
   matrices** as 12×11 and 11×4 binary matrices in the spec
3. **Specifies the bit prioritization scheme** that maps quantizer
   bits b̂_0, b̂_1, ..., b̂_{L+2} to encoded bit vectors
   û_0, û_1, ..., û_7
4. **Defines the pseudo-random bit modulation** with explicit
   recurrence p_r(n) = 173·p_r(n-1) + 13849 (mod 65536)
5. **Specifies frame validation logic** with thresholds (ε_T ≥ 11,
   ε_R ≥ 0.085, etc.) for declaring invalid frames
6. **Defines frame repeat and mute decisions** including the "4
   consecutive invalid frames → mute" rule
7. **Specifies adaptive smoothing** of spectral amplitudes and V/UV
   decisions under bit errors

**Every one of these elements is in BABA-A.** This patent is the
implementation specification for P25 Phase 1 IMBE FEC, written as a
patent. Now public domain (since ~2016).

### Relationship to US 5,517,511 (Hardwick '511)

US 5,870,405 is a **divisional** of US 5,517,511 — both share the
1992-11-30 priority date. '511 is referenced extensively throughout
the AMBE+2 prosecution history (see `DVSI/AMBE-3000/AMBE-3000_Patent_Reference.md`
§§6, 8) as prior art. US 5,870,405 covers the complementary scope of
the same disclosure.

### Frame Structure (P25 7.2 kbps IMBE)

| Component | Bits | Notes |
|---|---|---|
| Total frame | 144 | over 20 ms |
| Error control bits | 56 | Golay + Hamming + uncoded structure |
| Speech model parameter bits | 88 | quantizer bits b̂_0...b̂_{L+2} |
| Synchronization bit | 1 | alternating sequence (b̂_{L+2}) |

### Bit Allocation Among Model Parameters (Patent Table 1)

| Parameter | Bits | Notes |
|---|---|---|
| Fundamental Frequency | 8 | b̂_0, half-sample resolution |
| V/UV Decisions | K̂ | variable, depends on L̂ |
| Spectral Amplitudes | 79 - K̂ | variable |
| Synchronization | 1 | b̂_{L+2}, alternating |

K̂ is determined by the formula:
```
K̂ = floor((L̂ + 2) / 3)  if L̂ ≤ 36
K̂ = 12                    otherwise            (Eq 5)
```

### Fundamental Frequency Encoding (Equations 2–4)

The fundamental angular frequency is in the range:
```
2π/123.125 ≤ ω̂_0 ≤ 2π/19.875
```

Encoding (8 bits, b̂_0 ∈ [0, 255]):
```
b̂_0 = floor(4π/ω̂_0 - 39)                       (Eq 2)
```

Decoding:
```
ω̃_0 = 4π / (b̃_0 + 39.5)                         (Eq 3)
L̃ = floor(0.9254·π/ω̃_0 + 0.25)                  (Eq 4)
```

**Reserved pitch values** (the patent specifies these as part of the
P25 IMBE format — directly applicable to BABA-A):

| Range of b̃_0 | Meaning |
|---|---|
| 0 – 207 | Valid pitch (encodes ω̃_0) |
| 208 – 215 | Silence / DTMF tones |
| 216 – 219 | Future expansion (currently invalid) |
| 220 – 255 | DTMF, call progress signals, in-band data, etc. |

### V/UV Decision Encoding (Eq 6–8)

The K̂ V/UV bits are encoded as a single binary number b̂_1:
```
b̂_1 = Σ_{k=1}^{K̂} v̂_k · 2^(K̂-k)                  (Eq 6)
```

At the decoder, a separate V/UV decision is computed for each spectral
amplitude (l = 1...L̃):
```
κ_l = floor((l+2)/3)  if L̃ ≤ 36
κ_l = 12               otherwise                  (Eq 7)

ṽ_l = floor(b̃_1 / 2^(K̃-κ_l)) - 2·floor(b̃_1 / 2^(K̃-κ_l+1))   (Eq 8)
```

This expansion gives **per-amplitude V/UV decisions** at the decoder
even though the encoder sends band-level V/UV decisions.

### Spectral Amplitude Encoding (Equations 9–20, FIG. 3)

Pipeline (encoder):

1. **Compute prediction residual** (Eq 9–11):
   ```
   k̂_l = (L̂(-1) / L̂(0)) · l                      (Eq 9 — interpolation index)
   δ_l = k̂_l - floor(k̂_l)                          (Eq 10)
   T̂_l = log_2(M̂_l(0)) - ρ·{(1-δ_l)·log_2(M̂_floor(k̂_l)(-1)) + δ_l·log_2(M̂_floor(k̂_l)+1(-1))} - ρδ_0·log_2(M̂_floor(k̂_l)(-1)) + (ρ/L̂(0))·Σ ...      (Eq 11)
   ```
   The prediction coefficient ρ varies with L̂(0):
   ```
   ρ = 0.4   if L̃(0) ≤ 15
   ρ = 0.03·L̃(0) - 0.5  if 15 < L̃(0) ≤ 24
   ρ = 0.7   otherwise                              (Eq 12)
   ```

2. **Divide L̂ residuals into 6 blocks** (Eq 15–16):
   ```
   Σ_{i=1}^{6} Ĵ_i = L̂                              (Eq 15)
   floor(L̂/6) ≤ Ĵ_i ≤ Ĵ_{i+1} ≤ ceil(L̂/6)          (Eq 16)
   ```
   Example for L̂ = 34: J = [5, 5, 6, 6, 6, 6]

3. **DCT each block** (Eq 17):
   ```
   Ĉ_{i,k} = (1/Ĵ_i) · Σ_{j=1}^{Ĵ_i} ĉ_{i,j}·cos(π·(k-1)·(j-1/2)/Ĵ_i)
   for 1 ≤ k ≤ Ĵ_i                                  (Eq 17)
   ```

4. **Form gain vector R̂_i = Ĉ_{i,1}** (first DCT coefficient of each
   block, i = 1...6)

5. **Apply 6-pt DCT to gain vector** (Eq 18):
   ```
   Ĝ_m = (1/6) · Σ_{i=1}^{6} R̂_i · cos(π·(m-1)·(i-1/2)/6)
   for 1 ≤ m ≤ 6                                    (Eq 18)
   ```

6. **Quantize transformed gain vector**:
   - Ĝ_1 (overall gain): 6-bit non-uniform quantizer per APCO/NASTD/P25
     Vocoder Description Appendix E
   - Ĝ_2 through Ĝ_6: uniform quantizers, bits b̂_3 through b̂_7,
     allocated per Appendix F

7. **Quantize higher-order DCT coefficients** Ĉ_{i,k} for 2 ≤ k ≤ Ĵ_i:
   uniform quantization with step sizes from Patent Tables 3 and 4:

   **Table 3 — Uniform Quantizer Step Size for Higher Order DCT Coefficients:**

   | Number of Bits | Step Size |
   |---|---|
   | 1 | 1.2σ |
   | 2 | 0.85σ |
   | 3 | 0.65σ |
   | 4 | 0.40σ |
   | 5 | 0.28σ |
   | 6 | 0.15σ |
   | 7 | 0.08σ |
   | 8 | 0.04σ |
   | 9 | 0.02σ |
   | 10 | 0.01σ |

   **Table 4 — Standard Deviation of Higher Order DCT Coefficients:**

   | DCT Coefficient | σ |
   |---|---|
   | C_{1,2} | 0.307 |
   | C_{1,3} | 0.241 |
   | C_{1,4} | 0.207 |
   | C_{1,5} | 0.190 |
   | C_{1,6} | 0.190 |
   | C_{1,7} | 0.179 |
   | C_{1,8} | 0.173 |
   | C_{1,9} | 0.165 |
   | C_{1,10} | 0.170 |

   The bit allocation per coefficient is in APCO/NASTD/P25 Appendix G.

### Bit Prioritization (FIG. 9, FIG. 10, FIG. 11)

The 88 quantizer bits b̂_0, b̂_1, ..., b̂_{L+2} are **prioritized by
sensitivity** and reassigned to bit vectors û_0 through û_7 based on
their position-within-quantizer-value weights.

Result of prioritization:

| Vector | Length | FEC Code | Notes |
|---|---|---|---|
| û_0 | 12 bits | [23,12] Golay | Highest priority — fundamental MSBs + V/UV MSBs + gain MSBs |
| û_1 | 12 bits | [23,12] Golay | |
| û_2 | 12 bits | [23,12] Golay | |
| û_3 | 12 bits | [23,12] Golay | |
| û_4 | 11 bits | [15,11] Hamming | |
| û_5 | 11 bits | [15,11] Hamming | |
| û_6 | 11 bits | [15,11] Hamming | |
| û_7 | 7 bits | (uncoded) | Lowest priority — LSBs |

Total: 4×12 + 3×11 + 7 = 88 bits → 4×23 + 3×15 + 7 = 144 channel bits
(56 FEC bits + 88 model bits = 144).

The exact bit-by-bit prioritization follows from the weight assignment
algorithm:
- More-significant bit positions get higher weights than less-
  significant bit positions within the same quantizer value
- Lower-frequency spectral amplitudes get higher weights than higher-
  frequency amplitudes (claim 4)

### FEC Encoding (Equations 38–40)

```
v̂_i = û_i · P_G   for 0 ≤ i ≤ 3                   (Eq 38, Golay)
v̂_i = û_i · P_H   for 4 ≤ i ≤ 6                   (Eq 39, Hamming)
v̂_7 = û_7                                          (Eq 40, uncoded)
```

**P_G — [23,12] Golay parity matrix** (from patent col. 18, displayed
as the right-side parity portion of the systematic generator):

```
P_G = systematic [23,12] Golay generator
      I_12 (identity) | parity matrix shown in patent
```

The patent gives the parity portion explicitly as a 12×11 binary matrix.
The full generator is G = [I_12 | P_G_parity] where the parity submatrix
is read off the patent figure.

**P_H — [15,11] Hamming parity matrix** (from patent col. 18):

```
P_H = systematic [15,11] Hamming generator
      I_11 | parity matrix
```

The parity portion is given as an 11×4 binary matrix.

Standard decoders for both codes are described in coding theory
literature (the patent doesn't redefine them, just specifies the
generators).

### Bit Modulation (Equations 41–51)

This is **data-dependent scrambling** — used to detect uncorrectable
errors in the highest-priority codeword v̂_0.

Modulation key from û_0 (interpreted as 12-bit unsigned in [0, 4095]):

```
p_r(0) = 16 · û_0                                  (Eq 41)
p_r(n) = 173·p_r(n-1) + 13849 - 65536·floor((173·p_r(n-1) + 13849)/65536)
       for 1 ≤ n ≤ 114                              (Eq 42)
```

This is a **linear congruential pseudo-random generator** with
multiplier 173, increment 13849, modulus 65536 (= 2^16). Each output
in [0, 65535].

Modulation vectors derived from the sequence:

```
m̂_0 = [0, 0, ..., 0]                               (Eq 43, 23 bits)
m̂_1 = [floor(p_r(1)/32768), ..., floor(p_r(23)/32768)]    (Eq 44, 23 bits)
m̂_2 = [floor(p_r(24)/32768), ..., floor(p_r(46)/32768)]   (Eq 45, 23 bits)
m̂_3 = [floor(p_r(47)/32768), ..., floor(p_r(69)/32768)]   (Eq 46, 23 bits)
m̂_4 = [floor(p_r(70)/32768), ..., floor(p_r(84)/32768)]   (Eq 47, 15 bits)
m̂_5 = [floor(p_r(85)/32768), ..., floor(p_r(99)/32768)]   (Eq 48, 15 bits)
m̂_6 = [floor(p_r(100)/32768), ..., floor(p_r(114)/32768)] (Eq 49, 15 bits)
m̂_7 = [0, 0, ..., 0]                               (Eq 50, 7 bits)
```

`floor(p_r(n) / 32768)` produces a single bit (0 or 1) — the MSB of the
16-bit p_r(n).

Modulated codewords:

```
ĉ_i = v̂_i + m̂_i  (mod 2)   for 0 ≤ i ≤ 7         (Eq 51)
```

The decoder regenerates the same key from its decoded û_0 and XORs back
to recover v̂_i. If û_0 is decoded incorrectly (uncorrectable error in
[23,12] Golay), the decoder's modulation key will be wrong, the
remaining v̂_i will be effectively passed through a 50% BER channel, and
the resulting error count ε_T will be very high — flagging the frame
as invalid.

### Frame Validation (Equations 52–56, FIG. 13)

Per-codeword error counts ε_i for 0 ≤ i ≤ 6:
```
ε_i = (number of errors corrected by [23,12] Golay or [15,11] Hamming
       when decoding ĉ_i)                          (Eq 52, definition)
```

Aggregate error parameters:
```
ε_T = Σ_{i=0}^{6} ε_i                              (Eq 53, total errors)
ε_R(0) = 0.95 · ε_R(-1) + 0.000356 · ε_T            (Eq 54, running estimate)
```

ε_R is a leaky-integrator estimate of long-term error rate; smoothing
factor 0.95 gives ~25-frame time constant (~500 ms).

**Frame INVALID if any of:**

1. ε_0 ≥ 2 AND ε_T ≥ 11 (Eq 55–56) — uncorrectable errors detected
   via bit modulation
2. 216 ≤ b̃_0 ≤ 219 — reserved pitch values (currently invalid)
3. 220 ≤ b̃_0 ≤ 255 — reserved for DTMF/inband data, treated as invalid
   for voice synthesis

If 208 ≤ b̃_0 ≤ 215 — valid silence frame, mute output.

### Frame Repeat (Equations 57–62)

When frame is declared invalid, repeat previous frame's parameters:

```
ω̃_0(0) = ω̃_0(-1)                                  (Eq 57)
L̃(0) = L̃(-1)                                      (Eq 58)
K̃(0) = K̃(-1)                                      (Eq 59)
ṽ_k(0) = ṽ_k(-1)  for 1 ≤ k ≤ K̃                  (Eq 60)
M̃_l(0) = M̃_l(-1)  for 1 ≤ l ≤ L̃                  (Eq 61)
M̃_l(0) = M̃_l(-1)  for 1 ≤ l ≤ L̃                  (Eq 62, repeated for clarity)
```

**Mute logic:** After 4 consecutive invalid frames OR if ε_R > 0.085,
**mute the synthesized output**. Recommended muting method per the
patent: bypass the synthesis procedure and set ŝ(n) to random noise
uniformly distributed over [-5, 5] samples (16-bit PCM scale).

This explicit "4 consecutive invalid frames → mute" rule is
**directly applicable to BABA-A implementation** — and the threshold
constants ε_T ≥ 11, ε_R ≥ 0.085 are public-domain reference values.

### Adaptive Smoothing (Equations 63–74)

Spectral amplitude enhancement and V/UV smoothing reduce perceived
distortion under uncorrected bit errors.

**Step 1 — Compute spectral moments:**
```
R_M0 = Σ_{l=1}^{L̃} M̃_l²                            (Eq 63, energy)
R_M1 = Σ_{l=1}^{L̃} M̃_l² · cos(ω̃_0 · l)             (Eq 64, first moment)
```

**Step 2 — Per-amplitude weights:**
```
W_l = sqrt(M̃_l) · [(0.96π·(R_M0² + R_M1² - 2·R_M0·R_M1·cos(ω̃_0·l))) /
                    (ω̃_0·R_M0·(R_M0² - R_M1²))]^(1/4)
for 1 ≤ l ≤ L̃                                      (Eq 65)
```

**Step 3 — Enhanced amplitudes:**
```
M̄_l = M̃_l           if 8l ≤ L̃                     (Eq 66)
M̄_l = 1.2 · M̃_l     else if W_l > 1.2
M̄_l = 0.5 · M̃_l     else if W_l < 0.5
M̄_l = W_l · M̃_l     otherwise
```

**Step 4 — Energy parameter (running estimate):**
```
S_E(0) = 0.95·S_E(-1) + 0.05·R_M0   if 0.95·S_E(-1) + 0.05·R_M0 ≤ 10000
S_E(0) = 10000                       otherwise        (Eq 69)
```

**Step 5 — Voiced-bias threshold (V/UV smoothing):**
```
V_M = ∞                              if ε_R ≤ 0.005 AND ε_T ≤ 4
V_M = 45.255·S_E(0)^0.375 / exp(277.26·ε_R)
                                     else if ε_R ≤ 0.0125 AND ε_4 = 0
V_M = 1.414·S_E(0)^0.375              otherwise        (Eq 70)
```

When ε_R is large, V_M is small, so more amplitudes pass M̄_l > V_M and
get force-voiced. This is the "errors → bias toward voiced" mechanism.

**Step 6 — V/UV smoothing:**
```
v̄_l = 1            if M̄_l > V_M
v̄_l = ṽ_l           otherwise                       (Eq 71)
```

**Step 7 — Spectral amplitude scale factor under errors:**

Compute amplitude average:
```
A_M = Σ_{l=1}^{L̃} M̄_l                              (Eq 72)
```

Update amplitude threshold:
```
τ_M(0) = 20480                       if ε_R ≤ 0.005 AND ε_T ≤ 6
τ_M(0) = 6000 - 300·ε_T + τ_M(-1)    otherwise        (Eq 73)
```

Compute scale factor:
```
γ_M = 1.0              if τ_M(0) > A_M
γ_M = τ_M(0) / A_M     otherwise                      (Eq 74)
```

Final scaled amplitudes M̃_l are multiplied by γ_M for synthesis.

### Connection to BABA-A

Every element of BABA-A's IMBE FEC scheme is in this patent
specification:

| BABA-A element | Patent location |
|---|---|
| 144 bits/frame, 56 FEC + 88 model + 1 sync | col. 5–6, Table 1 |
| 8-bit pitch, K-bit V/UV, (79-K)-bit spectral, 1-bit sync | Table 1 |
| Pitch quantization formula | Eq 2–4 |
| Reserved pitch values 208–215 (silence), 216–255 (data) | col. 19–20 |
| 4×[23,12] Golay + 3×[15,11] Hamming + 1 uncoded | FIG. 7, 10, 11 |
| Bit prioritization (highest-priority bits in first vectors) | FIG. 9, col. 14–16 |
| Pseudo-random bit modulation | Eq 41–51 |
| Frame error detection (ε_0 ≥ 2 AND ε_T ≥ 11) | Eq 55–56 |
| Frame repeat | Eq 57–62 |
| 4-consecutive-invalid → mute | col. 21 |
| Adaptive spectral smoothing | Eq 63–74 |
| Spectral amplitude block-DCT structure (6 blocks) | FIG. 3, 4, 5 |
| Higher-order DCT quantization tables | Tables 3, 4 |

**This patent is, effectively, the implementation specification for
BABA-A IMBE FEC, written in patent form.** A clean-room implementer
working from this specification can produce a P25-compatible IMBE
encoder/decoder without needing to read BABA-A directly for the FEC
layer — though BABA-A has the canonical TIA-blessed values that should
be cross-checked against this patent's tables.

### Independent Claims

The patent has 14 claims with three independent claims:

**Claim 1** (encoding method — bit prioritization):
> "A method for encoding digital data in which a set of quantizer values
> is encoded into a plurality of bit vectors, each quantizer value
> containing a non-negative number of bit locations, the method
> comprising the steps of:
> - assigning each bit location in said quantizer values a weight, the
>   weight weighing more-significant bit locations more heavily than
>   less-significant bit locations of the same quantizer value;
> - prioritizing all of said quantizer bit locations in the order of
>   their weight, high priority given to bit locations with high
>   weights; and
> - partitioning bits associated with said prioritized bit locations into
>   contiguous blocks to form bit vectors,
> - wherein some of said quantizer values represent the spectrum of an
>   acoustic signal, and bits in a first quantizer value are given
>   higher weight than bits of the same significance in a second
>   quantizer value."

**Claim 8** (decoding method — modulation key validation):
> "A method for decoding digital data in which a plurality of quantizer
> values are represented by a plurality of bit vectors, the method
> comprising the steps of:
> - dividing said digital data into one or more frames;
> - further dividing each of said frames into a plurality of code
>   vectors;
> - generating a demodulation key from one of said code vectors;
> - initializing a pseudo-random sequence using said demodulation key;
> - using said pseudo-random sequence to generate a plurality of binary
>   demodulation vectors;
> - generating a plurality of demodulated code vectors by performing
>   modulo 2 addition of said binary demodulation vectors with said code
>   vectors;
> - error control decoding a plurality of said demodulated code vectors
>   to produce a plurality of bit vectors;
> - determining the quantizer bit allocation for each of said quantizer
>   values;
> - assigning each bit location in said quantizer bit allocation a
>   weight... [bit deprioritization] ...
> - computing an error measure by comparing said demodulated code
>   vectors before error control decoding with said demodulated code
>   vectors after error control decoding;
> - comparing said error measure against a threshold; and
> - declaring said quantizer values invalid if said error measure exceeds
>   said threshold."

**Claim 10** (apparatus — encoding):
Apparatus form of claim 1 with means-plus-function language.

**Claim 12** (technology coverage):
"The apparatus of claims 10 or 11 wherein said speech coder is one of
the following speech coders: Multi-Band Excitation (MBE) speech coder,
Improved Multi-Band Excitation (IMBE™) speech coder, or sinusoidal
transform speech coder (STC)."

This explicitly names IMBE — the P25 vocoder.

---

## Summary — The Public-Domain MBE Foundation

The three patents above, combined with US 5,701,390 (§1 of the AMBE-3000
patent reference), US 5,754,974 (Griffin '974), US 5,517,511 (Hardwick
'511), US 6,199,037 (Hardwick '037, §3), and US 5,715,365 / US 5,826,222
(Griffin's excitation parameter patents from §11), constitute the
**complete public-domain mathematical foundation of MBE-family speech
coding**.

For an open-source implementation targeting P25 Phase 1 IMBE
compatibility, the implementation roadmap is:

1. **Excitation parameter estimation** (pitch + V/UV detection):
   - US 5,715,365 — frequency-band nonlinear emphasis for pitch
   - US 5,826,222 — hybrid excitation parameter estimation
   - US 6,199,037 — joint quantization (also useful at low bit rates)

2. **MBE analysis-synthesis structure**:
   - US 5,664,051 — decoder architecture with random phase
   - US 5,081,681 — phase synthesis from fundamental + V/UV
   - US 5,701,390 — improved phase regeneration from spectral envelope
   - US 5,754,974 / EP 0,893,791 — spectral amplitude prediction
     residuals via block DCT

3. **P25 Phase 1 IMBE FEC and frame handling**:
   - **US 5,870,405 — the P25 IMBE FEC patent (this document, §3)**
   - US 5,517,511 (Hardwick '511) — parent of '405; relevant where '405
     references it

All of these are now public domain. None of them encumber a clean-room
P25-compatible IMBE implementation. The expired patents' specifications
contain implementation-quality disclosure that can be directly
transcribed.

What is **NOT covered by these public-domain patents** is the AMBE+2
half-rate codec — that's encumbered by US8359197 until 2028-05-20 (see
`DVSI/AMBE-3000/AMBE-3000_Patent_Reference.md` §6). For Phase 1 IMBE
work specifically, the foundational portfolio is fully open.

---

## Cross-References

- `DVSI/AMBE-3000/AMBE-3000_Patent_Reference.md` §1 — US5701390 phase
  regeneration (the improvement built on top of US5081681 phase
  synthesis)
- `DVSI/AMBE-3000/AMBE-3000_Patent_Reference.md` §11 — bibliographic
  catalog of the full DVSI foundational patent portfolio including
  these three
- `analysis/ambe_phase_regen_hilbert.md` — connects US5701390's phase
  regeneration kernel to the discrete Hilbert transform; companion
  analysis to the phase synthesis patents discussed here
- TIA-102.BABA Implementation Spec — BABA-A's canonical FEC layer
  values; cross-check against the patent tables here
- `analysis/vocoder_missing_specs.md` — context on why these public-
  domain patents matter for a clean-room AMBE+2-quality implementation

## Patent PDFs (for reference)

The three patent PDFs analyzed here are at
`/mnt/share/P25-IQ-Samples/uspto_batch5/`. They are USPTO-issued
publicly available documents (no copyright restrictions). Implementers
can pull them from Google Patents or the USPTO Patent Public Search by
patent number for direct reference.

- `US5081681.pdf` — 11 pages
- `US5664051.pdf` — 7 pages
- `US5870405.pdf` — 38 pages including 5-page Certificate of Correction
