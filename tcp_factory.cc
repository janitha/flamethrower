#include "tcp_factory.h"
#include "tcp_worker.h"


////////////////////////////////////////////////////////////////////////////////
void FactoryMaker::make(struct ev_loop *loop,
                        factory_params_t *params) {

    switch(params->factory_type) {
    case factory_params_t::TCP_SERVER:
        new TcpServerFactory(loop, &params->tcp_server);
        break;
    case factory_params_t::TCP_CLIENT:
        new TcpClientFactory(loop, &params->tcp_client);
        break;
    default:
        perror("invalid factory type\n");
        exit(EXIT_FAILURE);
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////
TcpFactory::TcpFactory(struct ev_loop *loop,
                       tcp_factory_params_t *params)
    : params(params),
      loop(loop) {

    stats_timer.data = this;
    ev_timer_init(&stats_timer, stats_cb, 0, 1.0);
    ev_timer_start(loop, &stats_timer);

    factory_async.data = this;
    ev_async_init(&factory_async, factory_cb);
    ev_async_start(loop, &factory_async);

    ev_async_send(loop, &factory_async);
}

TcpFactory::~TcpFactory() {
    ev_timer_stop(loop, &stats_timer);
}

void TcpFactory::factory_cb(struct ev_loop *loop,
                            struct ev_async *watcher,
                            int revents) {
    debug_print("called\n");
    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }

    TcpFactory *factory = (TcpFactory*)watcher->data;
    factory->factory_cb();

}

void TcpFactory::factory_cb() {

}

void TcpFactory::stats_cb(struct ev_loop *loop,
                          struct ev_timer *watcher,
                          int revents) {

    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }

    TcpFactory* factory = (TcpFactory*)watcher->data;

    debug_print("workers: %d\n", factory->workers.size());
};

////////////////////////////////////////////////////////////////////////////////
TcpServerFactory::TcpServerFactory(struct ev_loop *loop,
                                   tcp_server_factory_params_t *params)
    : TcpFactory(loop, params),
      params(params) {

    debug_print("ctor\n");

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
    sa_in.sin_port = params->bind_port;
    sa_in.sin_addr.s_addr = params->bind_addr;
    if(bind(accept_sock, (struct sockaddr*)&sa_in, sizeof(sa_in)) != 0) {
        perror("socket bind error");
        exit(EXIT_FAILURE);
    }

    // Listen
    if(listen(accept_sock, params->accept_backlog) < 0) {
        perror("socket listen error");
        exit(EXIT_FAILURE);
    }

    debug_socket_print(accept_sock, "listning\n");

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

    debug_print("called\n");

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

        debug_socket_print(client_sd, "accepted\n");

        // TODO(Janitha): factory should track workers to delete later on
        // TODO(Janitha): Have a function that dynamically generates workers

        TcpServerWorker *worker = new TcpServerWorker(loop,
                                                      factory,
                                                      &factory->params->server_worker,
                                                      client_sd);

    }
}

void TcpServerFactory::factory_cb() {

    debug_print("called\n");

}

////////////////////////////////////////////////////////////////////////////////
TcpClientFactory::TcpClientFactory(struct ev_loop *loop,
                                   tcp_client_factory_params_t *params)
    : TcpFactory(loop, params),
      params(params) {

    debug_print("ctor\n");

}

void TcpClientFactory::factory_cb() {

    debug_print("called\n");

    while(workers.size() <= params->concurrency) {
        TcpClientWorker *tcw = new TcpClientWorker(loop,
                                                   this,
                                                   &params->client_worker);
    }

}


TcpClientFactory::~TcpClientFactory() {
}
