# Board Debug Playbook

## 1. Build the Diagnostic ELF

Refresh `adc_dma_test` in Vitis, clean the project, and build. Confirm the
UART starts with:

```text
DBG build=BTN_LOCK_DIAG_20260718
Self-test passed. DDS is stopped until KEY1.
DBG key levels: KEY1=1 KEY2=1 active=0
```

`KEY1=1` and `KEY2=1` mean both pulled-up active-low keys are released. A zero
before a press means incorrect electrical level, button wiring, or GPIO mux.

## 2. Expected Normal Sequence

1. Configure the signal source first.
2. Optional: press KEY2 to select B'-to-A' phase.
3. Press and release KEY1 once.
4. UART must show `START accepted` followed by three accepted ADC/analysis
   attempts, `DBG DDS locked start`, then `RUNNING`.

Before `DBG DDS locked start`, both DAC outputs intentionally stay at midscale.
Two DC outputs at this stage are therefore expected behavior, not evidence of
failed waveform separation.

## 3. ADC and DMA Diagnosis

The first four lock attempts print lines such as:

```text
DBG ADC[1]: min=... max=... mean=... change=... sat_lo=... sat_hi=...
DBG S2MM: CR=... SR=... halted=... idle=... interr=... slverr=... decerr=...
```

| Observation | Meaning | Next action |
| --- | --- | --- |
| `min=max`, `change=0` | DMA buffer is constant, or ADC input is constant. | Check ADC clock/data with ILA or scope; then verify DMA TLAST/stream activity. |
| `sat_lo` or `sat_hi` is large | ADC clipping. | Reduce analogue input level or correct bias/gain. |
| `slverr=1` or `decerr=1` | AXI DMA memory/stream fault. | Check HP0 path, DDR buffer address, DMA status and PL AXIS connection. |
| `halted=1` after a capture | DMA did not remain in normal receive state. | Inspect the reported CR/SR before changing DSP code. |
| ADC values vary, no DMA errors | DMA/DDR/cache path is probably working. | Continue with algorithm logs. |

## 4. Algorithm Diagnosis

```text
DBG ANA[n]: signal_analyze_frame failed
DBG ANA[n]: A=... B=... residual_ppm=... lock=reject
```

- `signal_analyze_frame failed`: no two valid local peaks were found in
  20-100 kHz. Check source frequency, source wiring, ADC amplitude, and the
  raw ADC range first.
- `lock=reject` with valid A/B: the residual exceeds 0.30, a frequency differs
  from the nearest 5 kHz grid by more than 1 kHz, or the two results are not
  ordered `fA < fB`. Record the printed values before changing thresholds.
- The start commit occurs only after three consecutive matching normalized
  results. Until then DC output is intentional.

## 5. DDS and DAC Isolation Test

Set this macro in `User/include/app_config.h` temporarily:

```c
#define APP_DIAG_FORCE_DDS_TEST 1
```

Rebuild, press KEY1 once, and inspect UART:

```text
DBG forced DDS test enabled; ADC/DMA is bypassed
DBG DDS forced 50k/100k sine: ... ctrl=3 seq=...
```

This should produce A=50 kHz sine and B=100 kHz sine without using ADC, DMA,
or the analysis algorithm.

| Forced test result | Fault domain |
| --- | --- |
| Both channels are still DC while `ctrl=3`, both amplitudes are nonzero, and phase steps are nonzero | PL DDS, BRAM Port-B connection, DAC clock/reset, DAC analogue path, or wrong bitstream. |
| Forced test outputs correct sine waves | PL output path works; return the macro to `0` and debug ADC/DMA/algorithm logs. |
| BRAM readback is zero or differs from requested values | PS-to-BRAM AXI path, active hardware export, or wrong bitstream. |

Always restore `APP_DIAG_FORCE_DDS_TEST` to `0` before normal separation tests.

## 6. Capture for Review

Provide the complete UART output from boot through either `RUNNING` or `lock
timeout`, plus one oscilloscope screenshot of A/B DAC outputs. Do not change
multiple thresholds before collecting this baseline.
