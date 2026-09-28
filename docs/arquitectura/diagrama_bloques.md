[ Tag RFID / MFRC522 ] --(SPI: GPIO 18, 19, 21, 23)--> [ ESP32 DevKit ]
                                                             |
                                           +-----------------+-----------------+
                                           | (GPIO 2 - Salida)                 | (GPIO 5 - Salida)
                                           v                                   v
                                    [ LED Verde ]                      [ Buzzer Sonoro ]

--- Actividad 2: conectividad ---

                       [ ESP32 DevKit ]
                              |
                          (WiFi STA)
                              v
                     [ Router / Wokwi-GUEST ]
                              |
                          (MQTT / TCP)
                              v
                  [ Broker en la nube (secrets.h) ]
                    |        |        |        |
              telemetry   status    alert   command
             (cada 21 s) (conexión) (eventos) (AUTO/ARMAR/SILENCIAR)
