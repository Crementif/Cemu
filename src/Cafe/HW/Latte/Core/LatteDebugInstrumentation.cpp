#include "Cafe/HW/Latte/Core/LatteDebugInstrumentation.h"

#include "Cafe/HW/Espresso/PPCState.h"
#include "Cafe/OS/RPL/rpl_symbol_storage.h"
#include "Cafe/OS/libs/coreinit/coreinit_Thread.h"

namespace
{
constexpr uint32 kTraceRecordCount = 8192;
constexpr uint32 kTraceRecordMask = kTraceRecordCount - 1;
constexpr size_t kTraceSummaryLength = 320;
constexpr sint32 kTraceDepth = 5;
constexpr uint32 kSymbolSummaryCacheSize = 128;

struct TraceRecord
{
    std::atomic_uint32_t sequence{ 0 };
    char stackSummary[kTraceSummaryLength]{};
};

struct SymbolSummaryCacheEntry
{
	uint32 address{};
	char text[96]{};
};

std::array<TraceRecord, kTraceRecordCount> s_traceRecords{};
std::atomic_uint32_t s_nextTraceSequence{ 1 };
std::atomic_uint32_t s_currentTraceSequence{ 0 };
std::atomic_bool s_gpuMarkersEnabled{ false };
thread_local std::array<SymbolSummaryCacheEntry, kSymbolSummaryCacheSize> s_symbolSummaryCache{};

void AppendFormatted(char*& cursor, size_t& remaining, const char* format, ...)
{
    if (remaining <= 1)
        return;

    va_list args;
    va_start(args, format);
    int written = std::vsnprintf(cursor, remaining, format, args);
    va_end(args);

    if (written <= 0)
        return;

    size_t consumed = std::min<size_t>(static_cast<size_t>(written), remaining - 1);
    cursor += consumed;
    remaining -= consumed;
}

sint32 ScoreStackTrace(struct OSThread_t* thread, MPTR sp)
{
    if (!thread)
        return -1;

    uint32 stackMinAddr = thread->stackEnd.GetMPTR();
    uint32 stackMaxAddr = thread->stackBase.GetMPTR();

    sint32 score = 0;
    uint32 currentStackPtr = sp;
    for (sint32 i = 0; i < 8; i++)
    {
        if (!memory_isAddressRangeAccessible(currentStackPtr, 8))
            break;

        uint32 nextStackPtr = memory_readU32(currentStackPtr);
        if (nextStackPtr < currentStackPtr)
            break;
        if (nextStackPtr < stackMinAddr || nextStackPtr > stackMaxAddr)
            break;
        if ((nextStackPtr & 3) != 0)
            break;
        score += 10;

        uint32 returnAddress = memory_readU32(nextStackPtr + 4);
        if (returnAddress > 0 && returnAddress < 0x10000000 && (returnAddress & 3) == 0)
            score += 5;
        else
            score -= 5;

        currentStackPtr = nextStackPtr;
    }

    return score;
}

MPTR FindLikelyStackPointer(struct OSThread_t* thread, MPTR sp)
{
    sint32 bestScore = ScoreStackTrace(thread, sp);
    MPTR bestSP = sp;

    for (sint32 i = 1; i < 16; i++)
    {
        MPTR sampleSP = sp + i * 4;
        sint32 score = ScoreStackTrace(thread, sampleSP);
        if (score > bestScore)
        {
            bestScore = score;
            bestSP = sampleSP;
        }
    }

    return bestSP;
}

bool TryGetNextStackFrame(struct OSThread_t* thread, MPTR currentStackPtr, MPTR& nextStackPtr, uint32& returnAddress)
{
    if (!thread || !memory_isAddressRangeAccessible(currentStackPtr, 8))
        return false;

    uint32 stackMinAddr = thread->stackEnd.GetMPTR();
    uint32 stackMaxAddr = thread->stackBase.GetMPTR();

    nextStackPtr = memory_readU32(currentStackPtr);
    if (nextStackPtr < currentStackPtr)
        return false;
    if (nextStackPtr < stackMinAddr || nextStackPtr > stackMaxAddr)
        return false;
    if ((nextStackPtr & 3) != 0)
        return false;
    if (!memory_isAddressRangeAccessible(nextStackPtr + 4, 4))
        return false;

    returnAddress = memory_readU32(nextStackPtr + 4);
    if (returnAddress == 0)
        return false;
    return true;
}

MPTR GetInitialStackPointer(struct OSThread_t* thread, MPTR sp)
{
	MPTR nextStackPtr = MPTR_NULL;
	uint32 returnAddress = 0;
	if (TryGetNextStackFrame(thread, sp, nextStackPtr, returnAddress))
		return sp;
	return FindLikelyStackPointer(thread, sp);
}

uint64 MixTraceHash(uint64 hash, uint32 value)
{
	hash = std::rotl<uint64>(hash ^ 0x9E3779B97F4A7C15ull, 11);
	hash ^= static_cast<uint64>(value) + 0x85EBCA6Bull + (hash << 6) + (hash >> 2);
	return hash;
}

const char* LookupSymbolSummaryCached(uint32 returnAddress)
{
	SymbolSummaryCacheEntry& cacheEntry = s_symbolSummaryCache[(returnAddress >> 2) & (kSymbolSummaryCacheSize - 1)];
	if (cacheEntry.address == returnAddress && cacheEntry.text[0] != '\0')
		return cacheEntry.text;

	cacheEntry.address = returnAddress;
	cacheEntry.text[0] = '\0';

	RPLStoredSymbol* symbol = rplSymbolStorage_getByClosestAddress(returnAddress);
	if (symbol)
	{
		auto libName = static_cast<const char*>(symbol->libName);
		auto symbolName = static_cast<const char*>(symbol->symbolName);
		std::snprintf(cacheEntry.text, sizeof(cacheEntry.text), "%s.%s+0x%x", libName, symbolName, returnAddress - symbol->address);
	}
	else
	{
		std::snprintf(cacheEntry.text, sizeof(cacheEntry.text), "0x%08x", returnAddress);
	}

	return cacheEntry.text;
}

void AppendSymbolSummary(char*& cursor, size_t& remaining, uint32 returnAddress)
{
	AppendFormatted(cursor, remaining, "%s", LookupSymbolSummaryCached(returnAddress));
}

void CaptureTraceSummary(PPCInterpreter_t* hCPU, TraceRecord& record)
{
    record.stackSummary[0] = '\0';
    if (!hCPU)
        return;

    OSThread_t* thread = coreinit::OSGetCurrentThread();
    if (!thread)
    {
		std::snprintf(record.stackSummary, sizeof(record.stackSummary), "ip=0x%08x lr=0x%08x", hCPU->instructionPointer, hCPU->spr.LR);
        return;
    }

    MPTR stackPtr = hCPU->gpr[1];
    if (stackPtr == MPTR_NULL)
    {
		std::snprintf(record.stackSummary, sizeof(record.stackSummary), "ip=0x%08x lr=0x%08x", hCPU->instructionPointer, hCPU->spr.LR);
        return;
    }

    MPTR currentStackPtr = GetInitialStackPointer(thread, stackPtr);
    char* cursor = record.stackSummary;
    size_t remaining = std::size(record.stackSummary);
    sint32 frameCount = 0;

    while (frameCount < kTraceDepth)
    {
        MPTR nextStackPtr = MPTR_NULL;
        uint32 returnAddress = 0;
        if (!TryGetNextStackFrame(thread, currentStackPtr, nextStackPtr, returnAddress))
            break;

        if (frameCount != 0)
            AppendFormatted(cursor, remaining, " <- ");

        AppendSymbolSummary(cursor, remaining, returnAddress);
        currentStackPtr = nextStackPtr;
        frameCount++;
    }

    if (frameCount == 0)
		std::snprintf(record.stackSummary, sizeof(record.stackSummary), "ip=0x%08x lr=0x%08x", hCPU->instructionPointer, hCPU->spr.LR);
}

const TraceRecord* LookupTraceRecord(uint32 sequence)
{
    if (sequence == 0)
        return nullptr;

    const TraceRecord& record = s_traceRecords[sequence & kTraceRecordMask];
    if (record.sequence.load(std::memory_order_acquire) != sequence)
        return nullptr;
    return &record;
}
}

void LatteDebug_EnableGpuMarkers(bool enabled)
{
    s_gpuMarkersEnabled.store(enabled, std::memory_order_relaxed);
    if (!enabled)
        LatteDebug_ResetCurrentTraceTag();
}

bool LatteDebug_AreGpuMarkersEnabled()
{
    return s_gpuMarkersEnabled.load(std::memory_order_relaxed);
}

uint64 LatteDebug_CalculateTraceHash(PPCInterpreter_t* hCPU)
{
	if (!hCPU)
		return 0;

	uint64 hash = 0xC4CEB9FE1A85EC53ull;
	hash = MixTraceHash(hash, hCPU->instructionPointer);
	hash = MixTraceHash(hash, hCPU->spr.LR);

	OSThread_t* thread = coreinit::OSGetCurrentThread();
	MPTR stackPtr = hCPU->gpr[1];
	if (!thread || stackPtr == MPTR_NULL)
		return hash;

	MPTR currentStackPtr = GetInitialStackPointer(thread, stackPtr);
	for (sint32 i = 0; i < kTraceDepth; i++)
	{
		MPTR nextStackPtr = MPTR_NULL;
		uint32 returnAddress = 0;
		if (!TryGetNextStackFrame(thread, currentStackPtr, nextStackPtr, returnAddress))
			break;
		hash = MixTraceHash(hash, returnAddress);
		currentStackPtr = nextStackPtr;
	}

	return hash;
}

uint32 LatteDebug_CreateTraceTag(PPCInterpreter_t* hCPU)
{
    if (!LatteDebug_AreGpuMarkersEnabled() || !hCPU)
        return 0;

    uint32 sequence = s_nextTraceSequence.fetch_add(1, std::memory_order_relaxed);
    TraceRecord& record = s_traceRecords[sequence & kTraceRecordMask];
    CaptureTraceSummary(hCPU, record);
    record.sequence.store(sequence, std::memory_order_release);
    return sequence;
}

void LatteDebug_SetCurrentTraceTag(uint32 sequence)
{
    s_currentTraceSequence.store(sequence, std::memory_order_relaxed);
}

void LatteDebug_ResetCurrentTraceTag()
{
    s_currentTraceSequence.store(0, std::memory_order_relaxed);
}

const char* LatteDebug_GetCurrentTraceSummary()
{
    uint32 sequence = s_currentTraceSequence.load(std::memory_order_relaxed);
    const TraceRecord* record = LookupTraceRecord(sequence);
    if (!record || record->stackSummary[0] == '\0')
        return nullptr;
    return record->stackSummary;
}

std::unordered_map<uint32, std::vector<uint32>> s_displayListTags;

void LatteDebug_RegisterDisplayListTags(uint32 physAddr, std::vector<uint32>&& tags)
{
	if (physAddr == 0)
		return;
	s_displayListTags.insert_or_assign(physAddr, std::move(tags));
}

const std::vector<uint32>* LatteDebug_GetDisplayListTags(uint32 physAddr)
{
	auto it = s_displayListTags.find(physAddr);
	if (it == s_displayListTags.end())
		return nullptr;
	return &it->second;
}

// ---- VkApiProfiler ----

namespace
{
	std::array<std::atomic_uint64_t, kVkApiCategoryCount> s_vkApiCounters{};
	std::atomic_bool s_vkApiProfilingEnabled{ false };
}

void VkApiProfiler_Enable(bool enabled)
{
	s_vkApiProfilingEnabled.store(enabled, std::memory_order_relaxed);
	if (!enabled)
	{
		for (uint32 i = 0; i < kVkApiCategoryCount; i++)
			s_vkApiCounters[i].store(0, std::memory_order_relaxed);
	}
}

bool VkApiProfiler_IsEnabled()
{
	return s_vkApiProfilingEnabled.load(std::memory_order_relaxed);
}

void VkApiProfiler_RecordCall(VkApiCategory category, uint64 cycles)
{
	if (!s_vkApiProfilingEnabled.load(std::memory_order_relaxed))
		return;
	uint32 idx = static_cast<uint32>(category);
	if (idx < kVkApiCategoryCount)
		s_vkApiCounters[idx].fetch_add(cycles, std::memory_order_relaxed);
}

VkApiTimerSnapshot VkApiProfiler_Snapshot()
{
	VkApiTimerSnapshot snap;
	for (uint32 i = 0; i < kVkApiCategoryCount; i++)
		snap.counters[i] = s_vkApiCounters[i].load(std::memory_order_relaxed);
	return snap;
}

VkApiTimerSnapshot VkApiProfiler_SnapshotAndReset()
{
	VkApiTimerSnapshot snap;
	for (uint32 i = 0; i < kVkApiCategoryCount; i++)
		snap.counters[i] = s_vkApiCounters[i].exchange(0, std::memory_order_relaxed);
	return snap;
}

VkApiTimerSnapshot VkApiProfiler_Delta(const VkApiTimerSnapshot& before, const VkApiTimerSnapshot& after)
{
	VkApiTimerSnapshot delta;
	for (uint32 i = 0; i < kVkApiCategoryCount; i++)
	{
		delta.counters[i] = (after.counters[i] >= before.counters[i])
			? (after.counters[i] - before.counters[i])
			: 0;
	}
	return delta;
}

const char* VkApiProfiler_FormatToLabel(const VkApiTimerSnapshot& snapshot, char* buf, size_t bufSize)
{
	static constexpr struct
	{
		VkApiCategory cat;
		const char* shortName;
	} kCatNames[] = {
		{ VkApiCategory::Draw,               "draw"    },
		{ VkApiCategory::BindPipeline,       "bind"    },
		{ VkApiCategory::BindDescriptorSets, "desc"    },
		{ VkApiCategory::BindVertexBuffers,  "vb"      },
		{ VkApiCategory::BindIndexBuffer,    "ib"      },
		{ VkApiCategory::PipelineBarrier,    "barrier" },
		{ VkApiCategory::BeginRenderPass,    "begPass" },
		{ VkApiCategory::EndRenderPass,      "endPass" },
		{ VkApiCategory::ClearImage,         "clear"   },
		{ VkApiCategory::SetState,           "state"   },
		{ VkApiCategory::PushConstants,      "push"    },
		{ VkApiCategory::CopyBufferImage,    "copy"    },
		{ VkApiCategory::Other,              "other"   },
	};

	char* cursor = buf;
	size_t remaining = bufSize;

	bool first = true;
	for (const auto& entry : kCatNames)
	{
		uint64 cycles = snapshot.GetCyclesForCat(entry.cat);
		if (cycles == 0)
			continue;

		uint64 us = (uint64)PPCTimer_tscToMicroseconds(cycles);
		int written;
		if (first)
		{
			written = snprintf(cursor, remaining, "%s=%lluus", entry.shortName, (unsigned long long)us);
			first = false;
		}
		else
		{
			written = snprintf(cursor, remaining, " %s=%lluus", entry.shortName, (unsigned long long)us);
		}

		if (written <= 0)
			continue;

		size_t consumed = (size_t)written;
		if (consumed >= remaining)
			consumed = remaining - 1;
		cursor += consumed;
		remaining -= consumed;
		if (remaining <= 1)
			break;
	}

	return buf;
}

uint64 s_cumulativeCpuCycles{ 0 };

uint64 LatteDebug_AccumulateAndGetCpuTimeUs(uint64 cycles)
{
	s_cumulativeCpuCycles += cycles;
	return (uint64)PPCTimer_tscToMicroseconds(s_cumulativeCpuCycles);
}

void LatteDebug_ResetCumulativeCpuTime()
{
	s_cumulativeCpuCycles = 0;
}
