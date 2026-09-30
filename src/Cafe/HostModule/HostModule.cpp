#include "Cafe/HostModule/HostModule.h"
#include "Cafe/CafeSystem.h"
#include "Cafe/OS/common/OSCommon.h"
#include "Cafe/HW/MMU/MMU.h"
#include "config/ActiveSettings.h"
#include "WindowSystem.h"

#ifdef ENABLE_VULKAN
#include "Cafe/HW/Latte/Renderer/Vulkan/VulkanAPI.h"
#endif

namespace HostModule
{
	static std::vector<Module*> s_modules;
	static std::vector<std::pair<fs::path, PackOrder>> s_graphicPacks;

#ifdef ENABLE_VULKAN
	static std::mutex s_submitMutex;
	static std::vector<SubmitSemaphore> s_pendingWaits;
	static std::vector<SubmitSemaphore> s_pendingSignals;
#endif

	void Register(Module* module)
	{
		cemuLog_log(LogType::Force, "Registered host module: {}", module->GetName());
		s_modules.emplace_back(module);
	}

	void Startup()
	{
		RegisterExternalModules();
	}

	void BeforeConfigLoad()
	{
		const fs::path settingsFile = ActiveSettings::GetConfigPath("settings.xml");
		for (auto& module : s_modules)
			module->OnBeforeConfigLoad(settingsFile);
	}

	void Initialize()
	{
		for (auto& module : s_modules)
			module->OnInitialize();
	}

	bool HasModules()
	{
		return !s_modules.empty();
	}

	const std::vector<Module*>& GetModules()
	{
		return s_modules;
	}

	uint64 GetTitleId()
	{
		return CafeSystem::GetForegroundTitleId();
	}

	uint8* GetGuestMemoryBase()
	{
		return memory_base;
	}

	fs::path GetUserDataPath()
	{
		return ActiveSettings::GetUserDataPath();
	}

	void ShowErrorDialog(const char* title, const char* message)
	{
		WindowSystem::ShowErrorDialog(message, title);
	}

	void RegisterHLEFunction(const char* libraryName, const char* functionName, HLEFunction function)
	{
		osLib_addFunctionInternal(libraryName, functionName, function);
	}

	void RegisterGraphicPack(const fs::path& path, PackOrder order)
	{
		s_graphicPacks.emplace_back(path, order);
	}

	const std::vector<std::pair<fs::path, PackOrder>>& GetGraphicPacks()
	{
		return s_graphicPacks;
	}

#ifdef ENABLE_VULKAN
	PFN_vkGetInstanceProcAddr GetVkInstanceProcAddr()
	{
		return vkGetInstanceProcAddr;
	}

	PFN_vkGetDeviceProcAddr GetVkDeviceProcAddr()
	{
		return vkGetDeviceProcAddr;
	}

	// don't wait on a value the same submit signals, keep the earliest wait and the latest signal
	static void MergeSubmitSemaphore(std::vector<SubmitSemaphore>& pending, const SubmitSemaphore& semaphore, bool keepLatest)
	{
		if (semaphore.semaphore == VK_NULL_HANDLE)
			return;
		for (auto& it : pending)
		{
			if (it.semaphore == semaphore.semaphore)
			{
				it.value = keepLatest ? std::max(it.value, semaphore.value) : std::min(it.value, semaphore.value);
				return;
			}
		}
		pending.emplace_back(semaphore);
	}

	void AttachToNextSubmit(const SubmitSemaphore& wait, const SubmitSemaphore& signal)
	{
		std::unique_lock _l(s_submitMutex);
		MergeSubmitSemaphore(s_pendingWaits, wait, false);
		MergeSubmitSemaphore(s_pendingSignals, signal, true);
	}

	void TakeSubmitSemaphores(std::vector<SubmitSemaphore>& waits, std::vector<SubmitSemaphore>& signals)
	{
		std::unique_lock _l(s_submitMutex);
		waits.swap(s_pendingWaits);
		signals.swap(s_pendingSignals);
		s_pendingWaits.clear();
		s_pendingSignals.clear();
	}
#endif

	void NotifyFrameSubmitted(FrameKind kind)
	{
		for (auto& module : s_modules)
			module->OnFrameSubmitted(kind);
	}
}
