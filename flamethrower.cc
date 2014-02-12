#include "flamethrower.h"

Flamethrower::Flamethrower(char *params_filename) {

    loop = ev_loop_new(EVFLAG_AUTO);

    flamethrower_params_from_file(params_filename, &params);

    /*
    params.factory.factory_type = factory_params_t::TCP_SERVER;
    params.factory.tcp_server.bind_addr = htonl(INADDR_ANY);
    params.factory.tcp_server.bind_port = htons(5555);
    params.factory.tcp_server.accept_backlog = 5;
    params.factory.tcp_server.server_worker.stream_work_type = tcp_worker_params_t::ECHO;
    params.factory.tcp_server.count = 3;
    /**/

    //*
    params.factory.factory_type = factory_params_t::TCP_CLIENT;

    params.factory.tcp_client.concurrency = 10;
    params.factory.tcp_client.count = 10;

    params.factory.tcp_client.bind_addr = htonl(INADDR_ANY);
    params.factory.tcp_client.bind_port = htons(0);
    params.factory.tcp_client.server_addr = inet_addr("127.0.0.1");
    params.factory.tcp_client.server_port = htons(5555);
    //params.factory.tcp_client.connect_timeout = 1.0;

    params.factory.tcp_client.client_worker.stream_work_type = tcp_worker_params_t::RANDOM;
    params.factory.tcp_client.client_worker.linger = false;
    params.factory.tcp_client.client_worker.random_work.bytes = 10;
    params.factory.tcp_client.client_worker.random_work.shutdown = true;
    /**/

    FactoryMaker::make(loop, &params.factory);

}

Flamethrower::~Flamethrower() {
    ev_loop_destroy(loop);
}

void Flamethrower::loop_iteration() {
    ev_loop(loop, 0);
}
