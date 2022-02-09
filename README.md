### Main hardware components

- CYMA Glock 18C (CM.030)
- ESP32 (bare chip microcontroller, WROOM variant)
- 2S BMS 10A (Battery Management System with high Aperage troughput)
- CP2102 (USB-TTL Module for programming the esp32)
- USB C Connector (Female with solder pins, USB 2.0)
- Infrared Interruption Sensor (detects shots by piston recoil)
- 2x IRL8721 (N-Channel Mosfet in TO-220 Package)
- 2x ICR18350 (Li-Ion Battery with max supply of 20 Amps)
- [2S battery charger, 4 Amps out, 5V input](https://a.aliexpress.com/_u6qHNg)
(Main Controllers: CN3302, AO4468)
- [1x 74AHCT125 (logic shifter to control neopixel led's)](https://assets.nexperia.com/documents/data-sheet/74AHC_AHCT125.pdf)

Deprecated:

- SX1308 (DC-DC Step Up Converter)
- TP5100 (2S battery charger Mmodule, 8.4V)
