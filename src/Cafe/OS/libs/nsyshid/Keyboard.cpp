#include "Keyboard.h"

#include "WindowSystem.h"

namespace nsyshid
{
	// boot keyboard report descriptor from the HID specification (appendix B.1)
	static constexpr uint8 s_reportDescriptor[] = {
		0x05, 0x01, // Usage Page (Generic Desktop)
		0x09, 0x06, // Usage (Keyboard)
		0xA1, 0x01, // Collection (Application)
		0x05, 0x07, //   Usage Page (Key Codes)
		0x19, 0xE0, //   Usage Minimum (224)
		0x29, 0xE7, //   Usage Maximum (231)
		0x15, 0x00, //   Logical Minimum (0)
		0x25, 0x01, //   Logical Maximum (1)
		0x75, 0x01, //   Report Size (1)
		0x95, 0x08, //   Report Count (8)
		0x81, 0x02, //   Input (Data, Variable, Absolute) - modifier byte
		0x95, 0x01, //   Report Count (1)
		0x75, 0x08, //   Report Size (8)
		0x81, 0x01, //   Input (Constant) - reserved byte
		0x95, 0x05, //   Report Count (5)
		0x75, 0x01, //   Report Size (1)
		0x05, 0x08, //   Usage Page (LEDs)
		0x19, 0x01, //   Usage Minimum (1)
		0x29, 0x05, //   Usage Maximum (5)
		0x91, 0x02, //   Output (Data, Variable, Absolute) - LED report
		0x95, 0x01, //   Report Count (1)
		0x75, 0x03, //   Report Size (3)
		0x91, 0x01, //   Output (Constant) - LED report padding
		0x95, 0x06, //   Report Count (6)
		0x75, 0x08, //   Report Size (8)
		0x15, 0x00, //   Logical Minimum (0)
		0x25, 0x65, //   Logical Maximum (101)
		0x05, 0x07, //   Usage Page (Key Codes)
		0x19, 0x00, //   Usage Minimum (0)
		0x29, 0x65, //   Usage Maximum (101)
		0x81, 0x00, //   Input (Data, Array) - key array
		0xC0,		// End Collection
	};

	static constexpr uint8 s_deviceDescriptor[] = {
		18,			// bLength
		0x01,		// bDescriptorType
		0x10, 0x01, // bcdUSB 1.10
		0x00,		// bDeviceClass (defined per interface)
		0x00,		// bDeviceSubClass
		0x00,		// bDeviceProtocol
		8,			// bMaxPacketSize0
		KEYBOARD_VENDOR_ID & 0xFF, KEYBOARD_VENDOR_ID >> 8,
		KEYBOARD_PRODUCT_ID & 0xFF, KEYBOARD_PRODUCT_ID >> 8,
		0x00, 0x01, // bcdDevice
		0,			// iManufacturer
		0,			// iProduct
		0,			// iSerialNumber
		1,			// bNumConfigurations
	};

	static constexpr uint8 s_configurationDescriptor[] = {
		// configuration descriptor
		9, 0x02, 34, 0, // bLength, bDescriptorType, wTotalLength
		1,				// bNumInterfaces
		1,				// bConfigurationValue
		0,				// iConfiguration
		0xA0,			// bmAttributes (bus powered, remote wakeup)
		50,				// MaxPower (100mA)
		// interface descriptor
		9, 0x04,
		0,	  // bInterfaceNumber
		0,	  // bAlternateSetting
		1,	  // bNumEndpoints
		0x03, // bInterfaceClass (HID)
		0x01, // bInterfaceSubClass (boot interface)
		0x01, // bInterfaceProtocol (keyboard)
		0,	  // iInterface
		// HID descriptor
		9, 0x21,
		0x11, 0x01, // bcdHID 1.11
		0x00,		// bCountryCode
		1,			// bNumDescriptors
		0x22,		// bDescriptorType (report)
		sizeof(s_reportDescriptor), 0,
		// endpoint descriptor
		7, 0x05,
		0x81, // bEndpointAddress (in, 1)
		0x03, // bmAttributes (interrupt)
		8, 0, // wMaxPacketSize
		8,	  // bInterval (8ms)
	};

	static_assert(sizeof(s_configurationDescriptor) == 34);

	constexpr uint8 HID_USAGE_ERROR_ROLLOVER = 0x01;
	constexpr uint8 HID_USAGE_FIRST_KEY = 0x04;
	constexpr uint8 HID_USAGE_FIRST_MODIFIER = 0xE0;

	constexpr uint8 HID_REPORT_TYPE_INPUT = 1;
	constexpr uint8 HID_REPORT_TYPE_OUTPUT = 2;

	constexpr auto POLLING_INTERVAL = std::chrono::milliseconds(8);

	KeyboardDevice::KeyboardDevice()
		: Device(KEYBOARD_VENDOR_ID, KEYBOARD_PRODUCT_ID, 0, 1, 1) // boot interface subclass, keyboard protocol
	{
		m_maxPacketSizeRX = 8;
		m_maxPacketSizeTX = 8;
	}

	bool KeyboardDevice::Open()
	{
		std::unique_lock lock(m_mutex);
		if (!m_isOpened)
		{
			m_isOpened = true;
			m_keysHeld = WindowSystem::GetWindowInfo().start_hidkeyevents();
			m_lastReport = {};
			m_lastReportTime = std::chrono::steady_clock::now();
		}
		return true;
	}

	void KeyboardDevice::Close()
	{
		{
			std::unique_lock lock(m_mutex);
			if (!m_isOpened)
				return;
			m_isOpened = false;
			WindowSystem::GetWindowInfo().stop_hidkeyevents();
		}
		m_closeCondition.notify_all();
	}

	bool KeyboardDevice::IsOpened()
	{
		std::unique_lock lock(m_mutex);
		return m_isOpened;
	}

	KeyboardDevice::BootReport KeyboardDevice::BuildReport() const
	{
		BootReport report{};
		for (uint32 i = 0; i < 8; i++)
		{
			if (m_keysHeld.test(HID_USAGE_FIRST_MODIFIER + i))
				report[0] |= (1 << i);
		}
		uint32 numKeys = 0;
		for (uint32 usage = HID_USAGE_FIRST_KEY; usage < HID_USAGE_FIRST_MODIFIER; usage++)
		{
			if (!m_keysHeld.test(usage))
				continue;
			if (numKeys == 6)
			{
				// too many keys for a boot report, every key slot reports a rollover error instead
				std::fill(report.begin() + 2, report.end(), HID_USAGE_ERROR_ROLLOVER);
				break;
			}
			report[2 + numKeys] = (uint8)usage;
			numKeys++;
		}
		return report;
	}

	// emulates the interrupt in endpoint
	Device::ReadResult KeyboardDevice::Read(ReadMessage* message)
	{
		std::unique_lock lock(m_mutex);
		m_closeCondition.wait_until(lock, m_lastReportTime + POLLING_INTERVAL, [this] { return !m_isOpened; });
		while (m_isOpened)
		{
			// key events are applied one at a time, so a key which is released before the next poll is still reported
			BootReport report = BuildReport();
			while (report == m_lastReport)
			{
				auto event = WindowSystem::GetWindowInfo().pop_hidkeyevent();
				if (!event)
					break;
				m_keysHeld.set(event->usage, event->pressed);
				report = BuildReport();
			}
			const auto now = std::chrono::steady_clock::now();
			const bool idleExpired = m_idleDuration != 0 && now - m_lastReportTime >= std::chrono::milliseconds(4 * m_idleDuration);
			if (report != m_lastReport || idleExpired)
			{
				m_lastReport = report;
				m_lastReportTime = now;
				const uint32 length = std::min<uint32>(message->length, report.size());
				memcpy(message->data, report.data(), length);
				message->bytesRead = length;
				return ReadResult::Success;
			}
			m_closeCondition.wait_for(lock, POLLING_INTERVAL);
		}
		return ReadResult::Error;
	}

	Device::WriteResult KeyboardDevice::Write(WriteMessage* message)
	{
		// boot keyboards have no interrupt out endpoint, the LEDs are set with SetReport
		return WriteResult::Error;
	}

	bool KeyboardDevice::GetDescriptor(uint8 descType,
									   uint8 descIndex,
									   uint16 lang,
									   uint8* output,
									   uint32 outputMaxLength)
	{
		std::span<const uint8> descriptor;
		switch (descType)
		{
		case 0x01: // device
			descriptor = s_deviceDescriptor;
			break;
		case 0x02: // configuration
			descriptor = s_configurationDescriptor;
			break;
		case 0x22: // report
			descriptor = s_reportDescriptor;
			break;
		default:
			cemuLog_log(LogType::NSYSHID, "nsyshid::KeyboardDevice::GetDescriptor(): unsupported descriptor type 0x{:02x}", descType);
			return false;
		}
		memcpy(output, descriptor.data(), std::min<uint32>(outputMaxLength, descriptor.size()));
		return true;
	}

	bool KeyboardDevice::SetIdle(uint8 ifIndex,
								 uint8 reportId,
								 uint8 duration)
	{
		std::unique_lock lock(m_mutex);
		m_idleDuration = duration;
		return true;
	}

	bool KeyboardDevice::GetIdle(uint8 ifIndex,
								 uint8 reportId,
								 uint8* duration)
	{
		std::unique_lock lock(m_mutex);
		*duration = m_idleDuration;
		return true;
	}

	bool KeyboardDevice::SetProtocol(uint8 ifIndex, uint8 protocol)
	{
		// the report descriptor matches the boot report layout, so both protocols produce the same reports
		std::unique_lock lock(m_mutex);
		m_reportProtocol = protocol;
		return true;
	}

	bool KeyboardDevice::GetProtocol(uint8 ifIndex, uint8* protocol)
	{
		std::unique_lock lock(m_mutex);
		*protocol = m_reportProtocol;
		return true;
	}

	bool KeyboardDevice::SetReport(ReportMessage* message)
	{
		// bit 0 = num lock, bit 1 = caps lock, bit 2 = scroll lock, bit 3 = compose, bit 4 = kana
		if (message->reportType != HID_REPORT_TYPE_OUTPUT || message->length < 1)
			return false;
		std::unique_lock lock(m_mutex);
		m_leds = message->data[0];
		return true;
	}

	bool KeyboardDevice::GetReport(ReportMessage* message)
	{
		std::unique_lock lock(m_mutex);
		if (message->reportType == HID_REPORT_TYPE_INPUT)
		{
			const BootReport report = BuildReport();
			memcpy(message->data, report.data(), std::min<uint32>(message->length, report.size()));
			return true;
		}
		if (message->reportType == HID_REPORT_TYPE_OUTPUT && message->length >= 1)
		{
			message->data[0] = m_leds;
			return true;
		}
		return false;
	}
} // namespace nsyshid
