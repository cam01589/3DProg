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

// Define the screen resolution
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

// Global declarations
IDXGISwapChain *swapchain;			// The pointer to the swap chain interface
ID3D11Device *dev;					// The pointer to our Direct3D device interface
ID3D11DeviceContext *devcon;		// The pointer to our Direct3D device context
ID3D11RenderTargetView *backbuffer;	// Global declaration
ID3D11InputLayout *pLayout;			// The pointer to the input layout
ID3D11VertexShader *pVS;			// The pointer to the vertex shader
ID3D11PixelShader *pPS;				// The pointer to the pixel shader
ID3D11Buffer *pVBuffer;				// The pointer to the vertex buffer
ID3D11Buffer *pCBuffer;				// The pointer to the constant buffer
// A struct to define a single vertex
struct VERTEX 
{
	FLOAT X, Y, Z; 
	D3DXCOLOR color; 
};

struct OFFSET
{
	float X, Y, Z;
};

float Red = 0.0f, Green = 0.2f, Blue = 0.4f;

// Function prototypes
void InitD3D( HWND hWnd );			// Sets up and initializes Direct3D
void RenderFrame( void );			// Renders a single frame
void CleanD3D( void );				// Closes Direct3D and releases memory
void InitGraphics( void );			// Creates the shape to render
void InitPipeline( void );			// Loads and prepares the shaders

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
	//wc.hbrBackground = ( HBRUSH )COLOR_WINDOW; // No longer needed
	wc.lpszClassName = L"WindowClass";

	RegisterClassEx( &wc );

	RECT wr = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
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
		
		RenderFrame();
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
	scd.BufferDesc.Width = SCREEN_WIDTH;				// Set the back buffer width
	scd.BufferDesc.Height = SCREEN_HEIGHT;				// Set the back buffer height
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// How swap chain is to be used
	scd.OutputWindow = hWnd;							// The window to be used
	scd.SampleDesc.Count = 4;							// how many multisamples
	scd.Windowed = TRUE;								// Windowed/full-screen mode
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // Allow full-screen switching

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

	// Get the address of the back buffer
	ID3D11Texture2D *pBackBuffer;
	swapchain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );

	// Use the back buffer address to create the render target
	dev->CreateRenderTargetView( pBackBuffer, NULL, &backbuffer );
	pBackBuffer->Release();

	// Set the render target as the back buffer
	devcon->OMSetRenderTargets( 1, &backbuffer, NULL );

	// Set the viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory( &viewport, sizeof( D3D11_VIEWPORT ));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = SCREEN_WIDTH;
	viewport.Height = SCREEN_HEIGHT;

	devcon->RSSetViewports( 1, &viewport );

	InitPipeline();
	InitGraphics();
}

// This is the function used to render a single frame
void RenderFrame( void )
{

	// Set the constant buffer with offset data
	OFFSET Offset;
	Offset.X = 0.5f;
	Offset.Y = 0.2f;
	Offset.Z = 0.7f;

	devcon->UpdateSubresource(pCBuffer, 0, 0, &Offset, 0, 0);

	// Clear the back buffer to a deep blue
	devcon->ClearRenderTargetView( backbuffer, D3DXCOLOR( Red, Green, Blue, 1.0f ));

		// Select which vertex buffer to display
		UINT stride = sizeof( VERTEX );
		UINT offset = 0;
		devcon->IASetVertexBuffers( 0, 1, &pVBuffer, &stride, &offset );

		// Select which primitive type we are using
		devcon->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

		// Draw the vertex buffer to the back buffer
		devcon->Draw( 3, 0 );
	
	// Switch the back buffer and the front buffer
	swapchain->Present( 0, 0 );
}

// This is the function that cleans up Direct3D and COM
void CleanD3D( void )
{
	swapchain->SetFullscreenState( FALSE, NULL );	// Switch to windowed mode

	// Close and release all existing COM objects
	pLayout->Release();
	pVS->Release();
	pPS->Release();
	pVBuffer->Release();
	swapchain->Release();
	backbuffer->Release();
	dev->Release();
	devcon->Release();
}

// This is the function that creates the shape to render
void InitGraphics()
{
	// Create a triangle using the VERTEX struct
	VERTEX OurVertices[] =
	{
		{ 0.0f,  0.5f,  0.0f,	D3DXCOLOR( 1.0f, 0.0f, 0.0f, 1.0f ) },
		{ 0.45f, -0.5f, 0.0f,	D3DXCOLOR( 0.0f, 1.0f, 0.0f, 1.0f ) },
		{ -0.45f,-0.5f, 0.0f,	D3DXCOLOR( 0.0f, 0.0f, 1.0f, 1.0f ) }
	};

	// Create the vertex buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof( bd ));

	bd.Usage = D3D11_USAGE_DYNAMIC;				// Write access access by CPU and GPU
	bd.ByteWidth = sizeof( VERTEX ) * 3;		// Size is the VERTEX struct * 3
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;	// Use as a vertex buffer
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // Allow CPU to write in buffer

	dev->CreateBuffer( &bd, NULL, &pVBuffer );	// Create the buffer


	// Copy the vertices into the buffer
	D3D11_MAPPED_SUBRESOURCE ms;
	devcon->Map( pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms );	// Map teh buffer
	memcpy( ms.pData, OurVertices, sizeof( OurVertices ));				// Copy the data
	devcon->Unmap( pVBuffer, NULL );									// Unmap the buffer
}


// this function loads and prepares the shaders
void InitPipeline()
{
	// Load and compile the two shaders
	ID3D10Blob *VS, *PS;
	D3DX11CompileFromFile( L"shaders.hlsl", 0, 0, "VShader", "vs_4_1", 0, 0, 0, &VS, 0, 0 );
	D3DX11CompileFromFile( L"shaders.hlsl", 0, 0, "PShader", "ps_4_1", 0, 0, 0, &PS, 0, 0 );

	// Encapsulate both shaders into shader objects
	dev->CreateVertexShader( VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS );
	dev->CreatePixelShader( PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS );

	// Set the shader objects
	devcon->VSSetShader( pVS, 0, 0 );
	devcon->PSSetShader( pPS, 0, 0 );

	// Create the input layout object
	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
		D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	dev->CreateInputLayout( ied, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout );
	devcon->IASetInputLayout( pLayout );


	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof( bd ));

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = 16;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	dev->CreateBuffer( &bd, NULL, &pCBuffer );
	devcon->VSSetConstantBuffers( 0, 1, &pCBuffer );

}