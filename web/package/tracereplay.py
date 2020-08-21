import threading
import ctypes
from . import chart
import copy
import json
import os


##
# @brief 유닛 테스트를 위한 Test Class
class TraceReplayTest:
    def __init__(self, socketio):
        pass

    def set_config(self, config: dict):
        pass

    def trace_replay_free(self):
        pass

    def trace_replay_run(self):
        pass

    def get_interval_result(self, key):
        pass

    def refresh(self):
        pass

    def update_interval_results(self, interval_results):
        pass

    def trace_replay_driver(self):
        pass

    def run_all_trace_replay(self):
        pass


##
# @brief trace-replay 동작을 위한 Runner Class. 멀티 쓰레드 환경에서 동작.
# trace-replay 옵션을 설정하고 trace replay 동작.
# 결과 출력은 매 interval result마다 async하게 호출.
# 사용법:
#   TraceReplay 객체 생성 시 Frontend와 통신할 socketio를 인자로 전달.
#   set_config(config)로 config 옵션을 설정
#   run_all_trace_replay()로 trace-replay 동작
class TraceReplay:
    FIN = 3  # trace-replay 종료 Flag

    ##
    # @brief trace-replay 사용전 socketio로 Initializing.
    #
    # @param socketio Frontend와 통신할 socketio.
    def __init__(self, socketio):
        self.socketio = socketio
        self.libc = ctypes.CDLL("librunner.so")

    ##
    # @brief run_all_trace_replay()를 호출하기 전 trace-replay 설정 세팅.
    #
    # @param config Frontend에서 전달받은 trace-replay 설정. dictionary 형태여야 함.
    def set_config(self, config: dict):
        if isinstance(config["setting"]["nr_tasks"], str):
            config["setting"]["nr_tasks"] = int(config["setting"]["nr_tasks"])
        self.nr_tasks = config["setting"]["nr_tasks"]
        self.config_json = json.dumps(config)

    ##
    # @brief trace-replay를 호출 후 메모리를 free.
    # run_all_trace_replay() 호출 후 반드시 trace_replay_free()를 호출해야 함.
    # 안 할 경우, 그 다음 trace-replay run에서 에러 발생.
    def trace_replay_free(self):
        self.driver.join()
        self.libc.runner_free()

    ##
    # @brief set_config()에서 설정한 config 옵션으로 libc로 작성된 runner module을 호출.
    # 이 메소드는 오로지 trace_replay_driver()에서만 호출해야 함.
    def trace_replay_run(self):
        ret = self.libc.runner_init(str(self.config_json).encode())
        if ret != 0:
            raise OSError(ret, os.strerror(ret))

        ret = self.libc.runner_run()
        if ret != 0:
            raise OSError(ret, os.strerror(ret))

    ##
    # @brief trace-replay 결과를 받아온다. trace-replay와 async하게 작동.
    # 만약 받아올 result가 아직 없을 경우 pending.
    #
    # @param key 받아올 특정 그룹의 키 값.
    #
    # @return runner 모듈에서 해당 키 값에 매핑되는 result.
    def get_interval_result(self, key):
        self.libc.runner_get_interval_result.restype = ctypes.POINTER(ctypes.c_char)
        ptr = self.libc.runner_get_interval_result(key.encode())
        if ptr == 0:
            raise Exception("Memory Allocation 실패")
        ret = ctypes.cast(ptr, ctypes.c_char_p).value
        self.libc.runner_put_result_string(ptr)
        return ret

    ##
    # @brief trace-replay가 동작하면서 매 interval마다 Frontend의 차트를 갱신.
    # chart 모듈을 거쳐 Frontend로 전달.
    def refresh(self):
        key_set = set(["cgroup-" + str(i + 1) for i in range(self.nr_tasks)])
        remain_set = copy.copy(key_set)
        while len(remain_set):
            current = copy.copy(remain_set)
            interval_results = []

            for key in current:
                raw_data = self.get_interval_result(key).decode()
                chart_result = chart.get_chart_result(raw_data)

                if len(chart_result) == 0:
                    remain_set.remove(key)

                interval_results.append(chart_result)

            self.update_interval_results(interval_results)

    ##
    # @brief 전달받은 trace-replay의 interval result들을 Frontend 차트로 전달.
    #
    # @param interval_results Frontend 차트로 전달할 interval results.
    def update_interval_results(self, interval_results):
        if interval_results:
            self.socketio.emit("chart_data_result", interval_results)
        else:
            self.socketio.emit("chart_end")

    ##
    # @brief trace-replay 드라이버로 trace-replay와 Frontend 차트를 갱신하는 작업을 멀티쓰레딩으로 동작.
    def trace_replay_driver(self):
        trace_replay_proc = threading.Thread(target=self.trace_replay_run)
        refresh_proc = threading.Thread(target=self.refresh)

        trace_replay_proc.start()
        refresh_proc.start()

        trace_replay_proc.join()
        refresh_proc.join()

    ##
    # @brief trace-replay를 동작. 쓰레드를 분리하여 호출자는 waiting하지 않음.
    # sudo 권한으로 실행해야 하며 호출 전 TraceReplay Initializing과 setup_config()를 호출해야함.
    def run_all_trace_replay(self):
        if os.getuid() != 0:
            raise Exception("Execute by SuperUser!!!")
        self.driver = threading.Thread(target=self.trace_replay_driver)
        self.driver.start()
