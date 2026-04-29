#include "t32.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

static uint32_t now_ms(void)
{
    struct timeval now;

    if (gettimeofday(&now, NULL) != 0)
    {
        return 0;
    }

    return (uint32_t)(((uint64_t)now.tv_sec * 1000ULL) + ((uint64_t)now.tv_usec / 1000ULL));
}

static void append_arg(char *buffer, size_t buffer_size, const char *arg)
{
    size_t used = strlen(buffer);
    size_t arg_len = strlen(arg);

    if ((used + arg_len + 2U) >= buffer_size)
    {
        fprintf(stderr, "command buffer too small\n");
        exit(3);
    }

    memcpy(buffer + used, arg, arg_len);
    buffer[used + arg_len] = ' ';
    buffer[used + arg_len + 1U] = '\0';
}

int main(int argc, char **argv)
{
    const char *node = "localhost";
    const char *port = "20000";
    const char *timeout_seconds = NULL;
    uint32_t wait_ms = 0U;
    char command[2048];
    int command_index = -1;
    void *channel = NULL;
    int rc;

    command[0] = '\0';

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
        if (strncmp(argv[index], "timeout=", 8) == 0)
        {
            timeout_seconds = argv[index] + 8;
            continue;
        }
        if (strncmp(argv[index], "wait=", 5) == 0)
        {
            wait_ms = (uint32_t)strtoul(argv[index] + 5, NULL, 10);
            continue;
        }

        command_index = index;
        break;
    }

    if (command_index < 0)
    {
        fprintf(stderr, "usage: t32cmd_nostop [node=<host>] [port=<n>] [timeout=<s>] [wait=<ms>] <cmd>\n");
        return 3;
    }

    for (int index = command_index; index < argc; ++index)
    {
        append_arg(command, sizeof(command), argv[index]);
    }

    if (T32_RequestChannelNetTcp(&channel) != T32_OK)
    {
        fprintf(stderr, "failed to request NETTCP channel\n");
        return 2;
    }
    T32_SetChannel(channel);

    if (T32_Config("NODE=", node) != T32_OK || T32_Config("PORT=", port) != T32_OK)
    {
        fprintf(stderr, "invalid node or port\n");
        return 3;
    }
    if ((timeout_seconds != NULL) && (T32_Config("TIMEOUT=", timeout_seconds) != T32_OK))
    {
        fprintf(stderr, "invalid timeout\n");
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

    rc = T32_Cmd(command);
    if (rc != T32_OK)
    {
        fprintf(stderr, "T32_Cmd failed rc=%d errno=%d for: %s\n", rc, T32_Errno, command);
        T32_Exit();
        return 1;
    }

    if (wait_ms != 0U)
    {
        uint32_t start_ms = now_ms();
        int practice_state = -1;

        do
        {
            rc = T32_GetPracticeState(&practice_state);
            if (rc != T32_OK)
            {
                fprintf(stderr, "T32_GetPracticeState failed rc=%d errno=%d\n", rc, T32_Errno);
                T32_Exit();
                return 1;
            }
            if (practice_state == 0)
            {
                break;
            }
            if ((now_ms() - start_ms) >= wait_ms)
            {
                fprintf(stderr, "wait timeout after %u ms\n", wait_ms);
                T32_Exit();
                return 4;
            }
            usleep(1000);
        } while (1);
    }

    {
        char message[2048];
        uint16_t mode = 0;
        uint16_t length = 0;

        rc = T32_GetMessageString(message, (uint16_t)sizeof(message), &mode, &length);
        if (rc == T32_OK)
        {
            printf("msg_mode=0x%x msg_len=%u msg=%s\n", mode, length, message);
        }
    }

    T32_Exit();
    return 0;
}
