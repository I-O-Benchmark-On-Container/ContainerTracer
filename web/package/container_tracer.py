from abc import ABCMeta, abstractmethod
from flask_socketio import SocketIO
from threading import Thread
import ctypes
import os


##
# @brief container-tracer 동작을 위한 Runner Class로 멀티 쓰레드 환경에서 동작.
# container-tracer 옵션을 설정하고 trace replay 동작.
# 결과 출력은 매 interval result마다 async하게 호출.
# 사용법:
#   ContainerTracer 객체 생성 시 Frontend와 통신할 socketio를 인자로 전달.
#   set_config(config)로 config 옵션을 설정
#   run_all_container_tracer()로 container-tracer 동작
class ContainerTracer(metaclass=ABCMeta):
    libc_path = "librunner.so"

    ##
    # @brief container-tracer 사용전 socketio로 Initializing.
    #
    # @param socketio Frontend와 통신할 socketio.
    # @param config Frontend에서 전달받은 container-tracer 설정으로 dictionary 형태여야 함.
    def __init__(self, socketio: SocketIO, config: dict) -> None:
        self.socketio = socketio
        self.libc = ctypes.CDLL(self.libc_path)
        self.set_config(config)

    ##
    # @brief run_all_container_tracer()를 호출하기 전 container_tracer 설정.
    #
    # @param config Frontend에서 전달받은 container-tracer 설정으로 dictionary 형태여야 함.
    @abstractmethod
    def set_config(self, config: dict) -> None:
        pass

    ##
    # @brief container-tracer를 호출 후 메모리 해제.
    # run_all_container_tracer() 호출 후 반드시 container_tracer_free()를 호출해야 함.
    # 안 할 경우 그 다음 container-tracer run에서 에러 발생.
    def container_tracer_free(self) -> None:
        self.driver.join()
        self.libc.runner_free()

    ##
    # @brief set_config()에서 설정한 config 옵션으로 libc로 작성된 runner module 호출.
    # 이 메소드는 오로지 container_tracer_driver()에서만 호출.
    def container_tracer_run(self) -> None:
        ret = self.libc.runner_init(self.config_json.encode())
        if ret != 0:
            raise OSError(ret, os.strerror(ret))

        ret = self.libc.runner_run()
        if ret != 0:
            raise OSError(ret, os.strerror(ret))

    ##
    # @brief container-tracer 결과를 받음.
    # container-tracer와 async하게 동작.
    # 만약 받아올 result가 없을 경우 pending.
    #
    # @param key 받아올 특정 그룹의 키 값.
    @abstractmethod
    def get_interval_result(self, key: str) -> None:
        pass

    ##
    # @brief container-tracer가 동작하면서 매 interval마다 Frontend의 차트를 갱신.
    # chart 모듈을 거쳐 Frontend로 전달.
    @abstractmethod
    def refresh(self) -> None:
        pass

    ##
    # @brief 전달받은 container-tracer의 interval result들을 Frontend 차트로 전달.
    #
    # @param interval_result Frontend 차트로 전달할 interval result.
    def update_interval_result(self, interval_results: dict) -> None:
        if interval_results:
            self.socketio.emit("chart_data_result", interval_results)
        else:
            self.socketio.emit("chart_end")

    ##
    # @brief 지정된 드라이버로 container-tracer와 Frontend 차트를 갱신하는 작업을 멀티쓰레딩으로 동작.
    def container_tracer_driver(self) -> None:
        container_tracer_proc = Thread(target=self.container_tracer_run)
        refresh_proc = Thread(target=self.refresh)

        container_tracer_proc.start()
        refresh_proc.start()

        container_tracer_proc.join()
        refresh_proc.join()

    ##
    # @brief container-tracer를 동작.
    # 쓰레드를 분리하여 호출자는 waiting하지 않음.
    # sudo 권한으로 실행해야 하며 호출 전 setup_config()를 호출해야함.
    def run_all_container_tracer(self) -> None:
        if os.getuid() != 0:
            raise Exception("Execute by superuser!!!")
        self.driver = Thread(target=self.container_tracer_driver)
        self.driver.start()
