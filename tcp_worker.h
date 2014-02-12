#ifndef TCP_WORKER_H
#define TCP_WORKER_H

#include "common.h"

#include "tcp_factory.h"
#include "stream_work.h"

class TcpFactory;
class TcpServerFactory;
class TcpClientFactory;
class StreamWork;

////////////////////////////////////////////////////////////////////////////////
// Base class for TcpWorkers
////////////////////////////////////////////////////////////////////////////////
class TcpWorker {
private:
    static const size_t RECVBUF_SIZE = 1500;
    static const size_t SENDBUF_SIZE = 1500;
    tcp_worker_params_t *params;
protected:
    struct ev_loop *loop;
    TcpFactory *factory;
    StreamWork *work;

    int sock;
    struct ev_io sock_r_ev;
    struct ev_io sock_w_ev;
public:
    std::list<TcpWorker*>::iterator workers_it;

    TcpWorker(struct ev_loop *loop, TcpFactory* factory,
              tcp_worker_params_t *params, int sock=-1);
    virtual ~TcpWorker();

    static void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
    virtual void read_cb();

    static void write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
    virtual void write_cb();
};


////////////////////////////////////////////////////////////////////////////////
// Tcp server worker responsible for a single client
////////////////////////////////////////////////////////////////////////////////
class TcpServerWorker : public TcpWorker {
private:
    tcp_server_worker_params_t *params;
public:
    TcpServerWorker(struct ev_loop *loop, TcpServerFactory *factory,
                    tcp_server_worker_params_t *params, int sock);
    virtual ~TcpServerWorker();
};


////////////////////////////////////////////////////////////////////////////////
// Tcp client worker responsible for a single connection
////////////////////////////////////////////////////////////////////////////////
class TcpClientWorker : public TcpWorker {
private:
    tcp_client_worker_params_t *params;
    struct ev_timer sock_timeout;
public:
    TcpClientWorker(struct ev_loop *loop,
                    TcpClientFactory *factory,
                    tcp_client_worker_params_t *params);
    virtual ~TcpClientWorker();

    static void connect_cb(struct ev_loop *loop, struct ev_io *watcher,int revents);
    virtual void connect_cb();

    static void timeout_cb(struct ev_loop *loop, struct ev_timer *watcher, int revents);
    virtual void timeout_cb();
};


#endif
