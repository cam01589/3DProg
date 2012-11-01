// include the basic windows header files and the Direct3D header files
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>

// include the Direct3D Library file
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dx10.lib")

// define the screen resolution
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

// global declarations
IDXGISwapChain							*swapchain;     // the pointer to the swap chain interface
ID3D11Device							*dev;           // the pointer to our Direct3D device interface
ID3D11DeviceContext						*devcon;        // the pointer to our Direct3D device context
ID3D11RenderTargetView					*backbuffer;    // the pointer to our back buffer
ID3D11DepthStencilView					*zbuffer;       // the pointer to our depth buffer
ID3D11InputLayout						*pLayout;       // the pointer to the input layout
ID3D11VertexShader						*pVS;           // the pointer to the vertex shader
ID3D11PixelShader						*pPS;           // the pointer to the pixel shader
ID3D11Buffer							*pVBuffer;      // the pointer to the vertex buffer
ID3D11Buffer							*pCBuffer;      // the pointer to the constant buffer
ID3D11Buffer							*pIBuffer;      // the pointer to the index buffer

bool Rotate = FALSE; // Used for rotation switching between left spin and right
bool Travel = TRUE; // Used for movement of pyramid 

// various buffer structs
struct VERTEX{FLOAT X, Y, Z; D3DXCOLOR Color;};
struct PERFRAME{D3DXCOLOR Color; FLOAT X, Y, Z;};

// function prototypes
void InitD3D(HWND hWnd);    // sets up and initializes Direct3D
void RenderFrame(void);     // renders a single frame
void CleanD3D(void);        // closes Direct3D and releases memory
void InitGraphics(void);    // creates the shape to render
void InitPipeline(void);    // loads and prepares the shaders

float a = 0.000f; // Again this is used for movement of pyramid



// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    HWND hWnd;
    WNDCLASSEX wc;

    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"WindowClass";

    RegisterClassEx(&wc);

    RECT wr = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    hWnd = CreateWindowEx(NULL,
                          L"WindowClass",
                          L"Dan's Direct3D Program",
                          WS_OVERLAPPEDWINDOW,
                          300,
                          300,
                          wr.right - wr.left,
                          wr.bottom - wr.top,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    ShowWindow(hWnd, nCmdShow);

    // set up and initialize Direct3D
    InitD3D(hWnd);

    // enter the main loop:

    MSG msg;

    while(TRUE)
    {
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if(msg.message == WM_QUIT)
                break;
        }

        RenderFrame();
    }

    // clean up DirectX and COM
    CleanD3D();

    return msg.wParam;
}


// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
	case WM_KEYDOWN:
		switch( wParam )
		{
			// User presses escape key to exit program
		case VK_ESCAPE:
			PostQuitMessage( 0 );
			break;
			// User presses A key
		case 0x41:
			Rotate = FALSE;
			break;

			// User presses D key
		case 0x44:
			Rotate = TRUE;
			break;


			
		}

		break; // Needed else all key input quits program

		//------------------------
		
	case WM_LBUTTONDOWN:
		{
			//PostQuitMessage( 0 );
			Rotate = FALSE;
			break;
		}
		
	case WM_RBUTTONDOWN:
		{
			Rotate = TRUE;
			break;
		}
	break;
		//------------------------
	
		
	case WM_DESTROY:
            {
                PostQuitMessage(0);
                return 0;
            } 
			
			break;
    }

    return DefWindowProc (hWnd, message, wParam, lParam);
}


// this function initializes and prepares Direct3D for use
void InitD3D(HWND hWnd)
{
    // create a struct to hold information about the swap chain
    DXGI_SWAP_CHAIN_DESC scd;

    // clear out the struct for use
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    // fill the swap chain description struct
    scd.BufferCount = 1;                                   // one back buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;    // use 32-bit color
    scd.BufferDesc.Width = SCREEN_WIDTH;                   // set the back buffer width
    scd.BufferDesc.Height = SCREEN_HEIGHT;                 // set the back buffer height
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;     // how swap chain is to be used
    scd.OutputWindow = hWnd;                               // the window to be used
    scd.SampleDesc.Count = 4;                              // how many multisamples
    scd.Windowed = TRUE;                                   // windowed/full-screen mode
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;    // allow full-screen switching

    // create a device, device context and swap chain using the information in the scd struct
    D3D11CreateDeviceAndSwapChain(NULL,
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
                                  &devcon);


    // create the depth buffer texture
    D3D11_TEXTURE2D_DESC texd;
    ZeroMemory(&texd, sizeof(texd));

    texd.Width = SCREEN_WIDTH;
    texd.Height = SCREEN_HEIGHT;
    texd.ArraySize = 1;
    texd.MipLevels = 1;
    texd.SampleDesc.Count = 4;
    texd.Format = DXGI_FORMAT_D32_FLOAT;
    texd.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D *pDepthBuffer;
    dev->CreateTexture2D(&texd, NULL, &pDepthBuffer);

    // create the depth buffer
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
    ZeroMemory(&dsvd, sizeof(dsvd));

    dsvd.Format = DXGI_FORMAT_D32_FLOAT;
    dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

    dev->CreateDepthStencilView(pDepthBuffer, &dsvd, &zbuffer);
    pDepthBuffer->Release();

    // get the address of the back buffer
    ID3D11Texture2D *pBackBuffer;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    // use the back buffer address to create the render target
    dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
    pBackBuffer->Release();

    // set the render target as the back buffer
    devcon->OMSetRenderTargets(1, &backbuffer, zbuffer);


    // Set the viewport
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = SCREEN_WIDTH;
    viewport.Height = SCREEN_HEIGHT;
    viewport.MinDepth = 0;    // the closest an object can be on the depth buffer is 0.0
    viewport.MaxDepth = 1;    // the farthest an object can be on the depth buffer is 1.0

    devcon->RSSetViewports(1, &viewport);

    InitPipeline();
    InitGraphics();
}


// this is the function used to render a single frame
void RenderFrame(void)
{
    D3DXMATRIX matRotateY[4], matView, matRotateZ[4], matProjection, matTranslate[4];
    D3DXMATRIX matFinal[4];

    static float Time = 0.0f; Time += 0.001f;


	if( Rotate == FALSE )
	{
		// create a world matrices
		D3DXMatrixRotationZ( &matRotateZ[0], 0);//Time / 2 );
		D3DXMatrixRotationY( &matRotateY[0], 0);//Time );
	
		D3DXMatrixRotationZ( &matRotateZ[1], Time );
		D3DXMatrixRotationY( &matRotateY[1], Time + 3.14159f );
	
		D3DXMatrixRotationZ( &matRotateZ[2], Time * 2 );
		D3DXMatrixRotationY( &matRotateY[2], Time );
	
		D3DXMatrixRotationZ( &matRotateZ[3], -Time );
		D3DXMatrixRotationY( &matRotateY[3], Time + 3.14159f );
	}
	else if( Rotate == TRUE )
	{
		// create a world matrices
		D3DXMatrixRotationZ( &matRotateZ[0], 0);//Time * 2 );
		D3DXMatrixRotationY( &matRotateY[0], 0);//Time );
	
		D3DXMatrixRotationZ( &matRotateZ[1], -Time );
		D3DXMatrixRotationY( &matRotateY[1], Time + 3.14159f );
	
		D3DXMatrixRotationZ( &matRotateZ[2], Time - 2 );
		D3DXMatrixRotationY( &matRotateY[2], Time );
	
		D3DXMatrixRotationZ( &matRotateZ[3], Time );
		D3DXMatrixRotationY( &matRotateY[3], Time + 3.14159f );
	}

	


	D3DXMatrixTranslation( &matTranslate[0], a, 0.0f, 0.0f );
	D3DXMatrixTranslation( &matTranslate[1], 0.0f, 3.0f, 0.0f );
	D3DXMatrixTranslation( &matTranslate[2], 0.0f, 6.0f, 0.0f );
	D3DXMatrixTranslation( &matTranslate[3], 0.0f, 9.0f, 0.0f );

    // create a view matrix
    D3DXMatrixLookAtLH(&matView,
                       &D3DXVECTOR3(0.0f, 0.5f, 31.5f),   // the camera position
                       &D3DXVECTOR3(0.0f, 0.0f, 0.0f),    // the look-at position
                       &D3DXVECTOR3(0.0f, 1.0f, 0.0f));   // the up direction

    // create a projection matrix
    D3DXMatrixPerspectiveFovLH(&matProjection,
                               (FLOAT)D3DXToRadian(45),                    // field of view
                               (FLOAT)SCREEN_WIDTH / (FLOAT)SCREEN_HEIGHT, // aspect ratio
                               1.0f,                                       // near view-plane
                               100.0f);                                    // far view-plane

	//matRotateY[0] = matRotateY[0] * matRotateZ[0];
	matRotateY[1] = matRotateY[1] * matRotateZ[1];
	matRotateY[2] = matRotateY[2] * matRotateZ[2];
	matRotateY[3] = matRotateY[3] * matRotateZ[3];


	// Collision detection data
	//a = -15.75f;
	float halfModel = 1.56f; // half width of my object
	
	if( Travel == FALSE )
	a += 0.005f;
	else if ( Travel == TRUE )
	a -= 0.005f;

	if( a <= ( FLOAT )-SCREEN_WIDTH / 45 + halfModel ) //-15.75f )
		Travel = FALSE;
	if( a >= ( FLOAT )SCREEN_WIDTH / 45 - halfModel ) //18.75f )
		Travel = TRUE;
	//------------------------------------------------------


    // create the final transform
    matFinal[0] = matTranslate[0] * matRotateY[0] * matView * matProjection;
	matFinal[1] = matTranslate[1] * matRotateY[1] * matView * matProjection;
	matFinal[2] = matTranslate[2] * matRotateY[2] * matView * matProjection;
	matFinal[3] = matTranslate[3] * matRotateY[3] * matView * matProjection;

	
    // clear the back buffer to a deep blue
    devcon->ClearRenderTargetView(backbuffer, D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f));

    // clear the depth buffer
    devcon->ClearDepthStencilView(zbuffer, D3D11_CLEAR_DEPTH, 1.0f, 0);

        // select which vertex buffer to display
        UINT stride = sizeof(VERTEX);
        UINT offset = 0;
        devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
        devcon->IASetIndexBuffer(pIBuffer, DXGI_FORMAT_R32_UINT, 0);

        // select which primtive type we are using
        devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // draw the Model
        
		for( int i = 0; i <= 4; i++) // 4 is the number of matFinal[i]
		{
			devcon->UpdateSubresource(pCBuffer, 0, 0, &matFinal[i], 0, 0);
			devcon->DrawIndexed(18, 0, 0);
		}
		/*
		devcon->UpdateSubresource(pCBuffer, 0, 0, &matFinal[0], 0, 0);
        devcon->DrawIndexed(18, 0, 0);
		devcon->UpdateSubresource(pCBuffer, 0, 0, &matFinal[1], 0, 0);
        devcon->DrawIndexed(18, 0, 0);
		devcon->UpdateSubresource(pCBuffer, 0, 0, &matFinal[2], 0, 0);
        devcon->DrawIndexed(18, 0, 0);
		devcon->UpdateSubresource(pCBuffer, 0, 0, &matFinal[3], 0, 0);
        devcon->DrawIndexed(18, 0, 0);
		*/

    // switch the back buffer and the front buffer
    swapchain->Present(0, 0);
}


// this is the function that cleans up Direct3D and COM
void CleanD3D(void)
{
    swapchain->SetFullscreenState(FALSE, NULL);    // switch to windowed mode

    // close and release all existing COM objects
    pLayout->Release();
    pVS->Release();
    pPS->Release();
    zbuffer->Release();
    pVBuffer->Release();
    pCBuffer->Release();
    pIBuffer->Release();
    swapchain->Release();
    backbuffer->Release();
    dev->Release();
    devcon->Release();
}


// this is the function that creates the shape to render
void InitGraphics()
{
      // create vertices to represent the corners of the Hypercraft
    VERTEX OurVertices[] =
    {
		
		////--------------------- PYRAMID
        // base
		{-1.0f, -1.0f,  1.0f, D3DXCOLOR( 0.0f, 1.0f, 0.0f, 1.0f )},
        { 1.0f, -1.0f,  1.0f, D3DXCOLOR( 0.0f, 0.0f, 1.0f, 1.0f )},
        {-1.0f, -1.0f, -1.0f, D3DXCOLOR( 1.0f, 0.0f, 0.0f, 1.0f )},
        { 1.0f, -1.0f, -1.0f, D3DXCOLOR( 0.0f, 1.0f, 1.0f, 1.0f )},

        // top
        { 0.0f, 1.0f, 0.0f, D3DXCOLOR( 0.0f, 1.0f, 0.0f, 1.0f )}, 
//--------------------------------------------------------------------------------------------------

		//-------------CUBE
		{-1.0f, 1.0f, -1.0f, D3DXCOLOR(1.0f, 0.3f, 0.0f, 1.0f)},
        {1.0f, 1.0f, -1.0f, D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f)},
        {-1.0f, -1.0f, -1.0f, D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f)},
        {1.0f, -1.0f, -1.0f, D3DXCOLOR(1.0f, 0.0f, 1.0f, 1.0f)},
        {-1.0f, 1.0f, 1.0f, D3DXCOLOR(0.0f, 1.0f, 1.0f, 1.0f)},
        {1.0f, 1.0f, 1.0f, D3DXCOLOR(1.0f, 0.0f, 1.0f, 1.0f)},
        {-1.0f, -1.0f, 1.0f, D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f)},
        {1.0f, -1.0f, 1.0f, D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f)}, 
		//--------------------------------------------------------


	};

	
    // create the vertex buffer
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(VERTEX) * 5;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    dev->CreateBuffer(&bd, NULL, &pVBuffer);

    // copy the vertices into the buffer
    D3D11_MAPPED_SUBRESOURCE ms;
    devcon->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);    // map the buffer
    memcpy(ms.pData, OurVertices, sizeof(OurVertices));                 // copy the data
    devcon->Unmap(pVBuffer, NULL);


    // create the index buffer out of DWORDs
    DWORD OurIndices[] = 
    {
		
       0, 2, 1, // Base
	   1, 2, 3,
	   0, 1, 4, // Sides
	   1, 3, 4,
	   3, 2, 4,
	   2, 0, 4,
    };

	DWORD OurIndices1[] = 
    {
        0, 1, 2,    // side 1
        2, 1, 3,
        4, 0, 6,    // side 2
        6, 0, 2,
        7, 5, 6,    // side 3
        6, 5, 4,
        3, 1, 7,    // side 4
        7, 1, 5,
        4, 5, 0,    // side 5
        0, 5, 1,
        3, 7, 2,    // side 6
        2, 7, 6,
    };


    // create the index buffer
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof( DWORD ) * 36;//18;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = 0;

    dev->CreateBuffer(&bd, NULL, &pIBuffer);

    devcon->Map(pIBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);    // map the buffer
    memcpy(ms.pData, OurIndices, sizeof(OurIndices));                   // copy the data
    devcon->Unmap(pIBuffer, NULL);
//------------------------------------------------------

	

}


// this function loads and prepares the shaders
void InitPipeline()
{
    // load and compile the two shaders
    ID3D10Blob *VS, *PS;
    D3DX11CompileFromFile(L"shaders.hlsl", 0, 0, "VShader", "vs_4_1", 0, 0, 0, &VS, 0, 0);
    D3DX11CompileFromFile(L"shaders.hlsl", 0, 0, "PShader", "ps_4_1", 0, 0, 0, &PS, 0, 0);

    // encapsulate both shaders into shader objects
    dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
    dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);

    // set the shader objects
    devcon->VSSetShader(pVS, 0, 0);
    devcon->PSSetShader(pPS, 0, 0);

    // create the input layout object
    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    
		// Data from the instance buffer
		{ "INSTANCEPOS", 0, DXGI_FORMAT_R32G32B32_FLOAT,    1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "INSTANCECOLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,    1, 12, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "INSTANCEPOS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA }
	};

    dev->CreateInputLayout(ied, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout);
    devcon->IASetInputLayout(pLayout);

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = 64;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    dev->CreateBuffer(&bd, NULL, &pCBuffer);
    devcon->VSSetConstantBuffers(0, 1, &pCBuffer);
}