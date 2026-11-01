# BasicGameEngine

Motor básico de juegos 2D en C++17 sobre **SDL2**, que se ejecuta en escritorio (nativo) y
en el navegador (WebAssembly con Emscripten, publicado en GitHub Pages).
Migrado desde la versión original basada en Win32.

## Módulos

| Módulo | Contenido | QA |
|---|---|---|
| 1. Plataforma y ventana | `src/Ventana.*` — SDL2 RAII, foco, cierre, ESC, teclado/ratón | `tests/test_motor.cpp` (1.1–1.3) |
| 2. Núcleo y bucle de paso fijo | `src/NucleoMotor.*` — 60 Hz lógicos, `emscripten_set_main_loop_arg` | `tests/test_motor.cpp` (2.1) |
| 3. Renderizado 2D | `src/Dibujante.*` — primitivas, texto bitmap 5x7; el motor limpia el fondo en cada frame | `tests/test_graficos.cpp` (3.1, 3.2) |
| 4. Build y CI/CD | `CMakeLists.txt`, `web/minimal_shell.html`, `.github/workflows/deploy.yml` | CI (4.1, 4.2) |

## Juego Runner

Juego "runner" horizontal construido sobre el motor: esquiva los bugs rojos saltando con
**ESPACIO / ↑** (también reinicia tras GAME OVER); **M** activa o silencia el sonido. La puntuación es la distancia recorrida y la
velocidad aumenta con el tiempo. En la web es el `index.html` publicado.

| Módulo | Contenido | QA |
|---|---|---|
| R1. Texturas | `src/Textura.*` — carga de BMP con SDL2 (sin SDL_image), color clave magenta / alfa → `SDL_Texture` RAII | `tests/test_jugador.cpp` (1.1, 1.2) |
| R2. Jugador | `src/Jugador.*` — estados Corriendo/Saltando/Cayendo, gravedad, suelo, animación por tics | `tests/test_jugador.cpp` (2.1, 2.2) |
| R3.1/3.2. Obstáculos y colisión AABB | `src/Obstaculos.*`, `src/Partida.*` — generación aleatoria (con semilla), destrucción fuera de pantalla, GAME OVER | `tests/test_partida.cpp` (3.1, 3.2) |
| R3.3. HUD | `src/Hud.*` — puntos, mejor puntuación, velocidad y mensajes con `Dibujante::Texto` | `tests/test_partida.cpp` (3.3) |
| R4. Mundo, efectos y sonido | `src/Paleta.h` (colores del avatar), `src/Escenario.*` (parallax), `src/Efectos.*` (partículas, sacudida), `src/Sonido.*` (audio sintetizado) | `tests/test_efectos.cpp` (4.1–4.5) |
| R5. Móvil | `src/ControlTactil.*` (toques → saltar / sonido), dedos en `src/Ventana.*`, HUD tactil, `web/minimal_shell.html` responsive | `tests/test_tactil.cpp` (5.1–5.4) |
| R3.4. Assets web | `CMakeLists.txt` — `--preload-file assets@/assets` empaqueta `assets/` en `index.data` | CI (3.4) |

**Móvil:** tocar la pantalla salta (y reinicia tras GAME OVER); un toque justo antes de aterrizar encadena el
siguiente salto (buffer de 0,1 s). Un botón grande «SONIDO SI/NO» sustituye a la tecla M. Los textos pasan a
«TOCA…» al detectar un móvil o el primer toque. La página usa toda la pantalla, sin zoom ni desplazamiento, con
botón de pantalla completa (Android/tablets; iPhone no lo permite en webs) y un aviso para girar el móvil.

**Sonido:** sin archivos de audio. Los efectos (salto, aterrizaje, golpe, hito cada 100 puntos, inicio) y una
música en bucle de 4 s se sintetizan al arrancar con SDL2 puro (`Sintesis`/`Mezclador`/`Sonido`). Sin dispositivo
de audio el juego sigue mudo. En el navegador el audio empieza tras la primera pulsación de tecla o clic.
**Estilo:** todos los colores salen del avatar (`Paleta.h`); el Test 4.1 comprueba que siguen cerca de los del sprite.

`assets/avatar_run.bmp` (6 fotogramas de 64x90: 4 de trote, salto, caída) se genera desde `tools/avatar_fuente.webp` con
`python3 tools/generar_avatar.py` (requiere Pillow y numpy). Nativo: `./build/runner`. La demo antigua del motor sigue en
`./build/juego` (solo nativo).

> En Emscripten los archivos del disco no existen: `--preload-file` los empaqueta en `index.data`
> y los monta en un sistema de archivos virtual **antes de `main()`**. Si añades assets nuevos,
> basta con ponerlos en `assets/` (se relinka solo). Al publicar hay que subir `index.data` junto
> a `index.html`, `index.js` e `index.wasm`.

## Compilar y probar en nativo

```bash
sudo apt install libsdl2-dev cmake g++
cmake -S . -B build -DBGE_SANITIZE=ON
cmake --build build
ctest --test-dir build --output-on-failure   # SDL_VIDEODRIVER=dummy: no necesita pantalla
./build/juego                                # demo (ESC para salir)
```

## Compilar a WebAssembly en local

Instala el SDK de Emscripten (una sola vez):

```bash
git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
~/emsdk/emsdk install 6.0.13
~/emsdk/emsdk activate 6.0.13
```

Cada vez que abras una terminal nueva, carga el entorno y compila:

```bash
source ~/emsdk/emsdk_env.sh
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
```

Genera `build-web/index.html`, `index.js`, `index.wasm` e `index.data` (los assets). Para probarlo (los navegadores no
cargan `.wasm` desde `file://`, hay que servirlo por HTTP):

```bash
python3 -m http.server 8000 --directory build-web
# abre http://localhost:8000
```

Equivalente directo con `emcc`, sin CMake:

```bash
emcc -std=c++17 -O2 src/*.cpp -Isrc -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 \
     --shell-file web/minimal_shell.html -o index.html
```

> La primera compilación descarga el *port* de SDL2 desde GitHub y lo cachea; necesita acceso a internet.

## Publicar en GitHub Pages

1. En el repositorio: **Settings → Pages → Build and deployment → Source: GitHub Actions** (una sola vez).
2. Cada push a la rama por defecto ejecuta `.github/workflows/deploy.yml`: pruebas nativas con
   ASan/UBSan, compilación con Emscripten, prueba de integridad por HTTP y despliegue.
   En otras ramas y PR solo se compila y se sube el sitio como artefacto de revisión.

## Uso del motor

Implementa la interfaz `Juego` (`Comenzar`, `Actualizar(dt)`, `Pintar(SDL_Renderer*)`,
`Detener`, `Reactivar`, `Finalizar`) y pásala a `NucleoMotor`; dentro de `Pintar` usa
`Dibujante(renderizador)` para dibujar. Ver `src/main.cpp`.
En Emscripten, `NucleoMotor` y el `Juego` deben tener duración estática (el bucle sigue vivo
después de que `main()` retorne).
