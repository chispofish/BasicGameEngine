//------------------------------------------------------------------------------------
// Ventana  (Modulo 1: Plataforma y Ventana)
// Codigo fuente C++ - Ventana.cpp
//------------------------------------------------------------------------------------
#include "Ventana.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

//------------------------------------------------------------------------------------
// ContextoSdl
//------------------------------------------------------------------------------------
Ventana::ContextoSdl::ContextoSdl()
{
	// Los dedos se tratan como dedos (SDL_FINGER*), sin duplicarlos como clics de raton
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
		throw std::runtime_error(std::string("SDL_Init fallo: ") + SDL_GetError());
}

Ventana::ContextoSdl::~ContextoSdl()
{
	SDL_Quit();
}

//------------------------------------------------------------------------------------
// Constructor: crea ventana y renderizador (reemplaza RegisterClassEx/CreateWindow)
//------------------------------------------------------------------------------------
Ventana::Ventana(const ConfigVentana& config)
	: m_config(config)
{
	if (m_config.ancho <= 0 || m_config.altura <= 0)
		throw std::invalid_argument("Ventana: dimensiones invalidas");

	m_ventana.reset(SDL_CreateWindow(m_config.titulo.c_str(),
	                                 SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                                 m_config.ancho, m_config.altura,
	                                 SDL_WINDOW_SHOWN));
	if (!m_ventana)
		throw std::runtime_error(std::string("SDL_CreateWindow fallo: ") + SDL_GetError());

	// Renderizador acelerado; si no existe, se recurre al de software
	Uint32 flags = SDL_RENDERER_ACCELERATED;
	if (m_config.vsync)
		flags |= SDL_RENDERER_PRESENTVSYNC;

	m_renderizador.reset(SDL_CreateRenderer(m_ventana.get(), -1, flags));
	if (!m_renderizador)
		m_renderizador.reset(SDL_CreateRenderer(m_ventana.get(), -1, SDL_RENDERER_SOFTWARE));
	if (!m_renderizador)
		throw std::runtime_error(std::string("SDL_CreateRenderer fallo: ") + SDL_GetError());

	SDL_RendererInfo info;
	if (SDL_GetRendererInfo(m_renderizador.get(), &info) == 0)
		// El renderizador de software informa PRESENTVSYNC aunque no se pida: solo se
		// considera activo si se solicito Y el renderizador lo confirma.
		m_vsyncActivo = m_config.vsync && (info.flags & SDL_RENDERER_PRESENTVSYNC) != 0;
}

//------------------------------------------------------------------------------------
// Eventos (reemplaza WindowProc / TratarMensajes)
//------------------------------------------------------------------------------------
EventosFrame Ventana::ProcesarEventos()
{
	EventosFrame resultado;
	SDL_Event e;

	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
		case SDL_QUIT:
			resultado.salir = true;
			break;

		case SDL_WINDOWEVENT:
			switch (e.window.event)
			{
			case SDL_WINDOWEVENT_CLOSE:
				resultado.salir = true;
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				m_tieneFoco = false;
				LiberarEntrada();     // evita teclas "pegadas" al perder el foco
				break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				m_tieneFoco = true;
				break;
			default:
				break;
			}
			break;

		case SDL_FINGERDOWN:
		case SDL_FINGERMOTION:
		case SDL_FINGERUP:
		{
			m_entradaTactil = true;
			const int x = static_cast<int>(std::lround(e.tfinger.x * m_config.ancho));
			const int y = static_cast<int>(std::lround(e.tfinger.y * m_config.altura));
			const std::int64_t id = e.tfinger.fingerId;
			auto it = std::find_if(m_toques.begin(), m_toques.end(), [id](const Toque& t) { return t.id == id; });
			if (e.type == SDL_FINGERUP)
			{
				if (it != m_toques.end()) m_toques.erase(it);
				break;
			}
			if (it != m_toques.end()) { it->x = x; it->y = y; }
			else if (m_toques.size() < kMaxToques) m_toques.push_back(Toque{ id, x, y });
			if (e.type == SDL_FINGERDOWN)
			{
				if (m_toquesNuevos.size() >= 16) m_toquesNuevos.erase(m_toquesNuevos.begin());   // acotado
				m_toquesNuevos.push_back(SDL_Point{ x, y });
			}
			break;
		}

		case SDL_KEYDOWN:
			m_entradaTactil = false;
			if (m_config.escSale && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
				resultado.salir = true;
			if (e.key.keysym.scancode < SDL_NUM_SCANCODES)
				m_teclas[e.key.keysym.scancode] = true;
			break;

		case SDL_KEYUP:
			if (e.key.keysym.scancode < SDL_NUM_SCANCODES)
				m_teclas[e.key.keysym.scancode] = false;
			break;

		case SDL_MOUSEMOTION:
			m_ratonX = e.motion.x;
			m_ratonY = e.motion.y;
			break;

		case SDL_MOUSEBUTTONDOWN:
			m_ratonX = e.button.x;
			m_ratonY = e.button.y;
			m_botonesRaton |= SDL_BUTTON(e.button.button);
			break;

		case SDL_MOUSEBUTTONUP:
			m_ratonX = e.button.x;
			m_ratonY = e.button.y;
			m_botonesRaton &= ~SDL_BUTTON(e.button.button);
			break;

		default:
			break;
		}
	}

	return resultado;
}

//------------------------------------------------------------------------------------
// Entrada
//------------------------------------------------------------------------------------
bool Ventana::TeclaPulsada(SDL_Scancode tecla) const noexcept
{
	return tecla >= 0 && tecla < SDL_NUM_SCANCODES && m_teclas[tecla];
}

bool Ventana::BotonRaton(std::uint8_t boton) const noexcept
{
	return (m_botonesRaton & SDL_BUTTON(boton)) != 0;
}

std::vector<SDL_Point> Ventana::TomarToquesNuevos()
{
	std::vector<SDL_Point> r;
	r.swap(m_toquesNuevos);
	return r;
}

void Ventana::LiberarEntrada() noexcept
{
	m_toques.clear();
	m_toquesNuevos.clear();
	m_teclas.fill(false);
	m_botonesRaton = 0;
}
