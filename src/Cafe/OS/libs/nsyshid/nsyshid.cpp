#include "Cafe/OS/common/OSCommon.h"
#include "Cafe/HW/Espresso/PPCCallback.h"
#include <bitset>
#include <mutex>
#include "nsyshid.h"
#include "Cafe/OS/libs/coreinit/coreinit_Thread.h"
#include "Backend.h"
#include "Whitelist.h"
#include "util/helpers/helpers.h"

namespace nsyshid
{

	std::list<std::shared_ptr<Backend>> backendList;

	std::list<std::shared_ptr<Device>> deviceList;

	std::list<HIDClient_t*> HIDClientList;

	std::recursive_mutex hidMutex;

	void AttachClientToList(HIDClient_t* hidClient)
	{
		std::lock_guard<std::recursive_mutex> lock(hidMutex);
		// todo - append at the beginning or end of the list? List order matters because it also controls the order in which attach callbacks are called
		HIDClientList.push_front(hidClient);
	}

	void DetachClientFromList(HIDClient_t* hidClient)
	{
		std::lock_guard<std::recursive_mutex> lock(hidMutex);
		HIDClientList.remove(hidClient);
	}

	std::shared_ptr<Device> GetDeviceByHandle(uint32 handle, bool openIfClosed = false)
	{
		std::shared_ptr<Device> device;
		{
			std::lock_guard<std::recursive_mutex> lock(hidMutex);
			for (const auto& d : deviceList)
			{
				if (d->m_hid->handle == handle)
				{
					device = d;
					break;
				}
			}
		}
		if (device != nullptr)
		{
			if (openIfClosed && !device->IsOpened())
			{
				if (!device->Open())
				{
					return nullptr;
				}
			}
			return device;
		}
		return nullptr;
	}

	uint32 _lastGeneratedHidHandle = 1;

	uint32 GenerateHIDHandle()
	{
		std::lock_guard<std::recursive_mutex> lock(hidMutex);
		_lastGeneratedHidHandle++;
		return _lastGeneratedHidHandle;
	}

	const int HID_MAX_NUM_DEVICES = 128;

	SysAllocator<HID_t, HID_MAX_NUM_DEVICES> HIDPool;
	std::queue<size_t> HIDPoolIndexQueue;

	void InitHIDPoolIndexQueue()
	{
		static bool HIDPoolIndexQueueInitialized = false;
		std::lock_guard<std::recursive_mutex> lock(hidMutex);
		if (HIDPoolIndexQueueInitialized)
		{
			return;
		}
		HIDPoolIndexQueueInitialized = true;
		for (size_t i = 0; i < HID_MAX_NUM_DEVICES; i++)
		{
			HIDPoolIndexQueue.push(i);
		}
	}

	HID_t* GetFreeHID()
	{
		std::lock_guard<std::recursive_mutex> lock(hidMutex);
		InitHIDPoolIndexQueue();
		if (HIDPoolIndexQueue.empty())
		{
			return nullptr;
		}
		size_t index = HIDPoolIndexQueue.front();
		HIDPoolIndexQueue.pop();
		return HIDPool.GetPtr() + index;
	}

	void ReleaseHID(HID_t* device)
	{
		// this should never happen, but having a safeguard can't hurt
		if (device == nullptr)
		{
			cemu_assert_error();
		}
		std::lock_guard<std::recursive_mutex> lock(hidMutex);
		InitHIDPoolIndexQueue();
		size_t index = device - HIDPool.GetPtr();
		HIDPoolIndexQueue.push(index);
	}

	const int HID_CALLBACK_DETACH = 0;
	const int HID_CALLBACK_ATTACH = 1;

	uint32 DoAttachCallback(HIDClient_t* hidClient, const std::shared_ptr<Device>& device)
	{
		return PPCCoreCallback(hidClient->callbackFunc, memory_getVirtualOffsetFromPointer(hidClient),
							   memory_getVirtualOffsetFromPointer(device->m_hid), HID_CALLBACK_ATTACH);
	}

	void DoAttachCallbackAsync(HIDClient_t* hidClient, const std::shared_ptr<Device>& device)
	{
		coreinitAsyncCallback_add(hidClient->callbackFunc, 3, memory_getVirtualOffsetFromPointer(hidClient),
								  memory_getVirtualOffsetFromPointer(device->m_hid), HID_CALLBACK_ATTACH);
	}

	void DoDetachCallback(HIDClient_t* hidClient, const std::shared_ptr<Device>& device)
	{
		PPCCoreCallback(hidClient->callbackFunc, memory_getVirtualOffsetFromPointer(hidClient),
						memory_getVirtualOffsetFromPointer(device->m_hid), HID_CALLBACK_DETACH);
	}

	void DoDetachCallbackAsync(HIDClient_t* hidClient, const std::shared_ptr<Device>& device)
	{
		coreinitAsyncCallback_add(hidClient->callbackFunc, 3, memory_getVirtualOffsetFromPointer(hidClient),
								  memory_getVirtualOffsetFromPointer(device->m_hid), HID_CALLBACK_DETACH);
	}

	void AttachBackend(const std::shared_ptr<Backend>& backend)
	{
		{
			std::lock_guard<std::recursive_mutex> lock(hidMutex);
			backendList.push_back(backend);
		}
		backend->OnAttach();
	}

	void DetachBackend(const std::shared_ptr<Backend>& backend)
	{
		{
			std::lock_guard<std::recursive_mutex> lock(hidMutex);
			backendList.remove(backend);
		}
		backend->OnDetach();
	}

	void DetachAllBackends()
	{
		std::list<std::shared_ptr<Backend>> backendListCopy;
		{
			std::lock_guard<std::recursive_mutex> lock(hidMutex);
			backendListCopy = backendList;
			backendList.clear();
		}
		for (const auto& backend : backendListCopy)
		{
			backend->OnDetach();
		}
	}

	void UpdateEmulatedDevices()
	{
		std::list<std::shared_ptr<Backend>> backendListCopy;
		{
			std::lock_guard<std::recursive_mutex> lock(hidMutex);
			backendListCopy = backendList;
		}
		for (const auto& backend : backendListCopy)
		{
			backend->OnSettingsChanged();
		}
	}

	void AttachDefaultBackends()
	{
		backend::AttachDefaultBackends();
	}

	bool AttachDevice(const std::shared_ptr<Device>& device)
	{
		std::lock_guard<std::recursive_mutex> lock(hidMutex);

		// is the device already attached?
		{
			auto it = std::find(deviceList.begin(), deviceList.end(), device);
			if (it != deviceList.end())
			{
				cemuLog_logDebug(LogType::NSYSHID,
								 "nsyshid.AttachDevice(): failed to attach device: {:04x}:{:04x}: already attached",
								 device->m_vendorId,
								 device->m_productId);
				return false;
			}
		}

		HID_t* hidDevice = GetFreeHID();
		if (hidDevice == nullptr)
		{
			cemuLog_logDebug(LogType::NSYSHID,
							 "nsyshid.AttachDevice(): failed to attach device: {:04x}:{:04x}: no free device slots left",
							 device->m_vendorId,
							 device->m_productId);
			return false;
		}
		hidDevice->handle = GenerateHIDHandle();
		device->AssignHID(hidDevice);
		deviceList.push_back(device);

		// do attach callbacks
		for (auto client : HIDClientList)
		{
			DoAttachCallbackAsync(client, device);
		}

		cemuLog_logDebug(LogType::NSYSHID, "nsyshid.AttachDevice(): device attached: {:04x}:{:04x}",
						 device->m_vendorId,
						 device->m_productId);
		return true;
	}

	void DetachDevice(const std::shared_ptr<Device>& device)
	{
		{
			std::lock_guard<std::recursive_mutex> lock(hidMutex);

			// remove from list
			auto it = std::find(deviceList.begin(), deviceList.end(), device);
			if (it == deviceList.end())
			{
				cemuLog_logDebug(LogType::NSYSHID, "nsyshid.DetachDevice(): device not found: {:04x}:{:04x}",
								 device->m_vendorId,
								 device->m_productId);
				return;
			}
			deviceList.erase(it);

			// do detach callbacks
			for (auto client : HIDClientList)
			{
				DoDetachCallbackAsync(client, device);
			}
			ReleaseHID(device->m_hid);
		}

		device->Close();
		device->m_controlQueue.Stop();
		device->m_readQueue.Stop();
		device->m_writeQueue.Stop();

		cemuLog_logDebug(LogType::NSYSHID, "nsyshid.DetachDevice(): device removed: {:04x}:{:04x}",
						 device->m_vendorId,
						 device->m_productId);
	}

	std::shared_ptr<Device> FindDeviceById(uint16 vendorId, uint16 productId)
	{
		std::lock_guard<std::recursive_mutex> lock(hidMutex);
		for (const auto& device : deviceList)
		{
			if (device->m_vendorId == vendorId && device->m_productId == productId)
			{
				return device;
			}
		}
		return nullptr;
	}

	sint32 HIDAddClient(HIDClient_t* hidClient, MPTR callbackFuncMPTR)
	{
		hidClient->callbackFunc = callbackFuncMPTR;

		std::list<std::shared_ptr<Device>> devices;
		{
			std::lock_guard<std::recursive_mutex> lock(hidMutex);
			AttachClientToList(hidClient);
			devices = deviceList;
		}

		// callbacks may issue blocking requests, so they are called without holding hidMutex
		for (const auto& device : devices)
		{
			DoAttachCallback(hidClient, device);
		}

		return 0;
	}

	sint32 HIDDelClient(HIDClient_t* hidClient)
	{
		std::list<std::shared_ptr<Device>> devices;
		{
			std::lock_guard<std::recursive_mutex> lock(hidMutex);
			DetachClientFromList(hidClient);
			devices = deviceList;
		}

		for (const auto& device : devices)
		{
			DoDetachCallback(hidClient, device);
		}

		return 0;
	}

	// passive clients are used by system libraries like nsyskbd
	// todo - on hardware a device claimed by a regular client is probably not handed to passive clients
	sint32 HIDAddPassiveClient(HIDClient_t* hidClient, MPTR callbackFuncMPTR)
	{
		return HIDAddClient(hidClient, callbackFuncMPTR);
	}

	sint32 HIDDelPassiveClient(HIDClient_t* hidClient)
	{
		return HIDDelClient(hidClient);
	}

	void DoHIDTransferCallback(MPTR callbackFuncMPTR, MPTR callbackParamMPTR, uint32 hidHandle, uint32 errorCode,
							   MPTR buffer, sint32 length)
	{
		coreinitAsyncCallback_add(callbackFuncMPTR, 5, hidHandle, errorCode, buffer, length, callbackParamMPTR);
	}

	// transfer returns the number of transferred bytes or a negative error code
	// without a callback the calling thread blocks until the transfer is done
	sint32 SubmitTransfer(TransferQueue& queue, uint32 hidHandle, uint8* buffer, MPTR callbackFuncMPTR, MPTR callbackParamMPTR, std::function<sint32()> transfer)
	{
		if (callbackFuncMPTR != MPTR_NULL)
		{
			MPTR bufferMPTR = buffer ? memory_getVirtualOffsetFromPointer(buffer) : MPTR_NULL;
			bool queued = queue.Push([=]() {
				sint32 result = transfer();
				DoHIDTransferCallback(callbackFuncMPTR, callbackParamMPTR, hidHandle, std::min(result, 0), bufferMPTR, std::max(result, 0));
			});
			return queued ? 0 : -1;
		}
		StackAllocator<coreinit::OSEvent> event;
		coreinit::OSInitEvent(&event, coreinit::OSEvent::EVENT_STATE::STATE_NOT_SIGNALED, coreinit::OSEvent::EVENT_MODE::MODE_AUTO);
		coreinit::OSEvent* eventPtr = &event;
		sint32 result = -1;
		bool queued = queue.Push([&result, &transfer, eventPtr]() {
			result = transfer();
			coreinit::OSSignalEvent(eventPtr);
		});
		if (!queued)
			return -1;
		coreinit::OSWaitEvent(&event);
		return result;
	}

	sint32 HIDGetDescriptor(uint32 hidHandle, uint8 descType, uint8 descIndex, uint16 lang, uint8* output, uint32 outputMaxLength, MPTR cbFuncMPTR, MPTR cbParamMPTR)
	{
		std::shared_ptr<Device> device = GetDeviceByHandle(hidHandle, true);
		if (device == nullptr)
		{
			cemuLog_log(LogType::NSYSHID, "nsyshid.HIDGetDescriptor(): Unable to find device with hid handle {}", hidHandle);
			return -1;
		}
		Device* dev = device.get();
		return SubmitTransfer(dev->m_controlQueue, hidHandle, output, cbFuncMPTR, cbParamMPTR, [=]() -> sint32 {
			if (!dev->GetDescriptor(descType, descIndex, lang, output, outputMaxLength))
				return -1;
			return outputMaxLength;
		});
	}

	sint32 HIDSetIdle(uint32 hidHandle, uint8 ifIndex, uint8 reportId, uint8 duration, MPTR callbackFuncMPTR, MPTR callbackParamMPTR)
	{
		std::shared_ptr<Device> device = GetDeviceByHandle(hidHandle, true);
		if (device == nullptr)
		{
			cemuLog_log(LogType::NSYSHID, "nsyshid.HIDSetIdle(): Unable to find device with hid handle {}", hidHandle);
			return -1;
		}
		Device* dev = device.get();
		return SubmitTransfer(dev->m_controlQueue, hidHandle, nullptr, callbackFuncMPTR, callbackParamMPTR, [=]() -> sint32 {
			return dev->SetIdle(ifIndex, reportId, duration) ? 0 : -1;
		});
	}

	sint32 HIDGetIdle(uint32 hidHandle, uint8 ifIndex, uint8 reportId, uint8* duration, MPTR callbackFuncMPTR, MPTR callbackParamMPTR)
	{
		std::shared_ptr<Device> device = GetDeviceByHandle(hidHandle, true);
		if (device == nullptr)
		{
			cemuLog_log(LogType::NSYSHID, "nsyshid.HIDGetIdle(): Unable to find device with hid handle {}", hidHandle);
			return -1;
		}
		Device* dev = device.get();
		return SubmitTransfer(dev->m_controlQueue, hidHandle, duration, callbackFuncMPTR, callbackParamMPTR, [=]() -> sint32 {
			return dev->GetIdle(ifIndex, reportId, duration) ? 1 : -1;
		});
	}

	sint32 HIDGetProtocol(uint32 hidHandle, uint8 ifIndex, uint8* protocol, MPTR callbackFuncMPTR, MPTR callbackParamMPTR)
	{
		std::shared_ptr<Device> device = GetDeviceByHandle(hidHandle, true);
		if (device == nullptr)
		{
			cemuLog_log(LogType::NSYSHID, "nsyshid.HIDGetProtocol(): Unable to find device with hid handle {}", hidHandle);
			return -1;
		}
		Device* dev = device.get();
		return SubmitTransfer(dev->m_controlQueue, hidHandle, protocol, callbackFuncMPTR, callbackParamMPTR, [=]() -> sint32 {
			return dev->GetProtocol(ifIndex, protocol) ? 1 : -1;
		});
	}

	sint32 HIDSetProtocol(uint32 hidHandle, uint8 ifIndex, uint8 protocol, MPTR callbackFuncMPTR, MPTR callbackParamMPTR)
	{
		std::shared_ptr<Device> device = GetDeviceByHandle(hidHandle, true);
		if (device == nullptr)
		{
			cemuLog_log(LogType::NSYSHID, "nsyshid.HIDSetProtocol(): Unable to find device with hid handle {}", hidHandle);
			return -1;
		}
		Device* dev = device.get();
		return SubmitTransfer(dev->m_controlQueue, hidHandle, nullptr, callbackFuncMPTR, callbackParamMPTR, [=]() -> sint32 {
			return dev->SetProtocol(ifIndex, protocol) ? 0 : -1;
		});
	}

	sint32 HIDSetReport(uint32 hidHandle, uint8 reportType, uint8 reportId, uint8* data, uint32 dataLength, MPTR callbackFuncMPTR, MPTR callbackParamMPTR)
	{
		if (cemuLog_isLoggingEnabled(LogType::NSYSHID))
			cemuLog_logHexDump(LogType::NSYSHID, data, dataLength);

#ifdef CEMU_DEBUG_ASSERT
		if (reportType != 2 || reportId != 0)
			assert_dbg();
#endif

		std::shared_ptr<Device> device = GetDeviceByHandle(hidHandle, true);
		if (device == nullptr)
		{
			cemuLog_log(LogType::NSYSHID, "nsyshid.HIDSetReport(): Unable to find device with hid handle {}", hidHandle);
			return -1;
		}
		Device* dev = device.get();
		return SubmitTransfer(dev->m_controlQueue, hidHandle, data, callbackFuncMPTR, callbackParamMPTR, [=]() -> sint32 {
			ReportMessage message(reportType, reportId, data, dataLength);
			if (!dev->SetReport(&message))
				return -1;
			return dataLength;
		});
	}

	sint32 HIDGetReport(uint32 hidHandle, uint8 reportType, uint8 reportId, uint8* data, uint32 dataLength, MPTR callbackFuncMPTR, MPTR callbackParamMPTR)
	{
		std::shared_ptr<Device> device = GetDeviceByHandle(hidHandle, true);
		if (device == nullptr)
		{
			cemuLog_log(LogType::NSYSHID, "nsyshid.HIDGetReport(): Unable to find device with hid handle {}", hidHandle);
			return -1;
		}
		Device* dev = device.get();
		return SubmitTransfer(dev->m_controlQueue, hidHandle, data, callbackFuncMPTR, callbackParamMPTR, [=]() -> sint32 {
			ReportMessage message(reportType, reportId, data, dataLength);
			if (!dev->GetReport(&message))
				return -1;
			return dataLength;
		});
	}

	sint32 _hidReadInternalSync(Device* device, uint8* data, sint32 maxLength)
	{
		if (!device->IsOpened())
		{
			cemuLog_logDebug(LogType::NSYSHID, "nsyshid.hidReadInternalSync(): cannot read from a non-opened device");
			return -1;
		}
		memset(data, 0, maxLength);
		ReadMessage message(data, maxLength, 0);
		Device::ReadResult readResult = device->Read(&message);
		switch (readResult)
		{
		case Device::ReadResult::Success:
		{
			cemuLog_logDebug(LogType::NSYSHID, "nsyshid.hidReadInternalSync(): read {} of {} bytes",
							 message.bytesRead,
							 maxLength);
			return message.bytesRead;
		}
		break;
		case Device::ReadResult::Error:
		{
			cemuLog_logDebug(LogType::NSYSHID, "nsyshid.hidReadInternalSync(): read error");
			return -1;
		}
		break;
		case Device::ReadResult::ErrorTimeout:
		{
			cemuLog_logDebug(LogType::NSYSHID, "nsyshid.hidReadInternalSync(): read error: timeout");
			return -108;
		}
		break;
		}
		cemuLog_logDebug(LogType::NSYSHID, "nsyshid.hidReadInternalSync(): read error: unknown");
		return -1;
	}

	sint32 HIDRead(uint32 hidHandle, uint8* data, uint32 maxLength, MPTR callbackFuncMPTR, MPTR callbackParamMPTR)
	{
		std::shared_ptr<Device> device = GetDeviceByHandle(hidHandle, true);
		if (device == nullptr)
		{
			cemuLog_log(LogType::NSYSHID, "nsyshid.HIDRead(): Unable to find device with hid handle {}", hidHandle);
			return -1;
		}
		Device* dev = device.get();
		return SubmitTransfer(dev->m_readQueue, hidHandle, data, callbackFuncMPTR, callbackParamMPTR, [=]() -> sint32 {
			return _hidReadInternalSync(dev, data, maxLength);
		});
	}

	sint32 _hidWriteInternalSync(Device* device, uint8* data, sint32 maxLength)
	{
		if (!device->IsOpened())
		{
			cemuLog_logDebug(LogType::NSYSHID, "nsyshid.hidWriteInternalSync(): cannot write to a non-opened device");
			return -1;
		}
		WriteMessage message(data, maxLength, 0);
		Device::WriteResult writeResult = device->Write(&message);
		switch (writeResult)
		{
		case Device::WriteResult::Success:
		{
			cemuLog_logDebug(LogType::NSYSHID, "nsyshid.hidWriteInternalSync(): wrote {} of {} bytes", message.bytesWritten,
							 maxLength);
			return message.bytesWritten;
		}
		break;
		case Device::WriteResult::Error:
		{
			cemuLog_logDebug(LogType::NSYSHID, "nsyshid.hidWriteInternalSync(): write error");
			return -1;
		}
		break;
		case Device::WriteResult::ErrorTimeout:
		{
			cemuLog_logDebug(LogType::NSYSHID, "nsyshid.hidWriteInternalSync(): write error: timeout");
			return -108;
		}
		break;
		}
		cemuLog_logDebug(LogType::NSYSHID, "nsyshid.hidWriteInternalSync(): write error: unknown");
		return -1;
	}

	sint32 HIDWrite(uint32 hidHandle, uint8* data, uint32 maxLength, MPTR callbackFuncMPTR, MPTR callbackParamMPTR)
	{
		std::shared_ptr<Device> device = GetDeviceByHandle(hidHandle, true);
		if (device == nullptr)
		{
			cemuLog_log(LogType::NSYSHID, "nsyshid.HIDWrite(): Unable to find device with hid handle {}", hidHandle);
			return -1;
		}
		Device* dev = device.get();
		return SubmitTransfer(dev->m_writeQueue, hidHandle, data, callbackFuncMPTR, callbackParamMPTR, [=]() -> sint32 {
			return _hidWriteInternalSync(dev, data, maxLength);
		});
	}

	sint32 HIDDecodeError(uint32 errorCode, uint32be* ukn0, uint32be* ukn1)
	{
		// todo
		*ukn0 = 0x3FF;
		*ukn1 = (uint32)-0x7FFF;

		return 0;
	}

	bool Device::GetIdle(uint8 ifIndex, uint8 reportId, uint8* duration)
	{
		return false;
	}

	bool Device::GetProtocol(uint8 ifIndex, uint8* protocol)
	{
		return false;
	}

	bool Device::GetReport(ReportMessage* message)
	{
		return false;
	}

	void Backend::DetachAllDevices()
	{
		std::lock_guard<std::recursive_mutex> lock(this->m_devicesMutex);
		if (m_isAttached)
		{
			for (const auto& device : this->m_devices)
			{
				nsyshid::DetachDevice(device);
			}
			this->m_devices.clear();
		}
	}

	bool Backend::AttachDevice(const std::shared_ptr<Device>& device)
	{
		std::lock_guard<std::recursive_mutex> lock(this->m_devicesMutex);
		if (m_isAttached && nsyshid::AttachDevice(device))
		{
			this->m_devices.push_back(device);
			return true;
		}
		return false;
	}

	void Backend::DetachDevice(const std::shared_ptr<Device>& device)
	{
		std::lock_guard<std::recursive_mutex> lock(this->m_devicesMutex);
		if (m_isAttached)
		{
			nsyshid::DetachDevice(device);
			this->m_devices.remove(device);
		}
	}

	std::shared_ptr<Device> Backend::FindDevice(std::function<bool(const std::shared_ptr<Device>&)> isWantedDevice)
	{
		std::lock_guard<std::recursive_mutex> lock(this->m_devicesMutex);
		auto it = std::find_if(this->m_devices.begin(), this->m_devices.end(), std::move(isWantedDevice));
		if (it != this->m_devices.end())
		{
			return *it;
		}
		return nullptr;
	}

	std::shared_ptr<Device> Backend::FindDeviceById(uint16 vendorId, uint16 productId)
	{
		return nsyshid::FindDeviceById(vendorId, productId);
	}

	bool Backend::IsDeviceWhitelisted(uint16 vendorId, uint16 productId)
	{
		return Whitelist::GetInstance().IsDeviceWhitelisted(vendorId, productId);
	}

	Backend::Backend()
		: m_isAttached(false)
	{
	}

	void Backend::OnAttach()
	{
		std::lock_guard<std::recursive_mutex> lock(this->m_devicesMutex);
		m_isAttached = true;
		AttachVisibleDevices();
	}

	void Backend::OnDetach()
	{
		std::lock_guard<std::recursive_mutex> lock(this->m_devicesMutex);
		DetachAllDevices();
		m_isAttached = false;
	}

	void Backend::OnSettingsChanged()
	{
		std::lock_guard<std::recursive_mutex> lock(this->m_devicesMutex);
		if (m_isAttached)
			UpdateVisibleDevices();
	}

	bool Backend::IsBackendAttached()
	{
		std::lock_guard<std::recursive_mutex> lock(this->m_devicesMutex);
		return m_isAttached;
	}

	Device::Device(uint16 vendorId,
				   uint16 productId,
				   uint8 interfaceIndex,
				   uint8 interfaceSubClass,
				   uint8 protocol)
		: m_hid(nullptr),
		  m_vendorId(vendorId),
		  m_productId(productId),
		  m_interfaceIndex(interfaceIndex),
		  m_interfaceSubClass(interfaceSubClass),
		  m_protocol(protocol),
		  m_maxPacketSizeRX(0x20),
		  m_maxPacketSizeTX(0x20),
		  m_controlQueue("nsyshid control"),
		  m_readQueue("nsyshid read"),
		  m_writeQueue("nsyshid write")
	{
	}

	TransferQueue::TransferQueue(const char* threadName)
		: m_threadName(threadName)
	{
	}

	TransferQueue::~TransferQueue()
	{
		Stop();
	}

	bool TransferQueue::Push(std::function<void()> transfer)
	{
		std::lock_guard lock(m_mutex);
		if (m_stopped)
			return false;
		m_transfers.emplace_back(std::move(transfer));
		if (!m_thread.joinable())
			m_thread = std::thread(&TransferQueue::ThreadFunc, this);
		m_condition.notify_one();
		return true;
	}

	void TransferQueue::Stop()
	{
		{
			std::lock_guard lock(m_mutex);
			m_stopped = true;
		}
		m_condition.notify_one();
		if (m_thread.joinable())
			m_thread.join();
	}

	void TransferQueue::ThreadFunc()
	{
		SetThreadName(m_threadName);
		std::unique_lock lock(m_mutex);
		while (true)
		{
			m_condition.wait(lock, [this] { return m_stopped || !m_transfers.empty(); });
			if (m_transfers.empty())
				break;
			std::function<void()> transfer = std::move(m_transfers.front());
			m_transfers.pop_front();
			lock.unlock();
			transfer();
			lock.lock();
		}
	}

	void Device::AssignHID(HID_t* hid)
	{
		if (hid != nullptr)
		{
			hid->vendorId = this->m_vendorId;
			hid->productId = this->m_productId;
			hid->ifIndex = this->m_interfaceIndex;
			hid->subClass = this->m_interfaceSubClass;
			hid->protocol = this->m_protocol;
			hid->ukn04 = 0x11223344;
			hid->paddingGuessed0F = 0;
			hid->maxPacketSizeRX = this->m_maxPacketSizeRX;
			hid->maxPacketSizeTX = this->m_maxPacketSizeTX;
		}
		this->m_hid = hid;
	}

	class : public COSModule
	{
		public:
		std::string_view GetName() override
		{
			return "nsyshid";
		}

		void RPLMapped() override
		{
			cafeExportRegister("nsyshid", HIDAddClient, LogType::NSYSHID);
			cafeExportRegister("nsyshid", HIDDelClient, LogType::NSYSHID);
			cafeExportRegister("nsyshid", HIDAddPassiveClient, LogType::NSYSHID);
			cafeExportRegister("nsyshid", HIDDelPassiveClient, LogType::NSYSHID);
			cafeExportRegister("nsyshid", HIDGetDescriptor, LogType::NSYSHID);
			cafeExportRegister("nsyshid", HIDGetIdle, LogType::NSYSHID);
			cafeExportRegister("nsyshid", HIDSetIdle, LogType::NSYSHID);
			cafeExportRegister("nsyshid", HIDGetProtocol, LogType::NSYSHID);
			cafeExportRegister("nsyshid", HIDSetProtocol, LogType::NSYSHID);
			cafeExportRegister("nsyshid", HIDGetReport, LogType::NSYSHID);
			cafeExportRegister("nsyshid", HIDSetReport, LogType::NSYSHID);

			cafeExportRegister("nsyshid", HIDRead, LogType::NSYSHID);
			cafeExportRegister("nsyshid", HIDWrite, LogType::NSYSHID);

			cafeExportRegister("nsyshid", HIDDecodeError, LogType::NSYSHID);
		};
		void rpl_entry(uint32 moduleHandle, coreinit::RplEntryReason reason) override
		{
			if (reason == coreinit::RplEntryReason::Loaded)
			{
				// initialise whitelist
				Whitelist::GetInstance();
				AttachDefaultBackends();
			}
			else if (reason == coreinit::RplEntryReason::Unloaded)
			{
				// client structs live in guest memory which is about to be released
				{
					std::lock_guard<std::recursive_mutex> lock(hidMutex);
					HIDClientList.clear();
				}
				DetachAllBackends();
			}
		}
	}s_COSnsyshidModule;

	COSModule* GetModule()
	{
		return &s_COSnsyshidModule;
	}

} // namespace nsyshid
