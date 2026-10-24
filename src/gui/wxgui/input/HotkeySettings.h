#pragma once

#include <wx/wx.h>
#include "wxCemuConfig.h"
#include "input/api/Controller.h"

class HotkeyEntry;

class HotkeySettings : public wxPanel
{
public:
	static void Init(class MainWindow* mainWindow);

	static void CaptureInput(wxKeyEvent& event);
	static bool CaptureHoldableInput(wxEvent& event, bool gameWindow);
	static void CaptureInput(const ControllerState& currentState, const ControllerState& lastState);

	HotkeySettings(wxWindow* parent);
	~HotkeySettings();

private:
	inline static class MainWindow* s_mainWindow = nullptr;
	inline static bool s_gamePadScreenKeyDown = false;
	inline static bool s_fastForwardKeyDown = false;
	static void ReleaseGamePadScreenKey();
	static void ReleaseFastForwardKey();
	static std::unordered_map<sHotkeyCfg*, std::function<void(void)>> s_cfgHotkeyToFuncMap;
	inline static std::unordered_map<uint16, std::function<void(void)>> s_keyboardHotkeyToFuncMap{};
	inline static std::unordered_map<uint16, std::function<void(void)>> s_controllerHotkeyToFuncMap{};
	inline static auto& s_cfgHotkeys = GetWxGUIConfig().hotkeys;

	wxFlexGridSizer* m_sizer;
	wxButton* m_activeInputButton{ nullptr };
	wxTimer* m_controllerTimer{ nullptr };
	const wxSize m_minButtonSize{ 140, -1 };
	const wxString m_disabledHotkeyText{ _("Not set") };

	std::vector<HotkeyEntry> m_hotkeys;
	std::weak_ptr<ControllerBase> m_activeController{};
	bool m_needToSave = false;

	/* helpers */
	void SetInputHighlight(wxButton* button, bool active);
	void CreateColumnHeaders(void);
	void CreateHotkeyRow(const wxString& label, sHotkeyCfg& cfgHotkey, bool* toggleSetting = nullptr);
	wxChoice* CreateBehaviorChoice(bool& toggleSetting);
	wxString To_wxString(uKeyboardHotkey hotkey);
	wxString To_wxString(ControllerHotkey_t hotkey);
	bool IsValidKeycodeUp(int keycode);
	bool SetActiveController(void);

	template<typename T>
	void FinalizeInput(wxButton* inputButton);

	void RestoreInputButton(void);

	/* events */
	void OnKeyboardHotkeyInputLeftClick(wxCommandEvent& event);
	void OnControllerHotkeyInputLeftClick(wxCommandEvent& event);
	void OnKeyboardHotkeyInputRightClick(wxMouseEvent& event);
	void OnControllerHotkeyInputRightClick(wxMouseEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnControllerTimer(wxTimerEvent& event);
};
