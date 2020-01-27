# DirectX Examples
## Raytracing
A DXR path tracing example with a simple framework to learn and understand raytracing concept.

- Implement Monte Carlo pathtracing
- Native DirectX Raytracing
- Integrate With Dear Imgui

![Result of display](https://github.com/JulianAtGitHub/Direct3DExample/blob/master/screenshot.jpg)

**Requirements**
- Windows 10 October 2018 update (RS5 | version 1809) or newer.
- Windows 10 SDK version 10.0.17763.0 or newer.
- DXR supported GPU: NVIDIA Turing or Volta GPU or newer.
- Visual Studio 2017 v15.8.6+.

## PBR
A Raster example of PBR implementation on D3D12.

- Implement based on [SIGGRAPH 2012 Course: Practical Physically Based Shading in Film and Game Production](https://blog.selfshadow.com/publications/s2012-shading-course/#course_content)
- Using Direct3D 12
- Integrate With Dear Imgui

![Result of display](https://github.com/JulianAtGitHub/Direct3DExample/blob/master/pbrshot.jpg)

## Building project
1. Install python
2. Clone the project
3. Update sub resources
> 
    cd path/to/root/folder
    python UpdateExternals.py
4. Open Direct3DExample.sln and select an example project as start up project.
5. Set work directory to $(OutDir)
6. Build and run.

