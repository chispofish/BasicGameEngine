//------------------------------------------------------------------------------------
// Textura.cpp
//------------------------------------------------------------------------------------
#include "Textura.h"

#include <cstdint>
#include <cstring>

namespace
{
std::string g_ultimoError;

struct EliminaSuperficie
{
	void operator()(SDL_Surface* s) const noexcept { SDL_FreeSurface(s); }
};
using SuperficiePtr = std::unique_ptr<SDL_Surface, EliminaSuperficie>;

std::nullptr_t Fallo(const std::string& contexto)
{
	g_ultimoError = contexto + ": " + SDL_GetError();
	return nullptr;
}
} // namespace

const std::string& Textura::UltimoError() noexcept { return g_ultimoError; }

std::unique_ptr<Textura> Textura::Cargar(SDL_Renderer* renderizador, const std::string& ruta,
                                         std::optional<Color> colorClave)
{
	g_ultimoError.clear();
	if (ruta.empty()) { g_ultimoError = "ruta vacia"; return nullptr; }

	SuperficiePtr bmp(SDL_LoadBMP(ruta.c_str()));
	if (!bmp) return Fallo("no se pudo cargar '" + ruta + "'");
	return DesdeSuperficie(renderizador, bmp.get(), colorClave);
}

std::unique_ptr<Textura> Textura::DesdeSuperficie(SDL_Renderer* renderizador, SDL_Surface* superficie,
                                                  std::optional<Color> colorClave)
{
	g_ultimoError.clear();
	if (!renderizador || !superficie) { g_ultimoError = "renderizador o superficie nulos"; return nullptr; }

	// Formato canonico con alfa: independiente de la profundidad del BMP original
	SuperficiePtr argb(SDL_ConvertSurfaceFormat(superficie, SDL_PIXELFORMAT_ARGB8888, 0));
	if (!argb) return Fallo("conversion a ARGB8888");

	// Color clave -> alfa 0 (los demas pixeles conservan su alfa)
	if (colorClave)
	{
		const std::uint32_t clave = (std::uint32_t(colorClave->r) << 16) | (std::uint32_t(colorClave->g) << 8) |
		                            std::uint32_t(colorClave->b);
		if (SDL_LockSurface(argb.get()) != 0) return Fallo("bloqueo de superficie");
		auto* fila = static_cast<std::uint8_t*>(argb->pixels);
		for (int y = 0; y < argb->h; ++y, fila += argb->pitch)
		{
			auto* px = reinterpret_cast<std::uint32_t*>(fila);
			for (int x = 0; x < argb->w; ++x)
				if ((px[x] & 0x00FFFFFFu) == clave) px[x] = 0;   // totalmente transparente
		}
		SDL_UnlockSurface(argb.get());
	}

	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderizador, argb.get());
	if (!tex) return Fallo("SDL_CreateTextureFromSurface");
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
	return std::unique_ptr<Textura>(new Textura(tex, argb->w, argb->h));
}

void Textura::Dibujar(SDL_Renderer* renderizador, const SDL_Rect* origen, const SDL_Rect& destino,
                      bool espejoHorizontal) const
{
	SDL_RenderCopyEx(renderizador, m_textura.get(), origen, &destino, 0.0, nullptr,
	                 espejoHorizontal ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}
