# P25 Implementation — Specs Reference

## Standards Index
The master index is `~/blip25-specs/specs.toml` — it tracks all 129 TIA-102 documents
with processing status, dependencies, and file paths.

## When to Read Specs
Before writing or debugging P25 protocol code, read the relevant implementation
spec. These are derived works with code-ready constants, opcode tables, parser
pseudocode, and Rust type definitions.

## Implementation Specs by Area

### FDMA (Phase 1) Air Interface
- **C4FM modulation, frame types, NID, FEC, IMBE placement:**
  ~/blip25-specs/standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md

### TDMA (Phase 2) Air Interface
- **H-CPM/H-DQPSK modulation, burst timing, sync sequences:**
  ~/blip25-specs/standards/TIA-102.BBAB/P25_TDMA_Physical_Layer_Implementation_Spec.md
- **MAC layer (ISCH, Reed-Solomon FEC, superframe, ultraframe, ESS):**
  ~/blip25-specs/standards/TIA-102.BBAC-A/P25_TDMA_MAC_Layer_Implementation_Spec.md
- **Scrambling (LFSR, seed derivation, per-burst application):**
  ~/blip25-specs/standards/TIA-102.BBAC-1/P25_TDMA_Scrambling_Implementation_Spec.md
- **Burst bit allocation tables (Annex E):**
  ~/blip25-specs/standards/TIA-102.BBAC-1/P25_TDMA_Annex_E_Burst_Bit_Tables.md

### MAC Messages (both FDMA and TDMA)
- **TDMA MAC messages — 100+ opcodes, PDU structure, CRC-12/CRC-16:**
  ~/blip25-specs/standards/TIA-102.BBAD-A/P25_TDMA_MAC_Message_Parsing_Implementation_Spec.md
- **TDMA MAC message parsing (BBAC-1 addendum version):**
  ~/blip25-specs/standards/TIA-102.BBAC-1/P25_TDMA_MAC_Message_Parsing_Spec.md
- **LoPTT and User Alias messages/procedures:**
  ~/blip25-specs/standards/TIA-102.BBAD-A-1/P25_TDMA_LoPTT_UserAlias_Implementation_Spec.md

### MAC Procedures / State Machines
- **TDMA MAC procedures (call setup, teardown, preemption, timing):**
  ~/blip25-specs/standards/TIA-102.BBAE/P25_TDMA_MAC_Procedures_Implementation_Spec.md

### Trunking Control Channel
- **Control channel formats (TSBK framing, microslot timing, MBT):**
  ~/blip25-specs/standards/TIA-102.AABB-B/P25_Trunking_Control_Channel_Formats_Implementation_Spec.md
- **Control channel messages — all OSP/ISP opcodes, TSBK dispatch:**
  ~/blip25-specs/standards/TIA-102.AABC-E/P25_Trunking_Control_Channel_Messages_Implementation_Spec.md
- **Trunking procedures (registration, affiliation, grants, state machines):**
  ~/blip25-specs/standards/TIA-102.AABD-B/P25_Trunking_Procedures_Implementation_Spec.md

### Conventional Mode
- **Conventional control messages (direct/repeat, no controller):**
  ~/blip25-specs/standards/TIA-102.AABG/P25_Conventional_Control_Messages_Implementation_Spec.md
- **Conventional procedures (call setup, teardown, SU/FNE state machines):**
  ~/blip25-specs/standards/TIA-102.BAAD-B/P25_Conventional_Procedures_Implementation_Spec.md

### Shared / Cross-Cutting
- **Link control words (72-bit LCW, LCF dispatch, voice channel metadata):**
  ~/blip25-specs/standards/TIA-102.AABF-D/P25_Link_Control_Word_Implementation_Spec.md
- **Reserved values (NAC, MFID, ALGID, SAP, Service Class lookup tables):**
  ~/blip25-specs/standards/TIA-102.BAAC-D/P25_Reserved_Values_Implementation_Spec.md
- **Dynamic regrouping (dynamic regroup commands, lockout, patching):**
  ~/blip25-specs/standards/TIA-102.AABH/P25_Dynamic_Regrouping_Implementation_Spec.md

### Vocoder
- **IMBE vocoder (frame format, bit ordering, FEC, tone generation):**
  ~/blip25-specs/standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md

### Encryption
- **Block encryption protocol (ALGID, KID, MI, ESS, OFB keystream, LFSR):**
  ~/blip25-specs/standards/TIA-102.AAAD-B/P25_Block_Encryption_Protocol_Implementation_Spec.md
- **Link layer authentication (challenge-response, key management):**
  ~/blip25-specs/standards/TIA-102.AACE-A/P25_Link_Layer_Authentication_Implementation_Spec.md

### Data Services
- **IP data bearer (SNDCP, segmentation/reassembly, confirmed/unconfirmed):**
  ~/blip25-specs/standards/TIA-102.BAEB-C/P25_IP_Data_Bearer_Implementation_Spec.md
- **Packet data LLC (logical link control for data channel):**
  ~/blip25-specs/standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md

## Analysis Notes
Cross-document analysis that synthesizes insights beyond what individual specs provide.
These correct common misconceptions and provide implementation guidance.

- **IMBE frame format vs MBE vocoder — decoupling the wire from the codec:**
  ~/blip25-specs/analysis/vocoder_wire_vs_codec.md
- **The missing vocoder specs — AMBE+2, BABB, and BABG:**
  ~/blip25-specs/analysis/vocoder_missing_specs.md

## How to Use These Specs
- Each spec has code-ready constants (hex arrays, bit masks, generator polynomials)
- Opcode tables are formatted as Rust const arrays or match/enum patterns
- Parser pseudocode shows the dispatch + extraction pattern
- Cross-references to SDRTrunk and OP25 source files are included
- When a spec flags "needs raster verification," the value was extracted from
  text and should be cross-checked against SDRTrunk/OP25 source code

## Key Cross-References
- FDMA TSBK opcodes map to TDMA MAC MCO values — see Annex A in the BBAD-A spec
- Link control words (AABF-D) carry the same info as TDMA VCU messages (BBAD-A)
- Control channel formats (AABB-B) define transport; messages (AABC-E) define content
- Trunking procedures (AABD-B) define when to send which AABC-E messages
