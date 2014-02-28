{
    "version" : 0,
    "factories" : [
        {
            "type" : "tcp_client",

            "bind_addr" : "0.0.0.0",
            "bind_port" : 0,

            "server_addr" : "127.0.0.1",
            "server_port" : 9999,
            "connect_timeout" : 2,

            "concurrency" : 10,
            "count" : 10000000,

            "worker" : {
                "type" : "raw",

                "payloads" : [
                    {
                        "type" : "file",
                        "filename" : "sample/request.payload"
                    }
                ],

                "initiate_close" : false,
                "delay_close" : 0,

                "tcp_linger" : 0
            }
        }
    ]
}
