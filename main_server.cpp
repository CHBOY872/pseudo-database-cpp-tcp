#include <stdio.h>
#include <signal.h>
#include "server/server.hpp"

static volatile Server *serv_ptr = 0;

void sig_handler(int s)
{
    signal(SIGINT, sig_handler);
    if (serv_ptr)
        delete serv_ptr;
    _exit(0);
}

const int port = 8898;

int main()
{
    signal(SIGINT, sig_handler);
    DbHandler *db = DbHandler::Start();
    EventSelector *selector = new EventSelector();
    Server *serv = Server::Start(selector, db, port);
    if (!serv)
    {
        perror("Error: ");
        return 1;
    }
    serv_ptr = serv;
    selector->Run();
    if (serv)
        delete serv;

    return 0;
}