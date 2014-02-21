#ifndef TCP_WORKER_H
#define TCP_WORKER_H

#include "common.h"

#include "tcp_factory.h"
#include "payload.h"

class Payload;
class PayloadList;

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
    sock_act recv_buf(char *buf, size_t buflen, size_t &recvlen);
    sock_act send_buf(char *buf, size_t buflen, size_t &sentlen);
    void tcp_cork(bool state);

    // Event callbacks
    static void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
    virtual void read_cb();
    static void write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
    virtual void write_cb();

    // Misc abstractions
    sock_act read_echo();
    sock_act write_payloads(PayloadList &payloads, size_t sendlen, size_t &sentlen);
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
// Tcp raw server
////////////////////////////////////////////////////////////////////////////////
class TcpServerRaw : public TcpServerWorker {
private:
    TcpServerRawParams &params;

    PayloadList payloads;
public:
    TcpServerRaw(TcpServerFactory &factory, TcpServerRawParams &params, int sock);
    virtual ~TcpServerRaw();

    virtual void write_cb();
};


////////////////////////////////////////////////////////////////////////////////
// Tcp raw client
////////////////////////////////////////////////////////////////////////////////
class TcpClientRaw : public TcpClientWorker {
private:
    TcpClientRawParams &params;

    PayloadList payloads;
public:
    TcpClientRaw(TcpClientFactory &factory, TcpClientRawParams &params);
    virtual ~TcpClientRaw();

    virtual void write_cb();
};


////////////////////////////////////////////////////////////////////////////////
// Http server
////////////////////////////////////////////////////////////////////////////////
class TcpServerHttp : public TcpServerWorker {
private:
    TcpServerHttpParams &params;

    PayloadList firstline_payloads;
    PayloadList header_payloads;
    PayloadList body_payloads;

    enum class ServerState {
        REQUEST_FIRSTLINE,
        REQUEST_HEADER,
        REQUEST_BODY,
        REQUEST_DONE,
        RESPONSE_FIRSTLINE,
        RESPONSE_FIRSTLINE_END,
        RESPONSE_HEADER,
        RESPONSE_HEADER_END,
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

    virtual void read_cb();
    virtual void write_cb();

};

#endif
