// CurrentSensor.h - Simplified Version Matching Working_ACS712.ino Logic
#ifndef CURRENT_SENSOR_H
#define CURRENT_SENSOR_H

#include <Arduino.h>
#include <Filters.h>

class CurrentSensor {
private:
    uint8_t pin;
    float slope;
    float intercept;
    float offset;
    float window;
    float adcRef;
    int adcMax;
    
    RunningStatistics stats;
    bool calibrated;
    
public:
    // Constructor
    // Takes the pin number and the linear regression slope/intercept (A/mV and A)
    CurrentSensor(uint8_t _pin, float _slope, float _intercept) {
        pin = _pin;
        slope = _slope;
        intercept = _intercept;
        offset = 0;
        window = 40.0 / 50.0;  // 40ms window for 50Hz
        adcRef = 3300.0;       // ESP32 reference voltage in mV (3.3V)
        adcMax = 4095;         // 12-bit ADC resolution
        calibrated = false;
    }

    // Initialize the sensor
    void begin() {
        pinMode(pin, INPUT);
        analogReadResolution(12);
        stats.setWindowSecs(window);
    }

    // Calibration: Simplified to only calculate Zero Offset (matching .ino logic)
    void calibrate(int samples = 1500) {
        Serial.print("Calibrating sensor on pin ");
        Serial.print(pin);
        Serial.println("...");
        
        float sum = 0;
        
        // Collect samples for offset calculation
        for (int i = 0; i < samples; i++) {
            // Read and convert ADC value to mV
            float reading = (analogRead(pin) * adcRef) / adcMax;
            sum += reading;
            delay(1); // Small delay to match the pacing in the .ino file
        }

        offset = sum / samples;
        calibrated = true;
        
        Serial.print("Zero offset calibrated to: ");
        Serial.print(offset, 2);
        Serial.println(" mV");
    }

    // Collect samples continuously and feed them to the RunningStatistics filter
    void update() {
        if (!calibrated) return;
        
        float mv = (analogRead(pin) * adcRef) / adcMax;
        float corrected_mv = mv - offset;
        stats.input(corrected_mv);
    }

    // Compute RMS -> Amps using only linear regression and noise threshold (matching .ino logic)
    // Note: The original sen_num parameter was removed as it was unused and unnecessary.
    float getCurrent() {
        if (!calibrated) return 0.0;
        
        // 1. Get RMS voltage (standard deviation) from the RunningStatistics window
        float rms_voltage = stats.sigma();
        
        // 2. Convert RMS voltage to current using linear regression (A = Intercept + Slope * V_RMS)
        // This directly implements the core calculation from the .ino file.
        float current = intercept + slope * rms_voltage;
        
        // 3. Apply the simple noise threshold (clamp anything below 0.002A to 0, matching the .ino file)
        if (current < 0.002) {
            current = 0.0;
        }
        
        return current;
    }
    
    // Get current offset value
    float getOffset() const {
        return offset;
    }

    // Set custom window length
    void setWindow(float freq) {
        window = 40.0 / freq;
        stats.setWindowSecs(window);
    }
    
    // Check if calibrated
    bool isCalibrated() const {
        return calibrated;
    }
    
    // Removed all advanced filtering/noise suppression methods and members.
};

#endif