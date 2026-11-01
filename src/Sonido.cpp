//------------------------------------------------------------------------------------
// Sonido.cpp
//------------------------------------------------------------------------------------
#include "Sonido.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kFs = Sintesis::kFrecuencia;

enum class Onda { Cuadrada, Triangular, Sierra };

// Buffer de trabajo en coma flotante con osciladores simples
class Lienzo
{
public:
	explicit Lienzo(double segundos) : m_b(static_cast<std::size_t>(segundos * kFs), 0.f) {}

	// Tono con barrido de frecuencia f0 -> f1 y caida exponencial (constante 'decay' en s)
	void Tono(double t0, double dur, double f0, double f1, Onda onda, double vol, double decay, double duty = 0.5)
	{
		const std::size_t i0 = static_cast<std::size_t>(t0 * kFs), n = static_cast<std::size_t>(dur * kFs);
		double fase = 0;
		for (std::size_t i = 0; i < n && i0 + i < m_b.size(); ++i)
		{
			const double t = double(i) / n;
			fase += (f0 + (f1 - f0) * t) / kFs;
			fase -= std::floor(fase);
			double v = 0;
			switch (onda)
			{
			case Onda::Cuadrada:   // media cero para cualquier ciclo de trabajo (sin componente continua)
				v = (fase < duty ? 2.0 * (1.0 - duty) : -2.0 * duty) / std::max(2.0 * (1.0 - duty), 2.0 * duty);
				break;
			case Onda::Triangular: v = 4.0 * std::fabs(fase - 0.5) - 1.0; break;
			case Onda::Sierra:     v = 2.0 * fase - 1.0; break;
			}
			const double liberacion = std::min(1.0, double(n - i) / (0.004 * kFs));      // 4 ms finales a 0
			m_b[i0 + i] += static_cast<float>(v * vol * liberacion * std::exp(-double(i) / kFs / decay));
		}
	}

	// Ruido blanco filtrado (paso bajo de un polo: 'suave' en (0,1], menor = mas grave)
	void Ruido(double t0, double dur, double vol, double decay, double suave, std::uint32_t semilla = 12345u)
	{
		const std::size_t i0 = static_cast<std::size_t>(t0 * kFs), n = static_cast<std::size_t>(dur * kFs);
		std::uint32_t x = semilla;
		double y = 0;
		for (std::size_t i = 0; i < n && i0 + i < m_b.size(); ++i)
		{
			x ^= x << 13; x ^= x >> 17; x ^= x << 5;                     // xorshift32
			const double r = (double(x) / 4294967295.0) * 2.0 - 1.0;
			y += suave * (r - y);
			m_b[i0 + i] += static_cast<float>(y * vol * std::exp(-double(i) / kFs / decay));
		}
	}

	// A 16 bits: fundidos de 3 ms (sin clics) y pico normalizado a 'pico' (0..1)
	std::vector<std::int16_t> Terminar(double pico, bool fundidoFinal = true)
	{
		float max = 1e-9f;
		for (float v : m_b) max = std::max(max, std::fabs(v));
		const double k = pico / max;
		const std::size_t f = static_cast<std::size_t>(0.003 * kFs);
		std::vector<std::int16_t> r(m_b.size());
		for (std::size_t i = 0; i < m_b.size(); ++i)
		{
			double g = 1.0;
			if (i < f) g = double(i) / f;
			if (fundidoFinal && m_b.size() - 1 - i < f) g = std::min(g, double(m_b.size() - 1 - i) / f);
			r[i] = static_cast<std::int16_t>(std::lround(m_b[i] * k * g * 32767.0));
		}
		return r;
	}

private:
	std::vector<float> m_b;
};
} // namespace

namespace Sintesis
{
std::vector<std::int16_t> Generar(Efecto e)
{
	switch (e)
	{
	case Efecto::Salto:
	{
		Lienzo l(0.24);
		l.Tono(0, 0.22, 260, 640, Onda::Cuadrada, 1.0, 0.12);                  // "boing" ascendente
		return l.Terminar(0.55);
	}
	case Efecto::Aterrizaje:
	{
		Lienzo l(0.12);
		l.Tono(0, 0.10, 150, 55, Onda::Triangular, 1.0, 0.04);                 // golpe sordo
		l.Ruido(0, 0.07, 0.5, 0.03, 0.25);
		return l.Terminar(0.6);
	}
	case Efecto::Golpe:
	{
		Lienzo l(0.7);
		l.Ruido(0, 0.5, 1.0, 0.16, 0.18, 777u);                               // crujido
		l.Tono(0, 0.65, 330, 50, Onda::Sierra, 0.7, 0.22);                     // caida de tono
		return l.Terminar(0.8);
	}
	case Efecto::Hito:
	{
		Lienzo l(0.34);
		l.Tono(0.00, 0.10, 880, 880, Onda::Cuadrada, 1.0, 0.07, 0.25);        // "ding-ding"
		l.Tono(0.09, 0.24, 1320, 1320, Onda::Cuadrada, 1.0, 0.10, 0.25);
		return l.Terminar(0.5);
	}
	case Efecto::Inicio:
	{
		Lienzo l(0.34);
		const double notas[] = { 440.0, 554.4, 659.3 };                        // La mayor ascendente
		for (int i = 0; i < 3; ++i) l.Tono(0.08 * i, 0.12, notas[i], notas[i], Onda::Triangular, 1.0, 0.09);
		return l.Terminar(0.6);
	}
	}
	return {};
}

std::vector<std::int16_t> Musica()
{
	// 16 corcheas a 120 ppm = 4 s. La Menor pentatonica: bajo triangular + arpegio cuadrado
	constexpr double paso = 0.25;
	const double bajo[16] = { 110, 110, 164.8, 110,  130.8, 130.8, 196, 130.8,  146.8, 146.8, 220, 146.8,  98, 98, 146.8, 196 };
	const double arpa[16] = { 440, 523.3, 659.3, 523.3,  523.3, 659.3, 784, 659.3,  587.3, 740, 880, 740,  392, 494, 587.3, 784 };
	Lienzo l(16 * paso);
	for (int i = 0; i < 16; ++i)
	{
		l.Tono(i * paso, paso * 0.9, bajo[i], bajo[i], Onda::Triangular, 0.9, 0.35);
		if (i % 2 == 0 || i % 4 == 3) l.Tono(i * paso, paso * 0.8, arpa[i], arpa[i], Onda::Cuadrada, 0.22, 0.12, 0.125);
		if (i % 4 == 2) l.Ruido(i * paso, 0.05, 0.25, 0.02, 0.6, 99u + i);            // charles
	}
	return l.Terminar(0.9, false);    // sin fundido al final: el bucle enlaza (empieza y acaba en ~0)
}
} // namespace Sintesis

// -------------------------------------------------------------------------------- Mezclador
Mezclador::Mezclador() = default;

void Mezclador::Registrar(Efecto e, std::vector<std::int16_t> muestras)
{
	m_efectos[static_cast<std::size_t>(e)] = std::move(muestras);
}

void Mezclador::EstablecerMusica(std::vector<std::int16_t> muestras)
{
	m_musica = std::move(muestras);
	m_posMusica = 0;
}

void Mezclador::Silenciar(bool silencio) noexcept
{
	m_silencio = silencio;
	if (silencio) for (Voz& v : m_voces) v = Voz{};     // corta lo que estuviera sonando
}

void Mezclador::Reproducir(Efecto e)
{
	const auto& m = m_efectos[static_cast<std::size_t>(e)];
	if (m_silencio || m.empty()) return;
	Voz* destino = nullptr;
	for (Voz& v : m_voces) if (!v.muestras) { destino = &v; break; }
	if (!destino)                                          // todas ocupadas: se roba la mas avanzada
		destino = &*std::max_element(m_voces.begin(), m_voces.end(), [](const Voz& a, const Voz& b) { return a.pos < b.pos; });
	*destino = Voz{ &m, 0 };
}

int Mezclador::VocesActivas() const noexcept
{
	return static_cast<int>(std::count_if(m_voces.begin(), m_voces.end(), [](const Voz& v) { return v.muestras != nullptr; }));
}

void Mezclador::Mezclar(std::int16_t* salida, int n) noexcept
{
	for (int i = 0; i < n; ++i) salida[i] = 0;
	if (m_silencio) return;

	if (!m_musicaPausada && !m_musica.empty())
		for (int i = 0; i < n; ++i)
		{
			salida[i] = static_cast<std::int16_t>(std::lround(m_musica[m_posMusica] * GananciaMusica));
			if (++m_posMusica >= m_musica.size()) m_posMusica = 0;      // bucle
		}

	for (Voz& v : m_voces)
	{
		if (!v.muestras) continue;
		for (int i = 0; i < n && v.pos < v.muestras->size(); ++i, ++v.pos)
		{
			const long suma = salida[i] + std::lround((*v.muestras)[v.pos] * GananciaEfectos);
			salida[i] = static_cast<std::int16_t>(std::clamp<long>(suma, -32768, 32767));
		}
		if (v.pos >= v.muestras->size()) v = Voz{};
	}
}

// -------------------------------------------------------------------------------- Sonido
std::unique_ptr<Sonido> Sonido::Abrir()
{
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return nullptr;

	std::unique_ptr<Sonido> s(new Sonido());
	for (int i = 0; i < kNumEfectos; ++i) s->m_mezclador.Registrar(static_cast<Efecto>(i), Sintesis::Generar(static_cast<Efecto>(i)));
	s->m_mezclador.EstablecerMusica(Sintesis::Musica());

	SDL_AudioSpec pedido{};
	pedido.freq = Sintesis::kFrecuencia;
	pedido.format = AUDIO_S16SYS;
	pedido.channels = 1;
	pedido.samples = 1024;
	pedido.callback = &Sonido::Callback;
	pedido.userdata = s.get();
	SDL_AudioSpec obtenido{};
	// allowed_changes = 0: SDL convierte por su cuenta al formato real del dispositivo
	s->m_dispositivo = SDL_OpenAudioDevice(nullptr, 0, &pedido, &obtenido, 0);
	if (s->m_dispositivo == 0)
	{
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return nullptr;
	}
	SDL_PauseAudioDevice(s->m_dispositivo, 0);
	return s;
}

Sonido::~Sonido()
{
	if (m_dispositivo != 0)
	{
		SDL_CloseAudioDevice(m_dispositivo);       // espera a que termine el callback en curso
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	}
}

void Sonido::Callback(void* datos, Uint8* flujo, int longitud)
{
	static_cast<Sonido*>(datos)->m_mezclador.Mezclar(reinterpret_cast<std::int16_t*>(flujo), longitud / 2);
}

void Sonido::Reproducir(Efecto e)
{
	SDL_LockAudioDevice(m_dispositivo);
	m_mezclador.Reproducir(e);
	SDL_UnlockAudioDevice(m_dispositivo);
}

void Sonido::PausarMusica(bool pausa)
{
	SDL_LockAudioDevice(m_dispositivo);
	m_mezclador.PausarMusica(pausa);
	SDL_UnlockAudioDevice(m_dispositivo);
}

bool Sonido::AlternarSilencio()
{
	SDL_LockAudioDevice(m_dispositivo);
	m_mezclador.Silenciar(!m_mezclador.Silenciado());
	const bool r = m_mezclador.Silenciado();
	SDL_UnlockAudioDevice(m_dispositivo);
	return r;
}

bool Sonido::Silenciado() const
{
	SDL_LockAudioDevice(m_dispositivo);
	const bool r = m_mezclador.Silenciado();
	SDL_UnlockAudioDevice(m_dispositivo);
	return r;
}
