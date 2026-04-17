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

## 2. Gain DCT is Fixed 6-Point; HOC DCT is Per-Block (J̃_i)

**Source:** BABA-A §6.4.1 Eq. 69 (gain → residual, page 30) and §6.4.2
Eq. 73 (HOC → per-block residual, page 30).

**Correction (2026-04-16):** this section previously asserted that
**both** Eq. 69 and Eq. 73 use a per-block `J̃_i` denominator. Reading
the PDF directly and cross-checking against Eq. 61 reveals that Eq. 69
as printed in the PDF is an **editorial error** — the gain DCT is a
fixed 6-point transform, not per-block. Only Eq. 73 is genuinely
per-block.

### The two equations

**Eq. 69 (gain inverse DCT, 6-element ↔ 6-element):**
```
                                                     (as printed in BABA-A page 30)
R̃_i = Σ_{m=1}^{6} α(m) · G̃_m · cos[ π · (m−1) · (i − 0.5) / J̃_i ]   (Eq. 69)
                                                     — spec bug; use `6`
R̃_i = Σ_{m=1}^{6} α(m) · G̃_m · cos[ π · (m−1) · (i − 0.5) / 6 ]     (Eq. 69, corrected)
```

**Eq. 73 (HOC inverse DCT, 6-element ↔ `J̃_i`-element, per-block):**
```
c̃_{i,j} = Σ_{k=1}^{J̃_i} α(k) · C̃_{i,k} · cos[ π · (k−1) · (j − 0.5) / J̃_i ]   (Eq. 73)
```

### Why Eq. 69 as printed is an editorial error

1. **Forward/inverse DCT must have matching denominators.** Eq. 61
   (encoder forward DCT, page 27) has denominator `6`:
   ```
   Ĝ_m = (1/6) Σ_{i=1}^{6} R̂_i · cos[ π(m−1)(i − 1/2) / 6 ]
   ```
   A forward DCT with period `2N = 12` and an inverse DCT with period
   `2·J̃_i` are not a DCT pair unless `J̃_i = 6` — which is true only at
   `L̃ = 36`.

2. **Both operands are 6-element vectors.** `Ĝ_m` for `1 ≤ m ≤ 6` and
   `R̂_i` for `1 ≤ i ≤ 6` (Eq. 60, 61, 67). The DCT basis length is 6,
   fixed. `J̃_i` varies with `i` and changes each frame via Annex J;
   substituting it into the basis makes the cosine frequency depend on
   which output index is being computed, which is not any standard DCT.

3. **PDF text preceding Eq. 69 says "an inverse DCT of `G̃_m`"** —
   phrasing consistent with a single fixed transform. If the intent
   were per-block, the text would say "an inverse DCT per block" as it
   does in §6.4.2 for Eq. 73.

4. **Round-trip identity fails under the literal reading.** `R̂ → Ĝ`
   via Eq. 61 → `R̃` via Eq. 69 (with `J̃_i`) does not give `R̃ = R̂`
   for any `L̃ ≠ 36`. Under the corrected reading (`6` denominator),
   the round trip is exact (up to IEEE-754 rounding).

5. **Every working reference implementation uses `6`.** JMBE, mbelib,
   OP25, SDRTrunk all implement the gain inverse DCT as a fixed 6-point
   transform. Interoperable P25 voice has been decoded for over a decade
   on the basis of `6`, not `J̃_i`.

The most likely cause: BABA-A editors copied Eq. 73's `J̃_i` denominator
into Eq. 69 when revising the document. Eq. 73's `J̃_i` **is** correct
(each block's HOCs form a `J̃_i`-element vector, so the per-block inverse
DCT is genuinely `J̃_i`-point).

### Implementation guidance

- **Eq. 69 (gain):** use denominator `6`. Implement as a single fixed
  6-point inverse DCT, precomputable as a 6×6 matrix.
- **Eq. 73 (HOC):** use denominator `J̃_i`. Implement as a per-block
  inverse DCT, with `J̃_i` varying across the 6 blocks per Annex J.

The two transforms are **structurally different** and share only the
α-weighting and the asymmetric-forward/inverse convention discussed in
§9.

### Invariant that catches the bug

Forward-inverse round-trip identity of the gain DCT. Take arbitrary
`R̂_1..R̂_6`, run Eq. 61 forward to get `Ĝ_1..Ĝ_6`, run Eq. 69 inverse
to recover `R̃_1..R̃_6`. Under the literal reading (`J̃_i`) this fails
for every `L̃` with non-uniform `J̃_i` (i.e., every `L̃ ≠ 36`). Under
the corrected reading (`6`) it passes modulo IEEE-754 rounding. This
is the cheapest unit test for the gain DCT pair.

---

## 3. ρ Prediction Gain — full-rate is L-dependent, half-rate is literal 0.65

**Source:** BABA-A §6.3 Eq. 55 (full-rate ρ schedule, page 25),
Eq. 77 (full-rate decoder, page 31), §13.3 Eq. 155 (half-rate encoder,
page 60), §13.4 Eq. 185 (half-rate decoder, page 65).

**Correction (2026-04-15, updated 2026-04-16):** this section
previously asserted "ρ = 0.65 ... embedded as a literal `0.65` in
multiple equations" and that "both full-rate (Eq. 77) and half-rate
(Eq. 200) use the same value." Both parts are wrong:
1. The two rates use different conventions (full-rate L̂-dependent,
   half-rate literal 0.65) — rest of this section corrects that.
2. The half-rate decoder predictor is **Eq. 185, not Eq. 200**.
   Eq. 200 is actually the first of the Eq. 200–205 frame-repeat
   copy-forward block (BABA-A §14.6). PDF verified 2026-04-16.

- **Full-rate (Eq. 77 decoder, Eq. 54 encoder):** uses the symbol
  `ρ`, defined in Eq. 55 as a piecewise-linear function of `L̂(0)`:

  ```
  ρ = 0.4                           if L̂(0) ≤ 15
  ρ = 0.03 · L̂(0) − 0.05            if 15 < L̂(0) ≤ 24
  ρ = 0.7                           otherwise                (Eq. 55)
  ```

  Table of values:

  | `L̂(0)` | `ρ`  | `L̂(0)` | `ρ`  |
  |:-----:|:----:|:------:|:----:|
  | 9     | 0.40 | 22     | 0.61 |
  | 15    | 0.40 | 24     | 0.67 |
  | 16    | 0.43 | 25     | 0.70 |
  | 20    | 0.55 | 56     | 0.70 |

  Eq. 77 uses `ρ` unqualified — the reader must follow the chain back
  to Eq. 55. The full-rate `ρ` is **not** the literal 0.65 and **not**
  constant.

- **Half-rate (Eq. 155 encoder page 60, Eq. 185 decoder page 65):**
  uses the literal constant `0.65` directly, with no `ρ` symbol and no
  Eq. 55 analog. Half-rate is genuinely "just 0.65 inline".

The corrected full-rate form of the inverse log-magnitude prediction:

```
log₂ M̃_l(0) = T̃_l + ρ · (1 − δ̃_l) · log₂ M̃_{⌊k̃_l⌋}(−1)
             + ρ · δ̃_l         · log₂ M̃_{⌊k̃_l⌋+1}(−1)
             − (ρ / L̃(0)) · (DC-removal sum over previous frame)
             where ρ is given by Eq. 55 above (not 0.65).
```

**Why implementations (including this note until now) get this wrong:**
`ρ` is introduced in Eq. 55 on full-rate page 25 but re-used on page
31 in Eq. 77 without restating. A reader who jumps to Eq. 77 (e.g. to
extract decoder math) sees "ρ" as an opaque symbol. Pattern-matching
the half-rate literal 0.65 onto full-rate is an easy mistake — and
harmless for `L̂ ≈ 22–23` where Eq. 55 happens to yield values near
0.65. The error surfaces only at the tails (`L̂ ≤ 15` → true ρ = 0.40,
`L̂ > 24` → true ρ = 0.70), where implementations that hardcode 0.65
will mispredict by 40% or more.

**Sweep status (updated 2026-04-17):**

- `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §1.8.5
  has been updated with the Eq. 55 piecewise schedule and a
  `fullrate_rho()` reference helper; the stale "ρ = 0.65 everywhere"
  phrasing has been replaced. Committed upstream as part of
  `7e35238` (2026-04-15).
- `analysis/vocoder_analysis_encoder_addendum.md` §0.6.2 (encoder-side
  predictor) renders the same schedule with a correction header
  flagging the original mistake.
- `analysis/vocoder_decode_disambiguations.md` §8 (this file) had one
  remaining stale reference in its cold-start commentary; swept in
  the 2026-04-17 ρ-correction pass.
- Full-rate blip25-mbe implementations that derived `ρ` from this
  note's earlier version should rerun against DVSI full-rate
  vectors (AMBE-3000 rate 33; see
  [`dvsi_test_vector_modes.md`](./dvsi_test_vector_modes.md)) to
  confirm the fix. Error magnitude is bounded: tails (`L̂ ≤ 15` or
  `L̂ > 24`) see `|Δρ| ≤ 0.25`; the `L̂ ≈ 22–23` band sees no change
  because Eq. 55 happens to yield ≈ 0.65 there.
- Half-rate implementations are unaffected — Eq. 155 / Eq. 185's
  literal 0.65 is correct for half-rate.

For the encoder-side treatment of the same prediction residual
(full-rate Eq. 54), see
[`vocoder_analysis_encoder_addendum.md`](./vocoder_analysis_encoder_addendum.md)
§0.6.

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
  In log₂ units, `log₂ 1 = 0`, so the first-frame predictor contribution
  is exactly zero regardless of `ρ` (full-rate Eq. 55 piecewise or
  half-rate literal 0.65). Residual bias from the all-ones state decays
  over ≈ 3–5 frames to steady state as `M̃_l(−1)` rolls forward through
  real reconstructed values.

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
R̃_i = Σ_{m=1}^{6} α(m) · G̃_m · cos[ π·(m−1)·(i − 0.5) / 6 ]       (Eq. 69, corrected; see §2)
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

## 10. PN Mask Alignment Is Transmission-Order, Not Storage-Order

**Source:** BABA-A §7.4 Bit Modulation, Eq. 86–93, page 38, combined with
§7.3 row-vector convention ("leftmost bit is MSB").

**Caught by:** downstream DVSI vector comparison (2026-04-14). Not catchable
by self-consistent round-trip testing — see "Why implementations get this
wrong" below.

The modulation vectors in Eq. 87–92 are written as ordered bracket lists:

```
m̂_1 = [⌊p_r(1) / 32768⌋, ⌊p_r(2) / 32768⌋, …, ⌊p_r(23) / 32768⌋]
```

Per §7.3's row-vector convention, **element 0 of the bracket list is the MSB
(first-transmitted bit) of the corresponding code vector v̂_i**. That is:

```
m̂_i[0]   maps to v̂_i bit (len_i − 1)   = first transmitted
m̂_i[1]   maps to v̂_i bit (len_i − 2)
...
m̂_i[len−1] maps to v̂_i bit 0            = last transmitted
```

So if the mask is stored in a `uint32_t` with bit (len−1) = first-transmitted
(the natural convention for deinterleaver output), then:

```
m̂_i stored = (msb(p_r(start))   << (len−1))
           | (msb(p_r(start+1)) << (len−2))
           | ...
           | (msb(p_r(start+len−1)) << 0)
```

which is the **reverse** of the "element k at bit k" packing shown in the
reference C code in impl spec §1.6.

### The §1.6 Reference Code's Convention

The C code in `P25_Vocoder_Implementation_Spec.md` §1.6
(`imbe_modulation_vectors`) packs masks with `bits |= mask_bit << k`
(LSB-first). That produces a storage layout where p_r(start) sits at bit 0,
not at bit (len−1). To use this mask against a codeword that has bit
(len−1) = first-transmitted, the **mask must be bit-reversed within its
len_i-bit window** before XOR — or the packing must be rewritten
transmission-order.

Both orientations are valid *internal representations*; what matters is
that encode and decode agree. When the decoder's deinterleaver places the
first-transmitted bit at bit (len−1) of the stored codeword (the standard
P25 FDMA convention, matching Annex H's "bit 1 = MSB" column), then the
mask must be packed in transmission order, not LSB-first.

### Why Implementations Get This Wrong

A self-consistent encode→decode round-trip passes regardless of which
convention you pick — both sides XOR with the same bits, so the masks
cancel. The two orientations become distinguishable only when comparing
against an **external reference** (DVSI chip, real OTA capture, or any
implementation that independently follows the wire convention).

Symptom observed during DVSI vector comparison:
- `û₀` and `û₇` match (no PN modulation applied — m̂₀ = m̂₇ = 0).
- `û₁..û₆` mismatch.
- `c̃ ⊕ Golay(û_expected)` reconstructs a bit-reversed-within-vector-width
  copy of the (wrongly-oriented) mask. The reversed pattern is a dead
  giveaway and makes the fix unambiguous.

### Fix

Either:

(a) Keep the §1.6 LSB-first packing in memory, but reverse the mask bits
    within their len_i-bit window at the XOR site. Requires awareness at
    every consumer of the mask.

(b) Repack masks transmission-order, so `mv[i].bits` already aligns with
    the codeword's bit (len−1)-to-bit-0 order. XOR is a direct operation
    with no reversal. Preferred — less chance of convention drift.

Recommended implementation (option b):

```c
/* Eq. 87–92 with transmission-order packing: m̂_i[0] at bit (len−1),
 * m̂_i[len−1] at bit 0.  XOR directly against the stored codeword. */
static uint32_t pn_mask_range_txorder(const uint16_t *pr, int start, int len) {
    uint32_t bits = 0;
    for (int k = 0; k < len; k++)
        bits |= ((uint32_t)imbe_pn_mask_bit(pr[start + k])) << (len - 1 - k);
    return bits;
}
```

This is the convention DVSI follows, verified by bitstream comparison
against the AMBE-3000 chip output.

---

## 11. γ_w Empirical Mismatch Against DVSI (Investigation Pending)

**Source of the spec value:** BABA-A §11.2 Eq. 121, page 52.

**Empirical finding:** downstream implementer (2026-04-14) reported that
direct use of the spec-computed γ_w = 146.643269 produces unvoiced output
approximately 150× louder than DVSI's reference PCM for the same bitstream.
The error scales monotonically with γ_w; the empirical optimum sits near
γ_w ≈ 1.0. Sweep on the `alert.bit` test case:

| γ_w    | RMS error | SNR       |
|-------:|----------:|----------:|
| 0.5    | 3587      | −0.36 dB  |
| 1.0    | 3587      | −0.36 dB  |
| 5.0    | 3593      | −0.38 dB  |
| 10.0   | 3613      | −0.43 dB  |
| 50.0   | 4230      | −1.80 dB  |
| **146.6** | **6724** | **−5.82 dB** (spec value) |

The spec is committed as authoritative. This entry documents the
mismatch so the investigation below doesn't get lost.

### Candidate Causes

1. **Annex E quantizer log base.** The spec uses `log₂(M̃_l(0))` in Eq. 77,
   so `M̃_l = 2^{log₂ M̃_l(0)}`. Annex E values span [−2.842, +8.696];
   interpreted as log₂ they produce M̃_l in roughly [0.14, 414]. If DVSI
   instead treats Annex E as natural log or log₁₀, our M̃_l would be off
   by a constant factor (log₂ vs log₁₀ differs by ~3.32×; log₂ vs ln
   differs by ~1.44×). Neither gets to the observed 150× factor alone —
   but combined with §1.10's γ rescale or a different scale in Eq. 75–77
   it could.

2. **Global M̃_l normalization missing in our dequant.** If DVSI's encoder
   applies a constant pre-scaling before quantizing (say, dividing by a
   window-energy term) and the decoder restores it inside the synthesis
   pipeline, our literal reading of Annex E/Eq. 77 would produce M̃_l
   scaled up by that factor. γ_w then over-corrects by the same factor.
   Σ wR(n) = 110.02 is suspiciously close to 150; this is the prime
   candidate.

3. **Unvoiced norm formula** (Eq. 120's `[Σ|U_w|² / (⌈b̃⌉−⌈ã⌉)]^(1/2)`
   denominator). If our interpretation of "band range" off-by-ones vs
   DVSI's, the norm changes by a factor that γ_w then multiplies.

4. **Different spec revision.** BABA-A 2014 differs from the 1993 IMBE
   spec mbelib follows. If DVSI's AMBE-3000 is actually calibrated to
   the 1993 value of γ_w (which we haven't located in a 1993 reference),
   the spec-literal 2014 value may have been edited without a matching
   calibration update elsewhere.

5. **Window normalization convention.** Σ wR(n) = 110.02, the first
   factor in γ_w. Some signal-processing conventions normalize windows
   so Σ w = 1 (unit DC gain) or Σ w = N (unit-amplitude passband). Our
   wR has Σ = 110.02 because the values tabulate as raw coefficients.
   If Eq. 121's derivation assumed Σ wR = 1 (normalized), γ_w should
   be roughly Σ wR × smaller = ~1.33.

### How to Investigate

The minimum useful diagnostic is a **frame-level side-by-side of M̃_l
values** between our implementation and DVSI for the same input bits:

```
1. Feed the same raw IMBE bits to both pipelines.
2. At the MBE-parameter boundary (after §1.8.5 log-mag prediction,
   before enhancement §1.10), compare M̃_l(0) for each harmonic.
3. If our M̃_l is ~150× DVSI's: bug is in dequant (candidates 1/2).
4. If M̃_l matches but post-enhancement M̄_l is ~150×: bug in §1.10.
5. If M̄_l matches but synthesis output is ~150×: bug in γ_w formula
   or unvoiced norm (candidates 3/5).
```

DVSI's AMBE-3000 exposes intermediate MBE parameters via its protocol
(see `DVSI/AMBE-3000_Protocol_Spec.md` per your DVSI directory). A
one-frame capture at each pipeline checkpoint would localize the
factor.

### Recommended Action

- **Do not fit γ_w locally.** The mismatch may be compensating for a
  bug elsewhere (e.g., wrong Annex E log base), and trimming γ_w
  hides the root cause. If you need correct PCM today for demo
  purposes, gate the override behind a debug flag with a visible
  warning, not a silent constant.
- **Keep the spec value as default** until the root cause is found.
- **Update this entry** with findings once the M̃_l comparison runs.

### 2026-04-14 Update — Amplitude and Structural Decomposition

Downstream investigation (2026-04-14) ran a 2-D parameter sweep over
`(γ_w, m_scale)` — γ_w is the Eq. 121 unvoiced scaling coefficient;
`m_scale` is a hypothesized global spectral-amplitude pre-scale that
would be consistent with candidate #2 above. The finding splits the
original mismatch into two structurally independent problems:

**1. M̃_l is on the wrong scale (confirmed).** With γ_w = 0 (unvoiced
path silenced, isolating the voiced pathway) and m_scale = 1 on clean
input: our output RMS is 2289 vs DVSI's 1355 — approximately **1.7×
high**. That factor is *compatible with* candidate #2's Σ wR ≈ 110
story **if DVSI's M̃_l is ~1/100× ours, not ~1/150× as originally
hypothesized.** Applying a global `m_scale < 1` pre-scale eliminates
the clipping and reduces RMS monotonically toward DVSI's RMS floor.
The 150× estimate from the original α/alert.bit sweep likely
conflated unvoiced-path γ_w error with voiced-path M̃_l scale error.

**2. The voiced waveform is structurally different (new finding).**
No `(γ_w, m_scale)` pair anywhere in the sweep produces positive SNR
against DVSI reference PCM. At small `m_scale` our output collapses
to silence; at the amplitude that matches DVSI's RMS the waveform
still doesn't align. **Amplitude calibration alone cannot close the
gap.** Something in the voiced synthesis path — §1.12.2 Eq. 127–141 —
produces waveforms that differ from DVSI's in shape, not just scale.
Candidates localized to that path:
- **Phase tracking** (Eq. 136–141): ψ_l / φ_l accumulators, especially
  the `l·N/2` quadratic term and the Δφ_l(0) wrap in Eq. 138.
- **OLA seam between frames** (Eq. 131–134 branch selection): the
  four-way branch on voiced/unvoiced × current/previous × pitch-delta
  thresholds (`l ≥ 8` and `|Δω₀| ≥ 0.1 ω₀` in Eq. 133 vs Eq. 134).
- **Cross-frame state initialization** (§1.13): ψ_l(−1), ω̃₀(−1), and
  M̄_l(−1) values at decoder cold-start, or mis-updated state across
  erasure/silence/tone frames.

### 2026-04-14 Reprioritization

The candidates in the original "How to Investigate" section assumed a
single multiplicative factor — that framing is superseded. Revised
priority:

1. **Localize the voiced structural difference first.** Amplitude
   calibration is downstream of a working waveform; fitting M̃_l
   scale against a misaligned waveform just chases noise. Cleanest
   diagnostic: single-frame cross-correlation of our `s̃_v(n)` vs
   DVSI reference PCM for a frame with **a single dominant voiced
   harmonic** (minimal interaction between harmonics). Outcomes:
   - High correlation magnitude + non-zero lag → **phase-tracking
     bug** (Eq. 136–141 pathway).
   - Low correlation at all lags → **window or cross-frame state
     bug** (Eq. 131–134 branch selection, or §1.13 state init).
   - High correlation at zero lag with amplitude mismatch → isolated
     M̃_l scale problem; return to candidate #2 of the original list.
2. **Once the voiced pathway correlates, re-fit M̃_l scale.** The
   ~1.7× factor observed at `(γ_w = 0, m_scale = 1)` is a useful
   starting point but only meaningful once the waveform shape agrees.
3. **Defer γ_w re-examination** until voiced synthesis is
   bit-correlated. The unvoiced path runs through a separate
   Eq. 117–126 pipeline and its scale coefficient cannot be
   calibrated in isolation from the voiced path when both contribute
   to the summed output.

### Implication for Dependent Work

Half-rate HOC placement (§12 of this file) was resolved against this
same "uncalibrated full-rate" baseline. The resolution holds for the
algebraic-consistency reason stated there, but any future
bit-exactness claim on either rate is gated on the voiced-synthesis
diagnostic above landing first.

### 2026-04-14 Update — Cross-Correlation Diagnostic Landed

The single-frame voiced `s̃_v(n)` vs DVSI cross-correlation harness
called for in the reprioritization above was built and run (commit
`6ba6dea`, "Add xcorr-frame diagnostic for §11 voiced-synthesis
investigation"). Results on multiple voiced-heavy frames from DVSI
clean:

| Frame | voiced / L̃ |   ω̃₀  | Peak corr | Lag | Interpretation                                          |
|------:|-----------:|------:|----------:|----:|---------------------------------------------------------|
|   100 | 6 / 19     | 0.150 |    −0.72  |   4 | 180° + 34° shift at fundamental                         |
|   500 | 22 / 22    | 0.130 |    −0.45  |   8 | 180° + 60° shift; zero-lag +0.39 (partial alignment)    |
|  1500 | 9 / 20     | 0.144 |    −0.41  |  11 | 180° + 88° shift                                        |

**Signature:** peak correlations are **high magnitude, negative, at
small non-zero lags** across all voiced-heavy frames sampled. That
combination is the classic fingerprint of a **~180° phase offset**
(cos ↔ −cos, or equivalently sin ↔ cos mismatch — one sign inversion
in the voiced basis) plus a small fractional-sample shift on top. Low
correlation at all lags (the "window / state bug" branch of the
previous reprioritization) is ruled out. High zero-lag correlation
with amplitude mismatch is also ruled out.

Frame 500's zero-lag `+0.39` alongside the peak `−0.45` at lag 8
indicates **partial alignment with phase inversion dominating** — the
waveform shape is close to right, but half-cycle-flipped, so summing
many harmonics gives a shape that neither matches at lag 0 nor at a
simple sample-shift. That rules out a pure delay (which would show
high positive correlation at one specific lag) and rules in a
**systematic phase-sign error** in the voiced pathway.

### Narrowed Candidate List for the Voiced Sign Error

Ranked by consistency with the "negative peak, small lag" signature:

1. **Eq. 131 `cos(ω̃₀(−1)·n·l + φ_l(−1))` sign convention.** If DVSI
   implements Eq. 131 with `−cos` (or equivalently with `φ_l(−1)`
   offset by π), every voiced harmonic inverts. This is the single
   change that most directly produces the observed signature. Check:
   compare our Eq. 131 sign against DVSI's `s̃_{v,l}` contribution when
   the previous frame is voiced and the current is unvoiced.

2. **Eq. 132 `(n − N)` direction.** Eq. 132 handles the
   unvoiced→voiced crossfade with argument `ω̃₀(0)·(n − N)·l + φ_l(0)`.
   Inverting the `N` subtraction sign shifts every sample's phase by
   `N·ω̃₀·l`, which evaluates to ≈180° at typical ω̃₀ values (ω̃₀ ≈
   0.14, N = 160 → N·ω̃₀ ≈ 22.4 rad ≈ 3.57·π → wraps to ≈1.6π, or
   ~150° residual). Consistent with the observed offsets spread
   across 34–88°. Check: confirm sign on `(n − N)` vs DVSI reference
   — both forms appear in MBE literature.

3. **Eq. 139 ψ_l cumulative-phase sign.** If ψ_l accumulates with
   opposite sign convention from DVSI, phase inverts every frame
   consistently — but shouldn't by itself give a per-frame
   fractional-sample shift. Plausible as a **secondary** contributor
   behind #1 or #2.

4. **Eq. 137/138 wrap direction.** The Δφ_l wrap + Δω_l(0) rounding
   can produce per-harmonic fractional-sample shifts under some
   conditions but don't naturally produce a systematic 180° peak
   polarity flip across all frames. Lower-probability root cause.

5. **Annex A init values for φ_l / ψ_l.** Ruled out — initialization
   effects decay within a handful of frames, but the signature
   persists at frames 100, 500, 1500 (i.e. many seconds into the
   stream).

### Diagnostic Posture Going Forward

`xcorr-frame` is now a reusable diagnostic. Any proposed fix to the
voiced-synthesis pathway should be validated by re-running the same
three frames and watching for **peak correlation to shift toward +1.0
at lag 0** (or the closest integer lag to the expected fractional
shift, which should shrink as fixes accumulate).

**Expected progression for candidate #1 (Eq. 131 sign flip):**
- Before: peak −0.72 at lag 4 (frame 100)
- After a pure sign fix: peak +0.72 at lag 4 (same magnitude, polarity
  flipped), indicating the 180° component was the sole issue and the
  residual 34° is a second-order effect.

**Expected progression for candidate #2 (Eq. 132 (n−N) sign flip):**
- Peak polarity flips *and* lag shrinks toward 0, because the
  fractional offset component came from the same source as the 180°
  component.

These two candidates are empirically distinguishable by the lag
behavior of the fix.

### 2026-04-15 Update — Three Eliminations, One Remaining

Three targeted tests ran against the revised candidate list logged
in the `scan-transitions` commit (blip25-mbe `16b0dae`). Each test
applied the candidate as a code change, re-ran `scan-transitions
clean` (3700 frames, ~1100 transitions), and compared the aggregate
sign split and mean peak magnitude to the baseline.

**Rate-source prerequisite.** Before running any of the three, the
`halfrate-fec-histogram` diagnostic confirmed the full-rate vectors
used (`tv-rc/p25/`, `tv-rc/p25_nofec/`) pass the
`analysis/dvsi_test_vector_modes.md` fingerprint cleanly — 483/483
frames valid Golay [24,12] codewords on `alert`, 0 FEC errors. The
re-verify prerequisite flagged when `dvsi_test_vector_modes.md`
landed is satisfied: §11's candidate list is not confounded by
rate-39 contamination at the test-vector source.

**Test 1 — ψ_l cumulative / mod-2π mismatch (candidate #1, revised).**
Wrapped `state.psi[l]` and `state.phi[l]` to `[−π, π)` after every
frame's state update in the voiced synth. `cos()` is 2π-periodic so
the inner loop is unaffected; only the stored state value changes.
Re-ran `scan-transitions clean`: **every aggregate number bit-identical
to baseline** (V→UV 43/52, UV→V 56/51, V→V 478/460; |peak| 0.480,
0.406, 0.440). **Ruled out.** Unbounded f64 accumulation over
thousands of frames still has enough precision that the mod-2π
residue matches the wrapped value to double-precision epsilon.

**Test 2 — ρ_l noise-source alignment (candidate #2, revised).**
Forced `ρ_l = 0` on every frame to silence the Eq. 141 PN-noise
contribution in the `l > ⌊L̃/4⌋` branch of Eq. 140. Re-ran: maximum
sign-split shift was 2 frames out of 107 (UV→V), with mean |peak|
actually *rising* slightly (+0.009 / +0.019 / +0.004 across branches).
A real PN-alignment bug would have collapsed the sign split toward
uniformly-positive; the near-zero effect plus the slight |peak|
improvement says **ρ_l is weakly working against alignment, not
causing it.** **Ruled out.**

**Test 3 — Synthesis-window half-offset (new candidate).**
Hypothesis: DVSI's synthesis window `wS(n)` may be centered at `n =
N/2` vs our `n = 0`, which would shift our output by ~80 samples and
push the true xcorr peak beyond the default ±20 search window. The
fingerprint would be a large cluster in the ±80 bucket of the lag
histogram. Widened `scan-transitions` xcorr search to ±100 and
printed the lag top-6. Result:

- **No ±80 cluster.** The ±20 boundary elevation observed at the narrow
  search was not caused by peaks hiding beyond ±20.
- V→V top-6 lags: **+0:36, +3:29, −1:23, +2:19, −14:19, −13:19** —
  clear concentration at small integer lags (within ±5 samples ≈ one
  sub-harmonic period of ~20–40 samples at typical ω̃₀).
- Sign split shifted from 50/50 to modestly negative-biased:
  V→UV 56/39 (59% neg), UV→V 54/53, V→V 521/417 (56% neg). Mean
  |peak| rose across all branches (0.480 → 0.553, 0.406 → 0.468,
  0.440 → 0.496) — truncation by the ±20 window was hiding some real
  peaks.

**Ruled out.** No bulk sample-offset between our synth and DVSI's.

### Revised Candidate List After 2026-04-15 Eliminations

Of the four candidates logged in `16b0dae`, three are gone:

1. ~~ψ_l cumulative precision / mod-2π mismatch~~ — Test 1.
2. ~~ρ_l noise-source alignment~~ — Test 2.
3. **DVSI uses equivalent-but-different phase-tracking formulation.**
   Now the leading candidate. Consistent with: structural match
   (harmonic content correct, |peak| ≈ 0.5), weak systematic negative
   bias (~56%) overlaid with per-frame small-integer lag scatter
   (≤ ±5 samples), no single equation isolated by branch-specific
   transitions. Cannot be distinguished from spec-literal output
   without DVSI intermediate state.
4. Compound M̃_l error from Eq. 61/69 gain DCT — affects magnitude,
   not sign. Unlikely to explain the negative-bias signature but
   could be second-order alongside #3.

### Diagnostic Posture Going Forward (Updated)

Further progress on §11 **requires access to DVSI's intermediate MBE
parameter values** (ω̃₀, M̃_l, φ_l, ψ_l) for the same input bits.
The bug is no longer isolatable by PCM-level cross-correlation: the
scan-transitions signature (sign distribution + lag histogram + |peak|
magnitude) is what ruled out candidates 1 and 2, and those tools
have saturated.

The AMBE-3000 protocol supports intermediate-parameter readout per
DVSI's chip user's manual (the three-tier test architecture for
blip25-mbe calls this the "live-chip blackbox" tier). The cleanest
comparison is a single voiced-heavy frame, dumping both
implementations' φ_l array element-by-element:

- **Matches to float precision:** bug is downstream of phase (M̄_l,
  synthesis arithmetic, or the basis-function convention itself).
- **Differs by a consistent per-harmonic function:** DVSI uses an
  equivalent-but-shifted phase accumulator. Candidate #3 confirmed.
- **Differs randomly:** a state variable we haven't identified is
  being mis-initialized or mis-updated.

Until the live-chip harness is built, §11 is **bounded from above**:
we know the residual is not a simple-sign-flip and not any of the
three candidates above. PCM output is structurally close (|peak| ~0.5
says the harmonic skeleton is correct, amplitude calibration is a
separate known-open issue from the 2026-04-14 update). Downstream
work that doesn't require bit-exact PCM (parameter-level
conformance, bit-level roundtrip, tone-frame dispatch, half-rate
encode) is unblocked.

### 2026-04-15 Update — Perceptual Isolation: γ_w vs §11

A listening test on DVSI `tv-rc/p25/clean.pcm` (3700 frames, ~74 s
of Knox phonetically-balanced sentences) identified a specific
perceptual artifact: the word **"forest"** ( /f/ → /ɔɹəst/ ) was
heard as **"florist"** — a phantom /l/-like articulation inserted
at the UV→V boundary. A second artifact — a **click at the /k/ → /oʊ/
onset of "code"** — was audible in the default-mix output, which is
the same UV→V transition type.

To localize, two configurations were compared:
- **Default mix** (`m_scale=1.0`, `γ_w=146.6`): voiced + unvoiced
  both running at their spec-literal amplitudes. Phantom /l/ present.
- **Voiced-only** (`m_scale=1.0`, `γ_w=0`): unvoiced path silenced;
  voiced segments ring from silence with no unvoiced contribution.

In voiced-only, the voiced portion of "forest" plays as **"orest"
cleanly** — no phantom /l/. This rules out the voiced path as the
source of that artifact. The /l/ only appears when real unvoiced
energy is present on the preceding frame.

**Conclusion.** The phantom /l/ is driven by the separately-tracked
**γ_w unvoiced overshoot** (the ~150× scale error from the first
2026-04-14 update in this §), not by §11's voiced-phase residual.
The tail of the too-loud unvoiced /f/ bleeds into the first few
samples of the voiced /ɔ/ onset through the OLA window. The listener
categorizes that amplitude-modulated onset as a liquid consonant
release.

**Reprioritization implication.** The 2026-04-14 reprioritization
(§11.4) placed voiced-phase diagnostics ahead of γ_w re-examination,
on the grounds that amplitude calibration against a misaligned
waveform chases noise. That ordering remains correct for
*waveform-level bit-exactness*. But **for perceptual quality**, γ_w
calibration is now established as the dominant contributor — both
by magnitude (1.7× voiced overshoot vs 150× unvoiced overshoot) and
by the specific artifact it produces (audible consonant-insertion
at UV→V seams). Once the live-chip harness exists, **measure γ_w
first**: it's cheaper to localize (a scalar vs per-harmonic phase
array), it lands the larger perceptual win, and fixing it may
clean up enough of the auditory mess to make subjective validation
of the §11 fix easier.

### Implication for §12 (Half-Rate HOC Placement)

§12's resolution of the Eq. 179 bounds was logged at "similar quality
to full-rate's uncalibrated state" — i.e. under the sign error
diagnosed here. When the voiced-synthesis fix lands, re-run the
half-rate DVSI comparison. If Reading #1 holds under the corrected
voiced pathway, §12's resolution hardens to bit-exact; if it
diverges, §12 reopens. The xcorr harness can be pointed at half-rate
frames too.

---

## 12. Half-Rate HOC Placement — Eq. 179 Bounds Typo (Resolved)

**Source:** BABA-A §13.3.2 (encoder, page 62) and §13.4.3 Eq. 179 (decoder,
page 64).

**The two readings.** The printed decoder Eq. 179 says

```
C̃_{i,k} = { H̃_{i, k−2}   for 2 ≤ k ≤ J̃_i AND k ≤ 4
          { 0             otherwise                             (as printed)
```

Taken literally (Reading #2), this overwrites the PRBA-derived `C̃_{i,2}`
(Eq. 172/174/176/178) with an HOC value at `H̃_{i, 0}` — an out-of-range
index against the 4-entry Annex R codebooks — and caps coefficient
reconstruction at `k = 4`.

Reading #1 (encoder-consistent): `3 ≤ k ≤ min(J̃_i, 6)` with
`C̃_{i,k} = H̃_{i, k−2}`, zero-fill elsewhere. This is the only
interpretation consistent with §13.3.2's encoder-side definition
`H̄_{i,j} = Ĉ_{i, j+2}` for `1 ≤ j ≤ J_i − 2` and a 4-entry Annex R
codebook. It places HOC values at `C̃_{i, 3..6}` and leaves PRBA's
`C̃_{i, 2}` untouched.

**Resolution: Reading #1.** Half-rate decode-to-PCM under Reading #1
against DVSI `p25a.bit → p25a.pcm` reference vectors produces output
that correlates with the reference at the same quality level as the
current full-rate decode — which is itself not yet bit-calibrated, so
this is a ballpark match rather than a bit-exact one, but no empirical
contradiction of Reading #1 surfaced across the test vectors exercised.
Reading #2 was not attempted against DVSI (ruled out by the internal
inconsistency with §13.3.2 before implementation began).

**Implication for the printed PDF.** Eq. 179 as printed has what is
almost certainly a draft-edit artifact: the bounds `2 ≤ k ≤ 4` should
read `3 ≤ k ≤ 6`, with `k−2` mapping correctly into the 1-indexed
4-entry Annex R codebooks (`H̃_{i, 1}..H̃_{i, 4}` covering
`C̃_{i, 3}..C̃_{i, 6}`).

**Where the fix lives in the impl spec.**
`standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §2.12
carries the encoder-consistent rule as the working interpretation;
this entry promotes it from "disambiguation in progress" to "resolved
per DVSI vector agreement at current calibration level."

**Follow-up when full-rate reaches bit-exact calibration.** If the
full-rate decode path gets tightened to bit-match DVSI (the γ_w
investigation in §11 of this file is the likely gate), re-run the
half-rate DVSI comparison under Reading #1 for bit-exactness, not just
correlation. If it then diverges, revisit — but the algebraic
consistency with §13.3.2 makes an under-Reading-#1 divergence very
unlikely to be caused by this particular disambiguation.

---

## 13. Annex L Stores Cycles/Sample, Not Rad/Sample (Extraction Label Bug)

**Source:** BABA-A page 127 (Annex L) and §13.1 (page 58).

**The mislabel.** The PDF's Annex L column header is simply `ω₀` with no
unit annotation. The impl spec's §12.8 entry for `annex_l_pitch_table.csv`
originally described the column as "reconstructed fundamental frequency
ω̃₀ (rad/sample)". That annotation is wrong. The values are stored as
**cycles/sample** (= `f₀/fₛ` = normalized frequency = `ω̃₀/(2π)`).

**Proof that the values are cycles/sample:**

BABA-A §13.1 (page 58) states explicitly:

> When voice is present, the fundamental frequency is estimated in the
> interval `2π/123.125 ≤ ω̂₀ ≤ 2π/19.875`.

Evaluated: `ω̂₀ ∈ [0.05103, 0.31614]` rad/sample — i.e. `[65, 403] Hz`
at `fₛ = 8 kHz`, which is the standard adult voice fundamental range.

Annex L's stored range is `[0.008125, 0.049971]`. Two interpretations:

| Interpretation      | min → Hz at 8 kHz | max → Hz at 8 kHz | Matches §13.1? |
|---------------------|-------------------|-------------------|----------------|
| rad/sample (literal)| 10.3              | 63.6              | **No** — sub-voice |
| cycles/sample       | 65.0              | 399.8             | **Yes** — voice range ✓ |

Multiplying every entry by 2π brings the rad/sample range to
`[0.05105, 0.31400]`, matching §13.1's stated `[0.05103, 0.31614]`
range to within quantization. The stored values are exactly
`1 / (pitch_period_in_samples)` — e.g. `1/0.049971 = 20.01` samples
per pitch period at `b̂₀ = 0` (highest-pitch voice) and
`1/0.008125 = 123.08` samples per pitch period at `b̂₀ = 119`
(lowest-pitch voice). That is precisely the form of a
`cycles/sample` quantity.

**Why this was caught late.** The half-rate DVSI conformance harness
(`decode-pcm-halfrate`) compares synthesized PCM against DVSI r39
reference vectors and passed bit-exactly — which does not mean the
synthesis was semantically correct. If DVSI r39 applies the same
units interpretation as our code (either both multiplying by 2π on
load, or both reading the table verbatim), the two pipelines produce
identical PCM while potentially synthesizing sub-voice tones. The
regression surfaced only when the half-rate → full-rate cross-rate
converter was built: full-rate uses the analytical Eq. 46
(`ω̃₀ = 4π/(b̃₀ + 39.5)`), which is unambiguously rad/sample, and the
two tables don't line up until the Annex L values are reinterpreted
as cycles/sample. Audible verification (listening to the synthesized
output) is the other independent check — voice at 30 Hz sounds like
hum, not speech, and would be immediately obvious on any half-rate
test clip.

**Relationship to §11 (voiced-synthesis phase mismatch).** These are
two separate problems:

- §11's full-rate voiced-synthesis sign error affects Eq. 131–141
  and is rate-independent (full-rate uses Eq. 46, not Annex L).
- This §13 Annex L mislabel affects only half-rate pitch recovery.

If the half-rate decoder was reading Annex L as rad/sample and feeding
that into Eq. 131 `cos(ω̃₀·n·l + φ_l)`, the synthesized cosine would
oscillate at a fundamental `2π×` lower than intended — producing
sub-voice tones but still bit-matching DVSI r39 if DVSI applied the
same (mis)interpretation. The §11 phase flip signature (systematic
−1 correlation at small lag) is unrelated to this units issue and
would persist after Annex L is corrected.

**Fix applied.**

1. `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §12.8 —
   annotation rewritten to flag the cycles/sample convention and
   require callers to multiply by 2π on load. Recommended loader
   pattern: apply the `2π` factor once at table population so all
   downstream consumers see rad/sample.
2. `standards/TIA-102.BABA-A/annex_tables/annex_l_pitch_table.csv` —
   CSV header comment rewritten with the units statement, the §13.1
   range derivation, and a pointer to this disambiguation entry.
3. Downstream consumers that previously read the table verbatim as
   rad/sample will need either a one-line loader change (`value *= 2π`)
   or a consistent conversion at every call site. Prefer the loader
   fix — a single point of conversion eliminates the class of bug.

**Verification checklist after the loader fix:**

- Half-rate DVSI conformance: re-run `decode-pcm-halfrate`. If DVSI
  r39 also applies the 2π conversion, conformance will still pass;
  if DVSI reads verbatim, conformance will break and you'll need
  a debug flag to interop with r39 while spec-correct becomes the
  default.
- Audible listen test: synthesized half-rate speech should sound
  natural rather than ~30 Hz hum.
- Cross-rate round-trip: the half-rate → full-rate converter and
  full-rate → half-rate repeater tests that stubbed out over this
  gap should now close without diverging at the pitch-conversion
  boundary.

**Broader lesson.** The PDF's Annex tables that use ω/f notation
without explicit units are a known class of extraction trap. Column
headers with bare Greek letters should trigger a units check against
any equation in the surrounding prose that constrains the value's
range. In this case, §13.1's `2π/N` form is the explicit marker
that was overlooked during initial extraction.

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
