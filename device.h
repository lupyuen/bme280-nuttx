/****************************************************************************
 * drivers/sensors/bme280/device.h
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

#ifndef __INCLUDE_NUTTX_SENSORS_BME280_DEVICE_H
#define __INCLUDE_NUTTX_SENSORS_BME280_DEVICE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#if defined(CONFIG_I2C) && (defined(CONFIG_SENSORS_BME280) || defined(CONFIG_SENSORS_BME280_SCU))

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct device
{
  FAR struct sensor_lowerhalf_s sensor_lower;
  FAR struct i2c_master_s *i2c; /* I2C interface */
  uint8_t addr;                 /* BME280 I2C address */
  int freq;                     /* BME280 Frequency <= 3.4MHz */
  bool activated;               /* True if device is not in sleep mode */

  char *name;                   /* Name of the device */
  struct bme280_data *data;     /* Compensation parameters (bme280.c) */
};

#endif /* CONFIG_I2C && (CONFIG_SENSORS_BME280 || CONFIG_SENSORS_BME280_SCU) */
#endif /* __INCLUDE_NUTTX_SENSORS_BME280_DEVICE_H */
