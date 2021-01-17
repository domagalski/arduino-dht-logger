# arduino-dht-logger
Log DHT Temperature sensor data over serial using an Arduino

### Support and dependencies.

So far, this has only been tested with DHT22 temperature sensors.

See the `.gitmodules` file for the Arduino library dependencies.

### JSON

This logs temperature, humidity, and heat index over JSON. Each sensor is
enumerated in the json according to the pin number that it is connected to. The
JSON data for each pin is represented as `t` for temperature, `h` for humidity,
and `hi` for heat index, or `e` for error. All values are in Celcius.

### Library

The following code imports the DHT logger library and creates a logger object
that uses pins 2 and 4 as sensors and enables power over pin 8. It then logs
the temperature data over the `Serial` interface.
```c++
#include "dht_logger.h"

// Pin arrays must have 0 as the last element or else this will panic. Future
// revisions of this library will make that no longer necessary.
constexpr uint8_t kPowerPins[] = {8, 0};
constexpr uint8_t kSensorPins[] = {2, 4, 0};

// Create a logger object that logs DHT22 pins over Serial
dht::DhtLogger logger(kSensorPins, DHT22, &Serial, kPowerPins);

void setup() {
  // Note that Serial.begin() is not done from within logger.begin(). The
  // serial interface must be explicitly initialized somewhere before the
  // logger can begin.
  Serial.begin(115200);
  logger.begin();
}

void loop() {
  // This reads measurements and writes them over serial in JSON format.
  delay(logger.writeToSerial());
}
```

### Standalone

To use the standalone method on Ubuntu, install the `arduino-mk` package.
The standalone method is currently only supported on Ubuntu 20.04 and Raspbian
Buster, but may work on other OS configurations.

The standalone method generates an Arduino firmware binary based on a config
file. The config file is in JSON format. See `example_config.json` for an
example of a valid config file with every option used. The `serial` and
`power_pins` keys in the config are optional. All other keys listed in
`example_config.json` are required.

Configuration binaries can be built and flashed to Arduino devices as follows:
```
python3 build.py -c example_config.json -p /dev/ttyUSB0
```
