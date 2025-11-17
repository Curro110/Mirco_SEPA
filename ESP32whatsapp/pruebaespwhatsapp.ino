#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <UrlEncode.h>

// ====== CONFIGURACIÓN WIFI ======
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// ====== CONFIGURACIÓN CALLMEBOT ======
String phoneNumber = "+34XXXXXXXXX"; 
String apiKey = "REPLACE_WITH_YOUR_API_KEY";

// ====== PINES DE ENTRADA ======
const int pinManana = 4;     // GPIO para pastilla de la mañana
const int pinTarde  = 5;     // GPIO para pastilla de la tarde
const int pinNoche  = 18;    // GPIO para pastilla de la noche

// Variables para recordar último estado
bool lastManana = LOW;
bool lastTarde  = LOW;
bool lastNoche  = LOW;

// ====== FUNCIÓN PARA ENVIAR WHATSAPP ======
void sendMessage(String message){
  String url = "http://api.callmebot.com/whatsapp.php?phone=" 
                + phoneNumber + "&apikey=" + apiKey 
                + "&text=" + urlEncode(message);

  WiFiClient client;    
  HTTPClient http;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http.POST(url);

  if (httpResponseCode == 200){
    Serial.println("Mensaje enviado: " + message);
  } else {
    Serial.print("Error: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

// ====== INICIO ======
void setup() {
  Serial.begin(115200);

  pinMode(pinManana, INPUT);
  pinMode(pinTarde, INPUT);
  pinMode(pinNoche, INPUT);

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado. IP:");
  Serial.println(WiFi.localIP());
}

// ====== LOOP PRINCIPAL ======
void loop() {
  // Leer los pines
  bool estadoManana = digitalRead(pinManana);
  bool estadoTarde  = digitalRead(pinTarde);
  bool estadoNoche  = digitalRead(pinNoche);

  // ---- DETECCIÓN DE CAMBIOS ----
  
  if (estadoManana == HIGH && lastManana == LOW) {
    sendMessage(" NO se ha tomado la pastilla de la MAÑANA.");
    delay(2000);
  }

  if (estadoTarde == HIGH && lastTarde == LOW) {
    sendMessage(" NO se ha tomado la pastilla de la TARDE.");
    delay(2000);
  }

  if (estadoNoche == HIGH && lastNoche == LOW) {
    sendMessage(" NO se ha tomado la pastilla de la NOCHE.");
    delay(2000);
  }

  // Guardar estados previos
  lastManana = estadoManana;
  lastTarde  = estadoTarde;
  lastNoche  = estadoNoche;
}
