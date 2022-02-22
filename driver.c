/****************************************************************************
 * drivers/sensors/bme280/driver.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/nuttx.h>

#include <stdlib.h>
#include <fixedmath.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/kmalloc.h>
#include <nuttx/fs/fs.h>
#include <nuttx/i2c/i2c_master.h>
#include "driver.h" //// Previously: <nuttx/sensors/bme280.h>
#include <nuttx/sensors/sensor.h>

#if defined(CONFIG_I2C) && defined(CONFIG_SENSORS_BME280)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

////  Previously: I2C Address of BME280
////  #define BME280_ADDR         0x76

#warning Testing: I2C Address of BME280
#define BME280_ADDR         0x77 //// BME280

#define BME280_FREQ         CONFIG_BME280_I2C_FREQUENCY

////  Previously: Device ID of BME280
////  #define DEVID               0x58

#warning Testing: Device ID of BME280
#define DEVID               0x60 //// BME280

#define BME280_DIG_T1_LSB   0x88
#define BME280_DIG_T1_MSB   0x89
#define BME280_DIG_T2_LSB   0x8a
#define BME280_DIG_T2_MSB   0x8b
#define BME280_DIG_T3_LSB   0x8c
#define BME280_DIG_T3_MSB   0x8d
#define BME280_DIG_P1_LSB   0x8e
#define BME280_DIG_P1_MSB   0x8f
#define BME280_DIG_P2_LSB   0x90
#define BME280_DIG_P2_MSB   0x91
#define BME280_DIG_P3_LSB   0x92
#define BME280_DIG_P3_MSB   0x93
#define BME280_DIG_P4_LSB   0x94
#define BME280_DIG_P4_MSB   0x95
#define BME280_DIG_P5_LSB   0x96
#define BME280_DIG_P5_MSB   0x97
#define BME280_DIG_P6_LSB   0x98
#define BME280_DIG_P6_MSB   0x99
#define BME280_DIG_P7_LSB   0x9a
#define BME280_DIG_P7_MSB   0x9b
#define BME280_DIG_P8_LSB   0x9c
#define BME280_DIG_P8_MSB   0x9d
#define BME280_DIG_P9_LSB   0x9e
#define BME280_DIG_P9_MSB   0x9f

#define BME280_DEVID        0xd0
#define BME280_SOFT_RESET   0xe0
#define BME280_STAT         0xf3
#define BME280_CTRL_MEAS    0xf4
#define BME280_CONFIG       0xf5
#define BME280_PRESS_MSB    0xf7
#define BME280_PRESS_LSB    0xf8
#define BME280_PRESS_XLSB   0xf9
#define BME280_TEMP_MSB     0xfa
#define BME280_TEMP_LSB     0xfb
#define BME280_TEMP_XLSB    0xfc

/* Power modes */

#define BME280_SLEEP_MODE   (0x00)
#define BME280_FORCED_MODE  (0x01)
#define BME280_NORMAL_MODE  (0x03)

/* Oversampling for temperature. */

#define BME280_OST_SKIPPED (0x00 << 5)
#define BME280_OST_X1      (0x01 << 5)
#define BME280_OST_X2      (0x02 << 5)
#define BME280_OST_X4      (0x03 << 5)
#define BME280_OST_X8      (0x04 << 5)
#define BME280_OST_X16     (0x05 << 5)

/* Oversampling for pressure. */

#define BME280_OSP_SKIPPED (0x00 << 2)
#define BME280_OSP_X1      (0x01 << 2)
#define BME280_OSP_X2      (0x02 << 2)
#define BME280_OSP_X4      (0x03 << 2)
#define BME280_OSP_X8      (0x04 << 2)
#define BME280_OSP_X16     (0x05 << 2)

/* Predefined oversampling combinations. */

#define BME280_OS_ULTRA_HIGH_RES  (BME280_OSP_X16 | BME280_OST_X2)
#define BME280_OS_STANDARD_RES    (BME280_OSP_X4  | BME280_OST_X1)
#define BME280_OS_ULTRA_LOW_POWER (BME280_OSP_X1  | BME280_OST_X1)

/* Data combined from bytes to int */

#define COMBINE(d) (((int)(d)[0] << 12) | ((int)(d)[1] << 4) | ((int)(d)[2] >> 4))

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Sensor methods */

static int bme280_set_interval(FAR struct sensor_lowerhalf_s *lower,
                               FAR unsigned int *period_us);
static int bme280_activate(FAR struct sensor_lowerhalf_s *lower,
                           bool enable);
static int bme280_fetch(FAR struct sensor_lowerhalf_s *lower,
                        FAR char *buffer, size_t buflen);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct sensor_ops_s g_sensor_ops =
{
  .activate      = bme280_activate,
  .fetch         = bme280_fetch,
  .set_interval  = bme280_set_interval,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: get_sensor_value
 *
 * Description:
 *   Return the sensor value as a float.
 *
 ****************************************************************************/

static float get_sensor_value(const struct sensor_value *val)
{
  DEBUGASSERT(val != NULL);
  return 
    (float)(val->val1) +
    (float)(val->val2) / 1000000.0f;
}

/****************************************************************************
 * Name: bme280_bus_check
 *
 * Description:
 *   Check I2C Bus
 *
 ****************************************************************************/

static int bme280_bus_check(const struct device *dev)
{
  return 0;
}

/****************************************************************************
 * Name: bme280_reg_read
 *
 * Description:
 *   Read from 8-bit BME280 registers
 *
 ****************************************************************************/

static int bme280_reg_read(const struct device *priv,
    uint8_t start, uint8_t *buf, int size)
{
  sninfo("start=0x%02x, size=%d\n", start, size); ////
  struct i2c_msg_s msg[2];
  int ret;

  msg[0].frequency = priv->freq;
  msg[0].addr      = priv->addr;

  //// Previously:
  //// msg[0].flags     = 0;

  #warning Testing: I2C_M_NOSTOP for I2C Sub Address
  msg[0].flags     = I2C_M_NOSTOP;  ////  Testing I2C Sub Address

  msg[0].buffer    = &start;
  msg[0].length    = 1;

  msg[1].frequency = priv->freq;
  msg[1].addr      = priv->addr;
  msg[1].flags     = I2C_M_READ;
  msg[1].buffer    = buf;
  msg[1].length    = size;

  ret = I2C_TRANSFER(priv->i2c, msg, 2);
  if (ret < 0)
    {
      snerr("I2C_TRANSFER failed: %d\n", ret);
      return -1;
    }

  return OK;
}

/****************************************************************************
 * Name: bme280_reg_write
 *
 * Description:
 *   Write to an 8-bit BME280 register
 *
 ****************************************************************************/

static int bme280_reg_write(const struct device *priv, uint8_t reg,
    uint8_t val)
{
  sninfo("reg=0x%02x, val=0x%02x\n", reg, val); ////
  struct i2c_msg_s msg[2];
  uint8_t txbuffer[2];
  int ret;

  txbuffer[0] = reg;
  txbuffer[1] = val;

  msg[0].frequency = priv->freq;
  msg[0].addr      = priv->addr;
  msg[0].flags     = 0;
  msg[0].buffer    = txbuffer;
  msg[0].length    = 2;

  ret = I2C_TRANSFER(priv->i2c, msg, 1);
  if (ret < 0)
    {
      snerr("I2C_TRANSFER failed: %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Name: bme280_set_standby
 *
 * Description:
 *   Set Standby Duration. Zephyr assumes that Standby Duration is static,
 *   so we set it in NuttX.
 *
 ****************************************************************************/

static int bme280_set_standby(FAR struct device *priv, uint8_t value)
{
  sninfo("value=%d\n", value); ////
  
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

/****************************************************************************
 * Name: bme280_set_interval
 ****************************************************************************/

static int bme280_set_interval(FAR struct sensor_lowerhalf_s *lower,
                               FAR unsigned int *period_us)
{
  sninfo("period_us=%u\n", period_us); ////
  FAR struct device *priv = container_of(lower,
                                               FAR struct device,
                                               sensor_lower);
  int ret = 0;

  uint8_t regval;

  switch (*period_us)
    {
      case 500:
        regval = BME280_STANDBY_05_MS;
        break;
      case 62500:
        regval = BME280_STANDBY_63_MS;
        break;
      case 125000:
        regval = BME280_STANDBY_125_MS;
        break;
      case 250000:
        regval = BME280_STANDBY_250_MS;
        break;
      case 500000:
        regval = BME280_STANDBY_500_MS;
        break;
      case 1000000:
        regval = BME280_STANDBY_1000_MS;
        break;
      case 2000000:
        regval = BME280_STANDBY_2000_MS;
        break;
      case 4000000:
        regval = BME280_STANDBY_4000_MS;
        break;
      default:
        ret = -EINVAL;
        break;
    }

  if (ret == 0)
    {
      ret = bme280_set_standby(priv, regval);
    }

  return ret;
}

/****************************************************************************
 * Name: bme280_activate
 ****************************************************************************/

static int bme280_activate(FAR struct sensor_lowerhalf_s *lower,
                           bool enable)
{
  sninfo("TODO enable=%d\n", enable); ////
  int ret = 0;

#ifdef TODO
  FAR struct device *priv = container_of(lower,
                                               FAR struct device,
                                               sensor_lower);
  if (enable)
    {
      /* Set power mode to normal and standard sampling resolution. */

      ret = bme280_putreg8(priv, BME280_CTRL_MEAS, BME280_NORMAL_MODE |
                                 BME280_OS_STANDARD_RES);
    }
  else
    {
      /* Set to sleep mode */

      ret = bme280_putreg8(priv, BME280_CTRL_MEAS, BME280_SLEEP_MODE);
    }

  if (ret >= 0)
    {
      priv->activated = enable;
    }
#endif  //  TODO

  return ret;
}

/****************************************************************************
 * Name: bme280_fetch
 ****************************************************************************/

static int bme280_fetch(FAR struct sensor_lowerhalf_s *lower,
                        FAR char *buffer, size_t buflen)
{
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
  baro_data.pressure = get_sensor_value(&val);

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
  sninfo("temperature=%f °C, pressure=%f mbar, humidity=%f %%\n", baro_data.temperature, baro_data.pressure, humidity); ////

  return buflen;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: bme280_register
 *
 * Description:
 *   Register the BME280 character device
 *
 * Input Parameters:
 *   devno   - Instance number for driver
 *   i2c     - An instance of the I2C interface to use to communicate with
 *             BME280
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int bme280_register(int devno, FAR struct i2c_master_s *i2c)
{
  FAR struct device *priv;
  int ret;

  /* Initialize the BME280 device structure */

  priv = (FAR struct device *)kmm_zalloc(sizeof(struct device));
  if (!priv)
    {
      snerr("Failed to allocate instance\n");
      return -ENOMEM;
    }

  /* Allocate the Compensation Parameters */

  struct bme280_data *data = (FAR struct bme280_data *)kmm_zalloc(sizeof(struct bme280_data));
  if (!data)
    {
      snerr("Failed to allocate data\n");
      kmm_free(priv);
      return -ENOMEM;
    }

  priv->data = data;
  priv->i2c  = i2c;
  priv->addr = BME280_ADDR;
  priv->freq = BME280_FREQ;
  priv->name = "BME280";

  priv->sensor_lower.ops = &g_sensor_ops;
  priv->sensor_lower.type = SENSOR_TYPE_BAROMETER;

  /* Initialize the sensor */

  ret = bme280_chip_init(priv);

  if (ret < 0)
    {
      snerr("Failed to register driver: %d\n", ret);
      kmm_free(data);
      kmm_free(priv);
      return ret;
    }

  /* Register the character driver */

  ret = sensor_register(&priv->sensor_lower, devno);

  if (ret < 0)
    {
      snerr("Failed to register driver: %d\n", ret);
      kmm_free(data);
      kmm_free(priv);
    }

  sninfo("BME280 driver loaded successfully!\n");
  return ret;
}

#endif
