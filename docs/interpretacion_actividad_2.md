# Interpretación de la Asignación — Actividad 2

**Comportamiento local**
El sistema sigue funcionando igual que en la actividad 1: el ESP32 lee el lector RFID cada 2.6 segundos, valida que el UID tenga un tamaño correcto antes de usarlo, y compara el código contra la tarjeta autorizada (`75 F2 DD 13`). Ahora esa lectura ya no usa `delay()`, sino temporizadores con `millis()`, para que el WiFi y el MQTT puedan atenderse sin detener el lector.

**Cuándo se activa y cuándo vuelve a la normalidad**
El bloqueo se activa cuando hay 3 tarjetas rechazadas seguidas (regla original) y dura 17 segundos. Después de esos 17 segundos el sistema espera un margen corto extra antes de dar por normalizado todo, para que no quede entrando y saliendo del bloqueo justo en el límite.

**Confirmaciones e histéresis**
Se necesitan 4 lecturas de rechazo seguidas (no solo 3) para que se confirme una alarma más fuerte, así una lectura rara del lector no dispara la alarma sola. El margen de histéresis es ese tiempo de espera extra después del bloqueo antes de volver a "normal".

**Qué se envía a la nube**
Cada 21 segundos se publica por MQTT un mensaje con el id del dispositivo, el modo, si hay alarma o bloqueo, y un número de secuencia. También se pueden mandar los comandos AUTO, ARMAR y SILENCIAR desde la nube.

**Dos dificultades**
Cambiar los `delay()` por `millis()` sin dañar la lógica que ya tenía costó, porque hay que llevar varios tiempos a la vez. La segunda fue un error que no vi al principio: como el bloqueo se activa en el 3er rechazo, mi código cortaba la lectura ahí mismo y nunca llegaba a contar un 4to rechazo, así que la alarma de "4 lecturas consecutivas" nunca se podía activar. Lo corregí para que, aunque el sistema esté bloqueado, los rechazos sigan contando para la alarma (solo que ya no se vuelve a activar el bloqueo ni se concede acceso).
