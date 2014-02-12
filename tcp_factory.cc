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
    : loop(loop),
      params(params) {

    stats_timer.data = this;
    ev_timer_init(&stats_timer, stats_cb, 0, 1.0);
    ev_timer_start(loop, &stats_timer);

    factory_async.data = this;
    ev_async_init(&factory_async, factory_cb);
    ev_async_start(loop, &factory_async);

    ev_async_send(loop, &factory_async);
}

TcpFactory::~TcpFactory() {

    ev_async_stop(loop, &factory_async);
    ev_timer_stop(loop, &stats_timer);

    // TODO(Janitha): Free the workers
}

void TcpFactory::factory_cb(struct ev_loop *loop,
                            struct ev_async *watcher,
                            int revents) {
    debug_print("called\n");
    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }
    TcpFactory* factory = (TcpFactory*)watcher->data;
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
    factory->stats_cb();
}

void TcpFactory::stats_cb() {
    debug_print("workers: %d\n", workers.size());
}

void TcpFactory::worker_new_cb(TcpWorker *worker) {
    workers.push_back(worker);
    worker->workers_it = --workers.end();
    cumulative_count++;
}

void TcpFactory::worker_delete_cb(TcpWorker *worker) {
    workers.erase(worker->workers_it);
    ev_async_send(loop, &factory_async);
}

////////////////////////////////////////////////////////////////////////////////
TcpServerFactory::TcpServerFactory(struct ev_loop *loop,
                                   tcp_server_factory_params_t *params)
    : TcpFactory(loop, params),
      params(params) {

    debug_print("ctor\n");

    start_listening();

    accept_watcher.data = this;
    ev_io_init(&accept_watcher, accept_cb, accept_sock, EV_READ);
}

TcpServerFactory::~TcpServerFactory() {
    // TODO(Janitha): Free the workers
}

void TcpServerFactory::start_listening() {

    if((accept_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK,
                             IPPROTO_TCP)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    int optval1 = 1;
    if(setsockopt(accept_sock, SOL_SOCKET, SO_REUSEADDR, &optval1, sizeof(optval1)) < 0) {
        perror("setsockopt error");
        exit(EXIT_FAILURE);
    }

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

}

void TcpServerFactory::accept_cb(struct ev_loop *loop,
                                 struct ev_io *watcher,
                                 int revents) {
    debug_print("called\n");

    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }

    TcpServerFactory *factory = (TcpServerFactory*)watcher->data;
    factory->accept_cb();
}

void TcpServerFactory::accept_cb() {

    // TODO(Janitha): Maybe this is better done in the worker_new_cb?
    if(cumulative_count >= params->count) {
        debug_print("cumulative count reached\n");
        ev_io_stop(loop, &accept_watcher);
        ev_async_stop(loop, &factory_async);
        return;
    }

    // TODO(Janitha): Maybe this is better done in the worker_new_cb?
    if(workers.size() >= params->concurrency) {
        ev_io_stop(loop, &accept_watcher);
        return;
    }

    // Accept client request
    int client_sd;
    if((client_sd = accept(accept_sock, 0, 0)) < 0) {
        if(errno == EWOULDBLOCK || errno == EAGAIN) {
            // Expected behavior for non-blocking io
            return;
        } else {
            perror("socket accept error");
            return;
        }
    }

    debug_socket_print(client_sd, "accepted\n");

    TcpServerWorker *worker = new TcpServerWorker(loop, this,
                                                  &params->server_worker,
                                                  client_sd);
}

void TcpServerFactory::factory_cb() {

    debug_print("called\n");

    ev_io_start(loop, &accept_watcher);
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

    // TODO(Janitha): Maybe this is better done in the worker_new_cb?
    if(cumulative_count >= params->count) {
        debug_print("cumulative count reached\n");
        ev_async_stop(loop, &factory_async);
        return;
    }

    // TODO(Janitha): Maybe this is better done in the worker_new_cb?
    if(workers.size() >= params->concurrency) {
        return;
    }

    create_connection();
    ev_async_send(loop, &factory_async);
}

void TcpClientFactory::create_connection() {

    TcpClientWorker *tcw = new TcpClientWorker(loop, this,
                                               &params->client_worker);
}

TcpClientFactory::~TcpClientFactory() {
    // TODO(Janitha): Free the workers
}
