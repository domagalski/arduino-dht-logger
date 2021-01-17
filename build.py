#!/usr/bin/env python3

import argparse
import enum
import functools
import json
import os
import pathlib
import re
import sys
from typing import Any, Callable, Dict, Optional


class Sensor(enum.Enum):
    DHT11 = 11
    DHT12 = 12
    DHT22 = 22
    DHT21 = 21
    AM2301 = 21


class Serial(enum.Enum):
    Serial = 0
    Serial1 = 1
    Serial2 = 2
    Serial3 = 3


def get_mcu(board: str) -> str:
    with open("/usr/share/arduino/hardware/arduino/boards.txt") as f:
        all_boards = f.read().splitlines()

    mcu_line = list(
        filter(
            lambda l: re.fullmatch(f"{board}.build.mcu=" + r"\S+", l),
            all_boards,
        )
    )
    if not mcu_line:
        raise KeyError(f"Board not found: {board}")
    elif len(mcu_line) > 1:
        raise RuntimeError(f"Detected board {board!r} more than once in boards.txt")

    _, mcu = mcu_line.pop().split("=")
    return mcu


def run_cmd(cmd: str) -> int:
    print(cmd)
    return os.system(cmd)


def setenv(function: Callable) -> Callable:
    @functools.wraps(function)
    def _run(self, *args, **kwargs):
        os.environ["BOARD_TAG"] = self.board
        if self.port:
            os.environ["DEVICE_PATH"] = str(self.port)

        return function(self, *args, **kwargs)

    return _run


class ArduinoBuilder:
    def __init__(self, config: Dict[str, Any], port: Optional[pathlib.Path] = None):
        self.board = config["arduino"]
        self._mcu = get_mcu(self.board)
        self.port = pathlib.Path(port) if port else None

        self._serial = Serial[config.get("serial", "Serial")]
        self._baud = config.get("baud", 115200)
        self._sensor_type = Sensor[config["sensor_type"]]
        self._sensor_pins = config["sensor_pins"]
        self._power_pins = config.get("power_pins", [])

        self._sensor_pins.append(0)
        self._power_pins.append(0)

    def _generate_ino(self):
        main_ino = """#include "dht_publisher.h"

constexpr uint8_t kPowerPins[] = {POWER_PINS};
constexpr uint8_t kDht22Pins[] = {SENSOR_PINS};
dht::DhtPublisher publisher(kDht22Pins, SENSOR_TYPE, &SERIAL, kPowerPins);

void setup() {
  Serial.begin(BAUD);
  publisher.Setup();
}

void loop() {
  delay(publisher.Publish());
}
"""

        main_ino = main_ino.replace("POWER_PINS", ", ".join(map(str, self._power_pins)))
        main_ino = main_ino.replace("SENSOR_PINS", ", ".join(map(str, self._sensor_pins)))
        main_ino = main_ino.replace("SENSOR_TYPE", self._sensor_type.name)
        main_ino = main_ino.replace("SERIAL", self._serial.name)
        main_ino = main_ino.replace("BAUD", str(self._baud))
        return main_ino

    @setenv
    def make(self, no_build: bool, no_del_src: bool) -> int:
        with open("main.ino", "w") as f:
            f.write(self._generate_ino())

        print("Generated firmware source: main.ino")
        if no_build:
            return 0

        return run_cmd("make")

    @setenv
    def upload(self, copy_hex_path: Optional[pathlib.Path] = None) -> int:
        name = pathlib.Path(__file__).absolute().parent.name
        hex_path = pathlib.Path(f"build-{self.board}/{name}.hex")
        if copy_hex_path:
            copy_hex_path = pathlib.Path(copy_hex_path)
            retval = run_cmd(f"cp -v {hex_path} {copy_hex_path}")
            if retval:
                return retval

        if not self.port:
            return 0

        avrdude = [
            "avrdude",
            "-V",
            "-p",
            self._mcu,
            "-C",
            "/usr/share/arduino/hardware/tools/avrdude.conf",
            "-D",
            "-c",
            "arduino",
            "-P",
            str(self.port),
            "-U",
            f"flash:w:{hex_path}:i",
        ]
        return run_cmd(" ".join(avrdude))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--config-file", help="Path to the firmware config file.")
    parser.add_argument("-p", "--port", help="If specified, flash this serial port.")
    parser.add_argument(
        "--copy-hex-path",
        help="If specified, copy the output hex to this path",
    )
    parser.add_argument(
        "--no-build",
        action="store_true",
        help="Generate the main.ino, but do not build it.",
    )
    parser.add_argument(
        "--no-del-src",
        action="store_true",
        help="Do not delete the .ino source after building.",
    )
    parser.add_argument("--clean", action="store_true", help="Purge all local builds.")
    args = parser.parse_args()
    os.chdir(pathlib.Path(__file__).parent)

    if args.clean:
        sys.exit(run_cmd("rm -rf build-*"))

    if not args.config_file:
        raise ValueError("Missing config file.")

    with open(args.config_file) as f:
        config = json.load(f)

    builder = ArduinoBuilder(config, args.port)
    retcode = builder.make(args.no_build, args.no_del_src)
    if retcode:
        sys.exit(retcode)
    if not args.no_build:
        sys.exit(builder.upload(args.copy_hex_path))
