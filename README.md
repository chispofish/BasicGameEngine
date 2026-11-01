# BasicGameEngine

Motor básico de juegos 2D en C++17 sobre **SDL2**, pensado para ejecutarse en escritorio
(nativo) y en el navegador (WebAssembly con Emscripten / GitHub Pages).
Migrado desde la versión original basada en Win32.

## Estado de los módulos

| Módulo | Contenido | Estado |
|---|---|---|
| 1. Plataforma y ventana | `src/Ventana.*` (SDL2, foco, cierre, teclado/ratón) | Hecho + QA 1.1–1.3 |
| 2. Núcleo y bucle de paso fijo | `src/NucleoMotor.*` (60 Hz lógicos, `emscripten_set_main_loop_arg`) | Hecho + QA 2.1 (2.2 pendiente de verificar en navegador) |
| 3. Renderizado 2D | hook `Juego::Pintar(SDL_Renderer*)` listo; faltan helpers de dibujo | Pendiente |
| 4. Build y CI/CD | `CMakeLists.txt` básico; falta shell HTML, workflow de Pages | Pendiente |

## Compilar y probar (nativo)

```bash
sudo apt install libsdl2-dev cmake g++
cmake -S . -B build -DBGE_SANITIZE=ON
cmake --build build
ctest --test-dir build --output-on-failure   # usa SDL_VIDEODRIVER=dummy, no necesita pantalla
./build/juego                                # demo: cuadrado que rebota (ESC para salir)
```

## Uso

Implementa la interfaz `Juego` (`Comenzar`, `Actualizar(dt)`, `Pintar(SDL_Renderer*)`,
`Detener`, `Reactivar`, `Finalizar`) y pásala a `NucleoMotor`. Ver `src/main.cpp`.
En Emscripten, `NucleoMotor` y el `Juego` deben tener duración estática.
