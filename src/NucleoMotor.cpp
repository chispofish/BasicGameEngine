//------------------------------------------------------------------------------------
// NucleoMotor  (Modulo 2: Nucleo y bucle de juego con paso fijo)
// Codigo fuente C++ - NucleoMotor.cpp
//------------------------------------------------------------------------------------
#include "NucleoMotor.h"

#include <algorithm>
#include <stdexcept>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

//------------------------------------------------------------------------------------
// Constructor / Destructor
//------------------------------------------------------------------------------------
NucleoMotor::NucleoMotor(const ConfigMotor& config, Juego& juego)
	: m_config(config)
	, m_juego(juego)
	, m_ventana(config.ventana)
{
	if (m_config.frecuencia == 0)
		throw std::invalid_argument("NucleoMotor: la frecuencia debe ser > 0");
	if (m_config.maxPasosPorFrame == 0)
		m_config.maxPasosPorFrame = 1;

	m_paso = 1000.0 / static_cast<double>(m_config.frecuencia);
}

NucleoMotor::~NucleoMotor()
{
	Terminar();
}

void NucleoMotor::Terminar()
{
	if (m_comenzado && !m_finalizado)
	{
		m_finalizado = true;
		m_juego.Finalizar();
	}
}

//------------------------------------------------------------------------------------
// Bucle principal: nativo = while; navegador = callback asincrono
//------------------------------------------------------------------------------------
bool NucleoMotor::Arrancar()
{
	if (!m_juego.Comenzar(*this))
		return false;

	m_comenzado  = true;
	m_ejecutando = true;
	m_enEspera   = !m_ventana.TieneFoco();
	m_acumulador = 0.0;
	m_ultimoTick = SDL_GetTicks();
	return true;
}

int NucleoMotor::Ejecutar()
{
	if (!Arrancar())
		return 1;

#ifdef __EMSCRIPTEN__
	// fps = 0 -> usa requestAnimationFrame; simulate_infinite_loop = 0 -> main()
	// retorna y el navegador sigue llamando al callback sin bloquear la pestana.
	emscripten_set_main_loop_arg(&NucleoMotor::CallbackWeb, this, 0, 0);
	return 0;
#else
	while (m_ejecutando)
		EjecutarFrame();

	Terminar();
	return 0;
#endif
}

#ifdef __EMSCRIPTEN__
void NucleoMotor::CallbackWeb(void* arg)
{
	auto* motor = static_cast<NucleoMotor*>(arg);
	motor->EjecutarFrame();

	if (!motor->m_ejecutando)
	{
		emscripten_cancel_main_loop();
		motor->Terminar();
	}
}
#endif

//------------------------------------------------------------------------------------
// Un frame: eventos -> logica a paso fijo -> render
//------------------------------------------------------------------------------------
void NucleoMotor::EjecutarFrame()
{
	// Aritmetica sin signo: correcta aunque SDL_GetTicks() desborde (~49 dias)
	const std::uint32_t ahora = SDL_GetTicks();
	std::uint32_t delta = ahora - m_ultimoTick;
	m_ultimoTick = ahora;

	// 1) Eventos de la plataforma
	if (m_ventana.ProcesarEventos().salir)
		m_ejecutando = false;
	if (!m_ejecutando)
		return;

	// 2) Pausa / reanudacion segun el foco (WM_KILLFOCUS / WM_SETFOCUS)
	const bool foco = m_ventana.TieneFoco();
	if (!foco && !m_enEspera)
	{
		m_enEspera = true;
		m_juego.Detener();
	}
	else if (foco && m_enEspera)
	{
		m_enEspera   = false;
		m_acumulador = 0.0;        // el tiempo en pausa no se "recupera" a rafagas
		delta        = 0;          // ni siquiera el de este frame (incluye la espera en pausa)
		m_juego.Reactivar();
	}

	if (m_enEspera)
	{
#ifndef __EMSCRIPTEN__
		SDL_Delay(50);             // sin foco: casi 0% de CPU (el navegador ya limita rAF)
#endif
		return;
	}

	// 3) Logica a tics fijos. El delta se acota para evitar la espiral de la muerte.
	const double maxAcumulado = m_paso * static_cast<double>(m_config.maxPasosPorFrame);
	m_acumulador = std::min(m_acumulador + static_cast<double>(delta), maxAcumulado);

	const double dt = m_paso / 1000.0;
	while (m_ejecutando && m_acumulador >= m_paso)
	{
		m_juego.Actualizar(dt);
		m_acumulador -= m_paso;
		++m_estadisticas.actualizaciones;
	}
	if (!m_ejecutando)
		return;

	// 4) Renderizado libre: una vez por iteracion
	m_juego.Pintar(m_ventana.Renderizador());
	SDL_RenderPresent(m_ventana.Renderizador());
	++m_estadisticas.frames;

#ifndef __EMSCRIPTEN__
	if (!m_ventana.VsyncActivo())
		SDL_Delay(1);              // sin vsync: no quemar un nucleo al 100%
#endif
}
