# AMBE-3000 Next-Session Prompts

One self-contained prompt for a fresh Claude Code session in
`~/blip25-specs`. Pastable without modification.

**Recent history (2026-04-16 session, commits `f29e11e`..`c5d0af0`):**
- AMBE-3000 per-rate annex tables landed:
  `halfrate_bit_allocations.csv`, `rate_conversion_pairs.csv`,
  `multi_subframe_rates.csv` (chip-index keyed, with `pending`/`n/a`
  where public sources don't reach).
- US6199037 multi-subframe joint-quantization algorithm drafted in
  decoder §10.5, encoder §10.2, rate converter §10.4.
- Project aliases renamed `r33` → `p25_fullrate`, `r34` →
  `p25_halfrate` across impl specs (matches blip25-mbe module naming).
  Resolves Gap 2 of `analysis/ambe3000_rate_bit_allocation_gap.md`.

**Still open, but blocked elsewhere** (not this session's work):
- Gap 1 of `ambe3000_rate_bit_allocation_gap.md` — per-field bit
  widths for AMBE+2 chip rates 35–61 need `~/blip25-mbe` empirical
  characterization against DVSI test vectors.
- `analysis/ambe3000_multi_subframe_rate_mapping.md` — subframe count
  per chip rate index needs the same `~/blip25-mbe` work.

**This session's prompt:** a contained spec-repo correctness pass that
does not need `~/blip25-mbe`.

---

## Prompt — Test_Vector_Reference Correctness Audit

**Copy everything between the triple-dashes below into a fresh session.**

---

Project: `/home/chance/blip25-specs` — clean-room software
implementation of DVSI AMBE-3000 (P25 vocoder).

TASK: Audit the impl specs' references to DVSI test vectors and
reconcile them with DVSI's actual filesystem layout. Several impl-spec
§11 sections cite paths like `tv-std/r33/*` and `tv-rc/r33→r34/*` that
don't match what's on disk. Correct the mismatches without renaming
the project aliases (`p25_fullrate` / `p25_halfrate` were resolved in
prior commit `c5d0af0`).

KEY FINDINGS FROM PRIOR SESSION (use these as starting context):

1. DVSI test vectors live in **two distinct trees** with different
   naming conventions:
   - `/mnt/share/P25-IQ-Samples/DVSI Software/Docs/AMBE-3000_HDK_tv/`
     — flat `rNN/` directories (one per chip rate index 0–61), plus
     root-level PCM sources (`clean.pcm`, `dam.pcm`, `noisy.pcm`, …).
     **No `tv-std/`, no `tv-rc/` subdirs here.**
   - `/mnt/share/P25-IQ-Samples/DVSI Vectors/tv-std/` — contains
     `tv/` subdir with: source PCMs (`clean.pcm`, `alert.pcm`,
     `cp0.pcm`, …); test recipes (`cmpstd.txt`, `cmpp25.txt`,
     `cmpp25a.txt`, `cmpp25x.txt`); reference output subdirectories
     (`p25/`, `p25_nofec/`, and others that `cmp*.txt` recipes
     reference).
   - `/mnt/share/P25-IQ-Samples/DVSI Vectors/tv-rc/` — rate-conversion
     test vectors; separate tree from tv-std.

2. **Critical semantic issue** at the chip rate-index 33 vs. project
   `p25_fullrate` boundary: DVSI's chip index 33 is AMBE+2 half-rate
   with FEC (= `p25_halfrate` w/FEC). DVSI's chip index 34 is AMBE+2
   half-rate no-FEC. **P25 full-rate IMBE is NOT in the chip's
   rate-index table** — it is invoked via direct RCW programming
   (`0x0558 0x086B 0x1030 ...` with FEC;
   `0x0558 0x086B 0x0000 0x0000 0x0000 0x0158` no-FEC) and its
   reference test vectors live under the `cmpp25.txt` recipe at
   `DVSI Vectors/tv-std/tv/p25/*.pcm` and `p25_nofec/*.pcm`, **not**
   under any `rNN/` directory. Anywhere the impl specs write
   `tv-std/r33/*` claiming to mean "P25 full-rate test vectors,"
   that is incorrect — those vectors are in the `cmpp25.txt` path
   family.

READ FIRST:
1. `DVSI/AMBE-3000/AMBE-3000_Test_Vector_Reference.md` — primary doc
   under audit. Check every path claim against the actual filesystem.
2. `DVSI/AMBE-3000/AMBE-3000_Decoder_Implementation_Spec.md` §11 —
   has test-vector path citations in §11.3 (recommended test order,
   near line 1325) and the test vector table around line 1480.
3. `DVSI/AMBE-3000/AMBE-3000_Encoder_Implementation_Spec.md` §11 —
   similar table around line 1277; §11.3 validation items around
   line 1119.
4. `DVSI/AMBE-3000/AMBE-3000_Rate_Converter_Implementation_Spec.md`
   §11 — `tv-rc/*` path citations around line 1195; validation plan
   earlier in §11.
5. `analysis/ambe3000_rate_bit_allocation_gap.md` "Resolution Adopted
   — Option C" section — explains the project-alias rename context
   and flags this test-vector correctness pass as the follow-up.
6. `test_vectors/dvsi_manifest/tv-std.sha256` and `tv-rc.sha256` —
   authoritative file manifests (sha256 + relative path). Use `grep`
   on these to confirm what files actually exist before writing any
   path into the spec.

FIRST STEPS (before editing anything):
1. `ls` both DVSI trees (`AMBE-3000_HDK_tv/` flat root, and
   `DVSI Vectors/tv-std/tv/`, `DVSI Vectors/tv-rc/`) to enumerate
   top-level structure.
2. Read `cmpp25.txt` and `cmpstd.txt` from `tv-std/tv/` — these
   recipes define reference outputs and where DVSI stores them.
   `cmpp25.txt` is the authoritative P25 full-rate test driver.
3. Grep the impl specs for every `tv-std/` or `tv-rc/` path citation
   and classify each:
   - (a) correct as-is (path exists, references correct algorithm),
   - (b) path exists but refers to the wrong algorithm (e.g.,
     `tv-std/r33/*` cited as P25 full-rate when it is actually
     AMBE+2 half-rate w/FEC),
   - (c) path does not exist at all.

SCOPE CHECKPOINT — run past the user BEFORE drafting corrections:
Present the classified list (or a compact summary: how many (a), (b),
(c); notable patterns) and the proposed correction strategy for each
class. Don't edit until this is approved.

DELIVERABLES:
1. Corrections to test-vector path citations in the four files listed
   above. Prefer minimal edits: fix paths, don't restructure §11 unless
   the path confusion has propagated into the validation logic itself.
2. Expand `AMBE-3000_Test_Vector_Reference.md` if needed to:
   - Distinguish the two trees (HDK evaluation vectors vs DVSI
     tv-std/tv-rc normative vectors) and document which to use for
     which validation target.
   - Call out `cmpp25.txt` (and siblings) as the authoritative source
     for P25 full-rate (`p25_fullrate`) test vectors, including the
     RCW + input-PCM + reference-output mapping.
3. Commit as one logical unit: `"Test_Vector_Reference: reconcile path
   citations with DVSI filesystem layout"`.

CONVENTIONS:
- Keep chip-index filesystem path naming (e.g.,
  `AMBE-3000_HDK_tv/r33/`) — these ARE correct chip-index references
  to AMBE+2 half-rate vectors.
- Do NOT rename project aliases (`p25_fullrate`, `p25_halfrate`) —
  resolved in commit `c5d0af0`.
- If a spec cites a path that doesn't exist, either correct the path
  or remove the claim if the described test can't actually be run.
  Don't silently change a validation item into a different one.
- New facts about DVSI's test vector layout land in
  `Test_Vector_Reference.md`, not in a new `analysis/` entry. This is
  catalog-correctness work, not cross-document disambiguation.

OUT OF SCOPE (explicit):
- Running any test vectors through a decoder (blip25-mbe work).
- Resolving Gap 1 of `ambe3000_rate_bit_allocation_gap.md` (per-field
  bit widths for chip 35–61).
- Resolving `ambe3000_multi_subframe_rate_mapping.md` (subframe count
  per rate).
- Any rename of project aliases or chip-index numbering.
- `DVSI Vectors/tv-rc/` correctness audit may surface the same class
  of issue as `tv-std/` — address if trivial, defer to a follow-up
  prompt if the two trees' conventions diverge further.

Start by listing both DVSI trees and reading one `cmp*.txt` recipe to
understand the reference-output model. Then grep the impl specs for
`tv-std` and `tv-rc` references and present the classification. Run
the proposed fix strategy past me before making any edits.
