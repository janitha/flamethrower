#ifndef TCP_WORKER_H
#define TCP_WORKER_H

#include "common.h"

#include "tcp_factory.h"
#include "stream_work.h"

class TcpFactory;

////////////////////////////////////////////////////////////////////////////////
// Base class for TcpWorkers
////////////////////////////////////////////////////////////////////////////////
class TcpWorker {
private:
    tcp_worker_params_t *params;
    static const size_t RECVBUF_SIZE = 1500;
    static const size_t SENDBUF_SIZE = 1500;
protected:
    struct ev_loop *loop;
    TcpFactory *factory;
    StreamWork *work;

    int sock;
    struct ev_io sock_r_ev;
    struct ev_io sock_w_ev;
public:
    TcpWorker(struct ev_loop *loop, TcpFactory* factory,
              tcp_worker_params_t *params, int sock=-1);
    virtual ~TcpWorker();

    virtual void set_sock_opts();

    static void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
    static void write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
};


////////////////////////////////////////////////////////////////////////////////
// Tcp server worker responsible for a single client
////////////////////////////////////////////////////////////////////////////////
class TcpServerWorker : public TcpWorker {
private:
    tcp_server_worker_params_t *params;
public:
    TcpServerWorker(struct ev_loop *loop, TcpFactory *factory,
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
                    TcpFactory *factory,
                    tcp_client_worker_params_t *params);
    virtual ~TcpClientWorker();

    static void connect_cb(struct ev_loop *loop, struct ev_io *watcher,int revents);
    static void timeout_cb(struct ev_loop *loop, struct ev_timer *watcher, int revents);
};


#endif
