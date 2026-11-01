//------------------------------------------------------------------------------------
// main.cpp - Juego de demostracion minimo (nativo y Emscripten)
//------------------------------------------------------------------------------------
#include "NucleoMotor.h"

#include <exception>
#include <iostream>

namespace
{
// Cuadrado que rebota: valida que logica (paso fijo) y render funcionan juntos.
class JuegoDemo : public Juego
{
public:
	bool Comenzar(NucleoMotor& motor) override
	{
		m_ancho  = motor.ObtenerVentana().Ancho();
		m_altura = motor.ObtenerVentana().Altura();
		return true;
	}

	void Actualizar(double dt) override
	{
		m_x += m_vx * dt;
		m_y += m_vy * dt;
		if (m_x < 0 || m_x + kLado > m_ancho)  { m_vx = -m_vx; m_x = m_x < 0 ? 0 : m_ancho - kLado; }
		if (m_y < 0 || m_y + kLado > m_altura) { m_vy = -m_vy; m_y = m_y < 0 ? 0 : m_altura - kLado; }
	}

	void Pintar(SDL_Renderer* r) override
	{
		SDL_SetRenderDrawColor(r, 20, 24, 40, 255);
		SDL_RenderClear(r);
		SDL_SetRenderDrawColor(r, 240, 180, 40, 255);
		const SDL_Rect rect{ static_cast<int>(m_x), static_cast<int>(m_y), kLado, kLado };
		SDL_RenderFillRect(r, &rect);
	}

private:
	static constexpr int kLado = 48;
	int    m_ancho = 0, m_altura = 0;
	double m_x = 50, m_y = 50, m_vx = 180, m_vy = 130;   // pixeles por segundo
};
} // namespace

int main(int, char**)
{
	try
	{
		// Duracion estatica: en Emscripten el bucle sigue vivo cuando main() retorna.
		static JuegoDemo juego;
		static NucleoMotor motor(ConfigMotor{ ConfigVentana{ "BasicGameEngine", 640, 480, true }, 60, 5 }, juego);
		return motor.Ejecutar();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error fatal: " << e.what() << std::endl;
		return 1;
	}
}
