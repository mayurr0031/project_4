#ifndef THEFT_DETECTOR_H
#define THEFT_DETECTOR_H

#include <Arduino.h>

// Theft Detection System
class TheftDetector {
private:
    static const uint8_t BUZZER_PIN = 15;
    static const unsigned long DETECTION_DURATION = 15000; // 15 seconds
    static constexpr float THEFT_THRESHOLD = 0.004; // 0.004A difference threshold
    
    bool theftDetected;
    bool buzzerActive;
    unsigned long theftStartTime;
    unsigned long lastBuzzerToggle;
    bool buzzerState;
    bool continuousTheft;
    
public:
    TheftDetector() : theftDetected(false), buzzerActive(false), 
                      theftStartTime(0), lastBuzzerToggle(0), 
                      buzzerState(false), continuousTheft(false) {}
    
    void begin() {
        pinMode(BUZZER_PIN, OUTPUT);
        digitalWrite(BUZZER_PIN, LOW);
        Serial.println("🚨 Theft Detection System initialized");
    }
    
    // Check for theft condition
    bool checkTheft(float mainCurrent, float totalCurrent) {
        float currentDifference = mainCurrent - totalCurrent;
        
        // Check if difference exceeds threshold
        if (currentDifference > THEFT_THRESHOLD) {
            if (!continuousTheft) {
                // Start of potential theft
                continuousTheft = true;
                theftStartTime = millis();
                Serial.println("⚠️ Potential theft detected - monitoring...");
            } else {
                // Check if threshold exceeded for full duration
                if (millis() - theftStartTime >= DETECTION_DURATION) {
                    if (!theftDetected) {
                        theftDetected = true;
                        buzzerActive = true;
                        Serial.println("🚨 THEFT CONFIRMED! Alert activated!");
                        Serial.print("   Current Difference: ");
                        Serial.print(currentDifference, 4);
                        Serial.println(" A");
                        return true; // New theft detected
                    }
                }
            }
        } else {
            // Reset if difference drops below threshold
            if (continuousTheft && !theftDetected) {
                continuousTheft = false;
                Serial.println("✅ False alarm - current normalized");
            }
        }
        
        return false;
    }
    
    // Update buzzer (call this in loop)
    void updateBuzzer() {
        if (buzzerActive) {
            // Toggle buzzer at 2Hz (500ms on/off)
            if (millis() - lastBuzzerToggle >= 500) {
                buzzerState = !buzzerState;
                digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
                lastBuzzerToggle = millis();
            }
        } else {
            digitalWrite(BUZZER_PIN, LOW);
            buzzerState = false;
        }
    }
    
    // Reset theft alert (called when relay3 is manually turned on from web)
    void resetAlert() {
        theftDetected = false;
        buzzerActive = false;
        continuousTheft = false;
        digitalWrite(BUZZER_PIN, LOW);
        Serial.println("✅ Theft alert cleared by user");
    }
    
    // Check if theft is currently detected
    bool isTheftDetected() const {
        return theftDetected;
    }
    
    // Get remaining time until theft confirmation (for display)
    unsigned long getRemainingTime() const {
        if (continuousTheft && !theftDetected) {
            unsigned long elapsed = millis() - theftStartTime;
            if (elapsed < DETECTION_DURATION) {
                return (DETECTION_DURATION - elapsed) / 1000; // Return seconds
            }
        }
        return 0;
    }
};

#endif // THEFT_DETECTOR_H