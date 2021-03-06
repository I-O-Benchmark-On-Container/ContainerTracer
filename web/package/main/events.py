import os
import sys
from flask import session
from flask_socketio import emit
from .. import socketio
from .. import Config
if os.environ.get("PYTHON_UNIT_TEST") is None:
    from .. import app_recorder
    from .. import app_runner
import threading
import time
import subprocess
import json

set_config = Config()
app_finish = False
app_docker_name = None

APP_EPSILON = 1


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
    try:
        container_tracer = set_config.set_container_tracer(json_config)
        container_tracer.set_global_config(set_config)
        container_tracer.run_all_container_tracer()  # 여기서 trace-replay가 시작합니다
    except OSError as e:
        print(e, file=sys.stderr)
        sys.exit(-1)
    except Exception as e:
        emit("Invalid", str(e))
        return

    nr_cgroup = len(set_config.data["setting"]["task_option"])
    emit("chart_start", nr_cgroup)


@socketio.on("test_set_options")
def test_set_options(set_each, set_all):
    set_config.store(set_all, "set_all")
    set_config.store(set_each, "set_each")
    emit("save", set_config.get_config_data())


if os.environ.get("PYTHON_UNIT_TEST") is None:
    #### APP-DRIVER PART ####
    @socketio.on("app_driver_run")
    def app_driver_run(data):
        global app_docker_name
        app_docker_name = data["docker-name"]
        if len(data["app-path"]) != 0:
            app_runner.set_path(data["app-path"])
            app_runner.start()
        emit("app_driver_program_run")


    @socketio.on("app_driver_record")
    def app_driver_record():
        emit("app_driver_record_start")
        app_recorder.start_record()
        emit("app_driver_record_stop")


    def app_driver_perf_get():
        global app_docker_name
        if app_docker_name == None:
            return None

        try:
            values = subprocess.check_output(
                [
                    "docker",
                    "stats",
                    app_docker_name,
                    "--no-stream",
                    "--format",
                    '"{{json .}}"',
                    "--no-trunc",
                ]
            )
            values = values.decode().strip()[1:-1]
            values = json.loads(values)
            blockio = values["BlockIO"].split("/")
            values = [
                values["CPUPerc"],
                values["MemPerc"],
                blockio[0],
                blockio[1],
            ]  # 0 : read, 1: write

        except subprocess.SubprocessError as e:
            print(e, file=sys.stderr)
            return None
        return values

    def app_driver_background_thread():
        global app_finish
        app_recorder.play_record()
        app_finish = True

    def app_driver_translate_values(values):
        for i in range(2, 4):
            values[i] = values[i].strip().lower()
            if 'kb' in values[i]:
                values[i] = float(values[i].strip('kb')) * 0.001
            elif 'mb' in values[i]:
                values[i] = float(values[i].strip('mb'))
            elif 'gb' in values[i]:
                values[i] = float(values[i].strip('gb')) * 1000.0
            elif 'b' in values[i]:
                values[i] = float(values[i].strip('b')) * 0.001 * 0.001

        return [float(values[0].strip().strip('%')), float(values[1].strip().strip('%')), values[2], values[3]]

    @socketio.on("app_driver_replay")
    def app_driver_replay():
        global app_finish
        emit("app_driver_replay_start")
        replayer = threading.Thread(target=app_driver_background_thread)
        replayer.start()
        result = open("result.csv", "w")
        result.write("cpu(%),IO-R(MB),IO-W(MB),mem(%)\n")
        while True:
            if app_finish == True:
                app_finish = False
                break
            values = app_driver_perf_get()
            if values == None:
                time.sleep(APP_EPSILON)
                continue
            values = app_driver_translate_values(values)
            keys = {
                "cpu-chart": values[0],
                "io-read-chart": values[1],
                "io-write-chart": values[2],
                "memory-chart": values[3],
            }
            emit("app_driver_perf_put", keys)
            result.write("{},{},{},{}\n".format(values[0], values[1], values[2], values[3]))
            time.sleep(APP_EPSILON)
        replayer.join()
        result.close()
        emit("app_driver_replay_stop")
