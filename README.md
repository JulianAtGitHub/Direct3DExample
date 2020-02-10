# DirectX Examples

- Native Direct3D 12
- Native DirectX Raytracing
- Integrate With Assimp
- Integrate With Dear Imgui
- GLTF support

## PBR
A Raster example of PBR implementation on D3D12.
- Implement based on [SIGGRAPH 2012 Course: Practical Physically Based Shading in Film and Game Production](https://blog.selfshadow.com/publications/s2012-shading-course/#course_content)
![Result of display](https://github.com/JulianAtGitHub/Direct3DExample/blob/master/pbrshot.jpg)

## Raytracing
A DXR path tracing example with a simple framework to learn and understand raytracing concept.
- Implement Monte Carlo pathtracing
![Result of display](https://github.com/JulianAtGitHub/Direct3DExample/blob/master/screenshot.jpg)

**Requirements**
- Windows 10 October 2018 update (RS5 | version 1809) or newer.
- Windows 10 SDK version 10.0.17763.0 or newer.
- DXR supported GPU: NVIDIA Turing or Volta GPU or newer.
- Visual Studio 2017 v15.8.6+.

## Building project
1. Install CMake
2. Install python
3. Clone the project
4. Update sub resources
> 
    cd path/to/root/folder
    python UpdateExternals.py
5. Open Direct3DExample.sln and select an example project as start up project.
6. Set work directory to $(OutDir)
7. Build and run.

