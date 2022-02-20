#include <stdint.h>

//  Zephyr BME280 Options from
//  https://github.com/zephyrproject-rtos/zephyr/blob/main/drivers/sensor/bme280/Kconfig
#define CONFIG_BME280_MODE_NORMAL        //  Normal Sampling Mode (continuous measurements)
#define CONFIG_BME280_TEMP_OVER_2X       //  Temperature Oversamling 2x
#define CONFIG_BME280_PRESS_OVER_16X     //  Pressure Oversampling 16x
#define CONFIG_BME280_HUMIDITY_OVER_16X  //  Humidity Oversampling 16x
#define CONFIG_BME280_STANDBY_1000MS     //  Standby Time 1000ms
#define CONFIG_BME280_FILTER_4           //  Filter Coefficient 4

//  Other Options
#define BME280_BUS_I2C 0  //  I2C Bus
#define BME280_BUS_SPI 0  //  SPI Bus

//  Zephyr Sensor Channels to be fetched from the sensor
enum sensor_channel {
    SENSOR_CHAN_ALL,
    SENSOR_CHAN_AMBIENT_TEMP,
    SENSOR_CHAN_PRESS,
    SENSOR_CHAN_HUMIDITY,
};

//  Zephyr Sensor Value
struct sensor_value {
    int32_t val1;  //  Integer part of the value
    int32_t val2;  //  Fractional part of the value (in one-millionth parts)
};

#include "bme280/bme280.c"  //  Zephyr BME280 Driver

//  TODO: char *name;                   /* TODO: Name of the device */
//  TODO: struct bme280_data *data;     /* TODO: Compensation parameters */

////#include "bme280/driver.c"  //  NuttX Driver Shell
