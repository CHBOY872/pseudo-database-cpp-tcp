#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include "server.hpp"
#include "../serialize/serialize.hpp"

void message_handle_struct::Clear()
{
    id = 0;
    bzero(name, buffer_size);
    bzero(surname, buffer_size);
    pesel = 0;

    sel_pesel = 0;
    command = uninitialized;
}

FdHandler::~FdHandler()
{
    if (fd)
        close(fd);
}

EventSelector::~EventSelector()
{
    if (fd_array)
        delete[] fd_array;
}

void EventSelector::Add(FdHandler *h)
{
    int fd = h->GetFd();
    int i;
    if (!fd_array)
    {
        fd_array_len =
            fd > current_max_lcount - 1 ? fd + 1 : current_max_lcount;
        fd_array = new FdHandler *[fd_array_len];
        for (i = 0; i < fd_array_len; i++)
            fd_array[i] = 0;
        max_fd = -1;
    }
    if (fd_array_len <= fd)
    {
        FdHandler **tmp = new FdHandler *[fd + 1];
        for (i = 0; i <= fd; i++)
            tmp[i] = i < fd_array_len ? fd_array[i] : 0;
        fd_array_len = fd + 1;
        delete[] fd_array;
        fd_array = tmp;
    }
    if (fd > max_fd)
        max_fd = fd;
    fd_array[fd] = h;
}

void EventSelector::Remove(FdHandler *h)
{
    int fd = h->GetFd();
    if (fd >= fd_array_len || h != fd_array[fd])
        return;
    fd_array[fd] = 0;
    if (fd == max_fd)
    {
        while (max_fd >= 0 && !fd_array[max_fd])
            max_fd--;
    }
}

void EventSelector::Run()
{
    do
    {
        int i, res;
        fd_set rds, wrs;
        FD_ZERO(&rds);
        FD_ZERO(&wrs);
        for (i = 0; i <= max_fd; i++)
        {
            if (fd_array[i])
            {
                if (fd_array[i]->WantRead())
                    FD_SET(fd_array[i]->GetFd(), &rds);
                if (fd_array[i]->WantWrite())
                    FD_SET(fd_array[i]->GetFd(), &wrs);
            }
        }
        timeval time;
        time.tv_sec = 20;
        time.tv_usec = 0;
        res = select(max_fd + 1, &rds, &wrs, 0, &time);
        if (res <= 0)
        {
            quit_flag = false;
            return;
        }
        for (i = 0; i <= max_fd; i++)
        {
            if (fd_array[i])
            {
                bool r = FD_ISSET(fd_array[i]->GetFd(), &rds);
                bool w = FD_ISSET(fd_array[i]->GetFd(), &wrs);
                if (r || w)
                    fd_array[i]->Handle(r, w);
            }
        }
    } while (quit_flag);
}

////////////////////////////////////////////////

Server::Server(EventSelector *_the_selector, DbHandler *_db, int fd)
    : FdHandler(fd), the_selector(_the_selector), first(0), db(_db)
{
    the_selector->Add(this);
}

Server *Server::Start(EventSelector *_the_selector, DbHandler *_db, int port)
{
    int _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == _fd)
        return 0;
    int opt = 1;
    setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    int stat = bind(_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (-1 == stat)
        return 0;

    stat = listen(_fd, current_max_lcount);

    if (stat == -1)
        return 0;

    return new Server(_the_selector, _db, _fd);
}

Server::~Server()
{
    while (first)
    {
        item *tmp = first;
        first = first->next;
        the_selector->Remove(tmp->s);
        delete tmp->s;
        delete tmp;
    }
    if (db)
        delete db;
    if (the_selector)
        delete the_selector;
}

void Server::RemoveSession(Session *s)
{
    the_selector->Remove(s);
    item **p;
    for (p = &first; *p; p = &((*p)->next))
    {
        if ((*p)->s == s)
        {
            item *tmp = *p;
            *p = tmp->next;
            delete tmp->s;
            delete tmp;
            return;
        }
    }
}

void Server::Handle(bool r, bool w)
{
    if (!r)
        return;
    struct sockaddr_in addr_client;
    socklen_t len = sizeof(len);
    int _fd = accept(GetFd(), (struct sockaddr *)&addr_client, &len);
    if (-1 == _fd)
        return;

    item *p = new item();
    p->next = first;
    p->s = new Session(this, _fd);
    first = p;

    the_selector->Add(p->s);
}

//////////////////////////////////////////////////////////

void Session::Send(message_struct *msg)
{
    unsigned char *ptr = the_master->buf;
    ptr = serialize_message_struct(msg, ptr, buffer_size);
    write(GetFd(), the_master->buf, sizeof(message_struct));
    bzero(the_master->buf, sizeof(message_struct));
}

void Session::Handle(bool r, bool w)
{
    if (!r)
        return;
    buf_struct = new message_struct();
    int rc = read(GetFd(), the_master->buf, sizeof(message_struct));
    if (rc <= 0)
    {
        the_master->RemoveSession(this);
        return;
    }
    unsigned char *ptr = the_master->buf;
    deserialize_message_struct(buf_struct, ptr, buffer_size);
    bzero(the_master->buf, sizeof(message_struct));
    DbObject obj(buf_struct->id, buf_struct->name,
                 buf_struct->surname, buf_struct->pesel);
    switch (buf_struct->command)
    {
    case add_comm:
        the_master->db->Add(&obj);
        break;
    case edit_comm:
        the_master->db->EditByPesel(buf_struct->sel_pesel, &obj);
        break;
    case remove_comm:
        the_master->db->RemoveByPesel(buf_struct->sel_pesel);
        break;
    case get_comm:
        if (-1 == the_master->db->GetByPesel(buf_struct->sel_pesel,
                                             &obj))
            buf_struct->command = not_found;
        else
            buf_struct->command = success;
        buf_struct->id = obj.id;
        buf_struct->pesel = obj.pesel;
        strcpy(buf_struct->name, obj.name);
        strcpy(buf_struct->surname, obj.surname);
        Send(buf_struct);
        break;
    case quit_comm:
        the_master->RemoveSession(this);
        break;
    default:
        the_master->RemoveSession(this);
        break;
    }
    delete buf_struct;
    buf_struct = 0;
}

//////////////////////////////////////////////////////

Client::Client(int _fd) : FdHandler(_fd), quit_flag(true)
{
    msg.id = 0;
    msg.command = uninitialized;
    msg.pesel = 0;
    msg.sel_pesel = 0;
    bzero(msg.name, buffer_size);
    bzero(msg.surname, buffer_size);
    bzero(buf, sizeof(message_struct));
    SetRead(false);
    SetWrite(false);
}

Client *Client::Connect(const char *ip, int port)
{
    int _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == _fd)
        return 0;

    int opt = 1;
    setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr_client;
    addr_client.sin_addr.s_addr = htons(INADDR_ANY);
    addr_client.sin_port = htons(port);
    addr_client.sin_family = AF_INET;

    if (-1 == bind(_fd, (struct sockaddr *)&addr_client, sizeof(addr_client)))
        return 0;

    struct sockaddr_in addr_server;
    addr_server.sin_port = htons(port);
    addr_server.sin_family = AF_INET;
    addr_server.sin_addr.s_addr = inet_addr(ip);

    int stat =
        connect(_fd, (struct sockaddr *)&addr_server, sizeof(addr_server));

    if (stat == -1)
        return 0;
    return new Client(_fd);
}

void Client::Run()
{
    while (quit_flag)
    {
        char buf_command[10];
        char buf_pesel[40];
        char buf_sel_pesel[40];
        fd_set rds, wrs;
        FD_ZERO(&rds);
        FD_ZERO(&wrs);

        if (!WantRead())
        {
            scanf("%s", buf_command);

            if (!strcmp(buf_command, "add"))
                msg.command = add_comm;
            else if (!strcmp(buf_command, "edit"))
                msg.command = edit_comm;
            else if (!strcmp(buf_command, "get"))
                msg.command = get_comm;
            else if (!strcmp(buf_command, "remove"))
                msg.command = remove_comm;
            else if (!strcmp(buf_command, "quit"))
                msg.command = quit_comm;
            else
            {
                printf("Unknown command...\n");
                continue;
            }
        }

        switch (msg.command)
        {
        case add_comm:
            scanf("%s %s %s", msg.name, msg.surname,
                  buf_pesel);
            msg.id = 0;
            msg.pesel = atoll(buf_pesel);
            if (!msg.pesel)
            {
                printf("Usage: add <name> <surname> <pesel>\n");
                continue;
            }
            SetWrite(true);
            SetRead(false);
            break;
        case edit_comm:
            scanf("%s %s %s %s", buf_sel_pesel, msg.name,
                  msg.surname, buf_pesel);
            msg.id = 0;
            msg.pesel = atoll(buf_pesel);
            msg.sel_pesel = atoll(buf_sel_pesel);
            if (!msg.pesel || !msg.sel_pesel)
            {
                printf("Usage: edit <sel_pesel> <name> <surname> <pesel>\n");
                continue;
            }
            SetWrite(true);
            SetRead(false);
            break;
        case get_comm:
            if (WantRead())
                break;
            scanf("%s", buf_sel_pesel);
            msg.id = 0;
            msg.sel_pesel = atoll(buf_sel_pesel);
            if (!msg.sel_pesel)
            {
                printf("Usage: get <sel_pesel>\n");
                continue;
            }
            SetWrite(true);
            SetRead(true);
            break;
        case remove_comm:
            scanf("%s", buf_sel_pesel);
            msg.id = 0;
            msg.sel_pesel = atoll(buf_sel_pesel);
            if (!msg.sel_pesel)
            {
                printf("Usage: remove <sel_pesel>\n");
                continue;
            }
            SetWrite(true);
            SetRead(false);
            break;
        case quit_comm:
            quit_flag = false;
            continue;
        default:
            continue;
            break;
        }

        if (WantWrite())
            FD_SET(GetFd(), &wrs);
        if (WantRead())
            FD_SET(GetFd(), &rds);

        int stat = select(GetFd() + 1, &rds, &wrs, 0, 0);

        if (stat <= 0)
        {
            quit_flag = false;
            perror("error");
            continue;
        }

        bool r = FD_ISSET(GetFd(), &rds);
        bool w = FD_ISSET(GetFd(), &wrs);

        if (r || w)
            Handle(r, w);
    }
}

void Client::Handle(bool r, bool w)
{
    if (WantWrite())
    {
        unsigned char *ptr = buf;
        ptr = serialize_message_struct(&msg, ptr, buffer_size);
        int stat = write(GetFd(), buf, sizeof(buf));
        if (stat <= 0)
        {
            quit_flag = false;
            return;
        }
        bzero(buf, sizeof(buf));
        SetWrite(false);
    }

    if (r)
    {
        int rc = read(GetFd(), buf, sizeof(buf));
        if (rc <= 0)
        {
            quit_flag = false;
            return;
        }
        unsigned char *ptr = buf;
        deserialize_message_struct(&msg, ptr, buffer_size);
        bzero(buf, sizeof(buf));
        switch (msg.command)
        {
        case success:
            printf("Success!\n");
            break;
        case not_found:
            printf("Not found...\n");
            break;
        default:
            break;
        }
        SetRead(false);
    }
}