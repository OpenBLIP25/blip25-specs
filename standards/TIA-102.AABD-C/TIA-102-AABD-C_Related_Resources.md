# TIA-102.AABD-C Related Resources and Context
# Project 25 Trunking Procedures — Revision C Draft
# Prepared: 2026-04-22

---

## Document Status

| Field | Value |
|-------|-------|
| TIA Number | TIA-102.AABD-C |
| Internal Number | TR8.10-25-018 |
| Status | DRAFT (not yet published) |
| Draft Date | November 2025 |
| Committee | TIA TR8.10 |
| Supersedes | TIA-102.AABD-B (published) |
| Also relates to | TIA-102.AABD (original 1997 TSB), TIA-102.AABD-A |

Approximately 43 open editor's comments in the draft (tagged [JL28]–[JL43]), each
corresponding to a pending ballot issue. Text shown in red represents proposed new
or modified material. The document is not yet approved for publication as an
ANSI/TIA standard.

---

## Standards Family

### Direct Dependencies (cited normatively)

| Reference Tag | Document | Content |
|--------------|----------|---------|
| [AABC] / [AABC-E] | TIA-102.AABC-E | Control channel message opcodes, PDU formats |
| [AABB] | TIA-102.AABB-B | Control channel frame formats (TSBK, MBT) |
| [AABF] | TIA-102.AABF-D | Link Control Words (LCW / LC opcode dispatch) |
| [AACE] | TIA-102.AACE-A | Link layer authentication procedures |
| [BAAA] | TIA-102.BAAA-B | FDMA CAI (voice frame format, LDU structure) |
| [BAAC] | TIA-102.BAAC-D | Reserved values (NAC, MFID, ALGID lookup) |
| [BAAD] | TIA-102.BAAD-B | Conventional control messages |
| [BAEA] | TIA-102.BAEA | Packet data bearer (radio channel layer) |
| [BAEB] | TIA-102.BAEB-C | IP data bearer / SNDCP |
| [BAED] | TIA-102.BAED-A | Packet data LLC (logical link control) |
| [BACA] | TIA-102.BACA | Inter-Subsystem Interface (ISSI) |
| [BACA-B-1] | TIA-102.BACA-B-1 | ISSI addendum 1 |
| [MFID] | TIA-102.BAAC (or equivalent) | Manufacturer ID table |

### Related Procedural Documents

| Document | Relationship |
|----------|-------------|
| TIA-102.AABG | Conventional Control Messages — the conventional-mode counterpart |
| TIA-102.BAAD-B | Conventional Procedures — state machines for conventional (non-trunked) mode |
| TIA-102.AABD-B | Published predecessor; this draft supersedes it |
| TIA-102.BBAD-A-1 | TDMA addendum with LoPTT and User Alias MAC messages |

### Addenda Referenced (not yet published)

| Tag | Content |
|-----|---------|
| [AABC] (addendum) | Remotely Activated Emergency (RAE) opcodes — mentioned as to-be-published |
| [AABC] Annex D | Current ISP and OSP opcode lists (Annexes B and C of this document delegate here) |

---

## Key Concepts and Where to Find Them

### Addressing

AABD-C Annex A (normative) is the authoritative source for the full hierarchical
address scheme: WACN (20-bit) → System (12-bit) → RFSS (8-bit) → Site (8-bit),
plus SUID (56-bit) = WACN + System + Unit ID (24), WUID (24-bit local alias),
SGID (48-bit) = WACN + System + Group ID (16), WGID (16-bit local alias).

Special address table (Table 18-5 in this document):
- $FFFFFC = FNE reserved (radio control/dispatch)
- $FFFFFD = System Default (registration/mobility)
- $FFFFFE = Registration Default
- $FFFFFF = ALL UNIT broadcast

### Control Channel Message Opcodes

ISP and OSP opcode assignments are defined in TIA-102.AABC-E, not in AABD-C.
AABD-C references them by mnemonic (GRP_V_REQ, DENY_RSP, etc.) only. Annexes B
and C of this document are informative stubs that explicitly say "Refer to [AABC]
Annex D."

### Emergency Procedures

The most procedurally complex section in the document is §11. Key disambiguation:
- **Tracking RFSS** (§11.3.1): maintains talkgroup emergency state independently of
  each SU's E bit; always repeats audio with E bit set once group is in emergency state
- **Non-tracking RFSS**: follows each transmitting SU's E bit; no persistent state
- **RAE** (§11.2.4): new in Revision C; allows a privileged SU to remotely trigger
  another SU's emergency alarm without that SU user's action
- **Silent Emergency** (§11.2.5): new in Revision C; E bit set but no UI indication

### LoPTT

Two distinct manufacturer-specific implementations (§7.9):
- **MFIDA4** (L3Harris, §7.9.1): two-part GPS format transmitted after GVCU LCWs;
  RFSS combines and routes to mapping application
- **MFID90** (Motorola Solutions, §7.9.2): LC_MOT_PTT_LOC_HDR + LC_MOT_PTT_PAYLD
  alternating with LC_GRP_V_CH_USR; RFSS converts to GVCU LCs for repeat

Neither implementation is forwarded over ISSI/CSSI.

### Parameter Values

All timing and count parameters are in §17 (Table 17-1). Critical values:
- T_SLOT = 37.5 ms (slotted Aloha slot)
- T_BCST = 3 sec (broadcast interval for all system-level OSPs)
- T_ACK = 4 sec default (wait for target unit acknowledgment)
- T_wuid_validity = 4 hr (WUID expiry)
- T_Conv_Fallback_LC = 10 sec (max between LC_CONV_FALLBACK LCWs)
- T_CCC = 3 superframes default (CCC traffic channel RFSS_STS_BCST rate)

---

## Open-Source Implementation References

### SDRTrunk (Java)

The primary reference open-source P25 trunking decoder/monitor.

Relevant modules:
- `src/main/java/io/github/dsheirer/module/decode/p25/phase1/message/tsbk/` — TSBK
  ISP/OSP message decoding, maps directly to AABC-E opcodes referenced by AABD-C
- `src/main/java/io/github/dsheirer/module/decode/p25/phase1/P25P1TrafficChannelManager.java`
  — manages call grant and channel transitions per AABD-C §7
- `src/main/java/io/github/dsheirer/module/decode/p25/identifier/` — SUID/WUID/SGID/WGID
  address classes; aligns with Annex A
- `src/main/java/io/github/dsheirer/module/decode/p25/phase1/message/lc/` — LCW parsing;
  LC_GRP_V_CH_USR, LC_RFSS_STS_BCST, LC_CONV_FALLBACK

### OP25 (Python/C++)

GNURadio-based P25 decoder. Relevant:
- `op25/gr-op25-repeater/lib/p25_frame_assembler.cc` — control channel frame assembly
- `op25/gr-op25-repeater/apps/tk_trbo.py` — trunking state machine implements AABD-C
  registration, affiliation, call grant sequences
- `op25/gr-op25-repeater/apps/trunking.py` — site tracking and RFSS_STS_BCST parsing

---

## Implementation Notes for blip25-mbe

### What AABD-C Provides

- Complete SU-side state machine for trunking (what to send, when, in response to what)
- Complete RFSS-side behavior (what responses are valid and required vs optional)
- Timer and counter values (§17)
- Addressing scheme including all special WUIDs and WGIDs (Annex A)
- Fallback and CCC state machines

### What AABD-C Does Not Provide

- Air interface frame formats (→ BAAA-B for FDMA, BBAB/BBAC for TDMA)
- ISP/OSP PDU field encodings (→ AABC-E)
- TSBK/MBT framing (→ AABB-B)
- Link Control Word field encodings (→ AABF-D)
- Authentication cryptographic details (→ AACE-A)
- Encryption algorithms (→ AAAD-B)
- SNDCP/LLC for packet data (→ BAED-A, BAEB-C)
- ISSI/CSSI inter-RFSS signaling (→ BACA, BACA-B-1)

### Phase 3 Implementation Specs Needed

The following implementation spec update is needed to expose AABD-C content to the
implementer. The existing AABD-B implementation spec at:

  `standards/TIA-102.AABD-B/P25_Trunking_Procedures_Implementation_Spec.md`

should be either updated or replaced with an AABD-C version covering:

1. **Registration/Affiliation state machine** — the U_REG_REQ → U_REG_RSP → GRP_AFF_REQ
   flow with all branch conditions (REG_REFUSED, DENY_RSP, affiliation in emergency mode)

2. **Call setup state machines** — per §7 for group, UU, and PSTN calls; including
   transmission-trunking vs message-trunking continuation method selection

3. **Emergency procedures** — §11 in full; tracking vs non-tracking RFSS distinction;
   CAN_SRV_REQ encoding for emergency alarm and group emergency cancellation; RAE flow

4. **Parameter table** — §17 Table 17-1 in code-ready form (constants with units)

5. **WUID/WGID special address table** — Annex A Table 18-5 and Table 18-6 as Rust
   constants

6. **Conventional Fallback state machine** — Figure 16-1 fallback state diagram;
   LC_CONV_FALLBACK handling logic

7. **CCC procedures** — §14 composite mode detection and state transition

The AABD-B spec should be checked for what is already covered before writing
AABD-C-specific additions.

---

## Draft Change Log (Revision B → Revision C)

Based on editor's note in §11.1 and red text throughout the document:

| Section | Change |
|---------|--------|
| §11.2.4 | New: Remotely Activated Emergency (RAE) procedures |
| §11.2.5 | New: Silent Emergency Activation |
| §11.3.1 | Clarified tracking RFSS vs non-tracking RFSS terminology and state machine |
| §11.3.2 | Extended home/serving RFSS role descriptions with ISSI context |
| §11.3.4 | Clarified CAN_SRV_REQ encoding for group emergency state cancellation |
| §14 | Promoted from Annex G (informative) to normative §14 |
| §8 | Promoted from Annex H (informative) to normative §8 |
| Annex D | Added Figures 18-11 (MFID90 LoPTT) and 18-12 (RAE call sequence) |
| §13.3 | Clarified OTAR zeroize levels (TEK Level 1; TEK + UKEK Level 2) |
| §13.5 | Radio Detach cross-reference added |
| §10.4.5 | Editor's comment EF03 flagging correction to RFSS response table |
| §13.2 | Editor's comment M35 flagging Radio Check cross-reference |
| §13.5 | Editor's comment M35 flagging Radio Detach cross-reference |
| §16 | Editor's comment M40 flagging conventional fallback definition update |
