# P25 Documentation Suite Overview — TSB-102-D Implementation Spec

**Source:** TSB-102-D (March 2020), *Project 25 — TIA-102 Documentation
Suite Overview*. Fourth major revision of the original TSB-102.
Informative Telecommunications Systems Bulletin.

**Document type:** top-level navigation hub for the entire TIA-102
standards suite. Does not define any protocol, message format, or
algorithm. Provides: (1) what services P25 systems support, (2) how
those services map to functional system elements and open interfaces,
(3) which specific TIA-102 document defines each subject.

This derivation is the "front door" to this repo's navigation-hub
family. Where AABA-B (Trunking Overview), BAEA-C (Data Overview),
AAAB-B (Security Overview), and TSB-102.BBAA (TDMA Overview) are
area-specific hubs, **TSB-102-D is the hub of hubs**.

**Scope of this derived work:**
- §1 — The P25 services taxonomy (voice, data, supplementary)
- §2 — Functional system elements (SU, RFSS, KMF, CS, MDP, …)
- §3 — Open interfaces: the eight-interface P25 reference model
- §4 — Document suite structure: AA / BA / CA / CB / CC series
- §5 — Document-to-area quick-reference table
- §6 — Revision lineage of TSB-102 (A → B → C → D)
- §7 — Cite-to section references
- §8 — Cross-references (to every navigation-hub impl spec in this repo)

**Pipeline artifacts:**
- `standards/TSB-102-D/TSB-102-D_Full_Text.md` — clean-room extraction.
- `standards/TSB-102-D/TSB-102-D_Summary.txt` — retrieval summary.
- `standards/TSB-102-D/TSB-102-D_Related_Resources.md`.

---

## 1. P25 Services Taxonomy

Per TSB-102-D §2.

P25 supports three service classes:

```
┌───────────────────────────────────────────────────────────────┐
│ 1. TELESERVICES — end-user visible                            │
│    • Group Voice Call (talkgroup)                              │
│    • Individual Voice Call (private call)                      │
├───────────────────────────────────────────────────────────────┤
│ 2. BEARER SERVICES — data transport                           │
│    • CAI Data Bearer (opaque payload)                          │
│    • IP Data Bearer (IPv4 datagrams)                           │
├───────────────────────────────────────────────────────────────┤
│ 3. SUPPLEMENTARY SERVICES — attached to the above              │
│    • Voice Telephone Interconnect                              │
│    • Emergency Alarm / Emergency Call                          │
│    • Short Data Message                                        │
│    • Status Update / Status Query                              │
│    • Radio Inhibit / Uninhibit / Check / Detach                │
│    • Radio Unit Monitor                                        │
│    • Pre-programmed Data Messaging                             │
└───────────────────────────────────────────────────────────────┘
```

This three-class model recurs throughout the TIA-102 suite (AABA-B §3,
BAEA-C §2, etc.).

---

## 2. Functional System Elements

Per TSB-102-D §3. The P25 Functional Network Model defines these
entity types:

| Entity | Role |
|--------|------|
| **SU** (Subscriber Unit) | The radio — portable or mobile |
| **RFSS** (RF Subsystem) | The trunking authority; owns channels, controllers, RF |
| **Site** | One coverage cell within an RFSS |
| **FNE** (Fixed Network Equipment) | Infrastructure that isn't an SU (RFSS controllers, gateways, consoles) |
| **FSR** (Fixed Station Repeater) | Pure PHY/MAC relay; no LLC or IP awareness |
| **CS** (Console Subsystem) | Dispatch console host |
| **MDP** (Mobile Data Peripheral) | Laptop / terminal tethered to an SU |
| **DH** (Data Host) | IP-addressed server on the Data Host Network |
| **DHN** (Data Host Network) | Agency IP network behind the FNE |
| **KMF** (Key Management Facility) | OTAR key provisioning authority |
| **KFD** (Key Fill Device) | Hand-held device for wireline key fill |
| **LIS** (Location Information System) | GPS / LORAN source for location services |
| **LSHS** (Location Service Host System) | Mapping / dispatch UI consuming locations |
| **OMC-RF** (Operations Maintenance Center) | RFSS-internal management function |
| **NMC** (Network Management Center) | External management host |

---

## 3. Open Interfaces (Reference Model)

Per TSB-102-D §3. The interfaces that are **standardized** by TIA-102:

| Interface | Connects | Primary normative spec |
|-----------|----------|------------------------|
| **Um** | SU ↔ RFSS (or FSR, or peer SU) — Phase 1 FDMA | TIA-102.BAAA-B |
| **Um2** | SU ↔ RFSS — Phase 2 TDMA | TIA-102.BBAB + BBAC-A + BBAD-A + BBAE |
| **Ed** | FNE ↔ Data Host Network | TIA-102.BAEF |
| **A** | SU ↔ MDP (tethered) | TIA-102.BAEG-A |
| **Et** | RFSS ↔ PSTN / PABX | TIA-102.BADA-A + BADA-1 |
| **Ec** | CS ↔ CAR (Conventional ISSI) | TIA-102.BACE |
| **En** | OMC-RF ↔ NMC | TSB-102.BAFA-A |
| **Ef** | Fixed Station Subsystem Interface | TIA-102.BAHA-A |
| **G (ISSI)** | RFSS ↔ RFSS | TIA-102.BACA-B + BACE + BACF |
| **IKI** | KMF ↔ KMF | TIA-102.BAKA-A + BAKA-A-1 |
| **KFD-MR** | KFD ↔ SU | TIA-102.AACD-A + AACD-B |

Each of these interfaces has a corresponding **impl spec** in this
repo — see §8 for the links.

---

## 4. Document Suite Structure

Per TSB-102-D §4.

```
TIA-102 series organized by document-ID prefix:

┌──────────────────────────────────────────────────────────────────┐
│ AA — Trunking / Security / OTAR                                  │
│   • AABA — Trunking Overview                                     │
│   • AABB — Trunking Control Channel Formats                      │
│   • AABC — Trunking Control Channel Messages (opcodes)           │
│   • AABD — Trunking Procedures                                   │
│   • AABF — Link Control Words                                    │
│   • AABG — Conventional Control Messages                         │
│   • AACA — OTAR Messages and Procedures                          │
│   • AACD — KFD Interface Protocol                                │
│   • AACE — Link Layer Authentication                             │
│   • AAAD — Block Encryption Protocol                             │
│   • AAAB — Security Services Overview                            │
├──────────────────────────────────────────────────────────────────┤
│ BA — Voice, Vocoder, Data                                        │
│   • BAAA — FDMA Common Air Interface                             │
│   • BAAC — CAI Reserved Values                                   │
│   • BAAD — Conventional Procedures                               │
│   • BABA — IMBE Vocoder (+ BABA-1 Half-Rate)                     │
│   • BACA — ISSI Voice and Mobility                               │
│   • BACE — ISSI Conventional                                     │
│   • BACF — ISSI Packet Data                                      │
│   • BADA — Telephone Interconnect                                │
│   • BAEA–BAEJ — Data Services suite                              │
│   • BAFA — Network Management Interface                          │
│   • BAHA — Fixed Station Interface                               │
│   • BAJB / BAJC — Location Services Tier 1 / Tier 2              │
│   • BAJD — TCP/UDP Port Assignments                              │
│   • BAKA — KMF-to-KMF Interface                                  │
├──────────────────────────────────────────────────────────────────┤
│ BB — Phase 2 Two-Slot TDMA (standalone TDMA subtree)             │
│   • BBAA — TDMA Overview                                         │
│   • BBAB — TDMA PHY                                              │
│   • BBAC — TDMA MAC (+ BBAC-1 Scrambling)                        │
│   • BBAD — TDMA MAC Messages (+ BBAD-A-1 addendum)               │
│   • BBAE — TDMA MAC Procedures                                   │
├──────────────────────────────────────────────────────────────────┤
│ CA — Interoperability tests                                      │
│   • CAAA — Transceiver measurement methods                       │
│   • CABA — Conventional Voice Interoperability Tests             │
├──────────────────────────────────────────────────────────────────┤
│ CB — Conformance tests                                           │
│   • BCAD / BCAE / BCAF — Conformance profiles                    │
├──────────────────────────────────────────────────────────────────┤
│ CC — TDMA transceiver tests                                      │
│   • CCAA — TDMA Transceiver measurements                         │
│   • CCAB — TDMA Transceiver performance                          │
└──────────────────────────────────────────────────────────────────┘
```

**Four-letter codes** identify specific documents (e.g., AABA,
BAEB-C). Addenda append `-N` (e.g., BADA-1, AABD-B-3).

---

## 5. Document-to-Area Quick Reference

TSB-102-D provides tables mapping every functional area to the
authoritative normative spec. The table below summarizes the most
commonly-needed cross-references for an implementer:

| Area | Normative spec | Impl spec |
|------|----------------|-----------|
| FDMA PHY / MAC | BAAA-B | `standards/TIA-102.BAAA-B/…` |
| TDMA PHY | BBAB | `standards/TIA-102.BBAB/…` |
| TDMA MAC | BBAC-A + BBAC-1 | `standards/TIA-102.BBAC-A/…` + `.BBAC-1/…` |
| Trunking control-channel formats | AABB-B | `standards/TIA-102.AABB-B/…` |
| Trunking control-channel messages | AABC-E | `standards/TIA-102.AABC-E/…` |
| Trunking procedures | AABD-B (+ AABD-C draft) | `standards/TIA-102.AABD-B/…` + `.AABD-C/…` |
| Link Control Words | AABF-D | `standards/TIA-102.AABF-D/…` |
| Reserved values (NAC/MFID/ALGID/SAP) | BAAC-D | `standards/TIA-102.BAAC-D/…` |
| Block Encryption Protocol | AAAD-B | `standards/TIA-102.AAAD-B/…` |
| OTAR | AACA-D | `standards/TIA-102.AACA-D/…` |
| Link Layer Authentication | AACE-A | `standards/TIA-102.AACE-A/…` |
| KFD Interface | AACD-A + AACD-B delta | `standards/TIA-102.AACD-A/…` + `.AACD-B/…` |
| KMF-to-KMF (inter-agency) | BAKA-A + BAKA-A-1 TLS 1.3 | `standards/TIA-102.BAKA-A-1/…` |
| Vocoder (IMBE full rate) | BABA-A | `standards/TIA-102.BABA-A/…` |
| Data suite (overview) | BAEA-C | `standards/TIA-102.BAEA-C/…` |
| IP Data Bearer | BAEB-B / BAEB-C | `standards/TIA-102.BAEB-B/…` + `.BAEB-C/…` |
| Packet Data LLC | BAED-A | `standards/TIA-102.BAED-A/…` |
| Telephone Interconnect | BADA-A + BADA-1 | `standards/TIA-102.BADA-A/…` |
| Location Services | BAJB-B (Tier 1) / BAJC-B (Tier 2) | `standards/TIA-102.BAJB-B/…` |
| ISSI Voice | BACA-B | `standards/TIA-102.BACA/…` |
| ISSI Conventional (Ec) | BACE | `standards/TIA-102.BACE/…` |
| ISSI Packet Data | BACF | `standards/TIA-102.BACF/…` |
| Conventional procedures | BAAD-B | `standards/TIA-102.BAAD-B/…` |
| Conventional control messages | AABG | `standards/TIA-102.AABG/…` |
| Conventional Mgmt Service (packet data) | BAEJ-A | `standards/TIA-102.BAEJ-A/…` |
| Fixed Station Interface | BAHA-A | `standards/TIA-102.BAHA-A/…` |
| Network Management Interface | TSB-102.BAFA-A | `standards/TSB-102.BAFA-A/…` |
| LMR ↔ 3GPP MC Interworking | J-STD-200 | `standards/J-STD-200/…` |

---

## 6. Revision Lineage

```
TSB-102     — original     (Feb 2009)
TSB-102-A   — rev A        (date TBD)
TSB-102-B   — rev B        (date TBD)
TSB-102-C   — rev C        (pre-2020, superseded)
TSB-102-D   — rev D        (March 2020) ← current
```

TSB-102-C was marked `not_required` in the earlier hygiene pass of
this repo since TSB-102-D supersedes it.

---

## 7. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- Scope and purpose — TSB-102-D §1.
- Services Taxonomy — §2.
- Functional System Elements and Open Interfaces — §3.
- Document Suite Structure — §4.
- Area × document cross-reference tables — §4 / Annex A.

---

## 8. Cross-References

This is the top-level nav hub. It links to every downstream impl spec
in this repo.

**Area-specific navigation hubs:**
- `standards/TIA-102.AABA-B/P25_Trunking_Overview_Implementation_Spec.md`
  — Trunking.
- `standards/TIA-102.BAEA-C/P25_Data_Overview_Implementation_Spec.md`
  — Data Services.
- `standards/TIA-102.AAAB-B/P25_Security_Services_Overview_Implementation_Spec.md`
  — Security Services.
- `standards/TSB-102.BBAA/P25_Phase2_TDMA_Overview_Implementation_Spec.md`
  — Phase 2 TDMA.
- `standards/TSB-102.BAFA-A/P25_Network_Management_Interface_Overview_Implementation_Spec.md`
  — Network Management Interface (En).
- `standards/TSB-102.BAJA-B/…` (bundled with `.BAJB-B/…`) — Location
  Services architecture.
- `standards/J-STD-200/P25_LMR_3GPP_MC_Interworking_Study_Implementation_Spec.md`
  — LMR ↔ 3GPP interworking.

**All other processed impl specs** appear in the area tables in §5.

**Cross-repo analysis notes** (value-adds beyond spec extractions):
- `analysis/fdma_pdu_frame.md` — unified FDMA PDU view.
- `analysis/motorola_sndcp_npdu_preamble.md` — Motorola trunked IV&D layering.
- `analysis/motorola_conventional_scep_vs_trunked_sndcp.md` — conventional vs trunked data wrappers.
- `analysis/vocoder_wire_vs_codec.md` — wire format vs codec separation.
- `analysis/vocoder_missing_specs.md` — AMBE+2 / BABB / BABG gap context.
