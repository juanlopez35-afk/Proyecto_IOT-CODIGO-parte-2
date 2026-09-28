#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "secrets.h" // copiar secrets.example.h a secrets.h y completar credenciales

// --- Asignación de Pines (igual que en la actividad 1) ---
#define SS_PIN       21
#define RST_PIN      22
#define LED_PIN      2
#define BUZZER_PIN   5

// --- Parámetros Individuales Asignados (sección 2.3 de los insumos) ---
const String CODIGO_PROYECTO = "IOT-C05F59AF8A";
const String TARJETA_AUTORIZADA = "75 F2 DD 13";
const unsigned long TIEMPO_BLOQUEO_MS       = 17000;  // 17 s de bloqueo
const unsigned long INTERVALO_MUESTREO_MS   = 2600;   // lectura del sensor
const unsigned long PERIODO_PUBLICACION_MS  = 21000;  // telemetría MQTT
const unsigned long INTERVALO_RECONEXION_MS = 4000;   // reintento wifi / mqtt
const int MAX_INTENTOS_FALLIDOS   = 3; // rechazos consecutivos para bloquear (regla asignada)
const int CONFIRMACIONES_ALARMA   = 4; // lecturas consecutivas para confirmar alarma
const unsigned long MARGEN_HISTERESIS_MS = 1000; // margen de 10 (x100ms) antes de volver a normal

// --- Tópicos MQTT (plataforma en la nube) ---
const char* TOPIC_TELEMETRY = "iot/c05f59af8a/telemetry";
const char* TOPIC_STATUS    = "iot/c05f59af8a/status";
const char* TOPIC_COMMAND   = "iot/c05f59af8a/command";
const char* TOPIC_ALERT     = "iot/c05f59af8a/alert";

// --- Objetos ---
MFRC522 rfid(SS_PIN, RST_PIN);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// --- Variables de estado (reemplazan los delay() de la actividad 1) ---
int intentosFallidos = 0;      // rechazos consecutivos (regla del bloqueo)
int rechazosParaAlarma = 0;    // rechazos consecutivos (confirmación de alarma)
bool bloqueado = false;
bool alarmaConfirmada = false;
bool enGuardaHisteresis = false;
String modo = "AUTO";          // AUTO o ARMADO (comando remoto)
bool silenciado = false;       // comando SILENCIAR
unsigned long secuencia = 0;

unsigned long tUltimaMuestra = 0;
unsigned long tInicioBloqueo = 0;
unsigned long tFinGuardaHisteresis = 0;
unsigned long tUltimaPublicacion = 0;
unsigned long tUltimoIntentoConexion = 0;

// temporizadores no bloqueantes para los avisos cortos de LED/buzzer
bool avisoAccesoActivo = false;
unsigned long tFinAvisoAcceso = 0;
bool avisoRechazoActivo = false;
unsigned long tFinAvisoRechazo = 0;

// --- Prototipos ---
void leerRFID();
void actualizarBloqueo();
void actualizarInterfazLocal();
void conectarMqtt();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void aplicarComando(String comando);
void publicarTelemetria();
void publicarAlerta(const char* motivo);

void setup() {
  // Inicialización serial a 115200 baudios
  Serial.begin(115200);

  // Configuración de pines de entrada y salida
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Estado seguro inicial (actuadores apagados)
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Inicializar comunicación SPI y módulo RFID
  SPI.begin();
  rfid.PCD_Init();

  // Conexión WiFi y configuración del cliente MQTT (plataforma en la nube)
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  // Mensaje de arranque obligatorio con el código asignado
  Serial.println("==========================================");
  Serial.print("Sistema RFID conectado - Proyecto: ");
  Serial.println(CODIGO_PROYECTO);
  Serial.print("UID Autorizado: ");
  Serial.println(TARJETA_AUTORIZADA);
  Serial.println("==========================================");
}

void loop() {
  unsigned long ahora = millis();

  // Conectividad no bloqueante: el control local sigue funcionando aunque
  // el WiFi o el broker no estén disponibles
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

  // Lectura del sensor cada INTERVALO_MUESTREO_MS, sin usar delay()
  if (ahora - tUltimaMuestra >= INTERVALO_MUESTREO_MS) {
    tUltimaMuestra = ahora;
    leerRFID();
  }

  // Gestión del bloqueo temporal y de la histéresis
  actualizarBloqueo();

  // LED / buzzer sin detener el ciclo principal
  actualizarInterfazLocal();

  // Publicación de telemetría cada PERIODO_PUBLICACION_MS
  if (mqtt.connected() && (ahora - tUltimaPublicacion >= PERIODO_PUBLICACION_MS)) {
    tUltimaPublicacion = ahora;
    publicarTelemetria();
  }
}

void leerRFID() {
  // Verificar presencia de tarjeta y leer su serie
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return; // no hay tarjeta nueva, no se evalúa nada
  }

  // Validación del dato antes de usarlo (tamaño de UID admisible)
  if (rfid.uid.size < 4 || rfid.uid.size > 7) {
    Serial.println("-> Lectura no valida (tamano de UID fuera de rango).");
    rfid.PICC_HaltA();
    return;
  }

  // Formatear el UID a texto hexadecimal en mayúsculas (igual que actividad 1)
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
    // En bloqueo no se concede acceso ni se reinicia el temporizador,
    // pero los rechazos se SIGUEN contando: si no fuera asi, la alarma
    // (4 rechazos) nunca se alcanzaria porque el bloqueo ya se activo en
    // el rechazo numero 3 y las lecturas posteriores quedarian mudas.
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
      alarmaConfirmada = false; // una lectura autorizada limpia la alarma
    }

    avisoAccesoActivo = true;
    tFinAvisoAcceso = millis() + 1000; // mismo tiempo que en la actividad 1
  } else {
    intentosFallidos++;
    rechazosParaAlarma++;
    Serial.print("-> ACCESO DENEGADO (Intento ");
    Serial.print(intentosFallidos);
    Serial.print("/");
    Serial.print(MAX_INTENTOS_FALLIDOS);
    Serial.println(")");

    avisoRechazoActivo = true;
    tFinAvisoRechazo = millis() + 200; // mismo tiempo que en la actividad 1

    // Regla asignada: bloquear temporalmente tras 3 rechazos consecutivos
    if (intentosFallidos >= MAX_INTENTOS_FALLIDOS && !bloqueado) {
      bloqueado = true;
      tInicioBloqueo = millis();
      Serial.println("-> ALERTA: Tres rechazos consecutivos detectados.");
      Serial.print("-> SISTEMA BLOQUEADO TEMPORALMENTE POR ");
      Serial.print(TIEMPO_BLOQUEO_MS / 1000);
      Serial.println(" SEGUNDOS.");
      publicarAlerta("bloqueo_por_rechazos");
    }

    // Confirmación de alarma: 4 lecturas consecutivas de rechazo
    if (rechazosParaAlarma >= CONFIRMACIONES_ALARMA) {
      alarmaConfirmada = true;
      Serial.println("-> ALARMA CONFIRMADA (4 lecturas consecutivas de rechazo).");
      publicarAlerta("alarma_confirmada");
    }
  }

  // Detener comunicación con la tarjeta actual
  rfid.PICC_HaltA();
}

void actualizarBloqueo() {
  unsigned long ahora = millis();

  if (bloqueado && (ahora - tInicioBloqueo >= TIEMPO_BLOQUEO_MS)) {
    // Termina el tiempo de bloqueo, pero se aplica el margen de histéresis
    // antes de declarar el sistema totalmente normal otra vez.
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

  // Estado seguro: apagado, excepto ante alarma local confirmada
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
    digitalWrite(LED_PIN, (ahora / 500) % 2 == 0 ? HIGH : LOW); // parpadeo
    digitalWrite(BUZZER_PIN, LOW);
    return;
  }

  // Aviso corto de acceso concedido (1 s, igual que la actividad 1)
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

  // Pitido corto de rechazo (200 ms, igual que la actividad 1)
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
