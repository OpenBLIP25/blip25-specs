# TIA-102-AABD-B-3 Related Resources and Context

**Document:** TIA-102.AABD-B-3 — Project 25 Trunking Procedures Addendum: Remotely Activated Emergency  
**Published:** February 2025  
**Compiled:** 2026-04-13  

---

## Status

**Active — Interim Addendum.** This document was approved for publication in January 2025 and published February 2025. It is an addendum to TIA-102.AABD-B (published November 2014) and will be absorbed into the next full numbered revision of TIA-102.AABD-B. Until that revision is published, TIA-102.AABD-B-3 stands as a normative supplement that implementers must apply alongside TIA-102.AABD-B.

No patents have been identified as covering this document.

This is the third addendum to TIA-102.AABD-B. Previous addenda in the series:
- **TIA-102.AABD-B-1** — first addendum (content TBD; precedes this document)
- **TIA-102.AABD-B-2** — second addendum (User Alias functionality)
- **TIA-102.AABD-B-3** — this document (Remotely Activated Emergency)

---

## Standards Family

This document sits within the TIA-102.AA series, which covers the P25 Common Air Interface (CAI) signaling and procedures layer.

### AABD Lineage (Trunking Procedures)

```
TIA-102.AABD (base)
  └── TIA-102.AABD-A (revision A)
        ├── TIA-102.AABD-A-1 (addendum to A)
        └── TIA-102.AABD-B (revision B, November 2014) ← current base
              ├── TIA-102.AABD-B-1 (addendum 1)
              ├── TIA-102.AABD-B-2 (addendum 2 — User Alias)
              └── TIA-102.AABD-B-3 (addendum 3 — Remotely Activated Emergency) ← THIS DOCUMENT
```

### Companion and Dependency Documents

| Document | Title | Relationship |
|---|---|---|
| TIA-102.AABD-B | Project 25 Trunking Procedures | Parent — this addendum inserts into it |
| TIA-102.AABC (family) | Control Channel Messages | Defines RAE_REQ, RAE_CMD PDU formats; referenced as [1] |
| TIA-102.BACD-B | ISSI (Inter-RF Subsystem Interface) | Defines the unsuccessful forwarding indication used in cross-RFSS RAE failure path |
| TIA-102.AABD-B §11.2.1 | Emergency Alarm Procedures | EMRG_ALM_REQ procedure invoked by target SU after receiving RAE_CMD |
| TIA-102.AABD-B §3.6 | Signaling Reliability Rules | Governs message repetition for ACK_RSP_FNE in this procedure |
| TIA-102.AABD-B §4 / §4.1.3 | Random-Access Protocol | RAE_REQ transmission and retry procedure referenced but not redefined here |

### Broader TIA-102 Suite Context

This document belongs to the **AABD** (trunking procedures) pillar of the TIA-102 suite:

```
TIA-102 Suite (Project 25 CAI)
├── AA series — Air Interface
│   ├── AAAB — Physical Layer (FDMA)
│   ├── AAAC — Data Link Layer
│   ├── AABD — Trunking Procedures  ← this family
│   └── AABC — Control Channel Messages (PDU formats)
├── BA series — System/Network
│   ├── BACD-B — ISSI (cross-RFSS routing)
│   └── BAAD — Conventional Procedures
└── CA series — Conformance and Test
```

---

## Practical Context

### What the RAE Feature Does

Remotely Activated Emergency (RAE) enables a dispatch supervisor or partner unit to trigger the emergency alarm sequence on a remote radio without physical user interaction. This addresses scenarios where an officer is incapacitated, under duress, or otherwise unable to press the emergency button. The dispatch console (or an authorized mobile SU) sends a RAE_REQ; the RFSS relays a RAE_CMD to the target radio; the target radio autonomously sends EMRG_ALM_REQ with a RAE bit set (to distinguish it from user-initiated alarms).

Key operational characteristics:
- **Authorization enforced by the RFSS**: the DENY_RSP path allows the network to reject unauthorized RAE requests
- **Target SU may silently ignore**: devices not implementing RAE support are permitted to discard RAE_CMD, preserving backward compatibility
- **Cross-site operation**: ISSI routing is supported, allowing an initiating console at one RFSS to activate emergency on a radio registered at a distant RFSS
- **RAE bit in EMRG_ALM_REQ**: allows CAD (Computer Aided Dispatch) systems to distinguish remotely triggered alarms from self-initiated ones — important for investigating alarm authenticity

### Real-World Deployment Context

The RAE feature is relevant in:
- **Law enforcement trunked systems**: where command staff may need to remotely trigger emergency on units that have gone silent
- **Large P25 Phase I/II networks**: RFSS-to-RFSS RAE via ISSI enables multi-site agency coordination
- **Dispatch console integrations**: wireline dispatch points initiating RAE without an over-the-air RAE_REQ (entering the procedure at section 11.2.4.3 directly)

The RAE concept is analogous to "man-down" or "remote emergency" features found in some proprietary digital radio systems, but this document standardizes it at the P25 CAI level so it is interoperable across vendors.

---

## Key Online Resources

- **TIA Standards Store (Accuris)**: The authoritative source for purchasing TIA-102 standards.  
  `https://store.accuristech.com/tia`

- **TIA Online**: Standards portfolio page.  
  `https://www.tiaonline.org`

- **NIST P25 Compliance Assessment Program (P25 CAP)**: Government program testing P25 interoperability; relevant to RAE once conformance tests are defined.  
  `https://www.nist.gov/ctl/pscr/pscr-p25-cap`

- **PSCR (Public Safety Communications Research)**: DHS/NIST program for P25 and broadband research.  
  `https://www.nist.gov/ctl/pscr`

- **P25 Standards Approval List (APCO/NASTD/FED Steering Committee)**: Tracking of approved P25 TIA documents.  
  `https://project25.org`

- **Internet Archive — TIA-102 Document Archive**: Some historical TIA-102 documents available.  
  `https://archive.org/details/TIA-102_Series_Documents`

- **MSC Generator (used for Figure 18-12)**: Open-source message sequence chart tool referenced in the document figure caption.  
  `https://gitlab.com/msc-generator` (v8.2 used)

---

## Open-Source Implementations

No open-source P25 implementation has been confirmed as implementing RAE_REQ / RAE_CMD as of April 2026. This is consistent with RAE being a recently standardized (Feb 2025) feature that requires transmit capability.

Relevant projects to watch for future RAE support:

| Project | URL | Notes |
|---|---|---|
| OP25 | `https://github.com/boatbod/op25` | P25 Phase I/II decoder; receive-only; no RAE TX |
| SDRTrunk | `https://github.com/DSheirer/sdrtrunk` | P25 trunked decoder; receive-only; decodes EMRG_ALM_REQ but no RAE TX |
| gr-p25 | various forks | GNU Radio P25 blocks; no RAE implementation found |

For a RAE implementation to be meaningful, a project would need: (1) control channel transmit capability (RAE_REQ), (2) RAE_CMD forwarding logic in an RFSS emulator, and (3) EMRG_ALM_REQ generation with RAE bit. None of the receive-only SDR tools implement this. A full P25 RFSS implementation (e.g., OpenMHz or proprietary head-end software) would be the integration point.

The RAE_CMD and RAE_REQ PDU formats are defined in the Control Channel Messages spec (TIA-102.AABC family, referenced as [1] in the parent document). Implementers will need both this addendum and the AABC spec to decode/encode RAE PDUs.

---

## Standards Lineage (ASCII Tree)

```
APCO/NASTD/FED P25 Standard Suite
└── TIA-102 (Project 25 Common Air Interface)
    └── TIA-102.AA (Air Interface — Trunking Layer)
        └── TIA-102.AABD (Trunking Procedures)
            └── TIA-102.AABD-B (Revision B, Nov 2014)
                ├── TIA-102.AABD-B-1 (Addendum 1)
                ├── TIA-102.AABD-B-2 (Addendum 2 — User Alias)
                └── TIA-102.AABD-B-3 (Addendum 3 — RAE, Feb 2025) ← THIS DOCUMENT
                    Dependencies:
                    ├── TIA-102.AABC[x] — Control Channel Messages (RAE PDU formats)
                    └── TIA-102.BACD-B — ISSI (cross-RFSS failure signaling)
```

---

## Phase 3 Note

This is a **PROTOCOL** document. A Phase 3 implementation spec is warranted and should be produced in a follow-up pass covering:

- State machine tables for each actor (initiating SU, target SU, RFSS — same-site and cross-site variants)
- T_ACK timer behavior and expiry actions
- Retry sequence interaction (per AABD section 4.1.3) and relationship to the T_ACK start point
- ISSI routing decision logic (same-RFSS vs. cross-RFSS branching)
- RAE bit field position in EMRG_ALM_REQ (requires cross-referencing TIA-102.AABD-B §11.2.1 and AABC messages spec)
- Pseudocode for RFSS dispatch logic

Suggested output filename: `TIA-102-AABD-B-3_RAE_Protocol_Implementation_Spec.md`
