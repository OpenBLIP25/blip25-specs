# P25 Phase 2 TDMA Physical Layer -- Implementation Specification

**Source:** TIA-102.BBAB (July 2009), TIA-102.BBAC-1 (February 2013), TIA-102.BBAC-A (November 2019)  
**Classification:** ALGORITHM  
**Extracted:** 2026-04-12  
**Purpose:** Self-contained spec for implementing P25 Phase 2 TDMA physical layer modulation,
burst structures, and timing. No reference to the original PDF required.

---

## 1. Fundamental Parameters

| Parameter | Value |
|-----------|-------|
| Channel bandwidth | 12.5 kHz |
| Bit rate | 12,000 bps |
| Symbol rate | 6,000 symbols/sec |
| Symbol period T | 166.667 us (1/6000) |
| Modulation order M | 4 (quaternary) |
| Bits per symbol | 2 (dibits) |
| Slot duration | 30 ms |
| Symbols per slot | 180 quaternary symbols = 360 bits |
| Uplink modulation | H-CPM (constant envelope) |
| Downlink modulation | H-DQPSK (linear) |

### 1.1 Bit-to-Symbol Mapping (Gray Code)

Used by both H-CPM and H-DQPSK. First bit in the frame is MSB (b1).

| Input bits [b1 b0] | Quaternary Symbol |
|---------------------|-------------------|
| `0b01` | +3 |
| `0b00` | +1 |
| `0b10` | -1 |
| `0b11` | -3 |

```rust
/// Map a dibit (2 bits, MSB first) to a quaternary symbol.
/// Input: bits in range 0..=3 where bit layout is [b1 b0].
const DIBIT_TO_SYMBOL: [i8; 4] = [
    1,   // 0b00 -> +1
    3,   // 0b01 -> +3
    -1,  // 0b10 -> -1
    -3,  // 0b11 -> -3
];

/// Inverse mapping: symbol to dibit.
fn symbol_to_dibit(sym: i8) -> u8 {
    match sym {
         3 => 0b01,
         1 => 0b00,
        -1 => 0b10,
        -3 => 0b11,
        _ => unreachable!(),
    }
}
```

**Rust note:** The symbol alphabet `{-3, -1, +1, +3}` maps naturally to `i8`. For DSP
pipelines, convert to `f32`/`f64` at modulator input.

---

## 2. H-DQPSK Modulation (Outbound / Downlink)

Base stations transmit using Harmonized Differential Quadrature Phase Shift Keyed modulation.
This is pi/4-DQPSK with a specific IQ pulse-shaping filter. Requires a linear power amplifier.

### 2.1 Differential Phase Encoding

The phase accumulates differentially:

```
phi(t, I) = (pi/4) * sum_{k <= n} I_k       for t <= n
```

At each symbol interval, the phase increment is `(pi/4) * I_k` where `I_k` is from
`{-3, -1, +1, +3}`. This yields phase steps of `{-3*pi/4, -pi/4, +pi/4, +3*pi/4}`.

The modulated baseband signal is:

```
s(t) = integral{ sqrt(E_s/T) * exp(j * phi(tau, I)) * h(t - tau) d_tau }
```

where `h(t)` is the IQ pulse-shaping filter.

```rust
/// Differential phase encoder state.
struct DqpskEncoder {
    /// Accumulated phase in radians.
    phase: f64,
}

impl DqpskEncoder {
    fn new() -> Self { Self { phase: 0.0 } }

    /// Encode one symbol, return (I, Q) of the unfiltered constellation point.
    fn encode(&mut self, symbol: i8) -> (f64, f64) {
        self.phase += (symbol as f64) * std::f64::consts::FRAC_PI_4;
        (self.phase.cos(), self.phase.sin())
    }
}
```

### 2.2 IQ Pulse-Shaping Filter

The filter `h(t)` uses a raised-cosine frequency response with rolloff alpha = 1
(full rolloff). The 6 dB (one-sided) bandwidth is 3.6 kHz.

```
H(f) = (1 + cos(pi * f / f_h)) / 2    for 0 <= f < f_h
H(f) = 0                                for f >= f_h

where:
    f_h = 2 * BW = 7200 Hz
    BW  = 3600 Hz
```

| Filter parameter | Value |
|------------------|-------|
| Rolloff alpha | 1.0 |
| 6 dB bandwidth BW | 3600 Hz |
| Cutoff frequency f_h | 7200 Hz |
| Symbol rate | 6000 symbols/sec |
| Samples per symbol (suggested) | 8..16 for implementation |

**Rust note:** Generate the filter impulse response by IFFT of `H(f)` or analytically. With
alpha=1 full rolloff, the time-domain pulse is a sinc(t/T)*cos(pi*t/T)/(1-(2t/T)^2) shape
truncated to a practical length (8-16 symbol spans). Apply as FIR filter to upsampled I/Q
streams independently.

### 2.3 H-DQPSK Demodulation

Differential phase detection:
1. Compute `delta_phi(n) = arg( r(n) * conj(r(n-1)) )` where `r(n)` are matched-filtered samples.
2. Map differential phase to soft-decision LLRs for the two bits.

The phase differences nominally fall at `{-3*pi/4, -pi/4, +pi/4, +3*pi/4}`.

LLR computation is implementation-specific. A common approach:
- `LLR_b1 = -K * sin(delta_phi)` (negative for b1=1 region)
- `LLR_b0 = -K * cos(delta_phi)` (negative for b0=1 region)
- `K` is a scaling/SNR-dependent constant.

**Cross-reference:** SDRTrunk implements H-DQPSK demodulation in
`P25P2DemodulatorInstrumented.java`. OP25 implements it in `p25p2_tdma.cc`.

---

## 3. H-CPM Modulation (Inbound / Uplink)

Subscriber units transmit using Harmonized Continuous Phase Modulation. Constant-envelope
modulation allowing Class C (nonlinear) power amplifiers in portable radios.

### 3.1 Core Parameters

| Parameter | Symbol | Value |
|-----------|--------|-------|
| Modulation index | h | 1/3 |
| Pulse shaping parameter | lambda | 0.75 |
| Pulse response length | L | 4 symbols |
| Normalization factor | G | 4.3455e-4 |
| Symbol alphabet | I_k | {-3, -1, +1, +3} |
| Symbol rate | | 6000 sym/s |

### 3.2 CPM Signal Generation

Baseband signal:

```
s(t) = sqrt(E_s / T) * exp(j * phi(t, I))
```

Phase signal:

```
phi(t, I) = 2*pi*h * sum_{k <= n} I_k * q(t - k*T)    for n*T < t <= (n+1)*T
```

The phase pulse `q(t)` is the integral of the frequency pulse `g(t)`:

```
q(t) = integral_0^t g(u) du

g(t) = (1/G) * sinc(lambda/T * (t - L*T/2)) * cos^2(pi/(L*T) * (t - L*T/2))
       for t in [0, L*T]

g(t) = 0  elsewhere
```

where `sinc(x) = sin(pi*x) / (pi*x)`.

Boundary conditions:
- `q(t) = 0` for `t < 0`
- `q(t) = 1/2` for `t >= L*T`

### 3.3 Phase Shifts per Symbol

With `h = 1/3`, the total phase shift per symbol `I_k` after the L-symbol transient is:

```
delta_phi_k = 2*pi*h * I_k * q(L*T) = 2*pi*(1/3) * I_k * (1/2) = pi/3 * I_k
```

| Symbol I_k | Phase shift (radians) | Phase shift (degrees) |
|------------|----------------------|----------------------|
| +3 | +pi (3.1416) | +180 |
| +1 | +pi/3 (1.0472) | +60 |
| -1 | -pi/3 (-1.0472) | -60 |
| -3 | -pi (-3.1416) | -180 |

### 3.4 Frequency Pulse Computation

```rust
use std::f64::consts::PI;

const H: f64 = 1.0 / 3.0;
const LAMBDA: f64 = 0.75;
const L: usize = 4;
const G: f64 = 4.3455e-4;
const SYMBOL_RATE: f64 = 6000.0;
const T: f64 = 1.0 / SYMBOL_RATE;  // 166.667 us
const LT: f64 = L as f64 * T;       // 4 * T

/// Compute frequency pulse g(t) at time t (seconds).
/// Returns 0.0 outside [0, L*T].
fn frequency_pulse(t: f64) -> f64 {
    if t < 0.0 || t > LT {
        return 0.0;
    }
    let centered = t - LT / 2.0;
    let sinc_arg = LAMBDA / T * centered;
    let sinc_val = if sinc_arg.abs() < 1e-12 {
        1.0
    } else {
        (PI * sinc_arg).sin() / (PI * sinc_arg)
    };
    let cos2_arg = PI / LT * centered;
    let cos2_val = cos2_arg.cos().powi(2);
    (1.0 / G) * sinc_val * cos2_val
}

/// Compute phase pulse q(t) by numerical integration of g(t).
/// Uses Simpson's rule with `n_steps` intervals.
fn phase_pulse(t: f64, n_steps: usize) -> f64 {
    if t <= 0.0 { return 0.0; }
    if t >= LT { return 0.5; }
    let dt = t / n_steps as f64;
    let mut sum = frequency_pulse(0.0) + frequency_pulse(t);
    for i in 1..n_steps {
        let ti = i as f64 * dt;
        let weight = if i % 2 == 0 { 2.0 } else { 4.0 };
        sum += weight * frequency_pulse(ti);
    }
    sum * dt / 3.0
}
```

**Rust note:** For real-time operation, precompute `q(t)` as a lookup table at the
desired oversampling rate (e.g., 48 kHz -> 8 samples/symbol). Interpolate for
fractional indices.

### 3.5 CPM Modulator (Phase Accumulation)

The CPM modulator maintains a trellis of L-1 = 3 previous symbols as correlative state
plus a cumulative phase state theta_n.

```rust
/// Simplified CPM modulator using precomputed phase pulse table.
struct HcpmModulator {
    /// Phase pulse LUT: q_lut[i] = q(i * dt) for i in 0..lut_len.
    q_lut: Vec<f64>,
    /// Samples per symbol period.
    samples_per_symbol: usize,
    /// Ring buffer of recent symbols (last L symbols).
    symbol_history: [i8; L],
    /// Current accumulated phase (radians).
    phase_accum: f64,
}
```

### 3.6 H-CPM Demodulation (MLSE Concept)

The standard specifies a Maximum Likelihood Sequence Estimator (MLSE):

1. **Matched filters:** For each trellis state `sigma_n = {theta_n, I_{n-1}, ..., I_{n-L+1}}`,
   compute branch metrics `|r(t) - c_k(t) * M_k(t)|^2` where `M_k(t)` are CPM waveform
   templates and `c_k(t)` is the channel estimate.

2. **Viterbi algorithm:** Find the minimum-cost path through the trellis.

3. **Soft-decision LLRs:** Output per-bit log-likelihood ratios for downstream FEC.

State space size with h=1/3, M=4, L=4:
- Phase states: `p = denominator(h * M) = 3` possible phase increments modulo 2*pi
- Correlative states: `M^(L-1) = 4^3 = 64`
- Total trellis states: `3 * 64 = 192` (manageable for Viterbi)

**Rust note:** The MLSE demodulator is the most computationally intensive component.
Consider:
- Precomputed branch metric tables for all 192 states x 4 transitions.
- Fixed-point LLR representation (`i16` with 3-4 fractional bits) for FEC input.
- The Viterbi path memory depth should be at least 5*L = 20 symbols for reliable
  traceback.

**Cross-reference:** OP25's `p25p2_tdma.cc` uses a simplified differential demodulator
rather than full MLSE for H-CPM. SDRTrunk's Phase 2 demodulator similarly uses
differential detection. A full MLSE implementation would improve sensitivity by
approximately 2-3 dB but at significant computational cost.

---

## 4. Burst Timing

### 4.1 Slot Dimensions

```
Slot duration:          30 ms
Symbols per slot:       180 quaternary symbols
Bits per slot:          360 bits
Symbol period:          166.667 us
Information payload:    160 symbols = 320 bits = 26.667 ms (center of slot)
Overhead per slot:      20 symbols = 40 bits = 3.333 ms (edges of slot)
```

### 4.2 Outbound Burst (Normal Burst -- Continuous Transmission)

The base station transmits continuously. ISCH fields occupy the 10-symbol boundaries
between consecutive slots.

```
+--------+---------------------+--------+
| ISCH   |    Information      |  ISCH  |
| 10 sym |    160 symbols      | 10 sym |
| 1.67ms |    26.67 ms         | 1.67ms |
| 20 bits|    320 bits         | 20 bits|
+--------+---------------------+--------+
|<------------- 30 ms ----------------->|
```

The 10-symbol ISCH at the end of slot N and the 10-symbol ISCH at the start of slot N+1
together form a contiguous 20-symbol (40-bit) ISCH block. Two ISCH types alternate:

- **S-ISCH:** Contains a 20-symbol sync sequence (no encoded payload; sync pattern only).
- **I-ISCH:** Contains 9 info bits encoded with a (40,9,16) binary coset code.

### 4.3 Inbound Burst (Normal Burst -- Bursty Transmission)

Subscriber units transmit in bursts, one slot at a time.

```
+------+------+-----------------+------+------+
| FILL | PILOT|   Information   | PILOT| FILL |
| 6sym | 4sym |   160 symbols   | 4sym | 6sym |
| 1ms  |0.67ms|   26.67 ms      |0.67ms| 1ms  |
+------+------+-----------------+------+------+
|<--------------- 30 ms ------------------>|
```

| Field | Symbols | Duration | Content |
|-------|---------|----------|---------|
| INTERSLOT_FILL1 | 6 | 1.0 ms | All zeros (ramp-up / guard) |
| PILOT_NB1 (P1) | 4 | 0.667 ms | `[+1, -1, +1, -1]` (no-sync bursts) |
| Information | 160 | 26.667 ms | Payload (voice/signaling) |
| PILOT_NB2 (P2) | 4 | 0.667 ms | `[-1, +1, -1, +1]` (all bursts) |
| INTERSLOT_FILL2 | 6 | 1.0 ms | All zeros (ramp-down / guard) |

```rust
/// Interslot fill: 6 zero-valued symbols fed to modulator during guard time.
const INTERSLOT_FILL: [i8; 6] = [0, 0, 0, 0, 0, 0];

/// Pilot sequence P1 (beginning of inbound no-sync bursts).
const PILOT_P1: [i8; 4] = [1, -1, 1, -1];

/// Pilot sequence P2 (end of all inbound bursts).
const PILOT_P2: [i8; 4] = [-1, 1, -1, 1];
```

**Note:** For inbound signaling bursts with sync, the first 4 symbols of the sync sequence
replace P1. P2 is always sent at the trailing edge.

---

## 5. Superframe and Ultraframe Structure

### 5.1 Superframe

A superframe consists of 12 consecutive 30 ms time slots:

```
Duration:       12 * 30 ms = 360 ms
Slots:          0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
Bits:           12 * 360 = 4320 bits per logical channel direction
```

LCH 0 and LCH 1 alternate on even/odd slots with an inversion at slots 10/11:

```
Slot:   0    1    2    3    4    5    6    7    8    9    10   11
LCH:    0    1    0    1    0    1    0    1    0    1    1*   0*
                                                         ^^   ^^
                                                    (inverted pair)
```

The inversion at slots 10/11 allows a transmitting SU to switch from TX to RX
(to hear the SACCH on the outbound) without a tight TX/RX turnaround.

One SACCH signaling slot occurs per superframe on the inverted timeslot for each LCH.

### 5.2 Slot Numbering within Superframe

Each LCH sees 6 slots per superframe. For LCH 0:

```
LCH 0 slots: 0, 2, 4, 6, 8, 11  (slot 11 is the inverted SACCH slot)
```

For LCH 1:

```
LCH 1 slots: 1, 3, 5, 7, 9, 10  (slot 10 is the inverted SACCH slot)
```

### 5.3 Voice Burst Sequence per Superframe

Within each superframe, the 6 slots carry a repeating voice pattern:

```
4V, 4V, 4V, 4V, 2V, SACCH
```

- **4V burst:** 4 voice frames (from half-rate AMBE vocoder)
- **2V burst:** 2 voice frames + ESS (Encryption Sync Signal)
- **SACCH:** Slow Associated Control Channel signaling

This yields 18 voice frames per 360 ms = one voice frame every 20 ms, matching the
AMBE half-rate vocoder frame rate.

### 5.4 Ultraframe

```
Duration:           4 superframes = 4 * 360 ms = 1440 ms = 1.44 s
Superframes:        SF0, SF1, SF2, SF3
Total slots:        48
Total bits/LCH:     4 * 4320 = 17,280 bits
```

The ultraframe governs SACCH usage on voice channels:
- **SF0, SF1, SF2:** Transmitting SU sends on inbound SACCH.
- **SF3:** Transmitting SU listens on outbound SACCH (receives VCU messages).

### 5.5 Time Alignment

All outbound TDMA channels at a site must be symbol-, burst-, superframe-, and
ultraframe-synchronized.

When coexisting with an FDMA control channel, optional sync via SYNC_BCST TSBK:

```
Superframe_mark  = (Minutes * 8000 + Micro_Slots) MOD 48
Ultraframe_mark  = (Minutes * 8000 + Micro_Slots) MOD 192
```

where Micro_Slots counts at 7.5 ms per tick (0-7999 per minute).
Every third minute boundary aligns both superframe and ultraframe starts.

```rust
/// Compute superframe index from time alignment parameters.
fn superframe_mark(minutes: u32, micro_slots: u32) -> u32 {
    (minutes * 8000 + micro_slots) % 48
}

/// Compute ultraframe index from time alignment parameters.
fn ultraframe_mark(minutes: u32, micro_slots: u32) -> u32 {
    (minutes * 8000 + micro_slots) % 192
}
```

---

## 6. Synchronization Sequences

From TIA-102.BBAC-1 Table 5-1 (corrected). All values are signed dibit symbols
(+3 or -3) in order of transmission (S(N) transmitted first for highest N).

### 6.1 Inbound IEMI Sync (22 symbols)

Used in inbound signaling bursts (FACCH/SACCH).

```rust
/// IEMI sync sequence, 22 symbols, transmission order [S(21)..S(0)].
const IEMI_SYNC: [i8; 22] = [
    3, 3, -3, -3, 3, 3, 3, -3,  // S(21)..S(14)
    3, -3, -3, 3, 3, 3, 3, -3,  // S(13)..S(6)
    3, -3, -3, -3, -3, -3,      // S(5)..S(0)
];
```

Hex encoding (mapping +3 -> 0, -3 -> 1, packed MSB-first into bytes, 22 bits):

```
Bit pattern: 0 0 1 1 0 0 0 1  0 1 1 0 0 0 0 1  0 1 1 1 1 1
Hex (padded to 24 bits): 0x31_61_7C
```

### 6.2 Outbound S-OEMI Sync (21 symbols)

Used in outbound signaling bursts (FACCH with sync).

```rust
/// S-OEMI sync sequence, 21 symbols, transmission order [S(20)..S(0)].
const S_OEMI_SYNC: [i8; 21] = [
    -3, -3, -3, -3, 3, 3, 3, 3,  // S(20)..S(13)
    3, -3, 3, -3, -3, -3, 3, 3,  // S(12)..S(5)
    -3, -3, -3, 3, 3,            // S(4)..S(0)
];
```

Hex encoding (+3->0, -3->1, 21 bits):

```
Bit pattern: 1 1 1 1 0 0 0 0  0 1 0 1 1 1 0 0  1 1 1 0 0
Hex (padded to 24 bits): 0xF0_5C_E0
```

### 6.3 VCH S-ISCH Sync (20 symbols)

Used in the S-ISCH on voice channels for late-entry synchronization.

```rust
/// VCH S-ISCH sync sequence, 20 symbols, transmission order [S(19)..S(0)].
const VCH_SISCH_SYNC: [i8; 20] = [
    3, 3, 3, -3, 3, 3, -3, 3,    // S(19)..S(12)
    3, 3, 3, -3, -3, -3, 3, -3,  // S(11)..S(4)
    -3, -3, -3, -3,              // S(3)..S(0)
];
```

Hex encoding (+3->0, -3->1, 20 bits):

```
Bit pattern: 0 0 0 1 0 0 1 0  0 0 0 1 1 1 0 1  1 1 1 1
Hex (padded to 20 bits): 0x1_2_1-DF -> 0x121DF   (five nibbles)
Hex (padded to 24 bits): 0x12_1D_F0
```

### 6.4 CCH S-ISCH Sync (20 symbols, Informative)

Used on control channels. Informative only -- not normative.

```rust
/// CCH S-ISCH sync sequence, 20 symbols (informative).
const CCH_SISCH_SYNC: [i8; 20] = [
    -3, -3, -3, -3, -3, 3, -3, -3,  // S(19)..S(12)
    -3, 3, 3, 3, 3, -3, 3, 3,       // S(11)..S(4)
    -3, 3, 3, 3,                     // S(3)..S(0)
];
```

Hex encoding (+3->0, -3->1, 20 bits):

```
Bit pattern: 1 1 1 1 1 0 1 1  1 0 0 0 0 1 0 0  1 0 0 0
Hex (padded to 24 bits): 0xFB_84_80
```

### 6.5 Complete Sync Table (Code-Ready)

```rust
/// All sync sequences as a single lookup.
/// Encoding: +3 and -3 only. No intermediate values.
pub mod sync {
    pub const IEMI: &[i8]       = &[ 3, 3,-3,-3, 3, 3, 3,-3, 3,-3,-3, 3, 3, 3, 3,-3, 3,-3,-3,-3,-3,-3];
    pub const S_OEMI: &[i8]     = &[-3,-3,-3,-3, 3, 3, 3, 3, 3,-3, 3,-3,-3,-3, 3, 3,-3,-3,-3, 3, 3];
    pub const VCH_SISCH: &[i8]  = &[ 3, 3, 3,-3, 3, 3,-3, 3, 3, 3, 3,-3,-3,-3, 3,-3,-3,-3,-3,-3];
    pub const CCH_SISCH: &[i8]  = &[-3,-3,-3,-3,-3, 3,-3,-3,-3, 3, 3, 3, 3,-3, 3, 3,-3, 3, 3, 3];
}
```

### 6.6 Pilot Sequences

```rust
/// Pilot P1: beginning of inbound no-sync bursts, 4 symbols.
pub const PILOT_P1: [i8; 4] = [1, -1, 1, -1];

/// Pilot P2: end of all inbound bursts, 4 symbols.
pub const PILOT_P2: [i8; 4] = [-1, 1, -1, 1];
```

**Note on inbound sync bursts:** For inbound signaling bursts with sync, the first 4 symbols
of the IEMI sync sequence (`[+3, +3, -3, -3]`) replace the P1 pilot position.

---

## 7. Power Ramp Profiles (Inbound / Subscriber)

### 7.1 Guard Time Budget

| Parameter | Value |
|-----------|-------|
| Total guard time per slot | 2.0 ms (1.0 ms each end) |
| Minimum propagation delay protection | 0.8 ms |
| Maximum ramp duration | 1.2 ms |
| Symbols during guard | 6 symbols per end = 1.0 ms |

### 7.2 Ramp Timing

The modulation burst is centered in the 30 ms slot. The 1 ms guard at each end is
divided between ramp and propagation protection:

```
Leading edge:   0.4 ms pre-ramp guard + up to 1.2 ms ramp-up
Trailing edge:  up to 1.2 ms ramp-down + 0.4 ms post-ramp guard
```

With maximum ramp length (1.2 ms), the burst slightly exceeds the 30 ms slot boundary.
Overlapping of ramps between adjacent slots is permitted provided the center 28 ms
of each slot (containing information, sync, and pilot) is not corrupted.

### 7.3 Ramp Profile

**MISSING FROM TIA-102.BBAB:** The standard shows ramps "generically" as linear in the
timing diagrams but does not specify a normative ramp shape (raised cosine, linear,
polynomial, etc.). The ramp shape is left to the implementer.

Common implementations use a raised-cosine ramp:

```rust
/// Raised-cosine power ramp over `n_samples` samples.
/// Returns amplitude scaling factor [0.0, 1.0].
fn ramp_up(sample: usize, n_samples: usize) -> f64 {
    let t = sample as f64 / n_samples as f64;
    0.5 * (1.0 - (std::f64::consts::PI * t).cos())
}

fn ramp_down(sample: usize, n_samples: usize) -> f64 {
    ramp_up(n_samples - sample, n_samples)
}
```

### 7.4 Ramp Implementation Constraints

- At 6000 sym/s and 1.2 ms max ramp: ramp spans approximately 7.2 symbol periods
  (but only 6 symbols are allocated as INTERSLOT_FILL zeros).
- The ramp may extend slightly into adjacent slot guard time (overlap allowed).
- At maximum propagation delay (0.8 ms) + maximum ramp (1.2 ms), the ramp-down of
  one burst reaches zero just as the information payload of the next burst begins.
- Subscriber units at maximum range (0.8 ms one-way = ~240 km) consume the full
  propagation guard budget.

**Cross-reference:** SDRTrunk does not implement transmit ramp profiles (receive-only).
OP25 similarly is receive-only and does not include ramp generation.

---

## 8. MLSE Demodulator Concept (H-CPM)

### 8.1 Architecture

```
r(t) --> [Matched Filter Bank] --> [Branch Metrics] --> [Viterbi MLSE] --> LLRs
              ^
              |
         [Channel Tracker]
```

### 8.2 State Space

A state `sigma_n` is the tuple `{theta_n, I_{n-1}, I_{n-2}, I_{n-3}}` where:
- `theta_n` = accumulated phase state (modulo the phase periodicity)
- `I_{n-1}, ..., I_{n-L+1}` = the L-1 = 3 most recent symbols (correlative state)

| Component | Values | Count |
|-----------|--------|-------|
| Phase states theta_n | Depends on h=1/3 and M=4 | 3 |
| Correlative state | Each I in {-3,-1,+1,+3} for L-1=3 symbols | 4^3 = 64 |
| **Total states** | | **192** |
| Transitions per state | 4 (one per possible new symbol) | |
| Total branches per symbol | 192 * 4 = 768 | |

### 8.3 Matched Filters

For each state-to-state transition, a reference waveform template is precomputed from the
CPM parameters. The branch metric for transition from state `sigma` with input symbol `I_k` is:

```
metric(sigma, I_k) = |r_n - c * M(sigma, I_k)|^2
```

where:
- `r_n` = vector of received samples over one symbol period
- `c` = channel estimate (complex scalar from channel tracker)
- `M(sigma, I_k)` = precomputed matched filter waveform for this state+symbol

### 8.4 Viterbi Algorithm

Standard add-compare-select (ACS) per trellis stage:
1. For each state, compute branch metrics for all 4 incoming transitions.
2. Add branch metric to survivor path metric.
3. Compare and select minimum (ML criterion).
4. Store survivor path and decision.

Traceback depth: at least 5*L = 20 symbols (typically 24-32 for margin).

### 8.5 LLR Output

Per-bit soft decisions computed from the Viterbi path metrics:

```
LLR(b) = metric(best path with b=0) - metric(best path with b=1)
```

Positive LLR indicates b=0 more likely; negative indicates b=1 more likely.
Magnitude indicates confidence.

```rust
/// LLR output type. Positive = bit is 0, negative = bit is 1.
/// Fixed-point: 4 fractional bits (multiply float LLR by 16 and round).
type Llr = i16;

const LLR_FRAC_BITS: u32 = 4;

fn float_to_llr(llr_float: f64) -> Llr {
    let scaled = llr_float * (1 << LLR_FRAC_BITS) as f64;
    scaled.round().clamp(i16::MIN as f64, i16::MAX as f64) as i16
}
```

**Cross-reference:** Both SDRTrunk and OP25 use simplified (non-MLSE) demodulators for
H-CPM, relying on differential detection followed by slicer decisions. A full MLSE
implementation as described here is more complex but provides the soft-decision LLRs
that the downstream Reed-Solomon errors-and-erasures decoder benefits from.

---

## 9. Slot Structure Diagrams

### 9.1 Outbound Slot Pair (Continuous)

Two consecutive outbound slots showing the ISCH straddling the boundary:

```
Slot N                                   Slot N+1
+--------+--------------------+----+----+--------------------+--------+
| ISCH   |    Information     |ISCH|ISCH|    Information     | ISCH   |
| 10 sym |    160 symbols     |10sy|10sy|    160 symbols     | 10 sym |
+--------+--------------------+----+----+--------------------+--------+
                               |<------>|
                            20-symbol ISCH block
                          (S-ISCH or I-ISCH, alternating)
```

The outbound is never bursty. When only one TDMA slot is in use, MAC Idle PDUs
fill the other slot.

### 9.2 Inbound Slot (Bursty)

```
         |<-- guard -->|<------ information ------>|<-- guard -->|
         |  1.0 ms     |        26.67 ms           |  1.0 ms     |
         |             |                            |             |
    _____|    ____     |                            |     ____    |_____
   |     |   /    \    |                            |    /    \   |     |
   | OFF | RAMP  PILOT |       160 symbols          | PILOT RAMP | OFF |
   |_____|/   UP  |    |                            |    | DOWN  \|_____|
         |  6 sym |4sym|                            |4sym| 6 sym  |
         |  FILL  | P1 |     payload (scrambled)    | P2 |  FILL  |
```

### 9.3 Superframe Layout (One LCH, Outbound View)

```
Slot:  0     2     4     6     8     11
       +-----+-----+-----+-----+-----+------+
       | 4V  | 4V  | 4V  | 4V  | 2V  | SACCH|
       +-----+-----+-----+-----+-----+------+
       |<------------ 360 ms ---------------->|
       |  30ms  30ms  30ms  30ms  30ms  30ms  |

Note: Slot numbers shown are for LCH 0. LCH 1 uses slots 1,3,5,7,9,10.
Slots 10/11 are the inverted pair (SACCH slots).
```

### 9.4 ISCH Placement Detail

The ISCH straddles every outbound slot boundary. Between any two adjacent outbound
slots, the 20-symbol ISCH block alternates between S-ISCH and I-ISCH types:

```
... |slot N-1| ISCH |slot N| ISCH |slot N+1| ...
              ^              ^
         I-ISCH (info)   S-ISCH (sync)    -- alternating
```

The S-ISCH enables late-entering subscriber units to acquire synchronization.
The I-ISCH carries 9 bits of decoded information:
- LCH type (VCH/LCCH)
- Channel number
- ISCH location within superframe
- Ultraframe count
- SACCH access control

---

## 10. ISCH Encoding

### 10.1 I-ISCH: (40,9,16) Binary Coset Code

The I-ISCH carries 9 information bits encoded into 40 bits (20 symbols) using a
(40,9,16) binary coset code with minimum distance 16.

**MISSING FROM TIA-102.BBAB:** The I-ISCH coset code generator matrix is defined in
TIA-102.BBAC-A (not in the physical layer spec). Implementers must obtain the encoding
matrix from the MAC layer specification.

### 10.2 S-ISCH: Sync Pattern Only

The S-ISCH contains only a sync pattern (no encoded information). The 20-symbol
pattern is one of:
- VCH S-ISCH Sync (voice channels) -- see Section 6.3
- CCH S-ISCH Sync (control channels) -- see Section 6.4

### 10.3 DUID (Data Unit ID)

Every burst carries a distributed 8-bit DUID encoded with an (8,4,4) binary code.
The 4 information bits identify the burst type and logical channel. DUID symbols
are placed at fixed positions within the burst (see Annex E tables in TIA-102.BBAC-1).

---

## 11. Summary of Missing/Deferred Parameters

The following parameters are referenced in TIA-102.BBAB but not fully specified there:

| Parameter | Where Defined | Status |
|-----------|--------------|--------|
| Pilot symbol sequences | TIA-102.BBAC (MAC layer) | Extracted in Section 6.6 above from BBAC-1 |
| Sync sequences | TIA-102.BBAC-1 Table 5-1 | Extracted in Section 6 above |
| I-ISCH coset code matrix | TIA-102.BBAC-A | Not extracted here; needed for ISCH decode |
| DUID (8,4,4) code | TIA-102.BBAC-A | Not extracted here; needed for burst identification |
| Ramp shape (normative) | Not specified | Implementation-defined; raised cosine recommended |
| LLR generation method | Not specified | Implementation-defined; conceptual MLSE described |
| H-D8PSK parameters | TIA-102.BBAB Annex A | Informative only, not normative |
| Scrambling LFSR and seeds | TIA-102.BBAC-1 Section 7.2.5 | Extracted separately in P25_TDMA_Scrambling_Implementation_Spec.md |

---

## 12. Cross-Reference: Open-Source Implementations

### SDRTrunk (Java)

- H-DQPSK demodulation: differential phase detection (no MLSE)
- Phase 2 TDMA frame synchronization via ISCH sync correlation
- Burst parsing follows TIA-102.BBAC-A Annex E bit tables
- Repository: `github.com/DSheirer/sdrtrunk`
- Key classes: `P25P2DemodulatorInstrumented`, `P25P2MessageFramer`

### OP25 (C++/Python)

- H-CPM demodulation: differential detection (simplified, not full MLSE)
- H-DQPSK demodulation: differential phase detection
- TDMA frame sync and burst parsing
- Repository: `github.com/boatbod/op25`
- Key files: `op25/gr-op25_repeater/lib/p25p2_tdma.cc`

Both implementations are receive-only and do not implement transmit-side features
(H-CPM modulation, power ramping, burst assembly). A Rust implementation targeting
both TX and RX would need the full modulator and ramp profiles described above.
