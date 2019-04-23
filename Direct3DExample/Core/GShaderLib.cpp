#include "pch.h"
#include "GShaderLib.h"

GShaderLib * GShaderLib::msShaderLib = nullptr;

void GShaderLib::CreateDefault(void) {
    assert(msShaderLib == nullptr);
    msShaderLib = new GShaderLib();
}

GShaderLib::GShaderLib(void) {

}

GShaderLib::~GShaderLib(void) {
    for (ShaderMap::Iterator &iter = mShaders.Traverse(); iter.IsValid(); iter.Next()) {
        ReleaseIfNotNull(iter.GetValue());
    }
}

ID3DBlob * GShaderLib::FindShader(CHashString &name) {
    ShaderMap::ValueRef ref = mShaders.Find(name);
    return ref.IsValid() ? ref.Get() : nullptr;
}

bool GShaderLib::AddShader(CHashString &name, CString &code, CString &entrance, uint32_t compileFlags) {
    ID3DBlob *shader = FindShader(name);
    if (shader) {
        Print("GShaderLib: shader '%s' already exist\n", name.Get());
        return true;
    }

#if defined(_DEBUG)
    compileFlags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);
#endif

    ID3DBlob *error = nullptr;
    HRESULT result = D3DCompile(code.Get(), code.Length() + 1, nullptr, nullptr, nullptr, entrance.Get(), "vs_5_0", compileFlags, 0, &shader, &error);
    if (error) {
        Print("GShaderLib: compile shader '%s' with error %s", name.Get(), (char*)error->GetBufferPointer());
        ReleaseIfNotNull(error);
        return false;
    }

    mShaders.Insert(name, shader);

    return true;
}

