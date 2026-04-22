# P25 KFD Interface Protocol — TIA-102.AACD-B Delta Implementation Spec

**Source:** TIA-102.AACD-B (October 21, 2025), *Key Fill Device
Interface for Project 25*. Supersedes TIA-102.AACD-A (2014).

**Companion to:** `standards/TIA-102.AACD-A/P25_KFD_Interface_Protocol_Implementation_Spec.md`
— **primary normative reference** for everything that did NOT change
in AACD-B. This delta spec documents only the additions and changes;
read AACD-A's impl spec for the baseline protocol.

**Document type:** delta MESSAGE_FORMAT + PROTOCOL. Extension of AACD-A
adding KMM Forwarding, paired/unpaired authentication procedures, OTAR
provisioning over KFD, Warm Start WSTEK-based transport security, and
an expanded status code table.

**Scope of this delta:**
- §1 — Delta summary table (AACD-A → AACD-B)
- §2 — Version 0 (3WI legacy) vs Version 1 (IP with transport security) taxonomy
- §3 — KF Datagram and three-layer security model (new)
- §4 — 8-step Version 1 exchange procedure (new; replaces AACD-A's 5-step IP)
- §5 — Warm Start and WSTEK lifecycle
- §6 — KMM Forwarding (new, 3 procedures)
- §7 — Paired / Unpaired authentication key procedures (new)
- §8 — OTAR Provisioning over the KFD interface (new)
- §9 — View Active SUID (new)
- §10 — Expanded status code table (Table 45)
- §11 — What stayed identical to AACD-A
- §12 — Cite-to section references
- §13 — Cross-references

**Pipeline artifacts:**
- `standards/TIA-102.AACD-B/TIA-102-AACD-B_Full_Text.md` — clean-room
  extraction (copyrighted, git-ignored).
- `standards/TIA-102.AACD-B/TIA-102-AACD-B_Summary.txt` — retrieval summary.
- `standards/TIA-102.AACD-B/TIA-102-AACD-B_Related_Resources.md`.

---

## 1. Delta Summary

| Area | AACD-A state | AACD-B state | Impl impact |
|------|--------------|--------------|-------------|
| **Version taxonomy** | Implicit (3WI vs IP) | Explicit: **Version 0** (3WI) vs **Version 1** (TCP/UDP with transport security) | Clarifies the interop surface; Version 0 ≡ AACD-A 3WI |
| **Endpoints** | KFD ↔ MR only | **KFD ↔ SU / KMF / AF** | Version 1 adds KMF and AF as targets |
| **Exchange steps (IP path)** | 5 steps | **8 steps** | Adds Warm Start and Begin Session phases |
| **Transport security** | None (inner-layer KEK only) | **Outer-layer WSTEK + MAC** (optional per Warm Start) | New: session-scoped transport crypto |
| **KMM Forwarding** | Not defined | **New** — KFD as intermediary KMF↔SU | New sub-protocol with Key-Fill-Envelope KMMs |
| **Authentication keys** | Load / Delete only | **Load / Delete + Paired / Unpaired + Report + Transfer** | Richer LLA key provisioning |
| **OTAR over KFD** | Not defined | **New** — KFD can provision OTAR material | Bridges KFD-only and KMF-based |
| **Status codes** | Small set | **Expanded (Table 45, ~50 codes)** | More precise failure diagnosis |
| **View Active SUID** | Not present | **New procedure** | Additional LLA introspection |
| **3WI Annex B** | Normative in body | Moved to Annex B | Wire format unchanged |
| **CRC-16-CCITT** | Annex A | Annex A (unchanged) | Same poly, init, test vectors |

**Feature count:** AACD-A had 16 manual rekeying features; AACD-B
expands to **24 application procedures** (key ops, RSI/config,
view/inventory, keyset mgmt, auth keys, key material transfer to KMF,
KMM Forwarding, OTAR Provisioning, Warm Start).

---

## 2. Version 0 vs Version 1

AACD-B introduces an explicit versioning split:

| Aspect | **Version 0** (3WI legacy) | **Version 1** (TCP/UDP, current) |
|--------|-----------------------------|----------------------------------|
| Physical | K/F line, Keyload*, GND | TCP/UDP over USB or Ethernet |
| Endpoints | KFD ↔ SU only | KFD ↔ SU / KMF / AF |
| Transport security | None | **Optional WSTEK + MAC** (Warm Start) |
| Advanced features | No | **KMM Forwarding, OTAR Provisioning, Warm Start** |
| Where spec'd | AACD-B Annex B | AACD-B §3 main body |

**Version 0 ≡ AACD-A 3WI.** The 3WI wire format, opcodes ($C0, $D0,
$C1, $C2, $92, $90), frame layout, CRC, and byte format are carried
forward unchanged. An AACD-A-compliant 3WI implementation is
AACD-B Version 0-compliant without modification.

**Version 1 is the new architecture.** Every delta below applies to
Version 1 unless otherwise noted.

---

## 3. KF Datagram and Three-Layer Security Model (new)

AACD-B introduces the **KF Datagram** as the Version 1 transport
container. Every KMM on the wire is wrapped in a KF Datagram.

### 3.1 KF Datagram structure

```
┌─────────────────────┐
│ KMM Preamble        │
├─────────────────────┤
│ Message ID          │   from AACD-A Table 1 + B additions
├─────────────────────┤
│ Source RSI          │
├─────────────────────┤
│ Destination RSI     │
├─────────────────────┤
│ Message Number (MN) │   new — anti-replay (§3.4)
├─────────────────────┤
│ KMM body            │   possibly inner-layer encrypted per AACA-A
├─────────────────────┤
│ MAC (optional)      │   present after Warm Start; absent in clear
└─────────────────────┘
```

### 3.2 Three security layers

Three **independent** layers can be applied to a KF Datagram:

| Layer | Purpose | Key | Scope |
|-------|---------|-----|-------|
| **Outer (transport)** | Confidentiality of the entire datagram | **WSTEK** (Warm Start TEK, session-scoped) | Whole KF Datagram, including KMM body and header |
| **MAC** | Integrity / authentication of the datagram | Derived from WSTEK | Whole datagram; present when transport protection active |
| **Inner (key material)** | Confidentiality of sensitive KMM fields (TEKs) | **KEK** (per AACA-A inner-layer format) | Specific fields inside the KMM body (e.g., Key field in Modify-Key-Command) |

**Independence.** Inner-layer protection applies **regardless** of
whether Warm Start has established WSTEK — a TEK in transit is
always inner-layer encrypted (per AACA-A). Outer-layer protection is
**optional**, established only after a Warm Start exchange.

### 3.3 Warm Start sequence

Per AACD-B §3 Step 2 of the 8-step procedure:

```
KFD                                  Target (SU / KMF / AF)
 │                                   │
 │ 1. Generate random WSTEK          │
 │ 2. Encrypt WSTEK under target's   │
 │    pre-provisioned UKEK           │
 │    (inner-layer per AACA-A)       │
 │                                   │
 │ ── Warm Start KMM (WSTEK wrapped) ─▶
 │                                   │
 │                                   │ 3. Decrypt WSTEK under UKEK
 │                                   │ 4. Install WSTEK for this session
 │                                   │
 │ ◀─────── Rekey-Acknowledgment ────┤
 │                                   │
 │ 5. Install WSTEK on KFD side      │
 │                                   │
 │ WSTEK now used for outer-layer    │
 │ encryption + MAC on all           │
 │ subsequent KF Datagrams in this   │
 │ session                            │
```

**WSTEK lifecycle:** session-scoped. New WSTEK per connection.
Both parties discard WSTEK at Disconnect.

### 3.4 Anti-replay (Message Number)

Per AACD-B. Every KF Datagram carries a 16-bit Message Number (MN).

- `MNinit` is computed via XOR of a seed and an offset derived from
  session state.
- Each subsequent datagram increments MN (modular 16-bit).
- Receiver rejects datagrams with MN outside the expected window.

---

## 4. Version 1 Exchange: 8 Steps

Replaces the AACD-A 5-step IP exchange:

```
1. Establish Connection    — Session Control: Ready Req / Ready
                              General Mode (unencrypted, no MAC)
                              → WSTEK not yet established
2. Warm Start (optional)   — see §3.3
3. Begin Session           — negotiates session type (Key Fill,
                              KMM Forwarding, etc.)
4. KFD→Target KMMs         — one at a time, each ACK'd
5. Transfer Done (KFD)     — signals end of KFD→Target phase
6. Target→KFD KMMs         — typically empty for keyload-only;
                              Target sends Transfer Done
7. End Session             — End Session / End Session Ack
8. Disconnect              — Disconnect / Disconnect Ack;
                              both parties discard WSTEK
```

**Comparison with AACD-A 5-step:**
- Steps 1, 4, 6, 7, 8 map 1:1 to AACD-A's Ready / Transfer / End /
  Disconnect flow.
- **Steps 2 (Warm Start) and 3 (Begin Session) are new.**

---

## 5. Warm Start and WSTEK Lifecycle

### 5.1 Preconditions for Warm Start

Warm Start requires **the target has a pre-provisioned UKEK**. This
means:

- For SUs: UKEK must have been loaded previously (via an earlier
  KFD session without Warm Start, or factory-provisioned).
- For KMFs and AFs: UKEK must be provisioned via the operator's
  procedures for those devices.

**First-ever-contact problem.** A brand-new SU with no UKEK cannot
do Warm Start — the KFD must use a Version 0 (3WI) path or an IP
session without Warm Start (just the inner-layer KEK protection) for
the initial UKEK install. Once UKEK is loaded, subsequent sessions
can use Warm Start.

### 5.2 WSTEK rotation

WSTEK is generated **randomly** by the KFD at each session start.
This keeps forward secrecy for session content — a past WSTEK
compromise doesn't affect future sessions.

### 5.3 MAC computation

MAC is CBC-MAC (per AAAB-B §4.2 / AAAD-B) over the KF Datagram,
keyed on the WSTEK-derived MAC key. Attached as the last field of
the datagram.

Any datagram with a failing MAC check is dropped. No response is
required (sender will observe via higher-layer timeout).

---

## 6. KMM Forwarding (new)

AACD-B introduces **three** new sub-procedures for the case where the
KFD acts as an intermediary between a KMF and an SU that has no
direct network path to the KMF.

### 6.1 Architecture

```
 KMF ◀───KFD Session A───▶ KFD ◀───KFD Session B───▶ SU
  │                                                   │
  │         Logical key material flow                 │
  ●──────────────────────────────────────────────────▶●
```

### 6.2 Three procedures

| Procedure | Direction | Purpose |
|-----------|-----------|---------|
| **KMM Forwarding (KFD↔KMF)** | Session A | KFD fetches KMF-originated key material for later delivery |
| **KMM Forwarding (KFD↔SU)** | Session B | KFD delivers the cached material to the SU |
| **KMM Forwarding (KFD↔KMF completion)** | Session A resumed | KFD returns SU's response to the KMF |

### 6.3 Envelope KMMs

Three new KMMs implement this:

- **Key-Fill-Envelope-Command** — wraps a target KMM destined for the
  next hop.
- **Key-Fill-Envelope-Response** — returned by the immediate receiver
  to acknowledge the envelope receipt.
- **Key-Fill-Envelope-Report** — carries the target's eventual
  response back along the chain.

Each envelope carries a **sequence number** to correlate the three
procedures across the two separate KFD sessions.

### 6.4 WSTEK independence

Each of the 3 sessions uses its own WSTEK (fresh Warm Start in each
leg). The KFD's envelope wrapping operates on the inner KMM as
opaque; it does not need to decrypt the inner content.

**Implementation note.** AACD-B Annex C provides a worked example of
KMM Forwarding with the full message flows. Implementers should
read Annex C before attempting to build KMM Forwarding support —
the sequence-number correlation is subtle and mis-correlation
silently breaks the end-to-end key delivery.

---

## 7. Paired / Unpaired Authentication Key Procedures (new)

AACD-A supported Load / Delete Authentication Key. AACD-B adds:

- **Load and Report Paired Auth Keys** — loads a matched pair (e.g.,
  K and K') atomically and returns a report.
- **Transfer Unpaired Auth Keys** — moves a single-sided auth key
  between KFDs (operator-to-operator hand-off without reloading the SU).
- **Transfer Paired Auth Keys** — same, for pairs.
- **Report Delete Auth Key** — emits a report when an auth key is
  deleted (audit trail).

**Use case.** Multi-KFD agencies where different operators hold
different halves of auth material; paired procedures keep the two
halves synchronized.

---

## 8. OTAR Provisioning over the KFD Interface (new)

AACD-B adds a new application procedure that lets a **KFD deliver
OTAR KMMs to an SU over the KFD interface**, without using the air
interface. Use case: provisioning a radio for OTAR before
deployment, when the target RFSS isn't reachable.

**How it works:**
1. The KFD holds OTAR KMMs generated by or delivered from a KMF.
2. A KFD↔SU session is opened.
3. Begin Session indicates OTAR Provisioning session type (§4 step 3).
4. OTAR KMMs flow KFD→SU as normal KMMs with their OTAR Message IDs.
5. The SU processes them as if received over the air.

**Effect.** The SU's OTAR state (UKEK, RSI, MNP, KMF RSI) is
populated without any RF exchange. The SU can then immediately join
the OTAR flow on first boot.

---

## 9. View Active SUID (new)

Previously (AACD-A), SUID introspection was via **List Active SUID**
Inventory (Inventory Type `$F7`). AACD-B promotes this into an
explicit application procedure: **View Active SUID**. The wire
message is the same Inventory-Command / Response pair; the change is
that View Active SUID is now a top-level named procedure in the
feature list rather than just an Inventory variant. No wire-level
implementation change.

---

## 10. Expanded Status Code Table (Table 45)

AACD-B Table 45 catalogs ~50 status codes (`$00`–`$FF` range). A
representative sample:

| Code | Meaning |
|------|---------|
| `$00` | Success / operation completed |
| — | Key not found |
| — | Storage full |
| — | Algorithm unsupported |
| — | RSI not found |
| — | Various other conditions |

**Implementation consequence.** Decoders / parsers should treat
status codes as an open enumeration: map known values, log unknown
values without failing. AACD-B reserves much of the `$00`–`$FF`
range for future use.

**AACD-A equivalence.** AACD-A had a smaller subset of status codes
(mostly generic Success / Fail / Unknown). AACD-B's expansion is
additive — old codes retain their meaning; new codes enable finer
diagnosis.

---

## 11. What Stayed Identical to AACD-A

Read AACD-A impl spec for these (unchanged in AACD-B):

- **3WI wire format** (Annex B): opcodes, frame layout, byte framing,
  parity, K/F line electrical, key signature timing, 512-byte max
  frame, 4 kbps rate.
- **CRC-16-CCITT** (Annex A): polynomial, init value, byte-swap on
  transmission, test vector ($DE $AD → wire $7C4B).
- **KIT (Key Fill Inactivity Timer)**: default 15s, min 5s, max 30s,
  reset on each outbound KMM.
- **KMM Header** structure: Message ID, Length, Message Format
  (Rsp / MN / MAC bits), Source/Destination RSI.
- **Destination RSI `$FFFFFF`** broadcast rule: MR must accept.
- **Modify-Key-Command** format (Table 11 in AACD-A): Extended
  Decryption Instruction, MI conditional presence, Unique Key Item
  SEQUENCE with Key Format / SLN / Key ID / Key / optional Checksum
  / optional Key Name. **Unchanged in AACD-B.**
- **DES key format (Table 9)** and **block-cipher key format (Table 10)**.
- **All 33 AACD-A KMMs** — AACD-B uses them all; some (Unable-to-
  Decrypt) remain 3WI-unsupported.
- **Keyset initialization rules**: first-loaded = auto-active;
  subsequent loads = enabled-not-active until Changeover; at most
  one active keyset.
- **UDP port 49165** (from BAJD-A) is unchanged for the Version 1
  IP path.

---

## 12. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- Version 0 / Version 1 taxonomy — TIA-102.AACD-B §2.
- KF Datagram structure — §3.
- 8-step Version 1 exchange procedure — §3.3–§3.9.
- Warm Start and WSTEK lifecycle — §3.5.
- MAC computation — §3.6.
- Message Number anti-replay — §3.7.
- KMM Forwarding — §3.7.22, Annex C (worked example).
- Paired / Unpaired authentication procedures — §3.7.17–§3.7.19.
- Load and Report Paired Auth Keys — §3.7.18.
- OTAR Provisioning over KFD — §3.7.23.
- View Active SUID — §3.7.13.
- Status code table — Table 45.
- Three Wire Half Duplex Interface — Annex B (unchanged from AACD-A).
- CRC-16-CCITT — Annex A (unchanged).

---

## 13. Cross-References

**Upstream (this delta depends on):**
- `standards/TIA-102.AACD-A/P25_KFD_Interface_Protocol_Implementation_Spec.md`
  — **primary normative baseline**. Read together with this delta.
- `standards/TIA-102.AAAB-B/P25_Security_Services_Overview_Implementation_Spec.md`
  — security-taxonomy context.
- **AACA-A** — OTAR Messages and Procedures. **AACD-B OTAR
  Provisioning over KFD** relays AACA-A KMMs through the KFD interface
  — the KMM bodies are still AACA-A formats.
- **AACA-D** — current OTAR protocol impl spec in the repo (AACA-C
  superseded). Same caveat as above.
- `standards/TIA-102.AACE-A/P25_Link_Layer_Authentication_Implementation_Spec.md`
  — SUID, K/K' pair structure, LLA authentication keys.
- `standards/TIA-102.BAJD-A/P25_Port_Number_Assignments_Implementation_Spec.md`
  — UDP port 49165 for KFD Version 1.
- `standards/TIA-102.BAEG-A/P25_Mobile_Data_Peripheral_Interface_Implementation_Spec.md`
  — PPP over RS-232 / USB link layer for Version 1 IP path.

**Supporting annex table:**
- `annex_tables/aacd_kmm_catalog.csv` — the 33-row AACD-A KMM catalog
  (carried forward in AACD-B). New AACD-B-only KMMs (Key-Fill-
  Envelope Command/Response/Report, OTAR Provisioning session type,
  Warm Start wrappers) can be appended when BAJC-A-style Phase 3
  extension work is done.

**Related analysis:**
- None. If implementer work uncovers a gap in Warm Start, KMM
  Forwarding, or OTAR Provisioning integration, `analysis/` is the
  right home for a clarification note.
