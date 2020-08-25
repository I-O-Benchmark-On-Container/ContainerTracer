from .trace_replay import TraceReplay
from .container_tracer import ContainerTracer
from flask_socketio import SocketIO
import Object


##
# @brief conatiner-tracer를 위한 factory method pattern.
class ContainerTracerFactory(Object):
    ##
    # @brief
    #
    # @param socketio Frontend와 통신할 socketio.
    # @param dict Frontend에서 전달받은 container-tracer 설정으로 dictionary 형태여야 함.
    #
    # @return 해당 드라이버에 맞는 container-tracer 모듈을 반환.
    def get_instance(socketio: SocketIO, config: dict) -> ContainerTracer:
        driver = config["driver"]
        if driver == "trace-replay":
            return TraceReplay(socketio, config)
        else:
            return None
