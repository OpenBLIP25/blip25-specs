# TIA 102.AACD-B: Related Resources and Context

## Document Status in specs.toml

```toml
[docs."TIA-102.AACD-B"]
title = "Key Fill Device Interface for Project 25"
status = "extracted"
classification = "protocol"
published = "2025-10-21"
supersedes = "TIA-102.AACD-A"
directory = "TIA-102.AACD-B/"
pdf = "8.3_TIA-102.AACD-B_ Final Published Document_2025_10_21.pdf"
phase3_needed = true
```

---

## Standards Family and Lineage

### Direct Predecessors
- **TIA-102.AACD-A** — Previous revision of the KFD Interface specification. Superseded by this document (AACD-B, October 2025). Key additions in -B: KMM Forwarding expansion, paired/unpaired auth key procedures, OTAR provisioning, View Active SUID, Annex C worked example.
- **TIA-102.AACD** — Original KFD Interface specification. Established the foundational 3-wire interface and Version 0 exchange. Superseded by -A then -B.

### Normative Dependencies (referenced by AACD-B)

| Reference | Document | Relationship |
|-----------|----------|-------------|
| [AACA] | TIA-102.AACA | OTAR Messages and Procedures — defines the KMM header format, KMM encryption format (inner-layer), and keying procedures for OTAR. AACD-B relies on [AACA] for the KMM encryption format used in the 3-wire KMM Frame body and for OTAR parameter definitions. |
| [AACE] | TIA-102.AACE | Authentication — defines authentication procedures; referenced for AF (Authentication Facility) interactions and authentication key formats. |
| [BAJD] | TIA-102.BAJD | Port Assignments — defines the TCP/UDP port numbers used by the KFD interface for Version 1 connections. |

### Closely Related Standards

| Document | Title | Relationship |
|----------|-------|-------------|
| TIA-102.AACA | P25 OTAR Messages and Procedures | Sibling: defines OTAR over-the-air key management; KFD is the physical-side complement. Shares KMM formats. |
| TIA-102.AACA-B | P25 OTAR (2025 revision) | Latest OTAR spec; KMM types defined there are referenced in AACD-B for inner-layer encryption format. |
| TIA-102.AACE | P25 Authentication | AF target procedures in AACD-B are defined with reference to AACE. |
| TIA-102.AAAB-A/B | P25 Encryption | Defines the encryption algorithms (AES-256, DES, etc.) whose key material is loaded via the KFD interface. |
| TIA-102.AAAD-A | P25 Digital Voice | Context: the TEKs loaded by the KFD protect the voice traffic defined here. |
| TIA-102.BAAA | P25 CAI (Common Air Interface) | KMM formats reference CAI secondary SAP field ($00 = consistency with CAI format). |
| TIA-102.BABA | P25 DFSI (Inter-RF Subsystem Interface) | Related: key fill over network paths may use DFSI transport in infrastructure. |
| TIA-102.CAAA | P25 Conformance Testing | Conformance tests for KFD interface would reference AACD-B procedures. |

### Document Lineage Tree

```
TIA-102.AACD          (original KFD Interface)
    └── TIA-102.AACD-A (revision A — added Version 1, TCP/UDP)
            └── TIA-102.AACD-B (revision B, Oct 2025 — expanded KMM Forwarding,
                                 auth key procedures, OTAR provisioning, Annex C)
```

---

## Practical Context

### What This Spec Enables

TIA-102.AACD-B is the operational spec for **keying P25 radios**. In any P25 Phase 1 or Phase 2 system using encryption, every radio must be keyed before it can communicate on encrypted channels. The KFD interface (whether physical 3-wire or TCP/UDP) is the defined standard mechanism for that provisioning.

Operational scenarios:
- A technician connects a KVL (Key Variable Loader) device to a P25 portable radio via the 3-wire connector on the radio's accessory port. The KVL implements Version 0 (or Version 1 over 3-wire) per AACD-B to load TEKs, RSI, KMF RSI, and MNP.
- A KMF (e.g., Motorola KMF, Harris SecNet, or open-source implementation) connects to SUs over IP (Version 1) to perform OTAR (defined in AACA). When a radio is not reachable over IP, the KMF uses KMM Forwarding (AACD-B Section 3.7.22) to route key material through a KFD that is physically present with the radio.
- Bulk keying: A dispatch center administrator uses a KFD software application to provision all radios for a new operational period (e.g., annual rekey), loading new TEKs, updated RSIs, and authentication keys.

### Security Notes for Implementers

- **WSTEK lifecycle is critical:** The WSTEK MUST be cryptographically random, generated fresh for each session, and destroyed (zeroized) immediately after Disconnect Ack. Reusing WSKTEKs breaks forward secrecy.
- **Inner-layer protection (KEK) is the real key-in-transit protection** — it protects TEKs regardless of whether Warm Start is used. Never transmit unencrypted TEKs over either interface version.
- **Message Numbers:** Anti-replay via MN is optional per the spec but SHOULD be implemented in any security-sensitive deployment. Without MNs, a captured session recording could be replayed.
- **3-wire physical security:** The 3-wire interface has no transport security in Version 0. Physical access control to the radio and KFD is the only protection. For sensitive deployments, Version 1 with Warm Start provides authenticated key establishment.
- **CRC is not a security check** — CRC-16-CCITT detects accidental bit errors only; it provides no integrity protection against deliberate modification. The MAC (when Warm Start is used) provides integrity.

### Radio/Equipment Compatibility

- The 3-wire interface (Annex B) is backward-compatible: older radios implementing the legacy protocol (pre-AACD-A) will operate in Version 0 mode.
- The KFD Interface Protocol (B.2.7) provides the compatibility handshake: Ready Req → Ready General Mode negotiates KFD Interface Protocol support; if the SU doesn't respond, the KFD falls back to the legacy protocol.
- KMM size limit (512 bytes) on 3-wire interface limits the number of keys that can be loaded per Modify-Key-Command; the KFD repeats the command for large keyloads.

---

## Open-Source and Public Implementations

### op25 (GNU Radio-based P25)
- Repository: https://github.com/boatbod/op25 and https://github.com/osmocom/op25
- Scope: Phase 1 and Phase 2 P25 SDR. Does not implement KFD interface directly, but implements OTAR key management message parsing (AACA) which shares KMM formats with AACD-B.
- Relevance: KMM message parsing code in op25 can be extended to handle KF Datagram formats.

### p25lib / p25craft
- Various community P25 parsing libraries on GitHub
- Search: `github.com search: p25 kfd` — several experimental KFD protocol implementations exist as hobby/research projects

### IMBE/AMBE Voice Codec Libraries
- Not directly related to AACD-B, but context: the TEKs loaded via the KFD are used to encrypt IMBE voice frames over the air.

### No Complete Open-Source KFD Implementation Known
As of 2026, there is no publicly known complete, spec-compliant open-source KFD implementation covering the full AACD-B feature set. This is an open opportunity for the P25 open-source community.

---

## Implementation Roadmap for This Project

### Phase 3 Specs Needed (flagged, not produced in this run)

1. **KFD_Protocol_State_Machine_Impl_Spec.md**
   - Version 1 session lifecycle (8-step state machine)
   - Warm Start WSTEK generation, distribution, and disposal
   - Session type negotiation (Key Fill, KMM Forwarding)
   - Timeout and error handling
   - Both initiator (KFD) and responder (SU/KMF/AF) perspectives

2. **KF_Datagram_Message_Format_Impl_Spec.md**
   - Complete wire format for all KMM types (Table 5)
   - KF Datagram header encoding
   - Inner-layer KEK encryption of key material
   - Session Control v0 and v1 message formats
   - Status code handling (Table 45 complete)

3. **Three_Wire_Interface_Impl_Spec.md**
   - Physical layer: electrical specs, Key Signature generation/detection
   - Byte format: TXBYTE/RXBYTE procedures
   - Flow control and line turnaround state machine
   - Timeout logic (tout1/tout2)
   - KFD Interface Protocol: opcode handling, KMM frame encode/decode
   - CRC-16-CCITT implementation with test vector

4. **KMM_Forwarding_Impl_Spec.md**
   - 3-procedure state machine
   - KF-Env-Cmd/Rsp/Rpt sequence number management
   - WSTEK management across 3 independent sessions
   - SU response correlation and delivery to KMF

### Suggested Rust Crate Structure

```
p25-kfd/
├── src/
│   ├── lib.rs
│   ├── datagram.rs      // KF Datagram encode/decode
│   ├── kmm/
│   │   ├── mod.rs       // KMM trait, opcode dispatch
│   │   ├── session.rs   // Session Control v0/v1
│   │   ├── keyload.rs   // Modify-Key-Command, Rekey-Ack
│   │   ├── inventory.rs // Inventory-Command/Response (8 subtypes)
│   │   ├── config.rs    // Load-Config, Change-RSI, Zeroize
│   │   ├── auth.rs      // Load/Delete/Transfer Auth Key
│   │   ├── envelope.rs  // KF-Env-Cmd/Rsp/Rpt
│   │   └── status.rs    // Status code enum (Table 45)
│   ├── session/
│   │   ├── mod.rs
│   │   ├── v1.rs        // Version 1 state machine
│   │   ├── warm_start.rs // WSTEK management
│   │   └── forwarding.rs // KMM Forwarding 3-procedure
│   ├── wire/
│   │   ├── mod.rs
│   │   ├── byte_proto.rs // 4kbps TXBYTE/RXBYTE
│   │   ├── kfd_proto.rs  // KFD Interface Protocol opcodes
│   │   └── crc.rs        // CRC-16-CCITT
│   └── crypto/
│       ├── mod.rs
│       ├── wstek.rs     // WSTEK generation/disposal
│       └── inner_layer.rs // KEK-based key material encryption
```

---

## Cross-Reference: Key Tables and Figures

| Reference | Content |
|-----------|---------|
| Table 4 | Supported Procedures per Target type (SU/KMF/AF) |
| Table 5 | Complete KMM opcode list |
| Table 6 | KF Datagram Header format |
| Tables 12-13 | DES and block cipher key formats |
| Table 14 | Modify-Key-Command (Extended Decryption Instruction Format) |
| Tables 15-16 | Load-Config-Command/Response |
| Tables 17,19 | Load-Authentication-Key-Command/Response |
| Tables 20-21 | Delete-Authentication-Key-Command/Response |
| Tables 22-24 | Inventory-Command subtypes (SUID items, Active SUID) |
| Tables 25-26 | Session Control KMM v0 and v1 |
| Tables 27-31 | KF-Env-Cmd/Rsp/Rpt and valid KMM lists |
| Table 32 | Transfer-Unpaired-Authentication-Key-Command |
| Tables 36-39 | Report-Paired/Delete-Auth-Key Command/Response |
| Tables 40-42 | Transfer-Key-Command (Transfer Format Field) / Response |
| Tables 43-44 | OTAR-Provision-Command/Response |
| Table 45 | Complete Status code table ($00-$FF) |
| Table 46 | CRC-16-CCITT state table (test vector $DEAD → $7C4B) |
| Table 47 | KFD Interface Protocol Opcode values |
| Table 48 | KMM Frame Control bit definitions |
| Figure 10 | Version 1 8-step exchange sequence diagram |
| Figure 37 | Transport protection model diagram |
| Figure 38 | MNinit XOR calculation |
| Figures 42-45 | 3-wire byte format, KMM frame format, exchange session |
| Figures 46-61 | Complete 3WI exchange sessions for all 16 manual rekeying features |
| Figures 62-67 | KMM Forwarding 3-procedure example (Annex C) |
