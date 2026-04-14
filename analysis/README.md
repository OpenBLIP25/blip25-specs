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
