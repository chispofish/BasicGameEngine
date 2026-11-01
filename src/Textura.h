//------------------------------------------------------------------------------------
// Textura  (Juego Runner - Modulo 1: Gestion de texturas y assets graficos)
// Encabezado C++ - Textura.h
//------------------------------------------------------------------------------------
// Envoltorio RAII de SDL_Texture mas un cargador de imagenes BMP basado solo en SDL2
// (sin SDL_image: no anade dependencias ni al build nativo ni al de Emscripten).
//
// Transparencia: la superficie se convierte siempre a ARGB8888 y, si se indica un
// "color clave", todos los pixeles de ese color pasan a alfa 0. Si el BMP ya trae
// canal alfa (32 bits) se respeta tal cual. La textura se crea con SDL_BLENDMODE_BLEND.
//
// La textura pertenece al SDL_Renderer que la creo: debe destruirse antes que el.
//------------------------------------------------------------------------------------
#pragma once

#include "Dibujante.h"

#include <SDL.h>

#include <memory>
#include <optional>
#include <string>

// Magenta puro: convencion clasica de "color transparente" en pixel art
inline constexpr Color kColorClaveMagenta{ 255, 0, 255, 255 };

class Textura
{
public:
	// Carga un BMP desde disco. Devuelve nullptr (y deja el motivo en UltimoError())
	// si la ruta no existe, el archivo no es un BMP valido o falla la creacion.
	static std::unique_ptr<Textura> Cargar(SDL_Renderer* renderizador, const std::string& ruta,
	                                       std::optional<Color> colorClave = kColorClaveMagenta);

	// Crea una textura desde una superficie propia (no la libera). nullptr si falla.
	static std::unique_ptr<Textura> DesdeSuperficie(SDL_Renderer* renderizador, SDL_Surface* superficie,
	                                                std::optional<Color> colorClave = std::nullopt);

	~Textura() = default;
	Textura(const Textura&)            = delete;
	Textura& operator=(const Textura&) = delete;
	Textura(Textura&&)                 = default;
	Textura& operator=(Textura&&)      = default;

	// Dibuja el recorte 'origen' (nullptr = toda la textura) en 'destino'.
	void Dibujar(SDL_Renderer* renderizador, const SDL_Rect* origen, const SDL_Rect& destino,
	             bool espejoHorizontal = false) const;

	int Ancho()  const noexcept { return m_ancho; }
	int Altura() const noexcept { return m_altura; }
	SDL_Texture* Sdl() const noexcept { return m_textura.get(); }

	// Motivo del ultimo fallo de Cargar/DesdeSuperficie (vacio si no hubo ninguno)
	static const std::string& UltimoError() noexcept;

private:
	struct EliminaTextura
	{
		void operator()(SDL_Texture* t) const noexcept { SDL_DestroyTexture(t); }
	};

	Textura(SDL_Texture* textura, int ancho, int altura) noexcept
	    : m_textura(textura), m_ancho(ancho), m_altura(altura) {}

	std::unique_ptr<SDL_Texture, EliminaTextura> m_textura;
	int m_ancho  = 0;
	int m_altura = 0;
};
