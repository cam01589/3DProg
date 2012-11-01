// Include the basic header files
#define _WIN32_DCOM
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#include <XAudio2.h>
#include <ShellAPI.h>
#include <MMSystem.h>
#include <conio.h>
#include <xact3wb.h>
#include <strsafe.h>

// Include the Direct3D Library file
#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "d3dx11.lib" )
#pragma comment( lib, "d3dx10.lib" )

// Define the screen resolution
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

// Define sound buffers
#define STREAMING_BUFFER_SIZE 65536
#define MAX_BUFFER_COUNT 3

//--------------------------------------------------------------------------------------
// Helper macros
//--------------------------------------------------------------------------------------
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

// Global declarations
IDXGISwapChain *swapchain;			// The pointer to the swap chain interface
ID3D11Device *dev;					// The pointer to our Direct3D device interface
ID3D11DeviceContext *devcon;		// The pointer to our Direct3D device context
ID3D11RenderTargetView *backbuffer;	// Global declaration
ID3D11InputLayout *pLayout;			// The pointer to the input layout
ID3D11VertexShader *pVS;			// The pointer to the vertex shader
ID3D11PixelShader *pPS;				// The pointer to the pixel shader
ID3D11Buffer *pVBuffer;				// The pointer to the vertex buffer

// A struct to define a single vertex
struct VERTEX 
{
	FLOAT X, Y, Z; 
	D3DXCOLOR color; 
};

//--------------------------------------------------------------------------------------
// Callback structure
//--------------------------------------------------------------------------------------
struct StreamingVoiceContext : public IXAudio2VoiceCallback
{
            STDMETHOD_( void, OnVoiceProcessingPassStart )( UINT32 )
            {
            }
            STDMETHOD_( void, OnVoiceProcessingPassEnd )()
            {
            }
            STDMETHOD_( void, OnStreamEnd )()
            {
            }
            STDMETHOD_( void, OnBufferStart )( void* )
            {
            }
            STDMETHOD_( void, OnBufferEnd )( void* )
            {
                SetEvent( hBufferEndEvent );
            }
            STDMETHOD_( void, OnLoopEnd )( void* )
            {
            }
            STDMETHOD_( void, OnVoiceError )( void*, HRESULT )
            {
            }

    HANDLE hBufferEndEvent;

            StreamingVoiceContext() : hBufferEndEvent( CreateEvent( NULL, FALSE, FALSE, NULL ) )
            {
            }
    virtual ~StreamingVoiceContext()
    {
        CloseHandle( hBufferEndEvent );
    }
};


//--------------------------------------------------------------------------------------
// Wavebank class
//--------------------------------------------------------------------------------------
class Wavebank
{
public:
            Wavebank() : entries( NULL )
            {
            }
            ~Wavebank()
            {
                Release();
            }

    HRESULT Load( const WCHAR* wbfile );
    void    Release()
    {
        SAFE_DELETE_ARRAY( entries );
    }

    DWORD   GetEntryCount() const
    {
        return data.dwEntryCount;
    }
    DWORD   GetEntryLengthInBytes( DWORD index )
    {
        return ( index < data.dwEntryCount ) ? ( entries[index].PlayRegion.dwLength ) : 0;
    }
    DWORD   GetEntryOffset( DWORD index )
    {
        return ( index < data.dwEntryCount ) ? ( entries[index].PlayRegion.dwOffset ) : 0;
    }
    HRESULT GetEntryFormat( DWORD index, WAVEFORMATEX* pFormat );

private:
    WAVEBANKHEADER header;
    WAVEBANKDATA data;
    WAVEBANKENTRY* entries;
};


//--------------------------------------------------------------------------------------
// Forward declaration
//--------------------------------------------------------------------------------------
HRESULT FindMediaFileCch( WCHAR* strDestPath, int cchDest, LPCWSTR strFilename );



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
	HRESULT hr;


	 //
    // Initialize XAudio2
    //
    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    IXAudio2* pXAudio2 = NULL;

    UINT32 flags = 0;
#ifdef _DEBUG
    flags |= XAUDIO2_DEBUG_ENGINE;
#endif

    if( FAILED( hr = XAudio2Create( &pXAudio2, flags ) ) )
    {
        //wprintf( L"Failed to init XAudio2 engine: %#X\n", hr );
        CoUninitialize();
        return 0;
    }

    //
    // Create a mastering voice
    //
    IXAudio2MasteringVoice* pMasteringVoice = NULL;

    if( FAILED( hr = pXAudio2->CreateMasteringVoice( &pMasteringVoice ) ) )
    {
        //wprintf( L"Failed creating mastering voice: %#X\n", hr );
        SAFE_RELEASE( pXAudio2 );
        CoUninitialize();
        return 0;
    }

    //
    // Find our wave bank file
    //
    WCHAR wavebank[ MAX_PATH ];

    if( FAILED( hr = FindMediaFileCch( wavebank, MAX_PATH, L"wavebank.xwb" ) ) )
    {
//        wprintf( L"Failed to find media file (%#X)\n", hr );
        SAFE_RELEASE( pXAudio2 );
        CoUninitialize();
        return 0;
    }

    //
    // Extract wavebank data (entries, formats, offsets, and sizes)
    //
    // Note we are only using XACTBLD to create sector-aligned streaming data to allow us to use
    // async unbuffered I/O. Raw .WAV files do not meet these requirements.
    //
    Wavebank wb;

    if( FAILED( hr = wb.Load( wavebank ) ) )
    {
//        wprintf( L"Failed to wavebank data (%#X)\n", hr );
        SAFE_RELEASE( pXAudio2 );
        CoUninitialize();
        return 0;
    }

    //
    // Open the wavebank file for async operations
    //

    HANDLE hAsync = CreateFile( wavebank, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL );

    if( hAsync == INVALID_HANDLE_VALUE )
    {
//        wprintf( L"Failed to open wavebank for async (%#X)\n", HRESULT_FROM_WIN32( GetLastError() ) );
        SAFE_RELEASE( pXAudio2 );
        CoUninitialize();
        return 0;
    }

    //
    // Repeated loop through all the wavebank entries
    //
    bool exit = false;

    while( !exit )
    {
        for( DWORD i = 0; i < wb.GetEntryCount(); ++i )
        {
//            wprintf( L"Now playing wave entry %d", i );

            //
            // Get the info we need to play back this wave (need enough space for PCM, ADPCM, and xWMA formats)
            //
            char formatBuff[ 64 ]; 
            WAVEFORMATEX *wfx = reinterpret_cast<WAVEFORMATEX*>(&formatBuff);

            if( FAILED( hr = wb.GetEntryFormat( i, wfx ) ) )
            {
             //   wprintf( L"\nCouldn't get wave format for entry %d: error 0x%x\n", i, hr );
                exit = true;
                break;
            }

            DWORD waveOffset = wb.GetEntryOffset( i );
            DWORD waveLength = wb.GetEntryLengthInBytes( i );

            //
            // Create an XAudio2 voice to stream this wave
            //
            StreamingVoiceContext voiceContext;

            IXAudio2SourceVoice* pSourceVoice;
            if( FAILED( hr = pXAudio2->CreateSourceVoice( &pSourceVoice, wfx, 0, 1.0f, &voiceContext ) ) )
            {
           //     wprintf( L"\nError %#X creating source voice\n", hr );
                exit = true;
                break;
            }
            pSourceVoice->Start( 0, 0 );

            //
            // Create an overlapped structure and buffers to handle the async I/O
            //
            OVERLAPPED ovlCurrentRequest = {0};
            ovlCurrentRequest.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

            BYTE buffers[MAX_BUFFER_COUNT][STREAMING_BUFFER_SIZE];
            DWORD currentDiskReadBuffer = 0;
            DWORD currentPosition = 0;

            //
            // This sample code shows the simplest way to manage asynchronous
            // streaming. There are three different processes involved. One is the management
            // process, which is what we're writing here. The other two processes are
            // essentially hardware operations: disk reads from the I/O system, and
            // audio processing from XAudio2. Disk reads and audio playback both happen
            // without much intervention from our application, so our job is just to make
            // sure that the data being read off the disk makes it over to the audio
            // processor in time to be played back.
            //
            // There are two events that can happen in this system. The disk I/O system can
            // signal that data is ready, and the audio system can signal that it's done
            // playing back data. We can handle either or both of these events either synchronously
            // (via polling) or asynchronously (via callbacks or by waiting on an event
            // object).
            //
            while( currentPosition < waveLength )
            {
               // wprintf( L"." );

                if( GetAsyncKeyState( VK_ESCAPE ) )
                {
                    exit = true;

                    while( GetAsyncKeyState( VK_ESCAPE ) )
                        Sleep( 10 );

                    break;
                }

                //
                // Issue a request.
                //
                // Note: although the file read will be done asynchronously, it is possible for the
                // call to ReadFileEx to block for longer than you might think. If the I/O system needs
                // to read the file allocation table in order to satisfy the read, it will do that
                // BEFORE returning from ReadFileEx. That means that this call could potentially
                // block for several milliseconds! In order to get "true" async I/O you should put
                // this entire loop on a separate thread.
                //
                // Second note: async requests have to be a multiple of the disk sector size. Rather than
                // handle this conditionally, make all reads the same size but remember how many
                // bytes we actually want and only submit that many to the voice.
                //
                DWORD cbValid = min( STREAMING_BUFFER_SIZE, waveLength - currentPosition );
                ovlCurrentRequest.Offset = waveOffset + currentPosition;

                if( !ReadFileEx( hAsync, buffers[ currentDiskReadBuffer ], STREAMING_BUFFER_SIZE, &ovlCurrentRequest,
                                 NULL ) )
                {
//                    wprintf( L"\nCouldn't start async read: error %#X\n", HRESULT_FROM_WIN32( GetLastError() ) );
                    exit = true;
                    break;
                }

                currentPosition += cbValid;

                //
                // At this point the read is progressing in the background and we are free to do
                // other processing while we wait for it to finish. For the purposes of this sample,
                // however, we'll just go to sleep until the read is done.
                //
                DWORD cb;
                GetOverlappedResult( hAsync, &ovlCurrentRequest, &cb, TRUE );

                //
                // Now that the event has been signaled, we know we have audio available. The next
                // question is whether our XAudio2 source voice has played enough data for us to give
                // it another buffer full of audio. We'd like to keep no more than MAX_BUFFER_COUNT - 1
                // buffers on the queue, so that one buffer is always free for disk I/O.
                //
                XAUDIO2_VOICE_STATE state;
                for(; ; )
                {
                    pSourceVoice->GetState( &state );
                    if( state.BuffersQueued < MAX_BUFFER_COUNT - 1 )
                        break;

                    WaitForSingleObject( voiceContext.hBufferEndEvent, INFINITE );
                }

                //
                // At this point we have a buffer full of audio and enough room to submit it, so
                // let's submit it and get another read request going.
                //
                XAUDIO2_BUFFER buf = {0};
                buf.AudioBytes = cbValid;
                buf.pAudioData = buffers[currentDiskReadBuffer];
                if( currentPosition >= waveLength )
                    buf.Flags = XAUDIO2_END_OF_STREAM;

                pSourceVoice->SubmitSourceBuffer( &buf );

                currentDiskReadBuffer++;
                currentDiskReadBuffer %= MAX_BUFFER_COUNT;
            }

            if( !exit )
            {
//                wprintf( L"done streaming.." );

                XAUDIO2_VOICE_STATE state;
                for(; ; )
                {
                    pSourceVoice->GetState( &state );
                    if( !state.BuffersQueued )
                        break;

//                    wprintf( L"." );
                    WaitForSingleObject( voiceContext.hBufferEndEvent, INFINITE );
                }
            }

            //
            // Clean up
            //
            pSourceVoice->Stop( 0 );
            pSourceVoice->DestroyVoice();

            CloseHandle( ovlCurrentRequest.hEvent );

//            wprintf( L"stopped\n" );

            if( exit )
                break;

            Sleep( 500 );
        }
    }

    //
    // Cleanup XAudio2
    //

    // All XAudio2 interfaces are released when the engine is destroyed, but being tidy
    pMasteringVoice->DestroyVoice();

    CloseHandle( hAsync );

    SAFE_RELEASE( pXAudio2 );
    CoUninitialize();



		//--------------------------------------------
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


	//-------------------------------------------------------------------------

	




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
}









//--------------------------------------------------------------------------------------
// Wavebank methods
//--------------------------------------------------------------------------------------
HRESULT Wavebank::Load( const WCHAR* wbfile )
{
    Release();

    HANDLE hFile = CreateFile( wbfile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0L, NULL );
    if( hFile == INVALID_HANDLE_VALUE )
        return HRESULT_FROM_WIN32( GetLastError() );

    // Read and verify header
    DWORD bytes;
    if( !ReadFile( hFile, &header, sizeof( header ), &bytes, NULL )
        || bytes != sizeof( header ) )
    {
        CloseHandle( hFile );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    if( header.dwSignature != WAVEBANK_HEADER_SIGNATURE
        || ( WAVEBANK_HEADER_VERSION < header.dwHeaderVersion ) )
    {
        CloseHandle( hFile );
        return E_FAIL;
    }

    // Load wavebank data
    SetFilePointer( hFile, header.Segments[ WAVEBANK_SEGIDX_BANKDATA ].dwOffset, 0, SEEK_SET );

    if( !ReadFile( hFile, &data, sizeof( data ), &bytes, NULL )
        || bytes != sizeof( data ) )
    {
        CloseHandle( hFile );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    // Load entries
    DWORD cbEntries = header.Segments[ WAVEBANK_SEGIDX_ENTRYMETADATA ].dwLength;

    if( data.dwEntryCount != ( cbEntries / sizeof( WAVEBANKENTRY ) ) )
    {
        CloseHandle( hFile );
        return E_FAIL;
    }

    entries = new WAVEBANKENTRY[ data.dwEntryCount ];

    SetFilePointer( hFile, header.Segments[ WAVEBANK_SEGIDX_ENTRYMETADATA ].dwOffset, 0, SEEK_SET );

    if( !ReadFile( hFile, entries, cbEntries, &bytes, NULL )
        || bytes != cbEntries )
    {
        SAFE_DELETE_ARRAY( entries );
        CloseHandle( hFile );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    CloseHandle( hFile );

    return S_OK;
}

HRESULT Wavebank::GetEntryFormat( DWORD index, WAVEFORMATEX* pFormat )
{
    if( index >= data.dwEntryCount || !entries )
        return E_FAIL;

    WAVEBANKMINIWAVEFORMAT& miniFmt = ( data.dwFlags & WAVEBANK_FLAGS_COMPACT )
        ? data.CompactFormat : ( entries[ index ].Format );

    switch( miniFmt.wFormatTag )
    {
        case WAVEBANKMINIFORMAT_TAG_PCM:
            pFormat->wFormatTag = WAVE_FORMAT_PCM;
            pFormat->cbSize = 0;
            break;

        case WAVEBANKMINIFORMAT_TAG_ADPCM:
            pFormat->wFormatTag = WAVE_FORMAT_ADPCM;
            pFormat->cbSize = 32; /* MSADPCM_FORMAT_EXTRA_BYTES */
            {
                ADPCMWAVEFORMAT *adpcmFmt = reinterpret_cast<ADPCMWAVEFORMAT*>(pFormat);
                adpcmFmt->wSamplesPerBlock = (WORD) miniFmt.AdpcmSamplesPerBlock();
                miniFmt.AdpcmFillCoefficientTable( adpcmFmt );
            }
            break;

        case WAVEBANKMINIFORMAT_TAG_WMA:
            pFormat->wFormatTag = (miniFmt.wBitsPerSample & 0x1) ? WAVE_FORMAT_WMAUDIO3 : WAVE_FORMAT_WMAUDIO2;
            pFormat->cbSize = 0;
            break;

        default:
            // WAVEBANKMINIFORMAT_TAG_XMA is only valid for Xbox
            return E_FAIL;
    }

    pFormat->nChannels = miniFmt.nChannels;
    pFormat->wBitsPerSample = miniFmt.BitsPerSample();
    pFormat->nBlockAlign = (WORD) miniFmt.BlockAlign();
    pFormat->nSamplesPerSec = miniFmt.nSamplesPerSec;
    pFormat->nAvgBytesPerSec = miniFmt.AvgBytesPerSec();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Helper function to try to find the location of a media file
//--------------------------------------------------------------------------------------
HRESULT FindMediaFileCch( WCHAR* strDestPath, int cchDest, LPCWSTR strFilename )
{
    bool bFound = false;

    if( NULL == strFilename || strFilename[0] == 0 || NULL == strDestPath || cchDest < 10 )
        return E_INVALIDARG;

    // Get the exe name, and exe path
    WCHAR strExePath[MAX_PATH] = {0};
    WCHAR strExeName[MAX_PATH] = {0};
    WCHAR* strLastSlash = NULL;
    GetModuleFileName( NULL, strExePath, MAX_PATH );
    strExePath[MAX_PATH - 1] = 0;
    strLastSlash = wcsrchr( strExePath, TEXT( '\\' ) );
    if( strLastSlash )
    {
        StringCchCopy( strExeName, MAX_PATH, &strLastSlash[1] );

        // Chop the exe name from the exe path
        *strLastSlash = 0;

        // Chop the .exe from the exe name
        strLastSlash = wcsrchr( strExeName, TEXT( '.' ) );
        if( strLastSlash )
            *strLastSlash = 0;
    }

    StringCchCopy( strDestPath, cchDest, strFilename );
    if( GetFileAttributes( strDestPath ) != 0xFFFFFFFF )
        return S_OK;

    // Search all parent directories starting at .\ and using strFilename as the leaf name
    WCHAR strLeafName[MAX_PATH] = {0};
    StringCchCopy( strLeafName, MAX_PATH, strFilename );

    WCHAR strFullPath[MAX_PATH] = {0};
    WCHAR strFullFileName[MAX_PATH] = {0};
    WCHAR strSearch[MAX_PATH] = {0};
    WCHAR* strFilePart = NULL;

    GetFullPathName( L".", MAX_PATH, strFullPath, &strFilePart );
    if( strFilePart == NULL )
        return E_FAIL;

    while( strFilePart != NULL && *strFilePart != '\0' )
    {
        StringCchPrintf( strFullFileName, MAX_PATH, L"%s\\%s", strFullPath, strLeafName );
        if( GetFileAttributes( strFullFileName ) != 0xFFFFFFFF )
        {
            StringCchCopy( strDestPath, cchDest, strFullFileName );
            bFound = true;
            break;
        }

        StringCchPrintf( strFullFileName, MAX_PATH, L"%s\\%s\\%s", strFullPath, strExeName, strLeafName );
        if( GetFileAttributes( strFullFileName ) != 0xFFFFFFFF )
        {
            StringCchCopy( strDestPath, cchDest, strFullFileName );
            bFound = true;
            break;
        }

        StringCchPrintf( strSearch, MAX_PATH, L"%s\\..", strFullPath );
        GetFullPathName( strSearch, MAX_PATH, strFullPath, &strFilePart );
    }
    if( bFound )
        return S_OK;

    // On failure, return the file as the path but also return an error code
    StringCchCopy( strDestPath, cchDest, strFilename );

    return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
}
