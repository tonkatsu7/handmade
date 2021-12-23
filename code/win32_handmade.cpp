/* 
    $File: $
    $Date: $
    $Revision: $
    $Creator: Si Pham $
    $Notice: (C) Copyright 2021 by Si Pham, All Rights Reserved. $
 */

#include <windows.h>
#include <stdint.h>
#include <xinput.h>

#define internal        static 
#define local_persist   static
#define global_variable static

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
 
struct win32_offscreen_buffer
{
    BITMAPINFO  Info;  
    void       *Memory;
    int         Width;
    int         Height;
    int         Pitch;
    int         BytesPerPixel; // = 4;
};

struct win32_window_dimension  
{
    int Width;
    int Height;
};

// TODO: this is a global for now
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(0);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define  XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(0);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void
Win32LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}

internal void
RenderWeirdGradient(win32_offscreen_buffer Buffer, 
                                       int BlueOffset, 
                                       int GreenOffset)
{
    int Width = Buffer.Width;
    int Height = Buffer.Height;

    uint8 *Row = (uint8 *)Buffer.Memory; 

    for (int Y = 0; Y < Buffer.Height; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;

        for (int X = 0; X < Buffer.Width; ++X)
        {
            /*
                Memory:     BB GG RR xx
                Register:   xx RR GG BB
                Shift left  24 16  8  0
             */
            uint8 Blue = (X + BlueOffset);
            uint8 Green = (Y + GreenOffset);
            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Buffer.Pitch;
    }
}

internal void 
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer,
                                          int Width,
                                          int Height)
{
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory,
                    0,
                    MEM_RELEASE);
    }

    Buffer->BytesPerPixel = 4;
    Buffer->Width = Width;
    Buffer->Height = Height;

    /* 
        typedef struct tagBITMAPINFOHEADER {
                                            DWORD biSize;
                                            LONG  biWidth;
                                            LONG  biHeight;
                                            WORD  biPlanes;
                                            WORD  biBitCount;
                                            DWORD biCompression;
                                            DWORD biSizeImage;
                                            LONG  biXPelsPerMeter;
                                            LONG  biYPelsPerMeter;
                                            DWORD biClrUsed;
                                            DWORD biClrImportant;
                                            } BITMAPINFOHEADER, *PBITMAPINFOHEADER;
        */

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height; // top down 
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32; // DWROD aligned 32 instead of 24
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*Buffer->BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0,
                                  BitmapMemorySize,
                                  MEM_COMMIT,
                                  PAGE_READWRITE);

    Buffer->Pitch = Width*Buffer->BytesPerPixel;
}

internal void 
Win32DisplayBufferInWindow(win32_offscreen_buffer Buffer,
                                              HDC DeviceContext,
                                              int WindowWidth,
                                              int WindowHeight,
                                              int X,
                                              int Y,
                                              int Width,
                                              int Height)
{
    StretchDIBits(DeviceContext,
                  /*
                  X, Y, Width, Height,
                  X, Y, Width, Height,
                  */
                  X, Y, Buffer.Width, Buffer.Height,
                  X, Y, WindowWidth, WindowHeight,
                  Buffer.Memory,
                  &Buffer.Info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}

LRESULT CALLBACK 
Win32MainWindowCallback(HWND   hWnd,
                        UINT   uMsg,
                        WPARAM wParam,
                        LPARAM lParam) // WindowProc
{
    LRESULT result = 0;

    switch (uMsg)
    {
        case WM_SIZE:
        {
            win32_window_dimension Dimension = Win32GetWindowDimension(hWnd);
            Win32ResizeDIBSection(&GlobalBackbuffer, Dimension.Width, Dimension.Height);
            OutputDebugStringA("WM_SIZE\n");
        } break;

        case WM_CLOSE:
        {
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 VKCode = wParam;
            bool WasDown = ((lParam & (1 << 30)) != 0);
            bool IsDown = ((lParam & (1 << 31)) == 0);

            if (VKCode == 'W')
            {
                OutputDebugStringA("W\n");
            }
            else if (VKCode == 'A')
            {
                OutputDebugStringA("A\n");
            }
            else if (VKCode == 'S')
            {
                OutputDebugStringA("S\n");
            }
            else if (VKCode == 'D')
            {
                OutputDebugStringA("D\n");
            }
            else if (VKCode == 'Q')
            {
                OutputDebugStringA("Q\n");
            }
            else if (VKCode == 'E')
            {
                OutputDebugStringA("E\n");
            }
            else if (VKCode == VK_UP)
            {
                OutputDebugStringA("UP\n");
            }
            else if (VKCode == VK_DOWN)
            {
                OutputDebugStringA("DOWN\n");
            }
            else if (VKCode == VK_LEFT)
            {
                OutputDebugStringA("LEFT\n");
            }
            else if (VKCode == VK_RIGHT)
            {
                OutputDebugStringA("RIGHT\n");
            }
            else if (VKCode == VK_ESCAPE)
            {                
                OutputDebugStringA("ESC:");
                if (IsDown)
                {
                    OutputDebugStringA("IsDown ");
                }
                if (WasDown)
                {
                    OutputDebugStringA("WasDown ");
                }
                OutputDebugStringA("\n");
            }
            else if (VKCode == VK_SPACE)
            {
                OutputDebugStringA("SPACE\n");
            }
        } break;

        case WM_PAINT:
        {
            /*
                typedef struct tagPAINTSTRUCT {
                                               HDC  hdc;
                                               BOOL fErase;
                                               RECT rcPaint;
                                               BOOL fRestore;
                                               BOOL fIncUpdate;
                                               BYTE rgbReserved[32];
                                               } PAINTSTRUCT, *PPAINTSTRUCT, *NPPAINTSTRUCT, *LPPAINTSTRUCT;
            */
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(hWnd, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            win32_window_dimension Dimension = Win32GetWindowDimension(hWnd);
            
            Win32DisplayBufferInWindow(GlobalBackbuffer, 
                                       DeviceContext, 
                                       Dimension.Width, Dimension.Height, 
                                       X, Y, 
                                       Width, Height);
            EndPaint(hWnd, &Paint);
        } break;

        default:
        {
            OutputDebugStringA("default\n");
            result = DefWindowProcA(hWnd, uMsg, wParam, lParam);
        } break;
    }

    return(result);
}

int CALLBACK
WinMain(HINSTANCE instance,
        HINSTANCE prevInstance,
            LPSTR cmdLine,
              int cmdShow)
{
    Win32LoadXInput();

    WNDCLASS WindowClass = {};

    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = instance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND Window = CreateWindowEx(0,
                                     WindowClass.lpszClassName,
                                     "Handmade Hero",
                                     WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     0,
                                     0,
                                     instance,
                                     0);
        if (Window)
        {
            HDC DeviceContext = GetDC(Window);

            GlobalRunning = true;
            int XOffset = 0;
            int YOffset = 0;
            
            while (GlobalRunning)
            {    
                MSG Message;

                while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        GlobalRunning = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                for (DWORD ControllerIndex=0; 
                     ControllerIndex < XUSER_MAX_COUNT; 
                     ++ControllerIndex)
                {
                    XINPUT_STATE ControllerState;
                    if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                    {
                        // Contoller is plugged in

                        /*
                            typedef struct _XINPUT_STATE {
                                                            DWORD          dwPacketNumber;
                                                            XINPUT_GAMEPAD Gamepad;
                                                            } XINPUT_STATE, *PXINPUT_STATE;
                         */

                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                        /*
                            typedef struct _XINPUT_GAMEPAD {
                                                            WORD  wButtons;
                                                            BYTE  bLeftTrigger;
                                                            BYTE  bRightTrigger;
                                                            SHORT sThumbLX;
                                                            SHORT sThumbLY;
                                                            SHORT sThumbRX;
                                                            SHORT sThumbRY;
                                                            } XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;
                         */

                        bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool LeftSoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16 StickX = Pad->sThumbLX;
                        int16 StickY = Pad->sThumbLY;

                        if (AButton)
                        {
                            YOffset += 2;
                        }
                    }
                    else
                    {
                        // controller unavailable
                    }
                }

                // XINPUT_VIBRATION Vibration;
                // Vibration.wLeftMotorSpeed = 60000;
                // Vibration.wRightMotorSpeed = 60000;
                // XInputSetState(0, &Vibration);

                RenderWeirdGradient(GlobalBackbuffer, XOffset, YOffset);

                win32_window_dimension Dimension = Win32GetWindowDimension(Window);

                Win32DisplayBufferInWindow(GlobalBackbuffer, DeviceContext, 
                                           Dimension.Width, Dimension.Height, 
                                           0, 0, 
                                           Dimension.Width, Dimension.Height);

                ++XOffset;
                // ++YOffset;
            }

            ReleaseDC(Window, DeviceContext);
        }  
        else 
        {
            // TODO: logging
        }
    }
    else 
    {
        // TODO: logging
    }

    return(0);
}