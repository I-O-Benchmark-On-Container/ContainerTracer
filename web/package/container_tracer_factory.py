from .trace_replay import TraceReplay, TraceReplayTest
from .container_tracer import ContainerTracer
from flask_socketio import SocketIO


socketio = ()


##
# @brief Factory method pattern with conatiner-tracer.
class ContainerTracerFactory(object):
    ##
    # @brief Initialize with frontend socketio.
    #
    # @param[in] socketio Want to communicate with frontend.
    def __init__(self, socketio: SocketIO) -> None:
        self.socketio = socketio

    ##
    # @brief Get proper instance with driver option in config.
    #
    # @param[in] config Container-tracer config options from frontend.
    #
    # @return Proper container-tracer module.
    def get_instance(self, config: dict, debug: bool = False) -> ContainerTracer:
        driver = config["driver"]
        instance = None
        if driver in ["trace-replay", "docker"]:
            if debug:
                instance = TraceReplayTest(self.socketio, config)
            else:
                instance = TraceReplay(self.socketio, config)
        return instance 
