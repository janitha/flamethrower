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
StatsList::StatsList() {
}

StatsList::~StatsList() {
}

void StatsList::push(Stats *stats) {
    statslist.push_back(stats);
    stats->print();
}

////////////////////////////////////////////////////////////////////////////////
Stats::Stats() {

}

Stats::~Stats() {

}

void Stats::print() {

}

////////////////////////////////////////////////////////////////////////////////
TcpFactoryStats::TcpFactoryStats()
    : Stats() {
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

////////////////////////////////////////////////////////////////////////////////
TcpWorkerStats::TcpWorkerStats()
    : Stats() {
}

TcpWorkerStats::~TcpWorkerStats() {
}

void TcpWorkerStats::print() {
}

////////////////////////////////////////////////////////////////////////////////
TcpServerWorkerStats::TcpServerWorkerStats()
    : TcpWorkerStats() {

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

////////////////////////////////////////////////////////////////////////////////
TcpClientWorkerStats::TcpClientWorkerStats()
    : TcpWorkerStats() {

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
