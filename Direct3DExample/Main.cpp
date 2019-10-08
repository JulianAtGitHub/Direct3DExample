
#include "pch.h"
#include "Application.h"
#include "DXRExample.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    DXRExample example;
    Application app(1440, 900, &example);
    app.Run(hInstance, nCmdShow);
}
