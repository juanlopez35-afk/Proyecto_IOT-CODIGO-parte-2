# Proyecto_IOT-CODIGO

Control de acceso mediante RFID para un módulo de préstamo de materiales.
ESP32 + Wokwi + PlatformIO — Proyecto individual IOT-C05F59AF8A.

## Actividad 2 — Producto mínimo funcional conectado

Se toma el prototipo local de la actividad 1 y se conecta a una plataforma
MQTT en la nube, manteniendo el control local funcionando aunque el WiFi o
el broker no estén disponibles.

- Lectura del RFID cada 2.6 s, no bloqueante (`millis()`).
- Regla original: 3 rechazos consecutivos → bloqueo de 17 s.
- Nuevo: 4 rechazos consecutivos → alarma confirmada; margen de histéresis
  antes de volver a condición normal.
- Publicación MQTT de telemetría cada 21 s.
- Comandos remotos: `AUTO`, `ARMAR`, `SILENCIAR`.

Ver el detalle en [`docs/interpretacion_actividad_2.md`](docs/interpretacion_actividad_2.md)
y las pruebas en [`docs/pruebas_actividad_2.md`](docs/pruebas_actividad_2.md).

### Cómo correr

1. Copiar `include/secrets.example.h` a `include/secrets.h` y completar WiFi
   y broker MQTT.
2. Abrir el proyecto en PlatformIO o en Wokwi (`diagram.json`).
3. Monitor serial a 115200 baudios.
4. Suscribirse a `iot/c05f59af8a/telemetry` y `iot/c05f59af8a/alert` en el
   cliente MQTT usado como plataforma en la nube.
5. Publicar `AUTO`, `ARMAR` o `SILENCIAR` en `iot/c05f59af8a/command` para
   probar el control remoto.