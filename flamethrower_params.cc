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
TcpWorkerParams::TcpWorkerParams(boost::property_tree::ptree &ptree)
    : Params(ptree) {

    linger = ptree.get<int>("linger");
}

TcpServerWorkerParams* TcpServerWorkerParams::maker(boost::property_tree::ptree &ptree) {

    std::string worker_type = ptree.get<std::string>("type");
    if(worker_type == "echo") {
        return new TcpServerEchoParams(ptree);
    } else if(worker_type == "random") {
        return new TcpServerRandomParams(ptree);
    } else {
        printf("error: invalid factory type\n");
        exit(EXIT_FAILURE);
    }
}

TcpServerWorkerParams::TcpServerWorkerParams(boost::property_tree::ptree &ptree)
    : TcpWorkerParams(ptree) {
}

TcpClientWorkerParams* TcpClientWorkerParams::maker(boost::property_tree::ptree &ptree) {

    std::string worker_type = ptree.get<std::string>("type");
    if(worker_type == "echo") {
        return new TcpClientEchoParams(ptree);
    } else if(worker_type == "random") {
        return new TcpClientRandomParams(ptree);
    } else {
        printf("error: invalid factory type\n");
        exit(EXIT_FAILURE);
    }
}

TcpClientWorkerParams::TcpClientWorkerParams(boost::property_tree::ptree &ptree)
    : TcpWorkerParams(ptree) {
}

TcpServerEchoParams::TcpServerEchoParams(boost::property_tree::ptree &ptree)
    : TcpServerWorkerParams(ptree) {

    type = WorkerType::ECHO;
}

TcpClientEchoParams::TcpClientEchoParams(boost::property_tree::ptree &ptree)
    : TcpClientWorkerParams(ptree) {

    type = WorkerType::ECHO;
}

TcpServerRandomParams::TcpServerRandomParams(boost::property_tree::ptree &ptree)
    : TcpServerWorkerParams(ptree) {

    type = WorkerType::RANDOM;
    bytes = ptree.get<uint32_t>("bytes");
    shutdown = ptree.get<bool>("shutdown");
}

TcpClientRandomParams::TcpClientRandomParams(boost::property_tree::ptree &ptree)
    : TcpClientWorkerParams(ptree) {

    type = WorkerType::RANDOM;
    bytes = ptree.get<uint32_t>("bytes");
    shutdown = ptree.get<bool>("shutdown");
}

////////////////////////////////////////////////////////////////////////////////
FactoryParams* FactoryParams::maker(boost::property_tree::ptree &ptree) {

    std::string factory_type = ptree.get<std::string>("type");
    if(factory_type == "tcp_server") {
        return new TcpServerFactoryParams(ptree);
    } else if (factory_type == "tcp_client") {
        return new TcpClientFactoryParams(ptree);
    } else {
        printf("error: invalid factory type\n");
        exit(EXIT_FAILURE);
    }
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

    worker = TcpServerWorkerParams::maker(ptree.get_child("worker"));
}

TcpClientFactoryParams::TcpClientFactoryParams(boost::property_tree::ptree &ptree)
    : TcpFactoryParams(ptree) {

    type = FactoryType::TCP_CLIENT;
    server_addr = inet_addr(ptree.get<std::string>("server_addr").c_str());
    server_port = htons(ptree.get<uint16_t>("server_port"));
    connect_timeout = ptree.get<float>("connect_timeout");

    worker = TcpClientWorkerParams::maker(ptree.get_child("worker"));
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
