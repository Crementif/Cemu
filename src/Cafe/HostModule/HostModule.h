#pragma once

// modules get compiled in through CEMU_EXTERNAL_MODULES

#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>

#ifdef ENABLE_VULKAN
#include <vulkan/vulkan_core.h>
#endif

struct PPCInterpreter_t;

namespace HostModule
{
	using HLEFunction = void(*)(PPCInterpreter_t* hCPU);

	enum class FrameKind : uint32_t
	{
		Guest,
		ShaderCacheLoading,
	};

	enum class PackOrder : uint32_t
	{
		BeforeUser, // before the user's packs
		AfterUser, // after the user's packs
	};

#ifdef ENABLE_VULKAN
	struct VulkanDevice
	{
		PFN_vkGetInstanceProcAddr getInstanceProcAddr;
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		uint32_t graphicsQueueFamily; // Cemu submits on queue 0 of this family
	};

	struct EventSurface
	{
		VkImage image;
		VkImageLayout layout; // has to be left in this layout
		VkImageAspectFlags aspectMask;
		VkFormat format;
		VkExtent2D extent;
		uint32_t sliceIndex;
	};

	struct HostEvent
	{
		uint32_t id;
		uint32_t params[4];
		const EventSurface* surface;
		const EventSurface* rightSurface; // null if it's the same as surface
	};

	struct SubmitSemaphore
	{
		VkSemaphore semaphore;
		uint64_t value; // ignored for binary semaphores
	};
#endif

	class Module
	{
	public:
		virtual ~Module() = default;
		virtual const char* GetName() const = 0;

		// write settings.xml here to skip the Getting Started dialog
		virtual void OnBeforeConfigLoad(const std::filesystem::path& settingsFile) {}

		virtual void OnInitialize() {}

		// return false to let Cemu do it
#ifdef ENABLE_VULKAN
		virtual bool CreateVkInstance(const VkInstanceCreateInfo& createInfo, VkInstance& instance, VkResult& result) { return false; }
		virtual VkPhysicalDevice SelectPhysicalDevice(VkInstance instance) { return VK_NULL_HANDLE; }
		virtual bool CreateVkDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo& createInfo, VkDevice& device, VkResult& result) { return false; }
		virtual void OnVkDeviceCreated(const VulkanDevice& device) {}
		virtual void OnVkDeviceDestroying() {}

		virtual void OnHostEvent(const HostEvent& event, VkCommandBuffer commandBuffer) {}

		virtual void OnScanBuffer(const EventSurface& surface, bool isPadView, VkCommandBuffer commandBuffer) {}
#endif

		virtual void OnFrameSubmitted(FrameKind kind) {}
	};

	void Register(Module* module);
	bool HasModules();
	const std::vector<Module*>& GetModules();

	uint64_t GetTitleId();
	uint8_t* GetGuestMemoryBase();
	std::filesystem::path GetUserDataPath();
	void ShowErrorDialog(const char* title, const char* message);
	void RegisterHLEFunction(const char* libraryName, const char* functionName, HLEFunction function);
	void RegisterGraphicPack(const std::filesystem::path& path, PackOrder order);
	const std::vector<std::pair<std::filesystem::path, PackOrder>>& GetGraphicPacks();
	void RegisterBundledGraphicPacks(const std::filesystem::path& directory);
	const std::vector<std::filesystem::path>& GetBundledGraphicPackDirectories();
#ifdef ENABLE_VULKAN
	PFN_vkGetInstanceProcAddr GetVkInstanceProcAddr();
	PFN_vkGetDeviceProcAddr GetVkDeviceProcAddr();
	// only call this from OnHostEvent or OnScanBuffer
	void AttachToNextSubmit(const SubmitSemaphore& wait, const SubmitSemaphore& signal);
	void TakeSubmitSemaphores(std::vector<SubmitSemaphore>& waits, std::vector<SubmitSemaphore>& signals);
#endif

	void UseMultiviewStereo();

	// sizes are relative to the target height, full rate inside innerRadius and quarter rate past outerRadius
	void UseFoveatedShading(uint32_t guestWidth, uint32_t guestHeight, float innerRadius, float outerRadius);
	// 0,0 is the top left, 1,1 the bottom right
	void SetFoveationCentres(const float left[2], const float right[2]);
	void SetFoveationRadii(float innerRadius, float outerRadius);
	void SetFoveatedShadingActive(bool active);

	void RegisterExternalModules();
	void Startup();
	void BeforeConfigLoad();
	void Initialize();
	void NotifyFrameSubmitted(FrameKind kind);
}
