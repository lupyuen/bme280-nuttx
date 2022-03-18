# Apache NuttX Driver for Bosch BME280 I2C Sensor (Temperature + Humidity + Air Pressure) ported from Zephyr OS

Read the article...

-   ["Apache NuttX Driver for BME280 Sensor: Ported from Zephyr OS"](https://lupyuen.github.io/articles/bme280)

# Install Driver

To add this repo to your NuttX project...

```bash
pushd nuttx/nuttx/drivers/sensors
git submodule add https://github.com/lupyuen/bme280-nuttx bme280
ln -s bme280/bundle.c bme280.c
popd

pushd nuttx/nuttx/include/nuttx/sensors
ln -s ../../../drivers/sensors/bme280/bundle.h bme280.h
popd
```

Next update the Makefile and Kconfig...

-   [See the modified Makefile and Kconfig](https://github.com/lupyuen/incubator-nuttx/commit/1e0c62d409c863e866dbdea5d7e1e2d7b6d3cfc0)

Then update the NuttX Build Config...

```bash
## TODO: Change this to the path of our "incubator-nuttx" folder
cd nuttx/nuttx

## Preserve the Build Config
cp .config ../config

## Erase the Build Config and Kconfig files
make distclean

## For BL602: Configure the build for BL602
./tools/configure.sh bl602evb:nsh

## For ESP32: Configure the build for ESP32.
## TODO: Change "esp32-devkitc" to our ESP32 board.
./tools/configure.sh esp32-devkitc:nsh

## Restore the Build Config
cp ../config .config

## Edit the Build Config
make menuconfig 
```

In menuconfig, enable the Bosch BME280 Sensor under "Device Drivers → Sensor Device Support".

Edit the function `bl602_bringup` or `esp32_bringup` in this file...

```text
## For BL602:
nuttx/boards/risc-v/bl602/bl602evb/src/bl602_bringup.c

## For ESP32: Change "esp32-devkitc" to our ESP32 board 
nuttx/boards/xtensa/esp32/esp32-devkitc/src/esp32_bringup.c
```

And call `bme280_register` to register our BME280 Driver:

https://github.com/lupyuen/incubator-nuttx/blob/bme280/boards/risc-v/bl602/bl602evb/src/bl602_bringup.c#L623-L640    

```c
#ifdef CONFIG_SENSORS_BME280
#include <nuttx/sensors/bme280.h>
#endif /* CONFIG_SENSORS_BME280 */
...
int bl602_bringup(void) {
  ...
#ifdef CONFIG_SENSORS_BME280

  /* Init I2C bus for BME280 */

  struct i2c_master_s *bme280_i2c_bus = bl602_i2cbus_initialize(0);
  if (!bme280_i2c_bus)
    {
      _err("ERROR: Failed to get I2C%d interface\n", 0);
    }

  /* Register the BME280 driver */

  ret = bme280_register(0, bme280_i2c_bus);
  if (ret < 0)
    {
      _err("ERROR: Failed to register BME280\n");
    }
#endif /* CONFIG_SENSORS_BME280 */
```

To read the BME280 Sensor, enter this at the NuttX Shell...

```text
nsh> ls /dev/sensor
/dev/sensor:
 baro0
 humi0

nsh> sensortest -n 1 baro0
SensorTest: Test /dev/sensor/baro0 with interval(1000000us), latency(0us)
baro0: timestamp:256220000 value1:981.34 value2:28.73
SensorTest: Received message: baro0, number:1/1

nsh> sensortest -n 1 humi0
SensorTest: Test /dev/sensor/humi0 with interval(1000000us), latency(0us)
humi0: timestamp:553560000 value:92.90
SensorTest: Received message: humi0, number:1/1
```

That's 981.34 millibars, 28.73 degrees Celsius, and 92.90 % relative humidity.

The rest of this doc explains how we ported the BME280 Driver from Zephyr OS to NuttX RTOS.

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

https://github.com/lupyuen/incubator-nuttx/blob/bmp280/boards/risc-v/bl602/bl602evb/include/board.h#L85-L88

```c
/* I2C Configuration */

#define BOARD_I2C_SCL (GPIO_INPUT | GPIO_PULLUP | GPIO_FUNC_I2C | GPIO_PIN4)
#define BOARD_I2C_SDA (GPIO_INPUT | GPIO_PULLUP | GPIO_FUNC_I2C | GPIO_PIN3)
```

We disabled the UART1 Port because it uses the same pins as I2C...

https://github.com/lupyuen/incubator-nuttx/blob/bmp280/boards/risc-v/bl602/bl602evb/include/board.h#L63-L68

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

Configure NuttX to enable the I2C Character Driver, BMP280 Driver and Sensor Test App...

- System Type → BL602 Peripheral Support → I2C0
- Device Drivers → I2C Driver Support
- Device Drivers → I2C Driver Support →  I2C character driver
- Device Drivers → Sensor Device Support
- Device Drivers → Sensor Device Support →  Bosch BMP280 Barometic Pressure Sensor
- Application Configuration → Testing → Sensor driver test
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

# Change I2C Address and Device ID

For testing, we change the I2C Address and Device ID for BME280...

https://github.com/lupyuen/incubator-nuttx/blob/bmp280/drivers/sensors/bmp280.c#L45-L57

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

# Register BMP280 Driver

Register BMP280 Driver at startup...

https://github.com/lupyuen/incubator-nuttx/blob/bmp280/boards/risc-v/bl602/bl602evb/src/bl602_bringup.c#L623-L640

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

# Invalid Device ID

BMP280 Driver fails because the detected Device ID is 0 ... Let's find out why 🤔

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

# Invalid Register ID

Logic Analyser shows that BL602 sent the wrong Register ID to BME280.

To read the Device ID, the Register ID should be `0xD0`, not `0x00`.  Let's fix this 🤔

```text
Write [0xEE]
0x00 + ACK (Register ID is 0x00)
Read [0xEF]
0x00 + NAK (No Acknowledgement, because Register ID is incorrect)
```

![Invalid Register ID](https://lupyuen.github.io/images/bme280-logic.png)

[(Here's why Register ID should be `0xD0`)](https://lupyuen.github.io/articles/i2c#appendix-test-bme280-with-bus-pirate)

# Log I2C Transfers

BL602 NuttX I2C Driver doesn't log the data transferred ... Let's log ourselves

https://github.com/lupyuen/incubator-nuttx/blob/bmp280/arch/risc-v/src/bl602/bl602_i2c.c#L194-L197

```c
static void bl602_i2c_send_data(struct bl602_i2c_priv_s *priv)
{
  ...
  putreg32(temp, BL602_I2C_FIFO_WDATA);
  priv->bytes += count;
  i2cinfo("count=%d, temp=0x%x\n", count, temp); ////
}
```

https://github.com/lupyuen/incubator-nuttx/blob/bmp280/arch/risc-v/src/bl602/bl602_i2c.c#L207-L216

```c
static void bl602_i2c_recvdata(struct bl602_i2c_priv_s *priv)
{
  ...
  count = msg->length - priv->bytes;
  temp  = getreg32(BL602_I2C_FIFO_RDATA);
  i2cinfo("count=%d, temp=0x%x\n", count, temp); ////
```

https://github.com/lupyuen/incubator-nuttx/blob/bmp280/arch/risc-v/src/bl602/bl602_i2c.c#L740-L742

```c
static int bl602_i2c_transfer(struct i2c_master_s *dev,
                              struct i2c_msg_s *   msgs,
                              int                      count)
{
  ...
  for (i = 0; i < count; i++)
    {
      ...
      priv->msgid = i;
      i2cinfo("subflag=%d, subaddr=0x%x, sublen=%d\n", priv->subflag, priv->subaddr, priv->sublen); ////
      bl602_i2c_start_transfer(priv);
```

# Set I2C Sub Address

BL602 has a peculiar I2C Port ... We need to send the I2C Sub Address (Register ID) separately from the I2C Data ... This might cause the BMP280 Driver to fail

https://lupyuen.github.io/articles/i2c#set-i2c-device-address-and-register-address

BL602 NuttX I2C Driver needs us to provide the I2C Sub Address (Register ID)...

https://github.com/lupyuen/incubator-nuttx/blob/bmp280/arch/risc-v/src/bl602/bl602_i2c.c#L719-L738

```c
      /* if msgs[i].flag I2C_M_NOSTOP,means start i2c with subddr */

      if (msgs[i].flags & I2C_M_NOSTOP)
        {
          priv->subflag = 1;
          priv->subaddr = 0;
          for (j = 0; j < msgs[i].length; j++)
            {
              priv->subaddr += msgs[i].buffer[j] << (j * 8);
            }

          priv->sublen = msgs[i].length;
          i++;
        }
      else
        {
          priv->subflag = 0;
          priv->subaddr = 0;
          priv->sublen  = 0;
        }
```

Here's how we patch the NuttX BMP280 Driver to send the Register ID as I2C Sub Address (instead of I2C Data)

https://github.com/lupyuen/incubator-nuttx/blob/bmp280/drivers/sensors/bmp280.c#L202-L217

```c
static uint8_t bmp280_getreg8(FAR struct bmp280_dev_s *priv, uint8_t regaddr)
{
  ...
  msg[0].frequency = priv->freq;
  msg[0].addr      = priv->addr;
#ifdef CONFIG_BL602_I2C0
  //  For BL602: Register ID must be passed as I2C Sub Address
  msg[0].flags     = I2C_M_NOSTOP;
#else
  //  Otherwise pass Register ID as I2C Data
  msg[0].flags     = 0;
#endif  //  CONFIG_BL602_I2C0
  msg[0].buffer    = &regaddr;
  msg[0].length    = 1;
```

https://github.com/lupyuen/incubator-nuttx/blob/bmp280/drivers/sensors/bmp280.c#L244-L257

```c
static int bmp280_getregs(FAR struct bmp280_dev_s *priv, uint8_t regaddr,
                          uint8_t *rxbuffer, uint8_t length)
{
  ...
  msg[0].frequency = priv->freq;
  msg[0].addr      = priv->addr;
#ifdef CONFIG_BL602_I2C0
  //  For BL602: Register ID must be passed as I2C Sub Address
  msg[0].flags     = I2C_M_NOSTOP;
#else
  //  Otherwise pass Register ID as I2C Data
  msg[0].flags     = 0;
#endif  //  CONFIG_BL602_I2C0
  msg[0].buffer    = &regaddr;
  msg[0].length    = 1;
```

We also need to set the I2C Sub Address when writing registers...

https://github.com/lupyuen/incubator-nuttx/blob/bmp280/drivers/sensors/bmp280.c#L286-L300

```c
static int bmp280_putreg8(FAR struct bmp280_dev_s *priv, uint8_t regaddr,
                          uint8_t regval)
{
  ...
  msg[0].frequency = priv->freq;
  msg[0].addr      = priv->addr;
#ifdef CONFIG_BL602_I2C0
  //  For BL602: Register ID and value must be passed as I2C Sub Address
  msg[0].flags     = I2C_M_NOSTOP;
#else
  //  Otherwise pass Register ID and value as I2C Data
  msg[0].flags     = 0;
#endif  //  CONFIG_BL602_I2C0
  msg[0].buffer    = txbuffer;
  msg[0].length    = 2;

  //  For BL602: We read I2C Data because this forces BL602 to send the first message correctly
  msg[1].frequency = priv->freq;
  msg[1].addr      = priv->addr;
  msg[1].flags     = I2C_M_READ;
  msg[1].buffer    = rxbuffer;
  msg[1].length    = sizeof(rxbuffer);
```

We must read after writing the I2C Register ID and value, so that it forces BL602 to write the data.

# BMP280 Driver Loads OK

NuttX BMP280 Driver loads OK on BL602 ... After setting the Register ID as I2C Sub Address! 🎉

NuttX BMP280 Driver appears as "/dev/sensor/baro0"

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

bmp280_getreg8: regaddr=0xd0
bl602_i2c_transfer: subflag=1, subaddr=0xd0, sublen=1
bl602_i2c_recvdata: count=1, temp=0x60
bl602_i2c_transfer: i2c transfer success
bmp280_getreg8: regaddr=0xd0, regval=0x60
bmp280_checkid: devid: 0x60

bmp280_getregs: regaddr=0x88, length=24
bl602_i2c_transfer: subflag=1, subaddr=0x88, sublen=1
bl602_i2c_recvdata: count=24, temp=0x65e66e97
bl602_i2c_recvdata: count=20, temp=0x8f990032
bl602_i2c_recvdata: count=16, temp=0xbd0d581
bl602_i2c_recvdata: count=12, temp=0xffdb1e71
bl602_i2c_recvdata: count=8, temp=0x26acfff9
bl602_i2c_transfer: i2c transfer success

bmp280_initialize: T1 = 28311
bmp280_initialize: T2 = 26086
bmp280_initialize: T3 = 50
bmp280_initialize: P1 = 36761
bmp280_initialize: P2 = -10879
bmp280_initialize: P3 = 3024
bmp280_initialize: P4 = 7793
bmp280_initialize: P5 = -37
bmp280_initialize: P6 = -7
bmp280_initialize: P7 = 9900
bmp280_initialize: P8 = 15288
bmp280_initialize: P9 = 8964

bmp280_putreg8: regaddr=0xf4, regval=0x00
bl602_i2c_transfer: subflag=1, subaddr=0xf4, sublen=2
bl602_i2c_recvdata: count=1, temp=0x10bdd800
bl602_i2c_transfer: i2c transfer success

bmp280_getreg8: regaddr=0xf5
bl602_i2c_transfer: subflag=1, subaddr=0xf5, sublen=1
bl602_i2c_recvdata: count=1, temp=0x10bdd800
bl602_i2c_transfer: i2c transfer success
bmp280_getreg8: regaddr=0xf5, regval=0x00

bmp280_putreg8: regaddr=0xf5, regval=0x00
bl602_i2c_transfer: subflag=1, subaddr=0xf5, sublen=2
bl602_i2c_recvdata: count=1, temp=0x10bdd800
bl602_i2c_transfer: i2c transfer success

bmp280_getreg8: regaddr=0xf5
bl602_i2c_transfer: subflag=1, subaddr=0xf5, sublen=1
bl602_i2c_recvdata: count=1, temp=0x10bdd800
bl602_i2c_transfer: i2c transfer success
bmp280_getreg8: regaddr=0xf5, regval=0x00

sensor_custom_register: Registering /dev/sensor/baro0
bmp280_register: BMP280 driver loaded successfully!

NuttShell (NSH) NuttX-10.2.0-RC0
nsh>
nsh> ls /dev
/dev:
 console
 gpio0
 gpio1
 gpio2
 i2c0
 null
 sensor/
 spi0
 spitest0
 timer0
 urandom
 zero
nsh> ls /dev/sensor
/dev/sensor:
 baro0
nsh>
```

# Run Sensor Test App

Let's run the NuttX Sensor Test App to read the sensor values from "/dev/sensor/baro0"...

https://github.com/lupyuen/incubator-nuttx-apps/blob/bme280/testing/sensortest/sensortest.c

Configure NuttX to enable the Sensor Test App...

- Application Configuration → Testing → Sensor driver test

[(See the .config for BL602)](https://gist.github.com/lupyuen/9d84889f5e2415ecb0f28cea2c2a657f)

Read 10 sensor values from "/dev/sensor/baro0"...

```text
nsh> sensortest -n 10 baro0
SensorTest: Test /dev/sensor/baro0 with interval(1000000us), latency(0us)
baro0: timestamp:30680000 value1:674.93 value2:22.18
baro0: timestamp:30680000 value1:674.93 value2:22.18
baro0: timestamp:30680000 value1:674.93 value2:22.18
baro0: timestamp:30680000 value1:674.93 value2:22.18
baro0: timestamp:30680000 value1:674.93 value2:22.18
baro0: timestamp:30690000 value1:674.93 value2:22.18
baro0: timestamp:30690000 value1:674.93 value2:22.18
baro0: timestamp:30690000 value1:674.93 value2:22.18
baro0: timestamp:30690000 value1:1006.21 value2:30.78
baro0: timestamp:30690000 value1:1006.21 value2:30.78
SensorTest: Received message: baro0, number:10/10

nsh> sensortest -n 10 baro0
SensorTest: Test /dev/sensor/baro0 with interval(1000000us), latency(0us)
baro0: timestamp:61290000 value1:1006.27 value2:30.80
baro0: timestamp:61300000 value1:1006.27 value2:30.80
baro0: timestamp:61300000 value1:1006.27 value2:30.80
baro0: timestamp:61300000 value1:1006.27 value2:30.80
baro0: timestamp:61300000 value1:1006.27 value2:30.80
baro0: timestamp:61300000 value1:1006.27 value2:30.80
baro0: timestamp:61300000 value1:1006.27 value2:30.80
baro0: timestamp:61300000 value1:1006.27 value2:30.80
baro0: timestamp:61310000 value1:1006.27 value2:30.80
baro0: timestamp:61310000 value1:1006.27 value2:30.80
SensorTest: Received message: baro0, number:10/10

nsh> sensortest -n 10 baro0
SensorTest: Test /dev/sensor/baro0 with interval(1000000us), latency(0us)
baro0: timestamp:79360000 value1:1006.22 value2:30.80
baro0: timestamp:79360000 value1:1006.22 value2:30.80
baro0: timestamp:79360000 value1:1006.22 value2:30.80
baro0: timestamp:79370000 value1:1006.22 value2:30.80
baro0: timestamp:79370000 value1:1006.22 value2:30.80
baro0: timestamp:79370000 value1:1006.22 value2:30.80
baro0: timestamp:79370000 value1:1006.22 value2:30.80
baro0: timestamp:79370000 value1:1006.22 value2:30.80
baro0: timestamp:79370000 value1:1006.22 value2:30.80
baro0: timestamp:79370000 value1:1006.22 value2:30.80
SensorTest: Received message: baro0, number:10/10

nsh> sensortest -n 10 baro0
SensorTest: Test /dev/sensor/baro0 with interval(1000000us), latency(0us)
baro0: timestamp:82370000 value1:1006.30 value2:30.81
baro0: timestamp:82370000 value1:1006.30 value2:30.81
baro0: timestamp:82380000 value1:1006.30 value2:30.81
baro0: timestamp:82380000 value1:1006.30 value2:30.81
baro0: timestamp:82380000 value1:1006.30 value2:30.81
baro0: timestamp:82380000 value1:1006.30 value2:30.81
baro0: timestamp:82380000 value1:1006.30 value2:30.81
baro0: timestamp:82380000 value1:1006.30 value2:30.81
baro0: timestamp:82380000 value1:1006.30 value2:30.81
baro0: timestamp:82380000 value1:1006.30 value2:30.81
SensorTest: Received message: baro0, number:10/10

nsh> sensortest -n 10 baro0
SensorTest: Test /dev/sensor/baro0 with interval(1000000us), latency(0us)
baro0: timestamp:83950000 value1:1006.30 value2:30.79
baro0: timestamp:83950000 value1:1006.30 value2:30.79
baro0: timestamp:83950000 value1:1006.30 value2:30.79
baro0: timestamp:83960000 value1:1006.30 value2:30.79
baro0: timestamp:83960000 value1:1006.30 value2:30.79
baro0: timestamp:83960000 value1:1006.30 value2:30.79
baro0: timestamp:83960000 value1:1006.30 value2:30.79
baro0: timestamp:83960000 value1:1006.30 value2:30.79
baro0: timestamp:83960000 value1:1006.30 value2:30.79
baro0: timestamp:83960000 value1:1006.30 value2:30.79
SensorTest: Received message: baro0, number:10/10

nsh> sensortest -n 10 baro0
SensorTest: Test /dev/sensor/baro0 with interval(1000000us), latency(0us)
baro0: timestamp:85310000 value1:1006.24 value2:30.80
baro0: timestamp:85310000 value1:1006.24 value2:30.80
baro0: timestamp:85320000 value1:1006.24 value2:30.80
baro0: timestamp:85320000 value1:1006.24 value2:30.80
baro0: timestamp:85320000 value1:1006.24 value2:30.80
baro0: timestamp:85320000 value1:1006.24 value2:30.80
baro0: timestamp:85320000 value1:1006.24 value2:30.80
baro0: timestamp:85320000 value1:1006.24 value2:30.80
baro0: timestamp:85320000 value1:1006.24 value2:30.80
baro0: timestamp:85320000 value1:1006.24 value2:30.80
SensorTest: Received message: baro0, number:10/10
nsh>
```

That's 1006.24 millibar and 30.8 °C. Yep that looks reasonable for Sunny Singapore by the Seaside 👍

(Air Pressure at Sea Level is 1013.25 millibar)

Detailed log...

```text
nsh> sensortest -n 10 baro0

sensor_ioctl: cmd=a81 arg=4201c384
bmp280_getreg8: regaddr=0xf5
bl602_i2c_transfer: subflag=1, subaddr=0xf5, sublen=1
bl602_i2c_recvdata: count=1, temp=0x10bdd800
bl602_i2c_transfer: i2c transfer success
bmp280_getreg8: regaddr=0xf5, regval=0x00

bmp280_putreg8: regaddr=0xf5, regval=0xa0
bl602_i2c_transfer: subflag=1, subaddr=0xa0f5, sublen=2
bl602_i2c_recvdata: count=1, temp=0x10bdd8a0
bl602_i2c_transfer: i2c transfer success

bmp280_getreg8: regaddr=0xf5
bl602_i2c_transfer: subflag=1, subaddr=0xf5, sublen=1
bl602_i2c_recvdata: count=1, temp=0x10bdd8a0
bl602_i2c_transfer: i2c transfer success
bmp280_getreg8: regaddr=0xf5, regval=0xa0

sensor_ioctl: cmd=a82 arg=4201c388
sensor_ioctl: cmd=a80 arg=00000001
bmp280_putreg8: regaddr=0xf4, regval=0x2f
bl602_i2c_transfer: subflag=1, subaddr=0x2ff4, sublen=2
bl602_i2c_recvdata: count=1, temp=0x10bdd82f
bl602_i2c_transfer: i2c transfer success

SensorTest: Test /dev/sensor/baro0 with interval(1000000us), latency(0us)
sensor_pollnotify: Report events: 01
bmp280_getregs: regaddr=0xf7, length=6
bl602_i2c_transfer: subflag=1, subaddr=0xf7, sublen=1
bl602_i2c_recvdata: count=6, temp=0x80000080
bl602_i2c_recvdata: count=2, temp=0x80000000
bl602_i2c_transfer: i2c transfer success
bmp280_fetch: press = 524288, temp = 524288
baro0: timestamp:20540000 value1:714.07 value2:22.18

sensor_pollnotify: Report events: 01
bmp280_getregs: regaddr=0xf7, length=6
bl602_i2c_transfer: subflag=1, subaddr=0xf7, sublen=1
bl602_i2c_recvdata: count=6, temp=0x80000080
bl602_i2c_recvdata: count=2, temp=0x80000000
bl602_i2c_transfer: i2c transfer success
bmp280_fetch: press = 524288, temp = 524288
baro0: timestamp:20550000 value1:714.07 value2:22.18

sensor_pollnotify: Report events: 01
bmp280_getregs: regaddr=0xf7, length=6
bl602_i2c_transfer: subflag=1, subaddr=0xf7, sublen=1
bl602_i2c_recvdata: count=6, temp=0x80000080
bl602_i2c_recvdata: count=2, temp=0x80000000
bl602_i2c_transfer: i2c transfer success
bmp280_fetch: press = 524288, temp = 524288
baro0: timestamp:20550000 value1:714.07 value2:22.18

sensor_pollnotify: Report events: 01
bmp280_getregs: regaddr=0xf7, length=6
bl602_i2c_transfer: subflag=1, subaddr=0xf7, sublen=1
bl602_i2c_recvdata: count=6, temp=0x80000080
bl602_i2c_recvdata: count=2, temp=0x80000000
bl602_i2c_transfer: i2c transfer success
bmp280_fetch: press = 524288, temp = 524288
baro0: timestamp:20550000 value1:714.07 value2:22.18

sensor_pollnotify: Report events: 01
bmp280_getregs: regaddr=0xf7, length=6
bl602_i2c_transfer: subflag=1, subaddr=0xf7, sublen=1
bl602_i2c_recvdata: count=6, temp=0x86401752
bl602_i2c_recvdata: count=2, temp=0x86400035
bl602_i2c_transfer: i2c transfer success
bmp280_fetch: press = 336244, temp = 549712
baro0: timestamp:20550000 value1:1069.51 value2:30.09

sensor_pollnotify: Report events: 01
bmp280_getregs: regaddr=0xf7, length=6
bl602_i2c_transfer: subflag=1, subaddr=0xf7, sublen=1
bl602_i2c_recvdata: count=6, temp=0x86401752
bl602_i2c_recvdata: count=2, temp=0x86400035
bl602_i2c_transfer: i2c transfer success
bmp280_fetch: press = 336244, temp = 549712
baro0: timestamp:20560000 value1:1069.51 value2:30.09

sensor_pollnotify: Report events: 01
bmp280_getregs: regaddr=0xf7, length=6
bl602_i2c_transfer: subflag=1, subaddr=0xf7, sublen=1
bl602_i2c_recvdata: count=6, temp=0x86401752
bl602_i2c_recvdata: count=2, temp=0x86400035
bl602_i2c_transfer: i2c transfer success
bmp280_fetch: press = 336244, temp = 549712
baro0: timestamp:20560000 value1:1069.51 value2:30.09

sensor_pollnotify: Report events: 01
bmp280_getregs: regaddr=0xf7, length=6
bl602_i2c_transfer: subflag=1, subaddr=0xf7, sublen=1
bl602_i2c_recvdata: count=6, temp=0x86401752
bl602_i2c_recvdata: count=2, temp=0x86400035
bl602_i2c_transfer: i2c transfer success
bmp280_fetch: press = 336244, temp = 549712
baro0: timestamp:20560000 value1:1069.51 value2:30.09

sensor_pollnotify: Report events: 01
bmp280_getregs: regaddr=0xf7, length=6
bl602_i2c_transfer: subflag=1, subaddr=0xf7, sublen=1
bl602_i2c_recvdata: count=6, temp=0x86401752
bl602_i2c_recvdata: count=2, temp=0x86400035
bl602_i2c_transfer: i2c transfer success
bmp280_fetch: press = 336244, temp = 549712
baro0: timestamp:20560000 value1:1069.51 value2:30.09

sensor_pollnotify: Report events: 01
bmp280_getregs: regaddr=0xf7, length=6
bl602_i2c_transfer: subflag=1, subaddr=0xf7, sublen=1
bl602_i2c_recvdata: count=6, temp=0x86401752
bl602_i2c_recvdata: count=2, temp=0x86400035
bl602_i2c_transfer: i2c transfer success
bmp280_fetch: press = 336244, temp = 549712
baro0: timestamp:20570000 value1:1069.51 value2:30.09

SensorTest: Received message: baro0, number:10/10
sensor_ioctl: cmd=a80 arg=00000000
bmp280_putreg8: regaddr=0xf4, regval=0x00
bl602_i2c_transfer: subflag=1, subaddr=0xf4, sublen=2
bl602_i2c_recvdata: count=1, temp=0x80000000
bl602_i2c_transfer: i2c transfer success
nsh>
```

This shows that writing to I2C Registers works OK...

```text
## Register F5 has value 00
bmp280_getreg8: regaddr=0xf5, regval=0x00
...
## Set Register F5 to value A0
bmp280_putreg8: regaddr=0xf5, regval=0xa0
...
## Register F5 now has value A0
bmp280_getreg8: regaddr=0xf5, regval=0xa0
```

Yep the NuttX BMP280 Driver works OK! Now let's port the BME280 Driver from Zephyr OS to NuttX, so we can get the humidity.

# Port BME280 Driver from Zephyr OS

NuttX BMP280 Driver works OK with our BME280 Sensor ... But we're missing one thing: Humidity ... Can we port the BME280 Driver from Zephyr OS? 🤔

https://github.com/zephyrproject-rtos/zephyr/blob/main/drivers/sensor/bme280/bme280.c

Zephyr BME280 Driver looks similar to [NuttX BMP280 Driver](https://github.com/apache/incubator-nuttx/blob/master/drivers/sensors/bmp280.c) ... So porting Zephyr BME280 Driver to NuttX might not be so hard 🤔

`bme280_sample_fetch` and `bme280_channel_get` are explained in the Zephyr Sensor API:

https://docs.zephyrproject.org/latest/reference/peripherals/sensor.html

# Wrap Zephyr Driver as NuttX Driver

Zephyr BME280 Driver builds OK on #NuttX (with a few tweaks) 🎉 

https://github.com/lupyuen/bme280-nuttx/blob/main/bme280.c

Now we wrap the Zephyr Driver as a NuttX Driver...

https://github.com/lupyuen/bme280-nuttx/blob/main/bundle.c

Our NuttX Driver Wrapper wraps around the Zephyr BME280 Driver ... So it works like a NuttX Driver

https://github.com/lupyuen/bme280-nuttx/blob/main/driver.c#L349-L424

```c
static int bme280_fetch(FAR struct sensor_lowerhalf_s *lower,
                        FAR char *buffer, size_t buflen)
{
  sninfo("buflen=%d\n", buflen);
  FAR struct device *priv = container_of(lower,
                                               FAR struct device,
                                               sensor_lower);

  int ret;
  struct timespec ts;
  struct sensor_event_baro baro_data;
  struct sensor_value val;

  if (buflen != sizeof(baro_data))
    {
      return -EINVAL;
    }

  /* Zephyr BME280 Driver assumes that sensor is not in sleep mode */
  if (!priv->activated)
    {
      snerr("Device must be active before fetch\n");
      return -EIO;
    }

  /* Fetch the sensor data (from Zephyr BME280 Driver) */

  ret = bme280_sample_fetch(priv, SENSOR_CHAN_ALL);
  if (ret < 0)
    {
      return ret;
    }

  /* Get the temperature (from Zephyr BME280 Driver) */

  ret = bme280_channel_get(priv, SENSOR_CHAN_AMBIENT_TEMP, &val);
  if (ret < 0)
    {
      return ret;
    }
  baro_data.temperature = get_sensor_value(&val);

  /* Get the pressure (from Zephyr BME280 Driver) */

  ret = bme280_channel_get(priv, SENSOR_CHAN_PRESS, &val);
  if (ret < 0)
    {
      return ret;
    }
  baro_data.pressure = get_sensor_value(&val) * 10;

  /* Get the humidity (from Zephyr BME280 Driver) */

  ret = bme280_channel_get(priv, SENSOR_CHAN_HUMIDITY, &val);
  if (ret < 0)
    {
      return ret;
    }
  float humidity = get_sensor_value(&val);

  /* Get the timestamp */
  
#ifdef CONFIG_CLOCK_MONOTONIC
  clock_gettime(CLOCK_MONOTONIC, &ts);
#else
  clock_gettime(CLOCK_REALTIME, &ts);
#endif
  baro_data.timestamp = 1000000ull * ts.tv_sec + ts.tv_nsec / 1000;

  /* Return the sensor data */

  memcpy(buffer, &baro_data, sizeof(baro_data));
  sninfo("temperature=%f °C, pressure=%f mbar, humidity=%f %%\n", baro_data.temperature, baro_data.pressure, humidity);

  return buflen;
}
```

# Read Sensor Data from Zephyr Driver

Our NuttX BME280 Driver reads the Sensor Data from Zephyr Driver in two steps: 1️⃣ Fetch the sensor sample 2️⃣ Get the channel data

https://github.com/lupyuen/bme280-nuttx/blob/main/driver.c#L374-L421

```c
  /* Fetch the sensor data (from Zephyr BME280 Driver) */

  ret = bme280_sample_fetch(priv, SENSOR_CHAN_ALL);
  if (ret < 0)
    {
      return ret;
    }

  /* Get the temperature (from Zephyr BME280 Driver) */

  ret = bme280_channel_get(priv, SENSOR_CHAN_AMBIENT_TEMP, &val);
  if (ret < 0)
    {
      return ret;
    }
  baro_data.temperature = get_sensor_value(&val);

  /* Get the pressure (from Zephyr BME280 Driver) */

  ret = bme280_channel_get(priv, SENSOR_CHAN_PRESS, &val);
  if (ret < 0)
    {
      return ret;
    }
  baro_data.pressure = get_sensor_value(&val) * 10;

  /* Get the humidity (from Zephyr BME280 Driver) */

  ret = bme280_channel_get(priv, SENSOR_CHAN_HUMIDITY, &val);
  if (ret < 0)
    {
      return ret;
    }
  float humidity = get_sensor_value(&val);

  /* Get the timestamp */
  
#ifdef CONFIG_CLOCK_MONOTONIC
  clock_gettime(CLOCK_MONOTONIC, &ts);
#else
  clock_gettime(CLOCK_REALTIME, &ts);
#endif
  baro_data.timestamp = 1000000ull * ts.tv_sec + ts.tv_nsec / 1000;

  /* Return the sensor data */

  memcpy(buffer, &baro_data, sizeof(baro_data));
  sninfo("temperature=%f °C, pressure=%f mbar, humidity=%f %%\n", baro_data.temperature, baro_data.pressure, humidity);
```

# Power Management

Power Management works a little differently in NuttX vs Zephyr ... Here's how our NuttX BME280 Driver calls the Zephyr Driver to do Power Management

https://github.com/lupyuen/bme280-nuttx/blob/main/driver.c#L315-L343

```c
static int bme280_activate(FAR struct sensor_lowerhalf_s *lower,
                           bool enable)
{
  sninfo("enable=%d\n", enable);
  int ret = 0;

  FAR struct device *priv = container_of(lower,
                                               FAR struct device,
                                               sensor_lower);
  if (enable)
    {
      /* Set power mode to normal */

      ret = bme280_pm_action(priv, PM_DEVICE_ACTION_RESUME);
    }
  else
    {
      /* Set to sleep mode */

      ret = bme280_pm_action(priv, PM_DEVICE_ACTION_SUSPEND);
    }

  if (ret >= 0)
    {
      priv->activated = enable;
    }

  return ret;
}
```

# Standby Duration

BME280 Standby Duration is static in Zephyr but configured at runtime in NuttX ... So we set it in our NuttX BME280 Driver

https://github.com/lupyuen/bme280-nuttx/blob/main/driver.c#L217-L255

```c
static int bme280_set_standby(FAR struct device *priv, uint8_t value)
{
  sninfo("value=%d\n", value);
  
  uint8_t v_data_u8;
  uint8_t v_sb_u8;
  int ret;

  /* Set the standby duration value */

  ret = bme280_reg_read(priv, BME280_REG_CONFIG, &v_data_u8, 1);
  if (ret < 0)
    {
      return ret;
    }
  v_data_u8 = (v_data_u8 & ~(0x07 << 5)) | (value << 5);
  ret = bme280_reg_write(priv, BME280_REG_CONFIG, v_data_u8);
  if (ret < 0)
    {
      return ret;
    }

  /* Check the standby duration value */

  ret = bme280_reg_read(priv, BME280_REG_CONFIG, &v_data_u8, 1);
  if (ret < 0)
    {
      return ret;
    }
  v_sb_u8 = (v_data_u8 >> 5) & 0x07;

  if (v_sb_u8 != value)
    {
      snerr("Failed to set value for standby time.");
      return ERROR;
    }

  return OK;
}
```

# Zephyr Driver Modified For NuttX

Here are the minor modifications we made to the Zephyr BME280 Driver while porting to NuttX...

bme280.c:

https://github.com/lupyuen/bme280-nuttx/pull/1/files#diff-80464162211b7180f107757b7aee91398cdc088e5775ffadf7e6e1f0bbb4ad65

bme280.h:

https://github.com/lupyuen/bme280-nuttx/pull/1/files#diff-e13ff0ab44de7ead31a3dd6cbbbbf2a6fbfb2f04889300993b87ff5a31ffc233

The above files are wrapped by [bundle.c](bundle.c) and [bundle.h](bundle.h) to become a NuttX Driver.

# Combined Barometer and Humidity Sensor

NuttX doesn't have a Sensor Type that supports BME280 Temperature + Humidity + Pressure ... So our NuttX BME280 Driver combines 2 Sensor Types: 1️⃣ Barometer Sensor (Pressure + Temperature) 2️⃣ Humidity Sensor

https://github.com/lupyuen/bme280-nuttx/blob/main/device.h#L36-L49

```c
/* NuttX Device for BME280 */

struct device
{
  FAR struct sensor_lowerhalf_s sensor_baro;  /* Barometer and Temperature Sensor */
  FAR struct sensor_lowerhalf_s sensor_humi;  /* Humidity Sensor */
  FAR struct i2c_master_s *i2c; /* I2C interface */
  uint8_t addr;                 /* BME280 I2C address */
  int freq;                     /* BME280 Frequency <= 3.4MHz */
  bool activated;               /* True if device is not in sleep mode */

  char *name;                   /* Name of the device */
  struct bme280_data *data;     /* Compensation parameters (bme280.c) */
};
```

Each NuttX Sensor defines its operations for 1️⃣ Activating the sensor 2️⃣ Fetching sensor data 3️⃣ Setting the standby interval

https://github.com/lupyuen/bme280-nuttx/blob/main/driver.c#L71-L87

```c
/* Operations for Barometer and Temperature Sensor */

static const struct sensor_ops_s g_baro_ops =
{
  .activate      = bme280_activate_baro,
  .fetch         = bme280_fetch_baro,
  .set_interval  = bme280_set_interval_baro,
};

/* Operations for Humidity Sensor */

static const struct sensor_ops_s g_humi_ops =
{
  .activate      = bme280_activate_humi,
  .fetch         = bme280_fetch_humi,
  .set_interval  = bme280_set_interval_humi,
};
```

At NuttX Startup we register both BME280 sensors: Barometer Sensor and Humidity Sensor

https://github.com/lupyuen/bme280-nuttx/blob/main/driver.c#L755-L773

```c
int bme280_register(int devno, FAR struct i2c_master_s *i2c)
{
  ...
  /* Register the Barometer Sensor */

  ret = sensor_register(&priv->sensor_baro, devno);
  if (ret < 0)
    {
      snerr("Failed to register barometer sensor: %d\n", ret);
      kmm_free(data);
      kmm_free(priv);
    }

  /* Register the Humidity Sensor */

  ret = sensor_register(&priv->sensor_humi, devno);
  if (ret < 0)
    {
      snerr("Failed to register humidity sensor: %d\n", ret);
      kmm_free(data);
      kmm_free(priv);
    }
```

Our NuttX BME280 Driver appears as 2 sensors: 1️⃣ "/dev/sensor/baro0" (Barometer Sensor) 2️⃣ "/dev/sensor/humi0" (Humidity Sensor)

This is how we read each sensor:

```text
nsh> ls /dev/sensor
/dev/sensor:
 baro0
 humi0

nsh> sensortest -n 1 baro0
SensorTest: Test /dev/sensor/baro0 with interval(1000000us), latency(0us)
baro0: timestamp:256220000 value1:981.34 value2:28.73
SensorTest: Received message: baro0, number:1/1

nsh> sensortest -n 1 humi0
SensorTest: Test /dev/sensor/humi0 with interval(1000000us), latency(0us)
humi0: timestamp:553560000 value:92.90
SensorTest: Received message: humi0, number:1/1
```

# Output Log

Here's the detailed log when BME280 Driver loads during startup...

```text
spi_test_driver_register: devpath=/dev/spitest0, spidev=0
bme280_register: devno=0
bme280_register: priv=0x4201b800, sensor_baro=0x4201b800, sensor_humi=0x4201b81c
bl602_i2c_transfer: subflag=1, subaddr=0xd0, sublen=1
bl602_i2c_recvdata: count=1, temp=0x60
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xd0, size=1, buf[0]=0x60
bme280_chip_init: ID OK

bme280_reg_write: reg=0xe0, val=0xb6
bl602_i2c_transfer: subflag=1, subaddr=0xb6e0, sublen=2
bl602_i2c_recvdata: count=1, temp=0x0
bl602_i2c_transfer: i2c transfer success

bl602_i2c_transfer: subflag=1, subaddr=0xf3, sublen=1
bl602_i2c_recvdata: count=1, temp=0x0
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xf3, size=1, buf[0]=0x00

bl602_i2c_transfer: subflag=1, subaddr=0x88, sublen=1
bl602_i2c_recvdata: count=24, temp=0x65e66e97
bl602_i2c_recvdata: count=20, temp=0x8f990032
bl602_i2c_recvdata: count=16, temp=0xbd0d581
bl602_i2c_recvdata: count=12, temp=0xffdb1e71
bl602_i2c_recvdata: count=8, temp=0x10bdd80a
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0x88, size=24

bl602_i2c_transfer: subflag=1, subaddr=0xa1, sublen=1
bl602_i2c_recvdata: count=1, temp=0x10bdd84b
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xa1, size=1, buf[0]=0x4b

bl602_i2c_transfer: subflag=1, subaddr=0xe1, sublen=1
bl602_i2c_recvdata: count=7, temp=0x14000165
bl602_i2c_recvdata: count=3, temp=0x141e000b
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xe1, size=7

bme280_reg_write: reg=0xf2, val=0x05
bl602_i2c_transfer: subflag=1, subaddr=0x5f2, sublen=2
bl602_i2c_recvdata: count=1, temp=0x141e0005
bl602_i2c_transfer: i2c transfer success

bme280_reg_write: reg=0xf4, val=0x57
bl602_i2c_transfer: subflag=1,subaddr=0x57f4, sublen=2
bl602_i2c_recvdata: count=1, temp=0x141e0057
bl602_i2c_transfer: i2c transfer success

bme280_reg_write: reg=0xf5, val=0xa8
bl602_i2c_transfer: subflag=1, subaddr=0xa8f5, sublen=2
bl602_i2c_recvdata: count=1, temp=0x141e00a8
bl602_i2c_transfer: i2c transfer success
bme280_chip_init: "BME280" OK

bme280_reg_write: reg=0xf4, val=0x54
bl602_i2c_transfer: subflag=1, subaddr=0x54f4, sublen=2
bl602_i2c_recvdata: count=1, temp=0x141e0054
bl602_i2c_transfer: i2c transfer success

sensor_custom_register: Registering /dev/sensor/baro0
sensor_custom_register: Registering /dev/sensor/humi0
bme280_register: BME280 driver loaded successfully!

NuttShell (NSH) NuttX-10.2.0-RC0
nsh>
```

Here's the detailed log when we read the Barometer Sensor Data from BME280 Driver...

```text
nsh> sensortest -n 1 baro0

sensor_ioctl: cmd=a81 arg=4201c4b4
bme280_set_interval_baro: period_us=1000000
bme280_set_interval_baro: priv=0x4201b800, sensor_baro=0x4201b800
bme280_set_standby: value=5

bl602_i2c_transfer: subflag=1, subaddr=0xf5, sublen=1
bl602_i2c_recvdata: count=1, temp=0xcf9400a8
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xf5, size=1, buf[0]=0xa8

bme280_reg_write: reg=0xf5, val=0xa8
bl602_i2c_transfer: subflag=1, subaddr=0xa8f5, sublen=2
bl602_i2c_recvdata: count=1, temp=0xcf9400a8
bl602_i2c_transfer: i2c transfer success

bl602_i2c_transfer: subflag=1, subaddr=0xf5, sublen=1
bl602_i2c_recvdata: count=1, temp=0xcf9400a8
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xf5, size=1, buf[0]=0xa8

sensor_ioctl: cmd=a82 arg=4201c4b8
sensor_ioctl: cmd=a80 arg=00000001
bme280_activate_baro: enable=1
bme280_activate_baro: priv=0x4201b800, sensor_baro=0x4201b800

bl602_i2c_transfer: subflag=1, subaddr=0xd0, sublen=1
bl602_i2c_recvdata: count=1, temp=0xcf940060
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xd0, size=1, buf[0]=0x60
bme280_chip_init: ID OK

bme280_reg_write: reg=0xe0, val=0xb6
bl602_i2c_transfer: subflag=1, subaddr=0xb6e0, sublen=2
bl602_i2c_recvdata: count=1, temp=0xcf940000
bl602_i2c_transfer: i2c transfer success

bl602_i2c_transfer: subflag=1, subaddr=0xf3, sublen=1
bl602_i2c_recvdata: count=1, temp=0xcf940000
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xf3, size=1, buf[0]=0x00

bl602_i2c_transfer: subflag=1, subaddr=0x88, sublen=1
bl602_i2c_recvdata: count=24, temp=0x65e66e97
bl602_i2c_recvdata: count=20, temp=0x8f990032
bl602_i2c_recvdata: count=16, temp=0xbd0d581
bl602_i2c_recvdata: count=12, temp=0xffdb1e71
bl602_i2c_recvdata: count=8, temp=0x10bdd80a
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0x88, size=24

bl602_i2c_transfer: subflag=1, subaddr=0xa1, sublen=1
bl602_i2c_recvdata: count=1, temp=0x10bdd84b
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xa1, size=1, buf[0]=0x4b

bl602_i2c_transfer: subflag=1, subaddr=0xe1, sublen=1
bl602_i2c_recvdata: count=7, temp=0x14000165
bl602_i2c_recvdata: count=3, temp=0x141e000b
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xe1, size=7

bme280_reg_write: reg=0xf2, val=0x05
bl602_i2c_trasfer: subflag=1, subaddr=0x5f2, sublen=2
bl602_i2c_recvdata: count=1, temp=0x141e0005
bl602_i2c_transfer: i2c transfer success

bme280_reg_write: reg=0xf4, val=0x57
bl602_i2c_transfer: subflag=1, subaddr=0x57f4, sublen=2
bl602_i2c_recvdata: count=1, temp=0x141e0057
bl602_i2c_transfer: i2c transfer success

bme280_reg_write: reg=0xf5, val=0xa8
bl602_i2c_transfer: subflag=1, subaddr=0xa8f5, sublen=2
bl602_i2c_recvdata: count=1, temp=0x141e00a8
bl602_i2c_transfer: i2c transfer success
bme280_chip_init: "BME280" OK

SensorTest: Test /dev/sensor/baro0 with interval(1000000us), latency(0us)
sensor_pollnotify: Report events: 01
bme280_fetch_baro: buflen=16
bme280_fetch_baro: priv=0x4201b800, sensor_baro=0x4201b800

bl602_i2c_transfer: subflag=1, subaddr=0xf3, sublen=1
bl602_i2c_recvdata: count=1, temp=0x141e000c
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xf3, size=1, buf[0]=0x0c

bl602_i2c_transfer: subflag=1, subaddr=0xf3, sublen=1
bl602_i2c_recvdata: count=1, temp=0x141e000c
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xf3, size=1, buf[0]=0x0c

bl602_i2c_transfer: subflag=1, subaddr=0xf3, sublen=1
bl602_i2c_recvdata: count=1, temp=0x141e0004
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xf3, size=1, buf[0]=0x04

bl602_i2c_transfer: subflag=1, subaddr=0xf7, sublen=1
bl602_i2c_recvdata: count=8, temp=0x8530af51
bl602_i2c_recvdata: count=4, temp=0x61940024
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xf7, size=8

bme280_fetch: temperature=28.730000 °C, pressure=981.339661 mbar, humidity=93.127930 %
baro0: timestamp:256220000 value1:981.34 value2:28.73
SensorTest: Received message: baro0, number:1/1
sensor_ioctl: cmd=a80 arg=00000000
bme280_activate_baro: enable=0
bme280_activate_baro: priv=0x4201b800, sensor_baro=0x4201b800

bme280_reg_write: reg=0xf4, val=0x54
bl602_i2c_transfer: subflag=1, subaddr=0x54f4, sublen=2
bl602_i2c_recvdata: count=1, temp=0x61940054
bl602_i2c_transfer: i2c transfer success
```

Here's the detailed log when we read the Humidity Sensor Data from BME280 Driver...

```text
nsh> sensortest -n 1 humi0

sensor_ioctl: cmd=a81 arg=4201c4b4
bme280_set_interval_humi: period_us=1000000
bme280_set_interval_humi: priv=0x4201b800, sensor_humi=0x4201b81c
bme280_set_standby: value=5

bl602_i2c_transfer: subflag=1, subaddr=0xf5, sublen=1
bl602_i2c_recvdata: count=1, temp=0x619400a8
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xf5, size=1, buf[0]=0xa8

bme280_reg_write: reg=0xf5, val=0xa8
bl602_i2c_transfer: subflag=1, subaddr=0xa8f5, sublen=2
bl602_i2c_recvdata: cunt=1, temp=0x619400a8
bl602_i2c_transfer: i2c transfer success

bl602_i2c_transfer: subflag=1, subaddr=0xf5, sublen=1
bl602_i2c_recvdata: count=1, temp=0x619400a8
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xf5, size=1, buf[0]=0xa8

sensor_ioctl: cmd=a82 arg=4201c4b8
sensor_ioctl: cmd=a80 arg=00000001
bme280_activate_humi: enable=1
bme280_activate_humi: priv=0x4201b800, sensor_humi=0x4201b81c

bl602_i2c_transfer: subflag=1, subaddr=0xd0, sublen=1
bl602_i2c_recvdata: count=1, temp=0x61940060
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xd0, size=1, buf[0]=0x60
bme280_chip_init: ID OK

bme280_reg_write: reg=0xe0, val=0xb6
bl602_i2c_transfer: subflag=1, subaddr=0xb6e0, sublen=2
bl602_i2c_recvdata: count=1, temp=0x61940000
bl602_i2c_transfer: i2c transfer success
bl602_i2c_transfer: subflag=1, subaddr=0xf3, sublen=1
bl602_i2c_recvdata: count=1, temp=0x61940000
bl602_i2c_transfer: i2c transfer success

bme280_reg_read: start=0xf3, size=1, buf[0]=0x00
bl602_i2c_transfer: subflag=1, subaddr=0x88, sublen=1
bl602_i2c_recvdata: count=24, temp=0x65e66e97
bl602_i2c_recvdata: count=20, temp=0x8f990032
bl602_i2c_recvdata: count=16, temp=0xbd0d581
bl602_i2c_recvdata: count=12, temp=0xffdb1e71
bl602_i2c_recvdata: count=8, temp=0x10bdd80a
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0x88, size=24

bl602_i2c_transfer: subflag=1, subaddr=0xa1, sublen=1
bl602_i2c_recvdata: count=1, temp=0x10bdd84b
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xa1, size=1, buf[0]=0x4b

bl602_i2c_transfer: subflag=1, subaddr=0xe1, sublen=1
bl602_i2c_recvdata: count=7, temp=0x14000165
bl602_i2c_recvdata: count=3, temp=0x141e000b
bl02_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xe1, size=7

bme280_reg_write: reg=0xf2, val=0x05
bl602_i2c_transfer: subflag=1, subaddr=0x5f2, sublen=2
bl602_i2c_recvdata: count=1, temp=0x141e0005
bl602_i2c_transfer: i2c transfer success

bme280_reg_write: reg=0xf4, val=0x57
bl602_i2c_transfer: subflag=1, subaddr=0x57f4, sublen=2
bl602_i2c_recvdata: count=1, temp=0x141e0057
bl602_i2c_transfer: i2c transfer success

bme280_reg_write: reg=0xf5, val=0xa8
bl602_i2c_transfer: subflag=1, subaddr=0xa8f5, sublen=2
bl602_i2c_recvdata: count=1, temp=0x141e00a8
bl602_i2c_transfer: i2c transfer success
bme280_chip_init: "BME280" OK

SensorTest: Test /dev/sensor/humi0 with interval(1000000us), latency(0us)
sensor_pollnotify: Report events: 01
bme280_fetch_humi: buflen=16
bme280_fetch_humi: priv=0x4201b800, sensor_humi=0x4201b81c

bl602_i2c_transfer: subflag=1, subaddr=0xf3, sublen=1
bl602_i2c_recvdata: count=1, temp=0x141e000c
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xf3, size=1, buf[0]=0x0c

bl602_i2c_transfer: subflag=1, subaddr=0xf3, sublen=1
bl602_i2c_recvdata: count=1, temp=0x141e000c
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xf3, size=1, buf[0]=0x0c

bl602_i2c_transfer: subflag=1, subaddr=0xf3, sublen=1
bl602_i2c_recvdata: count=1, temp=0x141e0004
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xf3, size=1, buf[0]=0x04

bl602_i2c_transfer: subflag=1, subaddr=0xf7, sublen=1
bl602_i2c_recvdata: count=8, temp=0x85b0a451
l602_i2c_recvdata: count=4, temp=0x3b94000d
bl602_i2c_transfer: i2c transfer success
bme280_reg_read: start=0xf7, size=8

bme280_fetch: temperature=28.610001 °C, pressure=1028.345703 mbar, humidity=92.896484 %
humi0: timestamp:553560000 value:92.90
SensorTest: Received message: humi0, number:1/1
sensor_ioctl: cmd=a80 arg=00000000
bme280_activate_humi: enable=0
bme280_activate_humi: priv=0x4201b800, sensor_humi=0x4201b81c

bme280_reg_write: reg=0xf4, val=0x54
bl602_i2c_transfer: subflag=1, subaddr=0x54f4, sublen=2
bl602_i2c_recvdata: count=1, temp=0x3b940054
bl602_i2c_transfer: i2c transfer success
```

# Logic Analyser Output

Here's the Logic Analyser Output when BME280 Driver loads during startup...

![Top](https://lupyuen.github.io/images/bme280-boot1.png)

![Bottom](https://lupyuen.github.io/images/bme280-boot2.png)

Here's the Logic Analyser Output when we read the Sensor Data from BME280 Driver...

(`sensortest -n 1 baro0` from the previous section)

![Top](https://lupyuen.github.io/images/bme280-read1.png)

![Middle](https://lupyuen.github.io/images/bme280-read2.png)

![Bottom](https://lupyuen.github.io/images/bme280-read3.png)
