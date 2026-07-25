# Clean Transcript — EASE Air Quality Lab

## Introduction

Hello. Today, I am going to present our **EASE Air Quality Lab**.

The objective is to build an environmental monitoring station capable of measuring both indoor and outdoor air quality.

At the current stage, we have an enclosure that serves as a multi-sensor test bench. It allows us to compare different technologies and select the sensors best suited to future applications.

The system is modular. We use **Arduino UNO R4 WiFi** boards with the **Grove** ecosystem, eliminating the need for soldering.

Two architectures have been developed:

- a version that uses the local Wi-Fi network;
- a long-range version that uses LoRa radio technology.

## Power Supply and User Interface

The enclosure is designed to be flexible. It can be powered by an external USB charger or by a battery installed directly inside it.

A small OLED screen displays real-time data on the enclosure and allows us to verify that the system is operating correctly. A button can be used to turn off the screen and conserve battery power.

## First Architecture: Wi-Fi Version

The Wi-Fi version uses a single Arduino board. The board connects to the local Wi-Fi network and sends data through HTTP POST requests to a server developed in Python.

This architecture is simple and direct. Its main drawback is its range, which is limited to the available Wi-Fi coverage. Configuration can also be difficult in high schools, particularly because IP addresses may change. The use of fixed IP addresses will therefore need to be considered.

## Second Architecture: LoRa Version

The LoRa version uses two Arduino boards:

- a battery-powered transmitter equipped with the sensors and a LoRa antenna;
- a receiver connected by USB to the computer running the Python server.

The transmitter and receiver communicate by radio waves. Under good conditions, the range can reach several kilometres. The receiver then sends the data to the computer through its USB connection.

The advantage of this architecture is that the sensors can be installed in many locations—a garden, a forest, or another building—without relying on a Wi-Fi network. Its drawback is that it requires two Arduino boards.

## Data Collection and Processing

A server developed in Python collects all the data. It is based on the **Flask** framework and uses **Waitress** as its production server.

In the Wi-Fi version, the server listens on **port 5000** for the HTTP packets sent by the Arduino.

In the LoRa version, the server monitors the USB receiver’s COM port using the **PySerial** library.

## Dashboard

Once collected, the data is presented on a dashboard accessible through a web page.

The dashboard displays:

- real-time values;
- real-time charts when the system is connected;
- documentation;
- information about the assembly.

## Data Storage

The data is saved in a CSV file that can be opened in spreadsheet software. The file contains 23 columns, with a new row recorded every ten seconds. This recording interval can be configured.

The collected information includes:

- a timestamp containing the date and time;
- ambient parameters: temperature, humidity, and pressure;
- air-quality measurements: CO₂, VOCs, eCO₂, and HCHO;
- fine-particle measurements.

An RTC clock built into the Arduino board ensures reliable timestamping.

The data is also stored on an SD card inside the sensor enclosure. This local backup provides additional protection in the event of a connection problem.

The file is transferred automatically to the server. When the transmission works correctly, the same CSV file is therefore available on the computer, avoiding the need to remove the SD card from the enclosure.

## Sensors

The test bench contains many different sensors:

- general environmental sensors for temperature and humidity;
- sensors that measure volatile organic compounds, or VOCs;
- an optical sensor that measures actual CO₂ concentrations;
- fine-particle sensors;
- sensors that measure substances including NO₂, CO, ethanol, and VOCs;
- a formaldehyde, or HCHO, sensor;
- sensors that measure TVOCs;
- a sensor that provides a VOC index.

The objective is to compare these technologies and determine which sensors should be selected for the final versions of the stations, according to their intended applications.

## Planned Developments

Active ventilation has been integrated because a possible greenhouse effect was observed inside the enclosures. Testing will determine whether a fan is actually necessary.

A GPS module may also be added to provide precise geolocation without requiring different pieces of information to be entered manually.

The final solution will need to be adapted to each application. Several choices remain to be made:

- battery power;
- mains power;
- autonomous operation with a solar panel;
- Wi-Fi communication;
- LoRa communication;
- a direct USB connection.

## Three Final Applications

### 1. Classroom Air-Quality Monitoring

This station will include:

- a CO₂ sensor;
- a VOC sensor;
- temperature and humidity sensors;
- a push button for indicating a ventilation action, such as opening a door or window;
- data visualisation on the server.

### 2. Outdoor Monitoring Along Students’ Home-to-School Routes

This application will require a compact, battery-powered enclosure. It will measure fine particles and another gas that has yet to be specified.

### 3. Outdoor Monitoring at a Specific Location

The third station will measure air quality at a specific location, such as on the roof or in the courtyard of a high school.

The enclosure will need to be energy-autonomous, transmit data through a LoRa module, and withstand adverse weather conditions.

Thank you for your attention.
