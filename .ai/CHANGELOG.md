# 2026-07-19 - Vitis Workspace Recovery and Application Build Repair

## 2026-07-19 19:30 CST - Lock Timing and Optimized Debug Build

### Changed

- Changed the persistent Vitis Debug C/C++ optimization setting from `-O0` to
  the managed-builder `optimization.level.more` (`-O2`), retaining the Debug
  output directory and current BSP linkage.
- Updated PS build/program Tcl messages and added a generated-makefile check
  that rejects an ELF build when Debug compile rules have not regenerated with
  `-O2`.
- Added bounded UART timing for DMA capture, frame analysis, and total lock
  attempt duration. The build tag is now `BTN_LOCK_DIAG_O2_20260719`.
- Recorded that periodic forced-DDS scope spikes are downstream of continuous
  ILA data; no speculative XDC or RTL timing edit was made without DAC input
  timing specifications.

## Problem Fixed

- `ps_program_and_run.tcl` could not find the Debug ELF because the Vitis
  workspace carried a stale Eclipse lock and an invalid OSGi plug-in cache.
  Vitis logged an SDX native catalog service load failure, then reported an
  Invalid Workspace to XSCT.

## Changed

- Moved the stale `.metadata/.lock` and the re-creatable
  `.metadata/.plugins/org.eclipse.osgi` cache to timestamped backups. Project
  metadata, source, BSP source, XSA, and PL files were preserved.
- Rebuilt the system platform/BSP successfully with XSCT.
- Restored the Debug `_sdk` BSP dependency as a junction to the regenerated
  platform BSP, then rebuilt `Signal_separation_app.elf` successfully.
- Updated both PS build Tcl scripts to invoke the application Debug makefile
  after the system build and verify the ELF exists.
- Added `script/start_vitis_workspace.bat`, which starts this workspace through
  the official `vitis.bat` launcher so Vitis library paths are initialized.
- Updated that launcher to accept an optional workspace path. With no argument
  it opens `2023H_PS`; with an argument it opens the specified workspace using
  the same Vitis 2020.2 environment.

# 2026-07-19 - PS/PL Automation and Vivado Script Workflow

## Scope

Recorded the complete PS/PL automation work completed in this conversation.
No RTL, constraints, block design content, XSA, bitstream, ELF, or generated
platform output was intentionally modified during script validation.

## PS Changes

- Added `script/ps_update_platform_and_build.tcl` for latest-XSA update,
  BSP source regeneration, platform clean/generate, and system clean/build.
- Added `script/ps_rebuild_system.tcl` for source-edit system clean/build.
- Added `script/ps_program_and_run.tcl` for bitstream programming, PS7 init,
  ELF download, and core-0 execution.
- Added Vitis generated-output rules in `.gitignore`.
- Added timestamped stage output to every PS script.
- Corrected the BSP operation to `bsp regenerate`; `bsp reload` only reloads
  saved BSP settings and is not the GUI Revert BSP Sources action.

## PL Changes

- Added `../2023H_PL/script/pl_update_bd.tcl` and its double-click `.bat`.
  The TCL validates the BD with F6 before saving, regenerating products and
  wrapper, and exporting `system.tcl`; it safely reuses an open target project.
- Added `../2023H_PL/script/pl_build_bitstream.tcl` and its double-click `.bat`.
  It uses a default maximum of 24 Vivado worker threads/job slots, detects
  whether current hardware is valid, offers `1` rebuild or `0` keep-current,
  and supports `--rebuild`, `--keep`, and `--threads N`.
- The full rebuild path resets `impl_1` then `synth_1`, runs synthesis and
  implementation through bitstream generation, and exports the XSA with the
  bitstream. This fixes the prior `write_bitstream is already run and up to
  date` failure.
- PL BD generation uses forced Output Product and wrapper regeneration after
  successful validation so existing output files do not suppress a BD update.
- Added Vivado generated-output rules in `../2023H_PL/.gitignore` and added
  the user-requested Git upload rule to both PS and PL `AGENTS.md` files.

## Verification

- Vivado 2020.2 verified target project discovery, already-open reuse, F6 BD
  validation, current bitstream detection, `--check`, `--validate-only`, and
  the interactive `0` / `--keep` no-write paths.
- XSCT 2020.2 verified PS workspace, platform, system project, bitstream, XSA,
  PS initialization script, and ELF names in check mode.
- Temporary Vivado session files produced during checks were removed. The
  key generated hardware files retained their pre-check timestamps.

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
- Corrected the platform update flow: the GUI `Revert BSP Sources` operation
  is represented by `bsp regenerate`; `bsp reload` was removed because it
  reloads saved BSP settings instead of regenerating BSP source output.
- Added timestamped progress output to all PS scripts.

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
