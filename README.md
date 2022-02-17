# Apache NuttX Driver for Bosch BME280 I2C Sensor (Temperature + Humidity + Air Pressure) ported from Zephyr OS

Will Apache NuttX OS talk I2C with Bosch BME280 Sensor? (Temperature + Humidity + Air Pressure) ... Let's find out!

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
