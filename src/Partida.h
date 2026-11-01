//------------------------------------------------------------------------------------
// Partida  (Juego Runner - Modulo 3: reglas del juego)
// Encabezado C++ - Partida.h
//------------------------------------------------------------------------------------
// Une jugador + obstaculos + puntuacion + estado (Jugando / GameOver). No depende del
// renderizador ni del teclado, por lo que se prueba tic a tic sin ventana.
//  - Puntos = distancia recorrida / 10 (a 300 px/s suma 30 puntos por segundo).
//  - La velocidad del mundo sube con el tiempo hasta un maximo.
//  - Al chocar (AABB entre CajaColision del jugador y un obstaculo) pasa a GameOver y
//    el mundo se congela; con ESPACIO (flanco de pulsacion, tras una breve pausa para no
//    reiniciar por accidente) se empieza de nuevo conservando la mejor puntuacion.
//------------------------------------------------------------------------------------
#pragma once

#include "Jugador.h"
#include "Obstaculos.h"

enum class EstadoPartida { Jugando, GameOver };

struct ConfigPartida
{
	int    ancho  = 640;
	double ySuelo = 400.0;
	double velocidadInicial  = 300.0;   // px/s
	double velocidadMaxima   = 700.0;
	double aceleracion       = 8.0;     // px/s por segundo
	double pixelesPorPunto   = 10.0;
	int    ticsBloqueoGameOver = 30;    // 0,5 s a 60 Hz
};

// Sucesos de la partida desde la ultima llamada a TomarEventos(). El juego los traduce a
// sonido y efectos visuales; la logica no sabe nada de audio ni de graficos.
struct EventosPartida
{
	bool salto      = false;   // el jugador despega
	bool aterrizaje = false;   // el jugador toca el suelo
	bool choque     = false;   // paso a GameOver
	bool reinicio   = false;   // nueva partida
	int  hitos      = 0;       // veces que se cruzo un multiplo de 100 puntos
};

class Partida
{
public:
	explicit Partida(const ConfigPartida& config = {}, std::uint32_t semilla = 12345u);

	// Un tic de logica. 'saltar' = tecla mantenida (salto en juego; reinicio en GameOver)
	void Actualizar(double dt, bool saltar);
	void Reiniciar();

	// Devuelve y borra los sucesos acumulados (puede haber varios tics por frame)
	EventosPartida TomarEventos() noexcept { const EventosPartida e = m_eventos; m_eventos = {}; return e; }

	EstadoPartida Estado()    const noexcept { return m_estado; }
	int    Puntos()           const noexcept { return static_cast<int>(m_distancia / m_config.pixelesPorPunto); }
	int    Mejor()            const noexcept { return m_mejor; }
	double Velocidad()        const noexcept { return m_velocidad; }
	double Distancia()        const noexcept { return m_distancia; }

	const Jugador&          ElJugador()   const noexcept { return m_jugador; }
	GestorObstaculos&       Obstaculos()        noexcept { return m_obstaculos; }
	const GestorObstaculos& Obstaculos()  const noexcept { return m_obstaculos; }
	const ConfigPartida&    Config()      const noexcept { return m_config; }

private:
	ConfigPartida    m_config;
	Jugador          m_jugador;
	GestorObstaculos m_obstaculos;
	EstadoPartida    m_estado = EstadoPartida::Jugando;
	double m_velocidad = 0.0;
	double m_distancia = 0.0;
	int    m_mejor     = 0;
	int    m_ticsGameOver = 0;
	bool   m_saltarPrevio = false;
	int    m_hitoPrevio   = 0;
	EventosPartida m_eventos;
};
