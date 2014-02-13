#include "tcp_worker.h"

////////////////////////////////////////////////////////////////////////////////
TcpWorker::TcpWorker(struct ev_loop *loop,
                     TcpFactory &factory,
                     TcpWorkerParams &params,
                     int sock)
    : loop(loop),
      factory(factory),
      params(params),
      sock(sock) {

    debug_print("ctor\n");

    factory.worker_new_cb(*this);
}


TcpWorker::~TcpWorker() {

    debug_print("dtor\n");

    ev_io_stop(loop, &sock_r_ev);
    ev_io_stop(loop, &sock_w_ev);

    if(work) {
        delete work;
    }

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
    ssize_t recvlen;

    // Read
    memset(recvbuf, 0, sizeof(recvbuf));
    if((recvlen = recv(sock, recvbuf, RECVBUF_SIZE, 0)) < 0) {
        if(errno == EWOULDBLOCK || errno == EAGAIN) {
            // Expected behavior for non-blocking io
        } else {
            perror("socket recv error");
            return;
        }
    }

    // Handle closing socket
    if(recvlen == 0) {
        debug_socket_print(sock, "disconnected\n");

        delete this; // YOLO!
        return;
    }

    char sendbuf[SENDBUF_SIZE];
    size_t sendlen = sizeof(sendbuf);

    int hret;
    if((hret=work->handler(recvbuf, recvlen, sendbuf, sendlen)) < 0) {
        perror("work handler error");
        exit(EXIT_FAILURE);
        // TODO(Janitha): Shutdown socket and kill the worker instead of exit
    }

    if(sendlen) {
        // TODO(Janitha): Abstract the send to a single location (between cbs)
        if(send(sock, sendbuf, sendlen, MSG_NOSIGNAL) < 0) {
            if(errno == EPIPE) {
                perror("socket error: broken pipe");
            } else {
                // TODO(Janitha): Handle non blocking send conditions
                perror("send error");
                exit(EXIT_FAILURE);
            }
        }
    }

    if(hret == STREAMWORK_FINISHED) {
        ev_io_stop(loop, &sock_r_ev);
    }

    if(hret == STREAMWORK_SHUTDOWN) {

        debug_socket_print(sock, "shutdown\n");

        ev_io_stop(loop, &sock_r_ev);
        if(shutdown(sock, SHUT_RDWR) < 0) {
            perror("socket shutdown error");
            exit(EXIT_FAILURE);
        }
        //delete (TcpWorker*)worker; // TODO(Janitha): is this sane? what if the write side isn't done?
    }
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

    // TODO(Janitha): Test socket if it's writable and stuff

    debug_socket_print(sock, "called\n");

    char sendbuf[SENDBUF_SIZE];
    size_t sendlen = sizeof(sendbuf);

    memset(sendbuf, 0, sizeof(sendbuf));

    int hret;
    if((hret = work->handler(nullptr, 0, sendbuf, sendlen)) < 0) {
        // TODO(Janitha): Lets make this a bit more graceful
        perror("work handler error");
        exit(EXIT_FAILURE);
    }

    // TODO(Janitha): We can do TCP_CORK and TCP_NODELAY
    if(send(sock, sendbuf, sendlen, MSG_NOSIGNAL) < 0) {
        if(errno == EPIPE) {
            perror("socket error: broken pipe");
        } else {
            // TODO(Janitha): Handle non blocking send conditions
            perror("send error");
            exit(EXIT_FAILURE);
        }
    }

    if(hret == STREAMWORK_FINISHED) {
        debug_socket_print(sock, "finished\n");
        ev_io_stop(loop, &sock_w_ev);
        return;
    }

    if(hret == STREAMWORK_SHUTDOWN) {
        debug_socket_print(sock, "shutdown\n");
        ev_io_stop(loop, &sock_w_ev);
        //shutdown(sock, SHUT_WR);
        delete this;
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////
TcpServerWorker::TcpServerWorker(struct ev_loop *loop,
                                 TcpServerFactory &factory,
                                 TcpServerWorkerParams &params,
                                 int sock)
    : TcpWorker(loop, factory, params, sock),
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

    // Create a worker
    work = StreamWork::make(*params.work);

    // Hookup socket readable event
    sock_r_ev.data = this;
    ev_io_init(&sock_r_ev, read_cb, sock, EV_READ);
    ev_io_start(loop, &sock_r_ev);

    // Hookup socket writable event
    sock_w_ev.data = this;
    ev_io_init(&sock_w_ev, write_cb, sock, EV_WRITE);
    ev_io_start(loop, &sock_w_ev);
}

TcpServerWorker::~TcpServerWorker() {
    debug_print("dtor\n");
}

////////////////////////////////////////////////////////////////////////////////
TcpClientWorker::TcpClientWorker(struct ev_loop *loop,
                                 TcpClientFactory &factory,
                                 TcpClientWorkerParams &params)
    : TcpWorker(loop, factory, params),
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

    work = StreamWork::make(*params.work);

    // Hookup socket connected event
    sock_w_ev.data = this;
    ev_io_init(&sock_w_ev, connected_cb, sock, EV_READ|EV_WRITE);
    ev_io_start(loop, &sock_w_ev);

    // Initialize the sock_r_ev, but don't stop yet
    sock_r_ev.data = this;
    ev_io_init(&sock_r_ev, read_cb, sock, EV_READ);

    // TODO(Janitha): Implement the timeout_cb and reanable this
    // Hookup timeout event for socket
    sock_timeout.data = this;
    ev_timer_init(&sock_timeout, timeout_cb, factory.params.connect_timeout, 0);
    ev_timer_start(loop, &sock_timeout);
}


TcpClientWorker::~TcpClientWorker() {
    debug_print("dtor\n");
    ev_timer_stop(loop, &sock_timeout);
}


void TcpClientWorker::connected_cb(struct ev_loop *loop,
                                 struct ev_io *watcher,
                                 int revents) {
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

    ev_io_stop(loop, &sock_w_ev);
    ev_io_stop(loop, &sock_r_ev);
    ev_timer_stop(loop, &sock_timeout);

    if(error) {
        fprintf(stderr, "connect error: %s\n", strerror(error));
        delete this; // I too like to live dangerously
        return;
    }

    debug_socket_print(sock, "connection established\n");

    sock_r_ev.data = this;
    ev_io_init(&sock_r_ev, read_cb, sock, EV_READ);
    ev_io_start(loop, &sock_r_ev);

    sock_w_ev.data = this;
    ev_io_init(&sock_w_ev, write_cb, sock, EV_WRITE);
    ev_io_start(loop, &sock_w_ev);
}

void TcpClientWorker::timeout_cb(struct ev_loop *loop,
                                 struct ev_timer *watcher,
                                 int revents) {
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
