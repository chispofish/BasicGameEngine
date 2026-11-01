#!/usr/bin/env python3
"""Genera assets/avatar_run.bmp (hoja de sprites) a partir de tools/avatar_fuente.webp.

Requiere Pillow y numpy (solo para regenerar el asset; el juego no los necesita).
  python3 tools/generar_avatar.py [fuente.webp] [salida.bmp]

La imagen fuente tiene UNA sola pose, asi que los 6 fotogramas se derivan de ella con
deformaciones suaves (aplastar / estirar / elevar) que dan sensacion de trote y salto:
  0..3 ciclo de carrera (contacto, vuelo, contacto, vuelo) | 4 subiendo | 5 bajando
Para una animacion de piernas real hay que dibujar mas poses y sustituir 'poses' abajo.
El fondo se rellena con magenta (255,0,255) = color clave que Textura convierte en alfa 0.
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ALTO_SPRITE = 84          # altura del personaje en la hoja (px de pantalla a escala 1)
CELDA_W, CELDA_H = 64, 90 # tamano de cada fotograma (debe coincidir con ConfigJugador)
N = 6
CLAVE = (255, 0, 255)
UMBRAL_ALFA = 128         # alfa binario: sin halos al aplicar el color clave


def preparar(fuente):
    im = Image.open(fuente).convert("RGBA")
    a = np.array(im)
    ys, xs = np.where(a[..., 3] > 10)
    im = im.crop((xs.min(), ys.min(), xs.max() + 1, ys.max() + 1))
    w = round(im.width * ALTO_SPRITE / im.height)
    # Pillow redimensiona RGBA con alfa premultiplicado: los bordes no se ensucian
    return im.resize((w, ALTO_SPRITE), Image.LANCZOS)


def deformar(sprite, ancho_f, alto_f):
    w, h = sprite.size
    return sprite.resize((max(1, round(w * ancho_f)), max(1, round(h * alto_f))), Image.NEAREST)


def montar(sprite):
    # (factor ancho, factor alto, elevacion en px, desplazamiento x en px)
    # El personaje siempre apoya en la base de la celda; el balanceo en x hace que los
    # cuatro fotogramas de carrera sean distintos entre si.
    poses = [
        (1.04, 0.95, 0, -1),   # 0 contacto (aplastado)
        (1.00, 1.00, 5, -2),   # 1 vuelo
        (1.04, 0.95, 0, +1),   # 2 contacto
        (1.00, 1.00, 5, +2),   # 3 vuelo
        (0.97, 1.05, 0, 0),    # 4 salto: subiendo, estirado
        (1.03, 0.96, 0, 0),    # 5 caida: aplastandose hacia el suelo
    ]
    hoja = Image.new("RGBA", (CELDA_W * N, CELDA_H), (0, 0, 0, 0))
    for i, (fw, fh, elev, dx) in enumerate(poses):
        s = deformar(sprite, fw, fh)
        x = i * CELDA_W + (CELDA_W - s.width) // 2 + dx
        y = CELDA_H - s.height - elev
        assert s.width <= CELDA_W and y >= 0, "el sprite no cabe en la celda"
        hoja.alpha_composite(s, (x, y))
    return hoja


def a_bmp(hoja, salida):
    a = np.array(hoja)
    opaco = a[..., 3] >= UMBRAL_ALFA
    rgb = a[..., :3].copy()
    # Ningun pixel del personaje puede coincidir con el color clave
    choque = opaco & (rgb == np.array(CLAVE)).all(-1)
    rgb[choque] = (254, 0, 255)
    rgb[~opaco] = CLAVE
    Image.fromarray(rgb, "RGB").save(salida, format="BMP")   # BMP de 24 bits, sin compresion


def main():
    aqui = Path(__file__).resolve().parent
    fuente = Path(sys.argv[1]) if len(sys.argv) > 1 else aqui / "avatar_fuente.webp"
    salida = Path(sys.argv[2]) if len(sys.argv) > 2 else aqui.parent / "assets" / "avatar_run.bmp"
    sprite = preparar(fuente)
    a_bmp(montar(sprite), salida)
    print(f"{salida}: {CELDA_W * N}x{CELDA_H}, {N} fotogramas de {CELDA_W}x{CELDA_H} (personaje {sprite.width}x{sprite.height})")


if __name__ == "__main__":
    main()
