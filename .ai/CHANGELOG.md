# 2026-07-19 - PS/PL Automation Scripts

## Changed

- Added `script/ps_update_platform_and_build.tcl` to update
  `Signal_separation_platform` from the PL `top.xsa`, reload and regenerate
  the BSP, clean/generate the platform, then clean/build
  `Signal_separation_app_system`.
- Added `script/ps_rebuild_system.tcl` for the source-edit clean/build flow.
- Added `script/ps_program_and_run.tcl` to program the generated bitstream,
  initialize PS7, download `Signal_separation_app.elf`, and continue core 0.
- Added the PS `.gitignore` rules for Vitis-generated artifacts and local IDE
  state.
- Added the user-requested Git upload rule to `AGENTS.md`. The current PS Git
  repository has no configured remote, so this change cannot be pushed until a
  remote is configured.

# 2026-07-18 - PS Application Refactor

## Scope

Refactored `adc_dma_test/src` to match the active PL implementation and
`SIGNAL_SEPARATION_EXECUTION_PLAN.md`.

## Changed

- Removed the active two-DMA assumption. Application flow now initializes and
  uses only `XPAR_AXI_DMA_ADC_DEVICE_ID` in S2MM polling mode.
- Removed the unsupported DAC packed-sample stream path and its TX cache flush.
- Added `dds_control.c/.h`, which writes the existing ten-word AXI BRAM control
  snapshot and updates `COMMIT_SEQ` last.
- Corrected the PS sampling constant from 5.12000 MHz to 5.12080 MHz.
- Replaced one global waveform-type flag with independent A/B descriptors.
- Added bounded frame-to-frame frequency tracking that does not reload phase.
- Added synthetic startup regression cases for four waveform combinations and
  the `fB = 3*fA` harmonic-collision case.
- Reorganized buffers around acquisition and analysis; removed obsolete DAC
  output buffers.

## Not Changed

- No BSP, block design, RTL, XCI, linker script, or generated Debug artifact
  was modified.
- DDS amplitude remains a board-calibration task; the default word is not a
  measured voltage mapping.

# 2026-07-18 - Button-Gated Contest Run

## Problem Fixed

The user-added direct GPIO read in legacy `FFT.c` ran only while KEY1 was held
and retained the old continuous frequency-update path. It could start from
pre-configuration noise and could modulate DDS frequency from frame noise.

## Changed

- Added `User/src/button_input.c/.h`: MIO50/51 input initialization, 20 ms
  debounce, and one press event after release.
- Added `signal_separator_main.c` as the active entry point. It implements
  `ARMED -> LOCKING -> RUNNING` and stops DDS before a start press.
- Added three-frame type/frequency confirmation, a 5 kHz frequency-grid lock,
  18-second lock timeout, and no automatic DDS update after lock.
- Assigned KEY1 to the only start action and KEY2 to pre-start 5-degree phase
  selection.
- Excluded legacy `FFT.c`, `fft_test.c`, and `helloworld.c` from both Vitis
  Debug and Release source sets. Legacy `FFT.c` was left unchanged because it
  was user-edited and held by an external process.

## Verified Hardware Assumptions

The exported `ps7_init.c` configures MIO50/51 with GPIO mux selection and
pull-ups. Software treats the buttons as active-low. Board testing must still
confirm that an unpressed pin reads 1 and a pressed pin reads 0.

# 2026-07-18 - Layered Board Diagnostics

## Changed

- Added bounded UART diagnostics for build identity, button levels, DMA status,
  ADC raw-code range/changes/saturation, algorithm lock decisions, and BRAM
  control-word readback.
- Added disabled-by-default `APP_DIAG_FORCE_DDS_TEST`. It produces known 50 kHz
  and 100 kHz sine DDS outputs after KEY1 while bypassing ADC, DMA, and DSP.
- Added `DEBUG_PLAYBOOK.md` with a direct UART-to-fault-domain decision table.

## Reason

Board observation of two DC outputs is ambiguous: DDS may correctly be stopped
while source locking fails, or the output path may be faulty. The new logs make
the next run sufficient to distinguish those cases without modifying RTL.
