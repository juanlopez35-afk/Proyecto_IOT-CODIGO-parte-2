# Guía de prueba — Actividad 2

## 1. Antes de correr la simulación
`include/secrets.h` ya viene incluido en este proyecto, apuntando al broker
público de pruebas `broker.hivemq.com` (puerto 1883, sin usuario ni clave).
Si el enunciado de tu curso pide un broker distinto, edita ese archivo.

## 2. Cómo "tocar" tarjetas en Wokwi
En la simulación, haz clic sobre el módulo del lector RFID. Aparece un campo
de texto donde puedes escribir un UID manualmente y presionar "Scan"
(o el botón equivalente del componente). Usa:

- **Tarjeta autorizada:** `75 F2 DD 13`
- **Tarjeta no autorizada (para pruebas de rechazo):** cualquier otra, por
  ejemplo `AA BB CC DD`

Como el muestreo es cada 2.6 s, espera unos 3 segundos entre cada "toque"
para que el sistema alcance a procesar cada lectura por separado.

## 3. Secuencia de pruebas sugerida (en orden)

1. **Condición A:** toca `AA BB CC DD` una sola vez → debe salir
   "ACCESO DENEGADO (Intento 1/3)" y nada más.
2. **Condición B:** toca `AA BB CC DD` dos veces más seguidas (esperando
   ~3 s entre cada una) → al 3er rechazo debe salir "SISTEMA BLOQUEADO
   TEMPORALMENTE POR 17 SEGUNDOS" y el LED debe empezar a parpadear.
3. **Confirmación de alarma:** mientras sigue bloqueado, toca `AA BB CC DD`
   una vez más → debe salir "ALARMA CONFIRMADA" y el buzzer debe sonar
   intermitente (o quedar en silencio si antes mandaste el comando
   `SILENCIAR`).
4. **Dato inválido:** retira la tarjeta del lector a medio leer, o usa el
   botón de "tarjeta corrupta" si el simulador lo trae — debe salir
   "Lectura no valida" y no debe contar como rechazo.
5. **Espera y recuperación:** deja pasar unos 18-19 segundos sin tocar nada
   → debe salir "SISTEMA DESBLOQUEADO Y LISTO", pero la alarma sigue
   encendida hasta el siguiente paso.
6. **Limpiar la alarma:** toca la tarjeta autorizada `75 F2 DD 13` → debe
   salir "ACCESO CONCEDIDO" y la alarma y el LED se apagan (estado seguro).

## 4. Probar la interrupción de red (20 s)
Wokwi simula la red WiFi automáticamente (red `Wokwi-GUEST`). Para simular
una caída de 20 segundos, puedes pausar la simulación un momento o, si tu
entorno lo permite, desconectar momentáneamente el módulo de red. Verifica
en el monitor serial que sigan apareciendo los mensajes de "ACCESO
CONCEDIDO/DENEGADO" con normalidad aunque diga "Reintentando conexion
WiFi..." — eso demuestra que el control local no depende de la nube.

## 5. Cliente MQTT de prueba (HiveMQ)
1. Abre en el navegador: `https://www.hivemq.com/demos/websocket-client/`
2. En "Host" deja `broker.hivemq.com`, puerto `8000`, path `/mqtt`, y
   presiona **Connect**.
3. En la sección de suscripción, escribe el tópico:
   `iot/c05f59af8a/telemetry` y presiona **Subscribe**.
4. Suscríbete también a `iot/c05f59af8a/alert` para ver las alertas de
   bloqueo y alarma.
5. Espera hasta 21 segundos (o provoca un evento) y deberías ver llegar un
   mensaje JSON como:
   ```json
   {"device_id":"IOT-C05F59AF8A","variable":"identificador RFID","value":0,"unit":"rechazos_consecutivos","mode":"AUTO","alarm":false,"blocked":false,"sequence":1}
   ```

## 6. Probar los comandos remotos
En el mismo cliente HiveMQ, en la sección de publicación, escribe el tópico
`iot/c05f59af8a/command` y en el mensaje pon (uno a la vez, presionando
**Publish** cada vez):

- `AUTO` → el monitor serial debe reflejar el modo AUTO.
- `ARMAR` → cambia el modo a ARMADO.
- `SILENCIAR` → si hay alarma activa, el buzzer se apaga pero el LED sigue.
- `COMANDO_DESCONOCIDO` (o cualquier texto que no sea uno de los tres
  anteriores) → debe salir "COMANDO_DESCONOCIDO" en el monitor serial y
  llegar una alerta con ese mismo texto en `iot/c05f59af8a/alert`.

## 7. Para el video
Con este mismo cliente HiveMQ abierto al lado de la simulación de Wokwi,
ya tienes lo que pide la entrega: una publicación MQTT recibida en vivo.
Para la parte de "algo que aprendiste o corregiste" puedes usar el bug real
que se corrigió en este proyecto: al principio, el bloqueo (activado al 3er
rechazo) cortaba la lectura antes de que se pudiera contar un 4to rechazo,
así que la alarma de "4 lecturas consecutivas" nunca se activaba. Se
corrigió para que los rechazos sigan contando para la alarma incluso
mientras el sistema está bloqueado.
