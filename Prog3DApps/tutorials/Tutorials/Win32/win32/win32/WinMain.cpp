#include <Windows.h>	// Include the basic windows header file

// The entry point for any Windows program
int WINAPI WinMain( HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPSTR lpCmdLine,
					int nShowCmd )
{
	// Create a "Hello World" message box using MessageBox()
	MessageBox( NULL,
				L"Hello World!",
				L"Just another Hello World program!",
				MB_ICONEXCLAMATION | MB_OK );

	// Return 0 to Windows
	return 0;
}