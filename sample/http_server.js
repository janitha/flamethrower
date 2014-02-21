{
    "version" : 0,
    "factories" : [
        {
            "type" : "tcp_server",

            "bind_addr" : "0.0.0.0",
            "bind_port" : 9999,

            "concurrency" : 10000,
            "count" : 5000000,

            "accept_backlog" : 10,

            "worker" : {
                "type" : "http",

                "firstline_payloads" : [{
                    "type" : "string",
                    "string" : "HTTP/1.1 200 OK"
                }, {
                    "type" : "string",
                    "string" : "\r\n"
                }],

                "header_payloads" : [{
                    "type" : "http_headers",
                    "fields" : {
                        "Content-Type" : "text/plain",
                        "Doge" : "SuchMystery",
                        "Herp" : "Derp",
                        "Server" : "Flamethrower"
                    }
                }],

                "body_payloads" : [{
                    "type" : "string",
                    "string" : "\r\n"
                }, {
                    "type" : "file",
                    "filename" : "sample/response.body"
                }],

                "linger" : 0
            }
        }
    ]
}
