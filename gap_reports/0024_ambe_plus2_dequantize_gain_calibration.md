# Gap 0024 — JMBE half-rate (AMBE+2) decoder outputs ~17 dB louder than DVSI chip

**Status:** Closed (no spec change needed; finding documented in
`CHIP_GAIN_CALIBRATION.md`).
**Filed:** 2026-05-13.
**Spec section consulted:** `~/blip25-specs/standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md`
§2.11 (Gain + PRBA, Eq. 168) and §1.10 (enhancement).

## Original observation

JMBE 1.0.9 rendered six GMRS-Sachse Phase 2 calls ~7× (16-18 dB) louder
than our half-rate decode on the same 9-byte AMBE+2 inputs. JMBE's
`DifferentialGain.java:11-42` documents a per-b̂₂ +1.0..+1.55 log₂-domain
offset added to the spec-defined `Δ̃_γ` (Eq. 168), comment-labelled
"adjustment to increase gain level of generated audio that more closely
matches output gain of hardware generated audio." Steady-state effect
of that offset on Eq. 168's `γ̃(0) = Δ̃_γ + 0.5·γ̃(−1)` recurrence is
~2× the adjustment ≈ +2.8 log₂, which exponentiates through Eq. 188 to
the observed ~7× M̃ multiplier.

## Resolution via DVSI chip A/B (2026-05-13)

Probed the actual AMBE-3000R hardware (`PKT_RATET 33` half-rate mode)
on the same 6-call corpus. Result:

| Source | Full-corpus RMS | Per-frame ratio vs ours |
|--------|----------------:|-------------------------:|
| Chip   |          506.6  |                    0.99× |
| Ours   |          511.1  |                    1.00× |
| JMBE   |         3479.4  |                    6.87× |

**The chip matches us, not JMBE.** Our half-rate decode is at parity
with hardware emission (within −0.08 dB), identical in shape to the
2026-04-23 full-rate IMBE measurement against `tv-rc/r33/clean.pcm`
(1.001× ratio).

JMBE's comment about matching hardware is incorrect — JMBE's adjustment
table actually overshoots hardware emission by ~17 dB. The cause is
likely that JMBE was tuned against a downstream rendering pipeline
(SDRTrunk's playback chain, which itself applies AGC / post-vocoder
gain) rather than against the chip's raw PCM output.

## PCM round-trip sanity check pins this down

A round-trip cannot grow PCM amplitude — whisper in, whisper out.
Decoding the DVSI-supplied half-rate test vector
`Research/DVSI Vectors/tv-rc/r33/clean.bit` (chip-encoded from the
PCM source `tv-rc/clean.pcm`) through every available decoder, then
comparing the output RMS to the original input PCM RMS:

| Source                            | RMS    | vs input PCM |
|-----------------------------------|-------:|-------------:|
| Input PCM                         | 1291.6 |    +0.00 dB  |
| DVSI r33 reference output (oracle)| 1283.9 |    −0.05 dB  |
| Live AMBE-3000R chip              | 1283.9 |    −0.05 dB  |
| Ours (native)                     | 1261.1 |    −0.21 dB  |
| JMBE 1.0.9                        | 5114.7 |   +11.95 dB  |

Live chip output is byte-identical to DVSI's published reference at
this rate. Ours preserves input PCM within 0.21 dB (~0.16 dB quieter
than the chip — consistent with codec quantization loss alone). JMBE
outputs 4× the input PCM amplitude, which is structurally impossible
for a conformant decode of a recorded-then-encoded PCM file. The
+11.95 dB figure understates JMBE's gain bias because the longer
clean.pcm input has loud transients that JMBE's internal `0.95 ×
Short.MAX_VALUE` clipper truncates; the GMRS field-call analysis
(+16.7 dB) captured the un-clipped figure.

## What this means for the implementation spec

**Nothing to change.** The BABA-A §2.11 spec is correct as written. The
literal `γ̃(0) = Δ̃_γ + 0.5·γ̃(−1)` recurrence with Annex O's 32-entry
table produces chip-accurate output. No empirical adjustment is needed
on the decode side — that would push us *away* from chip parity.

## What this means for consumers wanting JMBE-equivalent loudness

Same disposition as the full-rate `CHIP_GAIN_CALIBRATION.md` decision:
post-vocoder loudness is a rendering-pipeline UX property, not a codec
constant. The existing `BLIP25_VOICE_GAIN_DB=+9` default applies +9 dB
of consumer-side gain (HPF + peaking + output gain in the
`enhancement::Classical` chain). To match JMBE-equivalent loudness,
the consumer should raise that to ~+17 dB; do not modify the codec.

## Diagnostic harness used

- `conformance/scripts/JMBEDecodeAMBE.java` — JMBE driver for 9-byte
  AMBE+2 input.
- `conformance/scripts/JMBEDecodeAMBEDump.java` — JMBE driver that also
  dumps per-frame M̃ / M̄ / ω₀ / L / gain.
- `crates/blip25-mbe/examples/jmbe_diag.rs` — same-shape diagnostic for
  our half-rate path, used for the M̃ A/B against JMBE.
- `conformance/chip/python/dvsi_driver.py` — extended in this session
  with `set_p25_halfrate()`, `decode_halfrate_frame()`,
  `decode_halfrate_file()` and a CLI subcommand
  `dvsi decode-halfrate <ambe9> <pcm_out>` against the AMBE-3000R on
  `pve` (192.168.1.6).

## Secondary finding: half-rate FEC error handling was using full-rate thresholds

While investigating the gain calibration question, the chip A/B exposed
a separate spec-faithfulness bug in our `frame_disposition`: half-rate
FEC error counts and thresholds (§2.8.1–§2.8.3, Eq. 196–199) were not
applied for the AmbePlus phase mode — the full-rate §1.11 thresholds
were used instead. Concretely:

1. `ε_T` was summed across all four FEC cosets; Eq. 196 specifies
   `ε_T = ε₀ + ε₁` (Golay codewords only) for half-rate.
2. ε_R recurrence weight was `0.05/144 ≈ 0.000347`; Eq. 197 specifies
   `0.001064` for half-rate.
3. Repeat trigger `ε_T ≥ 10 + 40·ε_R` (full-rate); Eq. 198-199 specify
   constant `ε_T ≥ 6` for half-rate.
4. Mute threshold `ε_R > 0.0875` (full-rate §1.11.2); §2.8.3 specifies
   `ε_R > 0.096` for half-rate.

Fixed in this session by adding a separate `frame_disposition_halfrate`
and dispatching from `synthesize_frame_with_mode` based on `PhaseMode`
(Baseline = IMBE full-rate, AmbePlus = AMBE+2 half-rate). Also fixed
`ambe_plus2_pipeline::decode` to compute `ε_T = ε₀ + ε₁` and to publish
the per-frame `FrameErrorContext` to `vocoder.synth.err` before the
synth call (it was previously left at default-zero, which masked all
of the above thresholds anyway).

## Tertiary finding: chip applies amplitude management beyond BABA-A §2.8

After the half-rate disposition fix, frame 773 of call #3537 correctly
triggers Repeat per Eq. 198-199 (ε₀=3, ε_T=ε₀+ε₁=6). Replaying the prior
voice frame's params through our synth still produces a peak of ~25 k
on that frame and ~27 k on frame 774 (the L=31→L=18 transition out of
the Repeat). The DVSI chip on the same input produces peaks of 6.9 k
and 2.6 k for those frames — substantially quieter than a strict
Repeat-and-synth path produces. This implies the chip applies
amplitude attenuation post-Repeat (decay over consecutive bad frames
or a Repeat-specific gain reduction) that BABA-A §2.8 does not
document. Out of scope for this gap report; left as a separate
follow-up item for the chip-amplitude-management investigation.
