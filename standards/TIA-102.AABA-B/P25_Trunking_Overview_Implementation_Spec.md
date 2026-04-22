# P25 Trunking Overview — TIA-102.AABA-B Implementation Spec

**Source:** TIA-102.AABA-B (April 2011), *Project 25 Trunking Overview*.
Supersedes AABA-A (April 1995). In turn superseded by AABA-C (October
2020) — see §11.

**Document type:** architectural overview. AABA-B is **not** a wire-format
or procedure spec — it establishes the terminology, service model, and
topology that the normative trunking documents (AABB / AABC / AABD /
AABF) are built on. This derivation is therefore a navigation hub, not
a parser spec: it fixes a small number of cross-cutting concepts an
implementer has to get right before reading any of the normative specs,
and points at the right normative doc for every concrete question.

**Scope of this derived work:**
- §1 — What counts as "trunking" (and what doesn't)
- §2 — The FDMA / TDMA split, and why the control channel is always FDMA
- §3 — Topology: subscriber ↔ site ↔ RFSS ↔ WACN
- §4 — Resource allocation: transmission vs message trunking
- §5 — Registration, roaming, and why site affiliation matters
- §6 — Services taxonomy: teleservices / bearer / supplementary
- §7 — Composite Control Channel (CCC)
- §8 — FDMA ↔ TDMA interoperability model
- §9 — Cross-reference map: which spec answers which question
- §10 — Terminology reference (the short, implementer-relevant subset)
- §11 — Revision history and supersession

**Pipeline artifacts:**
- `standards/TIA-102.AABA-B/TIA-102-AABA-B_Full_Text.md` — clean-room
  extraction (copyrighted, git-ignored).
- `standards/TIA-102.AABA-B/TIA-102-AABA-B_Summary.txt` — ~1000-word
  summary for retrieval.
- `standards/TIA-102.AABA-B/TIA-102-AABA-B_Related_Resources.md` —
  lineage, open-source implementations, external references.

---

## 1. What Counts as "Trunking"

Per AABA-B §2.0 and §2.4, the defining attribute of a trunked P25 system
is **centralized resource allocation**: subscriber units request access
to the system, and an infrastructure-side resource controller grants or
denies the requested RF channel. Anything without that centralized
access-control role is "conventional", even if it runs the same
C4FM / CAI framing.

Implementation consequence: a trunked receiver cannot treat any particular
voice-channel frequency as stable. Every call originates with a grant on
the control channel (AABB/AABC) that tells the SU which working channel
to tune; the SU follows the grant. A decoder that wants to reconstruct
trunked traffic must therefore track control-channel state, not just
demodulate a fixed voice frequency. This is the architectural reason
SDRTrunk and OP25 both implement a "follow the control channel" state
machine rather than a pool of static frequency decoders.

Non-consequence: trunking does *not* imply a different modulation, FEC,
CRC, or vocoder vs. conventional. Per AABA-B §2.2, trunked FDMA and
conventional FDMA share the same CAI — the wire bits on a Phase 1 voice
channel are indistinguishable between the two modes. The only difference
is who controls channel access (a resource controller for trunking; the
SU itself for conventional).

## 2. FDMA / TDMA Split

Per AABA-B §2.3:

- **FDMA** may be used for **control, voice, and data** channels.
- **TDMA** may be used for **voice channels only** (two time-slots per
  12.5 kHz RF channel, per BBAB / BBAC-A physical and MAC specs).
- The **control channel is always FDMA**. TDMA control channels
  (TIA-102.BBAD-A + BBAC-A's LCCH configurations) do exist in the
  Phase 2 suite, but the AABA-B overview stops at the FDMA-only
  control-channel framing. Any Phase 2 TDMA deployment that uses a TDMA
  LCCH is conformant to the Phase 2 standards but not to AABA-B's
  architectural assumptions — implementers targeting TDMA LCCH should
  read BBAD-A / BBAC-A directly.

Implementation consequence: a trunking decoder must always demodulate an
FDMA (C4FM) control channel. A Phase 2 voice call is signaled on FDMA,
granted to an H-DQPSK TDMA working channel, and the SU hands off
modulations when it follows the grant. This is why Phase 1 and Phase 2
decoders share a common control-channel front-end.

## 3. Topology: Subscriber ↔ Site ↔ RFSS ↔ WACN

AABA-B §1.4 defines **RF Subsystem (RFSS)** as the RF infrastructure
bounded by open P25 interfaces and standard computer network gateway
interfaces. It is the normative unit of trunking authority; other
hierarchy terms used across the trunking suite are not defined in AABA-B
itself but are inherited from the normative docs (BAAC / AABC):

```
┌───────────────────────────────────────────────────────────────────┐
│ WACN (Wide-Area Communications Network) — 20-bit ID               │
│  └── System (within WACN)                        — 12-bit ID      │
│       └── RFSS (RF Subsystem)                    — 8-bit ID       │
│            └── Site                              — 8-bit ID       │
│                 └── Channel(s)                                    │
│                     └── Subscriber Unit (SU)     — 24-bit ID      │
└───────────────────────────────────────────────────────────────────┘
```

The ID widths are defined in the TSBK / MAC message field definitions
(see AABC-E / BBAD-A impl specs). AABA-B §2.6 treats the **site** as the
unit of roaming: an SU registers its identity and *site* location with
the RFSS's resource controller so the RFSS can provision only the sites
that need the call.

**ISSI (Inter-SubSystem Interface).** Per AABA-B §3.1.1 and §3.1.2,
ISSI is how RFSSs from different manufacturers (or the same manufacturer
operating separate RFSSs) interoperate for group and individual calls.
Normative ISSI messaging and procedures live in the BACA document suite
— out of scope for this overview.

## 4. Resource Allocation: Transmission vs Message Trunking

Per AABA-B §2.5:

| Mode | Channel held for | Control-channel cost | When used |
|------|------------------|----------------------|-----------|
| **Transmission trunking** | One PTT transmission | One grant per transmission | Data channel grants; short voice transmissions in high-load systems |
| **Message trunking** | A sequence of exchanges among the same call participants | One grant per call, re-used across back-and-forth | Voice calls in almost every real deployment |

Implementation consequence: a passive decoder watching the control
channel sees different working-channel lifetimes depending on which
mode is in use. Message-trunked voice calls produce long dwell on one
working channel and terminate on an explicit teardown message;
transmission-trunked data grants are short-lived. This matters for
confidence labeling (how long to keep listening on a working channel
before declaring the call over).

Neither mode is negotiated per-call on the wire in a directly
observable way — it's an infrastructure configuration inherited by
how the resource controller grants channels. A decoder distinguishes
them empirically by watching grant / teardown cadence.

## 5. Registration, Roaming, and Site Affiliation

Per AABA-B §2.6:

- Every SU in a trunked system **registers its identity and site
  location** with its RFSS's resource controller.
- Site registration lets the RFSS provision only the sites actually
  needed for a given call — a talkgroup call goes out only on sites
  where a registered group member is affiliated, not system-wide.
- Roaming is the architectural mechanism by which an SU that has left
  its home RFSS's coverage can register on a visited RFSS and receive
  services (including reaching back into its home system via ISSI).

Implementation consequence: a trunking decoder that wants to track
talkgroup membership must watch for Group Affiliation (AABC-E) and
Registration (AABC-E / AABD-B procedures) messages on the control
channel. Without them, a passive decoder sees the grants but not the
affiliation state — it will follow the right working channel but won't
know which talkgroup IDs a given SU belongs to.

The normative registration, deregistration, group-affiliation, and
roaming procedures are defined in the AABD (trunking procedures) and
AABC (trunking messages) documents; AABA-B only establishes the
conceptual role.

## 6. Services Taxonomy

AABA-B §3 defines three service layers that map directly onto decoder
state-machine categories:

```
┌───────────────────────────────────────────────────────────┐
│ Teleservices (end-user visible)                          │
│   • Group Voice Call                (§3.1.1)              │
│   • Individual Voice Call           (§3.1.2)              │
├───────────────────────────────────────────────────────────┤
│ Bearer Services (data transport)                          │
│   • CAI Confirmed Data Packet Delivery   (§3.2.1 → BAEB) │
│   • CAI Unconfirmed Data Packet Delivery (§3.2.2 → BAEB) │
├───────────────────────────────────────────────────────────┤
│ Supplementary Services (attached to a bearer/teleservice) │
│   • Voice Telephone Interconnect (§3.3.1 → BADA)         │
│   • Pre-programmed Data Messaging / Status / Alert        │
│     (§3.3.2, also called "short data message")            │
└───────────────────────────────────────────────────────────┘
```

Cross-reference to wire-format specs:

| Service category | Normative wire format | Normative procedures |
|------------------|-----------------------|----------------------|
| Group / Individual Voice Call grant | AABB-B (CCH formats), AABC-E (messages) | AABD-B (trunking procedures) |
| Voice working-channel metadata | AABF-D (link control words) | AABD-B |
| Confirmed / Unconfirmed data | BAAA-B (CAI PDUs), BAEB-C (SNDCP) | BAED-A (LLC) |
| Telephone Interconnect | BADA-A (requirements, extraction pending) | AABD-B |
| Pre-programmed messaging / Status | AABC-E Status Query / Status Update / Message Update opcodes | AABD-B |

Implementation note: AABA-B §3.3.2 also lists the synonyms implementers
encounter in vendor documentation — "short message", "message update",
"status", "call alert" — all of which map to the AABC-E status/message
opcode family.

## 7. Composite Control Channel (CCC)

Per AABA-B §4.0:

- The system may reallocate the trunking control channel as a working
  channel when all other RF resources are in use.
- Control-channel operation resumes once the CCC's working-channel
  service completes.
- While CCC is active, the control channel is unavailable for new grants
  — SUs cannot request service until control-channel operation resumes.

AABA-B notes this as an architectural capability and defers detail to
**[AABD-1]** (AABD-A-1, the Trunking Procedures Addendum for CCC). An
implementer who encounters apparent control-channel "gaps" in a capture
should consider CCC as a candidate explanation, especially on loaded
systems.

**Passive-decoder implication:** a CCC transition means the frequency
that was the control channel now carries voice traffic for a time,
then returns to control-channel duty. A decoder that hard-assumes a
single static control-channel frequency-to-role mapping will mis-decode
during CCC intervals. The mitigation is SDRTrunk's pattern — re-probe
the configured control-channel frequencies for control-channel framing
periodically rather than assuming a fixed role.

## 8. FDMA ↔ TDMA Interoperability Model

AABA-B §2.8 is the architectural root of how Phase 1 and Phase 2
systems coexist:

1. **Shared control channel (FDMA).** Both FDMA SUs and TDMA SUs
   monitor the same FDMA control channel. The control-channel
   messages are the same wire format (AABB-B / AABC-E) for both.
2. **Divergent voice channels.** FDMA SUs use C4FM IMBE voice; TDMA
   SUs use H-DQPSK AMBE+2 voice. The grants on the control channel
   tell each SU which modulation to expect on the working channel.
3. **Common vocoder family.** Both use MBE-family vocoders (IMBE on
   Phase 1, AMBE+2 on Phase 2), enabling interoperable audio between
   FDMA and TDMA endpoints via vocoder transcoding at the RFSS.
4. **Migration path.** An existing FDMA deployment can be upgraded to
   support TDMA voice channels without changing the control channel
   or the SU's control-channel decoder. TDMA-capable SUs gain access
   to TDMA working channels; FDMA-only SUs continue to receive FDMA
   working channels on the same system.

Implementation consequence: a Phase 2-capable decoder needs two
working-channel demodulators (C4FM + H-DQPSK) but **one** control-channel
demodulator. The control-channel state machine is identical between
Phase 1 and Phase 2 systems at the AABB/AABC layer; only the
working-channel path forks.

## 9. Cross-Reference Map: Which Spec Answers Which Question

AABA-B's practical value is as a router. The table below maps
overview-level questions to the normative doc that actually answers
them.

| Question | Normative doc | Impl spec |
|----------|---------------|-----------|
| What does a TSBK opcode byte look like? | AABB-B | `standards/TIA-102.AABB-B/P25_Trunking_Control_Channel_Formats_Implementation_Spec.md` |
| What does each opcode mean? (grant, registration, deregistration, …) | AABC-E | `standards/TIA-102.AABC-E/P25_Trunking_Control_Channel_Messages_Implementation_Spec.md` |
| When should the SU send opcode X? | AABD-B | `standards/TIA-102.AABD-B/P25_Trunking_Procedures_Implementation_Spec.md` |
| How is a voice call torn down? | AABD-B | same |
| What do the LC words on a voice channel carry? | AABF-D | `standards/TIA-102.AABF-D/P25_Link_Control_Word_Implementation_Spec.md` |
| What are the legal NAC / MFID / ALGID / SAP values? | BAAC-D | `standards/TIA-102.BAAC-D/P25_Reserved_Values_Implementation_Spec.md` |
| How is data carried on a trunked system? | BAEB-C (SNDCP) + BAED-A (LLC) + BAAA-B (PDU) | `standards/TIA-102.BAEB-C/…` and companions |
| How are Phase 2 TDMA voice channels framed? | BBAC-A + BBAD-A + BBAE | `standards/TIA-102.BBAC-A/…`, `BBAD-A/…`, `BBAE/…` |
| What's the ISSI wire format? | BACA series | (not processed — see specs.toml) |
| What's the telephone-interconnect wire format? | BADA-A | (not processed — see specs.toml) |

For a first-time implementer: read AABB-B + AABC-E + AABD-B together to
cover the trunking control-channel path end-to-end. Read AABF-D for
voice-channel link control. Add BAAC-D for reserved-value lookups. The
rest (data, TDMA, ISSI, telephone interconnect) is additive scope.

## 10. Terminology Reference

Short subset of AABA-B §1.4 plus cross-suite terms an implementer needs.
Full term list is in the normative companion docs.

| Term | Meaning for implementers |
|------|--------------------------|
| **Trunking** | Centralized access control over a shared RF-channel pool. Defining contrast: conventional = self-coordinated access. |
| **Resource controller** | The RFSS-side function that grants / denies SU requests. Physically realized as a site/zone controller in vendor systems. |
| **RF Subsystem (RFSS)** | A self-contained trunking unit bounded by open P25 interfaces. Minimum trunking authority scope. |
| **Site** | One coverage cell within an RFSS. The unit of registration and per-call provisioning. |
| **Working channel** | An RF channel allocated by a grant to carry a specific call. Opposed to control channel. |
| **Control channel** | The always-on FDMA channel carrying grants, registrations, broadcasts. Never TDMA in the AABA-B architecture. |
| **Transmission trunking** | One grant per PTT transmission. Channel released between transmissions. |
| **Message trunking** | One grant per call. Channel held across multiple back-and-forth transmissions. |
| **Supplementary control** | Non-grant control traffic (status, alerts, cancellations) on either the control channel or the working channel. |
| **Composite Control Channel (CCC)** | Temporary reallocation of the control-channel frequency as a working channel under load. See AABD-A-1. |
| **Talkgroup** | A predefined group of SUs addressed by a single group ID. Basis of group voice calls (§3.1.1). |
| **ISSI** | Inter-SubSystem Interface. RFSS ↔ RFSS interop for group / individual calls and data. BACA-series spec. |
| **Teleservice** | End-user-visible service (voice call). AABA-B §3.1. |
| **Bearer service** | Data transport service. AABA-B §3.2, normative detail in BAEB. |
| **Supplementary service** | Attached to a bearer/teleservice (e.g., telephone interconnect, short data message). AABA-B §3.3. |

## 11. Revision History and Supersession

| Revision | Published | Scope change |
|----------|-----------|--------------|
| AABA-A | April 1995 | Original trunking overview. Security content inline. |
| AABA-B | April 2011 | Added Phase 2 TDMA provisions; removed security content (moved to AAAD suite); added CCC section. |
| **AABA-C** | October 2020 | **Current.** Further TDMA-era updates. |

**Current status.** AABA-B is **superseded by AABA-C** (approved
December 2019, published October 2020). This derivation targets
AABA-B as the best-available overview in the `blip25-specs` corpus —
AABA-C's PDF has not been acquired for this project. Implementers
should source AABA-C directly for a system that's being actively
deployed in 2026 and later. The architectural model captured here
(trunking definition, FDMA/TDMA split, RFSS topology, resource
allocation modes, service taxonomy, CCC) is expected to be stable
across AABA-B → AABA-C.

**What this derivation explicitly does NOT cover:**
- Wire formats, opcodes, or byte layouts — these are in the normative
  companion docs (see §9).
- State machines or procedures — AABD-B.
- Security, encryption, OTAR — AAAD / AACA series.
- TDMA LCCH architecture — BBAD-A + BBAC-A (Phase 2 control channel
  runs outside AABA-B's FDMA-only model).

## 12. Cite-To Section References

Per project convention, cite the PDF's own section numbers when sending
readers to verify:

- Trunking definition and resource-controller role — TIA-102.AABA-B §2.0 / §2.4.
- Channel access methodology (FDMA vs TDMA) — §2.3.
- Transmission vs message trunking — §2.5.
- Registration and roaming model — §2.6.
- Supplementary control on control vs working channel — §2.7.
- FDMA / TDMA interoperability and migration — §2.2 / §2.8.
- Service taxonomy (teleservice / bearer / supplementary) — §3.
- Group and individual voice calls (ISSI role) — §3.1.1 / §3.1.2.
- CAI confirmed / unconfirmed data packet delivery — §3.2.
- Telephone interconnect and pre-programmed data messaging — §3.3.
- Composite Control Channel — §4.0.

## 13. Cross-References

- `standards/TIA-102.AABB-B/P25_Trunking_Control_Channel_Formats_Implementation_Spec.md`
  — TSBK framing, microslot timing, MBT.
- `standards/TIA-102.AABC-E/P25_Trunking_Control_Channel_Messages_Implementation_Spec.md`
  — OSP / ISP opcode dispatch.
- `standards/TIA-102.AABD-B/P25_Trunking_Procedures_Implementation_Spec.md`
  — registration, affiliation, grants, call teardown.
- `standards/TIA-102.AABF-D/P25_Link_Control_Word_Implementation_Spec.md`
  — voice-channel metadata.
- `standards/TIA-102.BAAC-D/P25_Reserved_Values_Implementation_Spec.md`
  — NAC / MFID / ALGID / SAP / Service Class lookups.
- `standards/TIA-102.BBAC-A/P25_TDMA_MAC_Layer_Implementation_Spec.md`
  — Phase 2 voice-channel MAC (H-DQPSK path).
- `standards/TIA-102.BBAD-A/P25_TDMA_MAC_Message_Parsing_Implementation_Spec.md`
  — Phase 2 MAC messages (incl. TDMA LCCH if deployed).
- `analysis/fdma_pdu_frame.md` — unified FDMA PDU view across BAAA-B /
  AABB-B / BAED-A / BAEB-C.
