#include "flamethrower.h"

int main(int argc, char *argv[]) {

    debug_print("initializing the flamethrower\n");
    Flamethrower ft("client_params.msg");
    
    debug_print("entering the main event loop\n");
    while(1) {
        ft.loop_iteration();
    }

}
