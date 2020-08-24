import os
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
    json_config = set_config.get_config_data()
    # run_all_trace_replay()를 호출하기 전 반드시 configure을 해야합니다.
    trace_replay = set_config.trace_replay
    trace_replay.set_config(json_config)
    trace_replay.run_all_trace_replay()  # 여기서 trace-replay가 시작합니다

    nr_cgroup = len(set_config.data["setting"]["task_option"])
    emit("chart_start", nr_cgroup)


@socketio.on("test_set_options")
def test_set_options(set_each, set_all):
    set_config.store(set_all, "set_all")
    set_config.store(set_each, "set_each")
    emit("save", set_config.get_config_data())
