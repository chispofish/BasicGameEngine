//------------------------------------------------------------------------------------
// Escenario  (Juego Runner - Modulo 4: fondo con parallax)
// Encabezado C++ - Escenario.h
//------------------------------------------------------------------------------------
// Cielo en degradado por bandas, estrellas, luna, dos capas de colinas "escalonadas"
// (pixel art) que se desplazan a distinta velocidad, y suelo con hierba. Todo se dibuja
// con rectangulos del Dibujante, sin texturas. Es determinista (semilla) y avanza con
// los tics de logica, no con el reloj, asi que se congela solo cuando el juego se para.
//------------------------------------------------------------------------------------
#pragma once

#include "Dibujante.h"

#include <cstdint>
#include <vector>

class Escenario
{
public:
	Escenario(int ancho, int alto, double ySuelo, std::uint32_t semilla = 7);

	// 'velocidad' en px/s del suelo (0 = escenario congelado)
	void Actualizar(double dt, double velocidad) noexcept;
	void Pintar(const Dibujante& d) const;

	double DesplazamientoSuelo() const noexcept { return m_suelo; }
	double DesplazamientoCerca() const noexcept { return m_cerca; }
	double DesplazamientoLejos() const noexcept { return m_lejos; }

private:
	struct Estrella { int x, y, fase; };
	struct Mota     { int x, y, w; };

	void PintarCielo(const Dibujante& d) const;
	void PintarColinas(const Dibujante& d, const std::vector<int>& alturas, double desplazamiento, Color c) const;
	void PintarSuelo(const Dibujante& d) const;

	int    m_ancho, m_alto;
	double m_ySuelo;
	std::vector<Estrella> m_estrellas;
	std::vector<int> m_colinasLejos, m_colinasCerca;   // altura por columna, periodicas
	std::vector<Mota> m_motas;                         // detalles del suelo
	double m_suelo = 0, m_cerca = 0, m_lejos = 0;
	std::uint64_t m_tics = 0;

public:
	static constexpr int kAnchoColumna = 16;
	static constexpr int kPeriodoSuelo = 640;
};
