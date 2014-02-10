#ifndef TCP_FACTORY_H
#define TCP_FACTORY_H

#include "common.h"


////////////////////////////////////////////////////////////////////////////////
// Base class for TcpFactories
////////////////////////////////////////////////////////////////////////////////
class TcpFactory {
private:
    // TODO(Janitha): keep track of worker
public:
    struct ev_loop *loop;
    TcpFactory(struct ev_loop *loop);
    virtual ~TcpFactory();
};


////////////////////////////////////////////////////////////////////////////////
// Handles incoming TCP connections and assigns each one
// a new TcpServerWorker to actually
////////////////////////////////////////////////////////////////////////////////
class TcpServerFactory : public TcpFactory {
private:
    int accept_sock;
    struct ev_io accept_watcher;
    // TODO(Janitha): keep track of workers
public:
    TcpServerFactory(struct ev_loop *loop,
                     uint32_t bind_addr,       // htonl(INADDR_ANY)
                     uint16_t bind_port,       // htons(9999)
                     uint32_t accept_backlog);
    virtual ~TcpServerFactory();
    static void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
};


////////////////////////////////////////////////////////////////////////////////
// Creates outgoing TCP connections
////////////////////////////////////////////////////////////////////////////////
class TcpClientFactory : public TcpFactory {
public:
    TcpClientFactory(struct ev_loop *loop,
                     uint32_t bind_addr,   // htonl(INADDR_ANY)
                     uint16_t bind_port,   // htons(0)
                     uint32_t server_addr, // inet_addr("1.2.3.4")
                     uint16_t server_port, // htons(9999)
                     float    timeout);    // 5.4
    virtual ~TcpClientFactory();
};


#endif
