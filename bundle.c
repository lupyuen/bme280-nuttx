#include <stdint.h>

#define BME280_BUS_I2C 0  //  Use I2C Bus
#define BME280_BUS_SPI 0  //  Don't use SPI Bus

////#include "bme280/driver.c"  //  NuttX Driver Shell
#include "bme280/bme280.c"  //  Zephyr BME280 Driver
