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
