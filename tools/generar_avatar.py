#!/usr/bin/env python3
"""Genera assets/avatar_run.bmp: hoja de sprites 6x(16x24) del avatar (solo biblioteca estandar).

Fotogramas: 0-3 ciclo de carrera, 4 salto (subiendo), 5 caida. Fondo magenta = transparente
(el cargador de Textura lo convierte en alfa 0). Uso: python3 tools/generar_avatar.py [salida.bmp]
"""
import struct
import sys

W, H, N = 16, 24, 6
CLAVE = (255, 0, 255)
PIEL, PELO, CAMISA, PANTALON, ZAPATO, OJO = (240, 190, 150), (70, 45, 30), (60, 120, 220), (45, 50, 100), (235, 235, 240), (20, 20, 30)


def frame(i):
    px = [[CLAVE] * W for _ in range(H)]

    def r(x0, y0, x1, y1, c):          # rectangulo inclusivo
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                px[y][x] = c

    # Cabeza, pelo y ojo (mira a la derecha)
    r(5, 1, 10, 6, PIEL); r(5, 1, 10, 2, PELO); r(5, 3, 6, 4, PELO); r(9, 4, 9, 4, OJO)
    # Torso
    r(5, 7, 10, 14, CAMISA)

    if i in (0, 2):                    # zancada amplia (2 es el espejo de 0)
        back, front = (4, 6), (9, 11)
        if i == 2:
            back, front = front, back
        r(back[0], 15, back[1], 21, PANTALON);  r(back[0] - 1, 22, back[1], 23, ZAPATO)
        r(front[0], 15, front[1], 20, PANTALON); r(front[0], 21, front[1] + 1, 22, ZAPATO)
        arm_back, arm_front = (3, 4), (11, 12)
        r(arm_back[0], 8, arm_back[1], 12, PIEL if i == 0 else CAMISA)
        r(arm_front[0], 8, arm_front[1], 12, CAMISA if i == 0 else PIEL)
    elif i in (1, 3):                  # piernas juntas, una elevada
        r(6, 15, 8, 23, PANTALON); r(6, 22, 9, 23, ZAPATO)
        r(8, 15, 10, 19, PANTALON); r(8, 20, 11, 21, ZAPATO)
        r(7, 9, 8, 13, PIEL)
    elif i == 4:                       # salto: rodillas recogidas, brazos arriba
        r(5, 15, 9, 18, PANTALON); r(9, 17, 10, 19, PANTALON); r(5, 19, 8, 20, ZAPATO); r(9, 20, 11, 21, ZAPATO)
        r(3, 3, 4, 8, PIEL); r(11, 3, 12, 8, PIEL)
    else:                              # caida: piernas estiradas, brazos abiertos
        r(4, 15, 6, 22, PANTALON); r(3, 23, 6, 23, ZAPATO)
        r(9, 15, 11, 22, PANTALON); r(9, 23, 12, 23, ZAPATO)
        r(2, 8, 4, 10, PIEL); r(11, 8, 13, 10, PIEL)
    return px


def main(ruta):
    ancho = W * N
    filas = [[frame(f)[y][x] for f in range(N) for x in range(W)] for y in range(H)]
    datos = b"".join(b"".join(bytes((b, g, rr)) for rr, g, b in fila) for fila in reversed(filas))  # BGR, de abajo arriba
    cabecera = b"BM" + struct.pack("<IHHI", 54 + len(datos), 0, 0, 54)
    info = struct.pack("<IiiHHIIiiII", 40, ancho, H, 1, 24, 0, len(datos), 2835, 2835, 0, 0)
    with open(ruta, "wb") as f:
        f.write(cabecera + info + datos)
    print(f"{ruta}: {ancho}x{H}, {N} fotogramas de {W}x{H}")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "assets/avatar_run.bmp")
