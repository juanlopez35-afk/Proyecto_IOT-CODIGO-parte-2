1. Configuración previa
El archivo include/secrets.h viene listo para trabajar con el broker público broker.hivemq.com (puerto 1883, sin autenticación). Si en la entrega piden cambiar de servidor, solo hay que modificar las credenciales en ese archivo.

2. Manejo del lector RFID en Wokwi
Para probar las tarjetas en el simulador:

Haz clic sobre el componente del lector RFID.

Ingresa el código UID en el campo de texto y presiona Scan.

Tarjeta autorizada: 75 F2 DD 13

Tarjeta no autorizada: AA BB CC DD (o cualquier otro código no registrado)

Nota de tiempo: El código procesa lecturas cada 2.6 segundos. Es importante esperar unos 3 segundos entre cada pasada para que el sistema no omita lecturas.

3. Secuencia de pruebas en el simulador
Primer intento fallido: Pasa la tarjeta AA BB CC DD. En la consola debe aparecer ACCESO DENEGADO (Intento 1/3).

Bloqueo del sistema: Pasa la tarjeta incorrecta 2 veces más (respetando los 3 segundos de pausa). Al acumular 3 rechazos, el monitor indicará SISTEMA BLOQUEADO TEMPORALMENTE POR 17 SEGUNDOS y el LED rojo empezará a parpadear.

Disparo de la alarma: Con el sistema aún bloqueado, pasa la tarjeta incorrecta una 4.ª vez. Deberá mostrar ALARMA CONFIRMADA y el buzzer comenzará a sonar.

Lectura inválida/corrupta: Si se retira la tarjeta a mitad de lectura o se simula un fallo de lectura, la consola mostrará Lectura no valida. Esta lectura no debe sumar al contador de rechazos.

Recuperación por tiempo: Tras esperar unos 18 o 19 segundos sin interactuar, aparecerá SISTEMA DESBLOQUEADO Y LISTO. La alarma continuará activa hasta que se restablezca el sistema.

Restablecimiento y acceso correcto: Pasa la tarjeta autorizada 75 F2 DD 13. El sistema mostrará ACCESO CONCEDIDO, desactivando la alarma y el LED rojo.

4. Prueba de tolerancia a fallos de red
El simulador usa la red Wokwi-GUEST. Si se simula una pérdida de conexión a internet o se deshabilita la red un momento, el sistema mostrará Reintentando conexion WiFi... en la consola.

Durante esta desconexión, la validación de tarjetas debe seguir funcionando localmente sin interrupciones, demostrando la autonomía del dispositivo frente a caídas de red.

5. Verificación de la conexión MQTT (HiveMQ Web)
Abrir la herramienta web: [https://www.hivemq.com/demos/websocket-client/](https://www.hivemq.com/demos/websocket-client/)

Configurar la conexión con el host broker.hivemq.com, puerto 8884 (con SSL marcado) o 8000, y hacer clic en Connect.

En la sección de suscripciones, agregar los siguientes tópicos:

iot/c05f59af8a/telemetry (datos de estado)

iot/c05f59af8a/alert (alertas de bloqueo y alarma)
(O usar iot/c05f59af8a/# para recibir ambos).

Verificar que cada 21 segundos (o al generar eventos) llegue el paquete JSON correspondiente:

JSON
{
  "device_id": "IOT-C05F59AF8A",
  "variable": "identificador RFID",
  "value": 0,
  "unit": "rechazos_consecutivos",
  "mode": "AUTO",
  "alarm": false,
  "blocked": false,
  "sequence": 1
}
6. Envío de comandos remotos
En la sección Publish del cliente HiveMQ, usar el tópico iot/c05f59af8a/command y probar los siguientes mensajes:

SILENCIAR: Apaga el buzzer si la alarma se encuentra activa en ese momento.

AUTO: Cambia el modo de operación de vuelta a automático.

ARMAR: Activa el modo armado.

Texto no reconocido (ej. TEST_ERROR): El sistema imprime COMANDO_DESCONOCIDO en el monitor serial y publica la alerta correspondiente en el tópico iot/c05f59af8a/alert.

7. Corrección de lógica aplicada al proyecto
En la versión inicial del firmware existía un problema con el manejo de estados: al llegar al 3.er rechazo consecutivo, el sistema activaba el bloqueo de 17 segundos e ignoraba el lector RFID durante ese periodo. Esto impedía registrar el 4.º intento fallido y provocaba que la alarma nunca se disparara.

Para resolverlo, se reestructuró la lógica de la máquina de estados para que el sistema continúe procesando las lecturas del RFID durante el periodo de bloqueo. Con este ajuste, un 4.º intento en estado bloqueado incrementa el contador de fallos y activa la alarma del sistema.
