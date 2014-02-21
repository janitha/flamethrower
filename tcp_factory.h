#ifndef TCP_FACTORY_H
#define TCP_FACTORY_H

#include "common.h"

#include "tcp_worker.h"


class TcpWorker;

////////////////////////////////////////////////////////////////////////////////
// Base class for Factories
////////////////////////////////////////////////////////////////////////////////

// TODO(Janitha): base factory class
class Factory {
public:
    struct ev_loop *loop;
    FactoryParams &params;

    Factory(struct ev_loop *loop, FactoryParams &params);
    virtual ~Factory();

    static Factory* maker(struct ev_loop *loop, FactoryParams &params);
};

////////////////////////////////////////////////////////////////////////////////
// Base class for TcpFactories
////////////////////////////////////////////////////////////////////////////////
class TcpFactory : public Factory {
private:
    struct ev_timer stats_timer;
public:
    TcpFactoryParams &params;
    struct ev_async factory_async;
    std::list<TcpWorker*> workers;

    uint64_t cumulative_count;
    uint64_t bytes_in;
    uint64_t bytes_out;

    TcpFactory(struct ev_loop *loop, TcpFactoryParams &params);
    virtual ~TcpFactory();

    virtual void worker_new_cb(TcpWorker &worker);
    virtual void worker_delete_cb(TcpWorker &worker);
    static void  factory_cb(struct ev_loop *loop, struct ev_async *watcher, int revent);
    virtual void factory_cb();
    static void  stats_cb(struct ev_loop *loop, struct ev_timer *watcher, int revents);
    virtual void stats_cb();
};


////////////////////////////////////////////////////////////////////////////////
// Handles incoming TCP connections and assigns each one
// a new TcpServerWorker to actually
////////////////////////////////////////////////////////////////////////////////
class TcpServerFactory : public TcpFactory {
private:
    int accept_sock;
    struct ev_io accept_watcher;
public:
    TcpServerFactoryParams &params;

    TcpServerFactory(struct ev_loop *loop, TcpServerFactoryParams &params);
    virtual ~TcpServerFactory();

    virtual void start_listening();
    virtual void factory_cb();
    static void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
    virtual void accept_cb();
};


////////////////////////////////////////////////////////////////////////////////
// Creates outgoing TCP connections
////////////////////////////////////////////////////////////////////////////////
class TcpClientFactory : public TcpFactory {
private:
public:
    TcpClientFactoryParams &params;

    TcpClientFactory(struct ev_loop *loop, TcpClientFactoryParams &params);
    virtual ~TcpClientFactory();

    virtual void create_connection();
    virtual void factory_cb();
};


#endif
