#include "pch.h"
#include "DXRExample.h"
#include "Core/Render/RenderCore.h"
#include "Core/Render/GPUBuffer.h"
#include "Core/Render/CommandContext.h"


const static XMFLOAT3 vertices[] = {
    XMFLOAT3(0,          1,  0),
    XMFLOAT3(0.866f,  -0.5f, 0),
    XMFLOAT3(-0.866f, -0.5f, 0),
};

DXRExample::DXRExample(HWND hwnd)
: Example(hwnd)
, mVertexBuffer(nullptr)
{

}

DXRExample::~DXRExample(void) {

}

void DXRExample::Init(void) {
    Render::Initialize(mHwnd);

    InitAccelerationStructures();
}

void DXRExample::Update(void) {

}

void DXRExample::Render(void) {

}

void DXRExample::Destroy(void) {
    DeleteAndSetNull(mVertexBuffer);
    Render::Terminate();
}

void DXRExample::InitAccelerationStructures(void) {
    //uint32_t bufferSize = sizeof(XMFLOAT3) * _countof(vertices);
    //mVertexBuffer = new Render::GPUBuffer(AlignUp(bufferSize, 256));

    //Render::gCommand->Begin();
    //Render::gCommand->UploadBuffer(mVertexBuffer, 0, vertices, bufferSize);

    //// Bottom level acceleration structure
    //{
    //    D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
    //    geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

    //}

}

