from .container_tracer import ContainerTracer
from flask_socketio import SocketIO
from . import chart
import os
import ctypes
import stat
import copy
import json


##
# @brief Test class with unit-test.
class TraceReplayTest(ContainerTracer):
    def __init__(self, socketio: SocketIO, config: dict) -> None:
        pass

    def _set_config(self, config: dict) -> None:
        pass

    def trace_replay_free(self) -> None:
        pass

    def _trace_replay_run(self) -> None:
        pass

    def _get_interval_result(self, key: str) -> None:
        pass

    def _refresh(self) -> None:
        pass

    def _update_interval_results(self, interval_results: dict) -> None:
        pass

    def _trace_replay_driver(self) -> None:
        pass

    def run_all_trace_replay(self) -> None:
        pass


##
# @brief trace-replay driver.
class TraceReplay(ContainerTracer):
    FIN = 3  # trace-replay finish flag,

    def __init__(self, socketio: SocketIO, config: dict) -> None:
        super().__init__(socketio, config)

    ##
    # @brief Save task# and dump into json string.
    #
    # @param[in] confg config options from frontend.
    def _set_config(self, config: dict) -> None:
        super()._set_config(config)
        if config["setting"]["trace_replay_path"] != "trace-replay":
            self.socketio.emit("Invalid path")
            raise Exception("Invalid container tracer path!")
        device_path = "/dev/" + config["setting"]["device"]
        st_mode = os.stat(device_path).st_mode
        try:
            stat.S_ISBLK(st_mode)
        except:
            self.socketio.emit("Invalid device")
            raise Exception("Invalid device!")
        if isinstance(config["setting"]["nr_tasks"], str):
            config["setting"]["nr_tasks"] = int(config["setting"]["nr_tasks"])
        self.config_json = json.dumps(config)

    ##
    # @brief receive trace-replay result.
    #
    # @param[in] key Want to select certain group.
    #
    # @return result mapped with key in runner module.
    def _get_interval_result(self, key: str) -> None:
        super()._get_interval_result(key)

        interval_result = self.libc.runner_get_interval_result
        interval_result.restype = ctypes.POINTER(ctypes.c_char)

        ptr = interval_result(key.encode())
        if ptr == 0:
            raise Exception("Memory Allocation 실패")
        ret = ctypes.cast(ptr, ctypes.c_char_p).value
        self.libc.runner_put_result_string(ptr)
        return ret

    ##
    # @brief Refresh frotnend chart by a interval with trace-replay aysnc.
    # Send result via chart module.
    #
    # @return `True` for success to execute, `False` for fail to execute.
    def _refresh(self) -> bool:
        super()._refresh()
        frontend_chart = chart.Chart()
        key_set = set(["cgroup-" + str(i + 1) for i in range(self.nr_tasks)])
        remain_set = copy.copy(key_set)
        while len(remain_set):
            current = copy.copy(remain_set)
            interval_results = []

            for key in current:
                raw_data = self._get_interval_result(key).decode()
                frontend_chart.set_config(raw_data)
                chart_result = frontend_chart.get_chart_result()

                if len(chart_result) == 0:
                    remain_set.remove(key)
                    continue

                interval_results.append(chart_result)
            if len(key_set) == len(interval_results):
                self._update_interval_results(interval_results)
            else:
                self._get_total_result()
                self._update_interval_results({})
                return True
        return False
