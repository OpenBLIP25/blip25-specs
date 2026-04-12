# TIA-102.AABC-E: Related Resources, Document Lineage, and Implementation References

## Document Identification

| Field | Value |
|-------|-------|
| Standard Number | ANSI/TIA-102.AABC-E |
| Full Title | Project 25 — Trunking Control Channel Messages |
| Revision | E |
| ANSI Approval | April 16, 2019 |
| Publication Date | April 23, 2019 |
| Supersedes | TIA-102.AABC-D |
| Status | Active (as of 2026-04-12) |
| Developing Organization | Telecommunications Industry Association (TIA) |
| TIA Committee | TR-8.25 (Public Safety) |

---

## Document Family and Lineage

### Predecessor Revisions

| Revision | Notes |
|----------|-------|
| TIA-102.AABC (original) | First edition |
| TIA-102.AABC-A | Revision A |
| TIA-102.AABC-B | Revision B |
| TIA-102.AABC-C | Revision C |
| TIA-102.AABC-D | Immediate predecessor to current revision |
| **TIA-102.AABC-E** | **Current (2019)** |

### Key Changes in Revision E (relative to -D)

- Added TDMA-specific identifier update (IDEN_UP_TDMA) and Synchronization Broadcast (SYNC_BCST) for TDMA micro-slot synchronization
- Added Radio Unit Monitor Enhanced (RAD_MON_ENH_REQ / RAD_MON_ENH_CMD) supporting encrypted and talkgroup monitor modes
- Deprecated legacy authentication messages (AUTH_Q, AUTH_RSP, AUTH_CMD); replaced by AUTH_DMD / AUTH_RESP / AUTH_RESP_M / AUTH_FNE_RST / AUTH_FNE_RESP / AUTH_SU_DMD
- Deprecated legacy data messages (IND_DATA_REQ, GRP_DATA_REQ, IND_DATA_CH_GRANT, GRP_DATA_CH_GRANT, GRP_DATA_CH_ANN, GRP_DATA_CH_ANN_EXP)
- Deprecated P_PARM_REQ (protection parameter request)
- Added ADJ_STS_BCST_UNC (Uncoordinated Adjacent Status Broadcast)
- Added AUTH_SU_DMD (SU-initiated authentication demand)
- Extended Roaming Address Response formats to support up to 8 roaming stack entries in a single transaction (Formats 1-4)

---

## Related TIA-102 Standards

These standards are directly referenced by or tightly coupled to TIA-102.AABC-E:

| Standard | Title | Relationship |
|----------|-------|-------------|
| TIA-102.AABB | Project 25 System and Standards Description | Overall P25 system architecture |
| TIA-102.AABF | Common Air Interface (CAI) Standards | Physical layer and channel structure for TSBK transmission |
| TIA-102.AABD | Reserved Channel Messages | TSBK reserved opcode space for proprietary use |
| TIA-102.AACE | Authentication, Encryption, and Identification Procedures | Defines RES1, RES2, RAND1, RAND2, ALGID — referenced by AUTH_RESP, AUTH_RESP_M, AUTH_FNE_RST, AUTH_FNE_RESP, AUTH_DMD |
| TIA-102.AABG | IMBE Vocoder | Voice channel codec referenced by voice grant messages |
| TIA-102.AABH | Enhanced Multi-Band Excitation (IMBE) Vocoder | Enhanced vocoder variant |
| TIA-102.AABD | FDMA Reserved Channel Signaling | Companion to TSBK control channel messages |
| TIA-102.BABA | ISSI (Inter-RF Subsystem Interface) | Inter-site routing relates to RFSS_STS_BCST, ADJ_STS_BCST |
| TIA-102.BAHA | CSSI (Console Subsystem Interface) | Console integration uses GRP_V_CH_GRANT and status messages |

---

## Standards Purchase and Access

- TIA Store: https://standards.globalspec.com/standards/tia (search "TIA-102.AABC-E")
- Accuris Tech (authorized distributor): https://store.accuristech.com/tia
- ANSI Webstore: https://webstore.ansi.org
- Contact TIA directly: standards@tiaonline.org

Note: This document is a "Limited Use Copy" issued to US Government entities.
It is not to be shared or posted to a web server accessible to others.

---

## Open-Source P25 Implementations

The following open-source projects implement P25 TSBK/MBT decoding and encoding.
These are the primary reference implementations for the messages defined in
TIA-102.AABC-E:

### Decoders / Analyzers

| Project | Language | URL | Relevance |
|---------|----------|-----|-----------|
| **op25** | C++/Python | https://github.com/boatbod/op25 | Full P25 Phase 1 and Phase 2 receiver; decodes TSBK and MBT messages; most comprehensive open-source decoder |
| **p25rx** | Python | https://github.com/alaindargelas/p25rx | Older P25 decoder; TSBK decoding |
| **dsd (Digital Speech Decoder)** | C | https://github.com/szechyjs/dsd | Early P25 voice decoder; partial TSBK support |
| **trunk-recorder** | C++ | https://github.com/robotastic/trunk-recorder | Active P25 trunking recorder; full TSBK decode for voice grants, registration, and system broadcasts |
| **SDRTrunk** | Java | https://github.com/DSheirer/sdrtrunk | Comprehensive P25 Phase 1/Phase 2 decoder; extensive TSBK and MBT decoding including authentication messages |
| **gr-p25** | C++/Python (GNU Radio) | https://github.com/osmocom/gr-p25 (Osmocom fork) | GNU Radio P25 blocks |

### Protocol Libraries / Implementations

| Project | Language | URL | Relevance |
|---------|----------|-----|-----------|
| **p25lib** | C | Research/academic | Low-level P25 bit manipulation |
| **osmo-p25** | C | https://osmocom.org | Osmocom Project 25 work |
| **dvmhost / dvmfne** | C++ | https://github.com/DVMProject/dvmhost | P25 FNE/host implementation; implements TSBK encode/decode for trunking controller |
| **p25gateway** | C++ | https://github.com/DVMProject/p25gateway | P25 gateway with TSBK support |

### FNE (Controller) Implementations

| Project | Language | URL | Notes |
|---------|----------|-----|-------|
| **dvmhost** | C++ | https://github.com/DVMProject/dvmhost | Most complete open-source P25 FNE; implements registration, affiliation, voice grants, authentication |
| **HBlink3** (P25 mode) | Python | https://github.com/HBlink3/HBlink3 | Primarily DMR/P25 hybrid |

---

## Key Implementation Notes for Developers

### TSBK Opcode Dispatch
A TSBK decoder should:
1. Read bit 5 of Octet 0 for IO direction (MBT) or determine direction from channel context
2. Extract Opcode from bits 5-0 of Octet 0
3. Check MFID (Octet 1); if non-zero, treat as vendor-specific and handle per TIA-102.AABD
4. Dispatch on (direction, opcode) pair to the appropriate message parser
5. Validate TSBK CRC-16 (Octets 10-11) before processing payload

### MBT Header/Block Assembly
1. Collect Header Block and all Data Blocks (count from Blocks-to-Follow field)
2. For ISP: IO=%0; for OSP: IO=%1
3. Opcode is in Header Block Octet 7 bits 5-0
4. Validate Header CRC (Octets 10-11 of Header Block) and Packet CRC (last data block)

### Abbreviated vs Extended Selection (decoder)
- If IO=%0 and Format=%10111 → ISP MBT (extended) format
- If Opcode in range with standard TSBK layout and block is standalone → abbreviated
- Some opcodes (e.g., RAD_MON_ENH_REQ, RAD_MON_ENH_CMD) use MBT exclusively even for "abbreviated" mode

### Authentication Message Handling
AUTH_RESP, AUTH_RESP_M, AUTH_FNE_RST, AUTH_FNE_RESP, AUTH_DMD, AUTH_SU_DMD all
reference cryptographic values (RES1, RES2, RAND1, RAND2, ALGID) defined in
TIA-102.AACE. A decoder can parse the message structure from AABC-E; actual
cryptographic processing requires AACE.

### Obsolete Opcode Handling
Opcodes %010000-%010011 (ISP) and %010000-%010011 (OSP) are obsolete data
channel messages. A compliant modern decoder should log receipt of these but
not attempt to fulfill the requests. The obsolete AUTH_Q/AUTH_RSP and AUTH_CMD
were removed in earlier revisions.

---

## Phase 3 Implementation Specs Needed

This document's classification as `message_format, protocol` warrants the
following Phase 3 implementation specs (NOT included in this Phase 2 run):

1. **TSBK Message Parser Spec** — Rust struct definitions and decode logic for
   all TSBK and MBT message types. Field extraction with bit masks, CRC validation,
   abbreviated/extended format dispatch.

2. **Trunking State Machine Spec** — Transaction procedures: registration flow
   (U_REG_REQ → U_REG_RSP), affiliation flow (GRP_AFF_REQ → GRP_AFF_RSP),
   voice grant sequence (GRP_V_REQ → GRP_V_CH_GRANT → channel hold → release),
   deny/queue handling, authentication handshake state machine.

---

## Web and Community Resources

| Resource | URL | Notes |
|----------|-----|-------|
| P25 Wikipedia | https://en.wikipedia.org/wiki/Project_25 | Overview and standards list |
| RadioReference P25 Wiki | https://wiki.radioreference.com/index.php/APCO_Project_25 | Trunking system database and P25 decoder info |
| Osmocom P25 | https://osmocom.org/projects/op25/wiki | op25 documentation |
| DVMProject Docs | https://github.com/DVMProject/dvmhost/wiki | dvmhost implementation documentation |
| P25 Standards Portal | https://www.tiaonline.org/standards/p25 | TIA P25 standards index |
