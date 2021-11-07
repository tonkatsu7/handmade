/* 
    $File: $
    $Date: $
    $Revision: $
    $Creator: Si Pham $
    $Notice: (C) Copyright 2021 by Si Pham, All Rights Reserved. $
    */

#include <windows.h>

#define internal static 
#define local_persist static
#define global_variable static

// TODO: this is a global for now
global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;  
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;

internal void 
Win32ResizeDIBSection(int Width,
                      int Height)
{
    if (BitmapHandle)
    {
        DeleteObject(BitmapHandle);
    }
    
    if (!BitmapDeviceContext)
    {
        BitmapDeviceContext = CreateCompatibleDC(0);
    }
    /* 
        typedef struct tagBITMAPINFO {
                                      BITMAPINFOHEADER bmiHeader;
                                      RGBQUAD          bmiColors[1];
                                      } BITMAPINFO, *LPBITMAPINFO, *PBITMAPINFO;
        */

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

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = Height; 
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32; // DWROD aligned 32 instead of 24
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    BitmapHandle = CreateDIBSection(BitmapDeviceContext,
                                    &BitmapInfo,
                                    DIB_RGB_COLORS,
                                    &BitmapMemory,
                                    0,
                                    0);
    // ReleaseDC(0, DeviceContext);
}

internal void 
Win32UpdateWindow(HDC DeviceContext,
                  int X,
                  int Y,
                  int Width,
                  int Height)
{
    StretchDIBits(DeviceContext,
                  X, Y, Width, Height,
                  X, Y, Width, Height,
                  BitmapMemory,
                  &BitmapInfo,
                  DIB_RGB_COLORS,
                  SRCCOPY);

    // local_persist DWORD Operation = WHITENESS;

    // PatBlt(DeviceContext,
    //         X,
    //         Y,
    //         Width,
    //         Height,
    //         Operation
    //         );
    // if (Operation == WHITENESS)
    // {
    //     Operation = BLACKNESS;
    // }
    // else
    // {
    //     Operation = WHITENESS;
    // }
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
            Win32ResizeDIBSection(Width, Height);
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
            Win32UpdateWindow(DeviceContext, X, Y, Width, Height);
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
    // MessageBoxA(
    //     0,
    //     "This is Handmade Hero",
    //     "Handmade Hero",
    //     MB_OK | MB_ICONINFORMATION
    // );

    WNDCLASS WindowClass = {};

    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = instance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND WindowHandle = CreateWindowEx(0,
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
        if (WindowHandle)
        {
            Running = true;
            while (Running)
            {
                MSG Message;
                BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
                if (MessageResult > 0)
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                else
                {
                    break;
                }
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