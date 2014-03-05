#include "events.h"

#define NS_PER_S 1000000000

uint64_t timestamp_ns_now() {
    uint64_t nsec;
    timespec tspec;
    clock_gettime(CLOCK_REALTIME, &tspec);
    nsec = tspec.tv_sec * NS_PER_S + tspec.tv_nsec;
    return nsec;
}

