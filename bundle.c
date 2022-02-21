//  We bundle the Zephyr and NuttX Drivers into a single source file,
//  so we don't need to fix the NuttX build system.
#include <nuttx/config.h>
#include <nuttx/nuttx.h>
#include <stdlib.h>
#include <fixedmath.h>
#include <errno.h>
#include <debug.h>

//  Zephyr BME280 Options from
//  https://github.com/zephyrproject-rtos/zephyr/blob/main/drivers/sensor/bme280/Kconfig
#define CONFIG_BME280_MODE_NORMAL        //  Normal Sampling Mode (continuous measurements)
#define CONFIG_BME280_TEMP_OVER_2X       //  Temperature Oversamling 2x
#define CONFIG_BME280_PRESS_OVER_16X     //  Pressure Oversampling 16x
#define CONFIG_BME280_HUMIDITY_OVER_16X  //  Humidity Oversampling 16x
#define CONFIG_BME280_STANDBY_1000MS     //  Standby Time 1000ms
#define CONFIG_BME280_FILTER_4           //  Filter Coefficient 4

//  Other Defines
#define BME280_BUS_I2C  0  //  I2C Bus
#define BME280_BUS_SPI  0  //  SPI Bus
#define __ASSERT_NO_MSG assert  //  Assertion check
#define LOG_DBG         sninfo  //  Log info message

//  Zephyr Sensor Channel to be fetched from the sensor
enum sensor_channel {
    SENSOR_CHAN_ALL,           //  All Channels
    SENSOR_CHAN_AMBIENT_TEMP,  //  Ambient Temperature
    SENSOR_CHAN_PRESS,         //  Pressure
    SENSOR_CHAN_HUMIDITY,      //  Humidity
};

//  Zephyr Sensor Value
struct sensor_value {
    int32_t val1;  //  Integer part of the value
    int32_t val2;  //  Fractional part of the value (in one-millionth parts)
};

struct device;

//  Check I2C Bus
static int bme280_bus_check(const struct device *dev);

//  Read Register
static int bme280_reg_read(const struct device *dev,
    uint8_t start, uint8_t *buf, int size);

//  Write Register
static int bme280_reg_write(const struct device *dev, uint8_t reg,
    uint8_t val);

//  Embed Zephyr BME280 Driver
#include "bme280/bme280.c"

//  Embed NuttX Driver Shell
#include "bme280/driver.c"
