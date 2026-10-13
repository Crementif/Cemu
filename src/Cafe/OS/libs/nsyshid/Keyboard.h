#pragma once

#include "nsyshid.h"
#include "Backend.h"

namespace nsyshid
{
	constexpr uint16 KEYBOARD_VENDOR_ID = 0x04D9;
	constexpr uint16 KEYBOARD_PRODUCT_ID = 0x1503;

	// boot interface USB keyboard which is fed by the host keyboard
	class KeyboardDevice final : public Device
	{
	  public:
		KeyboardDevice();
		~KeyboardDevice() = default;

		bool Open() override;

		void Close() override;

		bool IsOpened() override;

		ReadResult Read(ReadMessage* message) override;

		WriteResult Write(WriteMessage* message) override;

		bool GetDescriptor(uint8 descType,
						   uint8 descIndex,
						   uint16 lang,
						   uint8* output,
						   uint32 outputMaxLength) override;

		bool SetIdle(uint8 ifIndex,
					 uint8 reportId,
					 uint8 duration) override;

		bool GetIdle(uint8 ifIndex,
					 uint8 reportId,
					 uint8* duration) override;

		bool SetProtocol(uint8 ifIndex, uint8 protocol) override;

		bool GetProtocol(uint8 ifIndex, uint8* protocol) override;

		bool SetReport(ReportMessage* message) override;

		bool GetReport(ReportMessage* message) override;

	  private:
		using BootReport = std::array<uint8, 8>;

		BootReport BuildReport() const;

		std::mutex m_mutex;
		std::condition_variable m_closeCondition;
		bool m_isOpened = false;
		std::bitset<256> m_keysHeld;
		uint8 m_idleDuration = 0;	 // in units of 4ms, 0 means reports are only sent when something changed
		uint8 m_reportProtocol = 1; // 0 = boot protocol, 1 = report protocol
		uint8 m_leds = 0;
		BootReport m_lastReport{};
		std::chrono::steady_clock::time_point m_lastReportTime{};
	};
} // namespace nsyshid
