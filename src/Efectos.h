//------------------------------------------------------------------------------------
// Efectos  (Juego Runner - Modulo 4: efectos visuales)
// Encabezado C++ - Efectos.h
//------------------------------------------------------------------------------------
// Particulas (polvo, explosion, chispas), sacudida de pantalla y borde de impacto.
// - Capacidad fija (kMaxParticulas): nunca asigna memoria durante el juego y, si se
//   llena, ignora las nuevas en vez de crecer.
// - Determinista (generador con semilla) y avanza con los tics de logica.
//------------------------------------------------------------------------------------
#pragma once

#include "Dibujante.h"

#include <SDL.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>

class Efectos
{
public:
	static constexpr std::size_t kMaxParticulas = 256;

	explicit Efectos(double ySuelo, std::uint32_t semilla = 1);

	void Actualizar(double dt);
	void Reiniciar() noexcept;

	void Polvo(double x, double y, int cantidad);    // al correr, saltar y aterrizar
	void Explosion(double x, double y);              // al chocar
	void Chispas(double x, double y);                // al superar un hito
	void Sacudir(double amplitudPx, double duracion);
	void Destello(double duracion);

	// Desplazamiento (px) a aplicar al mundo este frame; (0,0) en reposo
	SDL_Point Sacudida() const noexcept { return m_desplazamiento; }
	double    IntensidadDestello() const noexcept;      // 1 -> 0
	std::size_t Activas() const noexcept { return m_cuenta; }

	void Pintar(const Dibujante& d) const;                        // particulas
	void PintarDestello(const Dibujante& d, int ancho, int alto) const;   // borde de impacto

private:
	struct Particula
	{
		double x = 0, y = 0, vx = 0, vy = 0, g = 0;
		double vida = 0, vidaMax = 1;
		Color  color{};
		int    lado = 4;
		bool   rebota = false;
	};

	void Emitir(const Particula& p) noexcept;
	double Azar(double a, double b) { return std::uniform_real_distribution<double>(a, b)(m_rng); }

	double m_ySuelo;
	std::mt19937 m_rng;
	std::uint32_t m_semilla;
	std::array<Particula, kMaxParticulas> m_particulas{};
	std::size_t m_cuenta = 0;

	double m_sacudidaAmp = 0, m_sacudidaRestante = 0, m_sacudidaTotal = 1;
	double m_destelloRestante = 0, m_destelloTotal = 1;
	SDL_Point m_desplazamiento{ 0, 0 };
};
