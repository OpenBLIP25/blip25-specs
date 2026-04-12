# TIA-102.AACD-A Related Resources and Context

## Document Status

| Field | Value |
|---|---|
| Full Title | Project 25 Digital Land Mobile Radio – Key Fill Device (KFD) Interface Protocol |
| Document ID | TIA-102.AACD-A |
| Published | September 9, 2014 |
| Supersedes | TIA-102.AACD (2003) |
| Standards Body | Telecommunications Industry Association |
| Subcommittee | TR8.3 (Encryption) |
| Status | Active |
| Classification | PROTOCOL + MESSAGE_FORMAT |

---

## P25 Standards Lineage (Key Fill / Key Management Family)

```
TIA-102 (Project 25) Standards Suite
│
├── Air Interface Layer
│   └── TIA-102.BAAA — Common Air Interface (CAI)
│
├── Key Management (OTAR) Family
│   ├── TIA-102.AACA   — OTAR Messages and Procedures (original)
│   ├── TIA-102.AACA-A — OTAR Messages and Procedures (rev A)
│   ├── TIA-102.AACA-B — OTAR Messages and Procedures (rev B)
│   ├── TIA-102.AACA-C — OTAR Messages and Procedures (rev C)
│   └── TIA-102.AACA-D — OTAR Messages and Procedures (rev D, 2025)
│
├── Key Fill Device (KFD) Family  ← THIS DOCUMENT
│   ├── TIA-102.AACD   — KFD Interface Protocol (original, 2003)
│   └── TIA-102.AACD-A — KFD Interface Protocol (rev A, 2014)  ★
│
├── Authentication Family
│   ├── TIA-102.AACE   — Link Layer Authentication Protocol
│   └── TIA-102.AACE-A — Link Layer Authentication Protocol (rev A)
│
├── Infrastructure Key Management
│   └── TIA-102.AAAB   — Key Management Facility (KMF) Interface Protocol
│
├── IP / Transport Layer
│   ├── TIA-102.BAJD   — IP Port Assignments (assigns UDP 49165 to KFD)
│   └── TIA-102.BAEG   — Mobile Data Peripheral Interface (MDPI)
│
└── Conventional / FDMA
    └── TIA-102.BAAA-A — FDMA Common Air Interface
```

---

## Relationships to Other TIA-102 Documents

### Normative Dependencies (AACD-A depends on these)

| Document | Relationship |
|---|---|
| TIA-102.AACA-A | KMM wire format origin; KFD delivers KMMs that OTAR uses over-the-air |
| TIA-102.AACE-A | Defines SUID structure and auth keys managed via KFD features |
| TIA-102.BAJD | Assigns UDP port 49165 to KFD Interface Protocol |
| TIA-102.BAEG (MDPI) | IP connectivity layer for USB-connected KFD sessions |

### Documents That Depend on AACD-A

| Document | Relationship |
|---|---|
| TIA-102.AACA-D | OTAR provisioning bootstrap requires KFD to load UKEK first |
| KMF interface specs | KMF instructs KFD to load keys; AACD-A is the last-mile protocol |

### Comparison: KFD (AACD-A) vs. OTAR (AACA-A)

| Aspect | KFD (AACD-A) | OTAR (AACA-A) |
|---|---|---|
| Transport | Physical cable (3WI or USB/IP) | Over-the-Air radio channel |
| Initiator | KFD device | Key Management Facility (KMF) |
| Pre-condition | None (bootstrapping mechanism) | UKEK already loaded in MR |
| Use case | Initial provisioning, recovery | Day-to-day re-keying |
| Message format | KMM over KFD Interface Protocol | KMM over ASTRO/P25 OTAR protocol |

---

## Open-Source Implementations

### KFDtool
- **Repository**: https://github.com/KFDtool/KFDtool
- **License**: MIT
- **Language**: C# (.NET)
- **Status**: Active development; widely used in the P25 amateur and public safety community
- **Supports**: TWI (3-Wire Interface) hardware adapter and UDP/IP DLI mode
- **Hardware**: Dedicated KFDtool adapter board; also supports legacy Motorola KVL-compatible cables
- **Notes**: The most complete open-source implementation of TIA-102.AACD-A. Implements the full exchange session state machine, all KMM types, and both transport paths. Commonly used with Motorola, Harris, and Kenwood P25 radios.

### kiwikey
- **Description**: Smaller open-source KFD implementation
- **Scope**: Focused on 3WI transport; less complete than KFDtool
- **Notes**: Useful as a reference for 3WI physical layer framing details

---

## Practical Context

### Where KFDs Appear in Real Systems

Public safety agencies use KFDs in several operational scenarios:

1. **Initial provisioning**: A new radio arrives with no keys. A KFD operator connects via the radio's accessory port and loads the initial TEK set and UKEK, enabling the radio to communicate on encrypted channels and receive subsequent OTAR updates.

2. **Key compromise response**: If a key is compromised, operators may need to immediately replace keys on radios in the field. KFDs allow this without waiting for OTAR coverage.

3. **OTAR-less agencies**: Smaller agencies without a KMF server use KFDs as their sole key management mechanism. They distribute keys annually or per-incident by physically visiting each radio with a KFD.

4. **KMF bootstrapping**: Even KMF-equipped agencies use KFDs to load the initial UKEK that enables OTAR to function — the classic bootstrapping problem.

### Common Hardware

- **Motorola KVL3000/4000**: Legacy proprietary KFDs; 3WI only
- **EF Johnson WB-100**: KFD hardware supporting both 3WI and IP modes
- **KFDtool adapter**: Open-source hardware design; works with KFDtool software
- **Harris SecNet KFD products**: Proprietary but AACD-A compliant for the interface

### Security Considerations

The KFD interface inherently requires physical access to the radio. The protocol itself has no authentication layer — it assumes physical security of the connection. This is by design: the threat model assumes an attacker cannot gain physical access to the radio's key fill port without detection. The 3WI interface does not encrypt keys in transit on the wire (keys move in plaintext between KFD and radio). The IP-based DLI mode has the same property — security depends on the physical layer, not the protocol.

---

## Phase 3 Implementation Spec Flags

Two implementation specs are needed for this document. They should be produced in a follow-up pass:

### Spec 1: KFD Message Parsing and Serialization
- Opcode dispatch table (all 33 KMM types from Table 1)
- Byte-offset field decoder for each KMM body (Tables 2–24)
- CRC-16-CCITT computation: polynomial, init value, byte-swap rule, test vector
- 3WI frame framer: start bit insertion, parity, 1 ms gap
- 3WI frame deframer: opcode recognition, length extraction, CRC verification
- DLI frame structure (KFD Opcode | Length | Control | Dest RSI | [Esync] | KMM | CRC)
- Key field serialization: DES (8 octets, parity), AES-128 (16 octets, no check)

### Spec 2: KFD Exchange Session State Machine
- IP session FSM: 5-state machine (IDLE → CONNECTED → KFD→MR → MR→KFD → TERMINATING)
- 3WI session FSM: 4-state machine
- KIT timer: default 15 s, min 5 s, max 30 s; reset rule; expiry behavior
- Destination RSI fallback: use $FFFFFF when MR RSI unknown; MR acceptance rule
- Keyset initialization: first-keyset auto-enable+activate; subsequent enabled-not-activated
- KMF-based vs. KFD-only feature sets (16 features, split 4 basic / 12 KMF-extended)
- SUID field composition: WACN (20b) || System ID (12b) || Subscriber ID (24b)
