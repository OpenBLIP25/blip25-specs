# TIA-102-BADA-1 Related Resources and Context

**Document:** ANSI/TIA-102.BADA-1 2006  
**Topic:** P25 Telephone Interconnect — Conventional Individual Calls (Addendum 1)

---

## Status

This document is **active** as a published ANSI/TIA standard (approved May 3, 2006). It exists as a formal addendum to TIA/EIA-102.BADA. The foreword states that the content was intended to be incorporated into the next complete revision of TIA/EIA-102.BADA; whether that absorption has occurred depends on the current revision status of TIA-102.BADA. As of the document's publication, TIA/EIA-102.BADA was the base document and this addendum extended it with optional procedures specific to conventional individual phone calls. Practitioners should check TIA's current catalog for the latest revision status of both TIA-102.BADA and this addendum.

---

## Standards Family

This document is part of the TIA-102 (Project 25 / APCO-25) standards suite, specifically the **BADA sub-series** covering telephone interconnect (phone patch) for P25 voice services.

**Direct parent:**
- **TIA/EIA-102.BADA** — Project 25 Telephone Interconnect Requirements and Definitions (Voice Service) — the base document this addendum extends. Defines the core phone patch architecture, mandatory baseline procedures, and the original Annex A tables (replaced here).

**Message encoding dependencies:**
- **TIA-102.AABF** (Link Control Word Formats and Messages) — referenced as 1.2.6 (TIA-102.AABF-A, December 2004). Provides the `LC TELE INT V CH USR` link control word and the call termination/cancellation LC word used in §3.4.1 of this document.
- **TIA-102.AABG-1** (Conventional Control Messages Addendum 1 — Individual Telephone Calls) — referenced as 1.2.7, published as SP-3-3604-AD1 and expected to become TIA-102.AABG-1. Provides the wire-format definitions for the new messages introduced in this addendum: Telephone Interconnect Answer Request, Answer Response, Acknowledge/Deny/Queued responses, and Cancel Service Request on conventional channels.

**Related conventional channel specifications:**
- **TIA-102.BAAA** — Project 25 Conventional Channel Operation (core conventional channel procedures)
- **TIA-102.BAAD** — Project 25 Conventional Channel Operations (procedures, maintenance)
- **TIA-102.BABA** — Conventional vocoder (IMBE/AMBE audio codec in conventional systems)

**Trunked system telephone interconnect:**
- The base **TIA-102.BADA** covers both trunked and conventional systems. Trunked telephone interconnect uses different channel grant mechanisms (Voice Channel Grant messages) that are not applicable to conventional operation.

**Standards lineage (ASCII tree):**

```
TIA-102 (Project 25 Standards Suite)
└── Series B — Air Interface Standards
    └── Series BA — Conventional Systems
        └── Series BAD — Telephone Interconnect
            ├── TIA-102.BADA        (Base: Telephone Interconnect Req & Definitions, Voice)
            └── TIA-102.BADA-1      (THIS DOCUMENT: Addendum 1 — Conventional Individual Calls)
    └── Series BAB — Conventional Protocols
        └── TIA-102.BABA            (Conventional vocoder)
        └── TIA-102.BAAD            (Conventional channel procedures)
└── Series AA — Common Air Interface
    └── TIA-102.AABF               (Link Control Word Formats — message encoding for §3.4.1)
    └── TIA-102.AABG-1             (Conventional Control Messages Addendum 1 — message encoding)
```

---

## Practical Context

Telephone interconnect (phone patch) is a feature of P25 conventional and trunked systems that allows radio users to place or receive calls through the Public Switched Telephone Network (PSTN). A radio user keys PTT and dials digits to initiate an outgoing call; the RF subsystem (RFSS) — typically a repeater or base station controller with a telephone line interface — connects the call. Incoming calls from the PSTN to a specific radio user (individual call) require the system to identify the target radio, alert it if possible, and bridge the audio.

This addendum specifically addresses the **incoming individual call** scenario on **conventional** (non-trunked) P25 channels, which was underspecified in the base document. The key practical problem it solves is that on conventional channels there is no trunked channel grant mechanism — the SU is always monitoring the conventional frequency, so the RFSS must use an over-the-air signaling message (the Telephone Interconnect Answer Request) to perform an availability check before connecting the incoming PSTN call. Without this, the RFSS had no way to know whether the target radio user was available or willing to receive the call, and no clean mechanism to route unanswered calls to voicemail or an operator.

In real deployments, telephone interconnect on conventional channels is common in public safety, utility, and industrial systems where operators at dispatch or in the field need to bridge PSTN calls to radio users without requiring a full trunked system infrastructure. The RFSS in this context is typically implemented in conventional base stations and repeater controllers from vendors such as Motorola (ASTRO conventional), Harris/L3Harris, Kenwood, and Tait Communications. Tait (the contributor of this addendum via Rex Nisbet) has long been a significant vendor of conventional P25 infrastructure.

The availability check timeout of 1 second specified in §3.3.1 is a practical implementation constraint — it balances the need to give the radio user time to respond against the telephone caller's expectation of prompt ring-back.

The note in §3.1.2 that "tones relayed in this way may be distorted or blocked by the P25 vocoder" is a well-known practical limitation: DTMF and supervisory tones do not always survive IMBE/AMBE vocoding faithfully, and implementors typically use in-band signaling or parallel signaling paths rather than relying on vocoded tones for call progress.

---

## Key Online Resources

- **TIA Standards Catalog** — TIA-102.BADA-1 can be purchased from:  
  https://store.accuristech.com/tia  
  (Search "TIA-102.BADA-1")

- **APCO P25 Standards Overview** — APCO International maintains a P25 standards page:  
  https://www.apcointl.org/technology/p25/

- **P25 Technology Interest Group (PTIG)** — Industry implementation guidance:  
  https://www.project25.org/

- **FCC P25 CAP (Compliance Assessment Program)** — Tests telephone interconnect features among others:  
  https://www.fcc.gov/public-safety-and-homeland-security/policy-and-licensing-division/p25-compliance-assessment-program

- **DHS SAFECOM P25 Documentation** — US federal agency guidance on P25 phone patch and interoperability:  
  https://www.cisa.gov/safecom/p25

- **PTIG Telephone Interconnect documentation** — The P25 Technology Interest Group has published implementation guidance documents covering phone patch procedures. Check their document library at project25.org.

---

## Open-Source Implementations

Phone patch / telephone interconnect on conventional P25 is implemented in several open-source P25 projects, primarily at the RFSS (repeater/gateway) layer:

- **OP25** (Osmocom P25):  
  https://github.com/osmocom/op25  
  OP25 is a GNU Radio-based P25 receiver/transmitter. Its conventional channel handling includes the data link control layer where telephone interconnect messages would be processed. Search the codebase for `TELE_INT` or `telephone_interconnect` for relevant message handling.

- **SDRTrunk**:  
  https://github.com/DSheirer/sdrtrunk  
  SDRTrunk is a Java-based P25 decoder and trunking controller. It decodes conventional P25 channels and Link Control Words (including `LC TELE INT V CH USR` words referenced in Annex B of this document). Relevant source: `src/main/java/io/github/dsheirer/module/decode/p25/`.

- **p25patch** and related projects — Hobbyist implementations of phone patch gateways for P25 conventional repeaters are often shared on the repeater-builder.com forums and GitHub. These typically implement the RFSS side of the procedures described in Section 3 of this document.

- **Asterisk P25 modules** — Some integration projects connect P25 conventional systems to Asterisk PBX for telephone interconnect, implementing the RFSS Et-point interface described in Annex A Table A.2.

No open-source project is known to implement the full availability check procedure (§3.3) for incoming individual PSTN calls on conventional channels — this is typically handled in proprietary base station firmware (Tait, Motorola, Harris).

---

## Notes for Implementors

1. **Message encoding is in TIA-102.AABG-1**, not this document. This addendum defines behavioral procedures only; wire formats for the new messages are in TIA-102.AABG-1.

2. **Link control word for call teardown** is in TIA-102.AABF (§6.3.8 Call Termination/Cancellation).

3. **The 1-second availability check timeout** (§3.3.1) is the only timing parameter defined in this document. Other timers are inherited from TIA/EIA-102.BADA.

4. **Interoperability fallback is mandatory**: any RFSS implementing availability checks must also support disabling them per-subscriber; any SU must accept incoming phone call audio even without a preceding availability check.

5. **Annex A tables replace** (not supplement) Tables A.1 and A.2 of TIA/EIA-102.BADA. Implementations conforming to this addendum should reference the tables in Annex A of this document rather than those in the base spec.
