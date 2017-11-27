# cpfsae-telemetry
Web browser-based vehicle telemetry for Formula SAE vehicles using C and Node.js on BeagleBone Black

## Overview
The Formula SAE Telemetry System allows for real-time viewing of sensor data from the CP-17C and CP-17E open-wheeled race vehicles. While other telemetry systems are either expensive, inaccessible, inextensible, or some combination of these, the Formula SAE Telemetry System meets and exceeds the needs of Cal Poly Racing’s Formula SAE team at a much lower cost.

The system is unique in that this data is provided through a web interface. This allows it to serve a dual purpose. In addition to being a valuable tool used by the team during testing outings, spectators may connect to the interface simultaneously using a laptop or mobile device. As the Formula SAE vehicles are judged for both engineering design and marketability as products, this feature is a unique selling point that other teams cannot claim with their vehicles.

## System Architecture
A Debian Linux-powered BeagleBone Black microcomputer is the heart of the system; it runs a C program to interpret CAN data frames and a web server to display the data in real time. An Ubiquiti AirGateway LR wireless access point is connected to the BeagleBone using Ethernet, and this access point allows any number of devices to connect to the web server.

## Complete Project Report
The report for this project provides many more details. It can be found here: [Formula SAE Telemetry System Report](https://docs.google.com/document/d/1z2uWAGIEsSnG0PktTVHk5ej-sA3QeC9OkyvsWds6SQc/edit?usp=sharing)
