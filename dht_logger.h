#pragma once

#include <Arduino.h>

#include <DHT.h>

namespace dht {

// The DHT sensors can only be queried ever 2 seconds. This uses a 2.5 second
// interval for logging to ensure that requirement is enforced.
static constexpr unsigned long kLogIntervalMs = 2500;

// Basic thermal data struct
struct ThermalData {
  float temperature;
  float humidity;
  float heat_index;
};

// Error codes for read errors. This is used to set the error message in JSON.
enum Error {
  NO_SENSOR = 1,
  TEMPERATURE,
  HUMIDITY,
};

// Measurement response
struct Measurement {
  bool error;  // if true, union contains errno.
  union {
    ThermalData data;  // sensor measurements
    Error errno;       // error description
  };
};

class DhtLogger {
 public:
  // DHT Logger
  //
  // Args:
  //    pins        Array of DHT sensor pins of the same type.
  //                The last element of the array must be zero.
  //    type        The DHT sensor type.
  //    serial      The Serial interface to write to. The serial `begin` method
  //                must be called outside of DhtLogger.
  //    power_pins  Raise these pins high to use them as 5V power.
  //                The last element of the array must be zero.
  DhtLogger(const uint8_t pins[], const uint8_t type,
            HardwareSerial *serial = &Serial, const uint8_t power_pins[] = {0});

  // Initialize all DHT readers
  void begin();

  // Read all sensors and write the results over serial
  // Returns:
  //    The amount time in milliseconds remaining in the logging interval
  unsigned long writeToSerial();

 private:
  Measurement readSensor(size_t idx);

  const uint8_t *pins_;
  const size_t n_pins_;
  const uint8_t type_;
  HardwareSerial *const serial_;
  const uint8_t *power_pins_;
  const size_t n_power_pins_;

  DHT *dht_;
};

}  // namespace dht
