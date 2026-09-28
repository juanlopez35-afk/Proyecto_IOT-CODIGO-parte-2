# Interpretación de la Asignación

**Contexto y uso del proyecto**
El proyecto es un sistema básico de control de acceso con tarjeta. Esto lo podría usar una pequeña oficina o un laboratorio para controlar quién entra a una puerta sin necesidad de usar llaves físicas ni gastar en un sistema complejo.

**Entradas, decisiones y salidas**
* **Entradas:** La tarjeta o llavero RFID que la persona acerca al lector RC522.
* **Decisión:** El ESP32 recibe el código (UID) de la tarjeta y revisa si es exactamente igual al código permitido (`75 F2 DD 13`).
* **Salidas:** Si el código es correcto, prende un LED verde y hace sonar un pitido (buzzer) por 1 segundo. Si la tarjeta no es la correcta, no abre.

**Regla individual y estado seguro**
La regla principal es que solo una tarjeta específica tiene permiso de entrar. El estado seguro es un bloqueo de 17 segundos que se activa si se intentan usar 3 tarjetas incorrectas seguidas. Esto sirve para evitar que un desconocido intente adivinar la clave probando muchas tarjetas rápido.

**Supuestos y limitaciones**
* **Supuestos:** Asumo que el lector siempre tiene buen voltaje para funcionar y que la tarjeta no está rayada o dañada.
* **Limitaciones:** El código de la tarjeta permitida está guardado fijo dentro del programa (no se puede cambiar desde una app) y el sistema no guarda un historial de quién intentó entrar.
