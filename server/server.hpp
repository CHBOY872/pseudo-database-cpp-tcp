#ifndef SERVER_SENTER
#define SERVER_SENTER

#include "../psdatabase/psdatabase.hpp"

enum
{
    current_max_lcount = 16,
    buffsize = 1024,
};

enum command_stat
{
    add_comm = 1,
    edit_comm,
    remove_comm,
    get_comm,
    quit_comm,
    success,
    not_found = -2,
    uninitialized
};
extern "C"
{
    struct message_struct
    {
        command_stat command;
        _pesel_len sel_pesel;
        _id_len id;
        char name[buffer_size];
        char surname[buffer_size];
        _pesel_len pesel;
    };
}
struct message_handle_struct : message_struct
{
    message_handle_struct()
    {
        sel_pesel = 0;
        command = uninitialized;
    }

    void Clear();
};

class FdHandler
{
    int fd;
    bool want_read;
    bool want_write;

public:
    FdHandler(int _fd) : fd(_fd), want_read(true), want_write(false) {}
    virtual ~FdHandler();

    int GetFd() { return fd; }

    void SetRead(bool _want_read) { want_read = _want_read; }
    void SetWrite(bool _want_write) { want_write = _want_write; }

    bool WantRead() { return want_read; }
    bool WantWrite() { return want_write; }

    virtual void Handle(bool r, bool w) = 0;
};

class EventSelector
{
    FdHandler **fd_array;
    int fd_array_len;
    int max_fd;
    bool quit_flag;

public:
    EventSelector()
        : fd_array(0), quit_flag(true) {}
    ~EventSelector();

    void Add(FdHandler *h);
    void Remove(FdHandler *h);

    void Run();
    void BreakLoop() { quit_flag = false; }
};

////////////////////////////////////////////////////////////////////

class Server;

class Session : public FdHandler
{
    friend class Server;
    message_struct *buf_struct;
    // unsigned char buf[sizeof(message_struct)];

    Server *the_master;

    Session(Server *_the_master, int _fd)
        : FdHandler(_fd), buf_struct(0), the_master(_the_master) {}

public:
    ~Session()
    {
        if (buf_struct)
            delete buf_struct;
    }
    void Send(message_struct *msg);

private:
    virtual void Handle(bool r, bool w);
};

class Server : public FdHandler
{
    EventSelector *the_selector;
    struct item
    {
        Session *s;
        item *next;
    };
    item *first;
    Server(EventSelector *_the_selector, DbHandler *_db, int fd);

public:
    DbHandler *db;
    unsigned char buf[sizeof(message_struct)];

    virtual ~Server();
    static Server *Start(EventSelector *_the_selector,
                         DbHandler *_db, int port);
    void RemoveSession(Session *s);

private:
    virtual void Handle(bool r, bool w);
};

class Client : FdHandler
{
    message_struct msg;
    bool quit_flag;
    unsigned char buf[sizeof(message_struct)];

    Client(int _fd);

public:
    virtual ~Client() {}
    static Client *Connect(const char *ip, int port);
    void Run();

private:
    virtual void Handle(bool r, bool w);
};

#endif