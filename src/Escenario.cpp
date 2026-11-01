//------------------------------------------------------------------------------------
// Escenario.cpp
//------------------------------------------------------------------------------------
#include "Escenario.h"

#include "Paleta.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace
{
Color Mezcla(Color a, Color b, double t)
{
	auto m = [t](int x, int y) { return static_cast<std::uint8_t>(std::lround(x + (y - x) * t)); };
	return Color{ m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), 255 };
}

// Perfil periodico: paseo aleatorio al que se resta la deriva para que el final enlace con el principio
std::vector<int> Perfil(std::mt19937& rng, int columnas, int minimo, int maximo, int paso)
{
	std::vector<double> h(static_cast<std::size_t>(columnas));
	double v = (minimo + maximo) / 2.0;
	for (auto& x : h)
	{
		v = std::clamp(v + std::uniform_int_distribution<int>(-1, 1)(rng) * paso, double(minimo), double(maximo));
		x = v;
	}
	const double deriva = h.back() - h.front();
	std::vector<int> r(h.size());
	for (std::size_t i = 0; i < h.size(); ++i)
		r[i] = std::clamp(static_cast<int>(std::lround(h[i] - deriva * i / h.size())), minimo, maximo);
	return r;
}
} // namespace

Escenario::Escenario(int ancho, int alto, double ySuelo, std::uint32_t semilla)
    : m_ancho(ancho), m_alto(alto), m_ySuelo(ySuelo)
{
	std::mt19937 rng(semilla);
	for (int i = 0; i < 46; ++i)
		m_estrellas.push_back({ std::uniform_int_distribution<int>(0, ancho - 1)(rng),
		                        std::uniform_int_distribution<int>(44, std::max(45, int(ySuelo) - 150))(rng),
		                        std::uniform_int_distribution<int>(0, 3)(rng) });
	m_colinasLejos = Perfil(rng, 2 * ancho / kAnchoColumna, 50, 140, 12);
	m_colinasCerca = Perfil(rng, 2 * ancho / kAnchoColumna, 24, 80, 8);
	for (int i = 0; i < 18; ++i)
		m_motas.push_back({ std::uniform_int_distribution<int>(0, kPeriodoSuelo - 1)(rng),
		                    std::uniform_int_distribution<int>(18, std::max(19, alto - int(ySuelo) - 8))(rng),
		                    std::uniform_int_distribution<int>(6, 18)(rng) });
}

void Escenario::Actualizar(double dt, double velocidad) noexcept
{
	++m_tics;
	m_suelo = std::fmod(m_suelo + velocidad * dt, kPeriodoSuelo);
	m_cerca = std::fmod(m_cerca + velocidad * 0.30 * dt, 2.0 * m_ancho);
	m_lejos = std::fmod(m_lejos + velocidad * 0.10 * dt, 2.0 * m_ancho);
}

void Escenario::PintarCielo(const Dibujante& d) const
{
	const int banda = 8, fin = static_cast<int>(m_ySuelo);
	for (int y = 0; y < fin; y += banda)
	{
		// Se acerca al horizonte en pasos discretos (look de paleta limitada)
		const double t = std::floor(double(y) / fin * 10.0) / 10.0;
		d.RectanguloRelleno(SDL_Rect{ 0, y, m_ancho, std::min(banda, fin - y) }, Mezcla(Paleta::CieloArriba, Paleta::CieloHorizonte, t * t));
	}
	for (const Estrella& e : m_estrellas)
	{
		const bool brillo = ((m_tics / 40 + static_cast<std::uint64_t>(e.fase)) % 4) != 0;   // parpadeo lento
		d.RectanguloRelleno(SDL_Rect{ e.x, e.y, brillo ? 2 : 1, brillo ? 2 : 1 }, Paleta::Estrella);
	}
	// Luna de bloques de 4 px, con crateres
	const int cx = m_ancho - 150, cy = 92, r = 7;
	for (int j = -r; j <= r; ++j)
	{
		const int mitad = static_cast<int>(std::floor(std::sqrt(double(r * r - j * j) + 0.5)));
		d.RectanguloRelleno(SDL_Rect{ cx - mitad * 4, cy + j * 4, (2 * mitad + 1) * 4, 4 }, Paleta::Luna);
	}
	d.RectanguloRelleno(SDL_Rect{ cx - 12, cy - 8, 8, 8 }, Paleta::LunaCrater);
	d.RectanguloRelleno(SDL_Rect{ cx + 8, cy + 8, 12, 8 }, Paleta::LunaCrater);
	d.RectanguloRelleno(SDL_Rect{ cx - 4, cy + 16, 4, 4 }, Paleta::LunaCrater);
}

void Escenario::PintarColinas(const Dibujante& d, const std::vector<int>& alturas, double desplazamiento, Color c) const
{
	const int n = static_cast<int>(alturas.size());
	const int offset = static_cast<int>(desplazamiento);
	const int primera = offset / kAnchoColumna;
	const int resto   = offset % kAnchoColumna;
	for (int x = -resto, i = primera; x < m_ancho; x += kAnchoColumna, ++i)
	{
		const int h = alturas[static_cast<std::size_t>(i % n)];
		d.RectanguloRelleno(SDL_Rect{ x, static_cast<int>(m_ySuelo) - h, kAnchoColumna, h }, c);
	}
}

void Escenario::PintarSuelo(const Dibujante& d) const
{
	const int y0 = static_cast<int>(m_ySuelo);
	const int offset = static_cast<int>(m_suelo);
	d.RectanguloRelleno(SDL_Rect{ 0, y0, m_ancho, m_alto - y0 }, Paleta::Suelo);

	// Detalles del suelo (periodo 640 px, se dibujan dos copias para cubrir el borde)
	for (const Mota& m : m_motas)
		for (int copia = -1; copia <= 1; ++copia)
		{
			const int x = m.x - offset + copia * kPeriodoSuelo;
			if (x + m.w > 0 && x < m_ancho)
				d.RectanguloRelleno(SDL_Rect{ x, y0 + m.y, m.w, 4 }, Paleta::SueloOscuro);
		}

	// Hierba: contorno oscuro, cuerpo verde y pelillos que avanzan con el suelo
	d.RectanguloRelleno(SDL_Rect{ 0, y0, m_ancho, 10 }, Paleta::Hierba);
	d.RectanguloRelleno(SDL_Rect{ 0, y0, m_ancho, 3 }, Paleta::HierbaClara);
	d.RectanguloRelleno(SDL_Rect{ 0, y0 + 10, m_ancho, 3 }, Paleta::Contorno);
	const int paso = 32;
	for (int x = -(offset % paso); x < m_ancho; x += paso)
		d.RectanguloRelleno(SDL_Rect{ x + 8, y0 + 3, 4, 6 }, Paleta::SombraSuelo);
}

void Escenario::Pintar(const Dibujante& d) const
{
	PintarCielo(d);
	PintarColinas(d, m_colinasLejos, m_lejos, Paleta::ColinaLejana);
	PintarColinas(d, m_colinasCerca, m_cerca, Paleta::ColinaCercana);
	PintarSuelo(d);
}
