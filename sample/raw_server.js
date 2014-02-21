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
                        "length" : 10
                    },
                    {
                        "type" : "string",
                        "string" : "Janitha Karunaratne"
                    },
                    {
                        "type" : "string",
                        "string" : "Janitha Karunaratne"
                    }
                ],

                "shutdown" : true,

                "linger" : 0
            }
        }
    ]
}
