#include "tcp_worker.h"

////////////////////////////////////////////////////////////////////////////////
TcpWorker::TcpWorker(TcpFactory &factory, TcpWorkerParams &params, int sock)
    : factory(factory),
      params(params),
      sock(sock) {

    debug_socket_print(sock, "ctor\n");

    factory.worker_new_cb(*this);
}


TcpWorker::~TcpWorker() {

    debug_socket_print(sock, "dtor\n");

    ev_io_stop(factory.loop, &sock_r_ev);
    ev_io_stop(factory.loop, &sock_w_ev);

    if(close(sock) == -1) {
        perror("socket close error");
    }

    factory.worker_delete_cb(*this);
}

TcpWorker::sock_act TcpWorker::recv_buf(char *buf, size_t buflen, size_t &recvlen) {

    recvlen = 0;
    ssize_t ret = recv(sock, buf, buflen, 0);
    if(ret < 0) {
        if(errno == EWOULDBLOCK || errno == EAGAIN) {
            return sock_act::CONTINUE;
        } else {
            perror("socket recv error");
            return sock_act::ERROR;
        }
    } else if(ret == 0) {
        return sock_act::CLOSE;
    }

    recvlen = ret;
    factory.bytes_in += ret;
    return sock_act::CONTINUE;
}

TcpWorker::sock_act TcpWorker::send_buf(char *buf, size_t buflen, size_t &sentlen) {

    sentlen = 0;
    ssize_t ret = send(sock, buf, buflen, MSG_NOSIGNAL);
    if(ret < 0) {
        if(errno == EWOULDBLOCK || errno == EAGAIN) {
            return sock_act::CONTINUE;
        }

        perror("socket send error");
        return sock_act::ERROR;

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
    factory.bytes_out += ret;
    return sock_act::CONTINUE;
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

    worker->read_cb();
}

void TcpWorker::read_cb() {

    debug_socket_print(sock, "called\n");

    char recvbuf[RECVBUF_SIZE];
    memset(recvbuf, 0, sizeof(recvbuf));
    size_t recvlen;

    // Read
    switch(recv_buf(recvbuf, sizeof(recvbuf), recvlen)) {
    case sock_act::CONTINUE:
        break;
    case sock_act::ERROR:
        debug_socket_print(sock, "recv error\n");
        // Fallthrough
    case sock_act::CLOSE:
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

    worker->write_cb();
}

void TcpWorker::write_cb() {

    debug_socket_print(sock, "called\n");

    ev_io_stop(factory.loop, &sock_w_ev);

}

TcpWorker::sock_act TcpWorker::read_echo() {

    debug_socket_print(sock, "called\n");

    char buf[RECVBUF_SIZE];
    memset(buf, 0, sizeof(buf));
    size_t recvlen;

    sock_act recv_ret = recv_buf(buf, sizeof(buf), recvlen);
    switch(recv_ret) {
    case sock_act::CONTINUE:
        break;
    case sock_act::ERROR:
        debug_socket_print(sock, "recv error\n");
        // Fallthrough
    case sock_act::CLOSE:
        // Fallthrough
    default:
        return recv_ret;
    }

    debug_socket_print(sock, "echo: %s", buf);

    size_t sentlen;
    sock_act send_ret = send_buf(buf, recvlen, sentlen);
    switch(send_ret) {
    case sock_act::CONTINUE:
        break;
    case sock_act::ERROR:
        debug_socket_print(sock, "send error\n");
        // Fallthrough
    case sock_act::CLOSE:
        // Fallthrough
    default:
        return send_ret;
    }

    return sock_act::CONTINUE;
}

TcpWorker::sock_act TcpWorker::write_payloads(PayloadList &payloads, size_t sendlen, size_t &sentlen) {

    sendlen = (sendlen < SENDBUF_SIZE) ? sendlen : SENDBUF_SIZE;

    char *payload_ptr;
    size_t payload_len = 0;

    payload_ptr = payloads.peek(sendlen, payload_len);

    if(!payload_ptr) {
        return sock_act::CLOSE;
    }

    // TODO(Janitha): Handle payload_len = 0, this is work to be done in payloads

    sock_act send_ret = send_buf(payload_ptr, payload_len, sentlen);
    switch(send_ret) {
    case sock_act::CONTINUE:
        break;
    case sock_act::ERROR:
        debug_socket_print(sock, "send error\n");
        // Fallthrough
    case sock_act::CLOSE:
        // Fallthrough
    default:
        return send_ret;
        break;
    }

    debug_socket_print(sock, "payload wrote: %lu bytes\n", sentlen);

    if(!payloads.advance(sentlen)) {
        return sock_act::CLOSE;
    }

    return sock_act::CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////
TcpServerWorker::TcpServerWorker(TcpServerFactory &factory, TcpServerWorkerParams &params, int sock)
    : TcpWorker(factory, params, sock),
      params(params) {

    debug_socket_print(sock, "ctor\n");

    // Set linger behavior
    {
        struct linger lingerval;
        memset(&lingerval, 0, sizeof(lingerval));
        lingerval.l_onoff = params.linger ? 1 : 0;
        lingerval.l_linger = params.linger;
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
}

TcpServerWorker::~TcpServerWorker() {
    debug_socket_print(sock, "dtor\n");
}


TcpServerWorker* TcpServerWorker::maker(TcpServerFactory &factory, TcpServerWorkerParams &params, int sock) {

    switch(params.type) {
    case TcpServerWorkerParams::WorkerType::ECHO:
        return new TcpServerEcho(factory, (TcpServerEchoParams&)params, sock);
        break;
    case TcpServerWorkerParams::WorkerType::RAW:
        return new TcpServerRaw(factory, (TcpServerRawParams&)params, sock);
        break;
    case TcpServerWorkerParams::WorkerType::HTTP:
        return new TcpServerHttp(factory, (TcpServerHttpParams&)params, sock);
        break;
    default:
        perror("error: invalid worker type\n");
        exit(EXIT_FAILURE);
    }
    return nullptr;

}

////////////////////////////////////////////////////////////////////////////////
TcpClientWorker::TcpClientWorker(TcpClientFactory &factory, TcpClientWorkerParams &params)
    : TcpWorker(factory, params),
      params(params) {

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
        lingerval.l_onoff = params.linger ? 1 : 0;
        lingerval.l_linger = params.linger;
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
}


TcpClientWorker::~TcpClientWorker() {
    debug_socket_print(sock, "dtor\n");
    ev_timer_stop(factory.loop, &sock_timeout);
}

TcpClientWorker* TcpClientWorker::maker(TcpClientFactory &factory, TcpClientWorkerParams &params) {

    switch(params.type) {
    case TcpClientWorkerParams::WorkerType::ECHO:
        return new TcpClientEcho(factory, (TcpClientEchoParams&)params);
        break;
    case TcpClientWorkerParams::WorkerType::RAW:
        return new TcpClientRaw(factory, (TcpClientRawParams&)params);
        break;
    case TcpClientWorkerParams::WorkerType::HTTP:
        return new TcpClientHttp(factory, (TcpClientHttpParams&)params);
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
        delete this; // I too like to live dangerously
        return;
    }

    debug_socket_print(sock, "connection established\n");

    sock_r_ev.data = this;
    ev_io_init(&sock_r_ev, read_cb, sock, EV_READ);
    ev_io_start(factory.loop, &sock_r_ev);

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
TcpServerEcho::TcpServerEcho(TcpServerFactory &factory, TcpServerEchoParams &params, int sock)
    : TcpServerWorker(factory, params, sock),
      params(params) {
    debug_socket_print(sock, "ctor\n");
}

TcpServerEcho::~TcpServerEcho() {
    debug_socket_print(sock, "dtor\n");
}

void TcpServerEcho::read_cb() {
    debug_socket_print(sock, "called\n");

    switch(TcpWorker::read_echo()) {
    case sock_act::CONTINUE:
        break;
    case sock_act::ERROR:
        debug_socket_print(sock, "read echo error\n");
        // Fallthrough
    case sock_act::CLOSE:
        // Fallthrough
    default:
        delete this;
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////
TcpClientEcho::TcpClientEcho(TcpClientFactory &factory, TcpClientEchoParams &params)
    : TcpClientWorker(factory, params),
      params(params) {
    debug_print("ctor\n");
}

TcpClientEcho::~TcpClientEcho() {
    debug_socket_print(sock, "dtor\n");
}

void TcpClientEcho::read_cb() {
    debug_socket_print(sock, "called\n");

    switch(TcpWorker::read_echo()) {
    case sock_act::CONTINUE:
        break;
    case sock_act::ERROR:
        debug_socket_print(sock, "read echo error\n");
        // Fallthrough
    case sock_act::CLOSE:
        // Fallthrough
    default:
        delete this;
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////
TcpServerRaw::TcpServerRaw(TcpServerFactory &factory, TcpServerRawParams &params, int sock)
    : TcpServerWorker(factory, params, sock),
      params(params),
      payloads(*this, params.payloads) {
    debug_socket_print(sock, "ctor\n");
}

TcpServerRaw::~TcpServerRaw() {
    debug_socket_print(sock, "dtor\n");


}

void TcpServerRaw::write_cb() {
    debug_socket_print(sock, "called\n");

    size_t sentlen;
    switch(write_payloads(payloads, SENDBUF_SIZE, sentlen)) {
    case sock_act::CONTINUE:
        break;
    case sock_act::CLOSE:
        if(params.shutdown) {
            delete this;
        } else {
            ev_io_stop(factory.loop, &sock_w_ev);
        }
        break;
    case sock_act::ERROR:
    default:
        debug_socket_print(sock, "write payload error\n");
        delete this;
        return;
        break;
    }

}

////////////////////////////////////////////////////////////////////////////////
TcpClientRaw::TcpClientRaw(TcpClientFactory &factory, TcpClientRawParams &params)
    : TcpClientWorker(factory, params),
      params(params),
      payloads(*this, params.payloads) {
    debug_print("ctor\n");

}

TcpClientRaw::~TcpClientRaw() {
    debug_socket_print(sock, "dtor\n");
}

void TcpClientRaw::write_cb() {
    debug_socket_print(sock, "called\n");

    size_t sentlen;
    switch(write_payloads(payloads, SENDBUF_SIZE, sentlen)) {
    case sock_act::CONTINUE:
        break;
    case sock_act::CLOSE:
        if(params.shutdown) {
            delete this;
        } else {
            ev_io_stop(factory.loop, &sock_w_ev);
        }
        break;
    case sock_act::ERROR:
    default:
        debug_socket_print(sock, "write payload error\n");
        delete this;
        return;
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
TcpServerHttp::TcpServerHttp(TcpServerFactory &factory, TcpServerHttpParams &params, int sock)
    : TcpServerWorker(factory, params, sock),
      params(params),
      firstline_payloads(*this, params.firstline_payloads),
      header_payloads(*this, params.header_payloads),
      body_payloads(*this, params.body_payloads),
      state(ServerState::REQUEST_FIRSTLINE) {
    debug_socket_print(sock, "ctor\n");
}

TcpServerHttp::~TcpServerHttp() {
    debug_socket_print(sock, "dtor\n");
}

void TcpServerHttp::read_cb() {
    debug_socket_print(sock, "called\n");

    static const char CRLFCRLF[] = { 0x0d, 0x0a, 0x0d, 0x0a, 0x0 };
    static const char CRLF[] = { 0x0d, 0x0a, 0x0 };

    char recvbuf_array[RECVBUF_SIZE];
    char *recvbuf = recvbuf_array;
    memset(recvbuf, 0, sizeof(recvbuf));
    size_t recvlen;

    // Switch for reading decision
    switch(state) {
    case ServerState::REQUEST_FIRSTLINE:
    case ServerState::REQUEST_HEADER:
    case ServerState::REQUEST_BODY:

        // Switch for socket recv
        switch(recv_buf(recvbuf_array, sizeof(recvbuf_array), recvlen)) {
        case sock_act::CONTINUE:
            break;
        case sock_act::ERROR:
            debug_socket_print(sock, "recv error\n");
            // Fallthrough
        case sock_act::CLOSE:
            // Fallthrough
        default:
            delete this;
            return;
        }

        break;

    default:
        return;
    }

    if(!recvlen) {
        return;
    }

    debug_socket_print(sock, "read %lu bytes\n", recvlen);

    // Switch for action decision
    switch(state) {

    case ServerState::REQUEST_FIRSTLINE:
        debug_socket_print(sock, "state=request_firstline\n");

        // TODO(Janitha): For lazy/pragmetic/perf reasons, lets just "assume"
        //                that the termination isn't cross bufs

        char *firstline_end;
        if((firstline_end = std::strstr(recvbuf, CRLF)) == nullptr) {
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
        if((header_end = std::strstr(recvbuf, CRLFCRLF)) == nullptr) {
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

}

void TcpServerHttp::write_cb() {
    debug_socket_print(sock, "called\n");

    size_t sendlen = SENDBUF_SIZE;
    size_t sentlen = 0;
    sock_act ret;

    while(sendlen) {

        // Switch for action decision
        switch(state) {
        case ServerState::RESPONSE_FIRSTLINE:
            debug_socket_print(sock, "state=response_firstline\n");

            ret = write_payloads(firstline_payloads, sendlen, sentlen);
            if(ret == sock_act::CLOSE) {
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
            if(ret == sock_act::CLOSE) {
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
            if(ret == sock_act::CLOSE) {
                state = ServerState::RESPONSE_DONE;
            }
            sendlen -= sentlen;
            break;

        case ServerState::RESPONSE_DONE:
            debug_socket_print(sock, "state=response_done\n");

            // TODO(Janitha): We may want to do a half close here
            delete this;
            return;

        default:
            ev_io_stop(factory.loop, &sock_w_ev);
            return;
        }

        if(ret == sock_act::ERROR) {
            perror("error writeing payload");
            delete this;
            return;
        }

        sendlen -= sentlen;

    }

}

////////////////////////////////////////////////////////////////////////////////
TcpClientHttp::TcpClientHttp(TcpClientFactory &factory, TcpClientHttpParams &params)
    : TcpClientWorker(factory, params),
      params(params) {
    debug_print("ctor\n");

}

TcpClientHttp::~TcpClientHttp() {
    debug_socket_print(sock, "dtor\n");
}

void TcpClientHttp::read_cb() {

}

void TcpClientHttp::write_cb() {

}
