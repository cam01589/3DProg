// Include the basic windows header files and the Direct3D header files
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>

// Include the Direct3D Library file
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dx10.lib")

// Define the screen resolution
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

// Global declarations
IDXGISwapChain *swapchain;             // The pointer to the swap chain interface
ID3D11Device *dev;                     // The pointer to our Direct3D device interface
ID3D11DeviceContext *devcon;           // The pointer to our Direct3D device context
ID3D11RenderTargetView *backbuffer;    // The pointer to our back buffer
ID3D11DepthStencilView *zbuffer;       // The pointer to our depth buffer
ID3D11InputLayout *pLayout;            // The pointer to the input layout
ID3D11VertexShader *pVS;               // The pointer to the vertex shader
ID3D11PixelShader *pPS;                // The pointer to the pixel shader
ID3D11Buffer *pVBuffer;                // The pointer to the vertex buffer
ID3D11Buffer *pIBuffer;                // The pointer to the index buffer
ID3D11Buffer *pCBuffer;                // The pointer to the constant buffer
ID3D11ShaderResourceView *pTexture;	   // The pointer to the texture

// A struct to define a single vertex
struct VERTEX {FLOAT X, Y, Z; D3DXVECTOR3 Normal; FLOAT U, V; };

// A struct to define the constant buffer
struct CBUFFER
{
    D3DXMATRIX Final;
    D3DXMATRIX Rotation;
    D3DXVECTOR4 LightVector;
    D3DXCOLOR LightColor;
    D3DXCOLOR AmbientColor;
};

// Function prototypes
void InitD3D(HWND hWnd);    // Sets up and initializes Direct3D
void RenderFrame(void);     // Renders a single frame
void CleanD3D(void);        // Closes Direct3D and releases memory
void InitGraphics(void);    // Creates the shape to render
void InitPipeline(void);    // Loads and prepares the shaders

// The WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


// The entry point for any Windows program
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
                          L"Our First Direct3D Program",
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

    // Set up and initialize Direct3D
    InitD3D(hWnd);

    // Enter the main loop:

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

    // Clean up DirectX and COM
    CleanD3D();

    return msg.wParam;
}


// This is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_DESTROY:
            {
                PostQuitMessage(0);
                return 0;
            } break;
    }

    return DefWindowProc (hWnd, message, wParam, lParam);
}


// This function initializes and prepares Direct3D for use
void InitD3D(HWND hWnd)
{
    // Create a struct to hold information about the swap chain
    DXGI_SWAP_CHAIN_DESC scd;

    // Clear out the struct for use
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    // Fill the swap chain description struct
    scd.BufferCount = 1;                                   // one back buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;    // use 32-bit color
    scd.BufferDesc.Width = SCREEN_WIDTH;                   // set the back buffer width
    scd.BufferDesc.Height = SCREEN_HEIGHT;                 // set the back buffer height
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;     // how swap chain is to be used
    scd.OutputWindow = hWnd;                               // the window to be used
    scd.SampleDesc.Count = 4;                              // how many multisamples
    scd.Windowed = TRUE;                                   // windowed/full-screen mode
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;    // allow full-screen switching

    // Create a device, device context and swap chain using the information in the scd struct
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


    // Create the depth buffer texture
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

    // Create the depth buffer
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
    ZeroMemory(&dsvd, sizeof(dsvd));

    dsvd.Format = DXGI_FORMAT_D32_FLOAT;
    dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

    dev->CreateDepthStencilView(pDepthBuffer, &dsvd, &zbuffer);
    pDepthBuffer->Release();

    // Get the address of the back buffer
    ID3D11Texture2D *pBackBuffer;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    // Use the back buffer address to create the render target
    dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
    pBackBuffer->Release();

    // Set the render target as the back buffer
    devcon->OMSetRenderTargets(1, &backbuffer, zbuffer);


    // Set the viewport
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;				// Set the left to 0
    viewport.TopLeftY = 0;				// Set the top to 0
    viewport.Width = SCREEN_WIDTH;		// Set the width to the window's width
    viewport.Height = SCREEN_HEIGHT;    // Set the height to the window's height
    viewport.MinDepth = 0;				// She closest an object can be on the depth buffer is 0.0
    viewport.MaxDepth = 1;				// She farthest an object can be on the depth buffer is 1.0

    devcon->RSSetViewports(1, &viewport);

    InitPipeline();
    InitGraphics();
}


// This is the function used to render a single frame
void RenderFrame(void)
{
    CBUFFER cBuffer;

    cBuffer.LightVector = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 0.0f);
    cBuffer.LightColor = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);
    cBuffer.AmbientColor = D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.0f);

    D3DXMATRIX matRotate, matView, matProjection;
    D3DXMATRIX matFinal;

    static float Time = 0.0f; Time += 0.0003f;

    // Create a world matrices
    D3DXMatrixRotationY(&matRotate, Time);

    // Create a view matrix
    D3DXMatrixLookAtLH(&matView,
                       &D3DXVECTOR3(0.0f, 3.0f, 5.0f),   // the camera position
                       &D3DXVECTOR3(0.0f, 0.0f, 0.0f),    // the look-at position
                       &D3DXVECTOR3(0.0f, 1.0f, 0.0f));   // the up direction

    // Create a projection matrix
    D3DXMatrixPerspectiveFovLH(&matProjection,
                               (FLOAT)D3DXToRadian(45),                    // field of view
                               (FLOAT)SCREEN_WIDTH / (FLOAT)SCREEN_HEIGHT, // aspect ratio
                               1.0f,                                       // near view-plane
                               100.0f);                                    // far view-plane

    // Load the matrices into the constant buffer
    cBuffer.Final = matRotate * matView * matProjection;
    cBuffer.Rotation = matRotate;

    // Clear the back buffer to a deep blue
    devcon->ClearRenderTargetView(backbuffer, D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f));

    // Clear the depth buffer
    devcon->ClearDepthStencilView(zbuffer, D3D11_CLEAR_DEPTH, 1.0f, 0);

        // Select which vertex buffer to display
        UINT stride = sizeof(VERTEX);
        UINT offset = 0;
        devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
        devcon->IASetIndexBuffer(pIBuffer, DXGI_FORMAT_R32_UINT, 0);

        // Select which primtive type we are using
        devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // draw the Object
        devcon->UpdateSubresource(pCBuffer, 0, 0, &cBuffer, 0, 0);
        devcon->PSSetShaderResources( 0, 1, &pTexture );
		devcon->DrawIndexed(36, 0, 0);

    // Switch the back buffer and the front buffer
    swapchain->Present(0, 0);
}


// This is the function that cleans up Direct3D and COM
void CleanD3D(void)
{
    swapchain->SetFullscreenState(FALSE, NULL);    // switch to windowed mode

    // Close and release all existing COM objects
    zbuffer->Release();
    pLayout->Release();
    pVS->Release();
    pPS->Release();
    pVBuffer->Release();
    pIBuffer->Release();
    pCBuffer->Release();
	pTexture->Release();
    swapchain->Release();
    backbuffer->Release();
    dev->Release();
    devcon->Release();
}


// This is the function that creates the shape to render
void InitGraphics()
{
    // Create vertices to represent the corners of the cube
    VERTEX OurVertices[] =
    {
         {-1.0f, -1.0f, 1.0f, D3DXVECTOR3(0.0f, 0.0f, 1.0f), 0.0f, 0.0f},     // Side 1
        {1.0f, -1.0f, 1.0f, D3DXVECTOR3(0.0f, 0.0f, 1.0f), 0.0f, 1.0f},
        {-1.0f, 1.0f, 1.0f, D3DXVECTOR3(0.0f, 0.0f, 1.0f), 1.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, D3DXVECTOR3(0.0f, 0.0f, 1.0f), 1.0f, 1.0f},

        {-1.0f, -1.0f, -1.0f, D3DXVECTOR3(0.0f, 0.0f, -1.0f), 0.0f, 0.0f},    // Side 2
        {-1.0f, 1.0f, -1.0f, D3DXVECTOR3(0.0f, 0.0f, -1.0f), 0.0f, 1.0f},
        {1.0f, -1.0f, -1.0f, D3DXVECTOR3(0.0f, 0.0f, -1.0f), 1.0f, 0.0f},
        {1.0f, 1.0f, -1.0f, D3DXVECTOR3(0.0f, 0.0f, -1.0f), 1.0f, 1.0f},

        {-1.0f, 1.0f, -1.0f, D3DXVECTOR3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f},	  // Side 3
        {-1.0f, 1.0f, 1.0f, D3DXVECTOR3(0.0f, 1.0f, 0.0f), 0.0f, 1.0f},
        {1.0f, 1.0f, -1.0f, D3DXVECTOR3(0.0f, 1.0f, 0.0f), 1.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, D3DXVECTOR3(0.0f, 1.0f, 0.0f), 1.0f, 1.0f},

        {-1.0f, -1.0f, -1.0f, D3DXVECTOR3(0.0f, -1.0f, 0.0f), 0.0f, 0.0f},    // Side 4
        {1.0f, -1.0f, -1.0f, D3DXVECTOR3(0.0f, -1.0f, 0.0f), 0.0f, 1.0f},
        {-1.0f, -1.0f, 1.0f, D3DXVECTOR3(0.0f, -1.0f, 0.0f), 1.0f, 0.0f},
        {1.0f, -1.0f, 1.0f, D3DXVECTOR3(0.0f, -1.0f, 0.0f), 1.0f, 1.0f},

        {1.0f, -1.0f, -1.0f, D3DXVECTOR3(1.0f, 0.0f, 0.0f), 0.0f, 0.0f},      // Side 5
        {1.0f, 1.0f, -1.0f, D3DXVECTOR3(1.0f, 0.0f, 0.0f), 0.0f, 1.0f},
        {1.0f, -1.0f, 1.0f, D3DXVECTOR3(1.0f, 0.0f, 0.0f), 1.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, D3DXVECTOR3(1.0f, 0.0f, 0.0f), 1.0f, 1.0f},

        {-1.0f, -1.0f, -1.0f, D3DXVECTOR3(-1.0f, 0.0f, 0.0f), 0.0f, 0.0f},    // Side 6
        {-1.0f, -1.0f, 1.0f, D3DXVECTOR3(-1.0f, 0.0f, 0.0f), 0.0f, 1.0f},
        {-1.0f, 1.0f, -1.0f, D3DXVECTOR3(-1.0f, 0.0f, 0.0f), 1.0f, 0.0f},
        {-1.0f, 1.0f, 1.0f, D3DXVECTOR3(-1.0f, 0.0f, 0.0f), 1.0f, 1.0f},  
	};

    // Create the vertex buffer
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(VERTEX) * 24;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    dev->CreateBuffer(&bd, NULL, &pVBuffer);

    // Copy the vertices into the buffer
    D3D11_MAPPED_SUBRESOURCE ms;
    devcon->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);    // map the buffer
    memcpy(ms.pData, OurVertices, sizeof(OurVertices));                 // copy the data
    devcon->Unmap(pVBuffer, NULL);


    // Create the index buffer out of DWORDs
    DWORD OurIndices[] = 
    {
        0, 1, 2,    // Side 1
        2, 1, 3,
        4, 5, 6,    // Side 2
        6, 5, 7,
        8, 9, 10,   // Side 3
        10, 9, 11,
        12, 13, 14, // Side 4
        14, 13, 15,
        16, 17, 18, // Side 5
        18, 17, 19,
        20, 21, 22, // Side 6
        22, 21, 23,
    };

    // Create the index buffer
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(DWORD) * 36;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = 0;

    dev->CreateBuffer(&bd, NULL, &pIBuffer);

    devcon->Map(pIBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);    // map the buffer
    memcpy(ms.pData, OurIndices, sizeof(OurIndices));                   // copy the data
    devcon->Unmap(pIBuffer, NULL);
	D3DX11CreateShaderResourceViewFromFile( dev,			// The Direct3D device
											L"Wood.png",	// Load Wood.png in the local folder
											NULL,			// No additional information
											NULL,			// No multithreading
											&pTexture,		// Address of the shader-resource-view
											NULL );			// No multithreading
}


// This function loads and prepares the shaders
void InitPipeline()
{
    // Compile the shaders
    ID3D10Blob *VS, *PS;
    D3DX11CompileFromFile(L"shaders.hlsl", 0, 0, "VShader", "vs_4_1", 0, 0, 0, &VS, 0, 0);
    D3DX11CompileFromFile(L"shaders.hlsl", 0, 0, "PShader", "ps_4_1", 0, 0, 0, &PS, 0, 0);

    // Create the shader objects
    dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
    dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);

    // Set the shader objects
    devcon->VSSetShader(pVS, 0, 0);
    devcon->PSSetShader(pPS, 0, 0);

    // Create the input element object
    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        //{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

    dev->CreateInputLayout(ied, 3, VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout);
    devcon->IASetInputLayout(pLayout);

    // Create the constant buffer
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = 176;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    dev->CreateBuffer(&bd, NULL, &pCBuffer);

    devcon->VSSetConstantBuffers(0, 1, &pCBuffer);
}