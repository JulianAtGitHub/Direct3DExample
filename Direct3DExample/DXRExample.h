#pragma once

#include "Example.h"
#include "Core/Render/RenderCore.h"

namespace Render {
    class GPUBuffer;
}

class DXRExample : public Example {
public:
    DXRExample(HWND hwnd);
    virtual ~DXRExample(void);

    virtual void Init(void);
    virtual void Update(void);
    virtual void Render(void);
    virtual void Destroy(void);

private:
    void InitAccelerationStructures(void);

    Render::GPUBuffer *mVertexBuffer;
};
