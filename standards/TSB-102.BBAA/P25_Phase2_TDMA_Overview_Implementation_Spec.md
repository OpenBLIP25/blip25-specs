# P25 Phase 2 Two-Slot TDMA Overview — TSB-102.BBAA Implementation Spec

**Source:** TSB-102.BBAA (March 2010), *Project 25 Two-Slot TDMA
Overview*. Informative TIA Telecommunications Systems Bulletin.
Developed under TR-8.12 (Two Slot TDMA Subcommittee).

**Document type:** navigation-hub overview. TSB-102.BBAA is **not
normative** — it describes Phase 2 TDMA at a high level, cataloguing
the full suite of companion standards that define the normative
technical details and explaining the migration / interoperability
strategy between Phase 1 FDMA and Phase 2 TDMA equipment.

This derivation is therefore a short nav hub similar to AABA-B (Trunking
Overview) / BAEA-C (Data Overview) / AAAB-B (Security Overview) — it
points at the right downstream spec for each Phase 2 question.

**Scope of this derived work:**
- §1 — Why Phase 2 exists (6.25e spectrum efficiency)
- §2 — Um2 air interface: H-CPM inbound, H-DQPSK outbound
- §3 — Protocol layering: BBAB (PHY) + BBAC (MAC)
- §4 — Superframe structure (12 × 30 ms slots / 360 ms)
- §5 — Dual-rate vocoder (Full Rate + Half Rate AMBE+2) and rate conversion
- §6 — Trunking integration: shared FDMA control channel
- §7 — Encryption reuse and the FDMA/TDMA transcoding caveat
- §8 — Cross-reference map: which spec answers which question
- §9 — Cite-to section references
- §10 — Cross-references

**Pipeline artifacts:**
- `standards/TSB-102.BBAA/TSB-102-BBAA_full_text.md` — clean-room
  extraction (lowercase filename convention from earlier pipeline run).
- `standards/TSB-102.BBAA/TSB-102-BBAA_summary.txt` — retrieval summary.
- `standards/TSB-102.BBAA/TSB-102-BBAA_context.md` — lineage and external references.

---

## 1. Why Phase 2 Exists

Per TSB-102.BBAA §1 + §2.

**FCC spectrum efficiency requirement.** Public safety radio systems
must achieve **6.25 kHz-per-voice-channel equivalent** efficiency.

| Phase | Channel | Voices per 12.5 kHz | Efficiency |
|-------|---------|---------------------|------------|
| Phase 1 FDMA | 12.5 kHz | 1 voice call | 12.5 kHz per voice |
| **Phase 2 TDMA** | 12.5 kHz | **2 simultaneous voice calls** | **6.25e** (equivalent to 6.25 kHz per voice) |

Phase 2 doubles spectrum efficiency by fitting **two voice calls into a
single 12.5 kHz channel** using two-slot TDMA. Same bandwidth, twice
the voice capacity.

---

## 2. Um2 Air Interface

Per TSB-102.BBAA §3.

**Um2** is the new Phase 2 Common Air Interface, distinct from the Phase 1
Um interface. Key properties:

- **Channel bandwidth:** 12.5 kHz occupied.
- **Gross bit rate:** 12 kb/s.
- **Asymmetric modulation** — different schemes for inbound vs outbound.

### 2.1 Inbound (SU → infrastructure): H-CPM

**H-CPM** = Harmonized Continuous Phase Modulation. Constant-envelope.
Chosen because:
- **Constant envelope** allows subscriber portables to use efficient
  class-C amplifiers → **longer battery life**.
- Spectrally efficient enough to fit within the 12.5 kHz channel.

### 2.2 Outbound (infrastructure → SU): H-DQPSK

**H-DQPSK** = Harmonized Differential QPSK. Linear modulation. Chosen
because:
- **Large excess bandwidth ratio** → improved **simulcast delay-spread
  tolerance** compared to C4FM (Phase 1).
- Infrastructure equipment has no battery-life constraint, so a linear
  modulation with its higher transmitter back-off is acceptable.

This asymmetric design is unusual in LMR but represents a deliberate
trade-off: battery life where it matters (portable), robustness
where it matters (simulcast infrastructure).

---

## 3. Protocol Layering

Per TSB-102.BBAA §3–§4.

The TDMA CAI splits into two layers:

| Layer | Document | Purpose |
|-------|----------|---------|
| **Physical (PHY)** | **TIA-102.BBAB** | Modulation, framing, bit-level FEC |
| **Media Access Control (MAC)** | **TIA-102.BBAC-A** (+ scrambling in BBAC-1) | Slot organization, burst types, header formats |

MAC layer messages are defined in:

| Document | Purpose |
|----------|---------|
| **TIA-102.BBAD-A** (+ BBAD-A-1 LoPTT/User Alias) | MAC Layer messages (all opcodes) |
| **TIA-102.BBAE** | MAC Layer Procedures |

All of BBAB / BBAC-A / BBAD-A / BBAE / BBAC-1 are **already processed
in this repo** — see §8 for the direct impl-spec links.

---

## 4. Superframe Structure

Per TSB-102.BBAA §3.

- **Slot duration:** 30 ms.
- **Superframe:** 360 ms = **12 slots**.
- **Two logical voice paths** share the superframe in a 2-slot
  interleave (1/4, 2/5, 7/10, 8/11 or similar — see BBAC-A for the
  normative slot-allocation tables).

### 4.1 Dedicated signaling

- **Dedicated signaling slots** occur every 360 ms **without
  interrupting voice traffic**.
- **Inter-slot Signaling Channel (ISCH)** appears between slots on the
  outbound link for synchronization and channel supervision.

Phase 2 achieves continuous voice + embedded signaling without the
voice / signaling alternation Phase 1 FDMA uses (LDU1/LDU2/TDU).

---

## 5. Dual-Rate Vocoder

Per TSB-102.BBAA §5.

Phase 2 uses a **dual-rate vocoder**:

| Rate | Bitrate | Used on |
|------|---------|---------|
| **Full Rate** | 7.2 kb/s gross (same as Phase 1 IMBE/AMBE+2) | FDMA LDU (Phase 1 interop) |
| **Half Rate** | 3.6 kb/s | TDMA VTCH voice bursts (Phase 2) |

**Same fundamental speech-processing algorithm.** Both rates share
the same parameter estimation, enabling **parametric rate conversion
between them** for Phase 1 ↔ Phase 2 interoperability.

**Half rate vocoder annex:** TIA-102.**BABA-1** (separate spec, not
processed in this repo as of this pass).

**Tone encoding:** the vocoder also includes tone-encoding capability
for sending signaling tones through the voice path — used for
in-band telephone interconnect DTMF. See the BADA-A/BADA-1 telephone
interconnect impl spec for the conventional-mode implication.

---

## 6. Trunking Integration

Per TSB-102.BBAA §4.

**Key design decision:** Phase 2 **reuses the Phase 1 FDMA trunking
control channel** — the control channel is always FDMA, only voice
channels are TDMA.

Consequences:
1. **Incremental migration** — TDMA voice channels can be added to an
   existing FDMA trunked system **without replacing the control
   channel**.
2. **TSBK and LCW extensions** identify radio FDMA/TDMA capabilities
   during registration, assign TDMA traffic channels, and optionally
   synchronize FDMA control channel timing with TDMA traffic channel
   slot boundaries.
3. The single FDMA control channel serves both Phase 1 and Phase 2
   radios.

See `standards/TIA-102.AABA-B/P25_Trunking_Overview_Implementation_Spec.md`
§2 for the "always-FDMA control channel" architectural rule.

---

## 7. Encryption and the Transcoding Caveat

Per TSB-102.BBAA §6.

Phase 2 **reuses** the Phase 1 encryption architecture:
- The **Encryption Synchronization Signal (ESS)** (MI + ALGID + KID)
  is transmitted every 360 ms in TDMA just as in FDMA — **preserves
  late-entry performance**.
- The ESS clocking schedule is adapted for **49-bit half-rate vocoder
  frames** (vs 88-bit full-rate).
- **OTAR follows the Phase 1 standard** unchanged (AACA-D).

### 7.1 Critical limitation: end-to-end encrypted FDMA ↔ TDMA calls

When a call spans Phase 1 FDMA and Phase 2 TDMA radios and **rate
conversion is needed** (Full Rate ↔ Half Rate), the infrastructure
must:
1. **Decrypt** the incoming bitstream (breaks E2E encryption here).
2. **Rate-convert** at the parametric level.
3. **Re-encrypt** for the outgoing rate.

This breaks end-to-end encryption at the conversion point. The
encrypted bitstreams differ between rates, so transparent pass-through
is impossible.

**Operational consequence.** Agencies deploying mixed Phase 1 / Phase 2
encrypted groups must accept that the infrastructure holds keys at the
transcoding boundary. If that's unacceptable, deploy single-rate
encrypted groups only.

See
`standards/TIA-102.AAAB-B/P25_Security_Services_Overview_Implementation_Spec.md`
for the broader security context.

---

## 8. Cross-Reference Map

TSB-102.BBAA's value is as a router. The table below maps high-level
Phase 2 questions to the impl spec that answers them.

| Question | Normative spec | Impl spec in this repo |
|----------|----------------|------------------------|
| H-CPM / H-DQPSK modulation, bit framing | TIA-102.BBAB | `standards/TIA-102.BBAB/…` |
| Slot structure, burst types, ISCH | TIA-102.BBAC-A | `standards/TIA-102.BBAC-A/P25_TDMA_MAC_Layer_Implementation_Spec.md` |
| Scrambling (LFSR, seeds) | TIA-102.BBAC-1 | `standards/TIA-102.BBAC-1/P25_TDMA_Scrambling_Implementation_Spec.md` |
| MAC messages (all opcodes) | TIA-102.BBAD-A | `standards/TIA-102.BBAD-A/P25_TDMA_MAC_Message_Parsing_Implementation_Spec.md` |
| LoPTT + User Alias | TIA-102.BBAD-A-1 | `standards/TIA-102.BBAD-A-1/P25_TDMA_LoPTT_UserAlias_Implementation_Spec.md` |
| MAC procedures (state machines) | TIA-102.BBAE | `standards/TIA-102.BBAE/P25_TDMA_MAC_Procedures_Implementation_Spec.md` |
| Half-rate vocoder | TIA-102.BABA-1 | (not processed; see specs.toml) |
| FDMA control channel (used by TDMA too) | AABB-B / AABC-E / AABD-B | existing impl specs |
| Encryption architecture | AAAD-B | `standards/TIA-102.AAAD-B/P25_Block_Encryption_Protocol_Implementation_Spec.md` |

**For a first-time Phase 2 implementer:** read this overview first,
then BBAC-A for the burst framing, then BBAD-A for message parsing.
BBAB for physical-layer details. BBAC-1 for scrambling if needed.

---

## 9. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- Scope and motivation (6.25e efficiency requirement) — TSB-102.BBAA §1.
- Um2 interface architecture — §2 / §3.
- H-CPM inbound / H-DQPSK outbound modulation — §3.
- PHY (BBAB) / MAC (BBAC) / messages (BBAD) / procedures (BBAE) split — §3.
- Superframe structure (12 slots × 30 ms) — §3.
- Dual-rate vocoder and rate conversion — §5.
- TSBK and LCW extensions for FDMA/TDMA migration — §4.
- ESS reuse from Phase 1 — §6.
- Half-rate frame (49-bit) ESS clocking — §6.
- Encryption transcoding caveat — §6.

---

## 10. Cross-References

**Phase 2 TDMA spec family (all already processed):**
- `standards/TIA-102.BBAB/P25_TDMA_Physical_Layer_Implementation_Spec.md`
  — PHY.
- `standards/TIA-102.BBAC-A/P25_TDMA_MAC_Layer_Implementation_Spec.md`
  — MAC.
- `standards/TIA-102.BBAC-1/P25_TDMA_Scrambling_Implementation_Spec.md`
  — Scrambling.
- `standards/TIA-102.BBAD-A/P25_TDMA_MAC_Message_Parsing_Implementation_Spec.md`
  — Messages.
- `standards/TIA-102.BBAD-A-1/P25_TDMA_LoPTT_UserAlias_Implementation_Spec.md`
  — LoPTT + User Alias addendum.
- `standards/TIA-102.BBAE/P25_TDMA_MAC_Procedures_Implementation_Spec.md`
  — Procedures.

**Related overviews:**
- `standards/TIA-102.AABA-B/P25_Trunking_Overview_Implementation_Spec.md`
  — establishes the FDMA-control-channel-for-all rule that makes
  Phase 2 incremental migration possible.
- `standards/TIA-102.BAEA-C/P25_Data_Overview_Implementation_Spec.md`
  — Phase 2 packet data is flagged as future work in BAEA-C §3.7.

**Phase 1 / Phase 2 mapping:**
- `analysis/fdma_pdu_frame.md` — FDMA PDU view; Phase 2 reuses some of
  these constructs.
- `analysis/vocoder_wire_vs_codec.md` — separates wire format from codec,
  relevant to the dual-rate transcoding story.
