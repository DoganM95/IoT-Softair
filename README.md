# Intro

This is an attempt to replace the internals of an airsoft toy pistol with superior, smart ones. 

## Components

### Mainboard

The mainboard will be a long and thin pcb accomodating the important components, attached to the inner left side of the slide. 

### Daughterboard

A daughterboard will provide pads for external connections. Both board will connect via 24 pin usb c ports. 

### Mode selector

The shoot mode selector will (semi- & full-auto) get a small sense button that gets pushed in one mode, if the other mode has no space for such a button, a tiny magnet and hall sensor could help.

### Charge Port

A charging port with usb C (at least 12 pin with D+ and D-) will be added to the bottom to a charge pcb, to trigger 5V by pulling down the CC lines, making it compatible with serial programmers while charging.

## Connections (main- & daughter board)
- 2 pins: Usb Vcc
- 1 pin:  Usb D+
- 1 pin:  Usb D-
- 1 pin:  Capacitive touch (trigger)
- 4 pins: Batt + (high current)
- 1 pin:  Batt mid
- 4 pins: Batt - && Usb Gnd combined (high current)

## BOM (concept)

- CYMA Glock 18C (CM.030)
- 1x ESP32: microcontroller
- 2x 18350: Li-Ion Batteries with 20A drain
- 1x USB C PD pcb: 5V pd board with USB 2.0
- 1x QRE1113: detects piston recoil by reflection
- 2x KND3203: High current N-Channel Mosfet
- 1x 74AHCT125: logic shifter to control neopixels
- 2x MIC5219-3.3: to power any 3.3v consumer by battery
- 3x hall effect sensor
- 1x [CH340C-Pcb](https://github.com/DoganM95/CH340C-Pcb) which is a circuit for auto programming && resettiung the esp32
- 1x [2S-Charge-Balance-Pcb](https://github.com/DoganM95/2S-Charge-Balance-Pcb) that includes
  - 1x 2S BMS: Balancer for 2 cells
  - 1x 2S charger, 4A out, 5V in: Uses CN3302, AO4468 to charge batteries

## Dimensions
- Between slide inner wall and barrel: Max 6mm height

![image](https://github.com/DoganM95/IoT-Softair/assets/38842553/2528bff4-0c19-4a09-a9c6-f10ae86ed3b1)
