#include <iostream>

#include "memhunter.h"

int main(void) {

    char needle[] = "abcd";
    char needlelen = sizeof(needle)-1;
    MemHunter mh("abcd", needlelen);


    {
        char buf[] = "a";
        char *f = mh.findend(buf, sizeof(buf)-1);
        if(f) {
            printf(">>>%s<<<\n", f); exit(EXIT_FAILURE);
        }
    }

    {
        char buf[] = "b";
        char *f = mh.findend(buf, sizeof(buf)-1);
        if(f) {
            printf(">>>%s<<<\n", f); exit(EXIT_FAILURE);
        }
    }

    {
        char buf[] = "cdxx";
        char *f = mh.findend(buf, sizeof(buf)-1);
        if(f) {
            printf(">>>%s<<<\n", f); exit(EXIT_FAILURE);
        }
    }


}
