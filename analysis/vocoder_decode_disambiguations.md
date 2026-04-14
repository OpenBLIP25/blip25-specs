# BABA-A Vocoder Decode Pipeline: Disambiguations

**Scope:** clarifications and interpretations needed to implement the full-rate
IMBE decode pipeline (and, where noted, half-rate AMBE) from TIA-102.BABA-A.
These are *not* new requirements — every item below is derivable from the PDF
if you read the right equation carefully, but the impl spec (in-tree) glosses
or deferred several of these, and multiple implementations have been observed
to pick the wrong branch of each ambiguity.

This document is intended to survive any re-run of the Phase 1-3 extraction
pipeline, which would regenerate the impl spec from the PDF and might not
preserve inline clarifications.

Organized by pipeline stage, with direct equation citations into the BABA-A PDF.

---

## 1. Inverse Uniform Quantizer — Midtread, not Midrise

**Source:** BABA-A §6.4.1 Eq. 68 (gain) and §6.4.2 Eq. 71 (HOC), pages 29–30.

```
value = 0                                      if B̃_m = 0
value = Δ̃_m · (b̃_m − 2^{B̃_m−1} + 0.5)         otherwise
```

The **`+0.5` offset** makes this a midtread quantizer: the reconstruction
points are at `Δ̃_m · (−2^{B̃_m−1} + 0.5), …, Δ̃_m · (2^{B̃_m−1} − 0.5)`. A
midrise quantizer (reconstruction at `Δ · (k − 2^{B−1})` without the 0.5)
would be off by half a step — audibly equivalent to a fixed DC bias on the
spectral envelope.

**Why implementations get this wrong:** without reading §6.4 directly, the
standard textbook form is midrise (`Δ · (k − 2^{B−1})`). The `+0.5` is
a BABA-A-specific choice, not a convention.

**Applies to:** both Annex F (gain DCT coefficients G̃_2..G̃_6) and Annex G
(HOC coefficients C̃_{i,k}).

---

## 2. Inverse DCT is Per-Block, not Fixed 6-Point

**Source:** BABA-A §6.4.1 Eq. 69 (gain → residual) and §6.4.2 Eq. 73 (HOC →
per-block residual), pages 29–30.

```
R̃_i = Σ_{m=1}^{6} α(m) · G̃_m · cos[ π · (m−1) · (i − 0.5) / J̃_i ]   (Eq. 69)
c̃_{i,j} = Σ_{k=1}^{J̃_i} α(k) · C̃_{i,k} · cos[ π · (k−1) · (j − 0.5) / J̃_i ]   (Eq. 73)
```

The cosine denominator is **J̃_i (the block length)**, not a fixed 6 or any
other constant. This means:

- The inverse DCT **cannot be precomputed as a single fixed-size transform**.
  The basis frequency changes with block length.
- Each of the 6 blocks may have a different J̃_i (from Annex J / §12.5),
  ranging from ⌊L̃/6⌋ to ⌈L̃/6⌉. You need 6 potentially-different DCTs.
- Using a single 6×6 DCT for R̃_i (as might be inferred from "6-point inverse
  DCT on the gain vector") produces the wrong coefficients for every block
  except the unlikely case where all J̃_i = 6.

**Why implementations get this wrong:** the impl spec §6.4 originally said
"HOCs are encoded using block-level DCT" without citing Eq. 69/73. The word
"block-level" is easy to miss — and the "6-point DCT" language in analysis
notes elsewhere refers to the *encoder's* forward transform of the 6-element
gain vector, which *is* a fixed 6-point DCT. The inverse path at the decoder
is structurally different.

---

## 3. ρ = 0.65 (Prediction Gain)

**Source:** BABA-A §6.4 Eq. 77 body text and §13.3 (half-rate equivalent),
pages 30 and 60–61. Not a named constant anywhere — embedded as a literal
`0.65` in multiple equations.

The value ρ = 0.65 is the prediction gain in the inverse log-magnitude
prediction step:

```
log₂ M̃_l(0) = T̃_l + ρ · (1 − δ̃_l) · log₂ M̃_{⌊k̃_l⌋}(−1)
             + ρ · δ̃_l         · log₂ M̃_{⌊k̃_l⌋+1}(−1)
             − (ρ / L̃(0)) · (DC-removal sum over previous frame)
```

**Why implementations get this wrong:** `ρ` is used without a symbolic name
in the PDF — it's just `0.65` inline. An extractor that summarizes §6.4 may
describe the prediction as "scaled by a gain factor" without preserving the
literal value, leaving the implementer to guess. Both full-rate (Eq. 77) and
half-rate (Eq. 200) use the same value.

---

## 4. b̂_{L̂+2} is a Frame Sync Bit, Not a Spectral Amplitude

**Source:** BABA-A §6.5 Eq. 80, page 31.

The "L̃+2"-th quantizer value that shows up in the §7.1 bit prioritization
placement (it occupies û₇[0] via the final fixed slot) is **not** part of
the reconstruction pipeline. It's a 1-bit alternating synchronization marker:

```
b̂_{L̂+2}(0) = 0  if b̂_{L̂+2}(−1) = 1
b̂_{L̂+2}(0) = 1  otherwise                      (Eq. 80)
```

Initial value: 0. Decoders MAY use this for frame-boundary detection when no
external sync is available (e.g., stand-alone IMBE bitstream), or MAY ignore
it when the air-interface frame format already provides sync (the typical
P25 FDMA case — frame sync comes from BAAA-B at 48 bits per frame, which is
much stronger than a 1-bit alternation).

**Why implementations get this wrong:** the symbol `b̂_{L̂+2}` looks like it
belongs in the sequence of spectral quantizer values `b̂₀..b̂_{L̂+2}` and gets
passed to the reconstruction logic alongside `b̂₂..b̂_{L̂+1}`. Reading §6.4
alone without §6.5 leaves the role of the final bit undefined.

---

## 5. HOC Step Size = Table 3 × Table 4, Not Just Table 3

**Source:** BABA-A §6.3.2 Tables 3 and 4, page 28.

For higher-order DCT coefficients, the inverse quantizer step size is:

```
Δ̃_m = (Table 3)[B̃_m] · (Table 4)[k]
```

Where `k` is the DCT coefficient index within the block (2 ≤ k ≤ 10).
Table 3 gives step by bit count (10 rows), Table 4 gives per-coefficient
standard deviation σ (9 rows for k = 2..10). Both tables are now inlined
in §1.8.2 of the impl spec.

Example (from the PDF text): 4 bits allocated at k=3 →
`Δ̃ = 0.40 × 0.241 = 0.0964`.

**Why implementations get this wrong:** Annex G in the PDF (and our
`annex_g_hoc_allocation.csv`) only gives B_m, not Δ_m. Implementers may
assume a fixed step per bit count (Table 3 alone) and get the HOC
reconstruction ~4× wrong at high-k coefficients. Table 4 isn't in any
annex — it's embedded inline in §6.3.2.

---

## 6. Full_Text K Formula Is Wrong (Phase 2 Extraction Bug)

**Source:** observed during Phase 4 reading of BABA-A §4.1 and §5.2, Eq. 34.

`standards/TIA-102.BABA-A/TIA-102-BABA-A_Full_Text.md` §4.1 paraphrases Eq. 34
as:

```
K̂ = floor(2 · ω̂₀ / π × 56), K̂_max = 12       WRONG — Phase 2 extraction error
```

The **correct** Eq. 34 is identical to Eq. 48 in the impl spec:

```
K̂ = floor((L̂ + 2) / 3)   if L̂ ≤ 36
K̂ = 12                    otherwise
```

At L̂ = 9: the wrong formula gives K̂ = 11, the correct formula gives K̂ = 3.
Order-of-magnitude error — an implementer trusting the Full_Text would try
to decode 11 V/UV bands out of a K̂-bit field that only exists as 3 bits
post-prioritization (by Eq. 34 and §7.1).

**Status:** the gitignored Full_Text.md has been locally corrected. If it
ever regenerates, this bug will likely reappear; the impl spec §1.3.1/§1.3.2
cite the correct formula.

---

## 7. V/UV Band-to-Harmonic Mapping (Both Rates)

**Source:** BABA-A §5.2 Eq. 32–33 (band boundaries) and §13.2 Eq. 147–149
(half-rate codebook expansion).

### Full-rate

```
k_l = floor((l + 2) / 3)   if l ≤ 36
k_l = 12                    otherwise
ṽ_l = v̂_{k_l}             for 1 ≤ l ≤ L̃
```

Bands 1..K̃−1 each cover exactly 3 harmonics; band K̃ (the highest) absorbs
the remainder.

### Half-rate

```
j_l = floor(l · 16 · ω̃₀ / (2π)),   clamped to [0, 7]    (Eq. 147)
ṽ_l = v_{j_l}(n)   where n = b̃₁ and v(n) is the Annex M row     (Eq. 149)
```

The pdftotext rendering of Eq. 147 drops the `l` multiplicand — the correct
form is the only dimensionally-consistent interpretation (verified: at
ω̃₀ = 2π/19.875 rad/sample, j_1 = 0 and j_9 = 7, covering the full 8-element
codebook).

**Why implementations get this wrong:** the §4.1 prose says "For each band
k = 1 to K̂..." without specifying which harmonics belong to which band.
§5.2 Eq. 32–33 (with `â_{3k−2} ≤ m < b̂_{3k}`) gives the boundary but the
phrase "width = 3 ω̂₀" is easy to reduce to "3 harmonics per band" without
noticing the highest-band remainder clause.

---

## 8. Cross-Frame Decoder State

The spectral amplitude reconstruction at Eq. 77 depends on the previous
frame's `M̃_l(−1)` and `L̃(−1)`. This is cross-frame stateful decoding;
it interacts non-trivially with frame mute / frame repeat from §9 of the
impl spec:

- **Mute:** Preserve previous-frame `M̃_l(−1)` — do not zero it. The next
  valid frame uses it for prediction.
- **Repeat:** The repeated M̃_l becomes `M̃_l(−1)` for the frame after.
- **Cold start:** Per §10 Annex A, `M̃_l(−1) = 1 for all l`, `L̃(−1) = 30`.
  With ρ = 0.65, the uniform initial state creates a constant bias that
  decays over ≈ 3–5 frames to steady state.

**Why implementations get this wrong:** a stateless decoder (e.g., one that
resets internal buffers each frame for safety) will produce silence or
garbage until steady-state convergence, even on clean input.

---

## 9. Forward/Inverse DCT Are Asymmetric (Not Orthonormal)

**Source:** BABA-A §6.3.1 Eq. 60–61 (encoder forward DCT) and §6.4.1
Eq. 69 + §6.4.2 Eq. 73 (decoder inverse DCT), pages 25, 29, 30.

BABA-A's forward DCT (both the 6-point gain transform and the per-block
J̃_i-point transform) uses a **uniform `1/N` factor on every coefficient**
with no α-weighting:

```
G_m = (1/6) Σ_{i=1}^{6} R_i · cos[ π·(m−1)·(i − 0.5) / 6 ]           (Eq. 61)
C_{i,k} = (1/J_i) Σ_{j=1}^{J_i} c_{i,j} · cos[ π·(k−1)·(j − 0.5) / J_i ]  (Eq. 60)
```

The inverse DCT uses α(k) = {1 if k = 1, 2 otherwise} with **no `1/N`**:

```
R̃_i = Σ_{m=1}^{6} α(m) · G̃_m · cos[ π·(m−1)·(i − 0.5) / J̃_i ]     (Eq. 69)
c̃_{i,j} = Σ_{k=1}^{J̃_i} α(k) · C̃_{i,k} · cos[ π·(k−1)·(j − 0.5) / J̃_i ]  (Eq. 73)
```

This is an **asymmetric DCT-II / DCT-III pair**, not the textbook
orthonormal DCT-II where both forward and inverse carry `√(1/N)` (k=0)
and `√(2/N)` (k>0) symmetrically. It is invertible — the `α` weighting
on the inverse compensates for the missing factor — but only when
forward and inverse match BABA-A's specific convention.

**Why implementations get this wrong:** writing a test that round-trips
a known-good signal through forward and inverse DCT is standard practice.
If the forward uses orthonormal scaling (`√(1/N)` for k=0, `√(2/N)` for
k > 0) and the inverse uses BABA-A's α weighting (1, 2, 2, …), the round
trip drifts by a factor of `√N`. The symptom is that test vectors with
large L or large J̃_i show progressively larger discrepancies — easy to
mistake for "just numerical noise" until you compare against a known
BABA-A reference output.

**Fix:** match the spec exactly — forward uses uniform `1/N`, inverse
uses α(k) weighting with no normalization. Reference implementations:

```c
/* Eq. 60/61 — forward DCT (encoder side) */
static void imbe_forward_dct(const float *in, float *out, int N) {
    const double inv_N = 1.0 / (double)N;
    for (int k = 0; k < N; k++) {
        double acc = 0.0;
        for (int j = 0; j < N; j++) {
            acc += in[j] * cos(M_PI * (double)k * ((double)j + 0.5) / (double)N);
        }
        out[k] = (float)(acc * inv_N);
    }
}

/* Eq. 69/73 — inverse DCT (decoder side) */
static void imbe_inverse_dct(const float *in, float *out, int N) {
    for (int i = 0; i < N; i++) {
        double acc = in[0];   /* α(0) = 1 */
        for (int k = 1; k < N; k++) {
            acc += 2.0 * in[k] * cos(M_PI * (double)k * ((double)i + 0.5) / (double)N);
            /* α(k) = 2 for k > 0 */
        }
        out[i] = (float)acc;
    }
}
```

Verified round-trip identity for n ∈ {1, 2, 3, 5, 6, 7, 10}: forward→inverse
reproduces the input exactly modulo IEEE-754 rounding. Observed during
user-side downstream implementation work on 2026-04-14; a first-cut
implementation with orthonormal scaling failed the round-trip test, and
switching to this convention made it pass.

---

## Provenance

Every disambiguation above is traceable to a specific PDF equation number or
table. When the impl spec is regenerated, this file should be used as a
checklist: each item either gets inlined back into the spec, or remains here
as a pointer to the correct PDF location with the rationale preserved.

If a future Phase 4 run catches additional ambiguities or real spec bugs
related to the decode pipeline, they should be appended here rather than
only captured in commit messages (which are harder to grep than a dedicated
analysis file).
