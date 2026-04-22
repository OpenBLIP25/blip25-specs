# J-STD-200 LMR ↔ 3GPP Mission Critical Interworking — Implementation Spec

**Source:** J-STD-200 (February 12, 2025), *Study of Interworking
between Land Mobile Radio (as defined by TIA-102 series and TIA-603
series) and 3rd Generation Partnership Project (3GPP) Mission Critical
Services*. **Joint ATIS/TIA standard** developed by the JLMRLTE
Subcommittee (ATIS WTSC) and TR-8.8 Subcommittee (TIA TR-8).

**Document type:** **Study document.** Architectural reference, not a
wire-format specification. Identifies which services can interwork
between LMR and 3GPP MC, under what architectural model, and what the
fundamental limitations are. **All wire-format message definitions,
IWF reference-point specs, and detailed procedures are deferred to
companion documents** — primarily **3GPP TS 23.283** and various
TIA-102 specs.

**How to use this impl spec.** J-STD-200 is where implementers of an
LMR ↔ 3GPP Interworking Function (IWF) go **first** to understand
scope, then **pivot to the companion specs** for wire-level detail.
This impl spec is a navigation hub, not an implementation contract —
no code is directly derivable from J-STD-200 content.

**Scope of this derived work:**
- §1 — What J-STD-200 does and doesn't cover
- §2 — Three LMR system types examined
- §3 — IWF architectural model and reference points
- §4 — Service interworkability catalog (voice, supplementary, data)
- §5 — Security architectures (E2EE vs IWF transencryption)
- §6 — Addressing translation (WACN/System/Unit ↔ URI)
- §7 — 80+ detailed message-sequence scenarios (informative)
- §8 — Companion specs for wire-level implementation
- §9 — Known limitations and out-of-scope items
- §10 — Cite-to section references
- §11 — Cross-references

**Pipeline artifacts:**
- `standards/J-STD-200/J-STD-200_Full_Text.md` — clean-room extraction.
- `standards/J-STD-200/J-STD-200_Summary.txt` — retrieval summary.
- `standards/J-STD-200/J-STD-200_Related_Resources.md`.

---

## 1. Document Scope

### 1.1 In scope

- **Service interworkability assessment** — for each LMR service,
  is interworking possible with a 3GPP MC equivalent?
- **Architectural model** — how the IWF is structured logically.
- **Reference points** — named interfaces between IWF, LMR, and 3GPP MC.
- **Interoperability limitations** — features that cannot interwork
  due to fundamental design differences.
- **~80 message sequence scenarios** (§9 of the study) covering
  group affiliation, call setup / continuation / termination,
  emergency alarms / cancellations, status updates, OTAR flows.

### 1.2 Explicitly out of scope

- **IWF wire-format message definitions** — deferred to 3GPP TS 23.283.
- **Reference-point (L102-T / L102-C / L603 / IWF-1/2/3) procedures**
  — deferred to companion specs.
- **TETRA-based LMR systems** — only TIA-102 (P25) and TIA-603
  (analog FM) are examined.
- **Non-standardized key management methods** — vendor-specific IWF
  behaviors.
- **Identity resolution and authorization inside the IWF** —
  deployment-specific.
- **3GPP MC home-policy determination** — 3GPP-side operator policy.

---

## 2. Three LMR System Types Examined

J-STD-200 examines interworking for three distinct LMR categories:

| LMR type | Characteristics | IWF reference point |
|----------|-----------------|---------------------|
| **TIA-102 Trunking** (P25) | Controller-based; centralized channel assignment; group affiliation; full mobility management; full supplementary services | **L102-T** |
| **TIA-102 Digital Conventional** | Infrastructure-less or repeater-based; no registration or affiliation; limited supplementary services | **L102-C** |
| **TIA-603 Analog Conventional FM** | Non-digital; largely non-standardized; very few interworkable services | **L603** |

**3GPP side:** Release 15 MC services:
- **MCPTT** (Mission Critical Push-to-Talk) — voice.
- **MCData** — data services including status.
- **MCVideo** — video (briefly discussed).

---

## 3. IWF Architectural Model

Per J-STD-200 §4–§5.

```
   ┌──────────────────────┐  L102-T / L102-C / L603   ┌─────────────┐   IWF-1/2/3   ┌──────────────┐
   │ LMR System           │◀──────────────────────────│     IWF     │──────────────▶│ 3GPP MC      │
   │ (TIA-102 trunking,   │                            └─────────────┘                │ System       │
   │  digital conv,       │                                                           │ (LTE/5G)     │
   │  TIA-603 analog)     │                                                           │              │
   └──────────────────────┘                                                           └──────────────┘
```

**The IWF appears:**
- To the **3GPP MC system** as a peer MCPTT / MCData / MCVideo system.
- To the **LMR system** as a peer LMR infrastructure entity (RFSS
  peer for trunking; conventional peer for conventional; FM
  peer for analog).

**The IWF does:**
- Protocol translation between the two domains.
- **Identity mapping** (LMR WACN/System/Unit ↔ 3GPP URI).
- **Pre-configured** group and user mappings (not dynamic).
- **Group affiliation and de-affiliation on behalf of one system**
  when communicating with the other.

**Home / serving split for interworked groups:**
- **Group homed in 3GPP MC** — 3GPP is the controlling home for that
  group's calls.
- **Group homed in LMR** — LMR is the controlling home.

---

## 4. Service Interworkability Catalog

### 4.1 Voice services

| LMR service | 3GPP MC equivalent | P25 Trunking | P25 Conv | TIA-603 |
|-------------|--------------------|--------------|----------|---------|
| Group Call (non-emergency) | MCPTT Group Call | **Yes** | **Yes** | limited |
| Group Call (emergency) | MCPTT Emergency Group Call | **Yes** | **Yes** | limited |
| Broadcast Group Call | MCPTT Broadcast Group Call | **Yes** | **Yes** | — |
| Individual Call (Private Call) | MCPTT Private Call | **Yes** | **Yes** | — |
| Announcement Group Call | — | **LMR-unique, no IWK** | N/A | N/A |
| System Group Call | — | **Not currently supported** | N/A | N/A |

### 4.2 Supplementary services

| LMR service | Interworkable? | Notes |
|-------------|----------------|-------|
| Emergency Alarm / Alert | **Yes** | Mapped both directions |
| Emergency Alarm Cancel | **Yes** | |
| Group Emergency Cancel | **Yes** | (per BACA-B-1 + BACA-B-3) |
| Call Alert | **Pending standardization** | 3GPP has private-call call-back as potential equivalent |
| Status Update | **Yes** (trunking + conv ↔ MCData) | IWF translates code values |
| Short Message | **No** | No 3GPP equivalent |
| Status Query | **No** | LMR-specific |
| Radio Unit Monitor | **No** | LMR-specific |
| Radio Check | **No** | LMR-specific |
| Radio Detach | **No** | LMR-specific |
| Radio Inhibit / Uninhibit | **No** | LMR-specific |

### 4.3 Call control

| Feature | Interworkable? |
|---------|----------------|
| Priority interworking | **Yes** (IWF translates priority values) |
| Message trunking | **Yes** — default mode for interworked calls |
| Transmission trunking | **No** |
| Individual regrouping | **No** (fundamental LMR vs 3GPP preconfigured-group difference) |
| Group regrouping | **Ongoing standardization** |

### 4.4 Location services

Interworking is **ongoing work in 3GPP as of Release 17**.

- TIA-102 supports **Tier 1** (CAI data bearer, per BAJB-B) and
  **Tier 2** (UDP/IP, per BAJC-A).
- 3GPP MC tracks location via native GPS or network-derived methods.

Cross-mapping is not yet standardized.

---

## 5. Security Architectures

Per J-STD-200 §8. Two interworking encryption architectures:

### 5.1 End-to-End Encryption (E2EE)

- The IWF **only handles addressing** — media encryption passes
  through intact.
- Most secure; the IWF doesn't hold traffic keys.
- **Requires**: LMR and 3GPP MC systems use **compatible codecs,
  algorithms, algorithm modes, and keys**.
- In practice: both sides must use the same end-to-end key material
  and the same vocoder (IMBE or AMBE+2 or AMR). Transcoding breaks
  E2EE.

### 5.2 IWF Transencryption

- The IWF **decrypts at one boundary and re-encrypts at the other**.
- Less secure — requires all keys at the IWF.
- Allows different codecs / algorithms on each side (IWF handles
  the transition).
- Acceptable when E2EE compatibility can't be established, subject to
  agency policy.

### 5.3 Key distribution to 3GPP MC clients

Three standardized methods:

1. **P25 OTAR Proxy (MC OTAR Proxy / MOP)** — P25 KMF distributes
   OTAR keys to 3GPP MC clients via an MCPTT MOP.
2. **Key Fill Device (KFD)** — per TIA-102.AACD-A/B, used to load
   keys into 3GPP MC devices directly (requires device support).
3. **Manufacturer-specific methods** — out of scope of J-STD-200.

---

## 6. Addressing Translation

The IWF maps between disparate identity spaces:

| LMR type | Identity structure |
|----------|---------------------|
| **TIA-102 Trunking** | **56-bit SUID** = 20-bit WACN ID + 12-bit System ID + 24-bit Unit ID. **48-bit SGID** = WACN + System + 16-bit Group ID |
| **TIA-102 Digital Conventional** | 24-bit UID portion of the trunking SUID |
| **TIA-603 Analog** | **No standardized addressing** — vendor-specific |
| **3GPP MC** | **URIs** (Uniform Resource Identifiers) for both user and group identifiers |

The IWF holds a pre-configured mapping table. Dynamic
identity-assignment across the boundary is not supported.

---

## 7. 80+ Message Sequence Scenarios (Informative)

J-STD-200 §9 provides **approximately 80 detailed message sequence
diagrams** organized by LMR system type, service, and initiation
direction. Topics covered:

- Group affiliation / de-affiliation (both group-homed-in-LMR and
  group-homed-in-3GPP).
- Supplementary services registration.
- Group call setup / continuation / termination — with control-channel
  and direct methods.
- Individual call setup.
- Emergency alarm / alert + cancellation.
- Group emergency cancel.
- Status update flows.
- Audio takeover (console priority) by LMR CSU.
- OTAR key management flows initiated by both LMR KMF and 3GPP MC.

**Implementation consequence.** §9 is **informative** (not normative)
but is the practical starting point for IWF design. A vendor building
an IWF should treat §9 as a collection of canonical sequences to pass
through their own protocol stack, then deviate only where operational
requirements demand.

---

## 8. Companion Specs for Wire-Level Implementation

J-STD-200 defers wire-level detail to these companion documents:

| Topic | Normative spec |
|-------|----------------|
| Trunking procedures (LMR side) | TIA-102.AABD-B (+ AABD-C draft) |
| Conventional procedures (LMR side) | TIA-102.BAAD-B |
| OTAR messages | TIA-102.AACA-A / AACA-D |
| KFD interface | TIA-102.AACD-A / AACD-B |
| Inter-KMF interface | TIA-102.BAKA-A (+ BAKA-A-1 TLS 1.3 addendum) |
| Link Layer Authentication | TIA-102.AACE-A |
| ISSI (IWF-as-RFSS) | **TIA-102.BACA-B-3** (IWF addendum, Annex J) |
| 3GPP-side IWF reference | **3GPP TS 23.283** |

Implementing an IWF requires fluency in **both** the TIA-102
and 3GPP sides. J-STD-200 is the architectural glue; TS 23.283
is the normative glue on the 3GPP side; the TIA-102 side is the
collection of LMR specs above.

---

## 9. Known Limitations and Out-of-Scope

Per J-STD-200 §2 and throughout:

- No IWF wire-format or reference-point (L102-T / L102-C / L603 /
  IWF-x) detailed procedures are specified.
- Non-standardized key management methods not addressed.
- Identity resolution and authorization inside the IWF not specified.
- 3GPP home-policy determination not specified.
- **TETRA** LMR systems out of scope.
- **Announcement Group Call** (LMR trunking only) cannot interwork.
- **Individual regrouping** cannot interwork.
- **Transmission trunking** cannot interwork.
- Most LMR supplementary services (Short Message, Status Query,
  Radio Check, etc.) have no 3GPP equivalent.

---

## 10. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- Scope — J-STD-200 §1.
- System types (TIA-102 trunking / digital conv / TIA-603 analog) — §2.
- IWF architectural model — §4.
- Reference points (L102-T / L102-C / L603 / IWF-1/2/3) — §5.
- Voice services — §6.
- Supplementary services — §7.
- Security — §8 (E2EE and IWF transencryption; MOP; KFD; manufacturer).
- **Message sequence scenarios (~80)** — §9 (informative).
- Addressing — §5 / Annex.

---

## 11. Cross-References

**Companion / downstream impl specs in this repo:**
- `standards/TIA-102.BACA-B-3/P25_ISSI_IWF_Interworking_Implementation_Spec.md`
  — the LMR-side IWF behavior on the ISSI (most directly relevant).
- `standards/TIA-102.AABD-B/P25_Trunking_Procedures_Implementation_Spec.md`
  + `standards/TIA-102.AABD-C/P25_Trunking_Procedures_AABD_C_Draft_Implementation_Spec.md`
  — trunking procedures and their AABD-C draft evolution.
- `standards/TIA-102.AACA-D/…` — OTAR protocol.
- `standards/TIA-102.AACD-A/P25_KFD_Interface_Protocol_Implementation_Spec.md`
  + `standards/TIA-102.AACD-B/P25_KFD_Interface_Protocol_AACD_B_Delta_Implementation_Spec.md`
  — KFD key distribution (including for 3GPP MC devices per §5.3).
- `standards/TIA-102.BAKA-A-1/P25_KMF_to_KMF_Addendum_1_TLS_1_3_Implementation_Spec.md`
  — inter-KMF with TLS 1.3.
- `standards/TIA-102.AAAB-B/P25_Security_Services_Overview_Implementation_Spec.md`
  — P25 security taxonomy context for §5 discussions.
- `standards/TIA-102.BAJB-B/P25_Location_Services_Implementation_Spec.md`
  — Tier 1 / Tier 2 location context for the §4.4 deferral.

**External normative references:**
- **3GPP TS 23.283** — LMR interworking (3GPP-side specification,
  the companion to J-STD-200 on the 3GPP side).
- 3GPP **TS 22.179** / **TS 23.179** / **TS 24.379** — MCPTT core specs.
- **NPSTC LMR/LTE Interoperability Final Report** (January 2018) —
  historical input; this document formalizes work initially captured
  there.

**Related analysis opportunities:**
- IWF-specific implementation traps (vocoder transcoding edge cases,
  key material E2EE compatibility constraints, group home-policy
  ambiguities) would be candidate `analysis/` notes if implementer
  work uncovers them.
