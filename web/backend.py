from flask import Flask, render_template, session
import os
import time
import random
from flask_socketio import SocketIO, emit


app = Flask(__name__)
app.secret_key = "secret"
socketio = SocketIO(app)
user_no = 1


class Config:
    def __init__(self):
        self.config_data = dict()

    def store(self, data):
        for key in data:
            self.config_data[key] = data[key]


@app.before_request
def before_request():
    global user_no
    if "session" in session and "user-id" in session:
        pass
    else:
        session["session"] = os.urandom(24)
        session["username"] = "user" + str(user_no)
        user_no += 1


@socketio.on("connect", namespace="/config")
def connect():
    emit("response", {"data": "Connected", "username": session["username"]})


@socketio.on("disconnect", namespace="/config")
def disconnect():
    session.clear()
    print("Disconnected")


@socketio.on("request", namespace="/config")
def request(message):
    emit(
        "response",
        {"data": message["data"], "username": session["username"]},
        broadcast=True,
    )


@socketio.on("set_driver", namespace="/config")
def set_driver(message):
    c.store(message)
    emit("set_driver_ret", {}, broadcast=True)


@socketio.on("set_options", namespace="/config")
def set_options(message):
    c.store(message)
    nr_cgroup = int(c.config_data["nr_cgroup"])
    emit("chart_start", nr_cgroup, broadcast=True)

    response_data = [0] * nr_cgroup
    for _ in range(int(c.config_data["time"])):
        for idx in range(nr_cgroup):
            latency, throughput = get_data()
            response_data[idx] = [latency, throughput]
        emit("chart_data_result", response_data, broadcast=True)
        time.sleep(1)
    else:
        emit("chart_end", {}, broadcast=True)


def get_data():
    return int(random.random() * 100), int(random.random() * 100)


@app.route("/")
def info():
    return render_template("index.html")


if __name__ == "__main__":
    c = Config()
    socketio.run(app, port=3000, host="0.0.0.0", debug=True)
