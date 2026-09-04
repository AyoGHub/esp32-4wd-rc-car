# ESP32 Bluetooth 4WD RC Car

A Bluetooth-controlled RC car implementing the ESP32 Board

## Features

- Bidirectional drive control for 3 DC motors; steering control for a 4th motor
- L293D H-bridge IC for motor control
- Toggleable rainbow RGB headlight
- Musical buzzer with 3 programmed songs; a controller button cycles between the melodies
- Nintendo Joy-Con provides wireless Bluetooth control
- Rechargeable 9 V battery supply

## Hardware

- ESP32
- Nintendo Joy-Con
- L293D motor-driver IC
- 4 DC motors
- RGB LED
- Passive buzzer
- Breadboard
- Jumper wires
  
## Wiring and Power

- The ESP32 is safely powered via a USB-C cable from a smart device (stable 5 V), while the high-voltage motor system is powered externally using a 9 V battery. The ESP32 handles the Joy-Con's inputs and sends signals to the L293D via jumper wires (see [`20260904_140321.jpg`](20260904_140321.jpg))
- The L293D controls 3 driving DC motors on one side and a steering DC motor on the other
- The LED and buzzer are connected to ESP32 GPIO pins via a breadboard and jumper wires

## Controls (For both L/R Joy-Cons)

- Held horizontally, Nintendo Joy-Con SL and SR buttons map to forward and reverse drive, respectively
- The L or ZL/R or ZR buttons toggle the LED on/off
- The Y/"Up" buttons (when held on its side, north-facing) select the track
- The L3/R3 thumbstick buttons toggle off the current song
- The "Up"/"Down" analogue joystick inputs control the steering

## Code (Arduino IDE)

- "car.ino" is the C++ program running on the ESP32, available in [`car.ino`](car.ino)
- The program reads wireless Joy-Con inputs and maps them to the motor-driver, headlight, and buzzer commands
- It uses ESP32 GPIO outputs to control the L293D, RGB LED, buzzer, and motors

## Libraries

- Bluepad32 — used to connect the ESP32 to the Nintendo Joy-Con and read wireless controller inputs

## Photos/Demo



## Experience Gained





