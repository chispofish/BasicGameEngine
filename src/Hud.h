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
#include "Paleta.h"

#include <string>

namespace Hud
{
	std::string TextoPuntos(const Partida& p);      // "PUNTOS: 00123"
	std::string TextoMejor(const Partida& p);       // "MEJOR: 00500"
	std::string TextoVelocidad(const Partida& p);   // "VEL: 300"

	// Altura reservada para el HUD superior (nada del juego se dibuja ahi)
	constexpr int kAlturaHud = 40;

	struct Opciones
	{
		bool sonidoActivo   = true;    // indicador "M: SONIDO"
		bool resaltarPuntos = false;   // la puntuacion brilla un instante al superar un hito
		bool tactil         = false;   // movil: textos de toque y boton de sonido grande
	};

	// Textos segun el dispositivo de entrada
	std::string TextoInstrucciones(bool tactil);   // "TOCA LA PANTALLA PARA SALTAR" / "ESPACIO / FLECHA ARRIBA: SALTAR"
	std::string TextoReinicio(bool tactil);        // "TOCA PARA REINICIAR" / "ESPACIO: REINICIAR"

	// Boton de sonido tactil (esquina inferior derecha, sobre el suelo). Es la UNICA fuente
	// de verdad: el HUD lo dibuja y ControlTactil lo usa para detectar el toque.
	SDL_Rect RectBotonSonido(int ancho, int alto);

	// Texto con sombra (legible sobre el degradado del cielo)
	int TextoSombra(const Dibujante& d, int x, int y, const std::string& t, Color c, int escala);

	void Pintar(const Dibujante& d, const Partida& p, int ancho, int alto, const Opciones& op = {});
}
