//------------------------------------------------------------------------------------
// Jugador.cpp
//------------------------------------------------------------------------------------
#include "Jugador.h"

#include <cmath>

Jugador::Jugador(const ConfigJugador& config) noexcept
    : m_config(config), m_animacion(config.framesCorrer, config.ticsPorFrame), m_yPies(config.ySuelo)
{
}

void Jugador::Reiniciar() noexcept
{
	m_estado = EstadoJugador::Corriendo;
	m_yPies  = m_config.ySuelo;
	m_vy     = 0.0;
	m_animacion.Reiniciar();
}

void Jugador::Actualizar(double dt, bool pedirSalto) noexcept
{
	// Impulso inicial: solo desde el suelo
	if (EnElSuelo() && pedirSalto)
	{
		m_vy     = -m_config.velocidadSalto;
		m_estado = EstadoJugador::Saltando;
	}

	if (EnElSuelo())
	{
		m_animacion.Avanzar();     // el ciclo de carrera solo corre sobre el suelo
		return;
	}

	// Euler semi-implicito: primero la velocidad, luego la posicion
	m_vy    += m_config.gravedad * dt;
	m_yPies += m_vy * dt;

	if (m_yPies >= m_config.ySuelo)     // aterrizaje: nunca se atraviesa el suelo
	{
		m_yPies  = m_config.ySuelo;
		m_vy     = 0.0;
		m_estado = EstadoJugador::Corriendo;
		m_animacion.Reiniciar();
	}
	else
	{
		m_estado = (m_vy < 0.0) ? EstadoJugador::Saltando : EstadoJugador::Cayendo;
	}
}

int Jugador::FrameActual() const noexcept
{
	switch (m_estado)
	{
	case EstadoJugador::Saltando: return m_config.frameSalto;
	case EstadoJugador::Cayendo:  return m_config.frameCaida;
	case EstadoJugador::Corriendo: break;
	}
	return m_animacion.Frame();
}

SDL_Rect Jugador::Rectangulo() const noexcept
{
	const int w = m_config.anchoFrame * m_config.escala;
	const int h = m_config.altoFrame * m_config.escala;
	return SDL_Rect{ m_config.x, static_cast<int>(std::lround(m_yPies)) - h, w, h };
}

SDL_Rect Jugador::CajaColision() const noexcept
{
	// Recorta el borde transparente del sprite (en pixeles de la hoja: 4 por cada lado,
	// 1 por arriba, 2 por abajo) para que la colision sea justa; ver Test 3.2
	SDL_Rect r = Rectangulo();
	const int mx = 4 * m_config.escala, my = 1 * m_config.escala;
	r.x += mx; r.w -= 2 * mx;
	r.y += my; r.h -= my + 2 * m_config.escala;   // 2 px menos por abajo (el fotograma de salto recoge las piernas)
	return r;
}

void Jugador::Pintar(SDL_Renderer* renderizador, const Textura* hoja) const
{
	const SDL_Rect destino = Rectangulo();
	if (!hoja)
	{
		Dibujante(renderizador).RectanguloRelleno(destino, Colores::Azul);   // respaldo visible
		return;
	}
	const SDL_Rect origen{ FrameActual() * m_config.anchoFrame, 0, m_config.anchoFrame, m_config.altoFrame };
	hoja->Dibujar(renderizador, &origen, destino);
}
