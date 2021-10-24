/* 
    $File: $
    $Date: $
    $Revision: $
    $Creator: Si Pham $
    $Notice: (C) Copyright 2021 by Si Pham, All Rights Reserved. $
    */

#include <windows.h>

int CALLBACK
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
    MessageBoxA(
        0,
        "This is Handmade Hero",
        "Handmade Hero",
        MB_OK | MB_ICONINFORMATION
    );
    return(0);
}