//------------------------------------------------------------------------------------
// Sonido  (Juego Runner - Modulo 4: audio)
// Encabezado C++ - Sonido.h
//------------------------------------------------------------------------------------
// Audio con SDL2 puro, sin SDL_mixer ni archivos: los efectos y la musica se SINTETIZAN
// al arrancar (ondas cuadrada/triangular/sierra y ruido, a 22,05 kHz mono 16 bits).
//   Sintesis   funciones puras que generan las muestras (se prueban sin dispositivo)
//   Mezclador  suma voces y bucle de musica en un buffer; sin SDL, determinista
//   Sonido     abre el dispositivo SDL y alimenta el mezclador desde el hilo de audio
// Si no hay dispositivo de audio, Sonido::Abrir() devuelve nullptr y el juego sigue mudo.
//------------------------------------------------------------------------------------
#pragma once

#include <SDL.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

enum class Efecto { Salto, Aterrizaje, Golpe, Hito, Inicio };
inline constexpr int kNumEfectos = 5;

namespace Sintesis
{
	inline constexpr int kFrecuencia = 22050;     // Hz

	std::vector<std::int16_t> Generar(Efecto e);  // un efecto (deterministas: mismas muestras siempre)
	std::vector<std::int16_t> Musica();           // bucle sin cortes de 4 s
}

class Mezclador
{
public:
	static constexpr int kVoces = 8;

	Mezclador();

	// Registra las muestras de cada efecto (antes de reproducir) y la musica en bucle
	void Registrar(Efecto e, std::vector<std::int16_t> muestras);
	void EstablecerMusica(std::vector<std::int16_t> muestras);

	// Inicia un efecto. Sin voz libre sustituye a la mas avanzada. En silencio no hace nada.
	void Reproducir(Efecto e);
	void PausarMusica(bool pausa) noexcept { m_musicaPausada = pausa; }
	void Silenciar(bool silencio) noexcept;
	bool Silenciado() const noexcept { return m_silencio; }

	// Rellena 'n' muestras de salida (suma con saturacion, nunca desborda)
	void Mezclar(std::int16_t* salida, int n) noexcept;

	int VocesActivas() const noexcept;

	double GananciaEfectos = 0.7;
	double GananciaMusica  = 0.28;

private:
	struct Voz { const std::vector<std::int16_t>* muestras = nullptr; std::size_t pos = 0; };

	std::array<std::vector<std::int16_t>, kNumEfectos> m_efectos;
	std::vector<std::int16_t> m_musica;
	std::array<Voz, kVoces> m_voces{};
	std::size_t m_posMusica = 0;
	bool m_musicaPausada = false;
	bool m_silencio = false;
};

class Sonido
{
public:
	// nullptr si no se puede abrir un dispositivo de audio (sin tarjeta, navegador sin permiso...)
	static std::unique_ptr<Sonido> Abrir();
	~Sonido();
	Sonido(const Sonido&) = delete;
	Sonido& operator=(const Sonido&) = delete;

	void Reproducir(Efecto e);
	void PausarMusica(bool pausa);
	bool AlternarSilencio();             // devuelve true si queda en silencio
	bool Silenciado() const;

private:
	Sonido() = default;
	static void Callback(void* datos, Uint8* flujo, int longitud);

	SDL_AudioDeviceID m_dispositivo = 0;
	Mezclador m_mezclador;
};
