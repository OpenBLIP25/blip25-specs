# TIA-102-BAAC-C Related Resources and Context

**Document:** ANSI/TIA-102.BAAC-C — Project 25 Common Air Interface Reserved Values  
**Approved:** August 2011 | **Supersedes:** TIA-102.BAAC-B

---

## Status

**Active as of approval date (August 2011).** This document supersedes TIA-102.BAAC-B
and is the C revision of the BAAC series. No subsequent revision (BAAC-D or later) has
been identified in publicly available sources as of the knowledge cutoff, meaning this
is likely the current active version. The TR-8.15 subcommittee retains the right to add
additional reserved values in future releases. The document is an ANSI-approved national
standard (ANSI/TIA-102.BAAC-C).

---

## Standards Family

This document is part of the **TIA-102 Project 25 FDMA standards suite**, specifically
within the **BAAC sub-series** (Common Air Interface Reserved Values). It is a
normative supplement to the core CAI document and must be used alongside it.

**Direct relationships:**

- **TIA-102.BAAA-A** (normative ref [1]) — Project 25 FDMA Common Air Interface,
  September 2003. The primary CAI specification that defines the fields for which this
  document provides reserved values. All field definitions, bit widths, and framing
  rules live in BAAA-A.

- **TIA-102.AABF-B** (informative ref [3]) — Link Control Word Formats and Messages,
  July 2009. Defines additional LCF values beyond the four defined in this document.
  Implementers who need the full LCF space must consult both documents.

- **TR-8.15 MFID Assignment Guide** (informative ref [2]) — Manufacturers Identifier
  Number Assignment Guide and Procedures, July 2008. Administered by the TR-8.15
  committee; assigns non-standard MFID values ($02 and above) to specific equipment
  manufacturers. Available from tiaonline.org.

- **TIA-102.BAAC-B** — The predecessor revision, superseded by this document.

- **TSB102-A** — APCO Project 25 System and Standards Definition. The umbrella
  document that defines which equipment must conform to the P25 standard family.

---

## Standards Lineage (ASCII Tree)

```
TSB102-A (APCO P25 System and Standards Definition)
└── TIA-102 Suite (P25 FDMA Standards)
    ├── TIA-102.BAAA-A — FDMA Common Air Interface [normative parent]
    │   └── TIA-102.BAAC-C — CAI Reserved Values [this document]
    │       ├── Predecessor: TIA-102.BAAC-B (superseded)
    │       │   └── Predecessor: TIA-102.BAAC-A (superseded)
    │       │       └── Predecessor: P25.930125.x (1993 drafts)
    │       └── Companion: TIA-102.AABF-B — Link Control Word Formats
    │           (extends LCF value space)
    └── TIA-102.AABx — Encryption/Auth specs
        (referenced by ALGID values $00–$04, $41, $81–$85)
```

---

## Practical Context

This document functions as the **master enumeration registry** for P25 FDMA air
interface constants. Every P25 radio, repeater, console, and network element that
processes CAI frames must implement the reserved values defined here. Practically:

**NAC ($293 default):** Most P25 systems ship from the factory with NAC $293. System
administrators program site-specific NAC values. The "open squelch" NACs ($F7E for
receivers, $F7F for repeaters) are widely used in scan/monitor receivers and
inter-system gateway equipment. Radios should never transmit these two values.

**LCF (group vs. individual):** LCF $00 and $03 are the most commonly encountered
values on any P25 conventional or trunked system. The encrypted variants $80 and $83
appear on systems using P25 Phase 1 encryption (FDMA with encrypted voice). Implementers
decoding P25 link control words should check the high bit of the LCF to determine
whether the LC payload is encrypted before attempting to parse TGID or address fields.

**MFID ($00 vs. $01):** The $00/$01 split is operationally important for opcode
dispatch. Many P25 message tables have two parallel opcode spaces indexed by MFID.
The April 18, 2001 cutoff date is codified here and propagated into all downstream
P25 message parsing specifications. SDRTrunk and OP25 both implement MFID-aware
opcode dispatch.

**Source/Dest ID range partitioning:** The boundary at $989680 (decimal 10,000,000)
separates individual radio IDs from the talk group ID / special purposes range. This
is enforced by network equipment when routing calls. The 16-bit TGID to 24-bit mapping
rule ($FEF001–$FFEFFF) is critical for interoperability between conventional and
trunked systems in the Message Update and Status Update message flows.

**ALGID ($80 = unencrypted):** In practice, $80 is by far the most common ALGID seen
on unencrypted P25 systems. The deprecated DES ($81) and 3DES ($83) ALGIDs still
appear on legacy federal and state systems that have not yet been re-keyed or
re-equipped. AES-256 ($84) is the current preferred algorithm for Type 1 (classified)
systems; AES-128 ($85) was added in Version 4.0 (September 2010) to support 128-bit
AES OTAR and expanded encryption deployments.

**SAP:** The SAP field is carried in packet data headers and controls routing to the
correct service handler at the receiving end. MR (Mobile Radio) service SAPs
($23–$27) are used in subscriber management. The Location Service SAP ($30 = decimal
48) was added in Version 3.0 (2008) to support P25 location services (AVL/GPS).
Trunking Control ($3D) and Protected Trunking Control ($3F) SAPs appear in trunked
data messaging.

**DUID:** The 4-bit DUID is the first field decoded by any P25 receiver after the
NID (Network ID) sync word. All six FDMA data unit types have non-sequential DUID
values; this is because the P-bit (parity bit in the NID) constrains the valid
DUID space. Receivers use the DUID to select the correct frame parser: HDU, LDU1,
LDU2, TDU, TDULC, PDU.

---

## Key Online Resources

- **TIA Standards Catalog:** https://www.tiaonline.org/standards/catalog/
  Purchase or access TIA-102 family documents. This document (BAAC-C) is listed as
  an active standard.

- **APCO Project 25 Technology Interest Group (PTIG):**
  https://www.apcointl.org/technology/p25/
  Background on P25 system standards and APCO's role in governance.

- **IHS Markit / Accuristech (distributor):**
  https://store.accuristech.com/tia
  Commercial distributor of TIA standards documents.

- **TR-8.15 MFID Assignment Guide:**
  Available from tiaonline.org, TR-8.15 committee page. Required companion for
  manufacturer-specific MFID values.

---

## Open-Source Implementations

These projects implement P25 FDMA CAI decoding and directly use the reserved values
defined in this document:

**OP25 (GNU Radio-based P25 decoder/transceiver)**
- https://github.com/boatbod/op25
- Implements NAC filtering, MFID dispatch, LCF parsing, ALGID identification,
  TGID extraction, SAP routing, and DUID-based frame dispatch.
- Key files: `op25/gr-op25-r2/lib/p25_frame_assembler_impl.cc` (DUID dispatch),
  `op25/gr-op25-r2/lib/p25p1_fdma.cc` (NAC/NID processing).

**SDRTrunk (Java-based P25 trunk tracker)**
- https://github.com/DSheirer/sdrtrunk
- Full P25 Phase 1 FDMA implementation. MFID-aware opcode dispatch, complete
  ALGID table, SAP-based service routing, full DUID handling.
- Key files: `src/main/java/io/github/dsheirer/module/decode/p25/`
  - `P25P1MessageFramer.java` — DUID-based frame dispatch
  - `identifier/` — NAC, TGID, Source/Dest ID handling

**DSD (Digital Speech Decoder)**
- https://github.com/szechyjs/dsd
- Earlier P25 decoder; implements basic DUID and LCF parsing. Less actively
  maintained than OP25/SDRTrunk.

**Osmocom P25 (osmop25 / osmo-p25)**
- https://osmocom.org/projects/osmo-p25
- Osmocom project for P25 infrastructure implementation. Implements CAI field
  decoding per BAAA-A and uses BAAC-C reserved values.

**p25rx / p25lib**
- Various community implementations on GitHub; search "p25 DUID" or "p25 NAC"
  for decoder fragments that reference BAAC-C value tables directly.

---

## Notes for Implementers

1. **MFID $00 vs $01 dispatch:** When parsing trunking or link control messages,
   always read MFID before dispatching on opcode. The opcode spaces for $00 and $01
   are logically separate.

2. **NAC transmit guard:** Implementations must never transmit $F7E or $F7F. These
   are valid only in receive-side configuration registers, not in transmitted NID
   fields.

3. **16-bit TGID → 24-bit mapping:** The mapping formula ($FEF001 + TGID - 1 for
   TGID $0001–$FFFF, capped at $FFEFFF) is required for conventional-to-trunked
   interoperability in Message Update and Status Update messages. The range
   $FEF001–$FFEFFF holds all 65535 conventional TGIDs.

4. **ALGID deprecation:** New implementations should reject DES ($81) and 3DES ($83)
   in security-sensitive contexts. AES-128 ($85) is the minimum recommended for new
   deployments.

5. **DUID P-bit:** The P column in the DUID table is the 64th bit of the NID code
   word (not separately transmitted; it is derived from the NID Golay codeword).
   This is provided as a reference for NID encoder/decoder validation.
