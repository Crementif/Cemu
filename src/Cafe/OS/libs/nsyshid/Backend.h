#pragma once

#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>

#include "Common/precompiled.h"

namespace nsyshid
{
	typedef struct
	{
		/* +0x00 */ uint32be handle;
		/* +0x04 */ uint32 ukn04;
		/* +0x08 */ uint16 vendorId;  // little-endian ?
		/* +0x0A */ uint16 productId; // little-endian ?
		/* +0x0C */ uint8 ifIndex;
		/* +0x0D */ uint8 subClass;
		/* +0x0E */ uint8 protocol;
		/* +0x0F */ uint8 paddingGuessed0F;
		/* +0x10 */ uint16be maxPacketSizeRX;
		/* +0x12 */ uint16be maxPacketSizeTX;
	} HID_t;

	struct TransferCommand
	{
		uint8* data;
		uint32 length;

		TransferCommand(uint8* data, uint32 length)
			: data(data), length(length)
		{
		}
		virtual ~TransferCommand() = default;
	};

	struct ReadMessage final : TransferCommand
	{
		sint32 bytesRead;

		ReadMessage(uint8* data, uint32 length, sint32 bytesRead)
			: bytesRead(bytesRead), TransferCommand(data, length)
		{
		}
		using TransferCommand::TransferCommand;
	};

	struct WriteMessage final : TransferCommand
	{
		sint32 bytesWritten;

		WriteMessage(uint8* data, uint32 length, sint32 bytesWritten)
			: bytesWritten(bytesWritten), TransferCommand(data, length)
		{
		}
		using TransferCommand::TransferCommand;
	};

	struct ReportMessage final : TransferCommand
	{
		uint8 reportType;
		uint8 reportId;

		ReportMessage(uint8 reportType, uint8 reportId, uint8* data, uint32 length)
			: reportType(reportType), reportId(reportId), TransferCommand(data, length)
		{
		}
		using TransferCommand::TransferCommand;
	};

	static_assert(offsetof(HID_t, vendorId) == 0x8, "");
	static_assert(offsetof(HID_t, productId) == 0xA, "");
	static_assert(offsetof(HID_t, ifIndex) == 0xC, "");
	static_assert(offsetof(HID_t, protocol) == 0xE, "");

	// runs the transfers of one pipe in submission order on its own thread
	class TransferQueue
	{
	  public:
		explicit TransferQueue(const char* threadName);
		~TransferQueue();

		TransferQueue(const TransferQueue&) = delete;
		TransferQueue& operator=(const TransferQueue&) = delete;

		// returns false once the queue was stopped
		bool Push(std::function<void()> transfer);

		// runs the remaining transfers and joins the thread
		void Stop();

	  private:
		void ThreadFunc();

		const char* m_threadName;
		std::thread m_thread;
		std::mutex m_mutex;
		std::condition_variable m_condition;
		std::deque<std::function<void()>> m_transfers;
		bool m_stopped = false;
	};

	class Device
	{
	  public:
		Device() = delete;

		Device(uint16 vendorId,
			   uint16 productId,
			   uint8 interfaceIndex,
			   uint8 interfaceSubClass,
			   uint8 protocol);

		Device(const Device& device) = delete;

		Device& operator=(const Device& device) = delete;

		virtual ~Device() = default;

		HID_t* m_hid; // this info is passed to applications and must remain intact

		uint16 m_vendorId;
		uint16 m_productId;
		uint8 m_interfaceIndex;
		uint8 m_interfaceSubClass;
		uint8 m_protocol;
		uint16 m_maxPacketSizeRX;
		uint16 m_maxPacketSizeTX;

		// separate queues so a pending read doesn't hold back control or write transfers
		TransferQueue m_controlQueue;
		TransferQueue m_readQueue;
		TransferQueue m_writeQueue;

		virtual void AssignHID(HID_t* hid);

		virtual bool Open() = 0;

		virtual void Close() = 0;

		virtual bool IsOpened() = 0;

		enum class ReadResult
		{
			Success,
			Error,
			ErrorTimeout,
		};

		virtual ReadResult Read(ReadMessage* message) = 0;

		enum class WriteResult
		{
			Success,
			Error,
			ErrorTimeout,
		};

		virtual WriteResult Write(WriteMessage* message) = 0;

		virtual bool GetDescriptor(uint8 descType,
								   uint8 descIndex,
								   uint16 lang,
								   uint8* output,
								   uint32 outputMaxLength) = 0;

		virtual bool SetIdle(uint8 ifIndex,
							 uint8 reportId,
							 uint8 duration) = 0;

		virtual bool SetProtocol(uint8 ifIndex, uint8 protocol) = 0;

		virtual bool SetReport(ReportMessage* message) = 0;

		// the following requests are optional for HID devices, by default they are not supported
		virtual bool GetIdle(uint8 ifIndex, uint8 reportId, uint8* duration);

		virtual bool GetProtocol(uint8 ifIndex, uint8* protocol);

		virtual bool GetReport(ReportMessage* message);
	};

	class Backend
	{
	  public:
		Backend();

		Backend(const Backend& backend) = delete;

		Backend& operator=(const Backend& backend) = delete;

		virtual ~Backend() = default;

		void DetachAllDevices();

		// called from nsyshid when this backend is attached - do not call this yourself!
		void OnAttach();

		// called from nsyshid when this backend is detached - do not call this yourself!
		void OnDetach();

		bool IsBackendAttached();

		virtual bool IsInitialisedOk() = 0;

	  protected:
		// try to attach a device - only works if this backend is attached
		bool AttachDevice(const std::shared_ptr<Device>& device);

		void DetachDevice(const std::shared_ptr<Device>& device);

		std::shared_ptr<Device> FindDevice(std::function<bool(const std::shared_ptr<Device>&)> isWantedDevice);

		std::shared_ptr<Device> FindDeviceById(uint16 vendorId, uint16 productId);

		bool IsDeviceWhitelisted(uint16 vendorId, uint16 productId);

		// called from OnAttach() - attach devices that your backend can see here
		virtual void AttachVisibleDevices() = 0;

	  private:
		std::list<std::shared_ptr<Device>> m_devices;
		std::recursive_mutex m_devicesMutex;
		bool m_isAttached;
	};

	namespace backend
	{
		void AttachDefaultBackends();
	}
} // namespace nsyshid
