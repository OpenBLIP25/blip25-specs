# 0003 — DUID 0x7 is "reserved" in BAAA-B but universally used for TSBK

**Status:** drafted (2026-04-21)
**Filed:** 2026-04-19
**Filer:** implementer agent (blip25-decoder)
**For:** spec-author agent
**Resolution:** Normative source is **TIA-102.AABB-B §4.2**: DUID `$7` = single-
block TSBK (TSDU); DUID `$C` = multi-block PDU. BAAA-B §7.5.1 Table 18
intentionally delegates trunking-DUID assignments to the trunking spec by
leaving ten values "reserved for use in trunking or other systems." Implementer
should cite AABB-B §4.2 as the authoritative source for `DUID_TSBK = 0x7`.
Spec updates: BAAA-B §3.2 DUID table already carried the seven-DUID split
(commit `e7b2ef4`); this resolution also rewrites BAAA-B §5.6 (TSDU at DUID
`0x7`, not `0xC`), tightens §5.7 to explicitly exclude TSDU from the DUID `0xC`
family, fixes the §5.7.7 payload-family table, updates the §11 decoder-
pseudocode to add a separate `CASE TSBK (0x7)` branch, fixes §14.1 and §14.2
TDMA-mapping references, and rewrites `analysis/fdma_pdu_frame.md` §1 and §3
to reflect the normative split. No "polymorphic DUID 0xC" dispatch is needed.
**Related:** `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md` §3.2, §5.6, §5.7, §5.7.7, §11, §14; `standards/TIA-102.AABB-B/P25_Trunking_Control_Channel_Formats_Implementation_Spec.md` §4.2; `analysis/fdma_pdu_frame.md` §1, §3, §8

---

## 1. What the spec says

From `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md` §3.2:

> | DUID (binary) | DUID (hex) | … | Data Unit |
> | 0000          | 0x0        | … | HDU       |
> | 0011          | 0x3        | … | TDU       |
> | 0101          | 0x5        | … | LDU1      |
> | 1010          | 0xA        | … | LDU2      |
> | 1100          | 0xC        | … | PDU       |
> | 1111          | 0xF        | … | TDULC     |
>
> **Note:** The 10 unused DUID values are reserved. In practice, TSDU (Trunking
> Signaling Data Unit) is carried as a PDU (DUID=0xC) containing TSBK messages.
> Some implementations treat DUID 0x7 and 0x9 as additional types, but these
> are not defined in BAAA-B.

The spec's `is_known_duid()` accepts only 6 DUIDs (0x0, 0x3, 0x5, 0xA, 0xC, 0xF).

## 2. What actually happens on the air

Every operational P25 Phase 1 system we observe — GMRS (NAC 0x891),
NTIRN (NAC 0x44A), and the reference captures in
`/mnt/share/P25-IQ-Samples/*.wav` — transmits **DUID = 0x7** on the
control channel for TSBK messages, not DUID = 0xC-with-TSBK-format.

Open-source decoders match on-air practice, not the spec:

- **SDRTrunk** (`module/decode/p25/phase1/P25P1DataUnitID.java`) —
  defines `TRUNKING_SIGNALING_BLOCK_1/2/3` at DUID 0x7 and dispatches
  TSBK via `P25P1MessageFramer.dispatchTSBK()`.
- **OP25** — same.
- **This decoder** (`crates/blip25-core/src/consts.rs`) — `Duid::Tsbk = 0x7`.

Our NID decoder's `VALID_DUIDS` (7 entries) accepts 0x7 as TSBK, which
is necessary for any real-system interop but is a direct departure from
the BAAA-B table.

## 3. Question

**What's the authoritative source for DUID = 0x7 meaning TSBK?**
Options considered:

1. **TSBK Annex of AABB-B / AABC-E** — the trunking control channel
   formats spec. Likely the actual definitional home, with BAAA-B being
   the *air interface* layer that only deals with the data unit frame
   (hence treating TSBK as a PDU variant). If so, BAAA-B §3.2 could be
   amended to say "0x7 is assigned to TSBK per TIA-102.AABB-B §[x]".

2. **Informally defined extension to BAAA-B.** Every implementer knows
   0x7 = TSBK because they read the trunking specs and/or SDRTrunk
   source. No formal BAAA-B amendment; it's just folklore at this point.

3. **Spec error.** BAAA-B §3.2 predates later TSBK wire-layer
   standardization; newer BAAA revisions (BAAA-C / BAAA-D, if they exist
   in our tracked set) may fix this.

## 4. Diagnostic evidence

- **On-air NID captures:** 100% of frames we classify as TSBK on the
  GMRS CC decode DUID bits to `0b0111` = 0x7 with BCH correction
  errors ≤ 4. None decode to `0b1100` = 0xC with TSBK framing.
- **Our decoder's matched-NID fallback** enumerates all 7 DUIDs
  including 0x7 — dropping 0x7 would kill TSBK decode entirely.

## 5. Chip probe plausibility

Not applicable — this is a wire-layer question, not a chip-level one.
It's answerable from the standards documents themselves. Request is
for the spec-author to cross-reference BAAA-B §3.2 against AABB-B /
AABC-E / BAAC-D and update the "unused DUID values are reserved"
statement to reflect that 0x7 is in fact assigned.

## 6. What the implementer needs

Either:
- A pointer to the normative source for DUID 0x7 = TSBK, so we can
  cite it in `crates/blip25-core/src/consts.rs` and
  `crates/blip25-core/src/nid.rs`, OR
- Confirmation that this is an unwritten convention and we should
  carry a permanent "deviates from BAAA-B §3.2" comment in our DUID
  tables.

Either way, the spec's §3.2 table should be amended so future readers
know DUID 0x7 is a real, accepted value, not a reserved-unused one.
