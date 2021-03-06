#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cstdlib>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "common.h"

#include "params.h"


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
    payload_ptr = new char[payload_len];
    memcpy(payload_ptr, str.c_str(), payload_len);
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

    std::stringstream header_ss;
    for(auto &header_pair : ptree.get_child("fields")) {

        header_ss << std::string(header_pair.first.data())
                  << ": "
                  << std::string(header_pair.second.data())
                  << "\r\n";
    }
    payload_len = header_ss.str().size();
    payload_ptr = new char[payload_len];
    memcpy(payload_ptr, header_ss.str().c_str(), payload_len);
}

////////////////////////////////////////////////////////////////////////////////
TcpWorkerParams::TcpWorkerParams(boost::property_tree::ptree &ptree)
    : Params(ptree) {

    tcp_linger = ptree.get<int>("tcp_linger");
    initiate_close = ptree.get<bool>("initiate_close");
    delay_close = ptree.get<float>("delay_close");
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

}

TcpClientRawParams::TcpClientRawParams(boost::property_tree::ptree &ptree)
    : TcpClientWorkerParams(ptree) {

    type = WorkerType::RAW;

    for(auto &payload_pair : ptree.get_child("payloads")) {
        payloads.push_back(PayloadParams::maker(payload_pair.second));
    }

}

TcpServerHttpParams::TcpServerHttpParams(boost::property_tree::ptree &ptree)
    : TcpServerWorkerParams(ptree) {

    type = WorkerType::HTTP;

    for(auto &firstline_payload_pair : ptree.get_child("firstline_payloads")) {
        firstline_payloads.push_back(PayloadParams::maker(firstline_payload_pair.second));
    }

    for(auto &header_payload_pair : ptree.get_child("header_payloads")) {
        header_payloads.push_back(PayloadParams::maker(header_payload_pair.second));
    }

    for(auto &body_payload_pair : ptree.get_child("body_payloads")) {
        body_payloads.push_back(PayloadParams::maker(body_payload_pair.second));
    }

}

TcpClientHttpParams::TcpClientHttpParams(boost::property_tree::ptree &ptree)
    : TcpClientWorkerParams(ptree) {
    type = WorkerType::HTTP;

    for(auto &firstline_payload_pair : ptree.get_child("firstline_payloads")) {
        firstline_payloads.push_back(PayloadParams::maker(firstline_payload_pair.second));
    }

    for(auto &header_payload_pair : ptree.get_child("header_payloads")) {
        header_payloads.push_back(PayloadParams::maker(header_payload_pair.second));
    }

    for(auto &body_payload_pair : ptree.get_child("body_payloads")) {
        body_payloads.push_back(PayloadParams::maker(body_payload_pair.second));
    }

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
    count = ptree.get<uint64_t>("count");
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
StatsParams::StatsParams(boost::property_tree::ptree &ptree)
    : Params(ptree) {

    listener = ptree.get<std::string>("listener");
}

////////////////////////////////////////////////////////////////////////////////
FlamethrowerParams::FlamethrowerParams(boost::property_tree::ptree &ptree)
    : Params(ptree) {

    version = ptree.get<uint32_t>("version");

    stats = new StatsParams(ptree.get_child("stats"));

    for(auto &factory_pair : ptree.get_child("factories")) {
        auto &factory_ptree = factory_pair.second;

        factories.push_back(FactoryParams::maker(factory_ptree));
    }
}
