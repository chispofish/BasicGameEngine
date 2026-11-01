//------------------------------------------------------------------------------------
// runner.cpp - Juego Runner completo (nativo y Emscripten)
//   ESPACIO / FLECHA ARRIBA: saltar (y reiniciar tras GAME OVER). Esquiva los bugs rojos.
//   Integra Textura (M1), Jugador (M2), GestorObstaculos/Partida/Hud (M3) en los hooks
//   del motor: Actualizar(dt) a 60 Hz fijos y Pintar(SDL_Renderer*).
//------------------------------------------------------------------------------------
#include "Hud.h"
#include "NucleoMotor.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <memory>

// Nativo: ruta absoluta inyectada por CMake. Web: --preload-file monta la carpeta
// en /assets del sistema de archivos virtual, y el directorio de trabajo es "/".
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

		ConfigPartida cfg;
		cfg.ancho  = m_ventana->Ancho();
		cfg.ySuelo = m_ventana->Altura() - 80.0;
		m_partida  = std::make_unique<Partida>(cfg, SDL_GetTicks());   // semilla distinta por sesion
		return true;
	}

	void Actualizar(double dt) override
	{
		const Ventana& v = *m_ventana;
		const bool saltar = v.TeclaPulsada(SDL_SCANCODE_SPACE) || v.TeclaPulsada(SDL_SCANCODE_UP) ||
		                    v.TeclaPulsada(SDL_SCANCODE_W);
		m_partida->Actualizar(dt, saltar);
		if (m_partida->Estado() == EstadoPartida::Jugando)
			m_scroll = std::fmod(m_scroll + m_partida->Velocidad() * dt, 64.0);
	}

	void Pintar(SDL_Renderer* r) override
	{
		const Dibujante d(r);          // el motor ya limpio el fondo
		const int ancho = m_ventana->Ancho(), alto = m_ventana->Altura();
		const int suelo = static_cast<int>(m_partida->Config().ySuelo);

		d.RectanguloRelleno(SDL_Rect{ 0, suelo, ancho, alto - suelo }, Color{ 40, 48, 76, 255 });
		d.Linea(0, suelo, ancho, suelo, Colores::Gris);
		for (int x = -static_cast<int>(m_scroll); x < ancho; x += 64)
			d.Linea(x, suelo + 20, x + 24, suelo + 20, Colores::Gris);

		m_partida->Obstaculos().Pintar(d);
		m_partida->ElJugador().Pintar(r, m_hoja.get());
		Hud::Pintar(d, *m_partida, ancho, alto);
	}

private:
	Ventana*                 m_ventana = nullptr;
	std::unique_ptr<Textura> m_hoja;
	std::unique_ptr<Partida> m_partida;
	double                   m_scroll = 0.0;
};
} // namespace

int main(int, char**)
{
	try
	{
		ConfigMotor config;
		config.ventana = ConfigVentana{ "Runner", 640, 480, true,
#ifdef __EMSCRIPTEN__
			false      // en el navegador ESC no cierra el juego
#else
			true
#endif
		};
		// Duracion estatica: en Emscripten el bucle sigue vivo cuando main() retorna.
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
