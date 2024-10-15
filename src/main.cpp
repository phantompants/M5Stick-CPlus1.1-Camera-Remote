#include <M5StickC.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLEHIDDevice.h>

// Sony A6400 IR settings
const uint16_t kIrLedPin = 9; // M5StickCPlus IR LED pin
IRsend irsend(kIrLedPin);

// GoPro BLE settings for GP05, GP09, GP12
BLEClient* goproClient05;
BLEClient* goproClient09;
BLEClient* goproClient12;
BLERemoteCharacteristic* goproRemoteCharacteristic05;
BLERemoteCharacteristic* goproRemoteCharacteristic09;
BLERemoteCharacteristic* goproRemoteCharacteristic12;
const std::string goproAddress05 = "XX:XX:XX:XX:XX:XX"; // GP05 BLE MAC Address
const std::string goproAddress09 = "YY:YY:YY:YY:YY:YY"; // GP09 BLE MAC Address
const std::string goproAddress12 = "ZZ:ZZ:ZZ:ZZ:ZZ:ZZ"; // GP12 BLE MAC Address

// Insta360 BLE settings
BLEClient* insta360Client;
BLERemoteCharacteristic* insta360RemoteCharacteristic;
const std::string insta360Address = "AA:AA:AA:AA:AA:AA"; // Insta360 RS BLE MAC Address

// iPhone BLE settings
BLEServer* pServer;
BLEService* pService;
BLECharacteristic* pCharacteristic;
bool deviceConnected = false;
const std::string iphoneAddress = "BB:BB:BB:BB:BB:BB"; // iPhone BLE MAC Address

// Camera menu options
enum Camera { SONY_A6400, GOPRO_GP05, GOPRO_GP09, GOPRO_GP12, INSTA360, IPHONE };
Camera selectedCamera = SONY_A6400;

// Set up BLE security for iPhone, GoPro, and Insta360 RS
class MySecurity : public BLESecurityCallbacks {
    uint32_t onPassKeyRequest() {
      Serial.println("PassKeyRequest");
      return 123456; // Set a fixed passkey (optional)
    }

    void onPassKeyNotify(uint32_t pass_key) {
      Serial.printf("PassKeyNotify: %d\n", pass_key);
    }

    bool onConfirmPIN(uint32_t pin) {
      Serial.printf("ConfirmPIN: %d\n", pin);
      return true; // true to confirm the pin
    }

    bool onSecurityRequest() {
      Serial.println("SecurityRequest");
      return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) {
      if (auth_cmpl.success) {
        Serial.println("Pairing Success!");
        deviceConnected = true;
      } else {
        Serial.println("Pairing Failed!");
        deviceConnected = false;
      }
    }
};

// Setup BLE for iPhone (dummy implementation, adjust based on your logic)
void setupBLEForiPhone() {
    // BLE setup for iPhone
    Serial.println("Setting up BLE for iPhone...");
}

// Main display menu
void displayMainMenu() {
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Select Camera:");
    M5.Lcd.println("1. Sony A6400");
    M5.Lcd.println("2. GP05");
    M5.Lcd.println("3. GP09");
    M5.Lcd.println("4. GP12");
    M5.Lcd.println("5. Insta360");
    M5.Lcd.println("6. iPhone");
}

// Display the selected camera
void displaySelectedCamera() {
    M5.Lcd.setCursor(0, 120);
    M5.Lcd.println("Selected: ");
    switch (selectedCamera) {
        case SONY_A6400:
            M5.Lcd.println("Sony A6400");
            break;
        case GOPRO_GP05:
            M5.Lcd.println("GP05");
            break;
        case GOPRO_GP09:
            M5.Lcd.println("GP09");
            break;
        case GOPRO_GP12:
            M5.Lcd.println("GP12");
            break;
        case INSTA360:
            M5.Lcd.println("Insta360");
            break;
        case IPHONE:
            M5.Lcd.println("iPhone");
            break;
    }
}

// Control Sony A6400 via IR
void controlSonyA6400() {
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.println("Sony A6400 Control");
    M5.Lcd.println("A: Shutter");
    M5.Lcd.println("B: Back");

    if (M5.BtnA.wasPressed()) {
        // Send IR code to trigger the shutter
        M5.Lcd.println("Sending Shutter Command...");
        irsend.sendSony(0xA90, 12);  // IR code for Sony A6400 shutter
    }
}

// Control GoPro via BLE
void controlGoPro(BLEClient* goproClient, BLERemoteCharacteristic*& remoteCharacteristic, const std::string& address, const char* name) {
    connectToGoPro(goproClient, remoteCharacteristic, address);
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.printf("%s Control\n", name);
    M5.Lcd.println("A: Shutter");
    M5.Lcd.println("B: Back");

    if (M5.BtnA.wasPressed()) {
        // Send BLE command to start/stop recording
        uint8_t command[] = {0x01};  // Example BLE command to start recording
        remoteCharacteristic->writeValue(command, 1);
        M5.Lcd.printf("%s Recording...\n", name);
    }
}

// Control Insta360 via BLE
void controlInsta360() {
    connectToInsta360();
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.println("Insta360 Control");
    M5.Lcd.println("A: Shutter");
    M5.Lcd.println("B: Back");

    if (M5.BtnA.wasPressed()) {
        // Send BLE command to start/stop recording
        uint8_t command[] = {0x01};  // Example BLE command
        insta360RemoteCharacteristic->writeValue(command, 1);
        M5.Lcd.println("Insta360 Recording...");
    }
}

// Control iPhone via BLE
void controlIPhone() {
    if (!deviceConnected) {
        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Lcd.println("Waiting for iPhone Pairing...");
    } else {
        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Lcd.println("iPhone Control");
        M5.Lcd.println("A: Shutter");
        M5.Lcd.println("B: Back");

        if (M5.BtnA.wasPressed()) {
            // Send BLE command to trigger shutter
            uint8_t shutterCommand[] = {0x01};  // Example shutter command
            pCharacteristic->setValue(shutterCommand, 1);
            pCharacteristic->notify();
            M5.Lcd.println("iPhone Shutter Triggered!");
        }
    }
}

// Dummy functions to simulate connection setup
void connectToGoPro(BLEClient* goproClient, BLERemoteCharacteristic*& remoteCharacteristic, const std::string& address) {
    Serial.println("Connecting to GoPro...");
}

void connectToInsta360() {
    Serial.println("Connecting to Insta360...");
}

// Setup function
void setup() {
    M5.begin();
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("Camera Control");

    // Initialize IR for Sony A6400
    irsend.begin();

    // Initialize BLE for GoPro, Insta360, and iPhone
    BLEDevice::init("M5StickCPlus_CameraControl");

    // Initialize BLE Clients for GoPro cameras
    goproClient05 = BLEDevice::createClient();
    goproClient09 = BLEDevice::createClient();
    goproClient12 = BLEDevice::createClient();

    // Initialize BLE Client for Insta360 RS
    insta360Client = BLEDevice::createClient();

    // Initialize BLE for iPhone
    setupBLEForiPhone();
}

// Loop function
void loop() {
    M5.update();
    displayMainMenu();

    if (M5.BtnA.wasPressed()) {
        switch (selectedCamera) {
            case SONY_A6400:
                controlSonyA6400();
                break;
            case GOPRO_GP05:
                controlGoPro(goproClient05, goproRemoteCharacteristic05, goproAddress05, "GP05");
                break;
            case GOPRO_GP09:
                controlGoPro(goproClient09, goproRemoteCharacteristic09, goproAddress09, "GP09");
                break;
            case GOPRO_GP12:
                controlGoPro(goproClient12, goproRemoteCharacteristic12, goproAddress12, "GP12");
                break;
            case INSTA360:
                controlInsta360();
                break;
            case IPHONE:
                controlIPhone();
                break;
        }
    }

    if (M5.BtnB.wasPressed()) {
        displayMainMenu();  // Return to main menu
    }

    delay(100);
}
