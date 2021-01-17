#pragma once

#include <Arduino.h>

#include <DHT.h>

// Basic thermal data struct
struct ThermalData {
  float temperature;
  float humidity;
  float heat_index;
};

// Error codes for read errors.
enum Error {
  NO_SENSOR = 1,
  TEMPERATURE,
  HUMIDITY,
};

// measurement response
struct Measurement {
  bool error;  // if true, union contains errno.
  union {
    ThermalData data;  // sensor measurements
    Error errno;       // error description
  };
};

class DhtPublisher {
 public:
  // DHT Publisher
  //
  // Args:
  //    pins        Array of DHT sensor pins of the same type.
  //                Array must be null-terminated else runtime error.
  //    type        The DHT sensor type.
  //    serial      The Serial interface to write to (assume configured)
  //    power_pins  Raise these pins high to use them as 5V power.
  //                Array must be null-terminated else runtime error.
  DhtPublisher(const uint8_t *pins, const uint8_t type,
               HardwareSerial *serial = &Serial,
               const uint8_t *power_pins = {0});

  // Raise all pins in the power_pin array
  void Setup();

  // Run one loop iteration and publish over serial
  void Publish();

 private:
  Measurement ReadSensor(size_t idx);

  const uint8_t *pins_;
  const size_t n_pins_;
  const uint8_t type_;
  HardwareSerial *const serial_;
  const uint8_t *power_pins_;
  const size_t n_power_pins_;

  DHT *dht_;
};
