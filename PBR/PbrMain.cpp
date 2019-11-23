
#include "stdafx.h"
#include "PbrExample.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    PbrExample example;
    Utils::Application app(1440, 810, &example);
    return app.Run(hInstance, nCmdShow);
}