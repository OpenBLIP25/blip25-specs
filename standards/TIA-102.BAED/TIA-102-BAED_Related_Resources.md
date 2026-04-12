# TIA-102-BAED Related Resources and Context

**Document:** TIA-102.BAED — Project 25 Packet Data Logical Link Control Procedures  
**ANSI designation:** ANSI/TIA-102.BAED-2013  
**Approved:** September 26, 2013

---

## Status

**Active.** This document is the 1st (Original) edition and, as of the knowledge cutoff of this extraction, has not been superseded. It was published as a standalone document in 2013, having been extracted from Annex E of TIA-102.BAAD-A (Project 25 Conventional Procedures). The foreword explicitly states it "cancels and replaces the material specific to the Packet Data LLC procedures specified in Annex E of TIA-102.BAAD-A."

No later revision (e.g., TIA-102.BAED-A or TIA-102.BAED-B) is known to exist as of this writing. Verify current status at the TIA standards catalog: https://www.tiaonline.org/standards/catalog/

---

## Standards Family

This document belongs to the **TIA-102 BAE-series**, which covers P25 packet data services. The BAE series sits within the broader TIA-102 B-series (Common Air Interface documents) and the BA-series (FDMA CAI procedures).

**Direct dependencies (normative references):**

| Document | Title | Role in this document |
|----------|-------|----------------------|
| TIA-102.AAAD-A | P25 Block Encryption Protocol | Encryption of logical message fragments; Auxiliary Header chaining |
| TIA-102.AABD-A | Trunking Procedures | NAC qualification for trunked channels |
| TIA-102.BAAA-A | P25 FDMA Common Air Interface | CAI packet formats, CRC definitions, FSNF, header block structure, data block layout |
| TIA-102.BAAC-C | CAI Reserved Values | MFID standard values; everyone designator for LLID |
| TIA-102.BAAD-A | Conventional Procedures | NAC qualification for conventional channels; FS_R sender/receiver processing |
| TIA-102.BAEA-B | P25 Data Overview and Specification | Four data configurations; Um Interface definition; SU, FNE, FS_R role definitions |
| TIA-102.BAEJ | Conventional Management Service Specification for Packet Data | CMS function; Packet Data Supplementary Encryption Info; Static Packet Data Registration (SYN trigger) |

**Closely related documents in the P25 packet data stack:**

```
TIA-102.BAEA-B  (Data Overview and Specification)
       │
       ├── TIA-102.BAED   (LLC Procedures)           ← this document
       │         │
       │         └── TIA-102.BAAA-A (CAI — packet formats, CRCs, FSNF)
       │
       ├── TIA-102.BAEE   (SNDCP Procedures)
       ├── TIA-102.BAEJ   (CMS for Packet Data)
       └── TIA-102.BAEF   (Packet Data Gateway Procedures)
```

**Broader P25 standards lineage:**

```
TIA-102 Series (Project 25)
├── AAA-series: Security / Encryption (AAAD-A: Block Encryption)
├── AAB-series: Core Procedures (AABD-A: Trunking)
├── BAA-series: FDMA Common Air Interface
│   ├── TIA-102.BAAA-A  CAI Physical/Link Layer
│   ├── TIA-102.BAAC-C  Reserved Values
│   ├── TIA-102.BAAD-A  Conventional Procedures (source of Annex E → BAED)
│   └── TIA-102.BAAE    Trunked Procedures
├── BAE-series: Packet Data Services
│   ├── TIA-102.BAEA-B  Data Overview and Specification
│   ├── TIA-102.BAED    LLC Procedures              ← this document
│   ├── TIA-102.BAEE    SNDCP Procedures
│   ├── TIA-102.BAEF    Packet Data Gateway
│   └── TIA-102.BAEJ    CMS for Packet Data
└── (other series: BBA TDMA, C-series test/conformance, etc.)
```

---

## Practical Context

### Where LLC fits in the P25 packet data stack

P25 packet data uses a layered architecture. From bottom to top:

1. **Physical / RF layer** — modulation, FDMA channelization
2. **CAI (TIA-102.BAAA-A)** — frame sync, NID, data unit framing, CRC-9 block integrity, 32-bit packet CRC, header formats
3. **LLC (TIA-102.BAED)** — confirmed/unconfirmed conveyance, ARQ, segmentation/reassembly ← **here**
4. **SNDCP (TIA-102.BAEE)** — IP/protocol adaptation (compression, header stripping)
5. **Application** — IP traffic, CMS messaging

The LLC layer is the only layer that provides over-the-air reliability for packet data in P25 FDMA. It implements a stop-and-wait ARQ with selective retransmission (SACK) — more sophisticated than pure stop-and-wait, but simpler than a sliding-window protocol. This is appropriate given the low data rates and high latency of P25 FDMA packet data channels (typically 9.6 kbps payload, with channel access delays measured in seconds).

### Deployment context

- **Mobile data terminals (MDTs)** in police/fire/EMS vehicles use the confirmed conveyance mode for reliable data delivery (CAD queries, records checks, AVL updates).
- **Direct Data mode** (SU-to-SU) uses symmetric addressing; the two SUs maintain their own sequence number pairs.
- **FNE-based configurations** (conventional and trunked) use asymmetric addressing; the FNE maintains one V(S)/V(R) pair per SU in service.
- **Encryption integration**: The LLC layer handles encrypted logical messages transparently — it passes through Auxiliary Headers from TIA-102.AAAD-A and notifies the CMS function on decryption failure, enabling the Supplementary Encryption Info service (defined in TIA-102.BAEJ).
- **SNDCP interaction**: On the receiver side, the LLC layer delivers reassembled logical messages to the SAP. If the SAP is Packet Data Scan Preamble and SNDCP is active, the message goes to SNDCP for IP adaptation; otherwise it goes to CMS.

### Interoperability significance

Because P25 is a public safety interoperability standard, the LLC procedures must be consistent across all manufacturer implementations. The extraction of these procedures from TIA-102.BAAD-A (conventional-only) into a standalone document covering all four data configurations was an important step for ensuring that trunked and direct-data implementations follow the same LLC behavior.

---

## Key Online Resources

- **TIA Standards Catalog** (official source for current version and purchase):  
  https://www.tiaonline.org/standards/catalog/

- **IHS Markit / Accuristech** (purchase access):  
  https://store.accuristech.com/tia

- **APCO Project 25 Technology Interest Group (PTIG)**:  
  https://www.project25.org/  
  PTIG oversees P25 compliance testing and publishes implementation guidance for P25 packet data systems.

- **DHS SAFECOM P25 CAP (Compliance Assessment Program)**:  
  https://www.dhs.gov/safecom/p25  
  The P25 CAP tests equipment for conformance; packet data interoperability is part of the test suite.

- **NIST P25 documentation** (public safety communications research):  
  https://www.nist.gov/ctl/pscr/project-25-standards

---

## Open-Source Implementations

Several open-source P25 projects implement the packet data stack, including the LLC layer described in this document:

- **OP25** (SDR-based P25 receiver, Python/C++):  
  https://github.com/boatbod/op25  
  Implements P25 packet data reception including LLC reassembly. The data handling code handles confirmed and unconfirmed packet parsing. Most relevant files are in `op25/gr-op25_repeater/lib/`.

- **SDRTrunk** (Java-based P25 decoder):  
  https://github.com/DSheirer/sdrtrunk  
  Implements P25 packet data decoding including the LLC layer. Relevant classes are under `src/main/java/io/github/dsheirer/module/decode/p25/` in the packet data handling subsystem. Look for classes handling `PacketData` and `LLC`.

- **gr-p25** / **dsd** (Digital Speech Decoder):  
  https://github.com/szechyjs/dsd  
  Lower-level P25 decoder; partial packet data support.

- **Osmocom project** (osmo-gmr, broader SDR ecosystem):  
  https://osmocom.org/  
  Some P25 work exists in the broader Osmocom ecosystem, though P25 packet data is less covered than voice.

**Notes for implementors:**
- The confirmed conveyance stop-and-wait state machines (§3.2.5 and §3.2.6) are the core implementation target. The 13-step receiver state machine in §3.2.6 is particularly detailed and covers the FMF=0 edge case (step 2.1), the P(R) disambiguation variable (step 3.3), and the FSNF-based reassembly state machine (step 12).
- CRC types used: CRC_BLOCK (9-bit, per TIA-102.BAAA-A §CRC-9), CRC_HEADER (16-bit), CRC_PACKET (32-bit). These are defined in TIA-102.BAAA-A, not in this document.
- The BTF padding formulas differ between confirmed (÷16, P_MAXC=15) and unconfirmed (÷12, P_MAXU=11) — this reflects the different data block sizes in the respective CAI packet formats.

---

## Phase 3 Flag

**This document warrants a Phase 3 implementation spec (PROTOCOL category).**

Recommended follow-up: **`TIA-102-BAED_LLC_State_Machine_Spec.md`**

Should include:
- State tables for both confirmed sender (§3.2.5) and confirmed receiver (§3.2.6) procedures
- State table for unconfirmed sender (§3.3.4) and receiver (§3.3.5)
- Decision trees for BTF padding calculation (confirmed vs. unconfirmed)
- FSNF encoding/decoding logic with FSN wrap rule (7→1)
- V(S)/V(R) modulo-8 arithmetic and duplicate detection logic (CRC_LAST comparison)
- SYN resynchronization trigger conditions
- Pseudocode for the FMF=0 receiver edge case (step 2.1 of §3.2.6)
- Rust-specific notes: suitable data structures for per-SU sequence number state, fragment reassembly buffers
