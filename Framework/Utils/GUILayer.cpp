#include "stdafx.h"
#include "Render/RenderCore.h"
#include "Render/RootSignature.h"
#include "Render/PipelineState.h"
#include "Render/CommandContext.h"
#include "Render/DescriptorHeap.h"
#include "Render/Sampler.h"
#include "Render/Resource/PixelBuffer.h"
#include "Render/Resource/ConstantBuffer.h"
#include "Render/Resource/GPUBuffer.h"
#include "Render/Resource/RenderTargetBuffer.h"
#include "GUI/imgui.h"
#include "GUILayer.h"

#include "GuiVS.h"
#include "GuiPS.h"

namespace Utils {

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
{
    for (uint32_t i = 0; i < Render::FRAME_COUNT; ++i) {
        mVertexBuffer[i] = nullptr;
    }
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

    mRootSignature = new Render::RootSignature(Render::RootSignature::Graphics, RootSigSlotMax, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    mRootSignature->SetDescriptor(TransformSlot, D3D12_ROOT_PARAMETER_TYPE_CBV, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    mRootSignature->SetConstants(ConstantSlot, 1, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    mRootSignature->SetDescriptorTable(TextureSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    mRootSignature->SetDescriptorTable(SamplerSlot, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    mRootSignature->Create();

    D3D12_INPUT_ELEMENT_DESC inputElements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 8,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    mGraphicsState = new Render::GraphicsState();
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
    mGraphicsState->SetVertexShader(gscGuiVS, sizeof(gscGuiVS));
    mGraphicsState->SetPixelShader(gscGuiPS, sizeof(gscGuiPS));
    mGraphicsState->Create(mRootSignature);

    mResourceHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RESOURCE_COUNT);
    mSamplerHeap = new Render::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);

    uint8_t *texturePixels = nullptr;
    int32_t textureWidth = 0;
    int32_t textureHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&texturePixels, &textureWidth, &textureHeight);

    mFontTexture = new Render::PixelBuffer(textureWidth * Render::BytesPerPixel(DXGI_FORMAT_R8G8B8A8_UNORM), textureWidth, textureHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
    Render::gCommand->Begin();
    Render::gCommand->UploadTexture(mFontTexture, texturePixels);
    Render::gCommand->End(true);
    io.Fonts->TexID = mFontTexture;
    AddImage(mFontTexture);

    mSampler = new Render::Sampler();
    mSampler->Create(mSamplerHeap->Allocate());

    mConstBuffer = new Render::ConstantBuffer(sizeof(XMFLOAT4X4), 1);
    for (uint32_t i = 0; i < Render::FRAME_COUNT; ++i) {
        mVertexBuffer[i] = new Render::GPUBuffer(VERTEX_BUFFER_SIZE, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, true);
    }
}

void GUILayer::Destroy(void) {
    ImGui::DestroyContext(mContext);
    mContext = nullptr;

    for (uint32_t i = 0; i < Render::FRAME_COUNT; ++i) {
        DeleteAndSetNull(mVertexBuffer[i]);
    }
    DeleteAndSetNull(mConstBuffer);
    DeleteAndSetNull(mFontTexture);
    DeleteAndSetNull(mSampler);
    DeleteAndSetNull(mResourceHeap);
    DeleteAndSetNull(mSamplerHeap);
    DeleteAndSetNull(mRootSignature);
    DeleteAndSetNull(mGraphicsState);
}

bool GUILayer::AddImage(Render::PixelBuffer *image) {
    if (!image || !mResourceHeap->GetRemainingCount()) {
        return false;
    }

    // Already added
    if (mResourceMap.find(image) != mResourceMap.end()) {
        return true;
    }

    image->CreateSRV(mResourceHeap->Allocate(), false);
    mResourceMap[image] = { RESOURCE_COUNT - mResourceHeap->GetRemainingCount() - 1, 0 };
    return true;
}

bool GUILayer::SetImageMipLevel(Render::PixelBuffer *image, uint32_t mipLevel) {
    if (!image) {
        return false;
    }

    if (mipLevel >= image->GetMipLevels()) {
        return false;
    }

    // Already added
    if (mResourceMap.find(image) == mResourceMap.end()) {
        return false;
    }

    auto &info = mResourceMap[image];
    info.mipLevel = mipLevel;
    return true;
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

void GUILayer::EndFrame(uint32_t frameIdx) {
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
        mVertexBuffer[frameIdx]->UploadData(&drawList->VtxBuffer[0], subVertSize, vertStartAt);
        mVertexBuffer[frameIdx]->UploadData(&drawList->IdxBuffer[0], subIdxSize, idxStartAt);
        vertStartAt += subVertSize;
        idxStartAt += subIdxSize;
    }

    mVertexView = mVertexBuffer[frameIdx]->FillVertexBufferView(0, static_cast<uint32_t>(vertSize), static_cast<uint32_t>(sizeof(ImDrawVert)));
    mIndexView = mVertexBuffer[frameIdx]->FillIndexBufferView(vertSizeAligned, static_cast<uint32_t>(idxSize), sizeof(ImDrawIdx) == sizeof(uint16_t));

    const float L = 0.0f;
    const float R = float(mWidth);
    const float B = float(mHeight);
    const float T = 0.0f;
    XMFLOAT4X4 matrix = XMFLOAT4X4(2.0f/(R-L),   0.0f,           0.0f,       0.0f,
                                   0.0f,         2.0f/(T-B),     0.0f,       0.0f,
                                   0.0f,         0.0f,           0.5f,       0.0f,
                                   (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f);

    mConstBuffer->CopyData(&matrix, sizeof(XMFLOAT4X4), 0, frameIdx);
}

void GUILayer::Draw(uint32_t frameIdx, Render::RenderTargetBuffer *renderTarget) {
    if (!renderTarget) {
        return;
    }

    ImDrawData* drawData = ImGui::GetDrawData();
    Render::DescriptorHeap *heaps[] = { mResourceHeap, mSamplerHeap };

    Render::gCommand->SetViewport(0, 0, float(mWidth), float(mHeight));
    Render::gCommand->SetRenderTarget(renderTarget, nullptr);
    Render::gCommand->SetPipelineState(mGraphicsState);
    Render::gCommand->SetRootSignature(mRootSignature);
    Render::gCommand->SetDescriptorHeaps(heaps, _countof(heaps));
    Render::gCommand->SetGraphicsRootConstantBufferView(TransformSlot, mConstBuffer->GetGPUAddress(0, frameIdx));
    Render::gCommand->SetGraphicsRootDescriptorTable(SamplerSlot, mSampler->GetHandle());
    Render::gCommand->SetPrimitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Render::gCommand->SetVerticesAndIndices(mVertexView, mIndexView);

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
                    float mipLevel = 0;
                    auto it = mResourceMap.find(drawCmd->TextureId);
                    if (it != mResourceMap.end()) {
                        Render::gCommand->SetGraphicsRootDescriptorTable(TextureSlot, mResourceHeap->GetHandle(it->second.handlerIndex));
                        mipLevel = float(it->second.mipLevel);
                    }
                    Render::gCommand->SetGraphicsRootConstants(ConstantSlot, &mipLevel, 1);
                    Render::gCommand->SetScissor(r);
                    Render::gCommand->DrawIndexed(drawCmd->ElemCount, idxOffset, vtxOffset);
                }
            }
            idxOffset += drawCmd->ElemCount;
        }
        vtxOffset += drawList->VtxBuffer.size();
    }
}

bool GUILayer::IsHovered(void) {
    return ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
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