import os
import time
import random
from flask import session
from flask_socketio import emit
from .. import socketio
from .. import Config

set_config = Config()


@socketio.on("connect")
def connect():
    if "session" in session:
        pass
    else:
        session["session"] = os.urandom(24)


@socketio.on("disconnect")
def disconnect():
    session.clear()


@socketio.on("set_driver")
def set_driver(driver):
    set_config.store(driver, "driver")
    emit("set_driver_ret")


@socketio.on("set_options")
def set_options(set_each, set_all):
    set_config.store(set_all, "set_all")
    set_config.store(set_each, "set_each")
    nr_cgroup = len(set_config.data["setting"]["task_option"])
    emit("chart_start", nr_cgroup)
    add_data(nr_cgroup)


@socketio.on("test_set_options")
def test_set_options(set_each, set_all):
    set_config.store(set_all, "set_all")
    set_config.store(set_each, "set_each")
    emit("save", set_config.get_config_data())


def add_data(nr_cgroup):
    response_data = [0] * nr_cgroup
    for _ in range(int(set_config.data["setting"]["time"])):
        for idx in range(nr_cgroup):
            latency, throughput = get_data()
            response_data[idx] = [latency, throughput]
        emit("chart_data_result", response_data)
        time.sleep(1)
    emit("chart_end")


def get_data():
    return int(random.random() * 100), int(random.random() * 100)
