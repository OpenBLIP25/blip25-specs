# P25 Telephone Interconnect (Voice Service) — TIA-102.BADA-A + BADA-1 Implementation Spec

**Sources:**
- **TSB-102.BADA-A** (June 2012) — *Telephone Interconnect Overview*.
  Informative TIA Systems Bulletin (not normative). Lineage: IS
  102.BADA (1996) → TIA/EIA-102.BADA (2000) → TSB-102.BADA-A (2012).
- **ANSI/TIA-102.BADA-1** (May 2006) — *Addendum 1: Conventional
  Individual Calls*. Normative addendum to the TIA/EIA-102.BADA
  baseline. Adds availability checking, call-progress signaling, and
  structured call termination for conventional channels.

This impl spec **combines the two** because they're tightly coupled:
BADA-A defines the architecture and trunked-side procedures; BADA-1
extends it for conventional channels. An implementer needs both to
decode or build a complete telephone-interconnect stack.

**Document type:** PROTOCOL + PROCEDURE. Telephone interconnect lets
a P25 SU place / receive calls to / from the PSTN (Public Switched
Telephone Network) or a PABX (private branch exchange). The
interconnect bridge lives in the RFSS at the **Et interface** (RFSS
↔ telephone network).

**Scope of this derived work:**
- §1 — Architecture and the Et interface
- §2 — Three dialing modes (Buffered / Live Key / List)
- §3 — Outgoing call flow (SU → PSTN)
- §4 — Incoming call flow (PSTN → SU) including availability check
- §5 — Group incoming calls
- §6 — Call termination and the AABF call-tear-down LC
- §7 — Class of service catalog
- §8 — Et interface access methods (analog, ISDN, VoIP)
- §9 — Trunked vs conventional conformance matrix
- §10 — Vocoder considerations (full-rate distorts tones, half-rate carries them)
- §11 — Wire-message inventory (referenced from AABC / AABG / AABF)
- §12 — Cite-to section references
- §13 — Cross-references

**Pipeline artifacts:**
- `standards/TIA-102.BADA-A/TIA-102-BADA-A_Full_Text.md` — TSB clean-room
  extraction (copyrighted, git-ignored).
- `standards/TIA-102.BADA-A/TIA-102-BADA-A_Summary.txt` — retrieval summary.
- `standards/TIA-102.BADA-A/TIA-102-BADA-A_Related_Resources.md`.
- `standards/TIA-102.BADA-1/TIA-102-BADA-1_Full_Text.md` — addendum
  clean-room extraction.
- `standards/TIA-102.BADA-1/TIA-102-BADA-1_Summary.txt` — addendum summary.
- `standards/TIA-102.BADA-1/TIA-102-BADA-1_Related_Resources.md`.
- `annex_tables/bada_telephone_interconnect_features.csv` — full
  Mandatory / Standard Option / Optional matrix per role × mode.

---

## 1. Architecture and the Et Interface

```
                      Telephone Interconnect Architecture
┌──────────────┐  Um  ┌────────────────┐  Et  ┌─────────────────┐
│ Subscriber   │◀────▶│ RFSS (RF Sub-  │◀────▶│ PSTN / PABX     │
│ Unit (SU)    │      │ system)        │      │                 │
└──────────────┘      └────────────────┘      └─────────────────┘
   - dial / answer       - bridge          - call-progress tones
   - hookflash           - DTMF decode      - answer supervision
   - PTT                 - voice transcode  - hook events
```

**The Et interface** is the wire boundary between the RFSS and the
telephone network. It carries all PSTN-side signaling and audio. Per
BADA-A §4 it can be implemented via several methods (see §8 below);
BADA-A is silent on which one is required — it's an integrator
choice.

**Three over-the-air interfaces** are involved:
- **Um** (FDMA Common Air Interface) — voice + signaling between SU
  and RFSS.
- **Um2** (TDMA, Phase 2) — same, on TDMA voice channels. BADA-A
  §1.4 acknowledges Um2 but defers normative TDMA-side detail to
  BBAD-A.
- The **Et interface itself** — wireline to the PSTN.

**What BADA does NOT define:**
- **Roaming** across multiple RFSSs — BADA-A §1.1 explicitly out of scope.
- **Ergonomic / UI** aspects of the SU — out of scope.
- Per-message wire formats — those live in **AABC** (trunked control
  channel messages), **AABG** (conventional control messages), and
  **AABF** (link control words). BADA references them by name; the
  byte-level encoding is read from those companion specs.

---

## 2. Dialing Modes

Per BADA-A §2.1.2 + Annex A:

| Mode | How it works | Conformance |
|------|--------------|-------------|
| **Buffered Mode** | User pre-enters all digits, then presses send. SU transmits the complete digit string in one over-air message. | **Mandatory on trunked** SUs |
| **Live Key Mode** | Each keypress immediately triggers an over-air message. Sidetone may be generated locally. | Optional |
| **List Mode** | User selects from a preprogrammed list; SU sends the selected entry. | Optional |
| **Implicit Dialing** | "Speed dial" — single keypress / softkey triggers a preprogrammed number. | Standard Option (trunked) / Optional (conventional) |

**Buffer length:** 1 to 34 digits, accommodating international calls
through PABX circuits with external access codes.

**Decoder note.** A dialed-number string carried over the air is
typically wrapped in a **TIA-102.AABC Telephone Interconnect Request**
TSBK (trunked) or the equivalent **AABG** message (conventional). The
digit string is encoded as 4-bit BCD nibbles per AABC; long numbers
fragment across multiple TSBKs. The exact field width comes from
AABC-E, not BADA.

---

## 3. Outgoing Call Flow (SU → PSTN)

### 3.1 Trunked

```
SU                          RFSS                       PSTN
 |---- TELE_INT_REQ ------->|                           |  TSBK on CC
 |                          |  authorize, check class   |
 |<-- TELE_INT_V_CH_GRANT --|                           |  TSBK on CC, allocates TCH
 |                          |---- offhook --------------->|
 |                          |---- DTMF (dialed digits)  ->|
 |                          |<--- ringback / busy / etc -|
 |  (SU now on TCH)         |                           |
 |<====== SPEECH ===========|<======= SPEECH ===========>|
```

Trunked outgoing flow per BADA-A §3.8 and AABD-B procedures: SU
sends `TELE_INT_REQ` (or the dialed-number variant), RFSS authorizes
the call, allocates a traffic channel, sends `TELE_INT_V_CH_GRANT`,
SU tunes to the working channel. RFSS bridges Et and emits the call
to the PSTN.

### 3.2 Conventional (with BADA-1 enhancements)

```
SU                          RFSS                       PSTN
 |---- TELE_INT_REQ ------->|                           |  conventional CC
 |                          |---- offhook ------------->|
 |                          |---- DTMF digits --------->|
 |   (BADA-1 optional response signaling:)              |
 |<--- TELE_INT_ACK_RSP ----|                           |  Acknowledge
 |    (or Queued/Deny)      |                           |
 |                          |<--- ringback tone -------|  RFSS may relay to SU
 |<-(audio: ringback)-------|                           |
 |                          |<--- ANSWER -------------- |
 |<====== SPEECH ===========|<======= SPEECH ==========>|
```

BADA-1 §3.1 adds the optional **Acknowledge / Deny / Queued**
response signaling on conventional, so an SU has a clean, structured
indication of call-progress state instead of just receiving raw audio.
Per §2.1 the SU **must** fall back to the audio-only behavior of the
baseline if no signaling is received — interoperability with legacy
RFSSs is preserved.

---

## 4. Incoming Call Flow (PSTN → SU)

### 4.1 Availability Check Procedure (BADA-1 §3.3, BADA-A §3.9)

For individual incoming PSTN calls, the RFSS may perform an
availability check before connecting the audio. This avoids ringing
the PSTN through to nobody when the SU is offline / out of range /
user-rejecting.

**Wire sequence (per BADA-1 Annex B Figure B.1):**

```
SU                      RFSS                    PSTN
 |                        |<-- PSTN call alert---|
 |<-- TELE_INT_ANS_REQ ---|                      |  RFSS asks SU "ringing?"
 |                        |--- ringback tone --->|  RFSS holds caller with ringback
user answers / rejects:
 |--- TELE_INT_ANS_RSP -->|                      |  Proceed | Deny
                                                    │
                                                    ├─ Proceed → connect, signal ANSWER
                                                    │
                                                    └─ Deny / 1-second timeout →
                                                       route to tone / announcement /
                                                       voicemail / operator
                                              ┌─────┴─────┐
 |<====== SPEECH =========|======= SPEECH ====>           |
PTT press:
 |--- LC_TELE_INT_V_CH_USR -->|                  |  per-burst LC on TCH
 |<-- LC_TELE_INT_V_CH_USR ---|                  |
 |<====== SPEECH =========|======= SPEECH =====>|
```

**Timer:** RFSS waits **1 second** after sending `TELE_INT_ANS_REQ`
before abandoning the availability check (BADA-1 §3.3.1).

**Interoperability rule (BADA-1 §3.3.2):** RFSS must be configurable
to **disable** availability checks for SUs that don't support them —
otherwise legacy SUs would never receive incoming calls. Operator
provisions per-SU which mode applies.

**Treatment on Deny / timeout:** RFSS connects the PSTN side to a
"tone signal, recorded announcement, operator position, or voice
mail facility as may be appropriate" — per regional regulatory norms
(BADA-1 §3.3.1).

### 4.2 Direct Inward Dial (DID) vs Overdial

Per BADA-A §3.9:

| Method | How it routes | RFSS answer behavior |
|--------|---------------|----------------------|
| **DID** | PSTN number maps to a specific SU via a dedicated phone-number-per-SU pool | RFSS plays ringback to PSTN until SU responds (consistent with availability check) |
| **Overdial** | RFSS answers the PSTN call **immediately**, plays an audible prompt, then collects additional digits to map to a specific SU | RFSS signals ANSWER to PSTN at first ring — caller pays from that point regardless of SU availability |

Overdial is common where DID number space is scarce (single trunk
serving many SUs).

---

## 5. Group Incoming Calls

Per BADA-A §3.10 and BADA-1 Table A.1: a PSTN call may be routed to
a talkgroup; **all group members alert** and any member can answer.
An audible alert distinguishes the call from normal group traffic
(BADA-A §2.3).

| Mapping | Method |
|---------|--------|
| DID phone number → talkgroup | RFSS provisioning maps a specific PSTN number to a specific group ID |
| Default group per telephone line | RFSS may have a "default talkgroup for this PSTN line" config |

For trunked systems, the air-interface signaling per AABC announces
the call to the group; for conventional, AABG carries the equivalent
notification.

---

## 6. Call Termination

Per BADA-A §2.5 / §3.14 + BADA-1 §3.4:

### 6.1 Termination triggers

Three causes:

1. **SU user terminates** — over-air signaling, or preprogrammed
   DTMF disconnect code.
2. **PSTN side terminates** (far-end disconnect) — RFSS detects
   on Et and tears down toward SU.
3. **RFSS-initiated** — programmable air-time-conserving timer
   expires (BADA-A §3.14 last paragraph).

### 6.2 The Cancel Service Request path (trunked + BADA-1 conventional)

The cleanest user-initiated termination is the **Cancel Service
Request** message. **Mandatory on trunked SUs**; **optional on
conventional SUs per BADA-1 §2.3.1**. Conventional SUs that send
Cancel Service Request must also retain backward-compatible support
for the legacy DTMF disconnect code (§2.3.2).

### 6.3 RFSS tear-down: the AABF Call Termination LC

Per BADA-1 §3.4.1 and BADA-A §3.14.3: when the RFSS disconnects, it
**must** transmit a **Call Termination / Cancellation** Link Control
Word per **TIA-102.AABF §6.3.8** (BADA-1 cites the older AABF §
numbering; in current AABF-D this is `LC_CALL_TERMINATION` —
verify the exact opcode in
`standards/TIA-102.AABF-D/P25_Link_Control_Word_Implementation_Spec.md`).

The LC is sent **on the traffic or conventional channel** — it
notifies the SU (and any monitoring receivers) that the call is
ending. For conventional operation, BADA-1 §3.4.1 allows an
**audible confirmation tone** to precede the LC.

### 6.4 Programmable timer

Both trunked and conventional RFSSs may implement an **air-time
conservation timer**. When it expires, the RFSS:

1. (Optionally) emits an audible "impending disconnect" warning.
2. Tears down the call.
3. Sends the AABF Call Termination LC.

Timer duration is operator-configurable; not normatively specified.

---

## 7. Class of Service Catalog

Per BADA-A §3.5. Configurable on a **per-subscriber basis** (trunked)
or **RFSS-wide basis** (conventional, primarily — see Note 5):

| Class | Effect |
|-------|--------|
| **Restrict Toll Calls** | SU may not place toll-rated calls (long distance / international) |
| **Speed Dialing** | SU has access to RFSS-stored short-code → number mappings (Implicit Dialing) |
| **Outgoing Only** | SU can place calls but not receive PSTN-originated calls |
| **Incoming Only** | SU can receive PSTN calls but not originate |
| **Speed Dial Override** | Allows the SU to use speed-dial entries even when other restrictions would apply |
| **Hookflash** | SU may signal a 200–800 ms hookflash to the PSTN/PABX (for supplementary services) |

**Hookflash** is the **only mandatory advanced feature** for
SUs across both trunked and conventional (BADA-1 Table A.1).
Hookflash duration: **200–800 ms on-hook pulse at the Et point**
(BADA-A §2.4) — outside that range the PSTN/PABX may not recognize
it as a hookflash.

**Speed dialing** is satisfied by SUs supporting only Implicit
Dialing (BADA-A §3.5).

---

## 8. Et Interface Access Methods

Per BADA-A §4. **The standard does not mandate a specific method** —
all of the below are example implementations:

| Family | Method | Reference |
|--------|--------|-----------|
| **Analog** | 4-Wire E&M | TIA/EIA-464 |
| Analog | 2-Wire E&M | TIA/EIA-464 |
| Analog | 2-Wire Direct Inward Dial (DID) | TIA/EIA-464 |
| **Digital ISDN** | BRI (2B+D) | ITU-T standards |
| Digital ISDN | PRI (24B+D North America, 30B+D elsewhere) | ITU-T standards |
| **VoIP** | SIP for call control | RFC 3261 |
| VoIP | SDP for session description | RFC 4566 |
| VoIP | RTP for voice transport | RFC 3550 |

**Implementation consequence.** A blip25-style passive Um decoder
**does not see Et interface traffic at all**. The Et interface is
internal to the RFSS deployment. What's visible on Um is the
trunked TSBK or conventional control message traffic that initiates
or terminates the call, plus the voice channel itself once a TCH
grant lands.

---

## 9. Trunked vs Conventional Conformance Matrix

Full machine-readable matrix:
**`annex_tables/bada_telephone_interconnect_features.csv`** (39 rows
across SU and RFSS roles).

**Headline differences:**

| Feature | Trunked | Conventional |
|---------|---------|--------------|
| Voice Channel Grant (RFSS sends, SU acts) | **Mandatory** | Not Applicable |
| Cancel Service Request (SU sends) | **Mandatory** | Optional |
| Telephone Interconnect Answer Request (RFSS sends) | Standard Option | Optional (added by BADA-1) |
| Group Incoming Call | Standard Option | **Mandatory** |
| Hookflash | Standard Option | **Mandatory** |
| Class-of-Service items (toll restrict, speed dial, …) | Standard Option per-SU | **Mandatory** RFSS-wide |
| 2-wire loop start at Et | Standard Option | **Mandatory** |
| Overdial generation (RFSS) | Standard Option | **Mandatory** |
| Buffered Mode dialing (SU) | **Mandatory** | See note A.3.2 (deferred) |

The pattern: **trunked makes signaling and per-SU per-feature
control mandatory** (because you have a controller anyway).
**Conventional makes group support and the analog loop-start
interface mandatory** (because conventional sites are typically
single-line, group-call-focused).

---

## 10. Vocoder Considerations

Per BADA-A §3.12 and BADA-1 §3.1.2. Two important caveats:

### 10.1 Full-rate IMBE vocoder (TIA-102.BABA)

- **Distorts or blocks** call-progress tones (DTMF, ringback, busy)
  relayed through the system.
- The RFSS should **prefer structured signaling** (ACK / Deny /
  Queued / explicit ANSWER) over relayed tones whenever possible.
- DTMF for **inbound overdial** must be decoded **at the RFSS** —
  the RFSS needs a dedicated DTMF decoder; the vocoder won't
  preserve the tones for downstream decoding.

### 10.2 Half-rate AMBE+2 vocoder (TIA-102.BABA-1)

- **Carries tones as signaling**, not as speech audio.
- DTMF and call-progress tones survive the vocoder pipeline cleanly.
- An RFSS using half-rate vocoder may **omit the dedicated DTMF
  decoder** (BADA-A §3.12) — the vocoder side-channel handles it.

**Decoder consequence.** A passive decoder that wants to detect
DTMF in voice traffic on a Phase 2 (half-rate) system may be able to
do it via vocoder signaling-channel inspection; on Phase 1
(full-rate) the tones won't be cleanly recoverable from voice.

---

## 11. Wire-Message Inventory

BADA-A and BADA-1 reference messages by name; the wire format is
defined in companion specs. This table maps the BADA-named messages
to where their byte-level layout lives:

| BADA-named message | Trunked spec | Conventional spec | Type |
|--------------------|--------------|--------------------|------|
| Telephone Interconnect Request (with dialed digits) | AABC-E | AABG / AABG-1 | TSBK / Conventional control message |
| Telephone Interconnect Voice Channel Grant | AABC-E | (N/A — conventional doesn't grant) | TSBK |
| Acknowledge Response | (N/A) | AABG-1 | Conventional control message (BADA-1) |
| Deny Response | AABC-E | AABG-1 | TSBK / conv. control |
| Queued Response | AABC-E | AABG-1 | TSBK / conv. control |
| Telephone Interconnect Answer Request | AABC-E | AABG-1 | TSBK / conv. control |
| Telephone Interconnect Answer Response (Proceed/Deny) | AABC-E | AABG-1 | TSBK / conv. control |
| Cancel Service Request | AABC-E | AABG-1 | TSBK / conv. control |
| `LC_TELE_INT_V_CH_USR` (per-burst on TCH) | AABF-D | AABF-D | Link Control Word |
| Call Termination / Cancellation LC | AABF-D §6.3.8 | AABF-D §6.3.8 | Link Control Word |

**Decoder dispatch:** for trunked traffic, route `TELE_INT_*` opcodes
through the AABC-E TSBK dispatch (see
`standards/TIA-102.AABC-E/P25_Trunking_Control_Channel_Messages_Implementation_Spec.md`).
For conventional traffic, route through AABG / AABG-1. For voice-channel
traffic during an interconnect call, the LC bursts on the TCH carry
`LC_TELE_INT_V_CH_USR` (see AABF-D impl spec).

**Note on AABG-1.** TIA-102.AABG-1 (Conventional Control Messages
Addendum 1: Individual Telephone Calls) is the wire-level companion to
BADA-1 — it defines the Telephone Interconnect Answer Request /
Response, Acknowledge / Deny / Queued response, and Cancel Service
Request messages for conventional channels. The repo has AABG (the
base) processed; AABG-1's processing status lives in `specs.toml`
under `TIA-102.AABG-1`.

---

## 12. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

**BADA-A (overview / TSB):**
- Architecture and Et interface — §1.4 / §4.
- Three dialing modes — §2.1.2.
- 34-digit dialed-number buffer — §2.1.2.
- SU service request — §2.1.
- SU receiving calls (trunked + conventional) — §2.3.
- Hookflash semantics (200–800 ms) — §2.4 / §2.5.1.
- Three duplex modes (Full / Half / Simplex) — §3.1–§3.3.
- Authorization and roamers — §3.4.
- Class of Service catalog — §3.5.
- Conventional vs trunked operation — §3.7 / §3.8.
- Incoming calls (DID, Overdial) — §3.9.
- Group incoming calls — §3.10.
- Call termination — §3.14.
- Et interface access methods — §4.

**BADA-1 (conventional addendum, normative):**
- SU service request response signaling — §2.1.
- SU availability check on conventional — §2.2.
- SU call termination on conventional (Cancel Service Request) — §2.3.
- RFSS service request response signaling — §3.1.
- RFSS call termination by radio user — §3.2.
- RFSS availability check (1-second timer) — §3.3.
- RFSS-initiated call termination + AABF §6.3.8 LC — §3.4.
- Updated SU and RFSS conformance tables — Annex A.
- Incoming call message sequence (informative) — Annex B Figure B.1.

---

## 13. Cross-References

**Upstream (this doc depends on):**
- `standards/TIA-102.AABA-B/P25_Trunking_Overview_Implementation_Spec.md`
  — defines RFSS, Um, Et terminology used here.
- `standards/TIA-102.AABC-E/P25_Trunking_Control_Channel_Messages_Implementation_Spec.md`
  — wire formats for trunked TELE_INT TSBKs.
- `standards/TIA-102.AABD-B/P25_Trunking_Procedures_Implementation_Spec.md`
  — trunked telephone interconnect procedures (§7.4–§7.6 in AABD-B
  / AABD-C draft).
- `standards/TIA-102.AABF-D/P25_Link_Control_Word_Implementation_Spec.md`
  — `LC_TELE_INT_V_CH_USR` and `LC_CALL_TERMINATION` byte layouts.
- `standards/TIA-102.AABG/…` — conventional control messages base.
- *(pending)* `standards/TIA-102.AABG-1/…` — conventional addendum
  carrying the BADA-1 wire messages (Answer Request/Response, ACK,
  Cancel Service Request).
- `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` —
  full-rate vocoder; relevant for the tone-distortion caveat in §10.

**Companion specs:**
- `standards/TIA-102.AABD-C/…` — AABD-C draft promotes telephone
  interconnect into the unified procedures spec; the same flows
  documented here are normative there.

**Supporting annex table:**
- `annex_tables/bada_telephone_interconnect_features.csv` — full
  Mandatory / Standard Option / Optional matrix per role × mode.

**External references:**
- TIA/EIA-464 — analog Et interface (E&M, DID).
- ITU-T ISDN — BRI / PRI digital Et options.
- IETF **RFC 3261** (SIP), **RFC 4566** (SDP), **RFC 3550** (RTP) —
  VoIP Et options.
