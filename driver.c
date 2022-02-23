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
#include <nuttx/sensors/bme280.h>
#include <nuttx/sensors/sensor.h>

#if defined(CONFIG_I2C) && defined(CONFIG_SENSORS_BME280)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BME280_ADDR         0x77
#define BME280_FREQ         CONFIG_BME280_I2C_FREQUENCY

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Sensor methods */

static int bme280_set_interval_baro(FAR struct sensor_lowerhalf_s *lower,
                               FAR unsigned int *period_us);
static int bme280_set_interval_humi(FAR struct sensor_lowerhalf_s *lower,
                               FAR unsigned int *period_us);
static int bme280_activate_baro(FAR struct sensor_lowerhalf_s *lower,
                           bool enable);
static int bme280_activate_humi(FAR struct sensor_lowerhalf_s *lower,
                           bool enable);
static int bme280_fetch_baro(FAR struct sensor_lowerhalf_s *lower,
                        FAR char *buffer, size_t buflen);
static int bme280_fetch_humi(FAR struct sensor_lowerhalf_s *lower,
                        FAR char *buffer, size_t buflen);

/****************************************************************************
 * Private Data
 ****************************************************************************/

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

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: get_sensor_value
 *
 * Description:
 *   Return the sensor value as a float
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
 * Name: pm_device_state_get
 *
 * Description:
 *   Get the device state (active / suspended)
 *
 ****************************************************************************/

static int pm_device_state_get(const struct device *priv,
  enum pm_device_state *state)
{
  DEBUGASSERT(priv != NULL);
  if (priv->activated)
    {
      *state = PM_DEVICE_STATE_ACTIVE;
    }
  else
    {
      *state = PM_DEVICE_STATE_SUSPENDED;
    }
  return 0;  
}

/****************************************************************************
 * Name: bme280_bus_check
 *
 * Description:
 *   Check I2C Bus
 *
 ****************************************************************************/

static int bme280_bus_check(const struct device *priv)
{
  DEBUGASSERT(priv != NULL);
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
  DEBUGASSERT(priv != NULL);
  sninfo("start=0x%02x, size=%d\n", start, size);
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
  DEBUGASSERT(priv != NULL);
  sninfo("reg=0x%02x, val=0x%02x\n", reg, val);
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

/****************************************************************************
 * Name: bme280_set_interval_baro
 *
 * Description:
 *   Set Standby Interval for Barometer Sensor
 *
 ****************************************************************************/

static int bme280_set_interval_baro(FAR struct sensor_lowerhalf_s *lower,
                               FAR unsigned int *period_us)
{
  sninfo("period_us=%u\n", period_us);
  FAR struct device *priv = container_of(lower,
                                               FAR struct device,
                                               sensor_baro);
  sninfo("priv=%x, sensor_baro=%x\n", priv, lower); ////
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
 * Name: bme280_set_interval_humi
 *
 * Description:
 *   Set Standby Interval for Humidity Sensor
 *
 ****************************************************************************/

static int bme280_set_interval_humi(FAR struct sensor_lowerhalf_s *lower,
                               FAR unsigned int *period_us)
{
  sninfo("period_us=%u\n", period_us);
  FAR struct device *priv = container_of(lower,
                                               FAR struct device,
                                               sensor_humi);
  sninfo("priv=%x, sensor_humi=%x\n", priv, lower); ////
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
 * Name: bme280_activate_baro
 *
 * Description:
 *   Set Power Mode for Barometer Sensor. If enable is true, set Power Mode 
 *   to normal. Else set to sleep mode.
 *
 ****************************************************************************/

static int bme280_activate_baro(FAR struct sensor_lowerhalf_s *lower,
                           bool enable)
{
  sninfo("enable=%d\n", enable);
  int ret = 0;

  FAR struct device *priv = container_of(lower,
                                               FAR struct device,
                                               sensor_baro);
  sninfo("priv=%x, sensor_baro=%x\n", priv, lower); ////
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

/****************************************************************************
 * Name: bme280_activate_humi
 *
 * Description:
 *   Set Power Mode for Humidity Sensor. If enable is true, set Power Mode 
 *   to normal. Else set to sleep mode.
 *
 ****************************************************************************/

static int bme280_activate_humi(FAR struct sensor_lowerhalf_s *lower,
                           bool enable)
{
  sninfo("enable=%d\n", enable);
  int ret = 0;

  FAR struct device *priv = container_of(lower,
                                               FAR struct device,
                                               sensor_humi);
  sninfo("priv=%x, sensor_humi=%x\n", priv, lower); ////
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

/****************************************************************************
 * Name: bme280_fetch
 *
 * Description:
 *   Fetch pressure, temperature and humidity from sensor
 *
 ****************************************************************************/

static int bme280_fetch(FAR struct device *priv,
                        FAR struct sensor_event_baro *baro_data,
                        FAR struct sensor_event_humi *humi_data)
{
  DEBUGASSERT(priv != NULL);
  DEBUGASSERT(baro_data != NULL || humi_data != NULL);
  int ret;
  struct timespec ts;
  struct sensor_value val;

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

  /* Get the pressure (from Zephyr BME280 Driver) */

  ret = bme280_channel_get(priv, SENSOR_CHAN_PRESS, &val);
  if (ret < 0)
    {
      return ret;
    }
  float pressure = get_sensor_value(&val) * 10;

  /* Get the temperature (from Zephyr BME280 Driver) */

  ret = bme280_channel_get(priv, SENSOR_CHAN_AMBIENT_TEMP, &val);
  if (ret < 0)
    {
      return ret;
    }
  float temperature = get_sensor_value(&val);

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
  uint64_t timestamp = 1000000ull * ts.tv_sec + ts.tv_nsec / 1000;

  /* Return the pressure and temperature data */

  if (baro_data != NULL)
    {
      baro_data->pressure    = pressure;
      baro_data->temperature = temperature;
      baro_data->timestamp   = timestamp;
    }

  /* Return the humidity data */

  if (humi_data != NULL)
    {
      humi_data->humidity    = humidity;
      humi_data->timestamp   = timestamp;
    }

  sninfo("temperature=%f °C, pressure=%f mbar, humidity=%f %%\n", temperature, pressure, humidity);
  return 0;
}

/****************************************************************************
 * Name: bme280_fetch_baro
 *
 * Description:
 *   Called by NuttX to fetch pressure and temperature from sensor
 *
 ****************************************************************************/

static int bme280_fetch_baro(FAR struct sensor_lowerhalf_s *lower,
                        FAR char *buffer, size_t buflen)
{
  sninfo("buflen=%d\n", buflen);
  int ret;
  struct sensor_event_baro baro_data;

  /* Get device struct */

  FAR struct device *priv = container_of(lower,
                                               FAR struct device,
                                               sensor_baro);
  sninfo("priv=%x, sensor_baro=%x\n", priv, lower); ////

  /* Validate buffer size */

  if (buflen != sizeof(baro_data))
    {
      return -EINVAL;
    }

  /* Fetch the sensor data */

  ret = bme280_fetch(priv, &baro_data, NULL);
  if (ret < 0)
    {
      return ret;
    }

  /* Return the sensor data */

  memcpy(buffer, &baro_data, sizeof(baro_data));
  return buflen;
}

/****************************************************************************
 * Name: bme280_fetch_humi
 *
 * Description:
 *   Called by NuttX to fetch humidity from sensor
 *
 ****************************************************************************/

static int bme280_fetch_humi(FAR struct sensor_lowerhalf_s *lower,
                        FAR char *buffer, size_t buflen)
{
  sninfo("buflen=%d\n", buflen);
  int ret;
  struct sensor_event_humi humi_data;

  /* Get device struct */

  FAR struct device *priv = container_of(lower,
                                               FAR struct device,
                                               sensor_humi);
  sninfo("priv=%x, sensor_humi=%x\n", priv, lower); ////

  /* Validate buffer size */

  if (buflen != sizeof(humi_data))
    {
      return -EINVAL;
    }

  /* Fetch the sensor data */

  ret = bme280_fetch(priv, NULL, &humi_data);
  if (ret < 0)
    {
      return ret;
    }

  /* Return the sensor data */

  memcpy(buffer, &humi_data, sizeof(humi_data));
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
  sninfo("devno=%d\n", devno);
  FAR struct device *priv;
  int ret;

  /* Initialize the device structure */

  priv = (FAR struct device *)kmm_zalloc(sizeof(struct device));
  if (!priv)
    {
      snerr("Failed to allocate instance\n");
      return -ENOMEM;
    }
  sninfo("priv=%x, sensor_baro=%x, sensor_humi=%x\n", priv, &(priv->sensor_baro), &(priv->sensor_humi)); ////

  /* Allocate the Compensation Parameters */

  struct bme280_data *data = (FAR struct bme280_data *)kmm_zalloc(sizeof(struct bme280_data));
  if (!data)
    {
      snerr("Failed to allocate data\n");
      kmm_free(priv);
      return -ENOMEM;
    }

  priv->i2c  = i2c;
  priv->addr = BME280_ADDR;
  priv->freq = BME280_FREQ;
  priv->name = "BME280";
  priv->data = data;
  priv->activated = true;

  /* Initialize the Barometer Sensor */

  priv->sensor_baro.ops = &g_baro_ops;
  priv->sensor_baro.type = SENSOR_TYPE_BAROMETER;

  /* Initialize the Humidity Sensor */

  priv->sensor_humi.ops = &g_humi_ops;
  priv->sensor_humi.type = SENSOR_TYPE_RELATIVE_HUMIDITY;

  /* Initialize the Sensor Hardware */

  ret = bme280_chip_init(priv);
  if (ret < 0)
    {
      snerr("Failed to init: %d\n", ret);
      kmm_free(data);
      kmm_free(priv);
      return ret;
    }

  /* Set power mode to sleep */

  ret = bme280_pm_action(priv, PM_DEVICE_ACTION_SUSPEND);
  if (ret < 0)
    {
      snerr("Failed to sleep: %d\n", ret);
      kmm_free(data);
      kmm_free(priv);
      return ret;
    }
  priv->activated = false;

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

  sninfo("BME280 driver loaded successfully!\n");
  return ret;
}

#endif
