#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Include Bluetooth headers
#include "esp_bt.h"
#include "esp_bt_main.h"

// Define a structure for Wi-Fi credentials.
struct WiFiNetwork {
  const char* ssid;
  const char* password;
};

// List of available Wi-Fi networks.
WiFiNetwork wifiNetworks[] = {
  {"SSID1", "password1"},
  {"SSID2", "password2"},
  {"SSID3", "password3"}
};
const int NUM_WIFI_NETWORKS = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);

// Firebase Configuration
#define API_KEY "<FIREBASE_API_KEY>"
#define DATABASE_URL "<FIREBASE_DATABASE_URL>"
#define USER_EMAIL "<FIREBASE_USER_EMAIL>"
#define USER_PASSWORD "<FIREBASE_USER_PASSWORD>"

// Pin configuration
const int LED_PIN = 5;
const int BUTTON_PIN = 4;    // Using this pin as a capacitive touch sensor
const int BUILTIN_LED = 2;

// Define a threshold for touch detection (adjust based on your environment)
#define TOUCH_THRESHOLD 50

// Global variables for connection management
unsigned long lastReconnectAttempt = 0;
unsigned long lastButtonCheck = 0;
const unsigned long RECONNECT_INTERVAL = 5000;
const unsigned long BUTTON_CHECK_INTERVAL = 100;
bool isFirebaseInitialized = false;

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 3600000);

// Function declarations
bool initWiFi();
bool initFirebase();
void handleFirebaseError();
void handleOrbStates();

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting setup...");

  // Disable Bluetooth to reduce power consumption
  btStop();
  esp_bt_controller_disable();

  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
  // For capacitive touch sensor, no need to set BUTTON_PIN as INPUT_PULLUP
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUILTIN_LED, LOW);

  // Initialize Wi-Fi by trying each network in the list.
  while (!initWiFi()) {
    Serial.println("WiFi connection failed. Retrying in 5 seconds...");
    delay(5000);
  }

  // Initialize NTP
  timeClient.begin();
  timeClient.setTimeOffset(0);
  timeClient.update();
  Serial.println("Current time: " + timeClient.getFormattedTime());

  // Initialize Firebase
  while (!initFirebase()) {
    Serial.println("Firebase initialization failed. Retrying in 5 seconds...");
    delay(5000);
  }
}

bool initWiFi() {
  Serial.println("Attempting to connect to available Wi-Fi networks...");
  // Loop through each network in our list.
  for (int i = 0; i < NUM_WIFI_NETWORKS; i++) {
    Serial.print("Connecting to: ");
    Serial.println(wifiNetworks[i].ssid);
    WiFi.begin(wifiNetworks[i].ssid, wifiNetworks[i].password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.print("Connected to ");
      Serial.println(wifiNetworks[i].ssid);
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      return true;
    } else {
      Serial.println();
      Serial.print("Failed to connect to ");
      Serial.println(wifiNetworks[i].ssid);
    }
  }
  // None of the networks connected.
  return false;
}

bool initFirebase() {
  if (isFirebaseInitialized) {
    return true;
  }

  Serial.println("Initializing Firebase...");
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Firebase.RTDB.setReadTimeout(&fbdo, 1000 * 60);
  Firebase.RTDB.setwriteSizeLimit(&fbdo, "tiny");

  Serial.println("Waiting for Firebase authentication...");
  int authAttempts = 0;
  while (!Firebase.ready() && authAttempts < 30) {
    Serial.print(".");
    delay(1000);
    authAttempts++;
  }

  if (Firebase.ready()) {
    Serial.println("\nFirebase authenticated successfully!");
    isFirebaseInitialized = true;
    return true;
  }

  Serial.println("\nFirebase authentication failed!");
  return false;
}

void handleFirebaseError() {
  Serial.println("Firebase error: " + fbdo.errorReason());
  
  if (!Firebase.ready()) {
    isFirebaseInitialized = false;
    Serial.println("Attempting to reinitialize Firebase...");
    initFirebase();
  }
}

void handleOrbStates() {
  // Check FirstOrbState from Firebase
  if (Firebase.RTDB.getInt(&fbdo, "/FirstOrbState")) {
    int firstOrbState = fbdo.intData();
    if (firstOrbState == 1) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }
  } else {
    Serial.println("Error reading FirstOrbState: " + fbdo.errorReason());
  }

  // Read the capacitive touch sensor value
  int touchValue = touchRead(BUTTON_PIN);

  if (touchValue < TOUCH_THRESHOLD) {
    Serial.println("Touch detected! Updating FirstOrbState in Firebase.");
    
    // Light up local indicators immediately
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUILTIN_LED, HIGH);
    
    // Update Firebase state to 1.
    if (Firebase.RTDB.setInt(&fbdo, "/FirstOrbState", 1)) {
      // Wait 5000 ms (blocking delay here for simplicity)
      delay(5000);
      
      // Try to set Firebase state to 0.
      int attempts = 0;
      bool success = false;
      while (attempts < 3 && !success) {
        if (Firebase.RTDB.setInt(&fbdo, "/FirstOrbState", 0)) {
          success = true;
          Serial.println("Successfully updated FirstOrbState to 0.");
        } else {
          Serial.print("Attempt ");
          Serial.print(attempts);
          Serial.println(" to reset FirstOrbState failed. Retrying...");
          attempts++;
          delay(1000);  // Wait a second before retrying
        }
      }
      digitalWrite(BUILTIN_LED, LOW);
    } else {
      Serial.println("Failed to update FirstOrbState to 1.");
    }
  }
}

void loop() {
  // Check Wi-Fi connection.
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    if (initWiFi()) {
      initFirebase();
    }
    delay(1000);
    return;
  }

  // Ensure Firebase is ready.
  if (!Firebase.ready()) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastReconnectAttempt > RECONNECT_INTERVAL) {
      lastReconnectAttempt = currentMillis;
      Serial.println("Firebase not ready. Attempting to reinitialize...");
      initFirebase();
    }
    delay(100);
    return;
  }

  // Handle Orb states only when Firebase is ready.
  unsigned long currentMillis = millis();
  if (currentMillis - lastButtonCheck >= BUTTON_CHECK_INTERVAL) {
    lastButtonCheck = currentMillis;
    handleOrbStates();
  }
}
