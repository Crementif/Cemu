#pragma once

#include "Cafe/HW/Latte/Core/LattePerformanceMonitor.h"

void LatteDebug_EnableGpuMarkers(bool enabled);
bool LatteDebug_AreGpuMarkersEnabled();

uint64 LatteDebug_CalculateTraceHash(PPCInterpreter_t* hCPU);
uint32 LatteDebug_CreateTraceTag(PPCInterpreter_t* hCPU);
void LatteDebug_SetCurrentTraceTag(uint32 sequence);
void LatteDebug_ResetCurrentTraceTag();
const char* LatteDebug_GetCurrentTraceSummary();

void LatteDebug_RegisterDisplayListTags(uint32 physAddr, std::vector<uint32>&& tags);
const std::vector<uint32>* LatteDebug_GetDisplayListTags(uint32 physAddr);

class ScopedCpuTimer
{
public:
	ScopedCpuTimer(uint64& cycleCounter, LattePerfStatTimer* perfTimer = nullptr)
		: m_cycleCounter(cycleCounter), m_perfTimer(perfTimer), m_start(PPCTimer_getRawTsc())
	{
		if (m_perfTimer)
			m_perfTimer->beginMeasuring();
	}

	~ScopedCpuTimer()
	{
		if (m_perfTimer)
			m_perfTimer->endMeasuring();
		m_cycleCounter += (PPCTimer_getRawTsc() - m_start);
	}

private:
	uint64& m_cycleCounter;
	LattePerfStatTimer* m_perfTimer;
	uint64 m_start;
};

constexpr float kDrawLabelColor[4]  = { 0.18f, 0.80f, 0.33f, 1.0f };
constexpr float kClearLabelColor[4] = { 0.90f, 0.55f, 0.12f, 1.0f };
constexpr float kCopyLabelColor[4]  = { 0.23f, 0.58f, 0.95f, 1.0f };

// ---- Vulkan API Call Profiling ----

enum class VkApiCategory : uint32
{
	Draw,
	BindPipeline,
	BindDescriptorSets,
	BindVertexBuffers,
	BindIndexBuffer,
	PipelineBarrier,
	BeginRenderPass,
	EndRenderPass,
	ClearImage,
	SetState,
	PushConstants,
	CopyBufferImage,
	Other,
	_Count
};

constexpr uint32 kVkApiCategoryCount = static_cast<uint32>(VkApiCategory::_Count);

struct VkApiTimerSnapshot
{
	uint64 counters[kVkApiCategoryCount]{};

	uint64 GetCyclesForCat(VkApiCategory cat) const
	{
		return counters[static_cast<uint32>(cat)];
	}

	uint64 GetTotalCycles() const
	{
		uint64 total = 0;
		for (uint32 i = 0; i < kVkApiCategoryCount; i++)
			total += counters[i];
		return total;
	}
};

void VkApiProfiler_Enable(bool enabled);
bool VkApiProfiler_IsEnabled();
void VkApiProfiler_RecordCall(VkApiCategory category, uint64 cycles);
VkApiTimerSnapshot VkApiProfiler_Snapshot();
VkApiTimerSnapshot VkApiProfiler_SnapshotAndReset();
VkApiTimerSnapshot VkApiProfiler_Delta(const VkApiTimerSnapshot& before, const VkApiTimerSnapshot& after);
const char* VkApiProfiler_FormatToLabel(const VkApiTimerSnapshot& snapshot, char* buf, size_t bufSize);

uint64 LatteDebug_AccumulateAndGetCpuTimeUs(uint64 cycles);
void LatteDebug_ResetCumulativeCpuTime();
