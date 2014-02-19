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

                "header_payload" : "sample/response.header",
                "body_payload" : "sample/response.body",

                "linger" : 0
            }
        }
    ]
}
