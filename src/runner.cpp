//------------------------------------------------------------------------------------
// runner.cpp - Juego Runner completo (nativo y Emscripten)
//   ESPACIO / FLECHA ARRIBA o TOCAR LA PANTALLA: saltar (y reiniciar tras GAME OVER).
//   M o el boton tactil: sonido si/no.
//   Integra Textura (M1), Jugador (M2), Obstaculos/Partida/Hud (M3) y, en el Modulo 4,
//   Escenario (parallax), Efectos (particulas, sacudida) y Sonido (sintetizado).
//   La logica (Partida) solo emite EVENTOS; aqui se traducen a sonido y efectos.
//------------------------------------------------------------------------------------
#include "ControlTactil.h"
#include "Efectos.h"
#include "Escenario.h"
#include "Hud.h"
#include "NucleoMotor.h"
#include "Paleta.h"
#include "Sonido.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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

		m_sonido = Sonido::Abrir();   // nullptr = sin audio: el juego sigue mudo
		if (!m_sonido) std::cerr << "Aviso: sin dispositivo de audio, el juego sonara mudo" << std::endl;

		ConfigPartida cfg;
		cfg.ancho  = m_ventana->Ancho();
		cfg.ySuelo = m_ventana->Altura() - 80.0;
		m_partida   = std::make_unique<Partida>(cfg, SDL_GetTicks());   // semilla distinta por sesion
		m_escenario = std::make_unique<Escenario>(cfg.ancho, m_ventana->Altura(), cfg.ySuelo);
		m_efectos   = std::make_unique<Efectos>(cfg.ySuelo, SDL_GetTicks());
		m_tactil    = std::make_unique<ControlTactil>(Hud::RectBotonSonido(cfg.ancho, m_ventana->Altura()));
#ifdef __EMSCRIPTEN__
		// Movil/tablet: se muestran desde el primer frame los textos y el boton tactiles
		if (emscripten_run_script_int("(('ontouchstart' in window) || (navigator.maxTouchPoints > 0)) ? 1 : 0"))
			m_ventana->MarcarEntradaTactil(true);
#endif
		return true;
	}

	void Finalizar() override
	{
		m_sonido.reset();   // cerrar el audio antes de que el motor cierre SDL
		m_hoja.reset();
	}

	void Actualizar(double dt) override
	{
		const Ventana& v = *m_ventana;
		const bool saltar = v.TeclaPulsada(SDL_SCANCODE_SPACE) || v.TeclaPulsada(SDL_SCANCODE_UP) ||
		                    v.TeclaPulsada(SDL_SCANCODE_W);
		const bool tecla_m = v.TeclaPulsada(SDL_SCANCODE_M);
		if (tecla_m && !m_mPrevia && m_sonido) m_sonido->AlternarSilencio();
		m_mPrevia = tecla_m;

		// Pantalla tactil: tocar salta/reinicia; el boton de sonido alterna el audio
		const SalidaTactil t = m_tactil->Actualizar(m_ventana->ToquesActivos(), m_ventana->TomarToquesNuevos());
		if (t.alternarSonido && m_sonido) m_sonido->AlternarSilencio();

		m_partida->Actualizar(dt, saltar || t.saltar);
		Reaccionar(m_partida->TomarEventos());

		const bool jugando = m_partida->Estado() == EstadoPartida::Jugando;
		if (jugando && m_partida->ElJugador().EnElSuelo() && ++m_ticsPolvo % 7 == 0)
			m_efectos->Polvo(PiesX() - 14, m_partida->Config().ySuelo, 1);       // polvo al correr
		m_escenario->Actualizar(dt, jugando ? m_partida->Velocidad() : 0.0);
		m_efectos->Actualizar(dt);
		m_resaltado = m_resaltado > dt ? m_resaltado - dt : 0.0;
	}

	void Pintar(SDL_Renderer* r) override
	{
		const Dibujante d(r);          // el motor ya limpio el fondo
		const int ancho = m_ventana->Ancho(), alto = m_ventana->Altura();
		const double suelo = m_partida->Config().ySuelo;

		// El mundo se desplaza con la sacudida; la interfaz no
		const SDL_Point s = m_efectos->Sacudida();
		const SDL_Rect vista{ s.x, s.y, ancho, alto };
		SDL_RenderSetViewport(r, &vista);

		m_escenario->Pintar(d);
		PintarSombra(d, suelo);
		m_partida->Obstaculos().Pintar(d);
		m_partida->ElJugador().Pintar(r, m_hoja.get());
		m_efectos->Pintar(d);

		SDL_RenderSetViewport(r, nullptr);       // IMPORTANTE: restaurar para el siguiente frame
		m_efectos->PintarDestello(d, ancho, alto);
		Hud::Pintar(d, *m_partida, ancho, alto,
		            Hud::Opciones{ m_sonido && !m_sonido->Silenciado(), m_resaltado > 0.0, m_ventana->UltimaEntradaTactil() });
	}

private:
	double PiesX() const { const SDL_Rect r = m_partida->ElJugador().Rectangulo(); return r.x + r.w / 2.0; }

	// Sombra en el suelo: se estrecha al subir el jugador
	void PintarSombra(const Dibujante& d, double suelo) const
	{
		const double altura = suelo - m_partida->ElJugador().YPies();
		const int w = std::max(16, static_cast<int>(46 - altura * 0.18));
		d.RectanguloRelleno(SDL_Rect{ static_cast<int>(PiesX()) - w / 2, static_cast<int>(suelo) + 1, w, 5 }, Paleta::SombraSuelo);
	}

	void Reaccionar(const EventosPartida& ev)
	{
		const double suelo = m_partida->Config().ySuelo;
		auto sonar = [&](Efecto e) { if (m_sonido) m_sonido->Reproducir(e); };
		if (ev.reinicio)   { m_tactil->Cancelar(); sonar(Efecto::Inicio); if (m_sonido) m_sonido->PausarMusica(false); m_efectos->Reiniciar(); }
		if (ev.salto)      { sonar(Efecto::Salto); m_efectos->Polvo(PiesX(), suelo, 5); }
		if (ev.aterrizaje) { sonar(Efecto::Aterrizaje); m_efectos->Polvo(PiesX(), suelo, 9); }
		if (ev.hitos > 0)  { sonar(Efecto::Hito); m_efectos->Chispas(60, 24); m_resaltado = 0.6; }
		if (ev.choque)
		{
			sonar(Efecto::Golpe);
			if (m_sonido) m_sonido->PausarMusica(true);
			const SDL_Rect c = m_partida->ElJugador().CajaColision();
			m_efectos->Explosion(c.x + c.w / 2.0, c.y + c.h / 2.0);
			m_efectos->Sacudir(9, 0.45);
			m_efectos->Destello(0.3);
		}
	}

	Ventana*                  m_ventana = nullptr;
	std::unique_ptr<Textura>  m_hoja;
	std::unique_ptr<Sonido>   m_sonido;
	std::unique_ptr<Partida>  m_partida;
	std::unique_ptr<Escenario> m_escenario;
	std::unique_ptr<Efectos>  m_efectos;
	std::unique_ptr<ControlTactil> m_tactil;
	int    m_ticsPolvo = 0;
	bool   m_mPrevia = false;
	double m_resaltado = 0.0;
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
		config.colorFondo = Paleta::CieloArriba;
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
