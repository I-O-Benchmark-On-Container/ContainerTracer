from multiprocessing import Process
import ctypes
import json
import os

def get_nr_tasks():
    return 4


def get_time():
    return 60


def get_q_depth():
    return 32


def get_nr_thread():
    return 4


def get_prefix_cgroup_name():
    return "tester.trace."


def get_scheduler_name():
    return "bfq"


def get_weight(task_num):
	return task_num


def get_weights(nr_tasks):
    """
    각 task별 weight를 가져온다.
    """
    weights = []
    for task in range(nr_tasks):
    	weights.append({"weight":get_weight(task)})
    return weights


def get_configure_json():
    nr_tasks = get_nr_tasks()
    time = get_time()
    q_depth = get_q_depth()
    nr_thread = get_nr_thread()
    prefix_cgroup_name = get_prefix_cgroup_name()
    scheduler = get_scheduler_name()
    weights = get_weights(nr_tasks)

    setting = {"nr_tasks":nr_tasks, "time":time, "q_depth":q_depth, "nr_thread":nr_thread, \
		"prefix_cgroup_name":prefix_cgroup_name, "scheduler":scheduler, \
		"task_option":weights}
    
    config_json = json.dumps({"driver":"trace-replay", "setting":setting})

    return config_json


def run_all_trace_replay():
    libc = ctypes.CDLL("./librunner.so")
    config_json = get_configure_json()
    libc.runner_init(config_json.encode())
    return

run_all_trace_replay()
