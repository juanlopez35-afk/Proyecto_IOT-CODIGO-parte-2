#ifndef SECRETS_H
#define SECRETS_H

// Copiar este archivo como "secrets.h" (misma carpeta include/) y completar
// con los datos reales. secrets.h NO debe subirse al repositorio.

// --- WiFi (usar la red que provee Wokwi-IoT si se simula en el navegador) ---
#define WIFI_SSID     "Wokwi-GUEST"
#define WIFI_PASSWORD ""

// --- Plataforma MQTT en la nube (definir según el enunciado publicado) ---
// Ejemplo con broker público de pruebas HiveMQ:
#define MQTT_BROKER   "broker.hivemq.com"
#define MQTT_PORT     1883
#define MQTT_USER     "" // dejar vacio si el broker de prueba no pide usuario
#define MQTT_PASSWORD "" // dejar vacio si el broker de prueba no pide clave

#endif
