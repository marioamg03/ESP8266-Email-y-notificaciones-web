#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include "Gsender.h"
#include <FirebaseArduino.h>
#include <Thread.h>

// Configuramos FIREBASE y el host Wifi
#define FIREBASE_HOST "esp8266-513d6.firebaseio.com"
#define FIREBASE_AUTH "tP4eYMu6E7ucdFpDbWlMNtdpULczyDQ8vIavGqa2"

Thread thrCorreo = Thread();
Thread thrSerial = Thread();

int cont =0;
#pragma region Globals
const char* ssid = "CasaMG";                                 // Nombre de la Red Wifi
const char* password = "10121993Mar.";                       // Clave de la Red Wifi
uint8_t connection_state = 0;                                // COneccion a red Wifi si o no.
uint16_t reconnect_interval = 10000;                         // Si no esta conectado re-intentarlo en...
#pragma endregion Globals

SoftwareSerial serialpic(13,15);//Declaramos el pin Rx y Tx             // 9 y 10 /////GPIO 13 y 15 /// D7 y D8

uint8_t WiFiConnect(const char* nSSID = nullptr, const char* nPassword = nullptr)
{
    static uint16_t attempt = 0;
    Serial.print("Connecting to ");
    if(nSSID) {
        WiFi.begin(nSSID, nPassword);
        Serial.println(nSSID);
    } else {
        WiFi.begin(ssid, password);
        Serial.println(ssid);
    }

    uint8_t i = 0;
    while(WiFi.status()!= WL_CONNECTED && i++ < 50)
    {
        delay(200);
        Serial.print(".");
    }
    ++attempt;
    Serial.println("");
    if(i == 51) {
        Serial.print("Conexión: TIMEOUT en el intento: ");
        Serial.println(attempt);
        if(attempt % 2 == 0)
            Serial.println("Compruebe si el punto de acceso disponible o SSID y contraseña\r\n");
        return false;
    }
    Serial.println("Conexión establecida");
    Serial.print("Obtener dirección IP: ");
    Serial.println(WiFi.localIP());
    return true;
}

void Awaits()
{
    uint32_t ts = millis();
    while(!connection_state)
    {
        delay(50);
        if(millis() > (ts + reconnect_interval) && !connection_state){
            connection_state = WiFiConnect();
            ts = millis();
        }
    }
}

void setup()
{
    Serial.begin(115200);
    serialpic.begin(115200);                                                //Iniciamos el puerto serie del PIC
    connection_state = WiFiConnect();
    if(!connection_state)  // si no esta conectado a la red Wifi
        Awaits();          // intentar conectar constantemente

    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Firebase.stream("/Estado/Boton");

  thrCorreo.enabled = true;
  thrCorreo.setInterval(10);
  thrCorreo.onRun(correo);

  thrSerial.enabled = true;
  thrSerial.setInterval(10);
  thrSerial.onRun(serial);
}

void serial() {
  if (Serial.available() > 0) {
    String str = Serial.readStringUntil('\n');
    Serial.println(str);
    Firebase.setString("/Estado/Datos", str);
 }
}


void correo() { 
  if (Firebase.failed()) {
    // Serial.println("streaming error");
    // Serial.println(Firebase.error());
  }
  if (Firebase.available()) {
    FirebaseObject event = Firebase.readEvent();
    String eventType = event.getString("type");
    eventType.toLowerCase();
    Serial.println(eventType);
    
    if (eventType == "put") {
      Gsender *gsender = Gsender::Instance();    // Obtención de puntero a instancia de clase
      String subject = "Cambio de status!";   // Asunto del Mensaje

      Serial.print("Cambio de dato a: ");
      Serial.println(event.getString("data"));

      if(gsender->Subject(subject)->Send("mario.amg03@gmail.com", "Cambio a: " + event.getString("data"))) {
        Serial.println("Mensaje enviado satisfactoriamente.");
      } else {
        Serial.print("Error enviando el mensaje: ");
        Serial.println(gsender->getError());
      }
    }
  }
}




void loop(){
  
  if(thrCorreo.shouldRun()){
    thrCorreo.run();
  }
  
  if(thrSerial.shouldRun()){
    thrSerial.run();
  }
  
}
