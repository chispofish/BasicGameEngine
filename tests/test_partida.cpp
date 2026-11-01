//------------------------------------------------------------------------------------
// Pruebas de QA del juego Runner - Modulo 3 (obstaculos, colisiones, HUD)
// Se ejecutan sin pantalla: SDL_VIDEODRIVER=dummy, SDL_RENDER_DRIVER=software.
//   Test 3.1  Generacion, movimiento y destruccion de obstaculos (sin fugas)
//   Test 3.2  Colision AABB exacta, sin falsos positivos por los margenes del sprite
//   Test 3.3  HUD dinamico, estable y sin rastros entre frames
// (El Test 3.4, carga web de assets, se verifica en CI sobre el build de Emscripten.)
//------------------------------------------------------------------------------------
#include "Hud.h"
#include "NucleoMotor.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
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

Obstaculo Bloque(double x, int w, int h, double ySuelo, double vx = -300.0)
{
	Obstaculo o;
	o.x = x; o.ancho = w; o.alto = h; o.y = ySuelo - h; o.vx = vx;
	return o;
}

// ---- Test 3.1 -------------------------------------------------------------------
void Test_3_1()
{
	std::puts("Test 3.1: obstaculos (generacion, movimiento y destruccion)");
	ConfigObstaculos cfg;
	GestorObstaculos g(cfg, 7);

	const double v = 600.0;
	bool todosVisiblesOEnCamino = true, aparecenFuera = true, movimientoExacto = true;
	std::size_t maxActivos = 0, capacidadTrasCalentar = 0;
	std::uint64_t vistos = 0;
	std::vector<double> instantes;               // momentos de aparicion
	for (int tic = 0; tic < 7200; ++tic)         // 2 minutos de juego
	{
		const std::vector<Obstaculo> antes = g.Activos();
		const auto generadosAntes = g.Generados(), destruidosAntes = g.Destruidos();
		g.Actualizar(kDt, v);

		// Los supervivientes conservan el orden y se mueven exactamente vx*dt (velocidad constante)
		const std::size_t quitados = static_cast<std::size_t>(g.Destruidos() - destruidosAntes);
		const std::size_t nuevos   = static_cast<std::size_t>(g.Generados() - generadosAntes);
		if (g.Activos().size() != antes.size() - quitados + nuevos) movimientoExacto = false;
		for (std::size_t i = 0; quitados + i < antes.size() && i < g.Activos().size(); ++i)
			if (std::fabs(g.Activos()[i].x - (antes[quitados + i].x + antes[quitados + i].vx * kDt)) > 1e-9 ||
			    g.Activos()[i].vx != antes[quitados + i].vx)
				movimientoExacto = false;

		if (g.Generados() != generadosAntes)
		{
			instantes.push_back(tic * kDt);
			const Obstaculo& nuevo = g.Activos().back();
			if (nuevo.x < cfg.anchoPantalla) aparecenFuera = false;                // nace fuera, a la derecha
			if (std::fabs(nuevo.y + nuevo.alto - cfg.ySuelo) > 1e-9) aparecenFuera = false;   // apoya en el suelo
			++vistos;
		}
		for (const auto& o : g.Activos()) if (o.x + o.ancho < 0) todosVisiblesOEnCamino = false;
		maxActivos = std::max(maxActivos, g.Activos().size());
		if (tic == 1200) capacidadTrasCalentar = g.Capacidad();
	}

	CHECK(vistos > 50, "se generan obstaculos de forma continua durante 2 minutos");
	CHECK(aparecenFuera, "cada obstaculo nace fuera de pantalla por la derecha, apoyado en el suelo");
	CHECK(movimientoExacto, "se mueven hacia la izquierda a velocidad constante");
	CHECK(todosVisiblesOEnCamino, "ningun obstaculo activo esta ya completamente fuera por la izquierda");
	CHECK(g.Generados() - g.Destruidos() == g.Activos().size(), "contabilidad: generados - destruidos == activos (sin fugas)");
	CHECK(g.Destruidos() + 10 >= g.Generados(), "los que salen por la izquierda se destruyen (quedan <10 vivos)");
	CHECK(maxActivos <= 6, "el numero de activos permanece acotado");
	CHECK(g.Capacidad() == capacidadTrasCalentar, "la capacidad del vector no crece: se reutiliza la memoria");

	bool intervalos = true;
	for (std::size_t i = 1; i < instantes.size(); ++i)
	{
		const double d = instantes[i] - instantes[i - 1];
		if (d < cfg.intervaloMin - 2 * kDt || d > cfg.intervaloMax + 2 * kDt) intervalos = false;
	}
	CHECK(intervalos, "el intervalo entre apariciones esta en [min, max]");

	// Destruccion exacta: se conserva mientras toque el borde, se elimina al salir del todo
	GestorObstaculos h(cfg, 1);
	h.Agregar(Bloque(-40 + 0.5 + 300 * kDt, 40, 40, cfg.ySuelo));       // tras un tic quedara en x = -39.5
	h.Actualizar(kDt, v);
	CHECK(h.Activos().size() == 1, "con un borde aun visible no se destruye");
	h.Actualizar(kDt, v);                                               // x = -44.5 -> x + w = -4.5 < 0
	CHECK(h.Activos().empty() && h.Destruidos() == 1, "al salir por completo se destruye");

	// Determinismo y reinicio
	GestorObstaculos a(cfg, 99), b(cfg, 99);
	bool igual = true;
	for (int i = 0; i < 600; ++i)
	{
		a.Actualizar(kDt, v); b.Actualizar(kDt, v);
		if (a.Activos().size() != b.Activos().size()) igual = false;
	}
	CHECK(igual && a.Generados() == b.Generados(), "misma semilla -> misma secuencia (pruebas reproducibles)");
	a.Reiniciar();
	CHECK(a.Activos().empty() && a.Generados() - a.Destruidos() == 0, "Reiniciar vacia la pista y mantiene la contabilidad");
}

// Caja opaca (pixeles distintos del color clave) de cada fotograma de la hoja
struct Caja4 { int x0 = 1000, y0 = 1000, x1 = -1, y1 = -1; };

std::vector<Caja4> CajasOpacas()
{
	const ConfigJugador cfg;
	const int fw = cfg.anchoFrame;
	std::vector<Caja4> res(6);
	SDL_Surface* bmp = SDL_LoadBMP(BGE_ASSETS_DIR "/avatar_run.bmp");
	if (!bmp) return res;
	SDL_Surface* s = SDL_ConvertSurfaceFormat(bmp, SDL_PIXELFORMAT_ARGB8888, 0);
	SDL_FreeSurface(bmp);
	for (int y = 0; y < s->h; ++y)
		for (int x = 0; x < s->w; ++x)
		{
			const auto p = static_cast<std::uint32_t*>(s->pixels)[y * s->pitch / 4 + x] & 0xFFFFFFu;
			if (p == 0xFF00FFu) continue;
			Caja4& c = res[static_cast<std::size_t>(x / fw)];
			c.x0 = std::min(c.x0, x % fw); c.x1 = std::max(c.x1, x % fw);
			c.y0 = std::min(c.y0, y);      c.y1 = std::max(c.y1, y);
		}
	SDL_FreeSurface(s);
	return res;
}

// ---- Test 3.2 -------------------------------------------------------------------
void Test_3_2()
{
	std::puts("Test 3.2: colision AABB");
	const SDL_Rect a{ 100, 100, 50, 50 };
	CHECK(Colisionan(a, SDL_Rect{ 149, 100, 20, 20 }), "1 pixel de solape en X -> colision");
	CHECK(!Colisionan(a, SDL_Rect{ 150, 100, 20, 20 }), "bordes que solo se tocan en X -> no hay colision");
	CHECK(Colisionan(a, SDL_Rect{ 100, 149, 20, 20 }), "1 pixel de solape en Y -> colision");
	CHECK(!Colisionan(a, SDL_Rect{ 100, 150, 20, 20 }), "bordes que solo se tocan en Y -> no hay colision");
	CHECK(!Colisionan(a, SDL_Rect{ 80, 100, 20, 20 }), "borde derecho del otro toca el izquierdo -> no hay colision");
	CHECK(Colisionan(a, SDL_Rect{ 110, 110, 5, 5 }), "contenido por completo -> colision");
	CHECK(Colisionan(SDL_Rect{ 110, 110, 5, 5 }, a), "simetrica");
	CHECK(!Colisionan(a, SDL_Rect{ 120, 120, 0, 10 }), "rectangulo sin area -> nunca colisiona");

	// La caja de colision del jugador debe estar dentro de lo que realmente se ve del sprite
	const ConfigJugador cfg;
	Jugador j(cfg);
	const SDL_Rect sprite = j.Rectangulo(), caja = j.CajaColision();
	const int e = cfg.escala;
	const int cx0 = (caja.x - sprite.x) / e, cx1 = (caja.x + caja.w - sprite.x) / e - 1;
	const int cy0 = (caja.y - sprite.y) / e, cy1 = (caja.y + caja.h - sprite.y) / e - 1;
	const auto cajas = CajasOpacas();
	Caja4 uni;
	for (const auto& c : cajas)
	{
		uni.x0 = std::min(uni.x0, c.x0); uni.x1 = std::max(uni.x1, c.x1);
		uni.y0 = std::min(uni.y0, c.y0); uni.y1 = std::max(uni.y1, c.y1);
	}
	std::printf("         pixeles visibles del sprite x[%d..%d] y[%d..%d]; caja de colision x[%d..%d] y[%d..%d]\n",
	            uni.x0, uni.x1, uni.y0, uni.y1, cx0, cx1, cy0, cy1);
	CHECK(cx0 >= uni.x0 && cx1 <= uni.x1 && cy0 >= uni.y0 && cy1 <= uni.y1,
	      "la caja de colision queda dentro del area visible del avatar (sin falsos positivos en margenes)");
	const double areaCaja = double(cx1 - cx0 + 1) * (cy1 - cy0 + 1);
	const double areaVisible = double(uni.x1 - uni.x0 + 1) * (uni.y1 - uni.y0 + 1);
	std::printf("         la caja cubre el %.0f%% del rectangulo visible\n", 100.0 * areaCaja / areaVisible);
	CHECK(areaCaja >= 0.45 * areaVisible,
	      "y cubre una parte sustancial del avatar (>=45%: cabeza, gorra y manos no cuentan; no es una caja diminuta)");
	bool porFrame = true;
	for (const auto& c : cajas) if (cx0 < c.x0 - 1 || cx1 > c.x1 + 1 || cy0 < c.y0 - 1 || cy1 > c.y1 + 1) porFrame = false;
	CHECK(porFrame, "en cada fotograma sobresale como mucho 1 pixel del sprite visible");

	// Partida: un obstaculo en el margen transparente no mata; un pixel dentro de la caja, si
	{
		Partida p;
		const SDL_Rect c = p.ElJugador().CajaColision();
		const double suelo = p.Config().ySuelo;
		p.Obstaculos().Agregar(Bloque(c.x - 30 - 0, 30, 30, suelo, 0.0));            // pegado a la izquierda: toca el borde
		p.Actualizar(kDt, false);
		CHECK(p.Estado() == EstadoPartida::Jugando, "obstaculo que solo toca el borde de la caja -> sigue Jugando");
	}
	{
		Partida p;
		const SDL_Rect c = p.ElJugador().CajaColision();
		p.Obstaculos().Agregar(Bloque(c.x - 30 + 1, 30, 30, p.Config().ySuelo, 0.0));   // 1 pixel de solape
		p.Actualizar(kDt, false);
		CHECK(p.Estado() == EstadoPartida::GameOver, "1 pixel de solape -> GameOver inmediato");
	}
	{
		// Saltando por encima del obstaculo no hay colision aunque este en la misma X
		Partida p;
		for (int i = 0; i < 8; ++i) p.Actualizar(kDt, true);            // ya en el aire (~60 px)
		p.Obstaculos().Agregar(Bloque(p.ElJugador().Rectangulo().x + 5, 30, 20, p.Config().ySuelo, 0.0));
		for (int i = 0; i < 4; ++i) p.Actualizar(kDt, true);
		CHECK(p.Estado() == EstadoPartida::Jugando && p.ElJugador().YPies() < p.Config().ySuelo - 20,
		      "por encima del obstaculo (pies sobre su altura) no hay colision");
	}

	// Escenario real: obstaculo que avanza. Sin saltar -> choca; saltando a tiempo -> lo supera
	auto simular = [](int ticSalto) {
		Partida p;
		p.Obstaculos().Agregar(Bloque(300, 40, 40, p.Config().ySuelo, -300.0));
		for (int t = 0; t < 84; ++t) p.Actualizar(kDt, t == ticSalto);
		return p.Estado();
	};
	CHECK(simular(-1) == EstadoPartida::GameOver, "sin saltar, el obstaculo que llega provoca GameOver");
	int supervivientes = 0;
	for (int t = 0; t < 40; ++t) supervivientes += (simular(t) == EstadoPartida::Jugando);
	CHECK(supervivientes >= 5, "existe una ventana de tiempo para saltar y esquivar (el juego es superable)");

	// Estado de Game Over: mundo congelado, reinicio con pausa y flanco, mejor puntuacion
	{
		Partida p;
		for (int i = 0; i < 120; ++i) p.Actualizar(kDt, false);       // acumula puntos (sin obstaculos aun)
		const int antes = p.Puntos();
		CHECK(antes > 0, "la puntuacion sube con la distancia");
		p.Obstaculos().Agregar(Bloque(p.ElJugador().CajaColision().x, 30, 30, p.Config().ySuelo, 0.0));
		p.Actualizar(kDt, false);
		CHECK(p.Estado() == EstadoPartida::GameOver && p.Mejor() >= antes, "GameOver guarda la mejor puntuacion");
		const int congelado = p.Puntos();
		for (int i = 0; i < 10; ++i) p.Actualizar(kDt, false);
		CHECK(p.Puntos() == congelado, "en GameOver la puntuacion y el mundo se congelan");
		p.Actualizar(kDt, true);                                          // pulsa demasiado pronto
		CHECK(p.Estado() == EstadoPartida::GameOver, "una pulsacion inmediata no reinicia (evita reinicios accidentales)");
		p.Actualizar(kDt, false);
		for (int i = 0; i < 40; ++i) p.Actualizar(kDt, false);
		p.Actualizar(kDt, true);                                          // flanco tras la pausa
		CHECK(p.Estado() == EstadoPartida::Jugando && p.Puntos() == 0 && p.Mejor() >= antes && p.Obstaculos().Activos().empty(),
		      "tras la pausa, ESPACIO reinicia: puntos a 0, pista vacia y mejor puntuacion conservada");
	}
}

// ---- Test 3.3 -------------------------------------------------------------------
std::vector<std::uint32_t> LeerRegion(SDL_Renderer* r, int alto)
{
	std::vector<std::uint32_t> px(static_cast<std::size_t>(kAncho) * alto);
	SDL_Rect region{ 0, 0, kAncho, alto };
	SDL_RenderReadPixels(r, &region, SDL_PIXELFORMAT_ARGB8888, px.data(), kAncho * 4);
	return px;
}

std::vector<std::uint32_t> RenderHudLimpio(SDL_Renderer* r, const Partida& p)
{
	Dibujante d(r);
	d.Limpiar(Colores::Fondo);
	Hud::Pintar(d, p, kAncho, kAlto);
	return LeerRegion(r, Hud::kAlturaHud);
}

// Juego completo (sin entrada) para verificar frame a frame bajo el bucle real del motor
class JuegoVerificado : public Juego
{
public:
	Partida partida;
	int frames = 0, frameMalos = 0;
	std::set<std::vector<std::uint32_t>> imagenesDistintas;

	bool Comenzar(NucleoMotor&) override { return true; }
	void Actualizar(double dt) override { partida.Actualizar(dt, false); }
	void Pintar(SDL_Renderer* r) override
	{
		const Dibujante d(r);
		partida.Obstaculos().Pintar(d);
		partida.ElJugador().Pintar(r, nullptr);
		Hud::Pintar(d, partida, kAncho, kAlto);
		const auto real = LeerRegion(r, Hud::kAlturaHud);       // lo que hay en el back buffer
		const auto referencia = RenderHudLimpio(r, partida);    // el HUD solo, sobre fondo limpio
		if (real != referencia) ++frameMalos;
		imagenesDistintas.insert(real);
		++frames;
	}
};

void Test_3_3()
{
	std::puts("Test 3.3: HUD dinamico y estable");
	{
		Partida p;
		const std::string t0 = Hud::TextoPuntos(p);
		for (int i = 0; i < 60; ++i) p.Actualizar(kDt, false);            // 1 segundo
		const std::string t1 = Hud::TextoPuntos(p);
		for (int i = 0; i < 60; ++i) p.Actualizar(kDt, false);
		const std::string t2 = Hud::TextoPuntos(p);
		std::printf("         t=0s '%s'  t=1s '%s'  t=2s '%s'  velocidad '%s'\n", t0.c_str(), t1.c_str(), t2.c_str(),
		            Hud::TextoVelocidad(p).c_str());
		CHECK(t0 != t1 && t1 != t2, "el texto de puntuacion cambia cada segundo");
		CHECK(p.Puntos() > 0 && t0.size() == t1.size() && t1.size() == t2.size(), "puntuacion creciente y de ancho constante (sin saltos de maquetacion)");
		CHECK(p.Velocidad() > 300.0 && p.Velocidad() <= 700.0, "la velocidad aumenta con el tiempo, acotada");
	}

	Ventana ventana(ConfigVentana{ "hud", kAncho, kAlto, false, true });
	SDL_Renderer* r = ventana.Renderizador();
	{
		// Corrupcion de buffer: componer HUD B tras un HUD A distinto da lo mismo que B a secas
		Partida a, b;
		for (int i = 0; i < 300; ++i) b.Actualizar(kDt, false);
		const auto soloB = RenderHudLimpio(r, b);
		RenderHudLimpio(r, a);
		const auto trasA = RenderHudLimpio(r, b);
		CHECK(soloB == trasA, "dibujar un HUD tras otro distinto no deja rastros");
		CHECK(RenderHudLimpio(r, a) != soloB, "y dos puntuaciones distintas producen imagenes distintas");
		Dibujante d(r);
		d.Limpiar(Colores::Fondo);
		const auto vacio = LeerRegion(r, Hud::kAlturaHud);
		CHECK(soloB != vacio, "el HUD es visible (hay pixeles de texto)");

		// El texto se mantiene dentro de la franja reservada y de la pantalla (derecha incluida)
		d.Limpiar(Colores::Fondo);
		Hud::Pintar(d, b, kAncho, kAlto);
		bool fuera = false;
		std::vector<std::uint32_t> todo(static_cast<std::size_t>(kAncho) * kAlto);
		SDL_RenderReadPixels(r, nullptr, SDL_PIXELFORMAT_ARGB8888, todo.data(), kAncho * 4);
		for (int y = Hud::kAlturaHud; y < kAlto; ++y)
			for (int x = 0; x < kAncho; ++x)
				if (todo[static_cast<std::size_t>(y) * kAncho + x] != 0xFF000000u + (Colores::Fondo.r << 16 | Colores::Fondo.g << 8 | Colores::Fondo.b))
					if (y > kAlto / 2 - 70) {} else fuera = true;   // salvo el cartel central de instrucciones
		CHECK(!fuera, "el HUD no invade el resto de la pantalla");
	}
	{
		// Bajo el bucle real del motor, ~2 s a 60 FPS, con GameOver incluido
		JuegoVerificado juego;
		ConfigMotor c;
		c.ventana = ConfigVentana{ "hud-motor", kAncho, kAlto, false, true };
		NucleoMotor motor(c, juego);
		motor.Arrancar();
		for (int i = 0; i < 130; ++i) { motor.EjecutarFrame(); SDL_Delay(16); }
		std::printf("         %d frames, %zu imagenes de HUD distintas, estado final %s\n", juego.frames,
		            juego.imagenesDistintas.size(), juego.partida.Estado() == EstadoPartida::GameOver ? "GAME OVER" : "jugando");
		CHECK(juego.frames >= 100, "se pintaron >=100 frames");
		CHECK(juego.frameMalos == 0, "en cada frame el HUD del back buffer es identico a un HUD limpio (sin parpadeo ni basura)");
		CHECK(juego.imagenesDistintas.size() >= 3, "el HUD se actualiza dinamicamente durante la partida");
	}
}
} // namespace

int main()
{
	try
	{
		Test_3_1();
		Test_3_2();
		Test_3_3();
	}
	catch (const std::exception& e)
	{
		std::printf("  [FAIL] excepcion: %s\n", e.what());
		return 1;
	}
	std::printf(g_fallos == 0 ? "\nTODO OK\n" : "\n%d FALLO(S)\n", g_fallos);
	return g_fallos == 0 ? 0 : 1;
}
