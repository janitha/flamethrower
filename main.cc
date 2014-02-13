#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "flamethrower.h"

int main(int argc, char *argv[]) {

    if(argc != 2) {
        printf("Usage: %s params.js\n", argv[0]);
        printf("error: too few arguments\n");
        exit(EXIT_FAILURE);
    }

    debug_print("parsing params\n");
    char *params_filename = argv[1];
    boost::property_tree::ptree params_ptree;
    boost::property_tree::json_parser::read_json(params_filename, params_ptree);

    debug_print("initializing the flamethrower\n");
    Flamethrower flamethrower(params_ptree);

    debug_print("entering the main event loop\n");
    while(1) {
        flamethrower.loop_iteration();
    }

}
