from .trace_replay import TraceReplay
from .container_tracer import ContainerTracer
from flask_socketio import SocketIO


##
# @brief Factory method pattern with conatiner-tracer.
class ContainerTracerFactory(object):
    ##
    # @brief Initialize with frontend socketio.
    #
    # @param socketio Want to communicate with frontend.
    def __init__(self, socketio: SocketIO) -> None:
        self.socketio = socketio

    ##
    # @brief Get proper instance with driver option in config.
    #
    # @param socketio Want to communicate with frontend.
    # @param config Container-tracer config options from frontend.
    #
    # @return Proper container-tracer module.
    def get_instance(self, config: dict) -> ContainerTracer:
        driver = config["driver"]
        if driver == "trace-replay":
            return TraceReplay(self.socketio, config)
        else:
            return None


##
# @brief Factory method pattern with conatiner-tracer testor.
class ContainerTracerTest(object):
    ##
    # @brief Initialize with frontend socketio.
    #
    # @param socketio Want to communicate with frontend.
    def __init__(self, socketio: SocketIO) -> None:
        self.socketio = socketio

    ##
    # @brief Get proper instance with driver option in config.
    #
    # @param socketio Want to communicate with frontend.
    # @param config Container-tracer config options from frontend.
    #
    # @return Proper container-tracer module.
    def get_instance(self, config: dict) -> ContainerTracer:
        driver = config["driver"]
        if driver == "trace-replay":
            return TraceReplay(self.socketio, config)
        else:
            return None
