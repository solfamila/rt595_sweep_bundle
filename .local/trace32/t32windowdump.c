#include "t32.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    const char *node = "localhost";
    const char *port = "20000";
    const char *window = NULL;
    void *channel = NULL;
    char buffer[4096];
    uint32_t offset = 0U;
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
        window = argv[index];
        break;
    }

    if (window == NULL)
    {
        fprintf(stderr, "usage: t32windowdump [node=<host>] [port=<n>] <window>\n");
        return 3;
    }

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

    do
    {
        rc = T32_GetWindowContent(window, buffer, sizeof(buffer), offset, T32_PRINTCODE_ASCIIE);
        if (rc < 0)
        {
            fprintf(stderr, "T32_GetWindowContent failed errno=%d offset=%u window=%s\n", T32_Errno, offset, window);
            T32_Exit();
            return 1;
        }
        if (rc == 0)
        {
            break;
        }

        fwrite(buffer, 1U, (size_t)rc, stdout);
        offset += (uint32_t)rc;
    } while (rc > 0);

    T32_Exit();
    return 0;
}
