# ESP32 Air Conditioner Controller

This project allows remote control of an air conditioner using an ESP32 and a custom-built PCB. The PCB includes an ESP32, IR transmitter/receiver, and other necessary components.

A Flutter mobile application sends commands to a database. The ESP32 reads these commands and controls the air conditioner, allowing operation from anywhere with an internet connection.

## Features

- Custom PCB with ESP32 and IR components  
- Remote control via Flutter mobile app  
- Database used for communication between app and ESP32  
- Control your air conditioner from anywhere

## How it Works

1. The mobile app sends a command to the database.  
2. The ESP32 reads the command over Wi-Fi.  
3. The ESP32 sends the corresponding IR signal to the air conditioner.  
