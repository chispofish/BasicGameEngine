//------------------------------------------------------------------------------------
// Obstaculos.cpp
//------------------------------------------------------------------------------------
#include "Obstaculos.h"

#include <algorithm>
#include <cmath>

SDL_Rect Obstaculo::Caja() const noexcept
{
	return SDL_Rect{ static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)), ancho, alto };
}

GestorObstaculos::GestorObstaculos(const ConfigObstaculos& config, std::uint32_t semilla)
    : m_config(config), m_rng(semilla), m_semilla(semilla)
{
	m_activos.reserve(16);
	m_temporizador = m_config.primerObstaculo;
}

void GestorObstaculos::Reiniciar()
{
	m_destruidos += m_activos.size();
	m_activos.clear();                    // conserva la capacidad
	m_rng.seed(m_semilla);
	m_temporizador = m_config.primerObstaculo;
}

double GestorObstaculos::SiguienteIntervalo()
{
	return std::uniform_real_distribution<double>(m_config.intervaloMin, m_config.intervaloMax)(m_rng);
}

void GestorObstaculos::Generar(double velocidad)
{
	Obstaculo o;
	o.ancho = std::uniform_int_distribution<int>(m_config.anchoMin, m_config.anchoMax)(m_rng);
	o.alto  = std::uniform_int_distribution<int>(m_config.altoMin, m_config.altoMax)(m_rng);
	o.x  = m_config.anchoPantalla;                 // justo fuera de la pantalla, por la derecha
	o.y  = m_config.ySuelo - o.alto;
	o.vx = -velocidad;
	m_activos.push_back(o);
	++m_generados;
}

void GestorObstaculos::Actualizar(double dt, double velocidad)
{
	for (Obstaculo& o : m_activos) o.x += o.vx * dt;

	// Destruccion: fuera por completo por la izquierda
	const auto fin = std::remove_if(m_activos.begin(), m_activos.end(),
	                                [](const Obstaculo& o) { return o.FueraPorLaIzquierda(); });
	m_destruidos += static_cast<std::uint64_t>(m_activos.end() - fin);
	m_activos.erase(fin, m_activos.end());

	m_temporizador -= dt;
	if (m_temporizador <= 0.0)
	{
		Generar(velocidad);
		m_temporizador += SiguienteIntervalo();
	}
}

bool GestorObstaculos::ColisionaCon(const SDL_Rect& caja) const noexcept
{
	return std::any_of(m_activos.begin(), m_activos.end(),
	                   [&](const Obstaculo& o) { return Colisionan(caja, o.Caja()); });
}

void GestorObstaculos::Pintar(const Dibujante& d) const
{
	for (const Obstaculo& o : m_activos)
	{
		const SDL_Rect c = o.Caja();
		d.RectanguloRelleno(c, Colores::Rojo);
		d.Rectangulo(c, Color{ 120, 20, 20, 255 });
		// Dos "ojos" de bug para que no sea un bloque liso
		const int ojo = std::max(3, c.w / 8);
		d.RectanguloRelleno(SDL_Rect{ c.x + c.w / 4, c.y + c.h / 4, ojo, ojo }, Colores::Blanco);
		d.RectanguloRelleno(SDL_Rect{ c.x + c.w - c.w / 4 - ojo, c.y + c.h / 4, ojo, ojo }, Colores::Blanco);
	}
}
