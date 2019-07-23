#include "pch.h"
#include "RayTracingState.h"
#include "RootSignature.h"
#include "RenderCore.h"

namespace Render {

RayTracingState::RayTracingState(uint32_t count)
: mSubObjects(count)
, mState(nullptr)
{

}

RayTracingState::~RayTracingState(void) {
    CleanupSubObjects();
    ReleaseAndSetNull(mState);
}

int32_t RayTracingState::AddSubObject(D3D12_STATE_SUBOBJECT_TYPE type, const void *object) {
    if (!object || type >= D3D12_STATE_SUBOBJECT_TYPE_MAX_VALID) {
        return -1;
    }
    mSubObjects.PushBack({ type, object });
    return static_cast<int32_t>(mSubObjects.Count() - 1);
}

int32_t RayTracingState::AddStateObjectConfig(D3D12_STATE_OBJECT_FLAGS flags) {
    ASSERT_PRINT(mSubObjects.Count() < mSubObjects.Capacity());
    D3D12_STATE_OBJECT_CONFIG *object = new D3D12_STATE_OBJECT_CONFIG;
    object->Flags = flags;
    return AddSubObject(D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG, object);
}

int32_t RayTracingState::AddGlobalRootSignature(RootSignature *rs) {
    ASSERT_PRINT(mSubObjects.Count() < mSubObjects.Capacity());
    D3D12_GLOBAL_ROOT_SIGNATURE *object = new D3D12_GLOBAL_ROOT_SIGNATURE;
    object->pGlobalRootSignature = rs->Get();
    return AddSubObject(D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, object);
}

int32_t RayTracingState::AddLocalRootSignature(RootSignature *rs) {
    ASSERT_PRINT(mSubObjects.Count() < mSubObjects.Capacity());
    D3D12_LOCAL_ROOT_SIGNATURE *object = new D3D12_LOCAL_ROOT_SIGNATURE;
    object->pLocalRootSignature = rs->Get();
    return AddSubObject(D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, object);
}

int32_t RayTracingState::AddNodeMask(uint32_t mask) {
    ASSERT_PRINT(mSubObjects.Count() < mSubObjects.Capacity());
    D3D12_NODE_MASK *object = new D3D12_NODE_MASK;
    object->NodeMask = mask;
    return AddSubObject(D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK, object);
}

int32_t RayTracingState::AddDXILLibrary(const char *shaderFile, const wchar_t *functions[], uint32_t funcCount) {
    if (!shaderFile || !functions || !funcCount) {
        return -1;
    }

    ASSERT_PRINT(mSubObjects.Count() < mSubObjects.Capacity());
    D3D12_DXIL_LIBRARY_DESC *object = new D3D12_DXIL_LIBRARY_DESC;
    object->DXILLibrary.pShaderBytecode = ReadFileData(shaderFile, object->DXILLibrary.BytecodeLength);
    object->NumExports = funcCount;
    object->pExports = new D3D12_EXPORT_DESC[funcCount];
    for (uint32_t i = 0; i < funcCount; ++i) {
        object->pExports[i].Name = functions[i];
        object->pExports[i].ExportToRename = nullptr;
        object->pExports[i].Flags = D3D12_EXPORT_FLAG_NONE;
    }
    return AddSubObject(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, object);
}

int32_t RayTracingState::AddSubObjectToExportsAssociation(uint32_t subObjIndex, const wchar_t *exports[], uint32_t exportCount) {
    if (!exports || !exportCount || mSubObjects.Count() <= subObjIndex) {
        return -1;
    }

    ASSERT_PRINT(mSubObjects.Count() < mSubObjects.Capacity());
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION *object = new D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    object->pSubobjectToAssociate = mSubObjects.Data() + subObjIndex;
    object->NumExports = exportCount;
    object->pExports = exports;
    return AddSubObject(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, object);
}

int32_t RayTracingState::AddDXILSubObjectToExportsAssociation(const wchar_t *subObj, const wchar_t *exports[], uint32_t exportCount) {
    if (!exports || !exportCount || !subObj) {
        return -1;
    }

    ASSERT_PRINT(mSubObjects.Count() < mSubObjects.Capacity());
    D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION *object = new D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    object->SubobjectToAssociate = subObj;
    object->NumExports = exportCount;
    object->pExports = exports;
    return AddSubObject(D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION, object);
}

int32_t RayTracingState::AddRayTracingShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize) {
    ASSERT_PRINT(mSubObjects.Count() < mSubObjects.Capacity());
    D3D12_RAYTRACING_SHADER_CONFIG *object = new D3D12_RAYTRACING_SHADER_CONFIG;
    object->MaxPayloadSizeInBytes = maxPayloadSize;
    object->MaxAttributeSizeInBytes = maxAttributeSize;
    return AddSubObject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, object);
}

int32_t RayTracingState::AddRayTracingPipelineConfig(uint32_t maxTraceDepth) {
    ASSERT_PRINT(mSubObjects.Count() < mSubObjects.Capacity());
    D3D12_RAYTRACING_PIPELINE_CONFIG *object = new D3D12_RAYTRACING_PIPELINE_CONFIG;
    object->MaxTraceRecursionDepth = maxTraceDepth;
    return AddSubObject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, object);
}

int32_t RayTracingState::AddHitGroup(const wchar_t *hitGroup, const wchar_t *closetHit, const wchar_t *anyHit, const wchar_t *intersection, D3D12_HIT_GROUP_TYPE type) {
    if (!hitGroup) {
        return -1;
    }

    ASSERT_PRINT(mSubObjects.Count() < mSubObjects.Capacity());
    D3D12_HIT_GROUP_DESC *object = new D3D12_HIT_GROUP_DESC;
    object->HitGroupExport = hitGroup;
    object->ClosestHitShaderImport = closetHit;
    object->AnyHitShaderImport = anyHit;
    object->IntersectionShaderImport = intersection;
    object->Type = type;
    return AddSubObject(D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, object);
}

void RayTracingState::Create(void) {
    if (!mSubObjects.Count()) {
        return;
    }

    ReleaseAndSetNull(mState);

    D3D12_STATE_OBJECT_DESC desc = {};
    desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    desc.NumSubobjects = mSubObjects.Count();
    desc.pSubobjects = mSubObjects.Data();

#ifdef _DEBUG
    PrintStateObjectDesc(desc);
#endif

    ASSERT_SUCCEEDED(gDXRDevice->CreateStateObject(&desc, IID_PPV_ARGS(&mState)));

    CleanupSubObjects();
}

void RayTracingState::CleanupSubObjects(void) {
    for (uint32_t i = 0; i < mSubObjects.Count(); ++i) {
        auto &subObject = mSubObjects.At(i);
        switch (subObject.Type) {
        case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
            D3D12_DXIL_LIBRARY_DESC *object = (D3D12_DXIL_LIBRARY_DESC *)(subObject.pDesc);
            DeleteAndSetNull(object->pExports);
            DeleteAndSetNull(object);
        }   break;
        default:
            DeleteAndSetNull(subObject.pDesc);
            break;
        }
    }
    mSubObjects.Clear();
}

INLINE void RayTracingState::PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC &desc)
{
    Printf("\n");
    Printf("--------------------------------------------------------------------\n");
    Print("| D3D12 State Object 0x%llx: ", desc);
    if (desc.Type == D3D12_STATE_OBJECT_TYPE_COLLECTION) {
        Printf("Collection\n");
    }
    if (desc.Type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE) {
        Printf("Raytracing Pipeline\n");
    }

    auto ExportTree = [](UINT depth, UINT numExports, const D3D12_EXPORT_DESC* exports) {
        for (UINT i = 0; i < numExports; i++) {
            Printf("|");
            if (depth > 0) {
                for (UINT j = 0; j < 2 * depth - 1; j++) {
                    Printf(" ");
                }
            }
            Print(" [%d]: ", i);
            if (exports[i].ExportToRename) {
                WPrintf(exports[i].ExportToRename);
                Printf(" --> ");
            }
            WPrintf(exports[i].Name);
            Printf("\n");
        }
    };

    for (UINT i = 0; i < desc.NumSubobjects; i++) {
        Print("| [%d]: ", i);
        switch (desc.pSubobjects[i].Type) {
        case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE:
            Print("Global Root Signature 0x%llx\n", desc.pSubobjects[i].pDesc);
            break;
        case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE:
            Print("Local Root Signature 0x%llx\n", desc.pSubobjects[i].pDesc);
            break;
        case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK:
            Print("Node Mask: 0x%llx\n", *static_cast<const UINT*>(desc.pSubobjects[i].pDesc));
            break;
        case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
            auto lib = static_cast<const D3D12_DXIL_LIBRARY_DESC*>(desc.pSubobjects[i].pDesc);
            Print("DXIL Library 0x%llx, %llu bytes\n", lib->DXILLibrary.pShaderBytecode, lib->DXILLibrary.BytecodeLength);
            ExportTree(1, lib->NumExports, lib->pExports);
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
            auto collection = static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(desc.pSubobjects[i].pDesc);
            Print("Existing Library 0x%llx\n", collection->pExistingCollection);
            ExportTree(1, collection->NumExports, collection->pExports);
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
            auto association = static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc.pSubobjects[i].pDesc);
            UINT index = static_cast<UINT>(association->pSubobjectToAssociate - desc.pSubobjects);
            Print("Subobject to Exports Association (Subobject [%d])\n", index);
            for (UINT j = 0; j < association->NumExports; j++) {
                Print("|  [%d]: ", j); WPrintf(association->pExports[j]); Printf("\n");
            }
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
            auto association = static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc.pSubobjects[i].pDesc);
            Printf("DXIL Subobjects to Exports Association (");  WPrintf(association->SubobjectToAssociate); Printf(")\n");
            for (UINT j = 0; j < association->NumExports; j++) {
                Print("|  [%d]: ", j); WPrintf(association->pExports[j]); Printf("\n");
            }
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG: {
            Printf("Raytracing Shader Config\n");
            auto config = static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(desc.pSubobjects[i].pDesc);
            Print("|  [0]: Max Payload Size: %d bytes\n", config->MaxPayloadSizeInBytes);
            Print("|  [1]: Max Attribute Size: %d bytes\n", config->MaxAttributeSizeInBytes);
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG: {
            Printf("Raytracing Pipeline Config\n");
            auto config = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(desc.pSubobjects[i].pDesc);
            Print("|  [0]: Max Recursion Depth: %d\n", config->MaxTraceRecursionDepth);
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
            auto hitGroup = static_cast<const D3D12_HIT_GROUP_DESC*>(desc.pSubobjects[i].pDesc);
            Printf("Hit Group (");  WPrintf(hitGroup->HitGroupExport ? hitGroup->HitGroupExport : L"[none]"); Printf(")\n");
            Printf("|  [0]: Any Hit Import: "); WPrintf(hitGroup->AnyHitShaderImport ? hitGroup->AnyHitShaderImport : L"[none]"); Printf("\n");
            Printf("|  [1]: Closest Hit Import: "); WPrintf(hitGroup->ClosestHitShaderImport ? hitGroup->ClosestHitShaderImport : L"[none]"); Printf("\n");
            Printf("|  [2]: Intersection Import: "); WPrintf(hitGroup->IntersectionShaderImport ? hitGroup->IntersectionShaderImport : L"[none]"); Printf("\n");
            break;
        }
        }
        Printf("|--------------------------------------------------------------------\n");
    }
    Printf("\n");
}

}
