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

static uint8_t bme280_getreg8(FAR struct device *priv,
                              uint8_t regaddr);
static int bme280_putreg8(FAR struct device *priv, uint8_t regaddr,
                          uint8_t regval);

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
 * Name: bme280_getreg8
 *
 * Description:
 *   Read from an 8-bit BME280 register
 *
 ****************************************************************************/

static uint8_t bme280_getreg8(FAR struct device *priv, uint8_t regaddr)
{
  sninfo("regaddr=0x%02x\n", regaddr); ////
  struct i2c_msg_s msg[2];
  uint8_t regval = 0;
  int ret;

  msg[0].frequency = priv->freq;
  msg[0].addr      = priv->addr;

  //// Previously:
  //// msg[0].flags     = 0;

  #warning Testing: I2C_M_NOSTOP for I2C Sub Address
  msg[0].flags     = I2C_M_NOSTOP;  ////  Testing I2C Sub Address

  msg[0].buffer    = &regaddr;
  msg[0].length    = 1;

  msg[1].frequency = priv->freq;
  msg[1].addr      = priv->addr;
  msg[1].flags     = I2C_M_READ;
  msg[1].buffer    = &regval;
  msg[1].length    = 1;

  ret = I2C_TRANSFER(priv->i2c, msg, 2);
  if (ret < 0)
    {
      snerr("I2C_TRANSFER failed: %d\n", ret);
      return 0;
    }

  sninfo("regaddr=0x%02x, regval=0x%02x\n", regaddr, regval); ////
  return regval;
}

/****************************************************************************
 * Name: bme280_getregs
 *
 * Description:
 *   Read two 8-bit from a BME280 register
 *
 ****************************************************************************/

static int bme280_getregs(FAR struct device *priv, uint8_t regaddr,
                          uint8_t *rxbuffer, uint8_t length)
{
  sninfo("regaddr=0x%02x, length=%d\n", regaddr, length); ////
  struct i2c_msg_s msg[2];
  int ret;

  msg[0].frequency = priv->freq;
  msg[0].addr      = priv->addr;

  //// Previously:
  //// msg[0].flags     = 0;

  #warning Testing: I2C_M_NOSTOP for I2C Sub Address
  msg[0].flags     = I2C_M_NOSTOP;  ////  Testing I2C Sub Address

  msg[0].buffer    = &regaddr;
  msg[0].length    = 1;

  msg[1].frequency = priv->freq;
  msg[1].addr      = priv->addr;
  msg[1].flags     = I2C_M_READ;
  msg[1].buffer    = rxbuffer;
  msg[1].length    = length;

  ret = I2C_TRANSFER(priv->i2c, msg, 2);
  if (ret < 0)
    {
      snerr("I2C_TRANSFER failed: %d\n", ret);
      return -1;
    }

  return OK;
}

/****************************************************************************
 * Name: bme280_putreg8
 *
 * Description:
 *   Write to an 8-bit BME280 register
 *
 ****************************************************************************/

static int bme280_putreg8(FAR struct device *priv, uint8_t regaddr,
                          uint8_t regval)
{
  sninfo("regaddr=0x%02x, regval=0x%02x\n", regaddr, regval); ////
  struct i2c_msg_s msg[2];
  uint8_t txbuffer[2];
  int ret;

  txbuffer[0] = regaddr;
  txbuffer[1] = regval;

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
 * Name: bme280_checkid
 *
 * Description:
 *   Read and verify the BME280 chip ID
 *
 ****************************************************************************/

static int bme280_checkid(FAR struct device *priv)
{
  uint8_t devid = 0;

  /* Read device ID */

  devid = bme280_getreg8(priv, BME280_DEVID);
  up_mdelay(1);
  sninfo("devid: 0x%02x\n", devid);

  if (devid != (uint16_t) DEVID)
    {
      /* ID is not Correct */

      snerr("Wrong Device ID! %02x\n", devid);
      return -ENODEV;
    }

  return OK;
}

/****************************************************************************
 * Name: bme280_set_standby
 *
 * Description:
 *   set standby duration
 *
 ****************************************************************************/

static int bme280_set_standby(FAR struct device *priv, uint8_t value)
{
  uint8_t v_data_u8;
  uint8_t v_sb_u8;

  /* Set the standby duration value */

  v_data_u8 = bme280_getreg8(priv, BME280_CONFIG);
  v_data_u8 = (v_data_u8 & ~(0x07 << 5)) | (value << 5);
  bme280_putreg8(priv, BME280_CONFIG, v_data_u8);

  /* Check the standby duration value */

  v_data_u8 = bme280_getreg8(priv, BME280_CONFIG);
  v_sb_u8 = (v_data_u8 >> 5) & 0x07;

  if (v_sb_u8 != value)
    {
      snerr("Failed to set value for standby time.");
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: bme280_initialize
 *
 * Description:
 *   Initialize BME280 device
 *
 ****************************************************************************/

static int bme280_initialize(FAR struct device *priv)
{
  uint8_t buf[24];
  int ret;

  /* Get calibration data. */

  ret = bme280_getregs(priv, BME280_DIG_T1_LSB, buf, 24);
  if (ret < 0)
    {
      return ret;
    }

#ifdef TODO
  priv->calib.t1 = (uint16_t)buf[1]  << 8 | buf[0];
  priv->calib.t2 = (int16_t) buf[3]  << 8 | buf[2];
  priv->calib.t3 = (int16_t) buf[5]  << 8 | buf[4];

  priv->calib.p1 = (uint16_t)buf[7]  << 8 | buf[6];
  priv->calib.p2 = (int16_t) buf[9]  << 8 | buf[8];
  priv->calib.p3 = (int16_t) buf[11] << 8 | buf[10];
  priv->calib.p4 = (int16_t) buf[13] << 8 | buf[12];
  priv->calib.p5 = (int16_t) buf[15] << 8 | buf[14];
  priv->calib.p6 = (int16_t) buf[17] << 8 | buf[16];
  priv->calib.p7 = (int16_t) buf[19] << 8 | buf[18];
  priv->calib.p8 = (int16_t) buf[21] << 8 | buf[20];
  priv->calib.p9 = (int16_t) buf[23] << 8 | buf[22];

  sninfo("T1 = %u\n", priv->calib.t1);
  sninfo("T2 = %d\n", priv->calib.t2);
  sninfo("T3 = %d\n", priv->calib.t3);

  sninfo("P1 = %u\n", priv->calib.p1);
  sninfo("P2 = %d\n", priv->calib.p2);
  sninfo("P3 = %d\n", priv->calib.p3);
  sninfo("P4 = %d\n", priv->calib.p4);
  sninfo("P5 = %d\n", priv->calib.p5);
  sninfo("P6 = %d\n", priv->calib.p6);
  sninfo("P7 = %d\n", priv->calib.p7);
  sninfo("P8 = %d\n", priv->calib.p8);
  sninfo("P9 = %d\n", priv->calib.p9);
#endif  //  TODO

  /* Set power mode to sleep */

  bme280_putreg8(priv, BME280_CTRL_MEAS, BME280_SLEEP_MODE);

  /* Set stand-by time to 0.5 ms, no IIR filter */

  ret = bme280_set_standby(priv, BME280_STANDBY_05_MS);
  if (ret != OK)
    {
      snerr("Failed to set value for standby time.\n");
      return -1;
    }

  return ret;
}

/****************************************************************************
 * Name: bme280_set_interval
 ****************************************************************************/

static int bme280_set_interval(FAR struct sensor_lowerhalf_s *lower,
                               FAR unsigned int *period_us)
{
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
  FAR struct device *priv = container_of(lower,
                                               FAR struct device,
                                               sensor_lower);
  int ret;

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

  uint8_t buf[6];
  uint32_t press;
  int32_t temp;
  int ret;
  struct timespec ts;
  struct sensor_event_baro baro_data;

  if (buflen != sizeof(baro_data))
    {
      return -EINVAL;
    }

  if (!priv->activated)
    {
      /* Sensor is asleep, go to force mode to read once */

      ret = bme280_putreg8(priv, BME280_CTRL_MEAS, BME280_FORCED_MODE |
                                 BME280_OS_ULTRA_LOW_POWER);

      if (ret < 0)
        {
          return ret;
        }

      /* Wait time according to ultra low power mode set during sleep */

      up_mdelay(6);
    }

  /* Read pressure & data */

  ret = bme280_getregs(priv, BME280_PRESS_MSB, buf, 6);

  if (ret < 0)
    {
      return ret;
    }

  press = (uint32_t)COMBINE(buf);
  temp = COMBINE(&buf[3]);

  sninfo("press = %"PRIu32", temp = %"PRIi32"\n", press, temp);

  temp = bme280_compensate_temp(priv, temp);
  press = bme280_compensate_press(priv, press);

#ifdef CONFIG_CLOCK_MONOTONIC
  clock_gettime(CLOCK_MONOTONIC, &ts);
#else
  clock_gettime(CLOCK_REALTIME, &ts);
#endif

  baro_data.timestamp = 1000000ull * ts.tv_sec + ts.tv_nsec / 1000;
  baro_data.pressure = press / 100.0f;
  baro_data.temperature = temp / 100.0f;

  memcpy(buffer, &baro_data, sizeof(baro_data));

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

  #warning TODO: Init char *name;                   /* Name of the device */
  #warning TODO: Init struct bme280_data *data;     /* Compensation parameters */

  priv->i2c = i2c;
  priv->addr = BME280_ADDR;
  priv->freq = BME280_FREQ;

  priv->sensor_lower.ops = &g_sensor_ops;
  priv->sensor_lower.type = SENSOR_TYPE_BAROMETER;

  /* Check Device ID */

  ret = bme280_checkid(priv);

  if (ret < 0)
    {
      snerr("Failed to register driver: %d\n", ret);
      kmm_free(priv);
      return ret;
    }

  ret = bme280_initialize(priv);

  if (ret < 0)
    {
      snerr("Failed to initialize physical device bme280:%d\n", ret);
      kmm_free(priv);
      return ret;
    }

  /* Register the character driver */

  ret = sensor_register(&priv->sensor_lower, devno);

  if (ret < 0)
    {
      snerr("Failed to register driver: %d\n", ret);
      kmm_free(priv);
    }

  sninfo("BME280 driver loaded successfully!\n");
  return ret;
}

#endif
