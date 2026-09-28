# Proyecto IoT - Control de Acceso RFID para Módulo de Préstamos

**Código del proyecto:** IOT-C05F59AF8A  
**Placa usada:** ESP32 DevKit v1  
**Entorno:** VS Code + PlatformIO + Wokwi  

Este repositorio contiene la **Actividad 2** de la materia. Es un prototipo de control de acceso pensado para un módulo de préstamo de materiales. Lee credenciales con un lector RFID MFRC522 y envía telemetría a la nube por MQTT. Si se cae el Wi-Fi, el circuito sigue funcionando de forma local sin quedarse pegado.

---

## ¿Cómo funciona el proyecto?

* **Muestreo del sensor:** Lee el RFID cada **2.6 segundos** (2600 ms) usando `millis()` para no congelar el código con `delay()`.
* **Tarjeta autorizada:** El UID válido configurado en el sistema es `75 F2 DD 13` (en la simulación de Wokwi usé la Tarjeta Azul para este UID).
* **Bloqueo por fallos:** Si se detectan **3 lecturas no autorizadas seguidas**, el sistema entra en bloqueo temporal durante **17 segundos**.
* **Alarma e Histéresis:** Tras **4 lecturas erróneas consecutivas** se confirma la alarma. Para salir de la alarma y volver a estado normal, las lecturas deben superar un margen de histéresis de **10 unidades**.
* **Envío a la nube:** Publica la telemetría cada **21 segundos** por MQTT. Si la red se desconecta, intenta reconectar cada **4 segundos** en segundo plano.
* **Comandos remotos:** Permite recibir los comandos `AUTO`, `ARMAR` y `SILENCIAR`.

---

## Conexión de componentes (Wokwi / VS Code)

Para que Wokwi reconozca bien la placa `wokwi-esp32-devkit-v1`, los pines quedaron conectados de la siguiente forma:

* **Lector MFRC522 (SPI):**
  * SDA -> `D21`
  * SCK -> `D18`
  * MOSI -> `D23`
  * MISO -> `D19`
  * RST -> `D22`
  * 3V3 -> `3V3` | GND -> `GND`
* **LED de estado:** Pin `D2`
* **Buzzer:** Pin `D5`

---

## Estructura de carpetas

```text
proyecto_IOT-CODIGO/
├── include/
│   └── secrets.example.h
├── src/
│   └── main.cpp
├── docs/
│   ├── interpretacion_actividad_2.md
│   └── pruebas_actividad_2.md
├── platformio.ini
├── diagram.json
├── wokwi.toml
├── .gitignore
└── README.md
