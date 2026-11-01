cmake_minimum_required(VERSION 3.16)
project(BasicGameEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de build" FORCE)
endif()

option(BGE_SANITIZE "Compilar con AddressSanitizer/UBSan (solo nativo)" OFF)

# --- Biblioteca del motor (Modulos 1, 2 y 3) y del Runner (texturas, jugador) ---
add_library(nucleo STATIC
  src/Ventana.cpp src/NucleoMotor.cpp src/Dibujante.cpp
  src/Textura.cpp src/Jugador.cpp)
target_include_directories(nucleo PUBLIC src)

if(EMSCRIPTEN)
  # SDL2 viene como "port" de Emscripten (se descarga y cachea la primera vez)
  target_compile_options(nucleo PUBLIC -sUSE_SDL=2)
  target_link_options(nucleo PUBLIC -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1)
else()
  find_package(SDL2 REQUIRED)
  target_link_libraries(nucleo PUBLIC SDL2::SDL2)
  if(MSVC)
    target_compile_options(nucleo PRIVATE /W4)
  else()
    target_compile_options(nucleo PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  if(BGE_SANITIZE AND NOT MSVC)
    target_compile_options(nucleo PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(nucleo PUBLIC -fsanitize=address,undefined)
  endif()
endif()

# --- Ejecutable de demostracion --------------------------------------------------
add_executable(juego src/main.cpp)
target_link_libraries(juego PRIVATE nucleo)

if(EMSCRIPTEN)
  # Salida: index.html + index.js + index.wasm (en el directorio de build)
  set(BGE_SHELL "${CMAKE_SOURCE_DIR}/web/minimal_shell.html")
  set_target_properties(juego PROPERTIES
    OUTPUT_NAME index
    SUFFIX ".html"
    LINK_DEPENDS "${BGE_SHELL}")          # re-enlaza si cambia la plantilla
  target_link_options(juego PRIVATE
    "SHELL:--shell-file ${BGE_SHELL}"
    $<$<CONFIG:Debug>:-sASSERTIONS=1>)
endif()

# --- Runner (banco de pruebas visual; solo nativo hasta el Modulo 3) ---------------
if(NOT EMSCRIPTEN)
  add_executable(runner src/runner.cpp)
  target_link_libraries(runner PRIVATE nucleo)
  target_compile_definitions(runner PRIVATE BGE_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
endif()

# --- Pruebas de QA (solo nativo) ---------------------------------------------------
if(NOT EMSCRIPTEN)
  enable_testing()
  set(BGE_TEST_ENV "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software;SDL_AUDIODRIVER=dummy")

  add_executable(test_motor tests/test_motor.cpp)
  target_link_libraries(test_motor PRIVATE nucleo)
  add_test(NAME qa_modulos_1_2 COMMAND test_motor)

  add_executable(test_graficos tests/test_graficos.cpp)
  target_link_libraries(test_graficos PRIVATE nucleo)
  add_test(NAME qa_modulo_3 COMMAND test_graficos)

  add_executable(test_jugador tests/test_jugador.cpp)
  target_link_libraries(test_jugador PRIVATE nucleo)
  target_compile_definitions(test_jugador PRIVATE BGE_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
  add_test(NAME qa_runner_modulos_1_2 COMMAND test_jugador)

  set_tests_properties(qa_modulos_1_2 qa_modulo_3 qa_runner_modulos_1_2 PROPERTIES ENVIRONMENT "${BGE_TEST_ENV}")
endif()
