
#include "pch.h"
#include "Application.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    Application app(1280, 720, "Direct3D Example");
    app.Run(hInstance, nCmdShow);
}
