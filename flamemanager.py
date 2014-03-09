import sys
import time
import io
import pprint

import zmq
import msgpack

class Stats(object):

    STAT_VERSION = 1

    @staticmethod
    def unpack(stats_list, stats_dict):

        name = stats_list.pop(0)
        version = stats_list.pop(0)
        stats_dict["timestamp"] = stats_list.pop(0)
        subname = stats_list.pop(0)

        if name != "stats":
            raise Exception("invalid stat name")
        if version != Stats.STAT_VERSION:
            raise Exception("invalid stat version")

        stats_dict["name"] = "stats"
        stats_dict["stats"] = {}

        if subname == "tcpworker":
            return TcpWorkerStats.unpack(stats_list, stats_dict)
        elif subname == "tcpfactory":
            return TcpFactoryStats.unpack(stats_list, stats_dict)
        else:
            raise Exception("Invalid stat type")

class TcpFactoryStats(Stats):

    @staticmethod
    def unpack(stats_list, stats_dict):

        stats_dict["name"] += ":tcp:factory"

        stats_dict["stats"]["bytes_in"] = stats_list.pop(0)
        stats_dict["stats"]["bytes_out"] = stats_list.pop(0)
        stats_dict["stats"]["cumultaive_workers"] = stats_list.pop(0)
        stats_dict["stats"]["active_workers"] = stats_list.pop(0)




class TcpWorkerStats(Stats):

    @staticmethod
    def unpack(stats_list, stats_dict):

        stats_dict["name"] += ":tcp:worker"

        stats_dict["stats"]["readable_time"] = stats_list.pop(0)
        stats_dict["stats"]["writable_time"] = stats_list.pop(0)
        stats_dict["stats"]["close_time"] = stats_list.pop(0)
        stats_dict["stats"]["bytes_in"] = stats_list.pop(0)
        stats_dict["stats"]["bytes_out"] = stats_list.pop(0)

        subname = stats_list.pop(0)

        if subname == "server":
            return TcpServerWorkerStats.unpack(stats_list, stats_dict)
        elif subname == "client":
            return TcpClientWorkerStats.unpack(stats_list, stats_dict)
        else:
            raise Exception("Invalid tcpworker type")


class TcpServerWorkerStats(Stats):

    @staticmethod
    def unpack(stats_list, stats_dict):
        stats_dict["name"] += ":server"
        stats_dict["stats"]["established_time"] = stats_list.pop(0)


class TcpClientWorkerStats(Stats):

    @staticmethod
    def unpack(stats_list, stats_dict):
        stats_dict["name"] += ":client"
        stats_dict["stats"]["connect_time"] = stats_list.pop(0)
        stats_dict["stats"]["established_time"] = stats_list.pop(0)






def main():

    if len(sys.argv) != 3:
        print "Usage: %s host port" % (sys.argv[0],)
        exit(1)

    zhost = sys.argv[1]
    zport = int(sys.argv[2])

    zcontext = zmq.Context()
    zsocket = zcontext.socket(zmq.SUB)
    zsocket.connect("tcp://%s:%d" % (zhost, zport))
    zsocket.setsockopt(zmq.SUBSCRIBE, b'')

    unpacker = msgpack.Unpacker()

    while True:
        print "waiting..."

        unpacker.feed(zsocket.recv())

        stats_list = [obj for obj in unpacker]
        #print stats_list

        stats_dict = {}
        Stats.unpack(stats_list, stats_dict)

        pprint.pprint(stats_dict)



if __name__ == "__main__":
    main()
