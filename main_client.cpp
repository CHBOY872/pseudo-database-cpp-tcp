#include <stdio.h>
#include "server/server.hpp"

static int port = 8898;

int main(int argc, const char **argv)
{
    if (argc != 2)
    {
        printf("Usage: <server ip>\n");
        return 1;
    }

    printf("Connecting to ip: %s\n", argv[1]);
    Client *cl = Client::Connect(argv[1], port);
    if (!cl)
    {
        perror("Error");
        return 1;
    }
    printf("Connected to ip: %s\n", argv[1]);

    cl->Run();
    delete cl;
    return 0;
}