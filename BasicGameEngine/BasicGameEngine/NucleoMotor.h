//------------------------------------------------------------------------------------
// NucleoMotor
// Encabezado C++ - NucleoMotor.h
//------------------------------------------------------------------------------------
// Para evitar la doble inclusion de archivos
#pragma once

//------------------------------------------------------------------------------------
// Inclusion de archivos
//------------------------------------------------------------------------------------
#include <Windows.h>
#include <time.h>

//------------------------------------------------------------------------------------
// Declaraciones de funciones
//------------------------------------------------------------------------------------
// punto de entrada de los programas Windows
int WINAPI WinMain(HINSTANCE HInstancia, HINSTANCE hInstanciaPrev, PSTR szCmdLine, int iMostrar);

// procedimiento de ventana
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

//------------------------------------------------------------------------------------
// Funciones auxiliares del nuecleo motor
//------------------------------------------------------------------------------------
