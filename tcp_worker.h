#ifndef TCP_WORKER_H
#define TCP_WORKER_H

#include "common.h"

#include "tcp_factory.h"
#include "stream_work.h"


////////////////////////////////////////////////////////////////////////////////
// Base class for TcpWorkers
////////////////////////////////////////////////////////////////////////////////
class TcpWorker {
private:
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
    TcpWorker(struct ev_loop *loop, TcpFactory* factory, int sock=-1);
    virtual ~TcpWorker();
    static void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
    static void write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
};


////////////////////////////////////////////////////////////////////////////////
// Tcp server worker responsible for a single client
////////////////////////////////////////////////////////////////////////////////
class TcpServerWorker : public TcpWorker {
public:
    TcpServerWorker(struct ev_loop *loop, TcpFactory *factory, int sock);
    virtual ~TcpServerWorker();
};


////////////////////////////////////////////////////////////////////////////////
// Tcp client worker responsible for a single connection
////////////////////////////////////////////////////////////////////////////////
class TcpClientWorker : public TcpWorker {
private:
    struct ev_timer sock_timeout;
public:
    TcpClientWorker(struct ev_loop *loop,
                    TcpFactory *factory,
                    uint32_t bind_addr,   // htonl(INADDR_ANY)
                    uint16_t bind_port,   // htons(9999)
                    uint32_t server_addr, // inet_addr("1.2.3.4")
                    uint16_t server_port, // htons(9999)
                    float    timeout);    // 5.0
    virtual ~TcpClientWorker();
    static void connect_cb(struct ev_loop *loop, struct ev_io *watcher,int revents);
    static void timeout_cb(struct ev_loop *loop, struct ev_timer *watcher, int revents);
};


#endif
