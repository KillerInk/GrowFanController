#pragma once

#define SENSOR_ENS160AHT21
#define SENSOR_BME280
//#define USE_SDCARD

#define I2C_SDA 21 //default i2c sda
#define I2C_SCL 22 //default i2c scl
#define i2c_pwn_addr 95 //95 controls fans in and out
#define i2c_aht_addr 56
#define i2c_ens_add 83
#define i2c_light_addr 94
#define BME_I2C_ADD 119 // 118 bme280 can only have these two
// without summertime
#define time_zone_hour_utc_offset 1 


#define http_port 80