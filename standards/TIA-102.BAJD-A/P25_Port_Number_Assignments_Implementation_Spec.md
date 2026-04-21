# P25 TCP/UDP Port Number Assignments — Implementation Spec

**Source:** TIA-102.BAJD-A (2019-01), *Project 25 TCP/UDP Port Number
Assignments*. Supersedes TIA-102.BAJD (2010-08).

**Scope:** Registry-only document. Assigns specific TCP/UDP port
numbers from the IANA dynamic/private range (49152–65535) to named
TIA-102 data applications, plus reserved blocks for future
assignments. No protocol message formats or procedures — those live
in the application-specific TIA-102 documents that *use* these ports.

**Pipeline artifacts:**
- `annex_tables/baj_port_assignments.csv` — the normative Table 1
  assignment registry in CSV form, code-ready for consumers.
- `standards/TIA-102.BAJD-A/TIA-102-BAJD-A_Full_Text.md` — clean-room
  full-text extraction (copyrighted, git-ignored).

---

## 1. What This Spec Contains

A single normative table (Table 1) assigning port numbers to six named
TIA-102 data applications and defining reserved blocks for future P25
data applications. All assignments are drawn from the IANA
dynamic/private range.

The document is small (≈7 normative pages). Aside from Table 1 it
contains only framing prose explaining the purpose of the registry
and manufacturer-obligation rules.

## 2. Normative Port Assignments

See `annex_tables/baj_port_assignments.csv` for the full table in
machine-readable form. The assigned (non-reserved, non-available) P25
ports are:

| Port  | TIA-102 Application |
|-------|---------------------|
| 49198 | Tier 2 Location Service |
| 49199 | Inter-KMF Interface |
| 49200 | Key Fill Security Type — Radio Authentication |
| 49201 | Key Fill Security Type — End-to-End Encryption |
| 49202 | Key Fill Security Type — Link Layer Encryption |
| 64414 | OTAR (Over-the-Air Rekeying) |

Reserved blocks (no named application yet, but off-limits as defaults
for non-TIA-102 use):

| Block | Purpose |
|-------|---------|
| 49203 – 49213 | Reserved for TIA-102 data applications |
| 49226 – 49249 | Reserved for TIA-102 data applications |
| 49257 – 49279 | Reserved for TIA-102 data applications |
| 49281 – 49291 | Reserved for TIA-102 data applications |

"Available" blocks (49152–49197, 49214–49225, 49250–49256, 49280,
49292–64413, 64415–65535) remain within the IANA dynamic/private
range and are not restricted by TIA-102.

## 3. Manufacturer Rules (paraphrased from TIA-102.BAJD-A §2)

1. A TIA-102-assigned port number **SHALL** be used as the default
   destination port for its named application.
2. An implementation **SHOULD** make the port number configurable, so
   an operator can resolve network-specific conflicts or satisfy a
   security policy.
3. A manufacturer **SHOULD NOT** use any TIA-102-reserved port as the
   default for a non-TIA-102 application, even if the reserved block
   is not yet attached to a named P25 application.
4. A manufacturer **MAY** use any port outside the TIA-102-reserved
   blocks for non-TIA-102 applications, within the IANA rules.
5. Protocol (TCP vs. UDP vs. both) is determined by the application-
   specific document that uses the port, not by BAJD-A.

## 4. Revision Delta (A vs. original)

TIA-102.BAJD-A adds three Key Fill Security Type ports (49200, 49201,
49202) that were not in the 2010 original. Everything else —
Tier 2 Location (49198), Inter-KMF (49199), OTAR (64414), and the
reserved-block layout — was already present in the original, with
errata corrections only.

A consumer of the BAJD registry can therefore safely target Revision
A regardless of which revision a given deployment's reference
material cites; all original assignments are preserved by number and
meaning.

## 5. Cross-References

- **TIA-102.AACA series** — OTAR Messages and Procedures. Uses port
  **64414** as the default destination port.
- **TIA-102.BAJB / BAJC** series — Key Fill Device Interface. Uses
  ports **49200 / 49201 / 49202**, one per security type.
- **TIA-102.BACD** — Inter-KMF Interface. Uses port **49199**.
- **TIA-102.BAEE** series — Tier 2 Location Service. Uses port
  **49198**.
- **IANA Port Number Registry** — authoritative source for the
  dynamic/private range the P25 sub-range is drawn from.
- **RFC 768 (UDP) / RFC 793 (TCP)** — transport protocols whose port
  fields are being assigned from.

## 6. Implementation Guidance

- A P25 IP application implementing any of the six named services
  **MUST** open its listener on the assigned default port by default.
  Making that port configurable is a SHOULD, not an alternative to
  the default.
- A passive decoder parsing trunked or conventional data PDUs can use
  UDP destination port as the application discriminator above the
  LLC+SNDCP+IPv4+UDP stack (trunked) or SCEP+IPv4+UDP stack
  (conventional). The BAJD-A table is the canonical lookup — see
  `analysis/motorola_conventional_scep_vs_trunked_sndcp.md` §5 for
  the preamble invariants up to the UDP destination-port boundary.
- The six assigned ports are the only P25 ports worth
  hard-identifying in a monitor's UI today. Reserved-block ports
  (49203–49213, etc.) should be rendered as *"TIA-102 reserved
  (unnamed)"* rather than guessed at — the spec leaves them
  deliberately open for future assignments.
- Motorola-proprietary applications (ARS on 4005, CAD on 2002, LRRP
  on 4001, etc.) are **not** assigned by BAJD-A — they are vendor
  choices in the IANA registered range and are outside the TIA-102
  registry's scope. A decoder needs a separate vendor-port table for
  those.

## 7. What This Document Does *Not* Cover

- Message formats, session establishment, or security handshakes for
  any of the assigned applications. Those belong to the
  application-specific TIA-102 documents listed in §5.
- Service discovery or firewall-traversal conventions.
- Port negotiation protocols. BAJD-A is a static-assignment registry,
  not a dynamic port-allocation system.
- Whether a given named application uses TCP, UDP, or both. That
  choice is made in the application-specific document.

## 8. Cite-To Section References (for verifying against the PDF)

Per project convention, cite the PDF's own section numbers (not the
headings in this derived work):

- Manufacturer-rule prose — TIA-102.BAJD-A §2, pp. 3–4.
- Normative Table 1 (the port registry itself) — TIA-102.BAJD-A §2,
  Table 1, p. 3.
- Scope / motivation — §1.1, p. 1.
- Normative references (IANA, RFC 768, RFC 793) — §1.2.1, p. 2.
