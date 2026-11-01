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

void Pintar(const Dibujante& d, const Partida& p, int ancho, int alto)
{
	d.Texto(10, 8, TextoPuntos(p), Colores::Blanco, 2);
	d.Texto(10, 26, TextoVelocidad(p), Colores::Verde, 1);

	const std::string mejor = TextoMejor(p);
	d.Texto(ancho - 10 - Dibujante::AnchoTexto(mejor, 2), 8, mejor, Colores::Amarillo, 2);

	if (p.Estado() == EstadoPartida::GameOver)
	{
		const std::string t1 = "GAME OVER";
		const std::string t2 = "ESPACIO: REINICIAR";
		d.Texto((ancho - Dibujante::AnchoTexto(t1, 5)) / 2, alto / 2 - 60, t1, Colores::Rojo, 5);
		d.Texto((ancho - Dibujante::AnchoTexto(t2, 2)) / 2, alto / 2 - 10, t2, Colores::Blanco, 2);
	}
	else if (p.Puntos() < 40)   // instrucciones al empezar (~1,3 s)
	{
		const std::string t = "ESPACIO / FLECHA ARRIBA: SALTAR";
		d.Texto((ancho - Dibujante::AnchoTexto(t, 2)) / 2, alto / 2 - 60, t, Colores::Gris, 2);
	}
}
} // namespace Hud
