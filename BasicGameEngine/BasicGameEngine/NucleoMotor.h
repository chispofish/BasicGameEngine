//------------------------------------------------------------------------------------
// NucleoMotor
// Encabezado C++ - NucleoMotor.h
//------------------------------------------------------------------------------------
// Para evitar la doble inclusion de archivos
#pragma once

//------------------------------------------------------------------------------------
// Inclusion de archivos
//------------------------------------------------------------------------------------
#include <Windows.h>
#include <time.h>

//------------------------------------------------------------------------------------
// Declaraciones de funciones
//------------------------------------------------------------------------------------
// punto de entrada de los programas Windows
int WINAPI WinMain(HINSTANCE HInstancia, HINSTANCE hInstanciaPrev, PSTR szCmdLine, int iMostrar);

// procedimiento de ventana
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

//------------------------------------------------------------------------------------
// Funciones auxiliares del nuecleo motor
//------------------------------------------------------------------------------------
BOOL FJ_InicializarJuego(HINSTANCE hInstancia);
void FJ_ComenzarJuego(HWND hVentana);
void FJ_CicloJuego();
void FJ_ReactivarJuego(HWND hVentana);
void FJ_DetenerJuego(HWND hVentana);
void FJ_PintarJuego(HDC hDC);
void FJ_FinalizarJuego();

//------------------------------------------------------------------------------------
// Clase NucleoMotor
//------------------------------------------------------------------------------------
class NucleoMotor
{
protected:
	//-----------------------------------------------------
	// miembros protegidos de la clase
	//-----------------------------------------------------
	static NucleoMotor* m_pNucleoMotor;
	HINSTANCE	m_hInstancia;				// handle de la instancia
	HWND		m_hVentana;					// handle de ventana
	TCHAR		m_szClaseVentana[32];		// Clase de la ventana
	TCHAR		m_szTitulo[12];				// Titulo
	int			m_iAncho;					// Ancho de ventana
	int			m_iAltura;					// Altura de ventana
	WORD		m_wIcono;					// Icono normal
	WORD		m_wIconoSm;					// Icono pequeño
	int			m_iFrecuencia;				// Frecuencia del ciclo (fps)
	BOOL		m_bEnEspera;				// Indicador de estado

public:
	//-----------------------------------------------------
	// miembros publicos de la clase
	//-----------------------------------------------------
	// Constructor y Destructor
	//-----------------------------------------------------
	NucleoMotor(HINSTANCE hInstancia, LPTSTR szClaseVentana, LPTSTR szTitulo, WORD wIcono, WORD wIconoSm, int iAncho = 640, int iAltura = 480, int iFrecuencia = 50);

	virtual ~NucleoMotor();					// Destructor de la clase

	//---------------------------------------------------------------------------------------------------------
	// Metodos auxiliares
	//---------------------------------------------------------------------------------------------------------
	// Crea la clase ventan y despues crea la ventana
	BOOL Inicializar(int iMostrar);

	// Tratamiento de los mensakes
	LRESULT TratarMensajes(HWND hVentana, UINT msg, WPARAM wParam, LPARAM lParam);

	//---------------------------------------------------------------------------------------------------------
	// Metodos de acceso a los miembros protegidos
	//---------------------------------------------------------------------------------------------------------
	// Devuelve el puntero del objeto
	static NucleoMotor* ObtenerNucleoMotor()
	{
		return m_pNucleoMotor;
	};

	// Obtencion o asignacion de los miembros protegidos
	HINSTANCE ObtenerInstancia()
	{
		return m_hInstancia;
	};

	//Handle de la ventanan
	HWND ObtenerVentana()
	{
		return m_hVentana;
	};
	void AsignarVentana(HWND hVentana)
	{
		m_hVentana = hVentana;
	};

	// Titulo de la ventana
	LPTSTR ObtenerTitulo()
	{
		return m_szTitulo;
	};

	// Iconos del programa
	WORD ObtenerIcono()
	{
		return m_wIcono;
	};
	WORD ObtenerIconoSm()
	{
		return m_wIconoSm;
	};

	// Tamaño de la ventana
	int ObtenerAncho()
	{
		return m_iAncho;
	};
	int ObtenerAltura()
	{
		return m_iAltura;
	};

	// Frecuencia en frames por segundo (fps)
	int ObtenerFrecuencia()
	{
		return m_iFrecuencia;
	};
	void AsignarFrecuencia(int iFrecuencia)
	{
		m_iFrecuencia = iFrecuencia;
	};

	// Indicador de estado
	BOOL ObtenerEnEspera()
	{
		return m_bEnEspera;
	};
	void AsignarEnEspera(BOOL bEnEspera)
	{
		m_bEnEspera = bEnEspera;
	};
};