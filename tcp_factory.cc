#include "tcp_factory.h"
#include "tcp_worker.h"


TcpFactory::TcpFactory(struct ev_loop *loop)
    : loop(loop) {
}


TcpFactory::~TcpFactory() {
}


TcpServerFactory::TcpServerFactory(struct ev_loop *loop,
                                   uint32_t bind_addr,
                                   uint16_t bind_port,
                                   uint32_t accept_backlog)
    : TcpFactory(loop) {

    if((accept_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK,
                             IPPROTO_TCP)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    int optval1 = 1;
    setsockopt(accept_sock, SOL_SOCKET, SO_REUSEADDR, &optval1, sizeof(optval1));

    // Bind
    struct sockaddr_in sa_in;
    memset(&sa_in, 0, sizeof(sa_in));
    sa_in.sin_family = AF_INET;
    sa_in.sin_port = bind_port;
    sa_in.sin_addr.s_addr = bind_addr;
    if(bind(accept_sock, (struct sockaddr*)&sa_in, sizeof(sa_in)) != 0) {
        perror("socket bind error");
        exit(EXIT_FAILURE);
    }

    // Listen
    if(listen(accept_sock, accept_backlog) < 0) {
        perror("socket listen error");
        exit(EXIT_FAILURE);
    }

    // Init watcher
    accept_watcher.data = this;
    ev_io_init(&accept_watcher, accept_cb, accept_sock, EV_READ);
    ev_io_start(loop, &accept_watcher);

}


TcpServerFactory::~TcpServerFactory() {
}


void TcpServerFactory::accept_cb(struct ev_loop *loop,
                                 struct ev_io *watcher,
                                 int revents) {

    printf("accept_cb\n");

    TcpServerFactory *factory = (TcpServerFactory*)watcher->data;

    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }

    int client_sd;

    for(;;) {

        // Accept client request
        if((client_sd = accept(watcher->fd, 0, 0)) < 0) {
            if(errno == EWOULDBLOCK || errno == EAGAIN) {
                // Expected behavior for non-blocking io
                break;
            } else {
                perror("socket accept error");
                return;
            }
        }

        printf("accepted connection from client\n");

        // TODO(Janitha): factory should track workers to delete later on
        // TODO(Janitha): Have a function that dynamically generates workers
        TcpServerWorker *worker = new TcpServerWorker(loop, factory, client_sd);

    }
}


TcpClientFactory::TcpClientFactory(struct ev_loop *loop,
                                   uint32_t bind_addr,
                                   uint16_t bind_port,
                                   uint32_t server_addr,
                                   uint16_t server_port,
                                   float    timeout)
    : TcpFactory(loop) {

    // TODO(janitha): Shcedule workers here

    TcpClientWorker *tcw = new TcpClientWorker(loop,
                                               this,
                                               bind_addr,
                                               bind_port,
                                               server_addr,
                                               server_port,
                                               timeout);
}


TcpClientFactory::~TcpClientFactory() {
}
