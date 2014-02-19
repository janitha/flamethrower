#include "tcp_worker.h"

////////////////////////////////////////////////////////////////////////////////
TcpWorker::TcpWorker(TcpFactory &factory, TcpWorkerParams &params, int sock)
    : factory(factory),
      params(params),
      sock(sock) {

    debug_print("ctor\n");

    factory.worker_new_cb(*this);
}


TcpWorker::~TcpWorker() {

    debug_print("dtor\n");

    ev_io_stop(factory.loop, &sock_r_ev);
    ev_io_stop(factory.loop, &sock_w_ev);

    /*
    if(shutdown(sock, SHUT_RDWR) == -1) {
        perror("socket shutdown error");
    }
    */

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


    /*

    char sendbuf[SENDBUF_SIZE];
    memset(sendbuf, 0, sizeof(sendbuf));
    size_t sentlen = 0;

    int hret;
    if((hret=work->read_handler(recvbuf, recvlen, sendbuf, sizeof(sendbuf), sentlen)) < 0) {
        perror("work handler error");
        exit(EXIT_FAILURE);
        // TODO(Janitha): Shutdown socket and kill the worker instead of exit
    }

    if(sentlen) {
        // TODO(Janitha): Abstract the send to a single location (between cbs)
        if(send(sock, sendbuf, sentlen, MSG_NOSIGNAL) < 0) {
            if(errno == EPIPE) {
                perror("socket error: broken pipe");
            } else {
                // TODO(Janitha): Handle non blocking send conditions
                perror("send error");
                exit(EXIT_FAILURE);
            }
        } else{
            factory.bytes_out += sentlen;
        }
    }

    if(hret == STREAMWORK_FINISHED) {
        ev_io_stop(factory.loop, &sock_r_ev);
    }

    if(hret == STREAMWORK_SHUTDOWN) {

        debug_socket_print(sock, "read shutdown\n");

        ev_io_stop(factory.loop, &sock_r_ev);
        //delete (TcpWorker*)worker; // TODO(Janitha): is this sane? what if the write side isn't done?
    }

    */

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

    /*
    char sendbuf[SENDBUF_SIZE];
    memset(sendbuf, 0, sizeof(sendbuf));
    size_t sentlen = 0;

    int hret;
    if((hret = work->handler(nullptr, 0, sendbuf, sizeof(sendbuf), sentlen)) < 0) {
        // TODO(Janitha): Lets make this a bit more graceful
        perror("work handler error");
        exit(EXIT_FAILURE);
    }

    // TODO(Janitha): We can do TCP_CORK and TCP_NODELAY
    if(send(sock, sendbuf, sentlen, MSG_NOSIGNAL) < 0) {
        if(errno == EPIPE) {
            perror("socket error: broken pipe");
        } else {
            // TODO(Janitha): Handle non blocking send conditions
            perror("send error");
            exit(EXIT_FAILURE);
        }
    } else {
        factory.bytes_out += sentlen;
    }

    if(hret == STREAMWORK_FINISHED) {
        debug_socket_print(sock, "finished\n");
        ev_io_stop(factory.loop, &sock_w_ev);
        return;
    }

    if(hret == STREAMWORK_SHUTDOWN) {
        debug_socket_print(sock, "write shutdown\n");
        ev_io_stop(factory.loop, &sock_w_ev);
        shutdown(sock, SHUT_WR);
        //delete this;
        //return;
    }

    */
}

void TcpWorker::echo() {

    char buf[RECVBUF_SIZE];
    memset(buf, 0, sizeof(buf));
    size_t recvlen;

    switch(recv_buf(buf, sizeof(buf), recvlen)) {
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

    debug_print("echo: %s", buf);

    size_t sentlen;
    switch(send_buf(buf, recvlen, sentlen)) {
    case sock_act::CONTINUE:
        break;
    case sock_act::ERROR:
        debug_socket_print(sock, "send error\n");
        // Fallthrough
    case sock_act::CLOSE:
        // Fallthrough
        delete this;
        return;
    }

}

////////////////////////////////////////////////////////////////////////////////
TcpServerWorker::TcpServerWorker(TcpServerFactory &factory, TcpServerWorkerParams &params, int sock)
    : TcpWorker(factory, params, sock),
      params(params) {

    debug_print("ctor\n");

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
    debug_print("dtor\n");
}


TcpServerWorker* TcpServerWorker::maker(TcpServerFactory &factory, TcpServerWorkerParams &params, int sock) {

    switch(params.type) {
    case TcpServerWorkerParams::WorkerType::ECHO:
        return new TcpServerEcho(factory, (TcpServerEchoParams&)params, sock);
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
    debug_print("dtor\n");
    ev_timer_stop(factory.loop, &sock_timeout);
}

TcpClientWorker* TcpClientWorker::maker(TcpClientFactory &factory, TcpClientWorkerParams &params) {

    switch(params.type) {
    case TcpClientWorkerParams::WorkerType::ECHO:
        return new TcpClientEcho(factory, (TcpClientEchoParams&)params);
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
    debug_print("timeout_cb\n");
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
    debug_print("ctor\n");
}

TcpServerEcho::~TcpServerEcho() {
    debug_print("dtor\n");
}

void TcpServerEcho::read_cb() {
    debug_print("called\n");
    return TcpWorker::echo();
}

////////////////////////////////////////////////////////////////////////////////
TcpClientEcho::TcpClientEcho(TcpClientFactory &factory, TcpClientEchoParams &params)
    : TcpClientWorker(factory, params),
      params(params) {
    debug_print("ctor\n");
}

TcpClientEcho::~TcpClientEcho() {
    debug_print("dtor\n");
}

void TcpClientEcho::read_cb() {
    debug_print("called\n");
    return TcpWorker::echo();
}
