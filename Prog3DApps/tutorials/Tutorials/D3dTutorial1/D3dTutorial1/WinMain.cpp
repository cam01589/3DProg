// Include the basic header files
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>

// Include the Direct3D Library file
#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "d3dx11.lib" )
#pragma comment( lib, "d3dx10.lib" )

// Global declarations
IDXGISwapChain *swapchain;		// The pointer to the swap chain interface
ID3D11Device *dev;				// The pointer to our Direct3D device interface
ID3D11DeviceContext *devcon;	// The pointer to our Direct3D device context

// Function prototypes
void InitD3D( HWND hWnd );		// Sets up and initializes Direct3D
void CleanD3D( void );			// Closes Direct3D and releases memory

// The WindowProc function prototype
LRESULT CALLBACK WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

// The entry point for any Windows program
int WINAPI WinMain( HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPSTR lpCmdLine,
					int nCmdShow )
{
	HWND hWnd;
	WNDCLASSEX wc;

	ZeroMemory( &wc, sizeof( WNDCLASSEX ));

	wc.cbSize = sizeof( WNDCLASSEX );
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = ( HBRUSH )COLOR_WINDOW;
	wc.lpszClassName = L"WindowClass";

	RegisterClassEx( &wc );

	RECT wr = { 0, 0, 800, 600 };
	AdjustWindowRect( &wr, WS_OVERLAPPEDWINDOW, FALSE );

	hWnd = CreateWindowEx( NULL,
						   L"WindowClass",
						   L"Our First Direct3D Program",
						   WS_OVERLAPPEDWINDOW,
						   300,
						   300,
						   wr.right - wr.left,
						   wr.bottom - wr.top,
						   NULL,
						   NULL,
						   hInstance,
						   NULL );

	ShowWindow( hWnd, nCmdShow );

	// Set up and initialize Direct3D
	InitD3D( hWnd );

	// Enter the main loop:

	MSG msg;

	while( TRUE )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ))
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );

			if( msg.message == WM_QUIT )
				break;
		}
		else
		{
			// Run game code here
			//...
			//...
		}
	}

	// Clean up DirectX and COM
	CleanD3D();

	return msg.wParam;
}

// This is the main message handler for the program
LRESULT CALLBACK WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message )
	{
	case WM_DESTROY:
		{
			PostQuitMessage( 0 );
			return 0;
		}
			break;
	}

	return DefWindowProc( hWnd, message, wParam, lParam );
}

// This function initializes and prepares Direct3D for use
void InitD3D( HWND hWnd )
{
	// Create a struct to hold information about the swap chain
	DXGI_SWAP_CHAIN_DESC scd;

	// Clear out the struct for use
	ZeroMemory( &scd, sizeof( DXGI_SWAP_CHAIN_DESC ));

	// Fill the swap chain description struct
	scd.BufferCount = 1;			// One back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// One back buffer
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// How swap chain is to be used
	scd.OutputWindow = hWnd;							// The window to be used
	scd.SampleDesc.Count = 4;							// how many multisamples
	scd.Windowed = TRUE;								// Windowed/full-screen mode
	
	// Create a device, device context and swap chain using the information in the scd struct
	D3D11CreateDeviceAndSwapChain( NULL,
								   D3D_DRIVER_TYPE_HARDWARE,
								   NULL,
								   NULL,
								   NULL,
								   NULL,
								   D3D11_SDK_VERSION,
								   &scd,
								   &swapchain,
								   &dev,
								   NULL,
								   &devcon );
}

// This is the function that cleans up Direct3D and COM
void CleanD3D( void )
{
	// Close and release all existing COM objects
	swapchain->Release();
	dev->Release();
	devcon->Release();
}
