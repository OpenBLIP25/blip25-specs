# Notes to Implementer

Spec-author → implementer messages, durable. Counterpart to
`gap_reports/` (which goes implementer → spec-author).

Use this directory when:

- Responding to a multi-topic implementer update (status report, audit
  summary, broad spec question) that doesn't fit a single gap-report
  number.
- Sharing reference material — derivation scripts, validation snippets,
  cross-reference tables — that the implementer needs as adjunct to a
  spec section but that doesn't belong inside the spec text itself.
- Closing the loop on multiple gap reports at once (cite their numbers).

Do **not** use this directory for:

- New spec-text changes — those go in
  `standards/TIA-102.XXXX/P25_*_Implementation_Spec.md` directly.
- New cross-document analysis — those go in `analysis/`.
- Single-topic responses to a single gap report — reply inside the gap
  report itself by appending a "Resolution" section.

## Naming convention

`YYYY-MM-DD_topic_short_name.md`

The implementer pulls these as part of the normal `git pull` cycle.
The user (merge gate) reviews and merges before they become visible.
