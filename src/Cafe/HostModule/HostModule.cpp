#include "Cafe/HostModule/HostModule.h"
#include "Cafe/CafeSystem.h"
#include "Cafe/OS/common/OSCommon.h"
#include "Cafe/HW/MMU/MMU.h"
#include "config/ActiveSettings.h"
#include "config/CemuConfig.h"
#include "config/LaunchSettings.h"
#include "Cafe/GraphicPack/GraphicPack2.h"
#include "WindowSystem.h"
#include "Cafe/HW/Latte/Core/LatteView.h"
#include "Cafe/HW/Latte/Core/LatteTiming.h"

#ifdef ENABLE_VULKAN
#include "Cafe/HW/Latte/Renderer/Vulkan/VulkanAPI.h"
#endif

namespace HostModule
{
	static std::vector<Module*> s_modules;
	static std::vector<std::pair<fs::path, PackOrder>> s_graphicPacks;
	static std::vector<fs::path> s_bundledGraphicPackDirectories;
	static std::atomic<float> s_windowCropAspectRatio = 0.0f;

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
		const fs::path settingsFile = ActiveSettings::GetConfigPath(FORK_SETTINGS_FILE);
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

	void ShowLinkDialog(const char* title, const char* message, const char* linkLabel, const char* url)
	{
		WindowSystem::ShowLinkDialog(message, title, linkLabel, url);
	}

	void RegisterHLEFunction(const char* libraryName, const char* functionName, HLEFunction function)
	{
		osLib_addFunctionInternal(libraryName, functionName, function);
	}

	void RegisterGraphicPack(const fs::path& path, PackOrder order)
	{
		s_graphicPacks.emplace_back(path, order);
	}

	void RegisterBundledGraphicPacks(const fs::path& directory)
	{
		s_bundledGraphicPackDirectories.emplace_back(directory);
	}

	static std::shared_ptr<GraphicPack2> FindGraphicPack(const fs::path& path)
	{
		for (const auto& pack : GraphicPack2::GetGraphicPacks())
		{
			std::error_code error;
			if (fs::equivalent(pack->GetRulesPath().parent_path(), path, error))
				return pack;
		}
		return nullptr;
	}

	std::vector<std::string> GetGraphicPackPresets(const fs::path& path, std::string_view category)
	{
		std::vector<std::string> names;
		if (std::shared_ptr<GraphicPack2> pack = FindGraphicPack(path))
		{
			for (const auto& preset : pack->GetPresets())
			{
				if (preset->category == category && pack->IsPresetVisible(preset))
					names.emplace_back(preset->name);
			}
		}
		return names;
	}

	std::string GetGraphicPackPreset(const fs::path& path, std::string_view category)
	{
		std::shared_ptr<GraphicPack2> pack = FindGraphicPack(path);
		return pack ? pack->GetActivePreset(category) : std::string();
	}

	bool SetGraphicPackPreset(const fs::path& path, std::string_view category, std::string_view preset)
	{
		std::shared_ptr<GraphicPack2> pack = FindGraphicPack(path);
		if (!pack || !pack->SetActivePreset(category, preset))
			return false;
		// a preset can change what the other categories show
		auto& entry = GetConfigHandle().data().graphic_pack_entries[_utf8ToPath(pack->GetNormalizedPathString())];
		for (const auto& active : pack->GetActivePresets())
			entry.insert_or_assign(active->category, active->name);
		GetConfigHandle().Save();
		return true;
	}

	void UseMultiviewStereo()
	{
		LatteView_SetMultiviewRequested(true);
	}

	void UseFoveatedShading(uint32_t guestWidth, uint32_t guestHeight, float innerRadius, float outerRadius)
	{
		LatteView_RequestFoveation({ guestWidth, guestHeight, innerRadius, outerRadius });
	}

	void SetFoveationCentres(const float left[2], const float right[2])
	{
		LatteView_SetFoveationCentres(left, right);
	}

	void SetFoveationRadii(float innerRadius, float outerRadius)
	{
		LatteView_SetFoveationRadii(innerRadius, outerRadius);
	}

	void SetFoveatedShadingActive(bool active)
	{
		LatteView_SetFoveationActive(active);
	}

	void SetVsyncFrequency(int32_t frequency)
	{
		LatteTiming_setHostModuleVsyncFrequency(frequency);
	}

	void BootTitleOnStart(uint64_t titleId)
	{
		if (!LaunchSettings::GetLoadFile() && !LaunchSettings::GetLoadTitleID())
			LaunchSettings::SetLoadTitleID(titleId);
	}

	void CropWindowToAspectRatio(float aspectRatio)
	{
		s_windowCropAspectRatio.store(aspectRatio, std::memory_order_relaxed);
	}

	float GetWindowCropAspectRatio()
	{
		return s_windowCropAspectRatio.load(std::memory_order_relaxed);
	}

	const std::vector<std::pair<fs::path, PackOrder>>& GetGraphicPacks()
	{
		return s_graphicPacks;
	}

	const std::vector<fs::path>& GetBundledGraphicPackDirectories()
	{
		return s_bundledGraphicPackDirectories;
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
