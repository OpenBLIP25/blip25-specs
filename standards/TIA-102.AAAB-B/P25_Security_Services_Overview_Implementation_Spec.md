# P25 Security Services Overview — TIA-102.AAAB-B Implementation Spec

**Source:** ANSI/TIA-102.AAAB-B (February 2019), *Security Services
Overview*. Supersedes AAAB-A (2004 editorial revision) and the
original AAAB (2002).

**Document type:** architectural overview (like AABA-B for trunking and
BAEA-C for data). AAAB-B establishes the security taxonomy, threat
model, and key-management architecture for the entire TIA-102 security
sub-suite. It does not specify wire formats — every normative detail
is in a companion spec.

This derivation is therefore a navigation hub: it catalogs the
threats, service categories, and key-management architecture at the
level an implementer needs to **pick the right spec to read next** for
any given security-related implementation question.

**Scope of this derived work:**
- §1 — The four security service categories
- §2 — Nine threats catalogued
- §3 — Confidentiality: encryption types, cipher modes, the MI
- §4 — Integrity: chronological + message (CBC-MAC)
- §5 — Authentication: Group Source, Individual Source, Challenge-Response
- §6 — Key management lifecycle and distribution mechanisms
- §7 — Key Management Architecture: Voice, CAI Data, IP Data, LLA, Link Layer Encryption
- §8 — Incomplete sections flagged (IP Data Encryption, Link Layer Encryption)
- §9 — Deprecated algorithms (DES, 3DES)
- §10 — Cross-reference map: which spec answers which question
- §11 — Cite-to section references
- §12 — Cross-references

**Pipeline artifacts:**
- `standards/TIA-102.AAAB-B/TIA-102-AAAB-B_Full_Text.md` — clean-room
  extraction (copyrighted, git-ignored).
- `standards/TIA-102.AAAB-B/TIA-102-AAAB-B_Summary.txt` — retrieval summary.
- `standards/TIA-102.AAAB-B/TIA-102-AAAB-B_Related_Resources.md`.

---

## 1. Four Security Service Categories

AAAB-B §§3–6 partitions TIA-102 security into four disjoint services.
Every TIA-102 security feature belongs to exactly one:

| Service | What it guarantees | Primary TIA-102 spec |
|---------|-------------------|----------------------|
| **Confidentiality (Privacy)** | Content is readable only by authorized parties | AAAD-B (Block Encryption Protocol) |
| **Integrity** | Content was not altered or replayed in transit | AAAD-B (for in-message MAC); AACA-D (for KMM sequence numbers) |
| **Authentication** | The sender is who they claim to be | AACE-A (Link Layer Authentication) |
| **Key Management** | Keys are generated, distributed, rotated, and destroyed safely | AACA-D (OTAR), AACD-A/B (KFD), BAKA-A (inter-KMF) |

---

## 2. Nine Threats Catalogued

AAAB-B §2 enumerates the threats that security services counter.
Use this when triaging whether a reported anomaly is a known-scoped
issue vs a new gap:

| # | Threat | Countermeasure category |
|---|--------|--------------------------|
| 1 | **Message Interception** — passive eavesdropping | Confidentiality |
| 2 | **Message Replay** — re-transmit recorded traffic | Integrity (chronological) |
| 3 | **Spoofing** — impersonate a legitimate user | Authentication |
| 4 | **Misdirection** — impersonate the infrastructure | Authentication (FNE-side) |
| 5 | **Jamming** — RF interference | **Explicitly out of scope** |
| 6 | **Traffic Analysis** — inferring patterns without decryption | Air Interface Encryption (§7.2.2 — on hold) |
| 7 | **Subscriber Duplication** — cloning a radio's identity | Authentication + physical provisioning |
| 8 | **Theft of Service** — unauthorized system use | Authentication |
| 9 | **Theft of Unit** — physical theft | Zeroize procedures + FNE-side rejection |

**Implementation consequence for defensive tooling.** A decoder
logging "suspicious" traffic should classify alerts by threat number
when possible — this maps the alert to the standard's own vocabulary
and makes it easier for operators familiar with the threat model.

---

## 3. Confidentiality

### 3.1 Encryption Types (I–IV)

AAAB-B §3 defines four encryption types in descending security level.
The type constrains which algorithms are allowed and whether the
implementation requires export / classification controls:

| Type | Security level | Typical algorithm(s) | Notes |
|------|----------------|----------------------|-------|
| **Type I** | Highest (US Government classified) | NSA-approved classified algorithms | Not in TIA-102 proper; covered by separate classified docs |
| **Type II** | Sensitive-but-unclassified (US Government) | NSA-approved unclassified | Similar to Type I in scope |
| **Type III** | Commercial / public safety | AES-256, AES-128 | The common case for civilian P25 deployments |
| **Type IV** | Lower strength / legacy | DES (56-bit), 3DES | **Deprecated** — see §9 |

**Field observation.** Virtually all operational US public-safety P25
systems are Type III (AES-256). Legacy systems may still emit Type IV
(DES) — a decoder should log the ALGID and flag Type IV occurrences
as "deprecated" for operator awareness.

### 3.2 Cipher Modes

AAAB-B §3.1 names five cipher modes and their preferred use:

| Mode | Used for |
|------|----------|
| **ECB** (Electronic Code Book) | Discouraged for traffic; appears in some KMM wrapping scenarios |
| **OFB** (Output Feedback) | **Preferred for voice/data traffic** |
| **CFB** (Cipher Feedback) | Alternative streaming mode |
| **CBC** (Cipher Block Chaining) | **Used for MAC generation** (§4.2) |
| **CTR** (Counter) | **Preferred for voice/data traffic** (alternative to OFB) |

**Traffic encryption.** OFB / CTR are stream-cipher-style — they
XOR a keystream against plaintext, letting the underlying block
cipher run in parallel with voice frame arrival.

**MAC generation.** CBC is the only mode whose block-chaining produces
a key-dependent final block that can't be forged without the key —
hence its use for Message Authentication Code computation.

### 3.3 The Message Indicator (MI)

AAAB-B §3.1 + AAAD-B. The **MI** is the synchronization value that
the encryptor and decryptor share to derive the per-message keystream
(for OFB / CTR) or the initial block (for CBC / CFB).

**How it's carried:**
- **Traffic Encryption**: MI is in the Encryption Sync sequence; for
  FDMA it's distributed across the LDU1 / LDU2 hyperframes as defined
  in BAAA-B / AABF-D; for TDMA it's in the ESS field per BBAC-A.
- **Air Interface Encryption** (on-hold): the base station **periodically
  broadcasts** the MI so new entrants to the channel can decrypt
  in-progress signaling.

**Decoder use.** The MI is the anchor that ties a voice or data burst
to its crypto state. A decoder that sees ALGID=Type-III + KID=0x1234
+ MI=<72-bit value> has everything it needs to **attempt** decryption
(given the key) — the standard supplies the protocol; the key itself
must come from operational provisioning.

---

## 4. Integrity

Two sub-services per AAAB-B §4:

### 4.1 Chronological Integrity (§4.1)

Protects against message replay — an adversary recording a valid
message and re-emitting it later.

- **For Key Management Messages (KMMs)**: monotonic **sequence
  number** in every KMM. Receivers reject out-of-order KMMs.
  Protocol defined in AACA-D (OTAR).
- **For Air Interface encrypted traffic**: **MI synchronization
  window**. The receiver accepts MIs within a small drift from its
  current tracking; far-offset MIs are treated as replay candidates
  and discarded.

### 4.2 Message Integrity (§4.2)

Protects against content alteration. A **Message Authentication Code
(MAC)** is computed with a key-dependent function — specifically
**CBC mode over message content** — and appended **before encryption**.
The MAC is key-dependent, so a forger without the key cannot produce
a valid MAC even if they can decrypt.

**Decoder implication.** A decoder that recovers plaintext after
decryption should verify the MAC before trusting content. A failed
MAC check after successful decryption suggests either bit errors
(retry / request retransmission) or an active attack.

---

## 5. Authentication

AAAB-B §5 distinguishes three authentication mechanisms:

### 5.1 Group Source Authentication

Available when **all parties in a group share a common TEK**. Anyone
who successfully decrypts a group-encrypted message knows it came from
another member of the group — because no-one else has the key. This
is implicit authentication via shared-secret.

**Limitation.** No protection against one group member impersonating
another. Useful only at the group-membership granularity.

### 5.2 Individual Source Authentication

Available when a **unique TEK or KEK is shared between exactly two
parties**. Successful decryption proves one of the two — by
elimination, the sender on the other end.

**Use case.** SU-to-FNE management traffic with per-SU pairwise keys.

### 5.3 Challenge-Response (Link Layer Authentication)

Defined normatively in **AACE-A**. Used for authenticating **clear
traffic** on trunked systems (where the group-TEK trick isn't
available because there's no encryption):

- FNE sends `RAND1` (random challenge) + `RS` (random seed).
- SU derives `KS` (session key) from the shared K + RS, computes
  `RES1` = f(KS, RAND1), returns `RES1`.
- FNE computes the expected `RES1` and compares.

**Mutual variant:** SU then sends its own `RAND2`, FNE returns
`RES2` using `KS'`.

**Where LLA runs:** on the **Trunking Control Channel only** (not
conventional; not working-channel voice). See AACE-A impl spec for
the wire messages.

**Authentication material provisioning:** K/SUID pairs, RS, KS, KS'
are **provisioned via wireline Key Fill only** — there is **no OTAR
path** for LLA keys (AAAB-B §7.2.1).

### 5.4 Table 1 — Auth Service Availability

AAAB-B Table 1 maps auth service availability to message type and
system type:

| Message type | Trunked | Conventional |
|--------------|---------|--------------|
| Clear voice / data | LLA (Challenge-Response on CCh) | (not generally available) |
| Encrypted voice / data (group TEK) | Group Source Auth | Group Source Auth |
| Encrypted voice / data (pairwise TEK/KEK) | Individual Source Auth | Individual Source Auth |
| KMM | Individual Source Auth (KEK-based) | Individual Source Auth |

---

## 6. Key Management

### 6.1 Lifecycle (§6)

Every key transits through these phases:

```
Generation → Distribution → Entry → Use → Storage ← (possibly) → Archiving
                                         ↓
                                     Destruction
```

### 6.2 Distribution Mechanisms

Per AAAB-B §6.1 / §6.2:

- **Physical (wireline) Key Fill** — via a **Key Fill Device (KFD)**
  connected to the SU over the KFD-MR interface. Normative spec:
  **AACD-A / AACD-B**.
- **Over-the-Air Rekeying (OTAR)** — via the **Key Management
  Facility (KMF)** over the CAI. Normative spec: **AACA-D**.
- **Inter-KMF** — KMF-to-KMF key distribution using S/MIME-protected
  IKI messages. Normative spec: **BAKA-A**.

### 6.3 Automated Key Management Procedures

AAAB-B §6.2 catalogs the procedures. All are defined wire-level in
AACA-D (OTAR):

- **Key download** — FNE pushes new keys to an SU.
- **Key activation** (keyset switching) — cutover to new keys.
- **Key destruction** (including full zeroize) — wipe key material.
- **Rekey request** — SU initiates a key update.
- **SU provisioning** — initial key install.
- **Audit queries** — KMF queries SU inventory.
- **Registration / deregistration** with the KMF.

### 6.4 Key Compromise (§6.3)

The spec is silent on specific procedures, but notes that **zeroize**
is the ultimate response. Operators maintain key-compromise procedures
outside the standard.

---

## 7. Key Management Architecture

AAAB-B §7 is the largest section. It provides architecture diagrams
for each security sub-domain. The wire formats live elsewhere; this
section answers **"which boxes exchange which messages?"**

### 7.1 User Voice and Data Encryption (§7.1)

Endpoints:
- **Key Manager Endpoints**: KMF, KFD.
- **Voice/Data Encryption Endpoints**: SUs.

Architectural patterns:
- **KFD ↔ SU wireline fill** — AACD-A/B interface protocol.
- **KMF ↔ KFD wireline fill** — KMF pushes to KFD via wireline.
- **KMF ↔ SU OTAR over CAI** — AACA-D.
- **Inter-KMF via S/MIME + IKI** — BAKA-A.
- **Combined** wireline + wireless configurations.
- **ISSI** delivery across inter-RFSS boundaries — BACA-B.

### 7.2 CAI Data Encryption (§7.1.3)

**Applies only to conventional (not trunked) LMR systems.** Data
Security Endpoints may be SU-to-FNE or SU-to-SU.

Concepts:
- **Grouping Profiles** — associate channels with algorithms and keys.
- **KMM wrapping** — inner-layer KEK wraps key material, outer-layer
  TEK protects the whole KMM during transit.

### 7.3 IP Data Encryption (§7.1.4) — **ON HOLD**

AAAB-B §7.1.4 is explicitly **flagged as incomplete** pending ongoing
P25 architectural discussions as of the 2019 publication.
Implementers should NOT rely on this subsection; when a
standards-based IP data confidentiality solution is needed, follow
AAAB-B's informative reference to **RFC 4301 (IPsec)** or equivalent
off-the-shelf IP-layer security.

### 7.4 Link Layer Authentication (§7.2.1)

Endpoints:
- **SU LLA** (subscriber side).
- **FNE LLA** (network side).

Wire protocol: **AACE-A** (unit auth + mutual auth challenge-response,
summarized in §5.3 above).

**Auth material path:**
- Provisioned via **wireline KFD fill only** (no OTAR path).
- **AF-to-FNE interface** is explicitly **non-standardized** — how
  the Authentication Facility delivers material to the FNE is
  operator/vendor-specific.
- **Roaming**: when an SU roams to a visited RFSS, the visited RFSS
  retrieves session material from the home FNE via **ISSI**
  (see BACA-B).

### 7.5 Air Interface Encryption / Link Layer Encryption (§7.2.2) — **ON HOLD**

Same status as §7.1.4. **Incomplete** at publication. This would
encrypt signaling + identity fields at the link layer (countering
traffic analysis threat #6). Not normatively defined — implementers
needing link-layer confidentiality must wait for a future revision.

---

## 8. Incomplete Sections (On Hold in 2019)

Two sub-sections of AAAB-B are explicitly non-normative at the 2019
publication:

| Section | Status | What's missing |
|---------|--------|----------------|
| **§7.1.4 IP Data Encryption** | On hold | No standardized architecture for encrypting IPv4 payload beyond the CAI encryption. Informative reference to RFC 4301 only. |
| **§7.2.2 Air Interface Encryption / Link Layer Encryption** | On hold | No standard for encrypting the signaling layer or for periodic MI broadcast on the control channel. |

**Operational consequence.** Any deployment claiming "link-layer
encryption" or "air-interface encryption" beyond AAAD-B traffic
encryption is doing so outside the 2019 standard — vendor-specific,
with no interop guarantee across manufacturers.

---

## 9. Deprecated Algorithms

AAAB-B §1.4 / §3 acknowledges DES and 3DES as **deprecated throughout
the TIA-102 suite**. They are retained only for **legacy
interoperability** with older deployments. New implementations must
not use DES for new keys.

**Decoder policy recommendation:** log ALGID values mapping to DES /
3DES as `deprecated` in output metadata. Alert operators if a modern
capture shows them — it typically indicates a mis-provisioned or
legacy-configured SU.

**Authoritative algorithm identifiers:** see AAAD-B and BAAC-D for
ALGID byte values. BAAC-D's ALGID table is the canonical reference.

---

## 10. Cross-Reference Map

AAAB-B's practical value is as a router. This table maps
overview-level security questions to the normative doc that answers
them.

| Question | Normative doc | Impl spec in this repo |
|----------|---------------|------------------------|
| How is voice/data traffic encrypted (block cipher, MI, ESS)? | **AAAD-B** (Block Encryption Protocol) | `standards/TIA-102.AAAD-B/P25_Block_Encryption_Protocol_Implementation_Spec.md` |
| What's the OTAR wire protocol for KMM exchange? | **AACA-D** | `standards/TIA-102.AACA-D/…` (processed; see specs.toml) |
| How does a KFD talk to an SU for wireline key fill? | **AACD-A** (and **AACD-B** draft) | pending — see specs.toml |
| What's the Link Layer Authentication challenge-response protocol? | **AACE-A** | `standards/TIA-102.AACE-A/P25_Link_Layer_Authentication_Implementation_Spec.md` |
| How do two KMFs exchange keys (inter-agency)? | **BAKA-A** | `standards/TIA-102.BAKA-A/…` (processed) |
| What ALGID / KID values are legal? | **BAAC-D** reserved values | `standards/TIA-102.BAAC-D/P25_Reserved_Values_Implementation_Spec.md` |
| How is encryption carried across ISSI (inter-RFSS)? | **BACA-B** and BACA-B-3 | pending (see specs.toml) |

**For a first-time security implementer:** read AAAB-B (this overview)
first. Then pick the sub-domain most urgent to your work and read its
normative spec. Don't try to implement security from just the
overview — every wire detail is elsewhere.

---

## 11. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- Scope, revision history, and abbreviations — TIA-102.AAAB-B §1.1–§1.5.
- Normative companion doc references — §1.6.
- Nine threats — §2.1–§2.9.
- Confidentiality, encryption types, cipher modes — §3.
- MI and encryption sync — §3.1.
- Chronological integrity (sequence numbers, MI windows) — §4.1.
- Message integrity (CBC-MAC) — §4.2.
- Group Source / Individual Source / Challenge-Response auth — §5.
- Auth service availability table — §5, Table 1.
- Key management lifecycle — §6.
- Physical vs OTAR distribution — §6.1 / §6.2.
- Key compromise — §6.3.
- Voice / data encryption key management architecture — §7.1.
- CAI Data Encryption (conventional only) — §7.1.3.
- IP Data Encryption (ON HOLD) — §7.1.4.
- Link Layer Authentication architecture — §7.2.1.
- Air Interface / Link Layer Encryption (ON HOLD) — §7.2.2.

---

## 12. Cross-References

**Downstream spec impl specs (already in repo):**
- `standards/TIA-102.AAAD-B/P25_Block_Encryption_Protocol_Implementation_Spec.md`
  — the Block Encryption Protocol AAAB-B points at for traffic
  confidentiality.
- `standards/TIA-102.AACE-A/P25_Link_Layer_Authentication_Implementation_Spec.md`
  — AACE-A challenge-response wire protocol.
- `standards/TIA-102.AACA-D/…` — OTAR protocol (KMF ↔ SU over CAI).
- `standards/TIA-102.BAKA-A/…` — inter-KMF protocol.
- `standards/TIA-102.BAAC-D/P25_Reserved_Values_Implementation_Spec.md`
  — ALGID / KID / MFID reserved-value tables.

**Downstream spec impl specs (pending in specs.toml):**
- `standards/TIA-102.AACD-A/…` and `AACD-B/…` — KFD Interface Protocol
  (wireline key fill).
- `standards/TIA-102.BACA-B/…` + `BACA-B-3/…` — ISSI messages and
  procedures (cross-RFSS security material transport).

**Related P25 security documents outside TIA-102.AAAB scope:**
- **TIA-905 suite** — Phase 2 TDMA security services, designed to be
  interoperable with TIA-102 per AAAB-B §1.2. Not in this repo.
- **IETF RFC 4301** — IPsec Architecture; informative reference for IP
  Data Encryption (§7.1.4 on-hold fallback).

**Related analysis:**
- None yet for the security domain. If implementer work uncovers a
  gap, `analysis/` is the right home for a clarification note
  (per `analysis/README.md`).
