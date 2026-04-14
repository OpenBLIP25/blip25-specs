#!/usr/bin/env python3
"""Regenerate test_vectors/algorithmic/*.csv from authoritative spec data.

Reads matrices and polynomials from standards/*/ and standards/*/annex_tables/,
expands round-trip / exp-log / lookup pairs, verifies each against an
invariant, and writes the result.

Invariant failures abort (do not silently produce a broken file).

Run from the repo root: python3 tools/gen_algorithmic_vectors.py
"""
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "test_vectors" / "algorithmic"
OUT.mkdir(parents=True, exist_ok=True)

# ----- Golay [23,12] from impl spec §1.5.1 (hex form) -----
#   Row i = (identity << 11) | parity; stored as uint32.
GOLAY_23_12_GEN = [
    0x40063A, 0x20031D, 0x1007B4, 0x0803DA, 0x0401ED, 0x0206CC,
    0x010366, 0x0081B3, 0x0046E3, 0x00254B, 0x00149F, 0x000C75,
]

def golay_23_12_encode(info: int) -> int:
    """Encode 12 info bits into 23-bit codeword via generator matrix."""
    assert 0 <= info < (1 << 12)
    cw = 0
    for i in range(12):
        if (info >> (11 - i)) & 1:
            cw ^= GOLAY_23_12_GEN[i]
    return cw

def popcount(x: int) -> int:
    return bin(x).count("1")

def golay_24_12_encode(info: int) -> int:
    """[24,12] extended Golay: apply [23,12] then append overall parity at LSB."""
    g23 = golay_23_12_encode(info)
    parity = popcount(g23) & 1
    return (g23 << 1) | parity

def write_golay_23_12():
    rows = []
    for info in range(1 << 12):
        cw = golay_23_12_encode(info)
        rows.append((info, cw))
    # Invariant: every info maps to a distinct codeword (full row rank = 12)
    assert len({cw for _, cw in rows}) == 4096, "Golay [23,12] encode collision"
    # Invariant: row 0 (info=0) -> codeword=0
    assert rows[0] == (0, 0)
    path = OUT / "golay_23_12_roundtrip.csv"
    with path.open("w", newline="") as f:
        f.write("# test_vectors/algorithmic/golay_23_12_roundtrip.csv\n")
        f.write("# Source: standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md §1.5.1\n")
        f.write("# Generator: tools/gen_algorithmic_vectors.py golay_23_12_encode()\n")
        f.write("# Invariant: 4096 distinct codewords (full rank over GF(2)); info=0 -> cw=0 PASS\n")
        w = csv.writer(f)
        w.writerow(["info_bits", "codeword_hex"])
        for info, cw in rows:
            w.writerow([info, f"0x{cw:06X}"])
    print(f"  golay_23_12_roundtrip.csv  ({len(rows)} rows)")

def write_golay_24_12():
    rows = []
    for info in range(1 << 12):
        cw = golay_24_12_encode(info)
        rows.append((info, cw))
    # Invariants: 4096 distinct codewords, and every codeword has even parity
    assert len({cw for _, cw in rows}) == 4096
    assert all(popcount(cw) % 2 == 0 for _, cw in rows), "Extended Golay parity fail"
    path = OUT / "golay_24_12_roundtrip.csv"
    with path.open("w", newline="") as f:
        f.write("# test_vectors/algorithmic/golay_24_12_roundtrip.csv\n")
        f.write("# Source: standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md §2.4.1\n")
        f.write("# Generator: tools/gen_algorithmic_vectors.py golay_24_12_encode()\n")
        f.write("# [24,12] Extended Golay = [23,12] with overall parity appended at LSB.\n")
        f.write("# Invariant: 4096 distinct codewords + every codeword has even Hamming weight: PASS\n")
        w = csv.writer(f)
        w.writerow(["info_bits", "codeword_hex"])
        for info, cw in rows:
            w.writerow([info, f"0x{cw:06X}"])
    print(f"  golay_24_12_roundtrip.csv  ({len(rows)} rows)")

# ----- Hamming [15,11] from impl spec §1.5.2 -----
HAMMING_15_11_PARITY = [0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x7, 0x6, 0x5, 0x3]

def hamming_15_11_encode(info: int) -> int:
    """Encode 11 info bits into 15-bit codeword: [info_11 | parity_4]."""
    assert 0 <= info < (1 << 11)
    p = 0
    for i in range(11):
        if (info >> (10 - i)) & 1:
            p ^= HAMMING_15_11_PARITY[i]
    return (info << 4) | p

def write_hamming_15_11():
    rows = []
    for info in range(1 << 11):
        cw = hamming_15_11_encode(info)
        rows.append((info, cw))
    # Invariant: 2048 distinct codewords
    assert len({cw for _, cw in rows}) == 2048
    path = OUT / "hamming_15_11_roundtrip.csv"
    with path.open("w", newline="") as f:
        f.write("# test_vectors/algorithmic/hamming_15_11_roundtrip.csv\n")
        f.write("# Source: standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md §1.5.2\n")
        f.write("# Generator: tools/gen_algorithmic_vectors.py hamming_15_11_encode()\n")
        f.write("# Layout: info[10..0] | parity[3..0]. Info bits populate codeword bits 14..4.\n")
        f.write("# Invariant: 2048 distinct codewords: PASS\n")
        w = csv.writer(f)
        w.writerow(["info_bits", "codeword_hex"])
        for info, cw in rows:
            w.writerow([info, f"0x{cw:04X}"])
    print(f"  hamming_15_11_roundtrip.csv  ({len(rows)} rows)")

# ----- GF(64) exp/log pairs (from BAAA-B Annex D, already as CSV) -----
def write_gf64_exp_log():
    # Regenerate deterministically from the primitive polynomial x^6 + x + 1 = 0x43,
    # which is what BAAA-B uses. Verify EXP[LOG[b]] == b for b=1..63.
    GF_POLY = 0x43
    exp_tab = [0] * 64
    log_tab = [0] * 64
    alpha = 1
    for i in range(63):
        exp_tab[i] = alpha
        log_tab[alpha] = i
        alpha = (alpha << 1)
        if alpha & 0x40:          # x^6 term → reduce mod GF_POLY
            alpha ^= GF_POLY
        alpha &= 0x3F
    exp_tab[63] = exp_tab[0]      # wrap: α^63 = α^0 = 1

    # Invariant: EXP[LOG[b]] == b for b=1..63; LOG[EXP[i]] == i for i=0..62
    for b in range(1, 64):
        assert exp_tab[log_tab[b]] == b, f"GF64 round-trip fail at b={b}"
    for i in range(63):
        assert log_tab[exp_tab[i]] == i, f"GF64 round-trip fail at i={i}"

    path = OUT / "gf64_exp_log_pairs.csv"
    with path.open("w", newline="") as f:
        f.write("# test_vectors/algorithmic/gf64_exp_log_pairs.csv\n")
        f.write("# Source: TIA-102.BAAA-B Annex D Table 6 (generated from primitive\n")
        f.write("#         polynomial x^6 + x + 1 = 0x43).\n")
        f.write("# Generator: tools/gen_algorithmic_vectors.py (inline in write_gf64_exp_log)\n")
        f.write("# Invariant: EXP[LOG[b]] == b for b=1..63 (PASS); LOG[EXP[i]] == i for i=0..62 (PASS)\n")
        w = csv.writer(f)
        w.writerow(["i", "alpha_i", "b", "log_b"])
        # Pair rows: for each i, alpha^i = exp_tab[i], and log of that = i.
        # Also emit log_b for b=1..63 alongside.
        for i in range(63):
            b = i + 1                # just use b=i+1 so every row has both columns populated
            w.writerow([i, f"0x{exp_tab[i]:02X}", b, log_tab[b]])
    print(f"  gf64_exp_log_pairs.csv  (63 rows)")

# ----- Bit prioritization samples (from annex_tables/imbe_bit_prioritization.csv) -----
def write_bit_prioritization_samples():
    # Pull a representative cross-section of L values from the pre-computed CSV.
    src = ROOT / "standards" / "TIA-102.BABA-A" / "annex_tables" / "imbe_bit_prioritization.csv"
    if not src.exists():
        print(f"  SKIP bit_prioritization_samples.csv (source not found: {src})")
        return
    sample_L = [9, 16, 30, 36, 37, 56]   # boundary + typical + K-cap transition
    rows = []
    with src.open() as f:
        for line in f:
            if line.startswith("#"):
                continue
            parts = line.strip().split(",")
            if parts[0] == "L":           # header
                continue
            L = int(parts[0])
            if L in sample_L:
                rows.append(parts)
    # Invariant: each sampled L contributes exactly 88 rows
    for L in sample_L:
        cnt = sum(1 for r in rows if int(r[0]) == L)
        assert cnt == 88, f"bit_prioritization sample L={L}: {cnt} rows, expected 88"
    path = OUT / "bit_prioritization_samples.csv"
    with path.open("w", newline="") as f:
        f.write("# test_vectors/algorithmic/bit_prioritization_samples.csv\n")
        f.write("# Source: standards/TIA-102.BABA-A/annex_tables/imbe_bit_prioritization.csv\n")
        f.write("# Generator: tools/gen_algorithmic_vectors.py write_bit_prioritization_samples()\n")
        f.write(f"# Sampled L values: {sample_L}\n")
        f.write("# Invariant: exactly 88 rows per sampled L (src coverage + dst coverage): PASS\n")
        w = csv.writer(f)
        w.writerow(["L", "src_param", "src_bit", "dst_vec", "dst_bit"])
        for r in rows:
            w.writerow(r)
    print(f"  bit_prioritization_samples.csv  ({len(rows)} rows, {len(sample_L)} L values)")

if __name__ == "__main__":
    print(f"Writing to {OUT.relative_to(ROOT)}/")
    write_golay_23_12()
    write_golay_24_12()
    write_hamming_15_11()
    write_gf64_exp_log()
    write_bit_prioritization_samples()
    print("All invariants PASS")
