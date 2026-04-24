# Gap 0020 — AABC-E OSP pseudo-layouts omit fields present in TIA bit tables

**Status:** resolved 2026-04-24 on branch `gap-0020-aabc-e-bit-tables` —
spec §4.3 (ISP) and §5.3 (OSP) rewritten as bit-indexed tables sourced from
the TIA-102.AABC-E PDF bit tables (with SDRTrunk cross-check). Authority
notes added at top of both sections. General pitfall captured in
`analysis/trunking_tsbk_bit_tables_vs_pseudo_layouts.md`. Awaiting user
merge to `main` before implementer consumes the update.

**Filed:** 2026-04-24
**Filed by:** implementer (blip25-decoder)
**Spec area:** TIA-102.AABC-E §5.3 — OSP Message Field Layouts (Abbreviated TSBK).
**Derived work:**
`standards/TIA-102.AABC-E/P25_Trunking_Control_Channel_Messages_Implementation_Spec.md`
§5.3 (lines ~1128+).
**Severity:** Medium — implementer silently writes wrong bit offsets if they
trust only the pseudo-layout. Confirmed to produce at least seven latent parser
bugs in blip25-decoder (see "Observed implementation impact" below).

## The question

The §5.3 OSP pseudo-layouts in our derived implementation spec enumerate fields
octet-by-octet (e.g., `Oct 2: Service Options`, `Oct 3: reserved`, ...).
For several opcodes the pseudo-layout **stops short of the full TIA bit
table**, omitting or collapsing fields that are present in the authoritative
TIA-102.AABC-E PDF bit tables.

The most impactful example: OSP **0x00 GRP_V_CH_GRANT**.

Our pseudo-layout (§5.3):

```
Oct 0: LB|P|000000    Oct 1: MFID
Oct 2: Service Options
Oct 3: reserved
Oct 4: Channel(T)[15:8]
Oct 5: Channel(T)[7:0]
Oct 6: reserved (or Group Address high in some variants)
Oct 7: reserved (or Group Address low)
Oct 8: Group Address[15:8]
Oct 9: Group Address[7:0]
```

The "reserved / or Group Address high in some variants" hedge at Oct 6-7 makes
the layout sound ambiguous and variant-dependent, which is misleading. In the
actual TIA-102.AABC-E bit table (row for 0x00), and matching every field-by-
field implementation in SDRTrunk
(`GroupVoiceChannelGrant.java`), the layout is **unambiguous**:

| Bit range | Field           | Width |
|-----------|-----------------|------:|
| 16–23     | Service Options |    8  |
| 24–27     | Frequency Band  |    4  |
| 28–39     | Channel Number  |   12  |
| 40–55     | Group Address   |   16  |
| **56–79** | **Source Address** |  **24** |

Our pseudo-layout **omits the 24-bit Source Address at bits 56–79 entirely**
and leaves Oct 6-7 "reserved," which would lead an implementer to read the
Group Address from Oct 8-9 (bits 64-79) and drop the source. Neither of those
reads matches the TIA bit table.

## Other opcodes affected (same pattern)

Audit against SDRTrunk's `phase1/message/tsbk/standard/osp/*.java` canonical
bit-index tables (all of which match the TIA-102.AABC-E PDF bit tables exactly)
turned up the same pseudo-layout-vs-bit-table disagreement for:

- **0x02 GRP_V_CH_GRANT_UPDT** — pseudo-layout shows only 8 bits of
  "Group Address B[7:0]" at Oct 9 (missing the high byte). Real bit table:
  Group Address B is 16 bits at 64–79.
- **0x14 SN_DATA_CHN_GNT** — no field layout given at all in pseudo-layout
  ("FNE grants a SNDCP data channel."). Real bit table:
  `svc_opts(8) | DL band(4)|chan(12) | UL band(4)|chan(12) | target(24)`.
- **0x20 ACK_RSP_FNE** — pseudo-layout gives AIV/EX/Service Type + Additional
  Information + Target, but does **not** clearly show that bits 32–55 carry
  `WACN||System` (EX=1) vs `Source Address` (EX=0). The AIV/EX union is
  documented in prose after the table but the pseudo-octet layout doesn't
  reflect it.
- **0x27 DENY_RSP** — pseudo-layout lists `Additional Info` at Oct 4-6 and
  `Target` at Oct 8-9, which is 16-bit target. Real bit table: Target is
  24-bit at 56-79.
- **0x2B LOC_REG_RSP** — pseudo-layout: `reserved, RC, reserved, Group[23:0],
  Source[23:0]`. Real bit table:
  `reserved(6) | RESPONSE(2) | GROUP(16) | RFSS(8) | SITE(8) | TARGET(24)`.
  **No source-address field exists**; the 24-bit tail is the target.
  This single misread caused blip25's LOC_REG_RSP parser to emit wrong
  talkgroup / RFSS / site / target values on every live decode until 2026-04-24.
- **0x29 SCCB_EXP** — pseudo-layout mentions "rfss(8), site(8), tx_ch(12),
  rx_ch(12)" — but the real bit table has **16-bit band+channel pairs**, not
  12-bit channel-only fields. A 12-bit channel can't be resolved against the
  iden plan.
- **0x39 SEC_CTRL_CH_BCST** — pseudo-layout doesn't exist; only implied by
  prose. Real bit table: `RFSS(8) | SITE(8) | TX_A band|chan(16) | svc_A(8) |
  TX_B band|chan(16) | svc_B(8)`.

## What the derived spec should show

Every §5.3 opcode entry should include a **bit-indexed table** (0-based from
TSBK start, matching the TIA PDF bit-position conventions) alongside the
octet-oriented view. The bit-table form is what SDRTrunk and OP25 both use,
and is unambiguous for implementers. Example for 0x00:

```
GRP_V_CH_GRANT (0x00) — bit layout

  0- 7  LB(1) | P(1) | OPCODE(6=0x00)
  8-15  MFID(8)
 16-23  Service Options
 24-27  Frequency Band
 28-39  Channel Number
 40-55  Group Address
 56-79  Source Address
```

When an opcode has AIV/EX-dependent field unions (like 0x20 ACK_RSP_FNE),
the table should enumerate each variant explicitly rather than relying on
reader inference from prose.

## Observed implementation impact

Audit of `crates/blip25-core/src/tsbk.rs::parse_tsbk_opcode` against
SDRTrunk's canonical tables (2026-04-24) found seven real bugs where
the implementer had taken the pseudo-octet layout at face value, not the
TIA bit table:

| Opcode | Bug |
|--------|-----|
| 0x14   | `channel` read service_options+band rather than DL band+channel; target read from bits 32-55 (UL channel area) instead of 56-79. |
| 0x15   | "target" field read SOURCE ADDRESS (bits 56-79) instead of TARGET (bits 32-55). |
| 0x20   | `target` and `src` literally swapped. |
| 0x27   | `target` read ADDITIONAL_INFO; `src` read the real target. |
| 0x2B   | Every field (talkgroup, rfss, site) off by 8 bits. |
| 0x29   | channel_a/channel_b missed the band nibble → unresolvable. |
| 0x39   | channel_b off by 8 bits. |

Six of the seven bugs are silent — wrong data downstream, no crash, no log
anomaly (until A/B against SDRTrunk). All seven are now fixed in blip25 and
match the TIA bit tables / SDRTrunk as of 2026-04-24.

## Proposed resolution

1. Rewrite the §5.3 OSP pseudo-layouts as bit-indexed tables sourced from the
   TIA-102.AABC-E PDF bit tables (the authoritative §3.31 tables in the
   published spec).
2. For opcodes with AIV/EX or RC-dependent unions (0x20 ACK_RSP_FNE, 0x27
   DENY_RSP), enumerate each variant's bit layout explicitly.
3. Mirror the same treatment in §4.3 for ISP layouts — a spot-check of 0x38
   IDEN_UP_VHF_UHF etc. suggests the same abbreviation pattern exists there.
4. Add a one-line cross-reference at the top of §5.3 pointing implementers
   to the TIA-102.AABC-E source bit tables, not the pseudo-octet form, when
   in doubt.

## Cross-references

- SDRTrunk canonical field classes:
  `/home/chance/sdrtrunk/src/main/java/io/github/dsheirer/module/decode/p25/phase1/message/tsbk/standard/osp/*.java`
- blip25 post-fix commit: pending (2026-04-24 batch including this audit).
- Apr-24 fix commit for related (but separate) opcode 0x3D:
  blip25 fc2b273 "tsbk: align OSP dispatch to TIA-102.AABC-E spec (six opcode remappings)"
