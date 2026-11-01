//------------------------------------------------------------------------------------
// Hud  (Juego Runner - Modulo 3.3: puntuacion e interfaz)
// Encabezado C++ - Hud.h
//------------------------------------------------------------------------------------
// Dibuja con Dibujante::Texto (fuente bitmap del motor). Es una funcion pura del estado
// de la Partida: el motor limpia el back buffer antes de cada Pintar(), asi que cada
// frame se compone entero y no queda rastro del anterior. Los numeros se rellenan con
// ceros para que el texto no cambie de ancho al subir la puntuacion.
//------------------------------------------------------------------------------------
#pragma once

#include "Partida.h"

#include <string>

namespace Hud
{
	std::string TextoPuntos(const Partida& p);      // "PUNTOS: 00123"
	std::string TextoMejor(const Partida& p);       // "MEJOR: 00500"
	std::string TextoVelocidad(const Partida& p);   // "VEL: 300"

	// Altura reservada para el HUD superior (nada del juego se dibuja ahi)
	constexpr int kAlturaHud = 40;

	void Pintar(const Dibujante& d, const Partida& p, int ancho, int alto);
}
