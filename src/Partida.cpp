//------------------------------------------------------------------------------------
// Partida.cpp
//------------------------------------------------------------------------------------
#include "Partida.h"

#include <algorithm>

namespace
{
ConfigObstaculos ParaObstaculos(const ConfigPartida& c)
{
	ConfigObstaculos o;
	o.anchoPantalla = c.ancho;
	o.ySuelo        = c.ySuelo;
	return o;
}
ConfigJugador ParaJugador(const ConfigPartida& c)
{
	ConfigJugador j;
	j.ySuelo = c.ySuelo;
	return j;
}
} // namespace

Partida::Partida(const ConfigPartida& config, std::uint32_t semilla)
    : m_config(config), m_jugador(ParaJugador(config)),
      m_obstaculos(ParaObstaculos(config), semilla), m_velocidad(config.velocidadInicial)
{
}

void Partida::Reiniciar()
{
	m_jugador.Reiniciar();
	m_obstaculos.Reiniciar();
	m_estado       = EstadoPartida::Jugando;
	m_velocidad    = m_config.velocidadInicial;
	m_distancia    = 0.0;
	m_ticsGameOver = 0;
}

void Partida::Actualizar(double dt, bool saltar)
{
	const bool flanco = saltar && !m_saltarPrevio;
	m_saltarPrevio = saltar;

	if (m_estado == EstadoPartida::GameOver)
	{
		++m_ticsGameOver;
		if (flanco && m_ticsGameOver >= m_config.ticsBloqueoGameOver) Reiniciar();
		return;                                   // mundo congelado
	}

	m_jugador.Actualizar(dt, saltar);
	m_obstaculos.Actualizar(dt, m_velocidad);
	m_distancia += m_velocidad * dt;
	m_velocidad  = std::min(m_config.velocidadMaxima, m_velocidad + m_config.aceleracion * dt);

	if (m_obstaculos.ColisionaCon(m_jugador.CajaColision()))
	{
		m_estado       = EstadoPartida::GameOver;
		m_mejor        = std::max(m_mejor, Puntos());
		m_ticsGameOver = 0;
	}
}
