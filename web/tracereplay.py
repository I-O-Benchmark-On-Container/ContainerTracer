import threading
import ctypes
import chart
import copy
import json
import os


##
# @brief Runner class for trace replay with multithread.
# Run trace-replay with config options and return results async.
# Usage:
#   Create TraceReplay object with config options.
#   Call run_all_trace_replay().
class TraceReplay:
    FIN = 3  # FInish flag with trace replay.
    ##
    # @brief Initializing with config.
    # Before calling run_all_trace_replay(),
    # Initialize with create TraceReplay object with config options.
    #
    # @param config For configure trace replay. Must be dictionary form.
    #
    # @return
    def __init__(self, config):
        self.nr_tasks = config["setting"]["nr_tasks"]
        self.config_json = json.dumps(config)
        self.libc = ctypes.CDLL("./build/debug/librunner.so")

        return

    ##
    # @brief Run trace replay by initializing and running runner module written in libc.
    # Called by trace_replay_driver() only.
    def trace_replay_run(self):
        ret = self.libc.runner_init(str(self.config_json).encode())
        if ret != 0:
            raise Exception(ret)

        ret = self.libc.runner_run()
        if ret != 0:
            raise Exception
        return

    ##
    # @brief Get interval results in real time, run async with trace replay.
    # if no result exist, pend until get the result.
    #
    # @param key Want to get specific cgroup.
    #
    # @return
    def get_interval_result(self, key):
        self.libc.runner_get_interval_result.restype = ctypes.POINTER(ctypes.c_char)
        ptr = self.libc.runner_get_interval_result(key.encode())
        if ptr == 0:
            raise Exception
        ret = ctypes.cast(ptr, ctypes.c_char_p).value
        self.libc.runner_put_result_string(ptr)
        return ret

##
# @brief 
#
# @return 
    def refresh(self):
        """ Refreshing front-end chart until end of jobs.
            Send results via chart object.
        """
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

            update_interval_result(interval_results)

        self.libc.runner_free()
        return

    def trace_replay_driver(self):
        """ Driver for running trace replay and refresh async with threads.
        """
        trace_replay_proc = threading.Thread(target=self.trace_replay_run)
        refresh_proc = threading.Thread(target=self.refresh)

        trace_replay_proc.start()
        refresh_proc.start()
        return

    def run_all_trace_replay(self):
        """ Run trace replay module.
            Must be called after createing TraceReplay object
            Must be called by super user.
        """
        if os.getuid() != 0:
            print("Execute by SuperUser!!!")
            raise Exception
        driver = threading.Thread(target=self.trace_replay_driver)
        driver.start()
        return
