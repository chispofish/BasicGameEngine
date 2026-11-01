//------------------------------------------------------------------------------------
// Jugador  (Juego Runner - Modulo 2: Fisica y animacion del jugador)
// Encabezado C++ - Jugador.h
//------------------------------------------------------------------------------------
// - Animacion: cicla fotogramas de una hoja de sprites horizontal cada N tics logicos.
// - Fisica: gravedad vertical, impulso de salto y suelo plano (integracion de Euler
//   semi-implicita a paso fijo, pensada para Juego::Actualizar(dt) a 60 Hz).
// - Estados: Corriendo (en el suelo), Saltando (subiendo), Cayendo (bajando).
//
// Disposicion de la hoja de sprites (assets/avatar_run.bmp), todos los fotogramas del
// mismo tamano en una sola fila:
//     [0..framesCorrer-1] ciclo de carrera | [frameSalto] subiendo | [frameCaida] bajando
//
// Coordenadas: Y crece hacia abajo; la posicion vertical del jugador es la de sus PIES.
// La logica no depende de SDL salvo por SDL_Rect, asi que se prueba sin ventana.
//------------------------------------------------------------------------------------
#pragma once

#include "Textura.h"

#include <SDL.h>

enum class EstadoJugador { Corriendo, Saltando, Cayendo };

//------------------------------------------------------------------------------------
// Animacion ciclica por tics logicos
//------------------------------------------------------------------------------------
class Animacion
{
public:
	Animacion(int numFrames, int ticsPorFrame) noexcept
	    : m_numFrames(numFrames > 0 ? numFrames : 1), m_ticsPorFrame(ticsPorFrame > 0 ? ticsPorFrame : 1) {}

	// Un tic de logica; cada 'ticsPorFrame' tics pasa al siguiente fotograma (con vuelta al 0)
	void Avanzar() noexcept
	{
		if (++m_tics >= m_ticsPorFrame) { m_tics = 0; m_frame = (m_frame + 1) % m_numFrames; }
	}
	void Reiniciar() noexcept { m_frame = 0; m_tics = 0; }

	int Frame()     const noexcept { return m_frame; }
	int NumFrames() const noexcept { return m_numFrames; }

private:
	int m_numFrames;
	int m_ticsPorFrame;
	int m_frame = 0;
	int m_tics  = 0;
};

//------------------------------------------------------------------------------------
// Configuracion (valores pensados para una ventana de 640x480)
//------------------------------------------------------------------------------------
struct ConfigJugador
{
	int    x              = 80;      // borde izquierdo fijo del sprite (px)
	double ySuelo         = 400.0;   // Y del suelo: donde apoyan los pies (px)
	int    anchoFrame     = 16;      // tamano de un fotograma en la hoja (px)
	int    altoFrame      = 24;
	int    escala         = 3;       // el pixel art se amplia por enteros
	double gravedad       = 1800.0;  // px/s^2 hacia abajo
	double velocidadSalto = 620.0;   // px/s hacia arriba en el instante del salto
	int    ticsPorFrame   = 6;       // 6 tics a 60 Hz = 10 fotogramas/s de carrera
	int    framesCorrer   = 4;
	int    frameSalto     = 4;       // indice de fotograma en la hoja
	int    frameCaida     = 5;
};

//------------------------------------------------------------------------------------
// Clase Jugador
//------------------------------------------------------------------------------------
class Jugador
{
public:
	explicit Jugador(const ConfigJugador& config = {}) noexcept;

	// Un tic de logica. 'pedirSalto' = tecla de salto mantenida; solo tiene efecto
	// si el jugador esta en el suelo (no hay doble salto).
	void Actualizar(double dt, bool pedirSalto) noexcept;

	// Vuelve al suelo, quieto y con la animacion en el fotograma 0
	void Reiniciar() noexcept;

	// Dibuja el fotograma actual. Sin textura (fallo de carga) usa un rectangulo de color.
	void Pintar(SDL_Renderer* renderizador, const Textura* hoja) const;

	EstadoJugador Estado()      const noexcept { return m_estado; }
	bool          EnElSuelo()   const noexcept { return m_estado == EstadoJugador::Corriendo; }
	double        YPies()       const noexcept { return m_yPies; }
	double        VelocidadY()  const noexcept { return m_vy; }
	int           FrameActual() const noexcept;      // indice en la hoja segun estado

	// Rectangulo del sprite en pantalla y caja de colision (algo mas pequena que el sprite)
	SDL_Rect Rectangulo() const noexcept;
	SDL_Rect CajaColision() const noexcept;

	const ConfigJugador& Config() const noexcept { return m_config; }

private:
	ConfigJugador m_config;
	Animacion     m_animacion;
	EstadoJugador m_estado = EstadoJugador::Corriendo;
	double        m_yPies  = 0.0;
	double        m_vy     = 0.0;   // px/s, positivo = hacia abajo
};
