#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "secrets.h" 


#define SS_PIN       21
#define RST_PIN      22
#define LED_PIN      2
#define BUZZER_PIN   5


const String CODIGO_PROYECTO = "IOT-C05F59AF8A";
const String TARJETA_AUTORIZADA = "75 F2 DD 13";
const unsigned long TIEMPO_BLOQUEO_MS       = 17000;  // 17 s de bloqueo
const unsigned long INTERVALO_MUESTREO_MS   = 2600;   // lectura del sensor
const unsigned long PERIODO_PUBLICACION_MS  = 21000;  // telemetría MQTT
const unsigned long INTERVALO_RECONEXION_MS = 4000;   // reintento wifi / mqtt
const int MAX_INTENTOS_FALLIDOS   = 3; // rechazos consecutivos para bloquear (regla asignada)
const int CONFIRMACIONES_ALARMA   = 4; // lecturas consecutivas para confirmar alarma
const unsigned long MARGEN_HISTERESIS_MS = 1000; 


const char* TOPIC_TELEMETRY = "iot/c05f59af8a/telemetry";
const char* TOPIC_STATUS    = "iot/c05f59af8a/status";
const char* TOPIC_COMMAND   = "iot/c05f59af8a/command";
const char* TOPIC_ALERT     = "iot/c05f59af8a/alert";


MFRC522 rfid(SS_PIN, RST_PIN);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);


int intentosFallidos = 0;    
int rechazosParaAlarma = 0;    
bool bloqueado = false;
bool alarmaConfirmada = false;
bool enGuardaHisteresis = false;
String modo = "AUTO";          
bool silenciado = false;       
unsigned long secuencia = 0;

unsigned long tUltimaMuestra = 0;
unsigned long tInicioBloqueo = 0;
unsigned long tFinGuardaHisteresis = 0;
unsigned long tUltimaPublicacion = 0;
unsigned long tUltimoIntentoConexion = 0;


bool avisoAccesoActivo = false;
unsigned long tFinAvisoAcceso = 0;
bool avisoRechazoActivo = false;
unsigned long tFinAvisoRechazo = 0;


void leerRFID();
void actualizarBloqueo();
void actualizarInterfazLocal();
void conectarMqtt();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void aplicarComando(String comando);
void publicarTelemetria();
void publicarAlerta(const char* motivo);

void setup() {
 
  Serial.begin(115200);

  
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

 
  SPI.begin();
  rfid.PCD_Init();

 
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  
  Serial.println("==========================================");
  Serial.print("Sistema RFID conectado - Proyecto: ");
  Serial.println(CODIGO_PROYECTO);
  Serial.print("UID Autorizado: ");
  Serial.println(TARJETA_AUTORIZADA);
  Serial.println("==========================================");
}

void loop() {
  unsigned long ahora = millis();

  
  if (WiFi.status() != WL_CONNECTED) {
    if (ahora - tUltimoIntentoConexion >= INTERVALO_RECONEXION_MS) {
      tUltimoIntentoConexion = ahora;
      Serial.println("Reintentando conexion WiFi...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
  } else if (!mqtt.connected()) {
    if (ahora - tUltimoIntentoConexion >= INTERVALO_RECONEXION_MS) {
      tUltimoIntentoConexion = ahora;
      conectarMqtt();
    }
  } else {
    mqtt.loop();
  }

 
  if (ahora - tUltimaMuestra >= INTERVALO_MUESTREO_MS) {
    tUltimaMuestra = ahora;
    leerRFID();
  }

 
  actualizarBloqueo();

 
  actualizarInterfazLocal();

  
  if (mqtt.connected() && (ahora - tUltimaPublicacion >= PERIODO_PUBLICACION_MS)) {
    tUltimaPublicacion = ahora;
    publicarTelemetria();
  }
}

void leerRFID() {
  
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return; 
  }

 
  if (rfid.uid.size < 4 || rfid.uid.size > 7) {
    Serial.println("-> Lectura no valida (tamano de UID fuera de rango).");
    rfid.PICC_HaltA();
    return;
  }

  
  String tarjetaLeida = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    tarjetaLeida += String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    tarjetaLeida += String(rfid.uid.uidByte[i], HEX);
  }
  tarjetaLeida.toUpperCase();
  tarjetaLeida.trim();

  Serial.print("Tarjeta detectada (UID): ");
  Serial.println(tarjetaLeida);

  if (bloqueado) {
   
    if (tarjetaLeida != TARJETA_AUTORIZADA) {
      rechazosParaAlarma++;
      Serial.print("-> Rechazo durante el bloqueo (");
      Serial.print(rechazosParaAlarma);
      Serial.println(") - no se concede acceso.");

      if (rechazosParaAlarma >= CONFIRMACIONES_ALARMA && !alarmaConfirmada) {
        alarmaConfirmada = true;
        Serial.println("-> ALARMA CONFIRMADA (4 lecturas consecutivas de rechazo).");
        publicarAlerta("alarma_confirmada");
      }
    } else {
      Serial.println("-> Lectura autorizada recibida durante el bloqueo: se ignora (el bloqueo se cumple por tiempo).");
    }
    rfid.PICC_HaltA();
    return;
  }

  if (tarjetaLeida == TARJETA_AUTORIZADA) {
    Serial.println("-> ACCESO CONCEDIDO");
    intentosFallidos = 0;
    rechazosParaAlarma = 0;

    if (alarmaConfirmada) {
      alarmaConfirmada = false; 
    }

    avisoAccesoActivo = true;
    tFinAvisoAcceso = millis() + 1000;
  } else {
    intentosFallidos++;
    rechazosParaAlarma++;
    Serial.print("-> ACCESO DENEGADO (Intento ");
    Serial.print(intentosFallidos);
    Serial.print("/");
    Serial.print(MAX_INTENTOS_FALLIDOS);
    Serial.println(")");

    avisoRechazoActivo = true;
    tFinAvisoRechazo = millis() + 200; 
   
    if (intentosFallidos >= MAX_INTENTOS_FALLIDOS && !bloqueado) {
      bloqueado = true;
      tInicioBloqueo = millis();
      Serial.println("-> ALERTA: Tres rechazos consecutivos detectados.");
      Serial.print("-> SISTEMA BLOQUEADO TEMPORALMENTE POR ");
      Serial.print(TIEMPO_BLOQUEO_MS / 1000);
      Serial.println(" SEGUNDOS.");
      publicarAlerta("bloqueo_por_rechazos");
    }

   
    if (rechazosParaAlarma >= CONFIRMACIONES_ALARMA) {
      alarmaConfirmada = true;
      Serial.println("-> ALARMA CONFIRMADA (4 lecturas consecutivas de rechazo).");
      publicarAlerta("alarma_confirmada");
    }
  }


  rfid.PICC_HaltA();
}

void actualizarBloqueo() {
  unsigned long ahora = millis();

  if (bloqueado && (ahora - tInicioBloqueo >= TIEMPO_BLOQUEO_MS)) {
 
    if (!enGuardaHisteresis) {
      enGuardaHisteresis = true;
      tFinGuardaHisteresis = ahora + MARGEN_HISTERESIS_MS;
      Serial.println("-> Bloqueo cumplido, esperando margen de histeresis...");
    } else if (ahora >= tFinGuardaHisteresis) {
      bloqueado = false;
      enGuardaHisteresis = false;
      intentosFallidos = 0;
      rechazosParaAlarma = 0;
      Serial.println("-> SISTEMA DESBLOQUEADO Y LISTO.");
    }
  }
}

void actualizarInterfazLocal() {
  unsigned long ahora = millis();

  if (alarmaConfirmada) {
    digitalWrite(LED_PIN, HIGH);
    if (silenciado) {
      digitalWrite(BUZZER_PIN, LOW);
    } else {
      digitalWrite(BUZZER_PIN, (ahora / 250) % 2 == 0 ? HIGH : LOW);
    }
    return;
  }

  if (bloqueado) {
    digitalWrite(LED_PIN, (ahora / 500) % 2 == 0 ? HIGH : LOW);
    digitalWrite(BUZZER_PIN, LOW);
    return;
  }


  if (avisoAccesoActivo) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    if (ahora >= tFinAvisoAcceso) {
      avisoAccesoActivo = false;
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
    }
    return;
  }

 
  if (avisoRechazoActivo) {
    digitalWrite(BUZZER_PIN, HIGH);
    if (ahora >= tFinAvisoRechazo) {
      avisoRechazoActivo = false;
      digitalWrite(BUZZER_PIN, LOW);
    }
    return;
  }

  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

void conectarMqtt() {
  Serial.println("Intentando conectar a MQTT (nube)...");
  String clientId = "esp32-" + CODIGO_PROYECTO;
  if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("MQTT conectado.");
    mqtt.subscribe(TOPIC_COMMAND);
    mqtt.publish(TOPIC_STATUS, "{\"status\":\"online\"}");
  } else {
    Serial.print("Fallo MQTT, rc=");
    Serial.println(mqtt.state());
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String mensaje;
  for (unsigned int i = 0; i < length; i++) mensaje += (char)payload[i];
  mensaje.trim();
  Serial.print("Comando recibido: ");
  Serial.println(mensaje);
  aplicarComando(mensaje);
}

void aplicarComando(String comando) {
  if (comando == "AUTO") {
    modo = "AUTO";
    silenciado = false;
  } else if (comando == "ARMAR") {
    modo = "ARMADO";
  } else if (comando == "SILENCIAR") {
    silenciado = true;
  } else {
    Serial.println("COMANDO_DESCONOCIDO");
    publicarAlerta("COMANDO_DESCONOCIDO");
    return;
  }

  StaticJsonDocument<128> doc;
  doc["device_id"] = CODIGO_PROYECTO;
  doc["comando_aplicado"] = comando;
  doc["modo"] = modo;
  char buffer[128];
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_STATUS, buffer);
}

void publicarTelemetria() {
  secuencia++;

  StaticJsonDocument<256> doc;
  doc["device_id"] = CODIGO_PROYECTO;
  doc["variable"] = "identificador RFID";
  doc["value"] = intentosFallidos;
  doc["unit"] = "rechazos_consecutivos";
  doc["mode"] = modo;
  doc["alarm"] = alarmaConfirmada;
  doc["blocked"] = bloqueado;
  doc["sequence"] = secuencia;

  char buffer[256];
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_TELEMETRY, buffer);

  Serial.print("Telemetria publicada: ");
  Serial.println(buffer);
}

void publicarAlerta(const char* motivo) {
  StaticJsonDocument<192> doc;
  doc["device_id"] = CODIGO_PROYECTO;
  doc["evento"] = motivo;
  doc["mode"] = modo;
  doc["sequence"] = secuencia;

  char buffer[192];
  serializeJson(doc, buffer);

  if (mqtt.connected()) {
    mqtt.publish(TOPIC_ALERT, buffer);
  }
  Serial.print("Alerta: ");
  Serial.println(buffer);
}
