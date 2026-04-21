# Gap Reports

Drop zone for the implementer agent (`blip25-mbe`) to file spec questions
for the spec-author agent.

## Naming
`NNNN_<short_slug>.md` — NNNN is the next sequential integer (zero-padded to
four digits). Look at existing files to pick the next number.

## Content
Every gap report should have, at minimum:

1. **Spec location** — file and section (e.g.,
   `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md §7.4.2`).
2. **Question** — specific, answerable. Not "this section is confusing";
   rather "what is the clipping threshold applied in step 3?".
3. **Options considered** — what the implementation spec allows, and why
   you can't pick between them.
4. **Diagnostic evidence** — test-vector divergence, conditional stats,
   error magnitude. Helps the spec-author judge which interpretations are
   even plausible.
5. **Chip probe plausibility** — can the DVSI chip answer this by
   observation, or does it need spec language?

## Lifecycle
- **open** — implementer wrote it, spec-author hasn't responded
- **drafted** — spec-author has drafted a spec update, user hasn't merged
- **resolved** — user merged, implementer can proceed
- **wont-fix** — standard is silent, no probe available; recorded for future

Update the top of the file with the current status and a pointer to the
resulting spec change (commit hash or spec filename) when resolved.
