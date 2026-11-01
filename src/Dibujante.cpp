//------------------------------------------------------------------------------------
// Dibujante  (Modulo 3: Renderizado 2D y helpers graficos)
// Codigo fuente C++ - Dibujante.cpp
//------------------------------------------------------------------------------------
#include "Dibujante.h"

#include <algorithm>
#include <cctype>
#include <vector>

namespace
{
// Fuente 5x7: cada glifo son 7 filas de 5 bits (bit 4 = columna izquierda).
struct Glifo
{
	char         caracter;
	std::uint8_t filas[Dibujante::kAltoGlifo];
};

constexpr Glifo kFuente[] = {
	{ '0', { 0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110 } },
	{ '1', { 0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 } },
	{ '2', { 0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111 } },
	{ '3', { 0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110 } },
	{ '4', { 0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010 } },
	{ '5', { 0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110 } },
	{ '6', { 0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110 } },
	{ '7', { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000 } },
	{ '8', { 0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110 } },
	{ '9', { 0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100 } },
	{ 'A', { 0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 } },
	{ 'B', { 0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110 } },
	{ 'C', { 0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110 } },
	{ 'D', { 0b11100, 0b10010, 0b10001, 0b10001, 0b10001, 0b10010, 0b11100 } },
	{ 'E', { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111 } },
	{ 'F', { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000 } },
	{ 'G', { 0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111 } },
	{ 'H', { 0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 } },
	{ 'I', { 0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 } },
	{ 'J', { 0b00111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100 } },
	{ 'K', { 0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001 } },
	{ 'L', { 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111 } },
	{ 'M', { 0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001 } },
	{ 'N', { 0b10001, 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001 } },
	{ 'O', { 0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 } },
	{ 'P', { 0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000 } },
	{ 'Q', { 0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101 } },
	{ 'R', { 0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001 } },
	{ 'S', { 0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110 } },
	{ 'T', { 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 } },
	{ 'U', { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 } },
	{ 'V', { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100 } },
	{ 'W', { 0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010 } },
	{ 'X', { 0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001 } },
	{ 'Y', { 0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100 } },
	{ 'Z', { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111 } },
	{ '.', { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b01100, 0b01100 } },
	{ ',', { 0b00000, 0b00000, 0b00000, 0b00000, 0b01100, 0b00100, 0b01000 } },
	{ ':', { 0b00000, 0b01100, 0b01100, 0b00000, 0b01100, 0b01100, 0b00000 } },
	{ '-', { 0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000 } },
	{ '+', { 0b00000, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00000 } },
	{ '/', { 0b00001, 0b00010, 0b00010, 0b00100, 0b01000, 0b01000, 0b10000 } },
	{ '%', { 0b11001, 0b11010, 0b00010, 0b00100, 0b01000, 0b01011, 0b10011 } },
	{ '(', { 0b00010, 0b00100, 0b01000, 0b01000, 0b01000, 0b00100, 0b00010 } },
	{ ')', { 0b01000, 0b00100, 0b00010, 0b00010, 0b00010, 0b00100, 0b01000 } },
	{ '!', { 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b00100 } },
	{ '?', { 0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b00000, 0b00100 } },
	{ '=', { 0b00000, 0b00000, 0b11111, 0b00000, 0b11111, 0b00000, 0b00000 } },
	{ '_', { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111 } },
};

// Glifo para un caracter; nullptr si no esta en la fuente (el espacio no dibuja nada)
const Glifo* BuscarGlifo(char c) noexcept
{
	const char mayuscula = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	for (const Glifo& g : kFuente)
		if (g.caracter == mayuscula)
			return &g;
	return nullptr;
}

constexpr int kSeparacion = 1;   // pixeles (escala 1) entre glifos
} // namespace

//------------------------------------------------------------------------------------
// Primitivas
//------------------------------------------------------------------------------------
void Dibujante::Color_(Color c) const
{
	SDL_SetRenderDrawColor(m_r, c.r, c.g, c.b, c.a);
}

void Dibujante::Limpiar(Color c) const
{
	Color_(c);
	SDL_RenderClear(m_r);
}

void Dibujante::Punto(int x, int y, Color c) const
{
	Color_(c);
	SDL_RenderDrawPoint(m_r, x, y);
}

void Dibujante::Linea(int x1, int y1, int x2, int y2, Color c) const
{
	Color_(c);
	SDL_RenderDrawLine(m_r, x1, y1, x2, y2);
}

void Dibujante::Rectangulo(const SDL_Rect& r, Color c) const
{
	Color_(c);
	SDL_RenderDrawRect(m_r, &r);
}

void Dibujante::RectanguloRelleno(const SDL_Rect& r, Color c) const
{
	Color_(c);
	SDL_RenderFillRect(m_r, &r);
}

//------------------------------------------------------------------------------------
// Texto
//------------------------------------------------------------------------------------
int Dibujante::AnchoTexto(const std::string& texto, int escala) noexcept
{
	int maximo = 0, actual = 0;
	for (char c : texto)
	{
		if (c == '\n') { maximo = std::max(maximo, actual); actual = 0; continue; }
		actual += kAnchoGlifo + kSeparacion;
	}
	maximo = std::max(maximo, actual);
	return maximo > 0 ? (maximo - kSeparacion) * escala : 0;
}

int Dibujante::Texto(int x, int y, const std::string& texto, Color c, int escala) const
{
	if (escala < 1)
		escala = 1;

	std::vector<SDL_Rect> pixeles;       // un unico SDL_RenderFillRects para todo el texto
	pixeles.reserve(texto.size() * 16);

	int cx = x, cy = y;
	for (char ch : texto)
	{
		if (ch == '\n')
		{
			cx = x;
			cy += (kAltoGlifo + 2) * escala;
			continue;
		}
		if (const Glifo* g = BuscarGlifo(ch))
		{
			for (int fila = 0; fila < kAltoGlifo; ++fila)
				for (int col = 0; col < kAnchoGlifo; ++col)
					if (g->filas[fila] & (1 << (kAnchoGlifo - 1 - col)))
						pixeles.push_back(SDL_Rect{ cx + col * escala, cy + fila * escala, escala, escala });
		}
		cx += (kAnchoGlifo + kSeparacion) * escala;
	}

	if (!pixeles.empty())
	{
		Color_(c);
		SDL_RenderFillRects(m_r, pixeles.data(), static_cast<int>(pixeles.size()));
	}
	return AnchoTexto(texto, escala);
}
