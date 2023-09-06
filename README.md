# Bespoke Wearable IMU Device
This wearable system aims to be cost-effective, lightweight and compact size.
This project is separated in two sections done in parallel, prototyping and PCB Design.

For communication, we have used SPI protocol in MCU & IMU communication, QSPI protocol in Memory & MCU communication, 
USB transmission in Board & PC communication.

STML412CBT6 communicates with ICM-20948 with SPI, communicates with the Memory module with
QSPI, USB interface designed on board is responsible for transmit data to the PC and
present on the API.

Due to time constraints, the PCB file has not been updated to match the version of the schematics file.
The PCB board has been tested to have the low power characteristics, with 0.121W. Using YOJOCK USB C Tester Power Meter.

## Prototype
- NUCLEO-L412RB-P (STM32 Nucleo-64 development board with STM32L412RB MCU, SMPS, supports Arduino, ST Morpho connectivity)
- SparkFun 9DoF IMU (ICM-20948) Breakout Board
- MT25QL512ABB-based self-made Flash Memory Breakout Board
- Adafruit USB Micro-B Breakout Board
![image](https://github.com/mujiexu2/ELEC0054_Dissertation_XuMujie/blob/main/images/powertest1.jpg)

## PCB Design
Component List:
- InvenSense ICM20948 (World's Lowest-Power 9‑Axis MEMS MotionTracking® Device)
- STM32L412CBT6 (Ultra-low-power Arm® Cortex®-M4 32-bit MCU+FPU)
- MT25QL512ABB (Serial NOR Flash Memory)
- USB Connector, Micro USB Type B
![image](https://github.com/mujiexu2/ELEC0054_Dissertation_XuMujie/blob/main/images/pcb%20board.jpg)

## Component List:
![image](https://github.com/mujiexu2/ELEC0054_Dissertation_XuMujie/blob/main/images/components%20list.jpg)

## File Description
### ICM_SPI_rtc(STM32CubeIDE programs):
- icm20948.c: Read accelerometer(unit: g), gyroscope(units: dps) and magnetometer(units: uT) data
- time.c: Get UTC time, current local UK time, elapsed time
- main.c: Combined time data and sensor data, and sent to USB VCP for future analysis


#### STM32CubeIDE SPI configuration
![image](https://github.com/mujiexu2/ELEC0054_Dissertation_XuMujie/blob/main/images/stm32cube%20SPI%20configuration.jpg)

#### STM32CubeIDE USB + USB Device configuration
![image](https://github.com/mujiexu2/ELEC0054_Dissertation_XuMujie/blob/main/images/stm32cube%20USB%20configuration.jpg)

![image](https://github.com/mujiexu2/ELEC0054_Dissertation_XuMujie/blob/main/images/stm32cube%20USB%20device%20configuration.jpg)

#### STM32CubeIDE RTC configuration
![image](https://github.com/mujiexu2/ELEC0054_Dissertation_XuMujie/blob/main/images/stm32cube%20RTC%20configuration.jpg)

### PCB(DipTrace files):
- PCB-Xu Mujie-0614.dip: The PCB Diptrace file for assembled PCB board
![image](https://github.com/mujiexu2/ELEC0054_Dissertation_XuMujie/blob/main/images/pcb-0614.jpg)
- PCB-Xu Mujie-spi-0815.dip: The newest PCB Diptrace file
![image](https://github.com/mujiexu2/ELEC0054_Dissertation_XuMujie/blob/main/images/pcb%20board-diptrace.jpg)

### python API(python .py/.ipynb)
- time.c/time.ipynb:
  - receive data(time and sensor data) transmitted from the USB VCP
  - split data and save as .csv file in local directory
  - The csv file stored in the format:
    ![image](https://github.com/mujiexu2/ELEC0054_Dissertation_XuMujie/blob/main/images/csv.png)

### schematics(KiCad file)
- 1_nrst.kicad_sch: circuits schematic up to date version



