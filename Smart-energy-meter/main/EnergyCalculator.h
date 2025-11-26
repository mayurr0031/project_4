#ifndef ENERGY_CALCULATOR_H
#define ENERGY_CALCULATOR_H

#include <Arduino.h>
#include <Preferences.h>

// Energy Calculation and Cost Management
class EnergyCalculator {
private:
    float energyL1; // kWh
    float energyL2; // kWh
    float totalEnergy; // kWh
    float pricePerUnit; // Price per kWh
    
    unsigned long lastUpdateTime;
    Preferences preferences;
    
public:
    EnergyCalculator() : energyL1(0), energyL2(0), totalEnergy(0), 
                         pricePerUnit(0), lastUpdateTime(0) {}
    
    void begin(float price = 5.0) {
        preferences.begin("energy", false);
        
        // Load saved energy values
        energyL1 = preferences.getFloat("energyL1", 0);
        energyL2 = preferences.getFloat("energyL2", 0);
        totalEnergy = preferences.getFloat("totalEnergy", 0);
        pricePerUnit = preferences.getFloat("price", price);
        
        lastUpdateTime = millis();
        
        Serial.println("⚡ Energy Calculator initialized");
        Serial.print("   Load 1 Energy: ");
        Serial.print(energyL1, 3);
        Serial.println(" kWh");
        Serial.print("   Load 2 Energy: ");
        Serial.print(energyL2, 3);
        Serial.println(" kWh");
        Serial.print("   Total Energy: ");
        Serial.print(totalEnergy, 3);
        Serial.println(" kWh");
        Serial.print("   Price per Unit: ₹");
        Serial.println(pricePerUnit, 2);
    }
    
    // Update energy consumption (call this periodically with power readings)
    void updateEnergy(float power1, float power2) {
        unsigned long currentTime = millis();
        float elapsedHours = (currentTime - lastUpdateTime) / 3600000.0; // Convert ms to hours
        
        if (elapsedHours > 0) {
            // Calculate energy increment in kWh
            float energyIncrementL1 = (power1 / 1000.0) * elapsedHours;
            float energyIncrementL2 = (power2 / 1000.0) * elapsedHours;
            
            energyL1 += energyIncrementL1;
            energyL2 += energyIncrementL2;
            totalEnergy = energyL1 + energyL2;
            
            lastUpdateTime = currentTime;
        }
    }
    
    // Save energy values to flash (call periodically to prevent data loss)
    void saveToFlash() {
        preferences.putFloat("energyL1", energyL1);
        preferences.putFloat("energyL2", energyL2);
        preferences.putFloat("totalEnergy", totalEnergy);
    }
    
    // Set price per unit
    void setPricePerUnit(float price) {
        pricePerUnit = price;
        preferences.putFloat("price", price);
        Serial.print("💰 Price updated to ₹");
        Serial.print(price, 2);
        Serial.println(" per kWh");
    }
    
    // Get energy values
    float getEnergyL1() const { return energyL1; }
    float getEnergyL2() const { return energyL2; }
    float getTotalEnergy() const { return totalEnergy; }
    float getPricePerUnit() const { return pricePerUnit; }
    
    // Calculate costs
    float getCostL1() const { return energyL1 * pricePerUnit; }
    float getCostL2() const { return energyL2 * pricePerUnit; }
    float getTotalCost() const { return totalEnergy * pricePerUnit; }
    
    // Reset energy counters
    void resetEnergy() {
        energyL1 = 0;
        energyL2 = 0;
        totalEnergy = 0;
        saveToFlash();
        Serial.println("🔄 Energy counters reset");
    }
    
    // Print current energy status
    void printStatus() {
        Serial.println("\n========== ENERGY STATUS ==========");
        Serial.print("Load 1: ");
        Serial.print(energyL1, 3);
        Serial.print(" kWh (₹");
        Serial.print(getCostL1(), 2);
        Serial.println(")");
        
        Serial.print("Load 2: ");
        Serial.print(energyL2, 3);
        Serial.print(" kWh (₹");
        Serial.print(getCostL2(), 2);
        Serial.println(")");
        
        Serial.print("Total: ");
        Serial.print(totalEnergy, 3);
        Serial.print(" kWh (₹");
        Serial.print(getTotalCost(), 2);
        Serial.println(")");
        Serial.println("===================================\n");
    }
};

#endif // ENERGY_CALCULATOR_H