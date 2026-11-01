//------------------------------------------------------------------------------------
// Pruebas de QA del juego Runner - Modulos 1 (texturas) y 2 (fisica y animacion)
// Se ejecutan sin pantalla: SDL_VIDEODRIVER=dummy, SDL_RENDER_DRIVER=software.
//   Test 1.1  Carga exitosa y fallo seguro (ruta inexistente / archivo corrupto)
//   Test 1.2  Renderizado transparente (sin fondo magenta/negro/blanco)
//   Test 2.1  Gravedad y salto (sube desacelerando, cae, no atraviesa el suelo)
//   Test 2.2  Ciclo de animacion entre los fotogramas de la hoja
//------------------------------------------------------------------------------------
#include "Jugador.h"
#include "Ventana.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
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
constexpr int kAncho = 160, kAlto = 120;
constexpr double kDt = 1.0 / 60.0;
constexpr std::uint32_t Pack(Color c) { return 0xFF000000u | (c.r << 16) | (c.g << 8) | c.b; }

std::vector<std::uint32_t> Leer(SDL_Renderer* r)
{
	std::vector<std::uint32_t> px(static_cast<size_t>(kAncho) * kAlto, 0);
	SDL_RenderReadPixels(r, nullptr, SDL_PIXELFORMAT_ARGB8888, px.data(), kAncho * 4);
	return px;
}

void Rellenar(SDL_Renderer* r, Color c)
{
	SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
	SDL_RenderClear(r);
}

// Dibuja un fotograma de la hoja sobre 'fondo' y devuelve el framebuffer
std::vector<std::uint32_t> DibujarFrame(SDL_Renderer* r, const Textura& hoja, int frame, Color fondo, const SDL_Rect& destino)
{
	Rellenar(r, fondo);
	const SDL_Rect origen{ frame * 16, 0, 16, 24 };
	hoja.Dibujar(r, &origen, destino);
	return Leer(r);
}

int ContarPixeles(const std::vector<std::uint32_t>& px, std::uint32_t valor)
{
	int n = 0;
	for (auto p : px) n += (p == valor);
	return n;
}

// ---- Test 1.1 -------------------------------------------------------------------
void Test_1_1(SDL_Renderer* r)
{
	std::printf("Test 1.1: carga de texturas\n");
	auto hoja = Textura::Cargar(r, std::string(BGE_ASSETS_DIR) + "/avatar_run.bmp");
	CHECK(hoja != nullptr, "el sprite sheet del avatar se carga (puntero no nulo)");
	CHECK(hoja && hoja->Ancho() == 96 && hoja->Altura() == 24, "dimensiones 96x24 (6 fotogramas de 16x24)");

	auto falta = Textura::Cargar(r, "assets/no_existe.bmp");
	CHECK(falta == nullptr, "ruta inexistente -> nullptr, sin excepcion ni cierre");
	CHECK(!Textura::UltimoError().empty(), "el motivo del fallo queda disponible en UltimoError()");

	CHECK(Textura::Cargar(r, "") == nullptr, "ruta vacia -> nullptr");
	CHECK(Textura::Cargar(nullptr, std::string(BGE_ASSETS_DIR) + "/avatar_run.bmp") == nullptr, "renderizador nulo -> nullptr");

	const auto corrupto = std::filesystem::temp_directory_path() / "bge_corrupto.bmp";
	{ std::ofstream(corrupto, std::ios::binary) << "esto no es un bmp"; }
	CHECK(Textura::Cargar(r, corrupto.string()) == nullptr, "archivo corrupto -> nullptr");
	std::filesystem::remove(corrupto);

	// Tras un fallo, una carga correcta limpia el error
	auto otra = Textura::Cargar(r, std::string(BGE_ASSETS_DIR) + "/avatar_run.bmp");
	CHECK(otra && Textura::UltimoError().empty(), "una carga correcta posterior limpia UltimoError()");
}

// ---- Test 1.2 -------------------------------------------------------------------
void Test_1_2(SDL_Renderer* r)
{
	std::printf("Test 1.2: renderizado transparente\n");
	auto hoja = Textura::Cargar(r, std::string(BGE_ASSETS_DIR) + "/avatar_run.bmp");
	if (!hoja) { CHECK(false, "no se pudo cargar la hoja"); return; }

	const SDL_Rect destino{ 20, 10, 48, 72 };   // escala x3
	const Color fondos[] = { Colores::Negro, Colores::Blanco, Colores::Verde };
	for (const Color& f : fondos)
	{
		const auto px = DibujarFrame(r, *hoja, 0, f, destino);
		CHECK(ContarPixeles(px, Pack(kColorClaveMagenta)) == 0, "ningun pixel magenta (color clave) llega a pantalla");
		// Esquina superior izquierda del fotograma: transparente en la hoja -> se ve el fondo
		const std::uint32_t esq = px[static_cast<size_t>(destino.y + 1) * kAncho + destino.x + 1];
		CHECK(esq == Pack(f), "la esquina transparente muestra el fondo (sin halo negro/blanco)");
		// Y el avatar si se dibuja: hay pixeles distintos del fondo, dentro de su rectangulo
		int fuera = 0, dentro = 0;
		for (int y = 0; y < kAlto; ++y)
			for (int x = 0; x < kAncho; ++x)
			{
				if (px[static_cast<size_t>(y) * kAncho + x] == Pack(f)) continue;
				const bool in = x >= destino.x && x < destino.x + destino.w && y >= destino.y && y < destino.y + destino.h;
				(in ? dentro : fuera)++;
			}
		CHECK(dentro > 200 && fuera == 0, "el avatar es visible y no mancha fuera de su rectangulo");
	}

	// Alfa propio: superficie 2x1 (rojo opaco, verde con alfa 0) sin color clave
	SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, 2, 1, 32, SDL_PIXELFORMAT_ARGB8888);
	auto* p = static_cast<std::uint32_t*>(s->pixels);
	p[0] = 0xFFFF0000u; p[1] = 0x0000FF00u;
	auto alfa = Textura::DesdeSuperficie(r, s, std::nullopt);
	SDL_FreeSurface(s);
	if (alfa)
	{
		Rellenar(r, Colores::Azul);
		alfa->Dibujar(r, nullptr, SDL_Rect{ 0, 0, 2, 1 });
		const auto px = Leer(r);
		CHECK(px[0] == 0xFFFF0000u && px[1] == Pack(Colores::Azul), "el canal alfa del propio archivo tambien se respeta");
	}
	else CHECK(false, "DesdeSuperficie con alfa");
}

// ---- Test 2.1 -------------------------------------------------------------------
void Test_2_1()
{
	std::printf("Test 2.1: gravedad y salto\n");
	ConfigJugador cfg;
	Jugador j(cfg);

	for (int i = 0; i < 30; ++i) j.Actualizar(kDt, false);
	CHECK(j.Estado() == EstadoJugador::Corriendo && j.YPies() == cfg.ySuelo, "sin pulsar nada permanece en el suelo");

	// Salto: una pulsacion de un solo tic basta
	j.Actualizar(kDt, true);
	CHECK(j.Estado() == EstadoJugador::Saltando && j.VelocidadY() < 0, "al pulsar salto pasa a Saltando con velocidad hacia arriba");

	double minY = j.YPies(), velAnterior = j.VelocidadY();
	bool desacelera = true, fueraDelSuelo = false, vioCayendo = false, vioSaltando = true;
	int tics = 1;
	while (!j.EnElSuelo() && tics < 600)
	{
		j.Actualizar(kDt, false);
		++tics;
		if (j.YPies() > cfg.ySuelo) fueraDelSuelo = true;
		if (j.Estado() == EstadoJugador::Saltando) { if (j.VelocidadY() <= velAnterior) desacelera = false; vioSaltando = true; }
		if (j.Estado() == EstadoJugador::Cayendo) vioCayendo = true;
		if (j.YPies() < minY) minY = j.YPies();
		velAnterior = j.VelocidadY();
	}
	const double alturaEsperada = cfg.velocidadSalto * cfg.velocidadSalto / (2.0 * cfg.gravedad);
	const double alturaReal = cfg.ySuelo - minY;
	const double duracionEsperada = 2.0 * cfg.velocidadSalto / cfg.gravedad / kDt;   // en tics

	CHECK(vioSaltando && desacelera, "durante la subida la velocidad decrece de forma continua (desacelera)");
	CHECK(vioCayendo, "tras el apogeo pasa a Cayendo");
	CHECK(std::fabs(alturaReal - alturaEsperada) < 0.05 * alturaEsperada, "la altura maxima coincide con v^2/2g (+-5%)");
	CHECK(std::fabs(tics - duracionEsperada) <= 3, "la duracion del salto coincide con 2v/g (+-3 tics)");
	CHECK(!fueraDelSuelo, "en ningun tic el jugador atraviesa el suelo");
	CHECK(j.EnElSuelo() && j.YPies() == cfg.ySuelo && j.VelocidadY() == 0.0, "aterriza exactamente en el suelo con velocidad 0");

	// Sin doble salto: pulsar en el aire no cambia la trayectoria
	Jugador a(cfg), b(cfg);
	a.Actualizar(kDt, true); b.Actualizar(kDt, true);
	for (int i = 0; i < 20; ++i) { a.Actualizar(kDt, true); b.Actualizar(kDt, false); }
	CHECK(a.YPies() == b.YPies(), "pulsar salto en el aire no produce doble salto");

	// Robustez: un dt enorme (pestana en segundo plano) tampoco atraviesa el suelo
	Jugador c(cfg);
	c.Actualizar(kDt, true);
	for (int i = 0; i < 5; ++i) c.Actualizar(0.5, false);
	CHECK(c.YPies() <= cfg.ySuelo, "dt grande no hunde al jugador bajo el suelo");
}

// ---- Test 2.2 -------------------------------------------------------------------
void Test_2_2(SDL_Renderer* r)
{
	std::printf("Test 2.2: ciclo de animacion\n");
	ConfigJugador cfg;
	Jugador j(cfg);

	std::vector<int> frames;
	for (int i = 0; i < cfg.framesCorrer * cfg.ticsPorFrame * 2; ++i)
	{
		j.Actualizar(kDt, false);
		frames.push_back(j.FrameActual());
	}
	bool secuencia = true, duracion = true;
	for (size_t i = 0; i < frames.size(); ++i)
		if (frames[i] != static_cast<int>((i + 1) / cfg.ticsPorFrame) % cfg.framesCorrer) secuencia = false;
	for (size_t i = 0; i + 1 < frames.size(); ++i)
		if ((frames[i] != frames[i + 1]) != ((i + 2) % cfg.ticsPorFrame == 0)) duracion = false;   // cambia solo cada N tics
	CHECK(secuencia, "la carrera recorre 0,1,2,3 y vuelve a 0 (ciclo continuo)");
	CHECK(duracion, "cada fotograma dura exactamente ticsPorFrame tics");

	j.Actualizar(kDt, true);
	CHECK(j.FrameActual() == cfg.frameSalto, "subiendo muestra el fotograma de salto");
	while (j.Estado() == EstadoJugador::Saltando) j.Actualizar(kDt, false);
	CHECK(j.FrameActual() == cfg.frameCaida, "bajando muestra el fotograma de caida");
	while (!j.EnElSuelo()) j.Actualizar(kDt, false);
	CHECK(j.FrameActual() == 0, "al aterrizar el ciclo de carrera vuelve a empezar en el fotograma 0");

	// Visual: dos fotogramas distintos del ciclo producen imagenes distintas
	auto hoja = Textura::Cargar(r, std::string(BGE_ASSETS_DIR) + "/avatar_run.bmp");
	if (hoja)
	{
		const SDL_Rect d{ 20, 10, 48, 72 };
		const auto f0 = DibujarFrame(r, *hoja, 0, Colores::Fondo, d);
		const auto f1 = DibujarFrame(r, *hoja, 1, Colores::Fondo, d);
		const auto f2 = DibujarFrame(r, *hoja, 2, Colores::Fondo, d);
		CHECK(f0 != f1 && f1 != f2 && f0 != f2, "los fotogramas 0, 1 y 2 de la hoja se ven distintos en pantalla");

		// Jugador::Pintar usa la hoja y coloca el sprite sobre el suelo
		Rellenar(r, Colores::Fondo);
		ConfigJugador pequena = cfg;
		pequena.ySuelo = kAlto - 10.0;       // los pies caben en el lienzo de prueba
		Jugador p(pequena);
		p.Pintar(r, hoja.get());
		CHECK(ContarPixeles(Leer(r), Pack(Colores::Fondo)) < kAncho * kAlto, "Jugador::Pintar dibuja el sprite");
	}
	else CHECK(false, "no se pudo cargar la hoja");
}
} // namespace

int main()
{
	try
	{
		Ventana ventana(ConfigVentana{ "test jugador", kAncho, kAlto, false, true });
		SDL_Renderer* r = ventana.Renderizador();
		Test_1_1(r);
		Test_1_2(r);
		Test_2_1();
		Test_2_2(r);
	}
	catch (const std::exception& e)
	{
		std::printf("  [FAIL] excepcion: %s\n", e.what());
		return 1;
	}
	std::printf(g_fallos == 0 ? "\nTODO OK\n" : "\n%d FALLO(S)\n", g_fallos);
	return g_fallos == 0 ? 0 : 1;
}
