# TIA-102-AABF-C — Related Resources and Context

**Document:** ANSI/TIA-102.AABF-C-1-2012  
**Title:** Project 25 — Link Control Word Formats and Messages, Addendum 1  
**Last updated:** 2026-04-13

---

## Status

**TIA-102.AABF-C** (the parent standard) is the base document in the C revision cycle for Link Control Word Formats and Messages. This addendum (-1) was approved December 20, 2012 and published January 2013.

The addendum itself states that its contents are intended to be incorporated into "the next complete revision of TIA-102.AABF-C." As of the time of this writing, it is not confirmed whether a TIA-102.AABF-D revision has been published that absorbed this addendum. Users should verify the current revision at the TIA standards catalog.

**Likely status:** The Conventional Fallback LCW (LC_CONV_FALLBACK, LCO=42) defined here is expected to appear in any current-revision P25 LCW specification. The addendum itself may be listed as superseded if a full revision of AABF has been published.

---

## Standards Family

This document is part of the **TIA-102 (Project 25) CAI standards suite**, specifically in the **AABF series** covering Link Control Word formats. The broader family structure:

```
TIA-102 (Project 25)
└── Series AA — Common Air Interface (CAI)
    └── AABF — Link Control Word Formats and Messages
        ├── TIA-102.AABF      (original)
        ├── TIA-102.AABF-A    (revision A)
        ├── TIA-102.AABF-B    (revision B)
        ├── TIA-102.AABF-C    (revision C — parent of this addendum)
        │   └── TIA-102.AABF-C-1  [THIS DOCUMENT — Addendum 1]
        └── TIA-102.AABF-D?   (possible future revision absorbing this addendum)

Related companion documents:
├── TIA-102.AABC     — Trunking Control Channel Messages (trunking PDU formats)
├── TIA-102.AABF-*   — All LCW revisions (same series)
├── TIA-102.BAAA     — Trunking Protocol (procedures using these LCWs)
├── TIA-102.BBAB     — Physical Layer Description (burst structure carrying LCWs)
├── TIA-102.BABA     — Conventional Protocol
└── TIA-102.BCAB     — Conformance Testing (if applicable)
```

The LCW series (AABF) defines wire formats; behavioral use of those formats is defined in the protocol-layer documents (BAAA, BABA, etc.). The physical transport of LCWs within voice and control channel bursts is defined in BBAB.

---

## Practical Context

### Conventional Fallback / Failsoft in P25 Systems

P25 trunked systems are designed to gracefully degrade when infrastructure components fail. "Conventional fallback" (also called "failsoft") is the mode where trunked radio infrastructure has lost coordination capability — typically due to backhaul failure, site controller failure, or repeater isolation — and subscriber units revert to operating on a fixed conventional channel through a local repeater.

The Conventional Fallback LCW (LC_CONV_FALLBACK) defined in this addendum provides the signaling mechanism for this transition on the Common Air Interface. A repeater that detects it has entered fallback mode transmits this LCW outbound, carrying up to six Fallback Channel IDs. Subscriber units configured with matching Fallback CH IDs can locate the appropriate repeater and continue communications.

### Real-World Deployment

This message type is relevant in any P25 infrastructure deployment that supports:
- Multi-site trunked systems with site isolation risk (e.g., Motorola ASTRO 25, Harris OpenSky, Kenwood NEXEDGE P25)
- Simulcast networks where individual sites may enter failsoft independently
- Any P25 subscriber unit implementing failsoft operation per TIA-102.BAAA procedures

The Failsoft Network Flag (N bit) distinguishes between a single-site repeater going fallback versus a networked/simulcast system continuing to operate in fallback — an important operational distinction for dispatchers and field units.

The Alert Tone Sync Flag (T bit) allows network-coordinated audio notification to field personnel at the moment failsoft activates, without requiring each unit to independently detect the transition.

---

## Key Online Resources

- **TIA Standards Catalog** — The authoritative source for current document status and purchasing:  
  https://www.tiaonline.org/standards/catalog/

- **APCO Project 25 Technology Interest Group (PTIG)** — The end-user/agency-facing organization for P25 standards:  
  https://www.apcointl.org/technology/project-25/

- **PTIG P25 Statement of Requirements** — High-level requirements driving P25 specifications:  
  https://www.apcointl.org/technology/project-25/p25-resources/

- **NIST / DHS SAFECOM P25 Compliance Assessment Program (CAP)** — Federal compliance testing for P25 equipment; relevant to AABF-C-1 for failsoft interoperability:  
  https://www.dhs.gov/safecom/p25-compliance-assessment-program

- **IHS Markit / Accuris Standards Store** — Current reseller for TIA standards:  
  https://store.accuristech.com/tia

---

## Open-Source Implementations

The Conventional Fallback LCW (LCO=42) introduced by this addendum is present in the P25 LCW decode tables of the following open-source projects. Note that older versions may predate this addendum (approved 2012) and may not include LCO=42 if they were written against an earlier revision of AABF.

### OP25 (GNU Radio P25 Implementation)
- **Repository:** https://github.com/boatbod/op25
- **Relevant code:** `op25/gr-op25-r2/lib/p25_lc_block.cc` or equivalent LCW dispatch tables
- OP25 decodes P25 FDMA voice channel Link Control Words; search for `LC_CONV_FALLBACK` or opcode `0x2a` (42 decimal = 0x2a hex)
- OP25 focuses on receive-side monitoring; failsoft LCW decoding is used for display/logging

### SDRTrunk
- **Repository:** https://github.com/DSheirer/sdrtrunk
- **Relevant code:** `src/main/java/io/github/dsheirer/module/decode/p25/phase1/message/lc/` — LCW class hierarchy
- SDRTrunk implements comprehensive P25 Phase 1 LCW decoding; LC_CONV_FALLBACK should appear as a decoded message type
- Search for `CONVENTIONAL_FALLBACK` or opcode value `42` / `0x2a` in the LCW package

### p25lib / multimon-ng
- **multimon-ng:** https://github.com/EliasOenal/multimon-ng
- Implements basic P25 LCW decoding; may or may not include post-2012 addenda

### Trunk Recorder
- **Repository:** https://github.com/robotastic/trunk-recorder
- Relies on OP25 / gr-p25 libraries for LCW parsing; failsoft detection depends on upstream library support

---

## Standards Lineage

```
APCO/NASTD/FED MOU (December 1993)
└── Project 25 (P25) Digital Radio Standard
    └── TIA-102 Standards Suite
        └── Series AA — Common Air Interface
            └── AABF series — Link Control Word Formats and Messages
                ├── TIA-102.AABF     (original publication)
                │   └── defines the LCW structure and initial opcode table
                ├── TIA-102.AABF-A   (revision A)
                │   └── adds/modifies LCW types
                ├── TIA-102.AABF-B   (revision B)
                │   └── adds/modifies LCW types
                ├── TIA-102.AABF-C   (revision C)
                │   └── current base document at time of this addendum
                │       Provides: complete LCW opcode table through LCO=41,
                │                 all field definitions, usage tables
                └── TIA-102.AABF-C-1 [THIS DOCUMENT]
                    └── Adds: LCO=42 (LC_CONV_FALLBACK), Fallback CH ID field,
                             N (Failsoft Network) flag, T (Alert Tone Sync) flag
                             LCO values 43-63 reserved
```

---

## Notes for Implementors

- **LCO dispatch:** Add case `0x2a` (42) to your LCW opcode dispatch table.
- **Fixed length:** Like all P25 LCWs, this word is exactly 9 octets (72 bits). No length disambiguation needed.
- **Fallback CH ID array:** Octets 3–8 are six independent CH ID bytes. De-duplicate before use (repeated values fill unused slots per spec). Value 0x00 is wild-card.
- **Flag bits in octet 2:** T = bit 7, N = bit 6. Bits 5–0 are reserved (mask to zero for comparison).
- **Phase 3 implementation spec:** A MESSAGE_FORMAT implementation spec for LC_CONV_FALLBACK should be produced as a follow-up pass. Content would include: opcode dispatch pseudocode, Rust struct definition for the decoded LCW, byte extraction code, CH ID de-duplication logic, and cross-reference to SDRTrunk/OP25 implementations.
