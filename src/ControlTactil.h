//------------------------------------------------------------------------------------
// ControlTactil  (Juego Runner - Modulo 5: control para movil)
// Encabezado C++ - ControlTactil.h
//------------------------------------------------------------------------------------
// Traduce los dedos de la pantalla en las dos ordenes del juego, sin depender de SDL
// ni del renderizador (se prueba inyectando toques):
//   - Tocar en cualquier parte  -> saltar (y reiniciar tras GAME OVER).
//   - Tocar el boton de sonido  -> alternar sonido; ese toque NO salta.
// "Buffer de salto": un toque mantiene la orden activa unos tics (por defecto 6 = 0,1 s),
// asi tocar un instante ANTES de aterrizar tambien salta al tocar el suelo, como pide un
// juego tactil. Un dedo que se mantiene apoyado mantiene el salto (igual que mantener la
// barra espaciadora).
//------------------------------------------------------------------------------------
#pragma once

#include "Ventana.h"

#include <SDL.h>

#include <vector>

struct SalidaTactil
{
	bool saltar         = false;
	bool alternarSonido = false;   // flanco: true solo en el tic del toque
};

class ControlTactil
{
public:
	explicit ControlTactil(const SDL_Rect& botonSonido, int ticsBuffer = 6) noexcept
	    : m_boton(botonSonido), m_ticsBuffer(ticsBuffer) {}

	// Un tic de logica. 'nuevos' = TomarToquesNuevos(); 'activos' = ToquesActivos().
	SalidaTactil Actualizar(const std::vector<Toque>& activos, const std::vector<SDL_Point>& nuevos) noexcept;

	// Olvida el salto pendiente (p. ej. al reiniciar la partida)
	void Cancelar() noexcept { m_buffer = 0; }

	bool EnBoton(int x, int y) const noexcept
	{
		const SDL_Point p{ x, y };
		return SDL_PointInRect(&p, &m_boton) == SDL_TRUE;
	}

private:
	SDL_Rect m_boton;
	int      m_ticsBuffer;
	int      m_buffer = 0;
};
