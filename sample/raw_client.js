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

            "concurrency" : 50,
            "count" : 10000000,

            "worker" : {
                "type" : "raw",

                "payloads" : [
                    {
                        "type" : "random",
                        "length" : 1000
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

                "shutdown" : true,

                "linger" : 0
            }
        }
    ]
}
