#ifndef _TYPES_PBR_H_
#define _TYPES_PBR_H_

#ifdef __cplusplus
#define float4 XMFLOAT4
#define float3 XMFLOAT3
#define float4x4 XMFLOAT4X4
#define uint uint32_t
#endif

struct SettingsCB {
    bool    enableIBL;
    uint    numLight;
};

struct CameraCB {
    float4x4 mvp;
    float4   position;
};

struct MaterialCB {
    float3  albdo;
    float   metalness;
    float   roughness;
    float   ao;
};

struct LightCB {
    float3  position;
    uint    type;
    float3  color;
    float   angle;
};

#endif // _TYPES_PBR_H_
