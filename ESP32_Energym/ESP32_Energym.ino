#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "moto g(60)s_1622"
#define WIFI_PASSWORD "sofi1234"

// Insert Firebase project API Key
#define API_KEY "AIzaSyDabWGSqR2t98L11OSxnb8aTBGMmWgsxMY"

#define DATABASE_URL "https://energym-f9949-default-rtdb.firebaseio.com/"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "sofia2@gmail.com"
#define USER_PASSWORD "123456"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  // Initialize WiFi
  initWiFi();

  // Assign the api key (required)
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);
}

#define INIT 0
#define MEDIR 1
#define ENVIAR 2
#define REINICIAR 3
int estado = 0;

int voltaje = 12;
int corriente = 10;
int ultCorriente = 10;
int tiempoCambioDatos = 0;
int tiempo = 0;
int numDato = 0;
float energy[] = {};
float sumatoria = 0.00;
int cicloActual = 1;
int medicion = 0;

void loop() {
  if (Firebase.isTokenExpired()) {
    Firebase.refreshToken(&config);
    Serial.println("Refresh token");
  }
  if (Serial.available()) {
    corriente = medirCorriente();
  }
  maquina();
}

void maquina() {
  switch (estado) {
    case INIT:
      tiempoCambioDatos = millis();
      estado = MEDIR;
      break;

    case MEDIR:
      if ((ultCorriente - corriente > 2) || (ultCorriente - corriente < -2)) {
        ultCorriente = corriente;
        tiempo = millis() - tiempoCambioDatos;
        energy[numDato] = voltaje * corriente / (tiempo / 1000);
        numDato = numDato + 1;
        estado = INIT;
      }
      if (millis() > (60000 * cicloActual)) {
        if (numDato == 0) {
          energy[0] = voltaje * corriente / 60;
          Serial.print("Dato 0: ");
          Serial.println(energy[0]);
        }
        estado = ENVIAR;
      }
      break;

    case ENVIAR:
      for (byte i = 0; i == (sizeof(energy) / sizeof(energy[0])); i++) {  //i < (sizeof(energy) / sizeof(energy[0]))
        sumatoria = sumatoria + energy[i];
      }
      Serial.print("Sumatoria: ");
      Serial.println(sumatoria);
      enviarDatos();
      estado = REINICIAR;
      break;

    case REINICIAR:
      sumatoria = 0;
      numDato = 0;
      cicloActual = cicloActual + 1;
      estado = INIT;
      break;
  }
}

void enviarDatos() {
  if (Firebase.ready()) {
    // Escribir un float en usuarios/"id"/Energy
    if (Firebase.RTDB.setFloat(&fbdo, "users/" + uid + "/Energy", sumatoria)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

int medirCorriente() {
  medicion = Serial.parseInt();
  return medicion;
}