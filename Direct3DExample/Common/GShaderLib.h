#pragma once

class GShaderLib {
public:
    static void CreateDefault(void);
    static inline GShaderLib *Default(void) { return msShaderLib; }

    bool AddShader(CHashString &name, CString &code, CString &entrance, uint32_t compileFlag);

    ID3DBlob * FindShader(CHashString &name);

private:
    GShaderLib(void);
    ~GShaderLib(void);

    typedef CBSTree<CHashString, ID3DBlob *> ShaderMap;

    static GShaderLib *msShaderLib;

    ShaderMap mShaders;
};

