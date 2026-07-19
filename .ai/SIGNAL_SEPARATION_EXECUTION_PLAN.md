# 2023H Signal Separation Execution Plan

## 1. Purpose and Decision

This document defines the implementation plan and its current implementation status for the 2023H signal-separation device. RTL source is present for the dual DDS and the ten-word BRAM map; the corrected RTL and active TB passed a Vivado 2020.2 XSIM behavioral regression on 2026-07-18.

The chosen approach is:

1. Implement and verify two independent DDS state machines in PL.
2. Use the existing PS DMA capture path to identify two components and their waveform types with a joint residual fit, not a global third-harmonic threshold.
3. Rebuild each output from its own DDS ROM selection, frequency word, phase word, and amplitude word.
4. Use a frequency-tracking loop first. Add a true output-referenced DPLL only if hardware tests demonstrate residual drift that the frequency tracker cannot remove.
5. Treat PL decimation as a measured performance optimization, not as the first-line solution.

This sequence satisfies the contest requirements while avoiding an unnecessarily fragile high-order IIR filter at the raw sample rate. The first source implementation was reviewed, corrected, and behaviorally simulated on 2026-07-18; hardware validation remains pending.

## 2. Verified Existing Constraints

| Item | Verified fact | Consequence |
| --- | --- | --- |
| Current DAC path | `ad9767.sv` contains two phase accumulators, independent A/B waveform, step, amplitude, and RUN running registers, plus four ROM instances. | The shared-DDS limitation and the reviewed RUN/commit/reset defects are corrected; the active behavioral regression passes. |
| Current PS-to-PL control | The AXI BRAM controller exposes a true dual-port 2048 x 32 BRAM. PS owns Port A; the DAC logic reads Port B at 125 MHz. | Two register groups can use the existing BRAM. A second physical BRAM is not required. |
| Clock domains | The ADC runs at 5.12080 MHz. The DDS/DAC runs at 125 MHz. The BRAM ports and all configuration reads cross these clock domains through the dual-port BRAM. | Configuration must be shadowed and atomically committed. Individual polled words cannot be a synchronization event. |
| Existing frame | DMA TLAST marks 4096 input samples. | The nominal FFT spacing is `5120800 / 4096 = 1250.1953125 Hz`. A 5 kHz tone is not exactly an integer bin. |
| Contest target | A and B are 20 to 100 kHz, `fA < fB`; the extension uses 5 kHz multiples. The controlled B'-to-A' phase requirement applies only when both outputs are sine waves and `fB` is an integer multiple of `fA`. | Frequency and waveform type are per-channel. The requested phase is an output-to-output phase relation, not a requirement to reproduce the source's absolute phase. |

## 3. Assessment of the Proposed Ideas

| Proposal | Assessment | Engineering decision |
| --- | --- | --- |
| Two independent DDS channels | Correct and necessary. The present outputs are tied to one DDS state. | Implement as two phase accumulators and two independent channel-control structures. |
| Eight data registers at offsets `0x00` through `0x1C` | Correct as a data layout, but incomplete by itself. Polling the eight words can expose a half-updated A/B configuration. | Keep these eight words and add a command/commit register. |
| One common `start_trigger` | Correct, provided it is an internal one-cycle DAC-clock event that loads both channel states in the same clock cycle. | Generate it from an atomic commit transaction, rather than from an unsynchronized individual register write. |
| Classify by `E(3f)/E(f)` | Valid only for an isolated and sufficiently clean single waveform. It is invalid when applied directly to C. For example, if `fB = 3*fA`, A's triangular third harmonic and B's fundamental occupy the same bin. Leakage and the actual non-bin-aligned sample rate also invalidate fixed thresholds. | Use the ratio only as one residual feature after joint A/B fitting; do not use it as a global classifier. |
| Dynamically build an 8th-order narrow IIR bandpass at 5.12080 MHz | The motivation is understandable, but it is the wrong primary mechanism for 5 kHz-separated tones. Coefficient sensitivity, transient settling, and tuning effort outweigh the benefit. | Do not use a high-order raw-rate IIR bandpass for the main separator. |
| IQ mixing plus `atan2f` | Mathematically sound for an isolated component. It is more robust than `asin` for large phase error. | Use it for phase and frequency estimation after the two components have been modelled or sufficiently rejected. |
| PI-controlled phase-step update | A slow frequency-control loop is useful to eliminate drift. A conventional DPLL, however, needs a reference phase that is time-correlated with the physical 125 MHz DDS output. The current hardware exposes no such timestamp or phase readback. | First implement frame-to-frame frequency tracking with phase-continuous phase-step updates. Add an output-referenced DPLL only after adding the required observation mechanism. |
| Decimation in PL | Useful if profiling proves that continuous raw-rate processing is too expensive. A blind factor-16 decimator is unsafe for waveform classification because a 100 kHz triangle has a 300 kHz third harmonic and the decimated Nyquist frequency would be only about 160 kHz. | Keep it optional. If added, start with a designed anti-alias decimator of at most 4:1, preserving at least the third harmonic. |
| Use the lock-in LPF to reject a 55 kHz interferer while tracking 50 kHz | Correct after mixing: the unwanted component moves to 5 kHz and a low-pass filter can reject it. | Use this as an optional refinement after the initial two-tone model; it does not replace initial two-signal identification. |

## 4. Chosen System Architecture

```text
AD9226 (5.12080 MHz)
  -> existing CDC FIFO and AXI DMA
  -> PS DDR frame(s)
  -> windowed FFT coarse search
  -> arbitrary-frequency Goertzel / time-domain joint template fit
  -> per-channel {frequency, type, amplitude, phase} descriptor
  -> PS writes two DDS shadow configurations and COMMIT_SEQ
  -> PL atomically loads both DDS states on one 125 MHz clock edge
  -> independent DDS_A and DDS_B
  -> AD9767 channel A and channel B

Continuous frames after startup:
  -> per-channel frequency estimate
  -> bounded low-pass / PI frequency tracker
  -> phase-step-only commit (no phase reload)
```

The existing DMA path remains the baseline transport. No new IP, CIC, or FIR is introduced until profiling shows that the baseline cannot meet the 20 s completion requirement.

## 5. PL Architecture and Register Contract

### 5.1 Implemented Dual DDS Structure

The current `ad9767.sv` source contains A/B channel state with:

- `wave_sel`
- `phase_step`
- `phase_acc`
- `phase_offset`
- `amplitude`
- independent centered-ROM scaling and saturation result

The source instantiates a matched sine-ROM pair and triangle-ROM pair, one pair per channel. This is the correct use of the existing single-port ROM configurations.

Both DAC ports retain the current 125 MHz clock and write timing. The corrected source holds both DAC data outputs at midscale before RUN and on the commit clock, allowing ROM latency to settle before a newly enabled waveform is output. ROM latency is acceptable because both channels use the same latency and their phase states load together.

### 5.2 Register Map

The existing BRAM address range remains unchanged. All values are 32-bit little-endian words.

| Offset | Name | Meaning |
| ---: | --- | --- |
| `0x00` | `DDS_A_WAVE` | Bit 0: 0 sine, 1 triangle. |
| `0x04` | `DDS_A_STEP` | A DDS phase increment per 125 MHz DAC clock. |
| `0x08` | `DDS_A_PHASE` | A absolute initial phase word. |
| `0x0C` | `DDS_A_AMP` | A amplitude, bits `[13:0]`. |
| `0x10` | `DDS_B_WAVE` | Bit 0: 0 sine, 1 triangle. |
| `0x14` | `DDS_B_STEP` | B DDS phase increment per 125 MHz DAC clock. |
| `0x18` | `DDS_B_PHASE` | B absolute initial phase word. For the phase-control test, `DDS_B_PHASE - DDS_A_PHASE` encodes 0 to 180 degrees in 5 degree steps. |
| `0x1C` | `DDS_B_AMP` | B amplitude, bits `[13:0]`. |
| `0x20` | `DDS_CTRL` | Bit 0 `RUN`; bit 1 `PHASE_RELOAD`; other bits reserved as zero. |
| `0x24` | `DDS_COMMIT_SEQ` | PS increments this word only after all shadow words and `DDS_CTRL` are written. |

The extra two words are mandatory. The 8 KiB BRAM has ample capacity, so retaining only the first eight words gains nothing and leaves the update non-atomic.

### 5.3 Required Commit Protocol

1. PS writes all eight channel words.
2. PS writes `DDS_CTRL`.
3. PS writes an incremented `DDS_COMMIT_SEQ` last.
4. PL detects a changed sequence, reads all ten words into PL staging registers, and then applies the complete snapshot on exactly one `clk_dac` edge.

For startup, PS sets `RUN=1` and `PHASE_RELOAD=1`. On the apply edge, PL simultaneously loads both `phase_acc` registers from their phase words, latches both phase steps, and enables output. This edge is the required common `start_trigger`.

For frequency tracking, PS sets `RUN=1` and `PHASE_RELOAD=0`. PL updates `phase_step` only and lets both accumulators continue; this prevents phase discontinuities and visible output jumps.

The RTL must not apply a changed phase word merely because it was observed during polling. This replaces the former per-word `phase_load` behavior. `RUN` must also be a running-register value updated only by commit; using the shadow `RUN` bit directly breaks atomic stop and start semantics.

### 5.4 DDS Conversion

For a desired output frequency `f_out` in hertz:

```text
phase_step = round(f_out * 2^32 / 125000000)
phase_word = round(phi_degrees * 2^32 / 360)
```

For the phase-control requirement, use `DDS_A_PHASE = 0` and `DDS_B_PHASE = round(phi_requested * 2^32 / 360)`. The 32-bit phase resolution is far finer than the required 5 degree setting resolution. The user-facing phase setting must be restricted to 0, 5, ..., 180 degrees.

## 6. PS Identification Algorithm

### 6.1 Frequency Estimation

Use the measured sampling rate `Fs = 5120800 Hz`, not a rounded 5.12 MHz constant.

1. Remove ADC DC offset and apply a Hann window to an acquisition frame.
2. Use the FFT only to find candidate spectral regions.
3. Refine each candidate with an arbitrary-frequency Goertzel evaluation or a local time-domain sinusoidal least-squares fit. Interpolate around the FFT peak before the refinement.
4. Restrict the search to 20 through 100 kHz, require `fA < fB`, and favour the specified 5 kHz grid only after a continuous-frequency estimate has established the actual peak location.

Zero-padding may assist peak interpolation, but it does not create physical frequency resolution and shall not be treated as a substitute for refinement.

### 6.2 Per-Channel Waveform Type

The present global wave-type flag is insufficient. The descriptor must carry independent `typeA` and `typeB` values; valid combinations are sine/sine, sine/triangle, triangle/sine, and triangle/triangle.

For each candidate pair and type pair, construct the sampled model:

```text
C_hat[n] = A_A * wave(typeA, 2*pi*fA*n/Fs + phiA)
         + A_B * wave(typeB, 2*pi*fB*n/Fs + phiB)
```

`wave()` is either sine or a triangle template derived from the same intended waveform convention as the DAC ROM. Estimate amplitude and phase from the fundamental, refine the parameters against the time-domain residual, and select the candidate with the smallest normalized residual. A triangular candidate includes enough odd harmonics for the captured bandwidth; the residual model must add overlapping terms rather than double-counting them.

After selecting the joint model, use the residual third-harmonic-to-fundamental ratio only as a diagnostic confidence value. Do not classify either channel from a ratio computed directly from C.

This joint rule handles the important collision case `fB = 3*fA`: the bin at `fB` is modelled as the sum of A's triangle harmonic and B's fundamental instead of being incorrectly assigned to one source.

### 6.3 Amplitude and Initial Phase

Map the fitted input amplitude to the DAC amplitude word through a measured calibration curve. ADC code amplitude cannot directly imply the AD9767 output voltage because the analogue gains and offsets are separate.

Use the fitted phase to choose a consistent initial output phase at startup. The contest's stable-display requirement does not require A' to have zero absolute phase relative to A; it requires a non-drifting display. The requested B'-to-A' phase is set explicitly through the two DDS phase words and is independent of the fitted input phases.

## 7. Drift Control Decision

### 7.1 Baseline: Frequency Tracking

After startup, process successive frames and estimate each component's frequency from the phase progression of the fitted complex fundamental. Smooth the estimate with a bounded first-order low-pass or a low-rate PI controller and issue a phase-step-only commit when the value changes materially.

This is sufficient to remove visible frequency drift while preserving DDS phase continuity. It is computationally modest and uses information available in the present hardware.

### 7.2 Why a Full DPLL Is Deferred

IQ mixing and `atan2f(I, Q)` correctly estimate phase in the ADC sample-time domain. They do not by themselves reveal the instantaneous phase of the physically running 125 MHz PL DDS. The present design has neither a DAC phase readback nor an ADC/DAC common epoch timestamp. Therefore, writing a PI correction from the ADC-domain phase error directly to the DDS step word would be an uncalibrated pseudo-loop, not a verified DPLL.

A full DPLL is a later option only if frequency tracking still leaves observable drift. Its prerequisite is a designed timing-observation contract, for example:

- a readback counter that atomically captures the DDS accumulator epoch for a DMA-frame boundary, or
- a common hardware start and timestamp path that correlates ADC sample indices with DAC clock counts, or
- a measured analogue output feedback path.

With that prerequisite, use IQ mixing plus `atan2f`, a loop bandwidth far below the 5 kHz minimum component separation, saturating PI control, and phase-step-only commits. Do not use a 100 kHz PS interrupt by default; update at the measured block/control rate.

## 8. Optional PL Decimation

Decimation is not part of the minimum implementation. Add it only after measurements show that DMA traffic or PS processing prevents completion within 20 seconds.

If used:

1. Place a designed anti-alias decimator between ADC capture and the AXIS source.
2. Begin with 4:1 decimation: output rate 1.28020 MHz and Nyquist frequency 640.10 kHz. This keeps the 300 kHz third harmonic of a 100 kHz triangle observable.
3. Quantify passband droop and either compensate it in the waveform fit or use a compensation FIR.
4. Re-run frequency, amplitude, and triangle/sine classification tests after every decimator change.

A factor-16 CIC alone is rejected for the classification path because it aliases or removes required triangle harmonics. The proposed post-mixer low-pass remains useful, but it is a narrowband refinement, not a substitute for anti-alias filtering or the initial model fit.

## 9. Implementation Status and Acceptance Gates

The dual-DDS RTL and its ten-word testbench model contain the review corrections. The behavioral regression has passed; hardware and PS integration remain the next acceptance gates.

| Stage | Work | Acceptance gate |
| --- | --- | --- |
| 1 | Completed: run the corrected dual-DDS and ten-word TB regression in Vivado 2020.2 XSIM. | Passed at 7385 ns: deterministic reset, uncommitted isolation, atomic A/B startup, independent streams, tracking commit, 0 to 180 degree 5-degree phase reloads, and `RUN=0` midscale. |
| 2 | Inspect equivalent waveforms in the project-selected Questa flow and on hardware. | Confirm the same commit edge and ROM-latency behavior outside the XSIM behavioral models. Evaluate waveform distortion only during the stable stage-two/three intervals; the full TB view intentionally includes phase-reload jumps and the final stopped-midscale state. |
| 3 | Implement PS offline estimator using recorded or synthetic vectors. | Correctly identifies all four waveform combinations, 50/55 kHz, and the collision case `fB = 3*fA`. |
| 4 | Integrate DMA acquisition and initial descriptor write. | Both outputs appear within 20 s, remain undistorted, and each peak-to-peak amplitude is at least 1 V after analogue calibration. |
| 5 | Add frequency tracking. | With A as oscilloscope trigger, A' and B' remain stable for the chosen observation interval without output phase jumps. |
| 6 | Add decimation or output-referenced DPLL only if measured evidence requires it. | The change improves the documented failing metric without breaking stages 1 through 5. |

## 10. Explicit Non-Goals

- Do not modify the PLL frequency merely to force an FFT-bin grid before measuring the impact on ADC timing and the rest of the design.
- Do not use one global waveform flag for mixed sine/triangle inputs.
- Do not treat a single `E(3f)/E(f)` threshold as proof of a waveform type in the mixed signal.
- Do not build a high-order full-rate IIR bandpass as the main 5 kHz-spacing separator.
- Do not update an absolute phase register repeatedly during normal tracking; that produces deterministic phase jumps.
- Do not introduce new physical BRAMs just to create A/B register groups; the existing control BRAM has sufficient address space.

## 11. Evidence Sources

- Contest PDF: `2023H_Doc/2023_H题_信号分离装置.pdf`, page 2 for frequency, waveform, phase, start-button, and stable-display requirements.
- Current PL integration: `2023H.srcs/sources_1/new/H_top.v`.
- Current dual-DDS implementation under review: `2023H.srcs/sources_1/new/ad9767.sv`.
- Current ADC clock configuration: `2023H.srcs/sources_1/ip/PLL_AD/PLL_AD.xci` (`5.12080 MHz`).
- Current PS/PL architecture and BRAM mapping: `.ai/ARCHITECTURE.md`.
