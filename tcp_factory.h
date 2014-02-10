#ifndef TCP_FACTORY_H
#define TCP_FACTORY_H

#include "common.h"

#include "tcp_worker.h"


class TcpWorker;

////////////////////////////////////////////////////////////////////////////////
// Factory creator
////////////////////////////////////////////////////////////////////////////////
class FactoryMaker {
public:
    static void make(struct ev_loop *loop,
                     factory_params_t *parms);
};


////////////////////////////////////////////////////////////////////////////////
// Base class for TcpFactories
////////////////////////////////////////////////////////////////////////////////
class TcpFactory {
private:
    tcp_factory_params_t *params;
    // TODO(Janitha): keep track of workers
public:
    struct ev_loop *loop;
    TcpFactory(struct ev_loop *loop, tcp_factory_params_t *params);
    virtual ~TcpFactory();
};


////////////////////////////////////////////////////////////////////////////////
// Handles incoming TCP connections and assigns each one
// a new TcpServerWorker to actually
////////////////////////////////////////////////////////////////////////////////
class TcpServerFactory : public TcpFactory {
private:
    tcp_server_factory_params_t *params;
    int accept_sock;
    struct ev_io accept_watcher;
public:
    TcpServerFactory(struct ev_loop *loop,
                     tcp_server_factory_params_t *params);
    virtual ~TcpServerFactory();
    static void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
};


////////////////////////////////////////////////////////////////////////////////
// Creates outgoing TCP connections
////////////////////////////////////////////////////////////////////////////////
class TcpClientFactory : public TcpFactory {
private:
    tcp_client_factory_params_t *params;
public:
    TcpClientFactory(struct ev_loop *loop,
                     tcp_client_factory_params_t *params);
    virtual ~TcpClientFactory();

};


#endif
