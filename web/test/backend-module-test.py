import unittest
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(os.path.dirname(__file__))))

from package import create_app, socketio


class UnitTest(unittest.TestCase):
    def setUp(self):
        print("Hello")

    def test_something(self):
        print("World")


if __name__ == "__main__":
    unittest.main()
