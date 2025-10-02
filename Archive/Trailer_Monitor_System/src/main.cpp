// main.cpp - ESP32 Trailer Monitor System
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "config.h"

// Victron BLE Integration (proper protocol implementation)
#if ENABLE_VICTRON_MINIMAL
#include "victron_ble_stub.h"
VictronBLE victron;
#endif

// Web server for local monitoring
WebServer server(80);

// Web server handlers
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Trailer Monitor System</title>";
  html += "<meta http-equiv='refresh' content='30'>";
  html += "<style>body{font-family:Arial,sans-serif;margin:20px;} .device{border:1px solid #ccc;margin:10px 0;padding:15px;border-radius:5px;} .online{border-color:green;} .offline{border-color:red;}</style>";
  html += "</head><body>";
  html += "<h1>🚛 Trailer Monitor System</h1>";
  html += "<p><strong>System Status:</strong> Running | <strong>Time:</strong> " + String(millis()/1000) + "s</p>";
  
  #if ENABLE_VICTRON_MINIMAL
  html += "<h2>⚡ Victron Energy Devices</h2>";
  
  // Battery Monitor
  html += "<div class='device " + String(victron.isBMVOnline() ? "online" : "offline") + "'>";
  html += "<h3>🔋 Battery Monitor (BMV-712)</h3>";
  if (victron.isBMVOnline()) {
    html += "<p><strong>Voltage:</strong> " + String(victron.getBatteryVoltage(), 2) + "V</p>";
    html += "<p><strong>Current:</strong> " + String(victron.getBatteryCurrent(), 2) + "A</p>";
    html += "<p><strong>State of Charge:</strong> " + String(victron.getBatterySOC(), 1) + "%</p>";
    html += "<p><strong>Consumed Ah:</strong> " + String(victron.getConsumedAh(), 1) + "Ah</p>";
    html += "<p><strong>Aux Voltage:</strong> " + String(victron.getAuxVoltage(), 2) + "V</p>";
    if (victron.getTimeToGo() > 0) {
      html += "<p><strong>Time to Go:</strong> " + String(victron.getTimeToGo()) + " min</p>";
    }
    if (victron.hasLowVoltageAlarm()) html += "<p><strong>⚠️ LOW VOLTAGE ALARM</strong></p>";
    if (victron.hasHighVoltageAlarm()) html += "<p><strong>⚠️ HIGH VOLTAGE ALARM</strong></p>";
    if (victron.hasLowSOCAlarm()) html += "<p><strong>⚠️ LOW SOC ALARM</strong></p>";
  } else {
    html += "<p><strong>Status:</strong> ❌ OFFLINE</p>";
  }
  html += "</div>";
  
  // Solar Controller
  html += "<div class='device " + String(victron.isSolarOnline() ? "online" : "offline") + "'>";
  html += "<h3>☀️ Solar Controller (MPPT)</h3>";
  if (victron.isSolarOnline()) {
    html += "<p><strong>Voltage:</strong> " + String(victron.getSolarVoltage(), 2) + "V</p>";
    html += "<p><strong>Current:</strong> " + String(victron.getSolarCurrent(), 2) + "A</p>";
    html += "<p><strong>Power:</strong> " + String(victron.getSolarPower(), 2) + "W</p>";
    html += "<p><strong>State:</strong> " + victron.getSolarState() + "</p>";
    html += "<p><strong>Yield Today:</strong> " + String(victron.getYieldToday(), 2) + "kWh</p>";
    if (victron.getSolarError() != "No Error") {
      html += "<p><strong>⚠️ Error:</strong> " + victron.getSolarError() + "</p>";
    }
  } else {
    html += "<p><strong>Status:</strong> ❌ OFFLINE</p>";
  }
  html += "</div>";
  
  // AC Charger
  html += "<div class='device " + String(victron.isACOnline() ? "online" : "offline") + "'>";
  html += "<h3>🔌 AC Charger (IP22)</h3>";
  if (victron.isACOnline()) {
    html += "<p><strong>Voltage:</strong> " + String(victron.getACVoltage(), 2) + "V</p>";
    html += "<p><strong>Current:</strong> " + String(victron.getACCurrent(), 2) + "A</p>";
    html += "<p><strong>Temperature:</strong> " + String(victron.getACTemperature(), 1) + "°C</p>";
    html += "<p><strong>State:</strong> " + victron.getACState() + "</p>";
    if (victron.getACError() != "No Error") {
      html += "<p><strong>⚠️ Error:</strong> " + victron.getACError() + "</p>";
    }
  } else {
    html += "<p><strong>Status:</strong> ❌ OFFLINE</p>";
  }
  html += "</div>";
  #endif
  
  html += "<hr><p><small>Trailer Monitor System v2.0 | Victron Instant Readout</small></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleAPI() {
  #if ENABLE_VICTRON_MINIMAL
  // JSON API response with proper Victron BLE data
  String json = "{\"status\":\"victron_ble_protocol\",";
  json += "\"bmv\":{\"online\":" + String(victron.isBMVOnline() ? "true" : "false");
  json += ",\"voltage\":" + String(victron.getBatteryVoltage(), 2);
  json += ",\"current\":" + String(victron.getBatteryCurrent(), 3);
  json += ",\"soc\":" + String(victron.getBatterySOC(), 1);
  json += ",\"consumed_ah\":" + String(victron.getConsumedAh(), 1) + "},";
  json += "\"solar\":{\"online\":" + String(victron.isSolarOnline() ? "true" : "false");  
  json += ",\"power\":" + String(victron.getSolarPower(), 0);
  json += ",\"current\":" + String(victron.getSolarCurrent(), 2);
  json += ",\"state\":\"" + victron.getSolarState() + "\"},";
  json += "\"ac\":{\"online\":" + String(victron.isACOnline() ? "true" : "false");
  json += ",\"current\":" + String(victron.getACCurrent(), 2);
  json += ",\"temperature\":" + String(victron.getACTemperature(), 1) + "}}";
  server.send(200, "application/json", json);
  #else
  server.send(200, "application/json", "{\"error\":\"Victron module not enabled\"}");
  #endif
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=================================");
  Serial.println("🚛 Trailer Monitor System v2.0");
  Serial.println("Using Victron Instant Readout Protocol");
  Serial.println("=================================");
  
  // Initialize WiFi
  Serial.println("\n[WiFi Module]");
  Serial.printf("Connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long wifi_start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifi_start) < 30000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✓ WiFi Connected!");
    Serial.printf("✓ IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("✓ Web Interface: http://%s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println();
    Serial.println("✗ WiFi Connection Failed - continuing without WiFi");
  }
  
  // Setup web server
  server.on("/", handleRoot);
  server.on("/api/victron", handleAPI);
  server.begin();
  Serial.println("✓ Web Server Started");
  
  // Initialize Victron BLE (minimal optimized version)
  #if ENABLE_VICTRON_MINIMAL
    Serial.println("\n[Victron BLE Module - Minimal Version]");
    Serial.println("Initializing Victron BLE scanner...");
    Serial.println("Target devices:");
    Serial.printf("  🔋 BMV-712: %s\n", BMV712_MAC_ADDRESS);
    Serial.printf("  ☀️ MPPT: %s\n", MPPT_MAC_ADDRESS);
    Serial.printf("  🔌 IP22: %s\n", IP22_MAC_ADDRESS);
    
    // Allow WiFi to stabilize before BLE init (ESP32 timing issue)
    Serial.println("Waiting for system stability...");
    delay(2000);
    
    if (victron.setup()) {
      Serial.println("✓ Victron BLE initialized successfully");
      Serial.println("✓ Starting BLE scan...");
      victron.startScan();
    } else {
      Serial.println("✗ Victron BLE initialization failed");
    }
  #endif

  // OLD: Victron BLE with bindkeys (disabled)
  #if ENABLE_VICTRON_BLE
    Serial.println("\n[Victron BLE Module - DEPRECATED]");
    Serial.println("Initializing Victron BLE Advertising Listener...");
    
    if (victronBLE.init()) {
      Serial.println("✓ Victron BLE initialized");
      Serial.println("✓ Ready to listen for Victron advertising packets");
    } else {
      Serial.println("✗ Victron BLE initialization failed");
    }
  #endif

  Serial.println("\n=================================");
  Serial.println("Setup complete - starting monitoring");
  Serial.println("=================================\n");
}

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Victron BLE - Minimal Version
  #if ENABLE_VICTRON_MINIMAL
    // Restart scan every 30 seconds
    static unsigned long last_scan = 0;
    if (millis() - last_scan > 30000) {
      victron.startScan();
      last_scan = millis();
    }
    
    // Print status every 60 seconds
    static unsigned long last_print = 0;
    if (millis() - last_print > 60000) {
      Serial.println("\n=== Victron Status ===");
      Serial.printf("BMV-712: %s - %.2fV, %.3fA, %.1f%%\n", 
                    victron.isBMVOnline() ? "ONLINE" : "OFFLINE",
                    victron.getBatteryVoltage(), victron.getBatteryCurrent(), victron.getBatterySOC());
      Serial.printf("MPPT: %s - %.0fW, %s\n",
                    victron.isSolarOnline() ? "ONLINE" : "OFFLINE",
                    victron.getSolarPower(), victron.getSolarState().c_str());
      Serial.printf("IP22: %s - %.1fA, %s\n",
                    victron.isACOnline() ? "ONLINE" : "OFFLINE", 
                    victron.getACCurrent(), victron.getACState().c_str());
      last_print = millis();
    }
  #endif
  
  // OLD: Victron BLE with bindkeys (disabled)
  #if ENABLE_VICTRON_BLE
    Serial.println("[Victron] Scanning for advertising packets...");
    
    if (victronBLE.readData()) {
      // Print all detected devices
      victronBLE.printAllDevices();
    } else {
      Serial.println("No Victron devices found in this scan");
    }
  #endif
  
  // Monitor other modules here
  #if ENABLE_FRIDGE
    // TODO: Fridge monitoring
  #endif

  // Check WiFi connection and reconnect if needed
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Connection lost - attempting reconnection...");
    WiFi.reconnect();
  }

  delay(1000);  // 1 second delay for main loop
}