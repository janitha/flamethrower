{
    "version" : 0,
    "stats" : {
        "listener" : "tcp://*:12001"
    },
    "factories" : [
        {
            "type" : "tcp_client",

            "bind_addr" : "0.0.0.0",
            "bind_port" : 0,

            "server_addr" : "127.0.0.1",
            "server_port" : 9999,
            "connect_timeout" : 10,

            "concurrency" : 5,
            "count" : 2000,

            "worker" : {
                "type" : "http",

                "firstline_payloads" : [{
                    "type" : "string",
                    "string" : "GET / HTTP/1.1"
                }, {
                    "type" : "string",
                    "string" : "\r\n"
                }],

                "header_payloads" : [{
                    "type" : "http_headers",
                    "fields" : {
                        "User-Agent" : "Flamethrower",
                        "Doge" : "SuchMystery",
                        "Herp" : "Derp",
                        "Host" : "herpderp:11111"
                    }
                }],

                "body_payloads" : [{
                    "type" : "string",
                    "string" : "\r\n"
                }],

                "initiate_close" : false,
                "delay_close" : 0,

                "tcp_linger" : 0

            }
        }
    ]
}
