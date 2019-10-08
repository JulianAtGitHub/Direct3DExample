
#include "pch.h"
#include "DXRExample.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    DXRExample example;
    Utils::Application app(1440, 900, &example);
    return app.Run(hInstance, nCmdShow);
}
