# Intro
This is an attempt to replace the internals of an airsoft toy pistol with superior, smart ones. The mainboard will be a long and thin pcb accomodating the important components, attached to the inner left side of the slide. A daughterboard will provide pads for external connections. both boards will be connected by probably dual row, 4 column header pins, to provide high current flow as well as D+ & D-. The shoot mode selector will get a mini magnet glued to its leg and the pcb 2 hall effect sensors to sense where mode by position. Another hall sensor will be in the middle of the pcb, to sense the position of the trigger, that will also have a magnet glued on an extension of its front.

## Connections (main- & daughter board)
- Ucb Vcc
- Usb D+
- Usb D-
- Capacitive touch (trigger)
- Batt + (high current)
- Batt mid
- Batt - && Usb Gnd combined (high current)

## Main hardware components

- CYMA Glock 18C (CM.030)
- 1x ESP32-DOWD: microcontroller
- 2x ICR18350: Li-Ion Batt with 20A drain
- 1x 2S BMS: Balancer for 2 cells
- 1x CH340c: Circuit for programming the esp32
- 1x USB C PD pcb: 5V pd board with USB 2.0
- 1x QRE1113: detects piston recoil by reflection
- 2x KND3203: High current N-Channel Mosfet
- 1x 2S charger, 4A out, 5V in: Uses CN3302, AO4468 to charge batteries
- 1x 74AHCT125: logic shifter to control neopixels
- 2x MIC5219-3.3: to power any 3.3v consumer by battery
- 3x hall effect sensor

## Dimensions
- Between slide inner wall and barrel: Max 6mm height

![image](https://github.com/DoganM95/IoT-Softair/assets/38842553/2528bff4-0c19-4a09-a9c6-f10ae86ed3b1)
