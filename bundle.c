#include <stdint.h>

#define BME280_BUS_I2C 0  //  Use I2C Bus
#define BME280_BUS_SPI 0  //  Don't use SPI Bus

//  Sensor Channels to be fetched from the sensor
enum sensor_channel {
    SENSOR_CHAN_ALL,
    SENSOR_CHAN_AMBIENT_TEMP,
    SENSOR_CHAN_PRESS,
    SENSOR_CHAN_HUMIDITY,
};

#include "bme280/bme280.c"  //  Zephyr BME280 Driver

////#include "bme280/driver.c"  //  NuttX Driver Shell
