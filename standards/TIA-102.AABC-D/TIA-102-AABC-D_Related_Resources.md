# TIA-102.AABC-D: Related Resources
## Project 25 Trunking Control Channel Messages

---

## Standards and Official Documentation

### TIA Standards Index
- **[P25 SC Approved TIA Standards (June 2019)](https://www.project25.org/images/stories/ptig/P25_SC_19-06-002-R1_Approved_P25_TIA_Standards_-_June_2019.pdf)** — Official PTIG document listing all approved TIA-102 standards including the AABC series. Useful for understanding which addenda (AABC-D-1 through D-3) were later absorbed into AABC-E.

- **[TIA-102.AABC on GlobalSpec](https://standards.globalspec.com/std/9909661/tia-102-aabc)** — Standards database listing for TIA-102.AABC family; includes cross-references to predecessor and successor revisions.

- **[TIA-102.AABA Trunking Overview on GlobalSpec](https://standards.globalspec.com/std/14211744/TIA-102.AABA)** — The companion trunking overview standard that describes the trunking system architecture within which AABC-D messages operate.

- **[TIA-102 Series Archive at Internet Archive](https://archive.org/details/TIA-102_Series_Documents)** — Collection of TIA-102 series documents available for research.

- **[P25 TDMA Trunking Standards White Paper (Motorola Solutions, 2013)](https://www.motorolasolutions.com/content/dam/msi/Products/apx-p25/P25_TDMA_Trunking_Standard_White_Paper.pdf)** — Industry white paper covering the P25 TDMA trunking suite including AABC-series message context for two-slot TDMA operation.

---

## Open Source Implementations

### kchmck/p25.rs (Rust)
- **[GitHub: kchmck/p25.rs — tsbk.rs](https://github.com/kchmck/p25.rs/blob/master/src/trunking/tsbk.rs)** — The most directly relevant open source reference: a Rust implementation of TSBK decoding. Implements the TSBK receiver state machine (buffer dibits → descramble → decode convolutional code → parse payload). Covers ISP/OSP dispatch by IO+opcode.

- **[p25::trunking::tsbk module docs](http://kchmck.github.io/doc/p25/trunking/tsbk/index.html)** — Rendered Rust API documentation for the TSBK module. Shows the message type enumeration and payload struct design.

- **[TsbkReceiver struct](https://kchmck.github.io/doc/p25/trunking/tsbk/struct.TsbkReceiver.html)** — Receiver implementation details: dibit buffering, deinterleaving, Viterbi decode, CRC check, and message dispatch.

### OP25 (GNU Radio / Python / C++)
- **[OP25 — RadioReference Wiki](https://wiki.radioreference.com/index.php/OP25)** — Community documentation for OP25, the primary open source P25 Phase I and Phase II software-defined radio decoder. Covers TSBK decoding on live trunked systems at ~40 control channel messages/second.

- **[OP25 Decoder Page — Osmocom](https://projects.osmocom.org/projects/op25/wiki/DecoderPage)** — Technical detail on the OP25 decoder architecture: demodulation, decoding, and frame dispatch pipeline.

- **[GitHub: osmocom/op25](https://github.com/osmocom/op25)** — Osmocom fork of OP25. Contains TSBK parser and trunk tracker implementing GRP_V_CH_GRANT, registration, affiliation, and other messages from AABC-D.

- **[GitHub: boatbod/op25](https://github.com/boatbod/op25)** — Actively maintained OP25 fork with Phase II trunking support. Trunk tracker processes TSBK OSPs to follow channel grants across a P25 trunked system.

- **[OP25 Guide — Aaron Swartz Day Hackathon](https://www.aaronswartzday.org/op25/)** — Step-by-step OP25 setup guide including how to configure system ID, control channel frequency, and NAC for a trunked P25 system.

- **[GitHub: robotastic/p25-decoder](https://github.com/robotastic/p25-decoder)** — Alternative P25 decoder focused on recording trunked P25 calls via SDR; implements TSBK channel grant following.

---

## Educational and Technical Reference

### Signal Analysis and Protocol Overview
- **[APCO Project 25 — RadioReference Wiki](https://wiki.radioreference.com/index.php/APCO_Project_25)** — Comprehensive community reference covering P25 system architecture, TSBK basics, trunking operation, RFSS/WACN topology, and links to further technical resources.

- **[Project 25 (P25) — SigID Wiki](https://www.sigidwiki.com/wiki/Project_25_(P25))** — Signal identification page with spectral and technical descriptions of P25 Phase I and Phase II waveforms; useful for understanding the physical layer context in which AABC-D messages ride.

- **[A Look at Project 25 Digital Radio — GNURadio Wiki (Aaron Rossetto, NI)](https://wiki.gnuradio.org/images/f/f2/A_Look_at_Project_25_(P25)_Digital_Radio.pdf)** — Accessible technical overview of P25 from physical layer through trunking, including TSBK structure and control channel message flow.

- **[A Software-Defined Radio Receiver for APCO Project 25 Signals (ResearchGate)](https://www.researchgate.net/publication/220761783_A_software-defined_radio_receiver_for_APCO_project_25_signals)** — Academic paper describing SDR-based P25 receiver design; covers TSBK framing and message parsing in an implementation context.

### VIAVI (formerly JDSU/Aeroflex) Application Notes
- **[Understanding Advanced P25 Control Channel Functions](https://www.viavisolutions.com/en-us/literature/understanding-advanced-p25-control-channel-functions-discontinued-application-notes-en.pdf)** — Application note explaining TSBK message classes, trunked call setup signaling sequences, and control channel monitoring from a test equipment perspective.

- **[Understanding 800 MHz and VHF/UHF Implicit P25 Trunking Functions](https://www.viavisolutions.com/en-us/literature/understanding-800-mhz-and-vhf-uhf-implicit-p25-trunking-functions-using-ifr-29-application-notes.pdf)** — Covers band-specific trunking; relevant to IDEN_UP, IDEN_UP_VU, and IDEN_UP_TDMA message context.

### Industry and Vendor Resources
- **[Tait Radio Academy: Introduction to P25](https://www.taitradioacademy.com/courses/intro-to-p25/trg-00001-01-m_intro_to_p25-2/)** — Vendor training material; covers WACN/RFSS/Site hierarchy, registration, affiliation, and control channel function in operational context.

- **[Tait Radio Academy: Channel Configuration and Numbering](https://www.taitradioacademy.com/topic/p25-channel-configuration-numbering-1/)** — Explains channel identifier encoding (4-bit ID + 12-bit number) and how IDEN_UP messages define the frequency plan; directly relevant to AABC-D channel fields.

- **[P25 Trunking Setup Quick Start Guide (Tait)](https://partnerinfo.taitradio.com/__data/assets/pdf_file/0010/198631/MMB-00034-01.pdf)** — Practical configuration guide illustrating WACN, System ID, RFSS, Site configuration — the parameters broadcast via NET_STS_BCST, RFSS_STS_BCST, and ADJ_STS_BCST messages.

- **[Taking Control of Project 25 Systems — Urgent Communications (2004)](https://urgentcomm.com/2004/11/01/taking-control-of-project-25-systems/)** — Historical article on P25 trunking control channel operation; provides background on the original AABC message design intent.

- **[RFSS/WACN Discussion — RadioReference Forums](https://forums.radioreference.com/threads/rfss-wacn.353198/)** — Community discussion clarifying the WACN/System/RFSS/Site address hierarchy used throughout AABC-D messages.

- **[P25 Terminology Glossary Part 4 (S-Z) — Tait Communications](https://www.taitcommunications.com/en/about-us/news/2014/03/04/a-glossary-of-p25-terminology-part-4)** — Industry glossary; defines TSBK, TSBK-OSP/ISP, SU, FNE, and other acronyms used throughout AABC-D.

---

## Related TIA-102 Standards

The following standards define context required for full AABC-D implementation:

| Standard         | Title                                       | Relevance to AABC-D                                   |
|------------------|---------------------------------------------|-------------------------------------------------------|
| TIA-102.AABA     | Trunking Overview                           | System architecture; defines trunking call flows      |
| TIA-102.AABB     | Trunking Control Channel Formats            | Physical/link layer framing of TSBKs on the air       |
| TIA-102.AABD     | Trunking Subscriber Unit Requirements       | SU behavior when receiving/sending AABC-D messages    |
| TIA-102.AACD     | TDMA Trunking                               | IDEN_UP_TDMA, SYNC_BCST, slot-specific procedures     |
| TIA-102.AACE     | Authentication (OTAR)                       | AUTH_DMD, AUTH_FNE_RESP, AUTH_RESP message semantics  |
| TIA-102.BAAD     | Conventional Procedures                     | ACK/DENY/QUE procedure rules reused in trunking       |
| TIA-102.CABC     | Conventional Voice Interoperability         | Cross-mode interop context for voice channel grants   |
| TIA-102.BBAD     | Two-Slot TDMA Conventional Air Interface    | Channel Type field encoding in IDEN_UP_TDMA           |

---

## Implementation Notes for Developers

**TSBK Parser Entry Point**: Octet 0 of every received TSBK provides `IO` (bit implied by channel direction or explicit in multiblock header), `P` (bit 6), and `opcode` (bits 5:0). Dispatch table should be keyed on `(io, opcode)` tuple.

**Multiblock vs Single-Block Detection**: Single-block TSBKs have LB=`%1` and no preceding header. Multiblock sequences begin with a header block where Format field (`%10111` or `%10101`) and "Blocks to Follow" count determine reassembly. CRC must be verified on both the header and each data block independently, then a final Packet CRC on the last block covers the assembled payload.

**Abbreviated vs Extended Format**: Several messages (GRP_V_CH_GRANT, UU_V_CH_GRANT, AUTH_FNE_RESP, IDEN_UP_TDMA, RAD_MON_ENH_CMD) exist in both abbreviated (single-TSBK) and extended (multiblock) forms. The abbreviated form is used when both parties share the same Home system; the extended form carries full WACN+System+Unit addressing for cross-system calls. An implementation must select the correct form when transmitting and handle both when receiving.

**Channel Field Decoding**: A 16-bit channel field encodes `[Identifier(15:12) | Number(11:0)]`. The Identifier indexes into the locally maintained channel parameter table populated by IDEN_UP, IDEN_UP_TDMA, and IDEN_UP_VU messages. Frequency = Base_Frequency + (Transmit_Offset × Channel_Spacing × Number).

**Reason Code Handling**: DENY_RSP, QUE_RSP, and CAN_SRV_REQ all carry 8-bit reason codes from Annexes A, B, and C respectively. Implementations should surface these codes to upper layers for logging and user display; codes $80-$FF and ranges marked "user or system definable" require local configuration for interpretation.
