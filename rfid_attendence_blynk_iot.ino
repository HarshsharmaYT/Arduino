#include <WiFiManager.h>
#include <MFRC522.h>
#include <BlynkSimpleEsp8266.h>

#define SS_PIN D4      // RC522 SDA pin
#define RST_PIN D3     // RC522 RST pin
#define RELAY_PIN D1   // Relay pin for solenoid lock control

MFRC522 rfid(SS_PIN, RST_PIN);  // Create an MFRC522 instance

// Blynk Credentials
#define BLYNK_TEMPLATE_ID "TMPL3LBhFTNSd"
#define BLYNK_TEMPLATE_NAME "RFID LOCK"
#define BLYNK_AUTH_TOKEN "Z_wbNDMygB5kqsr_lVdR4XCZVXZ_HRm1"

char auth[] = BLYNK_AUTH_TOKEN;

// Array for authorized users
struct User {
    const char* name;
    const byte uid[4];
};

User authorizedUsers[] = {
    {"Viraj", {0x06, 0xA9, 0x0D, 0xF0}}, //0x06 0xA9 0x0D 0xF0
    {"Bob",   {0xAB, 0xCD, 0xEF, 0x90}}
};

void setup() {
    Serial.begin(115200);

    WiFiManager wifiManager;
    wifiManager.autoConnect("RFID_Access");

    Serial.println("✅ Wi-Fi Connected Successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    Blynk.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str());

    SPI.begin();
    rfid.PCD_Init();

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);  // Initially lock remains closed

    Serial.println("🔒 RFID System Ready");
}

void loop() {
    Blynk.run();

    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        return;  // No card detected
    }

    Serial.print("Card detected! UID: ");
    for (byte i = 0; i < rfid.uid.size; i++) {
        Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(rfid.uid.uidByte[i], HEX);
    }
    Serial.println();

    bool authorized = false;
    for (const auto& user : authorizedUsers) {
        if (memcmp(rfid.uid.uidByte, user.uid, 4) == 0) {
            Serial.print(user.name);
            Serial.println(" has entered.");

            // Relay ON (Unlock door)
            digitalWrite(RELAY_PIN, HIGH);
            delay(5000); // Door stays open for 5 seconds
            digitalWrite(RELAY_PIN, LOW); // Lock the door again

            Blynk.logEvent("attendance", String(user.name) + " entered the premises.");  // ✅ Authorized Notification
            authorized = true;
            break;
        }
    }

    if (!authorized) {
        Serial.println("❌ Unauthorized access detected!");
        Blynk.logEvent("security_alert", " ❌Unauthorized access detected!");  // ❗ Unauthorized Notification
    }

    rfid.PICC_HaltA();         // Stop reading
    rfid.PCD_StopCrypto1();    // Stop encryption on the RFID reader
    delay(2000);
}
