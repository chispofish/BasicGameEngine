//------------------------------------------------------------------------------------
// runner.cpp - Banco de pruebas visual del Runner (Modulos 1 y 2)
//   Avatar animado que salta con ESPACIO / FLECHA ARRIBA sobre un suelo con
//   lineas que se desplazan. Los obstaculos y el HUD llegan en el Modulo 3.
//------------------------------------------------------------------------------------
#include "Jugador.h"
#include "NucleoMotor.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <memory>

#ifndef BGE_ASSETS_DIR
#define BGE_ASSETS_DIR "assets"
#endif

namespace
{
class JuegoRunner : public Juego
{
public:
	bool Comenzar(NucleoMotor& motor) override
	{
		m_ventana = &motor.ObtenerVentana();
		m_hoja    = Textura::Cargar(m_ventana->Renderizador(), BGE_ASSETS_DIR "/avatar_run.bmp");
		if (!m_hoja)   // no abortamos: Jugador::Pintar usa un rectangulo de respaldo
			std::cerr << "Aviso: " << Textura::UltimoError() << std::endl;
		m_jugador = Jugador(ConfigJugador{ 80, m_ventana->Altura() - 80.0 });
		return true;
	}

	void Actualizar(double dt) override
	{
		const Ventana& v = *m_ventana;
		const bool salto = v.TeclaPulsada(SDL_SCANCODE_SPACE) || v.TeclaPulsada(SDL_SCANCODE_UP);
		m_jugador.Actualizar(dt, salto);
		m_scroll = std::fmod(m_scroll + 300.0 * dt, 64.0);
	}

	void Pintar(SDL_Renderer* r) override
	{
		const Dibujante d(r);
		const int suelo = static_cast<int>(m_jugador.Config().ySuelo);
		d.RectanguloRelleno(SDL_Rect{ 0, suelo, m_ventana->Ancho(), m_ventana->Altura() - suelo }, Color{ 40, 48, 76, 255 });
		d.Linea(0, suelo, m_ventana->Ancho(), suelo, Colores::Gris);
		for (int x = -64 + static_cast<int>(64 - m_scroll); x < m_ventana->Ancho(); x += 64)
			d.Linea(x, suelo + 20, x + 24, suelo + 20, Colores::Gris);

		m_jugador.Pintar(r, m_hoja.get());
		d.Texto(10, 10, "ESPACIO / FLECHA ARRIBA: SALTAR", Colores::Blanco, 1);
	}

private:
	Ventana*                 m_ventana = nullptr;
	std::unique_ptr<Textura> m_hoja;      // se destruye con el juego, antes que el renderizador
	Jugador                  m_jugador;
	double                   m_scroll = 0.0;
};
} // namespace

int main(int, char**)
{
	try
	{
		ConfigMotor config;
		config.ventana = ConfigVentana{ "Runner", 640, 480, true, true };
		static JuegoRunner juego;
		static NucleoMotor motor(config, juego);
		return motor.Ejecutar();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error fatal: " << e.what() << std::endl;
		return 1;
	}
}
