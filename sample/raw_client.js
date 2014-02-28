{
    "version" : 0,
    "factories" : [
        {
            "type" : "tcp_client",

            "bind_addr" : "0.0.0.0",
            "bind_port" : 0,

            "server_addr" : "127.0.0.1",
            "server_port" : 9999,
            "connect_timeout" : 10,

            "concurrency" : 2,
            "count" : 10,

            "worker" : {
                "type" : "raw",

                "payloads" : [
                    {
                        "type" : "random",
                        "length" : 10
                    },
                    {
                        "type" : "file",
                        "filename" : "sample/helloworld.payload"
                    },
                    {
                        "type" : "random",
                        "length" : 1000
                    }
                ],

                "initiate_close" : false,
                "delay_close" : 0,

                "tcp_linger" : 0
            }
        }
    ]
}
