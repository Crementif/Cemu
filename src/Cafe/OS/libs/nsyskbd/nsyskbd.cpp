#include "Cafe/OS/common/OSCommon.h"
#include "Cafe/OS/RPL/rpl.h"
#include "Cafe/OS/libs/coreinit/coreinit_Alarm.h"
#include "Cafe/OS/libs/coreinit/coreinit_Thread.h"
#include "Cafe/OS/libs/nsyshid/nsyshid.h"
#include "Cafe/OS/libs/nsyshid/Backend.h"
#include "nsyskbd.h"
#include "KeyboardLayouts.h"

// nsyskbd is a passive nsyshid client
// it drives attached boot protocol keyboards, turns their reports into key events, translates them to unicode using per-country layout tables and queues them per channel
// KBD* is the API for titles, SKBD* is a second set of channels used by system software
// KPR* is a helper for alt+numpad codes, dead keys and romaji input which operates on a title-provided queue
namespace nsyskbd
{
	enum KBDResult : sint32
	{
		KBD_RESULT_SUCCESS = 0,
		KBD_RESULT_NOT_INITIALIZED = 2,
		KBD_RESULT_ALREADY_INITIALIZED = 3,
		KBD_RESULT_INVALID_PARAM = 4,
		KBD_RESULT_NOT_CONNECTED = 5,
		KBD_RESULT_BUSY = 7,
	};

	enum KBDChannelStatus : uint32
	{
		KBD_CHANNEL_STATUS_CONNECTED = 0,
		KBD_CHANNEL_STATUS_DISCONNECTED = 1,
		KBD_CHANNEL_STATUS_ROLLOVER_ERROR = 2,
		KBD_CHANNEL_STATUS_REPORT_ERROR = 4,
	};

	enum KBDKeyState : uint32
	{
		KBD_KEY_STATE_UP = 0,
		KBD_KEY_STATE_DOWN = 1,
		KBD_KEY_STATE_REPEAT = 3,
	};

	enum KBDModState : uint32
	{
		KBD_MS_CTRL = 0x1,
		KBD_MS_SHIFT = 0x2,
		KBD_MS_ALT = 0x4,
		KBD_MS_GUI = 0x8,
		KBD_MS_EXTRA = 0x10,
		KBD_MS_ALTGR = 0x20,
		KBD_MS_LANG1 = 0x40, // first language mode (hiragana on the japanese layout)
		KBD_MS_LANG2 = 0x80, // second language mode (katakana on the japanese layout)
		KBD_MS_NUM_LOCK = 0x100,
		KBD_MS_CAPS_LOCK = 0x200,
		KBD_MS_SCROLL_LOCK = 0x400,
		KBD_MS_KEEP_CTRL_CHARS = 0x800, // ctrl/alt/gui don't drop the shift state during translation
		KBD_MS_SET_ALL = 0x1000, // KBDSetModState overwrites all bits instead of just the lock bits

		KBD_MS_LOCK_MASK = 0xFC0,
		KBD_MS_PRESSED_MASK = 0x3F,
	};

	enum KPRMode : uint32
	{
		KPR_MODE_ALT_CODE = 0x1,
		KPR_MODE_DEAD_KEY = 0x2,
		KPR_MODE_ROMAJI_HIRAGANA = 0x4,
		KPR_MODE_ROMAJI_KATAKANA = 0x8,
	};

	constexpr uint32 KBD_MAX_CHANNELS = 4;
	constexpr uint32 KBD_QUEUE_SIZE = 32;
	constexpr uint8 KBD_QUEUE_EMPTY = KBD_QUEUE_SIZE;
	constexpr uint8 KBD_HID_QUEUE_OVERFLOW = 0xFF;
	constexpr uint8 KBD_HID_MODIFIER_FIRST = 0xE0;

	struct KBDKeyEvent
	{
		/* +0x00 */ uint8be channel;
		/* +0x01 */ uint8be hidCode; // 0 if the queue is empty, 0xFF if events were dropped
		/* +0x02 */ uint8be padding02[2];
		/* +0x04 */ uint32be state;
		/* +0x08 */ uint32be modState;
		/* +0x0C */ uint16be unicode;
		/* +0x0E */ uint8be padding0E[2];
	};
	static_assert(sizeof(KBDKeyEvent) == 0x10);

	struct KBDChannelInfo // passed to the attach and detach callbacks
	{
		uint8be channel;
	};

	struct KBDLedRequest // passed to the KBDSetLedsAsyncEx callback
	{
		/* +0x00 */ uint8be channel;
		/* +0x01 */ uint8be padding01[3];
		/* +0x04 */ uint32be leds;
		/* +0x08 */ sint32be result;
	};
	static_assert(sizeof(KBDLedRequest) == 0xC);

	struct KPRQueue
	{
		/* +0x00 */ uint16be chars[6];
		/* +0x0C */ uint32be mode;
		/* +0x10 */ uint8be numReady; // chars ready to be read with KPRGetChar
		/* +0x11 */ uint8be numPending; // chars which are still being composed
		/* +0x12 */ uint8be padding12[2];
		/* +0x14 */ uint32be altCode;
	};
	static_assert(offsetof(KPRQueue, mode) == 0x0C);
	static_assert(offsetof(KPRQueue, altCode) == 0x14);

	uint16 TranslateHidCode(const KBDLayout& layout, uint8 hidCode, uint32 modState)
	{
		const uint32 numColumns = layout.numColumns;
		if (numColumns == 0)
			return 0;
		if (hidCode < 4)
			return KBK_INVALID;
		const uint16* entry;
		if (hidCode >= layout.numKeys + 4)
		{
			if ((uint8)(hidCode - KBD_HID_MODIFIER_FIRST) >= 8)
				return 0;
			entry = layout.modifierTable + (hidCode - KBD_HID_MODIFIER_FIRST) * numColumns;
		}
		else
		{
			entry = layout.keyTable + (hidCode - 4) * numColumns;
		}
		const uint16 keyFlags = entry[0];
		const uint16 keyType = keyFlags & KEY_FLAG_TYPE_MASK;
		if (keyType == KEY_FLAG_FIXED)
			return entry[1];

		uint32 stateMask;
		uint32 column;
		if ((modState & KBD_MS_ALTGR) || (modState & (KBD_MS_CTRL | KBD_MS_ALT)) == (KBD_MS_CTRL | KBD_MS_ALT))
		{
			stateMask = 0x40;
			column = layout.altGrColumn;
		}
		else if (modState & KBD_MS_LANG1)
		{
			stateMask = 0x4;
			column = layout.lang1Column;
		}
		else if (modState & KBD_MS_LANG2)
		{
			stateMask = 0x10;
			column = layout.lang2Column;
		}
		else
		{
			stateMask = 0x1;
			column = 1;
		}

		auto unshiftedCode = [&](uint32 shifted) -> uint16 {
			if (column == layout.altGrColumn && (layout.flags & 0x10) == 0)
				return 0;
			if ((column == layout.lang1Column && (layout.flags & 0x1) == 0) || (column == layout.lang2Column && (layout.flags & 0x4) == 0))
				return 0;
			return entry[column + shifted];
		};
		auto shiftedCode = [&](uint32 shifted) -> uint16 {
			if ((column == layout.altGrColumn && (layout.flags & 0x20) == 0) || (column == layout.lang1Column && (layout.flags & 0x2) == 0) || (column == layout.lang2Column && (layout.flags & 0x8) == 0))
				return 0;
			return entry[column + shifted];
		};

		if ((modState & KBD_MS_KEEP_CTRL_CHARS) == 0 && (modState & (KBD_MS_CTRL | KBD_MS_ALT | KBD_MS_GUI | KBD_MS_EXTRA)) != 0)
			return unshiftedCode(0);

		uint32 shifted = (modState & KBD_MS_SHIFT) ? 1 : 0;
		const uint16 lockMask = (uint16)(stateMask << shifted);
		if (keyType == 0 && (keyFlags & lockMask) != 0)
		{
			// caps lock inverts shift
			if (modState & KBD_MS_CAPS_LOCK)
				shifted ^= 1;
			return shifted ? shiftedCode(shifted) : unshiftedCode(shifted);
		}
		if (keyType == KEY_FLAG_NUM_LOCK && (keyFlags & lockMask) != 0)
		{
			shifted = ((modState & KBD_MS_NUM_LOCK) && (modState & KBD_MS_SHIFT) == 0) ? 1 : 0;
			return shifted ? shiftedCode(shifted) : unshiftedCode(shifted);
		}
		if (keyType == KEY_FLAG_CAPS_TO_LANG1 && column == 1 && (modState & KBD_MS_CAPS_LOCK))
			column = layout.lang1Column;
		return shifted ? shiftedCode(shifted) : unshiftedCode(shifted);
	}

	/* channels */

	struct KBDChannel
	{
		uint8 index;
		bool isSystemChannel; // SKBD channel
		uint32 status;
		uint32 country;
		uint32 lockProcessing;
		uint32 accessSticky;
		uint32 modState;
		uint32 currentLeds; // last LED state the keyboard accepted, KBD_LEDS_UNKNOWN if none
		uint32 pendingLeds; // LED state of an outstanding retried LED update
		bool autoLeds; // update the keyboard LEDs when the lock state changes
		uint16 repeatDelay; // in milliseconds
		uint16 repeatRate; // in milliseconds
		uint8 modCounters[6]; // number of pressed keys per modifier (ctrl, shift, alt, gui, extra, altgr)
		uint8 lastModifiers;
		uint8 lastKeys[6];
		KBDKeyEvent queue[KBD_QUEUE_SIZE];
		uint8 queueIn;
		uint8 queueOut;
		uint8 repeatHidCode;
		bool repeatArmed;
		MPTR keyCallback;
		// only used by KBD channels
		nsyshid::HID_t* hid;
		bool hidConnected;
	};

	struct PendingKeyEvent
	{
		KBDChannel* channel;
		MPTR callback;
		KBDKeyEvent event;
	};

	// HIDSetReport transfer used for LED updates
	struct KBDLedTransfer
	{
		/* +0x00 */ uint8 report;
		/* +0x01 */ uint8be padding01[3];
		/* +0x04 */ KBDLedRequest request; // passed to the KBDSetLedsAsyncEx callback
		/* +0x10 */ uint32be callback;
		/* +0x14 */ uint32be context;
		/* +0x18 */ uint8be padding18[8];
	};
	static_assert(sizeof(KBDLedTransfer) == 0x20);

	constexpr uint32 KBD_LEDS_UNKNOWN = 0x10;
	constexpr uint32 KBD_LED_TRANSFER_COUNT = 16;
	constexpr uint32 KBD_READ_BUFFER_SIZE = 64;
	constexpr uint32 KBD_BOOT_REPORT_SIZE = 8;
	constexpr uint16 KBD_IGNORED_VENDOR_ID = 0x046A;
	constexpr uint8 HID_PROTOCOL_KEYBOARD = 1;
	constexpr uint8 HID_SET_PROTOCOL_BOOT = 0;
	constexpr uint8 HID_REPORT_TYPE_OUTPUT = 2;

	// channel state is guarded by a host mutex, so nothing which can switch the guest thread may be called while holding it
	static std::recursive_mutex s_mutex;
	static KBDChannel s_kbdChannels[KBD_MAX_CHANNELS];
	static KBDChannel s_skbdChannels[KBD_MAX_CHANNELS];
	static bool s_kbdInitialized{false};
	static bool s_kbdSetup{false};
	static bool s_stopReading{false};
	static MPTR s_attachCallback{MPTR_NULL};
	static MPTR s_detachCallback{MPTR_NULL};
	static bool s_skbdInitialized{false};
	static sint32 s_skbdMode{0};
	static uint32 s_passiveClientCount{0};
	static std::bitset<KBD_LED_TRANSFER_COUNT> s_ledTransfersInUse;

	static SysAllocator<nsyshid::HIDClient_t> s_hidClient;
	static SysAllocator<coreinit::OSMutex> s_hidClientMutex; // guards s_passiveClientCount, held across attach callbacks
	static SysAllocator<uint8, KBD_READ_BUFFER_SIZE * KBD_MAX_CHANNELS> s_readBuffers;
	static SysAllocator<coreinit::OSAlarm_t, KBD_MAX_CHANNELS * 2> s_repeatAlarms; // KBD channels followed by SKBD channels
	static SysAllocator<coreinit::OSAlarm_t, KBD_MAX_CHANNELS> s_ledRetryAlarms;
	static SysAllocator<KBDLedTransfer, KBD_LED_TRANSFER_COUNT> s_ledTransfers;

	static MPTR s_hidAttachCallbackFunc{MPTR_NULL};
	static MPTR s_hidReadCallbackFunc{MPTR_NULL};
	static MPTR s_ledTransferCallbackFunc{MPTR_NULL};
	static MPTR s_ledRetryCallbackFunc{MPTR_NULL};
	static MPTR s_repeatAlarmHandlerFunc{MPTR_NULL};
	static MPTR s_ledRetryAlarmHandlerFunc{MPTR_NULL};

	sint32 KBDSetLedsRetryEx(uint32 channelIndex, uint8 leds);

	uint8 GetLedsFromModState(uint32 modState)
	{
		// num lock, caps lock and scroll lock map directly to the HID LED bits
		return (modState >> 8) & 7;
	}

	void UpdateLeds(const KBDChannel& channel)
	{
		KBDSetLedsRetryEx(channel.index, GetLedsFromModState(channel.modState));
	}

	coreinit::OSAlarm_t* GetRepeatAlarm(const KBDChannel& channel)
	{
		return s_repeatAlarms.GetPtr() + (channel.isSystemChannel ? KBD_MAX_CHANNELS : 0) + channel.index;
	}

	void CancelRepeat(KBDChannel& channel)
	{
		coreinit::OSCancelAlarm(GetRepeatAlarm(channel));
		channel.repeatArmed = false;
	}

	void SetModState(KBDChannel& channel, uint32 modState)
	{
		if (modState & KBD_MS_SET_ALL)
		{
			channel.modState = modState & ~KBD_MS_SET_ALL;
			return;
		}
		uint32 newModState = (modState & KBD_MS_LOCK_MASK) | (channel.modState & KBD_MS_PRESSED_MASK);
		if (channel.modState != newModState)
		{
			channel.modState = newModState;
			UpdateLeds(channel);
		}
	}

	void ResetChannel(KBDChannel& channel)
	{
		channel.lastModifiers = 0;
		std::fill(std::begin(channel.lastKeys), std::end(channel.lastKeys), 0);
		channel.queueOut = KBD_QUEUE_EMPTY;
		SetModState(channel, KBD_MS_SET_ALL);
		std::fill(std::begin(channel.modCounters), std::end(channel.modCounters), 0);
		channel.currentLeds = KBD_LEDS_UNKNOWN;
		channel.pendingLeds = KBD_LEDS_UNKNOWN;
		channel.repeatHidCode = 0;
		CancelRepeat(channel);
	}

	void InitChannel(KBDChannel& channel, uint8 index, bool isSystemChannel)
	{
		channel = {};
		channel.index = index;
		channel.isSystemChannel = isSystemChannel;
		channel.country = KBD_COUNTRY_US;
		channel.status = KBD_CHANNEL_STATUS_DISCONNECTED;
		channel.lockProcessing = 1;
		channel.accessSticky = 0;
		channel.repeatDelay = 500;
		channel.repeatRate = 33;
		channel.autoLeds = !isSystemChannel;
		coreinit::OSCreateAlarm(GetRepeatAlarm(channel));
		ResetChannel(channel);
	}

	// used when switching the system keyboard mode
	void CopyChannelSettings(KBDChannel& dst, const KBDChannel& src)
	{
		dst.country = src.country;
		dst.lastModifiers = src.lastModifiers;
		std::copy(std::begin(src.lastKeys), std::end(src.lastKeys), std::begin(dst.lastKeys));
		dst.repeatDelay = src.repeatDelay;
		dst.repeatRate = src.repeatRate;
		std::copy(std::begin(src.modCounters), std::end(src.modCounters), std::begin(dst.modCounters));
		dst.lockProcessing = src.lockProcessing;
		dst.accessSticky = src.accessSticky;
		dst.modState = src.modState;
		dst.repeatHidCode = src.repeatHidCode;
	}

	void HandleModifierKey(KBDChannel& channel, uint16 code, uint32 keyState)
	{
		const bool isDown = (keyState & 1) != 0;
		uint32 modState = channel.modState;
		auto updateModifier = [&](uint32 counterIndex, uint32 modBit) {
			if (channel.accessSticky)
			{
				if (isDown)
					modState ^= modBit;
				return;
			}
			channel.modCounters[counterIndex] += isDown ? 1 : -1;
			modState = (modState & ~modBit) | (channel.modCounters[counterIndex] != 0 ? modBit : 0);
		};
		auto toggleLock = [&](uint32 modBit) {
			if (isDown && channel.lockProcessing)
				modState ^= modBit;
		};
		switch (code)
		{
		case KBK_CTRL:
			updateModifier(0, KBD_MS_CTRL);
			break;
		case KBK_SHIFT:
			updateModifier(1, KBD_MS_SHIFT);
			break;
		case KBK_ALT:
			updateModifier(2, KBD_MS_ALT);
			break;
		case KBK_GUI:
			updateModifier(3, KBD_MS_GUI);
			break;
		case KBK_EXTRA:
			updateModifier(4, KBD_MS_EXTRA);
			break;
		case KBK_ALTGR:
			updateModifier(5, KBD_MS_ALTGR);
			break;
		case KBK_LANG:
			// cycles through the language modes supported by the layout
			if (isDown && channel.lockProcessing)
			{
				const uint8 layoutFlags = GetLayout(channel.country).flags;
				switch (modState & (KBD_MS_LANG1 | KBD_MS_LANG2))
				{
				case 0:
					if (layoutFlags & 0x1)
						modState |= KBD_MS_LANG1;
					break;
				case KBD_MS_LANG1:
					modState &= ~KBD_MS_LANG1;
					if (layoutFlags & 0x4)
						modState |= KBD_MS_LANG2;
					break;
				case KBD_MS_LANG2:
					modState &= ~KBD_MS_LANG2;
					break;
				default:
					modState &= ~(KBD_MS_LANG1 | KBD_MS_LANG2);
					break;
				}
			}
			break;
		case KBK_NUM_LOCK:
			toggleLock(KBD_MS_NUM_LOCK);
			break;
		case KBK_CAPS_LOCK:
			toggleLock(KBD_MS_CAPS_LOCK);
			break;
		case KBK_SCROLL_LOCK:
			toggleLock(KBD_MS_SCROLL_LOCK);
			break;
		default:
			break;
		}
		SetModState(channel, modState | KBD_MS_SET_ALL);
	}

	void Enqueue(KBDChannel& channel, const KBDKeyEvent& keyEvent)
	{
		const uint8 queueIn = channel.queueIn;
		const uint8 queueOut = channel.queueOut;
		if (queueIn == queueOut)
			return; // full
		const uint8 nextIn = (queueIn + 1) % KBD_QUEUE_SIZE;
		if (nextIn == queueOut)
		{
			// last free slot is used to signal that events were dropped
			KBDKeyEvent& overflowEvent = channel.queue[queueIn];
			overflowEvent = {};
			overflowEvent.channel = keyEvent.channel;
			overflowEvent.hidCode = KBD_HID_QUEUE_OVERFLOW;
		}
		else
		{
			channel.queue[queueIn] = keyEvent;
			if (queueOut == KBD_QUEUE_EMPTY)
				channel.queueOut = queueIn;
		}
		channel.queueIn = nextIn;
	}

	bool IsRepeatableKey(uint16 code)
	{
		switch (code)
		{
		case KBK_LANG:
		case KBK_CAPS_LOCK:
		case KBK_NUM_LOCK:
		case KBK_SCROLL_LOCK:
		case KBK_PRINT_SCREEN:
		case KBK_PAUSE:
		case KBK_APPLICATION:
			return false;
		default:
			return true;
		}
	}

	void ProcessKey(KBDChannel& channel, uint8 hidCode, uint32 keyState, std::vector<PendingKeyEvent>& pendingEvents)
	{
		const KBDLayout& layout = GetLayout(channel.country);
		for (uint32 i = 0; i < layout.numRemap; i++)
		{
			if (hidCode == layout.remap[i][0])
			{
				hidCode = layout.remap[i][1];
				break;
			}
		}
		PendingKeyEvent& pending = pendingEvents.emplace_back();
		pending.channel = &channel;
		pending.callback = channel.keyCallback;
		KBDKeyEvent& keyEvent = pending.event;
		keyEvent = {};
		keyEvent.channel = channel.index;
		keyEvent.hidCode = hidCode;
		keyEvent.state = keyState;
		keyEvent.modState = channel.modState;
		const uint16 code = TranslateHidCode(layout, hidCode, channel.modState);
		keyEvent.unicode = code;

		if ((code >= KBK_CTRL && code <= KBK_CAPS_LOCK) || code == KBK_SCROLL_LOCK)
		{
			HandleModifierKey(channel, code, keyState);
			// modifiers don't touch the repeat state, lock keys cancel it
			if (code != KBK_LANG && code != KBK_NUM_LOCK && code != KBK_CAPS_LOCK && code != KBK_SCROLL_LOCK)
				return;
		}

		if (keyState & 1)
		{
			CancelRepeat(channel);
			if (IsRepeatableKey(code))
			{
				channel.repeatHidCode = hidCode;
				const uint16 delay = (keyState & 2) ? channel.repeatRate : channel.repeatDelay;
				if (delay != 0)
				{
					coreinit::OSAlarm_t* alarm = GetRepeatAlarm(channel);
					coreinit::OSSetAlarmUserData(alarm, (channel.isSystemChannel ? KBD_MAX_CHANNELS : 0) + channel.index);
					coreinit::OSSetAlarm(alarm, (uint64)delay * ESPRESSO_TIMER_CLOCK / 1000, s_repeatAlarmHandlerFunc);
					channel.repeatArmed = true;
				}
			}
		}
		else if (hidCode == channel.repeatHidCode)
		{
			CancelRepeat(channel);
		}
	}

	// report is a boot protocol keyboard report: modifier bits, reserved byte, six key codes
	void ProcessReport(KBDChannel& channel, const uint8* report, std::vector<PendingKeyEvent>& pendingEvents)
	{
		const uint8 firstKey = report[2];
		if (firstKey >= 1 && firstKey <= 3)
		{
			// rollover or other error, keep the current key state
			CancelRepeat(channel);
			channel.status |= (firstKey == 1) ? KBD_CHANNEL_STATUS_ROLLOVER_ERROR : KBD_CHANNEL_STATUS_REPORT_ERROR;
			return;
		}
		const uint32 previousModState = channel.modState;
		channel.status = KBD_CHANNEL_STATUS_CONNECTED;
		const uint8 modifiers = report[0];
		const uint8* keys = report + 2;
		for (uint32 i = 0; i < 8; i++)
		{
			const uint8 bit = 1 << i;
			if ((channel.lastModifiers & bit) && !(modifiers & bit))
				ProcessKey(channel, KBD_HID_MODIFIER_FIRST + i, KBD_KEY_STATE_UP, pendingEvents);
			else if (!(channel.lastModifiers & bit) && (modifiers & bit))
				ProcessKey(channel, KBD_HID_MODIFIER_FIRST + i, KBD_KEY_STATE_DOWN, pendingEvents);
		}
		for (uint32 i = 0; i < 6; i++)
		{
			const uint8 lastKey = channel.lastKeys[i];
			if (lastKey != 0 && std::find(keys, keys + 6, lastKey) == keys + 6)
			{
				ProcessKey(channel, lastKey, KBD_KEY_STATE_UP, pendingEvents);
				channel.lastKeys[i] = 0;
			}
		}
		for (uint32 i = 0; i < 6; i++)
		{
			const uint8 key = keys[i];
			if (key != 0 && std::find(std::begin(channel.lastKeys), std::end(channel.lastKeys), key) == std::end(channel.lastKeys))
				ProcessKey(channel, key, KBD_KEY_STATE_DOWN, pendingEvents);
		}
		channel.lastModifiers = modifiers;
		std::copy(keys, keys + 6, std::begin(channel.lastKeys));
		if (channel.autoLeds && channel.modState != previousModState)
			UpdateLeds(channel);
	}

	// the key callback is called before the event is queued, so it can't run while holding the lock
	void DispatchPendingEvents(std::vector<PendingKeyEvent>& pendingEvents)
	{
		for (auto& pending : pendingEvents)
		{
			if (pending.callback != MPTR_NULL)
			{
				StackAllocator<KBDKeyEvent> callbackEvent;
				*callbackEvent.GetPointer() = pending.event;
				PPCCoreCallback(pending.callback, callbackEvent.GetPointer());
			}
			std::unique_lock _l(s_mutex);
			Enqueue(*pending.channel, pending.event);
		}
		pendingEvents.clear();
	}

	void CallChannelCallback(MPTR callback, uint8 channelIndex)
	{
		if (callback == MPTR_NULL)
			return;
		StackAllocator<KBDChannelInfo> channelInfo;
		channelInfo->channel = channelIndex;
		PPCCoreCallback(callback, channelInfo.GetPointer());
	}

	KBDChannel* GetChannel(KBDChannel* channels, uint32 channelIndex, std::string_view funcName)
	{
		// nsyskbd does not validate the channel and accesses memory past the channel array instead. Callers ignore the request and report success like it does
		if (channelIndex >= KBD_MAX_CHANNELS)
		{
			cemuLog_log(LogType::APIErrors, "{}(): Invalid channel {}", funcName, channelIndex);
			return nullptr;
		}
		return channels + channelIndex;
	}

	/* HID client */

	uint8* GetReadBuffer(uint8 channelIndex)
	{
		return s_readBuffers.GetPtr() + channelIndex * KBD_READ_BUFFER_SIZE;
	}

	void StartRead(uint8 channelIndex, uint32 hidHandle)
	{
		if (nsyshid::HIDRead(hidHandle, GetReadBuffer(channelIndex), KBD_BOOT_REPORT_SIZE, s_hidReadCallbackFunc, channelIndex) < 0)
			cemuLog_log(LogType::NSYSKBD, "nsyskbd: HIDRead failed for keyboard on channel {}", channelIndex);
	}

	void InitKeyboard(uint8 channelIndex, uint32 hidHandle, uint8 ifIndex)
	{
		if (nsyshid::HIDSetIdle(hidHandle, ifIndex, 0, 0, MPTR_NULL, MPTR_NULL) < 0)
			cemuLog_log(LogType::NSYSKBD, "nsyskbd: HIDSetIdle failed");
		if (nsyshid::HIDSetProtocol(hidHandle, ifIndex, HID_SET_PROTOCOL_BOOT, MPTR_NULL, MPTR_NULL) < 0)
			cemuLog_log(LogType::NSYSKBD, "nsyskbd: HIDSetProtocol failed");
		// some keyboards ignore the first SetIdle, so keep checking for a while
		uint8* idleDuration = GetReadBuffer(channelIndex);
		for (sint32 attemptsLeft = 30; attemptsLeft > 0; attemptsLeft--)
		{
			if (nsyshid::HIDGetIdle(hidHandle, ifIndex, 0, idleDuration, MPTR_NULL, MPTR_NULL) < 0)
				cemuLog_log(LogType::NSYSKBD, "nsyskbd: HIDGetIdle failed");
			if (*idleDuration == 0)
				break;
			if (attemptsLeft == 30)
			{
				if (nsyshid::HIDSetIdle(hidHandle, ifIndex, 0, 0, MPTR_NULL, MPTR_NULL) < 0)
					cemuLog_log(LogType::NSYSKBD, "nsyskbd: HIDSetIdle failed");
			}
			else
			{
				coreinit::OSSleepTicks(ESPRESSO_TIMER_CLOCK / 10);
			}
		}
		StartRead(channelIndex, hidHandle);
	}

	uint32 AttachKeyboard(nsyshid::HID_t* hid)
	{
		if (hid->vendorId == KBD_IGNORED_VENDOR_ID || hid->protocol != HID_PROTOCOL_KEYBOARD)
			return 0;
		uint8 channelIndex;
		MPTR attachCallback;
		{
			std::unique_lock _l(s_mutex);
			auto it = std::find_if(std::begin(s_kbdChannels), std::end(s_kbdChannels), [](const KBDChannel& channel) { return channel.hid == nullptr; });
			if (it == std::end(s_kbdChannels))
			{
				cemuLog_log(LogType::NSYSKBD, "nsyskbd: Max number of keyboards exceeded. Ignoring attach");
				return 0;
			}
			KBDChannel& channel = *it;
			channelIndex = channel.index;
			channel.hid = hid;
			channel.status &= ~KBD_CHANNEL_STATUS_DISCONNECTED;
			ResetChannel(channel);
			if (s_skbdMode == 1 || s_skbdMode == 2)
			{
				CopyChannelSettings(channel, s_skbdChannels[channelIndex]);
				UpdateLeds(channel);
			}
			channel.hidConnected = true;
			attachCallback = s_attachCallback;
		}
		InitKeyboard(channelIndex, hid->handle, hid->ifIndex);
		{
			std::unique_lock _l(s_mutex);
			UpdateLeds(s_kbdChannels[channelIndex]);
		}
		cemuLog_log(LogType::NSYSKBD, "nsyskbd: Keyboard {:04x}:{:04x} attached to channel {}", (uint16)hid->vendorId, (uint16)hid->productId, channelIndex);
		CallChannelCallback(attachCallback, channelIndex);
		return 1;
	}

	uint32 DetachKeyboard(nsyshid::HID_t* hid)
	{
		uint8 channelIndex;
		MPTR detachCallback;
		{
			std::unique_lock _l(s_mutex);
			auto it = std::find_if(std::begin(s_kbdChannels), std::end(s_kbdChannels), [hid](const KBDChannel& channel) { return channel.hid == hid; });
			if (it == std::end(s_kbdChannels))
				return 0;
			KBDChannel& channel = *it;
			channelIndex = channel.index;
			channel.hidConnected = false;
			channel.hid = nullptr;
			channel.status |= KBD_CHANNEL_STATUS_DISCONNECTED;
			detachCallback = s_detachCallback;
		}
		cemuLog_log(LogType::NSYSKBD, "nsyskbd: Keyboard detached from channel {}", channelIndex);
		CallChannelCallback(detachCallback, channelIndex);
		std::unique_lock _l(s_mutex);
		ResetChannel(s_kbdChannels[channelIndex]);
		ResetChannel(s_skbdChannels[channelIndex]);
		return 1;
	}

	// nsyshid attach callback: (HIDClient* client, HIDDevice* device, uint32 attach), returns 1 if the device is used
	void ppcHIDAttachCallback(PPCInterpreter_t* hCPU)
	{
		ppcDefineParamTypePtr(hid, nsyshid::HID_t, 1);
		ppcDefineParamU32(attach, 2);
		osLib_returnFromFunction(hCPU, attach ? AttachKeyboard(hid) : DetachKeyboard(hid));
	}

	// nsyshid transfer callback: (uint32 handle, sint32 error, uint8* buffer, uint32 bytesTransferred, void* context)
	void ppcHIDReadCallback(PPCInterpreter_t* hCPU)
	{
		ppcDefineParamU32(hidHandle, 0);
		ppcDefineParamS32(error, 1);
		ppcDefineParamU32(channelIndex, 4);
		std::vector<PendingKeyEvent> pendingEvents;
		{
			std::unique_lock _l(s_mutex);
			KBDChannel& channel = s_kbdChannels[channelIndex % KBD_MAX_CHANNELS];
			if (s_stopReading || !channel.hidConnected || channel.hid->handle != hidHandle)
			{
				osLib_returnFromFunction(hCPU, 0);
				return;
			}
			if (error != 0)
			{
				cemuLog_log(LogType::NSYSKBD, "nsyskbd: Reading keyboard on channel {} failed with error {}", channel.index, error);
				osLib_returnFromFunction(hCPU, 0);
				return;
			}
			const uint8* report = GetReadBuffer(channel.index);
			ProcessReport(channel, report, pendingEvents);
			if (s_skbdInitialized && s_skbdMode != 0)
				ProcessReport(s_skbdChannels[channel.index], report, pendingEvents);
		}
		DispatchPendingEvents(pendingEvents);
		StartRead(channelIndex, hidHandle);
		osLib_returnFromFunction(hCPU, 0);
	}

	// OSAlarm handler: (OSAlarm* alarm, OSContext* context)
	void ppcRepeatAlarmHandler(PPCInterpreter_t* hCPU)
	{
		ppcDefineParamTypePtr(alarm, coreinit::OSAlarm_t, 0);
		const uint32 alarmIndex = alarm->userData;
		std::vector<PendingKeyEvent> pendingEvents;
		{
			std::unique_lock _l(s_mutex);
			KBDChannel& channel = alarmIndex < KBD_MAX_CHANNELS ? s_kbdChannels[alarmIndex] : s_skbdChannels[alarmIndex % KBD_MAX_CHANNELS];
			// the key may have been released while the alarm was already firing
			if (channel.repeatArmed)
			{
				channel.repeatArmed = false;
				ProcessKey(channel, channel.repeatHidCode, KBD_KEY_STATE_REPEAT, pendingEvents);
			}
		}
		DispatchPendingEvents(pendingEvents);
		osLib_returnFromFunction(hCPU, 0);
	}

	// passive client registration is reference counted, it's used by KBD and by the system keyboard mode
	void AddPassiveClient()
	{
		coreinit::OSLockMutex(s_hidClientMutex.GetPtr());
		if (++s_passiveClientCount == 1)
			nsyshid::HIDAddPassiveClient(s_hidClient.GetPtr(), s_hidAttachCallbackFunc);
		coreinit::OSUnlockMutex(s_hidClientMutex.GetPtr());
	}

	void RemovePassiveClient()
	{
		coreinit::OSLockMutex(s_hidClientMutex.GetPtr());
		if (s_passiveClientCount != 0 && --s_passiveClientCount == 0)
			nsyshid::HIDDelPassiveClient(s_hidClient.GetPtr());
		coreinit::OSUnlockMutex(s_hidClientMutex.GetPtr());
	}

	/* shared KBD/SKBD implementations */

	KBDResult GetKey(KBDChannel& channel, KBDKeyEvent* keyEvent)
	{
		std::unique_lock _l(s_mutex);
		if (channel.status == KBD_CHANNEL_STATUS_DISCONNECTED || channel.status == KBD_CHANNEL_STATUS_REPORT_ERROR)
			return KBD_RESULT_NOT_CONNECTED;
		if (channel.queueOut == KBD_QUEUE_EMPTY)
		{
			*keyEvent = {};
			keyEvent->channel = channel.index;
			return KBD_RESULT_SUCCESS;
		}
		*keyEvent = channel.queue[channel.queueOut];
		channel.queueOut = (channel.queueOut + 1) % KBD_QUEUE_SIZE;
		if (channel.queueOut == channel.queueIn)
			channel.queueOut = KBD_QUEUE_EMPTY;
		return KBD_RESULT_SUCCESS;
	}

	KBDResult EmptyQueue(KBDChannel& channel)
	{
		KBDKeyEvent keyEvent;
		while (GetKey(channel, &keyEvent) == KBD_RESULT_SUCCESS && keyEvent.hidCode != 0)
			;
		return KBD_RESULT_SUCCESS;
	}

	KBDResult SetCountry(KBDChannel& channel, sint32 country)
	{
		if (country >= KBD_COUNTRY_COUNT)
			return KBD_RESULT_INVALID_PARAM;
		std::unique_lock _l(s_mutex);
		channel.country = country;
		ResetChannel(channel);
		return KBD_RESULT_SUCCESS;
	}

	/* KBD */

	sint32 KBDSetup(MPTR attachCallback, MPTR detachCallback, MPTR keyCallback)
	{
		{
			std::unique_lock _l(s_mutex);
			if (s_kbdSetup)
				return KBD_RESULT_ALREADY_INITIALIZED;
			s_kbdSetup = true;
			s_attachCallback = attachCallback;
			s_detachCallback = detachCallback;
			for (auto& channel : s_kbdChannels)
				channel.keyCallback = keyCallback;
			s_stopReading = false;
		}
		AddPassiveClient();
		return KBD_RESULT_SUCCESS;
	}

	sint32 KBDInit(uint32 ukn, MPTR attachCallback, MPTR detachCallback, MPTR keyCallback)
	{
		return KBDSetup(attachCallback, detachCallback, keyCallback);
	}

	sint32 KBDTeardown()
	{
		{
			std::unique_lock _l(s_mutex);
			if (!s_kbdSetup)
				return KBD_RESULT_NOT_INITIALIZED;
			// nsyskbd leaves the per-channel key callbacks in place
			s_attachCallback = MPTR_NULL;
			s_detachCallback = MPTR_NULL;
			s_stopReading = true;
		}
		RemovePassiveClient();
		std::unique_lock _l(s_mutex);
		s_kbdSetup = false;
		return KBD_RESULT_SUCCESS;
	}

	sint32 KBDExit()
	{
		return KBDTeardown();
	}

	sint32 KBDGetChannelStatus(uint32 channelIndex, uint32be* status)
	{
		KBDChannel* channel = GetChannel(s_kbdChannels, channelIndex, "KBDGetChannelStatus");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		std::unique_lock _l(s_mutex);
		*status = channel->status;
		return KBD_RESULT_SUCCESS;
	}

	sint32 KBDGetKey(uint32 channelIndex, KBDKeyEvent* keyEvent)
	{
		KBDChannel* channel = GetChannel(s_kbdChannels, channelIndex, "KBDGetKey");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		return GetKey(*channel, keyEvent);
	}

	sint32 KBDEmptyQueue(uint32 channelIndex)
	{
		KBDChannel* channel = GetChannel(s_kbdChannels, channelIndex, "KBDEmptyQueue");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		return EmptyQueue(*channel);
	}

	sint32 KBDResetChannel(uint32 channelIndex)
	{
		KBDChannel* channel = GetChannel(s_kbdChannels, channelIndex, "KBDResetChannel");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		std::unique_lock _l(s_mutex);
		ResetChannel(*channel);
		return KBD_RESULT_SUCCESS;
	}

	sint32 KBDSetCountry(uint32 channelIndex, sint32 country)
	{
		KBDChannel* channel = GetChannel(s_kbdChannels, channelIndex, "KBDSetCountry");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		return SetCountry(*channel, country);
	}

	sint32 KBDGetCountry(uint32 channelIndex, uint32be* country)
	{
		KBDChannel* channel = GetChannel(s_kbdChannels, channelIndex, "KBDGetCountry");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		*country = channel->country;
		return KBD_RESULT_SUCCESS;
	}

	sint32 KBDSetLockProcessing(uint32 channelIndex, uint32 enable)
	{
		KBDChannel* channel = GetChannel(s_kbdChannels, channelIndex, "KBDSetLockProcessing");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		channel->lockProcessing = enable;
		return KBD_RESULT_SUCCESS;
	}

	sint32 KBDGetLockProcessing(uint32 channelIndex, uint32be* enable)
	{
		KBDChannel* channel = GetChannel(s_kbdChannels, channelIndex, "KBDGetLockProcessing");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		*enable = channel->lockProcessing;
		return KBD_RESULT_SUCCESS;
	}

	sint32 KBDSetModState(uint32 channelIndex, uint32 modState)
	{
		KBDChannel* channel = GetChannel(s_kbdChannels, channelIndex, "KBDSetModState");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		std::unique_lock _l(s_mutex);
		SetModState(*channel, modState);
		return KBD_RESULT_SUCCESS;
	}

	sint32 KBDGetModState(uint32 channelIndex, uint32be* modState)
	{
		KBDChannel* channel = GetChannel(s_kbdChannels, channelIndex, "KBDGetModState");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		*modState = channel->modState;
		return KBD_RESULT_SUCCESS;
	}

	sint32 KBDSetRepeat(uint32 channelIndex, uint16 delay, uint16 rate)
	{
		KBDChannel* channel = GetChannel(s_kbdChannels, channelIndex, "KBDSetRepeat");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		channel->repeatDelay = delay;
		channel->repeatRate = rate;
		return KBD_RESULT_SUCCESS;
	}

	sint32 KBDGetRepeat(uint32 channelIndex, uint16be* delay, uint16be* rate)
	{
		KBDChannel* channel = GetChannel(s_kbdChannels, channelIndex, "KBDGetRepeat");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		*delay = channel->repeatDelay;
		*rate = channel->repeatRate;
		return KBD_RESULT_SUCCESS;
	}

	sint32 KBDSetAccessSticky(uint32 channelIndex, uint32 enable)
	{
		KBDChannel* channel = GetChannel(s_kbdChannels, channelIndex, "KBDSetAccessSticky");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		channel->accessSticky = enable;
		return KBD_RESULT_SUCCESS;
	}

	sint32 KBDGetAccessSticky(uint32 channelIndex, uint32be* enable)
	{
		KBDChannel* channel = GetChannel(s_kbdChannels, channelIndex, "KBDGetAccessSticky");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		*enable = channel->accessSticky;
		return KBD_RESULT_SUCCESS;
	}

	uint16 KBDTranslateHidCode(uint8 hidCode, uint32 modState, sint32 country)
	{
		if (country >= KBD_COUNTRY_COUNT)
			return KBK_INVALID;
		return TranslateHidCode(GetLayout(country), hidCode, modState);
	}

	/* LEDs */

	KBDResult CheckLedChannel(uint32 channelIndex)
	{
		if (!s_kbdInitialized)
			return KBD_RESULT_NOT_INITIALIZED;
		if (channelIndex >= KBD_MAX_CHANNELS)
			return KBD_RESULT_INVALID_PARAM;
		const KBDChannel& channel = s_kbdChannels[channelIndex];
		if (channel.status == KBD_CHANNEL_STATUS_DISCONNECTED || channel.status == KBD_CHANNEL_STATUS_REPORT_ERROR || !channel.hid)
			return KBD_RESULT_NOT_CONNECTED;
		return KBD_RESULT_SUCCESS;
	}

	KBDLedTransfer* AllocateLedTransfer()
	{
		for (uint32 i = 0; i < KBD_LED_TRANSFER_COUNT; i++)
		{
			if (!s_ledTransfersInUse.test(i))
			{
				s_ledTransfersInUse.set(i);
				return s_ledTransfers.GetPtr() + i;
			}
		}
		return nullptr;
	}

	void FreeLedTransfer(KBDLedTransfer* transfer)
	{
		s_ledTransfersInUse.reset(transfer - s_ledTransfers.GetPtr());
	}

	// nsyshid transfer callback: (uint32 handle, sint32 error, uint8* buffer, uint32 bytesTransferred, KBDLedTransfer* transfer)
	void ppcLedTransferCallback(PPCInterpreter_t* hCPU)
	{
		ppcDefineParamS32(error, 1);
		ppcDefineParamTypePtr(transfer, KBDLedTransfer, 4);
		MPTR callback;
		{
			std::unique_lock _l(s_mutex);
			if (error == 0)
				s_kbdChannels[transfer->request.channel % KBD_MAX_CHANNELS].currentLeds = transfer->request.leds;
			transfer->request.result = error;
			callback = transfer->callback;
		}
		if (callback != MPTR_NULL)
			PPCCoreCallback(callback, &transfer->request, (uint32)transfer->context);
		std::unique_lock _l(s_mutex);
		FreeLedTransfer(transfer);
		osLib_returnFromFunction(hCPU, 0);
	}

	sint32 KBDSetLedsAsyncEx(uint32 channelIndex, uint8 leds, MPTR callback, MPTR context)
	{
		std::unique_lock _l(s_mutex);
		KBDResult result = CheckLedChannel(channelIndex);
		if (result != KBD_RESULT_SUCCESS)
			return result;
		KBDLedTransfer* transfer = AllocateLedTransfer();
		if (!transfer)
			return KBD_RESULT_BUSY;
		*transfer = {};
		transfer->report = leds;
		transfer->request.channel = (uint8)channelIndex;
		transfer->request.leds = leds;
		transfer->request.result = 1; // pending
		transfer->callback = callback;
		transfer->context = context;
		const sint32 hidResult = nsyshid::HIDSetReport(s_kbdChannels[channelIndex].hid->handle, HID_REPORT_TYPE_OUTPUT, 0, &transfer->report, 1, s_ledTransferCallbackFunc, memory_getVirtualOffsetFromPointer(transfer));
		if (hidResult != 0)
		{
			FreeLedTransfer(transfer);
			return KBD_RESULT_BUSY;
		}
		return KBD_RESULT_SUCCESS;
	}

	sint32 KBDSetLedsEx(uint32 channelIndex, uint8 leds)
	{
		KBDLedTransfer* transfer;
		uint32 hidHandle;
		{
			std::unique_lock _l(s_mutex);
			KBDResult result = CheckLedChannel(channelIndex);
			if (result != KBD_RESULT_SUCCESS)
				return result;
			transfer = AllocateLedTransfer();
			if (!transfer)
				return KBD_RESULT_BUSY;
			transfer->report = leds;
			hidHandle = s_kbdChannels[channelIndex].hid->handle;
		}
		// synchronous transfers block the calling thread, so the lock can't be held here
		const bool success = nsyshid::HIDSetReport(hidHandle, HID_REPORT_TYPE_OUTPUT, 0, &transfer->report, 1, MPTR_NULL, MPTR_NULL) > 0;
		std::unique_lock _l(s_mutex);
		if (success && s_kbdChannels[channelIndex].hid)
			s_kbdChannels[channelIndex].currentLeds = leds;
		FreeLedTransfer(transfer);
		return KBD_RESULT_SUCCESS;
	}

	void ArmLedRetry(uint32 channelIndex)
	{
		coreinit::OSAlarm_t* alarm = s_ledRetryAlarms.GetPtr() + channelIndex;
		coreinit::OSCancelAlarm(alarm);
		coreinit::OSSetAlarmUserData(alarm, channelIndex);
		coreinit::OSSetAlarm(alarm, ESPRESSO_TIMER_CLOCK, s_ledRetryAlarmHandlerFunc);
	}

	// KBDSetLedsAsyncEx callback: (KBDLedRequest* request, uint32 channelIndex)
	void ppcLedRetryCallback(PPCInterpreter_t* hCPU)
	{
		ppcDefineParamTypePtr(request, KBDLedRequest, 0);
		ppcDefineParamU32(channelIndex, 1);
		std::unique_lock _l(s_mutex);
		if (request->result != 0)
			s_kbdChannels[channelIndex % KBD_MAX_CHANNELS].pendingLeds = KBD_LEDS_UNKNOWN;
		osLib_returnFromFunction(hCPU, 0);
	}

	// OSAlarm handler: (OSAlarm* alarm, OSContext* context)
	void ppcLedRetryAlarmHandler(PPCInterpreter_t* hCPU)
	{
		ppcDefineParamTypePtr(alarm, coreinit::OSAlarm_t, 0);
		const uint32 channelIndex = alarm->userData % KBD_MAX_CHANNELS;
		std::unique_lock _l(s_mutex);
		const uint32 pendingLeds = s_kbdChannels[channelIndex].pendingLeds;
		if (pendingLeds != KBD_LEDS_UNKNOWN)
			KBDSetLedsRetryEx(channelIndex, (uint8)pendingLeds);
		osLib_returnFromFunction(hCPU, 0);
	}

	// like KBDSetLedsAsyncEx, but retries every second while no transfer is available
	sint32 KBDSetLedsRetryEx(uint32 channelIndex, uint8 leds)
	{
		std::unique_lock _l(s_mutex);
		KBDResult result = CheckLedChannel(channelIndex);
		if (result != KBD_RESULT_SUCCESS)
			return result;
		KBDChannel& channel = s_kbdChannels[channelIndex];
		channel.pendingLeds = leds;
		const sint32 asyncResult = KBDSetLedsAsyncEx(channelIndex, leds, s_ledRetryCallbackFunc, channelIndex);
		if (asyncResult == KBD_RESULT_BUSY)
			ArmLedRetry(channelIndex);
		else if (asyncResult != KBD_RESULT_SUCCESS)
			channel.pendingLeds = KBD_LEDS_UNKNOWN;
		return asyncResult;
	}

	sint32 KBDSetLeds(uint32 channelIndex, uint8 leds)
	{
		cemuLog_log(LogType::APIErrors, "KBDSetLeds(): LEDs are set by KBD RPL, call KBDSetLedsEx if you really need to set LEDs");
		return KBD_RESULT_SUCCESS;
	}

	sint32 KBDSetLedsAsync(uint32 channelIndex, uint8 leds, MPTR callback, MPTR context)
	{
		cemuLog_log(LogType::APIErrors, "KBDSetLedsAsync(): LEDs are set by KBD RPL, call KBDSetLedsAsyncEx if you really need to set LEDs");
		return KBD_RESULT_SUCCESS;
	}

	sint32 KBDSetLedsRetry(uint32 channelIndex, uint8 leds)
	{
		cemuLog_log(LogType::APIErrors, "KBDSetLedsRetry(): LEDs are set by KBD RPL, call KBDSetLedsRetryEx if you really need to set LEDs");
		return KBD_RESULT_SUCCESS;
	}

	void KBDInitRegionUS()
	{
		cemuLog_log(LogType::APIErrors, "KBDInitRegionUS() is deprecated");
	}

	void KBDInitRegionJP()
	{
		cemuLog_log(LogType::APIErrors, "KBDInitRegionJP() is deprecated");
	}

	void KBDInitRegionEU()
	{
		cemuLog_log(LogType::APIErrors, "KBDInitRegionEU() is deprecated");
	}

	/* SKBD */

	sint32 SKBDSetup(MPTR keyCallback)
	{
		std::unique_lock _l(s_mutex);
		for (auto& channel : s_skbdChannels)
			channel.keyCallback = keyCallback;
		return KBD_RESULT_SUCCESS;
	}

	sint32 SKBDTeardown()
	{
		std::unique_lock _l(s_mutex);
		for (auto& channel : s_skbdChannels)
			channel.keyCallback = MPTR_NULL;
		return KBD_RESULT_SUCCESS;
	}

	// 0 = off, 1 = system keyboard settings are copied to KBD, 2 = KBD settings are copied to the system keyboard
	// while active, reports from attached keyboards are also processed by the SKBD channels
	void SKBDSetMode(sint32 mode)
	{
		bool addClient = false;
		bool removeClient = false;
		{
			std::unique_lock _l(s_mutex);
			if (s_skbdMode == mode)
				return;
			removeClient = s_skbdMode > 0 && mode == 0;
			s_skbdMode = mode;
			if (mode == 1)
			{
				for (uint32 i = 0; i < KBD_MAX_CHANNELS; i++)
				{
					CopyChannelSettings(s_kbdChannels[i], s_skbdChannels[i]);
					UpdateLeds(s_kbdChannels[i]);
				}
				addClient = true;
			}
			else if (mode == 2)
			{
				for (uint32 i = 0; i < KBD_MAX_CHANNELS; i++)
					CopyChannelSettings(s_skbdChannels[i], s_kbdChannels[i]);
				addClient = true;
			}
		}
		if (removeClient)
			RemovePassiveClient();
		if (addClient)
			AddPassiveClient();
	}

	// lets system software inject a boot protocol keyboard report
	void SKBDKeyEvent(uint32 channelIndex, uint8* report)
	{
		KBDChannel* channel = GetChannel(s_skbdChannels, channelIndex, "SKBDKeyEvent");
		if (!channel)
			return;
		std::vector<PendingKeyEvent> pendingEvents;
		{
			std::unique_lock _l(s_mutex);
			ProcessReport(*channel, report, pendingEvents);
			if (s_skbdMode != 0)
				ProcessReport(s_kbdChannels[channelIndex], report, pendingEvents);
		}
		DispatchPendingEvents(pendingEvents);
	}

	sint32 SKBDGetChannelStatus(uint32 channelIndex, uint32be* status)
	{
		KBDChannel* channel = GetChannel(s_skbdChannels, channelIndex, "SKBDGetChannelStatus");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		std::unique_lock _l(s_mutex);
		*status = channel->status;
		return KBD_RESULT_SUCCESS;
	}

	sint32 SKBDGetKey(uint32 channelIndex, KBDKeyEvent* keyEvent)
	{
		KBDChannel* channel = GetChannel(s_skbdChannels, channelIndex, "SKBDGetKey");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		return GetKey(*channel, keyEvent);
	}

	sint32 SKBDEmptyQueue(uint32 channelIndex)
	{
		KBDChannel* channel = GetChannel(s_skbdChannels, channelIndex, "SKBDEmptyQueue");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		return EmptyQueue(*channel);
	}

	sint32 SKBDResetChannel(uint32 channelIndex)
	{
		KBDChannel* channel = GetChannel(s_skbdChannels, channelIndex, "SKBDResetChannel");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		std::unique_lock _l(s_mutex);
		ResetChannel(*channel);
		return KBD_RESULT_SUCCESS;
	}

	sint32 SKBDSetCountry(uint32 channelIndex, sint32 country)
	{
		KBDChannel* channel = GetChannel(s_skbdChannels, channelIndex, "SKBDSetCountry");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		return SetCountry(*channel, country);
	}

	sint32 SKBDGetCountry(uint32 channelIndex, uint32be* country)
	{
		KBDChannel* channel = GetChannel(s_skbdChannels, channelIndex, "SKBDGetCountry");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		*country = channel->country;
		return KBD_RESULT_SUCCESS;
	}

	sint32 SKBDSetLockProcessing(uint32 channelIndex, uint32 enable)
	{
		KBDChannel* channel = GetChannel(s_skbdChannels, channelIndex, "SKBDSetLockProcessing");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		channel->lockProcessing = enable;
		return KBD_RESULT_SUCCESS;
	}

	sint32 SKBDGetLockProcessing(uint32 channelIndex, uint32be* enable)
	{
		KBDChannel* channel = GetChannel(s_skbdChannels, channelIndex, "SKBDGetLockProcessing");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		*enable = channel->lockProcessing;
		return KBD_RESULT_SUCCESS;
	}

	sint32 SKBDSetModState(uint32 channelIndex, uint32 modState)
	{
		KBDChannel* channel = GetChannel(s_skbdChannels, channelIndex, "SKBDSetModState");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		std::unique_lock _l(s_mutex);
		SetModState(*channel, modState);
		return KBD_RESULT_SUCCESS;
	}

	sint32 SKBDGetModState(uint32 channelIndex, uint32be* modState)
	{
		KBDChannel* channel = GetChannel(s_skbdChannels, channelIndex, "SKBDGetModState");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		*modState = channel->modState;
		return KBD_RESULT_SUCCESS;
	}

	sint32 SKBDSetRepeat(uint32 channelIndex, uint16 delay, uint16 rate)
	{
		KBDChannel* channel = GetChannel(s_skbdChannels, channelIndex, "SKBDSetRepeat");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		channel->repeatDelay = delay;
		channel->repeatRate = rate;
		return KBD_RESULT_SUCCESS;
	}

	sint32 SKBDGetRepeat(uint32 channelIndex, uint16be* delay, uint16be* rate)
	{
		KBDChannel* channel = GetChannel(s_skbdChannels, channelIndex, "SKBDGetRepeat");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		*delay = channel->repeatDelay;
		*rate = channel->repeatRate;
		return KBD_RESULT_SUCCESS;
	}

	sint32 SKBDSetAccessSticky(uint32 channelIndex, uint32 enable)
	{
		KBDChannel* channel = GetChannel(s_skbdChannels, channelIndex, "SKBDSetAccessSticky");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		channel->accessSticky = enable;
		return KBD_RESULT_SUCCESS;
	}

	sint32 SKBDGetAccessSticky(uint32 channelIndex, uint32be* enable)
	{
		KBDChannel* channel = GetChannel(s_skbdChannels, channelIndex, "SKBDGetAccessSticky");
		if (!channel)
			return KBD_RESULT_SUCCESS;
		*enable = channel->accessSticky;
		return KBD_RESULT_SUCCESS;
	}

	/* KPR */

	// code page 437, used for alt codes 128-255
	constexpr uint16 s_cp437HighChars[128] = {
		0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, 0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
		0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
		0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA, 0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
		0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
		0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
		0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
		0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4, 0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
		0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248, 0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0,
	};

	// windows-1252, used for alt codes 0128-0159
	constexpr uint16 s_cp1252C1Chars[32] = {
		0x20AC, 0x0000, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x0000, 0x017D, 0x0000,
		0x0000, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x0000, 0x017E, 0x0178,
	};

	struct DeadKeyComposition
	{
		uint16 deadKey;
		uint16 base; // same as deadKey for the standalone character
		uint16 result;
	};

	constexpr DeadKeyComposition s_deadKeyCompositions[] = {
		// grave
		{0x0300, 0x0300, 0x0060}, {0x0300, 'A', 0x00C0}, {0x0300, 'E', 0x00C8}, {0x0300, 'I', 0x00CC}, {0x0300, 'O', 0x00D2}, {0x0300, 'U', 0x00D9},
		{0x0300, 'a', 0x00E0}, {0x0300, 'e', 0x00E8}, {0x0300, 'i', 0x00EC}, {0x0300, 'o', 0x00F2}, {0x0300, 'u', 0x00F9},
		// acute
		{0x0301, 0x0301, 0x00B4}, {0x0301, 'A', 0x00C1}, {0x0301, 'E', 0x00C9}, {0x0301, 'I', 0x00CD}, {0x0301, 'O', 0x00D3}, {0x0301, 'U', 0x00DA}, {0x0301, 'Y', 0x00DD},
		{0x0301, 'a', 0x00E1}, {0x0301, 'e', 0x00E9}, {0x0301, 'i', 0x00ED}, {0x0301, 'o', 0x00F3}, {0x0301, 'u', 0x00FA}, {0x0301, 'y', 0x00FD},
		// circumflex
		{0x0302, 0x0302, 0x005E}, {0x0302, 'A', 0x00C2}, {0x0302, 'E', 0x00CA}, {0x0302, 'I', 0x00CE}, {0x0302, 'O', 0x00D4}, {0x0302, 'U', 0x00DB},
		{0x0302, 'a', 0x00E2}, {0x0302, 'e', 0x00EA}, {0x0302, 'i', 0x00EE}, {0x0302, 'o', 0x00F4}, {0x0302, 'u', 0x00FB},
		// tilde
		{0x0303, 0x0303, 0x007E}, {0x0303, 'A', 0x00C3}, {0x0303, 'N', 0x00D1}, {0x0303, 'O', 0x00D5}, {0x0303, 'a', 0x00E3}, {0x0303, 'n', 0x00F1}, {0x0303, 'o', 0x00F5},
		// diaeresis
		{0x0308, 0x0308, 0x00A8}, {0x0308, 'A', 0x00C4}, {0x0308, 'E', 0x00CB}, {0x0308, 'I', 0x00CF}, {0x0308, 'O', 0x00D6}, {0x0308, 'U', 0x00DC}, {0x0308, 'Y', 0x0178},
		{0x0308, 'a', 0x00E4}, {0x0308, 'e', 0x00EB}, {0x0308, 'i', 0x00EF}, {0x0308, 'o', 0x00F6}, {0x0308, 'u', 0x00FC}, {0x0308, 'y', 0x00FF},
		// cedilla
		{0x0327, 0x0327, 0x00B8}, {0x0327, 'C', 0x00C7}, {0x0327, 'c', 0x00E7},
		// apostrophe dead key, acute or cedilla depending on the letter
		{0x030D, 0x030D, 0x0027}, {0x030D, 'A', 0x00C1}, {0x030D, 'E', 0x00C9}, {0x030D, 'I', 0x00CD}, {0x030D, 'O', 0x00D3}, {0x030D, 'U', 0x00DA}, {0x030D, 'Y', 0x00DD}, {0x030D, 'C', 0x00C7},
		{0x030D, 'a', 0x00E1}, {0x030D, 'e', 0x00E9}, {0x030D, 'i', 0x00ED}, {0x030D, 'o', 0x00F3}, {0x030D, 'u', 0x00FA}, {0x030D, 'y', 0x00FD}, {0x030D, 'c', 0x00E7},
		// quotation mark dead key, diaeresis
		{0x030E, 0x030E, 0x0022}, {0x030E, 'A', 0x00C4}, {0x030E, 'E', 0x00CB}, {0x030E, 'I', 0x00CF}, {0x030E, 'O', 0x00D6}, {0x030E, 'U', 0x00DC}, {0x030E, 'Y', 0x0178},
		{0x030E, 'a', 0x00E4}, {0x030E, 'e', 0x00EB}, {0x030E, 'i', 0x00EF}, {0x030E, 'o', 0x00F6}, {0x030E, 'u', 0x00FC}, {0x030E, 'y', 0x00FF},
		// greek dialytika tonos, dialytika and tonos
		{0x0344, 0x0344, 0x0385}, {0x0344, 0x03B9, 0x0390}, {0x0344, 0x03C5, 0x03B0},
		{0x0308, 0x0399, 0x03AA}, {0x0308, 0x03A5, 0x03AB}, {0x0308, 0x03B9, 0x03CA}, {0x0308, 0x03C5, 0x03CB},
		{0x0301, 0x0391, 0x0386}, {0x0301, 0x0395, 0x0388}, {0x0301, 0x0397, 0x0389}, {0x0301, 0x0399, 0x038A}, {0x0301, 0x039F, 0x038C}, {0x0301, 0x03A5, 0x038E}, {0x0301, 0x03A9, 0x038F},
		{0x0301, 0x03B1, 0x03AC}, {0x0301, 0x03B5, 0x03AD}, {0x0301, 0x03B7, 0x03AE}, {0x0301, 0x03B9, 0x03AF}, {0x0301, 0x03BF, 0x03CC}, {0x0301, 0x03C5, 0x03CD}, {0x0301, 0x03C9, 0x03CE},
	};

	// handles alt+numpad codes, returns true if the char was consumed
	bool KPRProcessAltCode(KPRQueue* queue, uint16 c)
	{
		uint32 altCode = queue->altCode;
		const uint32 digit = (uint32)c - KBK_KEYPAD_0;
		if (altCode == 0)
		{
			if (digit < 10)
			{
				// a leading zero selects windows-1252 instead of code page 437
				queue->altCode = (c == KBK_KEYPAD_0) ? 0x80000000 : digit;
				return true;
			}
			return c == 0;
		}
		const uint32 leadingZero = altCode & 0x80000000;
		uint32 value = altCode & 0x7FFFFFFF;
		queue->altCode = value;
		if (digit < 10)
		{
			if (value <= 0x6666666)
				queue->altCode = (10 * value + digit) | leadingZero;
			return true;
		}
		// any other char completes the code
		uint16 result;
		if (value >= 128 && value <= 255)
		{
			if (!leadingZero)
				result = s_cp437HighChars[value - 128];
			else if (value < 160)
				result = s_cp1252C1Chars[value - 128];
			else
				result = (uint16)value;
		}
		else if (value > 255)
		{
			result = (value > 0x6666666) ? ' ' : (uint8)value;
		}
		else
		{
			result = (uint16)value;
		}
		const uint8 numReady = queue->numReady;
		for (uint32 i = numReady + queue->numPending; i > numReady; i--)
			queue->chars[i] = queue->chars[i - 1];
		queue->chars[numReady] = result;
		queue->numReady = numReady + 1;
		queue->altCode = 0;
		return c == 0;
	}

	void KPRProcessDeadKey(KPRQueue* queue)
	{
		const uint8 numReady = queue->numReady;
		if (queue->numPending == 1)
		{
			const uint16 c = queue->chars[numReady];
			for (const auto& composition : s_deadKeyCompositions)
			{
				if (composition.deadKey == c)
					return; // wait for the next char
			}
			queue->numReady = numReady + 1;
			queue->numPending = 0;
			return;
		}
		const uint16 first = queue->chars[numReady];
		const uint16 second = queue->chars[numReady + 1];
		size_t firstStandalone = 0;
		size_t secondStandalone = std::size(s_deadKeyCompositions);
		for (size_t i = 0; i < std::size(s_deadKeyCompositions); i++)
		{
			const auto& composition = s_deadKeyCompositions[i];
			if (first == composition.deadKey)
			{
				if (second == composition.base)
				{
					queue->chars[numReady] = composition.result;
					queue->numReady = numReady + 1;
					queue->numPending = 0;
					return;
				}
				if (first == composition.base)
					firstStandalone = i;
			}
			if (second == composition.deadKey && second == composition.base)
				secondStandalone = i;
		}
		// no composition, output both as standalone characters
		queue->chars[numReady] = s_deadKeyCompositions[firstStandalone].result;
		if (secondStandalone < std::size(s_deadKeyCompositions))
			queue->chars[numReady + 1] = s_deadKeyCompositions[secondStandalone].result;
		queue->numReady = numReady + 2;
		queue->numPending = 0;
	}

	struct RomajiSyllable
	{
		const char* romaji;
		uint16 kana[2]; // hiragana, the second char is 0 for single char syllables
	};

	// the first match wins, so longer sequences come first
	constexpr RomajiSyllable s_romajiSyllables[] = {
		{"ltsa", 0x3063, 0x3041}, {"ltse", 0x3063, 0x3047}, {"ltsi", 0x3063, 0x3043}, {"ltso", 0x3063, 0x3049}, {"ltsu", 0x3063, 0}, {"lwha", 0x3045, 0x3041},
		{"lwhe", 0x3045, 0x3047}, {"lwhi", 0x3045, 0x3043}, {"lwho", 0x3045, 0x3049}, {"lwhu", 0x3045, 0}, {"xtsa", 0x3063, 0x3041}, {"xtse", 0x3063, 0x3047},
		{"xtsi", 0x3063, 0x3043}, {"xtso", 0x3063, 0x3049}, {"xtsu", 0x3063, 0}, {"xwha", 0x3045, 0x3041}, {"xwhe", 0x3045, 0x3047}, {"xwhi", 0x3045, 0x3043},
		{"xwho", 0x3045, 0x3049}, {"xwhu", 0x3045, 0}, {"bya", 0x3073, 0x3083}, {"bye", 0x3073, 0x3047}, {"byi", 0x3073, 0x3043}, {"byo", 0x3073, 0x3087},
		{"byu", 0x3073, 0x3085}, {"cha", 0x3061, 0x3083}, {"che", 0x3061, 0x3047}, {"chi", 0x3061, 0}, {"cho", 0x3061, 0x3087}, {"chu", 0x3061, 0x3085},
		{"cya", 0x3061, 0x3083}, {"cye", 0x3061, 0x3047}, {"cyi", 0x3061, 0x3043}, {"cyo", 0x3061, 0x3087}, {"cyu", 0x3061, 0x3085}, {"dha", 0x3067, 0x3083},
		{"dhe", 0x3067, 0x3047}, {"dhi", 0x3067, 0x3043}, {"dho", 0x3067, 0x3087}, {"dhu", 0x3067, 0x3085}, {"dya", 0x3062, 0x3083}, {"dyi", 0x3062, 0x3043},
		{"dye", 0x3062, 0x3047}, {"dyo", 0x3062, 0x3087}, {"dyu", 0x3062, 0x3085}, {"fya", 0x3075, 0x3083}, {"fye", 0x3075, 0x3047}, {"fyi", 0x3075, 0x3043},
		{"fyo", 0x3075, 0x3087}, {"fyu", 0x3075, 0x3085}, {"gya", 0x304E, 0x3083}, {"gye", 0x304E, 0x3047}, {"gyi", 0x304E, 0x3043}, {"gyo", 0x304E, 0x3087},
		{"gyu", 0x304E, 0x3085}, {"hya", 0x3072, 0x3083}, {"hye", 0x3072, 0x3047}, {"hyi", 0x3072, 0x3043}, {"hyo", 0x3072, 0x3087}, {"hyu", 0x3072, 0x3085},
		{"jya", 0x3058, 0x3083}, {"jye", 0x3058, 0x3047}, {"jyi", 0x3058, 0x3043}, {"jyo", 0x3058, 0x3087}, {"jyu", 0x3058, 0x3085}, {"kya", 0x304D, 0x3083},
		{"kye", 0x304D, 0x3047}, {"kyi", 0x304D, 0x3043}, {"kyo", 0x304D, 0x3087}, {"kyu", 0x304D, 0x3085}, {"lya", 0x3083, 0}, {"lye", 0x3047, 0},
		{"lyi", 0x3043, 0}, {"lyo", 0x3087, 0}, {"lyu", 0x3085, 0}, {"ltu", 0x3063, 0}, {"lwa", 0x308E, 0}, {"mya", 0x307F, 0x3083},
		{"mye", 0x307F, 0x3047}, {"myi", 0x307F, 0x3043}, {"myo", 0x307F, 0x3087}, {"myu", 0x307F, 0x3085}, {"nya", 0x306B, 0x3083}, {"nye", 0x306B, 0x3047},
		{"nyi", 0x306B, 0x3043}, {"nyo", 0x306B, 0x3087}, {"nyu", 0x306B, 0x3085}, {"pya", 0x3074, 0x3083}, {"pye", 0x3074, 0x3047}, {"pyi", 0x3074, 0x3043},
		{"pyo", 0x3074, 0x3087}, {"pyu", 0x3074, 0x3085}, {"qya", 0x304F, 0x3083}, {"qye", 0x304F, 0x3047}, {"qyi", 0x304F, 0x3043}, {"qyo", 0x304F, 0x3087},
		{"qyu", 0x304F, 0x3085}, {"rya", 0x308A, 0x3083}, {"rye", 0x308A, 0x3047}, {"ryi", 0x308A, 0x3043}, {"ryo", 0x308A, 0x3087}, {"ryu", 0x308A, 0x3085},
		{"sha", 0x3057, 0x3083}, {"she", 0x3057, 0x3047}, {"shi", 0x3057, 0}, {"sho", 0x3057, 0x3087}, {"shu", 0x3057, 0x3085}, {"sya", 0x3057, 0x3083},
		{"sye", 0x3057, 0x3047}, {"syi", 0x3057, 0x3043}, {"syo", 0x3057, 0x3087}, {"syu", 0x3057, 0x3085}, {"tha", 0x3066, 0x3083}, {"the", 0x3066, 0x3047},
		{"thi", 0x3066, 0x3043}, {"tho", 0x3066, 0x3087}, {"thu", 0x3066, 0x3085}, {"tsa", 0x3064, 0x3041}, {"tse", 0x3064, 0x3047}, {"tsi", 0x3064, 0x3043},
		{"tso", 0x3064, 0x3049}, {"tsu", 0x3064, 0}, {"tya", 0x3061, 0x3083}, {"tye", 0x3061, 0x3047}, {"tyi", 0x3061, 0x3043}, {"tyo", 0x3061, 0x3087},
		{"tyu", 0x3061, 0x3085}, {"vya", 0x30F4, 0x3083}, {"vye", 0x30F4, 0x3047}, {"vyi", 0x30F4, 0x3043}, {"vyo", 0x30F4, 0x3087}, {"vyu", 0x30F4, 0x3085},
		{"wha", 0x3046, 0x3041}, {"whe", 0x3046, 0x3047}, {"whi", 0x3046, 0x3043}, {"who", 0x3046, 0x3049}, {"whu", 0x3046, 0}, {"xya", 0x3083, 0},
		{"xye", 0x3047, 0}, {"xyi", 0x3043, 0}, {"xyo", 0x3087, 0}, {"xyu", 0x3085, 0}, {"xtu", 0x3063, 0}, {"xwa", 0x308E, 0},
		{"zya", 0x3058, 0x3083}, {"zye", 0x3058, 0x3047}, {"zyi", 0x3058, 0x3043}, {"zyo", 0x3058, 0x3087}, {"zyu", 0x3058, 0x3085}, {"ba", 0x3070, 0},
		{"be", 0x3079, 0}, {"bi", 0x3073, 0}, {"bo", 0x307C, 0}, {"bu", 0x3076, 0}, {"ca", 0x304B, 0}, {"ce", 0x305B, 0},
		{"ci", 0x3057, 0}, {"co", 0x3053, 0}, {"cu", 0x304F, 0}, {"da", 0x3060, 0}, {"de", 0x3067, 0}, {"di", 0x3062, 0},
		{"do", 0x3069, 0}, {"du", 0x3065, 0}, {"fa", 0x3075, 0x3041}, {"fe", 0x3075, 0x3047}, {"fi", 0x3075, 0x3043}, {"fo", 0x3075, 0x3049},
		{"fu", 0x3075, 0}, {"ga", 0x304C, 0}, {"ge", 0x3052, 0}, {"gi", 0x304E, 0}, {"go", 0x3054, 0}, {"gu", 0x3050, 0},
		{"ha", 0x306F, 0}, {"he", 0x3078, 0}, {"hi", 0x3072, 0}, {"ho", 0x307B, 0}, {"hu", 0x3075, 0}, {"ja", 0x3058, 0x3083},
		{"je", 0x3058, 0x3047}, {"ji", 0x3058, 0}, {"jo", 0x3058, 0x3087}, {"ju", 0x3058, 0x3085}, {"ka", 0x304B, 0}, {"ke", 0x3051, 0},
		{"ki", 0x304D, 0}, {"ko", 0x3053, 0}, {"ku", 0x304F, 0}, {"la", 0x3041, 0}, {"le", 0x3047, 0}, {"li", 0x3043, 0},
		{"lo", 0x3049, 0}, {"lu", 0x3045, 0}, {"ma", 0x307E, 0}, {"me", 0x3081, 0}, {"mi", 0x307F, 0}, {"mo", 0x3082, 0},
		{"mu", 0x3080, 0}, {"na", 0x306A, 0}, {"ne", 0x306D, 0}, {"ni", 0x306B, 0}, {"no", 0x306E, 0}, {"nu", 0x306C, 0},
		{"pa", 0x3071, 0}, {"pe", 0x307A, 0}, {"pi", 0x3074, 0}, {"po", 0x307D, 0}, {"pu", 0x3077, 0}, {"qa", 0x304F, 0x3041},
		{"qe", 0x304F, 0x3047}, {"qi", 0x304F, 0x3043}, {"qo", 0x304F, 0x3049}, {"qu", 0x304F, 0}, {"ra", 0x3089, 0}, {"re", 0x308C, 0},
		{"ri", 0x308A, 0}, {"ro", 0x308D, 0}, {"ru", 0x308B, 0}, {"sa", 0x3055, 0}, {"se", 0x305B, 0}, {"si", 0x3057, 0},
		{"so", 0x305D, 0}, {"su", 0x3059, 0}, {"ta", 0x305F, 0}, {"te", 0x3066, 0}, {"ti", 0x3061, 0}, {"to", 0x3068, 0},
		{"tu", 0x3064, 0}, {"va", 0x30F4, 0x3041}, {"ve", 0x30F4, 0x3047}, {"vi", 0x30F4, 0x3043}, {"vo", 0x30F4, 0x3049}, {"vu", 0x30F4, 0},
		{"wa", 0x308F, 0}, {"we", 0x3046, 0x3047}, {"wi", 0x3046, 0x3043}, {"wo", 0x3092, 0}, {"wu", 0x3046, 0}, {"xa", 0x3041, 0},
		{"xe", 0x3047, 0}, {"xi", 0x3043, 0}, {"xo", 0x3049, 0}, {"xu", 0x3045, 0}, {"ya", 0x3084, 0}, {"ye", 0x3044, 0x3047},
		{"yi", 0x3044, 0}, {"yo", 0x3088, 0}, {"yu", 0x3086, 0}, {"za", 0x3056, 0}, {"ze", 0x305C, 0}, {"zi", 0x3058, 0},
		{"zo", 0x305E, 0}, {"zu", 0x305A, 0}, {"nn", 0x3093, 0}, {"a", 0x3042, 0}, {"e", 0x3048, 0}, {"i", 0x3044, 0},
		{"o", 0x304A, 0}, {"u", 0x3046, 0},
	};

	constexpr uint16 s_kanaN[2] = {0x3093, 0x30F3}; // hiragana and katakana
	constexpr uint16 s_kanaSmallTsu[2] = {0x3063, 0x30C3};

	bool IsRomajiVowel(uint16 c)
	{
		return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
	}

	// consonants which form a syllable start together with a following h (cha, sha, dha, tha, wha)
	bool IsRomajiHConsonant(uint16 c)
	{
		return c == 'c' || c == 's' || c == 'd' || c == 't' || c == 'w';
	}

	// whether two pending chars without a matching syllable can still start one
	bool IsRomajiPrefix(uint16 c0, uint16 c1)
	{
		if (c0 == c1)
			return true;
		if (c0 != 'w' && (c1 == 'y' || (c0 == 't' && c1 == 's')))
			return true;
		if (c1 == 'h' && IsRomajiHConsonant(c0))
			return true;
		return (c0 == 'l' || c0 == 'x') && (c1 == 't' || c1 == 'w');
	}

	bool IsRomajiPrefix(uint16 c0, uint16 c1, uint16 c2)
	{
		if ((c0 == 'l' || c0 == 'x') && ((c1 == 't' && c2 == 's') || (c1 == 'w' && c2 == 'h')))
			return true;
		if (c0 != c1)
			return false;
		// doubled consonant
		if (c1 != 'w' && c1 != 'y' && (c2 == 'y' || (c1 == 't' && c2 == 's')))
			return true;
		if (c2 == 'h' && IsRomajiHConsonant(c1))
			return true;
		return (c1 == 'l' || c1 == 'x') && (c2 == 't' || c2 == 'w');
	}

	// turns the pending chars into kana once they end in a syllable. Leading chars which can no longer be part of one
	// are passed through as they are
	void KPRProcessRomaji(KPRQueue* queue)
	{
		uint8 numReady = queue->numReady;
		uint8 numPending = queue->numPending;
		const uint32 kanaSet = (queue->mode & KPR_MODE_ROMAJI_KATAKANA) ? 1 : 0;
		const uint32 end = numReady + numPending;
		if ((uint32)(queue->chars[end - 1] - 'a') >= 26)
		{
			// anything but a lowercase letter ends the composition and turns a pending n into a kana
			if (numPending > 1 && queue->chars[end - 2] == 'n')
				queue->chars[end - 2] = s_kanaN[kanaSet];
			queue->numReady = end;
			queue->numPending = 0;
			return;
		}
		for (const RomajiSyllable& syllable : s_romajiSyllables)
		{
			const uint32 length = (uint32)strlen(syllable.romaji);
			if (numPending < length)
				continue;
			uint32 matched = 0;
			while (matched < length && queue->chars[end - length + matched] == (uint16)syllable.romaji[matched])
				matched++;
			if (matched != length)
				continue;
			// a doubled consonant in front of the syllable becomes a small tsu
			if (numPending > length && numPending - length != 2 && queue->chars[numReady] == queue->chars[numReady + 1])
				queue->chars[numReady] = s_kanaSmallTsu[kanaSet];
			uint32 pos = end - length;
			for (uint16 kana : syllable.kana)
			{
				if (kana == 0)
					break;
				// syllables which only exist in katakana are stored as such
				queue->chars[pos++] = (kana < 0x30A1) ? (uint16)(kana + 0x60 * kanaSet) : kana;
			}
			queue->numReady = pos;
			queue->numPending = 0;
			return;
		}
		if (numPending == 2)
		{
			// n followed by a consonant other than y
			const uint16 c0 = queue->chars[numReady];
			const uint16 c1 = queue->chars[numReady + 1];
			if (c0 == 'n' && !IsRomajiVowel(c1))
			{
				if (c1 != 'y')
				{
					queue->chars[numReady] = s_kanaN[kanaSet];
					queue->numReady = numReady + 1;
					queue->numPending = 1;
				}
				return;
			}
		}
		else
		{
			if (numPending == 4)
			{
				numReady++;
				numPending--;
			}
			if (numPending == 3 && !IsRomajiPrefix(queue->chars[numReady], queue->chars[numReady + 1], queue->chars[numReady + 2]))
			{
				numReady++;
				numPending--;
			}
			queue->numReady = numReady;
			queue->numPending = numPending;
			if (numPending != 2)
				return;
		}
		if (!IsRomajiPrefix(queue->chars[numReady], queue->chars[numReady + 1]))
		{
			queue->numReady = numReady + 1;
			queue->numPending = numPending - 1;
		}
	}

	void KPRClearQueue(KPRQueue* queue)
	{
		queue->numReady = 0;
		queue->numPending = 0;
		queue->altCode = 0;
	}

	void KPRSetMode(KPRQueue* queue, uint32 mode)
	{
		queue->mode = mode;
		KPRClearQueue(queue);
	}

	void KPRInitQueue(KPRQueue* queue)
	{
		KPRSetMode(queue, KPR_MODE_ALT_CODE);
	}

	uint32 KPRGetMode(KPRQueue* queue)
	{
		return queue->mode;
	}

	uint8 KPRPutChar(KPRQueue* queue, uint16 c)
	{
		if (queue->numReady + queue->numPending + 1 >= 5)
		{
			cemuLog_log(LogType::APIErrors, "KPRPutChar(): Queue overflow");
			return queue->numReady;
		}
		const uint32 mode = queue->mode;
		if ((mode & KPR_MODE_ALT_CODE) && KPRProcessAltCode(queue, c))
			return queue->numReady;
		const uint8 numPending = queue->numPending + 1;
		queue->chars[queue->numReady + queue->numPending] = c;
		queue->numPending = numPending;
		if (mode & KPR_MODE_DEAD_KEY)
		{
			KPRProcessDeadKey(queue);
		}
		else if (mode & (KPR_MODE_ROMAJI_HIRAGANA | KPR_MODE_ROMAJI_KATAKANA))
		{
			KPRProcessRomaji(queue);
		}
		else
		{
			// nsyskbd overwrites the ready count here instead of adding to it
			queue->numReady = numPending;
			queue->numPending = 0;
		}
		if (c == KBK_INVALID)
			queue->numReady = queue->numReady - 1;
		return queue->numReady;
	}

	uint16 KPRGetChar(KPRQueue* queue)
	{
		const uint8 numReady = queue->numReady;
		if (numReady == 0)
			return 0;
		const uint16 c = queue->chars[0];
		const uint32 numChars = numReady + queue->numPending;
		for (uint32 i = 1; i < numChars; i++)
			queue->chars[i - 1] = queue->chars[i];
		queue->numReady = numReady - 1;
		return c;
	}

	uint16 KPRRemoveChar(KPRQueue* queue)
	{
		const uint8 numPending = queue->numPending;
		if (numPending == 0)
			return 0;
		const uint16 c = queue->chars[queue->numReady + numPending - 1];
		queue->numPending = numPending - 1;
		return c;
	}

	uint8 KPRLookAhead(KPRQueue* queue, uint16be* buffer, uint32 maxChars)
	{
		const uint32 numChars = queue->numReady + queue->numPending;
		if (buffer && maxChars != 0)
		{
			for (uint32 i = 0; i < maxChars; i++)
			{
				if (i >= numChars)
				{
					buffer[i] = 0;
					break;
				}
				buffer[i] = queue->chars[i];
			}
		}
		return (uint8)numChars;
	}

	void KPRInitRegionUS()
	{
		cemuLog_log(LogType::APIErrors, "KPRInitRegionUS() is deprecated");
	}

	void KPRInitRegionJP()
	{
		cemuLog_log(LogType::APIErrors, "KPRInitRegionJP() is deprecated");
	}

	void KPRInitRegionEU()
	{
		cemuLog_log(LogType::APIErrors, "KPRInitRegionEU() is deprecated");
	}

	void Reset()
	{
		s_hidAttachCallbackFunc = RPLLoader_MakePPCCallable(ppcHIDAttachCallback);
		s_hidReadCallbackFunc = RPLLoader_MakePPCCallable(ppcHIDReadCallback);
		s_ledTransferCallbackFunc = RPLLoader_MakePPCCallable(ppcLedTransferCallback);
		s_ledRetryCallbackFunc = RPLLoader_MakePPCCallable(ppcLedRetryCallback);
		s_repeatAlarmHandlerFunc = RPLLoader_MakePPCCallable(ppcRepeatAlarmHandler);
		s_ledRetryAlarmHandlerFunc = RPLLoader_MakePPCCallable(ppcLedRetryAlarmHandler);
		std::unique_lock _l(s_mutex);
		s_kbdInitialized = true;
		for (uint8 i = 0; i < KBD_MAX_CHANNELS; i++)
		{
			InitChannel(s_kbdChannels[i], i, false);
			InitChannel(s_skbdChannels[i], i, true);
			coreinit::OSCreateAlarm(s_ledRetryAlarms.GetPtr() + i);
		}
		s_kbdSetup = false;
		s_stopReading = false;
		s_attachCallback = MPTR_NULL;
		s_detachCallback = MPTR_NULL;
		s_skbdInitialized = true;
		s_skbdMode = 0;
		s_passiveClientCount = 0;
		s_ledTransfersInUse.reset();
		*s_hidClient.GetPtr() = {};
		coreinit::OSInitMutex(s_hidClientMutex.GetPtr());
	}

	class : public COSModule
	{
		public:
		std::string_view GetName() override
		{
			return "nsyskbd";
		}

		std::vector<std::string_view> GetDependencies() override
		{
			return {"coreinit", "nsyshid"};
		}

		void RPLMapped() override
		{
			cafeExportRegister("nsyskbd", KBDSetup, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDInit, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDTeardown, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDExit, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDGetChannelStatus, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDGetKey, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDEmptyQueue, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDResetChannel, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDSetCountry, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDGetCountry, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDSetLockProcessing, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDGetLockProcessing, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDSetModState, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDGetModState, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDSetRepeat, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDGetRepeat, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDSetAccessSticky, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDGetAccessSticky, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDTranslateHidCode, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDSetLeds, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDSetLedsAsync, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDSetLedsRetry, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDSetLedsEx, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDSetLedsAsyncEx, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDSetLedsRetryEx, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDInitRegionUS, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDInitRegionJP, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KBDInitRegionEU, LogType::NSYSKBD);

			cafeExportRegister("nsyskbd", SKBDSetup, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDTeardown, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDSetMode, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDKeyEvent, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDGetChannelStatus, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDGetKey, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDEmptyQueue, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDResetChannel, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDSetCountry, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDGetCountry, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDSetLockProcessing, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDGetLockProcessing, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDSetModState, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDGetModState, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDSetRepeat, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDGetRepeat, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDSetAccessSticky, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", SKBDGetAccessSticky, LogType::NSYSKBD);

			cafeExportRegister("nsyskbd", KPRInitQueue, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KPRClearQueue, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KPRSetMode, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KPRGetMode, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KPRPutChar, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KPRGetChar, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KPRRemoveChar, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KPRLookAhead, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KPRInitRegionUS, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KPRInitRegionJP, LogType::NSYSKBD);
			cafeExportRegister("nsyskbd", KPRInitRegionEU, LogType::NSYSKBD);
		};

		void rpl_entry(uint32 moduleHandle, coreinit::RplEntryReason reason) override
		{
			if (reason == coreinit::RplEntryReason::Loaded)
				Reset();
		}
	}s_COSnsyskbdModule;

	COSModule* GetModule()
	{
		return &s_COSnsyskbdModule;
	}
}
