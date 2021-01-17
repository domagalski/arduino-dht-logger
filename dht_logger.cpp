#include <ArduinoJson.h>

#include "dht_logger.h"

namespace dht {

namespace {
template <typename T>
size_t ArraySize(const T *array) {
  size_t n = 0;
  while (array[n]) {
    n++;
  }
  return n;
}
}  // namespace

DhtLogger::DhtLogger(const uint8_t *pins, const uint8_t type,
                     HardwareSerial *serial, const uint8_t *power_pins)
    : pins_(pins),
      n_pins_(ArraySize(pins)),
      type_(type),
      serial_(serial),
      power_pins_(power_pins),
      n_power_pins_(ArraySize(power_pins)) {
  dht_ = (DHT *)malloc(n_pins_ * sizeof(DHT));
  for (size_t i = 0; i < n_pins_; i++) {
    dht_[i] = DHT(pins_[i], type);
  }
}

void DhtLogger::begin() {
  for (size_t i = 0; i < n_power_pins_; i++) {
    pinMode(power_pins_[i], OUTPUT);
    digitalWrite(power_pins_[i], HIGH);
  }

  // probably overkill, but only needs to happen once
  delay(1000);

  for (size_t i = 0; i < n_pins_; i++) {
    dht_[i].begin();
  }
}

unsigned long DhtLogger::writeToSerial() {
  DynamicJsonDocument msg(1024);

  const unsigned long start_time = millis();
  for (size_t i = 0; i < n_pins_; i++) {
    // char is u8, largest u8 is 255, three digits + null
    char *pin = new char[4];
    itoa(pins_[i], pin, 10);

    const Measurement read = readSensor(i);
    if (read.error) {
      const char *key = "e";
      switch (read.errno) {
        case Error::NO_SENSOR:
          msg[pin][key] = "Unknown sensor";
          break;
        case Error::TEMPERATURE:
          msg[pin][key] = "Error reading temperature";
          break;
        case Error::HUMIDITY:
          msg[pin][key] = "Error reading temperature";
          break;
        default:
          msg[pin][key] = "Unknown error";
      }
    } else {
      msg[pin]["t"] = read.data.temperature;
      msg[pin]["h"] = read.data.humidity;
      msg[pin]["hi"] = read.data.heat_index;
    }

    delete[] pin;
  }

  // log to serial
  serializeJson(msg, *serial_);
  serial_->println("");

  // calculate the remaining time in the interval
  const unsigned long end_time = millis();
  if (end_time <= start_time) {
    return kLogIntervalMs;  // clock overflows
  } else if (end_time - start_time >= kLogIntervalMs) {
    return 0;  // too much time elapsed
  } else {
    return kLogIntervalMs - (end_time - start_time);
  }
}

Measurement DhtLogger::readSensor(size_t idx) {
  const bool is_fahrenheit = false;
  Measurement measurement;
  measurement.error = true;

  if (idx >= n_pins_) {
    measurement.errno = Error::NO_SENSOR;
    return measurement;
  }

  const float temperature = dht_[idx].readTemperature(is_fahrenheit);
  if (isnan(temperature)) {
    measurement.errno = Error::TEMPERATURE;
    return measurement;
  }

  const float humidity = dht_[idx].readHumidity();
  if (isnan(humidity)) {
    measurement.errno = Error::HUMIDITY;
    return measurement;
  }

  const float heat_index =
      dht_[idx].computeHeatIndex(temperature, humidity, is_fahrenheit);

  measurement.error = false;
  measurement.data.temperature = temperature;
  measurement.data.humidity = humidity;
  measurement.data.heat_index = heat_index;

  return measurement;
}

}  // namespace dht
