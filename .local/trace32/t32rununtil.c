#include "t32.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define TRACE32_STATE_DOWN 0
#define TRACE32_STATE_HALTED 1
#define TRACE32_STATE_STOPPED 2
#define TRACE32_STATE_RUNNING 3

static uint32_t now_ms(void)
{
    struct timeval now;

    if (gettimeofday(&now, NULL) != 0)
    {
        return 0U;
    }

    return (uint32_t)(((uint64_t)now.tv_sec * 1000ULL) + ((uint64_t)now.tv_usec / 1000ULL));
}

static int connect_session(const char *node, const char *port)
{
    void *channel = NULL;

    if (T32_RequestChannelNetTcp(&channel) != T32_OK)
    {
        fprintf(stderr, "failed to request NETTCP channel\n");
        return 2;
    }
    T32_SetChannel(channel);

    if ((T32_Config("NODE=", node) != T32_OK) || (T32_Config("PORT=", port) != T32_OK))
    {
        fprintf(stderr, "invalid node or port\n");
        return 3;
    }
    if (T32_Init() != T32_OK)
    {
        fprintf(stderr, "T32_Init failed\n");
        return 2;
    }
    if (T32_Attach(1) != T32_OK)
    {
        fprintf(stderr, "T32_Attach failed\n");
        T32_Exit();
        return 2;
    }
    if (T32_Nop() != T32_OK)
    {
        fprintf(stderr, "T32_Nop failed\n");
        T32_Exit();
        return 1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    const char *node = "localhost";
    const char *port = "20000";
    const char *symbol = NULL;
    const char *break_cmd = NULL;
    const char *label = NULL;
    uint32_t timeout_ms = 120000U;
    char command[1024];
    char target_name[256];
    uint32_t symbol_address = 0U;
    uint32_t symbol_size = 0U;
    uint32_t symbol_access = 0U;
    int state = TRACE32_STATE_DOWN;
    uint32_t start_ms;
    uint32_t pc = 0U;
    int rc;

    for (int index = 1; index < argc; ++index)
    {
        if (strncmp(argv[index], "node=", 5) == 0)
        {
            node = argv[index] + 5;
            continue;
        }
        if (strncmp(argv[index], "port=", 5) == 0)
        {
            port = argv[index] + 5;
            continue;
        }
        if (strncmp(argv[index], "timeout_ms=", 11) == 0)
        {
            timeout_ms = (uint32_t)strtoul(argv[index] + 11, NULL, 10);
            continue;
        }
        if (strncmp(argv[index], "break_cmd=", 10) == 0)
        {
            break_cmd = argv[index] + 10;
            continue;
        }
        if (strncmp(argv[index], "label=", 6) == 0)
        {
            label = argv[index] + 6;
            continue;
        }
        symbol = argv[index];
        break;
    }

    if ((symbol == NULL) && (break_cmd == NULL))
    {
        fprintf(stderr,
                "usage: t32rununtil [node=<host>] [port=<n>] [timeout_ms=<n>] [label=<text>] [break_cmd=<Break.Set ...>] <symbol>\n");
        return 3;
    }

    rc = connect_session(node, port);
    if (rc != 0)
    {
        return rc;
    }

    if (break_cmd != NULL)
    {
        snprintf(target_name,
                 sizeof(target_name),
                 "%s",
                 (label != NULL) ? label : break_cmd);
    }
    else if ((strncmp(symbol, "P:0x", 4) == 0) || (strncmp(symbol, "0x", 2) == 0))
    {
        const char *address_text = (strncmp(symbol, "P:0x", 4) == 0) ? (symbol + 2) : symbol;

        symbol_address = (uint32_t)strtoul(address_text, NULL, 0);
        symbol_access = 0x80U;
        symbol_size = 0U;
        snprintf(target_name, sizeof(target_name), "P:0x%08x", (unsigned int)symbol_address);
    }
    else
    {
        rc = T32_GetSymbol(symbol, &symbol_address, &symbol_size, &symbol_access);
        if (rc != T32_OK)
        {
            fprintf(stderr, "T32_GetSymbol failed rc=%d errno=%d symbol=%s\n", rc, T32_Errno, symbol);
            T32_Exit();
            return 1;
        }

        snprintf(target_name, sizeof(target_name), "%s", symbol);
    }

    symbol_address &= ~1U;

    if (T32_Cmd("Break.Delete") != T32_OK)
    {
        fprintf(stderr, "Break.Delete failed errno=%d\n", T32_Errno);
        T32_Exit();
        return 1;
    }

    if (break_cmd != NULL)
    {
        snprintf(command, sizeof(command), "%s", break_cmd);
    }
    else
    {
        snprintf(command,
                 sizeof(command),
                 "Break.Set P:0x%08x /Program",
                 (unsigned int)symbol_address);
    }
    if (T32_Cmd(command) != T32_OK)
    {
        fprintf(stderr, "Break.Set failed errno=%d command=%s\n", T32_Errno, command);
        T32_Exit();
        return 1;
    }

    if (T32_Go() != T32_OK)
    {
        fprintf(stderr, "T32_Go failed errno=%d\n", T32_Errno);
        T32_Exit();
        return 1;
    }

    start_ms = now_ms();
    for (;;)
    {
        rc = T32_GetState(&state);
        if (rc != T32_OK)
        {
            fprintf(stderr, "T32_GetState failed rc=%d errno=%d\n", rc, T32_Errno);
            T32_Exit();
            return 1;
        }

        if ((state == TRACE32_STATE_HALTED) || (state == TRACE32_STATE_STOPPED))
        {
            break;
        }

        if (state == TRACE32_STATE_DOWN)
        {
            fprintf(stderr, "target went down while waiting for %s\n", target_name);
            T32_Exit();
            return 1;
        }

        if ((now_ms() - start_ms) >= timeout_ms)
        {
            fprintf(stderr, "timeout while waiting for %s\n", target_name);
            T32_Exit();
            return 4;
        }

        usleep(1000);
    }

    if (T32_ReadPP(&pc) == T32_OK)
    {
         printf("stopped symbol=%s addr=0x%08x size=0x%08x access=0x%08x pc=0x%08x state=%d\n",
             target_name,
               symbol_address,
               symbol_size,
               symbol_access,
               pc,
               state);
    }
    else
    {
         printf("stopped symbol=%s addr=0x%08x size=0x%08x access=0x%08x state=%d\n",
             target_name,
               symbol_address,
               symbol_size,
               symbol_access,
               state);
    }

    T32_Exit();
    return 0;
}
