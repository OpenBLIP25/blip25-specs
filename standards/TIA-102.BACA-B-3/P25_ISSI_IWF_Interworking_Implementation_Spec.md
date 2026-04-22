# P25 ISSI — IWF Interworking (TIA-102.BACA-B-3) Implementation Spec

**Source:** TIA-102.BACA-B-3 (April 2021), *ISSI Messages and
Procedures — Addendum 3: Interworking with an IWF*. Normative
addendum to **TIA-102.BACA-B** (November 2012). Developed jointly by
TIA TR-8.19 and the ATIS WTSC JLMRLTE study group (2019–2021).

**Document type:** delta / addendum. **Entire substantive content is a
single new Annex J of BACA-B**. Every other section of the parent
BACA-B spec is explicitly carried forward unchanged ("No change from
[102BACA]"). This impl spec is therefore focused narrowly on the IWF
interworking delta.

**Companion:** the base TIA-102.BACA-B (ISSI Messages and Procedures)
and its earlier addenda B-1 (Group Emergency Cancellation, 2013) and
B-2 (2016) are **not processed in this repo yet** as of this pass.
Read this delta spec alongside the BACA-B full-text extraction when
available.

**Scope of this delta spec:**
- §1 — What an IWF is and why this addendum exists
- §2 — IWF as an RFSS peer on the G interface
- §3 — Functional roles the IWF takes (and doesn't take)
- §4 — Vocoder mode negotiation on the ISSI
- §5 — Call model applicability: group calls and unit-to-unit
- §6 — Mobility management applicability (group only)
- §7 — SIP / SDP / RTP applicability
- §8 — Out-of-scope items explicitly flagged for further study
- §9 — Cite-to section references
- §10 — Cross-references

**Pipeline artifacts:**
- `standards/TIA-102.BACA-B-3/TIA-102-BACA-B-3_Full_Text.md` —
  clean-room extraction (copyrighted, git-ignored).
- `standards/TIA-102.BACA-B-3/TIA-102-BACA-B-3_Summary.txt` — retrieval
  summary.
- `standards/TIA-102.BACA-B-3/TIA-102-BACA-B-3_Related_Resources.md`.

---

## 1. What an IWF Is

**IWF** = 3GPP Mission Critical Push-to-Talk **Interworking Function**.
A gateway that bridges between:

- A **P25 (TIA-102) ISSI** system — RFSS ↔ RFSS over IP.
- A **3GPP Mission Critical service** — LTE / 5G MCPTT users per
  3GPP Rel-14+.

```
┌──────────────┐     G-interface (ISSI)     ┌────────────┐
│ TIA-102 RFSS │◀──────────────────────────▶│    IWF     │
└──────────────┘                             └─────┬──────┘
                                                   │
                                                   │ IWF-x reference point
                                                   │ (out of scope)
                                                   ▼
                                             ┌─────────────┐
                                             │ 3GPP MC     │
                                             │ System      │
                                             │ (LTE / 5G)  │
                                             └─────────────┘
```

**Design decision.** Rather than redesigning the ISSI, the IWF
**presents itself to the TIA-102 RFSS as a standard RFSS peer on the
G interface**. From the TIA-102 side, IWF traffic looks exactly like
peer RFSS traffic — with specific constraints documented in this
addendum.

**Internal IWF structure and the IWF-x interface** (how the IWF talks
to the 3GPP MC side) are **explicitly out of scope**.

---

## 2. IWF as an RFSS Peer on the G Interface

Per BACA-B-3 Annex J reference architecture. Five topology figures
are provided; key ones:

| Topology | Pattern |
|----------|---------|
| 1 | Basic RFSS ↔ IWF over ISSI |
| 2 | RFSS with embedded consoles ↔ IWF over ISSI |
| 3 | Console Subsystems ↔ IWF over CSSI |
| 4 | Combined: both RFSS-embedded consoles and external CSS |
| 5 | Multi-LMR combined topology |

**Implementation rule for P25 equipment:** treat the IWF as a peer
RFSS. All SIP / SDP / RTP handshakes work as they would with another
P25 RFSS, subject to the field / header / procedure constraints in
this addendum.

---

## 3. IWF Functional Roles

The IWF assumes **some** of the ISSI functional roles defined in
BACA-B, but not all. This is the key decision tree for what works
when IWF is the peer.

### 3.1 Roles the IWF **DOES** take on

| Role | Why it applies |
|------|----------------|
| **GMMF** (Group Mobility Management Function) | Group affiliation and tracking; group supplementary data interest |
| **UCCF** (Unit Call Control Function) | **Calling Home and Called Home roles only** — SU-to-SU call setup/teardown. Unit *roaming* roles are not supported |
| **GCCF** (Group Call Control Function) | Full scope including Group Emergency Cancellation (per B-1 addendum) |
| **MMF / SMF** (Master Media Function / Subordinate Media Function) | Talk-spurt arbitration and RTP fanout |
| **RCPF** (RFSS Service Capability Polling Function) | **Mandatory** response to SIP OPTIONS |

### 3.2 Roles the IWF **does NOT** take on

| Role | Why it doesn't apply |
|------|---------------------|
| **UMMF** (Unit Mobility Management Function) | Unit roaming is out of scope |
| **PDMMF** (Packet Data Mobility Management Function) | Packet Data interworking is out of scope (see BACF) |
| **Calling Serving RFSS** / **Called Serving RFSS** | Unit roaming out of scope — these roles only arise during SU roam |

**Implementation consequence.** An SU cannot physically "roam into"
the IWF system and have its state migrate there. Interoperability
happens at the call / group level, not the SU-mobility level.

---

## 4. Vocoder Mode Negotiation

The hardest interop problem. TIA-102 supports two vocoders
(FDMA Full Rate = **IMBE**, TDMA Half Rate = **AMBE+2**); 3GPP MCPTT
typically uses **AMR**. The IWF must negotiate vocoder mode on a
**talk-spurt-by-talk-spurt basis** on the ISSI.

### 4.1 Negotiation modes per BACA-B-3 Annex J

| Mode | Meaning |
|------|---------|
| **Full Rate** | IMBE only; for FDMA-terminated calls |
| **Half Rate** | AMBE+2 only; for TDMA-terminated calls |
| **Native** | IWF selects based on the destination path |

### 4.2 Deferred to further study

BACA-B-3 explicitly flags as pending JLMRLTE study:

- Full implications of **transcoding** between AMBE (P25) and AMR
  (3GPP).
- **Trans-encryption** between P25 (AAAD-B) and MCPTT (3GPP security)
  key spaces.
- Vocoder mode negotiation *limitations* when the peer RFSS is an
  IWF (specific scenarios not yet fully characterized).

**Operational consequence.** Early IWF deployments should either
constrain to a single vocoder family end-to-end or provide clear
transcoding boundaries at the IWF. Mixed-vocoder calls across
TIA-102 and 3GPP are a known fragility point.

---

## 5. Call Model Applicability

### 5.1 Group calls

**Fully supported.** The IWF may act as:
- **Home RFSS** for groups homed in the MCPTT system (fanning out
  audio to TIA-102 Serving RFSSs).
- **Serving RFSS** for groups homed in the TIA-102 RFSS (reporting
  affiliation and vocoder capabilities to the home RFSS).

Both **confirmed** and **unconfirmed** group call modes apply.
**Group Emergency Cancellation** (per BACA-B-1 addendum) applies.

**"Losing audio" concept:** when two talk spurts overlap on the
TIA-102 side, the non-selected stream is marked "losing." This
carries over to IWF interworking unchanged.

### 5.2 Unit-to-Unit calls

**Partially supported.** Only the **Calling Home – Called Home**
segment of the four possible call-leg roles applies.

Not applicable for the IWF peer:
- Calling Serving RFSS role.
- Called Serving RFSS role.
- All in-call roaming variants.

This matches the "no UMMF" constraint in §3.

---

## 6. Mobility Management

BACA-B defines four **classes** of mobility objects (Class 1 unit,
Class 2 group, Class 3 packet data, Class 4 group supplementary
data) and five **procedures** (Serving Registration, Serving Query,
Home Query, Serving Deregistration, Home Deregistration).

### 6.1 Applicability under IWF

| Mobility object class | Applies with IWF? |
|----------------------|-------------------|
| Class 1 (Unit) | **No** — unit roaming out of scope |
| **Class 2 (Group)** | **Yes** |
| Class 3 (Packet Data) | **No** — packet data out of scope |
| **Class 4 (Group Supplementary Data)** | **Yes** |

### 6.2 Procedures

All five apply **except Serving Query** (which pertains to SU
mobility objects and packet data — both out of scope).

---

## 7. SIP / SDP / RTP Applicability

Per BACA-B-3 Annex J. **Substantially all** SIP header fields and
SDP sections of BACA-B apply to the IWF scenario as-written.

### 7.1 Key exclusions

| Item | Why excluded |
|------|--------------|
| `snc-id` radical name | Packet data; out of scope |
| `nsapi-id` radical name | Packet data; out of scope |
| `x-tia-p25-sndcp` body type | Packet data |
| SUID registration | Unit mobility out of scope |
| Unit-roaming call flow examples | Out of scope |

### 7.2 Applicable `numplan-params`

Only these URI parameter values apply to IWF interworking:

- `;user=TIA-P25-SU` — Subscriber Unit addressing
- `;user=TIA-P25-SG` — System-wide Group
- `;user=TIA-P25-SD-SG` — Supplementary Data System-wide Group
- `;user=TIA-P25-RFSS` — RFSS

### 7.3 Errata note in BACA-B parent

BACA-B-3 flags that **BACA-B itself contains incorrect uses of
`;user=TIA-P25-SU`** in some group-call examples. Implementers
working from the parent spec should cross-check against BACA-B-3
Annex J for the correct param usage.

---

## 8. Out-of-Scope Items and Deferred Studies

BACA-B-3 explicitly defers these for further JLMRLTE study:

1. Vocoder mode negotiation limitations when the peer RFSS is an IWF.
2. Transcoding / trans-encrypting between MCPTT AMR and TIA-102 AMBE
   vocoders.
3. Mapping between LMR console SUs and LTE console UEs.
4. More-than-one-LMR-Serving-system topology.
5. The structure of the IWF itself and the IWF-x interface to the
   MC system.

**Implementation consequence.** A production IWF deployment will
require vendor-specific decisions on all five of these. Implementers
should document which choice they made and why, so future
standardization can incorporate operational feedback.

---

## 9. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- Addendum scope — TIA-102.BACA-B-3 Foreword.
- IWF architecture and reference points — Annex J (all), with
  five topology figures in the opening sub-clause.
- Functional roles of the IWF — Annex J (GMMF, UCCF, GCCF, MMF,
  SMF, RCPF sub-clauses).
- Vocoder mode negotiation — Annex J (Vocoder Mode sub-clause).
- Call control applicability — Annex J (Call Control sub-clause).
- Mobility management applicability — Annex J (Mobility Management
  sub-clause).
- SIP / SDP / RTP sub-clause-level applicability — Annex J (per-clause).
- Numplan-params applicable — Annex J (numplan-params list).
- Deferred studies — Annex J (notes and the informational notes
  attributed to ongoing JLMRLTE study).

---

## 10. Cross-References

**Upstream baseline (parent spec):**
- **TIA-102.BACA-B** (ISSI Messages and Procedures, 2012) — parent
  document; every section other than Annex J is unchanged. **Not yet
  processed in this repo.**
- **TIA-102.BACA-B-1** (Group Emergency Cancellation, 2013) —
  earlier addendum; referenced here for the emergency cancellation
  procedure that carries through unchanged.
- **TIA-102.BACA-B-2** (2016) — earlier addendum; referenced but
  unchanged under IWF.

**Companion (related P25 ISSI work):**
- `standards/TIA-102.BACE/P25_ISSI_Conventional_Implementation_Spec.md`
  — ISSI for Conventional Operation (Ec interface, next in this pass).
- **TIA-102.BACF** — ISSI for Packet Data — **packet data is out of
  scope** of IWF interworking, but BACF is the companion standard
  referenced for packet-data ISSI behavior.

**Companion (related TIA-102):**
- `standards/TIA-102.AABA-B/P25_Trunking_Overview_Implementation_Spec.md`
  — defines RFSS as an architectural entity.
- `standards/TIA-102.AABD-B/P25_Trunking_Procedures_Implementation_Spec.md`
  — trunking procedures; some behaviors referenced by ISSI call
  control.
- `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` —
  IMBE vocoder for Full Rate path.
- `standards/TIA-102.BBAC-A/P25_TDMA_MAC_Layer_Implementation_Spec.md`
  — TDMA Half Rate (AMBE+2) context.

**External references:**
- 3GPP **Rel-14** Mission Critical Services (TS 22.179, TS 23.179,
  TS 24.379) — authoritative for the MCPTT side.
- ATIS **WTSC JLMRLTE** liaison documents (2019–2020) —
  joint P25/3GPP study that drove this addendum.
- IETF **RFC 3261** (SIP), **RFC 4566** (SDP), **RFC 3550** (RTP) —
  ISSI transport.
