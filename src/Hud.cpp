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

	const std::string sonido = op.sonidoActivo ? "M: SONIDO SI" : "M: SONIDO NO";
	TextoSombra(d, ancho - 8 - Dibujante::AnchoTexto(sonido, 1), alto - 14, sonido, Paleta::TextoClaro, 1);

	if (p.Estado() == EstadoPartida::GameOver)
	{
		const std::string t1 = "GAME OVER";
		const std::string t2 = "ESPACIO: REINICIAR";
		TextoSombra(d, (ancho - Dibujante::AnchoTexto(t1, 5)) / 2, alto / 2 - 60, t1, Paleta::BugCuerpo, 5);
		TextoSombra(d, (ancho - Dibujante::AnchoTexto(t2, 2)) / 2, alto / 2 - 10, t2, Paleta::TextoClaro, 2);
	}
	else if (p.Puntos() < 40)   // instrucciones al empezar (~1,3 s)
	{
		const std::string t = "ESPACIO / FLECHA ARRIBA: SALTAR";
		TextoSombra(d, (ancho - Dibujante::AnchoTexto(t, 2)) / 2, alto / 2 - 60, t, Paleta::TextoClaro, 2);
	}
}
} // namespace Hud
