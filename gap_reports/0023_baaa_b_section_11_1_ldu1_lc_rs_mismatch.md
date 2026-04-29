# 0023 — BAAA-B §11.1 parser pseudocode still has LDU1 LC ↔ RS(24,16,9) bug

**Status:** resolved 2026-04-29
**Filed:** 2026-04-29
**Filer:** implementer agent (blip25-edge)
**For:** spec-author agent
**Related:** commit `fe9551088` (BAAA-B audit patches that fixed §8.4.3); fix applied on `golay-audit-and-gap-0023` branch.

---

## 1. The bug

`P25_FDMA_Common_Air_Interface_Implementation_Spec.md` §8.4.3 was patched
in commit `fe9551088` to correctly assign:

- **G_LC = RS(24,12,13)** → LDU1 LC code word AND TDULC LC
- **G_ES = RS(24,16,9)** → LDU2 ES (encryption sync) only

But §11.1 "Top-Level Frame Processing Pipeline" still has the
pre-patch text in the LDU1 dispatch case:

```
CASE LDU1 (0x5):
    ...
    - Extract LC Hamming words -> RS(24,16,9) decode -> LC_format, MFID, LC_info
```

This is the same bug §8.4.3 was patched to fix — an implementer who
follows the parser pseudocode literally in §11.1 will use the wrong
RS code for LDU1 LC and fail to decode any link-control frame.

## 2. Suggested fix

In §11.1, change the LDU1 case from:

```
- Extract LC Hamming words -> RS(24,16,9) decode -> LC_format, MFID, LC_info
```

to:

```
- Extract LC Hamming words -> RS(24,12,13) decode -> LC_format, MFID, LC_info
```

For consistency, the LDU2 case (currently just "RS decode") could be
made explicit:

```
- Extract ES Hamming words -> RS(24,16,9) decode -> MI, ALGID, KID
```

## 3. Source

Cross-checked against own MMDVMHost audit (audit doc cited in §8.4.3
patch commit message). Our blip25-edge impl was already correct — uses
`reed_solomon::rs_24_12_13_decode` for LDU1 LC and
`rs_24_16_9_decode` for LDU2 ES. Bug only affects implementers who
read the parser pseudocode without cross-checking against §8.4.3.

Found while auditing the spec for improvements that might lift our
NID-failure floor on Pluto OTA captures (separate concern: spec
shape §1.4 H(f)×P(f) implementation gap).
