import sys
import time
import io
import pprint

import zmq
import msgpack

class Stats(object):

    STAT_VERSION = 1

    @classmethod
    def process(cls, stats_list, stats_dict):

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
            return TcpWorkerStats.process(stats_list, stats_dict)
        elif subname == "tcpfactory":
            return TcpFactoryStats.process(stats_list, stats_dict)
        else:
            raise Exception("Invalid stat type")

class TcpFactoryStats(Stats):

    FD = None

    @classmethod
    def init(cls, filename):
        print "Opening", filename
        cls.FD = open(filename, 'w')
        cls.FD.write("{:19},{:15},{:15},{:15},{:15}\n".format(
            "timestamp", "bytes_in", "bytes_out", "cumulative_w", "active_w"))

    @classmethod
    def process(cls, stats_list, stats_dict):

        stats_dict["name"] += ":tcp:factory"

        stats_dict["stats"]["bytes_in"] = stats_list.pop(0)
        stats_dict["stats"]["bytes_out"] = stats_list.pop(0)
        stats_dict["stats"]["cumultaive_workers"] = stats_list.pop(0)
        stats_dict["stats"]["active_workers"] = stats_list.pop(0)

        # Write
        cls.FD.write("{:19},{:15},{:15},{:15},{:15}\n".format(
            stats_dict["timestamp"],
            stats_dict["stats"]["bytes_in"],
            stats_dict["stats"]["bytes_out"],
            stats_dict["stats"]["cumultaive_workers"],
            stats_dict["stats"]["active_workers"]))
        cls.FD.flush()

class TcpWorkerStats(Stats):

    @classmethod
    def process(cls, stats_list, stats_dict):

        stats_dict["name"] += ":tcp:worker"

        stats_dict["stats"]["readable_time"] = stats_list.pop(0)
        stats_dict["stats"]["writable_time"] = stats_list.pop(0)
        stats_dict["stats"]["close_time"] = stats_list.pop(0)
        stats_dict["stats"]["bytes_in"] = stats_list.pop(0)
        stats_dict["stats"]["bytes_out"] = stats_list.pop(0)

        subname = stats_list.pop(0)

        if subname == "server":
            return TcpServerWorkerStats.process(stats_list, stats_dict)
        elif subname == "client":
            return TcpClientWorkerStats.process(stats_list, stats_dict)
        else:
            raise Exception("Invalid tcpworker type")


class TcpServerWorkerStats(Stats):

    FD = None

    @classmethod
    def init(cls, filename):
        print "Opening", filename
        cls.FD = open(filename, 'w')
        cls.FD.write("{:19},{:15},{:15},{:15}\n".format(
            "timestamp","readable", "writable", "close"))

    @classmethod
    def process(cls, stats_list, stats_dict):
        stats_dict["name"] += ":server"
        stats_dict["stats"]["established_time"] = stats_list.pop(0)

        # Write
        cls.FD.write("{:19},{:15.4f},{:15.4f},{:15.4f}\n".format(
            stats_dict["timestamp"],
            (stats_dict["stats"]["readable_time"] - stats_dict["stats"]["established_time"])/1000. if stats_dict["stats"]["readable_time"] else 0,
            (stats_dict["stats"]["writable_time"] - stats_dict["stats"]["established_time"])/1000. if stats_dict["stats"]["writable_time"] else 0,
            (stats_dict["stats"]["close_time"] - stats_dict["stats"]["established_time"])/1000.))
        cls.FD.flush()


class TcpClientWorkerStats(Stats):

    FD = None

    @classmethod
    def init(cls, filename):
        print "Opening", filename
        cls.FD = open(filename, 'w')
        cls.FD.write("{:19},{:15},{:15},{:15},{:15}\n".format(
            "timestamp","connect","readable","writable","close"))

    @classmethod
    def process(cls, stats_list, stats_dict):
        stats_dict["name"] += ":client"
        stats_dict["stats"]["connect_time"] = stats_list.pop(0)
        stats_dict["stats"]["established_time"] = stats_list.pop(0)

        # Write
        cls.FD.write("{:19},{:15.4f},{:15.4f},{:15.4f},{:15.4f}\n".format(
            stats_dict["timestamp"],
            (stats_dict["stats"]["established_time"] - stats_dict["stats"]["connect_time"])/1000. if stats_dict["stats"]["connect_time"] else 0,
            (stats_dict["stats"]["readable_time"] - stats_dict["stats"]["established_time"])/1000. if stats_dict["stats"]["readable_time"] else 0,
            (stats_dict["stats"]["writable_time"] - stats_dict["stats"]["established_time"])/1000. if stats_dict["stats"]["writable_time"] else 0,
            (stats_dict["stats"]["close_time"] - stats_dict["stats"]["connect_time"])/1000.))
        cls.FD.flush()

def main():

    if len(sys.argv) != 4:
        print "Usage: %s prefix host port" % (sys.argv[0],)
        exit(1)

    # Read Args
    prefix = sys.argv[1]
    zhost = sys.argv[2]
    zport = int(sys.argv[3])

    # Setup Output Files
    TcpFactoryStats.init(prefix+".tcp.factory.csv")
    TcpServerWorkerStats.init(prefix+".tcp.worker.server.csv")
    TcpClientWorkerStats.init(prefix+".tcp.worker.client.csv")

    # Setup ZeroMQ
    zcontext = zmq.Context()
    zsocket = zcontext.socket(zmq.SUB)
    zsocket.connect("tcp://%s:%d" % (zhost, zport))
    zsocket.setsockopt(zmq.SUBSCRIBE, b'')

    # Setup Msgpack
    unpacker = msgpack.Unpacker()

    print "Listning..."

    while True:
        #print "waiting..."

        unpacker.feed(zsocket.recv())

        stats_list = [obj for obj in unpacker]
        #print stats_list

        stats_dict = {}
        Stats.process(stats_list, stats_dict)

        #pprint.pprint(stats_dict)



if __name__ == "__main__":
    main()
