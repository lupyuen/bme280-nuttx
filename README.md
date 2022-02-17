# Apache NuttX Driver for Bosch BME280 I2C Sensor (Temperature + Humidity + Air Pressure) ported from Zephyr OS

[__Follow the updates on Twitter__](https://twitter.com/MisterTechBlog/status/1494301654186823683)

Will Apache NuttX OS talk I2C with Bosch BME280 Sensor? (Temperature + Humidity + Air Pressure) ... Let's find out!

# Test with Bus Pirate

[__Bus Pirate__](http://dangerousprototypes.com/docs/Bus_Pirate) is a useful gadget for verifying whether our BME280 Sensor works OK. And for checking the I2C bytes that should be sent down the wire to BME280.

Here's how we test BME280 with Bus Pirate...

| Bus Pirate Pin | BME280 Pin
|:---:|:---:
| __`MOSI`__ | `SDA`
| __`CLK`__ | `SCL`
| __`3.3V`__ | `3.3V`
| __`GND`__ | `GND`

More details: https://lupyuen.github.io/articles/i2c#appendix-test-bme280-with-bus-pirate

# Connect BME280

Connect BME280 to Pine64 PineCone BL602...

| BL602 Pin | BME280 Pin | Wire Colour
|:---:|:---:|:---|
| __`GPIO 3`__ | `SDA` | Green 
| __`GPIO 4`__ | `SCL` | Blue
| __`3V3`__ | `3.3V` | Red
| __`GND`__ | `GND` | Black

The I2C Pins on BL602 are defined here...

https://github.com/lupyuen/incubator-nuttx/blob/bme280/boards/risc-v/bl602/bl602evb/include/board.h#L85-L88

```c
/* I2C Configuration */

#define BOARD_I2C_SCL (GPIO_INPUT | GPIO_PULLUP | GPIO_FUNC_I2C | GPIO_PIN4)
#define BOARD_I2C_SDA (GPIO_INPUT | GPIO_PULLUP | GPIO_FUNC_I2C | GPIO_PIN3)
```

We disabled the UART1 Port because it uses the same pins as I2C...

https://github.com/lupyuen/incubator-nuttx/blob/bme280/boards/risc-v/bl602/bl602evb/include/board.h#L63-L68

```c
#ifdef TODO  /* Remember to check for duplicate pins! */
#define BOARD_UART_1_RX_PIN (GPIO_INPUT | GPIO_PULLUP | \
                              GPIO_FUNC_UART | GPIO_PIN3)
#define BOARD_UART_1_TX_PIN (GPIO_INPUT | GPIO_PULLUP | \
                              GPIO_FUNC_UART | GPIO_PIN4)
#endif  /* TODO */
```

Verify with a Multimeter that BME280 is powered up with 3.3V.

We're using the SparkFun BME280 Breakout Board, which has Pull-Up Resistors (so we don't need to add our own)...

https://learn.sparkfun.com/tutorials/sparkfun-bme280-breakout-hookup-guide/all

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

For testing, we change the I2C Address and Device ID for BME280...

https://github.com/lupyuen/incubator-nuttx/blob/bme280/drivers/sensors/bmp280.c#L45-L57

```c
////  Previously: I2C Address of BMP280
////  #define BMP280_ADDR         0x76

#warning Testing: I2C Address of BME280
#define BMP280_ADDR         0x77 //// BME280

////  Previously: Device ID of BMP280
////  #define DEVID               0x58

#warning Testing: Device ID of BME280
#define DEVID               0x60 //// BME280
```

# TODO

Returned device ID is 0. Why???

```text
gpio_pin_register: Registering /dev/gpio0
gpio_pin_register: Registering /dev/gpio1
gpint_enable: Disable the interrupt
gpio_pin_register: Registering /dev/gpio2
bl602_gpio_set_intmod: ****gpio_pin=115, int_ctlmod=1, int_trgmod=0
bl602_spi_setfrequency: frequency=400000, actual=0
bl602_spi_setbits: nbits=8
bl602_spi_setmode: mode=0
spi_test_driver_register: devpath=/dev/spitest0, spidev=0
bl602_spi_select: devid: 0, CS: free
bl602_i2c_transfer: i2c transfer success
bl602_i2c_transfer: i2c transfer success
bmp280_checkid: devid: 0x00
bmp280_checkid: Wrong Device ID! 00
bmp280_register: Failed to register driver: -19
bl602_bringup: ERROR: Failed to register BMP280

NuttShell (NSH) NuttX-10.2.0-RC0
nsh>
```

TODO: Probe BL602 I2C Port with Logic Analyser

TODO: Log data transferred over I2C

TODO: Port the BME280 Driver from Zephyr OS to NuttX

https://github.com/zephyrproject-rtos/zephyr/blob/main/drivers/sensor/bme280/bme280.c
