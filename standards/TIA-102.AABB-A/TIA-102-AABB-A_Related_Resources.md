# TIA-102-AABB-A Related Resources and Context

**Document:** ANSI/TIA-102.AABB-A-2005  
**Subject:** Project 25 Trunking Control Channel Formats  

---

## Status

**Superseded.** TIA-102.AABB-A (approved January 10, 2005) was superseded by **TIA-102.AABB-B**, published July 2011. A further revision, **TIA-102.AABB-C**, was in progress as of mid-2019.

This document has not been absorbed into another standard — it has continued as its own document in the TIA-102 series through multiple revision cycles. The TSBK packet structure and multi-block PDU framing it defines have remained largely stable across revisions, making the -A revision a useful reference for the original design rationale.

**Patent encumbrance:** Ericsson Inc. holds Patent No. US 5,574,788, identified as potentially applicable to this standard. Patent holders filed APCO/NASTD/FED statements of willingness to license on RAND terms.

---

## Standards Family

This document occupies a specific structural role in the P25 trunking control channel stack. It defines the *containers* (packet framing), while companion documents define the *contents* (message semantics) and the *rules* (procedures):

```
TIA-102 Suite: Trunking Control Channel
│
├── TIA-102.AABA-A  ← Trunking Overview (system architecture)
│
├── TIA-102.AABB-A  ← THIS DOCUMENT: packet framing, TSBK/PDU structure
│   (superseded by TIA-102.AABB-B, 2011)
│
├── TIA-102.AABC-A  ← Trunking Control Channel Messages (opcode definitions,
│                      message layouts for each TSBK and PDU type)
│
├── TIA-102.AABD-B  ← Trunking Procedures (call flows, state machines,
│   (TSB102.AABD)      registration, handoff, encryption parameter broadcast)
│
└── TIA-102.BAAC-A  ← CAI Reserved Values (MFID registry, opcode reservations)

Physical/Link Layer Foundation:
└── TIA-102.BAAA-A  ← P25 FDMA Common Air Interface
                       (frame sync, NID, rate 1/2 trellis, CRC, data packet formats)
                       Normative reference for this document.
```

The TSBK format defined here directly consumes:
- The Frame Sync bit pattern (TIA-102.BAAA section 8.3)
- The Network Identifier / DUID coding (TIA-102.BAAA section 8.5)
- Rate 1/2 trellis error correction (TIA-102.BAAA section 7)
- CRC-CCITT for TSBK (TIA-102.BAAA section 6.2)
- Unconfirmed Data Packet header format (TIA-102.BAAA section 6.6, for multi-block PDU)
- 4-octet PDU CRC (TIA-102.BAAA section 6.3)

---

## Practical Context

The TSBK is the most frequently transmitted unit on a P25 trunked system. On a busy dedicated control channel, an FNE will emit a continuous stream of triple-TSBK OSPs (one every 75 ms), broadcasting system status, processing call grants, and handling affiliation updates. Every radio on the system must decode and process TSBKs in real time to participate in the trunked network.

The 12-octet TSBK was deliberately compact to maximize control channel throughput. The vast majority of trunking operations — call setup, call grant, group affiliation, status broadcasts — fit comfortably within 8 argument octets. The multi-block PDU format is rarely seen in routine operation but is required for registration and neighbor site table broadcasts.

The Slotted ALOHA inbound scheme means subscriber units must decode the outbound Status Symbol stream to identify slot boundaries before transmitting. This is why radios must first receive and lock onto the OSP stream before they can send any ISP. A radio that misses its slot will retry after the next boundary — the retry logic is governed by TSB102.AABD.

The NAC (Network Access Code) within the NID serves as a site/system identifier that filters out co-channel interference in multi-site deployments. WACN (Wide Area Communication Network) ID and System ID are separately assigned per the P25 WACN guidelines.

Both protected and nonprotected TSBKs may coexist on the same control channel. Non-crypto-capable radios can still function on the channel by ignoring the encrypted payload while using the plaintext LB, P, MFID, and CRC fields to maintain framing synchronization.

---

## Key Online Resources

- **APCO Project 25 Technology Interest Group (PTIG):** https://www.project25.org/  
  Official P25 standards body; publishes approved standards lists and technical documents.

- **TIA-102 Series Document Archive (Internet Archive):**  
  https://archive.org/details/TIA-102_Series_Documents  
  Contains scans of multiple TIA-102 documents.

- **RadioReference Wiki — APCO Project 25:**  
  https://wiki.radioreference.com/index.php/APCO_Project_25  
  Community reference covering P25 trunking, TSBK decoding, and frequency coordination.

- **GNURadio Wiki — A Look at Project 25 Digital Radio (Aaron Rossetto):**  
  https://wiki.gnuradio.org/images/f/f2/A_Look_at_Project_25_(P25)_Digital_Radio.pdf  
  Good practical overview of P25 air interface including TSBK structure.

- **VIAVI Solutions — Understanding Advanced P25 Control Channel Functions:**  
  https://www.viavisolutions.com/en-us/literature/understanding-advanced-p25-control-channel-functions-discontinued-application-notes-en.pdf  
  Application note covering control channel decoding and testing.

- **Approved P25 TIA Standards list (PTIG, April 2022):**  
  https://project25.org/images/stories/ptig/P25_SC_22-04-003_Approved_P25_TIA_Standards_Approved_4052022.pdf  
  Confirms TIA-102.AABB-B as the current published revision.

- **GlobalSpec Standards Index — TIA-102.AABC:**  
  https://standards.globalspec.com/std/9909661/tia-102-aabc  
  Lists the companion Trunking Control Channel Messages document.

---

## Open-Source Implementations

Several actively maintained open-source projects implement TSBK parsing and PDU decoding. All implement the formats defined in this document and its companion TIA-102.AABC:

### SDRTrunk (Java)
- **Repository:** https://github.com/DSheirer/sdrtrunk
- Most comprehensive P25 implementation. Full TSBK opcode dispatch with standard (MFID 0x00/0x01), Motorola (MFID 0x90), and M/A-COM (MFID 0xA4) manufacturer-specific message support. Implements both single-block TSBK and multi-block PDU decoding. Actively maintained as of 2024.
- Relevant source path: `src/main/java/io/github/dsheirer/module/decode/p25/`

### Trunk Recorder (C++)
- **Repository:** https://github.com/robotastic/trunk-recorder
- Widely deployed P25 trunked recorder. Implements `decode_tsbk()` and `decode_mbt_data()` for multi-block trunking. Handles channel grants and call tracking.
- Relevant source: `trunk-recorder/systems/p25_parser.cc`

### OP25 (Python / GNURadio)
- **Repository:** https://github.com/boatbod/op25
- GNURadio-based P25 receiver. Trunking logic in `trunking.py` handles TSBK and PDU processing for voice channel tracking and control channel following.
- Relevant source: `op25/gr-op25_repeater/apps/trunking.py`

### p25rx (Rust)
- **Repository:** https://github.com/kchmck/p25rx
- Rust-based P25 receiver optimized for embedded use (Raspberry Pi). Implements trunking-aware channel hopping. Relevant for Rust-based implementations in this project.

### radiocapture-rf (Python)
- **Repository:** https://github.com/MattMills/radiocapture-rf
- P25 CAI protocol definitions including TSBK and OSP packet structure definitions.

---

## Standards Lineage

```
P25 Trunking Control Channel Formats
│
├── P25.TTG.(93)10 [1993-05-27]
│   Unprotected Trunking Control Channel (original)
│
├── P25.TTG.(93)46 [1993-09-09]
│   Combined protected and unprotected formats
│
├── P25.TTG.(93)56 + addendum [1993-10-20]
│   Integrated doc: protected+unprotected; terminology,
│   channel access, status symbol, ISP timing changes
│
├── PN-3371 ballot [1994-10-11]
│   Modifications incorporated
│
├── PN-3450 ballot [1995-02-15]
│   EF Johnson TSBK DUID/Last Block Flag proposal incorporated
│
├── Ballot 960216 [1996-02-23]
│   Further modifications
│
├── SP-4623 ballot updates [1999-2000]
│   Patent Identification added; ballot resolution
│
├── TIA-102.AABB [~2000]
│   Original published TIA standard
│
├── TIA-102.AABB-A [2005-01-10 approved, July 2005 published]
│   THIS DOCUMENT — Section 5.4 aligned with Trunking Procedures;
│   section 4.2 clarified; Reference 6 (WACN guidelines) added
│
├── TIA-102.AABB-B [July 2011]
│   Current active revision (supersedes -A)
│
└── TIA-102.AABB-C [in progress as of 2019]
    Further revision
```

---

## Phase 3 Implementation Spec — Flag for Follow-up

This document is classified **MESSAGE_FORMAT** (with lightweight PROTOCOL elements for Slotted ALOHA timing). A Phase 3 implementation spec should be produced covering:

1. **TSBK parser spec** — complete byte-level dispatch logic:
   - DUID detection (0x7 vs. 0xC from NID)
   - LB/P/Opcode extraction from octet 0
   - MFID dispatch (standard vs. manufacturer-specific)
   - Arguments parsing framework (defers to TIA-102.AABC for per-opcode layouts)
   - CRC validation (2-octet TSBK CRC)
   - Single vs. double vs. triple TSBK OSP framing

2. **Multi-block PDU parser spec** — header block dispatch:
   - FORMAT field detection (%10101 standard vs. %10111 alternative)
   - SAP identifier values (61 / 63)
   - Block count from "Blocks to Follow" field
   - Logical Link ID extraction
   - 4-octet CRC across data blocks

3. **ISP Slotted ALOHA timing spec** — for transmit-capable implementations:
   - Status Symbol parsing for slot boundary detection (IDLE = %11)
   - TONT + TOFFT + PD guard time calculation (44 bit guard for 5 micro-slot slots)

These specs should cross-reference SDRTrunk (`p25` decoder module) and Trunk Recorder (`p25_parser.cc`) as validation references.
