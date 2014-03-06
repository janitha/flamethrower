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
private:
    struct ev_timer stats_timer;
public:
    struct ev_loop *loop;
    FactoryParams &params;

    StatsList statslist;

    struct ev_async factory_async;

    Factory(struct ev_loop *loop, FactoryParams &params);
    virtual ~Factory();

    static Factory* maker(struct ev_loop *loop, FactoryParams &params);

    static void  stats_cb(struct ev_loop *loop, struct ev_timer *watcher, int revents);
    virtual void stats_cb();

    static void  factory_cb(struct ev_loop *loop, struct ev_async *watcher, int revent);
    virtual void factory_cb();
};

////////////////////////////////////////////////////////////////////////////////
// Base class for TcpFactories
////////////////////////////////////////////////////////////////////////////////
class TcpFactory : public Factory {
public:
    TcpFactoryParams &params;
    TcpFactoryStats &stats;

    std::list<TcpWorker*> workers;

    TcpFactory(struct ev_loop *loop,
               TcpFactoryParams &params,
               TcpFactoryStats &stats);
    virtual ~TcpFactory();

    virtual void worker_new_cb(TcpWorker &worker);
    virtual void worker_delete_cb(TcpWorker &worker);
    virtual void factory_cb();
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
    TcpServerFactoryStats &stats;

    TcpServerFactory(struct ev_loop *loop,
                     TcpServerFactoryParams &params,
                     TcpServerFactoryStats &stats);
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
    TcpClientFactoryStats &stats;

    TcpClientFactory(struct ev_loop *loop,
                     TcpClientFactoryParams &params,
                     TcpClientFactoryStats &stats);
    virtual ~TcpClientFactory();

    virtual void create_connection();
    virtual void factory_cb();
};


#endif
