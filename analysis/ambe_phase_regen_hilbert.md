# AMBE+2 Phase Regeneration Kernel as Discrete Hilbert Transform

**Scope:** US5701390's phase regeneration kernel `h(m) = 2/(π·m)` for
`m ≠ 0`, `h(0) = 0`, is the truncated impulse response of the discrete
Hilbert transform. The regenerated phase `φ_l` equals the Hilbert
transform of `log|M̃_l|` applied as a linear filter across the harmonic
index. This is the frequency-domain analog of Kolmogorov's
minimum-phase formula: *the imaginary part of a causal log-spectrum is
the Hilbert transform of its real part*.

**Status:** Theoretical background for the AMBE-3000 decoder spec §5.
Not empirical — derivation from patent text plus DSP textbook identities.

**Date:** 2026-04-16

---

## 1. The Kernel in Context

US5701390 Eq. 8 (col. 7 lines 30–45) defines the regenerated phase of
harmonic `l`:

```
φ_l = Σ_{m = −D, m ≠ 0}^{+D}  h(m) · B_{l+m}            (Eq. 8)

h(m) = 2 / (π · m)    for m ≠ 0                         (Eq. 9)
h(0) = 0
D    = 19   (patent-recommended half-width)
```

where `B_l = log₂(M̄_l)` is the log-compressed spectral envelope.

The patent text describes `h(m)` as an "edge detection kernel" with
properties: antisymmetric (`h(−m) = −h(m)`), decays as `1/m`, zero at
the center. This description is correct but under-motivated — why
**this** kernel rather than any other antisymmetric, `1/m`-decaying,
zero-centered kernel?

The reason: `h(m) = 2/(π·m)` is the discrete impulse response of the
ideal Hilbert transform filter, and the Hilbert transform has a
specific signal-theoretic role in relating log-magnitude to phase.

---

## 2. The Hilbert Transform — Discrete Impulse Response

The continuous Hilbert transform is defined as:

```
H{f}(t) = (1/π) · p.v. ∫ f(τ) / (t − τ) dτ
```

In frequency domain it is a 90° phase shift: `H{f}` has Fourier
transform `−j · sign(ω) · F{f}`. Its impulse response is
`h_H(t) = 1/(π·t)` in the principal-value sense.

Sampling this impulse response at integer values yields:

```
h_H[n] = 1/(π·n)   for n ≠ 0
h_H[0] = 0
```

Truncating to a finite window of half-width `D` and doubling (to
account for the ideal filter's bilateral support) gives:

```
h[n] = 2/(π·n)     for n ∈ [−D, −1] ∪ [1, D]
h[n] = 0           for n = 0
```

This is **exactly US5701390's Eq. 9**.

So `h(m)` is not an arbitrary edge-detection kernel — it's the
canonical discrete Hilbert transform applied across the harmonic
index `l`.

---

## 3. Kolmogorov's Identity: Phase as Hilbert Transform of Log-Magnitude

For a **minimum-phase** signal (zeros and poles all inside the unit
circle of the z-plane), the logarithm of its frequency response
satisfies:

```
ln H(e^{jω}) = ln |H(e^{jω})| + j · arg H(e^{jω})

where:  arg H(ω) = H{ ln |H(ω)| }(ω)         (Kolmogorov's relation)
```

That is: the **phase of a minimum-phase signal is the Hilbert
transform of its log-magnitude**. This is a fundamental result in
complex-analysis-based signal theory (Oppenheim & Schafer,
*Discrete-Time Signal Processing*, §11.2 "Minimum-Phase Sequences").

The vocal tract transfer function is approximately minimum-phase over
speech bandwidths (poles dominate zeros; formant resonances are
all inside the unit circle). So **estimating phase from log-magnitude
via a Hilbert transform recovers approximately the correct phase** for
voiced speech.

---

## 4. Why This Is Better Than Random Phase

Baseline IMBE (BABA-A Eq. 141) seeds voiced-synthesis phase with uniform
random noise:

```
ρ_l(0) = (2π / 53125) · u(l) − π   ∈ [−π, π)        (Eq. 141)
```

Random phase means all harmonics are phase-incoherent — no structural
relationship between adjacent harmonics. The synthesized time-domain
waveform gets whatever interference pattern the random phases produce.
Perceptually this sounds "buzzy" or "reverberant" because natural
speech has coherent phase relationships dictated by the vocal tract's
impulse response.

AMBE+2's Hilbert-transform-derived phase captures this coherence: sharp
spectral peaks (formants) produce phase slopes that match what a
minimum-phase vocal-tract filter would produce for those peaks. Smooth
spectral regions produce smaller phase slopes. The synthesized waveform
gets the *qualitatively correct* phase relationships across harmonics,
and the perceived quality improvement is substantial at the same bit
rate.

---

## 5. The γ = 0.44 Scaling

US5701390 Eq. 9 includes a scaling factor γ applied to the Hilbert
transform output. Patent col. 7 line 55 states γ = 0.44 has been found
to work well. Why this value rather than 1.0?

Two plausible reasons (not explicitly stated in the patent):

1. **Practical attenuation of truncation artifacts.** A finite-length
   Hilbert filter has ringing sidebands from the rectangular window.
   Scaling down by γ < 1 attenuates the ringing's contribution to the
   phase estimate. Trade-off: under-estimates phase slope at sharp
   formants.
2. **Perceptual tuning.** Even the ideal Hilbert transform produces
   phase slopes that may be larger than perceptually ideal for
   vocoder synthesis (where only coarse phase structure matters).
   γ = 0.44 reduces the phase magnitudes to a range that matches
   what sounds natural rather than what's mathematically exact.

The value is empirical. Implementations may fine-tune γ to match
DVSI reference output; expect the range `0.4–0.5` to produce
perceptually similar output.

---

## 6. Truncation — Why D = 19

The ideal discrete Hilbert kernel has infinite support: `h[n] = 2/(π·n)`
for all `n ≠ 0`. Its energy decays as `1/n²`, so a truncated FIR
version at half-width `D` captures roughly `1 − 2/(π²·D)` of the
total energy.

- D = 5:   ~96% energy captured, noticeable truncation ringing
- D = 10:  ~98% energy, moderate ringing
- D = 19:  ~99% energy, minimal ringing
- D = 50:  ~99.6% energy, marginal improvement
- D = ∞:   100%, but computationally unbounded

US5701390 col. 7 line 40: "A value of D = 19 has been found to be
essentially equivalent to longer lengths." This matches the energy-
capture curve — beyond 19 the improvement flattens out.

The practical cost of D = 19: 38 non-zero taps per harmonic per frame.
For L̃ = 56 and 50 frames/second, that's 38 × 56 × 50 = 106,400
multiply-accumulates per second. Negligible on any modern CPU.

---

## 7. Relationship to Complex Cepstrum

A more direct computation of `φ_l` from `B_l = log₂|M̃_l|` uses the
complex cepstrum:

```
1. Compute FFT of B_l sequence → cepstral coefficients c[n]
2. Zero out the non-causal half (n < 0) → causal cepstrum
3. Inverse FFT → analytic signal
4. Phase = imaginary part of analytic signal
```

This is Oppenheim & Schafer's recipe for computing minimum-phase
spectra. It produces the same result as applying the Hilbert
transform directly to log|M̃|, but via a different path.

The Hilbert-kernel approach (US5701390) is more efficient for the
MBE case because:
1. `L̃ ≤ 56`, so the sequence is short — a 38-tap FIR is comparable
   cost to a 64-point FFT + IFFT pair
2. The FFT approach requires zero-padding and windowing to avoid
   circular-convolution artifacts, adding complexity
3. The FIR approach is more amenable to fixed-point implementation
   (the DVSI chip operates in Q-format arithmetic)

Either approach is correct. Implementations that want to verify their
Hilbert FIR are working correctly can cross-check against an FFT-based
complex-cepstrum reference on the same `B_l` sequence.

---

## 8. Lower-Harmonic Exclusion (Patent §8)

US5701390 col. 8 lines 15–30 specifies that phase regeneration is
**not applied** to harmonics 1 through ⌊L̃/4⌋ — those get the
accumulated phase `ψ_l(0)` from the previous frame directly, without
the Hilbert-transform correction.

Why: the lowest harmonics are pitch-locked and their phase is dominated
by the pitch period itself, not by spectral envelope shape. For these
harmonics, continuing the phase from the prior frame via pitch
accumulation is more accurate than regenerating from spectrum.

At L̃ = 20 this means harmonics 1–5 use pitch-accumulated phase and
6–20 use Hilbert-regenerated phase. At L̃ = 56 it's 1–14 vs 15–56.
The boundary is `⌊L̃/4⌋` precisely (floor division, patent text is
explicit).

See decoder spec §5.3 for how this integrates with the BABA-A §1.12.2
voiced synthesizer.

---

## 9. What Changes If You Don't Implement This Right

| Error | Symptom |
|-------|---------|
| Wrong kernel (e.g., `1/m` instead of `2/(π·m)`) | Output sounds muffled; phase energy is π/2× too small |
| Using random phase like IMBE (skipping regen) | Output sounds buzzy/reverberant (pre-AMBE+2 quality) |
| Kernel applied to linear magnitudes not log | Phase tracks amplitude rather than formant shape; sounds wrong at loud passages |
| Skipping the ⌊L̃/4⌋ lower-harmonic exclusion | Low-pitch content (male voice) sounds "phasey"; fundamental loses coherence |
| Wrong γ scaling | Output sounds too "bright" (γ too high) or "dull" (γ too low) |
| Forgetting h(0) = 0 | Phase accumulates a DC bias across harmonics; overall sounds tilted |
| Truncation too short (D < 10) | Ringing artifacts at formant edges |

The Hilbert-transform framing makes most of these diagnosable: if the
output has a known perceptual defect, the kernel's signal-theoretic
role tells you which parameter to adjust.

---

## 10. Related

- `DVSI/AMBE-3000/AMBE-3000_Decoder_Implementation_Spec.md` §5 — the
  normative implementation instructions
- `DVSI/AMBE-3000/AMBE-3000_Patent_Reference.md` §1 — US5701390
  algorithmic reference
- Oppenheim & Schafer, *Discrete-Time Signal Processing* (3rd ed.),
  §11.2 "Minimum-Phase Sequences" and §11.6 "The Complex Cepstrum"
  — standard references for Kolmogorov's identity and the Hilbert
  transform's role in minimum-phase signal analysis
- US5701390 col. 7 lines 25–55 — patent source text for the kernel
  definition, D = 19 recommendation, and γ = 0.44 value
