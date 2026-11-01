//------------------------------------------------------------------------------------
// ControlTactil.cpp
//------------------------------------------------------------------------------------
#include "ControlTactil.h"

SalidaTactil ControlTactil::Actualizar(const std::vector<Toque>& activos, const std::vector<SDL_Point>& nuevos) noexcept
{
	SalidaTactil s;
	for (const SDL_Point& p : nuevos)
	{
		if (EnBoton(p.x, p.y)) s.alternarSonido = true;
		else m_buffer = m_ticsBuffer;                 // toque de juego: salto "pendiente"
	}

	bool apoyado = false;                             // dedo mantenido fuera del boton
	for (const Toque& t : activos) if (!EnBoton(t.x, t.y)) { apoyado = true; break; }

	s.saltar = apoyado || m_buffer > 0;
	if (m_buffer > 0) --m_buffer;
	return s;
}
