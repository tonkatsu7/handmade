/* 
    $File: $
    $Date: $
    $Revision: $
    $Creator: Si Pham $
    $Notice: (C) Copyright 2021 by Si Pham, All Rights Reserved. $
 */

#include <windows.h>
#include <stdint.h>

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

// TODO: this is a global for now
global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackbuffer;

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
                                            RECT *WindowRect,
                                              int X,
                                              int Y,
                                              int Width,
                                              int Height)
{
    int WindowWidth = WindowRect->right - WindowRect->left;
    int WindowHeight = WindowRect->bottom - WindowRect->top;

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
            RECT ClientRect;
            GetClientRect(hWnd, &ClientRect);
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(&GlobalBackbuffer, Width, Height);
            OutputDebugStringA("WM_SIZE\n");
        } break;

        case WM_CLOSE:
        {
            Running = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY:
        {
            Running = false;
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
            RECT ClientRect;
            GetClientRect(hWnd, &ClientRect);
            Win32DisplayBufferInWindow(GlobalBackbuffer, DeviceContext, &ClientRect, X, Y, Width, Height);
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
            Running = true;
            int XOffset = 0;
            int YOffset = 0;
            
            while (Running)
            {    
                MSG Message;

                while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        Running = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                RenderWeirdGradient(GlobalBackbuffer, XOffset, YOffset);

                HDC DeviceContext = GetDC(Window);
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);
                int WindowWidth = ClientRect.right - ClientRect.left;
                int WindowHeight = ClientRect.bottom - ClientRect.top;
                Win32DisplayBufferInWindow(GlobalBackbuffer, DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
                ReleaseDC(Window, DeviceContext);

                ++XOffset;
                ++YOffset;
            }
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