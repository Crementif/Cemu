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

	COSModule* GetModule();
} // namespace nsyshid
