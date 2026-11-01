//------------------------------------------------------------------------------------
// Pruebas de QA del juego Runner - Modulo 4 (identidad visual, efectos y sonido)
// Se ejecutan sin pantalla ni altavoces: SDL_VIDEODRIVER=dummy, SDL_AUDIODRIVER=dummy.
//   Test 4.1  Paleta armonica con el avatar y legible (contraste)
//   Test 4.2  Escenario: parallax, ciclo sin saltos y cobertura total de pantalla
//   Test 4.3  Efectos: particulas acotadas, sacudida que se apaga, determinismo
//   Test 4.4  Eventos de la partida (la logica avisa; el juego reacciona)
//   Test 4.5  Sintesis de audio, mezclador y dispositivo (con y sin audio)
//------------------------------------------------------------------------------------
#include "Efectos.h"
#include "Escenario.h"
#include "Hud.h"
#include "Paleta.h"
#include "Sonido.h"
#include "Ventana.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <set>
#include <vector>

static int g_fallos = 0;
#define CHECK(cond, msg)                                                           \
	do {                                                                           \
		if (cond) std::printf("  [ OK ] %s\n", msg);                               \
		else { std::printf("  [FAIL] %s  (%s:%d)\n", msg, __FILE__, __LINE__); ++g_fallos; } \
	} while (0)

#ifndef BGE_ASSETS_DIR
#define BGE_ASSETS_DIR "assets"
#endif

namespace
{
constexpr double kDt = 1.0 / 60.0;
constexpr int kAncho = 640, kAlto = 480;
constexpr double kSuelo = 400.0;
constexpr std::uint32_t Pack(Color c) { return 0xFF000000u | (c.r << 16) | (c.g << 8) | c.b; }

std::vector<std::uint32_t> Leer(SDL_Renderer* r)
{
	std::vector<std::uint32_t> px(static_cast<std::size_t>(kAncho) * kAlto);
	SDL_RenderReadPixels(r, nullptr, SDL_PIXELFORMAT_ARGB8888, px.data(), kAncho * 4);
	return px;
}

double Lin(int c) { const double s = c / 255.0; return s <= 0.03928 ? s / 12.92 : std::pow((s + 0.055) / 1.055, 2.4); }
double Luz(Color c) { return 0.2126 * Lin(c.r) + 0.7152 * Lin(c.g) + 0.0722 * Lin(c.b); }
double Contraste(Color a, Color b)
{
	const double la = Luz(a), lb = Luz(b);
	return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}
double Dist(Color a, Color b) { return std::sqrt(double((a.r - b.r) * (a.r - b.r) + (a.g - b.g) * (a.g - b.g) + (a.b - b.b) * (a.b - b.b))); }

// ---- Test 4.1 -------------------------------------------------------------------
void Test_4_1()
{
	std::puts("Test 4.1: paleta");
	std::vector<Color> sprite;
	if (SDL_Surface* bmp = SDL_LoadBMP(BGE_ASSETS_DIR "/avatar_run.bmp"))
	{
		SDL_Surface* s = SDL_ConvertSurfaceFormat(bmp, SDL_PIXELFORMAT_ARGB8888, 0);
		SDL_FreeSurface(bmp);
		std::set<std::uint32_t> unicos;
		for (int i = 0; i < s->w * s->h; ++i) unicos.insert(static_cast<std::uint32_t*>(s->pixels)[i] & 0xFFFFFFu);
		for (auto p : unicos) if (p != 0xFF00FFu) sprite.push_back(Color{ std::uint8_t(p >> 16), std::uint8_t(p >> 8), std::uint8_t(p), 255 });
		SDL_FreeSurface(s);
	}
	CHECK(!sprite.empty(), "se pudo leer la hoja de sprites del avatar");

	auto cercano = [&](Color c) { double m = 1e9; for (Color s : sprite) m = std::min(m, Dist(c, s)); return m; };
	struct { const char* n; Color c; } acentos[] = { { "violeta", Paleta::Violeta }, { "verde", Paleta::Verde },
	                                                 { "marron", Paleta::Marron }, { "piel", Paleta::Piel } };
	for (auto& a : acentos)
	{
		char m[96]; std::snprintf(m, sizeof m, "el %s de la paleta existe en el sprite del avatar (distancia %.0f <= 25)", a.n, cercano(a.c));
		CHECK(cercano(a.c) <= 25.0, m);
	}
	std::printf("         (dorado: distancia al sprite %.0f; es el tono de los botones, no se exige)\n", cercano(Paleta::Dorado));

	// El mundo es mas oscuro que el personaje: el avatar siempre destaca
	const Color fondos[] = { Paleta::CieloArriba, Paleta::CieloHorizonte, Paleta::ColinaLejana, Paleta::ColinaCercana, Paleta::Suelo };
	bool masOscuro = true;
	for (Color f : fondos) if (Luz(f) >= Luz(Paleta::Violeta)) masOscuro = false;
	CHECK(masOscuro, "todos los colores del fondo son mas oscuros que el violeta del avatar");
	CHECK(Contraste(Paleta::Violeta, Paleta::CieloHorizonte) >= 1.8 && Contraste(Paleta::Verde, Paleta::Suelo) >= 3.0,
	      "violeta y verde del avatar contrastan con el cielo y el suelo");

	// Peligro y texto legibles
	CHECK(Contraste(Paleta::BugCuerpo, Paleta::CieloHorizonte) >= 3.0, "los bugs naranjas destacan sobre el cielo");
	CHECK(Contraste(Paleta::Contorno, Paleta::Hierba) >= 4.0 && Contraste(Paleta::Contorno, Paleta::BugCuerpo) >= 4.0,
	      "sobre la hierba (luminosidad parecida) el contorno oscuro del bug lo separa con claridad");
	CHECK(std::fabs(Dist(Paleta::BugCuerpo, Paleta::Verde) - Dist(Paleta::BugCuerpo, Paleta::Violeta)) < 400 &&
	      Dist(Paleta::BugCuerpo, Paleta::Verde) > 250 && Dist(Paleta::BugCuerpo, Paleta::Violeta) > 150,
	      "el naranja es un tono claramente distinto del violeta y del verde del avatar (no se confunde con el)");
	CHECK(Contraste(Paleta::TextoClaro, Paleta::CieloHorizonte) >= 4.5 && Contraste(Paleta::TextoClaro, Paleta::CieloArriba) >= 7.0,
	      "el texto del HUD cumple contraste AA sobre todo el degradado del cielo");
	CHECK(Contraste(Paleta::Dorado, Paleta::CieloArriba) >= 7.0 && Contraste(Paleta::Verde, Paleta::CieloArriba) >= 7.0,
	      "las cifras doradas y verdes del HUD tambien se leen bien");
}

// ---- Test 4.2 -------------------------------------------------------------------
void Test_4_2(SDL_Renderer* r)
{
	std::puts("Test 4.2: escenario");
	const Dibujante d(r);
	const Color centinela{ 255, 0, 255, 255 };

	Escenario e(kAncho, kAlto, kSuelo);
	d.Limpiar(centinela);
	e.Pintar(d);
	const auto px = Leer(r);
	CHECK(std::count(px.begin(), px.end(), Pack(centinela)) == 0, "el escenario cubre toda la pantalla (sin huecos)");
	CHECK(px[static_cast<std::size_t>(kSuelo + 5) * kAncho + 100] == Pack(Paleta::Hierba) ||
	      px[static_cast<std::size_t>(kSuelo + 5) * kAncho + 100] == Pack(Paleta::SombraSuelo), "bajo los pies hay hierba");
	CHECK(px[static_cast<std::size_t>(5) * kAncho + 3] == Pack(Paleta::CieloArriba), "arriba del todo el cielo es el violeta oscuro");

	// Parallax: capas lejanas mas lentas
	Escenario p(kAncho, kAlto, kSuelo);
	for (int i = 0; i < 60; ++i) p.Actualizar(kDt, 300.0);
	std::printf("         tras 1 s a 300 px/s: suelo %.1f, colinas cercanas %.1f, lejanas %.1f\n",
	            p.DesplazamientoSuelo(), p.DesplazamientoCerca(), p.DesplazamientoLejos());
	CHECK(std::fabs(p.DesplazamientoSuelo() - 300.0) < 1.0 && std::fabs(p.DesplazamientoCerca() - 90.0) < 1.0 &&
	      std::fabs(p.DesplazamientoLejos() - 30.0) < 1.0, "suelo 100%, colinas cercanas 30%, lejanas 10% de la velocidad");

	// Congelado cuando velocidad = 0 (GAME OVER)
	const double s0 = p.DesplazamientoSuelo();
	for (int i = 0; i < 30; ++i) p.Actualizar(kDt, 0.0);
	CHECK(p.DesplazamientoSuelo() == s0, "con velocidad 0 el escenario se congela");

	// Sin saltos al dar la vuelta al ciclo: antes y despues del cambio casi todos los pixeles coinciden
	Escenario s(kAncho, kAlto, kSuelo);
	s.Actualizar(1.0, 1279.0 / 0.30);          // colinas cercanas justo antes de completar el periodo
	d.Limpiar(centinela); s.Pintar(d); const auto antes = Leer(r);
	s.Actualizar(1.0, 1.0 / 0.30);             // cruzan el periodo (+1 px)
	d.Limpiar(centinela); s.Pintar(d); const auto despues = Leer(r);
	std::size_t difieren = 0;
	for (std::size_t i = 0; i < antes.size(); ++i) difieren += (antes[i] != despues[i]);
	const double pct = 100.0 * difieren / antes.size();
	std::printf("         pixeles distintos al cruzar el periodo: %.2f%%\n", pct);
	CHECK(pct < 4.0, "el ciclo de las colinas enlaza sin saltos visibles");

	// Determinismo
	Escenario a(kAncho, kAlto, kSuelo, 5), b(kAncho, kAlto, kSuelo, 5);
	d.Limpiar(centinela); a.Pintar(d); const auto pa = Leer(r);
	d.Limpiar(centinela); b.Pintar(d); const auto pb = Leer(r);
	CHECK(pa == pb, "misma semilla -> mismo escenario");
}

// ---- Test 4.3 -------------------------------------------------------------------
void Test_4_3(SDL_Renderer* r)
{
	std::puts("Test 4.3: efectos");
	const Dibujante d(r);
	const Color centinela{ 255, 0, 255, 255 };

	Efectos e(kSuelo, 3);
	for (int i = 0; i < 20; ++i) e.Explosion(300, 350);
	CHECK(e.Activas() <= Efectos::kMaxParticulas && e.Activas() == Efectos::kMaxParticulas,
	      "capacidad fija: 20 explosiones seguidas llenan el pool sin desbordarlo");

	// Las particulas no atraviesan el suelo (mientras viven)
	bool bajoSuelo = false;
	for (int i = 0; i < 90; ++i)
	{
		e.Actualizar(kDt);
		d.Limpiar(centinela); e.Pintar(d);
		const auto px = Leer(r);
		for (int y = static_cast<int>(kSuelo) + 6; y < kAlto && !bajoSuelo; ++y)
			for (int x = 0; x < kAncho; ++x) if (px[static_cast<std::size_t>(y) * kAncho + x] != Pack(centinela)) { bajoSuelo = true; break; }
	}
	CHECK(!bajoSuelo, "ninguna particula se dibuja por debajo del suelo");
	for (int i = 0; i < 120; ++i) e.Actualizar(kDt);
	CHECK(e.Activas() == 0, "todas las particulas mueren y se liberan (a los ~3 s no queda ninguna)");

	Efectos p(kSuelo, 3);
	p.Polvo(100, kSuelo, 5);
	CHECK(p.Activas() == 5, "Polvo(n) emite n particulas");
	p.Chispas(60, 24);
	CHECK(p.Activas() == 5 + 14, "Chispas emite 14 particulas");

	// Sacudida
	Efectos s(kSuelo, 9);
	CHECK(s.Sacudida().x == 0 && s.Sacudida().y == 0, "en reposo no hay desplazamiento");
	s.Sacudir(9, 0.45);
	int maxPrimera = 0, maxUltima = 0;
	bool acotada = true;
	for (int i = 0; i < 27; ++i)       // 0,45 s = 27 tics
	{
		s.Actualizar(kDt);
		const int m = std::max(std::abs(s.Sacudida().x), std::abs(s.Sacudida().y));
		if (m > 9) acotada = false;
		if (i < 9)  maxPrimera = std::max(maxPrimera, m);
		if (i >= 18) maxUltima = std::max(maxUltima, m);
	}
	s.Actualizar(kDt); s.Actualizar(kDt);
	CHECK(acotada, "el desplazamiento nunca supera la amplitud");
	CHECK(maxPrimera >= 4 && maxUltima < maxPrimera, "la sacudida es fuerte al principio y decae");
	CHECK(s.Sacudida().x == 0 && s.Sacudida().y == 0, "al terminar la duracion la pantalla queda quieta");

	// Destello
	Efectos f(kSuelo);
	f.Destello(0.3);
	CHECK(f.IntensidadDestello() == 1.0, "el destello empieza a intensidad 1");
	for (int i = 0; i < 6; ++i) f.Actualizar(kDt);
	const double mitad = f.IntensidadDestello();
	for (int i = 0; i < 20; ++i) f.Actualizar(kDt);
	CHECK(mitad > 0.3 && mitad < 0.8 && f.IntensidadDestello() == 0.0, "el destello se apaga linealmente");
	d.Limpiar(centinela); f.PintarDestello(d, kAncho, kAlto);
	const auto apagado = Leer(r);
	CHECK(std::count(apagado.begin(), apagado.end(), Pack(centinela)) == kAncho * kAlto, "apagado, no dibuja nada");

	// Determinismo
	Efectos a(kSuelo, 11), b(kSuelo, 11);
	for (Efectos* x : { &a, &b }) { x->Explosion(200, 300); for (int i = 0; i < 20; ++i) x->Actualizar(kDt); }
	d.Limpiar(centinela); a.Pintar(d); const auto pa = Leer(r);
	d.Limpiar(centinela); b.Pintar(d); const auto pb = Leer(r);
	CHECK(pa == pb && pa != std::vector<std::uint32_t>(pa.size(), Pack(centinela)), "misma semilla -> mismas particulas (y se ven)");
}

// ---- Test 4.4 -------------------------------------------------------------------
void Test_4_4()
{
	std::puts("Test 4.4: eventos de la partida");
	auto vacio = [](const EventosPartida& e) { return !e.salto && !e.aterrizaje && !e.choque && !e.reinicio && e.hitos == 0; };

	{
		Partida p;
		p.Obstaculos().Reiniciar();
		CHECK(vacio(p.TomarEventos()), "una partida recien creada no tiene eventos");
		int saltos = 0, aterrizajes = 0, otros = 0;
		for (int t = 0; t < 120; ++t)
		{
			p.Obstaculos().Reiniciar();                 // pista vacia: solo se prueba el salto
			p.Actualizar(kDt, t == 5);
			const auto e = p.TomarEventos();
			saltos += e.salto; aterrizajes += e.aterrizaje; otros += e.choque + e.reinicio + e.hitos;
		}
		CHECK(saltos == 1 && aterrizajes == 1 && otros == 0, "un salto emite exactamente un 'salto' y un 'aterrizaje'");
		p.Actualizar(kDt, false);
		CHECK(vacio(p.TomarEventos()), "TomarEventos() borra los eventos pendientes");
	}
	{
		Partida p;
		for (int i = 0; i < 20; ++i) p.Actualizar(kDt, false);          // varios tics antes de recoger: se acumulan
		const SDL_Rect c = p.ElJugador().CajaColision();
		Obstaculo o; o.x = c.x; o.y = p.Config().ySuelo - 30; o.ancho = 30; o.alto = 30; o.vx = 0;
		p.Obstaculos().Agregar(o);
		p.Actualizar(kDt, false);
		auto e = p.TomarEventos();
		CHECK(e.choque && !e.reinicio && p.Estado() == EstadoPartida::GameOver, "el choque emite 'choque' una vez");
		for (int i = 0; i < 100; ++i) p.Actualizar(kDt, false);
		CHECK(vacio(p.TomarEventos()), "en GameOver no hay mas eventos (el mundo esta congelado)");
		p.Actualizar(kDt, true);
		e = p.TomarEventos();
		CHECK(e.reinicio && p.Estado() == EstadoPartida::Jugando, "reiniciar emite 'reinicio'");
	}
	{
		Partida p;
		int hitos = 0; bool maxUno = true;
		for (int t = 0; t < 600; ++t)
		{
			p.Obstaculos().Reiniciar();
			p.Actualizar(kDt, false);
			const auto e = p.TomarEventos();
			hitos += e.hitos; if (e.hitos > 1) maxUno = false;
		}
		std::printf("         10 s corriendo: %d puntos, %d hitos\n", p.Puntos(), hitos);
		CHECK(hitos == p.Puntos() / 100 && hitos >= 2 && maxUno, "se emite un 'hito' por cada 100 puntos, sin repetir ni saltarse ninguno");
	}
}

// ---- Test 4.5 -------------------------------------------------------------------
double DuracionS(const std::vector<std::int16_t>& m) { return double(m.size()) / Sintesis::kFrecuencia; }
int Pico(const std::vector<std::int16_t>& m) { int p = 0; for (auto v : m) p = std::max(p, std::abs(int(v))); return p; }
double Rms(const std::vector<std::int16_t>& m) { double s = 0; for (auto v : m) s += double(v) * v; return std::sqrt(s / m.size()); }

void Test_4_5_Sintesis()
{
	std::puts("Test 4.5: sintesis, mezclador y dispositivo");
	struct { Efecto e; const char* n; double min, max; } casos[] = {
		{ Efecto::Salto, "salto", 0.15, 0.4 }, { Efecto::Aterrizaje, "aterrizaje", 0.05, 0.25 },
		{ Efecto::Golpe, "golpe", 0.4, 1.0 },  { Efecto::Hito, "hito", 0.2, 0.6 }, { Efecto::Inicio, "inicio", 0.2, 0.6 } };
	std::vector<std::vector<std::int16_t>> todos;
	for (auto& c : casos)
	{
		const auto m = Sintesis::Generar(c.e);
		todos.push_back(m);
		char msg[128];
		std::snprintf(msg, sizeof msg, "%s: %.2f s, pico %d, rms %.0f", c.n, DuracionS(m), Pico(m), Rms(m));
		std::printf("         %s\n", msg);
		const double media = std::accumulate(m.begin(), m.end(), 0.0, [](double a, std::int16_t v) { return a + v; }) / m.size();
		CHECK(DuracionS(m) >= c.min && DuracionS(m) <= c.max, "duracion razonable");
		CHECK(Pico(m) >= 0.3 * 32767 && Pico(m) <= 0.9 * 32767, "volumen audible y sin saturar (pico entre 30% y 90%)");
		CHECK(Rms(m) > 500 && std::fabs(media) < 0.05 * Pico(m), "no es silencio ni tiene componente continua");
		CHECK(std::abs(m.front()) <= 300 && std::abs(m.back()) <= 300, "empieza y termina en silencio (sin clics)");
		CHECK(m == Sintesis::Generar(c.e), "determinista: dos generaciones dan las mismas muestras");
	}
	bool distintos = true;
	for (std::size_t i = 0; i < todos.size(); ++i) for (std::size_t j = i + 1; j < todos.size(); ++j) if (todos[i] == todos[j]) distintos = false;
	CHECK(distintos, "los cinco efectos suenan distinto");

	const auto mus = Sintesis::Musica();
	std::printf("         musica: %.2f s, pico %d, rms %.0f\n", DuracionS(mus), Pico(mus), Rms(mus));
	CHECK(std::fabs(DuracionS(mus) - 4.0) < 0.01, "la musica dura 4 s (16 corcheas a 120 ppm)");
	CHECK(Pico(mus) <= 0.9 * 32767 && Rms(mus) > 500, "musica audible y sin saturar");
	CHECK(std::abs(mus.front()) <= 300 && std::abs(mus.back()) <= 300 && std::abs(mus.front() - mus.back()) <= 600,
	      "el bucle enlaza: final e inicio estan en silencio (sin chasquido al repetir)");
}

void Test_4_5_Mezclador()
{
	{
		Mezclador m;
		const auto salto = Sintesis::Generar(Efecto::Salto);
		m.Registrar(Efecto::Salto, salto);
		m.Reproducir(Efecto::Salto);
		CHECK(m.VocesActivas() == 1, "Reproducir ocupa una voz");
		std::vector<std::int16_t> sal;
		std::int16_t bloque[100];
		for (std::size_t i = 0; i < salto.size() + 500; i += 100) { m.Mezclar(bloque, 100); sal.insert(sal.end(), bloque, bloque + 100); }
		bool igual = true;
		for (std::size_t i = 0; i < salto.size(); ++i) if (sal[i] != static_cast<std::int16_t>(std::lround(salto[i] * 0.7))) igual = false;
		bool silencioDespues = true;
		for (std::size_t i = salto.size(); i < sal.size(); ++i) if (sal[i] != 0) silencioDespues = false;
		CHECK(igual && silencioDespues, "la salida es la muestra x ganancia, y despues silencio (mezclando en bloques de 100)");
		CHECK(m.VocesActivas() == 0, "la voz se libera al terminar");
	}
	{
		Mezclador m;
		m.EstablecerMusica({ 1000, 2000, 3000 });
		std::int16_t s[7];
		m.Mezclar(s, 7);
		CHECK(s[0] == 280 && s[1] == 560 && s[2] == 840 && s[3] == 280 && s[6] == 280, "la musica se repite en bucle sin cortes");
		m.PausarMusica(true); m.Mezclar(s, 7);
		CHECK(std::all_of(s, s + 7, [](std::int16_t v) { return v == 0; }), "pausada, la musica no suena");
		m.PausarMusica(false);
		m.Registrar(Efecto::Hito, std::vector<std::int16_t>(5000, 1000));
		m.Reproducir(Efecto::Hito);
		m.Silenciar(true);
		m.Mezclar(s, 7);
		CHECK(std::all_of(s, s + 7, [](std::int16_t v) { return v == 0; }) && m.VocesActivas() == 0, "silenciado: todo a cero y se cortan las voces");
		m.Reproducir(Efecto::Hito);
		CHECK(m.VocesActivas() == 0, "en silencio no se inician voces nuevas");
		m.Silenciar(false); m.Reproducir(Efecto::Hito);
		CHECK(m.VocesActivas() == 1, "al reactivar el sonido vuelve a funcionar");
	}
	{
		Mezclador m;
		m.GananciaEfectos = 1.0;
		m.Registrar(Efecto::Golpe, std::vector<std::int16_t>(50, 30000));
		m.Registrar(Efecto::Hito, std::vector<std::int16_t>(50, -30000));
		m.Reproducir(Efecto::Golpe); m.Reproducir(Efecto::Golpe);
		std::int16_t s[10]; m.Mezclar(s, 10);
		CHECK(std::all_of(s, s + 10, [](std::int16_t v) { return v == 32767; }), "la suma satura en 32767 (no da la vuelta a negativo)");
		std::int16_t resto[40]; m.Mezclar(resto, 40);                       // agota las voces anteriores
		m.Reproducir(Efecto::Hito); m.Reproducir(Efecto::Hito); m.Mezclar(s, 10);
		CHECK(std::all_of(s, s + 10, [](std::int16_t v) { return v == -32768; }), "y satura en -32768 por abajo");
		for (int i = 0; i < 30; ++i) m.Reproducir(Efecto::Golpe);
		CHECK(m.VocesActivas() == Mezclador::kVoces, "con todas las voces ocupadas se sustituye la mas antigua (sin desbordar)");
	}
}

void Test_4_5_Dispositivo()
{
	// Sin audio disponible: Abrir() devuelve nullptr y el juego puede seguir mudo
	SDL_setenv("SDL_AUDIODRIVER", "controlador_inexistente", 1);
	CHECK(Sonido::Abrir() == nullptr, "sin dispositivo de audio, Abrir() devuelve nullptr (sin excepcion ni cierre)");
	SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);

	auto s = Sonido::Abrir();
	CHECK(s != nullptr, "con dispositivo, Abrir() devuelve el sonido");
	if (!s) return;
	for (int i = 0; i < kNumEfectos; ++i) s->Reproducir(static_cast<Efecto>(i));   // en paralelo con el hilo de audio
	SDL_Delay(120);
	for (int i = 0; i < 200; ++i) { s->Reproducir(Efecto::Salto); s->PausarMusica(i % 2 == 0); }
	CHECK(!s->Silenciado() && s->AlternarSilencio() && s->Silenciado() && !s->AlternarSilencio(), "M alterna entre silencio y sonido");
	s.reset();
	CHECK(Sonido::Abrir() != nullptr, "se puede cerrar y volver a abrir el audio");
}
} // namespace

int main()
{
	try
	{
		Test_4_1();
		{
			Ventana v(ConfigVentana{ "efectos", kAncho, kAlto, false, true });
			Test_4_2(v.Renderizador());
			Test_4_3(v.Renderizador());
		}
		Test_4_4();
		Test_4_5_Sintesis();
		Test_4_5_Mezclador();
		Test_4_5_Dispositivo();
	}
	catch (const std::exception& e)
	{
		std::printf("  [FAIL] excepcion: %s\n", e.what());
		return 1;
	}
	std::printf(g_fallos == 0 ? "\nTODO OK\n" : "\n%d FALLO(S)\n", g_fallos);
	return g_fallos == 0 ? 0 : 1;
}
