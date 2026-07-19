# PS/PL Integration Notes

- Keep the ADC DMA receive buffer at `APP_DMA_BUFFER_BASE`. Earlier bring-up
  with linker-placed buffers produced DMA timeout or no-result behavior. This
  is a DDR allocation workaround, not a PL peripheral address.
- The current hardware export has one simple S2MM AXI DMA and no DMA interrupt.
  PS must poll it and invalidate the receive range after completion.
- The DAC has no PS MM2S stream. PS controls the dual DDS through the ten-word
  BRAM snapshot and must write the commit sequence last.
- MIO50 (KEY1) and MIO51 (KEY2) are exported as pulled-up GPIO inputs. Use
  debounced active-low press events; never begin ADC acquisition before KEY1
  is pressed after the signal source has been configured.
