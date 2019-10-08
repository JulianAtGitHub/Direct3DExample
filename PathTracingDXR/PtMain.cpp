
#include "stdafx.h"
#include "PtExample.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    PtExample example;
    Utils::Application app(1440, 810, &example);
    return app.Run(hInstance, nCmdShow);
}