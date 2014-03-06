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

}

////////////////////////////////////////////////////////////////////////////////
TcpClientWorkerStats::TcpClientWorkerStats()
    : TcpWorkerStats() {

}

TcpClientWorkerStats::~TcpClientWorkerStats() {

}

void TcpClientWorkerStats::print() {

}
