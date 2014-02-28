#ifndef TCP_WORKER_H
#define TCP_WORKER_H

#include "common.h"

#include "tcp_factory.h"
#include "payload.h"
#include "memhunter.h"

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

    struct ev_timer close_timer_ev;

    std::list<TcpWorker*>::iterator workers_list_pos;

    enum class SockAct {
        CONTINUE,
        CLOSE,
        ERROR
    };

    TcpWorker(TcpFactory &factory, TcpWorkerParams &params, int sock=-1);
    virtual ~TcpWorker();

    // Socket abstractions
    SockAct recv_buf(char *buf, size_t buflen, size_t &recvlen);
    SockAct send_buf(char *buf, size_t buflen, size_t &sentlen);
    void tcp_cork(bool state);

    // Event callbacks
    static void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
    virtual void read_cb();
    static void write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
    virtual void write_cb();

    static void close_cb(struct ev_loop *loop, struct ev_timer *watcher, int revents);
    virtual void close_cb();     // TODO(Janitha): Also have a shutdown equiavalant of this
    static void close_wait_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
    virtual void close_wait_cb();

    // Worker actions
    virtual void finish();

    // Misc abstractions
    SockAct read_echo();
    SockAct write_payloads(PayloadList &payloads, size_t sendlen, size_t &sentlen);
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

    virtual void finish();
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

    virtual void finish();
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

    virtual void finish();
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

    virtual void finish();
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

    virtual void finish();
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

    virtual void finish();
};


////////////////////////////////////////////////////////////////////////////////
// Http server
////////////////////////////////////////////////////////////////////////////////
class TcpServerHttp : public TcpServerWorker {
private:
    TcpServerHttpParams &params;

    enum class ServerState {
        START,
        REQUEST,
        RESPONSE_START,
        RESPONSE_FIRSTLINE,
        RESPONSE_FIRSTLINE_END,
        RESPONSE_HEADER,
        RESPONSE_HEADER_END,
        RESPONSE_BODY,
        RESPONSE_DONE,
        DONE
    } state;

    PayloadList firstline_payloads;
    PayloadList header_payloads;
    PayloadList body_payloads;

    MemHunter request_crlfcrlf_mh;

public:
    TcpServerHttp(TcpServerFactory &factory, TcpServerHttpParams &params, int sock);
    virtual ~TcpServerHttp();

    virtual void read_cb();
    virtual void write_cb();

    virtual void finish();
};


////////////////////////////////////////////////////////////////////////////////
// Http server
////////////////////////////////////////////////////////////////////////////////
class TcpClientHttp : public TcpClientWorker {
private:
    TcpClientHttpParams &params;
public:

    enum class ClientState {
        REQUEST_FIRSTLINE,
        REQUEST_HEADER,
        REQUEST_BODY,
        REQUEST_DONE,
        RESPONSE_FIRSTLINE,
        RESPONSE_HEADER,
        RESPONES_BODY,
        RESPONSE_DONE
    } state;

    PayloadList firstline_payloads;
    PayloadList header_payloads;
    PayloadList body_payloads;

    TcpClientHttp(TcpClientFactory &factory, TcpClientHttpParams &params);
    virtual ~TcpClientHttp();

    virtual void read_cb();
    virtual void write_cb();

    virtual void finish();
};

#endif
