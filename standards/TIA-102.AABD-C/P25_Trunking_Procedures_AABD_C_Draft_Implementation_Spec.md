# P25 Trunking Procedures — TIA-102.AABD-C Draft Implementation Spec

> **⚠ DRAFT. Do not rely on as a normative reference.**
>
> Source is **TR8.10-25-018 DRAFT**, *Project 25 Trunking Procedures* ("to
> be published as TIA-102.AABD-C"), November 2025. Approved in TR-8.10
> but not yet published as a TIA standard. The PDF carries a DRAFT
> watermark, approximately 43 editor's comments ([JL…] tags) flagging
> open issues, and red-text indicating proposed additions / changes
> relative to AABD-B. **Cite "AABD-C draft" — not "AABD-C" — until the
> published version lands.** Parameter values, opcode assignments, and
> field widths given here may shift before publication.
>
> Published baseline remains **AABD-B** (November 2014) + addenda
> **B-1** (LoPTT), **B-2** (User Alias), **B-3** (RAE). This document
> describes the delta from AABD-B to the AABD-C draft.

**Phase:** 3 — Implementation-ready (draft tracking)
**Classification:** PROTOCOL
**Extracted:** 2026-04-22
**Companion to:**
`standards/TIA-102.AABD-B/P25_Trunking_Procedures_Implementation_Spec.md`
— AABD-B impl spec remains the primary normative reference for every
procedure NOT listed in the delta tables below. Content unchanged from
AABD-B is **not re-documented here**; read this file for the delta,
read AABD-B for the full baseline.

**Cross-cutting companions (all still authoritative in AABD-C):**
- `standards/TIA-102.AABC-E/P25_Trunking_Control_Channel_Messages_Implementation_Spec.md`
  — opcode tables, ISP / OSP message layouts.
- `standards/TIA-102.AABB-B/P25_Trunking_Control_Channel_Formats_Implementation_Spec.md`
  — TSBK / MBT framing.
- `standards/TIA-102.AABF-D/P25_Link_Control_Word_Implementation_Spec.md`
  — LCWs referenced by traffic-channel procedures.
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`
  — FDMA PDU framing underlying the control channel.

---

## 1. Delta Summary (AABD-B → AABD-C Draft)

| Area | AABD-B state | AABD-C draft state | Impl impact |
|------|--------------|--------------------|-------------|
| **§8 Packet Data Procedures** | Informative Annex H | **Normative §8** | SUs and FNE implementing packet data must meet normative requirements; no longer optional. Existing implementations that tracked Annex H already are fine. |
| **§14 Composite Control Channel** | Informative Annex G | **Normative §14** | CCC behavior (composite mode bit, T_CCC, Direct-Method-only, no update messages) is now mandatory for CCC-capable sites. |
| **§11.2.4 Remotely Activated Emergency (RAE)** | Addendum B-3 (Feb 2025) | **Integrated into §11.2.4** | Promoted from addendum to base text. Procedure unchanged; path to spec is now unified. |
| **§11.2.5 Silent Emergency Activation** | Not present | **New** | New Silent Emergency Activation bit in Special Information field. SU must suppress UI indications. |
| **§11.3 Home/Serving RFSS split (emergency)** | Implicit in ISSI hand-off | **Explicit: serving RFSS informs group home RFSS via ISSI** | Tracking RFSSs that handle cross-RFSS groups must explicitly propagate emergency declaration across ISSI. |
| **§7.9 LoPTT** | Addendum B-1 + MFID90 in blip25 analysis | **Figure 18-11 added (MFID90 sequence diagram)** | Pattern unchanged from blip25's existing `analysis/fdma_pdu_frame.md` LoPTT coverage. |
| **§7.10 User Alias** | Addendum B-2 | **Integrated into §7.10** | Promoted from addendum. Procedure unchanged. |
| **§13.6 Alert Tones** | — | **New but marked "under development"** | Do not implement — spec text in this draft is incomplete. |
| **Throughout** | Home RFSS and serving RFSS roles ambiguous | Roles clarified in call-setup, affiliation, emergency procedures | Clarifications; no wire-format change. |

Open draft issues noted by the editor (~43 [JL] comments) are enumerated
per-section in `standards/TIA-102.AABD-C/TIA-102-AABD-C_Full_Text.md`;
implementers tracking the draft should re-read that file on each new
revision cut.

---

## 2. §11.2.4 — Remotely Activated Emergency (RAE)

**Purpose.** A privileged initiating entity (another SU or a dispatcher
console) causes a target SU to emit an emergency alarm without any
action by the target user. Use case: officer-in-distress where the
officer cannot manually trigger the alarm, remote dispatcher activation.

**Wire sequence (draft §11.2.4):**

```
Initiating SU / console                RFSS                     Target SU
          |                              |                         |
          | RAE_REQ (ISP)                |                         |
          |----------------------------->|                         |
          |                              | [auth/authorize check]  |
          |                              |                         |
          |                              |  RAE_CMD (OSP)          |
          |                              |------------------------>|
          |                              |                         |
          |                              |     (target SU may be on
          |                              |      traffic channel;
          |                              |      CMD is delivered
          |                              |      via LC_RAE_CMD or
          |                              |      on control channel
          |                              |      return)
          |                              |                         |
          |                              | EMRG_ALRM_REQ (RAE=1)   |
          |                              |<------------------------|
          |                              |                         |
          |       ACK_RSP_FNE            |  ACK_RSP_FNE            |
          |<-----------------------------|------------------------>|
          | T_ACK expiry if no ack       |                         |
```

**Normative SU behavior (target side):**
1. On receiving RAE_CMD, validate that the source is an authorized
   initiator (FNE or privileged WUID — RFSS-enforced).
2. Transmit EMRG_ALRM_REQ with the **RAE bit set in Special
   Information**.
3. Enter internal emergency alarm state as if the alarm had been
   user-initiated. Subsequent group / UU call requests carry E=1.
4. Silent-activation processing: if Silent Emergency Activation is also
   set (see §3 below), suppress all UI indications.

**Normative FNE behavior:**
1. On RAE_REQ, authorize the initiating SU or console.
2. If target SU is at a different RFSS, route the RAE_CMD across ISSI
   to the serving RFSS.
3. Deliver RAE_CMD to the target over the most current known path
   (control channel, or traffic channel via the appropriate LC).
4. Acknowledge the resulting EMRG_ALRM_REQ normally (ACK_RSP_FNE to
   the target, and notify consoles).

**Timers:** `T_ACK` governs the initiating side's wait for
acknowledgment of the RAE_REQ. See §9 of this doc for the consolidated
timer table (AABD-B values unless noted otherwise).

**Existing impl-spec coverage.** AABD-B-3 (RAE Addendum) introduced
this procedure; the phase-3 spec for AABD-B-3 was pending at the time
the AABD-B impl spec was written. AABD-C promotes it into the main
document — the wire-level procedure is the same as the addendum
defines.

---

## 3. §11.2.5 — Silent Emergency Activation

**Purpose.** Emergency alarm that produces no SU-side UI feedback. Use
cases explicitly named in the draft: undercover surveillance,
bus-driver duress.

**Encoding.** Silent Emergency Activation is carried as a bit in the
EMRG_ALRM_REQ Special Information field (exact bit position TBD per
draft [JL] comment — confirm against the finalized AABC-E field
definition before implementation). When set:

- No audible tone on the SU.
- No visual indicator change (LED, display update, icon).
- No vibration.
- Mic-hot / transmission behavior continues normally — the SU still
  joins subsequent emergency calls and emits the alarm on-air.

**Implementation note (passive decoder).** A passive receiver cannot
directly observe the SU's UI state. What it CAN observe is the Silent
Emergency Activation bit in the EMRG_ALRM_REQ — that's enough to label
the alarm correctly for dispatch / logging and to suppress any
UI-side "alarm received" indication on a decoder front-end that mimics
an SU's display.

**Normative SU behavior:**
- On receiving a local trigger configured for silent activation (e.g.,
  a concealed duress button): emit EMRG_ALRM_REQ with
  Silent Emergency Activation bit = 1. All downstream state transitions
  (alarm state entry, subsequent E=1 calls) proceed normally but with
  UI output suppressed.
- Cancellation (CAN_SRV_REQ) follows the same wire pattern as
  non-silent activation; the silent flag does not persist across
  cancellation.

**Normative FNE behavior:**
- Forward alarm to consoles normally. The console UI may suppress
  "alarm received" chime for silent alarms per operator policy; the
  draft is silent on whether FNE must enforce console-side
  suppression.

**Draft open issue.** Editor's comment [JL] flags that the exact bit
position of Silent Emergency Activation within Special Information is
still under review. Until publication, implementers should treat the
field offset as provisional and pin it down against AABC-E's final
layout.

---

## 4. §11 — Home / Serving RFSS Split in Emergency Procedures

AABD-B was written with the implicit assumption that a single RFSS
handles a given call end-to-end. AABD-C makes the home-RFSS /
serving-RFSS split explicit throughout §11 (and clarifies the same
split in §7 call-setup procedures). The wire messages are unchanged —
this is a behavioral clarification that affects FNE implementations
only, not SU-side or passive-decoder implementations.

**Tracking RFSS cross-ISSI emergency propagation (draft §11.3.2):**

| Event on serving RFSS | Required action toward home RFSS |
|------------------------|----------------------------------|
| Tracking GRP_V_REQ with E=1 received on serving RFSS for a group whose home RFSS is different | Serving RFSS informs home RFSS via ISSI that the talkgroup is in emergency state |
| Group emergency state cancellation (CAN_SRV_REQ, Svc Type=%000000) on serving RFSS | Serving RFSS informs home RFSS via ISSI that emergency state has been cleared |
| Member SU sends GRP_AFF_REQ with E=1 at a new site | Serving RFSS propagates emergency affiliation to home RFSS (for mobility tracking) |

**Non-tracking RFSSs** do not maintain per-talkgroup emergency state at
all and therefore do not propagate anything beyond the per-transmission
E-bit LCW pass-through. No ISSI change needed for non-tracking
deployments.

**Passive-decoder implication.** A decoder watching a single site sees
only the local side of this split. An apparent "emergency state
persists across talkgroup hand-offs between sites" observation in a
log is consistent with the home-RFSS bookkeeping described here; it's
not a bug in the decoder's per-site state machine.

---

## 5. §14 — Composite Control Channel Promoted to Normative

**Status change.** CCC was an informative Annex G in AABD-B; in AABD-C
it is normative §14. No wire format changes — the procedures are the
ones already documented in AABD-B impl spec §1.4 (CCC).

**Normative requirements now explicit (draft §14.2, §14.3):**
- Composite mode bit in System Service Class (Octet 9, bit 0) of
  RFSS_STS_BCST and NET_STS_BCST: set when CCC-capable.
- LC_RFSS_STS_BCST on traffic channel at rate ≤ T_CCC (default
  3 superframes, max 9 superframes).
- Direct-Method call continuation only (R bit = %0 in RFSS_STS_BCST) —
  Control-Channel-Method continuation is prohibited on CCC.
- No update messages on single-channel CCC (SUs cannot late-enter via
  control-channel update while CCC is in traffic state).
- SU must not send service requests while the CCC is in traffic state.
  Requests received during that interval may be silently discarded.

**Roaming constraint (§14.5).** A roaming SU must not disqualify a
channel that's emitting traffic-channel signaling: it may be a CCC in
traffic state. The SU should wait or continue scanning rather than
marking the channel bad.

**Passive-decoder implication.** A decoder using the SDRTrunk-style
"probe for control-channel framing periodically" pattern already
handles CCC transitions correctly. A decoder that hard-maps a
frequency to "control channel" for the lifetime of a capture will
mis-label traffic during CCC intervals. The mitigation is the same as
AABD-B impl spec §1.4 recommended, now made normatively binding by
the promotion to §14.

---

## 6. §8 — Packet Data Procedures Promoted to Normative

**Status change.** Packet Data Procedures were informative Annex H in
AABD-B; in AABD-C they are normative §8. Wire-level formats continue
to be defined in BAEB-C / BAED-A / BAEA-B.

**Normative requirements now explicit (draft §8):**
- SUs request packet data service via the control channel with the
  SNDCP grant mechanism defined in BAEB-C.
- Receiver NAC qualification (§8.2) — SUs accept packet-data grants
  only when the NAC matches the currently-affiliated NAC, preventing
  cross-system leakage.
- Packet data conveyance (§8.3) follows BAEB-C block structure on the
  granted traffic channel.

**Impact on blip25-style passive decoders.** The promotion to normative
§8 does not change what bits appear on the wire; all observable
behavior is still defined by BAEB-C / BAED-A. The impact is that an
RFSS that claims AABD-C conformance now MUST implement packet data
procedures correctly — so field-captured packet-data behavior should
become more uniform across vendors.

---

## 7. §7.9 — LoPTT MFID90 (Motorola) Sequence Diagram

AABD-C adds **Figure 18-11** (MFID90 LoPTT call sequence diagram)
formalizing Motorola's LoPTT implementation into an informative annex
reference.

**Wire-level pattern (already covered in existing derived works):**
- LCW sequence on LDU1 / LDU2 alternates `LC_MOT_PTT_LOC_HDR` +
  `LC_MOT_PTT_PAYLD` with standard `LC_GRP_V_CH_USR` link-control
  words.
- RFSS converts MFID90 LCs to standard GVCU LCs when repeating across
  ISSI / to non-MFID90 sites.
- GPS data is not forwarded across ISSI / CSSI — it's dropped at the
  RFSS boundary.

**No new implementation work** for passive decoders that already parse
the MFID90 LC pattern (see `standards/TIA-102.AABF-D/...` link control
word impl spec). AABD-C's contribution is formalizing the
repeat-conversion behavior that vendors were already implementing
ad-hoc.

**Existing derived works covering this territory:**
- `standards/TIA-102.AABF-D/P25_Link_Control_Word_Implementation_Spec.md`
  — LC_MOT_PTT_LOC_HDR / LC_MOT_PTT_PAYLD byte layouts.
- `analysis/fdma_pdu_frame.md` — LCW sequencing on LDU1/LDU2.

---

## 8. §7.10 — User Alias Promoted from Addendum B-2

AABD-B-2 (User Alias Addendum) is integrated into §7.10 of AABD-C. No
wire-level change:
- Phase 1: two-part (Part A + Part B) in odd/even conveyance slots.
- Phase 2: single MAC message in SACCH per BBAD-A-1.

Both forms continue to support up to 14 characters of talker ID.

---

## 9. §17 Parameter Values — Delta

**Unchanged parameters** (AABD-B → AABD-C draft): all timers and
counters from AABD-B Table 17-1 carry forward with the same
max/default/min values. See AABD-B impl spec §11 for the consolidated
timer table.

**New in AABD-C draft:**

| Parameter | Max | Default | Min | Scope | Description |
|-----------|-----|---------|-----|-------|-------------|
| `T_ACK` (RAE) | — | — | — | §11.2.4 | Initiating SU wait for RAE_CMD acknowledgment. Exact value TBD in draft; track [JL] comment on §11.2.4. |
| `T_CCC` | 9 superframes | 3 superframes | — | §14.3 | LC_RFSS_STS_BCST period on CCC traffic channel. Promoted from Annex G. |
| `T_Conv_Fallback_LC` | 10 s | 10 s | — | §16 | Conventional fallback LC broadcast interval. Already in AABD-B; explicitly named in AABD-C. |
| `N_Conv_Fallback_LC` | — | 2 | — | §16.11 | Consecutive missed LC_CONV_FALLBACK count before SU exits fallback. |

**Draft open issue.** Multiple [JL] comments in §17 flag that several
parameter values are under committee review. Values captured here
reflect the November 2025 draft snapshot; verify against the
published AABD-C for any production implementation.

---

## 10. §13.6 — Alert Tones (UNDER DEVELOPMENT, do not implement)

The draft carries a §13.6 "Alert Tones" placeholder flagged **under
development**. Implementers should:
- Not emit any Alert Tones extended-function command until AABD-C is
  published and §13.6 is finalized.
- Not assume the field layout shown in the draft is stable — it
  explicitly may change.
- Treat any Alert Tones EXT_FNCT_CMD received from an early-adopter
  system as an unknown function per AABD-B impl spec §13's
  forward-compatibility rule (log, ACK if required, take no other
  action).

---

## 11. Open Draft Issues ([JL] Editor Comments)

The draft carries approximately 43 editor comments flagging open
design questions. The full enumeration is in
`standards/TIA-102.AABD-C/TIA-102-AABD-C_Full_Text.md` at the point
where each comment appears. Categories:

| Category | Rough count | Examples |
|----------|-------------|----------|
| Wording clarifications (no impl impact) | ~15 | Section references, sentence rephrasing |
| Parameter values still under review | ~8 | `T_ACK`, `T_CCC` bounds, N_retry defaults in CCC mode |
| Cross-section consistency (home vs serving RFSS) | ~7 | §7 vs §11 wording alignment |
| ISSI propagation details (emergency) | ~5 | Exact ISSI message inventory for cross-RFSS emergency |
| Silent Emergency Activation field offset | ~2 | AABC-E bit position for Silent bit |
| Alert Tones design | ~3 | §13.6 scope, field layout |
| Editorial (figure numbers, typos) | ~3 | — |

**Implementation guidance:** pin AABD-C consumption to a specific
draft revision (or wait for publication) and treat the [JL]
comments as a changelog gate — any [JL] comment resolution in a
future draft revision may change parameter values or field layouts.

---

## 12. What This Doc Does NOT Cover

Unchanged from AABD-B — read AABD-B impl spec for these:

- Control channel acquisition (§5) — no procedural change, only
  wording clarifications.
- Short hunt / extended hunt / maximum extended hunt algorithm.
- Registration (§6) state machine — U_REG_REQ / U_REG_RSP / LOC_REG /
  full registration flow.
- Group affiliation (§6.8 / §6.9) procedure.
- Voice call setup (§7.1 Group, §7.3 Unit-to-Unit) apart from the
  home/serving RFSS clarifications in §4 above.
- PSTN interconnect procedures (§7.4–§7.6).
- Availability check (§7.7), Preemption (§7.8).
- Supplementary services: Call Alert (§10.1), Short Message (§10.2),
  Status Update / Query (§10.3–§10.4), Radio Unit Monitor (§10.5–§10.6).
- Non-emergency portions of §11 (Emergency Call handling apart from
  RAE, SEA, and the H/S-RFSS clarifications).
- System status broadcasts (§12): NET_STS_BCST, RFSS_STS_BCST,
  IDEN_UP family, ADJ_STS_BCST, SCCB, SYNC_BCST.
- Extended functions (§13): Radio Check, Radio Inhibit, Radio
  Uninhibit, Radio Detach — unchanged except Alert Tones (§10).
- Conventional fallback (§16) — unchanged.
- Addressing (Annex A) — unchanged: WACN/System/RFSS/Site hierarchy,
  SUID / WUID, SGID / WGID, special addresses ($FFFFFC FNE control,
  $FFFFFD System Default, $FFFFFE Registration Default, $FFFFFF ALL
  UNIT; $0000 null group, $FFFF ALL SYSTEM).

---

## 13. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify. Use "AABD-C draft, §X.Y (TR8.10-25-018,
Nov 2025)" to make the draft-vs-published status unambiguous:

- Packet Data Procedures (normative promotion) — §8, §8.1–§8.3.
- LoPTT and Figure 18-11 — §7.9, Annex D.
- Remotely Activated Emergency — §11.2.4, Annex D Figure 18-12.
- Silent Emergency Activation — §11.2.5.
- Group Emergency Call / home-serving RFSS split — §11.3.1–§11.3.8.
- Composite Control Channel (normative promotion) — §14.1–§14.6.
- Alert Tones (under development) — §13.6.
- Parameter values (including new timers) — §17, Table 17-1.

---

## 14. Cross-References

- `standards/TIA-102.AABD-B/P25_Trunking_Procedures_Implementation_Spec.md`
  — primary normative baseline. Read together with this delta spec.
- `standards/TIA-102.AABD-B-3/` — RAE Addendum (AABD-B era). AABD-C
  promotes the RAE procedure into §11.2.4 of the base document.
- `standards/TIA-102.AABC-E/P25_Trunking_Control_Channel_Messages_Implementation_Spec.md`
  — RAE_REQ, RAE_CMD, EMRG_ALRM_REQ opcode details. AABD-C refers to
  AABC-E for the wire-level message format; Silent Emergency
  Activation's field offset lives in AABC-E.
- `standards/TIA-102.AABB-B/P25_Trunking_Control_Channel_Formats_Implementation_Spec.md`
  — TSBK / MBT framing; unchanged by AABD-C.
- `standards/TIA-102.AABF-D/P25_Link_Control_Word_Implementation_Spec.md`
  — LC_GRP_V_CH_USR, LC_MOT_PTT_LOC_HDR / _PAYLD, LC_EXT_FNCT_CMD,
  LC_CONV_FALLBACK. Referenced by AABD-C throughout.
- `standards/TIA-102.BBAD-A/P25_TDMA_MAC_Message_Parsing_Implementation_Spec.md`
  — Phase 2 TDMA message parsing; AABD-C §7.10 (User Alias) references
  BBAD-A-1 for the Phase 2 SACCH carriage of User Alias.
- `standards/TIA-102.AABA-B/P25_Trunking_Overview_Implementation_Spec.md`
  — architectural overview; defines the home/serving RFSS terminology
  that AABD-C §11 clarifies.
- `standards/TIA-102.AABD-C/TIA-102-AABD-C_Full_Text.md` — full draft
  extraction with [JL] editor comments preserved. Re-read on each new
  draft revision.
