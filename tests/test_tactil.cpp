//------------------------------------------------------------------------------------
// Pruebas de QA del juego Runner - Modulo 5 (control para movil)
// Se ejecutan sin pantalla: SDL_VIDEODRIVER=dummy, SDL_RENDER_DRIVER=software.
//   Test 5.1  Ventana: dedos activos, coordenadas, multitoque y toques rapidos
//   Test 5.2  ControlTactil: salto, buffer, boton de sonido, dedo mantenido
//   Test 5.3  HUD tactil: textos, boton visible y tamano comodo para un dedo
//   Test 5.4  Partida jugable solo con toques (saltar, GAME OVER, reiniciar)
//------------------------------------------------------------------------------------
#include "ControlTactil.h"
#include "Hud.h"
#include "Ventana.h"

#include <cmath>
#include <cstdio>
#include <vector>

static int g_fallos = 0;
#define CHECK(cond, msg)                                                           \
	do {                                                                           \
		if (cond) std::printf("  [ OK ] %s\n", msg);                               \
		else { std::printf("  [FAIL] %s  (%s:%d)\n", msg, __FILE__, __LINE__); ++g_fallos; } \
	} while (0)

namespace
{
constexpr double kDt = 1.0 / 60.0;
constexpr int kAncho = 640, kAlto = 480;

void Dedo(Uint32 tipo, std::int64_t id, float x, float y)       // coordenadas normalizadas 0..1, como SDL
{
	SDL_Event e{};
	e.type = tipo;
	e.tfinger.fingerId = id;
	e.tfinger.x = x; e.tfinger.y = y;
	SDL_PushEvent(&e);
}
void Tecla(Uint32 tipo, SDL_Scancode sc)
{
	SDL_Event e{};
	e.type = tipo; e.key.keysym.scancode = sc;
	SDL_PushEvent(&e);
}

// ---- Test 5.1 -------------------------------------------------------------------
void Test_5_1()
{
	std::puts("Test 5.1: Ventana y dedos");
	Ventana v(ConfigVentana{ "tactil", kAncho, kAlto, false, true });
	CHECK(v.ToquesActivos().empty() && v.TomarToquesNuevos().empty() && !v.UltimaEntradaTactil(), "al empezar no hay toques ni modo tactil");

	Dedo(SDL_FINGERDOWN, 1, 0.5f, 0.25f);
	v.ProcesarEventos();
	CHECK(v.ToquesActivos().size() == 1 && v.ToquesActivos()[0].x == 320 && v.ToquesActivos()[0].y == 120,
	      "un dedo en (0,5; 0,25) se convierte a pixeles de la ventana (320, 120)");
	CHECK(v.UltimaEntradaTactil(), "tocar activa el modo tactil");
	CHECK(!v.BotonRaton(SDL_BUTTON_LEFT), "el dedo no se duplica como clic de raton");

	Dedo(SDL_FINGERMOTION, 1, 1.0f, 1.0f);
	v.ProcesarEventos();
	CHECK(v.ToquesActivos().size() == 1 && v.ToquesActivos()[0].x == 640 && v.ToquesActivos()[0].y == 480, "al arrastrar se actualiza la posicion (sin duplicar el dedo)");

	Dedo(SDL_FINGERDOWN, 2, 0.1f, 0.1f);
	v.ProcesarEventos();
	CHECK(v.ToquesActivos().size() == 2, "multitoque: dos dedos a la vez");
	Dedo(SDL_FINGERUP, 1, 0.0f, 0.0f);
	v.ProcesarEventos();
	CHECK(v.ToquesActivos().size() == 1 && v.ToquesActivos()[0].id == 2, "al levantar un dedo solo desaparece ese");
	Dedo(SDL_FINGERUP, 2, 0.0f, 0.0f);
	v.ProcesarEventos();
	CHECK(v.ToquesActivos().empty(), "sin dedos, sin toques activos");

	auto nuevos = v.TomarToquesNuevos();
	CHECK(nuevos.size() == 2 && nuevos[0].x == 320 && nuevos[0].y == 120 && nuevos[1].x == 64 && nuevos[1].y == 48,
	      "los toques NUEVOS recuerdan donde se poso cada dedo, aunque ya se hayan levantado");
	CHECK(v.TomarToquesNuevos().empty(), "y se consumen: la segunda lectura sale vacia");

	// Toque rapido: poner y quitar el dedo dentro del mismo frame no se pierde
	Dedo(SDL_FINGERDOWN, 7, 0.3f, 0.3f); Dedo(SDL_FINGERUP, 7, 0.3f, 0.3f);
	v.ProcesarEventos();
	CHECK(v.ToquesActivos().empty() && v.TomarToquesNuevos().size() == 1, "un toque que empieza y acaba entre dos tics sigue contando");

	// Acotado: no crece sin limite
	for (int i = 0; i < 100; ++i) { Dedo(SDL_FINGERDOWN, 100 + i, 0.5f, 0.5f); Dedo(SDL_FINGERUP, 100 + i, 0.5f, 0.5f); }
	v.ProcesarEventos();
	CHECK(v.TomarToquesNuevos().size() <= 16, "la cola de toques nuevos esta acotada (16)");
	for (int i = 0; i < 30; ++i) Dedo(SDL_FINGERDOWN, 200 + i, 0.5f, 0.5f);
	v.ProcesarEventos();
	CHECK(v.ToquesActivos().size() <= Ventana::kMaxToques, "los dedos activos estan acotados");

	// Perder el foco suelta los dedos (evita "dedo pegado")
	SDL_Event f{}; f.type = SDL_WINDOWEVENT; f.window.event = SDL_WINDOWEVENT_FOCUS_LOST; SDL_PushEvent(&f);
	v.ProcesarEventos();
	CHECK(v.ToquesActivos().empty() && v.TomarToquesNuevos().empty(), "al perder el foco se sueltan todos los dedos");

	// El teclado devuelve al modo teclado
	Tecla(SDL_KEYDOWN, SDL_SCANCODE_SPACE);
	v.ProcesarEventos();
	CHECK(!v.UltimaEntradaTactil(), "pulsar una tecla vuelve al modo teclado");
	v.MarcarEntradaTactil(true);
	CHECK(v.UltimaEntradaTactil(), "MarcarEntradaTactil permite activar el modo desde fuera (deteccion de movil)");
}

// ---- Test 5.2 -------------------------------------------------------------------
void Test_5_2()
{
	std::puts("Test 5.2: ControlTactil");
	const SDL_Rect boton = Hud::RectBotonSonido(kAncho, kAlto);
	const SDL_Point fuera{ 200, 200 }, dentro{ boton.x + boton.w / 2, boton.y + boton.h / 2 };
	const std::vector<Toque> sinDedos;

	{
		ControlTactil c(boton);
		auto s = c.Actualizar(sinDedos, {});
		CHECK(!s.saltar && !s.alternarSonido, "sin toques no pasa nada");
		s = c.Actualizar(sinDedos, { fuera });
		CHECK(s.saltar && !s.alternarSonido, "un toque en el juego salta en el mismo tic");
		int tics = 1;
		while (c.Actualizar(sinDedos, {}).saltar) ++tics;
		CHECK(tics == 6, "el salto pendiente (buffer) dura 6 tics = 0,1 s y luego se apaga");
	}
	{
		ControlTactil c(boton);
		auto s = c.Actualizar(sinDedos, { dentro });
		CHECK(s.alternarSonido && !s.saltar, "tocar el boton de sonido alterna el sonido y NO salta");
		CHECK(!c.Actualizar(sinDedos, {}).alternarSonido, "el cambio de sonido es un flanco: solo en el tic del toque");
		s = c.Actualizar(sinDedos, { dentro, fuera });
		CHECK(s.alternarSonido && s.saltar, "dos dedos a la vez: uno alterna el sonido y el otro salta");
	}
	{
		ControlTactil c(boton);
		const std::vector<Toque> apoyado{ Toque{ 1, fuera.x, fuera.y } };
		bool siempre = true;
		for (int i = 0; i < 120; ++i) siempre = siempre && c.Actualizar(apoyado, {}).saltar;
		CHECK(siempre, "un dedo mantenido en el juego mantiene la orden de salto (como mantener la barra)");
		const std::vector<Toque> enBoton{ Toque{ 1, dentro.x, dentro.y } };
		ControlTactil d(boton);
		bool nunca = true;
		for (int i = 0; i < 30; ++i) nunca = nunca && !d.Actualizar(enBoton, {}).saltar;
		CHECK(nunca, "un dedo apoyado sobre el boton de sonido no provoca saltos");
	}
	{
		ControlTactil c(boton);
		c.Actualizar(sinDedos, { fuera });
		c.Cancelar();
		CHECK(!c.Actualizar(sinDedos, {}).saltar, "Cancelar() borra el salto pendiente (no se salta justo tras reiniciar)");
	}
}

// ---- Test 5.3 -------------------------------------------------------------------
std::vector<std::uint32_t> Leer(SDL_Renderer* r)
{
	std::vector<std::uint32_t> px(static_cast<std::size_t>(kAncho) * kAlto);
	SDL_RenderReadPixels(r, nullptr, SDL_PIXELFORMAT_ARGB8888, px.data(), kAncho * 4);
	return px;
}

void Test_5_3()
{
	std::puts("Test 5.3: HUD tactil");
	CHECK(Hud::TextoInstrucciones(true) != Hud::TextoInstrucciones(false) && Hud::TextoInstrucciones(true).find("TOCA") == 0,
	      "las instrucciones cambian a 'TOCA...' en el movil");
	CHECK(Hud::TextoInstrucciones(true).find("ESPACIO") == std::string::npos && Hud::TextoReinicio(true).find("ESPACIO") == std::string::npos,
	      "en el movil no se menciona la barra espaciadora");
	CHECK(Hud::TextoReinicio(true) == "TOCA PARA REINICIAR", "el cartel de GAME OVER pide tocar para reiniciar");

	// Textos soportados por la fuente bitmap (si no, saldrian huecos)
	bool soportado = true;
	for (char c : Hud::TextoInstrucciones(true) + Hud::TextoReinicio(true) + "SONIDO SI NO")
		if (!((c >= 'A' && c <= 'Z') || c == ' ')) soportado = false;
	CHECK(soportado, "todos los textos nuevos usan caracteres de la fuente del motor");

	const SDL_Rect b = Hud::RectBotonSonido(kAncho, kAlto);
	CHECK(b.x >= 0 && b.y >= 0 && b.x + b.w <= kAncho && b.y + b.h <= kAlto, "el boton esta dentro de la pantalla");
	CHECK(b.y >= 400, "y sobre el suelo (no tapa el area de juego)");
	// Tamano comodo: 44 px css es el minimo recomendado. En un movil en horizontal el lienzo 640x480 se
	// muestra a ~0,56x (pantalla de 360 px de alto en css) hasta 0,75x: se exige >= 44 css px a 0,75x de escala
	std::printf("         boton %dx%d px de lienzo -> %.0fx%.0f px css a escala 0,75\n", b.w, b.h, b.w * 0.75, b.h * 0.75);
	CHECK(b.w * 0.75 >= 44 && b.h * 0.75 >= 40, "el boton mide al menos ~44 px css en un movil tipico (objetivo tactil comodo)");

	Ventana v(ConfigVentana{ "hud-tactil", kAncho, kAlto, false, true });
	SDL_Renderer* r = v.Renderizador();
	Partida p;
	auto render = [&](bool tactil, bool sonido) {
		Dibujante d(r); d.Limpiar(Colores::Fondo);
		Hud::Pintar(d, p, kAncho, kAlto, Hud::Opciones{ sonido, false, tactil });
		return Leer(r);
	};
	auto contar = [&](const std::vector<std::uint32_t>& px, const SDL_Rect& rc) {
		int n = 0;
		for (int y = rc.y; y < rc.y + rc.h; ++y) for (int x = rc.x; x < rc.x + rc.w; ++x)
			n += px[static_cast<std::size_t>(y) * kAncho + x] != 0xFF000000u + (Colores::Fondo.r << 16 | Colores::Fondo.g << 8 | Colores::Fondo.b);
		return n;
	};
	const auto tSi = render(true, true), tNo = render(true, false), teclado = render(false, true);
	CHECK(contar(tSi, b) > b.w * b.h * 0.9, "el boton tactil se dibuja casi lleno (es una superficie solida y visible)");
	CHECK(tSi != tNo, "el boton cambia de aspecto entre SONIDO SI y SONIDO NO");
	CHECK(contar(teclado, b) < b.w * b.h * 0.2, "en modo teclado no hay boton, solo el texto 'M: SONIDO'");
	CHECK(render(true, true) == tSi, "dibujar dos veces da la misma imagen (sin parpadeo)");

	p.Obstaculos().Agregar(Obstaculo{ double(p.ElJugador().CajaColision().x), p.Config().ySuelo - 30, 30, 30, 0.0 });
	p.Actualizar(kDt, false);
	CHECK(p.Estado() == EstadoPartida::GameOver, "(preparacion) la partida esta en GAME OVER");
	CHECK(render(true, true) != render(false, true), "el cartel de GAME OVER cambia entre tactil y teclado");
}

// ---- Test 5.4 -------------------------------------------------------------------
void Test_5_4()
{
	std::puts("Test 5.4: partida jugable solo con toques");
	const SDL_Rect boton = Hud::RectBotonSonido(kAncho, kAlto);
	auto tic = [&](Partida& p, ControlTactil& c, const std::vector<SDL_Point>& nuevos) {
		const auto s = c.Actualizar({}, nuevos);
		p.Actualizar(kDt, s.saltar);
		if (p.TomarEventos().reinicio) c.Cancelar();
		return s;
	};
	Partida p; ControlTactil c(boton);
	p.Obstaculos().Reiniciar();
	tic(p, c, {});
	CHECK(p.ElJugador().EnElSuelo(), "en el suelo al empezar");
	tic(p, c, { SDL_Point{ 100, 100 } });
	CHECK(!p.ElJugador().EnElSuelo() && p.ElJugador().Estado() == EstadoJugador::Saltando, "un toque hace saltar al personaje");
	for (int i = 0; i < 120; ++i) { p.Obstaculos().Reiniciar(); tic(p, c, {}); }
	CHECK(p.ElJugador().EnElSuelo(), "el personaje aterriza");

	// Buffer: tocar un poco ANTES de aterrizar encadena un salto al tocar el suelo
	Partida q; ControlTactil cq(boton);
	for (int i = 0; i < 3; ++i) { q.Obstaculos().Reiniciar(); tic(q, cq, i == 0 ? std::vector<SDL_Point>{ { 100, 100 } } : std::vector<SDL_Point>{}); }
	int aterrizajes = 0, tics = 0;
	bool tocadoAntes = false;
	while (tics++ < 200)
	{
		q.Obstaculos().Reiniciar();
		const bool aPuntoDeAterrizar = q.ElJugador().Estado() == EstadoJugador::Cayendo && q.ElJugador().YPies() > q.Config().ySuelo - 12;
		std::vector<SDL_Point> toque; if (aPuntoDeAterrizar && !tocadoAntes) { toque.push_back({ 100, 100 }); tocadoAntes = true; }
		const auto s = cq.Actualizar({}, toque);
		const bool enSueloAntes = q.ElJugador().EnElSuelo();
		q.Actualizar(kDt, s.saltar);
		const auto ev = q.TomarEventos();
		aterrizajes += ev.aterrizaje;
		if (tocadoAntes && !enSueloAntes && ev.aterrizaje) break;
	}
	bool saltoEncadenado = false;
	for (int i = 0; i < 10; ++i) { q.Obstaculos().Reiniciar(); tic(q, cq, {}); if (!q.ElJugador().EnElSuelo()) saltoEncadenado = true; }
	CHECK(tocadoAntes && saltoEncadenado, "un toque justo antes de aterrizar encadena otro salto (sensacion tactil fluida)");

	// GAME OVER y reinicio solo con toques
	Partida g; ControlTactil cg(boton);
	for (int i = 0; i < 20; ++i) tic(g, cg, {});
	g.Obstaculos().Agregar(Obstaculo{ double(g.ElJugador().CajaColision().x), g.Config().ySuelo - 30, 30, 30, 0.0 });
	tic(g, cg, {});
	CHECK(g.Estado() == EstadoPartida::GameOver, "al chocar: GAME OVER");
	tic(g, cg, { SDL_Point{ 100, 100 } });
	CHECK(g.Estado() == EstadoPartida::GameOver, "un toque desesperado justo tras chocar no reinicia (pausa de 0,5 s)");
	for (int i = 0; i < 40; ++i) tic(g, cg, {});
	tic(g, cg, { SDL_Point{ 100, 100 } });
	CHECK(g.Estado() == EstadoPartida::Jugando && g.Puntos() == 0, "tocar pasado ese tiempo reinicia la partida");
	for (int i = 0; i < 3; ++i) { g.Obstaculos().Reiniciar(); tic(g, cg, {}); }
	CHECK(g.ElJugador().EnElSuelo(), "y el toque de reinicio no provoca un salto inmediato");
}
} // namespace

int main()
{
	try
	{
		Test_5_1();
		Test_5_2();
		Test_5_3();
		Test_5_4();
	}
	catch (const std::exception& e)
	{
		std::printf("  [FAIL] excepcion: %s\n", e.what());
		return 1;
	}
	std::printf(g_fallos == 0 ? "\nTODO OK\n" : "\n%d FALLO(S)\n", g_fallos);
	return g_fallos == 0 ? 0 : 1;
}
