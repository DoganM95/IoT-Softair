## Intro
This is an attempt to replace the internals of an airsoft toy pistol with superior, smart ones. The mainboard will be a long and thin pcb accomodating the important components, attached to the inner left side of the slide. A daughterboard will provide pads for external connections. both boards will be connected by e.g. a flex cable, a usb c port or whatever suits best, to provide high current flow as well as data communication.

### Main hardware components

- CYMA Glock 18C (CM.030)
- [1x ESP32-DOWD](espressif.com/sites/default/files/documentation/esp32-wroom-32d_esp32-wroom-32u_datasheet_en.pdf) (microcontroller, WROOM variant)
- [2x ICR18350](aliexpress.com/item/4000270507847) (Li-Ion Battery with max supply of 20 Amps)
- [1x 2S BMS 10A](aliexpress.com/item/1005001653969293.html) (Battery Management System with high Aperage troughput)
- [1x CP2102](aliexpress.com/item/1005003238623602.html) (USB-TTL Module for programming the esp32)
- [USB C Connector](aliexpress.com/item/1005001446500637.html) (Female with solder pins, USB 2.0)
- [Infrared Interruption Sensor](aliexpress.com/item/1005001960582682.html) (detects shots by piston recoil)
- [2x IRL8721](cdn-shop.adafruit.com/datasheets/irlb8721pbf.pdf) (N-Channel Mosfet in TO-220 Package)
- [2S battery charger, 4 Amps out, 5V input](https://a.aliexpress.com/_u6qHNg) (Main Controllers: CN3302, AO4468)
- [1x 74AHCT125 (logic shifter to control neopixel led's)](assets.nexperia.com/documents/data-sheet/74AHC_AHCT125.pdf)
- [2x AMS1117-3.3](advanced-monolithic.com/pdf/ds1117.pdf) (Voltage regulator, 3.3V)
- [2x AMS1117-5.0](advanced-monolithic.com/pdf/ds1117.pdf) (Voltage regulator, 5V)

Deprecated:

- SX1308 (DC-DC Step Up Converter)
- TP5100 (2S battery charger Mmodule, 8.4V)
