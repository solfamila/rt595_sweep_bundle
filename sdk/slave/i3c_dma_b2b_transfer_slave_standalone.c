/*
 * Standalone local copy of the official RT595 I3C DMA slave board-to-board
 * example, adapted only to fit this workspace build/run harness.
 */

/* Standard C Included Files */
#include <stdbool.h>
#include <string.h>

/* SDK Included Files */
#include "app.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_i3c_dma.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define I3C_MASTER_SLAVE_ADDR_7BIT 0x1EU
#define I3C_DATA_LENGTH            32U
#define I3C_PACKET_LENGTH          (I3C_DATA_LENGTH + 2U)

/*******************************************************************************
 * Variables
 ******************************************************************************/
static uint8_t g_slave_txBuff[I3C_PACKET_LENGTH] = {0};
static uint8_t g_slave_rxBuff[I3C_PACKET_LENGTH] = {0};
static volatile bool g_slaveCompletionFlag = false;
static volatile bool g_slaveRequestSentFlag = false;
static i3c_slave_dma_handle_t g_i3cSlaveHandle;
static dma_handle_t g_txDmaHandle;
static dma_handle_t g_rxDmaHandle;

/*******************************************************************************
 * Code
 ******************************************************************************/
static void i3c_slave_callback(I3C_Type *base, i3c_slave_dma_transfer_t *xfer, void *userData)
{
    (void)base;
    (void)userData;

    switch ((uint32_t)xfer->event)
    {
        case kI3C_SlaveCompletionEvent:
            if (xfer->completionStatus == kStatus_Success)
            {
                g_slaveCompletionFlag = true;
            }
            break;

        case kI3C_SlaveRequestSentEvent:
            g_slaveRequestSentFlag = true;
            break;

        default:
            break;
    }
}

int main(void)
{
    i3c_slave_dma_transfer_t slaveXfer = {0};
    i3c_slave_config_t slaveConfig;
    uint8_t ibiData;

    BOARD_InitHardware();

    PRINTF("MCUX SDK version: %s\r\n", MCUXSDK_VERSION_FULL_STR);
    PRINTF("\r\nI3C board2board DMA example -- Slave transfer.\r\n");

    I3C_SlaveGetDefaultConfig(&slaveConfig);
    slaveConfig.staticAddr = I3C_MASTER_SLAVE_ADDR_7BIT;
    slaveConfig.vendorID = 0x123U;
    slaveConfig.offline = false;
    I3C_SlaveInit(EXAMPLE_SLAVE, &slaveConfig, I3C_SLAVE_CLOCK_FREQUENCY);
    I3C_SlaveSetWatermarks(
        EXAMPLE_SLAVE, kI3C_TxTriggerUntilOneLessThanFull, kI3C_RxTriggerOnNotEmpty, true, true);

    DMA_Init(EXAMPLE_DMA);
    DMA_EnableChannel(EXAMPLE_DMA, EXAMPLE_I3C_RX_CHANNEL);
    DMA_CreateHandle(&g_rxDmaHandle, EXAMPLE_DMA, EXAMPLE_I3C_RX_CHANNEL);
    DMA_EnableChannel(EXAMPLE_DMA, EXAMPLE_I3C_TX_CHANNEL);
    DMA_CreateHandle(&g_txDmaHandle, EXAMPLE_DMA, EXAMPLE_I3C_TX_CHANNEL);

    I3C_SlaveTransferCreateHandleDMA(
        EXAMPLE_SLAVE, &g_i3cSlaveHandle, i3c_slave_callback, NULL, &g_rxDmaHandle, &g_txDmaHandle);

    memset(g_slave_rxBuff, 0, sizeof(g_slave_rxBuff));
    slaveXfer.rxData = g_slave_rxBuff;
    slaveXfer.rxDataSize = sizeof(g_slave_rxBuff);
    I3C_SlaveTransferDMA(EXAMPLE_SLAVE, &g_i3cSlaveHandle, &slaveXfer, kI3C_SlaveCompletionEvent);

    PRINTF("slave: armed\r\n");
    PRINTF("slave: waiting for master traffic\r\n");

    while (!g_slaveCompletionFlag)
    {
    }
    g_slaveCompletionFlag = false;

            PRINTF("slave: rx count=%u b0=%u b1=%u b2=%u b3=%u b4=%u b5=%u b6=%u b7=%u b8=%u b9=%u b10=%u b11=%u\r\n",
            (unsigned)g_slave_rxBuff[0],
            (unsigned)g_slave_rxBuff[0],
            (unsigned)g_slave_rxBuff[1],
            (unsigned)g_slave_rxBuff[2],
            (unsigned)g_slave_rxBuff[3],
            (unsigned)g_slave_rxBuff[4],
                (unsigned)g_slave_rxBuff[5],
                (unsigned)g_slave_rxBuff[6],
                (unsigned)g_slave_rxBuff[7],
                (unsigned)g_slave_rxBuff[8],
                (unsigned)g_slave_rxBuff[9],
                (unsigned)g_slave_rxBuff[10],
                (unsigned)g_slave_rxBuff[11]);

    for (uint32_t index = 0U; index < I3C_DATA_LENGTH; index++)
    {
        g_slave_txBuff[index] = (uint8_t)index;
    }

            PRINTF("slave: tx count=%u t0=%u t1=%u t2=%u t3=%u t4=%u t5=%u t6=%u t7=%u t8=%u t9=%u t10=%u t11=%u\r\n",
               (unsigned)I3C_DATA_LENGTH,
            (unsigned)g_slave_txBuff[0],
            (unsigned)g_slave_txBuff[1],
            (unsigned)g_slave_txBuff[2],
            (unsigned)g_slave_txBuff[3],
            (unsigned)g_slave_txBuff[4],
                (unsigned)g_slave_txBuff[5],
                (unsigned)g_slave_txBuff[6],
                (unsigned)g_slave_txBuff[7],
                (unsigned)g_slave_txBuff[8],
                (unsigned)g_slave_txBuff[9],
                (unsigned)g_slave_txBuff[10],
                (unsigned)g_slave_txBuff[11]);

    memset(&slaveXfer, 0, sizeof(slaveXfer));
    slaveXfer.txData = g_slave_txBuff;
    slaveXfer.txDataSize = I3C_DATA_LENGTH;
    I3C_SlaveTransferDMA(
        EXAMPLE_SLAVE, &g_i3cSlaveHandle, &slaveXfer, kI3C_SlaveCompletionEvent | kI3C_SlaveRequestSentEvent);

    ibiData = I3C_DATA_LENGTH;
    I3C_SlaveRequestIBIWithData(EXAMPLE_SLAVE, &ibiData, 1U);

    while (!g_slaveRequestSentFlag)
    {
    }
    g_slaveRequestSentFlag = false;

    while (!g_slaveCompletionFlag)
    {
    }
    g_slaveCompletionFlag = false;

    PRINTF("I3C DMA official RX slave finished\r\n");

    while (1)
    {
        __NOP();
    }
}