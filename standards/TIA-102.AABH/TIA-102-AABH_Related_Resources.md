# TIA-102.AABH Related Resources and Context

**Document:** TIA-102.AABH — Project 25 Dynamic Regrouping Messages and Procedures  
**Published:** November 2014  
**Committee:** TIA TR-8.10, Subcommittee on Trunking and Conventional Control

---

## Status

**Active (first and only edition, November 2014).** No superseding document has been published
as of April 2026. This is the initial publication of these procedures into the TIA-102 suite;
prior to this document both the MFIDA4 and MFID90 regrouping methods existed as proprietary
vendor extensions deployed in the field without a formal TIA standard.

The document references TIA-102.AABC-D and TIA-102.AABF-D (both listed as "work in progress"
in editor's notes at time of publication), indicating the broader suite was being revised in
parallel. Implementors should check for current editions of those referenced specifications.

---

## Standards Family

This document is part of the TIA-102 (Project 25 / APCO-25) suite of standards for digital
public safety land mobile radio systems. Within that suite it is classified as a Layer 3
(trunking messaging and procedure) document, covering control channel and traffic channel
signaling for the Dynamic Regrouping feature.

**Companion documents directly referenced:**

| Document | Title | Role in this context |
|----------|-------|----------------------|
| TIA-102.AABC | Trunking Control Channel Messages | Standard ISPs/OSPs reused by MFIDA4 Group Regrouping voice procedures |
| TIA-102.AABF | Link Control Word Formats and Messages | Standard LCs reused for MFIDA4 FDMA traffic channel maintenance |
| TIA-102.AABD | Trunking Procedures | Defines Group Affiliation, Group Voice Call, Emergency, and state transition rules referenced throughout |
| TIA-102.BBAC | Phase 2 Two-Slot TDMA MAC Layer | Standard MAC messages reused by MFIDA4; base for MFID90 TDMA MAC extensions |

**Related suite documents (not directly cited but closely related):**

| Document | Title |
|----------|-------|
| TIA-102.AAAA | Common Air Interface (C4FM/CQPSK) |
| TIA-102.AABG | Trunking System Registration and Authentication Messages |
| TIA-102.BABA | Phase 2 IMBE Vocoder (FDMA/TDMA) |
| TIA-102.BBAB | Phase 2 Physical Layer |

---

## Practical Context

Dynamic Regrouping is a widely deployed operational feature in P25 trunked systems used by
public safety agencies (law enforcement, fire, EMS) and military/federal customers. The feature
allows dispatch operators or system administrators to temporarily merge multiple talk groups
into a single "supergroup" without manual radio reprogramming, enabling cross-agency or
cross-unit coordination during incidents.

**Two deployed methods, both standardized here:**

- **MFIDA4 (Explicit Encryption, MFID = $A4):** Infrastructure sends the encryption Key ID
  and Algorithm ID over the air in every group regroup command. Supports both two-way patches
  and one-way simulselects. This is the older of the two methods and was already in widespread
  deployment before this document was published.

- **MFID90 (Airlink Efficient, MFID = $90):** Pre-provisions encryption configuration in
  infrastructure and SUs; regrouping messages carry only a single P-bit rather than Key ID and
  Algorithm ID. Supports two-way patches and Individual Regrouping (IR), but not simulselect.
  The "Airlink Efficient" name refers to the reduced over-the-air signaling bandwidth compared
  to MFIDA4. This method is associated with Motorola infrastructure (MFID $90 is the Motorola
  Manufacturer ID).

**Real-world deployment context:**

- Interoperability requirement: Subscriber manufacturers must implement both MFIDA4 and
  MFID90 in SUs, while infrastructure vendors may choose one or both.
- The `T_grp_rgrp` timer (default 20 seconds) means SUs automatically leave the regrouped
  state if the infrastructure stops refreshing them — a safety mechanism preventing radios from
  staying locked to a stale supergroup after an incident ends.
- Supergroup voice calls on the TDMA (Phase 2) air interface use the MAC message variants
  defined in Section 6.2; these are distinct from the FDMA TSBK/LC messages in Sections 4 and 5.
- The MFID90 MOT_GRG_ADD_CMD can batch up to three WGIDs per message, allowing
  efficient supergroup construction on busy control channels.

---

## Key Online Resources

- **TIA Standards Store (AccurisT):** Purchase the official document at
  https://store.accuristech.com/tia — search for "102.AABH"
- **TIA Online Standards Catalog:** https://www.tiaonline.org/standards/
- **APCO International Project 25 Technology Interest Group (PTIG):**
  https://www.apcointl.org/technology/project-25/
- **NIST P25 CAP (Compliance Assessment Program):** https://www.dhs.gov/p25-cap —
  tests subscriber unit interoperability; regrouping behavior is part of tested feature sets
- **DHS SAFECOM P25 Documentation:**
  https://www.cisa.gov/safecom/p25

---

## Open-Source Implementations

Both MFIDA4 and MFID90 regrouping messages appear in P25 open-source decoder projects.
The following implement relevant portions:

### SDRTrunk
- **Repository:** https://github.com/DSheirer/sdrtrunk
- **Relevance:** SDRTrunk decodes P25 Phase 1 and Phase 2 control channel messages. It
  implements decoding for Motorola (MFID90) proprietary TSBK messages including
  MOT_GRG_ADD_CMD, MOT_GRG_DEL_CMD, MOT_GRG_CN_GRANT,
  MOT_GRG_CN_GRANT_UPDT, MOT_EXT_FNCT_CMD, MOT_QUE_RSP, MOT_DENY_RSP,
  MOT_ACK_RSP_FNE, and MOT_GRG_V_REQ ISP.
- **Relevant source paths (approximate):**
  - `src/main/java/io/github/dsheirer/module/decode/p25/phase1/message/tsbk/motorola/` —
    Motorola MFID90 TSBK message decoders
  - `src/main/java/io/github/dsheirer/module/decode/p25/phase2/message/mac/` —
    TDMA MAC message decoders including MFID90 MAC messages

### OP25 (GNURadio-based P25 receiver)
- **Repository:** https://github.com/boatbod/op25
- **Relevance:** OP25 is a GNURadio-based P25 trunked receiver that tracks supergroup
  assignments for follow-me tuning. It parses MFID90 Group Regroup Add/Delete commands
  to know which voice channel to follow when a subscriber unit is regrouped.
- **Relevant source path:** `op25/gr-op25-repeater/apps/trunking.py` (group tracking logic),
  `op25/gr-op25-repeater/lib/p25p2_tdma.cc` (Phase 2 MAC parsing)

### p25craft / p25lib
Various community projects for crafting P25 packets exist on GitHub. Search for
"p25 trunking" or "p25 motorola" to find additional tooling.

---

## Standards Lineage

```
TIA-102 Suite (Project 25 / APCO-25)
│
├── Layer 1 (Physical)
│   ├── TIA-102.AAAA  — C4FM/CQPSK Common Air Interface
│   └── TIA-102.BBAB  — Phase 2 Physical Layer (TDMA)
│
├── Layer 2 (Data Link / MAC)
│   ├── TIA-102.AABF  — Link Control Word Formats (FDMA)
│   └── TIA-102.BBAC  — Phase 2 TDMA MAC Layer Description
│
└── Layer 3 (Network / Trunking)
    ├── TIA-102.AABD  — Trunking Procedures
    │   └── (Group Affiliation, Call Setup, Emergency, Announcement Groups)
    ├── TIA-102.AABC  — Trunking Control Channel Messages
    │   └── (Standard ISPs / OSPs for voice services)
    ├── TIA-102.AABG  — Registration and Authentication Messages
    └── TIA-102.AABH  — Dynamic Regrouping Messages and Procedures [THIS DOCUMENT]
        ├── Section 2: MFIDA4 Explicit Encryption Group Regrouping Procedures
        ├── Section 3: MFID90 Airlink Efficient Group Regrouping Procedures
        ├── Section 4: TSBK Control Channel Messages (ISPs and OSPs)
        ├── Section 5: FDMA Link Control Messages
        └── Section 6: TDMA MAC Messages + Call Sequence Diagrams (Annexes A–D)
```

---

## Notes for Implementors

1. **Dual-method requirement:** A conformant subscriber unit must respond correctly to both
   MFIDA4 and MFID90 commands. Infrastructure may implement either or both.

2. **SP-WGID pool management:** The SP-WGID pool must be distinct from the WGID space
   used for normal talkgroups. The SSN (Supergroup Sequence Number) in MFIDA4 must be
   changed whenever an SP-WGID is reused.

3. **TDMA vs FDMA:** The MFID90 voice-channel LC words (Section 5.2) are used on FDMA
   (Phase 1) channels; the MAC messages (Section 6.2) are used on TDMA (Phase 2) channels.
   The abbreviated MAC format is used on the SU's home system; the extended format (16
   bytes, includes WACN/System/Source ID) is used when roaming.

4. **Emergency call handling differs by method:** Under MFIDA4, patch group emergencies are
   processed on the SP-WGID. Under MFID90, an emergency PTT causes the SU to exit the
   Group Regrouped state and send a standard GRP_V_REQ on the emergency working group.

5. **Phase 3 implementation spec flagged for follow-up:** This document warrants a
   MESSAGE_FORMAT + PROTOCOL implementation spec covering the opcode dispatch table for
   MFID90 TSBK messages (ISP and OSP), field parsing rules for GRG_EXENC_CMD including
   the GRG_Options bit field, the MFID90 MOT_GRG_CN_GRANT_EXP multi-block format,
   and SU state machine transitions for both MFIDA4 and MFID90 regrouping states.
