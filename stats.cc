#include "stats.h"

////////////////////////////////////////////////////////////////////////////////
#define NS_PER_S 1000000000

uint64_t timestamp_ns_now() {
    uint64_t nsec;
    timespec tspec;
    clock_gettime(CLOCK_REALTIME, &tspec);
    nsec = tspec.tv_sec * NS_PER_S + tspec.tv_nsec;
    return nsec;
}

////////////////////////////////////////////////////////////////////////////////
StatsCollector::StatsCollector(StatsParams &params)
    : params(params),
      zcontext(1),
      zpubsocket(zcontext, ZMQ_PUB) {

    debug_print("StatsCollector listening on %s\n", params.listener.c_str());
    zpubsocket.bind(params.listener.c_str());
}

StatsCollector::~StatsCollector() {
}

void StatsCollector::push(Stats *stats) {

    msgpack::sbuffer sbuffer;
    msgpack::packer<msgpack::sbuffer> packer(sbuffer);

    stats->serialize(packer);

    zmq::message_t msg(sbuffer.size());
    memcpy(msg.data(), sbuffer.data(), sbuffer.size());
    zpubsocket.send(msg);
}

////////////////////////////////////////////////////////////////////////////////
Stats::Stats()
    : version(1) {
}

Stats::~Stats() {
}

void Stats::print() {
}

void Stats::serialize(msgpack::packer<msgpack::sbuffer> &packer) {
    packer.pack(std::string("stats"));
    packer.pack(version);
    packer.pack(timestamp_ns_now());
}

////////////////////////////////////////////////////////////////////////////////
TcpFactoryStats::TcpFactoryStats()
    : Stats(),
      bytes_in(0),
      bytes_out(0),
      cumulative_workers(0),
      active_workers(0) {
}

TcpFactoryStats::~TcpFactoryStats() {
}

void TcpFactoryStats::print() {
    printf("bytes_in=%lu "
           "bytes_out=%lu "
           "cumulative_workers=%lu "
           "active_workers=%lu "
           "\n",
           bytes_in,
           bytes_out,
           cumulative_workers,
           active_workers);
}

void TcpFactoryStats::serialize(msgpack::packer<msgpack::sbuffer> &packer) {
    Stats::serialize(packer);
    packer.pack(std::string("tcpfactory"));
    packer.pack(bytes_in);
    packer.pack(bytes_out);
    packer.pack(cumulative_workers);
    packer.pack(active_workers);
}

////////////////////////////////////////////////////////////////////////////////
TcpWorkerStats::TcpWorkerStats()
    : Stats(),
      readable_time(0),
      writable_time(0),
      close_time(0),
      bytes_in(0),
      bytes_out(0) {
}

TcpWorkerStats::~TcpWorkerStats() {
}

void TcpWorkerStats::print() {
}

void TcpWorkerStats::serialize(msgpack::packer<msgpack::sbuffer> &packer) {
    Stats::serialize(packer);
    packer.pack(std::string("tcpworker"));
    packer.pack(readable_time);
    packer.pack(writable_time);
    packer.pack(close_time);
    packer.pack(bytes_in);
    packer.pack(bytes_out);
}

////////////////////////////////////////////////////////////////////////////////
TcpServerWorkerStats::TcpServerWorkerStats()
    : TcpWorkerStats(),
      established_time(0) {
}

TcpServerWorkerStats::~TcpServerWorkerStats() {

}

void TcpServerWorkerStats::print() {
    printf("readable=%luns "
           "writable=%luns "
           "close=%luns "
           "\n",
           readable_time ? readable_time - established_time : 0,
           writable_time ? writable_time - established_time : 0,
           close_time - established_time);
}

void TcpServerWorkerStats::serialize(msgpack::packer<msgpack::sbuffer> &packer) {
    TcpWorkerStats::serialize(packer);
    packer.pack(std::string("server"));
    packer.pack(established_time);
}

////////////////////////////////////////////////////////////////////////////////
TcpClientWorkerStats::TcpClientWorkerStats()
    : TcpWorkerStats(),
      connect_time(0),
      established_time(0) {

}

TcpClientWorkerStats::~TcpClientWorkerStats() {

}

void TcpClientWorkerStats::print() {

    printf("connect=%luns "
           "readable=%luns "
           "writable=%luns "
           "close=%luns "
           "\n",
           established_time ? established_time - connect_time : 0,
           readable_time ? readable_time - established_time : 0,
           writable_time ? writable_time - established_time : 0,
           close_time - connect_time);
}

void TcpClientWorkerStats::serialize(msgpack::packer<msgpack::sbuffer> &packer) {
    TcpWorkerStats::serialize(packer);
    packer.pack(std::string("client"));
    packer.pack(connect_time);
    packer.pack(established_time);
}
