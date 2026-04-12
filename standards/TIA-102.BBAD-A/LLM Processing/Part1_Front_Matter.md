# TIA-102.BBAD-A

**TIA STANDARD**

## Project 25

## Two-Slot TDMA MAC Layer Messages

TIA-102.BBAD-A
(Revision of TIA-102.BBAD)

November 2019

TELECOMMUNICATIONS INDUSTRY ASSOCIATION
tiaonline.org

---

## Foreword

*(This foreword is not part of this document.)*

This document has been created in response to a request by the Project 25 Steering Committee as provided for in a Memorandum of Understanding (MOU) dated April 1992 and amended December 1993. The MOU states that the Project 25 Steering Committee shall devise a Common System Standard for digital public safety communications (the Standard) commonly referred to as Project 25 or P25, and that TIA, as agreed to by the membership of the appropriate TIA Engineering Committee, will provide technical assistance in the development of documentation for the Standard. If the abbreviation "P25" or the wording "Project 25" appears on the cover sheet of this document when published that indicates the Project 25 Steering Committee has adopted this document as part of the Standard. The appearance of the abbreviation "P25" or the wording "Project 25" on the cover sheet of this document or in the title of any document referenced herein should not be interpreted as limiting the applicability of the information contained in any document to "P25" or "Project 25" implementations exclusively.

This document was developed by the TIA TR-8.12 Two Slot TDMA Subcommittee and specifies the Two-Slot TDMA MAC Layer Messages which is a component of the Two-Slot TDMA document suite that defines a common air interface for trunked services using a two-slot TDMA modulation format within a 12.5 kHz bandwidth radio channel for TIA-102 Land Mobile Radio systems. This document describes the protocol data units and message formats for the various logical channels of the Two-Slot TDMA common air interface.

The scope of the 2nd edition (Revision A) of this document was revised to only include the Two-Slot TDMA MAC Layer protocol data units and messages. The Two-Slot TDMA MAC layer bursts, coding, and procedures are now specified in separate documents published in parallel with this document. This document cancels and replaces TIA-102.BBAD.

There is one annex in this document. Annex A is informative and is not considered part of this document.

TIA does not evaluate, test, verify or investigate the information, accuracy, soundness, or credibility of the contents of the Document. In Publishing the Document, TIA disclaims any undertaking to perform any duty owed to or for anyone. Some aspects of the specifications contained in this document may not have been fully operationally tested; however, a great deal of time and good faith effort has been invested in the preparation of this document to ensure the accuracy of the information it contains.

---

## Patent Identification

The reader's attention is called to the possibility that compliance with this document may require the use of one or more inventions covered by patent rights.

By publication of this document, no position is taken with respect to the validity of those claims or any patent rights in connection therewith. The patent holders so far identified have, we believe, filed statements of willingness to grant licenses under those rights on reasonable and nondiscriminatory terms and conditions to applicants desiring to obtain such licenses.

The following patent holders and patents have been identified in accordance with the TIA intellectual property rights policy:

- No patents have been identified

TIA shall not be responsible for identifying patents for which licenses may be required by this document or for conducting inquiries into the legal validity or scope of those patents that are brought to its attention.

---

## Table of Contents

### 1. Introduction — 1
- 1.1 Scope — 1
- 1.2 References — 1
  - 1.2.1 Normative References — 1
  - 1.2.2 Informative References — 1
- 1.3 Acronyms and Abbreviations — 2
- 1.4 Definitions — 3
- 1.5 Conventions — 3
  - 1.5.1 Message Bit Numbering — 3
  - 1.5.2 PDU and Message Formats — 3
  - 1.5.3 Symbols — 4

### 2. MAC PDUs — 5
- 2.1 MAC PDU Structure — 5
- 2.2 LCCH PDUs — 6
  - 2.2.1 Outbound LCCH PDU — 6
  - 2.2.2 Inbound LCCH PDU — 7
  - 2.2.3 LCCH PDU Definitions — 7
- 2.3 VCH PDUs — 11
  - 2.3.1 Outbound VCH PDU — 11
  - 2.3.2 Inbound VCH PDU — 12
  - 2.3.3 VCH PDU Definitions — 12
    - 2.3.3.1 VCH MAC_PTT PDU — 12
    - 2.3.3.2 VCH MAC_END_PTT PDU — 14
    - 2.3.3.3 VCH MAC_IDLE, MAC_ACTIVE, & MAC_HANGTIME PDU — 15

### 3. MAC Messages — 19
- 3.1 Null Information — 23
- 3.2 Group Voice Channel User — 23
- 3.3 Unit to Unit Voice Channel User — 24
- 3.4 Telephone Interconnect Voice Channel User — 25
- 3.5 Unit to Unit Voice Service Request — 25
- 3.6 Group Voice Channel Grant Update Multiple — 26
- 3.7 Group Voice Channel Grant — 28
- 3.8 Group Voice Channel Grant Update — 28
- 3.9 Unit to Unit Voice Service Channel Grant — 29
- 3.10 Unit to Unit Answer Request — 31
- 3.11 Radio Unit Monitor Enhanced Command — 32
- 3.12 Telephone Interconnect Answer Request — 34
- 3.13 Unit to Unit Voice Channel Grant Update — 35
- 3.14 Acknowledge Response FNE — 37
- 3.15 SNDCP Data Channel Grant — 38
- 3.16 SNDCP Data Page Request — 39
- 3.17 SNDCP Data Channel Announcement - Explicit — 39
- 3.18 Adjacent Status Broadcast — 40
- 3.19 Call Alert — 41
- 3.20 Extended Function Command — 43
- 3.21 Group Affiliation Query — 44
- 3.22 Identifier Update — 45
- 3.23 Time and Date Announcement — 46
- 3.24 Network Status Broadcast — 46
- 3.25 Group Voice Service Request — 47
- 3.26 RFSS Status Broadcast — 47
- 3.27 Secondary Control Channel Broadcast — 48
- 3.28 Status Query — 49
- 3.29 Queued Response — 51
- 3.30 Deny Response — 51
- 3.31 System Service Broadcast — 52
- 3.32 Unit Registration Command — 52
- 3.33 Radio Unit Monitor Command - Obsolete — 53
- 3.34 Identifier Update for VHF/UHF Bands — 53
- 3.35 Identifier Update for TDMA — 54
- 3.36 Manufacturer Specific — 55
- 3.37 Individual Paging with Priority — 56
- 3.38 Indirect Group Paging without Priority — 57
- 3.39 Power Control Signal Quality — 57
- 3.40 MAC_Release — 58
- 3.41 Status Update — 58
- 3.42 Message Update — 60
- 3.43 Radio Unit Monitor Command — 62
- 3.44 Telephone Interconnect Voice Channel Grant — 64
- 3.45 Telephone Interconnect Voice Channel Grant Update — 65
- 3.46 Group Affiliation Response — 66
- 3.47 Unit Registration Response — 67
- 3.48 Unit Deregistration Acknowledge — 68
- 3.49 Location Registration Response — 69
- 3.50 Roaming Address Command — 69
- 3.51 Roaming Address Update — 70
- 3.52 Authentication Demand — 70
- 3.53 Authentication FNE Response — 72
- 3.54 Synchronization Broadcast — 73
- 3.55 Unit to Unit Voice Service Answer Response — 73
- 3.56 Telephone Interconnect Request – Explicit Dialing — 74
- 3.57 Telephone Interconnect Request – Implicit Dialing — 76
- 3.58 Telephone Interconnect Answer Response — 77
- 3.59 SNDCP Data Channel Request — 77
- 3.60 SNDCP Data Page Response — 77
- 3.61 SNDCP Reconnect Request — 78
- 3.62 Acknowledge Response Unit — 78
- 3.63 Call Alert Request — 79
- 3.64 Cancel Service Request — 80
- 3.65 Emergency Alarm Request — 81
- 3.66 Extended Function Response — 82
- 3.67 Group Affiliation Query Response — 83
- 3.68 Group Affiliation Request — 83
- 3.69 Identifier Update Request — 84
- 3.70 Message Update Request — 85
- 3.71 Status Query Request — 86
- 3.72 Status Query Response — 87
- 3.73 Status Update Request — 88
- 3.74 Unit Registration Request — 89
- 3.75 Unit Deregistration Request — 89
- 3.76 Location Registration Request — 90
- 3.77 Radio Unit Monitor Request — 91
- 3.78 Roaming Address Request — 92
- 3.79 Roaming Address Response — 93
- 3.80 Authentication FNE Result — 96
- 3.81 Authentication Response — 96
- 3.82 Authentication Response Mutual — 98
- 3.83 Authentication SU Demand — 99
- 3.84 Radio Unit Monitor Enhanced Request — 100
- 3.85 Null Avoid Zero Bias Information — 102
- 3.86 MFID90 Group Regroup Voice Channel User — 103
- 3.87 MFID90 Group Regroup Voice Channel Update — 104
- 3.88 MFID90 Extended Function Command — 104
- 3.89 MFID90 Group Regroup Voice Request — 105
- 3.90 MFID90 Extended Function Response — 105
- 3.91 MFIDA4 Group Regroup Explicit Encryption Command — 106
- 3.92 MFID90 Group Regroup Add Command — 108
- 3.93 MFID90 Group Regroup Delete Command — 108
- 3.94 MFID90 Group Regroup Channel Grant — 109
- 3.95 MFID90 Group Regroup Channel Update — 110
- 3.96 MFID90 Queued Response — 110
- 3.97 MFID90 Deny Response — 111
- 3.98 MFID90 Acknowledge Response — 111

### 4. MAC Fields — 113
- 4.1 Opcode — 113
- 4.2 Offset — 113
- 4.3 Protected (P) — 114
- 4.4 RF Level — 114
- 4.5 BER — 115
- 4.6 CRC-12 — 116
- 4.7 CRC-16 — 117
- 4.8 Unforced/Forced (U/F) — 120
- 4.9 Call Preemption/Audio (C/A) — 120
- 4.10 Color Code — 120
- 4.11 Length — 120
- 4.12 Data Length — 121
- 4.13 Subscriber Unit Identification (SUID) — 121
- 4.14 Subscriber Group Identification (SGID) — 121
- 4.15 Avoid Zero Bias — 121
- 4.16 Service Type — 121
- 4.17 Other Fields from FDMA — 122

### Annex A — MAC Message Mapping (Informative) — 125
- A.1 Inbound MAC Message Mapping — 125
- A.2 Outbound MAC Message Mapping — 126
- A.3 Inbound Dynamic Regrouping MAC Message Mapping — 128
- A.4 Outbound Dynamic Regrouping MAC Message Mapping — 128

---

## List of Figures

- Figure 1 – General MAC PDU Structure — 5
- Figure 2 – MAC PDU Header Format — 5
- Figure 3 – Outbound LCCH MAC PDU Trailer Format — 6
- Figure 4 – Inbound LCCH MAC PDU Trailer Format — 6
- Figure 5 – VCH MAC PDU Trailer Format — 6
- Figure 6 – OECI PDU Format — 7
- Figure 7 – IECI PDU Format — 7
- Figure 8 – General IECI MAC_SIGNAL PDU Format — 8
- Figure 9 – General OECI MAC_SIGNAL PDU Format — 8
- Figure 10 – I-OEMI PDU Format — 11
- Figure 11 – S-OEMI and IEMI PDU Format — 12
- Figure 12 – IEMI and S-OEMI MAC_PTT PDU Format — 13
- Figure 13 – I-OEMI MAC_PTT PDU Format — 13
- Figure 14 – IEMI and S-OEMI MAC_END_PTT PDU Format — 14
- Figure 15 – I-OEMI MAC_END_PTT PDU Format — 15
- Figure 16 – General I-OEMI MAC PDU Format — 16
- Figure 17 – General S-OEMI and IEMI MAC PDU Format — 16
- Figure 18 – Null Information — 23
- Figure 19 – Group Voice Channel User - Abbreviated — 23
- Figure 20 – Group Voice Channel User - Extended — 24
- Figure 21 – Unit to Unit Voice Channel User - Abbreviated — 24
- Figure 22 – Unit to Unit Voice Channel User - Extended — 25
- Figure 23 – Telephone Interconnect Voice Channel User — 25
- Figure 24 – Unit to Unit Voice Service Request - Abbreviated — 26
- Figure 25 – Unit to Unit Voice Service Request - Extended — 26
- Figure 26 – Group Voice Channel Update Multiple - Implicit — 27
- Figure 27 – Group Voice Channel Update Multiple - Explicit — 27
- Figure 28 – Group Voice Channel Grant - Implicit — 28
- Figure 29 – Group Voice Channel Grant - Explicit — 28
- Figure 30 – Group Voice Channel Grant Update - Implicit — 29
- Figure 31 – Group Voice Channel Grant Update - Explicit — 29
- Figure 32 – Unit to Unit Voice Service Channel Grant - Abbreviated — 30
- Figure 33 – Unit to Unit Voice Service Channel Grant - Extended VCH — 30
- Figure 34 – Unit to Unit Voice Service Channel Grant - Extended LCCH (1 of 2) — 31
- Figure 35 – Unit to Unit Voice Service Channel Grant - Extended LCCH (2 of 2) — 31
- Figure 36 – Unit to Unit Answer Request - Abbreviated — 32
- Figure 37 – Unit to Unit Answer Request - Extended — 32
- Figure 38 – Radio Unit Monitor Enhanced Command - Abbreviated — 33
- Figure 39 – Radio Unit Monitor Enhanced Command - Extended (1 of 3) — 33
- Figure 40 – Radio Unit Monitor Enhanced Command - Extended (2 of 3) — 34
- Figure 41 – Radio Unit Monitor Enhanced Command - Extended (3 of 3) — 34
- Figure 42 – Telephone Interconnect Answer Request — 35
- Figure 43 – Unit to Unit Voice Channel Grant Update - Abbreviated — 35
- Figure 44 – Unit to Unit Voice Channel Grant Update - Extended VCH — 36
- Figure 45 – Unit to Unit Voice Channel Grant Update - Extended LCCH (1 of 2) — 36
- Figure 46 – Unit to Unit Voice Channel Grant Update - Extended LCCH (2 of 2) — 37
- Figure 47 – Acknowledge Response FNE - Abbreviated — 37
- Figure 48 – Acknowledge Response FNE - Extended (1 of 2) — 38
- Figure 49 – Acknowledge Response FNE - Extended (2 of 2) — 38
- Figure 50 – SNDCP Data Channel Grant — 39
- Figure 51 – SNDCP Data Page Request — 39
- Figure 52 – SNDCP Data Channel Announcement - Explicit — 40
- Figure 53 – Adjacent Status Broadcast - Implicit — 40
- Figure 54 – Adjacent Status Broadcast - Explicit — 41
- Figure 55 – Adjacent Status Broadcast - Extended - Explicit — 41
- Figure 56 – Call Alert - Abbreviated — 42
- Figure 57 – Call Alert - Extended VCH — 42
- Figure 58 – Call Alert - Extended LCCH (1 of 2) — 42
- Figure 59 – Call Alert - Extended LCCH (2 of 2) — 43
- Figure 60 – Extended Function Command - Abbreviated — 43
- Figure 61 – Extended Function Command - Extended VCH — 44
- Figure 62 – Extended Function Command - Extended LCCH — 44
- Figure 63 – Group Affiliation Query - Abbreviated — 45
- Figure 64 – Group Affiliation Query - Extended — 45
- Figure 65 – Identifier Update — 45
- Figure 66 – Time and Date Announcement — 46
- Figure 67 – Network Status Broadcast - Implicit — 46
- Figure 68 – Network Status Broadcast - Explicit — 47
- Figure 69 – Group Voice Service Request — 47
- Figure 70 – RFSS Status Broadcast - Implicit — 48
- Figure 71 – RFSS Status Broadcast - Explicit — 48
- Figure 72 – Secondary Control Channel Broadcast - Implicit — 49
- Figure 73 – Secondary Control Channel Broadcast - Explicit — 49
- Figure 74 – Status Query - Abbreviated — 50
- Figure 75 – Status Query - Extended VCH — 50
- Figure 76 – Status Query - Extended LCCH (1 of 2) — 50
- Figure 77 – Status Query - Extended LCCH (2 of 2) — 51
- Figure 78 – Queued Response — 51
- Figure 79 – Deny Response — 52
- Figure 80 – System Service Broadcast — 52
- Figure 81 – Unit Registration Command - Abbreviated — 53
- Figure 82 – Radio Unit Monitor Command - Obsolete — 53
- Figure 83 – Identifier Update for VHF/UHF Bands — 54
- Figure 84 – Identifier Update for TDMA - Abbreviated — 54
- Figure 85 – Identifier Update for TDMA - Extended — 55
- Figure 86 – Manufacturer Specific — 55
- Figure 87 – Individual Paging with Priority — 56
- Figure 88 – Indirect Group Paging without Priority — 57
- Figure 89 – Power Control Signal Quality — 58
- Figure 90 – MAC Release — 58
- Figure 91 – Status Update - Abbreviated — 59
- Figure 92 – Status Update - Extended VCH — 59
- Figure 93 – Status Update - Extended LCCH (1 of 2) — 60
- Figure 94 – Status Update - Extended LCCH (2 of 2) — 60
- Figure 95 – Message Update - Abbreviated — 61
- Figure 96 – Message Update - Extended VCH — 61
- Figure 97 – Message Update - Extended LCCH (1 of 2) — 62
- Figure 98 – Message Update - Extended LCCH (2 of 2) — 62
- Figure 99 – Radio Unit Monitor Command - Abbreviated — 63
- Figure 100 – Radio Unit Monitor Command - Extended VCH — 63
- Figure 101 – Radio Unit Monitor Command - Extended LCCH (1 of 2) — 64
- Figure 102 – Radio Unit Monitor Command - Extended LCCH (2 of 2) — 64
- Figure 103 – Telephone Interconnect Voice Channel Grant - Implicit — 65
- Figure 104 – Telephone Interconnect Voice Channel Grant - Explicit — 65
- Figure 105 – Telephone Interconnect Voice Channel Grant Update - Implicit — 66
- Figure 106 – Telephone Interconnect Voice Channel Grant Update - Explicit — 66
- Figure 107 – Group Affiliation Response - Abbreviated — 67
- Figure 108 – Group Affiliation Response - Extended — 67
- Figure 109 – Unit Registration Response - Abbreviated — 68
- Figure 110 – Unit Registration Response - Extended — 68
- Figure 111 – Unit Deregistration Acknowledge — 69
- Figure 112 – Location Registration Response — 69
- Figure 113 – Roaming Address Command — 70
- Figure 114 – Roaming Address Update — 70
- Figure 115 – Authentication Demand (1 of 2) — 71
- Figure 116 – Authentication Demand (2 of 2) — 71
- Figure 117 – Authentication FNE Response - Abbreviated — 72
- Figure 118 – Authentication FNE Response - Extended — 72
- Figure 119 – TDMA Superframe Relationship to Time Micro-slot — 73
- Figure 120 – Synchronization Broadcast — 73
- Figure 121 – Unit to Unit Voice Service Answer Response - Abbreviated — 74
- Figure 122 – Unit to Unit Voice Service Answer Response - Extended — 74
- Figure 123 – Telephone Interconnect Request – Explicit Dialing (1 of 3) — 75
- Figure 124 – Telephone Interconnect Request – Explicit Dialing (2 of 3) — 76
- Figure 125 – Telephone Interconnect Request – Explicit Dialing (3 of 3) — 76
- Figure 126 – Telephone Interconnect Request – Implicit Dialing — 76
- Figure 127 – Telephone Interconnect Answer Response — 77
- Figure 128 – SNDCP Data Channel Request — 77
- Figure 129 – SNDCP Data Page Response — 78
- Figure 130 – SNDCP Reconnect Request — 78
- Figure 131 – Acknowledge Response Unit - Abbreviated — 79
- Figure 132 – Acknowledge Response Unit - Extended — 79
- Figure 133 – Call Alert Request - Abbreviated — 80
- Figure 134 – Call Alert Request - Extended — 80
- Figure 135 – Cancel Service Request - Abbreviated — 81
- Figure 136 – Cancel Service Request - Extended — 81
- Figure 137 – Emergency Alarm Request — 82
- Figure 138 – Extended Function Response - Abbreviated — 82
- Figure 139 – Extended Function Response - Extended — 83
- Figure 140 – Group Affiliation Query Response — 83
- Figure 141 – Group Affiliation Request - Abbreviated — 84
- Figure 142 – Group Affiliation Request - Extended — 84
- Figure 143 – Identifier Update Request — 85
- Figure 144 – Message Update Request - Abbreviated — 85
- Figure 145 – Message Update Request - Extended — 86
- Figure 146 – Status Query Request - Abbreviated — 86
- Figure 147 – Status Query Request - Extended — 87
- Figure 148 – Status Query Response - Abbreviated — 87
- Figure 149 – Status Query Response - Extended — 88
- Figure 150 – Status Update Request - Abbreviated — 88
- Figure 151 – Status Update Request - Extended — 89
- Figure 152 – Unit Registration Request — 89
- Figure 153 – Unit Deregistration Request — 90
- Figure 154 – Location Registration Request - Abbreviated — 90
- Figure 155 – Location Registration Request – Extended (1 of 2) — 91
- Figure 156 – Location Registration Request – Extended (2 of 2) — 91
- Figure 157 – Radio Unit Monitor Request - Abbreviated — 92
- Figure 158 – Radio Unit Monitor Request - Extended — 92
- Figure 159 – Roaming Address Request - Abbreviated — 93
- Figure 160 – Roaming Address Request - Extended — 93
- Figure 161 – Roaming Address Response (1) — 94
- Figure 162 – Roaming Address Response (Multiple Stack Entries) — 95
- Figure 163 – Roaming Address Response (Multi-Fragment CRC-16 Only) — 95
- Figure 164 – Authentication FNE Result - Abbreviated — 96
- Figure 165 – Authentication FNE Result - Extended — 96
- Figure 166 – Authentication Response - Abbreviated — 97
- Figure 167 – Authentication Response - Extended (1 of 2) — 97
- Figure 168 – Authentication Response - Extended (2 of 2) — 98
- Figure 169 – Authentication Response Mutual (1 of 2) — 98
- Figure 170 – Authentication Response Mutual (2 of 2) — 99
- Figure 171 – Authentication SU Demand - Abbreviated — 99
- Figure 172 – Authentication SU Demand - Extended — 100
- Figure 173 – Radio Unit Monitor Enhanced Request - Abbreviated (1 of 2) — 101
- Figure 174 – Radio Unit Monitor Enhanced Request - Abbreviated (2 of 2) — 101
- Figure 175 – Radio Unit Monitor Enhanced Request - Extended (1 of 2) — 102
- Figure 176 – Radio Unit Monitor Enhanced Request - Extended (2 of 2) — 102
- Figure 177 – Null Avoid Zero Bias Information — 103
- Figure 178 – MFID90 Group Regroup Voice Channel User - Abbreviated — 103
- Figure 179 – MFID90 Group Regroup Voice Channel User - Extended — 104
- Figure 180 – MFID90 Group Regroup Voice Channel Update — 104
- Figure 181 – MFID90 Extended Function Command — 105
- Figure 182 – MFID90 Group Regroup Voice Request — 105
- Figure 183 – MFID90 Extended Function Response — 106
- Figure 184 – MFIDA4 Group Regroup Explicit Encryption Command - WUID — 107
- Figure 185 – MFIDA4 Group Regroup Explicit Encryption Command - WGID — 107
- Figure 186 – MFID90 Group Regroup Add Command — 108
- Figure 187 – MFID90 Group Regroup Delete Command — 109
- Figure 188 – MFID90 Group Regroup Channel Grant – Implicit — 109
- Figure 189 – MFID90 Group Regroup Channel Grant – Explicit — 110
- Figure 190 – MFID90 Group Regroup Channel Update — 110
- Figure 191 – MFID90 Queued Response — 111
- Figure 192 – MFID90 Deny Response — 111
- Figure 193 – MFID90 Acknowledge Response — 112
- Figure 194 – Offset Field Usage — 114
- Figure 195 – Example Outbound Fragment 1 of 2 — 118
- Figure 196 – Example Outbound Fragment 2 of 2 — 119
- Figure 197 – General SUID Field Format — 121
- Figure 198 – General SGID Field Format — 121

---

## List of Tables

- Table 1 – LCCH MAC Messages — 9
- Table 2 – VCH MAC Messages — 17
- Table 3 – MCO Partitioning — 19
- Table 4 – MAC Message Lengths — 20
- Table 5 – PRIO Bit Definitions — 56
- Table 6 – Individual Paging Message Length — 57
- Table 7 – Indirect Group Paging Message Length — 57
- Table 8 – Roaming Address Response Length Fields — 95
- Table 9 – Opcode Values — 113
- Table 10 – Offset Values — 113
- Table 11 – RF Level Values — 115
- Table 12 – BER Values — 116
- Table 13 – FDMA Field References — 123
- Table 14 – ISP Opcode Summary — 125
- Table 15 – OSP Opcode Summary — 127
- Table 16 – Dynamic Regrouping ISP Opcode Summary — 128
- Table 17 – Dynamic Regrouping OSP Opcode Summary — 128

---

## Revision History

| Version | Date | Description |
|---------|------|-------------|
| Original | 08/02/2017 | Two-Slot TDMA Control Channel MAC Layer Specification; approved by the Project 25 Steering Committee June 22, 2017 as part of the Project 25 Standard |
| Revision A | 10/17/2019 | A revised Two-Slot TDMA document suite structure was agreed to during comment resolution discussions for the Two-Slot TDMA Control Channel MAC Layer Specification that concluded in January 2017 (see last extended proposed resolution in combined comment matrix TR812-16-012-R5). The title was changed to Two-Slot TDMA MAC Layer Messages, created appropriate front matter and introduction to replace sections 1 – 5, revised section 6 to allow future LLE content and future Data Channel PDUs and messages, and removed annexes no longer in scope. This document was adopted by the Project 25 Steering Committee October 10, 2019 as part of the Project 25 Standard. |
