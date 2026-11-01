//------------------------------------------------------------------------------------
// Hud.cpp
//------------------------------------------------------------------------------------
#include "Hud.h"

#include <cstdio>

namespace Hud
{
std::string TextoPuntos(const Partida& p)
{
	char b[32];
	std::snprintf(b, sizeof b, "PUNTOS: %05d", p.Puntos());
	return b;
}
std::string TextoMejor(const Partida& p)
{
	char b[32];
	std::snprintf(b, sizeof b, "MEJOR: %05d", p.Mejor());
	return b;
}
std::string TextoVelocidad(const Partida& p)
{
	char b[32];
	std::snprintf(b, sizeof b, "VEL: %03d", static_cast<int>(p.Velocidad()));
	return b;
}

std::string TextoInstrucciones(bool tactil)
{
	return tactil ? "TOCA LA PANTALLA PARA SALTAR" : "ESPACIO / FLECHA ARRIBA: SALTAR";
}
std::string TextoReinicio(bool tactil) { return tactil ? "TOCA PARA REINICIAR" : "ESPACIO: REINICIAR"; }

SDL_Rect RectBotonSonido(int ancho, int alto)
{
	// 132x56: tras escalar el lienzo 640x480 a un movil sigue siendo comodo de pulsar
	return SDL_Rect{ ancho - 142, alto - 66, 132, 56 };
}

int TextoSombra(const Dibujante& d, int x, int y, const std::string& t, Color c, int escala)
{
	d.Texto(x + escala, y + escala, t, Paleta::TextoSombra, escala);
	return d.Texto(x, y, t, c, escala);
}

void Pintar(const Dibujante& d, const Partida& p, int ancho, int alto, const Opciones& op)
{
	TextoSombra(d, 10, 8, TextoPuntos(p), op.resaltarPuntos ? Paleta::Dorado : Paleta::TextoClaro, 2);
	TextoSombra(d, 10, 26, TextoVelocidad(p), Paleta::Verde, 1);

	const std::string mejor = TextoMejor(p);
	TextoSombra(d, ancho - 10 - Dibujante::AnchoTexto(mejor, 2), 8, mejor, Paleta::Dorado, 2);

	if (op.tactil)
	{
		const SDL_Rect b = RectBotonSonido(ancho, alto);
		d.RectanguloRelleno(b, Paleta::Contorno);
		d.RectanguloRelleno(SDL_Rect{ b.x + 3, b.y + 3, b.w - 6, b.h - 6 }, Paleta::SueloOscuro);
		d.Rectangulo(SDL_Rect{ b.x + 3, b.y + 3, b.w - 6, b.h - 6 }, op.sonidoActivo ? Paleta::HierbaClara : Paleta::BugSombra);
		const std::string t1 = "SONIDO", t2 = op.sonidoActivo ? "SI" : "NO";
		d.Texto(b.x + (b.w - Dibujante::AnchoTexto(t1, 2)) / 2, b.y + 10, t1, Paleta::TextoClaro, 2);
		d.Texto(b.x + (b.w - Dibujante::AnchoTexto(t2, 2)) / 2, b.y + 32, t2, op.sonidoActivo ? Paleta::Verde : Paleta::BugCuerpo, 2);
	}
	else
	{
		const std::string sonido = op.sonidoActivo ? "M: SONIDO SI" : "M: SONIDO NO";
		TextoSombra(d, ancho - 8 - Dibujante::AnchoTexto(sonido, 1), alto - 14, sonido, Paleta::TextoClaro, 1);
	}

	if (p.Estado() == EstadoPartida::GameOver)
	{
		const std::string t1 = "GAME OVER";
		const std::string t2 = TextoReinicio(op.tactil);
		TextoSombra(d, (ancho - Dibujante::AnchoTexto(t1, 5)) / 2, alto / 2 - 60, t1, Paleta::BugCuerpo, 5);
		TextoSombra(d, (ancho - Dibujante::AnchoTexto(t2, 2)) / 2, alto / 2 - 10, t2, Paleta::TextoClaro, 2);
	}
	else if (p.Puntos() < 40)   // instrucciones al empezar (~1,3 s)
	{
		const std::string t = TextoInstrucciones(op.tactil);
		TextoSombra(d, (ancho - Dibujante::AnchoTexto(t, 2)) / 2, alto / 2 - 60, t, Paleta::TextoClaro, 2);
	}
}
} // namespace Hud
