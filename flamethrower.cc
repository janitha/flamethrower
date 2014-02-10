#include "common.h"

#include "flamethrower.h"
#include "flamethrower_params.h"

#include "tcp_factory.h"


int main(int argc, char *argv[]) {

    struct ev_loop *loop = ev_default_loop(0);

    flamethrower_params_t *params = flamethrower_params_from_file("client.msg");

    params->factory.factory_type = factory_params_t::TCP_SERVER;
    params->factory.tcp_server.bind_addr = htonl(INADDR_ANY);
    params->factory.tcp_server.bind_port = htons(9999);
    params->factory.tcp_server.accept_backlog = 5;
    params->factory.tcp_server.server_worker.stream_work_type = tcp_worker_params_t::ECHO;

    FactoryMaker::make(loop, &params->factory);


    printf("Starting event loop\n");

    while(1) {
        ev_loop(loop, 0);
    }


    /*
    TcpServerFactory *tsf = new TcpServerFactory(loop,
                                                 htonl(INADDR_ANY),
                                                 htons(9999),
                                                 5);

    TcpClientFactory *tcf = new TcpClientFactory(loop,
                                                 htonl(INADDR_LOOPBACK),
                                                 htons(0),
                                                 htonl(INADDR_LOOPBACK),
                                                 htons(5555),
                                                 5);
    */

}
