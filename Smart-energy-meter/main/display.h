// Display.h
#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Enhanced wrapper for 16x2 I2C LCD
class Display {
private:
    LiquidCrystal_I2C lcd;
    bool initialized;
    uint8_t displayMode;  // 0=currents, 1=power, 2=voltage

public:
    // Constructor: default address 0x27, 16x2 display
    Display(uint8_t address = 0x27, uint8_t cols = 16, uint8_t rows = 2)
      : lcd(address, cols, rows), initialized(false), displayMode(0) {}

    // Initialize the display
    void begin() {
        if (initialized) return;
        Wire.begin();
        lcd.init();
        lcd.backlight();
        lcd.clear();
        initialized = true;
    }

    // Show currents and voltage (synchronized with your requirements)
    void showCurrents(float i1, float i2, float voltage) {
        if (!initialized) return;
        lcd.clear();
        
        // Line 1: Current readings
        lcd.setCursor(0, 0);
        lcd.print("I1:");
        lcd.print(i1, 2);  // 2 decimal places for better readability
        lcd.print("A I2:");
        lcd.print(i2, 2);
        lcd.print("A");
        
        // Line 2: Voltage
        lcd.setCursor(0, 1);
        lcd.print("V:");
        lcd.print(voltage, 1);  // 1 decimal place
        lcd.print("V");
    }
    
    // Alternative: Show power and voltage
    void showPower(float power1, float power2, float voltage) {
        if (!initialized) return;
        lcd.clear();
        
        // Line 1: Power readings
        lcd.setCursor(0, 0);
        lcd.print("P1:");
        lcd.print(power1, 1);
        lcd.print("W P2:");
        lcd.print(power2, 1);
        lcd.print("W");
        
        // Line 2: Voltage
        lcd.setCursor(0, 1);
        lcd.print("V:");
        lcd.print(voltage, 1);
        lcd.print("V");
    }
    
    // Show total power and current
    void showTotals(float totalCurrent, float totalPower, float voltage) {
        if (!initialized) return;
        lcd.clear();
        
        // Line 1: Total current and power
        lcd.setCursor(0, 0);
        lcd.print("I:");
        lcd.print(totalCurrent, 2);
        lcd.print("A P:");
        lcd.print(totalPower, 1);
        lcd.print("W");
        
        // Line 2: Voltage
        lcd.setCursor(0, 1);
        lcd.print("Voltage:");
        lcd.print(voltage, 1);
        lcd.print("V");
    }
    
    // Display custom message
    void showMessage(const char* line1, const char* line2 = "") {
        if (!initialized) return;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(line1);
        if (strlen(line2) > 0) {
            lcd.setCursor(0, 1);
            lcd.print(line2);
        }
    }
    
    // Clear display
    void clear() {
        if (!initialized) return;
        lcd.clear();
    }
};

#endif // DISPLAY_H