from pynput.keyboard import Key
from pynput.keyboard import Listener as KeyboardListener
from pynput.keyboard import Controller as KeyboardController
from pynput.mouse import Button
from pynput.mouse import Listener as MouseListener
from pynput.mouse import Controller as MouseController
import sys
import time
import os
import subprocess
import threading


class AppRunner:
    def __init__(self, path=None):
        self.path = path
        if os.name == "posix":
            os.environ["HOME"] = "/home/{}".format(os.environ["SUDO_USER"])

    def set_path(self, path):
        print(path)
        self.path = path
        if not os.path.isfile(path):
            print("Invalid path is inputted {}".format(path), file=sys.stderr)

    def start(self):
        if self.path == None:
            print("path value is None. please set path first...", file=sys.stderr)
        else:
            subprocess.Popen(self.path)


class AppRecorder:
    def __init__(self):
        self.replay_file = "log.rpl"
        self.keyboard = KeyboardController()
        self.mouse = MouseController()

    def start_record(self):
        self.log = open(self.replay_file, "w")
        self.init_time = time.time() * 1000  # To make microseconds
        keyboard_listener = KeyboardListener(on_press=self.__on_press)
        mouse_listener = MouseListener(
            on_click=self.__on_click, on_scroll=self.__on_scroll
        )
        keyboard_listener.start()
        mouse_listener.start()
        keyboard_listener.join()
        mouse_listener.stop()
        keyboard_listener.stop()
        self.log.flush()
        self.log.close()

    def __key_press(self, cmd):
        if len(cmd) > 1:
            cmd = eval(cmd)
        self.keyboard.press(cmd)
        self.keyboard.release(cmd)

    def __mouse_click(self, cmd):
        x, y, button, pressed = cmd.split("\3")
        if pressed == "True":
            action = self.mouse.press
        else:
            action = self.mouse.release
        self.mouse.position = (int(x), int(y))
        action(eval("Button." + button))

    def __mouse_scroll(self, cmd):
        x, y, dx, dy = cmd.split("\3")
        self.mouse.position = (int(x), int(y))
        self.mouse.scroll(int(dx), int(dy))

    def __play(self, cmd):
        if len(cmd) < 1:
            return -1
        if cmd[-1] == "\0":
            action = self.__key_press
        elif cmd[-1] == "\1":
            action = self.__mouse_click
        elif cmd[-1] == "\2":
            self.__mouse_scroll
        action(cmd[:-1])
        return 1

    def play_record(self):
        f = open(self.replay_file, "r")
        prev_delay = 0
        while True:
            buf = "\3"
            cmd = ""
            while buf not in ["\0", "\1", "\2"]:
                if buf == "":
                    cmd = ""
                    break
                buf = f.read(1)
                cmd += buf
            if len(cmd) < 1:
                break
            delay, cmd = cmd.split("\3", maxsplit=1)
            delay = float(delay)
            if self.__play(cmd) < 0:
                break
            time.sleep((delay - prev_delay) / 1000.0)
            prev_delay = delay
        f.close()

    def __on_press(self, key):
        if key == Key.esc:
            return False
        if len(str(key)) < 4:
            cmd = str(key)[1:-1]
        else:
            cmd = str(key)
        self.log.write(str(time.time() * 1000 - self.init_time) + "\3" + cmd + "\0")

    def __on_click(self, x, y, button, pressed):
        self.log.write(
            str(time.time() * 1000 - self.init_time)
            + "\3"
            + str(x)
            + "\3"
            + str(y)
            + "\3"
            + button.name
            + "\3"
            + str(pressed)
            + "\1"
        )

    def __on_scroll(self, x, y, dx, dy):
        self.log.write(
            str(time.time() * 1000 - self.init_time)
            + "\3"
            + str(x)
            + "\3"
            + str(y)
            + "\3"
            + str(dx)
            + "\3"
            + str(dy)
            + "\2"
        )
