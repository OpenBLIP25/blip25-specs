# Gap 0022 — §1.11.1 frame-repeat: literal Eq. 103/104 vs §6.3 / §11.1 "attenuate, fade to silence"

**Status:** resolved 2026-04-25 on branch `gap-0022-frame-repeat-no-attenuation`.

**Resolution:** option (a). PDF BABA-A §7.7 (page 40) prescribes plain
assignment of previous-frame parameters via Eq. 99–104 — no attenuation,
no consecutive-repeat counter, no fade-to-silence at this layer. The
"attenuate, fade to silence" wording in §6.2 / §6.3 / §11.1 of the
derived implementation spec was inaccurate paraphrase introduced during
extraction; corrected on the topic branch above:

- §6.2 reworded to point at §9 adaptive smoothing as the spec's only
  attenuation-over-time mechanism during sustained repeats.
- §6.3 table row updated to "Copy previous M_l verbatim per Eq. 99–104;
  §9 γ_M applies on top."
- §11.1 item 6 rewritten: frame repeat is a plain copy; JMBE's
  `MAX_HEADROOM_THRESHOLD = 3` reset is documented as a beyond-spec
  quality choice, not a conformance requirement.
- §1.11.1 itself now carries a "No per-repeat attenuation" callout so
  future readers don't re-introduce the wrong paraphrase.

**Implementer guidance:** the literal-Eq.-99–104 implementation in
blip25-mbe is correct per spec. The 1.0 PESQ deficit vs JMBE on
chip-degraded streams is JMBE doing something the spec doesn't require.
If you want similar audible behavior on long repeat runs, add a
JMBE-style consecutive-repeat reset behind a feature flag, but ensure
conformance tests can disable it (the spec-faithful path must remain
available). Awaiting user merge.

**Filed:** 2026-04-25
**Filed by:** implementer (blip25-mbe)
**Spec area:** TIA-102.BABA-A (Vocoder), §1.11.1 Frame Repeat Trigger
(Eq. 99–104) vs §6.3 Frame Erasure/Muting Summary + §11.1 implementation
notes about "frame repeat attenuation."
**Severity:** Low–Medium — drives ~0.7–1.3 PESQ on chip-degraded bit
streams (chip.bit hits 41 consecutive Repeats at the start before
transitioning to Mute). On clean own-encoded bits, no impact (Repeat is
never triggered). But it's the dominant residual gap to JMBE on chip
input now that gap 0021's mute→noise fix has landed.

## The question

§1.11.1 (the authoritative implementation section) specifies the
frame-repeat substitution **literally** as Eq. 99–104:

```
ω̃₀(0) = ω̃₀(−1)                                                  (Eq. 99)
L̃(0)  = L̃(−1)                                                   (Eq. 100)
K̃(0)  = K̃(−1)                                                   (Eq. 101)
ṽ_k(0) = ṽ_k(−1)   for 1 ≤ k ≤ K̃                                 (Eq. 102)
M̃_l(0) = M̃_l(−1)  for 1 ≤ l ≤ L̃                                 (Eq. 103)
M̄_l(0) = M̄_l(−1)  for 1 ≤ l ≤ L̃                                 (Eq. 104)
```

These are **plain assignments** — no attenuation, no consecutive-repeat
counter, no fade-to-silence multiplier.

But §6.3 ("Frame Erasure/Muting Summary") states:

> | Repeat behavior | Attenuate previous M_l, fade to silence | Same principle |

And §11.1 item 6 ("Implementation Notes") states:

> Frame repeat attenuation: When repeating a frame due to errors, the
> previous frame's spectral amplitudes must be attenuated (not simply
> replayed). The attenuation factor increases with consecutive repeats,
> fading to silence.

These directly contradict the literal Eq. 103/104. Eq. 103 says "copy
M̃_l unchanged." §11.1 says "attenuate, must NOT simply replay."

**Which authoritative form should the implementer follow?**

- (a) Eq. 99–104 literal — copy unchanged on every consecutive Repeat.
  §6.3/§11.1 are inaccurate summaries to be ignored / corrected.
- (b) Eq. 99–104 + a §6.3-style attenuation curve. Specific curve to
  be specified by the spec-author. Common conventions:
  - JMBE's MAX_HEADROOM_THRESHOLD=3 (after 3 consecutive Repeats, reset
    to default fundamental + amps=1.0). Beyond-spec, JMBE-specific.
  - Geometric decay `M̃_l(repeat_n) = α^n · M̃_l(0)` for some
    `α ∈ (0, 1)` (typical α=0.7–0.9 in adjacent codecs).
  - Linear decay over a fixed window.
- (c) The PDF text resolves this elsewhere (§9 or Annex notes the
  derived spec didn't capture).

## Why it matters

On chip.bit (DVSI tv-std/tv/clean.pcm, 3700 frames):
- Frames 0–40: `FrameDisposition::Repeat` (each frame's ε_T ≥ 10+40·ε_R)
- Frames 41–3699: `FrameDisposition::Mute` (smoothed ε_R > 0.0875)

During the 41-frame Repeat run, our literal Eq. 99–104 keeps re-using
the prior `last_good` snapshot. On cold start `last_good = None` so
frame 0 falls through to the current (corrupt) frame's params. Subsequent
repeats lock onto frame 0's possibly-corrupt snapshot indefinitely.

JMBE on the same input audibly differs because its `IMBEModelParameters.
copy()` (codec/imbe/IMBEModelParameters.java) checks
`previous.getRepeatCount() > 3` and **resets to default fundamental +
spectral amplitudes = 1.0** on long repeat runs — a beyond-spec heuristic
that produces a steady comfort-tone instead of frozen-corrupt-replay.

Result: chip_enc_our_dec PESQ post-mute-fix is 1.39; JMBE on same bytes
is 2.66; chip-self-decode is 2.39. The 1.0 PESQ delta is plausibly the
repeat-attenuation behavior.

If the spec authoritatively prescribes attenuation (option b), our
literal-Eq.-103/104 implementation is the bug. If literal Eq. 103/104 is
authoritative (option a), our code is correct and JMBE / SDRTrunk's
behavior is beyond-spec.

## Diagnostic evidence

Probe scripts already in tree from gap 0021 (commit `cd42c53`):
- `conformance/speech-quality/examples/chip_bit_disposition.rs` produces
  the Use=6 / Repeat=35 / Mute=3659 histogram on chip.bit.
- `conformance/speech-quality/examples/probe_chip_dequant.rs` shows
  per-frame (ω₀, L, voicing, amp RMS) — illustrates the corrupt-but-
  plausible params during the Repeat phase.

JMBE source for cross-reference (read by spec-author only):
`~/jmbe/codec/src/main/java/jmbe/codec/imbe/IMBEModelParameters.java`
lines 107–143 (`copy()` method, `MAX_HEADROOM_THRESHOLD = 3`).

## Options considered

### (a) Literal Eq. 103/104 — no attenuation

Pro: §1.11.1 is an authoritative section with a numbered Eq. 103 that
plainly states the assignment. Implementing it requires no choice of
curve. Already what we ship.

Con: contradicts §6.3 + §11.1 summaries. On long Repeat runs the
output is "frozen replay of corrupt frame," which is empirically
worse-sounding than a controlled fade.

### (b) Eq. 103/104 + spec-defined attenuation

Pro: matches §6.3 / §11.1 textual guidance and aligns better with
audible quality on chip-degraded streams.

Con: spec doesn't give a specific attenuation curve. Spec-author would
need to choose one (or extract one from the PDF text I can't read).

### (c) Spec resolution — §6.3/§11.1 are inaccurate; remove

Pro: simplest path. Aligns spec internally, leaves our impl correct.

Con: closes the JMBE PESQ gap as "spec divergence" rather than fixable.

## Chip probe plausibility

Chip-side probe could measure: does the AMBE-3000R's decoder attenuate
during long repeat runs? Run a contrived bit stream that triggers many
consecutive Repeats and observe the chip's output amplitude trajectory.
Useful as evidence for option (b) but not authoritative — chip is
already known to be non-spec on multiple §1.10/§11/§0.* dimensions
(memory `project_chip_oracle_investigation_2026-04-16`).

Authoritative answer requires spec-author reading the BABA-A PDF text
that §6.3/§11.1 are paraphrasing.

## Suggested resolution path

1. **Spec-author reads BABA-A §7.7 (PDF p. 40)** and verifies whether
   the literal text describes any attenuation, fade, or repeat-counter
   behavior that the §1.11.1 transcription dropped.
2. If yes: fold the missing equation/rule into §1.11.1 as a new sub-
   section (Eq. 103a / 103b) with a specific attenuation curve.
3. If no: clarify §6.3 / §11.1 to remove the "attenuate, fade to
   silence" wording, which is currently misleading. Optionally note
   that JMBE's MAX_HEADROOM=3 reset is a pragmatic implementation
   choice, not spec-required.

## Relationship to gap 0021

Gap 0021 closed the FEC-convention question and landed the mute → comfort
noise fix (blip25-mbe `cd42c53`). This gap is the next step in the
chip_enc_our_dec quality narrative: with Mute now emitting noise, the
remaining PESQ deficit lives in the Repeat-phase behavior.
