# Esp Grow Controller

![](/connect.jpg)

## Using an Esp32 to control various different devices for a GrowTent.
### Hardware used:

1. Esp32 1x
2. Gravity: 2-Channel I2C DAC Module (0-10V) 2x
3. SD CARD Reader 
4. ENS160ATH21 Sensor
5. 12v stepdown to 5v

### Features:
1. Can control 2 different Fans(in/out) based on Temperature and Humidity and apply a speed difference to keep a negative pressure
2. Set a nightmode to reduce the fan speed for that time range
3. Automatic turn light on/off and apply sunrise/set
4. Log humidity, temperature, co2, .... to sdcard, it create files in that structure /year/month/day/hour.csv (/2024/03/05/12.csv) and store a value every sec
5. webinterface with a chart

![](/web.jpg)

### Notes about DAC Module
Every devices that use the same Power Source as the Esp is connected to,
only need the PWM signal and no GND.
Extern powered devices like the lamp(12v) or the big fan(24v) need a connection to GND on the DAC!
