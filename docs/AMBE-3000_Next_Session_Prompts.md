# AMBE-3000 Next-Session Prompts

One self-contained prompt for a fresh Claude Code session in
`~/blip25-specs`. Pastable without modification.

**Recent history (2026-04-16 session, commits `f29e11e`..`bb2ccf5`):**
- AMBE-3000 per-rate annex tables landed:
  `halfrate_bit_allocations.csv`, `rate_conversion_pairs.csv`,
  `multi_subframe_rates.csv` (chip-index keyed, with `pending`/`n/a`
  where public sources don't reach).
- US6199037 multi-subframe joint-quantization algorithm drafted in
  decoder §10.5, encoder §10.2, rate converter §10.4.
- Project aliases renamed `r33` → `p25_fullrate`, `r34` →
  `p25_halfrate` across impl specs (matches blip25-mbe module naming).
  Resolves Gap 2 of `analysis/ambe3000_rate_bit_allocation_gap.md`.
- `AMBE-3000_Test_Vector_Reference.md` reconciled with DVSI filesystem
  layout (commit `bb2ccf5`): HDK eval-kit tree vs `DVSI Vectors/`
  normative tree distinguished; `cmpp25.txt` documented as authoritative
  p25_fullrate driver; chip rate-index semantics (r33 = on-air
  p25_halfrate w/FEC, r34 = no-FEC, p25_fullrate = IMBE via RCW)
  captured. Decoder/encoder/rate-converter §11 path citations updated
  to match on-disk paths; nested `tv-rc/rA/rB/` layout replaces the
  incorrect `tv-rc/rA→rB/` arrow notation.

---

**Status:** No active AMBE-3000 prompt. Spec-repo side is caught up.
All remaining AMBE-3000 gaps are blocked on empirical characterization
in `~/blip25-mbe` against DVSI test vectors:

1. **Gap 1 of `analysis/ambe3000_rate_bit_allocation_gap.md`** —
   per-field bit widths for AMBE+2 chip rates 35–61 (60 of 64 rows in
   `annex_tables/halfrate_bit_allocations.csv` are `pending`).
2. **`analysis/ambe3000_multi_subframe_rate_mapping.md`** — subframe
   count per chip rate index (60 of 64 rows `pending` in
   `annex_tables/multi_subframe_rates.csv`).

Both require running DVSI test-vector decodes/encodes through a
working software pipeline and comparing against reference output — a
`~/blip25-mbe` workstream, not spec-extraction work.

When those empirical runs yield rate-by-rate data, the follow-up here
is to fill the `pending` CSV rows and tighten the `analysis/` gap
entries. Write a new prompt in this file at that time.

---

## Other Workstreams

For unrelated next-session work in `~/blip25-specs`:
- `python3 tools/process_specs.py --list` — Phase 3 impl-spec queue
  (43 processable, many marked `[NEEDS PHASE 3]`).
- `TIA-102.BABA` (original IMBE vocoder description) is a natural pair
  with the AMBE-3000 work above.
