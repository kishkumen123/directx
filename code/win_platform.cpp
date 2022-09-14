/*
TODO WRONG FIXME

INCOMPLETE STUDY QUESTION FUTURE CONSIDER UNCLEAR UNTESTED

IMPORTANT NOTE
*/

//QUESTION: Devon.
// 10) Do you have any thoughts on satisfying multiple interests of yours? So for example currently I'm:
//    a) working on DirectX to stay in line with our tutoring sessions + all the other interests / benefits that come from practicing and learning this
//    b) working on a side project with a friend ( this is just a once a week thing, and is text editor related, so learning how to load TTF files and render text, this will also require UI but using IMGUI)
//    c) I have a general plan of things that I want to learn and work on. So after the Ant Colony simulation, I started working on learning how to write a simple Retained Mode UI. To then later learn how to write my own Immediate Mode UI.
//    d) working on improving my own math library (vectors, matrices) and this directly ties to DirectX as well as my own software renderer, as I don't have good translation/rotation/scale/shear for my triangles/bitmaps, in my software renderer.
//    e) and now I'm finding that I'm interested in this youtube series called "From Nand to Tetris" which goes over how to go from writing logic gates, to getting a tetris game working.
//

#include "base_inc.h"
#include "win32_base_inc.h"

#include <d3d11.h>       // D3D interface
#include <d3dcompiler.h> // shader compiler
//#include <d3d9types.h>
#include <directxmath.h> // TODO: fucking ANNIHILATE this. Its fucking trash. And I dont want garbage imports
//#include <D3dx9math.h>
//#include <dxgi.h>
//#include <DirectXMath.h>

//#pragma comment( lib, "d3dx9.lib" )
#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "d3dcompiler" )
//#pragma comment( lib, "dxgi.lib" )

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#if DEBUG

enum{ DebugCycleCounter_count, };
enum{ DebugTickCounter_count, };

typedef struct DebugCycleCounter{
    u64 cycle_count;
    u32 hit_count;
    char *name;
} DebugCycleCounter;

typedef struct DebugTickCounter{
    f64 tick_count;
    u32 hit_count;
    char *name;
} DebugTickCounter;

#define BEGIN_CYCLE_COUNTER(ID) u64 StartCycleCounter_##ID = __rdtsc();
#define BEGIN_TICK_COUNTER(ID) u64 StartTickCounter_##ID = clock->get_ticks()
#define BEGIN_TICK_COUNTER_L(ID) u64 StartTickCounter_##ID = clock.get_ticks()
#define END_CYCLE_COUNTER(ID) cycle_counters[DebugCycleCounter_##ID].cycle_count += __rdtsc() - StartCycleCounter_##ID; ++cycle_counters[DebugCycleCounter_##ID].hit_count; cycle_counters[DebugCycleCounter_##ID].name = #ID;
#define END_TICK_COUNTER(ID) tick_counters[DebugTickCounter_##ID].tick_count += clock->get_seconds_elapsed(StartTickCounter_##ID, clock->get_ticks()); ++tick_counters[DebugTickCounter_##ID].hit_count; tick_counters[DebugTickCounter_##ID].name = #ID;
#define END_TICK_COUNTER_L(ID) tick_counters[DebugTickCounter_##ID].tick_count += clock.get_seconds_elapsed(StartTickCounter_##ID, clock.get_ticks()); ++tick_counters[DebugTickCounter_##ID].hit_count; tick_counters[DebugTickCounter_##ID].name = #ID;

DebugCycleCounter cycle_counters[DebugCycleCounter_count];
DebugTickCounter tick_counters[DebugTickCounter_count];

#define handle_debug_counters(sim) handle_debug_counters_(sim)
static void handle_debug_counters_(u32 simulations){
    print("Debug Cycle Counters:\n");
    print("    Name:%-18s Cycles:%-3s Hits:%-3s C/H:%s\n", "", "", "", "");
    for(u32 counter_index=0; counter_index < (u32)ArrayCount(cycle_counters); ++counter_index){
        DebugCycleCounter *counter = cycle_counters + counter_index;
        if(counter->hit_count){
            print("    %-23s %-10u %-8u %u\n", counter->name, counter->cycle_count, counter->hit_count, (counter->cycle_count / counter->hit_count));
            counter->cycle_count = 0;
            counter->hit_count = 0;
        }
    }
    print("Debug Tick Counters:\n");
    print("    Name:%-18s Ticks:%-4s Hits:%-3s T/H:%s\n", "", "", "", "");
    for(u32 counter_index=0; counter_index < (u32)ArrayCount(tick_counters); ++counter_index){
        DebugTickCounter *counter = tick_counters + counter_index;
        if(counter->hit_count){
            print("    %-23s %-10f %-8u %f\n", counter->name, counter->tick_count, counter->hit_count/simulations, (counter->tick_count / counter->hit_count));
            counter->tick_count = 0;
            counter->hit_count = 0;
        }
    }
}

#else
#define BEGIN_CYCLE_COUNTER(ID)
#define END_CYCLE_COUNTER(ID)
#define BEGIN_TICK_COUNTER(ID)
#define END_TICK_COUNTER(ID)
#define BEGIN_TICK_COUNTER_L(ID)
#define END_TICK_COUNTER_L(ID)
#define handle_debug_counters()
#endif

typedef s64 GetTicks(void);
typedef f64 GetSecondsElapsed(s64 start, s64 end);
typedef f64 GetMsElapsed(s64 start, s64 end);

typedef struct Clock{
    f64 dt;
    s64 frequency;
    GetTicks* get_ticks;
    GetSecondsElapsed* get_seconds_elapsed;
    GetMsElapsed* get_ms_elapsed;
} Clock;

typedef struct Button{
    bool pressed;
    bool held;
} Button;

typedef struct Controller{
    Button up;
    Button down;
    Button left;
    Button right;
    Button e;
    Button q;
    Button one;
    Button two;
    Button three;
    Button four;
    Button m_left;
    Button m_right;
    Button m_middle;
    v2 mouse_pos;
} Controller;

typedef struct RenderBuffer{
    void* base;
    size_t size;

    s32 padding;
    s32 width;
    s32 height;
    s32 bytes_per_pixel;
    s32 stride;

    BITMAPINFO bitmap_info;

    Arena* render_command_arena;
    Arena* arena;
    HDC device_context;
} RenderBuffer;

typedef struct Memory{
    void* base;
    size_t size;

    void* permanent_base;
    size_t permanent_size;
    void* transient_base;
    size_t transient_size;

    bool initialized;
} Memory;


global bool global_running;
global RenderBuffer render_buffer;
global Controller controller;
global Memory memory;
global Clock clock;
global Arena* global_arena;


static s64 get_ticks(){
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return(result.QuadPart);
}

static f64 get_seconds_elapsed(s64 start, s64 end){
    f64 result;
    result = ((f64)(end - start) / ((f64)clock.frequency));
    return(result);
}

static f64 get_ms_elapsed(s64 start, s64 end){
    f64 result;
    result = (1000 * ((f64)(end - start) / ((f64)clock.frequency)));
    return(result);
}

static void
init_clock(Clock* clock){
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock->frequency = frequency.QuadPart;
    clock->get_ticks = get_ticks;
    clock->get_seconds_elapsed = get_seconds_elapsed;
    clock->get_ms_elapsed = get_ms_elapsed;
}

static void
init_memory(Memory* memory){
    memory->permanent_size = MB(500);
    memory->transient_size = GB(1);
    memory->size = memory->permanent_size + memory->transient_size;
    memory->base = os_virtual_alloc(memory->size);
    memory->permanent_base = memory->base;
    memory->transient_base = (u8*)memory->base + memory->permanent_size;
}

static void
init_render_buffer(RenderBuffer* render_buffer){
    render_buffer->width = SCREEN_WIDTH;
    render_buffer->height = SCREEN_HEIGHT;
    render_buffer->padding = 10;

    render_buffer->bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    render_buffer->bitmap_info.bmiHeader.biWidth = SCREEN_WIDTH;
    render_buffer->bitmap_info.bmiHeader.biHeight = -SCREEN_HEIGHT;
    render_buffer->bitmap_info.bmiHeader.biPlanes = 1;
    render_buffer->bitmap_info.bmiHeader.biBitCount = 32;
    render_buffer->bitmap_info.bmiHeader.biCompression = BI_RGB;

    s32 bytes_per_pixel = 4;
    render_buffer->bytes_per_pixel = bytes_per_pixel;
    render_buffer->stride = SCREEN_WIDTH * bytes_per_pixel;
    render_buffer->size = SCREEN_WIDTH * SCREEN_HEIGHT * bytes_per_pixel;
    render_buffer->base = os_virtual_alloc(render_buffer->size);

    render_buffer->render_command_arena = allocate_arena(MB(16));
    render_buffer->arena = allocate_arena(MB(4));
}

static void
update_window(HWND window, RenderBuffer render_buffer){
    s32 padding = render_buffer.padding;

    PatBlt(render_buffer.device_context, 0, 0, render_buffer.width + padding, padding, WHITENESS);
    PatBlt(render_buffer.device_context, 0, padding, padding, render_buffer.height + padding, WHITENESS);
    PatBlt(render_buffer.device_context, render_buffer.width + padding, 0, padding, render_buffer.height + padding, WHITENESS);
    PatBlt(render_buffer.device_context, padding, render_buffer.height + padding, render_buffer.width + padding, padding, WHITENESS);

    if(StretchDIBits(render_buffer.device_context,
                     padding, padding, render_buffer.width, render_buffer.height,
                     0, 0, render_buffer.width, render_buffer.height,
                     render_buffer.base, &render_buffer.bitmap_info, DIB_RGB_COLORS, SRCCOPY))
    {
    }
    else{
        OutputDebugStringA("StrechDIBits failed\n");
    }
}


LRESULT win_message_handler_callback(HWND window, UINT message, WPARAM w_param, LPARAM l_param){
    LRESULT result = 0;
    switch(message){
        //case WM_PAINT:{
        //    PAINTSTRUCT paint;
        //    BeginPaint(window, &paint);
        //    update_window(window, render_buffer);
        //    EndPaint(window, &paint);
        //} break;
        case WM_CLOSE:
        case WM_QUIT:
        case WM_DESTROY:{
            OutputDebugStringA("quiting\n");
            global_running = false;
        } break;
        case WM_MOUSEMOVE: {
            controller.mouse_pos.x = (s32)(l_param & 0xFFFF);
            controller.mouse_pos.y = SCREEN_HEIGHT - (s32)(l_param >> 16);
        } break;
        case WM_LBUTTONUP:{ controller.m_left.held = false; } break;
        case WM_RBUTTONUP:{ controller.m_right.held = false; } break;
        case WM_MBUTTONUP:{ controller.m_middle.held = false; } break;
        case WM_LBUTTONDOWN:{ controller.m_left = {true, true}; } break;
        case WM_RBUTTONDOWN:{ controller.m_right = {true, true}; } break;
        case WM_MBUTTONDOWN:{ controller.m_middle = {true, true}; } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:{
            bool was_down = ((l_param & (1 << 30)) != 0);
            bool is_down = ((l_param & (1 << 31)) == 0);
            if(is_down != was_down){
                switch(w_param){
                    case 'E':{
                        controller.e.held = is_down;
                        controller.e.pressed = is_down;
                    } break;
                    case 'Q':{
                        controller.q.held = is_down;
                        controller.q.pressed = is_down;
                    } break;
                    case 'W':{
                        controller.up.held = is_down;
                        controller.up.pressed = is_down;
                    } break;
                    case 'S':{
                        controller.down.held = is_down;
                        controller.down.pressed = is_down;
                    } break;
                    case 'A':{
                        controller.left.held = is_down;
                        controller.left.pressed = is_down;
                    } break;
                    case 'D':{
                        controller.right.held = is_down;
                        controller.right.pressed = is_down;
                    } break;
                    case '1':{
                        controller.one.held = is_down;
                        controller.one.pressed = is_down;
                    } break;
                    case '2':{
                        controller.two.held = is_down;
                        controller.two.pressed = is_down;
                    } break;
                    case '3':{
                        controller.three.held = is_down;
                        controller.three.pressed = is_down;
                    } break;
                    case '4':{
                        controller.four.held = is_down;
                        controller.four.pressed = is_down;
                    } break;
                    case VK_ESCAPE:{
                        OutputDebugStringA("quiting\n");
                        global_running = false;
                    } break;
                }
            }
        } break;
        default:{
            result = DefWindowProcW(window, message, w_param, l_param);
        } break;
    }
    return(result);
}

static HWND
win32_create_window(String8 window_name, u32 width, u32 height){
    WNDCLASSW window_class = {
        .style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC,
        .lpfnWndProc = win_message_handler_callback,
        .hInstance = GetModuleHandle(0),
        .hIcon = LoadIcon(0, IDI_APPLICATION),
        .hCursor = LoadCursor(0, IDC_ARROW),
        .lpszClassName = L"window class",
    };

    HWND window = ZERO_INIT;
    if(RegisterClassW(&window_class)){
        ScratchArena scratch = begin_scratch(0);
        String16 window_name_16 = os_utf8_utf16(scratch.arena, window_name);
        window = CreateWindowW(window_class.lpszClassName, (wchar*)window_name_16.str, WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, window_class.hInstance, 0);
        end_scratch(scratch);
    }

    return(window);
}

// NOTE: DX11 global inits
global ID3D11Device*           d3d_device         = 0;
global ID3D11DeviceContext*    d3d_device_context = 0;
global IDXGISwapChain*         swap_chain         = 0;
global ID3D11RenderTargetView* backbuffer         = 0;
global ID3D11DepthStencilView* depthbuffer        = 0;
global ID3D11VertexShader* vertex_shader          = 0;
global ID3D11PixelShader* pixel_shader            = 0;
global ID3D11Buffer* vertex_buffer                = 0;
global ID3D11Buffer* constant_buffer              = 0;
global ID3D11Buffer* index_buffer                 = 0;
global ID3D11InputLayout* input_layout            = 0;
global ID3D11ShaderResourceView* texture_view     = 0;
global ID3D11RasterizerState* rasterizer_state    = 0;
global ID3D11SamplerState* sampler_state          = 0;
global ID3D11BlendState* blend_state              = 0;
// QUESTION: Devon. Why cant I use the dot operator on a struct globally

static HRESULT
d3d_create_device_and_swap_chain(HWND window){
    HRESULT h_result = ZERO_INIT;

    DXGI_SWAP_CHAIN_DESC swap_chain_desc = {0};
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // NOTE: 8btis per color channel, 8 bits for alpha.
    swap_chain_desc.BufferDesc.Width  = SCREEN_WIDTH;
    swap_chain_desc.BufferDesc.Height = SCREEN_HEIGHT;
    swap_chain_desc.SampleDesc.Count  = 1; // NOTE: Multisampling. Anti aliasing sample count (up to 4 guaranteed).
    swap_chain_desc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT; // NOTE: How we want to use the back buffer. In this case we want to draw to it.
    swap_chain_desc.BufferCount       = 1; // NOTE: Number of back buffers to use in our swap chain.
    swap_chain_desc.OutputWindow      = window; // NOTE: Which window to draw to.
    swap_chain_desc.Windowed          = true;
    swap_chain_desc.Flags             = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

#if DEBUG
    u32 flags = D3D11_CREATE_DEVICE_DEBUG;
#else
    u32 flags = 0;
#endif
    h_result = D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, flags, 0, 0, D3D11_SDK_VERSION, &swap_chain_desc, &swap_chain, &d3d_device, 0, &d3d_device_context);
    return(h_result);
}

static HRESULT
d3d_create_render_target(){
    HRESULT result = ZERO_INIT;

    ID3D11Texture2D* backbuffer_address;
    result = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer_address); // NOTE: Get the backbuffer address from the swap_chain
    if(FAILED(result)){ return(result); }

    result = d3d_device->CreateRenderTargetView(backbuffer_address, 0, &backbuffer); // NOTE: Create the backbuffer
    if(FAILED(result)){ return(result); }

    backbuffer_address->Release();
    d3d_device_context->OMSetRenderTargets(1, &backbuffer, depthbuffer); // NOTE: Sets the render target (count, targets, advanced)
    return(result);
}

static void
d3d_clean(){
    backbuffer->Release();
    depthbuffer->Release();
    swap_chain->Release();
    swap_chain->SetFullscreenState(FALSE, NULL);
    d3d_device->Release();
    d3d_device_context->Release();
    vertex_shader->Release();
    pixel_shader->Release();
    vertex_buffer->Release();
}

static bool
succeed_compile_shader(HRESULT value, ID3DBlob* error){
    bool result = false;
    if(FAILED(value)){
        if(error){
            print("SHADER ERROR: %s\n", (char*)error->GetBufferPointer());
            error->Release();
        }
    }
    else{
        result = true;
    }
    return(result);
}

static HRESULT
d3d_init_shaders(){
    HRESULT result = ZERO_INIT;

#if DEBUG
    u32 shader_compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    u32 shader_compile_flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    ID3DBlob* vs_blob, * ps_blob, * error = 0;
    result = D3DCompileFromFile(L"code\\shaders.hlsl", 0, 0, "vs_main", "vs_5_0", shader_compile_flags, 0, &vs_blob, &error);
    if(FAILED(result)) { return(result); }

    result = D3DCompileFromFile(L"code\\shaders.hlsl", 0, 0, "ps_main", "ps_5_0", shader_compile_flags, 0, &ps_blob, &error);
    if(FAILED(result)) { return(result); }

    result = d3d_device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), 0, &vertex_shader);
    if(FAILED(result)) { return(result); }

    result = d3d_device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), 0, &pixel_shader);
    if(FAILED(result)) { return(result); }

    u32 rgba = sizeof(RGBA);
    u32 v = sizeof(v3);
    D3D11_INPUT_ELEMENT_DESC input_element_desc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
      //{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(v3), D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(v3), D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(v3) + sizeof(v3), D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    result = d3d_device->CreateInputLayout(input_element_desc, ArrayCount(input_element_desc), vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &input_layout);
    if(FAILED(result)) { return(result); }

    d3d_device_context->IASetInputLayout(input_layout);

    return(result);
}

static void
d3d_init_viewport(){
    D3D11_VIEWPORT viewport = {0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 1.0f};
    d3d_device_context->RSSetViewports(1, &viewport); // NOTE: sets the viewport to use (count, viewports)
}

static HRESULT
d3d_create_depthbuffer(){
    HRESULT result = ZERO_INIT;

    D3D11_TEXTURE2D_DESC texture_desc = {0};
    texture_desc.Width = SCREEN_WIDTH;
    texture_desc.Height = SCREEN_HEIGHT;
    texture_desc.ArraySize = 1;
    texture_desc.MipLevels = 1;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Format = DXGI_FORMAT_D32_FLOAT;
    texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depthbuffer_texture;
    result = d3d_device->CreateTexture2D(&texture_desc, 0, &depthbuffer_texture);
    if(FAILED(result)) { return(result); }

    D3D11_DEPTH_STENCIL_VIEW_DESC depthbuffer_desc;
    memset(&depthbuffer_desc, 0, sizeof(depthbuffer_desc));
    depthbuffer_desc.Format = DXGI_FORMAT_D32_FLOAT;
    depthbuffer_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthbuffer_desc.Flags = 0;
    result = d3d_device->CreateDepthStencilView(depthbuffer_texture, &depthbuffer_desc, &depthbuffer);
    depthbuffer_texture->Release();
    if(FAILED(result)) { return(result); }

    return(result);
}


#include "game.h"
s32 WinMain(HINSTANCE instance, HINSTANCE pinstance, LPSTR command_line, s32 window_type){
    // NOTE Init win32
    HWND window = win32_create_window(str8_literal("DirectX"), SCREEN_WIDTH, SCREEN_HEIGHT);
    ASSERT(IsWindow(window));

    init_memory(&memory);
    init_clock(&clock);
    init_render_buffer(&render_buffer);

    // NOTE: init global arena
    global_arena = os_allocate_arena(MB(5));

    // NOTE: Create device, device_context, swap_chain
    HRESULT result = d3d_create_device_and_swap_chain(window);
    AssertHr(result);

    // NOTE: Init depthbuffer
    result = d3d_create_depthbuffer();
    assert_hr(result);

    // NOTE: Init backbuffer
    result = d3d_create_render_target();
    assert_hr(result);

    // NOTE: Init viewport
    d3d_init_viewport();

    // NOTE: Init render pipeline (shaders/viewport)
    result = d3d_init_shaders();
    assert_hr(result);

    // NOTE: rasterizer state
    D3D11_RASTERIZER_DESC rasterizer_desc = {
        .FillMode = D3D11_FILL_SOLID,
		.CullMode = D3D11_CULL_NONE,
    };
    d3d_device->CreateRasterizerState(&rasterizer_desc, &rasterizer_state);

    D3D11_SAMPLER_DESC sampler_desc = {
        .Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        .AddressU = D3D11_TEXTURE_ADDRESS_BORDER,
        .AddressV = D3D11_TEXTURE_ADDRESS_BORDER,
        .AddressW = D3D11_TEXTURE_ADDRESS_BORDER,
        .MaxAnisotropy = 16,
        .BorderColor[0] = 1.0f,
        .BorderColor[1] = 1.0f,
        .BorderColor[2] = 1.0f,
        .BorderColor[3] = 1.0f,
    };
    d3d_device->CreateSamplerState(&sampler_desc, &sampler_state);
    d3d_device_context->PSSetSamplers(0, 1, &sampler_state);

    D3D11_BLEND_DESC blend_desc = {
        .AlphaToCoverageEnable = true,
        .IndependentBlendEnable = false,
        .RenderTarget[0].BlendEnable = true,
        .RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD,
        .RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_ALPHA,
        .RenderTarget[0].DestBlend = D3D11_BLEND_INV_DEST_ALPHA,
    };
    d3d_device->CreateBlendState(&blend_desc, &blend_state);
    d3d_device_context->OMSetBlendState(blend_state, 0, 0xAFFFFFFF);

    // NOTE: texture loading
    String8 dir = str8_literal("data\\");
    String8 filename = str8_literal("image.bmp");
    Bitmap image = load_bitmap(global_arena, dir, filename);

    D3D11_TEXTURE2D_DESC texture_desc = {
        .Width = image.width,
        .Height = image.height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
        .SampleDesc = {1, 0},
        .Usage = D3D11_USAGE_IMMUTABLE,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
    };

    D3D11_SUBRESOURCE_DATA data = {
        .pSysMem = image.pixels,
        .SysMemPitch = image.stride,
    };

    ID3D11Texture2D* texture;
    HRESULT fr = d3d_device->CreateTexture2D(&texture_desc, &data, &texture);
    fr = d3d_device->CreateShaderResourceView((ID3D11Resource*)texture, 0, &texture_view);
    d3d_device_context->PSSetShaderResources(0, 1, &texture_view);

    typedef struct Vertex{
        v3 position;
        //RGBA color;
        v3 normal;
        v2 uv;
    } Vertex;

    u32 vertex_stride;
    u32 vertex_offset;
    u32 vertex_count;
    u32 index_count;

    Vertex vertex_data[] = {
        {{-20.0f, -20.0f, 20.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{20.0f, -20.0f, 20.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-20.0f, 20.0f, 20.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{20.0f, 20.0f, 20.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},

        {{-20.0f, -20.0f, -20.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{-20.0f, 20.0f, -20.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        {{20.0f, -20.0f, -20.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{20.0f, 20.0f, -20.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},

        {{-20.0f, 20.0f, -20.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-20.0f, 20.0f, 20.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{20.0f, 20.0f, -20.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{20.0f, 20.0f, 20.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},

        {{-20.0f, -20.0f, -20.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{20.0f, -20.0f, -20.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{-20.0f, -20.0f, 20.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        {{20.0f, -20.0f, 20.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},

        {{20.0f, -20.0f, -20.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{20.0f, 20.0f, -20.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{20.0f, -20.0f, 20.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{20.0f, 20.0f, 20.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},

        {{-20.0f, -20.0f, -20.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-20.0f, -20.0f, 20.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{-20.0f, 20.0f, -20.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{-20.0f, 20.0f, 20.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    };
    //Vertex vertex_data[] = {
    //    {{-0.5f,  0.5f, -0.5f}, {1.0, 0.5, 0.2, 1.0}},
    //    {{ 0.5f,  0.5f, -0.5f}, {1.0, 0.5, 0.2, 1.0}},
    //    {{-0.5f, -0.5f, -0.5f}, {1.0, 0.5, 0.2, 1.0}},
    //    {{ 0.5f, -0.5f, -0.5f}, {1.0, 0.5, 0.2, 1.0}},
    //    {{-0.5f,  0.5f,  0.5f}, {1.0, 0.5, 0.2, 1.0}},
    //    {{ 0.5f,  0.5f,  0.5f}, {1.0, 0.5, 0.2, 1.0}},
    //    {{-0.5f, -0.5f,  0.5f}, {1.0, 0.5, 0.2, 1.0}},
    //    {{ 0.5f, -0.5f,  0.5f}, {1.0, 0.5, 0.2, 1.0}},

    //    {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    //    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
    //    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
    //    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f, 1.0f}},
    //    {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 1.0f, 1.0f}},
    //    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f, 1.0f}},
    //    {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.0f, 1.0f}},
    //    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    //};
    //Vertex vertex_data[] = {
    //    {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
    //    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
    //    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    //    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f, 1.0f}},

    //    {{ 0.0f,  0.5f,  0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
    //};
    //pos - normals - texture coords - color? - weight values?
    vertex_stride = sizeof(Vertex);
    vertex_offset = 0;
    vertex_count = ArrayCount(vertex_data);

    D3D11_BUFFER_DESC vertex_buffer_desc = {0};
    vertex_buffer_desc.ByteWidth = sizeof(Vertex) * ArrayCount(vertex_data);
    vertex_buffer_desc.Usage     = D3D11_USAGE_DEFAULT;
    vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertex_sd  = {0};
    vertex_sd.pSysMem                 = vertex_data;
    HRESULT function_result = d3d_device->CreateBuffer(&vertex_buffer_desc, &vertex_sd, &vertex_buffer);
    assert(SUCCEEDED(function_result));

    u32 indices[] = {
        0, 1, 2,    // side 1
        2, 1, 3,
        4, 5, 6,    // side 2
        6, 5, 7,
        8, 9, 10,    // side 3
        10, 9, 11,
        12, 13, 14,    // side 4
        14, 13, 15,
        16, 17, 18,    // side 5
        18, 17, 19,
        20, 21, 22,    // side 6
        22, 21, 23,
    };
    //u32 indices[] = {
    //    1, 2, 0,    // side 1
    //    2, 1, 3,
    //    0, 6, 4,    // side 2
    //    6, 0, 2,
    //    7, 4, 6,    // side 3
    //    4, 7, 5,
    //    3, 5, 7,    // side 4
    //    5, 3, 1,
    //    5, 0, 4,    // side 5
    //    0, 5, 1,
    //    6, 3, 7,    // side 6
    //    3, 6, 2,
    //};
    //u32 indices[] = {
    //    0, 2, 1,    // base
    //    1, 2, 3,
    //    0, 1, 4,    // sides
    //    1, 3, 4,
    //    3, 2, 4,
    //    2, 0, 4,
    //};
    index_count = ArrayCount(indices);

    D3D11_BUFFER_DESC index_buffer_desc = {0};
    index_buffer_desc.ByteWidth = sizeof(u32) * index_count;
    index_buffer_desc.Usage     = D3D11_USAGE_DEFAULT;
    index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA index_sd  = {0};
    index_sd.pSysMem                 = indices;
    function_result = d3d_device->CreateBuffer(&index_buffer_desc, &index_sd, &index_buffer);
    assert(SUCCEEDED(function_result));

    typedef struct ConstantBuffer{
        DirectX::XMMATRIX final_matrix;
        DirectX::XMMATRIX rotation_matrix;
        v4 light_direction;
        v4 light_color;
        v4 ambient_color;
    } ConstantBuffer;
    ConstantBuffer c_buffer;
    c_buffer.light_direction = {-1.0f, -1.0f, -1.0f, 0.0f};
    c_buffer.light_color = {0.5f, 0.5f, 0.5f, 1.0f};
    c_buffer.ambient_color = {0.2f, 0.2f, 0.2f, 1.0f};

    D3D11_BUFFER_DESC const_buffer_desc = {0};
    const_buffer_desc.ByteWidth = sizeof(ConstantBuffer);
    const_buffer_desc.Usage     = D3D11_USAGE_DEFAULT;
    const_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    function_result = d3d_device->CreateBuffer(&const_buffer_desc, 0, &constant_buffer);
    assert(SUCCEEDED(function_result));
    d3d_device_context->VSSetConstantBuffers(0, 1, &constant_buffer);

    global_running = true;

    f64 FPS = 0;
    f64 MSPF = 0;
    u64 frame_count = 0;

    clock.dt =  1.0/60.0;
    f64 accumulator = 0.0;
    s64 start_ticks = clock.get_ticks();
    f64 second_marker = clock.get_ticks();

    float rotation = 0.0f;

    render_buffer.device_context = GetDC(window);
    f32 x = 0.0f;
    f32 y = 0.2f;
    f32 z = 5.0f;

    while(global_running){
        MSG message;
        while(PeekMessageW(&message, window, 0, 0, PM_REMOVE)){
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        s64 end_ticks = clock.get_ticks();
        f64 frame_time = clock.get_seconds_elapsed(start_ticks, end_ticks);
        MSPF = 1000/1000/((f64)clock.frequency / (f64)(end_ticks - start_ticks));
        start_ticks = end_ticks;

        if(controller.left.held){
            x += 10.0f * clock.dt;
        }
        if(controller.right.held){
            x -= 10.0f * clock.dt;
        }
        if(controller.up.held){
            z -= 10.0f * clock.dt;
        }
        if(controller.down.held){
            z += 10.0f * clock.dt;
        }
        if(controller.e.held){
            y += 10.0f * clock.dt;
        }
        if(controller.q.held){
            y -= 10.0f * clock.dt;
        }

        accumulator += frame_time;
        u32 simulations = 0;
        while(accumulator >= clock.dt){
            update_game(&memory, &render_buffer, &controller, &clock);
            accumulator -= clock.dt;
            //TODO: put in a function
            controller.up.pressed = false;
            controller.down.pressed = false;
            controller.left.pressed = false;
            controller.right.pressed = false;
            controller.one.pressed = false;
            controller.two.pressed = false;
            controller.three.pressed = false;
            controller.four.pressed = false;
            controller.m_right.pressed = false;
            controller.m_left.pressed = false;
            controller.m_middle.pressed = false;
            simulations++;
        }

        frame_count++;
        f64 time_elapsed = clock.get_seconds_elapsed(second_marker, clock.get_ticks());
        if(time_elapsed > 1){
            FPS = (frame_count / time_elapsed);
            second_marker = clock.get_ticks();
            frame_count = 0;
        }
        print("FPS: %f - MSPF: %f - time_dt: %f\n", FPS, MSPF, clock.dt);

        //draw_commands(&render_buffer, render_buffer.render_command_arena);
        //update_window(window, render_buffer);
        //


        using namespace DirectX;
        XMMATRIX view_matrix = XMMatrixLookAtLH((FXMVECTOR){x, y, z}, (FXMVECTOR){0, 0, 0}, (FXMVECTOR){0, 1, 0});
        XMMATRIX projection_matrix = XMMatrixPerspectiveFovLH(PI_f32*0.5f, (f32)((f32)SCREEN_WIDTH/(f32)SCREEN_HEIGHT), 1.0f, 100.0f);

        //rotation += clock.dt;
        //XMMATRIX rotation_x = XMMatrixRotationX(rotation/8);
        //XMMATRIX rotation_y = XMMatrixRotationY(rotation/8);
        //XMMATRIX rotation_z = XMMatrixRotationZ(rotation/30);
        //c_buffer.rotation_matrix = rotation_y * rotation_x;
        c_buffer.final_matrix = view_matrix * projection_matrix;

        float background_color[4] = {0.2f, 0.29f, 0.29f, 1.0f};
        d3d_device_context->ClearRenderTargetView(backbuffer, background_color);
        d3d_device_context->ClearDepthStencilView(depthbuffer, D3D11_CLEAR_DEPTH, 1.0f, 0);

        d3d_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        d3d_device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &vertex_stride, &vertex_offset);
        d3d_device_context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R32_UINT, 0);
        d3d_device_context->RSSetState(rasterizer_state);

        d3d_device_context->VSSetShader(vertex_shader, 0, 0);
        d3d_device_context->PSSetShader(pixel_shader, 0, 0);

        d3d_device_context->UpdateSubresource(constant_buffer, 0, 0, &c_buffer, 0, 0);
        d3d_device_context->DrawIndexed(index_count, 0, 0);

        swap_chain->Present(1, 0);

        if(simulations){
            //handle_debug_counters(simulations);
        }
    }
    // TODO: add to cleanup function D3D_cleanup()
    d3d_clean();
    ReleaseDC(window, render_buffer.device_context);
    return(0);
}
