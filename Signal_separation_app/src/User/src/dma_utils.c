/******************************************************************************
 * dma_utils.c
 *
 * AXI DMA helper functions. The design uses simple mode and polling, which is
 * easier to debug during bring-up than interrupt-driven transfers.
 ******************************************************************************/

#include "../include/app_config.h"
#include "../include/dma_utils.h"

#include "xil_printf.h"

void dma_dump_s2mm_regs(const char *tag, XAxiDma *dma)
{
    u32 cr = XAxiDma_ReadReg(dma->RegBase + XAXIDMA_RX_OFFSET,
                             XAXIDMA_CR_OFFSET);
    u32 sr = XAxiDma_ReadReg(dma->RegBase + XAXIDMA_RX_OFFSET,
                             XAXIDMA_SR_OFFSET);

    xil_printf("%s CR=0x%08x SR=0x%08x | halted=%d idle=%d "
               "interr=%d slverr=%d decerr=%d | busy=%d\r\n",
               tag,
               (unsigned int)cr, (unsigned int)sr,
               (int)((sr >> 0) & 1),
               (int)((sr >> 1) & 1),
               (int)((sr >> 4) & 1),
               (int)((sr >> 5) & 1),
               (int)((sr >> 6) & 1),
               XAxiDma_Busy(dma, XAXIDMA_DEVICE_TO_DMA));
}

int dma_init_s2mm(XAxiDma *dma, u16 device_id)
{
    XAxiDma_Config *config;
    int status;

    config = XAxiDma_LookupConfig(device_id);
    if (config == NULL) {
        xil_printf("ERROR: no config for DMA %d\r\n", device_id);
        return XST_FAILURE;
    }

    status = XAxiDma_CfgInitialize(dma, config);
    if (status != XST_SUCCESS) {
        xil_printf("ERROR: DMA %d init failed\r\n", device_id);
        return XST_FAILURE;
    }

    if (XAxiDma_HasSg(dma) || !dma->HasS2Mm || dma->HasMm2S) {
        xil_printf("ERROR: DMA does not match simple S2MM-only PL design\r\n");
        return XST_FAILURE;
    }

    XAxiDma_IntrDisable(dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

    return XST_SUCCESS;
}

int dma_capture_frame(XAxiDma *dma, u16 *buffer, u32 length_bytes)
{
    u32 timeout = APP_DMA_TIMEOUT;

    if (XAxiDma_SimpleTransfer(dma, (UINTPTR)buffer, length_bytes,
                               XAXIDMA_DEVICE_TO_DMA) != XST_SUCCESS) {
        xil_printf("ERROR: ADC DMA start failed\r\n");
        return XST_FAILURE;
    }

    while (XAxiDma_Busy(dma, XAXIDMA_DEVICE_TO_DMA)) {
        if (--timeout == 0U) {
            xil_printf("ERROR: ADC DMA timeout\r\n");
            return XST_FAILURE;
        }
    }

    return XST_SUCCESS;
}
