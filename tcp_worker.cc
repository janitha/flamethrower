#include "tcp_worker.h"

////////////////////////////////////////////////////////////////////////////////
TcpWorker::TcpWorker(TcpFactory &factory,
                     TcpWorkerParams &params,
                     TcpWorkerStats &stats,
                     int sock)
    : factory(factory),
      params(params),
      stats(stats),
      sock(sock) {

    debug_socket_print(sock, "ctor\n");

    close_timer_ev.data = this;
    ev_timer_init(&close_timer_ev, close_cb, params.delay_close, 0);

    factory.worker_new_cb(*this);
}


TcpWorker::~TcpWorker() {

    debug_socket_print(sock, "dtor\n");

    ev_io_stop(factory.loop, &sock_r_ev);
    ev_io_stop(factory.loop, &sock_w_ev);
    ev_timer_stop(factory.loop, &close_timer_ev);

    if(close(sock) == -1) {
        perror("socket close error");
    }

    if(!stats.close_time) {
        stats.close_time = timestamp_ns_now();
    }

    factory.worker_delete_cb(*this);
}

void TcpWorker::finish() {

    // TODO(Janitha): This also needs to start a read handler
    //                to check if the otherside kills the conn

    ev_io_stop(factory.loop, &sock_r_ev);
    ev_io_stop(factory.loop, &sock_w_ev);

    if(params.initiate_close) {

        if(params.delay_close > 0) {
            debug_socket_print(sock, "delaying close %fs\n", params.delay_close);
            ev_timer_start(factory.loop, &close_timer_ev);
        } else {
            return close_cb();
            // TODO(Janitha): Or should we also schedule this to a timerout of 0
        }

    } else {

        debug_socket_print(sock, "wait for other side to close\n");
        sock_r_ev.data = this;
        ev_io_init(&sock_r_ev, close_wait_cb, sock, EV_READ);
        ev_io_start(factory.loop, &sock_r_ev);

    }
}

void TcpWorker::close_cb(struct ev_loop *loop, struct ev_timer *watcher, int revents) {
    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }
    TcpWorker *worker = (TcpWorker*)watcher->data;
    worker->close_cb();
}

void TcpWorker::close_cb() {
    debug_socket_print(sock, "called\n");
    delete this;     // I too like to live dangerously
    return;
}

void TcpWorker::close_wait_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }
    TcpWorker *worker = (TcpWorker*)watcher->data;
    worker->close_wait_cb();
}

void TcpWorker::close_wait_cb() {
    debug_socket_print(sock, "called\n");

    // Just recv until the other side kills the connection

    char buf[RECVBUF_SIZE];
    size_t recvlen;

    SockAct recv_ret = recv_buf(buf, sizeof(buf), recvlen);
    switch(recv_ret) {
    case SockAct::CONTINUE:
        break;
    case SockAct::ERROR:
        debug_socket_print(sock, "recv error\n");
        // Fallthrough
    case SockAct::CLOSE:
        // Fallthrough
    default:
        delete this;
        return;
    }

    debug_socket_print(sock, "read bytes=%lu\n", recvlen);

}

TcpWorker::SockAct TcpWorker::recv_buf(char *buf, size_t buflen, size_t &recvlen) {

    recvlen = 0;
    ssize_t ret = recv(sock, buf, buflen, 0);
    if(ret < 0) {
        if(errno == EWOULDBLOCK || errno == EAGAIN) {
            return SockAct::CONTINUE;
        } else {
            perror("socket recv error");
            return SockAct::ERROR;
        }
    } else if(ret == 0) {
        return SockAct::CLOSE;
    }

    recvlen = ret;

    // Update stats
    stats.bytes_in += ret;
    factory.stats.bytes_in += ret;

    return SockAct::CONTINUE;
}

TcpWorker::SockAct TcpWorker::send_buf(char *buf, size_t buflen, size_t &sentlen) {

    sentlen = 0;
    ssize_t ret = send(sock, buf, buflen, MSG_NOSIGNAL);
    if(ret < 0) {
        if(errno == EWOULDBLOCK || errno == EAGAIN) {
            return SockAct::CONTINUE;
        }

        perror("socket send error");
        return SockAct::ERROR;

        /* TODO(Janitha): Is this a valid close criteria?
        if(errno == EPIPE) {
            perror("socket pipe error");
            return ret;
        } else {
            perror("socket error");
            return ret;
        }
        */
    }

    sentlen = ret;

    // Update stats
    stats.bytes_out += ret;
    factory.stats.bytes_out += ret;

    return SockAct::CONTINUE;
}

void TcpWorker::tcp_cork(bool state) {
    int val = state;
    if(setsockopt(sock, SOL_TCP, TCP_CORK, &val, sizeof(val)) < 0) {
        perror("setsockopt error");
        exit(EXIT_FAILURE);
    }
}

void TcpWorker::read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }

    TcpWorker *worker = (TcpWorker*)watcher->data;

    // Timestamp
    if(!worker->stats.readable_time) {
        worker->stats.readable_time = timestamp_ns_now();
    }

    worker->read_cb();
}

void TcpWorker::read_cb() {

    debug_socket_print(sock, "called\n");

    char recvbuf[RECVBUF_SIZE];
    memset(recvbuf, 0, sizeof(recvbuf));
    size_t recvlen;

    // Read
    switch(recv_buf(recvbuf, sizeof(recvbuf), recvlen)) {
    case SockAct::CONTINUE:
        break;
    case SockAct::CLOSE:
        return finish();
    case SockAct::ERROR:
        debug_socket_print(sock, "recv error\n");
        // Fallthrough
    default:
        delete this; // YOLO!
        return;
    }

    debug_socket_print(sock, "read bytes=%lu\n", recvlen);

}

void TcpWorker::write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }

    TcpWorker *worker = (TcpWorker*)watcher->data;

    // Timestamp
    if(!worker->stats.writable_time) {
        worker->stats.writable_time = timestamp_ns_now();
    }

    worker->write_cb();
}

void TcpWorker::write_cb() {

    debug_socket_print(sock, "called\n");

    ev_io_stop(factory.loop, &sock_w_ev);

}

TcpWorker::SockAct TcpWorker::read_echo() {

    debug_socket_print(sock, "called\n");

    char buf[RECVBUF_SIZE];
    memset(buf, 0, sizeof(buf));
    size_t recvlen;

    SockAct recv_ret = recv_buf(buf, sizeof(buf), recvlen);
    switch(recv_ret) {
    case SockAct::CONTINUE:
        break;
    case SockAct::ERROR:
        debug_socket_print(sock, "recv error\n");
        // Fallthrough
    case SockAct::CLOSE:
        // Fallthrough
    default:
        return recv_ret;
    }

    debug_socket_print(sock, "echo: %s", buf);

    size_t sentlen;
    SockAct send_ret = send_buf(buf, recvlen, sentlen);
    switch(send_ret) {
    case SockAct::CONTINUE:
        break;
    case SockAct::ERROR:
        debug_socket_print(sock, "send error\n");
        // Fallthrough
    case SockAct::CLOSE:
        // Fallthrough
    default:
        return send_ret;
    }

    return SockAct::CONTINUE;
}

TcpWorker::SockAct TcpWorker::write_payloads(PayloadList &payloads, size_t sendlen, size_t &sentlen) {

    sendlen = (sendlen < SENDBUF_SIZE) ? sendlen : SENDBUF_SIZE;

    char *payload_ptr;
    size_t payload_len = 0;

    payload_ptr = payloads.peek(sendlen, payload_len);

    if(!payload_ptr) {
        return SockAct::CLOSE;
    }

    // TODO(Janitha): Handle payload_len = 0, this is work to be done in payloads

    SockAct send_ret = send_buf(payload_ptr, payload_len, sentlen);
    switch(send_ret) {
    case SockAct::CONTINUE:
        break;
    case SockAct::ERROR:
        debug_socket_print(sock, "send error\n");
        // Fallthrough
    case SockAct::CLOSE:
        // Fallthrough
    default:
        return send_ret;
        break;
    }

    debug_socket_print(sock, "payload wrote: %lu bytes\n", sentlen);

    if(!payloads.advance(sentlen)) {
        return SockAct::CLOSE;
    }

    return SockAct::CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////
TcpServerWorker::TcpServerWorker(TcpServerFactory &factory,
                                 TcpServerWorkerParams &params,
                                 TcpServerWorkerStats &stats,
                                 int sock)
    : TcpWorker(factory, params, stats, sock),
      params(params),
      stats(stats) {

    debug_socket_print(sock, "ctor\n");

    // Set linger behavior
    {
        struct linger lingerval;
        memset(&lingerval, 0, sizeof(lingerval));
        lingerval.l_onoff = params.tcp_linger ? 1 : 0;
        lingerval.l_linger = params.tcp_linger;
        if(setsockopt(sock, SOL_SOCKET, SO_LINGER, &lingerval, sizeof(lingerval)) < 0) {
            perror("setsockopt error");
            exit(EXIT_FAILURE);
        }
    }

    // Hookup socket readable event
    sock_r_ev.data = this;
    ev_io_init(&sock_r_ev, read_cb, sock, EV_READ);
    ev_io_start(factory.loop, &sock_r_ev);

    // Hookup socket writable event
    sock_w_ev.data = this;
    ev_io_init(&sock_w_ev, write_cb, sock, EV_WRITE);
    ev_io_start(factory.loop, &sock_w_ev);

    // Accept timestamp
    stats.established_time = timestamp_ns_now();
}

TcpServerWorker::~TcpServerWorker() {
    debug_socket_print(sock, "dtor\n");

    if(!stats.close_time) {
        stats.close_time = timestamp_ns_now();
    }
}

void TcpServerWorker::finish() {
    return TcpWorker::finish();
}

TcpServerWorker* TcpServerWorker::maker(TcpServerFactory &factory,
                                        TcpServerWorkerParams &params,
                                        int sock) {

    switch(params.type) {
    case TcpServerWorkerParams::WorkerType::ECHO:
        return new TcpServerEcho(factory,
                                 (TcpServerEchoParams&)params,
                                 *new TcpServerEchoStats(),
                                 sock);
        break;
    case TcpServerWorkerParams::WorkerType::RAW:
        return new TcpServerRaw(factory,
                                (TcpServerRawParams&)params,
                                *new TcpServerRawStats(),
                                sock);
        break;
    case TcpServerWorkerParams::WorkerType::HTTP:
        return new TcpServerHttp(factory,
                                 (TcpServerHttpParams&)params,
                                 *new TcpServerHttpStats(),
                                 sock);
        break;
    default:
        perror("error: invalid worker type\n");
        exit(EXIT_FAILURE);
    }
    return nullptr;

}

////////////////////////////////////////////////////////////////////////////////
TcpClientWorker::TcpClientWorker(TcpClientFactory &factory,
                                 TcpClientWorkerParams &params,
                                 TcpClientWorkerStats &stats)
    : TcpWorker(factory, params, stats),
      params(params),
      stats(stats) {

    debug_print("ctor\n");

    // Create
    if((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // Set linger behavior
    {
        struct linger lingerval;
        memset(&lingerval, 0, sizeof(lingerval));
        lingerval.l_onoff = params.tcp_linger ? 1 : 0;
        lingerval.l_linger = params.tcp_linger;
        if(setsockopt(sock, SOL_SOCKET, SO_LINGER, &lingerval, sizeof(lingerval)) < 0) {
            perror("setsockopt error");
            exit(EXIT_FAILURE);
        }
    }

    // Allow address reuse
    int optval1 = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval1, sizeof(optval1)) < 0) {
        perror("setsockopt error");
        exit(EXIT_FAILURE);
    }

    // Bind to local
    struct sockaddr_in sa_bind;
    memset(&sa_bind, 0, sizeof(sa_bind));
    sa_bind.sin_family = AF_INET;
    sa_bind.sin_port = factory.params.bind_port;
    sa_bind.sin_addr.s_addr = factory.params.bind_addr;
    if(bind(sock, (struct sockaddr*)&sa_bind, sizeof(sa_bind)) != 0) {
        perror("socket bind error");
        exit(EXIT_FAILURE);
    }

    // Connect to remote
    struct sockaddr_in sa_connect;
    memset(&sa_connect, 0, sizeof(sa_connect));
    sa_connect.sin_family = AF_INET;
    sa_connect.sin_port = factory.params.server_port;
    sa_connect.sin_addr.s_addr = factory.params.server_addr;
    if(connect(sock, (struct sockaddr *)&sa_connect, sizeof(sa_connect)) < 0) {
        if(errno != EINPROGRESS) {
            perror("socket connect error");
            exit(EXIT_FAILURE);
        }
    }

    debug_socket_print(sock, "connecting\n");

    // Hookup socket connected event
    sock_w_ev.data = this;
    ev_io_init(&sock_w_ev, connected_cb, sock, EV_READ|EV_WRITE);
    ev_io_start(factory.loop, &sock_w_ev);

    // Initialize the sock_r_ev, but don't start yet
    sock_r_ev.data = this;
    ev_io_init(&sock_r_ev, read_cb, sock, EV_READ);

    // TODO(Janitha): Implement the timeout_cb and reanable this
    // Hookup timeout event for socket
    sock_timeout.data = this;
    ev_timer_init(&sock_timeout, timeout_cb, factory.params.connect_timeout, 0);
    ev_timer_start(factory.loop, &sock_timeout);

    // Connect timestamp
    stats.connect_time = timestamp_ns_now();
}


TcpClientWorker::~TcpClientWorker() {
    debug_socket_print(sock, "dtor\n");
    ev_timer_stop(factory.loop, &sock_timeout);

    if(!stats.close_time) {
        stats.close_time = timestamp_ns_now();
    }
}

void TcpClientWorker::finish() {
    ev_timer_stop(factory.loop, &sock_timeout);
    return TcpWorker::finish();
}

TcpClientWorker* TcpClientWorker::maker(TcpClientFactory &factory,
                                        TcpClientWorkerParams &params) {

    switch(params.type) {
    case TcpClientWorkerParams::WorkerType::ECHO:
        return new TcpClientEcho(factory,
                                 (TcpClientEchoParams&)params,
                                 *new TcpClientEchoStats());
        break;
    case TcpClientWorkerParams::WorkerType::RAW:
        return new TcpClientRaw(factory,
                                (TcpClientRawParams&)params,
                                *new TcpClientRawStats());
        break;
    case TcpClientWorkerParams::WorkerType::HTTP:
        return new TcpClientHttp(factory,
                                 (TcpClientHttpParams&)params,
                                 *new TcpClientHttpStats());
        break;
    default:
        perror("error: invalid worker type\n");
        exit(EXIT_FAILURE);
    }
    return nullptr;

}

void TcpClientWorker::connected_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }

    TcpClientWorker *worker = (TcpClientWorker*)watcher->data;

    if(!worker->stats.established_time) {
        worker->stats.established_time = timestamp_ns_now();
    }

    worker->connected_cb();
}

void TcpClientWorker::connected_cb() {

    debug_socket_print(sock, "called\n");

    int error;
    socklen_t errsz = sizeof(error);
    if(getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &errsz) == -1) {
        perror("getsockopt error");
        exit(EXIT_FAILURE);
    }

    ev_io_stop(factory.loop, &sock_w_ev);
    ev_io_stop(factory.loop, &sock_r_ev);
    ev_timer_stop(factory.loop, &sock_timeout);

    if(error) {
        fprintf(stderr, "connect error: %s\n", strerror(error));
        delete this;
        return;
    }

    debug_socket_print(sock, "connection established\n");

    // Hookup socket readable event
    sock_r_ev.data = this;
    ev_io_init(&sock_r_ev, read_cb, sock, EV_READ);
    ev_io_start(factory.loop, &sock_r_ev);

    // Hookup socket writable event
    sock_w_ev.data = this;
    ev_io_init(&sock_w_ev, write_cb, sock, EV_WRITE);
    ev_io_start(factory.loop, &sock_w_ev);
}

void TcpClientWorker::timeout_cb(struct ev_loop *loop, struct ev_timer *watcher, int revents) {
    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }
    TcpClientWorker *worker = (TcpClientWorker*)watcher->data;

    worker->timeout_cb();
}

void TcpClientWorker::timeout_cb() {
    debug_socket_print(sock, "timed out\n");
    delete this;
    return;
}


////////////////////////////////////////////////////////////////////////////////
TcpServerEcho::TcpServerEcho(TcpServerFactory &factory,
                             TcpServerEchoParams &params,
                             TcpServerEchoStats &stats,
                             int sock)
    : TcpServerWorker(factory, params, stats, sock),
      params(params),
      stats(stats) {
    debug_socket_print(sock, "ctor\n");
}

TcpServerEcho::~TcpServerEcho() {
    debug_socket_print(sock, "dtor\n");
}

void TcpServerEcho::finish() {
    return TcpServerWorker::finish();
}

void TcpServerEcho::read_cb() {
    debug_socket_print(sock, "called\n");

    switch(TcpWorker::read_echo()) {
    case SockAct::CONTINUE:
        break;
    case SockAct::CLOSE:
        return finish();
    case SockAct::ERROR:
        debug_socket_print(sock, "read echo error\n");
        // Fallthrough
    default:
        delete this;
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////
TcpClientEcho::TcpClientEcho(TcpClientFactory &factory,
                             TcpClientEchoParams &params,
                             TcpClientEchoStats &stats)
    : TcpClientWorker(factory, params, stats),
      params(params),
      stats(stats) {
    debug_print("ctor\n");
}

TcpClientEcho::~TcpClientEcho() {
    debug_socket_print(sock, "dtor\n");
}

void TcpClientEcho::finish() {
    return TcpClientWorker::finish();
}

void TcpClientEcho::read_cb() {
    debug_socket_print(sock, "called\n");

    switch(TcpWorker::read_echo()) {
    case SockAct::CONTINUE:
        break;
    case SockAct::CLOSE:
        return finish();
    case SockAct::ERROR:
        debug_socket_print(sock, "read echo error\n");
        // Fallthrough
    default:
        delete this;
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////
TcpServerRaw::TcpServerRaw(TcpServerFactory &factory,
                           TcpServerRawParams &params,
                           TcpServerRawStats &stats,
                           int sock)
    : TcpServerWorker(factory, params, stats, sock),
      params(params),
      stats(stats),
      payloads(*this, params.payloads) {
    debug_socket_print(sock, "ctor\n");
}

TcpServerRaw::~TcpServerRaw() {
    debug_socket_print(sock, "dtor\n");
}

void TcpServerRaw::finish() {
    return TcpServerWorker::finish();
}

void TcpServerRaw::write_cb() {
    debug_socket_print(sock, "called\n");

    size_t sentlen;
    switch(write_payloads(payloads, SENDBUF_SIZE, sentlen)) {
    case SockAct::CONTINUE:
        break;
    case SockAct::CLOSE:
        return finish();
    case SockAct::ERROR:
    default:
        debug_socket_print(sock, "write payload error\n");
        delete this;
        return;
    }

}

////////////////////////////////////////////////////////////////////////////////
TcpClientRaw::TcpClientRaw(TcpClientFactory &factory,
                           TcpClientRawParams &params,
                           TcpClientRawStats &stats)
    : TcpClientWorker(factory, params, stats),
      params(params),
      stats(stats),
      payloads(*this, params.payloads) {
    debug_print("ctor\n");

}

TcpClientRaw::~TcpClientRaw() {
    debug_socket_print(sock, "dtor\n");
}

void TcpClientRaw::finish() {
    return TcpClientWorker::finish();
}

void TcpClientRaw::write_cb() {
    debug_socket_print(sock, "called\n");

    size_t sentlen;
    switch(write_payloads(payloads, SENDBUF_SIZE, sentlen)) {
    case SockAct::CONTINUE:
        break;
    case SockAct::CLOSE:
        return finish();
    case SockAct::ERROR:
    default:
        debug_socket_print(sock, "write payload error\n");
        delete this;
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////
TcpServerHttp::TcpServerHttp(TcpServerFactory &factory,
                             TcpServerHttpParams &params,
                             TcpServerHttpStats &stats,
                             int sock)
    : TcpServerWorker(factory, params, stats, sock),
      params(params),
      stats(stats),
      state(ServerState::START),
      firstline_payloads(*this, params.firstline_payloads),
      header_payloads(*this, params.header_payloads),
      body_payloads(*this, params.body_payloads),
      request_crlfcrlf_mh("\r\n\r\n", 4) {

    debug_socket_print(sock, "ctor\n");

}

TcpServerHttp::~TcpServerHttp() {
    debug_socket_print(sock, "dtor\n");
}

void TcpServerHttp::finish() {
    return TcpServerWorker::finish();
}

void TcpServerHttp::read_cb() {
    debug_socket_print(sock, "called\n");

    char recvbuf_array[RECVBUF_SIZE];
    char *recvbuf = recvbuf_array;
    memset(recvbuf, 0, sizeof(recvbuf));
    size_t recvlen;

    // Switch for socket recv
    switch(recv_buf(recvbuf_array, sizeof(recvbuf_array), recvlen)) {
    case SockAct::CONTINUE:
        break;
    case SockAct::CLOSE:
        return finish();
    case SockAct::ERROR:
        debug_socket_print(sock, "recv error\n");
        // Fallthrough
    default:
        delete this;
        return;
    }

    if(!recvlen) {
        return;
    }

    debug_socket_print(sock, "read %lu bytes\n", recvlen);

    char *request_end;

    // Switch for reading decision
    switch(state) {
    case ServerState::START:
        state = ServerState::REQUEST;

    case ServerState::REQUEST:
        debug_socket_print(sock, "reading_request\n");

        request_end = request_crlfcrlf_mh.findend(recvbuf, recvlen);
        if(!request_end) break;

        ev_io_start(factory.loop, &sock_w_ev);
        state = ServerState::RESPONSE_START;

    default:

        // TODO(Janitha): Letting the sock_r_ev to continue to eat
        // Whatever additional data is being sent, eg. req body
        //ev_io_stop(factory.loop, &sock_r_ev);

        return;
    }

    /*

    // Switch for action decision
    switch(state) {
    case ServerState::READING_REQUEST:

    case ServerState::REQUEST_FIRSTLINE:
        debug_socket_print(sock, "state=request_firstline\n");

        // TODO(Janitha): For lazy/pragmetic/perf reasons, lets just "assume"
        //                that the termination isn't cross bufs

        char *firstline_end;
        if((firstline_end = (char*)memmem(recvbuf, recvlen, CRLF, sizeof(CRLF))) == nullptr) {
            // first line end not found
            break;
        }

        firstline_end += sizeof(CRLF);

        // TODO(Janitha): Do something with the first line

        recvlen -= firstline_end - recvbuf;
        recvbuf = firstline_end;

        if(!recvlen) {
            break;
        }

        state = ServerState::REQUEST_HEADER;
        // Fallthrough

    case ServerState::REQUEST_HEADER:
        debug_socket_print(sock, "state=request_header\n");

        // TODO(Janitha): For lazy/pragmetic/perf reasons, lets just "assume"
        //                that the header termination "0D0A0D0A" isn't cross bufs

        char *header_end;
        if((header_end = (char*)memmem(recvbuf, recvlen, CRLFCRLF, sizeof(CRLFCRLF))) == nullptr) {
            // Header teminator not found
            break;
        }

        header_end += sizeof(CRLFCRLF);

        // TODO(Janitha): Do smoething with the header here

        recvlen -= header_end - recvbuf;
        recvbuf = header_end;

        state = ServerState::REQUEST_BODY;
        // Fallthrough

    case ServerState::REQUEST_BODY:
        debug_socket_print(sock, "state=request_body\n");

        // TODO(Janitha): Implement handling request body, RFC 2616 4.4

        state = ServerState::REQUEST_DONE;
        // Fallthrough

    case ServerState::REQUEST_DONE:
        debug_socket_print(sock, "state=request_done\n");

        ev_io_start(factory.loop, &sock_w_ev);

        state = ServerState::RESPONSE_FIRSTLINE;
        // Fallthrough

    default:
        debug_socket_print(sock, "state=default\n");

        break;
    }

    return;

    */

}

void TcpServerHttp::write_cb() {
    debug_socket_print(sock, "called\n");

    /*
    size_t sentlen;
    SockAct ret;

    // Switch for write decision
    switch(state) {
    case ServerState::WRITING_RESPONSE:
        debug_socket_print(sock, "writing_response\n");

        ret = write_payloads(payloads, SENDBUF_SIZE, sentlen);
        if(ret == SockAct::CONTINUE) break;

        state = ServerState::DONE;

    default:
        ev_io_stop(factory.loop, &sock_w_ev);
        return;
    }
    */

    size_t sendlen = SENDBUF_SIZE;
    size_t sentlen;
    SockAct ret;

    tcp_cork(true);

    while(sendlen) {

        sentlen = 0;

        // Switch for action decision
        switch(state) {
        case ServerState::RESPONSE_START:
            state = ServerState::RESPONSE_FIRSTLINE;

        case ServerState::RESPONSE_FIRSTLINE:
            debug_socket_print(sock, "state=response_firstline\n");

            ret = write_payloads(firstline_payloads, sendlen, sentlen);
            if(ret == SockAct::CLOSE) {
                state = ServerState::RESPONSE_FIRSTLINE_END;
            }
            sendlen -= sentlen;
            break;

        case ServerState::RESPONSE_FIRSTLINE_END:
            debug_socket_print(sock, "state=response_firstline_end\n");

            // TODO(Janitha): Emit CRLF (workaround in config file payload)

            state = ServerState::RESPONSE_HEADER;

        case ServerState::RESPONSE_HEADER:
            debug_socket_print(sock, "state=response_header\n");

            ret = write_payloads(header_payloads, sendlen, sentlen);
            if(ret == SockAct::CLOSE) {
                state = ServerState::RESPONSE_HEADER_END;
            }
            sendlen -= sentlen;
            break;

        case ServerState::RESPONSE_HEADER_END:
            debug_socket_print(sock, "state=response_header_end\n");

            // TODO(Janitha): Emit CRLF (workaround in config file payload)

            state = ServerState::RESPONSE_BODY;

        case ServerState::RESPONSE_BODY:
            debug_socket_print(sock, "state=response_body\n");

            ret = write_payloads(body_payloads, sendlen, sentlen);
            if(ret == SockAct::CLOSE) {
                state = ServerState::RESPONSE_DONE;
            }
            sendlen -= sentlen;
            break;

        case ServerState::RESPONSE_DONE:
            debug_socket_print(sock, "state=response_done\n");

            state = ServerState::DONE;

        case ServerState::DONE:
            debug_socket_print(sock, "state=done\n");

            tcp_cork(false);

            // TODO(Janitha): We may want to do a half close here
            return finish();

        default:
            ev_io_stop(factory.loop, &sock_w_ev);
            return;
        }

        if(ret == SockAct::ERROR) {
            perror("error writing payload");
            delete this;
            return;
        }
    }

    tcp_cork(false);
}

////////////////////////////////////////////////////////////////////////////////
TcpClientHttp::TcpClientHttp(TcpClientFactory &factory,
                             TcpClientHttpParams &params,
                             TcpClientHttpStats &stats)
    : TcpClientWorker(factory, params, stats),
      params(params),
      stats(stats),
      state(ClientState::REQUEST_FIRSTLINE),
      firstline_payloads(*this, params.firstline_payloads),
      header_payloads(*this, params.header_payloads),
      body_payloads(*this, params.body_payloads) {

    debug_print("ctor\n");

    debug_print("HTTP CLIENT IS BROKEN, WORK IN PROGRESS\n");
    exit(EXIT_FAILURE);
}

TcpClientHttp::~TcpClientHttp() {
    debug_socket_print(sock, "dtor\n");
}

void TcpClientHttp::finish() {
    return TcpClientWorker::finish();
}

void TcpClientHttp::read_cb() {
    debug_socket_print(sock, "called\n");

}

void TcpClientHttp::write_cb() {
    debug_socket_print(sock, "called\n");

}
