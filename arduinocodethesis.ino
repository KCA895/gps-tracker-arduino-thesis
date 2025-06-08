#define TINY_GSM_MODEM_SIM808
#include <TinyGsmClient.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <SoftwareSerial.h>

const char* ssid = "Speed";
const char* password = "bayar100jtdulu";

FirebaseConfig config;
FirebaseAuth auth;
#define FIREBASE_HOST "https://gps-tracker-a09df-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "3phvJvgBzbJRBNJjrqLlyQ848FlpvaEIFjWMDcNY"

FirebaseData firebaseData;

SoftwareSerial sim808(D7, D6); // RX, TX
TinyGsm modem(sim808);
TinyGsmClient client(modem);

float latitude, longitude;
bool useWiFi = false;
bool useGSM = false;
bool gpsAvailable = false;
bool firebaseConnected = false;

void setup() {
  Serial.begin(115200);
  sim808.begin(9600);
  
  Serial.println("Initializing SIM808...");
  if (!modem.restart()) {
    Serial.println("❌ SIM808 initialization failed");
    
    powerCycleSIM808();
  } else {
    Serial.println("✅ SIM808 initialized successfully");
  }

  setupConnection();  

  Serial.println("Initializing Firebase...");
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (Firebase.ready()) {
    Serial.println("✅ Firebase is ready!");
    firebaseConnected = true;
  } else {
    Serial.println("❌ Firebase not ready!");
  }

  modem.enableGPS();
}

void loop() {
  Serial.println("Checking GPS Signal...");
  gpsAvailable = false;
  
  for (int i = 0; i < 5; i++) { 
    if (modem.getGPS(&latitude, &longitude)) {
      gpsAvailable = true;
      break;
    }
    Serial.println("❌ GPS signal not available, retrying...");
    delay(5000);
  }
  
  if (gpsAvailable) {
    Serial.print("Latitude: ");
    Serial.println(latitude, 6);
    Serial.print("Longitude: ");
    Serial.println(longitude, 6);
    
    if (firebaseConnected) {
      sendToFirebase(latitude, longitude);
    } else {
      Serial.println("❌ Firebase not connected, retrying...");
      reconnectFirebase();
    }
  } else {
    Serial.println("❌ GPS data unavailable, retrying loop...");
  }

  delay(10000); 
}

void sendToFirebase(float lat, float lon) {
  Serial.println("📡 Sending data to Firebase...");

  String path = "/gps_tracker/latest";
  String timestamp = String(millis() / 1000); 

  FirebaseJson json;
  json.add("latitude", String(lat, 6));
  json.add("longitude", String(lon, 6));
  json.add("timestamp", timestamp);

  if (Firebase.setJSON(firebaseData, path, json)) {
    Serial.println("✅ Data Sent to Firebase!");
  } else {
    Serial.println("❌ Failed to send data");
    Serial.println(firebaseData.errorReason()); 
  }

  String historyPath = "/gps_tracker/history/" + timestamp;
  if (Firebase.setJSON(firebaseData, historyPath, json)) {
    Serial.println("✅ History entry saved!");
  } else {
    Serial.println("❌ Failed to save history entry");
    Serial.println(firebaseData.errorReason());
  }
}

void setupConnection() {
  Serial.println("Checking WiFi...");
  WiFi.begin(ssid, password);
  unsigned long wifiStart = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    useWiFi = true;
    return;
  }

  Serial.println("\n❌ WiFi not available, switching to GSM...");
  
  Serial.println("Initializing SIM808...");
  if (!modem.restart()) {
    Serial.println("❌ SIM808 initialization failed");
    powerCycleSIM808();
  } else {
    Serial.println("✅ SIM808 initialized successfully");
  }

  if (modem.gprsConnect("internet", "", "")) {
    Serial.println("✅ GSM Connected!");
    useGSM = true;
  } else {
    Serial.println("❌ GSM failed, retrying in 5 seconds...");
    delay(5000);
    setupConnection(); 
  }
}

void reconnectFirebase() {
  Serial.println("Reconnecting to Firebase...");
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (Firebase.ready()) {
    Serial.println("✅ Firebase reconnected!");
    firebaseConnected = true;
  } else {
    Serial.println("❌ Firebase reconnection failed!");
  }
}

void powerCycleSIM808() {
  Serial.println("🔄 Power cycling SIM808...");
  const int powerKeyPin = D5; 
  pinMode(powerKeyPin, OUTPUT);
  digitalWrite(powerKeyPin, LOW);
  delay(1000); 
  digitalWrite(powerKeyPin, HIGH);
  delay(5000); 
  if (modem.restart()) {
    Serial.println("✅ SIM808 restarted successfully");
  } else {
    Serial.println("❌ Failed to restart SIM808");
  }
}
