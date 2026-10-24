#pragma once

#include <wx/gbsizer.h>
#include <wx/timer.h>
#include "wxgui/input/panels/InputPanel.h"

class wxInputDraw;
class wxCheckBox;
class wxChoice;
class wxButton;
class wxSlider;
class wxStaticText;
class wxTextCtrl;

class VPADInputPanel : public InputPanel
{
public:
	VPADInputPanel(wxWindow* parent);
	~VPADInputPanel() override;

	void on_timer(const EmulatedControllerPtr& emulated_controller, const ControllerPtr& controller) override;
	virtual void load_controller(const EmulatedControllerPtr& controller) override;

private:
	wxSizer* create_capabilities_sizer();

	void on_motion_choice(wxCommandEvent& event);
	void on_dsu_setup(wxCommandEvent& event);
	void on_touch_choice(wxCommandEvent& event);
	void on_rumble_slider(wxCommandEvent& event);

	void update_motion_controls();
	sint32 resolve_motion_selection(VPADController* vpad);
	void update_touch_controls();
	void update_rumble_controls(const ControllerPtr& controller);
	void stop_rumble_preview();

	wxInputDraw* m_left_draw, * m_right_draw;
	wxCheckBox* m_togglePadViewCheckBox;

	std::weak_ptr<EmulatedController> m_emulated_controller;

	wxChoice* m_motion_choice;
	wxButton* m_dsu_setup_button;
	std::vector<ControllerPtr> m_motion_sources;
	bool m_dsu_setup_selected = false;

	wxChoice* m_touch_choice;
	std::vector<ControllerPtr> m_touch_sources;

	wxSlider* m_rumble_slider;
	wxStaticText* m_rumble_text;
	std::weak_ptr<ControllerBase> m_rumble_controller;
	std::weak_ptr<ControllerBase> m_rumble_preview_controller;
	wxTimer m_rumble_preview_timer;

	wxTextCtrl* add_button_row(wxGridBagSizer *sizer, sint32 row, sint32 column, const VPADController::ButtonId &button_id);
	wxTextCtrl* add_button_row(wxGridBagSizer *sizer, sint32 row, sint32 column, const VPADController::ButtonId &button_id, const wxString &label);
};
