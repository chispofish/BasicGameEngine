//------------------------------------------------------------------------------------
// Pruebas de QA del Modulo 3 (Dibujante + limpieza/presentacion por frame)
// Se ejecutan sin pantalla: SDL_VIDEODRIVER=dummy, SDL_RENDER_DRIVER=software.
//------------------------------------------------------------------------------------
#include "NucleoMotor.h"

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
constexpr int  kAncho = 320, kAlto = 200, kLado = 32;
constexpr std::uint32_t Pack(Color c) { return 0xFF000000u | (c.r << 16) | (c.g << 8) | c.b; }
constexpr std::uint32_t kFondo = Pack(Colores::Fondo);
constexpr std::uint32_t kRojo  = Pack(Colores::Rojo);

// Lee el framebuffer actual del renderizador (ARGB8888)
std::vector<std::uint32_t> Leer(SDL_Renderer* r)
{
	std::vector<std::uint32_t> px(static_cast<size_t>(kAncho) * kAlto, 0);
	SDL_RenderReadPixels(r, nullptr, SDL_PIXELFORMAT_ARGB8888, px.data(), kAncho * 4);
	return px;
}

struct Analisis { int distintos = 0; bool coincide = false; };

// Cuenta pixeles distintos del fondo y comprueba que son exactamente el rectangulo esperado
Analisis Analizar(const std::vector<std::uint32_t>& px, const SDL_Rect& esperado)
{
	Analisis a;
	bool todoRojoDentro = true;
	for (int y = 0; y < kAlto; ++y)
		for (int x = 0; x < kAncho; ++x)
		{
			const std::uint32_t p = px[static_cast<size_t>(y) * kAncho + x];
			const bool dentro = x >= esperado.x && x < esperado.x + esperado.w &&
			                    y >= esperado.y && y < esperado.y + esperado.h;
			if (p != kFondo) ++a.distintos;
			if (dentro && p != kRojo)   todoRojoDentro = false;
			if (!dentro && p != kFondo) todoRojoDentro = false;   // resto/rastro fuera del rectangulo
		}
	a.coincide = todoRojoDentro;
	return a;
}

// Rectangulo que se mueve cada tic de logica; se verifica en cada frame
class JuegoFlicker : public Juego
{
public:
	int framesVerificados = 0, framesMalosBack = 0, framesMalosPresent = 0, posicionesDistintas = 0;
	SDL_Rect ultimo{ 0, 0, kLado, kLado };

	bool Comenzar(NucleoMotor&) override { return true; }
	void Actualizar(double) override
	{
		m_x += 4;
		if (m_x + kLado > kAncho) m_x = 0;
	}
	void Pintar(SDL_Renderer* r) override
	{
		const SDL_Rect nuevo{ m_x, 60 + (m_x / 8) % 40, kLado, kLado };   // se mueve en X e Y
		if (nuevo.x != ultimo.x || nuevo.y != ultimo.y) ++posicionesDistintas;
		ultimo = nuevo;
		Dibujante(r).RectanguloRelleno(ultimo, Colores::Rojo);

		// Back buffer, antes de presentar
		if (!Analizar(Leer(r), ultimo).coincide) ++framesMalosBack;
		++framesVerificados;
	}
	int m_x = 0;
};

// Ejecuta 'frames' frames a ~60 FPS reales y verifica tambien el buffer tras Present
void CorrerYVerificar(NucleoMotor& m, JuegoFlicker& j, int frames)
{
	m.Arrancar();
	while (j.framesVerificados < frames)
	{
		const int antes = j.framesVerificados;
		m.EjecutarFrame();
		if (j.framesVerificados != antes && !Analizar(Leer(m.ObtenerVentana().Renderizador()), j.ultimo).coincide)
			++j.framesMalosPresent;
		SDL_Delay(16);   // ~60 FPS
	}
}

ConfigMotor Config(bool limpiar)
{
	ConfigMotor c;
	c.ventana = ConfigVentana{ "grafico", kAncho, kAlto, false, true };
	c.limpiarCadaFrame = limpiar;
	return c;
}

//---------------------------------------------------------------- Test 3.1
void Test_3_1_SinParpadeo()
{
	std::puts("Test 3.1: sin parpadeo ni rastros (back buffer y tras Present), ~60 FPS");
	{
		JuegoFlicker j; NucleoMotor m(Config(true), j);
		CorrerYVerificar(m, j, 120);
		std::printf("         %d frames, %d posiciones distintas\n", j.framesVerificados, j.posicionesDistintas);
		CHECK(j.framesMalosBack == 0,    "back buffer: solo fondo + rectangulo exacto en los 120 frames");
		CHECK(j.framesMalosPresent == 0, "tras Present: identico al frame pintado en los 120 frames");
		CHECK(j.posicionesDistintas >= 100, "el rectangulo se mueve casi cada frame (animacion real)");
	}
	{
		// Control negativo: sin limpiar el fondo DEBEN aparecer rastros; si no, la prueba no sirve
		JuegoFlicker j; NucleoMotor m(Config(false), j);
		CorrerYVerificar(m, j, 30);
		CHECK(j.framesMalosBack > 0, "control negativo: sin limpiar, la prueba detecta los rastros");
	}
}

//---------------------------------------------------------------- Helpers de dibujo
void Test_3_x_Primitivas()
{
	std::puts("Test 3.2: primitivas y texto del Dibujante");
	Ventana v(ConfigVentana{ "prim", kAncho, kAlto, false, true });
	SDL_Renderer* r = v.Renderizador();
	const Dibujante d(r);
	auto contar = [&] { int n = 0; for (auto p : Leer(r)) n += (p != kFondo); return n; };
	auto pix = [&](int x, int y) { return Leer(r)[static_cast<size_t>(y) * kAncho + x]; };

	d.Limpiar(Colores::Fondo);
	CHECK(contar() == 0, "Limpiar deja todo el buffer con el color de fondo");

	d.RectanguloRelleno(SDL_Rect{ 10, 10, 20, 10 }, Colores::Rojo);
	CHECK(contar() == 200, "RectanguloRelleno 20x10 = 200 pixeles");

	d.Limpiar(Colores::Fondo);
	d.Rectangulo(SDL_Rect{ 10, 10, 20, 10 }, Colores::Rojo);
	CHECK(contar() == 2 * 20 + 2 * 10 - 4, "Rectangulo (borde) 20x10 = 56 pixeles");

	d.Limpiar(Colores::Fondo);
	d.Linea(0, 5, 99, 5, Colores::Rojo);
	CHECK(contar() == 100, "Linea horizontal de 100 pixeles");

	d.Limpiar(Colores::Fondo);
	d.Punto(7, 9, Colores::Rojo);
	CHECK(contar() == 1 && pix(7, 9) == kRojo, "Punto dibuja exactamente un pixel");

	d.Limpiar(Colores::Fondo);
	const int ancho = d.Texto(20, 20, "0", Colores::Rojo, 2);
	CHECK(ancho == 10, "Texto('0', escala 2) devuelve ancho 10");
	CHECK(pix(20 + 2 * 2, 20) == kRojo && pix(20, 20) == kFondo, "glifo '0': arriba-centro encendido, esquina apagada");
	CHECK(Dibujante::AnchoTexto("AB") == 11 && Dibujante::AnchoTexto("A\nABC") == 17, "AnchoTexto con y sin saltos de linea");

	d.Limpiar(Colores::Fondo);
	d.Texto(0, 0, "abc", Colores::Rojo);
	const int minusculas = contar();
	d.Limpiar(Colores::Fondo);
	d.Texto(0, 0, "ABC", Colores::Rojo);
	CHECK(minusculas > 0 && minusculas == contar(), "las minusculas se dibujan como mayusculas");
}
} // namespace

int main()
{
	Test_3_1_SinParpadeo();
	Test_3_x_Primitivas();
	std::printf("\n%s (%d fallos)\n", g_fallos == 0 ? "TODAS LAS PRUEBAS PASARON" : "HAY FALLOS", g_fallos);
	return g_fallos == 0 ? 0 : 1;
}
