#include <string>
#include <map>

#include <msgpack.hpp>

#include "common.h"

#include "flamethrower_params.h"


typedef std::map<std::string, msgpack::object> confmap_t;

flamethrower_params_t* flamethrower_params_from_file(char* filename) {

    /*
    // Read config file and unpack it
    std::ifstream fin(filename, std::ios::in | std::ios::binary);
    std::istreambuf_iterator<char> fin_start(fin), fin_end;
    std::string confstr(fin_start, fin_end);
    msgpack::unpacked confmsg;
    msgpack::unpack(&confmsg, confstr.data(), confstr.size());
    msgpack::object confobj = confmsg.get();
    confmap_t confmap;
    confobj.convert<confmap_t>(&confmap);

    std::cout << confobj << std::endl;
    */


    flamethrower_params_t *params = (flamethrower_params_t*)malloc(sizeof(flamethrower_params_t));
    memset(params, 0, sizeof(flamethrower_params_t));

    return params;
}
