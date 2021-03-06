#include "tcp_factory.h"
#include "tcp_worker.h"

////////////////////////////////////////////////////////////////////////////////
Factory::Factory(Flamethrower &flamethrower,
                 FactoryParams &params)
    : flamethrower(flamethrower),
      params(params),
      loop(flamethrower.loop) {

    debug_print("ctor\n");

    stats_timer.data = this;
    ev_timer_init(&stats_timer, stats_cb, 0, 1.0);
    ev_set_priority(&stats_timer, EV_MAXPRI);
    ev_timer_start(loop, &stats_timer);

    factory_async.data = this;
    ev_async_init(&factory_async, factory_cb);
    ev_async_start(loop, &factory_async);

    ev_async_send(loop, &factory_async);
}

Factory::~Factory() {
    debug_print("dtor\n");

    ev_async_stop(loop, &factory_async);
    ev_timer_stop(loop, &stats_timer);
}

Factory* Factory::maker(Flamethrower &flamethrower, FactoryParams &params) {

    switch(params.type) {
    case FactoryParams::FactoryType::TCP_SERVER:
        return new TcpServerFactory(flamethrower,
                                    (TcpServerFactoryParams&)params,
                                    *new TcpServerFactoryStats());
        break;
    case FactoryParams::FactoryType::TCP_CLIENT:
        return new TcpClientFactory(flamethrower,
                                    (TcpClientFactoryParams&)params,
                                    *new TcpClientFactoryStats());
        break;
    default:
        perror("error: invalid factory type\n");
        exit(EXIT_FAILURE);
    }
    return nullptr;
}

void Factory::factory_cb(struct ev_loop *loop,
                         struct ev_async *watcher,
                         int revents) {
    debug_print("called\n");
    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }
    Factory* factory = (Factory*)watcher->data;
    factory->factory_cb();
}

void Factory::factory_cb() {
}

void Factory::stats_cb(struct ev_loop *loop,
                       struct ev_timer *watcher,
                       int revents) {
    if(revents & EV_ERROR) {
        perror("invalid event");
        return;
    }
    Factory* factory = (Factory*)watcher->data;
    factory->stats_cb();
}

void Factory::stats_cb() {
}

////////////////////////////////////////////////////////////////////////////////
TcpFactory::TcpFactory(Flamethrower &flamethrower,
                       TcpFactoryParams &params,
                       TcpFactoryStats &stats)
    : Factory(flamethrower, params),
      params(params),
      stats(stats) {

    debug_print("ctor\n");
}

TcpFactory::~TcpFactory() {

    debug_print("dtor\n");

    while(!workers.empty()) {
        delete workers.front();
        workers.pop_front();
    }

    flamethrower.statsbucket.push(&stats);

    delete &stats;
}

void TcpFactory::factory_cb() {
    return Factory::factory_cb();
}

void TcpFactory::stats_cb() {
    stats.active_workers = workers.size();
    stats.print();
    flamethrower.statsbucket.push(&stats);
    return Factory::stats_cb();
}

void TcpFactory::worker_new_cb(TcpWorker &worker) {
    workers.push_back(&worker);
    worker.workers_list_pos = --workers.end();
    stats.cumulative_workers++;
}

void TcpFactory::worker_delete_cb(TcpWorker &worker) {

    // Gather stats from worker
    flamethrower.statsbucket.push(&worker.stats);

    // Remove worker from the worker list
    workers.erase(worker.workers_list_pos);

    // Schedule the factory to wake up
    ev_async_send(loop, &factory_async);
}

////////////////////////////////////////////////////////////////////////////////
TcpServerFactory::TcpServerFactory(Flamethrower &flamethrower,
                                   TcpServerFactoryParams &params,
                                   TcpServerFactoryStats &stats)
    : TcpFactory(flamethrower, params, stats),
      params(params),
      stats(stats) {

    debug_print("ctor\n");

    start_listening();

    accept_watcher.data = this;
    ev_io_init(&accept_watcher, accept_cb, accept_sock, EV_READ);
}

TcpServerFactory::~TcpServerFactory() {

    debug_print("dtor\n");

    ev_io_stop(loop, &accept_watcher);
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
    sa_in.sin_port = params.bind_port;
    sa_in.sin_addr.s_addr = params.bind_addr;
    if(bind(accept_sock, (struct sockaddr*)&sa_in, sizeof(sa_in)) != 0) {
        perror("socket bind error");
        exit(EXIT_FAILURE);
    }

    // Listen
    if(listen(accept_sock, params.accept_backlog) < 0) {
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
    if(stats.cumulative_workers >= params.count) {
        debug_print("cumulative count reached\n");
        ev_io_stop(loop, &accept_watcher);
        ev_async_stop(loop, &factory_async);
        return;
    }

    // TODO(Janitha): Maybe this is better done in the worker_new_cb?
    if(workers.size() >= params.concurrency) {
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

    // New worker
    TcpServerWorker::maker(*this, *params.worker, client_sd);

}

void TcpServerFactory::factory_cb() {

    debug_print("called\n");

    ev_io_start(loop, &accept_watcher);
}

////////////////////////////////////////////////////////////////////////////////
TcpClientFactory::TcpClientFactory(Flamethrower &flamethrower,
                                   TcpClientFactoryParams &params,
                                   TcpClientFactoryStats &stats)
    : TcpFactory(flamethrower, params, stats),
      params(params),
      stats(stats) {

    debug_print("ctor\n");

}

TcpClientFactory::~TcpClientFactory() {

}

void TcpClientFactory::factory_cb() {

    debug_print("called\n");

    // TODO(Janitha): Maybe this is better done in the worker_new_cb?
    if(stats.cumulative_workers >= params.count) {
        debug_print("cumulative count reached\n");
        ev_async_stop(loop, &factory_async);
        return;
    }

    // TODO(Janitha): Maybe this is better done in the worker_new_cb?
    if(workers.size() >= params.concurrency) {
        return;
    }

    create_connection();
    ev_async_send(loop, &factory_async);
}

void TcpClientFactory::create_connection() {

    // New worker
    TcpClientWorker::maker(*this, *params.worker);

}
