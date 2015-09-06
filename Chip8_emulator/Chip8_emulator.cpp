// Chip8_emulator.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Chip8_emulator.h"
#include <thread>
#include <Windows.h>
#include <Commdlg.h>
#include <tchar.h>
#include <stdlib.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_CHIP8_EMULATOR, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CHIP8_EMULATOR));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CHIP8_EMULATOR));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_CHIP8_EMULATOR);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	SDL_Init(SDL_INIT_EVERYTHING);
	window = SDL_CreateWindowFrom(hWnd);
	SDL_SetWindowTitle(window, "SDL Window - Set by SDL");
	SDL_SetWindowSize(window, 800, 600);
	SDL_SetWindowSize(window, 1024, 768);

	//SDL_Surface* s = SDL_GetWindowSurface(window);
	//SDL_FillRect(s, &s->clip_rect, 0xffff00ff);
	//SDL_UpdateWindowSurface(window);

	unsigned int SCREEN_WIDTH = Chip8::SCREEN_WIDTH;
	unsigned int SCREEN_HEIGHT = Chip8::SCREEN_HEIGHT;

	renderer = SDL_CreateRenderer(window, 0, SDL_WINDOW_OPENGL);
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	return TRUE;
}

std::thread emulateThread;
bool stopEmulation = false;

void UpdateChip8KeyState()
{
	const Uint8 *keyState = SDL_GetKeyboardState(NULL);

	chip8.SetKey(0x1, keyState[SDL_SCANCODE_1]);
	chip8.SetKey(0x2, keyState[SDL_SCANCODE_2]);
	chip8.SetKey(0x3, keyState[SDL_SCANCODE_3]);
	chip8.SetKey(0xC, keyState[SDL_SCANCODE_4]);
	chip8.SetKey(0x4, keyState[SDL_SCANCODE_Q]);
	chip8.SetKey(0x5, keyState[SDL_SCANCODE_W]);
	chip8.SetKey(0x6, keyState[SDL_SCANCODE_E]);
	chip8.SetKey(0xD, keyState[SDL_SCANCODE_R]);
	chip8.SetKey(0x7, keyState[SDL_SCANCODE_A]);
	chip8.SetKey(0x8, keyState[SDL_SCANCODE_S]);
	chip8.SetKey(0x9, keyState[SDL_SCANCODE_D]);
	chip8.SetKey(0xE, keyState[SDL_SCANCODE_F]);
	chip8.SetKey(0xA, keyState[SDL_SCANCODE_Z]);
	chip8.SetKey(0x0, keyState[SDL_SCANCODE_X]);
	chip8.SetKey(0xB, keyState[SDL_SCANCODE_C]);
	chip8.SetKey(0xF, keyState[SDL_SCANCODE_V]);
}

void Run()
{
	unsigned char pixels[Chip8::SCREEN_SIZE * 3];

	while (!stopEmulation)
	{
		UpdateChip8KeyState();

		chip8.EmulateCycle();

		if (chip8.DrawGraphics(pixels))
		{
			{
				std::lock_guard<std::mutex> lock(textureMutex);
				SDL_UpdateTexture(texture, NULL, &pixels, Chip8::SCREEN_WIDTH * 3 * sizeof(unsigned char));
				SDL_RenderClear(renderer);
				SDL_RenderCopy(renderer, texture, NULL, NULL);
			}
			SDL_RenderPresent(renderer);
		}
	}

	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
	stopEmulation = false;
}

char* OpenDialog()
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	TCHAR* filename = new TCHAR[MAX_PATH];

	ofn.lStructSize = sizeof(ofn);
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFile = filename;
	ofn.lpstrFile[0] = '\0';
	ofn.lpstrTitle = _T("Select ROM");
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	GetOpenFileName(&ofn);

	if (_tcslen(filename) == 0) return nullptr;

	char c_szText[MAX_PATH];
	size_t size = _tcslen(filename);
	wcstombs_s(&size, c_szText, filename, wcslen(filename) + 1);

	char* result = new char[MAX_PATH];
	memcpy(result, c_szText, size);

	return result;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		const char* file = nullptr;

		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case ID_LOAD_ROM:

			if ((file = OpenDialog()) != nullptr)
			{
				if (chip8.LoadGame(file))
				{
					emulateThread = std::thread(Run);
				}
			}
			//chip8.LoadGame("H:/projects/Chip8_emulator/Roms/Games/pong2.c8");
			//chip8.LoadGame("H:/projects/Chip8_emulator/Roms/Games/tetris.c8");
			//chip8.LoadGame("H:/projects/Chip8_emulator/Roms/Chip-8 Pack/Chip-8 Programs/Clock Program [Bill Fisher, 1981].ch8");
			break;
		case ID_STOP_EMULATION:
			stopEmulation = true;
			emulateThread.join();
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code that uses hdc here...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		stopEmulation = true;
		emulateThread.join();
		PostQuitMessage(0);
		break;
	case WM_SIZE:
	{
		//std::lock_guard<std::mutex> lock(textureMutex);
		//SDL_DestroyTexture(texture);
		//texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, Chip8::SCREEN_WIDTH, Chip8::SCREEN_HEIGHT);
		//SDL_RenderClear(renderer);
		//SDL_RenderPresent(renderer);

		/*RECT wndRect;
		GetWindowRect(hWnd, &wndRect);

		SDL_SetWindowSize(window, wndRect.right - wndRect.left + 1, wndRect.bottom - wndRect.top + 1);*/
	}
	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
