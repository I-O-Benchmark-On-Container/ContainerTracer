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
    # File setting
    KEY_PRESS = '\0'
    KEY_RELEASE = '\1'
    MOUSE_CLICK = '\2'
    MOUSE_SCROLL = '\3'
    DELIM = '\4'

    LOG_FILE_NAME = "log.rpl"

    def __init__(self):
        self.replay_file = self.LOG_FILE_NAME
        self.keyboard = KeyboardController()
        self.mouse = MouseController()
        
        self.INPUT_LIST = [self.KEY_PRESS, self.KEY_RELEASE, self.MOUSE_CLICK, self.MOUSE_SCROLL]

    def start_record(self):
        self.log = open(self.replay_file, "w")
        self.init_time = time.time() * 1000  # To make microseconds
        keyboard_listener = KeyboardListener(on_press=self.__on_press, on_release=self.__on_release)
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

    def __key_release(self, cmd):
        if len(cmd) > 1:
            cmd = eval(cmd)
        self.keyboard.release(cmd)

    def __mouse_click(self, cmd):
        x, y, button, pressed = cmd.split(self.DELIM)
        if pressed == "True":
            action = self.mouse.press
        else:
            action = self.mouse.release
        self.mouse.position = (int(x), int(y))
        action(eval("Button." + button))

    def __mouse_scroll(self, cmd):
        x, y, dx, dy = cmd.split(self.DELIM)
        self.mouse.position = (int(x), int(y))
        self.mouse.scroll(int(dx), int(dy))

    def __play(self, cmd):
        if len(cmd) < 1:
            return -1
        if cmd[-1] == self.KEY_PRESS:
            action = self.__key_press
        elif cmd[-1] == self.MOUSE_CLICK:
            action = self.__mouse_click
        elif cmd[-1] == self.MOUSE_SCROLL:
            action = self.__mouse_scroll
        elif cmd[-1] == self.KEY_RELEASE:
            action = self.__key_release
        action(cmd[:-1])
        return 1

    def play_record(self):
        f = open(self.replay_file, "r")
        prev_delay = 0
        while True:
            buf = self.DELIM
            cmd = ""
            while buf not in self.INPUT_LIST:
                if buf == "":
                    cmd = ""
                    break
                buf = f.read(1)
                cmd += buf
            if len(cmd) < 1:
                break
            delay, cmd = cmd.split(self.DELIM, maxsplit=1)
            delay = float(delay)
            if self.__play(cmd) < 0:
                break
            time.sleep((delay - prev_delay) / 1000.0)
            prev_delay = delay
        f.close()

    def __on_press(self, key):
        if key == Key.esc:
            return False
        if '<' in str(key):
            return True

        if len(str(key)) < 4:
            cmd = str(key)[1:-1]
        else:
            cmd = str(key)
        self.log.write(str(time.time() * 1000 - self.init_time) + self.DELIM + cmd + self.KEY_PRESS)

    def __on_release(self, key):
        if key == Key.esc:
            return False
        if '<' in str(key):
            return True

        if len(str(key)) < 4:
            cmd = str(key)[1:-1]
        else:
            cmd = str(key)
        self.log.write(str(time.time() * 1000 - self.init_time) + self.DELIM + cmd + self.KEY_RELEASE)

    def __on_click(self, x, y, button, pressed):
        self.log.write(
            str(time.time() * 1000 - self.init_time)
            + self.DELIM
            + str(x)
            + self.DELIM
            + str(y)
            + self.DELIM
            + button.name
            + self.DELIM
            + str(pressed)
            + self.MOUSE_CLICK
        )

    def __on_scroll(self, x, y, dx, dy):
        self.log.write(
            str(time.time() * 1000 - self.init_time)
            + self.DELIM
            + str(x)
            + self.DELIM
            + str(y)
            + self.DELIM
            + str(dx)
            + self.DELIM
            + str(dy)
            + self.MOUSE_SCROLL
        )
