#include "BackendEmulated.h"

#include "Dimensions.h"
#include "Infinity.h"
#include "Keyboard.h"
#include "Skylander.h"
#include "config/CemuConfig.h"
#include "SkylanderXbox360.h"

namespace nsyshid::backend::emulated
{
	BackendEmulated::BackendEmulated()
	{
		cemuLog_logDebug(LogType::NSYSHID, "nsyshid::BackendEmulated: emulated backend initialised");
	}

	BackendEmulated::~BackendEmulated() = default;

	bool BackendEmulated::IsInitialisedOk()
	{
		return true;
	}

	void BackendEmulated::AttachVisibleDevices()
	{
		if (GetConfig().emulated_usb_devices.emulate_skylander_portal && !FindDeviceById(0x1430, 0x0150))
		{
			cemuLog_logDebug(LogType::NSYSHID, "Attaching Emulated Portal");
			// Add Skylander Portal
			auto device = std::make_shared<SkylanderPortalDevice>();
			AttachDevice(device);
		}
	#ifdef HAS_LIBUSB
		else if (auto usb_portal = FindDeviceById(0x1430, 0x1F17))
		{
			cemuLog_logDebug(LogType::NSYSHID, "Attaching Xbox 360 Portal");
			// Add Skylander Xbox 360 Portal
			auto device = std::make_shared<SkylanderXbox360PortalLibusb>(usb_portal);
			AttachDevice(device);
		}
	#endif
		if (GetConfig().emulated_usb_devices.emulate_infinity_base && !FindDeviceById(0x0E6F, 0x0129))
		{
			cemuLog_logDebug(LogType::NSYSHID, "Attaching Emulated Base");
			// Add Infinity Base
			auto device = std::make_shared<InfinityBaseDevice>();
			AttachDevice(device);
		}
		if (GetConfig().emulated_usb_devices.emulate_dimensions_toypad && !FindDeviceById(0x0E6F, 0x0241))
		{
			cemuLog_logDebug(LogType::NSYSHID, "Attaching Emulated Toypad");
			// Add Dimensions Toypad
			auto device = std::make_shared<DimensionsToypadDevice>();
			AttachDevice(device);
		}
		UpdateVisibleDevices();
	}

	// the keyboard can be connected and disconnected while a title is running
	void BackendEmulated::UpdateVisibleDevices()
	{
		auto keyboard = FindDevice([](const std::shared_ptr<Device>& device) {
			return dynamic_cast<KeyboardDevice*>(device.get()) != nullptr;
		});
		const bool emulateKeyboard = GetConfig().emulated_usb_devices.emulate_keyboard;
		if (emulateKeyboard && !keyboard)
		{
			cemuLog_log(LogType::NSYSHID, "Attaching emulated keyboard");
			AttachDevice(std::make_shared<KeyboardDevice>());
		}
		else if (!emulateKeyboard && keyboard)
		{
			cemuLog_log(LogType::NSYSHID, "Detaching emulated keyboard");
			DetachDevice(keyboard);
		}
	}
} // namespace nsyshid::backend::emulated