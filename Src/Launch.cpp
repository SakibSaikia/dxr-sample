#include "App.h"
#include <string>

// Link necessary libraries.
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

// Main window handle for the application. HWND is of type void* and refers to a handle to a window.
HWND ghMainWnd = nullptr;

// Initializes the Windows application. HINSTANCE is of type void* and refers to a handle to an instance.
bool InitWindowsApp(HINSTANCE instanceHandle, int show);

// Wraps the message loop code
int Run();

// Windows procedure to handle events the window receives. CALLBACK is equivalent to __stdcall (it is used to distinguish it as a callback function)
// LRESULT is of type __int64.
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main entry point. WINAPI is equivalent __stdcall (callee cleans the stack leading to slightly smaller program sizes)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nShowCmd)
{
	// Initialize the application window
	if (!InitWindowsApp(hInstance, nShowCmd))
		return 0;
	
	// Enter the message loop, which runs indefinitely until a WM_QUIT message is received.
	return Run();
}

bool InitWindowsApp(HINSTANCE instanceHandle, int show)
{
	// Describe the window characteristics
	WNDCLASS desc;
	desc.style = CS_HREDRAW | CS_VREDRAW;				// Redraws the entire window if the width/height of the client area is changed
	desc.lpfnWndProc = WndProc;							// Pointer to the window procedure
	desc.cbClsExtra = 0;								// Not used
	desc.cbWndExtra = 0;								// Not used
	desc.hInstance = instanceHandle;					// Handle to the application instance
	desc.hIcon = LoadIcon(0, IDI_APPLICATION);			// Default application handle
	desc.hCursor = LoadCursor(0, IDC_ARROW);			// Default arrow cursor
	desc.hbrBackground = (HBRUSH)COLOR_WINDOW;			// Background color for client area of window
	desc.lpszMenuName = nullptr;						// Menu. Not used.
	desc.lpszClassName = L"D3D12SampleWindow";			// Window class name. Used to identify class by name.

	// Register the above WNDCLASS instance with Windows
	if (!RegisterClass(&desc))
	{
		MessageBox(0, L"Failed to register window", 0, 0);
		return false;
	}

	// Create the application window
	ghMainWnd = CreateWindow(
		L"D3D12SampleWindow",							// Registered WNDCLASS instance
		L"D3D12Sample",									// Window title
		WS_OVERLAPPEDWINDOW,							// Window Style
		CW_USEDEFAULT,									// X-coordinate
		CW_USEDEFAULT,									// Y-coordinate
		CW_USEDEFAULT,									// Width
		CW_USEDEFAULT,									// Height
		nullptr,										// Parent Window. Not Used.
		nullptr,										// Menu. Not Used.
		instanceHandle,									// App instance
		nullptr											// Not Used
	);

	if (ghMainWnd == nullptr)
	{
		MessageBox(0, L"Failed to create window", 0, 0);
		return false;
	}
	

	// Initialize D3D
	App d3dApp;
	d3dApp.Init();

	// Window is not shown until we call ShowWindow()
	ShowWindow(ghMainWnd, show);

	// Sends a WM_PAINT message directly to the window procedure of the specified window
	UpdateWindow(ghMainWnd);

	return true;
}

int Run()
{
	MSG msg = { 0 };

	// GetMessage returns 0 when a WM_QUIT message is received
	while (msg.message != WM_QUIT)
	{

		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// Run game code
		}
	}

	return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		MessageBox(0, L"Hello World", L"Hello", MB_OK);
		return 0;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			DestroyWindow(ghMainWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}


