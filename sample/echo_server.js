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
                "type" : "echo",

                "initiate_close" : false,
                "delay_close" : 0,

                "tcp_linger" : 0
            }
        }
    ]
}
