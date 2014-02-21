#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <cstdlib>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "common.h"

#include "flamethrower_params.h"


////////////////////////////////////////////////////////////////////////////////
Params::Params(boost::property_tree::ptree &ptree) {
}

////////////////////////////////////////////////////////////////////////////////
PayloadParams::PayloadParams(boost::property_tree::ptree &ptree)
    : Params(ptree) {

}

PayloadParams* PayloadParams::maker(boost::property_tree::ptree &ptree) {

    std::string payload_type = ptree.get<std::string>("type");
    if(payload_type == "random") {
        return new PayloadRandomParams(ptree);
    } else if(payload_type == "file") {
        return new PayloadFileParams(ptree);
    } else if(payload_type == "string") {
        return new PayloadStringParams(ptree);
    } else if(payload_type == "http_headers") {
        return new PayloadHttpHeadersParams(ptree);
    } else {
        printf("error: invalid payload type\n");
        exit(EXIT_FAILURE);
    }

}

PayloadRandomParams::PayloadRandomParams(boost::property_tree::ptree &ptree)
    : PayloadParams(ptree) {

    type = PayloadType::RANDOM;

    payload_len = ptree.get<size_t>("length");

    for(size_t i = 0; i<RANDOM_BUF_SIZE; i++) {
        // Random printable ascii
        random_buf[i] = '!' + (std::rand()%93);
    }

}

PayloadBufAbstractParams::PayloadBufAbstractParams(boost::property_tree::ptree &ptree)
    : PayloadParams(ptree) {
}

PayloadStringParams::PayloadStringParams(boost::property_tree::ptree &ptree)
    : PayloadBufAbstractParams(ptree) {

    type = PayloadType::STRING;

    std::string str = ptree.get<std::string>("string");
    payload_len = str.size();
    payload_ptr = new char[str.size()];
    memcpy(payload_ptr, str.c_str(), str.size());
}

PayloadFileParams::PayloadFileParams(boost::property_tree::ptree &ptree)
    : PayloadBufAbstractParams(ptree) {

    type = PayloadType::FILE;

    std::ifstream payload_file(ptree.get<std::string>("filename"), std::ios::in | std::ios::binary);
    if(!payload_file) {
        perror("error: invalid payload file");
        exit(EXIT_FAILURE);
    }
    payload_file.seekg(0, payload_file.end);
    payload_len = payload_file.tellg();
    payload_file.seekg(0, payload_file.beg);
    payload_ptr = new char[payload_len]; // TODO(Janitha): Cleanup in dtor
    payload_file.read(payload_ptr, payload_len);
}

PayloadHttpHeadersParams::PayloadHttpHeadersParams(boost::property_tree::ptree &ptree)
    : PayloadBufAbstractParams(ptree) {

    type = PayloadType::HTTP_HEADERS;
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
    } else if(worker_type == "raw") {
        return new TcpServerRawParams(ptree);
    } else if(worker_type == "http") {
        return new TcpServerHttpParams(ptree);
    } else {
        printf("error: invalid worker type\n");
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
    } else if(worker_type == "raw") {
        return new TcpClientRawParams(ptree);
    } else if(worker_type == "http") {
        return new TcpClientHttpParams(ptree);
    } else {
        printf("error: invalid worker type\n");
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

TcpServerRawParams::TcpServerRawParams(boost::property_tree::ptree &ptree)
    : TcpServerWorkerParams(ptree) {

    type = WorkerType::RAW;

    for(auto &payload_pair : ptree.get_child("payloads")) {
        payloads.push_back(PayloadParams::maker(payload_pair.second));
    }

    shutdown = ptree.get<bool>("shutdown");
}

TcpClientRawParams::TcpClientRawParams(boost::property_tree::ptree &ptree)
    : TcpClientWorkerParams(ptree) {

    type = WorkerType::RAW;

    for(auto &payload_pair : ptree.get_child("payloads")) {
        payloads.push_back(PayloadParams::maker(payload_pair.second));
    }

    shutdown = ptree.get<bool>("shutdown");
}

TcpServerHttpParams::TcpServerHttpParams(boost::property_tree::ptree &ptree)
    : TcpServerWorkerParams(ptree) {

    type = WorkerType::HTTP;

    std::ifstream header_file(ptree.get<std::string>("header_payload"), std::ios::in | std::ios::binary);
    if(!header_file) {
        perror("error: invalid http response header file");
        exit(EXIT_FAILURE);
    }
    header_file.seekg(0, header_file.end);
    header_payload_len = header_file.tellg();
    header_file.seekg(0, header_file.beg);
    header_payload_ptr = new char[header_payload_len];
    header_file.read(header_payload_ptr, header_payload_len);

    std::ifstream body_file(ptree.get<std::string>("body_payload"), std::ios::in | std::ios::binary);
    if(!body_file) {
        perror("error: invalid http response body file");
        exit(EXIT_FAILURE);
    }
    body_file.seekg(0, body_file.end);
    body_payload_len = body_file.tellg();
    body_file.seekg(0, body_file.beg);
    body_payload_ptr = new char[body_payload_len];
    body_file.read(body_payload_ptr, body_payload_len);

}

TcpClientHttpParams::TcpClientHttpParams(boost::property_tree::ptree &ptree)
    : TcpClientWorkerParams(ptree) {
    type = WorkerType::HTTP;
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
