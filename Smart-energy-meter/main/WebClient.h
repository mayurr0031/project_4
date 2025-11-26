#ifndef WEB_CLIENT_H
#define WEB_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class WebClient {
private:
    const char* ssid;
    const char* password;
    String serverUrl;
    bool connected;
    unsigned long lastReconnectAttempt;
    const unsigned long reconnectInterval = 30000;
    HTTPClient http;

public:
    WebClient(const char* wifi_ssid, const char* wifi_password, const char* server_url)
        : ssid(wifi_ssid), 
          password(wifi_password),
          serverUrl(server_url),
          connected(false),
          lastReconnectAttempt(0) {}

    void begin() {
        Serial.println("\n========================================");
        Serial.println("🌐 WebClient: Connecting to WiFi...");
        Serial.print("   SSID: ");
        Serial.println(ssid);
        Serial.println("========================================");
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
            Serial.println("\n✅ WiFi Connected Successfully!");
            Serial.print("   IP Address: ");
            Serial.println(WiFi.localIP());
            Serial.print("   Signal Strength: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
            Serial.print("   Server URL: ");
            Serial.println(serverUrl);
        } else {
            connected = false;
            Serial.println("\n❌ WiFi Connection Failed!");
        }
        Serial.println("========================================\n");
    }

    void maintain() {
        if (WiFi.status() != WL_CONNECTED) {
            connected = false;
            if (millis() - lastReconnectAttempt >= reconnectInterval) {
                lastReconnectAttempt = millis();
                Serial.println("🔄 WebClient: Reconnecting to WiFi...");
                WiFi.disconnect();
                WiFi.begin(ssid, password);
            }
        } else if (!connected) {
            connected = true;
            Serial.println("\n✅ WebClient: WiFi Reconnected!");
            Serial.print("   IP Address: ");
            Serial.println(WiFi.localIP());
        }
    }

    // Send complete data including energy and theft status
    bool sendCompleteData(float voltage, float current1, float current2, float current3,
                         float totalCurrent, float power1, float power2, float totalPower,
                         float energyL1, float energyL2, float totalEnergy,
                         float costL1, float costL2, float totalCost, bool theftDetected) {
        
        if (!connected) return false;

        StaticJsonDocument<768> doc;
        doc["voltage"] = voltage;
        doc["current1"] = current1;
        doc["current2"] = current2;
        doc["current3"] = current3;
        doc["total_current"] = totalCurrent;
        doc["power1"] = power1;
        doc["power2"] = power2;
        doc["total_power"] = totalPower;
        doc["energy_l1"] = energyL1;
        doc["energy_l2"] = energyL2;
        doc["total_energy"] = totalEnergy;
        doc["cost_l1"] = costL1;
        doc["cost_l2"] = costL2;
        doc["total_cost"] = totalCost;
        doc["theft_detected"] = theftDetected;

        String jsonData;
        serializeJson(doc, jsonData);

        String endpoint = serverUrl + "/api/data";
        http.begin(endpoint);
        http.addHeader("Content-Type", "application/json");
        
        int httpResponseCode = http.POST(jsonData);
        
        if (httpResponseCode > 0) {
            Serial.print("✅ Complete data sent | Response: ");
            Serial.println(httpResponseCode);
            http.end();
            return true;
        } else {
            Serial.print("❌ Failed to send data | Error: ");
            Serial.println(httpResponseCode);
            http.end();
            return false;
        }
    }

    bool postRelayState(bool relay1State, bool relay2State) {
        if (!connected) return false;

        StaticJsonDocument<128> doc;
        doc["relay1"] = relay1State;
        doc["relay2"] = relay2State;

        String jsonData;
        serializeJson(doc, jsonData);

        String endpoint = serverUrl + "/api/relay/state";
        http.begin(endpoint);
        http.addHeader("Content-Type", "application/json");
        
        int httpResponseCode = http.POST(jsonData);
        
        if (httpResponseCode == 200) {
            Serial.print("✅ Relay state posted: R1=");
            Serial.print(relay1State ? "ON" : "OFF");
            Serial.print(", R2=");
            Serial.println(relay2State ? "ON" : "OFF");
            http.end();
            return true;
        } else {
            http.end();
            return false;
        }
    }

    // Get relay states and settings including price and relay3 control
    bool getRelayAndSettings(bool &relay1State, bool &relay2State, bool &relay3State, float &price) {
        if (!connected) return false;

        String endpoint = serverUrl + "/api/relay/state";
        http.begin(endpoint);
        http.setTimeout(5000);
        
        int httpResponseCode = http.GET();
        
        if (httpResponseCode == 200) {
            String payload = http.getString();
            
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, payload);
            
            if (!error) {
                bool newRelay1 = doc["relay1"];
                bool newRelay2 = doc["relay2"];
                bool newRelay3 = doc["relay3"] | relay3State; // Default to current if not present
                float newPrice = doc["price"] | 0;
                
                bool changed = false;
                
                if (newRelay1 != relay1State) {
                    relay1State = newRelay1;
                    Serial.print("🌐 Web command: Relay 1 → ");
                    Serial.println(relay1State ? "ON" : "OFF");
                    changed = true;
                }
                
                if (newRelay2 != relay2State) {
                    relay2State = newRelay2;
                    Serial.print("🌐 Web command: Relay 2 → ");
                    Serial.println(relay2State ? "ON" : "OFF");
                    changed = true;
                }
                
                if (newRelay3 != relay3State) {
                    relay3State = newRelay3;
                    Serial.print("🌐 Web command: Relay 3 (Main) → ");
                    Serial.println(relay3State ? "ON" : "OFF");
                    changed = true;
                }
                
                if (newPrice > 0) {
                    price = newPrice;
                }
                
                http.end();
                return changed;
            }
        }
        
        http.end();
        return false;
    }

    // Send theft alert status
    bool sendTheftAlert(bool detected) {
        if (!connected) return false;

        StaticJsonDocument<128> doc;
        doc["theft_detected"] = detected;

        String jsonData;
        serializeJson(doc, jsonData);

        String endpoint = serverUrl + "/api/theft/alert";
        http.begin(endpoint);
        http.addHeader("Content-Type", "application/json");
        
        int httpResponseCode = http.POST(jsonData);
        
        http.end();
        return (httpResponseCode == 200);
    }

    bool isConnected() {
        return connected && (WiFi.status() == WL_CONNECTED);
    }

    int getSignalStrength() {
        return WiFi.RSSI();
    }

    String getIPAddress() {
        return WiFi.localIP().toString();
    }
};

#endif // WEB_CLIENT_H