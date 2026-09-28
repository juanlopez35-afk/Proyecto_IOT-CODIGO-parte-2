#Interpretación de la asignación — Actividad 2
Comportamiento local del sistema
En la parte física, el proyecto funciona básicamente igual que en la primera actividad. El ESP32 lee el lector RFID cada 2.6 segundos, verifica que el código de la tarjeta esté completo y lo compara contra la tarjeta autorizada (75 F2 DD 13). La gran diferencia es que quité todos los delay() y los cambié por temporizadores con millis(), logrando que la placa atienda la red WiFi y las publicaciones MQTT en segundo plano sin pausar el sensor.

Bloqueo, confirmaciones e histéresis
El bloqueo temporal se activa al acumular 3 tarjetas rechazadas seguidas y dura 17 segundos. Para evitar que el sistema esté entrando y saliendo del bloqueo continuamente si alguien intenta pasar la tarjeta justo en el límite, se agregó un pequeño margen de tiempo extra (histéresis) antes de volver a la normalidad.

Por otro lado, para que la alarma fuerte se dispare se requieren 4 rechazos consecutivos en lugar de 3. Esto funciona como un filtro de confirmación para asegurar que una lectura errónea o un fallo puntual del sensor no disparen la alarma de forma no deseada.

Envío de datos a la nube
Cada 21 segundos el ESP32 reporta su estado enviando un paquete de información por MQTT hacia la nube. En ese envío van la identificación del equipo, el modo actual, si hay bloqueo o alarma activos y el número de secuencia. Desde la nube, el sistema también puede recibir y procesar los comandos AUTO, ARMAR y SILENCIAR.

Dificultades presentadas

Cambiar de delay() a millis(): Coordinar varios tiempos simultáneamente (la lectura del RFID, los temporizadores del bloqueo y los envíos a la nube) usando millis() fue un reto, ya que tocaba estructurar todo sin detener el procesador.

El error en la lógica de la alarma: Al principio cometí el detalle de hacer que el bloqueo suspendiera la lectura de las tarjetas al llegar al 3.er rechazo. Por eso mismo, el sistema jamás registraba el 4.º intento fallido y la alarma no se activaba. Lo corregí modificando el flujo para que, aun estando bloqueado, el lector continúe contando los fallos para poder disparar la alarma, sin dar acceso ni reiniciar el tiempo de bloqueo.
