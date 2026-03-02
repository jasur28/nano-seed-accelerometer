# NanoSEED Accelerometer Architecture

## Goal
Low-cost real-time 3-axis accelerometer system with NanoSEED-like format.

## Hardware
- Arduino Nano (ATmega328P)
- MPU6050
- Temperature sensor (DS18B20)
- USB-TTL streaming

## Firmware
- 1 kHz sampling
- Ring buffer
- 512-byte NanoSEED block
- CRC32
- Binary streaming

## Desktop GUI
- Qt 6 + QML
- Real-time plotting
- CSV export
- Binary block parsing
