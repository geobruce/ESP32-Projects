# WiFi blink stability test

This program is meant to test how stable the ESP32 can be.
You can controll a blinking LED via a webpage.
In case somethings goes wrong a watchdog timer will restart / reset the ESP32.
The last state of the output is stored in the EEPROM of the ESP32, so when the ESP32 restarts the output will be loaded again and everything will be as before.

If you have feedback or any stability improvements on this code, please let me know.