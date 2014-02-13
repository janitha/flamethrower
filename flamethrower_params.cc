#include <iostream>
#include <string>
#include <map>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "common.h"

#include "flamethrower_params.h"


////////////////////////////////////////////////////////////////////////////////
Params::Params(boost::property_tree::ptree &ptree) {
}


////////////////////////////////////////////////////////////////////////////////
StreamWorkParams* StreamWorkParams::maker(boost::property_tree::ptree &ptree) {

    StreamWorkParams* work;
    std::string work_type = ptree.get<std::string>("type");

    if(work_type == "echo") {
        work = new StreamWorkEchoParams(ptree);
    } else if(work_type == "random") {
        work = new StreamWorkRandomParams(ptree);
    } else if(work_type == "http_client" ) {
        work = new StreamWorkHttpClientParams(ptree);
    } else {
        printf("error: invalid factory type\n");
        exit(EXIT_FAILURE);
    }
    return work;
}

StreamWorkParams::StreamWorkParams(boost::property_tree::ptree &ptree)
    : Params(ptree) {
}

StreamWorkEchoParams::StreamWorkEchoParams(boost::property_tree::ptree &ptree)
    : StreamWorkParams(ptree) {

    type = StreamWorkType::ECHO;
}

StreamWorkRandomParams::StreamWorkRandomParams(boost::property_tree::ptree &ptree)
    : StreamWorkParams(ptree) {

    type = StreamWorkType::RANDOM;
    bytes = ptree.get<uint32_t>("bytes");
    shutdown = ptree.get<bool>("shutdown");
}

StreamWorkHttpClientParams::StreamWorkHttpClientParams(boost::property_tree::ptree &ptree)
    : StreamWorkParams(ptree) {

    type = StreamWorkType::HTTP_CLIENT;
}

////////////////////////////////////////////////////////////////////////////////
TcpWorkerParams::TcpWorkerParams(boost::property_tree::ptree &ptree)
    : Params(ptree) {

    linger = ptree.get<int>("linger");

    work = StreamWorkParams::maker(ptree.get_child("work"));
}

TcpServerWorkerParams::TcpServerWorkerParams(boost::property_tree::ptree &ptree)
    : TcpWorkerParams(ptree) {
}

TcpClientWorkerParams::TcpClientWorkerParams(boost::property_tree::ptree &ptree)
    : TcpWorkerParams(ptree) {
}

////////////////////////////////////////////////////////////////////////////////
FactoryParams* FactoryParams::maker(boost::property_tree::ptree &ptree) {

    FactoryParams *factory;
    std::string factory_type = ptree.get<std::string>("type");

    if(factory_type == "tcp_server") {
        factory = new TcpServerFactoryParams(ptree);
    } else if (factory_type == "tcp_client") {
        factory = new TcpClientFactoryParams(ptree);
    } else {
        printf("error: invalid factory type\n");
        exit(EXIT_FAILURE);
    }
    return factory;
}

FactoryParams::FactoryParams(boost::property_tree::ptree &ptree)
    : Params(ptree) {
}

TcpFactoryParams::TcpFactoryParams(boost::property_tree::ptree &ptree)
    : FactoryParams(ptree) {

    bind_addr = inet_addr(ptree.get<std::string>("bind_addr").c_str());
    bind_port = htons(ptree.get<uint16_t>("bind_port"));
    concurrency  = ptree.get<uint32_t>("concurrency");
    count = ptree.get<uint32_t>("count");
}

TcpServerFactoryParams::TcpServerFactoryParams(boost::property_tree::ptree &ptree)
    : TcpFactoryParams(ptree) {

    type = FactoryType::TCP_SERVER;
    accept_backlog = ptree.get<uint32_t>("accept_backlog");

    worker = new TcpServerWorkerParams(ptree.get_child("worker"));
}

TcpClientFactoryParams::TcpClientFactoryParams(boost::property_tree::ptree &ptree)
    : TcpFactoryParams(ptree) {

    type = FactoryType::TCP_CLIENT;
    server_addr = inet_addr(ptree.get<std::string>("server_addr").c_str());
    server_port = htons(ptree.get<uint16_t>("server_port"));
    connect_timeout = ptree.get<float>("connect_timeout");

    worker = new TcpClientWorkerParams(ptree.get_child("worker"));
}

////////////////////////////////////////////////////////////////////////////////
FlamethrowerParams::FlamethrowerParams(boost::property_tree::ptree &ptree)
    : Params(ptree) {

    version = ptree.get<uint32_t>("version");

    for(auto &factory_pair : ptree.get_child("factories")) {
        auto &factory_ptree = factory_pair.second;

        factories.push_back(FactoryParams::maker(factory_ptree));
    }
}
