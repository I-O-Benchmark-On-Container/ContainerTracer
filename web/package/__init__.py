from flask import Flask
from flask_socketio import SocketIO
from . import tracereplay
import os

socketio = SocketIO()

class Config:
    def __init__(self):
        self.data = dict()
        
        unit_test_mode = os.environ.get("PYTHON_UNIT_TEST")
        unit_test_mode = "" if unit_test_mode == None else unit_test_mode
        unit_test_mode = unit_test_mode.lower() == "true"
        if unit_test_mode == False:
            self.trace_replay = tracereplay.TraceReplay(socketio)
        else:
            self.trace_replay = tracereplay.TraceReplayTest(socketio)

    def store(self, input_data=dict, set_type=str):
        each_data = dict()
        for key, value in input_data.items():
            if set_type == "driver":
                self.data[key] = value
            elif set_type == "set_all":
                self.data["setting"][key] = value
            else:
                if "path" not in key:
                    each_data["cgroup_id"], each_data["weight"] = key, value
                else:
                    each_data["trace_data_path"] = value

                if len(each_data.keys()) == 3:
                    self.data["setting"]["task_option"].append(each_data)
                    each_data = dict()

        if set_type == "driver":
            self.data["setting"] = {}
        elif set_type == "set_all":
            self.data["setting"]["task_option"] = []

    def get_config_data(self):
        return self.data


def create_app(debug=False):
    app = Flask(__name__)
    app.secret_key = "secret"
    app.debug = debug
    from .main import main as main_blueprint
    app.register_blueprint(main_blueprint)
    socketio.init_app(app)
    return app
