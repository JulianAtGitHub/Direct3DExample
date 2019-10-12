#include "stdafx.h"
#include "RenderCore.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "CommandContext.h"
#include "DescriptorHeap.h"
#include "Sampler.h"
#include "Resource/PixelBuffer.h"
#include "Resource/ConstantBuffer.h"
#include "Resource/GPUBuffer.h"
#include "Resource/RenderTargetBuffer.h"
#include "GUI/imgui.h"
#include "GUILayer.h"

namespace Render {

GUILayer::GUILayer(HWND hwnd, uint32_t width, uint32_t height)
: mWidth(width)
, mHeight(height)
, mContext(nullptr)
, mFontTexture(nullptr)
, mSampler(nullptr)
, mResourceHeap(nullptr)
, mSamplerHeap(nullptr)
, mRootSignature(nullptr)
, mGraphicsState(nullptr)
, mConstBuffer(nullptr)
, mVertexBuffer(nullptr)
{
    Initialize(hwnd);
}

GUILayer::~GUILayer(void) {
    Destroy();
}

void GUILayer::Initialize(HWND hwnd) {
    mContext = ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.RenderDrawListsFn = nullptr;
    io.ImeWindowHandle = hwnd;
    io.KeyMap[ImGuiKey_Tab] = VK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
    io.KeyMap[ImGuiKey_Home] = VK_HOME;
    io.KeyMap[ImGuiKey_End] = VK_END;
    io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = 'A';
    io.KeyMap[ImGuiKey_C] = 'C';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';

    mRootSignature = new RootSignature(RootSignature::Graphics, 3, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    mRootSignature->SetDescriptor(0, D3D12_ROOT_PARAMETER_TYPE_CBV, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    mRootSignature->SetDescriptorTable(1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    mRootSignature->SetDescriptorTable(2, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    mRootSignature->Create();

    const char* vsSource = " \
        cbuffer vertexBuffer : register(b0) {\
            float4x4 ProjectionMatrix; \
        };\
        struct VS_INPUT {\
            float2 pos : POSITION;\
            float2 uv  : TEXCOORD;\
            float4 col : COLOR;\
        };\
        struct PS_INPUT {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR;\
            float2 uv  : TEXCOORD;\
        };\
        \
        PS_INPUT main(VS_INPUT input) {\
            PS_INPUT output;\
            output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
            output.col = input.col;\
            output.uv  = input.uv;\
            return output;\
        } \
        ";

    const char* psSource = " \
        struct PS_INPUT {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR;\
            float2 uv  : TEXCOORD;\
        };\
        SamplerState sampler0 : register(s0);\
        Texture2D texture0 : register(t0);\
        \
        float4 main(PS_INPUT input) : SV_Target {\
            float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
            return out_col; \
        } \
        ";

    WRL::ComPtr<ID3DBlob> vs;
    WRL::ComPtr<ID3DBlob> ps;
    WRL::ComPtr<ID3DBlob> error;
    ASSERT_SUCCEEDED(D3DCompile(vsSource, strlen(vsSource), NULL, NULL, NULL, "main", "vs_5_0", 0, 0, &vs, &error));
    ASSERT_SUCCEEDED(D3DCompile(psSource, strlen(psSource), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &ps, &error));

    D3D12_INPUT_ELEMENT_DESC inputElements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 8,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    mGraphicsState = new GraphicsState();
    mGraphicsState->GetInputLayout() = { inputElements, _countof(inputElements) };
    mGraphicsState->GetRasterizerState().CullMode = D3D12_CULL_MODE_NONE;
    D3D12_BLEND_DESC &blendDesc = mGraphicsState->GetBlendState();
    blendDesc.RenderTarget[0].BlendEnable = true;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    mGraphicsState->EnableDepth(false);
    mGraphicsState->CopyVertexShader(vs.Get());
    mGraphicsState->CopyPixelShader(ps.Get());
    mGraphicsState->Create(mRootSignature);

    mResourceHeap = new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    mSamplerHeap = new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);

    uint8_t *texturePixels = nullptr;
    int32_t textureWidth = 0;
    int32_t textureHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&texturePixels, &textureWidth, &textureHeight);

    mFontTexture = new PixelBuffer(textureWidth * BytesPerPixel(DXGI_FORMAT_R8G8B8A8_UNORM), textureWidth, textureHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
    gCommand->Begin();
    gCommand->UploadTexture(mFontTexture, texturePixels);
    gCommand->End(true);
    mFontTexture->CreateSRV(mResourceHeap->Allocate());
    io.Fonts->TexID = mFontTexture;

    mSampler = new Sampler();
    mSampler->Create(mSamplerHeap->Allocate());

    mConstBuffer = new ConstantBuffer(sizeof(XMFLOAT4X4), 1);
    mVertexBuffer = new GPUBuffer(VERTEX_BUFFER_SIZE, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, true);
}

void GUILayer::Destroy(void) {
    ImGui::DestroyContext(mContext);
    mContext = nullptr;

    DeleteAndSetNull(mConstBuffer);
    DeleteAndSetNull(mVertexBuffer);
    DeleteAndSetNull(mFontTexture);
    DeleteAndSetNull(mSampler);
    DeleteAndSetNull(mResourceHeap);
    DeleteAndSetNull(mSamplerHeap);
    DeleteAndSetNull(mRootSignature);
    DeleteAndSetNull(mGraphicsState);
}

void GUILayer::BeginFrame(float deltaTime) {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(mWidth), static_cast<float>(mHeight));
    io.DeltaTime = deltaTime;

    // Read keyboard modifiers inputs
    io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;

    ImGui::NewFrame();
}

void GUILayer::EndFrame(uint32_t currentFrame) {
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();

    // Update vertices and indices
    const size_t vertSize = sizeof(ImDrawVert) * drawData->TotalVtxCount;
    const size_t idxSize = sizeof(ImDrawIdx) * drawData->TotalIdxCount;
    const size_t vertSizeAligned = AlignUp(vertSize, 256);
    const size_t idxSizeAligned = AlignUp(idxSize, 256);
    if ((vertSizeAligned + idxSizeAligned) > VERTEX_BUFFER_SIZE) {
        DEBUG_PRINT("GUI vertex buffer size exceed 2M");
        return;
    }

    size_t vertStartAt = 0;
    size_t idxStartAt = vertSizeAligned;
    for (int32_t i = 0; i < drawData->CmdListsCount; ++i) {
        const ImDrawList* drawList = drawData->CmdLists[i];
        const size_t subVertSize = drawList->VtxBuffer.size() * sizeof(ImDrawVert);
        const size_t subIdxSize = drawList->IdxBuffer.size() * sizeof(ImDrawIdx);
        mVertexBuffer->UploadData(&drawList->VtxBuffer[0], subVertSize, vertStartAt);
        mVertexBuffer->UploadData(&drawList->IdxBuffer[0], subIdxSize, idxStartAt);
        vertStartAt += subVertSize;
        idxStartAt += subIdxSize;
    }

    mVertexView = mVertexBuffer->FillVertexBufferView(0, static_cast<uint32_t>(vertSize), static_cast<uint32_t>(sizeof(ImDrawVert)));
    mIndexView = mVertexBuffer->FillIndexBufferView(vertSizeAligned, static_cast<uint32_t>(idxSize), sizeof(ImDrawIdx) == sizeof(uint16_t));

    const float L = 0.0f;
    const float R = float(mWidth);
    const float B = float(mHeight);
    const float T = 0.0f;
    XMFLOAT4X4 matrix = XMFLOAT4X4(2.0f/(R-L),   0.0f,           0.0f,       0.0f,
                                   0.0f,         2.0f/(T-B),     0.0f,       0.0f,
                                   0.0f,         0.0f,           0.5f,       0.0f,
                                   (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f);

    mConstBuffer->CopyData(&matrix, sizeof(XMFLOAT4X4), 0, currentFrame);
}

void GUILayer::Draw(uint32_t currentFrame, RenderTargetBuffer *renderTarget) {
    if (!renderTarget) {
        return;
    }

    ImDrawData* drawData = ImGui::GetDrawData();
    DescriptorHeap *heaps[] = { mResourceHeap, mSamplerHeap };

    gCommand->SetViewport(0, 0, float(mWidth), float(mHeight));
    gCommand->SetRenderTarget(renderTarget, nullptr);
    gCommand->SetPipelineState(mGraphicsState);
    gCommand->SetRootSignature(mRootSignature);
    gCommand->SetDescriptorHeaps(heaps, _countof(heaps));
    gCommand->SetGraphicsRootConstantBufferView(0, mConstBuffer->GetGPUAddress(0, currentFrame));
    gCommand->SetGraphicsRootDescriptorTable(1, mFontTexture->GetSRVHandle());
    gCommand->SetGraphicsRootDescriptorTable(2, mSampler->GetHandle());
    gCommand->SetPrimitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    gCommand->SetVerticesAndIndices(mVertexView, mIndexView);

    int32_t vtxOffset = 0;
    uint32_t idxOffset = 0;
    for(int32_t i = 0; i < drawData->CmdListsCount; ++i) {
        const ImDrawList* drawList = drawData->CmdLists[i];
        for(int32_t j = 0; j < drawList->CmdBuffer.size(); ++j) {
            const ImDrawCmd* drawCmd = &drawList->CmdBuffer[j];
            if(drawCmd->UserCallback) {
                drawCmd->UserCallback(drawList, drawCmd);
            } else {
                const D3D12_RECT r = { long(drawCmd->ClipRect.x), long(drawCmd->ClipRect.y), long(drawCmd->ClipRect.z), long(drawCmd->ClipRect.w) };
                if(r.left < r.right && r.top < r.bottom) {
                    //PixelBuffer* texture = reinterpret_cast<PixelBuffer*>(drawCmd->TextureId);
                    //gCommand->SetGraphicsRootDescriptorTable(1, mFontTexture->GetSRVHandle());
                    gCommand->SetScissor(r);
                    gCommand->DrawIndexed(drawCmd->ElemCount, idxOffset, vtxOffset);
                }
            }
            idxOffset += drawCmd->ElemCount;
        }
        vtxOffset += drawList->VtxBuffer.size();
    }
}

void GUILayer::OnKeyDown(uint8_t key) {
    ImGuiIO& io = ImGui::GetIO();
    if (key < 256) {
        io.KeysDown[key] = 1;
    }
}

void GUILayer::OnKeyUp(uint8_t key) {
    ImGuiIO& io = ImGui::GetIO();
    if (key < 256) {
        io.KeysDown[key] = 0;
    }
}

void GUILayer::OnChar(uint16_t cha) {
    ImGuiIO& io = ImGui::GetIO();
    if (cha > 0 && cha < 0x10000) {
        io.AddInputCharacter(cha);
    }
}

void GUILayer::OnMouseLButtonDown(void) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[0] = true;
}

void GUILayer::OnMouseLButtonUp(void) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[0] = false;
}

void GUILayer::OnMouseRButtonDown(void) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[1] = true;
}

void GUILayer::OnMouseRButtonUp(void) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[1] = false;
}

void GUILayer::OnMouseMButtonDown(void) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[2] = true;
}

void GUILayer::OnMouseMButtonUp(void) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[2] = false;
}

void GUILayer::OnMouseMove(int64_t pos) {
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos.x = static_cast<float>(GET_X_LPARAM(pos));
    io.MousePos.y = static_cast<float>(GET_Y_LPARAM(pos));
}

void GUILayer::OnMouseWheel(uint64_t param) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel += GET_WHEEL_DELTA_WPARAM(param) > 0 ? +1.0f : -1.0f;
}

}