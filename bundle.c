//  We bundle the Zephyr and NuttX Drivers into a single source file,
//  so we don't need to fix the NuttX build system.
#include <nuttx/config.h>
#include <nuttx/nuttx.h>
#include <unistd.h>
#include <stdlib.h>
#include <fixedmath.h>
#include <errno.h>
#include <debug.h>
#include <assert.h>

//  Zephyr BME280 Options from
//  https://github.com/zephyrproject-rtos/zephyr/blob/main/drivers/sensor/bme280/Kconfig
#define CONFIG_BME280_MODE_NORMAL        //  Normal Sampling Mode (continuous measurements)
#define CONFIG_BME280_TEMP_OVER_2X       //  Temperature Oversamling 2x
#define CONFIG_BME280_PRESS_OVER_16X     //  Pressure Oversampling 16x
#define CONFIG_BME280_HUMIDITY_OVER_16X  //  Humidity Oversampling 16x
#define CONFIG_BME280_STANDBY_1000MS     //  Standby Time 1000ms. Note: Will be overwritten in bme280_set_standby
#define CONFIG_BME280_FILTER_4           //  Filter Coefficient 4
#define CONFIG_PM_DEVICE                 //  Enable Power Management

//  Other Zephyr Defines
#define BME280_BUS_I2C  0  //  I2C Bus
#define BME280_BUS_SPI  0  //  SPI Bus
#define __ASSERT_NO_MSG DEBUGASSERT  //  Assertion check
#define LOG_DBG         sninfo       //  Log info message
#define K_MSEC(ms)      (ms * 1000)  //  Convert milliseconds to microseconds
#define k_sleep(us)     usleep(us)   //  Sleep for microseconds
#define sys_le16_to_cpu(x) (x)       //  Convert from little endian to host endian. TODO: Handle big endian

//  Zephyr Sensor Channel to be fetched from the sensor
enum sensor_channel {
    SENSOR_CHAN_ALL,           //  All Channels
    SENSOR_CHAN_AMBIENT_TEMP,  //  Ambient Temperature
    SENSOR_CHAN_PRESS,         //  Pressure
    SENSOR_CHAN_HUMIDITY,      //  Humidity
};

//  Zephyr Power Management Action
enum pm_device_action {
    PM_DEVICE_ACTION_SUSPEND,  //  Suspend the sensor
    PM_DEVICE_ACTION_RESUME,   //  Resume the sensor
};

//  Zephyr Power Management State
enum pm_device_state {
    PM_DEVICE_STATE_ACTIVE,     //  Sensor is active
    PM_DEVICE_STATE_SUSPENDED,  //  Sensor is suspended
};
 
//  Zephyr Sensor Value
struct sensor_value {
    int32_t val1;  //  Integer part of the value
    int32_t val2;  //  Fractional part of the value (in one-millionth parts)
};

struct device;

//  Get the device state (active / suspended)
static int pm_device_state_get(const struct device *priv,
    enum pm_device_state *state);

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

//  Embed NuttX Driver Wrapper
#include "bme280/driver.c"
