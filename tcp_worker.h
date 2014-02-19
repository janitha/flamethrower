#ifndef TCP_WORKER_H
#define TCP_WORKER_H

#include "common.h"

#include "tcp_factory.h"

class TcpFactory;
class TcpServerFactory;
class TcpClientFactory;


////////////////////////////////////////////////////////////////////////////////
// Base class for TcpWorkers
////////////////////////////////////////////////////////////////////////////////
class TcpWorker {
public:
    static const size_t RECVBUF_SIZE = 1500;
    static const size_t SENDBUF_SIZE = 1500;

    TcpFactory &factory;
    TcpWorkerParams &params;

    int sock;

    struct ev_io sock_r_ev;
    struct ev_io sock_w_ev;

    std::list<TcpWorker*>::iterator workers_list_pos;

    enum class sock_act {
        CONTINUE,
        CLOSE,
        ERROR
    };

    TcpWorker(TcpFactory &factory, TcpWorkerParams &params, int sock=-1);
    virtual ~TcpWorker();

    // Socket abstractions
    virtual sock_act recv_buf(char *buf, size_t buflen, size_t &recvlen);
    virtual sock_act send_buf(char *buf, size_t buflen, size_t &sentlen);

    // Event callbacks
    static void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
    virtual void read_cb();
    static void write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
    virtual void write_cb();

    // Misc abstractions
    void read_echo();
    void write_random(uint32_t &len, bool shutdown);
};


////////////////////////////////////////////////////////////////////////////////
// Tcp server worker responsible for a single client
////////////////////////////////////////////////////////////////////////////////
class TcpServerWorker : public TcpWorker {
private:
    TcpServerWorkerParams &params;
public:
    TcpServerWorker(TcpServerFactory &factory, TcpServerWorkerParams &params, int sock);
    virtual ~TcpServerWorker();

    static TcpServerWorker* maker(TcpServerFactory &factory, TcpServerWorkerParams &params, int sock);
};


////////////////////////////////////////////////////////////////////////////////
// Tcp client worker responsible for a single connection
////////////////////////////////////////////////////////////////////////////////
class TcpClientWorker : public TcpWorker {
private:
    TcpClientWorkerParams &params;
    struct ev_timer sock_timeout;
public:
    TcpClientWorker(TcpClientFactory &factory, TcpClientWorkerParams &params);
    virtual ~TcpClientWorker();

    static TcpClientWorker* maker(TcpClientFactory &factory, TcpClientWorkerParams &params);

    static void connected_cb(struct ev_loop *loop, struct ev_io *watcher,int revents);
    virtual void connected_cb();

    static void timeout_cb(struct ev_loop *loop, struct ev_timer *watcher, int revents);
    virtual void timeout_cb();
};


////////////////////////////////////////////////////////////////////////////////
// Tcp echo server
////////////////////////////////////////////////////////////////////////////////
class TcpServerEcho : public TcpServerWorker {
private:
    TcpServerEchoParams &params;
public:
    TcpServerEcho(TcpServerFactory &factory, TcpServerEchoParams &params, int sock);
    virtual ~TcpServerEcho();

    virtual void read_cb();
};


////////////////////////////////////////////////////////////////////////////////
// Tcp echo client
////////////////////////////////////////////////////////////////////////////////
class TcpClientEcho : public TcpClientWorker {
private:
    TcpClientEchoParams &params;
public:
    TcpClientEcho(TcpClientFactory &factory, TcpClientEchoParams &params);
    virtual ~TcpClientEcho();

    virtual void read_cb();
};


////////////////////////////////////////////////////////////////////////////////
// Tcp random server
////////////////////////////////////////////////////////////////////////////////
class TcpServerRandom : public TcpServerWorker {
private:
    TcpServerRandomParams &params;
    uint32_t bytes_remaining;
public:
    TcpServerRandom(TcpServerFactory &factory, TcpServerRandomParams &params, int sock);
    virtual ~TcpServerRandom();

    virtual void write_cb();
};


////////////////////////////////////////////////////////////////////////////////
// Tcp random client
////////////////////////////////////////////////////////////////////////////////
class TcpClientRandom : public TcpClientWorker {
private:
    TcpClientRandomParams &params;
    uint32_t bytes_remaining;
public:
    TcpClientRandom(TcpClientFactory &factory, TcpClientRandomParams &params);
    virtual ~TcpClientRandom();

    virtual void write_cb();
};


////////////////////////////////////////////////////////////////////////////////
// Http server
////////////////////////////////////////////////////////////////////////////////
class TcpServerHttp : public TcpServerWorker {
private:
    TcpServerHttpParams &params;

    const char* res_header_ptr;
    uint32_t res_header_remaining;

    const char* res_body_ptr;
    uint32_t res_body_remaining;

    enum class ServerState {
        REQUEST_HEADER,
        REQUEST_BODY,
        REQUEST_DONE,
        RESPONSE_HEADER,
        RESPONSE_BODY,
        RESPONSE_DONE
    } state;

public:
    TcpServerHttp(TcpServerFactory &factory, TcpServerHttpParams &params, int sock);
    virtual ~TcpServerHttp();

    virtual void read_cb();
    virtual void write_cb();
};


////////////////////////////////////////////////////////////////////////////////
// Http server
////////////////////////////////////////////////////////////////////////////////
class TcpClientHttp : public TcpClientWorker {
private:
    TcpClientHttpParams &params;
public:
    TcpClientHttp(TcpClientFactory &factory, TcpClientHttpParams &params);
    virtual ~TcpClientHttp();

};

#endif
