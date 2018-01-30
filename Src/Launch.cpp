#include "App.h"
#include <string>
#include <WindowsX.h>

#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")

HWND g_wndHandle = nullptr;

bool InitWindowsApp(HINSTANCE instanceHandle, int show);
int Run();
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nShowCmd)
{
	if (!InitWindowsApp(hInstance, nShowCmd))
	{
		return 0;
	}
	
	return Run();
}

bool InitWindowsApp(HINSTANCE instanceHandle, int show)
{
	WNDCLASS desc;
	desc.style = CS_HREDRAW | CS_VREDRAW;				// Redraws the entire window if the width/height of the client area is changed
	desc.lpfnWndProc = WndProc;							// Pointer to the window procedure
	desc.cbClsExtra = 0;								// Not used
	desc.cbWndExtra = 0;								// Not used
	desc.hInstance = instanceHandle;					// Handle to the application instance
	desc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);	// Default application handle
	desc.hCursor = LoadCursor(nullptr, IDC_ARROW);		// Default arrow cursor
	desc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW); // Background color for client area of window
	desc.lpszMenuName = nullptr;						// Menu. Not used.
	desc.lpszClassName = L"D3D12SampleWindow";			// Window class name. Used to identify class by name.

	if (!RegisterClass(&desc))
	{
		MessageBox(nullptr, L"Failed to register window", nullptr, 0);
		return false;
	}

	g_wndHandle = CreateWindow(
		L"D3D12SampleWindow",							// Registered WNDCLASS instance
		L"D3D12Sample",									// Window title
		WS_OVERLAPPEDWINDOW,							// Window Style
		CW_USEDEFAULT,									// X-coordinate
		CW_USEDEFAULT,									// Y-coordinate
		k_screenWidth,									// Width
		k_screenHeight,									// Height
		nullptr,										// Parent Window. Not Used.
		nullptr,										// Menu. Not Used.
		instanceHandle,									// App instance
		nullptr											// Not Used
	);

	if (g_wndHandle == nullptr)
	{
		MessageBox(nullptr, L"Failed to create window", nullptr, 0);
		return false;
	}

	AppInstance()->Init(g_wndHandle);
	ShowWindow(g_wndHandle, show);
	UpdateWindow(g_wndHandle);

	// Hide cursor
	ShowCursor(FALSE);

	// Lock cursor to screen
	RECT screenRect;
	GetWindowRect(g_wndHandle, &screenRect);
	ClipCursor(&screenRect);

	// Set cursor to window center
	int windowCenterX = screenRect.left + (screenRect.right - screenRect.left) / 2;
	int windowCenterY = screenRect.top + (screenRect.bottom - screenRect.top) / 2;
	SetCursorPos(windowCenterX, windowCenterY);

	return true;
}

int Run()
{
	MSG msg = { nullptr };

	while (msg.message != WM_QUIT)
	{

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			AppInstance()->Update(0.0166f);
			AppInstance()->Render();
		}
	}

	return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			AppInstance()->Destroy();
			DestroyWindow(g_wndHandle);
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_MOUSEMOVE:
		AppInstance()->OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}


