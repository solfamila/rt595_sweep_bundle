/*
 * Companion slave for validating the official master DMA RX path. It keeps the
 * same write -> IBI -> readback protocol shape as the NXP example, but uses
 * the known-good interrupt slave transfer API instead of the broken slave DMA
 * path observed in this workspace.
 */

#include <stdbool.h>
#include <string.h>

#include "app.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_i3c.h"

#define I3C_MASTER_SLAVE_ADDR_7BIT 0x1EU
#define I3C_DATA_LENGTH            32U
#define I3C_PACKET_LENGTH          (I3C_DATA_LENGTH + 1U)
#define EXPERIMENT_SLAVE_BCR_IBI_REQUEST_CAPABLE (1U << 1)
#define EXPERIMENT_SLAVE_BCR_IBI_PAYLOAD         (1U << 2)

static uint8_t g_slave_txBuff[I3C_DATA_LENGTH] = {0};
static uint8_t g_slave_rxBuff[I3C_PACKET_LENGTH] = {0};
static volatile bool g_slaveCompletionFlag = false;
static volatile bool g_slaveRequestSentFlag = false;
static volatile bool g_lastTransferWasReceive = false;
static i3c_slave_handle_t g_i3cSlaveHandle;

uint8_t *g_txBuff = g_slave_txBuff;
uint32_t g_txSize = I3C_DATA_LENGTH;
volatile bool g_slaveIbiRequestSent = false;
volatile bool g_slavePostIbiEchoPending = false;
volatile bool g_slavePostIbiEchoArmed = false;
volatile bool g_slavePostIbiAddressMatched = false;

static void i3c_slave_callback(I3C_Type *base, i3c_slave_transfer_t *xfer, void *userData)
{
    (void)base;
    (void)userData;

    switch ((uint32_t)xfer->event)
    {
        case kI3C_SlaveTransmitEvent:
            g_lastTransferWasReceive = false;
            g_slavePostIbiEchoArmed = g_slavePostIbiEchoPending;
            g_slavePostIbiAddressMatched = g_slaveIbiRequestSent;
            xfer->txData = g_txBuff;
            xfer->txDataSize = g_txSize;
            break;

        case kI3C_SlaveReceiveEvent:
            g_lastTransferWasReceive = true;
            xfer->rxData = g_slave_rxBuff;
            xfer->rxDataSize = I3C_PACKET_LENGTH;
            break;

        case (kI3C_SlaveTransmitEvent | kI3C_SlaveHDRCommandMatchEvent):
            g_lastTransferWasReceive = false;
            g_slavePostIbiEchoArmed = g_slavePostIbiEchoPending;
            g_slavePostIbiAddressMatched = g_slaveIbiRequestSent;
            xfer->txData = g_txBuff;
            xfer->txDataSize = g_txSize;
            break;

        case (kI3C_SlaveReceiveEvent | kI3C_SlaveHDRCommandMatchEvent):
            g_lastTransferWasReceive = true;
            xfer->rxData = g_slave_rxBuff;
            xfer->rxDataSize = I3C_PACKET_LENGTH;
            break;

        case kI3C_SlaveCompletionEvent:
            if ((xfer->completionStatus == kStatus_Success) ||
                (g_lastTransferWasReceive && ((uint32_t)xfer->transferredCount != 0U) &&
                 (xfer->completionStatus == kStatus_I3C_Term)))
            {
                if (!g_lastTransferWasReceive)
                {
                    g_slavePostIbiEchoPending = false;
                    g_slavePostIbiEchoArmed = false;
                    g_slavePostIbiAddressMatched = false;
                    g_slaveIbiRequestSent = false;
                }
                g_slaveCompletionFlag = true;
            }
            break;

        case kI3C_SlaveRequestSentEvent:
            g_slaveIbiRequestSent = true;
            g_slaveRequestSentFlag = true;
            break;

#if defined(I3C_ASYNC_WAKE_UP_INTR_CLEAR)
        case kI3C_SlaveAddressMatchEvent:
            I3C_ASYNC_WAKE_UP_INTR_CLEAR
            break;
#endif

        default:
            break;
    }
}

int main(void)
{
    i3c_slave_config_t slaveConfig;
    uint32_t eventMask = kI3C_SlaveAllEvents;
    uint8_t ibiData = I3C_DATA_LENGTH;

    BOARD_InitHardware();

    PRINTF("MCUX SDK version: %s\r\n", MCUXSDK_VERSION_FULL_STR);
    PRINTF("\r\nI3C official RX companion -- interrupt slave transfer.\r\n");

    I3C_SlaveGetDefaultConfig(&slaveConfig);
    slaveConfig.staticAddr = I3C_MASTER_SLAVE_ADDR_7BIT;
    slaveConfig.vendorID = 0x123U;
    slaveConfig.bcr |= EXPERIMENT_SLAVE_BCR_IBI_REQUEST_CAPABLE | EXPERIMENT_SLAVE_BCR_IBI_PAYLOAD;
    slaveConfig.maxWriteLength = I3C_PACKET_LENGTH;
    slaveConfig.maxReadLength = I3C_DATA_LENGTH;
    slaveConfig.offline = false;

    I3C_SlaveInit(EXAMPLE_SLAVE, &slaveConfig, I3C_SLAVE_CLOCK_FREQUENCY);
    I3C_SlaveSetWatermarks(
        EXAMPLE_SLAVE, kI3C_TxTriggerUntilOneLessThanFull, kI3C_RxTriggerOnNotEmpty, true, true);
    I3C_SlaveTransferCreateHandle(EXAMPLE_SLAVE, &g_i3cSlaveHandle, i3c_slave_callback, NULL);

    for (uint32_t index = 0U; index < I3C_DATA_LENGTH; index++)
    {
        g_slave_txBuff[index] = (uint8_t)index;
    }
    g_txBuff = g_slave_txBuff;
    g_txSize = I3C_DATA_LENGTH;
    memset(g_slave_rxBuff, 0, sizeof(g_slave_rxBuff));

    I3C_SlaveTransferNonBlocking(EXAMPLE_SLAVE, &g_i3cSlaveHandle, eventMask);

    PRINTF("slave: armed\r\n");
    PRINTF("slave: waiting for master traffic\r\n");

    while (!g_slaveCompletionFlag)
    {
    }
    g_slaveCompletionFlag = false;

    PRINTF("slave: rx count=%u b0=%u b1=%u b2=%u b3=%u b4=%u b5=%u\r\n",
           (unsigned)g_slave_rxBuff[0],
           (unsigned)g_slave_rxBuff[0],
           (unsigned)g_slave_rxBuff[1],
           (unsigned)g_slave_rxBuff[2],
           (unsigned)g_slave_rxBuff[3],
           (unsigned)g_slave_rxBuff[4],
           (unsigned)g_slave_rxBuff[5]);

        g_slavePostIbiEchoPending = true;
    I3C_SlaveRequestIBIWithData(EXAMPLE_SLAVE, &ibiData, 1U);
    while (!g_slaveRequestSentFlag)
    {
    }
    g_slaveRequestSentFlag = false;

    while (!g_slaveCompletionFlag)
    {
    }
    g_slaveCompletionFlag = false;

    PRINTF("I3C official RX companion slave finished\r\n");

    while (1)
    {
        __NOP();
    }
}