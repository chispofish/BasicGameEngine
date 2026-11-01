cmake_minimum_required(VERSION 3.16)
project(BasicGameEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BGE_SANITIZE "Compilar con AddressSanitizer/UBSan (solo nativo)" OFF)

# --- Biblioteca del motor (Modulos 1 y 2) ---------------------------------------
add_library(nucleo STATIC src/Ventana.cpp src/NucleoMotor.cpp)
target_include_directories(nucleo PUBLIC src)

if(EMSCRIPTEN)
  # Flags web basicos; el empaquetado completo (shell, index.html) es el Modulo 4.
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
  set_target_properties(juego PROPERTIES OUTPUT_NAME index SUFFIX ".html")
endif()

# --- Pruebas de QA (solo nativo) ---------------------------------------------------
if(NOT EMSCRIPTEN)
  enable_testing()
  add_executable(test_motor tests/test_motor.cpp)
  target_link_libraries(test_motor PRIVATE nucleo)
  add_test(NAME qa_modulos_1_2 COMMAND test_motor)
  set_tests_properties(qa_modulos_1_2 PROPERTIES
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software;SDL_AUDIODRIVER=dummy")
endif()
