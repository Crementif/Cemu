#include "Cafe/HW/Latte/Core/LatteHostEvent.h"
#include "Cafe/HW/Latte/Core/Latte.h"
#include "Cafe/HW/Latte/Core/LattePM4.h"
#include "Cafe/HW/Latte/Core/LatteTexture.h"
#include "Cafe/HW/Latte/ISA/LatteReg.h"
#include "Cafe/HostModule/HostModule.h"

#ifdef ENABLE_VULKAN
#include "Cafe/HW/Latte/Renderer/Vulkan/VulkanRenderer.h"
#endif

enum class HostEventSurfaceKind : uint32
{
	None = 0,
	Color = 1,
	Depth = 2,
};

// same surface layout as IT_HLE_CLEAR_COLOR_DEPTH_STENCIL
static LatteTexture* LatteHostEvent_resolveSurface(HostEventSurfaceKind kind, MPTR physAddr, Latte::E_GX2SURFFMT format, Latte::E_HWTILEMODE tileMode, uint32 width, uint32 height, uint32 pitch, uint32 sliceIndex)
{
	if (kind == HostEventSurfaceKind::None || physAddr == MPTR_NULL || width == 0 || height == 0)
		return nullptr;
	if (Latte::TM_IsMacroTiled(tileMode))
		physAddr &= ~(7 << 8);

	if (kind == HostEventSurfaceKind::Color)
	{
		sint32 searchIndex = 0;
		while (LatteTextureView* view = LatteTC_LookupTextureByData(physAddr, width, height, pitch, 0, 1, sliceIndex, 1, &searchIndex))
		{
			LatteTexture* texture = view->baseTexture;
			if (texture->isDepth || Latte::GetFormatBits(format) != Latte::GetFormatBits(texture->format))
				continue;
			if (texture->pitch == pitch && texture->height == height)
				return texture;
		}
		return nullptr;
	}

	std::vector<LatteTexture*> textures;
	LatteTC_LookupTexturesByPhysAddr(physAddr, textures);
	for (LatteTexture* texture : textures)
	{
		if (!texture->isDepth || sliceIndex >= (uint32)texture->depth)
			continue;
		if (texture->pitch == pitch && texture->height == height)
			return texture;
	}
	return nullptr;
}

void LatteHostEvent_itHostEvent(const uint32be* data, uint32 nWords)
{
	cemu_assert_debug(nWords == 13);
	if (nWords < 13 || !HostModule::HasModules())
		return;

	HostModule::HostEvent event{};
	event.id = data[0];
	for (uint32 i = 0; i < 4; i++)
		event.params[i] = data[1 + i];

	HostEventSurfaceKind surfaceKind = (HostEventSurfaceKind)(uint32)data[5];
	MPTR physAddr = data[6];
	Latte::E_GX2SURFFMT format = (Latte::E_GX2SURFFMT)(uint32)data[7];
	Latte::E_HWTILEMODE tileMode = (Latte::E_HWTILEMODE)(uint32)data[8];
	uint32 width = data[9];
	uint32 height = data[10];
	uint32 pitch = data[11];
	uint32 sliceIndex = data[12];
	LatteTexture* texture = LatteHostEvent_resolveSurface(surfaceKind, physAddr, format, tileMode, width, height, pitch, sliceIndex);

#ifdef ENABLE_VULKAN
	if (g_renderer && g_renderer->GetType() == RendererAPI::Vulkan)
		VulkanRenderer::GetInstance()->HostModule_DispatchEvent(event, texture, sliceIndex);
#endif
	// the module might have written to the surface
	if (texture)
		LatteTexture_MarkDynamicTextureAsChanged(texture->baseView, sliceIndex, 0, LatteTexture_getNextUpdateEventCounter());
}

static uint32 s_hostEventCurrentView = 0;

void LatteHostEvent_itSetView(const uint32be* data, uint32 nWords)
{
	cemu_assert_debug(nWords == 1);
	s_hostEventCurrentView = (uint32)data[0] != 0 ? 1 : 0;
}

bool LatteHostEvent_itViewAluConst(const uint32be* data, uint32 nWords, bool& isVertexStage)
{
	cemu_assert_debug(nWords == 4);
	isVertexStage = false;
	if (nWords < 4)
		return false;
	uint32 registerOffset = data[0];
	uint32 count = data[1];
	MPTR slot = s_hostEventCurrentView == 0 ? (MPTR)data[2] : (MPTR)data[3];
	if (slot == MPTR_NULL || count == 0 || (registerOffset + count) > 0x800)
		return false;

	const uint32be* values = (const uint32be*)memory_getPointerFromPhysicalOffset(slot);
	uint32* registers = LatteGPUState.contextRegister + LATTE_REG_BASE_ALU_CONST + registerOffset;
	bool hasChanged = false;
	for (uint32 i = 0; i < count; i++)
	{
		uint32 v = values[i];
		hasChanged |= registers[i] != v;
		registers[i] = v;
	}
	isVertexStage = registerOffset >= 0x400;
	return hasChanged;
}
