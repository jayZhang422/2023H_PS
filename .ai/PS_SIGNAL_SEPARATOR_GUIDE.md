# PS Signal Separator Guide

## 1. Purpose

This application separates the two components of the ADC mixture and programs
the existing PL dual DDS. It does not generate a DAC sample stream in PS. The
only PL interfaces used by software are:

1. `axi_dma_adc`: one simple-mode S2MM channel that writes a 4096-sample ADC
   frame into DDR.
2. `axi_bram_ctrl_0`: the ten-word control block consumed by `ad9767.sv`.

This matches the implemented PL topology. No PS MM2S DMA and no DMA interrupt
are present in the exported hardware, so the application uses polling.

## 2. Runtime Flow

```text
AD9226 -> PL FIFO/AXIS -> S2MM DMA -> fixed DDR receive buffer
       -> remove DC + Hann window + FFT coarse search
       -> candidate-pair, four waveform-model residual comparison
       -> A/B descriptors {frequency, type, amplitude, measured phase}
       -> BRAM shadow words + control word + COMMIT_SEQ last
       -> PL atomically starts or tracks independent DDS A/B outputs
```

`FFT.c` contains this schedule only. Hardware access, analysis, and DDS
serialization are isolated under `User/src`.

## 3. Signal Algorithm

### 3.1 Coarse Frequency Search

The ADC frame contains 4096 samples at the PL-measured rate of 5.12080 MHz.
The nominal bin interval is 1250.1953125 Hz. The algorithm removes the frame
DC value, applies a Hann window, runs the CMSIS real FFT, and searches local
peaks only in the contest range of 20 to 100 kHz. Quadratic interpolation gives
the initial frequency for each peak.

The FFT does not make the waveform-type decision. It is only a computationally
cheap way to obtain a small set of frequency candidates.

### 3.2 Per-Channel Waveform Classification

For every frequency pair, the code evaluates all four legal models:

```text
sine(A) + sine(B)
sine(A) + triangle(B)
triangle(A) + sine(B)
triangle(A) + triangle(B)
```

For each component, a sine/cosine projection estimates the fundamental
amplitude and phase. A triangle model is formed from the odd harmonic series
through the 15th harmonic, using the same sine/triangle selection convention as
the PL DDS. Two alternating residual projections then refine the pair before
the two models are summed and compared with the original unwindowed ADC frame.
The normalized RMS residual selects the result.

This is intentionally different from the old single global `E(3f)/E(f)`
threshold. In particular, when `fB = 3*fA`, the fundamental of B and the third
harmonic of triangular A share a bin. The new joint residual contains both
terms, so it does not assign that bin to only one channel.

### 3.3 Output and Tracking

At the first valid result, PS writes both DDS shadow groups, requests a phase
reload, and commits once. A uses 0 degrees; B uses
`APP_B_TO_A_PHASE_DEGREES`, which must be set in 5-degree steps from 0 through
180 for the contest phase-control test.

For the contest's specified 5 kHz/10 kHz frequency grids, the application now
requires three consecutive matching descriptors and snaps both frequencies to
the 5 kHz grid before starting DDS. It then holds the phase steps unchanged.
This prevents frame noise from modulating the DDS frequency and producing a
thick or jittering oscilloscope trace. Measured input phase is retained for
diagnostics, not copied to the output phase words.

Frequency tracking remains an optional later feature for non-grid sources. It
must use a confidence gate, a deadband, and a verified output reference before
it is enabled in a contest build.

## 3.4 Button-Controlled Start

MIO50 (KEY1) is the sole start action. The application starts with DDS stopped
at midscale, waits for a debounced KEY1 press, then has at most 18 seconds to
obtain three matching valid frames and issue one atomic start commit. It does
not capture or output before that press.

MIO51 (KEY2) is pre-start configuration only: each debounced press changes the
requested B'-to-A' phase by 5 degrees, wrapping from 180 to 0. It has no effect
after KEY1 starts the run. Use KEY2 only for the sine/sine integer-multiple
phase-control test; leave it at 0 for all other tests.

## 4. Source Layout

| File | Responsibility |
| --- | --- |
| `signal_separator_main.c` | Button state machine, capture, lock, and one DDS start commit. |
| `FFT.c` | Preserved user-edited legacy entry; excluded from Debug and Release builds. |
| `User/include/app_config.h` | Sampling, DMA, BRAM protocol, and algorithm constants. |
| `User/src/dma_utils.c` | Single simple S2MM DMA initialization and polling capture. |
| `User/src/signal_analysis.c` | FFT candidates, joint waveform models, tracking, self-test. |
| `User/src/dds_control.c` | Ten-word BRAM snapshot and commit-last protocol. |
| `User/src/app_buffers.c` | DMA-visible receive buffer and CPU workspaces. |

`signal_processing.c` is an empty compatibility translation unit. It remains
only because it was already part of the Vitis source tree; it owns no active
algorithm.

## 5. Build and Run

1. In Vitis, refresh the application project so `signal_analysis.c` and
   `dds_control.c` are discovered, then clean and build the Debug target.
2. Program the matching bitstream from the current `real_platform` export.
3. Start the ELF and inspect UART1. The first required message is
   `Signal algorithm self-test passed`.
4. Set the source parameters. Use KEY2 only if a nonzero requested phase is
   needed, then press KEY1 once. UART reports the three-frame lock result.
5. Verify A' and B' on the oscilloscope. Trigger on A and inspect at least
   4 to 8 periods to check stable non-drifting display.

## 6. Calibration and Limits

`APP_DDS_UNITY_AMPLITUDE` is an initial digital amplitude word, not a voltage
calibration. Measure AD9767 output peak-to-peak voltage and adjust this value
or replace it with a measured ADC-code-to-DAC-code calibration curve before
claiming the required 1 V peak-to-peak result.

The following require board verification and are not proven by C compilation:

- AD9226 and AD9767 analogue gain/offset and distortion.
- DDS commit ordering over the real AXI BRAM path.
- Stable display over the contest observation interval.
- Frequency accuracy for non-grid input frequencies and low-SNR inputs.

Do not add PL decimation or an output-referenced DPLL unless measured results
show that this baseline fails an acceptance criterion.
