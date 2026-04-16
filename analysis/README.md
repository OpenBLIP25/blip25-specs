# P25 Analysis Notes

Value-add analysis that goes beyond what the TIA specs say on their own.
These pieces synthesize cross-document insights, correct common misconceptions,
and provide implementation guidance informed by real-world P25 system behavior.

These are **not** spec extractions — they are original analysis derived from
the TIA-102 specifications combined with practical engineering knowledge.

## Index

1. [IMBE Frame Format vs MBE Vocoder: Decoupling the Wire from the Codec](vocoder_wire_vs_codec.md)
2. [The Missing Vocoder Specs: AMBE+2, BABB, and BABG](vocoder_missing_specs.md)
3. [BABA-A Vocoder Decode Pipeline: Disambiguations](vocoder_decode_disambiguations.md) — clarifications needed to implement the decode math that the impl spec glossed or deferred (quantizer form, per-block DCT, ρ = 0.65, b̂_{L+2} role, V/UV band mapping, etc.)
4. [Phase 4 Findings Log](phase4_findings_log.md) — running log of real spec correctness bugs and material ambiguities caught during Phase 4 verification runs. Persistent home for findings that would otherwise live only in commit messages.
5. [LCW Implementation Traps](lcw_implementation_traps.md) — four common errors when implementing P25 Link Control Word parsing: the two "P" bits with different meanings, mandatory Source ID Extension state machine, LCO 9/10 trunked-only restriction, and channel field bit layout.
6. [Open-Source P25 Vocoder Implementations: Lessons Learned](oss_implementations_lessons_learned.md) — what's trustworthy and what's known-flawed in mbelib / JMBE / OP25 / SDRTrunk when used as secondary reference for implementation verification (not as spec source).
7. [BABA-A Analysis Encoder (PCM → MbeParams): Implementation-Spec Gap](vocoder_analysis_encoder_gap.md) — the implementation spec is decode-only; this enumerates what BABA-A §§2–5 material (framing, pitch estimation, pitch refinement, V/UV discriminant, spectral amplitude estimator, prediction residual, encoder state, tone/silence dispatch) is missing from the derived spec and must land before the blip25-mbe analysis encoder can be written.
8. [BABA-A Analysis Encoder Addendum — Draft](vocoder_analysis_encoder_addendum.md) — staging file for the proposed §0 addendum to the BABA-A implementation spec. §0.2 (the `S_w(m, ω₀)` basis function, Eq. 24–30) drafted; remaining subsections pending.
9. [Analysis Encoder Addendum: Eq. 7 `r(t)` Transcription Error](vocoder_analysis_eq7_correction.md) — the addendum's §0.3.2 / §0.3.8 / §0.3.9 originally squared `s_LPF` on both factors of Eq. 7, breaking dimensional analysis of Eq. 5 and producing `E(P)` excursions of 10⁴–10⁶ on voiced content. Corrected; flags §0.2/§0.4/§0.5/§0.7 as still-unaudited for the same class of mis-transcription.
10. [AMBE Predictor State Separation Invariant](ambe_predictor_state_separation.md) — the three AMBE pipeline roles (decoder, encoder, rate converter) each maintain a prior-frame log-magnitude predictor. Coefficients often coincide (0.65 for half-rate intra-rate and for rate-converter cross-rate), but state instances must stay independent — sharing them produces AR-loop instability and ~50-frame spectral drift.
11. [AMBE Encoder Closed-Loop Predictor Feedback](ambe_encoder_closed_loop_predictor.md) — the encoder must run a matched decoder internally so its prior-frame predictor state tracks what the wire decoder will reconstruct, not what the encoder analyzed. Open-loop predictors look correct at frame 1 but drift over ~50 frames. Test: encoder and decoder predictor states must be bit-identical at every frame boundary.
12. [AMBE+2 Phase Regeneration Kernel as Discrete Hilbert Transform](ambe_phase_regen_hilbert.md) — US5701390's `h(m) = 2/(π·m)` is the discrete Hilbert transform impulse response. The regenerated phase is Kolmogorov's identity: phase = Hilbert{log|M̃|} for minimum-phase signals. Explains why this kernel, why γ = 0.44 scaling, why D = 19 truncation, and the ⌊L̃/4⌋ lower-harmonic exclusion.
