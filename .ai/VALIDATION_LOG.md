# Validation Log - 2026-07-18

## Scope

Validation covered the refactored PS application against the current exported
hardware headers and the PL architecture document. No RTL, BSP, XSA, block
design, linker script, or generated `Debug` file was changed.

## Contract Checks

| Check | Result |
| --- | --- |
| DMA instance count in exported `xparameters.h` | Pass: one AXI DMA instance. |
| DMA directions in exported `xparameters.h` | Pass: MM2S disabled, S2MM enabled. |
| BRAM output control macro | Pass: code uses `XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR`. |
| DMA completion policy | Pass: polling only; no interrupt API or interrupt number introduced. |
| Cache handling | Pass: receive range is invalidated only after S2MM completion. |
| DDS update order | Pass: A/B words and control are written before `COMMIT_SEQ`. |

## Cross-Compile Checks

The following sources were compiled independently with
`D:\armGCC\bin\arm-none-eabi-gcc.exe`, the current exported BSP include path,
CMSIS include path, Cortex-A9 hard-float flags, and
`-Wall -Wextra -Werror`.

| Source | Result |
| --- | --- |
| `FFT.c` | Pass |
| `User/src/app_buffers.c` | Pass |
| `User/src/dma_utils.c` | Pass |
| `User/src/dds_control.c` | Pass |
| `User/src/signal_analysis.c` | Pass |
| `User/src/signal_processing.c` compatibility unit | Pass |

An additional temporary link attempt used the locally installed standalone ARM
GCC 15.2 runtime with the Vitis 2020.2 linker script. It was intentionally not
treated as an application failure: that runtime requires startup symbols such
as `__bss_start__`, `_init`, and `_fini` that are supplied differently by the
Vitis 2020.2 build chain. Run the final full link from Vitis after project
refresh; do not replace the Vitis-generated startup or linker configuration.

## Static Flow Checks

- Active `FFT.c` and `User` sources contain no TX-DMA device macro, `AXIDMA_1`,
  `XAXIDMA_DMA_TO_DEVICE`, old DAC output module, or global third-harmonic
  classifier.
- The active entry point invalidates the DMA receive cache range and has two
  BRAM commits: initial `PHASE_RELOAD=1`, tracking `PHASE_RELOAD=0`.
- `fft_test.c` and `helloworld.c` still define `main`, but `.cproject` excludes
  both from the active source set. Do not remove that exclusion.

## Button-Gated Run Checks

- Exported BSP contains `XPAR_XGPIOPS_0_DEVICE_ID` for PS GPIO.
- Exported `ps7_init.c` configures MIO50 and MIO51 as pulled-up GPIO MIO pins.
- `button_input.c` and `signal_separator_main.c` compile with the current BSP,
  Cortex-A9 hard-float flags, `-Wall -Wextra -Werror`.
- Both Debug and Release configurations exclude `FFT.c`, `fft_test.c`, and
  `helloworld.c`; `signal_separator_main.c` is the active `main()` after a
  Vitis project refresh.
- The active main performs no `signal_track_result()` call after DDS start.
  It requires three matching 5 kHz-grid results before the single start commit.

## Layered Diagnostic Checks

- Added `diagnostics.c/.h`; it compiles with current BSP headers and strict
  warnings.
- ADC diagnostics use the post-invalidate DMA buffer and therefore observe the
  same data passed to the algorithm.
- BRAM diagnostics use `Xil_In32` at the BSP-provided BRAM base and read only
  the ten defined DDS control words.
- The forced DDS test is disabled by default and requires an explicit rebuild
  after changing `APP_DIAG_FORCE_DDS_TEST`.

## Embedded Self-Test Coverage

`signal_run_self_tests()` is compiled into the application and executes before
the first hardware capture. It generates and analyzes these vectors:

1. sine A + sine B at 50/55 kHz.
2. sine A + triangle B at 50/55 kHz.
3. triangle A + sine B at 50/55 kHz.
4. triangle A + triangle B at 50/55 kHz.
5. triangle A + sine B at 25/75 kHz, including the `fB = 3*fA` collision.

The local environment has no ARM execution emulator or attached board, so this
startup test has not been executed here. A Vitis refresh, clean build, FPGA
program, and UART observation are required before treating it as runtime pass.

## Required Board Verification

1. Confirm UART prints `Signal algorithm self-test passed`.
2. Confirm the initial BRAM commit starts both DAC channels together.
3. Test all four waveform combinations and the 25/75 kHz collision case.
4. Measure A'/B' output amplitude and calibrate `APP_DDS_UNITY_AMPLITUDE` to
   meet the 1 V peak-to-peak criterion.
5. Trigger the oscilloscope on A and verify A'/B' remain stable without phase
   jumps during the selected observation period.
