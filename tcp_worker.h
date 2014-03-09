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
    TcpWorkerStats &stats;

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

    TcpWorker(TcpFactory &factory,
              TcpWorkerParams &params,
              TcpWorkerStats &stats,
              int sock=-1);
    virtual ~TcpWorker();

    // Worker actions
    virtual void finish();

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
    TcpServerWorkerStats &stats;
public:

    TcpServerWorker(TcpServerFactory &factory,
                    TcpServerWorkerParams &params,
                    TcpServerWorkerStats &stats,
                    int sock);
    virtual ~TcpServerWorker();

    static TcpServerWorker* maker(TcpServerFactory &factory,
                                  TcpServerWorkerParams &params,
                                  int sock);

    virtual void finish();
};


////////////////////////////////////////////////////////////////////////////////
// Tcp client worker responsible for a single connection
////////////////////////////////////////////////////////////////////////////////
class TcpClientWorker : public TcpWorker {
private:
    TcpClientWorkerParams &params;
    TcpClientWorkerStats &stats;

    struct ev_timer sock_timeout;

public:

    TcpClientWorker(TcpClientFactory &factory,
                    TcpClientWorkerParams &params,
                    TcpClientWorkerStats &stats);
    virtual ~TcpClientWorker();

    static TcpClientWorker* maker(TcpClientFactory &factory,
                                  TcpClientWorkerParams &params);

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
    TcpServerEchoStats &stats;
public:
    TcpServerEcho(TcpServerFactory &factory,
                  TcpServerEchoParams &params,
                  TcpServerEchoStats &stats,
                  int sock);
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
    TcpClientEchoStats &stats;
public:
    TcpClientEcho(TcpClientFactory &factory,
                  TcpClientEchoParams &params,
                  TcpClientEchoStats &stats);
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
    TcpServerRawStats &stats;

    PayloadList payloads;
public:
    TcpServerRaw(TcpServerFactory &factory,
                 TcpServerRawParams &params,
                 TcpServerRawStats &stats,
                 int sock);
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
    TcpClientRawStats &stats;

    PayloadList payloads;
public:
    TcpClientRaw(TcpClientFactory &factory,
                 TcpClientRawParams &params,
                 TcpClientRawStats &stats);
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
    TcpServerHttpStats &stats;

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
    TcpServerHttp(TcpServerFactory &factory,
                  TcpServerHttpParams &params,
                  TcpServerHttpStats &stats,
                  int sock);
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
    TcpClientHttpStats &stats;
public:

    enum class ClientState {
        START,
        REQUEST_FIRSTLINE,
        REQUEST_FIRSTLINE_END,
        REQUEST_HEADER,
        REQUEST_HEADER_END,
        REQUEST_BODY,
        REQUEST_DONE,
        RESPONSE,
        DONE
    } state;

    PayloadList firstline_payloads;
    PayloadList header_payloads;
    PayloadList body_payloads;

    TcpClientHttp(TcpClientFactory &factory,
                  TcpClientHttpParams &params,
                  TcpClientHttpStats &stats);
    virtual ~TcpClientHttp();

    virtual void read_cb();
    virtual void write_cb();

    virtual void finish();
};

#endif
