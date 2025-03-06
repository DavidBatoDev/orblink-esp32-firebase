# ESP32 Firebase Touch Orb

![Project GIF Placeholder](./orbs.gif)

## Table of Contents
1. [Introduction](#introduction)
2. [Hardware Requirements](#hardware-requirements)
3. [Library Requirements](#library-requirements)
4. [Code Overview](#code-overview)
5. [Setup & Configuration](#setup--configuration)
6. [How It Works](#how-it-works)
7. [Usage](#usage)
---

## Introduction
This project demonstrates how to use an **ESP32** board with the **Firebase Realtime Database** to control an LED based on a capacitive touch sensor. It also uses **NTP** to fetch the current time. The goal is to illustrate how to:

- Connect two orbs that lights each other up even from afar
- Connect to multiple Wi-Fi networks by cycling through a list of credentials.
- Disable Bluetooth on the ESP32 to reduce power consumption.
- Initialize and authenticate with Firebase (using the [Firebase ESP Client](https://github.com/mobizt/Firebase-ESP-Client)).
- Read and update data in the Realtime Database.
- Use a capacitive touch pin to trigger an event.

---

## Hardware Requirements
1. **ESP32 Board** (e.g., ESP32 DevKit V1)
2. **LED** and **Current-Limiting Resistor** (for the LED on pin 5)
3. **Wires** and a **Breadboard** (optional, depending on your setup)

**Pin Assignments:**
- **LED_PIN**: `GPIO 5`
- **BUTTON_PIN** (Capacitive Touch): `GPIO 4`
- **BUILTIN_LED**: `GPIO 2` (On-board LED)

> **Note:** Capacitive touch performance may vary by pin and environment. Adjust the threshold in the code if needed.

---

## Library Requirements
In the Arduino IDE, you’ll need to install the following libraries (go to **Sketch** > **Include Library** > **Manage Libraries...** and search):

1. **WiFi** (usually included with the ESP32 board package)
2. **Firebase ESP Client** (by mobizt)
3. **NTPClient** (by Fabrice Weinberg)
4. **ESP BT** (usually included in the ESP32 core)

**ESP32 Board Support**:  
Make sure you have installed the **ESP32 board package** by Espressif Systems. In Arduino IDE:
- Go to **File** > **Preferences**.
- In **Additional Board Manager URLs**, add:  
  `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- Then go to **Tools** > **Board** > **Boards Manager**, search for `esp32` and install.

---

## Code Overview
1. **Wi-Fi Credentials**: You can store multiple SSIDs and passwords. The code will attempt each one in turn.
2. **Firebase Configuration**: Replace the placeholders (`<FIREBASE_API_KEY>`, `<FIREBASE_DATABASE_URL>`, `<FIREBASE_USER_EMAIL>`, and `<FIREBASE_USER_PASSWORD>`) with your actual Firebase credentials.
3. **Capacitive Touch**: The code uses `touchRead(BUTTON_PIN)` to detect a touch event. Adjust `TOUCH_THRESHOLD` to suit your environment.
4. **NTP**: Uses `NTPClient` to fetch and display the current time.

---

## Setup & Configuration
1. **Clone or download** this repository.
2. **Open the `.ino` or `.cpp` file** in the Arduino IDE (or PlatformIO, etc.).
3. **Update the placeholders** in the code:
   ```cpp
   #define API_KEY "<FIREBASE_API_KEY>"
   #define DATABASE_URL "<FIREBASE_DATABASE_URL>"
   #define USER_EMAIL "<FIREBASE_USER_EMAIL>"
   #define USER_PASSWORD "<FIREBASE_USER_PASSWORD>"
    ```
- API_KEY: Your Firebase project's Web API Key.
- DATABASE_URL: Your Firebase Realtime Database URL (e.g., https://<your-project>.firebaseio.com).
- USER_EMAIL and USER_PASSWORD: Credentials for a Firebase user (either the service account or a user account).
4. **Set your Wi-Fi** credentials inside the `wifiNetworks` array.
5. **Compile and upload** the code to your ESP32 board.


## How It Works

1. **Wi-Fi Connection**  
   - The code loops through the listed Wi-Fi networks, attempting to connect to each.  
   - If none succeed, it retries every 5 seconds.

2. **Bluetooth Disabled**  
   - `btStop()` and `esp_bt_controller_disable()` are called to disable Bluetooth, reducing power usage.

3. **Firebase Initialization**  
   - Configures API key, Database URL, and user credentials.  
   - Waits for Firebase to be ready before proceeding.

4. **NTP**  
   - The device fetches the current time from an NTP server.

5. **Capacitive Touch & LED**  
   - Reads the touch sensor on `BUTTON_PIN`.  
   - If touched, it updates the Firebase Realtime Database field `FirstOrbState` to `1`, waits 5 seconds, and sets it back to `0`.  
   - The LED on `LED_PIN` and the built-in LED on `GPIO 2` are controlled accordingly.

6. **Real-time Updates**  
   - The code periodically checks `FirstOrbState` in the Realtime Database.  
   - If it’s `1`, the LED is on; otherwise, it’s off.

---

## Usage

1. **Power up** the ESP32 with the code flashed.  
2. **Open Serial Monitor** (115200 baud) to view logs.  
3. **Touch** the sensor to trigger a state change in Firebase.  
4. **Observe** the LED turning on and off, both locally and in your Firebase console (where `FirstOrbState` changes).


