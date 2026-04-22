# P25 ISSI — Voice and Mobility (TIA-102.BACA-B) Implementation Spec

**Source:** TIA-102.BACA-B (November 2012), *Project 25 Inter-RF
Subsystem Interface — Messages and Procedures for Voice and Mobility*.
Supersedes TIA-102.BACA-A. 506 pages.

**Addenda (bundled in repo dir):**
- **BACA-B-1** (July 2013) — Group Emergency Cancellation. Processed
  via note in companion BACA-B-3 spec.
- **BACA-B-2** (November 2016) — errata / editorial.
- **BACA-B-3** (April 2021) — IWF Interworking (Annex J). Processed
  as a delta impl spec in this repo.

**Document type:** MESSAGE_FORMAT + PROTOCOL. The **core normative
spec for the P25 Inter-RF Subsystem Interface (ISSI)** — IP-based
interconnection between P25 RF Subsystems (RFSSs) for voice calls,
mobility management, and RFSS capability polling. Uses standard IETF
protocols (SIP per RFC 3261, SDP per RFC 3264, RTP per RFC 3550)
extended with P25-specific headers and payload types.

**Where this fits:**
- Underlies **BACA-B-3** (IWF addendum — which cites this as "parent").
- Underlies **BACF** (ISSI packet data — which delegates SIP REGISTER
  to this document).
- Underlies **BACE** (Conventional ISSI / Ec — reuses Block Type 5
  "ISSI Header Word" directly from this spec).

**Scope of this derived work:**
- §1 — ISSI topology and the four-RFSS SU-to-SU call model
- §2 — SIP transport, methods, and P25-specific headers
- §3 — RTP audio transport: codec types, SSRCs, block / PTT packet types
- §4 — Call procedures: SU-to-SU, group call, RFSS capability polling
- §5 — Mobility management: mobility objects, REGISTER, in-call roaming
- §6 — Vocoder mode negotiation (Full Rate / Half Rate / Native)
- §7 — SIP response code universe (generic + group + SU-to-SU)
- §8 — Annexes: timers, vocoder error mitigation, MSCs, SDP examples
- §9 — Cite-to section references
- §10 — Cross-references

**Pipeline artifacts:**
- `standards/TIA-102.BACA/TIA-102-BACA_Full_Text.md` — condensed
  extraction of BACA-B + B-1 + B-2 (copyrighted source; git-ignored in spirit — note the dir
  holds the base + B-1 + B-2 PDFs).
- `standards/TIA-102.BACA/TIA-102-BACA_Summary.txt` — retrieval summary.
- `standards/TIA-102.BACA/TIA-102-BACA_Related_Resources.md`.

---

## 1. ISSI Topology

### 1.1 Point-to-point within a WACN

Per BACA-B §4.

The ISSI is a **point-to-point IP interface** between pairs of RFSSs
**within the same P25 system (same WACN)**. Each RFSS may connect
to multiple peer RFSSs — the topology is a mesh of bilateral links,
not a star.

```
 [RFSS-A] ◀──ISSI──▶ [RFSS-B]
 [RFSS-A] ◀──ISSI──▶ [RFSS-C]
 [RFSS-B] ◀──ISSI──▶ [RFSS-C]
```

**Same-WACN constraint:** ISSI is for inter-RFSS traffic within one
logical P25 system. Cross-WACN interworking is not natively supported
— that would require an IWF (see BACA-B-3 for MCPTT-style
interworking, which presents itself as an RFSS peer).

### 1.2 RFSS Roles

Per call type, each RFSS takes on **one or more** roles:

**SU-to-SU Calls (up to four roles total):**

| Role | Meaning |
|------|---------|
| **Calling Serving RFSS** | Currently serves the calling SU |
| **Calling Home RFSS** | SU's home RFSS (registration target) |
| **Called Home RFSS** | Home RFSS for the called SU |
| **Called Serving RFSS** | Currently serves the called SU |

**Group Calls (two roles):**

| Role | Meaning |
|------|---------|
| **Home RFSS** | Owns the talk group; audio aggregation point |
| **Serving RFSS** | Serves one or more group members; connects to Home |

### 1.3 Four-RFSS SU-to-SU topology

Three **independent** SIP dialogs (call segments) tie four RFSSs
together when all four roles are distinct:

```
Calling SU
    │
[Calling Serving RFSS] ──Segment 1── [Calling Home RFSS]
                                            │
                                        Segment 2
                                            │
                                     [Called Home RFSS] ──Segment 3── [Called Serving RFSS]
                                                                                │
                                                                           Called SU
```

Each segment uses its own SIP INVITE. Collapses degenerate: if the
calling SU is at home, Segment 1 is internal (no ISSI); etc. Audio
ultimately flows end-to-end through RTP with specific SSRC
assignments (see §3.2).

### 1.4 Group Call Star Topology

```
[Serving RFSS-1] ┐
[Serving RFSS-2] ├──▶ [Home RFSS]   (GCCF + MMF functions)
[Serving RFSS-3] ┘
```

The Home RFSS is the **audio mixing / bridging point**. Each Serving
RFSS connects to it independently. Audio from any Serving RFSS is
distributed to all others through the Home's MMF (Master Media
Function).

---

## 2. SIP Transport and Addressing

### 2.1 Transport

Per BACA-B §5:

- **SIP messages**: UDP (default port **5060**).
- **RTP audio**: UDP on negotiated ports.
- **SIP URI format** per RFC 3261 with `;user=` parameters (see §2.4).

### 2.2 SIP Methods Used

| Method | Usage |
|--------|-------|
| `INVITE` | Establish a call segment |
| `Re-INVITE` | Modify a **group call** (e.g., add a vocoder mode). **Not used for SU-to-SU**. |
| `CANCEL` | Cancel a pending INVITE |
| `BYE` | Terminate an established call |
| `ACK` | Acknowledge final INVITE response |
| `REGISTER` | Unit registration / deregistration / query |
| `OPTIONS` | RFSS capability polling |

### 2.3 P25-specific SIP Headers

Two custom headers carry P25-specific parameters:

**`X-TIA-P25-ISSI`** — 40+ parameters covering call setup, identity,
and encryption. Key ones:

| Parameter | Meaning |
|-----------|---------|
| `c-wacn` / `c-sysid` / `c-rfssid` / `c-siteid` | Network hierarchy identifiers |
| `c-type` | Call type |
| `c-priority` | Call priority |
| `c-emergency` | Emergency flag |
| `c-icr` | In-Call Roaming flag (`1` = ICR active) |
| `c-suid` | Subscriber Unit ID (full SUID: WACN + System + Unit) |
| `c-tgid` | Talk Group ID |
| `c-algid` | Encryption algorithm ID |
| `c-kid` | Encryption key ID |
| `c-mi` | Message Indicator (IV) |
| `c-rf-resource` | RF Resource Availability (confirmed group calls) |

**`X-TIA-P25-SNDCP`** — 21 parameters for SNDCP packet data context
identification (used together with BACF for packet data mobility).

### 2.4 Route Header (RFSS Role Indicator)

The SIP Route header tag carries the originator's role:

| Route Tag | Meaning |
|-----------|---------|
| `TIA-P25-U2Uorig` | Sender is the originating RFSS for a SU-to-SU call |
| `TIA-P25-U2Udest` | Sender is the destination RFSS for a SU-to-SU call |
| `TIA-P25-GroupCall` | Group-call signaling |

Role is determined by comparing Route header + Request-URI + To / From
headers (BACA-B Table 44).

**User-part params (applicable in IWF context per BACA-B-3):**
`;user=TIA-P25-SU`, `;user=TIA-P25-SG`, `;user=TIA-P25-SD-SG`,
`;user=TIA-P25-RFSS`.

---

## 3. RTP Audio Transport

### 3.1 RTP Codec Types

Per BACA-B §6. Three P25 payload types:

| RTP PT | MIME Type | Description |
|--------|-----------|-------------|
| **100** | `X-TIA-P25-FullRate/8000` | **IMBE Full Rate** (4.4 kbps vocoder) |
| **101** | `X-TIA-P25-HalfRate/8000` | **AMBE+2 Half Rate** (2.2 kbps) |
| **102** | `X-TIA-P25-Native/8000` | **Native** (transparent RF frames passed through without transcoding) |

**Vocoder mode negotiation** uses SDP offer/answer per RFC 3264 (see
§6).

**Preference order** (highest to lowest): **Native > HalfRate > FullRate**.
Rationale: Native avoids transcoding entirely when both sides can
handle it; HalfRate saves bandwidth vs FullRate.

### 3.2 SSRC assignments

RTP SSRC uniquely identifies each media flow:

**Group Calls:**

| SSRC | Stream |
|------|--------|
| `1` | Home RFSS → Serving (audio out) |
| `2` | Serving RFSS → Home (audio in) |

**SU-to-SU Calls (4-RFSS topology):**

| SSRC | Direction |
|------|-----------|
| `11` | Called Serving → Called Home |
| `12` | Calling Serving → Called Serving (via Calling Home path) |
| `13` | Called Home → Called Serving |
| `14` | Calling Home → Calling Serving |
| `16` | Calling Serving → Calling Home |

These fixed SSRC values make flow identification deterministic —
a decoder watching an ISSI link can identify every flow by SSRC
without consulting SIP state.

### 3.3 RTP Block Types

P25 RTP packets are composed of typed **blocks**. A single RTP
packet may contain multiple blocks. The per-block framing is shared
with **BACE (Conventional ISSI / Ec)**, which extends this spec's
base block types with conventional-specific variants (Block Types
16–30). Block Type 5 (ISSI Header Word) is the structure BACE
Block Type 5 directly copies.

### 3.4 Frame Types and PTT Packet Types

- **IMBE Frame Types** (`$62`–`$73`) identify frame position in a
  superframe and select supplemental data layout — same framing as
  BACE / BAHA-A (see those impl specs for byte-level detail).
- **11 PTT Packet Types** cover the transmission-control life cycle
  (analogous to BACE's TC Packet Types in §3.3 of BACE impl spec).

---

## 4. Call Procedures

### 4.1 SU-to-SU (Unit-to-Unit) Calls

Per BACA-B §8.1.

Establishment walks **segment by segment**:
1. Calling Serving → Calling Home INVITE (Segment 1).
2. Calling Home → Called Home INVITE (Segment 2).
3. Called Home → Called Serving INVITE (Segment 3).

Each INVITE carries the full SIP `X-TIA-P25-ISSI` header — the
Called Home RFSS evaluates the call, alerts the called SU, and
responds. Ringback happens inside the originating air-interface
signaling, not in SIP.

**ICR (In-Call Roaming):** `c-icr=1` signals that the call should be
preserved if an SU roams mid-call. Specific ICR procedures are in
BACA-B §9.3.

### 4.2 Group Calls

Per BACA-B §8.2.

Star topology around the Home RFSS. Each Serving RFSS sends an
INVITE to the Home RFSS; the Home aggregates audio and fans out.

**Confirmed vs Unconfirmed modes** — the Home RFSS owns this choice
per group configuration. Confirmed group calls use the
`c-rf-resource` parameter in `X-TIA-P25-ISSI` to coordinate RF
resource availability across Servings.

**Emergency cancellation** (BACA-B-1 addendum, 2013) adds specific
teardown semantics when a group emergency clears — the Home notifies
all Servings via a cancellation message. The addendum is normative
reference for any ISSI implementation handling emergency groups.

### 4.3 RFSS Capability Polling

Per BACA-B §8.3. Uses SIP `OPTIONS`. **All RFSSs must respond** to
an OPTIONS poll. Detailed capability transfer (vocoder modes
supported, ICR capability, etc.) is optional but common.

---

## 5. Mobility Management

### 5.1 Mobility Objects

Per BACA-B §9.1. Four **classes** of mobility object:

| Class | Object | Status |
|-------|--------|--------|
| 1 | Unit | Applies in TIA-102 ISSI; **disabled under IWF (BACA-B-3)** |
| 2 | Group | Applies always |
| 3 | Packet Data | Applies; **tracked by BACF** (not this document) |
| 4 | Group Supplementary Data | Applies always |

### 5.2 Unit Registration (REGISTER)

Per BACA-B §9.2. SIP REGISTER is the mechanism. The REGISTER body
includes `X-TIA-P25-ISSI` with the current Serving RFSS's SUID, the
home RFSS's ID, and the registration action (register / deregister /
query).

**Five mobility procedures:**
- Serving Registration
- Serving Query
- Home Query
- Serving Deregistration
- Home Deregistration

### 5.3 In-Call Roaming

Per BACA-B §9.3. An SU can physically cross RFSS boundaries during a
call without dropping audio. The Calling/Called Serving RFSS
dynamically migrates; the segment-based SIP dialog chain adapts
through Re-INVITE and updated mobility REGISTER.

---

## 6. Vocoder Mode Negotiation

Per BACA-B §10. Negotiation via SDP offer/answer:

### 6.1 Three modes

- **Full Rate** (PT 100) — IMBE at 4.4 kbps.
- **Half Rate** (PT 101) — AMBE+2 at 2.2 kbps.
- **Native** — passes RF-rate frames end-to-end without transcoding.

### 6.2 Rate Conversion

When endpoints disagree on rate, the Home RFSS (group) or the path
(SU-to-SU) performs **parametric rate conversion**. A key caveat for
encrypted calls: **rate conversion breaks end-to-end encryption** —
the encrypted bitstreams differ between Full Rate and Half Rate, so
the call must be decrypted before conversion and re-encrypted after.

### 6.3 Failure Response

If no common vocoder mode can be agreed, the call setup fails with
a specific SIP response code (BACA-B §10.3). Annexes F and G
(informative) provide worked examples of successful and failed
negotiations.

---

## 7. SIP Response Codes

Per BACA-B §11.

### 7.1 General SIP Failure Responses (Table 45)

Standard RFC 3261 codes apply (401, 403, 404, 408, 500, 503, etc.).
BACA-B specifies which are valid for what.

### 7.2 Group Call Specific (Table 46)

Additional reason-phrase extensions for group-call-specific failures
(e.g., "group home unavailable", "call denied by home policy").

### 7.3 SU-to-SU Specific (Table 47)

Extensions for SU-to-SU failures (e.g., "called unit unreachable",
"calling not authorized").

### 7.4 Generic GEN_ prefix response catalog (Annex A Table 57)

Enumerated response codes that span all call types.

---

## 8. Annexes

| Annex | Content |
|-------|---------|
| **A** (Table 57) | Generic SIP Responses (`GEN_` prefix) |
| **B** (Table 58) | Timers — complete set of BACA-B timers and defaults |
| **C** | Vocoder Error Mitigation (error bit allocations: 9 bits full rate, 5 bits half rate, + 3 bits enhanced mode) |
| **D** | Message Sequence Charts (MSCs) — all canonical call flows |
| **E** | SDP Examples — Full Rate only, Half Rate only, all three modes |
| **F** | SU-to-SU Call Flows with Vocoder Mode Negotiation (single mode, first SDP declined, native with rate conversion) |
| **G** | Group Call Flows with Vocoder Mode Negotiation (mixed modes with/without rate conversion; serving-initiated mixed mode) |
| **H** | Future Considerations (Informative) — security, compression, bandwidth reservation, mixed IANA/ISSI payload, service profile refresh |

**Annex H** flags future-work items — not normative but useful for
implementer context.

---

## 9. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- Scope — TIA-102.BACA-B §1.
- Normative references — §2.
- Definitions, functional entities, SIP URI format — §3.
- Architecture, RFSS roles, 4-RFSS SU-to-SU topology, group star topology — §4.
- SIP transport, methods, `X-TIA-P25-ISSI` header — §5.
- Route header and RFSS role determination (Table 44) — §5.
- RTP audio transport — §6.
- SIP state machines — §7.
- SU-to-SU call procedures — §8.1.
- Group call procedures — §8.2.
- RFSS Capability Polling — §8.3.
- Mobility objects — §9.1.
- Unit Registration (REGISTER) — §9.2.
- In-Call Roaming — §9.3.
- Vocoder mode negotiation (SDP offer/answer, rate conversion, failure) — §10.
- SIP response codes (Tables 45/46/47; Annex A Table 57) — §11.
- Timers (Annex B Table 58).
- Vocoder Error Mitigation (Annex C).

---

## 10. Cross-References

**Addenda (processed in this repo):**
- `standards/TIA-102.BACA-B-3/P25_ISSI_IWF_Interworking_Implementation_Spec.md`
  — IWF interworking (Annex J of BACA-B series; delta on top of this
  document).

**Companion ISSI specs:**
- `standards/TIA-102.BACE/P25_ISSI_Conventional_Implementation_Spec.md`
  — Conventional ISSI (Ec interface); reuses Block Type 5 "ISSI Header
  Word" directly from this spec.
- `standards/TIA-102.BACF/P25_ISSI_Packet_Data_Implementation_Spec.md`
  — Packet data over ISSI; delegates SIP REGISTER to this document.

**Upstream P25 infrastructure:**
- `standards/TIA-102.AABA-B/P25_Trunking_Overview_Implementation_Spec.md`
  — defines RFSS, WACN, System, Site terminology.
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`
  — IMBE frame structure underlying RTP block types.
- `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` —
  IMBE vocoder spec; Full Rate vocoder.
- `standards/TIA-102.AAAD-B/P25_Block_Encryption_Protocol_Implementation_Spec.md`
  — MI / ALGID / KID used in `c-algid` / `c-kid` / `c-mi` parameters.

**External normative references:**
- IETF **RFC 3261** (SIP), **RFC 3264** (SDP offer/answer),
  **RFC 3550** (RTP), **RFC 3551** (RTP A/V profile), **RFC 3311**
  (UPDATE method, via BACA-B-1 addendum).
