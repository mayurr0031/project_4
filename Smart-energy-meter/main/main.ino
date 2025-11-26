#include <Arduino.h>
#include "PinConfig.h"
#include "IRHandler.h"
#include "Current.h"
#include "Voltage.h"
#include "display.h"
#include "WebClient.h"
#include "TheftDetector.h"
#include "EnergyCalculator.h"

// ===================== CONFIGURATION =====================
const char* WIFI_SSID = "RCB";
const char* WIFI_PASSWORD = "http@007";
const char* SERVER_URL = "http://192.168.31.222:5000";

// Sensor Calibration Constants
const float slope_1 = 0.0007272;    
const float intercept_1 = -0.01636;
const float slope_2 = 0.0007272;     
const float intercept_2 = -0.01672;
const float slope_3 = 0.0006825;     
const float intercept_3 = -0.01442;

// Voltage Sensor Configuration
const uint8_t VOLTAGE_PIN = 35;
const float Vref = 3.3;
const float VOLTAGE_CALIBRATION = 890.0;

// ===================== CREATE INSTANCES =====================
PinConfig pinConfig;
CurrentSensor sensor1(32, slope_1, intercept_1);
CurrentSensor sensor2(33, slope_2, intercept_2);
CurrentSensor sensor3(34, slope_3, intercept_3);
VoltageSensor voltageSensor(VOLTAGE_PIN, Vref, VOLTAGE_CALIBRATION);
IRHandler irHandler(pinConfig);
Display display;
WebClient webClient(WIFI_SSID, WIFI_PASSWORD, SERVER_URL);
TheftDetector theftDetector;
EnergyCalculator energyCalc;

// ===================== TIMING VARIABLES =====================
unsigned long samplePeriod = 1500;   
unsigned long printPeriod = 1500;
unsigned long previousMillis = 0;
unsigned long webSendPeriod = 10000;
unsigned long previousWebMillis = 0;
unsigned long relayPollPeriod = 1500;
unsigned long previousRelayPoll = 0;
unsigned long energySavePeriod = 60000; // Save energy every minute
unsigned long previousEnergySave = 0;

// ===================== GLOBAL VARIABLES =====================
float lastVoltage = 0;
float lastCurrent1 = 0;
float lastCurrent2 = 0;
float lastCurrent3 = 0;
float lastTotalCurrent = 0;
float lastPower1 = 0;
float lastPower2 = 0;
float lastTotalPower = 0;

bool previousRelay1State = false;
bool previousRelay2State = false;
bool initialSyncDone = false;

// ===================== SETUP =====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n========================================");
    Serial.println("⚡ SMART ENERGY METER v2.0");
    Serial.println("   with Theft Detection & Energy Monitoring");
    Serial.println("========================================\n");
    delay(2000);
    
    // Initialize Current Sensors
    Serial.println("📊 Initializing current sensors...");
    sensor1.begin();
    sensor2.begin();
    sensor3.begin();

    // Calibrate Current Sensors
    Serial.println("🔧 Calibrating current sensors...");
    Serial.println("   Ensure NO LOAD is connected!");
    delay(1000);
    sensor1.calibrate(1000);
    delay(500);
    sensor2.calibrate(1000);
    delay(500);
    sensor3.calibrate(1000);
    Serial.println("✅ Calibration complete\n");
    
    // Initialize Voltage Sensor
    Serial.println("⚡ Initializing voltage sensor...");
    voltageSensor.begin();
    
    // Initialize Hardware
    Serial.println("🔌 Initializing relay control...");
    pinConfig.begin();
    
    Serial.println("📡 Initializing IR receiver...");
    irHandler.begin();
    
    Serial.println("📺 Initializing LCD display...");
    display.begin();
    
    // Initialize Theft Detector
    theftDetector.begin();
    
    // Initialize Energy Calculator (default price ₹5 per kWh)
    energyCalc.begin(5.0);
    
    // Initialize WiFi
    webClient.begin();
    
    // Initial relay state sync with server
    if (webClient.isConnected()) {
        Serial.println("🔄 Performing initial relay state sync...");
        previousRelay1State = pinConfig.getRelay1State();
        previousRelay2State = pinConfig.getRelay2State();
        webClient.postRelayState(previousRelay1State, previousRelay2State);
        initialSyncDone = true;
    }
    
    Serial.println("\n========================================");
    Serial.println("✅ SYSTEM READY");
    Serial.println("========================================");
    Serial.println("Features:");
    Serial.println("  • IR Remote & Web Dashboard Control");
    Serial.println("  • Real-time Theft Detection");
    Serial.println("  • Energy Consumption Tracking");
    Serial.println("  • Cost Calculator");
    Serial.println("\nData Flow:");
    Serial.println("  • Sensors → Server: Every 10s");
    Serial.println("  • IR Change → Server: Immediate POST");
    Serial.println("  • Server → ESP32: Every 1.5s (poll)");
    Serial.println("========================================\n");
    
    delay(1000);
}

void readSensors() {
    unsigned long startTime = millis();
    
    while (millis() - startTime < samplePeriod) {
        sensor1.update();
        sensor2.update();
        sensor3.update();
        delayMicroseconds(100);
    }
    
    lastCurrent1 = sensor1.getCurrent();
    lastCurrent2 = sensor2.getCurrent();
    lastCurrent3 = sensor3.getCurrent();
    lastVoltage = voltageSensor.getRmsVoltage();
    
    lastTotalCurrent = lastCurrent1 + lastCurrent2;
    lastPower1 = lastVoltage * lastCurrent1;
    lastPower2 = lastVoltage * lastCurrent2;
    lastTotalPower = lastPower1 + lastPower2;
}

void updateAllDisplays() {
    Serial.println("\n========== READINGS ==========");
    Serial.print("Voltage: ");
    Serial.print(lastVoltage, 2);
    Serial.println(" V");
    
    Serial.print("Current 1: ");
    Serial.print(lastCurrent1, 3);
    Serial.println(" A");
    
    Serial.print("Current 2: ");
    Serial.print(lastCurrent2, 3);
    Serial.println(" A");

    Serial.print("Main Current 3: ");
    Serial.print(lastCurrent3, 3);
    Serial.println(" A");
    
    Serial.print("Total Current: ");
    Serial.print(lastTotalCurrent, 3);
    Serial.println(" A");
    
    Serial.print("Power 1: ");
    Serial.print(lastPower1, 2);
    Serial.println(" W");
    
    Serial.print("Power 2: ");
    Serial.print(lastPower2, 2);
    Serial.println(" W");
    
    Serial.print("Total Power: ");
    Serial.print(lastTotalPower, 2);
    Serial.println(" W");
    
    Serial.print("Relays: R1=");
    Serial.print(pinConfig.getRelay1State() ? "ON" : "OFF");
    Serial.print(" | R2=");
    Serial.println(pinConfig.getRelay2State() ? "ON" : "OFF");
    
    if (theftDetector.isTheftDetected()) {
        Serial.println("⚠️ THEFT ALERT ACTIVE!");
    }
    
    Serial.println("==============================\n");

    display.showCurrents(lastCurrent1, lastCurrent2, lastVoltage);
}

void loop() {
    webClient.maintain();
    
    // Update buzzer if theft detected
    theftDetector.updateBuzzer();
    
    // ========== IR REMOTE CONTROL ==========
    if (irHandler.update()) {
        bool currentRelay1 = pinConfig.getRelay1State();
        bool currentRelay2 = pinConfig.getRelay2State();
        
        Serial.println("📡 IR remote triggered - syncing with server...");
        
        if (webClient.isConnected()) {
            webClient.postRelayState(currentRelay1, currentRelay2);
        }
        
        previousRelay1State = currentRelay1;
        previousRelay2State = currentRelay2;
    }

    // ========== PRINT READINGS AND UPDATE DISPLAY ==========
    if ((unsigned long)(millis() - previousMillis) >= printPeriod) {
        previousMillis = millis();
        
        readSensors();
        updateAllDisplays();
        
        // Update energy calculation
        energyCalc.updateEnergy(lastPower1, lastPower2);
        
        // Check for theft
        if (theftDetector.checkTheft(lastCurrent3, lastTotalCurrent)) {
            // New theft detected - turn off relay3
            pinConfig.setRelay3(false);
            Serial.println("🚨 RELAY 3 TURNED OFF DUE TO THEFT!");
            
            // Notify server about theft
            if (webClient.isConnected()) {
                webClient.sendTheftAlert(true);
            }
        }
    }

    // ========== SEND DATA TO SERVER ==========
    if ((unsigned long)(millis() - previousWebMillis) >= webSendPeriod) {
        previousWebMillis = millis();
        
        if (webClient.isConnected()) {
            Serial.println("📤 Sending data to server...");
            
            // Send sensor data with energy values
            webClient.sendCompleteData(
                lastVoltage,
                lastCurrent1,
                lastCurrent2,
                lastCurrent3,
                lastTotalCurrent,
                lastPower1,
                lastPower2,
                lastTotalPower,
                energyCalc.getEnergyL1(),
                energyCalc.getEnergyL2(),
                energyCalc.getTotalEnergy(),
                energyCalc.getCostL1(),
                energyCalc.getCostL2(),
                energyCalc.getTotalCost(),
                theftDetector.isTheftDetected()
            );
        }
    }
    
    // ========== POLL SERVER FOR COMMANDS ==========
    if ((unsigned long)(millis() - previousRelayPoll) >= relayPollPeriod) {
        previousRelayPoll = millis();
        
        if (webClient.isConnected()) {
            bool relay1 = pinConfig.getRelay1State();
            bool relay2 = pinConfig.getRelay2State();
            bool relay3 = !theftDetector.isTheftDetected(); // Current state
            float newPrice = 0;
            
            // Check server for new commands
            if (webClient.getRelayAndSettings(relay1, relay2, relay3, newPrice)) {
                pinConfig.setRelay1(relay1);
                pinConfig.setRelay2(relay2);
                
                // Check if relay3 is being turned on (theft reset)
                if (relay3 && theftDetector.isTheftDetected()) {
                    pinConfig.setRelay3(true);
                    theftDetector.resetAlert();
                    webClient.sendTheftAlert(false); // Clear theft status
                    Serial.println("✅ Theft alert cleared from web dashboard");
                }
                
                // Update price if changed
                if (newPrice > 0 && newPrice != energyCalc.getPricePerUnit()) {
                    energyCalc.setPricePerUnit(newPrice);
                }
                
                previousRelay1State = relay1;
                previousRelay2State = relay2;
            }
        }
    }
    
    // ========== SAVE ENERGY DATA ==========
    if ((unsigned long)(millis() - previousEnergySave) >= energySavePeriod) {
        previousEnergySave = millis();
        energyCalc.saveToFlash();
    }
}