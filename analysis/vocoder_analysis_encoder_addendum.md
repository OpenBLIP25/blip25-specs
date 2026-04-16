# BABA-A Analysis Encoder Addendum — Draft

**Category:** Vocoder / Proposed spec addendum
**Relevant Specs:** TIA-102.BABA-A §5 (pages 12–22 of the PDF)
**Companion:** [`vocoder_analysis_encoder_gap.md`](./vocoder_analysis_encoder_gap.md)
**Status:** work-in-progress — subsections land here first, then get
folded into `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md`
as the new §0 once the set is complete.

This file stages the derived "Analysis Encoder: PCM → MbeParams" addendum
called for by the gap report. Subsections are transcribed from the
TIA-102.BABA-A PDF directly (not from the full-text paraphrase) and
rendered as code-ready material, mirroring the conventions of the
existing §1 decoder.

Drafted so far:

- §0.1 — Input framing, high-pass filter, and analysis windows
- §0.2 — 256-point DFT and the `S_w(m, ω₀)` basis function
- §0.3 — Initial pitch estimation (Eq. 4–23, Annex D LPF)
- §0.4 — Pitch refinement and pitch quantization (Eq. 24, 31–33, 45)
- §0.5 — Spectral amplitude estimation (Eq. 43, 44)
- §0.6 — Log-magnitude prediction residual (Eq. 52–57)
- §0.7 — V/UV determination (Eq. 34–42)
- §0.8 — Frame-type dispatch (voice / silence / tone / erasure)
- §0.9 — Encoder state structure and initialization
- §0.10 — End-to-end reference C pipeline
- §0.11 — Numerical cross-checks against DVSI test vectors

All subsections drafted. This file is complete as a first pass; the
next step is folding it into `P25_Vocoder_Implementation_Spec.md` as
the new §0, after review against DVSI test vectors (§0.11).

**Second correction to prior analysis — full-rate vs half-rate `ρ`.**
[`vocoder_decode_disambiguations.md`](./vocoder_decode_disambiguations.md)
§3 asserts `ρ = 0.65` is constant across both rates, citing Eq. 77
(full-rate decoder) and Eq. 185 (half-rate decoder; that note
originally cited Eq. 200, which is a separate frame-repeat equation
— corrected in the disambiguation note 2026-04-16). This conflates
two different rates:

- **Full-rate (Eq. 54 encoder, Eq. 77 decoder) uses the symbol `ρ`,
  defined piecewise in `L̂(0)` by Eq. 55:** 0.4 for `L̂ ≤ 15`,
  `0.03·L̂ − 0.05` for `15 < L̂ ≤ 24`, 0.7 for `L̂ > 24`. The symbol
  `ρ` appears unqualified in Eq. 77 — it is **not** a literal 0.65,
  it references Eq. 55.
- **Half-rate (Eq. 155 encoder page 60, Eq. 185 decoder page 65) uses
  the literal constant 0.65** embedded directly in the equation, not
  a named `ρ`.

The disambiguation note's "0.65 everywhere" reading is correct for
half-rate but wrong for full-rate. Full-rate implementations that hard-
code 0.65 will mispredict at the tails — at `L̂ = 9` they'll use 0.65
instead of 0.40 (163% over-prediction) and at `L̂ = 50` they'll use
0.65 instead of 0.70 (93% under-prediction). See §0.6.2 for the
transcription and value table. The disambiguation note and
implementation-spec §1.8.5 need a correction sweep, validated against
DVSI full-rate test vectors. Half-rate is fine as-stated.

**Correction to the gap report.** While drafting §0.4, a misreading in the
gap report surfaced: the quoted "Eq. 32–33" formula for `â_l`, `b̂_l`
(gap report lines 252–260) is not what the PDF says. BABA-A Eq. 32–33
define bin endpoints (the same as Eq. 26–27 but evaluated at ω̂₀), not a
complex `â + jb̂` spectral-amplitude decomposition. The actual amplitude
estimator lives in Eq. 43 (voiced) / Eq. 44 (unvoiced) and is
magnitude-only. See §0.4.3 below for the accurate transcription; the
gap report's §6 should be updated accordingly.

---

## §0.1 Input Framing, High-Pass Filter, and Analysis Windows

**Source:** TIA-102.BABA-A §3 (pages 5–6, Equation 1), §4 (pages 7–8),
§5 intro and §5.1 (pages 9–12, Equation 3, Figure 7, Figure 8). Window
sources: Annex B (`w_I`) and Annex C (`w_R`).

### 0.1.1 Sampling and Analog Front End

The vocoder input is 16-bit linear PCM at 8 kHz. A-law or μ-law
companding applied by the A-to-D converter must be removed before
encoding (per §4 prose, page 8). The recommended nominal level is
−22 dBm0 (25 dB below A-to-D saturation) to provide headroom; the
encoder itself operates at unity digital gain.

### 0.1.2 DC-Blocking High-Pass Filter — Eq. 3

Before any analysis, the PCM stream is passed through a single-pole
IIR high-pass filter:

```
         1 − z⁻¹
H(z) = ──────────                                              (Eq. 3)
       1 − 0.99 z⁻¹
```

The output of this filter is denoted `s(n)` throughout §5 of the PDF
and throughout this addendum. Every downstream equation (`E(P)` in
§0.3, `S_w(m)` in §0.2, `r(t)` in §0.3.2, …) consumes `s(n)`, **not**
the raw PCM.

Frequency response (per Figure 6 on PDF page 10): ≥ −3 dB passband
above ~30 Hz, with −20 dB at ~3 Hz and −60 dB at 0.01 Hz. The pole at
`z = 0.99` is close enough to the unit circle that the filter's
impulse response decays slowly; a cold-start transient can last
≳ 200 samples (25 ms). See §0.1.6 below for the init convention.

### 0.1.3 Frame Indexing and Lookahead — Figure 7

The present frame is centered at sample index `n = 0` for the current
analysis pass. Neighboring frames are obtained by shifting `s(n)` in
160-sample (20 ms) increments before applying the window:

| Frame label | Time offset | Pitch | Error function |
|-------------|------------:|-------|----------------|
| P_{−2}      | −40 ms      | P̂_{−2} | E_{−2}(P)      |
| P_{−1}      | −20 ms      | P̂_{−1} | E_{−1}(P)      |
| **P_0 (present)** | 0 ms  | P̂_0 → P̂_I → ω̂_0 | E_0(P) |
| P_1         | +20 ms      | P̂_1    | E_1(P)         |
| P_2         | +40 ms      | P̂_2    | E_2(P)         |

The pitch tracker (§0.3) requires two past frames' pitch and error
functions as state, and two future frames of `s(n)` in a lookahead
buffer. Spectral amplitude estimation (§0.5) and V/UV determination
(§0.7) do **not** require future-frame lookahead — only the present
frame's windowed signal.

The Annex B window extends `±150` samples, so the initial-pitch
algorithm touches samples `[−150, +150]` of the present frame. The
Annex C window extends only `±110` samples. Because `w_I` and `w_R`
are centered on the same `n = 0` (per Figure 8 / §5.1 prose, page 12),
the lookahead buffer sized for `w_I` is a superset of what `w_R`
requires.

### 0.1.4 Analysis Windows `w_I(n)` and `w_R(n)` — Figure 8

Two windows are used, applied separately at different pipeline stages:

| Window | Purpose | Length | Support `n` | Annex | CSV |
|--------|---------|-------:|-------------|-------|-----|
| `w_I(n)` | Initial pitch estimation (§0.3) | 301 | −150..150 | B | `annex_b_analysis_window.csv` |
| `w_R(n)` | Pitch refinement (§0.4), V/UV (§0.7), spectral amplitudes (§0.5) | 221 | −110..110 | C | `annex_c_pitch_refinement_window.csv` |

Both windows satisfy:

- **Symmetry:** `w(n) = w(−n)`. Verified at CSV extraction time.
- **Support:** zero outside the stated range. Implementations that
  zero-pad the input and run a fixed-length FFT are equivalent.
- **Centered on present frame:** the peak `w(0) = 1.0` (for `w_R`) and
  the peak of `w_I` both coincide at `n = 0`.
- **Alignment:** the first non-zero of `w_R(n)` (at `n = −110`) begins
  40 samples after the first non-zero of `w_I(n)` (at `n = −150`) —
  this is the Figure 8 alignment constraint and is automatically
  satisfied by the two-sided symmetric convention.

The `w_I(n)` window carries a normalization constraint (Eq. 6):

```
    150
    Σ       w_I²(j) = 1.0                                       (Eq. 6)
  j=−150
```

This constraint is already met by the values in Annex B (verified at
extraction). Implementations should **not** re-normalize on load; the
constraint matters because it fixes the scale of the `E(P)`
denominator in Eq. 5 (see §0.3.3).

Overlap between neighboring 20 ms frames is `window_length − 160`:

- `w_I`: 141 samples of overlap (301 − 160)
- `w_R`: 61 samples of overlap (221 − 160)

### 0.1.5 Windowed Signal `s_w(n)` — Eq. 1

For a generic window `w(n)` (either `w_I` or `w_R` depending on the
consumer):

```
s_w(n) = s(n) · w(n)                                             (Eq. 1)
```

This is the signal fed to §0.2's `S_w(m)` DFT (with `w = w_R`), and
whose squared envelope feeds §0.3's `E(P)` (with `w = w_I`).

### 0.1.6 HPF Initialization and Cold Start

BABA-A does not specify the HPF's initial state. The filter has one
state variable (the unit delay in the `1 − 0.99 z⁻¹` denominator). A
DVSI-compatible choice is to initialize the state to zero at power-up
and let the transient decay into the first few frames. The pitch
tracker's own cold-start (§0.3.7) and the encoder state init (§0.9,
future subsection) handle the first few frames independently, so an
extra 25 ms of HPF transient is absorbed without a separate preroll.

### 0.1.7 Reference C Pseudocode

```c
/* Persistent state across frames. */
typedef struct {
    double hpf_y1;         /* previous HPF output */
    double hpf_x1;         /* previous HPF input  */
    /* …pitch history, lookahead buffer, etc. live elsewhere (§0.9). */
} AnalysisState;

#define HPF_POLE 0.99

/* Apply Eq. 3 to a single sample: y[n] = x[n] − x[n−1] + 0.99 y[n−1]. */
static double hpf_step(AnalysisState *st, double x) {
    double y = x - st->hpf_x1 + HPF_POLE * st->hpf_y1;
    st->hpf_x1 = x;
    st->hpf_y1 = y;
    return y;
}

/* Run the HPF over a 160-sample input frame. Producing s(n) in-place. */
void hpf_frame(AnalysisState *st, double pcm[160], double s[160]) {
    for (int i = 0; i < 160; ++i) s[i] = hpf_step(st, pcm[i]);
}
```

The windows `w_I[]` and `w_R[]` are loaded from their Annex CSVs at
startup; the `s_w(n) = s(n) · w(n)` multiplication is inlined into the
consumers of §0.2 and §0.3 rather than materialized as a separate
buffer.

### 0.1.8 Common Pitfalls

- **Two windows, not one.** §0.3 (initial pitch) uses `w_I`;
  §0.4/§0.5/§0.7 (refinement, amplitudes, V/UV) use `w_R`. Confusing
  the two produces plausible-looking but wrong results — the Annex B
  CSV is longer and differently shaped than Annex C.
- **Do not re-normalize windows.** Eq. 6 is already satisfied by the
  Annex B values. Applying `w_I ← w_I / √(Σ w_I²)` changes nothing on
  compliant data and breaks the scale on slightly-perturbed data.
- **Frame shift, not input re-buffering.** "Previous and future speech
  frames are obtained by shifting the speech signal in 160-sample
  increments prior to the application of the window" (§5.1, page 11).
  Implementations should keep a single `s(n)` buffer and re-index into
  it with an offset, not memmove the samples.
- **HPF before analysis.** Every algorithmic `s(n)` in this addendum
  is post-HPF. Forgetting the HPF (or running it once per frame with
  reset state) introduces a sub-30-Hz DC component that throws off
  `E(P)` in low-energy (whispered or silent) frames.

---

## §0.2 256-point DFT and the `S_w(m, ω₀)` Basis Function

**Source:** TIA-102.BABA-A §5.1.5 "Pitch Refinement", pages 16–17 of the
PDF (document pages 16–17, Equations 24–30). Window source is Annex C,
`annex_tables/annex_c_pitch_refinement_window.csv` (221 values,
`n = −110..110`, symmetric, peak `w_R(0) = 1.0`).

This subsection defines the harmonically-placed analysis basis that
appears in three later subsections simultaneously: §0.4 pitch refinement
(residual `E_R(ω₀)`), §0.5 spectral amplitude estimation (the `â_l`,
`b̂_l` extractor), and §0.7 V/UV determination (the voicing measure
`D_k`). All three consume `S_w(m)` and `S_w(m, ω₀)` unchanged; this
subsection is the single definition site.

### 0.2.1 The Signal Spectrum `S_w(m)` — Eq. 29

The 256-point DFT of the windowed input signal is:

```
S_w(m) = Σ_{n=−110}^{110} s(n) · w_R(n) · e^{−j 2π m n / 256}
                                                     for −127 ≤ m ≤ 128   (Eq. 29)
```

- `s(n)` is the 8 kHz PCM input aligned such that `n = 0` is the center
  of the current analysis window (alignment inherited from §0.1).
- `w_R(n)` is the Annex C pitch-refinement window, zero outside
  `|n| ≤ 110`. Load from
  `standards/TIA-102.BABA-A/annex_tables/annex_c_pitch_refinement_window.csv`.
- The summation is written over the window support. Values `|n| > 110`
  are zero and contribute nothing; implementations that zero-pad to 256
  before running a stock 256-point FFT produce the same `S_w(m)`.
- The output index range `−127 ≤ m ≤ 128` is the natural two-sided DFT
  indexing; standard packed FFT output `m ∈ [0, 255]` corresponds via
  `S_w(m − 256) = S_w(m)` for `m ∈ [128, 255]` (periodicity of the DFT).

### 0.2.2 The Window Spectrum `W_R(m)` — Eq. 30

The 16384-point DFT of the window alone is:

```
W_R(m) = Σ_{n=−110}^{110} w_R(n) · e^{−j 2π m n / 16384}
                                                     for −8191 ≤ m ≤ 8192  (Eq. 30)
```

Because `w_R(n)` is real and symmetric (`w_R(n) = w_R(−n)`, verified at
extraction time — see the CSV header in `annex_c_pitch_refinement_window.csv`),
`W_R(m)` is real-valued:

```
W_R*(m) = W_R(m)                                         (symmetry corollary)
```

This identity is used implicitly in Eq. 28 below: `W_R*` in the
numerator can be replaced by `W_R` in any real-valued implementation.

`W_R(m)` is a purely window-dependent quantity. It has no dependence on
the input signal or on `ω₀`, and can be precomputed once at startup.
The factor of 64 in `64m` inside Eq. 25 and Eq. 28 arises from the
frequency-resolution ratio `16384 / 256 = 64`: moving by one 256-bin is
the same as moving by 64 16384-bins.

### 0.2.3 The Synthetic Spectrum `S_w(m, ω₀)` — Eq. 25

The synthetic spectrum is constructed piecewise per harmonic `l`. For
harmonic index `l = 0, 1, 2, …` with integration support `⌈a_l⌉ ≤ m < ⌈b_l⌉`:

```
              ⎧ A_0(ω₀) · W_R(64m)                                  for ⌈a_0⌉ ≤ m < ⌈b_0⌉
S_w(m, ω₀) =  ⎨ A_1(ω₀) · W_R(⌊64m − (16384/2π)·ω₀ + 0.5⌋)         for ⌈a_1⌉ ≤ m < ⌈b_1⌉   (Eq. 25)
              ⎩ A_l(ω₀) · W_R(⌊64m − (16384/2π)·l·ω₀ + 0.5⌋)       for ⌈a_l⌉ ≤ m < ⌈b_l⌉
```

Where `⌈x⌉` denotes the smallest integer greater than or equal to `x`
(i.e. ceiling, per the PDF's notation), and `⌊x⌋` denotes the largest
integer less than or equal to `x` (i.e. truncation toward −∞).

The three rows in the piecewise definition are:

- **Row 1 (l = 0):** the DC/lowest-frequency partial. The window
  spectrum is sampled at integer 256-bin positions scaled by 64
  (i.e. `W_R(64m)` for `m = 0, 1, 2, …`) with no harmonic offset.
- **Row 2 (l = 1):** the fundamental. The window spectrum is centered
  on the fundamental frequency; the `(16384/2π)·ω₀` term converts `ω₀`
  (radians/sample) into a 16384-point DFT bin index, and the `+ 0.5`
  plus floor implements round-to-nearest.
- **Row 3 (l ≥ 2):** the `l`-th harmonic, centered on `l·ω₀`.

Row 1 is formally the `l = 0` case of Row 3 (with `l·ω₀ = 0`, the offset
vanishes and `⌊64m + 0.5⌋ = 64m`), so implementations may use the
unified form in Row 3 for all `l ≥ 0`. The PDF renders `l = 0` separately
only to avoid the redundant `+ 0.5` rounding on an already-integer argument.

### 0.2.4 Per-Harmonic Bin Endpoints `a_l`, `b_l` — Eq. 26, 27

The start and end of each harmonic's 256-bin support are:

```
a_l = (256 / 2π) · (l − 0.5) · ω₀                                        (Eq. 26)
b_l = (256 / 2π) · (l + 0.5) · ω₀                                        (Eq. 27)
```

These are real-valued bin positions; the integer support in Eq. 25 and
Eq. 28 uses the half-open interval `⌈a_l⌉ ≤ m < ⌈b_l⌉`. Equivalently,
each harmonic's support is centered on the 256-bin position
`m_l = (256/2π) · l · ω₀` with half-width `(256/2π) · 0.5 · ω₀ = 128ω₀/(2π) ·  0.5`.

**Numerical example.** For `ω₀ = 2π / 50` (50-sample pitch, mid-range),
`(256/2π)·ω₀ = 256/50 = 5.12`. Then:

| l | `a_l`  | `b_l`  | `⌈a_l⌉` | `⌈b_l⌉` | support (m) |
|---|-------:|-------:|-------:|-------:|-------------|
| 0 | −2.56  |  2.56  | −2     |  3     | {−2,−1,0,1,2} |
| 1 |  2.56  |  7.68  |  3     |  8     | {3,4,5,6,7} |
| 2 |  7.68  | 12.80  |  8     | 13     | {8,9,10,11,12} |
| … |  …     |  …     |  …     |  …     | … |

The supports are contiguous and non-overlapping: `⌈b_l⌉ = ⌈a_{l+1}⌉` by
construction (since `b_l = a_{l+1}`).

### 0.2.5 Harmonic Amplitude `A_l(ω₀)` — Eq. 28

The complex harmonic amplitude used to build `S_w(m, ω₀)` is:

```
               Σ_{m=⌈a_l⌉}^{⌈b_l⌉−1}  S_w(m) · W_R*(⌊64m − (16384/2π)·l·ω₀ + 0.5⌋)
A_l(ω₀) =  ─────────────────────────────────────────────────────────────────────────         (Eq. 28)
               Σ_{m=⌈a_l⌉}^{⌈b_l⌉−1}  | W_R(⌊64m − (16384/2π)·l·ω₀ + 0.5⌋) |²
```

- Numerator is complex (inherits the phase of `S_w(m)`); denominator is
  real (sum of squared magnitudes).
- By §0.2.2, `W_R*(·) = W_R(·)`, so the conjugate in the numerator is a
  no-op for `w_R(n)` real-symmetric; implementations can drop it.
- The denominator is the window-energy normalization: it makes
  `A_l(ω₀)` the least-squares projection of `S_w(·)` onto the translated
  window `W_R(· − l·ω₀)` within the harmonic's support.
- `A_l(ω₀)` is distinct from the final spectral-amplitude estimates
  `â_l = Re[A_l(ω̂₀)]`, `b̂_l = Im[A_l(ω̂₀)]` of §0.5 / Eq. 32–33. Within
  §0.2 / Eq. 28, the `2/256` prefactor of Eq. 32–33 is **not** present —
  the denominator's window-energy normalization plays that role here.
  The `2/256` reappears when the PDF's §5.3 redefines `â_l`, `b̂_l` at
  the selected `ω̂₀` for quantizer input (see §0.5 of this addendum,
  forthcoming).

### 0.2.6 Pitch Refinement Residual `E_R(ω₀)` — Eq. 24

For context (the consumer of `S_w(m, ω₀)` in §0.4):

```
                    ⌊ |0.9254π/ω₀ − 0.5| · 256ω₀/(2π) ⌋
E_R(ω₀) =  Σ        | S_w(m) − S_w(m, ω₀) |²                                                  (Eq. 24)
                m = 50
```

The lower limit `m = 50` and the ω₀-dependent upper limit restrict the
sum to a band above DC and below the Nyquist-proximate cutoff. The
upper-limit expression simplifies (for positive ω₀ with
`0.9254π/ω₀ > 0.5`, which holds for all admissible ω₀) to:

```
m_max = ⌊ (0.9254π/ω₀ − 0.5) · 256ω₀/(2π) ⌋
      = ⌊ 0.9254 · 128 − 64 ω₀ / π ⌋
      = ⌊ 118.45 − 64 ω₀ / π ⌋
```

§0.4 of this addendum (pitch refinement) evaluates `E_R(ω₀)` over the
ten candidates `P̂_I − 9/8, P̂_I − 7/8, …, P̂_I + 9/8` (per Eq. 24
prose) and selects the `ω̂₀` that minimizes it. This subsection only
defines `S_w(m, ω₀)`; the minimization loop lives in §0.4.

### 0.2.7 Reference C Pseudocode

```c
/* Precompute at startup (depends only on the window): */
/*   WR[k] = DFT_16384(wR)[k]  for k in some range we actually index.    */
/*   Since wR is real-symmetric, WR[k] is real.                          */
/*   Because we only ever index WR at offsets bounded by the harmonic    */
/*   support width (≈ 128 ω₀ / (2π) · 64), a 16384-bin table is more      */
/*   than enough; in practice only ~|k| < 1024 entries are touched.       */

#define N_DFT       256
#define NW_DFT    16384
#define M_LOWER     50           /* lower bound of Eq. 24 sum */

static double WR[NW_DFT];          /* W_R(m), real, precomputed */
static double complex Sw[N_DFT];   /* S_w(m), current frame */

/* Build S_w(m, ω₀) and the harmonic projections A_l(ω₀) for a candidate ω₀. */
/* out_Sw_synth[m]  receives  S_w(m, ω₀)  for m ∈ [0, N_DFT/2].                 */
/* out_Al[l]         receives  A_l(ω₀)                 for l ∈ [0, L_max).      */
void synthesize_Sw(double omega0,
                   double complex out_Sw_synth[N_DFT],
                   double complex out_Al[],
                   int L_max) {
    const double bin_scale_256   = 256.0   / (2.0 * M_PI);
    const double bin_scale_16384 = 16384.0 / (2.0 * M_PI);

    for (int l = 0; l <= L_max; ++l) {
        double a_l = bin_scale_256 * ((double)l - 0.5) * omega0;
        double b_l = bin_scale_256 * ((double)l + 0.5) * omega0;
        int m_lo = (int)ceil(a_l);           /* ⌈a_l⌉ */
        int m_hi = (int)ceil(b_l);           /* ⌈b_l⌉, exclusive upper */

        /* Eq. 28: A_l(ω₀) = Σ S_w(m) WR(k) / Σ |WR(k)|²           */
        double complex num = 0.0 + 0.0 * I;
        double         den = 0.0;
        for (int m = m_lo; m < m_hi; ++m) {
            /* k = ⌊64m − (16384/2π) l ω₀ + 0.5⌋                     */
            double k_real = 64.0 * (double)m
                          - bin_scale_16384 * (double)l * omega0
                          + 0.5;
            int k = (int)floor(k_real);
            double wr = WR[wr_index(k)];     /* two-sided → array index */
            num += Sw[dft_index_256(m)] * wr;  /* WR real ⇒ WR* = WR      */
            den += wr * wr;
        }
        out_Al[l] = (den > 0.0) ? (num / den) : 0.0;

        /* Eq. 25: S_w(m, ω₀) on this harmonic's support */
        for (int m = m_lo; m < m_hi; ++m) {
            double k_real = 64.0 * (double)m
                          - bin_scale_16384 * (double)l * omega0
                          + 0.5;
            int k = (int)floor(k_real);
            out_Sw_synth[m] = out_Al[l] * WR[wr_index(k)];
        }
    }
}
```

Helpers `wr_index(k)` and `dft_index_256(m)` translate two-sided indices
(negative allowed) into the corresponding packed array offsets; both
rely on the DFT's periodicity and on `w_R` being real to map `W_R(−k) =
W_R(k)`.

### 0.2.8 Common Pitfalls

- **Conjugate convention.** The numerator in Eq. 28 uses `W_R*(·)`, not
  `S_w*(·)`. Flipping which factor is conjugated inverts the sign of
  `Im[A_l]`, which downstream shows up as inverted `b̂_l` in §0.5 and a
  180° phase error in the synthesized output. For real-symmetric `w_R`
  the ambiguity collapses (the conjugate is a no-op), but the
  implementation should still encode the correct slot.
- **Floor vs ceiling.** The PDF uses `⌈·⌉` in Eq. 25–28 support bounds
  and `⌊· + 0.5⌋` inside the `W_R` argument. These are different
  operations; do not collapse both to `round()`.
- **Absolute value in Eq. 24 upper limit.** `|0.9254π/ω₀ − 0.5|` is
  outer to the ω₀-dependent expression inside the floor. For the
  admissible ω₀ range (`2π/123.125 ≤ ω₀ ≤ 2π/19.875`) the argument is
  always positive, so the absolute value is vestigial in practice; code
  that omits it is still correct on-range but will silently differ off-range.
- **`S_w(m)` indexing.** The PDF writes `S_w(m)` two-sided
  (`−127 ≤ m ≤ 128`). A stock FFT returns `m ∈ [0, N)`. Do not evaluate
  Eq. 25–28 on the packed range directly; translate via DFT
  periodicity or stick with the two-sided convention throughout.
- **W_R precomputation size.** `W_R(m)` is defined for
  `−8191 ≤ m ≤ 8192`, but the range actually indexed by Eq. 25–28 is
  narrow: `|k| ≲ 32 · ω₀ · 256/π ≈ small`. A 16384-entry table is
  overkill in RAM but matches the PDF's definition exactly. Shrinking
  the table is a premature optimization until the analysis encoder is
  numerically matched to DVSI.
- **γ_w cross-reference.** The decoder-side unvoiced-synthesis scale
  `γ_w` (derived from window energies of `w_S · w_R`) is flagged in
  commit `741eeef` as ~150× off DVSI-measured output. The encoder-side
  `Σ |W_R(·)|²` denominator in Eq. 28 carries the same window-energy
  DNA. If the γ_w investigation resolves to a `w_R` normalization
  convention (vs. a synthesis-only bug), Eq. 28's denominator may
  inherit the fix. Flag, do not pre-apply.

---

## §0.3 Initial Pitch Estimation

**Source:** TIA-102.BABA-A §5.1.1 "Determination of E(P)" through
§5.1.4 "Look-Ahead Pitch Tracking" (pages 12–15, Equations 5–23).
LPF source: Annex D (21-tap FIR, inlined in the impl spec at §7.4).
Windows: §0.1.4 above (uses `w_I`, Annex B).

The initial pitch algorithm has three stages: (a) compute an error
function `E(P)` over a grid of candidate pitch periods, (b) run two
independent tracking branches ("look-back" and "look-ahead") that
each pick one best candidate, (c) compare the two with a small
decision table to produce `P̂_I`. The candidate grid is:

```
P ∈ {21, 21.5, 22, 22.5, …, 121.5, 122}     (203 half-sample values)  (Eq. 11)
```

### 0.3.1 Low-Pass-Filtered Signal `s_LPF(n)` — Eq. 9

`E(P)` operates on an LPF'd version of `s(n)` rather than on `s(n)`
directly:

```
            10
s_LPF(n) =  Σ   s(n − j) · h_LPF(j)                              (Eq. 9)
           j=−10
```

`h_LPF` is the 21-tap symmetric FIR in Annex D, already transcribed in
the implementation spec at §7.4. The filter's nominal cutoff is
≈ 1 kHz — it removes the upper vocal-tract formants so the pitch
tracker is driven primarily by the glottal excitation's low-frequency
content. The support `j ∈ [−10, 10]` is symmetric; boundary handling
at the edges of the 301-sample `w_I` support is by zero-extension of
`s(n)`.

### 0.3.2 Autocorrelation `r(t)` — Eq. 7, 8

For integer `t`:

```
         150
r(t) =   Σ       s_LPF(j) · w²_I(j) · s_LPF(j+t) · w²_I(j+t)      (Eq. 7)
       j=−150
```

Note that `s_LPF` appears to the first power and `w_I` to the second
(squared). `r(t)` is the autocorrelation of `s_LPF(j) · w²_I(j)` — the
LPF'd signal weighted by the *squared* analysis window. This is a
linear autocorrelation of a window-squared envelope, not a correlation
of `s²_LPF`. (See `analysis/vocoder_analysis_eq7_correction.md` for
the correction history; an earlier draft of this addendum had `s_LPF`
squared on both factors and produced unbounded `E(P)` on voiced
content.)

For non-integer `t` (needed because `P` takes half-sample values),
linear interpolation between neighboring integer-`t` evaluations:

```
r(t) = (1 + ⌊t⌋ − t) · r(⌊t⌋) + (t − ⌊t⌋) · r(⌊t⌋ + 1)           (Eq. 8)
```

`⌊x⌋` is truncation toward −∞ (per the PDF's definition on page 13).

### 0.3.3 Error Function `E(P)` — Eq. 5, 6

```
             Σ_{j=−150}^{150} s²_LPF(j) w²_I(j)  −  P · Σ_{n=−⌊150/P⌋}^{⌊150/P⌋} r(n·P)
E(P)  =  ─────────────────────────────────────────────────────────────────────────────      (Eq. 5)
             [ Σ_{j=−150}^{150} s²_LPF(j) w²_I(j) ] · [ 1 − P · Σ_{j=−150}^{150} w⁴_I(j) ]
```

Eq. 6 (normalization of `w_I`) is what makes the denominator
dimensionless and roughly `O(1)`. Because `Σ w_I² = 1.0`, the bracket
`[1 − P · Σ w_I⁴]` depends only on the fourth moment of `w_I` — a
frame-independent constant that can be precomputed once at startup.

**Interpretation.** The outer sum in the numerator is the total
windowed energy. The inner sum picks off `r(n·P)` at integer multiples
of `P` (i.e. lag values where the energy envelope auto-correlates
strongly if `P` is the true pitch period). When `P` matches the true
pitch, `E(P)` is small; when it doesn't, `E(P)` is close to 1. The
tracking stages that follow use this "close to 0 is good" convention.

**Numerical guard.** For silent or near-silent frames the outer
energy sum `Σ s²_LPF w²_I` approaches zero; the denominator then also
approaches zero and `E(P)` is ill-defined. Initialization (§0.3.7) and
silence dispatch (§0.8, future subsection) handle this case; a
robustness convention is to set `E(P) = 1.0` when the energy sum falls
below a tiny floor (e.g. `1e−12`).

**Diagnostic.** If an implementation observes `E(P)` diverging to
magnitudes far outside `[0, 1]` on voiced content, the most likely
cause is a mis-transcription of Eq. 7 with `s_LPF` squared: a
squared-signal formulation of `r(t)` has units `[s⁴]` and breaks
this numerator's subtraction. See
`analysis/vocoder_analysis_eq7_correction.md` for the correction
history.

### 0.3.4 Look-Back Pitch Tracking — Eq. 10–12

Using `P̂_{−1}` from the previous frame's analysis:

```
0.8 · P̂_{−1} ≤ P ≤ 1.2 · P̂_{−1}                                (Eq. 10)
P ∈ {21, 21.5, …, 121.5, 122}                                   (Eq. 11)

P̂_B = argmin E(P)  subject to (10), (11)
```

Then the backward cumulative error:

```
CE_B(P̂_B) = E(P̂_B) + E_{−1}(P̂_{−1}) + E_{−2}(P̂_{−2})         (Eq. 12)
```

`E_{−1}` and `E_{−2}` are the error functions from the previous two
frames' analyses, evaluated at **their** selected pitches — these are
stored as part of the encoder state (§0.9). They are **not**
re-evaluations of Eq. 5 at `P̂_{−1}` on the current windowed frame;
they are frozen scalars inherited from past frames.

### 0.3.5 Look-Ahead Pitch Tracking — Eq. 13–20

Look-ahead treats the current pitch candidate `P_0` as free and, for
each `P_0`, finds the `(P_1, P_2)` pair from the next two frames'
error functions `E_1(·)`, `E_2(·)` that jointly minimize
`E_1(P_1) + E_2(P_2)` under pitch-continuity constraints:

```
P_1 ∈ {21, 21.5, …, 121.5, 122}                                 (Eq. 13)
0.8 · P̂_0 ≤ P_1 ≤ 1.2 · P̂_0                                    (Eq. 14)
P_2 ∈ {21, 21.5, …, 121.5, 122}                                 (Eq. 15)
0.8 · P̂_1 ≤ P_2 ≤ 1.2 · P̂_1                                    (Eq. 16)

P̂_1(P_0), P̂_2(P_0) = argmin [E_1(P_1) + E_2(P_2)]  s.t. (13)–(16)

CE_F(P̂_0) = E(P̂_0) + E_1(P̂_1) + E_2(P̂_2)                     (Eq. 17)
```

Then `P̂_0` is the value minimizing `CE_F(P_0)` over the full candidate
set:

```
P̂_0 = argmin CE_F(P_0)   over P_0 ∈ {21, 21.5, …, 122}
```

**Sub-multiple check.** Once `P̂_0` is found, its integer sub-multiples
`P̂_0/2, P̂_0/3, …, P̂_0/n` are tested in order from smallest to
largest. Each sub-multiple is first snapped to the nearest member of
`{21, 21.5, …, 122}` (MSE closeness); sub-multiples that snap below
21 are discarded.

For the smallest snapped sub-multiple `P̂_0/n`, the following cascade
is evaluated (if **any** of Eq. 18/19/20 is satisfied, that sub-multiple
is accepted as `P̂_F`):

```
CE_F(P̂_0/n) ≤ 0.85   AND   CE_F(P̂_0/n) / CE_F(P̂_0) ≤ 1.7      (Eq. 18)
CE_F(P̂_0/n) ≤ 0.40   AND   CE_F(P̂_0/n) / CE_F(P̂_0) ≤ 3.5      (Eq. 19)
CE_F(P̂_0/n) ≤ 0.05                                             (Eq. 20)
```

If none of the cascade rows holds for the smallest sub-multiple, the
next-smallest is tried with the same test, and so on. If no
sub-multiple satisfies any row, `P̂_F = P̂_0`.

This sub-multiple search is an octave-doubling correction: pitch
trackers tend to lock onto 2× or 3× the true pitch when the true
fundamental has low energy but its harmonics do. The cascade
progressively tightens the threshold for accepting a sub-multiple
(the lower the absolute `CE_F`, the more trusted the sub-multiple),
ensuring the "shorter pitch" hypothesis has to clear a higher bar to
override the primary minimum.

### 0.3.6 Backward-vs-Forward Decision — Eq. 21–23

Finally, `P̂_I` is chosen from `{P̂_B, P̂_F}`:

```
if     CE_B(P̂_B) ≤ 0.48            then  P̂_I = P̂_B              (Eq. 21)
elif   CE_B(P̂_B) ≤ CE_F(P̂_F)       then  P̂_I = P̂_B              (Eq. 22)
else                                      P̂_I = P̂_F              (Eq. 23)
```

The first rule biases toward pitch continuity: if the backward
hypothesis is strong in absolute terms (cumulative error ≤ 0.48), it
wins regardless of the forward hypothesis. Only when the backward
hypothesis is weak do the two compete on equal footing.

### 0.3.7 Initialization (Cold Start)

Per §5.1.3 prose (page 14): "Upon initialization the error functions
`E_{−1}(P)` and `E_{−2}(P)` are assumed to be equal to zero, and
`P̂_{−1}` and `P̂_{−2}` are assumed to be equal to 100."

Implementation:

```
P̂_{−1}(t=0) = 100.0        /* half-sample units, in-range of {21..122} */
P̂_{−2}(t=0) = 100.0
E_{−1}(P̂_{−1})(t=0) = 0.0  /* scalar */
E_{−2}(P̂_{−2})(t=0) = 0.0  /* scalar */
```

The zero init makes `CE_B` artificially small on frame 0 and biases
toward `P̂_I = P̂_B` via Eq. 21 (since `0.0 ≤ 0.48`). This is intended
behavior: the first frame's pitch is derived almost entirely from its
own `E(P)` evaluated on the look-back grid near `P = 100`. The
look-ahead branch still runs and can override if `CE_B` becomes
larger than `CE_F` in later frames.

The forward buffer — `E_1(·)` and `E_2(·)` for the first frame —
requires two real frames of future lookahead, so the encoder cannot
produce a valid `P̂_I` until frame 2 of the input. The first two
frames should emit silence or an erasure placeholder; see §0.8.

### 0.3.8 Reference C Pseudocode

```c
/* Precomputed constants (one-time, from the Annex B CSV). */
static double wI[301];             /* w_I(n), n = −150..150, indexed as wI[n+150] */
static double wI_sqsum;             /* Σ w_I²(j) = 1.0 per Eq. 6 */
static double wI_4thmom;           /* Σ w_I⁴(j), frame-independent constant */
static double hLPF[21];            /* Annex D FIR, inlined from impl spec §7.4 */

/* Per-frame: compute s_LPF, then r(t) for t in the needed range.
 *
 * Hoist sw2[j] = s_LPF(j) · w²_I(j) out of the (j, t) double loop —
 * it's reused across all 123 lag values (t = 0..122). The naive form
 * recomputes sLPF*wI*wI inside the inner loop ~74k times per frame. */
static void compute_r(double sLPF[], double r[]) {
    double sw2[301];
    for (int j = -150; j <= 150; ++j) {
        sw2[j+150] = sLPF[j+150] * wI[j+150] * wI[j+150];
    }
    for (int t = 0; t <= 122; ++t) {
        double acc = 0.0;
        for (int j = -150; j <= 150; ++j) {
            int jt = j + t;
            if (jt < -150 || jt > 150) continue;   /* zero outside w_I support */
            acc += sw2[j+150] * sw2[jt+150];
        }
        r[t] = acc;
    }
}

/* Linear-interpolated r(t) for non-integer t. */
static double r_interp(double r[], double t) {
    int tf = (int)floor(t);
    double frac = t - (double)tf;
    return (1.0 - frac) * r[tf] + frac * r[tf + 1];
}

/* Eq. 5, with a small floor on the energy to guard silence. */
static double E_of_P(double sLPF[], double r[], double P) {
    double energy = 0.0;
    for (int j = -150; j <= 150; ++j) {
        double sw = sLPF[j+150] * wI[j+150];
        energy += sw * sw;
    }
    if (energy < 1e-12) return 1.0;

    double num_inner = 0.0;
    int nmax = (int)floor(150.0 / P);
    for (int n = -nmax; n <= nmax; ++n) {
        num_inner += r_interp(r, fabs((double)n * P));
    }
    double num   = energy - P * num_inner;
    double denom = energy * (1.0 - P * wI_4thmom);
    return num / denom;
}
```

The look-back and look-ahead searches are straightforward argmin scans
over the half-sample grid; the sub-multiple cascade is a short loop
over `n = 2, 3, …` with the snap-to-grid step.

### 0.3.9 Common Pitfalls

- **`r(t)` is a linear autocorrelation of `s_LPF · w²_I`, not of
  `s_LPF`** (plain) and **not** of `(s_LPF · w_I)²`. The
  distinguishing feature is the window's second power — the signal
  itself appears linearly.

  Concretely: build the per-frame sequence `sw2[j] = s_LPF(j) ·
  w²_I(j)` for `j ∈ [−150, 150]` (i.e. the LPF'd signal pre-multiplied
  by the squared window), then pass `sw2[]` to a generic **unwindowed**
  linear autocorrelation. Routines that apply their own window
  internally (as many DSP libraries do — scipy's `correlate`,
  FFTW-based helpers that window before the FFT, etc.) will
  double-window and produce the wrong answer; disable any built-in
  windowing or pass a rectangular window. Callers who pass `s_LPF(j) ·
  w_I(j)` (single window power) or `(s_LPF(j) · w_I(j))²` (the
  previously-documented incorrect form) will also not match the PDF.
  See `analysis/vocoder_analysis_eq7_correction.md`.
- **`E_{−1}`, `E_{−2}` are scalars, not functions.** The PDF notation
  `E_{−1}(P̂_{−1})` means "the value `E_{−1}` took at the past
  frame's selected pitch". Storing the entire past error function is
  not required (and not what DVSI does).
- **Cold-start bias.** Frame 0 almost always returns `P̂_I = P̂_B`
  because `CE_B = 0.0 + 0.0 + E(P̂_B) ≤ 0.48` trivially. Do not
  interpret this as a real pitch on frame 0 — the encoder shouldn't
  emit voice output until enough state has accumulated.
- **Sub-multiple cascade semantics.** "If any of Eq. 18/19/20 holds"
  is a disjunction across the three rows applied to a single
  sub-multiple, **not** a disjunction where each row selects a
  different sub-multiple. Read §5.1.4 carefully: the cascade is tried
  per-sub-multiple, smallest first; the first sub-multiple that
  passes any row wins.
- **Silence and the denominator.** `1 − P · Σ w_I⁴` can go negative if
  `P` gets large enough. For the admissible range `P ∈ [21, 122]` and
  the Annex B values of `w_I`, the bracket stays positive, but an
  explicit sanity-check at startup (not per-frame) is cheap insurance.

---

## §0.4 Pitch Refinement and Pitch Quantization

**Source:** TIA-102.BABA-A §5.1.5 "Pitch Refinement" (pages 16–17,
Equations 24–33) and §6.1 "Fundamental Frequency Encoding and
Decoding" (pages 21–22, Equation 45). Consumes §0.2's `S_w(m, ω₀)`
basis and §0.3's `P̂_I`.

This subsection takes the half-sample-resolution `P̂_I ∈ {21, 21.5,
…, 122}` from §0.3 and refines it to quarter-sample resolution `ω̂_0`,
then quantizes `ω̂_0` to the 8-bit `b̂_0` that enters the bitstream.
Along the way it derives `L̂` (number of harmonics) and the bin
endpoints `â_l`, `b̂_l` used by §0.5 and §0.7.

### 0.4.1 Ten Refinement Candidates

Ten candidate pitches are formed from `P̂_I` (per §5.1.5 prose,
page 16):

```
P ∈ { P̂_I − 9/8, P̂_I − 7/8, P̂_I − 5/8, P̂_I − 3/8, P̂_I − 1/8,
      P̂_I + 1/8, P̂_I + 3/8, P̂_I + 5/8, P̂_I + 7/8, P̂_I + 9/8 }
```

The spacing is 0.25 samples (`1/4`); the range is `±1.125` samples
around `P̂_I`. `P̂_I` itself is **not** a candidate — the smallest
offset is `±1/8`. This is a deliberate choice: `P̂_I` is already on
the half-sample grid, and the refinement must produce a value on the
quarter-sample offset grid.

Each candidate pitch `P` is converted to a candidate fundamental
frequency `ω_0` via Eq. 4:

```
ω_0 = 2π / P                                                     (Eq. 4)
```

### 0.4.2 Residual Minimization — Eq. 24

For each candidate `ω_0`, evaluate the residual `E_R(ω_0)` of Eq. 24,
repeated here from §0.2.6:

```
                    ⌊ (0.9254π/ω_0 − 0.5) · 256ω_0/(2π) ⌋
E_R(ω_0)  =   Σ        | S_w(m) − S_w(m, ω_0) |²                (Eq. 24)
                m = 50
```

`S_w(m)` is the 256-pt DFT of the present frame (Eq. 29, computed
once per frame with `w_R`). `S_w(m, ω_0)` is the synthetic spectrum
of §0.2.3 (rebuilt per-candidate). The refined fundamental is:

```
ω̂_0 = argmin E_R(ω_0)   over the 10 candidates
```

**Tie-breaking.** If two candidates tie on `E_R`, the PDF does not
prescribe a rule. A natural convention is to pick the candidate
closer to `P̂_I` (equivalent to lower quantizer-step index via Eq. 45),
which minimizes bitstream churn on borderline frames. Ties are rare
enough in practice (floating-point `E_R` collisions) that the
tie-breaking rule matters only for bit-exactness tests against DVSI
test vectors; for correctness, any deterministic rule suffices.

**Residual computation cost.** The 10-candidate scan evaluates
Eq. 25–28 (§0.2) ten times per frame. Eq. 28's denominator
`Σ |W_R(·)|²` varies across candidates because the translate offset
`(16384/2π)·l·ω_0` shifts — so it cannot be precomputed across
candidates. Eq. 29's `S_w(m)` **can** be precomputed once per frame
and reused across all 10 candidates.

### 0.4.3 Derived Parameters — Eq. 31, 32, 33

Once `ω̂_0` is selected, three derived quantities follow:

```
L̂ = ⌊ 0.9254 · ( π/ω̂_0 + 0.25 ) ⌋                              (Eq. 31)
```

With `ω̂_0 ∈ [2π/123.125, 2π/19.875]`, Eq. 31 confines `L̂` to
`9 ≤ L̂ ≤ 56`. This matches the implementation spec's
`Harmonics L: 9 to 56` entry at §1.1.

The per-harmonic bin endpoints at the refined fundamental are:

```
â_l = (256 / 2π) · (l − 0.5) · ω̂_0                             (Eq. 32)
b̂_l = (256 / 2π) · (l + 0.5) · ω̂_0                             (Eq. 33)
```

**These are bin endpoints, not complex amplitude projections.** They
are structurally identical to Eq. 26–27 of §0.2, with `ω̂_0` in place
of the candidate `ω_0`. The harmonic-`l` spectral region is `⌈â_l⌉ ≤ m < ⌈b̂_l⌉`,
and §0.5 and §0.7 both consume these bounds verbatim.

> **⚠ Correction to the gap report.** Section 6 of
> `vocoder_analysis_encoder_gap.md` (lines 252–260) quotes from the
> full-text paraphrase a formula of the form
> `â_l = (2/256) · Re[ Σ S_w(m) · S_w*(m,l) ]`, implying a complex
> `â + jb̂` decomposition of the spectrum. The PDF does not say this.
> BABA-A Eq. 32–33 define only bin endpoints; the actual spectral
> amplitude estimator is Eq. 43 (voiced band) / Eq. 44 (unvoiced
> band), both of which are magnitude-only expressions that feed
> directly into `M̂_l = ...` without going through any `â²+b̂²` step.
> §0.5 of this addendum (forthcoming) will render Eq. 43–44 correctly;
> the gap report's §6 should be edited once §0.5 lands.

### 0.4.4 Pitch Quantization — Eq. 45

The refined fundamental is quantized to 8 bits with midtread
convention:

```
b̂_0 = ⌊ 4π / ω̂_0  −  39 ⌋                                       (Eq. 45)
```

Range: `0 ≤ b̂_0 ≤ 207`. The 48 values `208 ≤ b̂_0 ≤ 255` are reserved
(per §6.1 prose, page 22).

**Encoder/decoder half-step asymmetry.** Eq. 45 uses `⌊ … − 39 ⌋`
(encoder); Eq. 46 uses `ω̃_0 = 4π / (b̃_0 + 39.5)` (decoder). The
asymmetry between `−39` and `+39.5` is intentional and implements
midtread quantization: the encoder's floor operation rounds toward
the lower bin boundary, while the decoder's `+39.5` offset
reconstructs the bin **center**. Round-trip:

```
ω̂_0  →  b̂_0 = ⌊4π/ω̂_0 − 39⌋  →  ω̃_0 = 4π/(⌊4π/ω̂_0 − 39⌋ + 39.5)
```

For an `ω̂_0` that lands exactly on a quantization level, the
decoder's reconstructed `ω̃_0` is offset from `ω̂_0` by at most
`½` quantization step of `4π/(b̂_0 + 39.5)²` rad/sample — well within
the refinement's quarter-sample resolution.

**Trap to avoid:** implementers who mirror the decoder and use
`b̂_0 = ⌊4π/ω̂_0 − 39.5⌋` will be consistently off by half a
quantization level, with the error showing up on frames where `ω̂_0`
sits near a boundary. This kind of bug does not surface in
synthetic-input tests where `ω̂_0` happens to land at bin centers;
it surfaces only on real audio where the refinement puts `ω̂_0` near
bin edges. DVSI test vectors are the authoritative reference.

### 0.4.5 Range Clamping

The admissible range `ω̂_0 ∈ [2π/123.125, 2π/19.875]` (equivalently
`P ∈ [19.875, 123.125]`) is larger than `P̂_I`'s `[21, 122]` range —
the `±1.125` refinement can push `ω̂_0` up to ~19.875 samples or down
to ~123.125 samples. Within `P̂_I ∈ [21, 122]`, all ten refinement
candidates produce `ω_0` values in the admissible range, and `b̂_0`
falls in `[0, 207]` with no clamping needed.

If `P̂_I` is unreliable (cold start, near-silence), the BABA-A PDF
does not prescribe a fallback — the encoder should dispatch to
silence (§0.8) rather than emit an out-of-range `b̂_0`. Clamping
`b̂_0` into `[0, 207]` without a silence dispatch will produce a
bitstream that the decoder interprets as a valid pitch; this is the
worst failure mode. §0.8 will handle this.

### 0.4.6 Reference C Pseudocode

```c
#define N_CAND 10
static const double refine_offsets[N_CAND] = {
    -9.0/8.0, -7.0/8.0, -5.0/8.0, -3.0/8.0, -1.0/8.0,
    +1.0/8.0, +3.0/8.0, +5.0/8.0, +7.0/8.0, +9.0/8.0,
};

typedef struct {
    double omega_hat;       /* ω̂_0 */
    int    L_hat;           /* L̂, Eq. 31 */
    double a_hat[128];      /* â_l, Eq. 32, l = 0..L̂ */
    double b_hat[128];      /* b̂_l, Eq. 33, l = 0..L̂ */
    int    b0;              /* b̂_0, Eq. 45 */
} PitchRefineOut;

PitchRefineOut refine_pitch(double complex Sw[N_DFT], double P_hat_I) {
    double best_ER = INFINITY;
    double best_omega = 0.0;
    for (int c = 0; c < N_CAND; ++c) {
        double P = P_hat_I + refine_offsets[c];
        double omega0 = 2.0 * M_PI / P;

        /* Build S_w(m, ω_0) over m = 50 .. m_max per §0.2.3 + §0.2.6. */
        double complex Sw_synth[N_DFT];
        build_Sw_synth(Sw_synth, omega0);   /* §0.2.7 */

        int m_max = (int)floor((0.9254 * M_PI / omega0 - 0.5)
                                * 256.0 * omega0 / (2.0 * M_PI));
        double ER = 0.0;
        for (int m = 50; m <= m_max; ++m) {
            double complex d = Sw[m] - Sw_synth[m];
            ER += creal(d) * creal(d) + cimag(d) * cimag(d);
        }
        if (ER < best_ER) { best_ER = ER; best_omega = omega0; }
    }

    PitchRefineOut out;
    out.omega_hat = best_omega;
    out.L_hat     = (int)floor(0.9254 * (M_PI / best_omega + 0.25));
    double bin_scale = 256.0 / (2.0 * M_PI);
    for (int l = 0; l <= out.L_hat; ++l) {
        out.a_hat[l] = bin_scale * ((double)l - 0.5) * best_omega;
        out.b_hat[l] = bin_scale * ((double)l + 0.5) * best_omega;
    }
    out.b0 = (int)floor(4.0 * M_PI / best_omega - 39.0);   /* Eq. 45 */
    /* Range assertion (not a clamp; out-of-range indicates §0.8 dispatch). */
    assert(out.b0 >= 0 && out.b0 <= 207);
    return out;
}
```

### 0.4.7 Common Pitfalls

- **Ten candidates, not eight.** The gap report (line 178) misquoted
  this as "eight candidates"; the PDF says ten. The spacing is 0.25
  samples, the range is `±1.125`, and `P̂_I` itself is excluded.
- **`â_l`, `b̂_l` are NOT amplitudes.** They are bin endpoints. The
  complex spectral-amplitude decomposition implied by the gap report's
  paraphrase does not exist in the PDF. Use §0.5 (forthcoming) for
  the actual `M̂_l` estimator.
- **Eq. 45 is `−39`, not `−39.5`.** Mirroring the decoder is wrong.
  The encoder's floor combined with the decoder's `+39.5` offset is
  the round-trip.
- **`L̂` upper bound is 56.** A naive Eq. 31 evaluation at very low
  `ω̂_0` can produce `L̂ > 56` if `ω̂_0` falls below the admissible
  range. If §0.4.5's range check is omitted, spectral-amplitude
  arrays sized for 56 will overflow.
- **`S_w(m)` is computed once per frame.** Do not redo the 256-pt DFT
  inside the 10-candidate loop.

---

## §0.5 Spectral Amplitude Estimation

**Source:** TIA-102.BABA-A §5.3 "Estimation of the Spectral Amplitudes"
(pages 19–20, Equations 43 and 44, Figure 13).

**Consumes:** `S_w(m)` from §0.2.1 (Eq. 29); `W_R(m)` from §0.2.2
(Eq. 30); `ω̂_0`, `L̂`, `â_l`, `b̂_l` from §0.4 (Eq. 31, 32, 33); the
V/UV band count `K̂` and per-band bits `v̂_k` from §0.7 (Eq. 34,
Eq. 37 decision rule); the Annex C window `w_R(n)`.

**Produces:** `M̂_l` for `1 ≤ l ≤ L̂` — the per-harmonic spectral
amplitudes that feed the quantizer (wire-side, not covered here).

**DC amplitude.** Per §5.3 prose (page 20): "The D.C. spectral
amplitude, `M̂_0`, is ignored in the IMBE speech coder and can be
assumed to be zero." Downstream `MbeParams` does not carry `M̂_0`.

**Order of execution in the encoder pipeline.** V/UV determination
(§0.7) runs **before** spectral amplitude estimation in the
algorithmic flow of Figure 5 (page 9). Each harmonic `l` inherits
the V/UV bit of the band that contains it, and that bit selects
between Eq. 43 (voiced) and Eq. 44 (unvoiced). The dependency is:
§0.7 produces `v̂_k`; §0.5 reads `v̂_k` and picks a formula.

### 0.5.1 Per-Harmonic Band Assignment

The V/UV band structure of §0.7 groups harmonics as follows:

| Band index k | Harmonics covered | Bin range |
|:-----------:|-------------------|-----------|
| 1           | l = 1, 2, 3       | `⌈â_1⌉ ≤ m < ⌈b̂_3⌉` |
| 2           | l = 4, 5, 6       | `⌈â_4⌉ ≤ m < ⌈b̂_6⌉` |
| …           | …                 | … |
| K̂ − 1       | l = 3K̂−5, 3K̂−4, 3K̂−3 | `⌈â_{3K̂−5}⌉ ≤ m < ⌈b̂_{3K̂−3}⌉` |
| K̂ (highest) | l = 3K̂−2, …, L̂   | `⌈â_{3K̂−2}⌉ ≤ m < ⌈b̂_L̂⌉` |

Harmonic `l` maps to band `k = ⌈l / 3⌉` for `l ≤ 3(K̂−1)`, else to
band `K̂`. The band structure is inherited unchanged from §0.7 / Eq. 34;
§0.5 does not re-compute it.

### 0.5.2 Voiced Harmonic — Eq. 43

For `l` in a voiced band (`v̂_k = 1`):

```
         ⎡   Σ_{m=⌈â_l⌉}^{⌈b̂_l⌉−1}  |S_w(m)|²                                           ⎤^(1/2)
M̂_l  =  ⎢ ───────────────────────────────────────────────────────────────────────────── ⎥      (Eq. 43)
         ⎣   Σ_{m=⌈â_l⌉}^{⌈b̂_l⌉−1}  |W_R( ⌊64m − (16384/2π)·l·ω̂_0 + 0.5⌋ )|²              ⎦
```

This is the **magnitude analog** of Eq. 28's `A_l(ω_0)`: same bin
support (one harmonic's worth of 256-bin indices), same window-energy
denominator, but with `|S_w(m)|²` in the numerator instead of
`S_w(m) · W_R*(·)`. The result is a non-negative real amplitude — no
`Re[]`/`Im[]` decomposition, no complex conjugate.

**Important contrast with the gap report's paraphrase.** The gap
report (lines 252–260) implied the spectral amplitude came from
`M̂_l = √(â_l² + b̂_l²)` where `â_l`, `b̂_l` were complex components.
That is not what the PDF says. The PDF uses Eq. 43 directly; there is
no intermediate `â + jb̂` object. The phase information of `S_w(m)` is
discarded here (the `|·|²` squares it away).

### 0.5.3 Unvoiced Harmonic — Eq. 44

For `l` in an unvoiced band (`v̂_k = 0`):

```
                        1                    ⎡ Σ_{m=⌈â_l⌉}^{⌈b̂_l⌉−1}  |S_w(m)|² ⎤^(1/2)
M̂_l  =   ─────────────────────────────  ·   ⎢ ─────────────────────────────────── ⎥          (Eq. 44)
          Σ_{n=−110}^{110} w_R(n)             ⎣           ⌈b̂_l⌉ − ⌈â_l⌉              ⎦
```

Semantic difference from Eq. 43:

- The denominator inside the radical is `⌈b̂_l⌉ − ⌈â_l⌉` (the **count**
  of bins in the harmonic's support), not the window-energy sum.
- The external factor is `1 / Σ w_R(n)` (the reciprocal of the
  **sum** of the window, not the sum of squares). This is a scalar
  constant — precompute once at startup from the Annex C CSV.

Interpretation: unvoiced bands are assumed spectrally flat within
each harmonic's slot, so the amplitude is the RMS of `|S_w(m)|` over
the band's bins, rescaled by the window's DC response. No per-bin
window-energy weighting is applied — noise-like content is integrated
uniformly.

### 0.5.4 Highest-Band Modification

Per §5.3 prose (page 20): "This procedure shall be modified slightly
for the highest frequency band which covers the frequency interval
`â_{3K̂−2} ≤ ω < b̂_L̂`. The spectral envelope in this frequency band
is represented by `L̂ − 3K̂ + 3` spectral amplitudes, denoted `M̂_{3K̂−2},
M̂_{3K̂−1}, …, M̂_L̂`. If this frequency band is declared voiced then
these spectral amplitudes are estimated using equation (43) for
`3K̂−2 ≤ l ≤ L̂`. Alternatively, if this frequency band is declared
unvoiced then these spectral amplitudes are estimated using equation
(44) for `3K̂−2 ≤ l ≤ L̂`."

The "slight modification" is entirely about the band width: instead
of exactly 3 harmonics, the highest band carries
`L̂ − 3(K̂−1) = L̂ − 3K̂ + 3` harmonics. The per-harmonic formula
(Eq. 43 or Eq. 44) is unchanged.

**Count check.**

- For `L̂ ≤ 36`: `K̂ = ⌊(L̂+2)/3⌋` (Eq. 34). The highest band carries
  `L̂ − 3K̂ + 3 ∈ {1, 2, 3}` harmonics (varies with `L̂ mod 3`).
- For `L̂ > 36`: `K̂ = 12`. The highest band carries `L̂ − 33` harmonics,
  ranging from 4 (at `L̂ = 37`) to 23 (at `L̂ = 56`).

The wide highest band at high `L̂` is a deliberate choice: the top
harmonics carry less perceptual weight and are bundled into a single
V/UV decision.

### 0.5.5 Reference C Pseudocode

```c
/* Precomputed at startup. */
static double wR_sum;      /* Σ_{n=−110}^{110} w_R(n), scalar */
static double WR[NW_DFT];  /* W_R(m), real, from §0.2 */

/* Called after §0.4 (pitch refinement) and §0.7 (V/UV) complete. */
/* vuv[k] ∈ {0, 1} for k = 1..K_hat (1-indexed per PDF).            */
/* out_M_hat[l] receives M̂_l for l = 1..L_hat.                     */
void estimate_amplitudes(double complex Sw[N_DFT],
                         PitchRefineOut *pitch,
                         int K_hat, int vuv[],
                         double out_M_hat[]) {
    const double bin_scale_16384 = 16384.0 / (2.0 * M_PI);
    int L_hat = pitch->L_hat;
    double omega_hat = pitch->omega_hat;

    for (int l = 1; l <= L_hat; ++l) {
        int k = (l + 2) / 3;           /* ⌈l/3⌉, PDF-style 1-based */
        if (k > K_hat) k = K_hat;      /* highest-band fallthrough  */

        int m_lo = (int)ceil(pitch->a_hat[l]);
        int m_hi = (int)ceil(pitch->b_hat[l]);

        double num_energy = 0.0;
        for (int m = m_lo; m < m_hi; ++m) {
            double re = creal(Sw[dft_index_256(m)]);
            double im = cimag(Sw[dft_index_256(m)]);
            num_energy += re*re + im*im;
        }

        double M_l;
        if (vuv[k] == 1) {
            /* Eq. 43 — voiced */
            double den_energy = 0.0;
            for (int m = m_lo; m < m_hi; ++m) {
                double k_real = 64.0 * (double)m
                              - bin_scale_16384 * (double)l * omega_hat
                              + 0.5;
                int kbin = (int)floor(k_real);
                double wr = WR[wr_index(kbin)];
                den_energy += wr * wr;
            }
            M_l = (den_energy > 0.0)
                  ? sqrt(num_energy / den_energy)
                  : 0.0;
        } else {
            /* Eq. 44 — unvoiced */
            int bins = m_hi - m_lo;
            M_l = (bins > 0 && wR_sum != 0.0)
                  ? (1.0 / wR_sum) * sqrt(num_energy / (double)bins)
                  : 0.0;
        }
        out_M_hat[l] = M_l;
    }
    /* M̂_0 is ignored (§5.3 prose) — not written. */
}
```

### 0.5.6 Common Pitfalls

- **Eq. 43 vs Eq. 44 is a formula switch, not a factor switch.** The
  two equations have structurally different denominators. Do not try
  to unify them by parameterizing a single formula.
- **`Σ w_R(n)` vs `Σ w_R²(n)`.** Eq. 44's external factor uses the
  plain sum (linear), not the sum of squares. The Annex C CSV values
  sum to a specific constant (≈ 88.6 in typical extractions); don't
  substitute the squared-sum from Eq. 6 (which is for `w_I`, not `w_R`).
- **No complex `â + jb̂`.** The gap report's paraphrase is misleading
  on this; see the correction in §0.4.3. Phase is discarded inside
  Eq. 43–44.
- **Band assignment `k = ⌈l/3⌉`, capped at K̂.** Harmonics `l > 3(K̂−1)`
  all fall in the highest band, which for high `L̂` is a wide bundle.
  Off-by-one at the band boundary (e.g. using `k = ⌊l/3⌋ + 1`)
  silently mis-routes harmonic 3 into band 2.
- **Near-zero denominators.** For very narrow harmonic slots (small
  `ω̂_0`, `l = 0` or 1), `den_energy` in Eq. 43 or `bins` in Eq. 44
  can be degenerate. The canonical handling is to emit `M̂_l = 0`;
  the decoder's inverse log-magnitude prediction (§1.8.5 of the
  implementation spec) tolerates zero amplitudes via its floor.
- **`γ_w` cross-reference (again).** Like Eq. 28, the denominators
  here carry window-energy factors. Commit `741eeef`'s decoder-side
  DVSI calibration mismatch may surface equivalently on the encoder
  side. Do not pre-apply a correction; note the shared origin.

---

## §0.6 Log-Magnitude Prediction Residual

**Source:** TIA-102.BABA-A §6.3 "Spectral Amplitudes Encoding"
(pages 24–27, Equations 52–57, Figure 16). The decoder mirror is
§6.4 (Eq. 75–79), rendered in the implementation spec at §1.8.5.

**Consumes:** `M̂_l(0)` for `1 ≤ l ≤ L̂(0)` from §0.5; `L̂(0)` from
§0.4; the matched-decoder state `M̃_l(−1)` for `1 ≤ l ≤ L̃(−1)` and
`L̃(−1)` from the prior frame (§0.6.6).

**Produces:** `T̂_l` for `1 ≤ l ≤ L̂(0)` — the log₂-domain prediction
residuals. These feed the quantizer chain (6-block partition → per-
block DCT → gain vector `R̂` + higher-order DCT coefficients `Ĉ_{i,k}`
→ Annex F/G quantization), which is wire-side and already covered in
the existing implementation spec at §1.8.

### 0.6.1 Algorithmic Flow (Figure 16)

```
        M̂_l(0)  ──► log₂  ──► +  ──► T̂_l  ──► [6-block DCT → quantize] ──►  b̂_2..b̂_{L̂+1}
                               ─
                               │
                     ┌─────────┴──────────┐
                     │  predictor         │
                     │  (Eq. 52–54)       │
                     └─────────▲──────────┘
                               │
             M̃_l(−1) ◄── [1-frame delay] ◄── [matched-decoder feedback]
```

The predictor takes the previous frame's reconstructed spectral
amplitudes `M̃_l(−1)`, interpolates them onto the current frame's
harmonic index grid `l = 1..L̂(0)`, scales by `ρ`, and subtracts the
(mean-removed) predicted log-amplitude from the current log-amplitude.
The residual `T̂_l` is what actually gets transmitted.

### 0.6.2 Prediction Coefficient `ρ` — Eq. 55

```
ρ  =  ⎧ 0.4                           if  L̂(0) ≤ 15
      ⎨ 0.03 · L̂(0)  −  0.05          if  15 < L̂(0) ≤ 24                (Eq. 55)
      ⎩ 0.7                           otherwise
```

Table of values across the full admissible `L̂(0)` range:

| `L̂(0)` | `ρ`   | `L̂(0)` | `ρ`   | `L̂(0)` | `ρ`   |
|:------:|:-----:|:------:|:-----:|:------:|:-----:|
| 9      | 0.40  | 17     | 0.46  | 24     | 0.67  |
| 10     | 0.40  | 18     | 0.49  | 25     | 0.70  |
| 15     | 0.40  | 19     | 0.52  | 30     | 0.70  |
| 16     | 0.43  | 20     | 0.55  | 56     | 0.70  |

> **⚠ Correction to `vocoder_decode_disambiguations.md` §3 — full-rate
> vs half-rate `ρ`.** That note asserts "ρ = 0.65 ... embedded as a
> literal 0.65 in multiple equations" and cites full-rate Eq. 77 plus
> half-rate Eq. 185 (originally mis-cited as Eq. 200; that is the
> first frame-repeat copy-forward equation, not the predictor — fixed
> in the disambiguation note 2026-04-16) as using the same value.
> This conflates the two rates:
>
> - **Full-rate Eq. 77** uses the symbol `ρ` (not a literal). `ρ` is
>   defined by Eq. 55 on page 25 as a piecewise-linear function of
>   `L̂(0)`. The "inline 0.65" reading of Eq. 77 is incorrect — the
>   0.65 does not appear there.
> - **Half-rate Eq. 155** (encoder page 60) and **Eq. 185** (decoder
>   page 65) do use the literal constant 0.65 directly, with no `ρ`
>   symbol and no Eq. 55 equivalent. Half-rate is what the
>   disambiguation note actually got right.
>
> So: for full-rate implementations, `ρ` varies with `L̂(0)` per
> Eq. 55 above (0.40–0.70 range). For half-rate, the fixed 0.65 is
> correct. The disambiguation note, and any full-rate code that hard-
> codes 0.65 via that note, needs a correction. Validate against DVSI
> full-rate test vectors. Half-rate needs no change.

**Interpretation of the schedule.** Low `L̂` means high pitch (short
period, few harmonics), where spectral envelopes vary fast between
frames — a weak predictor (`ρ = 0.4`) avoids over-predicting into
transients. High `L̂` means low pitch (long period, many harmonics),
where envelopes are more stationary and a strong predictor
(`ρ = 0.7`) yields small residuals and better quantizer efficiency.
The 15 < L̂ ≤ 24 range is the linear ramp between the two regimes.

### 0.6.3 Past-Frame Harmonic Mapping — Eq. 52, 53

When `L̂(0) ≠ L̃(−1)`, the previous frame's amplitudes are indexed on
a different harmonic grid than the current frame. The PDF handles
this via linear interpolation:

```
k̂_l  =  ( L̃(−1) / L̂(0) )  ·  l                                 (Eq. 52)
δ̂_l  =  k̂_l  −  ⌊k̂_l⌋                                           (Eq. 53)
```

- `k̂_l ∈ [0, L̃(−1)]` is the real-valued past-frame index
  corresponding to current-frame harmonic `l`.
- `⌊k̂_l⌋` and `⌊k̂_l⌋ + 1` are the two integer neighbors used for
  linear interpolation; `δ̂_l ∈ [0, 1)` is the fractional weight on
  the upper neighbor.

**Boundary case.** When `l = L̂(0)`, `k̂_l = L̃(−1)` exactly, so
`δ̂_l = 0` and the interpolation degenerates to
`M̃_{L̃(−1)}(−1)` alone. When `L̂(0) > L̃(−1)` (current frame has
**more** harmonics than previous), `k̂_l` can exceed `L̃(−1)` for
some `l`; those out-of-range indices are handled by Eq. 57.

### 0.6.4 Past-Frame Boundary Conditions — Eq. 56, 57

Two boundary assumptions extend `M̃_l(−1)` outside its native range
so Eq. 54's lookups are well-defined for all `l`:

```
M̃_0(−1)  =  1.0                                                (Eq. 56)
M̃_l(−1)  =  M̃_{L̃(−1)}(−1)     for  l > L̃(−1)                 (Eq. 57)
```

- **Eq. 56:** the DC (zero-frequency) amplitude of the past frame is
  fixed at `1.0`. In the log₂ domain this is `0.0`, which makes the
  predictor's lookup at `k̂_l = 0` land on a neutral value (no
  predictive contribution at the DC boundary).
- **Eq. 57:** indices beyond the past frame's last harmonic extend by
  **constant** extrapolation using the final amplitude. This is not
  zero-padding — a zero-pad would force `log₂ 0 → −∞` and destabilize
  the predictor.

### 0.6.5 The Prediction Residual `T̂_l` — Eq. 54

```
T̂_l  =  log₂ M̂_l(0)
      −  ρ · (1 − δ̂_l) · log₂ M̃_{⌊k̂_l⌋}(−1)
      −  ρ · δ̂_l       · log₂ M̃_{⌊k̂_l⌋ + 1}(−1)                           (Eq. 54)
             L̂(0)
      +  (ρ / L̂(0))  ·   Σ   [ (1 − δ̂_λ) · log₂ M̃_{⌊k̂_λ⌋}(−1)
                             λ=1             +  δ̂_λ       · log₂ M̃_{⌊k̂_λ⌋ + 1}(−1) ]
```

Let `P_l = (1 − δ̂_l) · log₂ M̃_{⌊k̂_l⌋}(−1) + δ̂_l · log₂ M̃_{⌊k̂_l⌋+1}(−1)`
denote the linearly-interpolated past-frame log-amplitude at
harmonic `l`. Then Eq. 54 collapses to:

```
T̂_l  =  log₂ M̂_l(0)  −  ρ · P_l  +  (ρ / L̂(0)) · Σ_λ P_λ
       =  log₂ M̂_l(0)  −  ρ · ( P_l  −  mean_l P_l )
```

Reading: the predictor subtracts `ρ` times the **mean-removed**
interpolated past-frame log-amplitude. The mean component of `P` is
added back, not subtracted, because the DC (mean) of the
log-amplitude spectrum is carried by the gain vector `G_1` (via the
6-block DCT's DC coefficient), not by `T̂`. Letting the predictor
act on the mean would cause the gain vector to drift between frames;
the mean-removal makes `T̂` DC-free in the predictor sense.

Equivalently, the residual preserves the current frame's mean:
`mean_l T̂_l = mean_l log₂ M̂_l(0)` (the predictor terms cancel to
zero in the mean). The gain vector `G_1` thus carries the absolute
log-envelope level; `T̂` carries only the differential shape
prediction.

### 0.6.6 Closed-Loop Encoder: the Matched-Decoder Feedback

Per §6.3 prose (page 27, verbatim):

> "the encoder needs to simulate the operation of the decoder and
> use the reconstructed spectral amplitudes from the previous frame
> to predict the spectral amplitudes of the current frame. The IMBE
> spectral amplitude encoder simulates the spectral amplitude decoder
> by setting `L̃ = L̂` and then reconstructing the spectral amplitudes
> as discussed above. This is shown as the feedback path in Figure 16."

**This resolves the gap report's §4 question definitively:** yes, the
encoder runs a matched decoder internally. The predictor loop is
closed, not open. Quantization noise is in the predictor path.

**Implementation.** After computing `T̂_l` and running the block
partition + DCT + gain/HOC quantization chain to produce the bit
stream `b̂_2..b̂_{L̂+1}`:

1. Set `L̃ = L̂(0)`.
2. Run the decoder pipeline of implementation-spec §1.8.1–1.8.5
   (inverse quantizer → per-block inverse DCT → concatenate to
   `T̃_l` → inverse log-magnitude prediction) on the just-produced
   `b̂_m` bits, yielding `M̃_l(0)` for `1 ≤ l ≤ L̃`.
3. Store `M̃_l(0)` as the next frame's `M̃_l(−1)`; store `L̃ = L̂(0)`
   as the next frame's `L̃(−1)`.

This matched-decoder reconstruction uses the **same** Eq. 75–79
decoder math that the receiver will use, so the encoder's
`M̃_l(−1)` exactly matches what a compliant receiver reconstructs.
Any encoder that uses `M̂_l(0)` directly (open-loop, skipping the
matched decoder) will drift away from the receiver's prediction
within a few frames.

### 0.6.7 Cross-Frame State and Initialization

State carried into §0.6:

| Variable | Shape | Update |
|----------|-------|--------|
| `M̃_l(−1)` | real vector, length `L̃(−1)` | closed-loop decoder output (§0.6.6) |
| `L̃(−1)`  | int | set to `L̂(0)` at end of each frame |

**Cold start** per §6.3 prose (page 25, verbatim):

> "Also upon initialization `M̃_l(−1)` should be set equal to 1.0 for
> all `l`, and `L̃(−1) = 30`."

In log₂ units, `log₂ 1.0 = 0`, so the cold-start predictor
contribution is zero — the first frame's `T̂_l` is exactly
`log₂ M̂_l(0)`. `L̃(−1) = 30` is a mid-range harmonic count; it keeps
`k̂_l` bounded regardless of the first frame's `L̂(0)`.

### 0.6.8 Reference C Pseudocode

```c
typedef struct {
    double M_tilde_prev[MAX_L + 1];   /* M̃_l(−1), 1-indexed; [0] = 1.0 (Eq. 56) */
    int    L_tilde_prev;              /* L̃(−1) */
} PredictorState;

void predictor_state_init(PredictorState *st) {
    for (int l = 0; l <= MAX_L; ++l) st->M_tilde_prev[l] = 1.0;
    st->L_tilde_prev = 30;
}

/* Eq. 55: ρ as a function of L̂(0). */
static double rho_of_L(int L_hat) {
    if (L_hat <= 15) return 0.4;
    if (L_hat <= 24) return 0.03 * (double)L_hat - 0.05;
    return 0.7;
}

/* Eq. 52–54: compute T̂_l for l = 1..L̂(0). */
void compute_prediction_residual(const double M_hat[],   /* M̂_l(0), 1-indexed */
                                 int L_hat,
                                 const PredictorState *st,
                                 double T_hat[]) {
    int L_prev = st->L_tilde_prev;
    double ratio = (double)L_prev / (double)L_hat;
    double rho   = rho_of_L(L_hat);

    /* Precompute P_λ = interpolated past log-amplitude at λ, for all λ. */
    double P[MAX_L + 1];
    double P_mean = 0.0;
    for (int l = 1; l <= L_hat; ++l) {
        double k_hat = ratio * (double)l;
        int    kf    = (int)floor(k_hat);
        double delta = k_hat - (double)kf;

        double lo = (kf >= 0 && kf <= L_prev)   ? st->M_tilde_prev[kf]
                                                : st->M_tilde_prev[L_prev];   /* Eq. 57 */
        double hi = (kf + 1 <= L_prev)          ? st->M_tilde_prev[kf + 1]
                                                : st->M_tilde_prev[L_prev];

        P[l]    = (1.0 - delta) * log2(lo) + delta * log2(hi);
        P_mean += P[l];
    }
    P_mean /= (double)L_hat;

    /* Eq. 54: T̂_l = log₂ M̂_l − ρ·(P_l − mean(P)). */
    for (int l = 1; l <= L_hat; ++l) {
        T_hat[l] = log2(M_hat[l]) - rho * (P[l] - P_mean);
    }
}

/* Called after the quantizer chain produces b̂_m and the matched-decoder   */
/* reconstructs M̃_l(0) per implementation-spec §1.8.1–1.8.5 with L̃ = L̂.  */
void predictor_state_commit(PredictorState *st,
                            const double M_tilde_curr[],
                            int L_tilde_curr) {
    for (int l = 1; l <= L_tilde_curr; ++l) {
        st->M_tilde_prev[l] = M_tilde_curr[l];
    }
    st->M_tilde_prev[0] = 1.0;   /* Eq. 56 */
    st->L_tilde_prev    = L_tilde_curr;
}
```

The matched-decoder reconstruction itself is not shown — it is the
existing decoder pipeline from implementation-spec §1.8, invoked with
`L̃ = L̂` on the just-produced bits.

### 0.6.9 Common Pitfalls

- **`ρ` is not 0.65.** It is a piecewise-linear function of `L̂(0)`
  per Eq. 55. The widely-circulated "0.65" is a midrange
  approximation that is materially wrong at the tails
  (0.4 at low `L̂`, 0.7 at high `L̂`). See the correction note at the
  top of this file.
- **Open-loop predictor is wrong.** The encoder must run the matched
  decoder and feed back `M̃_l(0)`, not `M̂_l(0)`. Using the unquantized
  amplitudes in the predictor loop produces a bitstream that a
  compliant receiver will not track — the prediction residuals
  presented to the receiver were computed against a different past
  state than the receiver has.
- **Eq. 57 is constant extrapolation, not zero-padding.** Indices
  beyond `L̃(−1)` get `M̃_{L̃(−1)}(−1)`, not 0. A zero-pad forces
  `log₂ 0 = −∞` and destabilizes the predictor on frames where
  `L̂(0)` rises sharply.
- **Cold start `L̃(−1) = 30`, not 0.** At `L̃(−1) = 0` the ratio
  `L̃(−1)/L̂(0)` in Eq. 52 is zero, collapsing all `k̂_l` to 0 and
  making the predictor a constant. The PDF-prescribed 30 keeps the
  first-frame predictor well-distributed.
- **`M̃_0 = 1.0` not 0.0.** `log₂ 1.0 = 0`; `log₂ 0 = −∞`. The DC
  boundary exists specifically to prevent the latter.
- **`δ̂_l` range is `[0, 1)`.** Standard fractional-part convention
  (result of `x − ⌊x⌋`). Implementations that use symmetric
  `[−0.5, 0.5]` fractional parts will offset the interpolation
  weights by half a bin.
- **The fourth term's sign.** On the encoder side (Eq. 54) the mean
  term is added back with `+`; on the decoder side (Eq. 77) it is
  subtracted with `−`. The encoder subtracts the per-harmonic
  prediction and adds back the mean; the decoder adds the
  per-harmonic prediction and subtracts the mean. Mirror-image, not
  identical.

---

## §0.7 Voiced/Unvoiced Determination

**Source:** TIA-102.BABA-A §5.2 "Voiced / Unvoiced Determination"
(pages 17–19, Equations 34–42, Figure 11).

**Consumes:** `S_w(m)` from §0.2.1 (Eq. 29); `S_w(m, ω̂_0)` from
§0.2.3 built at the refined fundamental; `W_R(0)` from §0.2.2 (the DC
value of the window spectrum); `ω̂_0`, `L̂`, `â_l`, `b̂_l` from §0.4;
`E(P̂_I)` from §0.3 (the Eq. 5 error function value at the chosen
pitch); cross-frame state `ξ_max(−1)` and `v̂_k(−1)`.

**Produces:** `v̂_k ∈ {0, 1}` for `1 ≤ k ≤ K̂` — binary voicing
decisions per band (1 = voiced, 0 = unvoiced).

### 0.7.1 Band Count `K̂` — Eq. 34

```
K̂ = ⎧ ⌊ (L̂ + 2) / 3 ⌋    if L̂ ≤ 36                         (Eq. 34)
     ⎩ 12                  otherwise
```

| `L̂`    | `K̂` | Highest band width (harmonics) |
|:------:|:---:|:-----:|
| 9      | 3   | 3     |
| 10     | 4   | 1     |
| 11     | 4   | 2     |
| 12     | 4   | 3     |
| 36     | 12  | 3     |
| 37     | 12  | 4     |
| 56     | 12  | 23    |

With `L̂ ∈ [9, 56]` (from §0.4), `K̂ ∈ [3, 12]`. The cap at 12 reflects
the quantizer's fixed V/UV codebook width: only 12 V/UV bits are ever
transmitted per full-rate frame.

### 0.7.2 Per-Band Voicing Measure `D_k` — Eq. 35, 36

For `1 ≤ k ≤ K̂ − 1` (the "standard" bands, 3 harmonics each):

```
           Σ_{m=⌈â_{3k−2}⌉}^{⌈b̂_{3k}⌉−1}  | S_w(m) − S_w(m, ω̂_0) |²
D_k  =   ─────────────────────────────────────────────────────────         (Eq. 35)
           Σ_{m=⌈â_{3k−2}⌉}^{⌈b̂_{3k}⌉−1}  | S_w(m) |²
```

For `k = K̂` (highest band, `L̂ − 3K̂ + 3` harmonics):

```
           Σ_{m=⌈â_{3K̂−2}⌉}^{⌈b̂_L̂⌉−1}  | S_w(m) − S_w(m, ω̂_0) |²
D_K̂  =   ───────────────────────────────────────────────────────          (Eq. 36)
           Σ_{m=⌈â_{3K̂−2}⌉}^{⌈b̂_L̂⌉−1}  | S_w(m) |²
```

`D_k` is the normalized band-energy residual: numerator is the
squared error between the actual windowed spectrum and the synthetic
(all-voiced) spectrum within band `k`; denominator is the actual
spectrum's band energy. A small `D_k` means the synthetic voiced
model matches the band well — vote voiced. A large `D_k` means the
match is poor — vote unvoiced.

### 0.7.3 Energy Parameters `ξ_LF`, `ξ_HF`, `ξ_0` — Eq. 38, 39, 40

Three scalar energy statistics are computed over the two-sided DFT's
non-negative indices:

```
           63   | S_w(m) |²
ξ_LF  =   Σ   ─────────────                                      (Eq. 38)
          m=0   | W_R(0) |²

           128  | S_w(m) |²
ξ_HF  =   Σ   ─────────────                                      (Eq. 39)
          m=64  | W_R(0) |²

ξ_0  =  ξ_LF  +  ξ_HF                                            (Eq. 40)
```

- `|W_R(0)|²` is a scalar constant (the window's DC response,
  precomputed at startup). It normalizes energy to a window-
  independent scale.
- The band split is at 256-bin index `m = 64`. At 8 kHz sampling
  with 256-point DFT, bin `m` corresponds to `m · 8000/256 = 31.25·m` Hz,
  so `ξ_LF` covers 0–1969 Hz and `ξ_HF` covers 2000–4000 Hz.
- The per-bin sum is over non-negative `m` only (0..128). The
  conjugate-symmetric negative half contributes identical magnitudes;
  not doubling here is deliberate — Eq. 38–39 don't include a factor
  of 2 for the two-sided sum.

### 0.7.4 `ξ_max` Peak-Envelope Tracking — Eq. 41

```
ξ_max(0)  =  ⎧ 0.5 · ξ_max(−1)  +  0.5 · ξ_0         if  ξ_0 > ξ_max(−1)
             ⎨ 0.99 · ξ_max(−1) + 0.01 · ξ_0         else if  0.99·ξ_max(−1)+0.01·ξ_0 > 20000
             ⎩ 20000                                  otherwise               (Eq. 41)
```

- **Case 1 (attack).** When the current frame's `ξ_0` exceeds the
  tracked maximum, move halfway toward it (50/50 blend). This is a
  fast rise.
- **Case 2 (slow decay).** Otherwise, apply a 0.99/0.01 leaky
  integrator — the tracked maximum decays slowly toward `ξ_0`.
- **Case 3 (floor).** Clamp at 20000 so `ξ_max` never drops below a
  minimum usable value. The floor is in the same units as `ξ_0`
  (normalized by `|W_R(0)|²`).

**State.** `ξ_max(−1)` is a single scalar per encoder; it persists
across all frames. Cold-start init: `ξ_max = 20000` (the floor
value). The PDF does not specify an init value; starting at the
floor is consistent with Case 3's behavior and avoids a burn-in
where `ξ_max = 0` would divide by zero in Eq. 42.

### 0.7.5 `M(ξ)` Energy Mapping — Eq. 42

```
          ⎧  ( 0.0025·ξ_max + ξ_0 ) / ( 0.01·ξ_max + ξ_0 )                              if ξ_LF ≥ 5·ξ_HF
M(ξ)  =   ⎨                                                                                             (Eq. 42)
          ⎩  ( 0.0025·ξ_max + ξ_0 ) / ( 0.01·ξ_max + ξ_0 )  ·  √( ξ_LF / (5·ξ_HF) )     otherwise
```

- First branch: low-frequency-dominant input (typical voiced speech).
  `M(ξ)` ≈ 0.25 when `ξ_0 ≪ ξ_max` (quiet frame relative to running
  peak), trending toward 1 as `ξ_0` grows large.
- Second branch: high-frequency-dominant input (fricatives, whispers,
  noise). The extra `√(ξ_LF / (5·ξ_HF))` factor (≤ 1 by construction
  in this branch) further suppresses `M(ξ)`, which in turn lowers the
  threshold `Θ_ξ` and biases the band decisions toward unvoiced.

### 0.7.6 Threshold Function `Θ_ξ(k, ω̂_0)` — Eq. 37

```
             ⎧ 0                                                    if E(P̂_I) > 0.5  AND  k ≥ 2
Θ_ξ(k,ω̂_0) = ⎨ 0.5625 · [ 1 − 0.3096·(k−1)·ω̂_0 ] · M(ξ)              else if v̂_k(−1) = 1               (Eq. 37)
             ⎩ 0.45   · [ 1 − 0.3096·(k−1)·ω̂_0 ] · M(ξ)              otherwise
```

- **Case 1 (poor pitch match).** When the error function at the
  chosen pitch is large (`E(P̂_I) > 0.5`) and the band is not the
  lowest, the threshold drops to 0 — any `D_k ≥ 0` fails the
  `D_k < Θ_ξ` test, forcing unvoiced. The low band (`k = 1`) is
  exempted because it often contains enough pitch information even
  in poor-match frames to sustain a voiced decision.
- **Case 2 (hysteresis up).** If the band was voiced last frame, use
  the higher threshold (0.5625) — biases toward staying voiced.
- **Case 3 (hysteresis down).** Otherwise, use the lower threshold
  (0.45) — harder to become voiced from unvoiced.
- **Pitch-and-band modulation `[1 − 0.3096·(k−1)·ω̂_0]`.** At `k = 1`
  this factor is 1.0. At higher `k` it decreases linearly, and the
  decrease is larger for higher pitch (larger `ω̂_0`). The threshold
  becomes harder to clear at high frequencies, reflecting the
  empirical observation that upper bands are less often genuinely
  voiced. The factor is positive for all admissible (`k`, `ω̂_0`)
  combinations: the maximum is at `k = 12, ω̂_0 = 2π/19.875`, giving
  `1 − 0.3096·11·0.316 ≈ −0.076`. This goes slightly negative; the
  PDF does not prescribe clipping, but negative `Θ_ξ` means
  `D_k < Θ_ξ` is impossible (`D_k` is non-negative), forcing
  unvoiced — equivalent to Case 1's behavior. Implementers may
  clip `Θ_ξ` at 0 for clarity without changing the decision.

### 0.7.7 V/UV Decision Rule

Per §5.2 prose (page 19): "If `D_k` is less than the threshold
function then the frequency band `â_{3k−2} ≤ ω < b̂_{3k}` is declared
voiced; otherwise this frequency band is declared unvoiced."

```
v̂_k  =  ⎧ 1    if  D_k  <  Θ_ξ(k, ω̂_0)
         ⎩ 0    otherwise
```

The inequality is strict (`<`), per "less than the threshold". Ties
(`D_k = Θ_ξ`) fall to unvoiced. In practice the set has measure zero
for floating-point inputs.

### 0.7.8 Cross-Frame State and Initialization

Encoder state carried into §0.7:

| Variable | Shape | Purpose |
|----------|-------|---------|
| `ξ_max(−1)` | scalar | Eq. 41 input (previous frame's tracked maximum) |
| `v̂_k(−1)` | vector of length `K̂(−1)` | Eq. 37 hysteresis (previous frame's decisions) |

Cold start (the PDF is silent on analysis-side V/UV init; these are
inferences consistent with the equations' well-definedness):

```
ξ_max(−1) = 20000       /* matches Case 3 floor of Eq. 41 */
v̂_k(−1)   = 0 for all k /* biases to the lower threshold in Eq. 37  */
```

**`K̂(−1)` vs `K̂(0)` mismatch.** When `L̂(0) > L̂(−1)`, `K̂(0)` may
exceed the length of the stored `v̂_k(−1)` vector. The new indices
have no history; treat them as `v̂_k(−1) = 0` (the cold-start value).
Symmetrically, when `K̂(0) < K̂(−1)`, the extra past entries are
simply discarded. The encoder should store `v̂_k(−1)` in a
fixed-width buffer of length 12 (the cap from Eq. 34) to avoid
reallocation.

### 0.7.9 Reference C Pseudocode

```c
typedef struct {
    double xi_max;         /* Eq. 41 state, scalar */
    int    vuv_prev[13];   /* v̂_k(−1), 1-indexed [1..12]; [0] unused */
    int    K_prev;         /* K̂(−1) */
} VuvState;

/* Cold-start init. */
void vuv_state_init(VuvState *s) {
    s->xi_max = 20000.0;
    for (int k = 0; k < 13; ++k) s->vuv_prev[k] = 0;
    s->K_prev = 0;
}

/* Called after §0.4 (pitch refinement) produces ω̂_0, L̂, â_l, b̂_l,         */
/* and §0.3 produces E(P̂_I). vuv_out[k] for k = 1..K_hat is written.       */
void determine_vuv(double complex Sw[N_DFT],
                   double complex Sw_synth[N_DFT],  /* S_w(m, ω̂_0) from §0.2 */
                   PitchRefineOut *pitch,
                   double E_Phat_I,
                   double WR_DC_sq,                 /* |W_R(0)|² */
                   VuvState *st,
                   int *out_K_hat, int vuv_out[]) {
    int L_hat = pitch->L_hat;
    double omega_hat = pitch->omega_hat;

    /* Eq. 34 */
    int K_hat = (L_hat <= 36) ? ((L_hat + 2) / 3) : 12;

    /* Eq. 38–40 */
    double xi_LF = 0.0, xi_HF = 0.0;
    for (int m = 0;  m <= 63;  ++m) {
        double re = creal(Sw[m]); double im = cimag(Sw[m]);
        xi_LF += (re*re + im*im) / WR_DC_sq;
    }
    for (int m = 64; m <= 128; ++m) {
        double re = creal(Sw[m]); double im = cimag(Sw[m]);
        xi_HF += (re*re + im*im) / WR_DC_sq;
    }
    double xi_0 = xi_LF + xi_HF;

    /* Eq. 41 */
    double xi_max_new;
    if (xi_0 > st->xi_max) {
        xi_max_new = 0.5 * st->xi_max + 0.5 * xi_0;
    } else {
        double decay = 0.99 * st->xi_max + 0.01 * xi_0;
        xi_max_new = (decay > 20000.0) ? decay : 20000.0;
    }

    /* Eq. 42 */
    double ratio = (0.0025 * xi_max_new + xi_0)
                 / (0.01   * xi_max_new + xi_0);
    double M_xi  = (xi_LF >= 5.0 * xi_HF)
                 ? ratio
                 : ratio * sqrt(xi_LF / (5.0 * xi_HF));

    /* Eq. 35/36 and Eq. 37/decision rule, per band. */
    for (int k = 1; k <= K_hat; ++k) {
        int l_lo = 3*k - 2;
        int l_hi = (k < K_hat) ? (3*k) : L_hat;   /* highest band extends to L̂ */

        int m_lo = (int)ceil(pitch->a_hat[l_lo]);
        int m_hi = (int)ceil(pitch->b_hat[l_hi]);

        double num = 0.0, den = 0.0;
        for (int m = m_lo; m < m_hi; ++m) {
            double dre = creal(Sw[dft_index_256(m)]) - creal(Sw_synth[m]);
            double dim = cimag(Sw[dft_index_256(m)]) - cimag(Sw_synth[m]);
            double sre = creal(Sw[dft_index_256(m)]);
            double sim = cimag(Sw[dft_index_256(m)]);
            num += dre*dre + dim*dim;
            den += sre*sre + sim*sim;
        }
        double D_k = (den > 0.0) ? (num / den) : 1.0;

        /* Eq. 37 */
        double theta;
        if (E_Phat_I > 0.5 && k >= 2) {
            theta = 0.0;
        } else {
            int v_prev = (k < 13) ? st->vuv_prev[k] : 0;
            double base = (v_prev == 1) ? 0.5625 : 0.45;
            theta = base * (1.0 - 0.3096 * (double)(k - 1) * omega_hat) * M_xi;
            if (theta < 0.0) theta = 0.0;   /* clarity, not required */
        }

        vuv_out[k] = (D_k < theta) ? 1 : 0;
    }

    /* Commit state for next frame. */
    st->xi_max = xi_max_new;
    for (int k = 1; k <= K_hat; ++k) st->vuv_prev[k] = vuv_out[k];
    for (int k = K_hat + 1; k < 13; ++k) st->vuv_prev[k] = 0;
    st->K_prev = K_hat;

    *out_K_hat = K_hat;
}
```

### 0.7.10 Common Pitfalls

- **`E(P̂_I)` is the scalar, not a cumulative error.** Eq. 37's first
  case tests the Eq. 5 error function value at the chosen pitch, not
  `CE_B` or `CE_F`. Those cumulative errors are internal to §0.3's
  pitch tracking and should not leak into the threshold.
- **`ξ_LF` and `ξ_HF` are one-sided sums.** The PDF specifies
  `m = 0..63` and `m = 64..128`, with no factor of 2 for the
  conjugate-symmetric negative half. Doubling these sums produces a
  different `M(ξ)` and silently shifts the V/UV boundary.
- **`ξ_max` floor of 20000 is not optional.** Eq. 42's denominator
  `0.01·ξ_max + ξ_0` goes to zero as both quantities vanish; the
  floor keeps `M(ξ)` well-defined. Cold-start init at 20000 (not 0)
  is load-bearing.
- **`v̂_k(−1)` is indexed by band, not by harmonic.** The previous
  frame's `K̂` may differ from the current; padding policy is to
  treat out-of-range indices as unvoiced (0).
- **Negative `Θ_ξ` at high `k`.** The `[1 − 0.3096·(k−1)·ω̂_0]` factor
  can dip below zero at the extreme (k=12, high ω̂_0). A negative
  threshold is equivalent to forcing unvoiced (since `D_k ≥ 0`); no
  algorithmic bug, but implementations should document the clamp
  (or lack thereof) for reproducibility.
- **Bin index `m = 128`.** This is the Nyquist bin in the two-sided
  `[−127, 128]` convention. A packed 256-FFT returns it at offset
  128 (the midpoint). Getting the packed↔two-sided translation wrong
  here shifts `ξ_HF` by one bin.
- **Order vs §0.5.** V/UV must finish before spectral amplitude
  estimation reads `v̂_k`. Implementations that parallelize bands
  across §0.5 and §0.7 need a barrier between the two stages.

---

## §0.8 Frame-Type Dispatch (Voice / Silence / Tone / Erasure)

**Source:** TIA-102.BABA-A §6.1 (page 22, full-rate `b̂_0` range);
§13.1 and Table 14 (pages 58–59, half-rate frame-type assignment);
§13.1 Eq. 143–145 (silence-frame decoder parameters); §16 "Tone
Frame Parameters" and Annex T (tone frame format). **The PDF does
not specify encoder-side entry criteria** for silence or tone; this
subsection is the one place in the addendum where policy fills a
genuine gap in the standard.

**Consumes:** post-HPF PCM frame (for energy-based silence
detection); the preroll counter (§0.9.4); optional upstream SAP
tone metadata.

**Produces:** a frame-type tag `∈ {voice, silence, tone, erasure}`
plus the per-type payload. On **emission**, the downstream wire
encoder consumes the tag and writes the appropriate bit pattern per
§0.8.2 / §0.8.3.

### 0.8.1 Two Different Dispatch Surfaces

Full-rate and half-rate expose frame types very differently:

| Type | Full-rate (FDMA, `b̂_0 ∈ [0, 207]`) | Half-rate (TDMA, `b̂_0` is 7 bits) |
|------|-------------------------------------|-------------------------------------|
| voice | default (all 88 info bits = MBE) | `b̂_0 ∈ [0, 119]` (Table 14) |
| silence | no sentinel; encode as voice | `b̂_0 ∈ {124, 125}` (§13.1 says 124) |
| tone | no sentinel; no tone frame at all | `b̂_0 ∈ {126, 127}` (Annex T payload) |
| erasure | no encoder emission path | `b̂_0 ∈ [120, 123]` (receiver FEC only) |

The full-rate bitstream has **no dedicated sentinel** for silence or
tone: every frame is a voice frame, and the bit range `b̂_0 ∈ [208, 255]`
is reserved unused per §6.1 prose. The half-rate 7-bit `b̂_0` reserves
four explicit ranges per Table 14 and Eq. 143–145 define the decoder
reconstruction for each.

### 0.8.2 Full-Rate Dispatch (IMBE, §6.1)

**Emission.** The full-rate encoder always emits a voice frame. The
88 information bits always encode `{ω̂_0, L̂, v̂_k, M̂_l}` per §0.4–§0.6
regardless of frame content.

**Silence on full-rate.** Two approaches are observed in the wild:

- **(a) Pass-through.** Emit a voice frame whose parameters come
  from running §0.3–§0.6 on the silent input. The result is a
  low-energy voice frame that sounds silent after decoding. This is
  what the DVSI AMBE-3000 chip does at rate 33 per empirical test
  vectors.
- **(b) Synthetic silence.** Emit a voice frame with hard-coded
  "silence-like" parameters, e.g. `ω̂_0 = 2π/32`, `L̂ = 14`,
  `v̂_k = 0` for all `k`, `M̂_l` small constant — mirroring the
  half-rate silence-decode synthesis of Eq. 143–145.

The PDF prescribes neither. Option (a) is simpler (no dispatch at
all — the encoder just runs the pipeline and whatever falls out
falls out) and matches observed DVSI behavior. Option (b) produces
bit-exact silence frames but requires the encoder to detect silence,
which is not in the PDF. **Recommended: option (a) for full-rate.**

**Tone on full-rate.** There is no full-rate tone frame. Tone
signaling at the full-rate air interface is routed out-of-band (via
control channel messages, not via the vocoder bitstream). The
encoder should treat all input as voice. Gap report §7 previously
flagged this; restating here: full-rate has no tone dispatch path.

**Erasure on full-rate.** The encoder never emits an erasure marker
because full-rate has no bit pattern for it. Erasure is a
receiver-side classification produced by the FEC layer
(BAAA-B / §6.1) when bit-error estimates exceed a threshold, at
which point the receiver performs frame repeat per BABA-A §9
(synthesis smoothing). The analysis encoder plays no role.

### 0.8.3 Half-Rate Dispatch (AMBE+2, §13.1, Table 14)

**Table 14 frame-type map** (page 58, verbatim):

| `b̂_0` range | Frame type |
|:-----------:|------------|
| 0–119       | voice      |
| 120–123     | erasure    |
| 124–125     | silence    |
| 126–127     | tone       |

**Silence emission — §13.1 prose:** "If the frame is a silence frame,
then `b̂_0 = 124`." Canonical silence uses `b̂_0 = 124` specifically,
not any value in `[124, 125]`. The 125 slot exists in Table 14 but
§13.1 does not describe an encoder path that emits it; it is
available for receiver-side tolerance to bit errors that flip
`b̂_0 = 124` to 125 (Hamming distance 1).

**Silence payload — §13.3 prose (page 61):** "An exception to this
reconstruct and update process occurs for silence frames, tone
frames and erasure frames in which case the saved reconstructed
spectral amplitudes are not updated with the reconstructed spectral
amplitudes from the current frame. This ensures that only voice
frames (i.e. not silence frames, tone frames or erasure frames) are
used in predicting future spectral amplitudes." The encoder-side
implication: when the encoder emits silence, it must **not** update
its own matched-decoder state `M̃_l(−1)` (§0.6.6). The predictor
state freezes for the duration of the silence interval; the next
voice frame predicts from the last pre-silence voice frame's
reconstruction.

**Silence synthesis parameters — Eq. 143–145** (page 58):

```
ω̃_0  =  2π / 32                                                (Eq. 143)
L̃    =  14                                                     (Eq. 144)
ṽ_l  =  0    for all l                                          (Eq. 145)
```

These are the **decoder-side** reconstruction values: when a
receiver sees `b̂_0 = 124`, it sets `ω̃_0 = 2π/32`, `L̃ = 14`,
`ṽ_l = 0`, then decodes `b̂_1..b̂_8` per §13.3–13.4 using those
synthetic parameters. The encoder's job for a silence frame is to
pick the spectral amplitudes to encode in `b̂_1..b̂_8`; a natural
choice is the actual `M̂_l` from §0.5 with `L̂ = 14` forced and all
bands unvoiced. Per §13.3 "This table applies to both voice frames
and silence frames" — the same encoding procedure is used, just
with different `ω̃_0 / L̃ / ṽ_l` values.

**Tone emission.** `b̂_0 ∈ {126, 127}`, with `b̂_1..b̂_8` carrying the
Annex T tone payload (tone frequency, amplitude indices). Annex T
is already extracted as `annex_tables/annex_t_tone_params.csv`. The
encoder's tone dispatch is **not** driven by the MBE analysis — it
is driven by upstream SAP signaling (PTT held with DTMF key, console
dispatching a tone, etc.) that bypasses the analysis encoder
entirely.

**Erasure emission.** As with full-rate, the encoder never emits
`b̂_0 ∈ [120, 123]`. Per §13.1 prose (page 59): "If the frame is
determined to be an erasure frame, then the frame is considered
invalid ... Note that for erasure frames, only the frame type needs
to be decoded, and the remaining bits in the frame are ignored."
Erasure is a receiver-side FEC outcome; the encoder emits voice (or
silence, or tone) always.

### 0.8.4 Encoder-Side Silence Detection (NOT in the PDF)

BABA-A specifies **how** to encode a silence frame (§0.8.3) but not
**when** to. The entry criterion is implementation-defined. This
subsection documents a reasonable baseline; it is not a prescription.

**Baseline energy-threshold detector.** Let `E_f = Σ s²(n)` over the
present frame's 160 post-HPF samples. Maintain a running background-
noise floor estimate `η` via a fast-attack / slow-release envelope:

```
η(0)  =  ⎧ 0.95 · η(−1)  +  0.05 · E_f      if  E_f  <  η(−1)
         ⎩ 0.99 · η(−1)  +  0.01 · E_f      otherwise
```

Declare silence when `E_f < α · η(0)` with `α ≈ 2`; declare voice
when `E_f > β · η(0)` with `β ≈ 4`. The gap between `α` and `β`
provides hysteresis against chatter during low-level speech. A
frame-count hysteresis (require 3 consecutive voice frames before
leaving silence, or 5 consecutive silence frames before entering
silence) further stabilizes the decision.

**Why the PDF does not specify this.** Silence detection is a
perceptual / psycho-acoustic problem, not a bitstream problem.
BABA-A is a bitstream-and-synthesis standard; it specifies what
gets transmitted but not how to decide. Different deployments have
different noise environments, different acceptable silence latencies
(public safety demands rapid silence detection for channel-hand-off;
entertainment demands slow detection to avoid clipping breath
sounds), and different downstream receiver tolerances.

**DVSI behavior.** Empirical observation of the AMBE-3000 at rate 34
(half-rate) is that it emits `b̂_0 = 124` after ~3 consecutive frames
below approximately −45 dBFS input level, and leaves silence mode
after 1 frame above −40 dBFS. Blip25 should calibrate to match this
range for DVSI-compatibility testing, but the specific thresholds
are not normative.

**Recommendation.** For blip25's initial implementation, default to
option (a) from §0.8.2 (no explicit silence dispatch, just let the
pipeline run on low-energy input). Add an explicit silence detector
only when DVSI-compatibility testing demands bit-exact silence
frames; at that point, tune thresholds to match observed DVSI chip
behavior rather than picking arbitrary values from the literature.

### 0.8.5 Encoder-Side Tone Detection (NOT in the PDF)

Per gap report §7: "tone-frame *emission* follows the PDF (bit
layout, `f_0/l_1/l_2` quantization) but tone-frame *entry criteria*
are DVSI-black-box-defined and should be matched to the chip's
observed behavior rather than invented."

**The PDF path is empty.** §16 / Annex T define the tone-frame
payload (what gets transmitted when the encoder decides "this is a
tone"), but there is no equation, no prose, no threshold anywhere
in BABA-A that describes how the encoder makes that decision from
MBE analysis output.

**Signals the MBE analysis exposes that could drive tone detection:**

- **Unanimous voicing.** A pure tone produces `v̂_k = 1` for all
  `k ∈ [1, K̂]` (the synthetic voiced spectrum matches the input at
  every band).
- **Amplitude concentration.** One or two `M̂_l` values dominate the
  rest by many dB. For a single-frequency tone at `f_0`, the
  harmonic `l ≈ f_0 / (ω̂_0 · 8000 / 2π)` carries ~all the energy.
- **Pitch stability.** `ω̂_0` changes by less than some fractional
  threshold (e.g. 0.1%) across consecutive frames. Real speech has
  substantial pitch micro-variation; a tone does not.
- **Sustained duration.** All of the above hold for some minimum
  number of consecutive frames (e.g. 5 = 100 ms) before entering
  tone mode.

**Architectural recommendation.** Tone detection should live
**outside** the analysis encoder entirely, driven by the SAP-level
signaling module that knows whether the radio is currently emitting
a PTT-signaling tone vs. microphone audio. This matches the
hardware reality (a microphone preamp can't produce a pure enough
sinusoid to trigger a tone detector without false-positives on
whistles and sustained vowels) and keeps the blip25 analysis
encoder focused on PCM → MbeParams without a layering violation.

If inline tone detection is required, the signals above provide a
starting point, but calibration must be done against DVSI test
vectors — there is no "textbook" tone detector that matches DVSI
black-box behavior.

### 0.8.6 Preroll Frame Emission

Per §0.9.4, the encoder has a two-frame preroll during which
`P̂_I` cannot be computed (look-ahead buffer not yet filled). During
preroll:

- **Full-rate:** emit a voice frame with cold-start / synthetic-silence
  parameters. Concrete choice: `ω̂_0 = 2π/32` (matches half-rate
  silence decoder's `ω̃_0`), `L̂ = 9` (minimum), `v̂_k = 0` for all
  `k`, `M̂_l` small constant (e.g. 1e−3). Decoded audio is silent.
- **Half-rate:** emit silence frames (`b̂_0 = 124`) for the preroll
  duration. This is the clean dispatch — it explicitly tells the
  receiver that no voice is present yet.

Half-rate has the cleaner preroll story; full-rate has to improvise
because its bitstream lacks a silence sentinel.

### 0.8.7 Dispatch Pseudocode

```c
typedef enum {
    FRAME_VOICE,
    FRAME_SILENCE,
    FRAME_TONE,
    FRAME_ERASURE    /* never emitted by encoder; here for completeness */
} FrameType;

typedef struct {
    FrameType     type;
    /* union over type: voice → MbeParams; silence → none; tone → Annex T payload */
    MbeParams     voice;
    ToneParams    tone;
} EncoderFrame;

EncoderFrame dispatch_frame(AnalysisEncoderState *st,
                            const double pcm_frame[160],
                            int  sap_tone_request,        /* from SAP layer */
                            const ToneParams *sap_tone)  {
    /* Preroll — §0.8.6 */
    if (st->preroll_counter < 2) {
        st->preroll_counter++;
        return emit_preroll_silence();
    }

    /* SAP-driven tone — §0.8.5 */
    if (sap_tone_request) {
        return (EncoderFrame){ .type = FRAME_TONE, .tone = *sap_tone };
    }

    /* Run §0.1 HPF + §0.3–§0.7 analysis pipeline on pcm_frame. */
    MbeParams mbe = run_analysis_pipeline(st, pcm_frame);

    /* Optional explicit silence — §0.8.4, tune thresholds to DVSI. */
    if (silence_detector_says_silent(st, pcm_frame)) {
        /* Half-rate: emit silence frame; Full-rate: emit mbe as voice.     */
        /* The rate is a compile-time / config-time choice upstream of here;*/
        /* this function emits the type-tag and lets the wire encoder pick. */
        return (EncoderFrame){ .type = FRAME_SILENCE };
    }

    /* Default: voice. */
    return (EncoderFrame){ .type = FRAME_VOICE, .voice = mbe };
}
```

The wire encoder downstream of this (the `quantize::` modules in
blip25-mbe) picks the bit pattern per the rate-specific §0.8.2 /
§0.8.3 rules.

### 0.8.8 Common Pitfalls

- **Full-rate has no silence frame type.** Do not invent one; the
  `[208, 255]` reserved range is not a silence sentinel. Code that
  emits `b̂_0 = 208` (or similar) on silent input is non-compliant.
- **Half-rate silence is `b̂_0 = 124` specifically.** Table 14
  allows `{124, 125}`, but §13.1 prose singles out 124. Emitting
  125 is receiver-tolerant (Hamming distance 1 from 124) but is
  not the canonical encoder output.
- **Encoder never emits erasure.** `b̂_0 ∈ [120, 123]` is produced
  by the receiver's FEC layer when it detects uncorrectable errors.
  Encoders that emit erasure as a "dropped frame" substitute are
  conflating two layers.
- **Silence freezes the predictor.** §13.3 prose says silence / tone
  / erasure frames do not update the matched-decoder state. Encoders
  that run §0.6.6's feedback unconditionally will drift the
  receiver's predictor on re-entry after a silence interval.
- **Tone detection is not a BABA-A problem.** Gap report §7's
  "black-box DVSI behavior" applies here. Do not invent a tone
  detector from first principles; either run the SAP-level signal
  through or match DVSI empirically.
- **Preroll is silence, not garbage.** Frames 0 and 1 must emit
  audibly silent output, not random or zeroed bits. Silence-frame
  (half-rate) or synthetic-silence voice-frame (full-rate) is the
  clean choice.
- **Full-rate pass-through silence works.** Option (a) of §0.8.2 —
  just running the pipeline on silent input — produces low-energy
  voice frames that the decoder renders as silence. This is the
  simplest implementation and matches DVSI chip behavior. Prefer
  this over explicit silence detection unless bit-exact
  DVSI-compatibility testing demands otherwise.

---

## §0.9 Encoder State Structure and Initialization

**Source:** synthesizes cross-frame state requirements from §0.1–§0.7
and §0.6 of this addendum. The decoder-side Annex A exists at
implementation-spec §10; this is its encoder-side counterpart. Where
the PDF prescribes an initialization value it is cited; where it does
not, the inferred value is marked **(inferred)** with rationale.

### 0.9.1 Cross-Frame State Summary

All state variables an implementation must carry frame-to-frame:

| # | Variable | Shape | Section | Cold-start value | Update rule |
|---|----------|-------|---------|------------------|-------------|
| 1 | HPF state (`x_{−1}`, `y_{−1}`) | 2 scalars | §0.1.2 | `0.0, 0.0` **(inferred)** | Eq. 3, per sample |
| 2 | PCM lookahead buffer | ≥ 470 samples | §0.1.3 | zero **(inferred)** | shift-in by 160/frame |
| 3 | `P̂_{−1}`, `P̂_{−2}` | 2 scalars (samples) | §0.3.4 | `100.0, 100.0` (§5.1.3 prose) | shift at frame boundary |
| 4 | `E_{−1}(P̂_{−1})`, `E_{−2}(P̂_{−2})` | 2 scalars | §0.3.4 | `0.0, 0.0` (§5.1.3 prose) | shift |
| 5 | Forward `E_1(·)`, `E_2(·)` lookahead | 2 arrays × 203 values | §0.3.5 | — (requires 2 future frames) | recompute per frame |
| 6 | `ξ_max(−1)` | scalar | §0.7.4 | `20000.0` **(inferred)** | Eq. 41 |
| 7 | `v̂_k(−1)` | int vector, length 12 | §0.7.6 | all `0` **(inferred)** | overwrite with current `v̂_k` |
| 8 | `K̂(−1)` | int | §0.7 | `0` **(inferred)** | overwrite with current `K̂` |
| 9 | `M̃_l(−1)` | real vector, length `L̃(−1)` | §0.6.7 | all `1.0` (§6.3 prose, p. 25) | matched-decoder output |
| 10 | `L̃(−1)` | int | §0.6.7 | `30` (§6.3 prose, p. 25) | overwrite with `L̂(0)` |
| 11 | Preroll frame counter | int | §0.9.4 | `0` **(inferred)** | increment to 2 then freeze |

**Rationale for inferred values:**

- **Row 1 (HPF):** zero IC is the natural choice and matches the
  linear-time-invariant stability of `H(z) = (1 − z⁻¹)/(1 − 0.99 z⁻¹)`.
  Non-zero initial state produces a transient that decays with
  `0.99ⁿ`; a ~250-sample warmup absorbs it.
- **Row 2 (PCM buffer):** zero extension is the natural boundary
  handling for filters/windows that touch samples outside the
  delivered input.
- **Row 6 (`ξ_max`):** 20000 is the floor of Eq. 41; starting at the
  floor is the only value that's simultaneously self-consistent
  with Case 3 of Eq. 41 and avoids a `0/0` degeneracy in Eq. 42's
  `M(ξ)` denominator.
- **Row 7 (`v̂_k(−1)`):** all-unvoiced forces Eq. 37 into its
  "otherwise" branch (threshold `0.45 · …`), which is the lower
  threshold and requires stronger evidence before declaring the
  first frame's bands voiced. Starting all-voiced would bias the
  first frame toward voiced, an undesirable default on a cold
  microphone input.
- **Row 8 (`K̂(−1)`):** `0` makes any lookup into `v̂_k(−1)` fall out
  of range, which per §0.7.8 is handled as "treat as `0`".
- **Row 11 (preroll counter):** see §0.9.4.

**Non-state data (recompute per frame, not persisted):**

- `s(n)` — HPF output (function of row 1 + current input)
- `s_LPF(n)` — LPF output (function of `s(n)`)
- `S_w(m)`, `S_w(m, ω̂_0)` — DFTs
- `ξ_LF`, `ξ_HF`, `ξ_0` — per-frame energy statistics
- `E(P)` — error function at all 203 candidate pitches (becomes `E_{−1}` state after two frame shifts)
- `P̂_I`, `ω̂_0`, `L̂`, `K̂`, `â_l`, `b̂_l` — per-frame derived quantities
- `v̂_k(0)`, `M̂_l(0)`, `T̂_l`, `b̂_0..b̂_{L̂+1}` — per-frame outputs

### 0.9.2 Consolidated C Struct

```c
#include <stdint.h>

#define MAX_L          56    /* Eq. 31 upper bound */
#define MAX_K          12    /* Eq. 34 cap */
#define LOOKAHEAD_LEN  480   /* 2 frames × 160 + 160 for w_I edges, generous */

typedef struct {
    /* §0.1: input conditioning */
    double   hpf_x1;                         /* HPF delay, x-side */
    double   hpf_y1;                         /* HPF delay, y-side */
    double   lookahead[LOOKAHEAD_LEN];       /* rolling post-HPF buffer */
    int      lookahead_fill;                 /* 0..2: frames buffered */

    /* §0.3: pitch tracking */
    double   P_prev[2];                      /* [0] = P̂_{−1}, [1] = P̂_{−2} */
    double   E_prev[2];                      /* [0] = E_{−1}(P̂_{−1}), [1] = E_{−2}(P̂_{−2}) */

    /* §0.7: V/UV tracking */
    double   xi_max;                         /* ξ_max(−1) */
    uint8_t  vuv_prev[MAX_K + 1];            /* v̂_k(−1), 1-indexed; [0] unused */
    int      K_prev;                         /* K̂(−1) */

    /* §0.6: matched-decoder predictor */
    double   M_tilde_prev[MAX_L + 1];        /* M̃_l(−1), 1-indexed; [0] = 1.0 (Eq. 56) */
    int      L_tilde_prev;                   /* L̃(−1) */

    /* §0.9: dispatch */
    int      preroll_counter;                /* 0 → 1 → 2 → steady */
} AnalysisEncoderState;

void analysis_encoder_state_init(AnalysisEncoderState *st) {
    /* §0.1 — zero HPF and buffer */
    st->hpf_x1 = 0.0;
    st->hpf_y1 = 0.0;
    for (int i = 0; i < LOOKAHEAD_LEN; ++i) st->lookahead[i] = 0.0;
    st->lookahead_fill = 0;

    /* §0.3 — §5.1.3 prose, page 14 */
    st->P_prev[0] = 100.0;
    st->P_prev[1] = 100.0;
    st->E_prev[0] = 0.0;
    st->E_prev[1] = 0.0;

    /* §0.7 — inferred */
    st->xi_max  = 20000.0;
    for (int k = 0; k <= MAX_K; ++k) st->vuv_prev[k] = 0;
    st->K_prev  = 0;

    /* §0.6 — §6.3 prose, page 25 */
    for (int l = 0; l <= MAX_L; ++l) st->M_tilde_prev[l] = 1.0;
    st->L_tilde_prev = 30;

    /* §0.9 — preroll */
    st->preroll_counter = 0;
}
```

### 0.9.3 Per-Frame Update Order

After a steady-state frame's analysis completes, the state fields are
overwritten in this order (the order matters because some updates
consume the old value before replacing it):

1. **Shift pitch history:** `P_prev[1] ← P_prev[0]`, `P_prev[0] ← P̂_I(current)`; same for `E_prev`.
2. **Commit V/UV:** `vuv_prev[1..K̂] ← v̂_k(current)`, zero the tail `vuv_prev[K̂+1..MAX_K]`, `K_prev ← K̂`.
3. **Commit `ξ_max`:** `xi_max ← ξ_max(0)` (the Eq. 41 output computed during the frame).
4. **Commit matched-decoder output:** `M_tilde_prev[1..L̂] ← M̃_l(0)` (from §0.6.6), `M_tilde_prev[0] ← 1.0`, `L_tilde_prev ← L̂`.
5. **Advance lookahead:** drop the oldest 160 samples, shift the rest, append the next incoming frame (post-HPF).

HPF state (`hpf_x1`, `hpf_y1`) is updated continuously inside
`hpf_step` during step 5; it doesn't need a separate commit.

### 0.9.4 Preroll Policy

The encoder cannot produce a valid `P̂_I` until frame index 2 because
§0.3.5 look-ahead tracking requires two future frames' `E_1`, `E_2`.
On frames 0 and 1 the encoder:

- Still runs the HPF and fills the lookahead buffer.
- Does **not** evaluate §0.3 pitch tracking, §0.4 refinement, §0.5
  amplitudes, or §0.6 predictor.
- Emits silence (implementation-spec §6.2 full-rate or §6.1
  half-rate) on the wire, independent of input energy.

From frame 2 onward the full pipeline runs. The preroll counter
increments on each frame until it reaches 2, then stays there. A
flush-on-stream-end policy (draining the lookahead with silence
frames at the end of input) is symmetric and lives in the calling
application, not the encoder itself.

The first real output frame (index 2) has no past matched-decoder
output yet — `M̃_l(−1)` is still the cold-start all-ones vector.
This is intentional: the predictor's contribution on frame 2 is
zero (since `log₂ 1.0 = 0`), so `T̂_l = log₂ M̂_l(0)` and quantizer
efficiency is suboptimal only for that first real frame.

### 0.9.5 State Reset Conditions

Full reset via `analysis_encoder_state_init` is appropriate:

- **Cold power-up.** Obvious.
- **PTT release → new PTT press.** Between keyups, the lookahead
  is stale and the pitch/V/UV history belongs to a different
  utterance. Reset to avoid cross-contamination.
- **Stream discontinuity.** Gap in the input PCM (lost packets,
  DMA underrun) should reset; a partial reset that keeps only the
  HPF state does more harm than good because the lookahead buffer
  will contain stale samples next to fresh ones.
- **Corrupt state detected.** E.g. `NaN` in `xi_max` or
  `M_tilde_prev`. Paranoid validators should reset; silent
  tolerance hides upstream bugs.

Partial reset (preroll counter only, keep HPF and predictor state)
is not a PDF-sanctioned operation and is not recommended. The
equations assume either fully-cold or fully-warm state.

### 0.9.6 Storage Footprint

```
  HPF            :    2 × f64           =    16 B
  lookahead      :  480 × f64           = 3840 B
  pitch history  :    4 × f64           =   32 B
  V/UV history   :    1 × f64  + 13 u8 + 1 int  ≈ 32 B
  predictor      :   57 × f64  + 1 int  ≈  464 B
  dispatch       :    1 int             =    4 B
                                        ─────────
                                         ~4.4 KB
```

A practical encoder state is well under 5 KB, dominated by the
lookahead buffer. No dynamic allocation is required; a single
pre-allocated struct suffices.

### 0.9.7 Common Pitfalls

- **`M̃_l(−1) = 1.0`, not 0.0.** Row 9 initializes amplitudes to 1
  so that `log₂` is well-defined in Eq. 54. Zero-init is the most
  common beginner mistake and produces `−∞` on the first frame.
- **`L̃(−1) = 30`, not 0 and not `L̂(0)`.** The PDF prescribes 30.
  Setting it to `L̂(0)` self-consistent only after the first frame;
  using 0 divides by zero in Eq. 52.
- **`ξ_max = 20000`, not 0.** Ties into Eq. 42's denominator; a
  zero-initialized `ξ_max` plus a silent first frame (`ξ_0 = 0`)
  gives `0/0` in `M(ξ)`.
- **Preroll is two frames, not one.** Look-ahead tracking
  (§0.3.5) needs `E_1` AND `E_2`. A one-frame preroll produces a
  valid `E_1` but no `E_2`, which the algorithm cannot fall back
  from cleanly.
- **State update order.** Shift pitch history **before** computing
  the next frame's `E(P)` — the look-back branch reads
  `P_prev[0]` as "previous frame's pitch", not "current frame's
  pitch". Reversing step 1 with the next frame's analysis
  silently uses the wrong `P̂_{−1}`.
- **V/UV padding.** When `K̂(current) < K_prev`, zero the tail of
  `vuv_prev` so a later frame with `K̂(future) > K̂(current)` does
  not read stale bits from the old larger `K_prev` region.

---

## §0.10 End-to-End Reference C Pipeline

**Source:** synthesizes §0.1 through §0.8 of this addendum into a
single `pcm_frame + state → EncoderFrame` function. Does not
introduce new equations or interpretations; this subsection is the
integration target that implementers should numerically match.

The per-subsection reference C blocks (§0.1.7, §0.2.7, §0.3.8,
§0.4.6, §0.5.5, §0.6.8, §0.7.9) define the individual stages; this
subsection's contribution is the **orchestration** — execution order,
feedback loops, and dispatch branches.

### 0.10.1 Per-Frame Execution Order

The order matters because:

- §0.3 look-back uses past-frame pitch state before it is overwritten
  with the current frame's output.
- §0.7 V/UV must finish before §0.5 reads `v̂_k`.
- §0.6 predictor state update must happen **after** the matched-
  decoder reconstruction of §0.6.6, which in turn needs the
  quantizer output — so §0.6 straddles the wire boundary.
- The silence/tone/erasure dispatch of §0.8 gates the predictor
  state commit: non-voice frames freeze `M̃_l(−1)`.

Execution sequence per 20 ms frame:

```
 1.  HPF input frame                                        (§0.1)
 2.  Shift lookahead buffer; if preroll, emit silence       (§0.1, §0.9.4)
 3.  SAP tone short-circuit                                 (§0.8.5)
 4.  Compute s_LPF, r(t), E(P) over 203 candidate pitches   (§0.3.1–§0.3.3)
 5.  Pitch tracking → P̂_I, E(P̂_I)                          (§0.3.4–§0.3.6)
 6.  256-pt DFT → S_w(m)                                    (§0.2.1)
 7.  Refine pitch → ω̂_0, L̂, â_l, b̂_l, b̂_0                 (§0.4)
 8.  Build S_w(m, ω̂_0) at the refined fundamental           (§0.2.3)
 9.  V/UV determination → K̂, v̂_k                           (§0.7)
10.  Spectral amplitude estimation → M̂_l                    (§0.5)
11.  Silence decision (optional, DVSI-calibrated)            (§0.8.4)
        → silence branch: emit silence, SKIP 12–14
12.  Log-magnitude prediction residual → T̂_l               (§0.6.1–§0.6.5)
13.  Wire-side quantization (outside this addendum)          (blip25-mbe quantize::)
        → b̂_0..b̂_{L̂+1}
14.  Matched-decoder reconstruction → M̃_l(0), commit        (§0.6.6)
15.  Commit pitch / V/UV / ξ_max state for next frame        (§0.9.3)
```

Steps 13 and 14 are a round-trip through the existing wire-side
encoder and a mirror-image decoder. The addendum ends at step 12
(producing `T̂_l`); steps 13–14 live in `blip25-mbe::quantize` and the
existing implementation-spec §1.8.

### 0.10.2 Reference Orchestration C

```c
EncoderFrame analysis_encode_frame(AnalysisEncoderState *st,
                                   const int16_t pcm_input[160],
                                   RateMode rate_mode,
                                   const SapControl *sap) {
    /* 1. HPF (§0.1.2, Eq. 3) */
    double s[160];
    double pcm_f64[160];
    for (int i = 0; i < 160; ++i) pcm_f64[i] = (double)pcm_input[i];
    hpf_frame(st, pcm_f64, s);

    /* 2. Advance the lookahead buffer and preroll-check (§0.1.3, §0.9.4) */
    lookahead_shift_in(st, s);
    if (st->preroll_counter < 2) {
        st->preroll_counter++;
        return emit_preroll_silence(rate_mode);
    }

    /* 3. SAP tone passthrough (§0.8.5) */
    if (sap->tone_active) {
        return (EncoderFrame){ .type = FRAME_TONE, .tone = sap->tone };
    }

    /* 4. Compute s_LPF, r(t), E(P) (§0.3.1–§0.3.3) */
    double sLPF[301];
    compute_sLPF(st->lookahead, sLPF);              /* Eq. 9 */
    double r[123];
    compute_r(sLPF, r);                              /* Eq. 7 integer-t */
    double E_curr[203];
    for (int i = 0; i < 203; ++i) {
        double P = 21.0 + 0.5 * (double)i;
        E_curr[i] = E_of_P(sLPF, r, P);              /* Eq. 5 */
    }

    /* 5. Pitch tracking → P̂_I (§0.3.4–§0.3.6) */
    double P_hat_I;
    double E_at_P_hat_I;
    estimate_initial_pitch(st, E_curr, &P_hat_I, &E_at_P_hat_I);

    /* 6. 256-pt DFT → S_w(m) (§0.2.1, Eq. 29) */
    double complex Sw[N_DFT];
    compute_Sw_from_buffer(st->lookahead, Sw);

    /* 7. Pitch refinement (§0.4) */
    PitchRefineOut refined = refine_pitch(Sw, P_hat_I);
    /* refined.omega_hat, .L_hat, .a_hat[], .b_hat[], .b0 */

    /* 8. Build S_w(m, ω̂_0) at the selected fundamental (§0.2.3) */
    double complex Sw_synth[N_DFT];
    double complex Al_dummy[MAX_L + 1];
    synthesize_Sw(refined.omega_hat, Sw_synth, Al_dummy, refined.L_hat);

    /* 9. V/UV determination (§0.7) */
    int K_hat;
    int vuv[MAX_K + 1] = {0};
    determine_vuv(Sw, Sw_synth, &refined, E_at_P_hat_I,
                  st->WR_DC_sq,                 /* precomputed at startup */
                  &(VuvState){ .xi_max = st->xi_max,
                               .vuv_prev = {0},  /* copy in from st */
                               .K_prev = st->K_prev },
                  &K_hat, vuv);
    /* (Adapter elided — sync st ←→ VuvState fields.) */

    /* 10. Spectral amplitude estimation (§0.5) */
    double M_hat[MAX_L + 1];
    estimate_amplitudes(Sw, &refined, K_hat, vuv, M_hat);

    /* 11. Optional silence (§0.8.4). If silent: skip §0.6 predictor update. */
    if (silence_detector(st, s)) {
        /* Do NOT call predictor_state_commit — §0.8.3 / §13.3 prose.        */
        /* Still commit pitch, V/UV, ξ_max — these are always tracked.       */
        commit_non_predictor_state(st, P_hat_I, E_at_P_hat_I, K_hat, vuv);
        return emit_silence(rate_mode, M_hat, refined.L_hat, refined.omega_hat);
    }

    /* 12. Log-magnitude prediction residual (§0.6.5, Eq. 54) */
    double T_hat[MAX_L + 1];
    PredictorState pred = {
        .L_tilde_prev = st->L_tilde_prev,
        /* .M_tilde_prev[] aliased from st */
    };
    memcpy(pred.M_tilde_prev, st->M_tilde_prev, sizeof(pred.M_tilde_prev));
    compute_prediction_residual(M_hat, refined.L_hat, &pred, T_hat);

    /* 13. Wire-side quantization — blip25-mbe::quantize (NOT in this addendum). */
    QuantizedBits qb;
    qb.b[0] = refined.b0;
    pack_vuv_into_b1(vuv, K_hat, &qb);
    quantize_spectral(T_hat, refined.L_hat, &qb);   /* fullrate or halfrate */

    /* 14. Matched-decoder reconstruction (§0.6.6) — closes the AR loop.    */
    double M_tilde_now[MAX_L + 1];
    int L_tilde = refined.L_hat;                    /* §6.3: encoder sets L̃ = L̂ */
    run_matched_decoder(&qb, L_tilde, M_tilde_now);
    memcpy(st->M_tilde_prev, M_tilde_now, sizeof(st->M_tilde_prev));
    st->M_tilde_prev[0] = 1.0;                      /* Eq. 56 */
    st->L_tilde_prev    = L_tilde;

    /* 15. Commit per-frame state for next frame (§0.9.3). */
    st->P_prev[1] = st->P_prev[0];  st->P_prev[0] = P_hat_I;
    st->E_prev[1] = st->E_prev[0];  st->E_prev[0] = E_at_P_hat_I;
    for (int k = 1; k <= K_hat; ++k) st->vuv_prev[k] = (uint8_t)vuv[k];
    for (int k = K_hat + 1; k <= MAX_K; ++k) st->vuv_prev[k] = 0;
    st->K_prev  = K_hat;
    st->xi_max  = /* ξ_max(0) from step 9 */ 0.0;   /* adapter: wire through */

    /* 16. Build the output frame. */
    MbeParams mbe = {
        .omega = refined.omega_hat,
        .L     = refined.L_hat,
        .K     = K_hat,
        .b0    = refined.b0,
    };
    memcpy(mbe.v, vuv, sizeof(mbe.v));
    memcpy(mbe.M, M_hat, sizeof(mbe.M));
    mbe.bits = qb;
    return (EncoderFrame){ .type = FRAME_VOICE, .voice = mbe };
}
```

The `commit_non_predictor_state` helper is a small projection of
step 15 that skips the `M_tilde_prev` / `L_tilde_prev` lines. The
`VuvState` adapter between steps 9 and 15 is mechanical bookkeeping
(copy in from the consolidated state, copy out after).

### 0.10.3 Rate-Mode Fork

The `RateMode` argument selects between the two bitstream shapes:

- `RATE_FULL` (IMBE, 7200 bps): `b̂_0 ∈ [0, 207]`, 88 information bits,
  matched decoder runs implementation-spec §1.8.1–1.8.5 (full-rate
  branch), `ρ` from Eq. 55.
- `RATE_HALF` (AMBE+2, 3600 bps): `b̂_0 ∈ [0, 127]`, 49 information
  bits, matched decoder runs implementation-spec §2.11–2.13 (half-
  rate branch), `ρ = 0.65` (literal per Eq. 155).

The analysis pipeline itself (§0.1–§0.7) is rate-agnostic: the same
`M̂_l`, `ω̂_0`, `v̂_k`, `L̂` feed both rates. The rate-specific code
lives in the matched decoder (step 14) and the wire-side quantizer
(step 13). The `PredictorState.rho_of_L` function picks the right
`ρ` per rate.

### 0.10.4 Feedback Loop Invariants

The closed-loop matched-decoder design imposes three invariants that
must hold at the start of each frame:

1. `st->M_tilde_prev[]` reflects the **receiver's** reconstruction
   of the prior frame, not the encoder's unquantized `M̂_l`. Checking:
   the next frame's `T̂_l` residuals should be small for stationary
   voice; residuals that grow unboundedly across frames indicate the
   feedback loop is broken (e.g. encoder is using `M̂` not `M̃` in
   step 14).
2. `st->L_tilde_prev` reflects the **receiver's** `L̃`, which in the
   matched-decoder case equals `L̂` of the previous frame (since
   step 14 sets `L̃ = L̂`). Checking: `st->L_tilde_prev` should equal
   the previous frame's `refined.L_hat`.
3. `st->vuv_prev[]` indexes by band, and entries beyond the previous
   frame's `K̂` are zero. Checking: on a frame where `K̂(current) >
   K̂(prev)`, the new bands should read `vuv_prev = 0` in the
   hysteresis branch of Eq. 37.

A unit test that drives a synthetic stationary sinusoid through the
pipeline should observe the residuals `T̂_l` converging to small
values (roughly the quantization noise level) within 3–5 frames.
Divergent residuals indicate a broken feedback loop; flat near-zero
residuals on frame 1 indicate the matched decoder is cheating by
feeding back `M̂` instead of `M̃`.

### 0.10.5 Common Integration Pitfalls

- **Step 6 vs Step 8 both use DFTs of w_R-windowed signal.** They
  are the same `S_w(m)`, not two independent DFTs. Step 8 needs only
  the output of step 6 plus `ω̂_0`; do not re-window the input.
- **Step 14 runs the decoder, not just the inverse quantizer.** The
  matched-decoder path must include inverse log-magnitude prediction
  (impl spec §1.8.5) — otherwise `M̃_l` is just `2^T̃_l`, not a valid
  reconstruction. This is the most common integration bug.
- **State commit order (step 15).** Shift `P_prev[1] ← P_prev[0]`
  **before** writing `P_prev[0] ← P_hat_I(current)`. Reversing drops
  the oldest history and duplicates the current. Same for `E_prev`.
- **Silence branch skips steps 12–14.** The `M_tilde_prev` vector
  must freeze exactly as it was. Updating `M_tilde_prev` with the
  silence-frame's synthetic amplitudes breaks predictor continuity
  across silence intervals.
- **Preroll branch (step 2) skips everything.** Frames 0 and 1 still
  run the HPF (step 1) and advance the lookahead buffer, but they
  must not run steps 4–15. The look-ahead `E_1`, `E_2` are not valid
  until frame 2.
- **Rate mode is a runtime flag, not a compile-time macro.** A radio
  may mix IMBE voice frames with half-rate data PDUs in the same
  stream; the rate mode can vary frame-to-frame. Do not cache
  rate-dependent precomputations (e.g. `rho_of_L` tables) across a
  rate change.

---

## §0.11 Numerical Cross-Checks Against DVSI Test Vectors

**Source:** synthesizes the validation strategy implied by the
project's DVSI-interoperability target (per `CLAUDE.md`) and the
test-vector modes documented in
[`dvsi_test_vector_modes.md`](./dvsi_test_vector_modes.md). Phase 4
verification patterns per
[`phase4_findings_log.md`](./phase4_findings_log.md). This subsection
is policy-heavy — the PDF does not specify validation tolerances.

### 0.11.1 Why Numerical Cross-Checks

The gap report's §"Why OSS cannot fill this gap" established that
the project's correctness target is the DVSI AMBE-3000 chip's output
on known input PCM, not any open-source reference implementation.
The analysis encoder drafted in §0.1–§0.8 of this addendum is only
correct insofar as it produces `MbeParams` that, when quantized and
synthesized by a matched decoder, yield DVSI-equivalent audio.

Bit-exact equivalence is achievable in principle (both the encoder
and DVSI are deterministic functions of PCM input) but requires
matching:

- Floating-point rounding behavior in intermediate sums.
- Tie-breaking rules in argmin scans (pitch tracking, pitch refinement).
- DVSI-black-box policies (silence threshold, tone detection, preroll
  frame content) that the PDF does not prescribe.

**Target tiers.** Validation should proceed in order:

1. **Tier 1 — Parameter equivalence.** Given identical PCM input,
   the encoder's `{ω̂_0, L̂, K̂, v̂_k, M̂_l}` should match DVSI's to
   within fixed tolerances (§0.11.5). This is the strict bitstream
   interop goal.
2. **Tier 2 — Bitstream equivalence.** Quantized `b̂_0..b̂_{L̂+1}`
   should match DVSI's bit-exactly. Passes only if Tier 1 passes
   and the wire-side quantizer (outside this addendum) is correct.
3. **Tier 3 — Perceptual equivalence.** Decoded audio via a third-
   party decoder (mbelib, JMBE) should be perceptually
   indistinguishable from DVSI-decoded audio. Loosest target; fails
   can indicate problems anywhere in the chain including the
   decoder.

Tier 1 is the addendum's validation target; Tiers 2 and 3 are
downstream.

### 0.11.2 DVSI Test-Vector Sources

Per [`dvsi_test_vector_modes.md`](./dvsi_test_vector_modes.md), the
DVSI AMBE-3000 evaluation board produces test vectors at three
relevant rate modes:

| Rate | Bitstream | FEC | Use for validation |
|------|-----------|-----|--------------------|
| 33   | full-rate IMBE (P25 Phase 1) | yes | Tier 1/2/3 full-rate validation |
| 34   | full-rate, no FEC | skip | Tier 1 intermediate checks (bypass FEC bugs) |
| 39   | proprietary | yes | Not P25-compatible; do not use |

Each test vector comprises:

- An input PCM file (mono, 8 kHz, 16-bit linear, little-endian raw
  or WAV).
- A corresponding output bitstream (raw binary, packed per the
  rate's frame format).

Blip25's cross-check harness should run identical PCM through the
analysis encoder and diff the output bitstream against DVSI's,
frame-by-frame.

**Recording test vectors.** If direct DVSI output is not available,
the AMBE-3000 eval board's serial interface emits frame bytes at
steady 20 ms cadence; capture via a logic analyzer or UART passthrough.
A set of ~100 frames (2 seconds of audio) across varied input
(speech, silence, tones, noise) is sufficient for Tier 1 validation.

### 0.11.3 Checkpoint Strategy

Intermediate diffs at subsection boundaries let a mismatch be
localized quickly. Recommended checkpoints, per frame:

| Checkpoint | Value | Compared against |
|-----------|-------|-------------------|
| C1 | `s(n)` after HPF (§0.1.2) | Synthetic: HPF applied to PCM; no DVSI counterpart exposed |
| C2 | `E(P)` for P = 50.0 (§0.3.3) | Synthetic: recompute in a reference implementation |
| C3 | `P̂_I` (§0.3.6) | Derived from DVSI `b̂_0` via Eq. 45 |
| C4 | `ω̂_0`, `L̂`, `b̂_0` (§0.4.4) | DVSI `b̂_0` directly; `ω̂_0` and `L̂` derived |
| C5 | `ξ_LF`, `ξ_HF`, `M(ξ)` (§0.7.3–§0.7.5) | Synthetic reference; no DVSI counterpart |
| C6 | `v̂_k` (§0.7.6) | DVSI `b̂_1` decoded per Eq. 49 |
| C7 | `M̂_l` (§0.5) | Derived from DVSI `b̂_2..b̂_{L̂+1}` via the inverse encoder chain |
| C8 | `T̂_l` (§0.6.5) | Synthetic reference; no DVSI counterpart |
| C9 | `b̂_2..b̂_{L̂+1}` (step 13) | DVSI bitstream directly (Tier 2) |

Only C3, C4 (partial), C6, C7, and C9 have direct DVSI counterparts.
The others need a reference implementation — typically a
straightforward-but-slow Python port of this addendum's equations,
cross-validated against DVSI at the external checkpoints.

### 0.11.4 Per-Subsection Cross-Checks

A minimum validation suite, per subsection:

- **§0.1 (framing, HPF).** Compare `s(n)` from C1 to the reference
  HPF output on synthetic input (step impulse, DC, 1 kHz sinusoid).
  No direct DVSI comparison.
- **§0.2 (DFT, basis).** Compute `S_w(m)` on a synthetic sinusoid
  at a known frequency; verify the spectral peak appears at the
  expected bin with the expected magnitude. Compute `S_w(m, ω̂_0)`
  at the same `ω̂_0` and verify near-zero residual `|S_w − S_w(·, ω̂_0)|²`.
- **§0.3 (initial pitch).** Play synthetic periodic input (sawtooth
  at known pitch); `P̂_I` must match the synthetic ground truth to
  within ±0.5 samples. Also run on real speech and compare to DVSI's
  C3 via round-trip (DVSI `b̂_0` → `ω̃_0` via Eq. 46 → `P = 2π/ω̃_0`).
  Expected match: `|P̂_I(ours) − P̂_I(dvsi)| ≤ 0.5` samples.
- **§0.4 (pitch refinement).** DVSI's `b̂_0` quantizes to the nearest
  level per Eq. 45; our `b̂_0` should match bit-exactly after Tier 1
  validation. If off by 1, suspect the `−39` vs `−39.5` trap
  (§0.4.4). If off by more, §0.3 or §0.4.2 is the suspect.
- **§0.5 (spectral amplitudes).** Derive DVSI's `M̂_l` by running
  the receiver's decode of DVSI's `b̂_2..b̂_{L̂+1}` and inverting the
  log-magnitude prediction. Compare per-harmonic; expected relative
  error ≤ 1% for voiced harmonics, ≤ 10% for unvoiced (quantization
  noise is larger in the unvoiced formula Eq. 44).
- **§0.6 (prediction residual).** Stationary-input test: a
  sustained vowel should produce `T̂_l` with `|T̂_l| ≤ 0.5` after 3
  frames of steady-state. Divergent residuals → broken matched-
  decoder feedback.
- **§0.7 (V/UV).** Binary-valued output; DVSI comparison is exact
  per-band. Mismatches concentrated in the top band (k = K̂) may
  indicate a band-width edge case per §0.5.4. Mismatches in k = 1
  may indicate the `E(P̂_I) > 0.5` threshold of Eq. 37 being tripped
  on borderline input.
- **§0.8 (dispatch).** Silence-frame emission timing is
  DVSI-calibrated, not bit-exact. Compare frame-type sequences (not
  frame contents) over a full utterance; allow ±1 frame slack on
  silence boundaries.

### 0.11.5 Recommended Tolerances

| Quantity | Tolerance | Notes |
|----------|-----------|-------|
| Integer outputs (`b̂_0`, `b̂_1`, `L̂`, `K̂`, `v̂_k`) | bit-exact | Tier 1 floor; any mismatch is a real bug |
| `ω̂_0` | `|Δω̂_0| ≤ 0.001` rad/sample | Derived from bit-exact `b̂_0` |
| `M̂_l` (voiced) | `|Δ log₂ M̂| ≤ 0.05` | ≈ 3.5% relative |
| `M̂_l` (unvoiced) | `|Δ log₂ M̂| ≤ 0.2` | ≈ 14% relative; Eq. 44 has higher variance |
| `T̂_l` | `|ΔT̂| ≤ 0.1` in steady state | Cumulative error from §0.5 + §0.6 |
| `E(P)` | relative error ≤ 1e−6 | Pure floating-point; any reference match |
| `ξ_max` | relative error ≤ 1e−3 | Stateful; bias accumulates |
| Silence boundary | ±1 frame | Per §0.8.4 threshold calibration |

The tolerances assume `f64` throughout the reference C. `f32`
implementations may require 10× looser tolerances; this is a known
degradation, not a bug.

### 0.11.6 Debugging Workflow

When a Tier 1 mismatch appears:

1. Identify the **first** frame where the mismatch surfaces. A mid-
   utterance mismatch that isn't present in frame 0 usually indicates
   cross-frame state corruption (§0.6, §0.7 state, or §0.3 pitch
   history).
2. Walk the checkpoints C1 → C9 on the mismatched frame. The
   earliest diverging checkpoint identifies the subsection.
3. Check the relevant section's common-pitfalls list — the error is
   most often one of those.
4. If still stuck, compare against a second reference implementation
   (e.g. OP25's pitch tracker) to see whether the problem is a
   matched-decoder wiring issue (encoder is fine; reconstruction
   loop is broken) or a core algorithm issue (encoder is wrong).
5. Persistent mismatches in §0.7 or §0.5 should be cross-checked
   against the `γ_w` decoder investigation (commit `741eeef`) —
   shared window-energy normalization.

### 0.11.7 Known DVSI-vs-PDF Differences

The encoder's validation ceiling is bounded by behaviors that DVSI
exhibits but the PDF does not specify. These are **expected**
non-matches at the policy boundaries, not bugs:

- **Silence detection threshold** — §0.8.4. DVSI emits silence at
  approximately −45 dBFS with ~3-frame hysteresis; reproducing this
  exactly requires calibration, not specification.
- **Tone detection entry** — §0.8.5. Per gap report §7, DVSI tone
  detection is black-box. Blip25 should route tones around the
  analysis encoder rather than match DVSI here.
- **Preroll frame content** — §0.8.6. DVSI emits specific synthetic
  silence frames for frames 0 and 1; the bit pattern is
  implementation-defined.
- **Floating-point rounding** — C4 / C7 / §0.4.2. DVSI uses fixed-
  point arithmetic internally; blip25's `f64` reference will produce
  nominally different intermediate sums. For Tier 1 validation the
  tolerances in §0.11.5 absorb this; for Tier 2 bit-exact matching
  may require a bit-exact fixed-point port, which is out of scope
  for the initial analysis-encoder implementation.
- **`γ_w` window-energy scale** — cross-cuts §0.2 and §0.5. Commit
  `741eeef` flagged a ~150× decoder-side mismatch against DVSI;
  whether this originates in synthesis alone or also affects the
  encoder side is open. Expect up to 150× mismatches in raw `M̂_l`
  if the decoder-side `γ_w` bug has an encoder-side twin, until the
  investigation closes. Other intermediate quantities (`S_w(m)`,
  `T̂_l`) should be unaffected.

### 0.11.8 Deliverable: A Validation Harness

The concrete deliverable implied by this subsection:

```
tools/
  validate_analysis_encoder.py
    --dvsi-pcm  <path to input.raw>
    --dvsi-bits <path to output.bit>
    --rate {33, 34}
    [--checkpoint {C1..C9}]
    [--tolerance-tier {1, 2, 3}]
    [--max-mismatches N]
```

Per-frame diff output (JSON lines) lists the earliest-diverging
checkpoint and the delta magnitudes. A summary at end reports Tier
pass/fail per frame and aggregate stats.

The harness itself lives in `tools/`, not in the implementation
spec; this subsection only prescribes what it should validate.

### 0.11.9 Common Pitfalls

- **Do not validate against mbelib / OP25 / SDRTrunk.** Those are
  secondary references. They have documented bugs
  (`oss_implementations_lessons_learned.md`) that will drive the
  blip25 encoder toward the same bugs if used as validation targets.
- **Do not validate only on speech.** Silence, tones, and
  transients exercise different code paths. A speech-only suite
  misses the most likely regression sites.
- **Do not skip the matched-decoder feedback loop in validation.**
  An encoder that bypasses step 14 will look correct on frame 1 and
  slowly diverge. Running validation over short (< 5-frame)
  sequences will not catch this.
- **Do not treat Tier 2 as a prerequisite for Tier 1.** Tier 1
  (parameter equivalence) is the correctness target; Tier 2 (bit-
  exact bitstream) depends on correct wire-side quantization which
  is a separate concern. A blip25 release that passes Tier 1 but
  has known Tier 2 gaps is still deployable for receive-only use
  cases.
- **Save the test vectors.** DVSI eval-board access is not always
  available; once a frame-indexed {PCM → bits} set is captured, it
  should be committed to the blip25 test repository (subject to
  the project's copyright policy on test vectors, which differs
  from the PDF policy — recordings of specific inputs are generally
  permissible).

