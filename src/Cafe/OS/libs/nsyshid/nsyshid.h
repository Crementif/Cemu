#pragma once
#include "Cafe/OS/RPL/COSModule.h"

namespace nsyshid
{
	class Backend;

	struct HIDClient_t
	{
		uint32be callbackFunc; // attach/detach callback
	};

	void AttachBackend(const std::shared_ptr<Backend>& backend);
	void DetachBackend(const std::shared_ptr<Backend>& backend);

	// attaches or detaches emulated devices which can be toggled while a title is running
	void UpdateEmulatedDevices();

	// used by nsyskbd, these may call guest code or block, so only call them from PPC threads
	sint32 HIDAddPassiveClient(HIDClient_t* hidClient, MPTR callbackFunc);
	sint32 HIDDelPassiveClient(HIDClient_t* hidClient);
	sint32 HIDGetIdle(uint32 hidHandle, uint8 ifIndex, uint8 reportId, uint8* duration, MPTR callbackFunc, MPTR callbackParam);
	sint32 HIDSetIdle(uint32 hidHandle, uint8 ifIndex, uint8 reportId, uint8 duration, MPTR callbackFunc, MPTR callbackParam);
	sint32 HIDSetProtocol(uint32 hidHandle, uint8 ifIndex, uint8 protocol, MPTR callbackFunc, MPTR callbackParam);
	sint32 HIDSetReport(uint32 hidHandle, uint8 reportType, uint8 reportId, uint8* data, uint32 dataLength, MPTR callbackFunc, MPTR callbackParam);
	sint32 HIDRead(uint32 hidHandle, uint8* data, uint32 maxLength, MPTR callbackFunc, MPTR callbackParam);

	COSModule* GetModule();
} // namespace nsyshid
