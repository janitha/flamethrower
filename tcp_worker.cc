#include "tcp_worker.h"


////////////////////////////////////////////////////////////////////////////////
TcpWorker::TcpWorker(struct ev_loop *loop,
                     TcpFactory *factory,
                     tcp_worker_params_t *params,
                     int sock)
    : params(params),
      loop(loop),
      factory(factory),
      sock(sock) {
}


TcpWorker::~TcpWorker() {
    ev_io_stop(loop, &sock_r_ev);
    ev_io_stop(loop, &sock_w_ev);
    if(work) {
        delete work;
    }
}


void TcpWorker::read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {

    debug_print("called\n");

    TcpWorker *worker = (TcpWorker*)watcher->data;

    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }

    char recvbuf[RECVBUF_SIZE];
    ssize_t recvlen;

    // Read
    memset(recvbuf, 0, sizeof(recvbuf));
    if((recvlen = recv(worker->sock, recvbuf, RECVBUF_SIZE, 0)) < 0) {
        if(errno == EWOULDBLOCK || errno == EAGAIN) {
            // Expected behavior for non-blocking io
        } else {
            perror("socket recv error");
            return;
        }
    }

    // Handle closing socket
    if(recvlen == 0) {

        debug_socket_print(worker->sock, "disconnected\n");

        delete (TcpWorker*)worker; // TODO(Janitha): is this sane? what if the write side isn't done?
        return;
    }

    int hret;
    if((hret=worker->work->read_handler(recvbuf, recvlen)) < 0) {

        perror("work read_handler error");
        exit(EXIT_FAILURE);
        // TODO(Janitha): Shutdown socket and kill the worker
    }

    if(hret == STREAMWORK_FINISHED) {
        ev_io_stop(loop, watcher);
    }

    if(hret == STREAMWORK_SHUTDOWN) {

        debug_socket_print(worker->sock, "shutdown\n");

        ev_io_stop(loop, watcher);
        if(shutdown(worker->sock, SHUT_RDWR) < 0) {
            perror("socket shutdown error");
            exit(EXIT_FAILURE);
        }
        //delete (TcpWorker*)worker; // TODO(Janitha): is this sane? what if the write side isn't done?
    }
}

void TcpWorker::write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {

    debug_print("called\n");

    TcpWorker *worker = (TcpWorker*)watcher->data;

    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }

    // TODO(Janitha): Test socket if it's writable and stuff

    char sendbuf[SENDBUF_SIZE];
    ssize_t sendlen = sizeof(sendbuf);

    memset(sendbuf, 0, sizeof(sendbuf));

    int hret;
    if((hret = worker->work->write_handler(sendbuf, sendlen)) < 0) {
        // TODO(Janitha): Lets make this a bit more graceful
        perror("work write_handler error");
        exit(EXIT_FAILURE);
    }

    // TODO(Janitha): We can do TCP_CORK and TCP_NODELAY
    if(send(worker->sock, sendbuf, sendlen, 0) < 0) {
        // TODO(Janitha): Handle non blocking send conditions
        perror("send error");
        exit(EXIT_FAILURE);
    }

    if(hret == STREAMWORK_FINISHED) {
        ev_io_stop(loop, watcher);
    }

    if(hret == STREAMWORK_SHUTDOWN) {

        debug_socket_print(worker->sock, "shutdown-half\n");

        ev_io_stop(loop, watcher);
        shutdown(worker->sock, SHUT_WR);
        //delete (TcpWorker*)worker; // TODO(Janitha): is this sane? what is the read side isn't done?
    }
}

////////////////////////////////////////////////////////////////////////////////
TcpServerWorker::TcpServerWorker(struct ev_loop *loop,
                                 TcpFactory *factory,
                                 tcp_server_worker_params_t *params,
                                 int sock)
    : TcpWorker(loop, factory, params, sock),
      params(params) {

    work = StreamWorkMaker::make(params, sock);

    // Hookup socket readable event
    sock_r_ev.data = this;
    ev_io_init(&sock_r_ev, read_cb, sock, EV_READ);
    ev_io_start(factory->loop, &sock_r_ev);

    // Hookup socket writable event
    sock_w_ev.data = this;
    ev_io_init(&sock_w_ev, write_cb, sock, EV_WRITE);
    ev_io_start(factory->loop, &sock_w_ev);
}

TcpServerWorker::~TcpServerWorker() {

}

////////////////////////////////////////////////////////////////////////////////
TcpClientWorker::TcpClientWorker(struct ev_loop *loop,
                                 TcpFactory *factory,
                                 tcp_client_worker_params_t *params)
    : TcpWorker(loop, factory, params),
      params(params) {

    debug_print("ctor\n");

    // Create
    if((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK,
                      IPPROTO_TCP)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // Bind to local
    struct sockaddr_in sa_bind;
    memset(&sa_bind, 0, sizeof(sa_bind));
    sa_bind.sin_family = AF_INET;
    sa_bind.sin_port = params->bind_port;
    sa_bind.sin_addr.s_addr = params->bind_addr;
    if(bind(sock, (struct sockaddr*)&sa_bind, sizeof(sa_bind)) != 0) {
        perror("socket bind error");
        exit(EXIT_FAILURE);
    }

    // Connect to remote
    struct sockaddr_in sa_connect;
    memset(&sa_connect, 0, sizeof(sa_connect));
    sa_connect.sin_family = AF_INET;
    sa_connect.sin_port = params->server_port;
    sa_connect.sin_addr.s_addr = params->server_addr;
    if(connect(sock, (struct sockaddr *)&sa_connect, sizeof(sa_connect)) < 0) {
        if(errno != EINPROGRESS) {
            perror("socket connect error");
            exit(EXIT_FAILURE);
        }
    }

    debug_socket_print(sock, "connecting\n");

    work = StreamWorkMaker::make(params, sock);

    // TODO(Janitha): connect_cb be read, write or both?
    // Hookup socket connected event
    sock_w_ev.data = this;
    ev_io_init(&sock_w_ev, connect_cb, sock, EV_READ|EV_WRITE);
    ev_io_start(factory->loop, &sock_w_ev);

    /* TODO(Janitha): Implement the timeout_cb and reanable this
    // Hookup timeout event for socket
    sock_timeout.data = this;
    ev_timer_init(&sock_timeout, timeout_cb, timeout, 0);
    ev_timer_start(loop, &sock_timeout);
    */
}


TcpClientWorker::~TcpClientWorker() {
    ev_timer_stop(loop, &sock_timeout);
}


void TcpClientWorker::connect_cb(struct ev_loop *loop,
                                 struct ev_io *watcher,
                                 int revents) {

    debug_print("called\n");

    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }

    TcpClientWorker *worker = (TcpClientWorker*)watcher->data;

    int error;
    socklen_t errsz = sizeof(error);
    if(getsockopt(worker->sock, SOL_SOCKET, SO_ERROR, &error, &errsz) == -1) {
        perror("getsockopt error");
        exit(EXIT_FAILURE);
    }

    ev_io_stop(loop, &worker->sock_r_ev);
    ev_io_stop(loop, &worker->sock_w_ev);
    ev_timer_stop(loop, &worker->sock_timeout);

    if(error) {
        fprintf(stderr, "connect error: %s\n", strerror(error));
        delete (TcpClientWorker*)worker;
        return;
    }

    debug_socket_print(worker->sock, "connection established\n");

    worker->sock_r_ev.data = worker;
    ev_io_init(&worker->sock_r_ev, read_cb, worker->sock, EV_READ);
    ev_io_start(loop, &worker->sock_r_ev);

    worker->sock_w_ev.data = worker;
    ev_io_init(&worker->sock_w_ev, write_cb, worker->sock, EV_WRITE);
    ev_io_start(loop, &worker->sock_w_ev);

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

    // TODO(Janitha): Implement the timeout logic and cleanup!!
}
