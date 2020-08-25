import ctypes
from . import chart
import copy
import json
from .container_tracer import ContainerTracer


##
# @brief 유닛 테스트를 위한 Test Class
class TraceReplayTest(ContainerTracer):
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
# @brief trace-replay 드라이버
class TraceReplay(ContainerTracer):
    FIN = 3  # trace-replay 종료 Flag

    ##
    # @brief task 개수를 저장 후 json string으로 dump.
    #
    # @param config Frontend에서 전달받은 config options.
    def set_config(self, config: dict) -> None:
        if isinstance(config["setting"]["nr_tasks"], str):
            config["setting"]["nr_tasks"] = int(config["setting"]["nr_tasks"])
        self.nr_tasks = config["setting"]["nr_tasks"]
        self.config_json = json.dumps(config)

    ##
    # @brief trace-replay 결과를 받음.
    #
    # @param key 받아올 특정 그룹의 키 값.
    #
    # @return runner 모듈에서 해당 키 값에 매핑되는 result.
    def get_interval_result(self, key: str) -> None:
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
    def refresh(self) -> None:
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
