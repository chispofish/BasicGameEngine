//------------------------------------------------------------------------------------
// Efectos.cpp
//------------------------------------------------------------------------------------
#include "Efectos.h"

#include "Paleta.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr double kPi = 3.14159265358979323846;
}

Efectos::Efectos(double ySuelo, std::uint32_t semilla) : m_ySuelo(ySuelo), m_rng(semilla), m_semilla(semilla) {}

void Efectos::Reiniciar() noexcept
{
	m_cuenta = 0;
	m_rng.seed(m_semilla);
	m_sacudidaRestante = m_destelloRestante = 0;
	m_desplazamiento = SDL_Point{ 0, 0 };
}

void Efectos::Emitir(const Particula& p) noexcept
{
	if (m_cuenta < kMaxParticulas) m_particulas[m_cuenta++] = p;    // lleno: se descarta
}

void Efectos::Polvo(double x, double y, int cantidad)
{
	for (int i = 0; i < cantidad; ++i)
	{
		Particula p;
		p.x = x + Azar(-6, 6); p.y = y - Azar(0, 3);
		p.vx = Azar(-110, -30); p.vy = Azar(-70, -15); p.g = 60;
		p.vidaMax = p.vida = Azar(0.28, 0.5);
		p.color = Paleta::Polvo; p.lado = static_cast<int>(Azar(4, 7));
		Emitir(p);
	}
}

void Efectos::Explosion(double x, double y)
{
	const Color colores[] = { Paleta::Violeta, Paleta::Verde, Paleta::Dorado, Paleta::BugCuerpo, Paleta::Piel, Colores::Blanco };
	for (int i = 0; i < 44; ++i)
	{
		const double ang = Azar(0, 2 * kPi), vel = Azar(120, 420);
		Particula p;
		p.x = x; p.y = y;
		p.vx = std::cos(ang) * vel; p.vy = std::sin(ang) * vel - 120; p.g = 900;
		p.vidaMax = p.vida = Azar(0.6, 1.1);
		p.color = colores[i % 6]; p.lado = static_cast<int>(Azar(5, 9)); p.rebota = true;
		Emitir(p);
	}
}

void Efectos::Chispas(double x, double y)
{
	for (int i = 0; i < 14; ++i)
	{
		Particula p;
		p.x = x + Azar(-20, 20); p.y = y;
		p.vx = Azar(-90, 90); p.vy = Azar(-220, -80); p.g = 260;
		p.vidaMax = p.vida = Azar(0.45, 0.8);
		p.color = (i % 2) ? Paleta::Dorado : Colores::Blanco; p.lado = static_cast<int>(Azar(3, 5));
		Emitir(p);
	}
}

void Efectos::Sacudir(double amplitudPx, double duracion)
{
	m_sacudidaAmp = amplitudPx; m_sacudidaRestante = m_sacudidaTotal = std::max(duracion, 1e-6);
}

void Efectos::Destello(double duracion)
{
	m_destelloRestante = m_destelloTotal = std::max(duracion, 1e-6);
}

double Efectos::IntensidadDestello() const noexcept
{
	return m_destelloRestante > 0 ? m_destelloRestante / m_destelloTotal : 0.0;
}

void Efectos::Actualizar(double dt)
{
	// Particulas: integracion, rebote en el suelo y compactado de las muertas
	std::size_t vivas = 0;
	for (std::size_t i = 0; i < m_cuenta; ++i)
	{
		Particula p = m_particulas[i];
		p.vida -= dt;
		if (p.vida <= 0) continue;
		p.vy += p.g * dt;
		p.x  += p.vx * dt;
		p.y  += p.vy * dt;
		if (p.y > m_ySuelo) { p.y = m_ySuelo; p.vy = p.rebota ? -p.vy * 0.4 : 0; p.vx *= 0.6; }
		m_particulas[vivas++] = p;
	}
	m_cuenta = vivas;

	// Sacudida: la amplitud decae linealmente hasta 0
	if (m_sacudidaRestante > 0)
	{
		m_sacudidaRestante = std::max(0.0, m_sacudidaRestante - dt);
		const double a = m_sacudidaAmp * m_sacudidaRestante / m_sacudidaTotal;
		const int r = static_cast<int>(std::lround(a));
		m_desplazamiento = r > 0 ? SDL_Point{ std::uniform_int_distribution<int>(-r, r)(m_rng),
		                                      std::uniform_int_distribution<int>(-r, r)(m_rng) }
		                         : SDL_Point{ 0, 0 };
	}
	else m_desplazamiento = SDL_Point{ 0, 0 };

	if (m_destelloRestante > 0) m_destelloRestante = std::max(0.0, m_destelloRestante - dt);
}

void Efectos::Pintar(const Dibujante& d) const
{
	for (std::size_t i = 0; i < m_cuenta; ++i)
	{
		const Particula& p = m_particulas[i];
		const double f = p.vida / p.vidaMax;                       // 1 -> 0: se encoge al morir
		const int lado = std::max(1, static_cast<int>(std::lround(p.lado * (0.4 + 0.6 * f))));
		d.RectanguloRelleno(SDL_Rect{ static_cast<int>(p.x) - lado / 2, static_cast<int>(p.y) - lado / 2, lado, lado }, p.color);
	}
}

void Efectos::PintarDestello(const Dibujante& d, int ancho, int alto) const
{
	const double k = IntensidadDestello();
	if (k <= 0) return;
	const int g = std::max(2, static_cast<int>(std::lround(14 * k)));     // marco naranja que se cierra
	d.RectanguloRelleno(SDL_Rect{ 0, 0, ancho, g }, Paleta::BugCuerpo);
	d.RectanguloRelleno(SDL_Rect{ 0, alto - g, ancho, g }, Paleta::BugCuerpo);
	d.RectanguloRelleno(SDL_Rect{ 0, 0, g, alto }, Paleta::BugCuerpo);
	d.RectanguloRelleno(SDL_Rect{ ancho - g, 0, g, alto }, Paleta::BugCuerpo);
}
