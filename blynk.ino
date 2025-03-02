#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN "" 

#include <MFRC522.h>
#include <SPI.h>
#include <ESP32Firebase.h>
#include <ArduinoJson.h>
#include <BlynkSimpleEsp32.h>

#define _SSID "" // Your WiFi SSID
#define _PASSWORD "" // Your WiFi Password
#define REFERENCE_URL ""  // Your Firebase project reference URL

#define SS_PIN 5
#define RST_PIN 27

MFRC522 mfrc522(SS_PIN, RST_PIN);

Firebase firebase(REFERENCE_URL);

BlynkTimer timer;

String UID = "";

void setup() {
  Serial.begin(115200);
  SPI.begin(); // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522
  delay(100);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(1000);

  // Connect to WiFi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(_SSID);
  WiFi.begin(_SSID, _PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("-");
  }

  Serial.println("");
  Serial.println("WiFi Connected");

  // Print the IP address
  Serial.print("IP Address: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  // Initialize Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, _SSID, _PASSWORD);

  timer.setInterval(1000L, checkRFID);

  // Initially hide the widget
  Blynk.setProperty(V1, "isHidden", true);
  Blynk.setProperty(V2, "isHidden", true);
}

void loop() {
  Blynk.run();
  timer.run();
}

void checkRFID() {
  // Look for new cards
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    UID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      UID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
      UID += String(mfrc522.uid.uidByte[i], HEX);
    }
    UID.toUpperCase();

    // Clear output of V3 and V4
    Blynk.virtualWrite(V3, "");
    Blynk.virtualWrite(V4, "");

    Blynk.virtualWrite(V0, UID);

    // Check if UID exists in Firebase
    if (checkUIDExists(UID)) {
      // If UID exists, print existing data
      printDataFromFirebase(UID);

      // Hide Text Input widgets
      Blynk.setProperty(V1, "isHidden", true);
      Blynk.setProperty(V2, "isHidden", true);
    } else {
      // Ask for clothing type and color via Blynk
      Blynk.syncVirtual(V1); // Request data from app

      Blynk.syncVirtual(V2); // Request data from app

      // Show Text Input widgets
      Blynk.setProperty(V1, "isHidden", false);
      Blynk.setProperty(V2, "isHidden", false);
    }

    // Wait a bit before reading another card
    delay(2000);
  }
}

BLYNK_WRITE(V0) {
  UID = param.asStr();
  writeDataToFirebase(UID, "", "");
}

BLYNK_WRITE(V1) { // Clothing type input from app
  String jenis = param.asStr();
  writeDataToFirebase(UID, jenis, "");
}

BLYNK_WRITE(V2) { // Color input from app
  String color = param.asStr();
  writeDataToFirebase(UID, "", color);
}

void writeDataToFirebase(String UID, String jenis, String color) {
  // If jenis is empty, do not update it
  if (!jenis.isEmpty()) {
    firebase.setString("Clothes/" + UID + "/jenis", jenis);
  }
  // If color is empty, do not update it
  if (!color.isEmpty()) {
    firebase.setString("Clothes/" + UID + "/color", color);
  }

  // Send UID, clothing type, and color to Blynk app
  Blynk.virtualWrite(V3, jenis);
  Blynk.virtualWrite(V4, color);
}

bool checkUIDExists(String UID) {
  String value = firebase.getString("Clothes/" + UID);
  // Check if the value is the default value
  if (value == "ul") {
    return false;
  } else {
    return value.length() > 0;
  }
}

void printDataFromFirebase(String UID) {
  // Get data for the UID from Firebase
  String jenis = firebase.getString("Clothes/" + UID + "/jenis");
  String color = firebase.getString("Clothes/" + UID + "/color");

  // Display the fetched data in the Label widgets
  Blynk.virtualWrite(V3, jenis);
  Blynk.virtualWrite(V4, color);
}
