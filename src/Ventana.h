//------------------------------------------------------------------------------------
// Ventana  (Modulo 1: Plataforma y Ventana)
// Encabezado C++ - Ventana.h
//------------------------------------------------------------------------------------
// Envoltorio RAII de SDL2: inicializa el subsistema de video/eventos, crea la
// ventana y el renderizador, traduce los eventos del sistema (cierre, foco,
// teclado y raton) y libera todo en orden inverso al destruirse.
// Sin dependencias de Windows; funciona en nativo y en Emscripten.
//------------------------------------------------------------------------------------
#pragma once

#include <SDL.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

//------------------------------------------------------------------------------------
// Configuracion de la ventana
//------------------------------------------------------------------------------------
struct ConfigVentana
{
	std::string titulo = "BasicGameEngine";
	int         ancho  = 640;
	int         altura = 480;
	bool        vsync  = true;   // se pide sincronizacion vertical si esta disponible
	bool        escSale = true;  // ESC cierra el programa (en web conviene false)
};

//------------------------------------------------------------------------------------
// Resultado de procesar la cola de eventos en un frame
//------------------------------------------------------------------------------------
struct EventosFrame
{
	bool salir = false;          // SDL_QUIT, cierre de ventana o tecla ESC
};

//------------------------------------------------------------------------------------
// Dedo apoyado en la pantalla (coordenadas en pixeles de la ventana)
//------------------------------------------------------------------------------------
struct Toque
{
	std::int64_t id = 0;
	int x = 0, y = 0;
};

//------------------------------------------------------------------------------------
// Clase Ventana
//------------------------------------------------------------------------------------
class Ventana
{
public:
	// Lanza std::runtime_error si SDL no puede inicializarse o crear la ventana
	explicit Ventana(const ConfigVentana& config);
	~Ventana() = default;

	Ventana(const Ventana&)            = delete;
	Ventana& operator=(const Ventana&) = delete;
	Ventana(Ventana&&)                 = delete;
	Ventana& operator=(Ventana&&)      = delete;

	// Vacia la cola de eventos de SDL y actualiza el estado interno
	EventosFrame ProcesarEventos();

	// Acceso (no propietario) a los objetos de SDL
	SDL_Window*   VentanaSdl()   const noexcept { return m_ventana.get(); }
	SDL_Renderer* Renderizador() const noexcept { return m_renderizador.get(); }

	// Datos de la ventana
	const std::string& Titulo() const noexcept { return m_config.titulo; }
	int  Ancho()  const noexcept { return m_config.ancho; }
	int  Altura() const noexcept { return m_config.altura; }
	bool VsyncActivo() const noexcept { return m_vsyncActivo; }

	// Foco de la ventana (SDL_WINDOWEVENT_FOCUS_GAINED / FOCUS_LOST)
	bool TieneFoco() const noexcept { return m_tieneFoco; }

	// Entrada de teclado y raton
	bool TeclaPulsada(SDL_Scancode tecla) const noexcept;
	bool BotonRaton(std::uint8_t boton) const noexcept;   // SDL_BUTTON_LEFT, ...
	int  RatonX() const noexcept { return m_ratonX; }
	int  RatonY() const noexcept { return m_ratonY; }

	// Pantalla tactil. Los dedos NO generan eventos de raton (SDL_HINT_TOUCH_MOUSE_EVENTS=0).
	//  - ToquesActivos: dedos apoyados ahora mismo (maximo kMaxToques).
	//  - TomarToquesNuevos: posiciones donde se POSO un dedo desde la ultima llamada. Se
	//    conservan hasta que alguien los toma, asi un toque rapido (poner y quitar el dedo
	//    entre dos tics de logica) no se pierde nunca.
	//  - UltimaEntradaTactil: true si lo ultimo que uso el jugador fue un dedo (false: teclado).
	static constexpr std::size_t kMaxToques = 10;
	const std::vector<Toque>& ToquesActivos() const noexcept { return m_toques; }
	std::vector<SDL_Point>    TomarToquesNuevos();
	bool UltimaEntradaTactil() const noexcept { return m_entradaTactil; }
	void MarcarEntradaTactil(bool tactil) noexcept { m_entradaTactil = tactil; }   // p. ej. al detectar un movil

private:
	// Subsistema SDL: se declara primero para destruirse el ultimo
	struct ContextoSdl
	{
		ContextoSdl();
		~ContextoSdl();
		ContextoSdl(const ContextoSdl&)            = delete;
		ContextoSdl& operator=(const ContextoSdl&) = delete;
	};

	struct EliminaVentana
	{
		void operator()(SDL_Window* v) const noexcept { SDL_DestroyWindow(v); }
	};
	struct EliminaRenderizador
	{
		void operator()(SDL_Renderer* r) const noexcept { SDL_DestroyRenderer(r); }
	};

	void LiberarEntrada() noexcept;

	ConfigVentana m_config;
	ContextoSdl   m_contexto;
	std::unique_ptr<SDL_Window, EliminaVentana>         m_ventana;
	std::unique_ptr<SDL_Renderer, EliminaRenderizador>  m_renderizador;

	bool m_vsyncActivo = false;
	bool m_tieneFoco   = true;
	int  m_ratonX      = 0;
	int  m_ratonY      = 0;
	std::uint32_t m_botonesRaton = 0;
	std::vector<Toque>     m_toques;
	std::vector<SDL_Point> m_toquesNuevos;
	bool m_entradaTactil = false;
	std::array<bool, SDL_NUM_SCANCODES> m_teclas{};
};
