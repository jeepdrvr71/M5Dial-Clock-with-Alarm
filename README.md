# M5Stack Dial Smart Clock

A feature-rich, open-source digital smart clock built specifically for the M5Stack Dial (ESP32-S3).

This project turns your M5Dial into a fully functional desktop clock, featuring a robust state-machine driven rotary menu, an alarm system, and automatic daily atomic time synchronization via Wi-Fi.

## Features
* **NTP Internet Sync:** Automatically connects to Wi-Fi to fetch the precise atomic time and programs it into the internal hardware RTC (BM8563). It resyncs automatically every 24 hours.
* **Flicker-Free UI:** Uses LovyanGFX text-padding to create perfectly smooth screen updates with zero screen flashing.
* **Rotary Encoder Menu:** Simply spin the dial to access a clean, state-driven UI to adjust settings.
* **Alarm System:** Built-in alarm with visual flashing and buzzer alerts.
* **Live Wi-Fi Status:** Custom-drawn geometry-based Wi-Fi icon that updates in real-time if the connection drops.
* **Manual Overrides:** Manually set the date, time, and alarm directly from the hardware without needing to re-flash the code.

## Hardware Requirements
* [M5Stack Dial](https://m5stack.com/products/m5stack-dial-esp32-s3-smart-rotary-knob)

## Software Dependencies
Before uploading, make sure you have the following installed in your Arduino IDE:
1. ESP32 Board Support Package (by Espressif)
2. `M5Dial` Library (install via the Library Manager, install all dependencies when prompted)
3. `M5Unified` Library

## Setup & Installation
1. Clone this repository or download the `.ino` file.
2. Open `M5Dial_Clock.ino` in the Arduino IDE.
3. Update the Wi-Fi credentials at the top of the file:
   ```cpp
   const char* ssid       = "YOUR_WIFI_SSID";
   const char* password   = "YOUR_WIFI_PASSWORD";

Adjust your timezone offset if needed (the default is gmtOffset_sec = -18000 for EST).

Select the M5Dial board in the Arduino Tools menu.

Compile and upload!

Usage
View Clock: Displays 12-hour time, AM/PM, Date, Wi-Fi status, and active alarm status.

Open Menu: Spin the dial exactly 1 tick in either direction to open the main menu.

Navigate: Spin the dial to cycle through options (requires 2 physical "clicks" per menu jump to prevent accidental skipping).

Select: Press the screen (BtnA) to select a menu option or save a setting.

Dismiss Alarm: When the alarm rings, press the screen to turn it off.
