//------------------------------------------------------------------------------------
// Dibujante  (Modulo 3: Renderizado 2D y helpers graficos)
// Encabezado C++ - Dibujante.h
//------------------------------------------------------------------------------------
// Capa fina y sin propiedad sobre SDL_Renderer: primitivas (punto, linea, rectangulo,
// rectangulo relleno) y texto con una fuente bitmap 5x7 incorporada (no necesita
// SDL_ttf ni archivos de recursos, asi funciona igual en nativo y en Emscripten).
// Es barata de construir: crear una instancia por frame dentro de Juego::Pintar.
//------------------------------------------------------------------------------------
#pragma once

#include <SDL.h>

#include <cstdint>
#include <string>

struct Color
{
	std::uint8_t r = 0, g = 0, b = 0, a = 255;
};

namespace Colores
{
	inline constexpr Color Negro    {   0,   0,   0, 255 };
	inline constexpr Color Blanco   { 255, 255, 255, 255 };
	inline constexpr Color Rojo     { 220,  50,  47, 255 };
	inline constexpr Color Verde    {  80, 200, 120, 255 };
	inline constexpr Color Azul     {  60, 120, 220, 255 };
	inline constexpr Color Amarillo { 240, 180,  40, 255 };
	inline constexpr Color Gris     { 140, 140, 150, 255 };
	inline constexpr Color Fondo    {  20,  24,  40, 255 };
}

class Dibujante
{
public:
	// Ancho / alto de una celda de la fuente bitmap (en pixeles, escala 1)
	static constexpr int kAnchoGlifo = 5;
	static constexpr int kAltoGlifo  = 7;

	explicit Dibujante(SDL_Renderer* renderizador) noexcept : m_r(renderizador) {}

	// Limpia TODO el back buffer con un color plano (SetRenderDrawColor + RenderClear)
	void Limpiar(Color c) const;

	void Punto(int x, int y, Color c) const;
	void Linea(int x1, int y1, int x2, int y2, Color c) const;
	void Rectangulo(const SDL_Rect& r, Color c) const;          // solo el borde
	void RectanguloRelleno(const SDL_Rect& r, Color c) const;

	// Texto con fuente 5x7. Soporta A-Z (las minusculas se dibujan como mayusculas),
	// 0-9 y . , : - + / % ( ) ! ? = _ y espacio; '\n' baja de linea.
	// Devuelve el ancho dibujado de la linea mas larga.
	int Texto(int x, int y, const std::string& texto, Color c, int escala = 1) const;

	// Ancho en pixeles que ocuparia 'texto' (linea mas larga) con esa escala
	static int AnchoTexto(const std::string& texto, int escala = 1) noexcept;

private:
	void Color_(Color c) const;
	SDL_Renderer* m_r;
};
