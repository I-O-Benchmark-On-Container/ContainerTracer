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
        if isinstance(config["setting"]["nr_tasks"], str):
            config["setting"]["nr_tasks"] = int(config["setting"]["nr_tasks"])

        for elem in config["setting"]["task_option"]:
            if not (10<= int(elem["weight"]) and int(elem["weight"])<= 1000):
                raise Exception("weight")
            if not os.path.isfile(elem["trace_data_path"]):
                raise Exception("trace_data_path")

        if config["setting"]["trace_replay_path"] != "trace-replay":
            raise Exception("tracer_replay_path")
        try:
            device_path = "/dev/" + config["setting"]["device"]
            st_mode = os.stat(device_path).st_mode
            stat.S_ISBLK(st_mode)
        except:
            raise Exception("device")

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
    # @brief Create a data dictionary which will contain the interval results.
    #
    # @param[in] ket_set Set of keys.
    #
    # @return data structure which will contain the interval results.
    @staticmethod
    def prepare_data_dict(key_set: set) -> dict:
        data_dict = {}
        for key in key_set:
            data_dict[key] = {"results": []}
        return data_dict

    ##
    # @brief save interval results to json file.
    #
    # @param[in] data_dict dictionary data structure which contains the interval results.
    @staticmethod
    def save_interval_to_files(data_dict: dict) -> None:
        import pprint
        pprint.pprint(data_dict)
        for key, value in data_dict.items():
            filename = ContainerTracer.get_valid_filename(f"{key}-interval-result.json")
            current_file = open(filename, "w")
            current_file.write(json.dumps(value, indent=4, sort_keys=True))
            current_file.close()

    ##
    # @brief Refresh frotnend chart by a interval with trace-replay aysnc.
    # Send result via chart module.
    #
    # @return `True` for success to execute, `False` for fail to execute.
    #
    # @note This function will free resources.
    def _refresh(self) -> bool:
        super()._refresh()
        frontend_chart = chart.Chart()
        key_set = set(["cgroup-" + str(i + 1) for i in range(self.nr_tasks)])
        remain_set = copy.copy(key_set)

        data_dict = self.prepare_data_dict(key_set)

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

                data_dict[key]["results"].append(json.loads(raw_data)['data'])
                interval_results.append(chart_result)
            if len(key_set) == len(interval_results):
                self._update_interval_results(interval_results)
            else:
                self._get_total_result()
                self.save_interval_to_files(data_dict)
                self._update_interval_results({})
                return True

        return False
