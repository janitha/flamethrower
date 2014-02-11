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
    */

    params.factory.factory_type = factory_params_t::TCP_CLIENT;

    params.factory.tcp_client.concurrency = 2;

    params.factory.tcp_client.client_worker.bind_addr = htonl(INADDR_ANY);
    params.factory.tcp_client.client_worker.bind_port = htons(0);
    params.factory.tcp_client.client_worker.server_addr = htonl(INADDR_LOOPBACK);
    params.factory.tcp_client.client_worker.server_port = htons(5555);
    params.factory.tcp_client.client_worker.stream_work_type = tcp_worker_params_t::RANDOM;
    params.factory.tcp_client.client_worker.linger = false;

    params.factory.tcp_client.client_worker.random_work.bytes = -1;
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
