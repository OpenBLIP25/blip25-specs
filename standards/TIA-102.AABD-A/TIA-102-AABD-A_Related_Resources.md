# TIA-102-AABD-A: Related Resources and Context

**Document covered:** TIA-102.AABD-A-2 — Project 25 Trunking Procedures, Addendum 2  
**Published:** September 2012 (Addendum 2); base standard December 2008

---

## Status

**Absorbed / Superseded.**

TIA-102.AABD-A-2 (this addendum) was superseded when TIA-102.AABD-B was published in
November 2014. The TIA press release explicitly states that AABD-B "absorbs the existing
Composite Control Channel and Conventional Fallback addendums (TIA-102.AABD-A-1 and
TIA-102.AABD-A-2 respectively)" while adding additional clarifications to wide-area
trunking operation. The addendum's content — specifically the Conventional Fallback
procedures (Section 16) and the updated control channel hunt state machine (Section 5) —
is now incorporated into the current consolidated trunking procedures standard.

The base document TIA-102.AABD-A (December 2008) has similarly been superseded by
AABD-B. Equipment implemented against AABD-A plus its addenda should be functionally
equivalent to equipment implemented against AABD-B for the features covered by this
addendum, since the normative content was carried forward unchanged.

---

## Standards Family

This document is part of the **TIA-102 "AA" (FDMA Trunking) cluster**, specifically:

```
TIA-102 (P25 Standards Suite)
└── AA Series: Trunking
    ├── TIA-102.AABA   — Trunking Overview
    ├── TIA-102.AABB   — Trunking Control Channel Messages
    ├── TIA-102.AABC   — RFSS-to-RFSS Messages (ISS)
    ├── TIA-102.AABD   — Trunking Procedures (original, superseded)
    │   ├── TIA-102.AABD-A   — Trunking Procedures (Dec 2008)
    │   │   ├── TIA-102.AABD-A-1  — Addendum 1: Composite Control Channel (Apr 2011)
    │   │   └── TIA-102.AABD-A-2  — Addendum 2: Conventional Fallback (Sep 2012) ← THIS DOCUMENT
    │   └── TIA-102.AABD-B   — Trunking Procedures, consolidated revision (Nov 2014)
    │                           [absorbs -A-1 and -A-2]
    └── TIA-102.AABE   — Trunking Procedures for Interconnect
```

### Normative references within this addendum

| Document | Title | Role |
|----------|-------|------|
| TIA-102.AABD-A | Trunking Procedures (base) | All trunking procedures not in this addendum |
| TIA-102.BAAD-A (Feb 2010) | Project 25 Conventional Procedures | SU and repeater behavior during active fallback calls |

### Closely related documents

| Document | Relevance |
|----------|-----------|
| TIA-102.AABB | Defines the LC_CONV_FALLBACK Link Control Word bit format used throughout this addendum |
| TIA-102.BAAD-A | Defines conventional repeater voice/data procedures referenced by FNE and SU during fallback |
| TIA-102.AABD-B | Current consolidated trunking procedures; this addendum's content is incorporated there |
| TIA-102.AABD-A-1 | Companion addendum; adds Composite Control Channel procedures |

---

## Practical Context

### What problem this solves

Trunked P25 systems require a functioning RFSS (RF Subsystem) controller and control
channel to operate. When that infrastructure fails — hardware fault, power outage,
maintenance window, or disaster — radios configured for trunked-only operation cannot
communicate. Prior to the introduction of Conventional Fallback, subscribers in the
affected coverage area were simply out of service.

Conventional Fallback allows a repeater with appropriate firmware intelligence to
automatically detect the loss of trunking control and transition to broadcasting a
Conventional Fallback Link Control Word (LC_CONV_FALLBACK). P25 subscriber radios
that are configured for fallback will, upon exhausting their control channel hunt sequence,
scan for these fallback signals and attach to a conventional repeater service. This
enables basic push-to-talk voice communications to continue in the coverage area even
without the trunking controller.

### Real-world deployment considerations

- **Configuration is required.** Both the FNE (repeater infrastructure) and SUs must
  be provisioned with matching Fallback Channel IDs. This requires coordinated
  planning by the system administrator and is typically done during system commissioning.

- **Graceful degradation.** The design is intentionally minimal: talkgroup continuity,
  encryption parameter continuity, and NAC continuity are all addressed, allowing
  users to maintain communication habits close to normal operation during an outage.

- **Interoperability boundary.** Because the LC_CONV_FALLBACK format is defined
  in TIA-102.AABB and the conventional channel behavior references TIA-102.BAAD-A,
  conventional fallback provides an interoperability "floor" across vendors, unlike purely
  proprietary failsoft solutions.

- **Typical vendors.** Motorola Solutions (APX family), Harris (XL series), Kenwood,
  EF Johnson, and other P25 FDMA equipment vendors implement conventional fallback
  per this standard in infrastructure and subscriber products.

---

## Key Online Resources

- **TIA Press Release — TIA-102.AABD-B publication (Nov 2014):**
  [TIA Issues New Project 25 Trunking Procedures Standard](https://standards.tiaonline.org/tia-issues-new-project-25-trunking-procedures-standard)
  — Confirms AABD-B absorbed addenda -A-1 and -A-2.

- **RR Media Group news item on AABD-B:**
  [TIA Publishes New P25 Trunking Procedures Document (Nov 2014)](https://www.rrmediagroup.com/News/NewsDetails/NewsID/11410)

- **GlobalSpec standards entry for TIA-102.AABD:**
  [TIA-102.AABD — Project 25 Trunking Procedures](https://standards.globalspec.com/std/9880063/TIA-102.AABD)

- **P25 standards registry (PTIG, April 2022):**
  [Approved P25 TIA Standards](https://project25.org/images/stories/ptig/P25_SC_22-04-003_Approved_P25_TIA_Standards_Approved_4052022.pdf)

- **TIA-102 Series on Internet Archive:**
  [TIA-102_Series_Documents](https://archive.org/details/TIA-102_Series_Documents)
  — Collection of TIA-102 document scans.

- **Motorola Solutions TDMA Trunking White Paper:**
  [Project 25 TDMA Trunking Suite — White Paper (2013)](https://www.motorolasolutions.com/content/dam/msi/Products/apx-p25/P25_TDMA_Trunking_Standard_White_Paper.pdf)
  — Industry context on the evolution of P25 trunking standards including conventional fallback.

- **RadioReference APCO Project 25 Wiki:**
  [APCO Project 25 — RadioReference Wiki](https://wiki.radioreference.com/index.php/APCO_Project_25)
  — Practical user-level documentation on P25 system types and conventional fallback.

---

## Open-Source Implementations

### SDRTrunk — `DSheirer/sdrtrunk`

SDRTrunk is the primary open-source P25 decoder and monitor. It implements
LC_CONV_FALLBACK decoding as `LCConventionalFallback` in the P25 Phase 1 link control
message hierarchy.

- **GitHub repository:** [https://github.com/DSheirer/sdrtrunk](https://github.com/DSheirer/sdrtrunk)
- **Issue #1934 — P25P1 Conventional Fallback Link Control Message:**
  [https://github.com/DSheirer/sdrtrunk/issues/1934](https://github.com/DSheirer/sdrtrunk/issues/1934)
  Reported a `toString()` override defect in the `LCConventionalFallback` class;
  fixed in PR #1935 and released in v0.6.1 (June 2024). This issue confirms that
  SDRTrunk decodes and processes the LC_CONV_FALLBACK LCW per this standard.

The state machine described in Sections 5 and 16 of this addendum — extended hunt
escalating to fallback channel scan, channel selection by Fallback CH ID / NAC matching,
wildcard channel fallback, and exit on missed LCWs — maps directly to logic that SDRTrunk
would need to implement for trunked system monitoring that follows a site into fallback mode.

### OP25 — `boatbod/op25`

OP25 is a GNU Radio-based P25 decoder.

- **GitHub:** [https://github.com/boatbod/op25](https://github.com/boatbod/op25)
- OP25's trunking FSM (`trunking.py`, `rx.py`) handles P25 Phase 1 control channel
  acquisition and conventional channel monitoring; conventional fallback handling
  in the SDR monitoring context may not be fully implemented but the LCW decode path
  is present.

---

## Standards Lineage

```
TIA-102 (P25 digital radio standards suite)
│
├── AA-series: Trunking
│   │
│   ├── TIA-102.AABA   Project 25 Trunking Overview
│   ├── TIA-102.AABB   Trunking Control Channel Messages (defines LC_CONV_FALLBACK format)
│   │
│   └── TIA-102.AABD   Trunking Procedures
│       │
│       ├── TIA-102.AABD-A   Base standard (December 2008)          [superseded]
│       │   ├── TIA-102.AABD-A-1   Addendum 1 (April 2011)         [absorbed into AABD-B]
│       │   └── TIA-102.AABD-A-2   Addendum 2 (September 2012) ◄── THIS DOCUMENT
│       │                          [absorbed into AABD-B]
│       │
│       └── TIA-102.AABD-B   Consolidated revision (November 2014)  [current]
│
└── BA-series: Conventional
    └── TIA-102.BAAD-A   Conventional Procedures (February 2010)
                         (referenced normatively by this addendum)
```
