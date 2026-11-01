//------------------------------------------------------------------------------------
// Pruebas de QA de los Modulos 1 y 2 (se ejecutan sin pantalla real:
// SDL_VIDEODRIVER=dummy, SDL_RENDER_DRIVER=software; ver CMakeLists.txt)
//------------------------------------------------------------------------------------
#include "NucleoMotor.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

static int g_fallos = 0;
#define CHECK(cond, msg)                                                           \
	do {                                                                           \
		if (cond) std::printf("  [ OK ] %s\n", msg);                               \
		else { std::printf("  [FAIL] %s  (%s:%d)\n", msg, __FILE__, __LINE__); ++g_fallos; } \
	} while (0)

namespace
{
class JuegoPrueba : public Juego
{
public:
	std::uint32_t retardoPintarMs = 0;     // simula un render lento
	std::uint64_t pausas = 0, reanudaciones = 0, pintados = 0;
	std::uint64_t paraAlLlegar = 0;        // si > 0: pide SDL_QUIT al alcanzar N tics
	bool finalizado = false;
	std::uint64_t tics = 0;

	bool Comenzar(NucleoMotor&) override { return true; }
	void Actualizar(double) override
	{
		if (++tics == paraAlLlegar)
		{
			SDL_Event e{}; e.type = SDL_QUIT; SDL_PushEvent(&e);
		}
	}
	void Pintar(SDL_Renderer* r) override
	{
		++pintados;
		SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
		SDL_RenderClear(r);
		if (retardoPintarMs) SDL_Delay(retardoPintarMs);
	}
	void Detener() override   { ++pausas; }
	void Reactivar() override { ++reanudaciones; }
	void Finalizar() override { finalizado = true; }
};

ConfigMotor Config(bool vsync = false)
{
	return ConfigMotor{ ConfigVentana{ "Titulo de prueba", 320, 200, vsync }, 60, 5 };
}

void PushFoco(Uint8 evento)
{
	SDL_Event e{};
	e.type = SDL_WINDOWEVENT;
	e.window.event = evento;
	SDL_PushEvent(&e);
}

// Ejecuta frames durante 'ms' milisegundos reales
void Correr(NucleoMotor& m, std::uint32_t ms)
{
	const std::uint32_t t0 = SDL_GetTicks();
	while (SDL_GetTicks() - t0 < ms) m.EjecutarFrame();
}

//---------------------------------------------------------------- Test 1.1
void Test_1_1_VentanaSeAbre()
{
	std::puts("Test 1.1: ventana con titulo y resolucion configurados");
	JuegoPrueba j;
	NucleoMotor m(Config(), j);
	m.Arrancar();
	SDL_Window* v = m.ObtenerVentana().VentanaSdl();
	int w = 0, h = 0;
	SDL_GetWindowSize(v, &w, &h);
	CHECK(v != nullptr && m.ObtenerVentana().Renderizador() != nullptr, "ventana y renderizador creados");
	CHECK(std::string(SDL_GetWindowTitle(v)) == "Titulo de prueba", "titulo correcto");
	CHECK(w == 320 && h == 200, "resolucion 320x200");
}

//---------------------------------------------------------------- Test 1.2
void Test_1_2_Cierre()
{
	std::puts("Test 1.2: cierre por SDL_QUIT / ESC / ventana y liberacion limpia");
	{   // SDL_QUIT durante Ejecutar() (bucle nativo completo)
		JuegoPrueba j; j.paraAlLlegar = 10;
		NucleoMotor m(Config(), j);
		const int codigo = m.Ejecutar();
		CHECK(codigo == 0 && !m.EnEjecucion(), "SDL_QUIT termina Ejecutar() con codigo 0");
		CHECK(j.finalizado, "Juego::Finalizar() se llamo");
	}
	{   // ESC
		JuegoPrueba j;
		Ventana v(ConfigVentana{ "esc", 100, 100, false });
		SDL_Event e{}; e.type = SDL_KEYDOWN; e.key.keysym.scancode = SDL_SCANCODE_ESCAPE;
		SDL_PushEvent(&e);
		CHECK(v.ProcesarEventos().salir, "tecla ESC solicita salir");
	}
	{   // Boton de cierre de la ventana
		Ventana v(ConfigVentana{ "x", 100, 100, false });
		PushFoco(SDL_WINDOWEVENT_CLOSE);
		CHECK(v.ProcesarEventos().salir, "boton X (WINDOWEVENT_CLOSE) solicita salir");
	}
	{   // Crear/destruir varias veces: sin fugas (verificado con ASan/LSan)
		for (int i = 0; i < 5; ++i) { JuegoPrueba j; NucleoMotor m(Config(), j); m.Arrancar(); m.EjecutarFrame(); }
		CHECK(true, "5 ciclos crear/destruir sin errores (ASan/LSan detectaria fugas)");
	}
}

//---------------------------------------------------------------- Test 1.3
void Test_1_3_Foco()
{
	std::puts("Test 1.3: perdida y recuperacion de foco");
	JuegoPrueba j;
	NucleoMotor m(Config(), j);
	m.Arrancar();

	Correr(m, 200);
	CHECK(j.tics > 0 && !m.EnEspera(), "con foco la logica avanza");

	PushFoco(SDL_WINDOWEVENT_FOCUS_LOST);
	m.EjecutarFrame();
	CHECK(m.EnEspera() && j.pausas == 1, "FOCUS_LOST pausa el motor y llama a Juego::Detener");

	const std::uint64_t ticsPausa = j.tics, pintadosPausa = j.pintados;
	const std::uint32_t t0 = SDL_GetTicks();
	std::uint64_t iteraciones = 0;
	while (SDL_GetTicks() - t0 < 500) { m.EjecutarFrame(); ++iteraciones; }
	CHECK(j.tics == ticsPausa && j.pintados == pintadosPausa, "en pausa no hay logica ni render");
	CHECK(iteraciones < 30, "en pausa el bucle duerme (poca CPU: <30 iteraciones en 500 ms)");

	PushFoco(SDL_WINDOWEVENT_FOCUS_GAINED);
	m.EjecutarFrame();
	CHECK(!m.EnEspera() && j.reanudaciones == 1, "FOCUS_GAINED reanuda y llama a Juego::Reactivar");
	CHECK(j.tics - ticsPausa <= 1, "al reanudar no hay rafaga de tics por el tiempo en pausa");
}

//---------------------------------------------------------------- Test 2.1
void Test_2_1_FrameratEstable()
{
	std::puts("Test 2.1: tasa logica estable (60 Hz) aunque el render fluctue");
	for (std::uint32_t retardo : { 0u, 10u, 30u })
	{
		JuegoPrueba j; j.retardoPintarMs = retardo;
		NucleoMotor m(Config(), j);
		m.Arrancar();
		const std::uint32_t t0 = SDL_GetTicks();
		Correr(m, 2000);
		const double seg = (SDL_GetTicks() - t0) / 1000.0;
		const double hz = static_cast<double>(m.Estadisticas().actualizaciones) / seg;
		const double fps = static_cast<double>(m.Estadisticas().frames) / seg;
		std::printf("         render=%2u ms/frame -> logica %.1f Hz, render %.1f fps\n", retardo, hz, fps);
		CHECK(std::fabs(hz - 60.0) <= 2.0, "logica = 60 Hz +/- 2");
	}
}
} // namespace

int main()
{
	Test_1_1_VentanaSeAbre();
	Test_1_2_Cierre();
	Test_1_3_Foco();
	Test_2_1_FrameratEstable();
	std::printf("\n%s (%d fallos)\n", g_fallos == 0 ? "TODAS LAS PRUEBAS PASARON" : "HAY FALLOS", g_fallos);
	return g_fallos == 0 ? 0 : 1;
}
