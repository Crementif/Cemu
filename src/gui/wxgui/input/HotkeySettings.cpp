#include "wxgui/input/HotkeySettings.h"
#include "Cafe/HW/Latte/Renderer/Renderer.h"
#include "interface/WindowSystem.h"
#include <config/ActiveSettings.h>
#include "input/InputManager.h"
#include "HotkeySettings.h"
#include "MainWindow.h"

#include <wx/app.h>
#include <wx/settings.h>
#include <wx/clipbrd.h>

#if BOOST_OS_WINDOWS
#include <ole2.h>
#endif

#if BOOST_OS_LINUX || BOOST_OS_MACOS || BOOST_OS_BSD
#include "resource/embedded/resources.h"
#endif

std::optional<fs::path> GenerateScreenshotFilename(bool isDRC)
{
	fs::path screendir = ActiveSettings::GetUserDataPath("screenshots");
	// build screenshot name with format Screenshot_YYYY-MM-DD_HH-MM-SS[_GamePad].png
	// if the file already exists add a suffix counter (_2.png, _3.png etc)
	std::time_t time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm* tm = std::localtime(&time_t);

	std::string screenshotFileName = fmt::format("Screenshot_{:04}-{:02}-{:02}_{:02}-{:02}-{:02}", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	if (isDRC)
		screenshotFileName.append("_GamePad");

	fs::path screenshotPath;
	for (sint32 i = 0; i < 999; i++)
	{
		screenshotPath = screendir;
		if (i == 0)
			screenshotPath.append(fmt::format("{}.png", screenshotFileName));
		else
			screenshotPath.append(fmt::format("{}_{}.png", screenshotFileName, i + 1));

		std::error_code ec;
		bool exists = fs::exists(screenshotPath, ec);

		if (!ec && !exists)
			return screenshotPath;
	}
	return std::nullopt;
}

bool SaveScreenshotToFile(const fs::path& imagePath, const wxImage& image)
{
	std::error_code ec;
	fs::create_directories(imagePath.parent_path(), ec);
	if (ec)
		return false;

	// suspend wxWidgets logging for the lifetime this object, to prevent a message box if wxImage::SaveFile fails
	wxLogNull _logNo;
	return image.SaveFile(imagePath.wstring());
}

bool SaveScreenshotToClipboard(const wxImage& image)
{
	static std::mutex s_clipboardMutex;
	bool success = false;

	s_clipboardMutex.lock();
	if (wxTheClipboard->Open())
	{
		wxTheClipboard->SetData(new wxImageDataObject(image));
		wxTheClipboard->Close();
		success = true;
	}
	s_clipboardMutex.unlock();

	return success;
}

std::optional<std::string> SaveScreenshot(std::vector<uint8> data, int width, int height, bool mainWindow)
{
#if BOOST_OS_WINDOWS
	// on Windows wxWidgets uses OLE API for the clipboard
	// to make this work we need to call OleInitialize() on the same thread
	OleInitialize(nullptr);
#endif
	bool save_screenshot = GetWxGUIConfig().save_screenshot;
	wxImage image(width, height, data.data(), true);
	if (mainWindow)
	{
		if (SaveScreenshotToClipboard(image))
		{
			if (!save_screenshot)
				return _tr("Screenshot saved to clipboard");
		}
		else
		{
			return _tr("Failed to open clipboard");
		}
	}
	if (save_screenshot)
	{
		auto imagePath = GenerateScreenshotFilename(mainWindow);
		if (imagePath.has_value() && SaveScreenshotToFile(imagePath.value(), image))
		{
			if (mainWindow)
				return _tr("Screenshot saved");
		}
		else
		{
			return _tr("Failed to save screenshot to file");
		}
	}
	return std::nullopt;
}

extern WindowSystem::WindowInfo g_window_info;

std::unordered_map<sHotkeyCfg*, std::function<void(void)>> HotkeySettings::s_cfgHotkeyToFuncMap;

struct HotkeyEntry
{
	std::unique_ptr<wxStaticText> name;
	std::unique_ptr<wxButton> keyInput;
	std::unique_ptr<wxButton> controllerInput;
	sHotkeyCfg& hotkey;

	HotkeyEntry(wxStaticText* name, wxButton* keyInput, wxButton* controllerInput, sHotkeyCfg& hotkey)
		: name(name), keyInput(keyInput), controllerInput(controllerInput), hotkey(hotkey)
	{
		keyInput->SetClientData(&hotkey);
		controllerInput->SetClientData(&hotkey);
	}
};

HotkeySettings::HotkeySettings(wxWindow* parent)
	: wxPanel(parent, wxID_ANY)
{
	auto* hint = new wxStaticText(this, wxID_ANY, _("Click a hotkey to change it. Right-click to clear it."));
	m_sizer = new wxFlexGridSizer(0, 3, FromDIP(4), FromDIP(12));

	auto* layout = new wxBoxSizer(wxVERTICAL);
	layout->Add(hint, 0, wxALL, FromDIP(12));
	layout->Add(m_sizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(12));
	SetSizer(layout);

	SetActiveController();

	CreateColumnHeaders();

	/* global modifier */
	CreateHotkeyRow(_tr("Hotkey modifier"), s_cfgHotkeys.modifiers);
	m_hotkeys.at(0).keyInput->Hide();

	/* hotkeys */
	CreateHotkeyRow(_tr("Toggle fullscreen"), s_cfgHotkeys.toggleFullscreen);
	CreateHotkeyRow(_tr("Take screenshot"), s_cfgHotkeys.takeScreenshot);
	CreateHotkeyRow(_tr("Toggle fast-forward"), s_cfgHotkeys.toggleFastForward);
#ifdef CEMU_DEBUG_ASSERT
	CreateHotkeyRow(_tr("End emulation"), s_cfgHotkeys.endEmulation);
#endif
	CreateHotkeyRow(_tr("Exit application"), s_cfgHotkeys.exitApplication);
	CreateHotkeyRow(_tr("Release mouse"), s_cfgHotkeys.releaseMouse);

	Bind(wxEVT_SHOW, [this](wxShowEvent& event)
	{
		if (!event.IsShown() && m_activeInputButton)
		{
			RestoreInputButton();
		}
		event.Skip();
	});
	Bind(wxEVT_SYS_COLOUR_CHANGED, [this](wxSysColourChangedEvent& event)
	{
		if (m_activeInputButton)
			SetInputHighlight(m_activeInputButton, true);
		event.Skip();
	});
	m_controllerTimer = new wxTimer(this);
	Bind(wxEVT_TIMER, &HotkeySettings::OnControllerTimer, this);

	Layout();
}

HotkeySettings::~HotkeySettings()
{
	m_controllerTimer->Stop();
	if (m_needToSave)
	{
		GetConfigHandle().Save();
	}
}

void HotkeySettings::Init(MainWindow* mainWindowFrame)
{
	s_cfgHotkeyToFuncMap.insert({
		{&s_cfgHotkeys.toggleFullscreen, [](void) {
			 s_mainWindow->SetFullScreen(!s_mainWindow->IsFullScreen());
		 }},
		{&s_cfgHotkeys.toggleFullscreenAlt, [](void) {
			 s_mainWindow->SetFullScreen(!s_mainWindow->IsFullScreen());
		 }},
		{&s_cfgHotkeys.exitFullscreen, [](void) {
			 s_mainWindow->SetFullScreen(false);
		 }},
		{&s_cfgHotkeys.takeScreenshot, [](void) {
			 if (g_renderer)
				 g_renderer->RequestScreenshot(SaveScreenshot);
		 }},
		{&s_cfgHotkeys.toggleFastForward, [](void) {
			 ActiveSettings::SetTimerShiftFactor((ActiveSettings::GetTimerShiftFactor() < 3) ? 3 : 1);
		 }},
		{&s_cfgHotkeys.exitApplication, [](void) {
			auto closeEvent = new wxCloseEvent{wxEVT_CLOSE_WINDOW, s_mainWindow->GetId()};
			closeEvent->SetCanVeto(false);
			wxQueueEvent(s_mainWindow, closeEvent);
		 }},
		{&s_cfgHotkeys.releaseMouse, [](void) {
			 s_mainWindow->SetMouseCaptured(false);
		 }},
#ifdef CEMU_DEBUG_ASSERT
		{&s_cfgHotkeys.endEmulation, [](void) {
			 wxTheApp->CallAfter([]() {
				s_mainWindow->EndEmulation();
			 });
		 }},
#endif
	});

	s_keyboardHotkeyToFuncMap.reserve(s_cfgHotkeyToFuncMap.size());
	for (const auto& [cfgHotkey, func] : s_cfgHotkeyToFuncMap)
	{
		auto keyboardHotkey = cfgHotkey->keyboard.raw;
		if (keyboardHotkey > sHotkeyCfg::keyboardNone)
		{
			s_keyboardHotkeyToFuncMap[keyboardHotkey] = func;
		}
		auto controllerHotkey = cfgHotkey->controller;
		if (controllerHotkey > sHotkeyCfg::controllerNone)
		{
			s_controllerHotkeyToFuncMap[controllerHotkey] = func;
		}
	}
	s_mainWindow = mainWindowFrame;
}

void HotkeySettings::SetInputHighlight(wxButton* button, bool active)
{
	button->SetBackgroundColour(active ? wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT) : wxNullColour);
	button->SetForegroundColour(active ? wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT) : wxNullColour);
	button->SetFont(active ? GetFont().Bold() : GetFont());
	button->Refresh();
}

void HotkeySettings::CreateColumnHeaders(void)
{
	for (const auto& label : {_("Action"), _("Keyboard"), _("Controller")})
	{
		auto* heading = new wxStaticText(this, wxID_ANY, label);
		heading->SetFont(heading->GetFont().Bold());
		m_sizer->Add(heading, 0, wxBOTTOM, FromDIP(6));
	}
}

void HotkeySettings::CreateHotkeyRow(const wxString& label, sHotkeyCfg& cfgHotkey)
{
	auto* name = new wxStaticText(this, wxID_ANY, label);
	auto* keyInput = new wxButton(this, wxID_ANY, To_wxString(cfgHotkey.keyboard), wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);
	auto* controllerInput = new wxButton(this, wxID_ANY, To_wxString(cfgHotkey.controller), wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);

	/* for starting input */
	keyInput->Bind(wxEVT_BUTTON, &HotkeySettings::OnKeyboardHotkeyInputLeftClick, this);
	controllerInput->Bind(wxEVT_BUTTON, &HotkeySettings::OnControllerHotkeyInputLeftClick, this);

	/* for cancelling and clearing input */
	keyInput->Bind(wxEVT_RIGHT_UP, &HotkeySettings::OnKeyboardHotkeyInputRightClick, this);
	controllerInput->Bind(wxEVT_RIGHT_UP, &HotkeySettings::OnControllerHotkeyInputRightClick, this);

	keyInput->SetMinSize(FromDIP(m_minButtonSize));
	controllerInput->SetMinSize(FromDIP(m_minButtonSize));

	auto flags = wxSizerFlags().CenterVertical();
	m_sizer->Add(name, flags);
	m_sizer->Add(keyInput, wxSizerFlags().Expand());
	m_sizer->Add(controllerInput, wxSizerFlags().Expand());

	m_hotkeys.emplace_back(name, keyInput, controllerInput, cfgHotkey);
}

void HotkeySettings::OnControllerTimer(wxTimerEvent& event)
{
	if (m_activeController.expired())
	{
		m_controllerTimer->Stop();
		return;
	}
	auto& controller = *m_activeController.lock();
	auto buttons = controller.update_state().buttons;
	if (!buttons.IsIdle())
	{
		for (const auto& newHotkey : buttons.GetButtonList())
		{
			m_controllerTimer->Stop();
			auto* inputButton = static_cast<wxButton*>(m_controllerTimer->GetClientData());
			auto& cfgHotkey = *static_cast<sHotkeyCfg*>(inputButton->GetClientData());
			const auto oldHotkey = cfgHotkey.controller;
			const bool isModifier = (&cfgHotkey == &s_cfgHotkeys.modifiers);
			/* ignore same hotkeys and block duplicate hotkeys */
			if ((newHotkey != oldHotkey) && (isModifier || (newHotkey != s_cfgHotkeys.modifiers.controller)) &&
				(s_controllerHotkeyToFuncMap.find(newHotkey) == s_controllerHotkeyToFuncMap.end()))
			{
				m_needToSave |= true;
				cfgHotkey.controller = newHotkey;
				/* don't bind modifier to map */
				if (!isModifier)
				{
					s_controllerHotkeyToFuncMap.erase(oldHotkey);
					s_controllerHotkeyToFuncMap[newHotkey] = s_cfgHotkeyToFuncMap.at(&cfgHotkey);
				}
			}
			FinalizeInput<ControllerHotkey_t>(inputButton);
			return;
		}
	}
}

void HotkeySettings::OnKeyboardHotkeyInputLeftClick(wxCommandEvent& event)
{
	auto* inputButton = static_cast<wxButton*>(event.GetEventObject());
	if (m_activeInputButton)
	{
		/* ignore multiple clicks of the same button */
		if (inputButton == m_activeInputButton) return;
		RestoreInputButton();
	}
	inputButton->Bind(wxEVT_KEY_UP, &HotkeySettings::OnKeyUp, this);
	inputButton->SetLabelText(_("Press a key..."));
	SetInputHighlight(inputButton, true);
	inputButton->SetFocus();
	m_activeInputButton = inputButton;
}

void HotkeySettings::OnControllerHotkeyInputLeftClick(wxCommandEvent& event)
{
	auto* inputButton = static_cast<wxButton*>(event.GetEventObject());
	if (m_activeInputButton)
	{
		/* ignore multiple clicks of the same button */
		if (inputButton == m_activeInputButton) return;
		RestoreInputButton();
	}
	m_controllerTimer->Stop();
	if (!SetActiveController())
	{
		return;
	}
	inputButton->SetLabelText(_("Press a button..."));
	SetInputHighlight(inputButton, true);
	inputButton->SetFocus();
	m_controllerTimer->SetClientData(inputButton);
	m_controllerTimer->Start(25);
	m_activeInputButton = inputButton;
}

void HotkeySettings::OnKeyboardHotkeyInputRightClick(wxMouseEvent& event)
{
	if (m_activeInputButton)
	{
		RestoreInputButton();
		return;
	}
	auto* inputButton = static_cast<wxButton*>(event.GetEventObject());
	auto& cfgHotkey = *static_cast<sHotkeyCfg*>(inputButton->GetClientData());
	uKeyboardHotkey newHotkey{sHotkeyCfg::keyboardNone};
	if (cfgHotkey.keyboard.raw != newHotkey.raw)
	{
		m_needToSave |= true;
		s_keyboardHotkeyToFuncMap.erase(cfgHotkey.keyboard.raw);
		cfgHotkey.keyboard = newHotkey;
		FinalizeInput<uKeyboardHotkey>(inputButton);
	}
}

void HotkeySettings::OnControllerHotkeyInputRightClick(wxMouseEvent& event)
{
	if (m_activeInputButton)
	{
		RestoreInputButton();
		return;
	}
	auto* inputButton = static_cast<wxButton*>(event.GetEventObject());
	auto& cfgHotkey = *static_cast<sHotkeyCfg*>(inputButton->GetClientData());
	ControllerHotkey_t newHotkey{sHotkeyCfg::controllerNone};
	if (cfgHotkey.controller != newHotkey)
	{
		m_needToSave |= true;
		s_controllerHotkeyToFuncMap.erase(cfgHotkey.controller);
		cfgHotkey.controller = newHotkey;
		FinalizeInput<ControllerHotkey_t>(inputButton);
	}
}

bool HotkeySettings::SetActiveController(void)
{
	auto emulatedController = InputManager::instance().get_controller(0);
	if (emulatedController.use_count() <= 1)
	{
		return false;
	}
	const auto& controllers = emulatedController->get_controllers();
	if (controllers.empty())
	{
		return false;
	}
	m_activeController = controllers.at(0);
	return true;
}

void HotkeySettings::OnKeyUp(wxKeyEvent& event)
{
	auto* inputButton = static_cast<wxButton*>(event.GetEventObject());
	auto& cfgHotkey = *static_cast<sHotkeyCfg*>(inputButton->GetClientData());
	if (auto keycode = event.GetKeyCode(); IsValidKeycodeUp(keycode))
	{
		auto oldHotkey = cfgHotkey.keyboard;
		uKeyboardHotkey newHotkey{};
		newHotkey.key = keycode;
		newHotkey.alt = event.AltDown();
		newHotkey.ctrl = event.ControlDown();
		newHotkey.shift = event.ShiftDown();
		if ((newHotkey.raw != oldHotkey.raw) &&
			(s_keyboardHotkeyToFuncMap.find(newHotkey.raw) == s_keyboardHotkeyToFuncMap.end()))
		{
			m_needToSave |= true;
			cfgHotkey.keyboard = newHotkey;
			s_keyboardHotkeyToFuncMap.erase(oldHotkey.raw);
			s_keyboardHotkeyToFuncMap[newHotkey.raw] = s_cfgHotkeyToFuncMap.at(&cfgHotkey);
		}
	}
	FinalizeInput<uKeyboardHotkey>(inputButton);
}

template<typename T>
void HotkeySettings::FinalizeInput(wxButton* inputButton)
{
	auto& cfgHotkey = *static_cast<sHotkeyCfg*>(inputButton->GetClientData());
	if constexpr (std::is_same_v<T, uKeyboardHotkey>)
	{
		inputButton->Unbind(wxEVT_KEY_UP, &HotkeySettings::OnKeyUp, this);
		inputButton->SetLabelText(To_wxString(cfgHotkey.keyboard));
	}
	else if constexpr (std::is_same_v<T, ControllerHotkey_t>)
	{
		inputButton->SetLabelText(To_wxString(cfgHotkey.controller));
	}
	SetInputHighlight(inputButton, false);
	m_activeInputButton = nullptr;
}

void HotkeySettings::RestoreInputButton(void)
{
	if (m_controllerTimer->IsRunning())
	{
		m_controllerTimer->Stop();
		FinalizeInput<ControllerHotkey_t>(m_activeInputButton);
	}
	else
		FinalizeInput<uKeyboardHotkey>(m_activeInputButton);
}

bool HotkeySettings::IsValidKeycodeUp(int keycode)
{
	switch (keycode)
	{
	case WXK_NONE:
	case WXK_ESCAPE:
	case WXK_ALT:
	case WXK_CONTROL:
	case WXK_SHIFT:
		return false;
	default:
		return true;
	}
}

wxString HotkeySettings::To_wxString(uKeyboardHotkey hotkey)
{
	if (hotkey.raw == sHotkeyCfg::keyboardNone) {
		return m_disabledHotkeyText;
	}
	wxString ret{};
	if (hotkey.alt)
	{
		ret.append(_("Alt + "));
	}
	if (hotkey.ctrl)
	{
		ret.append(_("Ctrl + "));
	}
	if (hotkey.shift)
	{
		ret.append(_("Shift + "));
	}
	ret.append(wxAcceleratorEntry(0, hotkey.key).ToString());
	return ret;
}

wxString HotkeySettings::To_wxString(ControllerHotkey_t hotkey)
{
	if ((hotkey == sHotkeyCfg::controllerNone) || m_activeController.expired()) {
		return m_disabledHotkeyText;
	}
	return m_activeController.lock()->get_button_name(hotkey);
}

void HotkeySettings::CaptureInput(wxKeyEvent& event)
{
	if (g_window_info.is_hidkeyboardopen())
		return;

	uKeyboardHotkey hotkey{};
	hotkey.key = event.GetKeyCode();
	hotkey.alt = event.AltDown();
	hotkey.ctrl = event.ControlDown();
	hotkey.shift = event.ShiftDown();
	const auto it = s_keyboardHotkeyToFuncMap.find(hotkey.raw);
	if (it != s_keyboardHotkeyToFuncMap.end())
		it->second();
}

void HotkeySettings::CaptureInput(const ControllerState& currentState, const ControllerState& lastState)
{
	const auto& modifier = s_cfgHotkeys.modifiers.controller;
	if ((modifier >= 0) && currentState.buttons.GetButtonState(modifier))
	{
		for (const auto& buttonId : currentState.buttons.GetButtonList())
		{
			const auto it = s_controllerHotkeyToFuncMap.find(buttonId);
			if (it == s_controllerHotkeyToFuncMap.end())
				continue;
			/* only capture clicks */
			if (lastState.buttons.GetButtonState(buttonId))
				break;
			it->second();
			break;
		}
	}
}
