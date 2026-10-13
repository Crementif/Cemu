#include "KeyboardLayouts.h"

// translation tables of nsyskbd. Column 0 of each key holds its flags, followed by the unshifted and shifted code of
// the default, lang1, lang2 and altgr modes as far as the layout has them

namespace nsyskbd
{
	constexpr uint16 s_layoutUSKeys[100][3] = {
		{KEY_CAPS_LETTER, 'a', 'A'}, {KEY_CAPS_LETTER, 'b', 'B'}, {KEY_CAPS_LETTER, 'c', 'C'}, {KEY_CAPS_LETTER, 'd', 'D'}, // 0x04
		{KEY_CAPS_LETTER, 'e', 'E'}, {KEY_CAPS_LETTER, 'f', 'F'}, {KEY_CAPS_LETTER, 'g', 'G'}, {KEY_CAPS_LETTER, 'h', 'H'}, // 0x08
		{KEY_CAPS_LETTER, 'i', 'I'}, {KEY_CAPS_LETTER, 'j', 'J'}, {KEY_CAPS_LETTER, 'k', 'K'}, {KEY_CAPS_LETTER, 'l', 'L'}, // 0x0C
		{KEY_CAPS_LETTER, 'm', 'M'}, {KEY_CAPS_LETTER, 'n', 'N'}, {KEY_CAPS_LETTER, 'o', 'O'}, {KEY_CAPS_LETTER, 'p', 'P'}, // 0x10
		{KEY_CAPS_LETTER, 'q', 'Q'}, {KEY_CAPS_LETTER, 'r', 'R'}, {KEY_CAPS_LETTER, 's', 'S'}, {KEY_CAPS_LETTER, 't', 'T'}, // 0x14
		{KEY_CAPS_LETTER, 'u', 'U'}, {KEY_CAPS_LETTER, 'v', 'V'}, {KEY_CAPS_LETTER, 'w', 'W'}, {KEY_CAPS_LETTER, 'x', 'X'}, // 0x18
		{KEY_CAPS_LETTER, 'y', 'Y'}, {KEY_CAPS_LETTER, 'z', 'Z'}, {0, '1', '!'}, {0, '2', '@'}, // 0x1C
		{0, '3', '#'}, {0, '4', '$'}, {0, '5', '%'}, {0, '6', '^'}, // 0x20
		{0, '7', '&'}, {0, '8', '*'}, {0, '9', '('}, {0, '0', ')'}, // 0x24
		{0, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9}, // 0x28 enter, escape, backspace, tab
		{0, ' ', ' '}, {0, '-', '_'}, {0, '=', '+'}, {0, '[', '{'}, // 0x2C
		{0, ']', '}'}, {0, '\\', '|'}, {0, 0, 0}, {0, ';', ':'}, // 0x30
		{0, '\'', '"'}, {0, '`', '~'}, {0, ',', '<'}, {0, '.', '>'}, // 0x34
		{0, '/', '?'}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061}, {0, 0xF062, 0xF062}, // 0x38 caps lock, F1, F2
		{0, 0xF063, 0xF063}, {0, 0xF064, 0xF064}, {0, 0xF065, 0xF065}, {0, 0xF066, 0xF066}, // 0x3C F3 - F6
		{0, 0xF067, 0xF067}, {0, 0xF068, 0xF068}, {0, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A}, // 0x40 F7 - F10
		{0, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44 F11, F12
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9}, // 0x48 pause, insert, home, page up
		{0, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6}, // 0x4C delete, end, page down, right
		{0, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50 left, down, up, num lock
		{0, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B}, // 0x54 keypad / * - +
		{0, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131}, {KEY_KEYPAD, 0xF172, 0xF132}, {KEY_KEYPAD, 0xF173, 0xF133}, // 0x58 keypad enter, 1 - 3
		{KEY_KEYPAD, 0xF174, 0xF134}, {KEY_KEYPAD, 0xF175, 0xF135}, {KEY_KEYPAD, 0xF176, 0xF136}, {KEY_KEYPAD, 0xF177, 0xF137}, // 0x5C keypad 4 - 7
		{KEY_KEYPAD, 0xF178, 0xF138}, {KEY_KEYPAD, 0xF179, 0xF139}, {KEY_KEYPAD, 0xF170, 0xF130}, {KEY_KEYPAD, 0xF16E, 0xF12E}, // 0x60 keypad 8, 9, 0, .
		{0, 0, 0}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0}, {0, 0xF13D, 0xF13D}, // 0x64 non-us \, application, power, keypad =
	};

	constexpr uint16 s_layoutUSModifiers[8][3] = {
		{KEY_FLAG_FIXED, KBK_CTRL, KBK_CTRL}, {KEY_FLAG_FIXED, KBK_SHIFT, KBK_SHIFT}, {KEY_FLAG_FIXED, KBK_ALT, KBK_ALT}, {KEY_FLAG_FIXED, KBK_GUI, KBK_GUI},
		{KEY_FLAG_FIXED, KBK_CTRL, KBK_CTRL}, {KEY_FLAG_FIXED, KBK_SHIFT, KBK_SHIFT}, {KEY_FLAG_FIXED, KBK_ALT, KBK_ALT}, {KEY_FLAG_FIXED, KBK_GUI, KBK_GUI},
	};

	constexpr uint16 s_layoutUSInternationalKeys[100][5] = {
		{0x00FF, 'a', 'A', 0x00E1, 0x00C1}, {KEY_CAPS_LETTER, 'b', 'B', 0, 0}, {KEY_CAPS_LETTER, 'c', 'C', 0x00A9, 0x00A2}, {0x00FF, 'd', 'D', 0x00F0, 0x00D0}, // 0x04
		{0x00FF, 'e', 'E', 0x00E9, 0x00C9}, {KEY_CAPS_LETTER, 'f', 'F', 0, 0}, {KEY_CAPS_LETTER, 'g', 'G', 0, 0}, {KEY_CAPS_LETTER, 'h', 'H', 0, 0}, // 0x08
		{0x00FF, 'i', 'I', 0x00ED, 0x00CD}, {KEY_CAPS_LETTER, 'j', 'J', 0, 0}, {KEY_CAPS_LETTER, 'k', 'K', 0, 0}, {0x00FF, 'l', 'L', 0x00F8, 0x00D8}, // 0x0C
		{KEY_CAPS_LETTER, 'm', 'M', 0x00B5, 0}, {0x00FF, 'n', 'N', 0x00F1, 0x00D1}, {0x00FF, 'o', 'O', 0x00F3, 0x00D3}, {0x00FF, 'p', 'P', 0x00F6, 0x00D6}, // 0x10
		{0x00FF, 'q', 'Q', 0x00E4, 0x00C4}, {KEY_CAPS_LETTER, 'r', 'R', 0x00AE, 0}, {KEY_CAPS_LETTER, 's', 'S', 0x00DF, 0x00A7}, {0x00FF, 't', 'T', 0x00FE, 0x00DE}, // 0x14
		{0x00FF, 'u', 'U', 0x00FA, 0x00DA}, {KEY_CAPS_LETTER, 'v', 'V', 0, 0}, {0x00FF, 'w', 'W', 0x00E5, 0x00C5}, {KEY_CAPS_LETTER, 'x', 'X', 0, 0}, // 0x18
		{0x00FF, 'y', 'Y', 0x00FC, 0x00DC}, {0x00FF, 'z', 'Z', 0x00E6, 0x00C6}, {0, '1', '!', 0x00A1, 0x00B9}, {0, '2', '@', 0x00B2, 0}, // 0x1C
		{0, '3', '#', 0x00B3, 0}, {0, '4', '$', 0x00A4, 0x00A3}, {0, '5', '%', 0x20AC, 0}, {0, '6', 0x0302, 0x00BC, 0}, // 0x20
		{0, '7', '&', 0x00BD, 0}, {0, '8', '*', 0x00BE, 0}, {0, '9', '(', 0x2018, 0}, {0, '0', ')', 0x2019, 0}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' ', ' '}, {0, '-', '_', 0x00A5, 0}, {0, '=', '+', 0x00D7, 0x00F7}, {0, '[', '{', 0x00AB, 0}, // 0x2C
		{0, ']', '}', 0x00BB, 0}, {0, '\\', '|', 0x00AC, 0x00A6}, {0, 0, 0, 0, 0}, {0, ';', ':', 0x00B6, 0x00B0}, // 0x30
		{0, 0x030D, 0x030E, 0x00B4, 0x00A8}, {0, 0x0300, 0x0303, 0, 0}, {0x00C0, ',', '<', 0x00E7, 0x00C7}, {0, '.', '>', 0, 0}, // 0x34
		{0, '/', '?', 0x00BF, 0}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0, 0}, {KEY_KEYPAD, 0xF16E, 0xF12E, 0, 0}, // 0x60
		{0, '\\', '|', 0, 0}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutAltGrModifiers5[8][5] = {
		{KEY_FLAG_FIXED, KBK_CTRL, KBK_CTRL, KBK_CTRL, KBK_CTRL}, {KEY_FLAG_FIXED, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT}, {KEY_FLAG_FIXED, KBK_ALT, KBK_ALT, KBK_ALT, KBK_ALT}, {KEY_FLAG_FIXED, KBK_GUI, KBK_GUI, KBK_GUI, KBK_GUI},
		{KEY_FLAG_FIXED, KBK_CTRL, KBK_CTRL, KBK_CTRL, KBK_CTRL}, {KEY_FLAG_FIXED, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT}, {KEY_FLAG_FIXED, KBK_ALTGR, KBK_ALTGR, KBK_ALTGR, KBK_ALTGR}, {KEY_FLAG_FIXED, KBK_GUI, KBK_GUI, KBK_GUI, KBK_GUI},
	};

	constexpr uint16 s_layoutCanadian1Keys[100][4] = {
		{KEY_CAPS_LETTER, 'a', 'A', 0}, {KEY_CAPS_LETTER, 'b', 'B', 0}, {KEY_CAPS_LETTER, 'c', 'C', 0}, {KEY_CAPS_LETTER, 'd', 'D', 0}, // 0x04
		{KEY_CAPS_LETTER, 'e', 'E', 0}, {KEY_CAPS_LETTER, 'f', 'F', 0}, {KEY_CAPS_LETTER, 'g', 'G', 0}, {KEY_CAPS_LETTER, 'h', 'H', 0}, // 0x08
		{KEY_CAPS_LETTER, 'i', 'I', 0}, {KEY_CAPS_LETTER, 'j', 'J', 0}, {KEY_CAPS_LETTER, 'k', 'K', 0}, {KEY_CAPS_LETTER, 'l', 'L', 0}, // 0x0C
		{KEY_CAPS_LETTER, 'm', 'M', 0}, {KEY_CAPS_LETTER, 'n', 'N', 0}, {KEY_CAPS_LETTER, 'o', 'O', 0}, {KEY_CAPS_LETTER, 'p', 'P', 0}, // 0x10
		{KEY_CAPS_LETTER, 'q', 'Q', 0}, {KEY_CAPS_LETTER, 'r', 'R', 0}, {KEY_CAPS_LETTER, 's', 'S', 0}, {KEY_CAPS_LETTER, 't', 'T', 0}, // 0x14
		{KEY_CAPS_LETTER, 'u', 'U', 0}, {KEY_CAPS_LETTER, 'v', 'V', 0}, {KEY_CAPS_LETTER, 'w', 'W', 0}, {KEY_CAPS_LETTER, 'x', 'X', 0x00BB}, // 0x18
		{KEY_CAPS_LETTER, 'y', 'Y', 0}, {KEY_CAPS_LETTER, 'z', 'Z', 0x00AB}, {0, '1', '!', 0}, {0, '2', '@', 0}, // 0x1C
		{0, '3', '#', 0}, {0, '4', '$', 0}, {0, '5', '%', 0}, {0, '6', '?', 0}, // 0x20
		{0, '7', '&', '{'}, {0, '8', '*', '}'}, {0, '9', '(', '['}, {0, '0', ')', ']'}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' '}, {0, '-', '_', 0}, {0, '=', '+', 0x00AC}, {0, 0x0302, 0x0308, 0x0300}, // 0x2C
		{KEY_CAPS_LETTER, 0x00E7, 0x00C7, 0x0303}, {0, 0, 0, 0}, {KEY_CAPS_LETTER, 0x00E0, 0x00C0, 0}, {0, ';', ':', 0x00B0}, // 0x30
		{KEY_CAPS_LETTER, 0x00E8, 0x00C8, 0}, {0, '/', '\\', '|'}, {0, ',', '\'', '<'}, {0, '.', '"', '>'}, // 0x34
		{KEY_CAPS_LETTER, 0x00E9, 0x00C9, 0}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0}, {KEY_KEYPAD, 0xF16E, 0xF12E, 0}, // 0x60
		{KEY_CAPS_LETTER, 0x00F9, 0x00D9, 0}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutAltGrModifiers4[8][4] = {
		{KEY_FLAG_FIXED, KBK_CTRL, KBK_CTRL, KBK_CTRL}, {KEY_FLAG_FIXED, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT}, {KEY_FLAG_FIXED, KBK_ALT, KBK_ALT, KBK_ALT}, {KEY_FLAG_FIXED, KBK_GUI, KBK_GUI, KBK_GUI},
		{KEY_FLAG_FIXED, KBK_CTRL, KBK_CTRL, KBK_CTRL}, {KEY_FLAG_FIXED, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT}, {KEY_FLAG_FIXED, KBK_ALTGR, KBK_ALTGR, KBK_ALTGR}, {KEY_FLAG_FIXED, KBK_GUI, KBK_GUI, KBK_GUI},
	};

	constexpr uint16 s_layoutCanadian2Keys[100][4] = {
		{KEY_CAPS_LETTER, 'a', 'A', 0}, {KEY_CAPS_LETTER, 'b', 'B', 0}, {KEY_CAPS_LETTER, 'c', 'C', 0}, {KEY_CAPS_LETTER, 'd', 'D', 0}, // 0x04
		{KEY_CAPS_LETTER, 'e', 'E', 0}, {KEY_CAPS_LETTER, 'f', 'F', 0}, {KEY_CAPS_LETTER, 'g', 'G', 0}, {KEY_CAPS_LETTER, 'h', 'H', 0}, // 0x08
		{KEY_CAPS_LETTER, 'i', 'I', 0}, {KEY_CAPS_LETTER, 'j', 'J', 0}, {KEY_CAPS_LETTER, 'k', 'K', 0}, {KEY_CAPS_LETTER, 'l', 'L', 0}, // 0x0C
		{KEY_CAPS_LETTER, 'm', 'M', 0x00B5}, {KEY_CAPS_LETTER, 'n', 'N', 0}, {KEY_CAPS_LETTER, 'o', 'O', 0x00A7}, {KEY_CAPS_LETTER, 'p', 'P', 0x00B6}, // 0x10
		{KEY_CAPS_LETTER, 'q', 'Q', 0}, {KEY_CAPS_LETTER, 'r', 'R', 0}, {KEY_CAPS_LETTER, 's', 'S', 0}, {KEY_CAPS_LETTER, 't', 'T', 0}, // 0x14
		{KEY_CAPS_LETTER, 'u', 'U', 0}, {KEY_CAPS_LETTER, 'v', 'V', 0}, {KEY_CAPS_LETTER, 'w', 'W', 0}, {KEY_CAPS_LETTER, 'x', 'X', 0}, // 0x18
		{KEY_CAPS_LETTER, 'y', 'Y', 0}, {KEY_CAPS_LETTER, 'z', 'Z', 0}, {0, '1', '!', 0x00B1}, {0, '2', '"', '@'}, // 0x1C
		{0, '3', '/', 0x00A3}, {0, '4', '$', 0x00A2}, {0, '5', '%', 0x00A4}, {0, '6', '?', 0x00AC}, // 0x20
		{0, '7', '&', 0x00A6}, {0, '8', '*', 0x00B2}, {0, '9', '(', 0x00B3}, {0, '0', ')', 0x00BC}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' '}, {0, '-', '_', 0x00BD}, {0, '=', '+', 0x00BE}, {0, 0x0302, 0x0302, '['}, // 0x2C
		{0, 0x0327, 0x0308, ']'}, {0, 0, 0, 0}, {0, '<', '>', '}'}, {0, ';', ':', '~'}, // 0x30
		{0, 0x0300, 0x0300, '{'}, {0, '#', '|', '\\'}, {0, ',', '\'', 0x00AF}, {0, '.', '.', '-'}, // 0x34
		{KEY_CAPS_LETTER, 0x00E9, 0x00C9, 0x0301}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0}, {KEY_KEYPAD, 0xF16E, 0xF12E, 0}, // 0x60
		{0, 0x00AB, 0x00BB, 0x00B0}, {0, 0xF02F, 0xF02F, 0xF02F}, {0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutDanishKeys[100][4] = {
		{KEY_CAPS_LETTER, 'a', 'A', 0}, {KEY_CAPS_LETTER, 'b', 'B', 0}, {KEY_CAPS_LETTER, 'c', 'C', 0}, {KEY_CAPS_LETTER, 'd', 'D', 0}, // 0x04
		{KEY_CAPS_LETTER, 'e', 'E', 0x20AC}, {KEY_CAPS_LETTER, 'f', 'F', 0}, {KEY_CAPS_LETTER, 'g', 'G', 0}, {KEY_CAPS_LETTER, 'h', 'H', 0}, // 0x08
		{KEY_CAPS_LETTER, 'i', 'I', 0}, {KEY_CAPS_LETTER, 'j', 'J', 0}, {KEY_CAPS_LETTER, 'k', 'K', 0}, {KEY_CAPS_LETTER, 'l', 'L', 0}, // 0x0C
		{KEY_CAPS_LETTER, 'm', 'M', 0x00B5}, {KEY_CAPS_LETTER, 'n', 'N', 0}, {KEY_CAPS_LETTER, 'o', 'O', 0}, {KEY_CAPS_LETTER, 'p', 'P', 0}, // 0x10
		{KEY_CAPS_LETTER, 'q', 'Q', 0}, {KEY_CAPS_LETTER, 'r', 'R', 0}, {KEY_CAPS_LETTER, 's', 'S', 0}, {KEY_CAPS_LETTER, 't', 'T', 0}, // 0x14
		{KEY_CAPS_LETTER, 'u', 'U', 0}, {KEY_CAPS_LETTER, 'v', 'V', 0}, {KEY_CAPS_LETTER, 'w', 'W', 0}, {KEY_CAPS_LETTER, 'x', 'X', 0}, // 0x18
		{KEY_CAPS_LETTER, 'y', 'Y', 0}, {KEY_CAPS_LETTER, 'z', 'Z', 0}, {0, '1', '!', 0}, {0, '2', '"', '@'}, // 0x1C
		{0, '3', '#', 0x00A3}, {0, '4', 0x00A4, '$'}, {0, '5', '%', 0x20AC}, {0, '6', '&', 0}, // 0x20
		{0, '7', '/', '{'}, {0, '8', '(', '['}, {0, '9', ')', ']'}, {0, '0', '=', '}'}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' '}, {0, '+', '?', 0}, {0, 0x0301, 0x0300, '|'}, {KEY_CAPS_LETTER, 0x00E5, 0x00C5, 0}, // 0x2C
		{0, 0x0308, 0x0302, 0x0303}, {0, 0, 0, 0}, {0, '\'', '*', 0}, {KEY_CAPS_LETTER, 0x00E6, 0x00C6, 0}, // 0x30
		{KEY_CAPS_LETTER, 0x00F8, 0x00D8, 0}, {0, 0x00BD, 0x00A7, 0}, {0, ',', ';', 0}, {0, '.', ':', 0}, // 0x34
		{0, '-', '_', 0}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0}, {KEY_KEYPAD, 0xF16E, 0xF12C, 0}, // 0x60
		{0, '<', '>', '\\'}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutFinnishSwedishKeys[100][4] = {
		{KEY_CAPS_LETTER, 'a', 'A', 0}, {KEY_CAPS_LETTER, 'b', 'B', 0}, {KEY_CAPS_LETTER, 'c', 'C', 0}, {KEY_CAPS_LETTER, 'd', 'D', 0}, // 0x04
		{KEY_CAPS_LETTER, 'e', 'E', 0x20AC}, {KEY_CAPS_LETTER, 'f', 'F', 0}, {KEY_CAPS_LETTER, 'g', 'G', 0}, {KEY_CAPS_LETTER, 'h', 'H', 0}, // 0x08
		{KEY_CAPS_LETTER, 'i', 'I', 0}, {KEY_CAPS_LETTER, 'j', 'J', 0}, {KEY_CAPS_LETTER, 'k', 'K', 0}, {KEY_CAPS_LETTER, 'l', 'L', 0}, // 0x0C
		{KEY_CAPS_LETTER, 'm', 'M', 0x00B5}, {KEY_CAPS_LETTER, 'n', 'N', 0}, {KEY_CAPS_LETTER, 'o', 'O', 0}, {KEY_CAPS_LETTER, 'p', 'P', 0}, // 0x10
		{KEY_CAPS_LETTER, 'q', 'Q', 0}, {KEY_CAPS_LETTER, 'r', 'R', 0}, {KEY_CAPS_LETTER, 's', 'S', 0}, {KEY_CAPS_LETTER, 't', 'T', 0}, // 0x14
		{KEY_CAPS_LETTER, 'u', 'U', 0}, {KEY_CAPS_LETTER, 'v', 'V', 0}, {KEY_CAPS_LETTER, 'w', 'W', 0}, {KEY_CAPS_LETTER, 'x', 'X', 0}, // 0x18
		{KEY_CAPS_LETTER, 'y', 'Y', 0}, {KEY_CAPS_LETTER, 'z', 'Z', 0}, {0, '1', '!', 0}, {0, '2', '"', '@'}, // 0x1C
		{0, '3', '#', 0x00A3}, {0, '4', 0x00A4, '$'}, {0, '5', '%', 0x20AC}, {0, '6', '&', 0}, // 0x20
		{0, '7', '/', '{'}, {0, '8', '(', '['}, {0, '9', ')', ']'}, {0, '0', '=', '}'}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' '}, {0, '+', '?', '\\'}, {0, 0x0301, 0x0300, 0}, {KEY_CAPS_LETTER, 0x00E5, 0x00C5, 0}, // 0x2C
		{0, 0x0308, 0x0302, 0x0303}, {0, 0, 0, 0}, {0, '\'', '*', 0}, {KEY_CAPS_LETTER, 0x00F6, 0x00D6, 0}, // 0x30
		{KEY_CAPS_LETTER, 0x00E4, 0x00C4, 0}, {0, 0x00A7, 0x00BD, 0}, {0, ',', ';', 0}, {0, '.', ':', 0}, // 0x34
		{0, '-', '_', 0}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0}, {KEY_KEYPAD, 0xF16E, 0xF12C, 0}, // 0x60
		{0, '<', '>', '|'}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutFrenchKeys[100][4] = {
		{KEY_CAPS_LETTER, 'q', 'Q', 0}, {KEY_CAPS_LETTER, 'b', 'B', 0}, {KEY_CAPS_LETTER, 'c', 'C', 0}, {KEY_CAPS_LETTER, 'd', 'D', 0}, // 0x04
		{KEY_CAPS_LETTER, 'e', 'E', 0x20AC}, {KEY_CAPS_LETTER, 'f', 'F', 0}, {KEY_CAPS_LETTER, 'g', 'G', 0}, {KEY_CAPS_LETTER, 'h', 'H', 0}, // 0x08
		{KEY_CAPS_LETTER, 'i', 'I', 0}, {KEY_CAPS_LETTER, 'j', 'J', 0}, {KEY_CAPS_LETTER, 'k', 'K', 0}, {KEY_CAPS_LETTER, 'l', 'L', 0}, // 0x0C
		{KEY_CAPS_LETTER, ',', '?', 0}, {KEY_CAPS_LETTER, 'n', 'N', 0}, {KEY_CAPS_LETTER, 'o', 'O', 0}, {KEY_CAPS_LETTER, 'p', 'P', 0}, // 0x10
		{KEY_CAPS_LETTER, 'a', 'A', 0}, {KEY_CAPS_LETTER, 'r', 'R', 0}, {KEY_CAPS_LETTER, 's', 'S', 0}, {KEY_CAPS_LETTER, 't', 'T', 0}, // 0x14
		{KEY_CAPS_LETTER, 'u', 'U', 0}, {KEY_CAPS_LETTER, 'v', 'V', 0}, {KEY_CAPS_LETTER, 'z', 'Z', 0}, {KEY_CAPS_LETTER, 'x', 'X', 0}, // 0x18
		{KEY_CAPS_LETTER, 'y', 'Y', 0}, {KEY_CAPS_LETTER, 'w', 'W', 0}, {KEY_CAPS_LETTER, '&', '1', 0}, {KEY_CAPS_LETTER, 0x00E9, '2', 0x0303}, // 0x1C
		{KEY_CAPS_LETTER, '"', '3', '#'}, {KEY_CAPS_LETTER, '\'', '4', '{'}, {KEY_CAPS_LETTER, '(', '5', '['}, {KEY_CAPS_LETTER, '-', '6', '|'}, // 0x20
		{KEY_CAPS_LETTER, 0x00E8, '7', 0x0300}, {KEY_CAPS_LETTER, '_', '8', '\\'}, {KEY_CAPS_LETTER, 0x00E7, '9', '^'}, {KEY_CAPS_LETTER, 0x00E0, '0', '@'}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' '}, {KEY_CAPS_LETTER, ')', 0x00B0, ']'}, {KEY_CAPS_LETTER, '=', '+', '}'}, {KEY_CAPS_LETTER, 0x0302, 0x0308, 0}, // 0x2C
		{KEY_CAPS_LETTER, '$', 0x00A3, 0x00A4}, {0, 0, 0, 0}, {KEY_CAPS_LETTER, '*', 0x00B5, 0}, {KEY_CAPS_LETTER, 'm', 'M', 0}, // 0x30
		{KEY_CAPS_LETTER, 0x00F9, '%', 0}, {0, 0x00B2, 0, 0}, {KEY_CAPS_LETTER, ';', '.', 0}, {KEY_CAPS_LETTER, ':', '/', 0}, // 0x34
		{KEY_CAPS_LETTER, '!', 0x00A7, 0}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0}, {KEY_KEYPAD, 0xF16E, 0xF12E, 0}, // 0x60
		{0, '<', '>', 0}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutGermanKeys[100][4] = {
		{KEY_CAPS_LETTER, 'a', 'A', 0}, {KEY_CAPS_LETTER, 'b', 'B', 0}, {KEY_CAPS_LETTER, 'c', 'C', 0}, {KEY_CAPS_LETTER, 'd', 'D', 0}, // 0x04
		{KEY_CAPS_LETTER, 'e', 'E', 0x20AC}, {KEY_CAPS_LETTER, 'f', 'F', 0}, {KEY_CAPS_LETTER, 'g', 'G', 0}, {KEY_CAPS_LETTER, 'h', 'H', 0}, // 0x08
		{KEY_CAPS_LETTER, 'i', 'I', 0}, {KEY_CAPS_LETTER, 'j', 'J', 0}, {KEY_CAPS_LETTER, 'k', 'K', 0}, {KEY_CAPS_LETTER, 'l', 'L', 0}, // 0x0C
		{KEY_CAPS_LETTER, 'm', 'M', 0x00B5}, {KEY_CAPS_LETTER, 'n', 'N', 0}, {KEY_CAPS_LETTER, 'o', 'O', 0}, {KEY_CAPS_LETTER, 'p', 'P', 0}, // 0x10
		{KEY_CAPS_LETTER, 'q', 'Q', '@'}, {KEY_CAPS_LETTER, 'r', 'R', 0}, {KEY_CAPS_LETTER, 's', 'S', 0}, {KEY_CAPS_LETTER, 't', 'T', 0}, // 0x14
		{KEY_CAPS_LETTER, 'u', 'U', 0}, {KEY_CAPS_LETTER, 'v', 'V', 0}, {KEY_CAPS_LETTER, 'w', 'W', 0}, {KEY_CAPS_LETTER, 'x', 'X', 0}, // 0x18
		{KEY_CAPS_LETTER, 'z', 'Z', 0}, {KEY_CAPS_LETTER, 'y', 'Y', 0}, {KEY_CAPS_LETTER, '1', '!', 0}, {KEY_CAPS_LETTER, '2', '"', 0x00B2}, // 0x1C
		{KEY_CAPS_LETTER, '3', 0x00A7, 0x00B3}, {KEY_CAPS_LETTER, '4', '$', 0}, {KEY_CAPS_LETTER, '5', '%', 0}, {KEY_CAPS_LETTER, '6', '&', 0}, // 0x20
		{KEY_CAPS_LETTER, '7', '/', '{'}, {KEY_CAPS_LETTER, '8', '(', '['}, {KEY_CAPS_LETTER, '9', ')', ']'}, {KEY_CAPS_LETTER, '0', '=', '}'}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' '}, {KEY_CAPS_LETTER, 0x00DF, '?', '\\'}, {0, 0x0301, 0x0300, 0}, {KEY_CAPS_LETTER, 0x00FC, 0x00DC, 0}, // 0x2C
		{KEY_CAPS_LETTER, '+', '*', '~'}, {0, 0, 0, 0}, {KEY_CAPS_LETTER, '#', '\'', 0}, {KEY_CAPS_LETTER, 0x00F6, 0x00D6, 0}, // 0x30
		{KEY_CAPS_LETTER, 0x00E4, 0x00C4, 0}, {0, 0x0302, 0x00B0, 0}, {KEY_CAPS_LETTER, ',', ';', 0}, {KEY_CAPS_LETTER, '.', ':', 0}, // 0x34
		{0, '-', '_', 0}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0}, {KEY_KEYPAD, 0xF16E, 0xF12C, 0}, // 0x60
		{0, '<', '>', '|'}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutGreekKeys[100][6] = {
		{KEY_CAPS_LETTER, 'a', 'A', 0x03B1, 0x0391, 0}, {KEY_CAPS_LETTER, 'b', 'B', 0x03B2, 0x0392, 0}, {KEY_CAPS_LETTER, 'c', 'C', 0x03C8, 0x03A8, 0}, {KEY_CAPS_LETTER, 'd', 'D', 0x03B4, 0x0394, 0}, // 0x04
		{KEY_CAPS_LETTER, 'e', 'E', 0x03B5, 0x0395, 0x20AC}, {KEY_CAPS_LETTER, 'f', 'F', 0x03C6, 0x03A6, 0}, {KEY_CAPS_LETTER, 'g', 'G', 0x03B3, 0x0393, 0}, {KEY_CAPS_LETTER, 'h', 'H', 0x03B7, 0x0397, 0}, // 0x08
		{KEY_CAPS_LETTER, 'i', 'I', 0x03B9, 0x0399, 0}, {KEY_CAPS_LETTER, 'j', 'J', 0x03BE, 0x039E, 0}, {KEY_CAPS_LETTER, 'k', 'K', 0x03BA, 0x039A, 0}, {KEY_CAPS_LETTER, 'l', 'L', 0x03BB, 0x039B, 0}, // 0x0C
		{KEY_CAPS_LETTER, 'm', 'M', 0x03BC, 0x039C, 0}, {KEY_CAPS_LETTER, 'n', 'N', 0x03BD, 0x039D, 0}, {KEY_CAPS_LETTER, 'o', 'O', 0x03BF, 0x039F, 0}, {KEY_CAPS_LETTER, 'p', 'P', 0x03C0, 0x03A0, 0}, // 0x10
		{0x0003, 'q', 'Q', 0x00B7, 0x2014, 0}, {KEY_CAPS_LETTER, 'r', 'R', 0x03C1, 0x03A1, 0}, {KEY_CAPS_LETTER, 's', 'S', 0x03C3, 0x03A3, 0}, {KEY_CAPS_LETTER, 't', 'T', 0x03C4, 0x03A4, 0}, // 0x14
		{KEY_CAPS_LETTER, 'u', 'U', 0x03B8, 0x0398, 0}, {KEY_CAPS_LETTER, 'v', 'V', 0x03C9, 0x03A9, 0}, {0x0003, 'w', 'W', 0x03C2, 0x00A6, 0}, {KEY_CAPS_LETTER, 'x', 'X', 0x03C7, 0x03A7, 0}, // 0x18
		{KEY_CAPS_LETTER, 'y', 'Y', 0x03C5, 0x03A5, 0}, {KEY_CAPS_LETTER, 'z', 'Z', 0x03B6, 0x0396, 0}, {0, '1', '!', '1', '!', 0}, {0, '2', '"', '2', '"', 0}, // 0x1C
		{0, '3', '#', '3', 0x00A3, 0}, {0, '4', '$', '4', '$', 0}, {0, '5', '%', '5', '%', 0}, {0, '6', '&', '6', 0x00AC, 0}, // 0x20
		{0, '7', '/', '7', '/', 0}, {0, '8', '(', '8', '(', 0}, {0, '9', ')', '9', ')', 0}, {0, '0', '=', '0', '=', 0}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' ', ' ', ' '}, {0, '\'', '?', '\'', 0x00B0, 0}, {0, '+', '*', '+', '*', 0}, {0, '[', '{', '[', 0x00AB, 0}, // 0x2C
		{0, ']', '}', ']', 0x00BB, 0}, {0, 0, 0, 0, 0, 0}, {0, 0x0300, '@', 0x00B2, 0x00B3, 0}, {0, 0x0301, 0x0308, 0x0301, 0x0308, 0x0344}, // 0x30
		{0, 0x0302, 0x0303, 0x2019, 0x2018, 0}, {0, '\\', '|', 0x00BD, 0x00B1, 0}, {0, ',', ';', ',', ';', '<'}, {0, '.', ':', '.', ':', '>'}, // 0x34
		{0, '-', '_', '-', '_', '-'}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0xF171, 0xF131, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0xF172, 0xF132, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0xF173, 0xF133, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0xF174, 0xF134, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0xF175, 0xF135, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0xF176, 0xF136, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0xF177, 0xF137, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0xF178, 0xF138, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0xF179, 0xF139, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0xF170, 0xF130, 0}, {KEY_KEYPAD, 0xF16E, 0xF12C, 0xF16E, 0xF12C, 0}, // 0x60
		{0, '<', '>', 0x00A7, 0x00A9, 0}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutGreekModifiers[8][6] = {
		{KEY_FLAG_FIXED, KBK_CTRL, KBK_CTRL, KBK_CTRL, KBK_CTRL, KBK_CTRL}, {KEY_FLAG_FIXED, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT}, {KEY_FLAG_FIXED, KBK_ALT, KBK_ALT, KBK_ALT, KBK_ALT, KBK_ALT}, {KEY_FLAG_FIXED, KBK_GUI, KBK_GUI, KBK_GUI, KBK_GUI, KBK_GUI},
		{KEY_FLAG_FIXED, KBK_CTRL, KBK_CTRL, KBK_CTRL, KBK_CTRL, KBK_CTRL}, {KEY_FLAG_FIXED, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT}, {KEY_FLAG_FIXED, KBK_ALTGR, KBK_ALTGR, KBK_ALTGR, KBK_ALTGR, KBK_ALTGR}, {KEY_FLAG_FIXED, KBK_GUI, KBK_GUI, KBK_GUI, KBK_GUI, KBK_GUI},
	};

	constexpr uint16 s_layoutItalianKeys[100][5] = {
		{KEY_CAPS_LETTER, 'a', 'A', 0, 0}, {KEY_CAPS_LETTER, 'b', 'B', 0, 0}, {KEY_CAPS_LETTER, 'c', 'C', 0, 0}, {KEY_CAPS_LETTER, 'd', 'D', 0, 0}, // 0x04
		{KEY_CAPS_LETTER, 'e', 'E', 0x20AC, 0}, {KEY_CAPS_LETTER, 'f', 'F', 0, 0}, {KEY_CAPS_LETTER, 'g', 'G', 0, 0}, {KEY_CAPS_LETTER, 'h', 'H', 0, 0}, // 0x08
		{KEY_CAPS_LETTER, 'i', 'I', 0, 0}, {KEY_CAPS_LETTER, 'j', 'J', 0, 0}, {KEY_CAPS_LETTER, 'k', 'K', 0, 0}, {KEY_CAPS_LETTER, 'l', 'L', 0, 0}, // 0x0C
		{KEY_CAPS_LETTER, 'm', 'M', 0, 0}, {KEY_CAPS_LETTER, 'n', 'N', 0, 0}, {KEY_CAPS_LETTER, 'o', 'O', 0, 0}, {KEY_CAPS_LETTER, 'p', 'P', 0, 0}, // 0x10
		{KEY_CAPS_LETTER, 'q', 'Q', 0, 0}, {KEY_CAPS_LETTER, 'r', 'R', 0, 0}, {KEY_CAPS_LETTER, 's', 'S', 0, 0}, {KEY_CAPS_LETTER, 't', 'T', 0, 0}, // 0x14
		{KEY_CAPS_LETTER, 'u', 'U', 0, 0}, {KEY_CAPS_LETTER, 'v', 'V', 0, 0}, {KEY_CAPS_LETTER, 'w', 'W', 0, 0}, {KEY_CAPS_LETTER, 'x', 'X', 0, 0}, // 0x18
		{KEY_CAPS_LETTER, 'y', 'Y', 0, 0}, {KEY_CAPS_LETTER, 'z', 'Z', 0, 0}, {0, '1', '!', 0, 0}, {0, '2', '"', 0, 0}, // 0x1C
		{0, '3', 0x00A3, 0, 0}, {0, '4', '$', 0, 0}, {0, '5', '%', 0x20AC, 0}, {0, '6', '&', 0, 0}, // 0x20
		{0, '7', '/', 0, 0}, {0, '8', '(', 0, 0}, {0, '9', ')', 0, 0}, {0, '0', '=', 0, 0}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' ', ' '}, {0, '\'', '?', 0, 0}, {0, 0x00EC, '^', '~', 0}, {0, 0x00E8, 0x00E9, '[', '{'}, // 0x2C
		{0, '+', '*', ']', '}'}, {0, 0, 0, 0, 0}, {0, 0x00F9, 0x00A7, '`', 0}, {0, 0x00F2, 0x00E7, '@', 0}, // 0x30
		{0, 0x00E0, 0x00B0, '#', 0}, {0, '\\', '|', 0, 0}, {0, ',', ';', 0, 0}, {0, '.', ':', 0, 0}, // 0x34
		{0, '-', '_', 0, 0}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0, 0}, {KEY_KEYPAD, 0xF16E, 0xF12C, 0, 0}, // 0x60
		{0, '<', '>', 0, 0}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutJapaneseKeys[136][7] = {
		{0x0003, 'a', 'A', 0x3061, 0x3061, 0x30C1, 0x30C1}, {0x0003, 'b', 'B', 0x3053, 0x3053, 0x30B3, 0x30B3}, {0x0003, 'c', 'C', 0x305D, 0x305D, 0x30BD, 0x30BD}, {0x0003, 'd', 'D', 0x3057, 0x3057, 0x30B7, 0x30B7}, // 0x04
		{0x0003, 'e', 'E', 0x3044, 0x3043, 0x30A4, 0x30A3}, {0x0003, 'f', 'F', 0x306F, 0x306F, 0x30CF, 0x30CF}, {0x0003, 'g', 'G', 0x304D, 0x304D, 0x30AD, 0x30AD}, {0x0003, 'h', 'H', 0x304F, 0x304F, 0x30AF, 0x30AF}, // 0x08
		{0x0003, 'i', 'I', 0x306B, 0x306B, 0x30CB, 0x30CB}, {0x0003, 'j', 'J', 0x307E, 0x307E, 0x30DE, 0x30DE}, {0x0003, 'k', 'K', 0x306E, 0x306E, 0x30CE, 0x30CE}, {0x0003, 'l', 'L', 0x308A, 0x308A, 0x30EA, 0x30EA}, // 0x0C
		{0x0003, 'm', 'M', 0x3082, 0x3082, 0x30E2, 0x30E2}, {0x0003, 'n', 'N', 0x307F, 0x307F, 0x30DF, 0x30DF}, {0x0003, 'o', 'O', 0x3089, 0x3089, 0x30E9, 0x30E9}, {0x0003, 'p', 'P', 0x305B, 0x305B, 0x30BB, 0x30BB}, // 0x10
		{0x0003, 'q', 'Q', 0x305F, 0x305F, 0x30BF, 0x30BF}, {0x0003, 'r', 'R', 0x3059, 0x3059, 0x30B9, 0x30B9}, {0x0003, 's', 'S', 0x3068, 0x3068, 0x30C8, 0x30C8}, {0x0003, 't', 'T', 0x304B, 0x304B, 0x30AB, 0x30AB}, // 0x14
		{0x0003, 'u', 'U', 0x306A, 0x306A, 0x30CA, 0x30CA}, {0x0003, 'v', 'V', 0x3072, 0x3072, 0x30D2, 0x30D2}, {0x0003, 'w', 'W', 0x3066, 0x3066, 0x30C6, 0x30C6}, {0x0003, 'x', 'X', 0x3055, 0x3055, 0x30B5, 0x30B5}, // 0x18
		{0x0003, 'y', 'Y', 0x3093, 0x3093, 0x30F3, 0x30F3}, {0x0003, 'z', 'Z', 0x3064, 0x3063, 0x30C4, 0x30C3}, {0, '1', '!', 0x306C, 0x306C, 0x30CC, 0x30CC}, {0, '2', '"', 0x3075, 0x3075, 0x30D5, 0x30D5}, // 0x1C
		{0, '3', '#', 0x3042, 0x3041, 0x30A2, 0x30A1}, {0, '4', '$', 0x3046, 0x3045, 0x30A6, 0x30A5}, {0, '5', '%', 0x3048, 0x3047, 0x30A8, 0x30A7}, {0, '6', '&', 0x304A, 0x3049, 0x30AA, 0x30A9}, // 0x20
		{0, '7', '\'', 0x3084, 0x3083, 0x30E4, 0x30E3}, {0, '8', '(', 0x3086, 0x3085, 0x30E6, 0x30E5}, {0, '9', ')', 0x3088, 0x3087, 0x30E8, 0x30E7}, {0, '0', 0, 0x308F, 0x3092, 0x30EF, 0x30F2}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' ', ' ', ' ', ' '}, {0, '-', '=', 0x307B, 0x307B, 0x30DB, 0x30DB}, {0, '^', '~', 0x3078, 0x3078, 0x30D8, 0x30D8}, {0, '@', '`', 0xFF9E, 0xFF9E, 0xFF9E, 0xFF9E}, // 0x2C
		{0, '[', '{', 0xFF9F, 0xFF62, 0xFF9F, 0xFF62}, {0, ']', '}', 0x3080, 0xFF63, 0x30E0, 0xFF63}, {0, ']', '}', 0x3080, 0xFF63, 0x30E0, 0xFF63}, {0, ';', '+', 0x308C, 0x308C, 0x30EC, 0x30EC}, // 0x30
		{0, ':', '*', 0x3051, 0x3051, 0x30B1, 0x30B1}, {0, 0xF043, 0xF043, 0xF043, 0xF043, 0xF043, 0xF043}, {0, ',', '<', 0x306D, 0xFF64, 0x30CD, 0xFF64}, {0, '.', '>', 0x308B, 0xFF61, 0x30EB, 0xFF61}, // 0x34
		{0, '/', '?', 0x3081, 0xFF65, 0x30E1, 0xFF65}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0xF171, 0xF131, 0xF171, 0xF131}, {KEY_KEYPAD, 0xF172, 0xF132, 0xF172, 0xF132, 0xF172, 0xF132}, {KEY_KEYPAD, 0xF173, 0xF133, 0xF173, 0xF133, 0xF173, 0xF133}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0xF174, 0xF134, 0xF174, 0xF134}, {KEY_KEYPAD, 0xF175, 0xF135, 0xF175, 0xF135, 0xF175, 0xF135}, {KEY_KEYPAD, 0xF176, 0xF136, 0xF176, 0xF136, 0xF176, 0xF136}, {KEY_KEYPAD, 0xF177, 0xF137, 0xF177, 0xF137, 0xF177, 0xF137}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0xF178, 0xF138, 0xF178, 0xF138}, {KEY_KEYPAD, 0xF179, 0xF139, 0xF179, 0xF139, 0xF179, 0xF139}, {KEY_KEYPAD, 0xF170, 0xF130, 0xF170, 0xF130, 0xF170, 0xF130}, {KEY_KEYPAD, 0xF16E, 0xF12E, 0xF16E, 0xF12E, 0xF16E, 0xF12E}, // 0x60
		{0, 0, 0, 0, 0, 0, 0}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D, 0xF13D, 0xF13D, 0xF13D}, // 0x64
		{0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, // 0x68
		{0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, // 0x6C
		{0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, // 0x70
		{0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, // 0x74
		{0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, // 0x78
		{0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, // 0x7C
		{0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, // 0x80
		{0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}, {0, '\\', '_', 0x308D, 0x308D, 0x30ED, 0x30ED}, // 0x84
		{0, 0xF040, 0xF040, 0xF040, 0xF040, 0xF040, 0xF040}, {0, '\\', '|', 0xFF70, 0xFF70, 0xFF70, 0xFF70}, {0, 0xF041, 0xF041, 0xF041, 0xF041, 0xF041, 0xF041}, {0, 0xF042, 0xF042, 0xF042, 0xF042, 0xF042, 0xF042}, // 0x88
	};

	constexpr uint16 s_layoutJapaneseModifiers[8][7] = {
		{KEY_FLAG_FIXED, KBK_CTRL, KBK_CTRL, KBK_CTRL, KBK_CTRL, KBK_CTRL, KBK_CTRL}, {KEY_FLAG_FIXED, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT}, {KEY_FLAG_FIXED, KBK_ALT, KBK_ALT, KBK_ALT, KBK_ALT, KBK_ALT, KBK_ALT}, {KEY_FLAG_FIXED, KBK_GUI, KBK_GUI, KBK_GUI, KBK_GUI, KBK_GUI, KBK_GUI},
		{KEY_FLAG_FIXED, KBK_CTRL, KBK_CTRL, KBK_CTRL, KBK_CTRL, KBK_CTRL, KBK_CTRL}, {KEY_FLAG_FIXED, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT}, {KEY_FLAG_FIXED, KBK_ALT, KBK_ALT, KBK_ALT, KBK_ALT, KBK_ALT, KBK_ALT}, {KEY_FLAG_FIXED, KBK_GUI, KBK_GUI, KBK_GUI, KBK_GUI, KBK_GUI, KBK_GUI},
	};

	constexpr uint16 s_layoutLatinAmericanKeys[100][4] = {
		{KEY_CAPS_LETTER, 'a', 'A', 0}, {KEY_CAPS_LETTER, 'b', 'B', 0}, {KEY_CAPS_LETTER, 'c', 'C', 0}, {KEY_CAPS_LETTER, 'd', 'D', 0}, // 0x04
		{KEY_CAPS_LETTER, 'e', 'E', 0}, {KEY_CAPS_LETTER, 'f', 'F', 0}, {KEY_CAPS_LETTER, 'g', 'G', 0}, {KEY_CAPS_LETTER, 'h', 'H', 0}, // 0x08
		{KEY_CAPS_LETTER, 'i', 'I', 0}, {KEY_CAPS_LETTER, 'j', 'J', 0}, {KEY_CAPS_LETTER, 'k', 'K', 0}, {KEY_CAPS_LETTER, 'l', 'L', 0}, // 0x0C
		{KEY_CAPS_LETTER, 'm', 'M', 0}, {KEY_CAPS_LETTER, 'n', 'N', 0}, {KEY_CAPS_LETTER, 'o', 'O', 0}, {KEY_CAPS_LETTER, 'p', 'P', 0}, // 0x10
		{KEY_CAPS_LETTER, 'q', 'Q', '@'}, {KEY_CAPS_LETTER, 'r', 'R', 0}, {KEY_CAPS_LETTER, 's', 'S', 0}, {KEY_CAPS_LETTER, 't', 'T', 0}, // 0x14
		{KEY_CAPS_LETTER, 'u', 'U', 0}, {KEY_CAPS_LETTER, 'v', 'V', 0}, {KEY_CAPS_LETTER, 'w', 'W', 0}, {KEY_CAPS_LETTER, 'x', 'X', 0}, // 0x18
		{KEY_CAPS_LETTER, 'y', 'Y', 0}, {KEY_CAPS_LETTER, 'z', 'Z', 0}, {0, '1', '!', 0}, {0, '2', '"', 0}, // 0x1C
		{0, '3', '#', 0}, {0, '4', '$', 0}, {0, '5', '%', 0}, {0, '6', '&', 0}, // 0x20
		{0, '7', '/', 0}, {0, '8', '(', 0}, {0, '9', ')', 0}, {0, '0', '=', 0}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' '}, {0, '\'', '?', '\\'}, {0, 0x00BF, 0x00A1, 0}, {0, 0x0301, 0x0308, 0}, // 0x2C
		{0, '+', '*', '~'}, {0, 0, 0, 0}, {0, '}', ']', 0x0300}, {KEY_CAPS_LETTER, 0x00F1, 0x00D1, 0}, // 0x30
		{0, '{', '[', 0x0302}, {0, '|', 0x00B0, 0x00AC}, {0, ',', ';', 0}, {0, '.', ':', 0}, // 0x34
		{0, '-', '_', 0}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0}, {KEY_KEYPAD, 0xF16E, 0xF12E, 0}, // 0x60
		{0, '<', '>', 0}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutDutchKeys[100][4] = {
		{KEY_CAPS_LETTER, 'a', 'A', 0}, {KEY_CAPS_LETTER, 'b', 'B', 0}, {KEY_CAPS_LETTER, 'c', 'C', 0x00A2}, {KEY_CAPS_LETTER, 'd', 'D', 0}, // 0x04
		{KEY_CAPS_LETTER, 'e', 'E', 0x20AC}, {KEY_CAPS_LETTER, 'f', 'F', 0}, {KEY_CAPS_LETTER, 'g', 'G', 0}, {KEY_CAPS_LETTER, 'h', 'H', 0}, // 0x08
		{KEY_CAPS_LETTER, 'i', 'I', 0}, {KEY_CAPS_LETTER, 'j', 'J', 0}, {KEY_CAPS_LETTER, 'k', 'K', 0}, {KEY_CAPS_LETTER, 'l', 'L', 0}, // 0x0C
		{KEY_CAPS_LETTER, 'm', 'M', 0x00B5}, {KEY_CAPS_LETTER, 'n', 'N', 0}, {KEY_CAPS_LETTER, 'o', 'O', 0}, {KEY_CAPS_LETTER, 'p', 'P', 0}, // 0x10
		{KEY_CAPS_LETTER, 'q', 'Q', 0}, {KEY_CAPS_LETTER, 'r', 'R', 0x00B6}, {KEY_CAPS_LETTER, 's', 'S', 0x00DF}, {KEY_CAPS_LETTER, 't', 'T', 0}, // 0x14
		{KEY_CAPS_LETTER, 'u', 'U', 0}, {KEY_CAPS_LETTER, 'v', 'V', 0}, {KEY_CAPS_LETTER, 'w', 'W', 0}, {KEY_CAPS_LETTER, 'x', 'X', 0x00BB}, // 0x18
		{KEY_CAPS_LETTER, 'y', 'Y', 0}, {KEY_CAPS_LETTER, 'z', 'Z', 0x00AB}, {0, '1', '!', 0x00B9}, {0, '2', '"', 0x00B2}, // 0x1C
		{0, '3', '#', 0x00B3}, {0, '4', '$', 0x00BC}, {0, '5', '%', 0x00BD}, {0, '6', '&', 0x00BE}, // 0x20
		{0, '7', '_', 0x00A3}, {0, '8', '(', '{'}, {0, '9', ')', '}'}, {0, '0', '\'', 0}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' '}, {0, '/', '?', '\\'}, {0, 0x00B0, '~', 0x00B8}, {0, 0x00A8, '^', 0}, // 0x2C
		{0, '*', '|', 0}, {0, 0, 0, 0}, {0, '<', '>', 0}, {0, '+', 0x00B1, 0}, // 0x30
		{0, 0x00B4, '`', 0}, {0, '@', 0x00A7, 0x00AC}, {0, ',', ';', 0}, {0, '.', ':', 0x00B7}, // 0x34
		{0, '-', '=', 0}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0}, {KEY_KEYPAD, 0xF16E, 0xF12C, 0}, // 0x60
		{0, ']', '[', 0x00A6}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutNorwegianKeys[100][4] = {
		{KEY_CAPS_LETTER, 'a', 'A', 0}, {KEY_CAPS_LETTER, 'b', 'B', 0}, {KEY_CAPS_LETTER, 'c', 'C', 0}, {KEY_CAPS_LETTER, 'd', 'D', 0}, // 0x04
		{KEY_CAPS_LETTER, 'e', 'E', 0x20AC}, {KEY_CAPS_LETTER, 'f', 'F', 0}, {KEY_CAPS_LETTER, 'g', 'G', 0}, {KEY_CAPS_LETTER, 'h', 'H', 0}, // 0x08
		{KEY_CAPS_LETTER, 'i', 'I', 0}, {KEY_CAPS_LETTER, 'j', 'J', 0}, {KEY_CAPS_LETTER, 'k', 'K', 0}, {KEY_CAPS_LETTER, 'l', 'L', 0}, // 0x0C
		{KEY_CAPS_LETTER, 'm', 'M', 0x00B5}, {KEY_CAPS_LETTER, 'n', 'N', 0}, {KEY_CAPS_LETTER, 'o', 'O', 0}, {KEY_CAPS_LETTER, 'p', 'P', 0}, // 0x10
		{KEY_CAPS_LETTER, 'q', 'Q', 0}, {KEY_CAPS_LETTER, 'r', 'R', 0}, {KEY_CAPS_LETTER, 's', 'S', 0}, {KEY_CAPS_LETTER, 't', 'T', 0}, // 0x14
		{KEY_CAPS_LETTER, 'u', 'U', 0}, {KEY_CAPS_LETTER, 'v', 'V', 0}, {KEY_CAPS_LETTER, 'w', 'W', 0}, {KEY_CAPS_LETTER, 'x', 'X', 0}, // 0x18
		{KEY_CAPS_LETTER, 'y', 'Y', 0}, {KEY_CAPS_LETTER, 'z', 'Z', 0}, {0, '1', '!', 0}, {0, '2', '"', '@'}, // 0x1C
		{0, '3', '#', 0x00A3}, {0, '4', 0x00A4, '$'}, {0, '5', '%', 0}, {0, '6', '&', 0}, // 0x20
		{0, '7', '/', '{'}, {0, '8', '(', '['}, {0, '9', ')', ']'}, {0, '0', '=', '}'}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' '}, {0, '+', '?', 0}, {0, '\\', 0x0300, 0x0301}, {KEY_CAPS_LETTER, 0x00E5, 0x00C5, 0}, // 0x2C
		{0, 0x0308, 0x0302, 0x0303}, {0, 0, 0, 0}, {0, '\'', '*', 0}, {KEY_CAPS_LETTER, 0x00F8, 0x00D8, 0}, // 0x30
		{KEY_CAPS_LETTER, 0x00E6, 0x00C6, 0}, {0, '|', 0x00A7, 0}, {0, ',', ';', 0}, {0, '.', ':', 0}, // 0x34
		{0, '-', '_', 0}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0}, {KEY_KEYPAD, 0xF16E, 0xF12C, 0}, // 0x60
		{0, '<', '>', 0}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutPortugueseKeys[100][4] = {
		{KEY_CAPS_LETTER, 'a', 'A', 0}, {KEY_CAPS_LETTER, 'b', 'B', 0}, {KEY_CAPS_LETTER, 'c', 'C', 0}, {KEY_CAPS_LETTER, 'd', 'D', 0}, // 0x04
		{KEY_CAPS_LETTER, 'e', 'E', 0x20AC}, {KEY_CAPS_LETTER, 'f', 'F', 0}, {KEY_CAPS_LETTER, 'g', 'G', 0}, {KEY_CAPS_LETTER, 'h', 'H', 0}, // 0x08
		{KEY_CAPS_LETTER, 'i', 'I', 0}, {KEY_CAPS_LETTER, 'j', 'J', 0}, {KEY_CAPS_LETTER, 'k', 'K', 0}, {KEY_CAPS_LETTER, 'l', 'L', 0}, // 0x0C
		{KEY_CAPS_LETTER, 'm', 'M', 0}, {KEY_CAPS_LETTER, 'n', 'N', 0}, {KEY_CAPS_LETTER, 'o', 'O', 0}, {KEY_CAPS_LETTER, 'p', 'P', 0}, // 0x10
		{KEY_CAPS_LETTER, 'q', 'Q', 0}, {KEY_CAPS_LETTER, 'r', 'R', 0}, {KEY_CAPS_LETTER, 's', 'S', 0}, {KEY_CAPS_LETTER, 't', 'T', 0}, // 0x14
		{KEY_CAPS_LETTER, 'u', 'U', 0}, {KEY_CAPS_LETTER, 'v', 'V', 0}, {KEY_CAPS_LETTER, 'w', 'W', 0}, {KEY_CAPS_LETTER, 'x', 'X', 0}, // 0x18
		{KEY_CAPS_LETTER, 'y', 'Y', 0}, {KEY_CAPS_LETTER, 'z', 'Z', 0}, {0, '1', '!', 0}, {0, '2', '"', '@'}, // 0x1C
		{0, '3', '#', 0x00A3}, {0, '4', '$', 0x00A7}, {0, '5', '%', 0}, {0, '6', '&', 0}, // 0x20
		{0, '7', '/', '{'}, {0, '8', '(', '['}, {0, '9', ')', ']'}, {0, '0', '=', '}'}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' '}, {0, '\'', '?', 0}, {0, 0x00AB, 0x00BB, 0}, {0, '+', '*', 0x0308}, // 0x2C
		{0, 0x0301, 0x0300, 0}, {0, 0, 0, 0}, {0, '~', '^', 0}, {KEY_CAPS_LETTER, 0x00E7, 0x00C7, 0}, // 0x30
		{0, 0x00BA, 0x00AA, 0}, {0, '\\', '|', 0}, {0, ',', ';', 0}, {0, '.', ':', 0}, // 0x34
		{0, '-', '_', 0}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0}, {KEY_KEYPAD, 0xF16E, 0xF12E, 0}, // 0x60
		{0, '<', '>', 0}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutSpanishKeys[100][4] = {
		{KEY_CAPS_LETTER, 'a', 'A', 0}, {KEY_CAPS_LETTER, 'b', 'B', 0}, {KEY_CAPS_LETTER, 'c', 'C', 0}, {KEY_CAPS_LETTER, 'd', 'D', 0}, // 0x04
		{KEY_CAPS_LETTER, 'e', 'E', 0x20AC}, {KEY_CAPS_LETTER, 'f', 'F', 0}, {KEY_CAPS_LETTER, 'g', 'G', 0}, {KEY_CAPS_LETTER, 'h', 'H', 0}, // 0x08
		{KEY_CAPS_LETTER, 'i', 'I', 0}, {KEY_CAPS_LETTER, 'j', 'J', 0}, {KEY_CAPS_LETTER, 'k', 'K', 0}, {KEY_CAPS_LETTER, 'l', 'L', 0}, // 0x0C
		{KEY_CAPS_LETTER, 'm', 'M', 0}, {KEY_CAPS_LETTER, 'n', 'N', 0}, {KEY_CAPS_LETTER, 'o', 'O', 0}, {KEY_CAPS_LETTER, 'p', 'P', 0}, // 0x10
		{KEY_CAPS_LETTER, 'q', 'Q', 0}, {KEY_CAPS_LETTER, 'r', 'R', 0}, {KEY_CAPS_LETTER, 's', 'S', 0}, {KEY_CAPS_LETTER, 't', 'T', 0}, // 0x14
		{KEY_CAPS_LETTER, 'u', 'U', 0}, {KEY_CAPS_LETTER, 'v', 'V', 0}, {KEY_CAPS_LETTER, 'w', 'W', 0}, {KEY_CAPS_LETTER, 'x', 'X', 0}, // 0x18
		{KEY_CAPS_LETTER, 'y', 'Y', 0}, {KEY_CAPS_LETTER, 'z', 'Z', 0}, {0, '1', '!', '|'}, {0, '2', '"', '@'}, // 0x1C
		{0, '3', 0x00B7, '#'}, {0, '4', '$', 0x0303}, {0, '5', '%', 0x20AC}, {0, '6', '&', 0x00AC}, // 0x20
		{0, '7', '/', 0}, {0, '8', '(', 0}, {0, '9', ')', 0}, {0, '0', '=', 0}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' '}, {0, '\'', '?', 0}, {0, 0x00A1, 0x00BF, 0}, {0, 0x0300, 0x0302, '['}, // 0x2C
		{0, '+', '*', ']'}, {0, 0, 0, 0}, {KEY_CAPS_LETTER, 0x00E7, 0x00C7, '}'}, {KEY_CAPS_LETTER, 0x00F1, 0x00D1, 0}, // 0x30
		{0, 0x0301, 0x0308, '{'}, {0, 0x00BA, 0x00AA, '\\'}, {0, ',', ';', 0}, {0, '.', ':', 0}, // 0x34
		{0, '-', '_', 0}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0}, {KEY_KEYPAD, 0xF16E, 0xF12E, 0}, // 0x60
		{0, '<', '>', 0}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutUKKeys[100][5] = {
		{0x00FF, 'a', 'A', 0x00E1, 0x00C1}, {KEY_CAPS_LETTER, 'b', 'B', 0, 0}, {KEY_CAPS_LETTER, 'c', 'C', 0, 0}, {KEY_CAPS_LETTER, 'd', 'D', 0, 0}, // 0x04
		{0x00FF, 'e', 'E', 0x00E9, 0x00C9}, {KEY_CAPS_LETTER, 'f', 'F', 0, 0}, {KEY_CAPS_LETTER, 'g', 'G', 0, 0}, {KEY_CAPS_LETTER, 'h', 'H', 0, 0}, // 0x08
		{0x00FF, 'i', 'I', 0x00ED, 0x00CD}, {KEY_CAPS_LETTER, 'j', 'J', 0, 0}, {KEY_CAPS_LETTER, 'k', 'K', 0, 0}, {KEY_CAPS_LETTER, 'l', 'L', 0, 0}, // 0x0C
		{KEY_CAPS_LETTER, 'm', 'M', 0, 0}, {KEY_CAPS_LETTER, 'n', 'N', 0, 0}, {0x00FF, 'o', 'O', 0x00F3, 0x00D3}, {KEY_CAPS_LETTER, 'p', 'P', 0, 0}, // 0x10
		{KEY_CAPS_LETTER, 'q', 'Q', 0, 0}, {KEY_CAPS_LETTER, 'r', 'R', 0, 0}, {KEY_CAPS_LETTER, 's', 'S', 0, 0}, {KEY_CAPS_LETTER, 't', 'T', 0, 0}, // 0x14
		{0x00FF, 'u', 'U', 0x00FA, 0x00DA}, {KEY_CAPS_LETTER, 'v', 'V', 0, 0}, {KEY_CAPS_LETTER, 'w', 'W', 0, 0}, {KEY_CAPS_LETTER, 'x', 'X', 0, 0}, // 0x18
		{KEY_CAPS_LETTER, 'y', 'Y', 0, 0}, {KEY_CAPS_LETTER, 'z', 'Z', 0, 0}, {0, '1', '!', 0, 0}, {0, '2', '"', 0, 0}, // 0x1C
		{0, '3', 0x00A3, 0, 0}, {0, '4', '$', 0x20AC, 0}, {0, '5', '%', 0, 0}, {0, '6', '^', 0, 0}, // 0x20
		{0, '7', '&', 0, 0}, {0, '8', '*', 0, 0}, {0, '9', '(', 0, 0}, {0, '0', ')', 0, 0}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' ', ' '}, {0, '-', '_', 0, 0}, {0, '=', '+', 0, 0}, {0, '[', '{', 0, 0}, // 0x2C
		{0, ']', '}', 0, 0}, {0, 0, 0, 0, 0}, {0, '#', '~', 0, 0}, {0, ';', ':', 0, 0}, // 0x30
		{0, '\'', '@', 0, 0}, {0, '`', 0x00AC, 0x00A6, 0}, {0, ',', '<', 0, 0}, {0, '.', '>', 0, 0}, // 0x34
		{0, '/', '?', 0, 0}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0, 0}, {KEY_KEYPAD, 0xF16E, 0xF12E, 0, 0}, // 0x60
		{0, '\\', '|', 0, 0}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutRussianKeys[100][4] = {
		{KEY_CAPS_LETTER, 0x0444, 0x0424, 0}, {KEY_CAPS_LETTER, 0x0438, 0x0418, 0}, {KEY_CAPS_LETTER, 0x0441, 0x0421, 0}, {KEY_CAPS_LETTER, 0x0432, 0x0412, 0}, // 0x04
		{KEY_CAPS_LETTER, 0x0443, 0x0423, 0}, {KEY_CAPS_LETTER, 0x0430, 0x0410, 0}, {KEY_CAPS_LETTER, 0x043F, 0x041F, 0}, {KEY_CAPS_LETTER, 0x0440, 0x0420, 0}, // 0x08
		{KEY_CAPS_LETTER, 0x0448, 0x0428, 0}, {KEY_CAPS_LETTER, 0x043E, 0x041E, 0}, {KEY_CAPS_LETTER, 0x043B, 0x041B, 0}, {KEY_CAPS_LETTER, 0x0434, 0x0414, 0}, // 0x0C
		{KEY_CAPS_LETTER, 0x044C, 0x042C, 0}, {KEY_CAPS_LETTER, 0x0442, 0x0422, 0}, {KEY_CAPS_LETTER, 0x0449, 0x0429, 0}, {KEY_CAPS_LETTER, 0x0437, 0x0417, 0}, // 0x10
		{KEY_CAPS_LETTER, 0x0439, 0x0419, 0}, {KEY_CAPS_LETTER, 0x043A, 0x041A, 0}, {KEY_CAPS_LETTER, 0x044B, 0x042B, 0}, {KEY_CAPS_LETTER, 0x0435, 0x0415, 0}, // 0x14
		{KEY_CAPS_LETTER, 0x0433, 0x0413, 0}, {KEY_CAPS_LETTER, 0x043C, 0x041C, 0}, {KEY_CAPS_LETTER, 0x0446, 0x0426, 0}, {KEY_CAPS_LETTER, 0x0447, 0x0427, 0}, // 0x18
		{KEY_CAPS_LETTER, 0x043D, 0x041D, 0}, {KEY_CAPS_LETTER, 0x044F, 0x042F, 0}, {0, '1', '!', 0}, {0, '2', '"', 0}, // 0x1C
		{0, '3', 0x2116, 0}, {0, '4', ';', 0}, {0, '5', '%', 0}, {0, '6', ':', 0}, // 0x20
		{0, '7', '?', 0}, {0, '8', '*', 0}, {0, '9', '(', 0}, {0, '0', ')', 0}, // 0x24
		{0, 0xF1CD, 0xF1CD, 0xF1CD}, {0, 0xF1DB, 0xF1DB, 0xF1DB}, {0, 0xF1C8, 0xF1C8, 0xF1C8}, {0, 0xF1C9, 0xF1C9, 0xF1C9}, // 0x28
		{0, ' ', ' ', ' '}, {0, '-', '_', 0}, {0, '=', '+', 0}, {KEY_CAPS_LETTER, 0x0445, 0x0425, 0}, // 0x2C
		{KEY_CAPS_LETTER, 0x044A, 0x042A, 0}, {0, '\\', '/', 0x001C}, {0, 0, 0, 0}, {KEY_CAPS_LETTER, 0x0436, 0x0416, 0}, // 0x30
		{KEY_CAPS_LETTER, 0x044D, 0x042D, 0}, {KEY_CAPS_LETTER, 0x0451, 0x0401, 0}, {KEY_CAPS_LETTER, 0x0431, 0x0411, 0}, {KEY_CAPS_LETTER, 0x044E, 0x042E, 0}, // 0x34
		{0, '.', ',', 0}, {KEY_FLAG_FIXED, KBK_CAPS_LOCK, KBK_CAPS_LOCK, KBK_CAPS_LOCK}, {0, 0xF061, 0xF061, 0xF061}, {0, 0xF062, 0xF062, 0xF062}, // 0x38
		{0, 0xF063, 0xF063, 0xF063}, {0, 0xF064, 0xF064, 0xF064}, {0, 0xF065, 0xF065, 0xF065}, {0, 0xF066, 0xF066, 0xF066}, // 0x3C
		{0, 0xF067, 0xF067, 0xF067}, {0, 0xF068, 0xF068, 0xF068}, {0, 0xF069, 0xF069, 0xF069}, {0, 0xF06A, 0xF06A, 0xF06A}, // 0x40
		{0, 0xF06B, 0xF06B, 0xF06B}, {0, 0xF06C, 0xF06C, 0xF06C}, {KEY_FLAG_FIXED, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN, KBK_PRINT_SCREEN}, {KEY_FLAG_FIXED, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK, KBK_SCROLL_LOCK}, // 0x44
		{KEY_FLAG_FIXED, KBK_PAUSE, KBK_PAUSE, KBK_PAUSE}, {0, 0xF1B0, 0xF1B0, 0xF1B0}, {0, 0xF1B7, 0xF1B7, 0xF1B7}, {0, 0xF1B9, 0xF1B9, 0xF1B9}, // 0x48
		{0, 0xF1AE, 0xF1AE, 0xF1AE}, {0, 0xF1B1, 0xF1B1, 0xF1B1}, {0, 0xF1B3, 0xF1B3, 0xF1B3}, {0, 0xF1B6, 0xF1B6, 0xF1B6}, // 0x4C
		{0, 0xF1B4, 0xF1B4, 0xF1B4}, {0, 0xF1B2, 0xF1B2, 0xF1B2}, {0, 0xF1B8, 0xF1B8, 0xF1B8}, {KEY_FLAG_FIXED, KBK_NUM_LOCK, KBK_NUM_LOCK, KBK_NUM_LOCK}, // 0x50
		{0, 0xF12F, 0xF12F, 0xF12F}, {0, 0xF12A, 0xF12A, 0xF12A}, {0, 0xF12D, 0xF12D, 0xF12D}, {0, 0xF12B, 0xF12B, 0xF12B}, // 0x54
		{0, 0xF10D, 0xF10D, 0xF10D}, {KEY_KEYPAD, 0xF171, 0xF131, 0}, {KEY_KEYPAD, 0xF172, 0xF132, 0}, {KEY_KEYPAD, 0xF173, 0xF133, 0}, // 0x58
		{KEY_KEYPAD, 0xF174, 0xF134, 0}, {KEY_KEYPAD, 0xF175, 0xF135, 0}, {KEY_KEYPAD, 0xF176, 0xF136, 0}, {KEY_KEYPAD, 0xF177, 0xF137, 0}, // 0x5C
		{KEY_KEYPAD, 0xF178, 0xF138, 0}, {KEY_KEYPAD, 0xF179, 0xF139, 0}, {KEY_KEYPAD, 0xF170, 0xF130, 0}, {0, '\\', '/', 0x001C}, // 0x60
		{0, '\\', '/', 0x001C}, {KEY_FLAG_FIXED, KBK_APPLICATION, KBK_APPLICATION, KBK_APPLICATION}, {0, 0, 0, 0}, {0, 0xF13D, 0xF13D, 0xF13D}, // 0x64
	};

	constexpr uint16 s_layoutRussianModifiers[8][4] = {
		{KEY_FLAG_FIXED, KBK_ALTGR, KBK_ALTGR, KBK_ALTGR}, {KEY_FLAG_FIXED, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT}, {KEY_FLAG_FIXED, KBK_ALT, KBK_ALT, KBK_ALT}, {KEY_FLAG_FIXED, KBK_GUI, KBK_GUI, KBK_GUI},
		{KEY_FLAG_FIXED, KBK_ALTGR, KBK_ALTGR, KBK_ALTGR}, {KEY_FLAG_FIXED, KBK_SHIFT, KBK_SHIFT, KBK_SHIFT}, {KEY_FLAG_FIXED, KBK_ALT, KBK_ALT, KBK_ALT}, {KEY_FLAG_FIXED, KBK_GUI, KBK_GUI, KBK_GUI},
	};

	constexpr KBDLayout s_layouts[KBD_COUNTRY_COUNT] = {
		{5, 0x30, &s_layoutUSInternationalKeys[0][0], &s_layoutAltGrModifiers5[0][0], 0, 0, 3, 100, {{0x32, 0x31}}, 1}, // 0
		{4, 0x10, &s_layoutCanadian1Keys[0][0], &s_layoutAltGrModifiers4[0][0], 0, 0, 3, 100, {{0x31, 0x32}}, 1}, // 1
		{4, 0x10, &s_layoutCanadian2Keys[0][0], &s_layoutAltGrModifiers4[0][0], 0, 0, 3, 100, {{0x31, 0x32}}, 1}, // 2
		{4, 0x10, &s_layoutDanishKeys[0][0], &s_layoutAltGrModifiers4[0][0], 0, 0, 3, 100, {{0x31, 0x32}}, 1}, // 3
		{4, 0x10, &s_layoutFinnishSwedishKeys[0][0], &s_layoutAltGrModifiers4[0][0], 0, 0, 3, 100, {{0x31, 0x32}}, 1}, // 4
		{4, 0x10, &s_layoutFrenchKeys[0][0], &s_layoutAltGrModifiers4[0][0], 0, 0, 3, 100, {{0x31, 0x32}}, 1}, // 5
		{4, 0x10, &s_layoutGermanKeys[0][0], &s_layoutAltGrModifiers4[0][0], 0, 0, 3, 100, {{0x31, 0x32}}, 1}, // 6
		{6, 0x13, &s_layoutGreekKeys[0][0], &s_layoutGreekModifiers[0][0], 3, 0, 5, 100, {{0x31, 0x32}}, 1}, // 7
		{5, 0x30, &s_layoutItalianKeys[0][0], &s_layoutAltGrModifiers5[0][0], 0, 0, 3, 100, {{0x31, 0x32}}, 1}, // 8
		{7, 0x0F, &s_layoutJapaneseKeys[0][0], &s_layoutJapaneseModifiers[0][0], 3, 5, 0, 136, {{0x31, 0x32}, {0x94, 0x35}}, 2}, // 9
		{4, 0x10, &s_layoutLatinAmericanKeys[0][0], &s_layoutAltGrModifiers4[0][0], 0, 0, 3, 100, {{0x31, 0x32}}, 1}, // 10
		{4, 0x10, &s_layoutDutchKeys[0][0], &s_layoutAltGrModifiers4[0][0], 0, 0, 3, 100, {{0x31, 0x32}}, 1}, // 11
		{4, 0x10, &s_layoutNorwegianKeys[0][0], &s_layoutAltGrModifiers4[0][0], 0, 0, 3, 100, {{0x31, 0x32}}, 1}, // 12
		{4, 0x10, &s_layoutPortugueseKeys[0][0], &s_layoutAltGrModifiers4[0][0], 0, 0, 3, 100, {{0x31, 0x32}}, 1}, // 13
		{4, 0x10, &s_layoutSpanishKeys[0][0], &s_layoutAltGrModifiers4[0][0], 0, 0, 3, 100, {{0x31, 0x32}}, 1}, // 14
		{4, 0x10, &s_layoutFinnishSwedishKeys[0][0], &s_layoutAltGrModifiers4[0][0], 0, 0, 3, 100, {{0x31, 0x32}}, 1}, // 15
		{5, 0x30, &s_layoutUKKeys[0][0], &s_layoutAltGrModifiers5[0][0], 0, 0, 3, 100, {{0x31, 0x32}}, 1}, // 16
		{3, 0x00, &s_layoutUSKeys[0][0], &s_layoutUSModifiers[0][0], 0, 0, 0, 100, {{0x32, 0x31}}, 1}, // 17
		{4, 0x10, &s_layoutRussianKeys[0][0], &s_layoutRussianModifiers[0][0], 0, 0, 3, 100, {{0x32, 0x31}}, 1}, // 18
	};

	const KBDLayout& GetLayout(sint32 country)
	{
		static constexpr KBDLayout s_emptyLayout{};
		if (country < 0 || country >= KBD_COUNTRY_COUNT)
			return s_emptyLayout;
		return s_layouts[country];
	}
}
