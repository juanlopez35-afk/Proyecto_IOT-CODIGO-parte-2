# Script de verificacion (no es parte del firmware): simula la misma
# logica de main.cpp para revisar que las reglas de bloqueo, confirmacion
# de alarma e histeresis encadenan bien antes de probar en Wokwi.
# Correr con: python3 docs/verificacion_logica.py

# Simulacion en Python de la MISMA logica del main.cpp (no es Wokwi real,
# es para verificar que las reglas quedan bien encadenadas antes de probar
# en la simulacion real).

TIEMPO_BLOQUEO_MS = 17000
MAX_INTENTOS_FALLIDOS = 3
CONFIRMACIONES_ALARMA = 4
MARGEN_HISTERESIS_MS = 1000
TARJETA_AUTORIZADA = "75 F2 DD 13"

class Sistema:
    def __init__(self):
        self.intentosFallidos = 0
        self.rechazosParaAlarma = 0
        self.bloqueado = False
        self.alarmaConfirmada = False
        self.enGuardaHisteresis = False
        self.t = 0  # ms simulados
        self.tInicioBloqueo = 0
        self.tFinGuardaHisteresis = 0
        self.log = []

    def avanzar_tiempo(self, ms):
        self.t += ms
        self.actualizar_bloqueo()

    def actualizar_bloqueo(self):
        if self.bloqueado and (self.t - self.tInicioBloqueo >= TIEMPO_BLOQUEO_MS):
            if not self.enGuardaHisteresis:
                self.enGuardaHisteresis = True
                self.tFinGuardaHisteresis = self.t + MARGEN_HISTERESIS_MS
                self.log.append(f"[t={self.t}] Bloqueo cumplido, esperando histeresis...")
            elif self.t >= self.tFinGuardaHisteresis:
                self.bloqueado = False
                self.enGuardaHisteresis = False
                self.intentosFallidos = 0
                self.rechazosParaAlarma = 0
                self.log.append(f"[t={self.t}] SISTEMA DESBLOQUEADO Y LISTO.")

    def leer(self, uid, valido=True):
        if not valido:
            self.log.append(f"[t={self.t}] Lectura no valida (descartada).")
            return
        if self.bloqueado:
            if uid != TARJETA_AUTORIZADA:
                self.rechazosParaAlarma += 1
                self.log.append(f"[t={self.t}] Rechazo durante bloqueo ({self.rechazosParaAlarma})")
                if self.rechazosParaAlarma >= CONFIRMACIONES_ALARMA and not self.alarmaConfirmada:
                    self.alarmaConfirmada = True
                    self.log.append(f"[t={self.t}] >> ALARMA CONFIRMADA (durante bloqueo)")
            else:
                self.log.append(f"[t={self.t}] Autorizada ignorada (en bloqueo).")
            return
        if uid == TARJETA_AUTORIZADA:
            self.intentosFallidos = 0
            self.rechazosParaAlarma = 0
            if self.alarmaConfirmada:
                self.alarmaConfirmada = False
                self.log.append(f"[t={self.t}] Alarma limpiada por acceso autorizado.")
            self.log.append(f"[t={self.t}] ACCESO CONCEDIDO")
        else:
            self.intentosFallidos += 1
            self.rechazosParaAlarma += 1
            self.log.append(f"[t={self.t}] ACCESO DENEGADO (intento {self.intentosFallidos}/{MAX_INTENTOS_FALLIDOS})")
            if self.intentosFallidos >= MAX_INTENTOS_FALLIDOS and not self.bloqueado:
                self.bloqueado = True
                self.tInicioBloqueo = self.t
                self.log.append(f"[t={self.t}] >> BLOQUEO TEMPORAL ACTIVADO (17s)")
            if self.rechazosParaAlarma >= CONFIRMACIONES_ALARMA:
                self.alarmaConfirmada = True
                self.log.append(f"[t={self.t}] >> ALARMA CONFIRMADA")

def escenario(nombre, pasos):
    print("="*70)
    print("ESCENARIO:", nombre)
    print("="*70)
    s = Sistema()
    for paso in pasos:
        if paso[0] == "leer":
            s.leer(paso[1], valido=paso[2] if len(paso) > 2 else True)
        elif paso[0] == "esperar":
            s.avanzar_tiempo(paso[1])
    for linea in s.log:
        print(linea)
    print(f"Estado final -> bloqueado={s.bloqueado} alarma={s.alarmaConfirmada} intentosFallidos={s.intentosFallidos}")
    print()

# Condicion A: una credencial no autorizada, una sola vez
escenario("A - un rechazo aislado", [
    ("leer", "AA BB CC DD"),
])

# Condicion B: tres rechazos consecutivos -> bloqueo
escenario("B - tres rechazos consecutivos", [
    ("leer", "AA BB CC DD"),
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),
])

# Dato invalido
escenario("Dato invalido", [
    ("leer", "", False),
])

# Cuatro rechazos consecutivos -> bloqueo + alarma
escenario("4 rechazos seguidos -> bloqueo y alarma", [
    ("leer", "AA BB CC DD"),
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),
])

# Bloqueo completo + histeresis + vuelta a normal
escenario("Bloqueo completo -> espera 17s + histeresis -> normal", [
    ("leer", "AA BB CC DD"),
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),
    ("esperar", 17000),   # se cumple el bloqueo
    ("esperar", 1000),    # margen de histeresis
])

# Acceso autorizado limpia la alarma
escenario("Alarma confirmada y luego acceso autorizado la limpia", [
    ("leer", "AA BB CC DD"),
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),
    ("esperar", 20000),
    ("leer", TARJETA_AUTORIZADA),
])

print("### Prueba corregida: 4 rechazos -> bloqueo+alarma -> se cumple el tiempo -> tarjeta autorizada limpia la alarma ###")
escenario("4 rechazos, bloqueo+alarma, pasa el tiempo, autorizada limpia", [
    ("leer", "AA BB CC DD"),
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),   # 3er rechazo -> bloqueo
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),   # 4to rechazo -> alarma (aunque este en bloqueo no se ignora, ya se conto antes de bloquear? revisar)
    ("esperar", 18000),        # pasan los 17s + histeresis
    ("leer", TARJETA_AUTORIZADA),
])

print("### Ciclo completo: bloqueo+alarma -> tiempo total -> pasa el margen -> SI se desbloquea -> autorizada limpia ###")
escenario("Ciclo completo correcto", [
    ("leer", "AA BB CC DD"),
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),   # bloqueo
    ("esperar", 2600),
    ("leer", "AA BB CC DD"),   # alarma confirmada
    ("esperar", 17000),       # se cumple el bloqueo -> entra a histeresis
    ("esperar", 1500),        # pasa el margen -> se desbloquea de verdad
    ("leer", TARJETA_AUTORIZADA),  # ahora si se procesa y limpia la alarma
])
