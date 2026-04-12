# TIA-102-AACD-1 Related Resources & Context

**Document:** TIA-102.AACD-1 — Project 25 KFD Interface Protocol Addendum 1: Key Fill for Link Layer Authentication  
**Published:** April 2011  

---

## Status

This document is an **active addendum** to TIA-102.AACD. It has not been superseded or withdrawn as of 2026. It remains the normative specification for loading Link Layer Authentication keys via the KFD interface. Because it is an addendum rather than a revision, it coexists with the base TIA-102.AACD document — both must be implemented together for full KFD interoperability including LLA key management.

The TR-8.3 Encryption Subcommittee maintains this standard. The note in the document that Algorithm ID $85 (AES-128 authentication) "is still under proposal in TR8.3" was resolved in later editions of companion standards — implementors should treat $85 as the established AES-128 LLA algorithm ID.

---

## Standards Family

This addendum sits within the TIA-102 encryption/key management cluster:

```
TIA-102 Project 25 Standards Suite
└── Encryption & Key Management
    ├── TIA-102.AACA — OTAR Protocol (defines KMM framing, primitive fields, status codes)
    │   └── (base KMM vocabulary used throughout AACD-1)
    ├── TIA-102.AACD — Key Fill Device (KFD) Interface Protocol (base)
    │   └── TIA-102.AACD-1 — THIS DOCUMENT: Addendum 1, Key Fill for LLA
    ├── TIA-102.AACE — Link Layer Authentication (the LLA protocol this addendum supports)
    └── TIA-102.AAAD — (encryption algorithm suite)
```

**Companion documents:**
- **TIA-102.AACE** (December 2005) — Defines what Link Layer Authentication is: the challenge-response handshake between SU and FNE, the role of K, SUID scoping, and the on-air protocol. This addendum is meaningless without AACE.
- **TIA-102.AACD** (base, ~2005) — Defines the KFD-P25 transport: connection establishment (Ready P25 Req / Ready P25 General Mode), Transfer Done, Disconnect/Ack, and the original key fill KMMs for traffic encryption keys.
- **TIA-102.AACA** (April 2001) — OTAR protocol; provides the KMM framing convention, Primitive Field Definitions (Algorithm ID, Key ID, Inventory Type, etc.), the full status code table (Table 4 in this document), and the 512-octet KMM size limit. All field encodings in this addendum reference AACA's Primitive Field Definition sections.
- **FIPS 197** — AES specification; defines the cipher whose keys this addendum transports.

---

## Practical Context

Link Layer Authentication is a P25 security feature that allows infrastructure (Fixed Network Equipment, FNE) to challenge subscriber radios before allowing them to operate on the channel. The challenge-response is based on AES-128 CMAC (a Message Authentication Code), using a pre-provisioned shared secret K. This document defines how that K gets into the radio in the first place.

**KFD workflow in the field:**
A Key Fill Device (KFD) — typically a handheld device or software tool — connects directly to a radio via a standardized physical interface (usually a 6-pin Hirose connector, or in modern deployments, Bluetooth or USB adapters). The KFD holds the authentication keys assigned to each subscriber by the system administrator. The procedure in this addendum (§4.1 Load Authentication Key) is run once per radio to provision its K for the network's WACN/System. The mandatory View Active SUID Info procedure (§4.4) lets the KFD discover which SUID the radio considers "currently active" before deciding what key to load.

**Why this matters operationally:**
Without a standardized KFD protocol for LLA keys, a system operator would need manufacturer-specific tools for every radio brand in their fleet. This addendum enables interoperability: a KFD from one vendor can load authentication keys into a radio from another vendor, provided both implement this addendum and the base AACD protocol.

**Deployment context:**
LLA is typically enabled in high-security P25 deployments (federal law enforcement, military, critical infrastructure). Many P25 systems in standard public safety use do not enable LLA. Where it is enabled, the KFD provisioning workflow using this addendum is a precondition for radio operation on that system.

---

## Key Online Resources

- **TIA standards catalog** — The official source for purchasing TIA-102.AACD-1 and companion documents:  
  https://www.tiaonline.org/standards/catalog/

- **APCO Project 25 Technology Interest Group (PTIG)** — Industry body that oversees P25 interoperability testing and publishes implementation guidance:  
  https://www.project25.org/

- **DHS SAFECOM P25 CAP** — The Compliance Assessment Program that tests P25 radios for standard conformance. LLA-related CAP testing references AACE:  
  https://www.dhs.gov/safecom/p25-cap

- **NIST FIPS 197 (AES)** — Free download of the AES specification referenced normatively:  
  https://csrc.nist.gov/publications/detail/fips/197/final

---

## Open-Source Implementations

Open-source P25 projects that are relevant to this specification:

- **OP25** (GNU Radio-based P25 SDR receiver/decoder):  
  https://github.com/boatbod/op25  
  OP25 implements P25 decoding and trunking. KFD protocol support is not a focus (OP25 is a receiver, not a provisioning tool), but the SUID structure and LLA-related message parsing are relevant to understanding captured traffic.

- **SDRTrunk** (Java-based P25 SDR decoder):  
  https://github.com/DSheirer/sdrtrunk  
  SDRTrunk decodes P25 trunking and includes message parsing. The SUID and KMM structures from AACA (which this addendum extends) are implemented in the message parsing layer.

- **p25lib** and related tools:  
  Various community tools for parsing P25 KMM frames exist in the amateur radio and public safety SDR community. Search GitHub for "p25 kmm" or "p25 kfd" for experimental implementations.

- **GR-P25** (GNU Radio P25 blocks):  
  https://github.com/gr-p25  
  Another SDR implementation; relevant for understanding the framing this addendum's KMMs are wrapped in.

**Note:** No known open-source implementation provides a complete, standards-conformant KFD provisioning tool implementing this specific addendum. Commercial KFD software (e.g., Motorola KVL 4000, Harris SecNet, Tait KFD tools) implements this standard but is not open-source.

---

## Standards Lineage

```
FIPS 197 (AES, 2001)
    │
    └── Provides cipher for K
    
TIA-102.AACA (OTAR Protocol, April 2001)
    │
    ├── Defines KMM framing, Primitive Fields, status codes, 512-octet KMM limit
    │
    └── TIA-102.AACD (KFD Interface Protocol, ~2005)
            │
            ├── Defines KFD-P25 transport (Ready, Transfer Done, Disconnect)
            ├── Defines original traffic encryption key fill KMMs
            │
            └── TIA-102.AACD-1 (THIS DOCUMENT, April 2011)
                    │
                    ├── Adds 8 KMMs: Load/Delete/Inventory for LLA authentication keys
                    └── Requires TIA-102.AACE for the LLA protocol context

TIA-102.AACE (Link Layer Authentication, December 2005)
    │
    └── Defines the on-air LLA challenge-response; this addendum provisions its keys
```

---

## Phase 3 Flag

**Implementation spec recommended for follow-up pass:**

This document is dual **PROTOCOL + MESSAGE_FORMAT**. A follow-up implementation spec should cover:

1. **Message parsing spec** — Byte-level dispatch table for the four new message IDs ($28, $29, $2A, $2B) and the two Inventory Type discriminators ($F7, $F8) within message IDs $0D/$0E. Variable-length handling for the Load Authentication Key-Command (Decryption Instruction Block with optional 9-byte Message Indicator field). SUID bit-field extraction (56-bit compound: WACN[20] || SystemID[12] || SubscriberID[24]).

2. **Protocol/procedures spec** — State machine for each of the four procedures (§4.1–4.4), including the mandatory vs. optional distinction, the Inventory Marker continuation loop for List SUID Items, and the combined List Active SUID + Load Key session variant. Error handling per status code table.
