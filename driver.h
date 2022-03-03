/****************************************************************************
 * drivers/sensors/bme280/driver.h
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

#ifndef __INCLUDE_NUTTX_SENSORS_BME280_H
#define __INCLUDE_NUTTX_SENSORS_BME280_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#if defined(CONFIG_I2C) && (defined(CONFIG_SENSORS_BME280) || defined(CONFIG_SENSORS_BME280_SCU))

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* Prerequisites:
 *
 * CONFIG_BME280
 *   Enables support for the BME280 driver
 */

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct i2c_master_s;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/* IOCTL Commands ***********************************************************/

/* Standby duration */

#define BME280_STANDBY_05_MS   (0x00) /* 0.5 ms */
#define BME280_STANDBY_63_MS   (0x01) /* 62.5 ms */
#define BME280_STANDBY_125_MS  (0x02) /* 125 ms */
#define BME280_STANDBY_250_MS  (0x03) /* 250 ms */
#define BME280_STANDBY_500_MS  (0x04) /* 500 ms */
#define BME280_STANDBY_1000_MS (0x05) /* 1000 ms */
#define BME280_STANDBY_2000_MS (0x06) /* 2000 ms */
#define BME280_STANDBY_4000_MS (0x07) /* 4000 ms */

#ifdef CONFIG_SENSORS_BME280_SCU
/****************************************************************************
 * Name: bme280_init
 *
 * Description:
 *   Initialize BME280 pressure device
 *
 * Input Parameters:
 *   i2c     - An instance of the I2C interface to use to communicate with
 *             BME280
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int bme280_init(FAR struct i2c_master_s *i2c, int port);
#endif

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

#ifdef CONFIG_SENSORS_BME280_SCU
int bme280press_register(FAR const char *devpath, int minor,
                         FAR struct i2c_master_s *i2c, int port);
int bme280temp_register(FAR const char *devpath, int minor,
                        FAR struct i2c_master_s *i2c, int port);
#else
int bme280_register(int devno, FAR struct i2c_master_s *i2c);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_I2C && (CONFIG_SENSORS_BME280 || CONFIG_SENSORS_BME280_SCU) */
#endif /* __INCLUDE_NUTTX_SENSORS_BME280_H */
