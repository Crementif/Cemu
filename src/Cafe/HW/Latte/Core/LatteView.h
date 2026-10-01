#pragma once
#include "Cafe/HW/Latte/Core/LatteConst.h"

class LatteTexture;
class LatteTextureView;

// two-view rendering, draws inside an IT_HLE_VIEW_SCOPE get drawn again for the right view

void LatteView_itViewScope(const uint32be* data, uint32 nWords);
void LatteView_itViewMemWrite(const uint32be* data, uint32 nWords);
void LatteView_itSetAluConst(const uint32be* data, uint32 nWords);
void LatteView_itLoadAluConst(const uint32be* data, uint32 nWords);

bool LatteView_IsStereo();

// right view siblings, the right view reads the texture itself until it writes its own
LatteTexture* LatteView_GetOrCreateSibling(LatteTexture* texture);
void LatteView_DeleteSibling(LatteTexture* texture);
bool LatteView_IsSiblingValid(LatteTexture* texture, sint32 sliceIndex, sint32 mipIndex);
void LatteView_SetSiblingValid(LatteTexture* texture, sint32 sliceIndex, sint32 mipIndex, bool isValid);
void LatteView_SeedSibling(LatteTexture* texture, sint32 sliceIndex, sint32 mipIndex);
bool LatteView_HasValidSiblingInRange(LatteTexture* texture, sint32 firstSlice, sint32 numSlice, sint32 firstMip, sint32 numMip);
uint32 LatteView_GetSiblingEpoch();
LatteTexture* LatteView_OnTextureClear(LatteTexture* texture, sint32 sliceIndex, sint32 mipIndex);
bool LatteView_OnTextureCopy(LatteTexture* src, sint32 srcSlice, sint32 srcMip, LatteTexture* dst, sint32 dstSlice, sint32 dstMip, LatteTexture*& srcSibling, LatteTexture*& dstSibling);

bool LatteView_HasRightWords();
uint32 LatteView_GetRightDataEpoch();
const uint8* LatteView_GetRightUniformBlock(MPTR physAddr, uint32 size);
void LatteView_SyncRightUniformBuffer(LatteConst::ShaderType shaderType, uint32 bufferIndex, MPTR physAddr, uint32 size, uint32 bindOffset);
uint32 LatteView_GetRightUniformBufferOffset(LatteConst::ShaderType shaderType, uint32 bufferIndex);

// uniform registers set by IT_HLE_VIEW_ALU_CONST, until the game writes them again
void LatteView_SetRightAluConst(uint32 registerOffset, uint32 count, const uint32be* values);
void LatteView_ClearRightAluConst(uint32 registerOffset, uint32 count);
// pixel vec4s are 0-255, vertex 256-511
bool LatteView_HasRightAluConst(bool isVertexStage);
bool LatteView_IsRightAluConstVec4(uint32 vec4Index);
const uint32* LatteView_GetRightAluConst();
