# PDU Reassembly Fixtures

Structural fixtures for testing passive cross-copy PDU assembly — the merge
procedure documented in `analysis/passive_cross_copy_pdu_assembly.md`.
These are **decoder behavior** tests, not CRC-math tests; they exercise the
assembler's block-substitution logic and confidence labeling, not the
underlying integrity primitives.

## When to add a fixture

When a new pathological multi-copy scenario is found on-air (e.g., a copy
pattern the current assembler handles incorrectly) or when a new confidence
label edge case needs lock-in.

## Fixture format

Each fixture is a directory under this one named
`NNNN_<short_slug>/` with:

- `README.md` — describes what the fixture exercises, including:
  - The observed on-air scenario (LLID, timing, copy count).
  - What each copy looks like in terms of per-block CRC state.
  - The expected merged output's confidence label and per-block source map.
- `copy_N.hex` — one file per physical copy. Each file holds the
  post-trellis-decode pre-CRC-check block contents as hex octets, one
  block per line. The first line is the header block (12 octets, 10 pre-CRC
  + 2 CRC); subsequent lines are data blocks (12 octets for Unconfirmed,
  18 octets for Confirmed).
- `expected_merged.hex` — same octet format, with blocks selected per the
  merge procedure. Emit the expected per-block `source_copy_index` as a
  trailing comment on each line (`# from copy 2`).

## Suggested first fixtures

(Not yet produced; track as follow-up work.)

| ID | Scenario | Tests |
|----|----------|-------|
| 0001 | Unconfirmed 4-block PDU, 2 copies, Copy A has block 3 errored, Copy B has block 1 errored | `merged` label, block 1 source=copy2, blocks 2/3 source=copy1 |
| 0002 | Confirmed 5-block PDU, 3 copies, Copy A has blocks 2+4 errored, Copies B and C each clean one of them | `merged` label, blocks 2/4 sourced from later copies |
| 0003 | Unconfirmed 3-block PDU, 4 copies, block 2 never clean across all copies | `partial` label with dirty-block mask = {2} |
| 0004 | Unconfirmed 2-block PDU, 2 copies, both copies clean | `native_clean` label (merge is a no-op when all copies are pristine) |
| 0005 | Unconfirmed 14-block PDU modeled on the Sachse gold-standard CAD alert, 8 copies with empirically-observed corruption patterns | Real-world regression; validate that the production decoder produces 3 `merged` + 1 `partial` against SDRTrunk's 4 clean + 4 corrupt-unmerged |

## Scope note

These fixtures are FDMA-only. TDMA Phase 2 has a different PDU framing
layer; if passive assembly becomes interesting for TDMA, mirror this
directory structure under `annex_tables/tdma_reassembly_fixtures/` rather
than extending this one.
