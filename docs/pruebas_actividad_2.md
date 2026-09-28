# Pruebas — Actividad 2

Registro de las pruebas hechas sobre el prototipo conectado, según las condiciones
de referencia de la ficha de insumos (2.5). Completar cada fila con lo observado
al correr la simulación en Wokwi y el cliente MQTT de prueba.

| # | Condición de referencia | Cómo se provocó | Resultado esperado | Resultado observado | Evidencia |
|---|---|---|---|---|---|
| 1 | A — una credencial no autorizada una sola vez | Acercar una tarjeta distinta a la autorizada, una vez | Acceso rechazado, sin bloqueo | | |
| 2 | B — tres credenciales no autorizadas consecutivas | Acercar 3 tarjetas distintas a la autorizada, seguidas, esperando ~3 s entre cada una | Bloqueo temporal de 17 s activado en el 3er rechazo | | |
| 2b | Confirmación de alarma (4 rechazos) | Acercar una 4ta tarjeta no autorizada mientras el sistema sigue bloqueado | Se confirma la alarma (LED fijo + buzzer intermitente), sin desactivar el bloqueo | | |
| 3 | Dato no válido | Simular un UID fuera de rango | Lectura descartada, no cuenta como rechazo | | |
| 4 | Interrupción de red de 20 s | Apagar el WiFi del simulador 20 s | El RFID y el bloqueo local siguen funcionando igual | | |
| 5 | Reconexión (4 s) | Reactivar el WiFi | El dispositivo vuelve a conectar a MQTT sin reiniciar | | |
| 6 | Comando AUTO | Publicar "AUTO" en el tópico de comandos | Modo vuelve a AUTO, se apaga el silenciado | | |
| 7 | Comando ARMAR | Publicar "ARMAR" | Modo cambia a ARMADO | | |
| 8 | Comando SILENCIAR | Publicar "SILENCIAR" durante alarma | Buzzer se apaga, LED sigue indicando alarma | | |
| 9 | Comando no reconocido | Publicar "COMANDO_DESCONOCIDO" | Se publica alerta con ese texto, no cambia el modo | | |
| 10 | Estado seguro | Sin alarma confirmada | LED y buzzer apagados | | |

## Publicación MQTT recibida

*(Pegar aquí una captura del cliente/servicio en la nube mostrando al menos un
mensaje recibido en `iot/c05f59af8a/telemetry`.)*
