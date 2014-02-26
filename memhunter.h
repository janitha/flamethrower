#ifndef MEMHUNTER_H
#define MEMHUNTER_H

#include <cstring>
#include <cstdio>
#include <cstdlib>

class MemHunter {

    // TODO(Janitha): This NEEEEDDSSS some serious testing!!!

    char *needle;
    size_t needlelen;

    char *lookback;
    size_t lookbacklen;

public:

    MemHunter(const char *needlebuf, size_t len) {

        needlelen = len;
        needle = new char[needlelen];
        memcpy(needle, needlebuf, needlelen);

        lookback = new char[needlelen*2];
        lookbacklen = 0;
    }

    inline char* findend(char *haystack, size_t haylen) {

        size_t cpylen = needlelen*2 - lookbacklen;
        cpylen = (cpylen < haylen) ? cpylen : haylen;

        memcpy(lookback+lookbacklen, haystack, cpylen);

        char *found;

        // Find it in lookback
        found = (char*)memmem(lookback, lookbacklen+cpylen, needle, needlelen);
        if(found) {
            return haystack + (found - lookback) - lookbacklen + needlelen;
        }

        // Find it in the haystack
        found = (char*)memmem(haystack, haylen, needle, needlelen);
        if(found) {
            return found + needlelen;
        }

        // Needle wasn't found, let's buffer up and prep for next time

        if(lookbacklen + cpylen <= needlelen) {
            // Lookback is still growing, let it be
            lookbacklen += cpylen;
        } else if(lookbacklen + cpylen < needlelen*2) {
            // The haylen must have been short, shift the lookback until it's needlelen
            size_t shift = lookbacklen + cpylen - needlelen;
            memcpy(lookback, lookback + shift, needlelen);
            lookbacklen = needlelen;
        } else {
            // Haystack was bigger than needle, so just save off the tail of that
            memcpy(lookback, haystack + haylen - needlelen, needlelen);
            lookbacklen = needlelen;
        }

        return nullptr;

    }

    ~MemHunter() {
        delete needle;
        delete lookback;
    }
};

#endif
