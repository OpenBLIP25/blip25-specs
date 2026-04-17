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
  (42 processable, many marked `[NEEDS PHASE 3]`).
- `TIA-102.BABA` was closed out 2026-04-16 (commit `45c121b`): marked
  `not_required`, superseded by the already-processed BABA-A. No further
  BABA work is planned.

---

## Recommended next session: Key Management cluster

Four related documents, self-contained workstream, complements the
already-processed `AAAD-B` (block encryption) and `AACE-A` (link layer
authentication). Key management is the largest unspecced crypto gap.

### Prompt — paste into a fresh session in `~/blip25-specs`

> Process the P25 key management cluster through Phase 2+3. These four
> documents cover Key Fill Device (KFD) interface protocol across three
> revisions plus OTAR (Over-The-Air-Rekeying) rev C. All four are
> classified `protocol` and marked `[NEEDS PHASE 3]` in the queue.
>
> Suggested order (oldest → newest, so the diffs are useful):
>
> 1. `TIA-102.AACD`   — KFD Interface Protocol (original)
> 2. `TIA-102.AACD-A` — KFD Interface Protocol (revision A)
> 3. `TIA-102.AACD-B` — KFD Interface Protocol (revision B, in progress)
> 4. `TIA-102.AACA-C` — OTAR Protocol (revision C)
>
> For each document:
> - `python3 tools/process_specs.py TIA-102.<id>` to run Phase 1+2
>   (extraction + summary + resources). Inspect the output and commit.
> - Produce a Phase 3 implementation spec following
>   `docs/TIA_P25_Processing_Prompt_v2.md` conventions: language-neutral
>   (C snippets, not Rust), bulk tables as CSV under `annex_tables/`,
>   inline message opcode tables in the spec.
> - Update `specs.toml` with `status = "processed"`, `processed_date`,
>   and `extracted_files` paths. Commit each document separately.
>
> Cross-check points:
> - KFD revision diffs: note added/removed/renamed messages between
>   AACD / AACD-A / AACD-B. This is the most useful output for
>   implementers — previous revisions exist in the wild.
> - OTAR rev C should cross-reference the encryption primitives in
>   AAAD-B (ALGID, KID, MI) — don't re-specify them, cite the existing
>   impl spec.
> - Check whether `~/blip25-specs/analysis/` needs a new entry for
>   cross-document disambiguation (KFD transport assumptions, OTAR
>   state machine).
>
> Avoid scope creep: do NOT touch `AACE` (original link-layer auth) or
> `AAAD-A` (original encryption) in this session — those are separate
> "original revision" passes, lower priority than getting current
> revisions specced.

### Alternative clusters (if you'd rather something else)

- **ISSI** (3 docs): `BACA-B-3` (IWF addendum), `BACE` (conventional),
  `BACF` (packet data). Smaller and more self-contained than the data
  stack, larger than the key-management cluster.
- **Data services stack** (6–7 docs): `BAEA-B`/`BAEA-C` (overview),
  `BAEB-B` (IP bearer, pairs with done `BAEB-C`), `BAED` (packet data
  LLC), `BAEF` (host NI), `BAEJ`/`BAEJ-A` (conventional mgmt). Biggest
  coherent workstream remaining.
