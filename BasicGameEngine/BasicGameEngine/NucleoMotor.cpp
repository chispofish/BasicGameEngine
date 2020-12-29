//------------------------------------------------------------------------------------------
// Objto NucleoMotor
// Codigo fuente C++ - NucleoMotor.cpp
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
// Archivos de inclusion
//------------------------------------------------------------------------------------------
#include "NucleoMotor.h"

//------------------------------------------------------------------------------------------
// Inicializacion de variables estaticas
//------------------------------------------------------------------------------------------
NucleoMotor* NucleoMotor::m_pNucleoMotor = NULL;

//------------------------------------------------------------------------------------------
// Funciones Windows
//------------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstancia, HINSTANCE hInstanciaPrev, PSTR szCmdLine, int iMostrar)
{
	MSG msg;
	static int iSiguienteCiclo = 0;
	int iTicsPasados;

	if (FJ_InicializarJuego(hInstancia))
	{
		// Inicializa el nucleo motor
		// (crea ventana y la muestra en pantalla)
		if (!NucleoMotor::ObtenerNucleoMotor()->Inicializar(iMostrar))
			return FALSE;
		// Bucle principal de mensajes
		while (TRUE)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				// Proceso del mensaje
				// El mensaje WM_QUIT hace salir del bucle
				// La funcion PeekMessage entrega los mensajes que hay en la cola
				if (msg.message == WM_QUIT)
					break;

				// Traduce los mensajes de teclas virtuales
				TranslateMessage(&msg);

				// Envia el mensaje al procedimiento de ventana
				// donde seran tratados por el metodo TratarMensajes()
				DispatchMessage(&msg);
			}

			// Cuando hay mensajes pendientes
			// verifica si ya se debe ejecutar un ciclo
			else
			{
				// Se verifica que el juego no este en pausa
				if (!NucleoMotor::ObtenerNucleoMotor()->ObtenerEnEspera())
				{
					// Se verifica la cuenta de tics para ver si
					// ha pasado el cliclo del juego
					iTicsPasados = GetTickCount();

					if (iTicsPasados > iSiguienteCiclo)
					{
						// Antes de ir al ciclo del juego
						// actualiza el tiempo del ciclo siguiente
						iSiguienteCiclo = iTicsPasados + 1000 / NucleoMotor::ObtenerNucleoMotor()->ObtenerFrecuencia();

						//Ejecucion del ciclo del juego
						FJ_CicloJuego();
					}
				}
			}
		}

		return (int)msg.wParam;
	}

	// Final del juego
	FJ_FinalizarJuego();

	return TRUE;
}

LRESULT CALLBACK WindowProc(HWND hVentana, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//Procedimiento de ventana
	// Envia los mensajes a la funcion de tratamiento de los eventos
	return NucleoMotor::ObtenerNucleoMotor()->TratarMensajes(hVentana, msg, wParam, lParam);
}

//-------------------------------------------------------------------------------------------------------------
// NucleoMotor Constructor(s)/Destructor
//-------------------------------------------------------------------------------------------------------------
NucleoMotor::NucleoMotor(HINSTANCE hInstancia, LPTSTR szClaseVentana, LPTSTR szTitulo, WORD wIcono, WORD wIconoSm, int iAncho, int iAltura, int iFrecuencia)
{
	// Asigna las varialbes miembro del objeto NucleoMotor
	m_pNucleoMotor = this;
	m_hInstancia = hInstancia;

	// Queda pendiente asignar el handle de la ventana
	m_hVentana = NULL;

	// Copia la cadena cuando hay informacion
	if (lstrlen(szClaseVentana) > 0)
		lstrcpy(m_szClaseVentana, szClaseVentana);

	// Copia la cadena cuando hay informacion
	if (lstrlen(szTitulo) > 0)
		lstrcpy(m_szTitulo, szTitulo);

	// Identificadores del recurso Icono
	m_wIcono = wIcono;
	m_wIconoSm = wIconoSm;

	// Dimensiones de la ventana
	m_iAncho = iAncho;
	m_iAltura = iAltura;

	// La frecuencia del ciclo es en fps
	m_iFrecuencia = iFrecuencia;

	// El juego comienza en pausa
	m_bEnEspera = TRUE;
}

NucleoMotor::~NucleoMotor()
{
	//De momento no hace nada en particular
}

//-------------------------------------------------------------------------------------------
// Metodos del objeto NucleoMotor
//-------------------------------------------------------------------------------------------
BOOL NucleoMotor::Inicializar(int iMostrar)
{
	WNDCLASSEX wndclass;

	// Crea la clase ventana para la ventana principal
	wndclass.cbSize = sizeof(wndclass);
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WindowProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = m_hInstancia;
	wndclass.hIcon = LoadIcon(m_hInstancia, MAKEINTRESOURCE(ObtenerIcono()));
	wndclass.hIconSm = LoadIcon(m_hInstancia, MAKEINTRESOURCE(ObtenerIconoSm()));
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = m_szClaseVentana;

	// Registra la clase de ventanas
	if (!RegisterClassEx(&wndclass))
		return FALSE;

	// Calcula el tamaño de la ventana
	int iAncho = m_iAncho + GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
	int iAltura = m_iAltura + GetSystemMetrics(SM_CXFIXEDFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);

	// Verifica si hay titulo de ventana
	// en ese caso corrige la altura
	if (wndclass.lpszMenuName != NULL)
		iAltura += GetSystemMetrics(SM_CYMENU);

	// Posicionamiento de la ventana en el centro de la pantalla
	int iPosX = (GetSystemMetrics(SM_CXSCREEN) - iAncho) / 2;
	int iPosY = (GetSystemMetrics(SM_CYSCREEN) - iAltura) / 2;

	// Crea la ventana
	// No se puede redimensionar pero se puede minimizar y tiene barra de titulo
	m_hVentana = CreateWindow(m_szClaseVentana, m_szTitulo, WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX, iPosX, iPosY, iAncho, iAltura, NULL, NULL, m_hInstancia, NULL);

	// Si no puede crear la ventana devuelve false
	if (!m_hVentana)
		return FALSE;

	// Mostrar y actualizar ventana
	ShowWindow(m_hVentana, iMostrar);
	UpdateWindow(m_hVentana);

	return TRUE;
}

LRESULT NucleoMotor::TratarMensajes(HWND hVentana, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Tratamiento de los mensajes
	switch (msg)
	{
		//-----------------------------------------
	case WM_CREATE:
		// Asigna la ventana y comienza el juego
		AsignarVentana(hVentana);
		FJ_ComenzarJuego(hVentana);
		return 0;

		//-----------------------------------------
	case WM_KILLFOCUS:
		// Desactiva el juego y actualiza el estado de pausa
		FJ_DetenerJuego(hVentana);
		AsignarEnEspera(TRUE);
		return 0;

		//-----------------------------------------
	case WM_PAINT:
		HDC hDC;
		PAINTSTRUCT ps;
		hDC = BeginPaint(hVentana, &ps);

		// Dibuja el juego
		FJ_PintarJuego(hDC);

		EndPaint(hVentana, &ps);
		return 0;

		//-----------------------------------------
	case WM_SETFOCUS:
		// Activa el juego y actualiza el estado en pausa
		FJ_ReactivarJuego(hVentana);
		AsignarEnEspera(FALSE);
		return 0;

		//-----------------------------------------
	case WM_DESTROY:
		// Final del juego y sale de la aplicacion
		FJ_FinalizarJuego();
		PostQuitMessage(0);
		return 0;

	}

	// Tratamiento del resto de los mensajes
	// no gestionados por NucleoMotor
	return DefWindowProc(hVentana, msg, wParam, lParam);
}