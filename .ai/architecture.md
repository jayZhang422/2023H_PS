# PS Application Architecture

## Hardware Contract

- `axi_dma_adc` is the only AXI DMA instance. It is simple mode, S2MM only,
  16-bit AXIS input, 64-bit memory interface, and has no connected interrupt.
- The input frame is 4096 16-bit ADC samples. PS invalidates the receive buffer
  cache only after polling DMA completion.
- `axi_bram_ctrl_0` is the only output-control interface. Its base is obtained
  from `XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR`; no PL address is hard-coded.
- PL applies a new DDS configuration only after `DDS_COMMIT_SEQ` changes.

## Software Modules

```text
FFT.c
 |- dma_utils: polling capture from PL into DDR
 |- signal_analysis: identify A and B independently
 `- dds_control: atomically write PL DDS configuration
```

The input and output sides are intentionally asymmetric: data returns through
DMA, while output waveform generation is entirely in PL DDS logic.

## Start State Machine

```text
ARMED (DDS stopped) -- KEY1 debounced --> LOCKING
LOCKING -- 3 matching grid-valid frames --> RUNNING
LOCKING -- 18 second timeout --> ARMED
RUNNING -- no automatic DDS update --> stable output
```

KEY1/MIO50 starts a run. The active-low EMIO controls use N16/EMIO54 for
software reset, T17/EMIO55 for a +5-degree B-to-A adjustment, and
R17/EMIO56 for a -5-degree adjustment. Reset stops DDS and returns to `ARMED`.
The phase buttons work before start and during `RUNNING`; a live adjustment
does not restart the measurement state machine.

## BRAM Commit Rule

PS must write A words, B words, and `DDS_CTRL` first. It writes
`DDS_COMMIT_SEQ` last. Start commits use `RUN | PHASE_RELOAD`; tracking commits
use `RUN` only; live B phase commits use `RUN | B_PHASE_ADJUST` and encode a
signed delta in B's phase word. This protocol is mandatory because PL scans
the dual-port BRAM across its own 125 MHz clock domain.
