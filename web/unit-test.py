#!/usr/bin/python3
# source from: https://blog.nerdfactory.ai/2019/04/04/flask-unittest.html
import unittest
import backend
import json


class UnitTest(unittest.TestCase):
    def setUp(self):
        self.app = backend.app.test_client()
        self.right_parameter = {"parameter1": 3, "parameter2": 6}
        self.wrong_parameter = {"parameter1": 3}

    def test_wrong_parameter(self):
        response = self.app.get("/api/multiply", data=self.wrong_parameter)
        data = json.loads(response.get_data())
        self.assertEqual(18, data["response"])
        self.assertEqual(1, data["state"])

    def test_wrong_result(self):
        response = self.app.get("/api/multiply", data=self.right_parameter)
        data = json.loads(response.get_data())
        self.assertEqual(10, data["response"])
        self.assertEqual(1, data["state"])

    def test_multiply_right(self):
        response = self.app.get("/api/multiply", data=self.right_parameter)
        data = json.loads(response.get_data())
        self.assertEqual(18, data["response"])
        self.assertEqual(1, data["state"])


if __name__ == "__main__":
    unittest.main()
