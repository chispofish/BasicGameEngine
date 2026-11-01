//------------------------------------------------------------------------------------
// main.cpp - Juego de demostracion (nativo y Emscripten)
//   - Cuadrado amarillo que rebota (logica a paso fijo, 60 Hz)
//   - Cuadrado azul controlable con las flechas / WASD
//   - Texto con FPS y tics de logica, rejilla de fondo y marco
//------------------------------------------------------------------------------------
#include "NucleoMotor.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>

namespace
{
class JuegoDemo : public Juego
{
public:
	bool Comenzar(NucleoMotor& motor) override
	{
		m_motor  = &motor;
		m_ventana = &motor.ObtenerVentana();
		m_ancho  = m_ventana->Ancho();
		m_altura = m_ventana->Altura();
		m_jx = m_ancho / 2.0;
		m_jy = m_altura / 2.0;
		m_marcaFps = SDL_GetTicks();
		return true;
	}

	void Actualizar(double dt) override
	{
		// Cuadrado que rebota
		m_x += m_vx * dt;
		m_y += m_vy * dt;
		if (m_x < 0 || m_x + kLado > m_ancho)  { m_vx = -m_vx; m_x = std::clamp<double>(m_x, 0, m_ancho - kLado); }
		if (m_y < 0 || m_y + kLado > m_altura) { m_vy = -m_vy; m_y = std::clamp<double>(m_y, 0, m_altura - kLado); }

		// Cuadrado del jugador (flechas o WASD)
		const Ventana& v = *m_ventana;
		const double dx = (v.TeclaPulsada(SDL_SCANCODE_RIGHT) || v.TeclaPulsada(SDL_SCANCODE_D) ? 1 : 0)
		                - (v.TeclaPulsada(SDL_SCANCODE_LEFT)  || v.TeclaPulsada(SDL_SCANCODE_A) ? 1 : 0);
		const double dy = (v.TeclaPulsada(SDL_SCANCODE_DOWN)  || v.TeclaPulsada(SDL_SCANCODE_S) ? 1 : 0)
		                - (v.TeclaPulsada(SDL_SCANCODE_UP)    || v.TeclaPulsada(SDL_SCANCODE_W) ? 1 : 0);
		m_jx = std::clamp<double>(m_jx + dx * 260.0 * dt, 0, m_ancho - kLado);
		m_jy = std::clamp<double>(m_jy + dy * 260.0 * dt, 0, m_altura - kLado);
	}

	void Pintar(SDL_Renderer* renderizador) override
	{
		const Dibujante d(renderizador);   // el motor ya limpio el fondo

		// Rejilla
		for (int x = 0; x <= m_ancho; x += 80) d.Linea(x, 0, x, m_altura, Color{ 36, 42, 66, 255 });
		for (int y = 0; y <= m_altura; y += 80) d.Linea(0, y, m_ancho, y, Color{ 36, 42, 66, 255 });

		// Cuadrados
		d.RectanguloRelleno(SDL_Rect{ int(m_x), int(m_y), kLado, kLado }, Colores::Amarillo);
		d.RectanguloRelleno(SDL_Rect{ int(m_jx), int(m_jy), kLado, kLado }, Colores::Azul);
		d.Rectangulo(SDL_Rect{ int(m_jx), int(m_jy), kLado, kLado }, Colores::Blanco);
		d.Rectangulo(SDL_Rect{ 0, 0, m_ancho, m_altura }, Colores::Gris);

		// FPS (se recalcula cada segundo)
		++m_framesVentana;
		const std::uint32_t ahora = SDL_GetTicks();
		if (ahora - m_marcaFps >= 1000)
		{
			m_fps = m_framesVentana * 1000.0 / (ahora - m_marcaFps);
			m_framesVentana = 0;
			m_marcaFps = ahora;
		}
		d.Texto(10, 10, "BASICGAMEENGINE", Colores::Blanco, 2);
		d.Texto(10, 34, "FPS: " + std::to_string(int(m_fps + 0.5)) +
		                "  LOGICA: " + std::to_string(m_motor->Frecuencia()) + " HZ", Colores::Verde, 1);
		d.Texto(10, m_altura - 18, "FLECHAS / WASD: MOVER EL CUADRADO AZUL", Colores::Gris, 1);
	}

private:
	static constexpr int kLado = 48;
	NucleoMotor* m_motor = nullptr;
	Ventana*     m_ventana = nullptr;
	int    m_ancho = 0, m_altura = 0;
	double m_x = 50, m_y = 90, m_vx = 180, m_vy = 130;   // pixeles por segundo
	double m_jx = 0, m_jy = 0;
	std::uint32_t m_marcaFps = 0, m_framesVentana = 0;
	double m_fps = 0;
};
} // namespace

int main(int, char**)
{
	try
	{
		ConfigMotor config;
		config.ventana = ConfigVentana{ "BasicGameEngine", 640, 480, true,
#ifdef __EMSCRIPTEN__
			false      // en el navegador ESC no cierra el juego (sale de pantalla completa)
#else
			true
#endif
		};

		// Duracion estatica: en Emscripten el bucle sigue vivo cuando main() retorna.
		static JuegoDemo juego;
		static NucleoMotor motor(config, juego);
		return motor.Ejecutar();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error fatal: " << e.what() << std::endl;
		return 1;
	}
}
