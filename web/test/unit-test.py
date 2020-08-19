import unittest
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(os.path.dirname(__file__))))

from package import create_app, socketio

app = create_app()


class UnitTest(unittest.TestCase):
    def setUp(self):
        self.config = None
        self.client = socketio.test_client(app)
        self.test_set_driver = {"driver": "trace-replay"}
        self.test_set_each = {
            "cgroup-1": "1000",
            "cgroup-1-path": "./sample/sample1.dat",
            "cgroup-2": "1000",
            "cgroup-2-path": "./sample/sample1.dat",
            "cgroup-3": "1000",
            "cgroup-3-path": "./sample/sample1.dat",
            "cgroup-4": "1000",
            "cgroup-4-path": "./sample/sample1.dat",
        }
        self.test_set_all = {
            "trace_replay_path": "./build/release/trace-replay",
            "device": "none",
            "nr_tasks": "4",
            "time": "60",
            "q_depth": "32",
            "nr_thread": "4",
            "prefix_cgroup_name": "tester.trace.",
            "scheduler": "bfq",
        }
        self.right_set_config = {
            "driver": "trace-replay",
            "setting": {
                "trace_replay_path": "./build/release/trace-replay",
                "device": "none",
                "nr_tasks": "4",
                "time": "60",
                "q_depth": "32",
                "nr_thread": "4",
                "prefix_cgroup_name": "tester.trace.",
                "scheduler": "bfq",
                "task_option": [
                    {
                        "cgroup_id": "cgroup-1",
                        "weight": "1000",
                        "trace_data_path": "./sample/sample1.dat",
                    },
                    {
                        "cgroup_id": "cgroup-2",
                        "weight": "1000",
                        "trace_data_path": "./sample/sample1.dat",
                    },
                    {
                        "cgroup_id": "cgroup-3",
                        "weight": "1000",
                        "trace_data_path": "./sample/sample1.dat",
                    },
                    {
                        "cgroup_id": "cgroup-4",
                        "weight": "1000",
                        "trace_data_path": "./sample/sample1.dat",
                    },
                ],
            },
        }

    def test_connect(self):
        self.assertTrue(self.client.is_connected())

    def test_disconnect(self):
        self.client.disconnect()
        self.assertFalse(self.client.is_connected())

    def test_set_config(self):
        self.client.emit("set_driver", self.test_set_driver)
        self.client.emit("test_set_options", self.test_set_each, self.test_set_all)
        received = self.client.get_received()[1]["args"].pop()
        self.assertEqual(received, self.right_set_config)


if __name__ == "__main__":
    unittest.main()
