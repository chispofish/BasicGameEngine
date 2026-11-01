//------------------------------------------------------------------------------------
// NucleoMotor  (Modulo 2: Nucleo y bucle de juego con paso fijo)
// Encabezado C++ - NucleoMotor.h
//------------------------------------------------------------------------------------
// - La logica se actualiza a tics fijos (por defecto 60 Hz) con un acumulador.
// - El renderizado es libre: una vez por iteracion del bucle.
// - Nativo: bucle while estandar. Navegador: emscripten_set_main_loop_arg
//   (callback asincrono, nunca bloquea la pestana).
// - Sin Windows.h: el tiempo se mide con SDL_GetTicks().
//
// IMPORTANTE (Emscripten): Ejecutar() registra el callback y DEVUELVE de inmediato.
// El NucleoMotor y el Juego deben tener duracion estatica (o en el heap), nunca
// vivir en la pila de main(), porque el bucle continua despues de que main() acabe.
//------------------------------------------------------------------------------------
#pragma once

#include "Ventana.h"

#include <cstdint>

class NucleoMotor;

//------------------------------------------------------------------------------------
// Interfaz que implementa el juego (sustituye a las antiguas funciones FJ_*)
//------------------------------------------------------------------------------------
class Juego
{
public:
	virtual ~Juego() = default;

	// FJ_InicializarJuego + FJ_ComenzarJuego. Devolver false aborta el arranque.
	virtual bool Comenzar(NucleoMotor& motor) = 0;

	// FJ_CicloJuego: un tic de logica a paso fijo. 'dt' son segundos (1/frecuencia).
	virtual void Actualizar(double dt) = 0;

	// FJ_PintarJuego(HDC) -> FJ_PintarJuego(SDL_Renderer*). El motor llama a
	// SDL_RenderPresent() despues de este metodo.
	virtual void Pintar(SDL_Renderer* renderizador) = 0;

	// FJ_DetenerJuego / FJ_ReactivarJuego: la ventana pierde / recupera el foco.
	virtual void Detener() {}
	virtual void Reactivar() {}

	// FJ_FinalizarJuego
	virtual void Finalizar() {}
};

//------------------------------------------------------------------------------------
// Configuracion y estadisticas
//------------------------------------------------------------------------------------
struct ConfigMotor
{
	ConfigVentana ventana;
	std::uint32_t frecuencia        = 60;   // tics de logica por segundo
	std::uint32_t maxPasosPorFrame  = 5;    // evita la "espiral de la muerte"
};

struct EstadisticasMotor
{
	std::uint64_t actualizaciones = 0;      // tics de logica ejecutados
	std::uint64_t frames          = 0;      // frames renderizados
};

//------------------------------------------------------------------------------------
// Clase NucleoMotor
//------------------------------------------------------------------------------------
class NucleoMotor
{
public:
	// 'juego' debe sobrevivir al motor. Lanza excepcion si la ventana falla.
	NucleoMotor(const ConfigMotor& config, Juego& juego);
	~NucleoMotor();

	NucleoMotor(const NucleoMotor&)            = delete;
	NucleoMotor& operator=(const NucleoMotor&) = delete;

	// Nativo: bloquea hasta que termina el juego y devuelve el codigo de salida.
	// Web: registra el callback y devuelve 0 enseguida (ver nota de cabecera).
	int Ejecutar();

	// Llama a Juego::Comenzar y deja el motor listo para EjecutarFrame().
	// Ejecutar() ya lo invoca; es publico para poder avanzar frame a frame en pruebas.
	bool Arrancar();

	// Un frame completo: eventos -> N tics de logica -> pintar. Publico para pruebas.
	void EjecutarFrame();

	// Solicita terminar (equivalente a PostQuitMessage)
	void Detener() noexcept { m_ejecutando = false; }

	bool EnEjecucion() const noexcept { return m_ejecutando; }
	bool EnEspera()    const noexcept { return m_enEspera; }   // pausado por perdida de foco

	Ventana&                 ObtenerVentana()  noexcept { return m_ventana; }
	const EstadisticasMotor& Estadisticas() const noexcept { return m_estadisticas; }
	std::uint32_t            Frecuencia() const noexcept { return m_config.frecuencia; }

private:
	void Terminar();                         // llama a Juego::Finalizar una sola vez
#ifdef __EMSCRIPTEN__
	static void CallbackWeb(void* arg);
#endif

	ConfigMotor m_config;
	Juego&      m_juego;
	Ventana     m_ventana;

	double        m_paso         = 0.0;      // ms por tic de logica
	double        m_acumulador   = 0.0;      // ms pendientes de simular
	std::uint32_t m_ultimoTick   = 0;
	bool          m_ejecutando   = false;
	bool          m_enEspera     = false;
	bool          m_comenzado    = false;
	bool          m_finalizado   = false;
	EstadisticasMotor m_estadisticas;
};
