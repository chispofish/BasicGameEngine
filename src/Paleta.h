//------------------------------------------------------------------------------------
// Paleta  (Juego Runner - Modulo 4: identidad visual)
// Encabezado C++ - Paleta.h
//------------------------------------------------------------------------------------
// Todos los colores del mundo salen de los del avatar (muestreados de la hoja de sprites;
// el Test 4.x comprueba que siguen estando cerca de ellos): violeta de la gorra y la
// camiseta, verde del mono, marron de los zapatos y tono de piel. El mundo usa versiones
// oscuras/desaturadas de esos tonos para que el personaje destaque, y los peligros usan
// el naranja calido (vecino del marron y el dorado) para contrastar con el violeta/verde.
//------------------------------------------------------------------------------------
#pragma once

#include "Dibujante.h"

namespace Paleta
{
	// --- Colores del avatar (referencia) ---
	inline constexpr Color Violeta { 164,   0, 255, 255 };
	inline constexpr Color Verde   {  20, 253,   2, 255 };
	inline constexpr Color Marron  { 126,  60,  27, 255 };
	inline constexpr Color Piel    { 238, 168, 123, 255 };
	inline constexpr Color Dorado  { 236, 190,  48, 255 };
	inline constexpr Color Contorno{  17,   9,   6, 255 };   // el negro calido de su contorno

	// --- Mundo ---
	inline constexpr Color CieloArriba   {  14,   6,  38, 255 };   // violeta casi negro
	inline constexpr Color CieloHorizonte{  78,  22, 112, 255 };   // violeta calido de atardecer
	inline constexpr Color Estrella      { 236, 222, 255, 255 };
	inline constexpr Color Luna          { 255, 226, 190, 255 };   // piel muy clara
	inline constexpr Color LunaCrater    { 232, 196, 168, 255 };
	inline constexpr Color ColinaLejana  {  52,  18,  92, 255 };
	inline constexpr Color ColinaCercana {  34,  12,  66, 255 };
	inline constexpr Color Suelo         {  64,  30,  60, 255 };   // marron + violeta
	inline constexpr Color SueloOscuro   {  44,  18,  46, 255 };
	inline constexpr Color Hierba        {  36, 176,  22, 255 };   // verde del mono, apagado
	inline constexpr Color HierbaClara   {  96, 230,  40, 255 };
	inline constexpr Color SombraSuelo   {  20,  96,  14, 255 };

	// --- Peligro: los "bugs" ---
	inline constexpr Color BugCuerpo { 252, 120,  24, 255 };
	inline constexpr Color BugLuz    { 255, 186,  80, 255 };
	inline constexpr Color BugSombra { 176,  66,  18, 255 };

	// --- Interfaz y efectos ---
	inline constexpr Color TextoClaro { 236, 222, 255, 255 };
	inline constexpr Color TextoSombra{  14,   6,  38, 255 };
	inline constexpr Color Polvo      { 186, 152, 204, 255 };
}
