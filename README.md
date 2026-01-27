# CameraBoard – UTS Rocketry Avionics
High-reliability camera control and downlink board built around the **STM32F405RGT6**, designed for UTS Rocketry’s flight avionics stack.  
The board provides **camera power switching**, **camera UART control**, **video downlink support**, and **CAN-based communication** with the central avionics system.

---

## Project Overview
The CameraBoard is a custom PCB designed to interface action cameras with the UTS Rocketry avionics system.  
It provides:

- **Reliable power delivery** to the camera through a load-switch architecture  
- **UART-based camera control** (start/stop recording, mode switching, etc.)  
- **Video downlink support** (placeholder until RF/video module is finalised)  
- **CAN Bus** connectivity for inter-module communication  
- **Debug UART** and activity LEDs  
- **5V → 3.3V power tree** and full onboard decoupling network  

Built around the **STM32F405RGT6**, chosen for performance, CAN support, and strong STM32 ecosystem compatibility.

---

## Key Features
- **STM32F405RGT6 MCU**
- **Camera Power Switching**
  - Load switch controlled via GPIO  
  - Local decoupling + bulk capacitance
- **Camera Control (UART)**
  - Compatible with RunCam-style UART protocols
- **CAN Bus Interface**
  - TX/RX routed to CAN transceiver  
  - Interfaces with the main flight computer
- **Video Downlink Support**
  - Reserved connector footprint  
  - Pins allocated for future video transmitter module
- **Debug Interface**
  - UART for logging  
  - Status, power, and activity LEDs
- **Power Topology**
  - Headers for 5V buck converters  
  - 3.3V rail for MCU and logic  
  - Full decoupling network following ST guidelines

## Current Project Status
Schematic Finalized

Need to finish routing for pcb
<img width="777" height="653" alt="Screenshot from 2026-01-27 12-23-50" src="https://github.com/user-attachments/assets/33f15858-5c52-4196-ade5-3a43e443f8d7" />

## Contributors
Reed Basangan — Software Lead, UTS Rocketry
UTS Rocketry Avionics Team
