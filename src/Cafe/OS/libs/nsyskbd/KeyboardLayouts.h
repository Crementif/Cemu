#pragma once

namespace nsyskbd
{
	// translated codes of non-printable keys
	enum KBDSpecialKey : uint16
	{
		KBK_CTRL = 0xF000,
		KBK_SHIFT = 0xF001,
		KBK_ALT = 0xF002,
		KBK_GUI = 0xF003,
		KBK_EXTRA = 0xF004,
		KBK_ALTGR = 0xF005,
		KBK_LANG = 0xF006,
		KBK_NUM_LOCK = 0xF007,
		KBK_CAPS_LOCK = 0xF008,
		KBK_PRINT_SCREEN = 0xF020,
		KBK_SCROLL_LOCK = 0xF021,
		KBK_PAUSE = 0xF022,
		KBK_APPLICATION = 0xF02F,
		KBK_KEYPAD_0 = 0xF130,
		KBK_KEYPAD_9 = 0xF139,
		KBK_INVALID = 0xFFFF,
	};

	constexpr sint32 KBD_COUNTRY_COUNT = 19;
	constexpr uint32 KBD_COUNTRY_US = 17;

	struct KBDLayout
	{
		uint8 numColumns; // first column holds flags, followed by unshifted/shifted pairs per modifier mode
		uint8 flags; // which shift states of the lang1/lang2/altgr columns are available
		const uint16* keyTable; // hid codes 0x04 and up
		const uint16* modifierTable; // hid codes 0xE0 - 0xE7
		uint8 lang1Column;
		uint8 lang2Column;
		uint8 altGrColumn;
		uint8 numKeys;
		uint8 remap[4][2]; // hid codes which are swapped before translation
		uint8 numRemap;
	};

	// column 0 flags: type in the upper two bits, the lower bits mark the columns that caps lock (or num lock) affects
	constexpr uint16 KEY_FLAG_FIXED = 0xC000; // the code in column 1 is returned regardless of modifiers
	constexpr uint16 KEY_FLAG_NUM_LOCK = 0x8000; // shifted column is used while num lock is on
	constexpr uint16 KEY_FLAG_CAPS_TO_LANG1 = 0x4000;
	constexpr uint16 KEY_FLAG_TYPE_MASK = 0xC000;
	constexpr uint16 KEY_CAPS_LETTER = 0x003F;
	constexpr uint16 KEY_KEYPAD = 0x80FF;

	// country ids: 0 US international, 1/2 canadian, 3 danish, 4 finnish, 5 french, 6 german, 7 greek, 8 italian, 9 japanese,
	// 10 latin american, 11 dutch, 12 norwegian, 13 portuguese, 14 spanish, 15 swedish, 16 UK, 17 US, 18 russian
	// countries outside of 0-18 get an empty layout which translates every key to 0
	const KBDLayout& GetLayout(sint32 country);
}
