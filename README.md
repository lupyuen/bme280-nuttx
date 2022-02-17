# Apache NuttX Driver for Bosch BME280 I2C Sensor (Temperature + Humidity + Air Pressure) ported from Zephyr OS

Will Apache NuttX OS talk I2C with Bosch BME280 Sensor? (Temperature + Humidity + Air Pressure) ... Let's find out!

# Connect BME280

Connect BME280 to Pine64 PineCone BL602...

| BL602 Pin | BME280 Pin | Wire Colour
|:---:|:---:|:---|
| __`GPIO 3`__ | `SDA` | Green 
| __`GPIO 4`__ | `SCL` | Blue
| __`3V3`__ | `3.3V` | Red
| __`GND`__ | `GND` | Black

# Configure NuttX

NuttX has a driver for BMP280 (Air Pressure only), let's test it with BME280.

Configure NuttX for I2C to enable the I2C Character Driver, BMP280 Driver and BMP280 App...

- System Type → BL602 Peripheral Support → I2C0
- Device Drivers → I2C Driver Support
- Device Drivers → I2C Driver Support →  I2C character driver
- Device Drivers → Sensor Device Support
- Device Drivers → Sensor Device Support →  Bosch BMP280 Barometic Pressure Sensor
- Application Configuration → Examples →  BMP180/280 Barometer sensor example
- Build Setup → Debug Options
  - → Enable Informational Debug Output 
  - → I2C Debug Features
  - → I2C Debug Features →  I2C Error Output
  - → I2C Debug Features →  I2C Warnings Output
  - → I2C Debug Features →  I2C Informational Output  
  - → Sensor Debug Features
  - → Sensor Debug Features → Sensor Error Output
  - → Sensor Debug Features → Sensor Warnings Output  
  - → Sensor Debug Features → Sensor Informational Output 

[(See the .config for BL602)](https://gist.github.com/lupyuen/9d84889f5e2415ecb0f28cea2c2a657f)

# Register BMP280 Driver

Register BMP280 Driver at startup...

https://github.com/lupyuen/incubator-nuttx/blob/bme280/boards/risc-v/bl602/bl602evb/src/bl602_bringup.c#L623-L640

```c
#ifdef CONFIG_SENSORS_BMP280
#include <nuttx/sensors/bmp280.h>
#endif /* CONFIG_SENSORS_BMP280 */
...
int bl602_bringup(void)
{
...
#ifdef CONFIG_SENSORS_BMP280

  /* Init I2C bus for BMP280 */

  struct i2c_master_s *bmp280_i2c_bus = bl602_i2cbus_initialize(0);
  if (!bmp280_i2c_bus)
    {
      _err("ERROR: Failed to get I2C%d interface\n", 0);
    }

  /* Register the BMP280 driver */

  ret = bmp280_register(0, bmp280_i2c_bus);
  if (ret < 0)
    {
      _err("ERROR: Failed to register BMP280\n");
    }
#endif /* CONFIG_SENSORS_BMP280 */
```

# Change I2C Address and Device ID

For testing, we change the I2C address and Device ID for BME280...

https://github.com/lupyuen/incubator-nuttx/blob/bme280/drivers/sensors/bmp280.c#L45-L57

```c
////  Previously: I2C Address of BMP280
////  #define BMP280_ADDR         0x76

#warning Testing: I2C Address of BME280
#define BMP280_ADDR         0x77 //// BME280

////  Previously: Device ID of BMP280
////  #define DEVID               0x58

#warning Testing: Device ID of BME280
#define DEVID               0x60 //// BME260
```

# TODO

Returned device ID is 0. Why???
