//------------------------------------------------------------------------------------
// Obstaculos  (Juego Runner - Modulo 3.1 y 3.2: obstaculos y colision AABB)
// Encabezado C++ - Obstaculos.h
//------------------------------------------------------------------------------------
// - Obstaculo: "bug" rojo con posicion, velocidad horizontal constante y caja de colision.
// - GestorObstaculos: los genera fuera de la pantalla por la derecha a intervalos
//   aleatorios (generador con semilla -> reproducible en pruebas), los mueve hacia la
//   izquierda y los DESTRUYE en cuanto salen por completo. Reutiliza la capacidad del
//   vector, asi que en regimen estable no hay mas asignaciones de memoria.
// - Colisionan(): interseccion AABB exacta con rectangulos enteros de SDL.
//------------------------------------------------------------------------------------
#pragma once

#include "Dibujante.h"

#include <SDL.h>

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

//------------------------------------------------------------------------------------
// AABB: dos rectangulos se solapan solo si comparten area. Tocarse por el borde
// (x + w == otro.x) NO cuenta como colision; solapar un pixel SI.
//------------------------------------------------------------------------------------
constexpr bool Colisionan(const SDL_Rect& a, const SDL_Rect& b) noexcept
{
	return a.w > 0 && a.h > 0 && b.w > 0 && b.h > 0 &&
	       a.x < b.x + b.w && b.x < a.x + a.w &&
	       a.y < b.y + b.h && b.y < a.y + a.h;
}

struct Obstaculo
{
	double x = 0.0, y = 0.0;    // esquina superior izquierda (px)
	int    ancho = 32, alto = 32;
	double vx = -300.0;         // px/s, negativo = hacia la izquierda; constante de por vida

	SDL_Rect Caja() const noexcept;                  // coincide con lo que se dibuja
	bool     FueraPorLaIzquierda() const noexcept { return x + ancho < 0.0; }
};

struct ConfigObstaculos
{
	int    anchoPantalla = 640;
	double ySuelo        = 400.0;   // los obstaculos apoyan sobre el suelo
	int    anchoMin = 28, anchoMax = 48;
	int    altoMin  = 28, altoMax  = 60;     // saltables: el salto llega a ~107 px
	double intervaloMin = 0.9;               // s entre apariciones (> duracion del salto)
	double intervaloMax = 1.8;
	double primerObstaculo = 1.2;            // s hasta el primero tras (re)empezar
};

class GestorObstaculos
{
public:
	explicit GestorObstaculos(const ConfigObstaculos& config = {}, std::uint32_t semilla = 12345u);

	// 'velocidad' (px/s, positiva) es la que reciben los obstaculos NUEVOS
	void Actualizar(double dt, double velocidad);
	void Reiniciar();
	void Agregar(const Obstaculo& o) { m_activos.push_back(o); ++m_generados; }   // para pruebas

	bool ColisionaCon(const SDL_Rect& caja) const noexcept;
	void Pintar(const Dibujante& d) const;

	const std::vector<Obstaculo>& Activos() const noexcept { return m_activos; }
	std::size_t Capacidad()  const noexcept { return m_activos.capacity(); }
	std::uint64_t Generados() const noexcept { return m_generados; }
	std::uint64_t Destruidos() const noexcept { return m_destruidos; }

private:
	void Generar(double velocidad);
	double SiguienteIntervalo();

	ConfigObstaculos m_config;
	std::mt19937     m_rng;
	std::uint32_t    m_semilla;
	std::vector<Obstaculo> m_activos;
	double        m_temporizador = 0.0;     // s hasta la proxima aparicion
	std::uint64_t m_generados  = 0;
	std::uint64_t m_destruidos = 0;
};
