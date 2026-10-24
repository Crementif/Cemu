#pragma once

#include "input/api/ControllerState.h"

namespace WindowSystem
{
	struct WindowHandleInfo
	{
		enum class Backend
		{
			X11,
			Wayland,
			Cocoa,
			Windows,
		} backend;
		void* display = nullptr;
		void* surface = nullptr;
	};

	enum struct PlatformKeyCodes : uint32
	{
		LCONTROL,
		RCONTROL,
		TAB,
		ESCAPE,
	};

	struct WindowInfo
	{
		std::atomic_bool app_active; // our app is active/has focus

		std::atomic_int32_t width, height;			 // client size of main window
		std::atomic_int32_t phys_width, phys_height; // client size of main window in physical pixels
		std::atomic<double> dpi_scale;

		std::atomic_bool pad_open;							 // if separate pad view is open
		std::atomic_int32_t pad_width, pad_height;			 // client size of pad window
		std::atomic_int32_t phys_pad_width, phys_pad_height; // client size of pad window in physical pixels
		std::atomic<double> pad_dpi_scale;

		std::atomic_bool pad_maximized = false;
		std::atomic_int32_t restored_pad_x = -1, restored_pad_y = -1;
		std::atomic_int32_t restored_pad_width = -1, restored_pad_height = -1;
		std::atomic_bool pad_screen_held = false;
		std::atomic_bool pad_screen_toggle_requested = false;

		std::atomic_bool is_fullscreen;
		std::atomic_bool debugger_focused;

		void set_keystate(uint32 keycode, bool state)
		{
			const std::lock_guard<std::mutex> lock(keycode_mutex);
			m_keydown[keycode] = state;
		}

		bool get_keystate(uint32 keycode)
		{
			const std::lock_guard<std::mutex> lock(keycode_mutex);
			auto result = m_keydown.find(keycode);
			if (result == m_keydown.end())
				return false;
			return result->second;
		}

		void set_keystatesup()
		{
			const std::lock_guard<std::mutex> lock(keycode_mutex);
			std::for_each(m_keydown.begin(), m_keydown.end(), [](std::pair<const uint32, bool>& el) { el.second = false; });
			for (uint32 usage = 0; usage < m_hidKeysHeld.size(); usage++)
			{
				if (m_hidKeysHeld.test(usage) && m_hidKeyEventsEnabled)
					m_hidKeyEvents.push_back({(uint8)usage, false});
			}
			m_hidKeysHeld.reset();
		}

		struct HIDKeyEvent
		{
			uint8 usage;
			bool pressed;
		};

		// physical keys of the game windows by USB HID usage id, used by the emulated USB keyboard
		void set_hidkeystate(uint8 usage, bool pressed)
		{
			const std::lock_guard<std::mutex> lock(keycode_mutex);
			if (m_hidKeysHeld.test(usage) == pressed)
				return; // key repeat
			m_hidKeysHeld.set(usage, pressed);
			if (!m_hidKeyEventsEnabled)
				return;
			// a title may open the keyboard without reading it
			if (m_hidKeyEvents.size() >= 256)
				m_hidKeyEvents.pop_front();
			m_hidKeyEvents.push_back({usage, pressed});
		}

		std::bitset<256> get_hidkeysheld()
		{
			const std::lock_guard<std::mutex> lock(keycode_mutex);
			return m_hidKeysHeld;
		}

		// starts queueing key events, returns the keys which are already held
		std::bitset<256> start_hidkeyevents()
		{
			const std::lock_guard<std::mutex> lock(keycode_mutex);
			m_hidKeyEventsEnabled = true;
			m_hidKeyEvents.clear();
			return m_hidKeysHeld;
		}

		void stop_hidkeyevents()
		{
			const std::lock_guard<std::mutex> lock(keycode_mutex);
			m_hidKeyEventsEnabled = false;
			m_hidKeyEvents.clear();
		}

		bool is_hidkeyboardopen()
		{
			const std::lock_guard<std::mutex> lock(keycode_mutex);
			return m_hidKeyEventsEnabled;
		}

		std::optional<HIDKeyEvent> pop_hidkeyevent()
		{
			const std::lock_guard<std::mutex> lock(keycode_mutex);
			if (m_hidKeyEvents.empty())
				return std::nullopt;
			HIDKeyEvent event = m_hidKeyEvents.front();
			m_hidKeyEvents.pop_front();
			return event;
		}

		struct HIDMouseState
		{
			sint32 deltaX;
			sint32 deltaY;
			sint32 wheel;
			uint8 buttons;
		};

		// set by the emulated USB mouse once a title reads from it, the game window only captures the mouse then
		std::atomic_bool hid_mouse_active = false;

		void add_hidmousemotion(sint32 deltaX, sint32 deltaY, sint32 wheel)
		{
			const std::lock_guard<std::mutex> lock(m_hidMouseMutex);
			m_hidMouse.deltaX += deltaX;
			m_hidMouse.deltaY += deltaY;
			m_hidMouse.wheel += wheel;
		}

		// bit 0 = left, bit 1 = right, bit 2 = middle, bit 3 = back, bit 4 = forward
		void set_hidmousebutton(uint8 button, bool pressed)
		{
			const std::lock_guard<std::mutex> lock(m_hidMouseMutex);
			if (pressed)
			{
				m_hidMouse.buttons |= (1 << button);
				m_hidMouseButtonsClicked |= (1 << button);
			}
			else
				m_hidMouse.buttons &= ~(1 << button);
		}

		void set_hidmousebuttonsup()
		{
			const std::lock_guard<std::mutex> lock(m_hidMouseMutex);
			m_hidMouse.buttons = 0;
		}

		// returns the motion since the last call, a button which was clicked in between is reported as held once
		HIDMouseState pop_hidmousestate()
		{
			const std::lock_guard<std::mutex> lock(m_hidMouseMutex);
			HIDMouseState state = m_hidMouse;
			state.buttons |= m_hidMouseButtonsClicked;
			m_hidMouseButtonsClicked = 0;
			m_hidMouse.deltaX = 0;
			m_hidMouse.deltaY = 0;
			m_hidMouse.wheel = 0;
			return state;
		}

		template<typename fn>
		void iter_keystates(fn f)
		{
			const std::lock_guard<std::mutex> lock(keycode_mutex);
			std::for_each(m_keydown.cbegin(), m_keydown.cend(), f);
		}

		WindowHandleInfo window_main;
		WindowHandleInfo window_pad;

		// canvas
		WindowHandleInfo canvas_main;
		WindowHandleInfo canvas_pad;

	  private:
		std::mutex keycode_mutex;
		std::unordered_map<uint32, bool> m_keydown;
		std::bitset<256> m_hidKeysHeld;
		std::deque<HIDKeyEvent> m_hidKeyEvents;
		bool m_hidKeyEventsEnabled = false;
		std::mutex m_hidMouseMutex;
		HIDMouseState m_hidMouse{};
		uint8 m_hidMouseButtonsClicked = 0;
	};

	enum class ErrorCategory
	{
		KEYS_TXT_CREATION = 0,
		GRAPHIC_PACKS = 1,
	};

	void ShowErrorDialog(std::string_view message, std::string_view title, std::optional<ErrorCategory> errorCategory = {});
	inline void ShowErrorDialog(std::string_view message, std::optional<ErrorCategory> errorCategory = {})
	{
		ShowErrorDialog(message, "", errorCategory);
	}

	void Create();

	WindowInfo& GetWindowInfo();

	void UpdateWindowTitles(bool isIdle, bool isLoading, double fps);
	void GetWindowSize(int& w, int& h);
	void GetPadWindowSize(int& w, int& h);
	void GetWindowPhysSize(int& w, int& h);
	void GetPadWindowPhysSize(int& w, int& h);
	double GetWindowDPIScale();
	double GetPadDPIScale();
	bool IsPadWindowOpen();
	bool IsKeyDown(uint32 key);
	bool IsKeyDown(PlatformKeyCodes key);
	std::string GetKeyCodeName(uint32 key);

	bool InputConfigWindowHasFocus();

	void NotifyGameLoaded();
	void NotifyGameExited();

	void RefreshGameList();

	bool IsFullScreen();

	void CaptureInput(const ControllerState& currentState, const ControllerState& lastState);
}; // namespace WindowSystem
