# TIA-102.AABG — Related Resources & Context

**Document:** TIA-102.AABG, Project 25 Conventional Control Messages
**Published:** April 2009
**Subcommittee:** TIA TR-8.10 (Trunking and Convention Control)

---

## Status

**Active.** This document was published in April 2009 as a normative TIA standard. It evolved from the earlier informative Technical Service Bulletin TSB-102.AABG-1 and was elevated to normative status in July 2007 as part of TIA's Compliance Assessment program, at which point the content of TSB-102.AABG-1 was incorporated into this document. No successor or superseding document has been identified.

The P25 Technology Interest Group (PTIG) lists this document as part of the current approved P25 TIA Standards suite. It continues to be referenced by conformance and interoperability testing programs.

---

## Standards Family

This document occupies a bridging position between the trunking and conventional branches of the TIA-102 suite. All wire-format definitions are inherited from TIA-102.AABC-B; this document provides the conventional-specific mapping, naming, and addressing rules.

### Normative Dependencies (documents this standard depends on)

| Document | Title |
|---|---|
| TIA-102.AABC-B | Trunking Control Channel Messages (source of all TSBK wire formats) |
| TIA-102.BAAA-A | Recommended Common Air Interface (physical/link layer) |
| TIA-102.BAAD | Common Air Interface Operational Description for Conventional Channels |
| TIA-102.AABD-A | Trunking Procedures |
| TIA-102.BADA | Telephone Interconnect Requirements and Definitions (Voice Service) |
| TIA-102.BADA-1 | Addendum 1 to Telephone Interconnect Requirements and Definitions |
| TIA-102.BAAC-A | Link Control Word Formats and Messages |

### Informative Context

| Document | Title |
|---|---|
| TSB-102.A | Project 25 System and Standards Definition (1995 — overall system context) |

### Companion / Related Documents

| Document | Relevance |
|---|---|
| TIA-102.AABC-B | Defines the TSBK formats used verbatim by this document |
| TIA-102.BACD | ISSI Messages and Procedures for Supplementary Data (ISSI extension) |
| TIA-102.AABD-A | Trunking Procedures (context for what conventional systems exclude) |
| TIA-102.BABA | Half-rate vocoder (parallel conventional voice standard) |

### Standards Lineage (ASCII tree)

```
TSB-102.A  (Project 25 System and Standards Definition — 1995)
├── TIA-102 Suite: Air Interface Standards
│   ├── TIA-102.BAAA-A  (Recommended Common Air Interface — FDMA CAI)
│   │   └── TIA-102.BAAD  (Conventional CAI Operational Description)
│   │       └── TIA-102.AABG  <== THIS DOCUMENT
│   │           (Conventional Control Messages — maps trunking TSBKs to conventional use)
│   └── TIA-102.AABC-B  (Trunking Control Channel Messages — provides TSBK formats)
│       └── [TIA-102.AABG references these formats by section number]
├── TIA-102.AABD-A  (Trunking Procedures)
├── TIA-102.BAAC-A  (Link Control Word Formats and Messages)
├── TIA-102.BADA / BADA-1  (Telephone Interconnect — Voice Service)
└── TIA-102.BACD  (ISSI Supplementary Data Messages)
```

---

## Practical Context

### How This Document Is Used in Real-World Systems

P25 conventional systems — which include most rural, simplex, and repeater-based public safety radio deployments — support a subset of the supplementary data features that trunked systems provide. This document is the normative reference that equipment manufacturers use when implementing those features on conventional channels.

In practice, the features defined here enable:

- **Emergency Alarm:** A subscriber radio transmits an emergency signal (EMRG_ALRM_REQ) which is displayed on dispatch consoles. The conventional RFSS does not convert the message — it is received directly by all listeners on the channel, including other mobile radios.

- **Call Alert (Selective Paging):** A radio or dispatcher can page a specific subscriber (CALL_ALRT_REQ / CALL_ALRT). On conventional channels the message is broadcast openly; the addressed subscriber's radio activates an alert.

- **Radio Check / Inhibit / Uninhibit:** Dispatchers or FNE equipment can check whether a radio is active on a channel (EXT_FNCT_CMD with Radio Check function) or remotely disable/re-enable lost or stolen radios (Inhibit/Uninhibit functions).

- **Status Update and Status Request:** Radios can send pre-programmed short status codes to dispatch (STS_UPDT_REQ), or dispatch can poll a radio for its current status (STS_Q).

- **Short Message (Text Message):** Brief text messages can be sent between subscribers or from dispatch to a radio (MSG_UPDT_REQ / MSG_UPDT).

- **Telephone Interconnect Dialing:** Subscribers can initiate telephone interconnect calls from conventional channels by transmitting dialing digits (TELE_INT_DIAL_REQ for explicit dialing, TELE_INT_PSTN_REQ for implicit/predefined number dialing).

- **Radio Unit Monitor:** Dispatchers or authorized consoles can remotely activate the microphone on a subscriber radio for covert monitoring (RAD_MON_CMD).

Because conventional systems lack a centralized channel controller, the ISP/OSP (inbound/outbound service packet) distinction from the trunking standard does not apply. Messages are not "converted" by the RFSS from request to command form — they propagate as transmitted.

### Equipment Vendor Context

Major P25 radio manufacturers (Motorola Solutions, Harris/L3Harris, Kenwood, Tait Communications, Hytera) implement the features cataloged in this document in their P25 conventional-capable subscriber and infrastructure equipment. Compliance with the conventional supplementary data feature set is tested under the P25 Compliance Assessment Program (P25 CAP) administered by DHS/SAFECOM.

---

## Key Online Resources

### Official P25 / TIA Resources

- **P25 Technology Interest Group (PTIG) — Approved Standards List:**
  https://www.project25.org/images/stories/ptig/P25_SC_22-04-003_Approved_P25_TIA_Standards_Approved_4052022.pdf

- **P25 Standards List (2023):**
  https://www.project25.org/images/stories/ptig/P25%20SC%2023-02-001-R1%20P25%20TIA%20Standards_Approved_16Feb2023.pdf

- **TIA Online (Standards Catalog):**
  https://www.tiaonline.org/standards/catalog/

- **P25 Features Matrix (2009):**
  https://project25.org/images/stories/ptig/docs/P25%20Features%20Matrix%20Combined.pdf

### Educational / Reference

- **RadioReference Wiki — APCO Project 25:**
  https://wiki.radioreference.com/index.php?title=APCO_Project_25

- **Tait Radio Academy — P25 Overview:**
  https://www.taitradioacademy.com/courses/intro-to-p25/

- **Internet Archive — TIA-102 Series Documents:**
  https://archive.org/details/TIA-102_Series_Documents

- **P25 Standards Reference (QSL.net):**
  https://www.qsl.net/kb9mwr/projects/dv/apco25/P25_Standards.pdf

- **GNU Radio P25 Technical Paper:**
  https://wiki.gnuradio.org/images/f/f2/A_Look_at_Project_25_(P25)_Digital_Radio.pdf

---

## Open-Source Implementations

The features defined in this document (TSBK-based supplementary data messages on conventional channels) are implemented in several open-source P25 decoding and monitoring projects. Because this document defines only which TSBKs apply to conventional systems — not new wire formats — any project that implements TIA-102.AABC-B TSBK decoding will inherently handle these messages.

### SDRTrunk
- **Repository:** https://github.com/DSheirer/sdrtrunk
- **Language:** Java
- **Relevance:** Full P25 Phase 1 and Phase 2 decoder. Implements TSBK parsing including Emergency Alarm, Call Alert, Status Update, Message, Extended Function Command (Radio Check, Inhibit, Uninhibit), and Telephone Interconnect messages. Conventional channel decoding is supported. The `io.github.dsheirer.module.decode.p25.phase1.message.tsbk` package contains individual TSBK message classes keyed by opcode, directly corresponding to the messages referenced in Table 2-1 of this document.

### OP25
- **Repository:** https://github.com/boatbod/op25 (and forks)
- **Language:** Python / C++ (GNU Radio blocks)
- **Relevance:** Mature P25 Phase 1 SDR decoder with TSBK message decoding. Supports monitoring of conventional channel supplementary data. Trunking and conventional modes are handled in `op25/gr-op25-r2/lib/p25_frame_assembler.cc` and related Python decoder scripts.
- **RadioReference Wiki entry:** https://wiki.radioreference.com/index.php/OP25

### p25-decoder (robotastic)
- **Repository:** https://github.com/robotastic/p25-decoder
- **Language:** C
- **Relevance:** Standalone P25 decoder with TSBK message support.

### JMBE (AMBE vocoder library — adjacent)
- **Repository:** https://github.com/DSheirer/jmbe
- **Relevance:** Not directly related to this document's message formats, but typically paired with SDRTrunk for audio decoding on conventional P25 channels.

### Implementation Notes for Conventional TSBK Decoding

When implementing the conventional supplementary data messages described in this document, key considerations include:

1. **No ISP/OSP gate:** Unlike trunked implementations, do not apply inbound/outbound filtering. Any TSBK on a conventional channel may be received by any listener.

2. **Dual-name aliases:** CALL_ALRT_REQ and CALL_ALRT share the same non-extended TSBK opcode and bit layout. Decode once; the "name" depends on context (mobile vs. FNE originator), which is derivable from the source address field.

3. **EXT_FNCT_CMD dispatch:** Radio Check, Inhibit, and Uninhibit are all carried by the same EXT_FNCT_CMD opcode. The specific function is encoded in the Extended Function field within the TSBK payload (see TIA-102.AABC-B §6.2.6).

4. **ACK_RSP_FNE with AIV set:** When parsing ACK_RSP_FNE in conventional context, expect AIV=1 and EX=0, enabling both source and target address fields to be populated for symmetric message exchange.

5. **LC_CALL_TERM_CAN:** Telephone Call Clear Down uses a Link Control word carried in the Expanded TDU/CTC (Terminator Data Unit with Link Control / Call Termination Continue) frame — not a TSBK. See TIA-102.BAAC-A §7.3.8 and TIA-102.BAAA-A for framing.

---

## Phase 3 Implementation Spec — Flag for Follow-Up

This is a MESSAGE_FORMAT document. A Phase 3 implementation spec should be produced in a follow-up pass covering:

1. **Conventional TSBK Message Dispatch Spec** — A complete alias-to-opcode mapping table cross-referencing TIA-102.AABC-B section numbers with their opcodes, so an implementer can go directly from Table 2-1 (this document) to a byte-level format definition. This requires TIA-102.AABC-B to be processed first or referenced.

2. **Conventional Addressing Rules Spec** — A structured reference for the addressing table (Table 2-2), including the AIV/EX flag requirements for ACK_RSP_FNE and the conventional-specific constraints (no ISP/OSP, no message conversion by RFSS, wireline console restrictions).

3. **Conventional vs. Trunked Feature Delta** — A concise table of what is excluded (Radio Detach, ACK_RSP_U) and why, useful for implementers porting trunking code to conventional contexts.

**Prerequisite:** TIA-102.AABC-B (Trunking Control Channel Messages) should be processed and its TSBK opcode table extracted before producing the full implementation spec for this document, since all wire formats are defined there.
