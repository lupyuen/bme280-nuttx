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

https://github.com/lupyuen/incubator-nuttx/blob/bme280/arch/risc-v/src/bl602/bl602_i2c.c#L194-L197

```c
static void bl602_i2c_send_data(struct bl602_i2c_priv_s *priv)
{
  ...
  putreg32(temp, BL602_I2C_FIFO_WDATA);
  priv->bytes += count;
  i2cinfo("count=%d, temp=0x%x\n", count, temp); ////
}
```

https://github.com/lupyuen/incubator-nuttx/blob/bme280/arch/risc-v/src/bl602/bl602_i2c.c#L207-L216

```c
static void bl602_i2c_recvdata(struct bl602_i2c_priv_s *priv)
{
  ...
  count = msg->length - priv->bytes;
  temp  = getreg32(BL602_I2C_FIFO_RDATA);
  i2cinfo("count=%d, temp=0x%x\n", count, temp); ////
```

https://github.com/lupyuen/incubator-nuttx/blob/bme280/arch/risc-v/src/bl602/bl602_i2c.c#L740-L742

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

https://github.com/lupyuen/incubator-nuttx/blob/bme280/arch/risc-v/src/bl602/bl602_i2c.c#L719-L738

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

https://github.com/lupyuen/incubator-nuttx/blob/bme280/drivers/sensors/bmp280.c#L202-L217

```c
static uint8_t bmp280_getreg8(FAR struct bmp280_dev_s *priv, uint8_t regaddr)
{
  ...
  //// Previously:
  //// msg[0].flags     = 0;

  #warning Testing: I2C_M_NOSTOP for I2C Sub Address
  msg[0].flags     = I2C_M_NOSTOP;  ////  Testing I2C Sub Address

  msg[0].buffer    = &regaddr;
  msg[0].length    = 1;
```

https://github.com/lupyuen/incubator-nuttx/blob/bme280/drivers/sensors/bmp280.c#L244-L257

```c
static int bmp280_getregs(FAR struct bmp280_dev_s *priv, uint8_t regaddr,
                          uint8_t *rxbuffer, uint8_t length)
{
  ...
  msg[0].frequency = priv->freq;
  msg[0].addr      = priv->addr;

  //// Previously:
  //// msg[0].flags     = 0;

  #warning Testing: I2C_M_NOSTOP for I2C Sub Address
  msg[0].flags     = I2C_M_NOSTOP;  ////  Testing I2C Sub Address

  msg[0].buffer    = &regaddr;
  msg[0].length    = 1;
```

We don't need to set the I2C Sub Address when writing registers...

https://github.com/lupyuen/incubator-nuttx/blob/bme280/drivers/sensors/bmp280.c#L286-L300

```c
static int bmp280_putreg8(FAR struct bmp280_dev_s *priv, uint8_t regaddr,
                          uint8_t regval)
{
  ...
  txbuffer[0] = regaddr;
  txbuffer[1] = regval;

  msg[0].frequency = priv->freq;
  msg[0].addr      = priv->addr;
  msg[0].flags     = 0;
  msg[0].buffer    = txbuffer;
  msg[0].length    = 2;
```

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
bl602_i2c_transfer: subflag=0, subaddr=0x0, sublen=0
bl602_i2c_send_data: count=2, temp=0xf4
bl602_i2c_transfer: i2c transfer success

bmp280_getreg8: regaddr=0xf5
bl602_i2c_transfer: subflag=1, subaddr=0xf5, sublen=1
bl602_i2c_recvdata: count=1, temp=0x10bdd800
bl602_i2c_transfer: i2c transfer success
bmp280_getreg8: regaddr=0xf5, regval=0x00

bmp280_putreg8: regaddr=0xf5, regval=0x00
bl602_i2c_transfer: subflag=0, subaddr=0x0, sublen=0
bl602_i2c_send_data: count=2, temp=0xf5
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
bl602_i2c_transfer: i2c transfer error, event = 4

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
bl602_i2c_transfer: i2c transfer error, event = 4

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

(Air Pressue at Sea Level is 1013.25 millibar)

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
bl602_i2c_transfer: subflag=0, subaddr=0x0, sublen=0
bl602_i2c_send_data: count=2, temp=0xa0f5
bl602_i2c_transfer: i2c transfer success

bmp280_getreg8: regaddr=0xf5
bl602_i2c_transfer: subflag=1, subaddr=0xf5, sublen=1
bl602_i2c_recvdata: count=1, temp=0x10bdd8a0
bl602_i2c_transfer: i2c transfer success
bmp280_getreg8: regaddr=0xf5, regval=0xa0

sensor_ioctl: cmd=a82 arg=4201c388
sensor_ioctl: cmd=a80 arg=00000001
bmp280_putreg8: regaddr=0xf4, regval=0x2f
bl602_i2c_transfer: subflag=0, subaddr=0x0, sublen=0
bl602_i2c_send_data: count=2, temp=0x2ff4
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
bl602_i2c_transfer: subflag=0, subaddr=0x0, sublen=0
bl602_i2c_send_data: count=2, temp=0xf4
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

`bme280_sample_fetch` and `bme280_channel_get` are explained in the Zephyr Sensor API: https://docs.zephyrproject.org/latest/reference/peripherals/sensor.html

TODO

# Install Driver

To add this repo to your NuttX project...

```bash
cd nuttx/drivers/sensors
git submodule add https://github.com/lupyuen/bme280-nuttx bme280
```

TODO: Next update the Makefiles and Kconfig...

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
