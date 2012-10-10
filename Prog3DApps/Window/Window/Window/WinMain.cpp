// Include the basic windows header file
#include <Windows.h>
#include <WindowsX.h>

// The WindowProc function prototype
LRESULT CALLBACK WindowProc( HWND hWnd,
							 UINT message,
							 WPARAM wParam,
							 LPARAM lParam );

// The entry point for any Windows program
int WINAPI WinMain( HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPSTR lpCmdLine,
					int nCmdShow )
{
	// The handle for the window, filled by a function
	HWND hWnd;
	// This struct holds information for the window class
	WNDCLASSEX wc;

	// Clear out the window class for use
	ZeroMemory( &wc, sizeof( WNDCLASSEX ));

	// Fill in the struct with the needed information
	wc.cbSize = sizeof( WNDCLASSEX );
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = ( HBRUSH )COLOR_WINDOW;
	wc.lpszClassName = L"WindowClass1";

	// Register the window class
	RegisterClassEx( &wc );

	// Create the window and use the result as the handle
	hWnd = CreateWindowEx( NULL,
						   L"WindowClass1",	// Name of the window class
						   L"Our First Windowed Program", // Title of the window
						   WS_OVERLAPPEDWINDOW,				// Window style
						   300,			// x pos of window
						   300,			// y pos of window
						   500,			// Width of window
						   400,			// Height of window
						   NULL,		// We have no parent window, NULL
						   NULL,		// We are not currently using menus, NULL
						   hInstance,	// Application handle
						   NULL );		// Used with multiple windows, NULL

	// Display the window on the screen
	ShowWindow( hWnd, nCmdShow );

	// Enter the main loop

	// This struct holds Windows event messages
	MSG msg;

	// Wait for the next message in the queue, store the result in 'msg'
	while( GetMessage( &msg, NULL, 0, 0 ))
	{
		// Translate keystroke messages into the right format
		TranslateMessage( &msg );

		// Send the message to the WindowProc function
		DispatchMessage( &msg );
	}

	// Return this part of the WM_QUIT message to Windows
	return msg.wParam;
}

// This is the main message handler for the program
LRESULT CALLBACK WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	// Sort through and find the code to run for the message given
	switch( message )
	{
		// This message is read when the window is closed
	case WM_DESTROY:
		{
			// Close the application entirely
			PostQuitMessage( 0 );
			return 0;
		} 
			break;
	}

	// Handle any messages the switch statement didn't 
	return DefWindowProc( hWnd, message, wParam, lParam );
}

