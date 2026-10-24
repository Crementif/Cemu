#include "wxgui/input/panels/VPADInputPanel.h"

#include <wx/gbsizer.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/slider.h>
#include <wx/button.h>
#include <wx/checkbox.h>


#include "wxgui/helpers/wxHelpers.h"
#include "wxgui/components/wxInputDraw.h"
#include "wxgui/input/InputSettings2.h"
#include "input/emulated/VPADController.h"

constexpr VPADController::ButtonId g_kFirstColumnItems[] =
{
	VPADController::kButtonId_A, VPADController::kButtonId_B, VPADController::kButtonId_X, VPADController::kButtonId_Y,
	VPADController::kButtonId_L, VPADController::kButtonId_R, VPADController::kButtonId_ZL, VPADController::kButtonId_ZR,
	VPADController::kButtonId_Plus, VPADController::kButtonId_Minus
};

constexpr VPADController::ButtonId g_kSecondColumnItems[] =
{
	VPADController::kButtonId_StickL, VPADController::kButtonId_StickL_Up, VPADController::kButtonId_StickL_Down, VPADController::kButtonId_StickL_Left, VPADController::kButtonId_StickL_Right
};

constexpr VPADController::ButtonId g_kThirdColumnItems[] =
{
	VPADController::kButtonId_StickR, VPADController::kButtonId_StickR_Up, VPADController::kButtonId_StickR_Down, VPADController::kButtonId_StickR_Left, VPADController::kButtonId_StickR_Right
};

constexpr VPADController::ButtonId g_kFourthRowItems[] =
{
	VPADController::kButtonId_Up, VPADController::kButtonId_Down, VPADController::kButtonId_Left, VPADController::kButtonId_Right
};

// the motion and touch choices list these entries first, followed by the controllers
constexpr sint32 kSourceDisabled = 0;
constexpr sint32 kSourceMouse = 1;
constexpr sint32 kFirstControllerSource = 2;

static wxString get_source_label(const ControllerPtr& controller)
{
	const auto& name = controller->display_name();
	wxString label;
	if (controller->api() == InputAPI::DSUClient)
		label = "DSUController";
	else if (name.find("DualSense") != std::string::npos)
		label = "DualSense";
	else if (name.find("DualShock") != std::string::npos || name.find("PS4") != std::string::npos)
		label = "DualShock";
	else
		label = wxString::FromUTF8(name);

	if (!controller->is_connected())
		return formatWxString(_("{} (offline)"), label.utf8_string());
	return label;
}

static void update_choice_items(wxChoice* choice, const wxArrayString& items)
{
	if (choice->GetStrings() != items)
		choice->Set(items);
}

static void update_tooltip(wxWindow* window, const wxString& tooltip)
{
	if (window->GetToolTipText() != tooltip)
		window->SetToolTip(tooltip);
}

VPADInputPanel::VPADInputPanel(wxWindow* parent)
	: InputPanel(parent), m_rumble_preview_timer(this)
{
	auto bold_font = GetFont();
	bold_font.MakeBold();

	auto* main_sizer = new wxGridBagSizer();

	sint32 row = 0;
	sint32 column = 0;
	for (const auto& id : g_kFirstColumnItems)
	{
		row++;
		add_button_row(main_sizer, row, column, id);
	}

	//////////////////////////////////////////////////////////////////

	main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVERTICAL), wxGBPosition(0, column + 2), wxGBSpan(11, 1), wxALL | wxEXPAND, 5);

	row = 0;
	column += 3;

	auto text = new wxStaticText(this, wxID_ANY, _("Left Axis"));
	text->SetFont(bold_font);
	main_sizer->Add(text, wxGBPosition(row, column), wxGBSpan(1, 3), wxALL | wxEXPAND, 5);

	for (const auto& id : g_kSecondColumnItems)
	{
		row++;
		add_button_row(main_sizer, row, column, id);
	}

	row++;
	
	// input drawer
	m_left_draw = new wxInputDraw(this, wxID_ANY, wxDefaultPosition, { 60, 60 });
	main_sizer->Add(m_left_draw, wxGBPosition(row, column + 1), wxGBSpan(2, 1), wxTOP | wxBOTTOM | wxEXPAND | wxALIGN_CENTER, 5);

	main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVERTICAL), wxGBPosition(0, column + 3), wxGBSpan(11, 1), wxALL | wxEXPAND, 5);

	//////////////////////////////////////////////////////////////////

	row = 0;
	column += 4;

	text = new wxStaticText(this, wxID_ANY, _("Right Axis"));
	text->SetFont(bold_font);
	main_sizer->Add(text, wxGBPosition(row, column), wxGBSpan(1, 3), wxALL | wxEXPAND, 5);

	for (const auto& id : g_kThirdColumnItems)
	{
		row++;
		add_button_row(main_sizer, row, column, id);
	}

	row++;

	// input drawer
	m_right_draw = new wxInputDraw(this, wxID_ANY, wxDefaultPosition, { 60, 60 });
	main_sizer->Add(m_right_draw, wxGBPosition(row, column + 1), wxGBSpan(2, 1), wxTOP | wxBOTTOM | wxEXPAND | wxALIGN_CENTER, 5);

	main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVERTICAL), wxGBPosition(0, column + 3), wxGBSpan(11, 1), wxALL | wxEXPAND, 5);

	//////////////////////////////////////////////////////////////////

	row = 0;
	column += 4;

	text = new wxStaticText(this, wxID_ANY, _("D-pad"));
	text->SetFont(bold_font);
	main_sizer->Add(text, wxGBPosition(row, column), wxGBSpan(1, 3), wxALL | wxEXPAND, 5);

	for (const auto& id : g_kFourthRowItems)
	{
		row++;
		add_button_row(main_sizer, row, column, id);
	}

	// the capabilities are aligned to the bottom of the button columns
	auto* capabilities_sizer = new wxBoxSizer(wxVERTICAL);
	capabilities_sizer->AddStretchSpacer();
	capabilities_sizer->Add(create_capabilities_sizer(), 0, wxEXPAND);
	main_sizer->Add(capabilities_sizer, wxGBPosition(6, column), wxGBSpan(5, 3), wxTOP | wxLEFT | wxRIGHT | wxEXPAND, 5);

	//////////////////////////////////////////////////////////////////

	Bind(wxEVT_TIMER, [this](wxTimerEvent&) { stop_rumble_preview(); }, m_rumble_preview_timer.GetId());
	Bind(wxEVT_SHOW, [this](wxShowEvent& event)
	{
		if (!event.IsShown())
			stop_rumble_preview();
		event.Skip();
	});

	SetSizer(main_sizer);
	Layout();
}

VPADInputPanel::~VPADInputPanel()
{
	stop_rumble_preview();
}

wxSizer* VPADInputPanel::create_capabilities_sizer()
{
	auto* sizer = new wxBoxSizer(wxVERTICAL);

	auto* heading_sizer = new wxBoxSizer(wxHORIZONTAL);
	auto* heading = new wxStaticText(this, wxID_ANY, _("Extra capabilities"));
	heading->SetFont(GetFont().Bold());
	heading_sizer->Add(heading, 1, wxALIGN_CENTER_VERTICAL);

	m_dsu_setup_button = new wxButton(this, wxID_ANY, _("Set up"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	m_dsu_setup_button->Bind(wxEVT_BUTTON, &VPADInputPanel::on_dsu_setup, this);
	m_dsu_setup_button->Hide();
	heading_sizer->Add(m_dsu_setup_button, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(6));
	sizer->Add(heading_sizer, 0, wxEXPAND | wxBOTTOM, 4);

	auto* fields = new wxGridBagSizer();
	fields->AddGrowableCol(1);
	const wxSize field_size = FromDIP(wxSize(130, -1));

	// motion
	fields->Add(new wxStaticText(this, wxID_ANY, _("Motion")), wxGBPosition(0, 0), wxDefaultSpan, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	m_motion_choice = new wxChoice(this, wxID_ANY);
	m_motion_choice->SetMinSize(field_size);
	m_motion_choice->Bind(wxEVT_CHOICE, &VPADInputPanel::on_motion_choice, this);
	fields->Add(m_motion_choice, wxGBPosition(0, 1), wxDefaultSpan, wxALL | wxEXPAND, 5);

	// touch
	fields->Add(new wxStaticText(this, wxID_ANY, _("Touch")), wxGBPosition(1, 0), wxDefaultSpan, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	m_touch_choice = new wxChoice(this, wxID_ANY);
	m_touch_choice->SetMinSize(field_size);
	m_touch_choice->Bind(wxEVT_CHOICE, &VPADInputPanel::on_touch_choice, this);
	fields->Add(m_touch_choice, wxGBPosition(1, 1), wxDefaultSpan, wxALL | wxEXPAND, 5);

	// rumble
	fields->Add(new wxStaticText(this, wxID_ANY, _("Rumble")), wxGBPosition(2, 0), wxDefaultSpan, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	auto* rumble_sizer = new wxBoxSizer(wxHORIZONTAL);
	m_rumble_slider = new wxSlider(this, wxID_ANY, 0, 0, 100);
	m_rumble_slider->SetMinSize(FromDIP(wxSize(100, -1)));
	m_rumble_slider->Disable();
	m_rumble_slider->Bind(wxEVT_SLIDER, &VPADInputPanel::on_rumble_slider, this);
	rumble_sizer->Add(m_rumble_slider, 1, wxRIGHT | wxALIGN_CENTER_VERTICAL, 4);
	m_rumble_text = new wxStaticText(this, wxID_ANY, "0%", wxDefaultPosition, FromDIP(wxSize(36, -1)), wxST_NO_AUTORESIZE | wxALIGN_RIGHT);
	rumble_sizer->Add(m_rumble_text, 0, wxALIGN_CENTER_VERTICAL);
	fields->Add(rumble_sizer, wxGBPosition(2, 1), wxDefaultSpan, wxALL | wxEXPAND, 5);

	// mic and screen
	add_button_row(fields, 3, 0, VPADController::kButtonId_Mic, _("blow mic"))->SetMinSize(field_size);
	add_button_row(fields, 4, 0, VPADController::kButtonId_Screen, _("show screen"))->SetMinSize(field_size);

	auto* toggleScreenText = new wxStaticText(this, wxID_ANY, _("toggle screen"));
	fields->Add(toggleScreenText, wxGBPosition(5, 0), wxDefaultSpan, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	m_togglePadViewCheckBox = new wxCheckBox(this, wxID_ANY, {}, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	wxString toggleScreenTT = _("Makes the \"show screen\" button toggle between the TV and gamepad screens");
	m_togglePadViewCheckBox->SetToolTip(toggleScreenTT);
	toggleScreenText->SetToolTip(toggleScreenTT);
	fields->Add(m_togglePadViewCheckBox, wxGBPosition(5, 1), wxDefaultSpan, wxALL | wxEXPAND, 5);

	sizer->Add(fields, 0, wxEXPAND);
	return sizer;
}

wxTextCtrl* VPADInputPanel::add_button_row(wxGridBagSizer *sizer, sint32 row, sint32 column, const VPADController::ButtonId &button_id) {
	return add_button_row(sizer, row, column, button_id, wxGetTranslation(wxString::FromUTF8(VPADController::get_button_name(button_id))));
}

wxTextCtrl* VPADInputPanel::add_button_row(wxGridBagSizer *sizer, sint32 row, sint32 column,
                                    const VPADController::ButtonId &button_id, const wxString &label) {
	sizer->Add(
		new wxStaticText(this, wxID_ANY, label),
		wxGBPosition(row, column),
		wxDefaultSpan,
		wxALL | wxALIGN_CENTER_VERTICAL, 5);

	auto* text_ctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB);
	text_ctrl->SetClientData((void*)button_id);
	text_ctrl->SetMinSize(wxSize(150, -1));
	text_ctrl->SetEditable(false);
	text_ctrl->SetBackgroundColour(kKeyColourNormalMode);
	bind_hotkey_events(text_ctrl);
	sizer->Add(text_ctrl, wxGBPosition(row, column + 1), wxDefaultSpan, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	return text_ctrl;
}

void VPADInputPanel::on_timer(const EmulatedControllerPtr& emulated_controller, const ControllerPtr& controller_base)
{
	if (controller_base)
		InputPanel::on_timer(emulated_controller, controller_base);

	static_cast<VPADController*>(emulated_controller.get())->set_screen_toggle(m_togglePadViewCheckBox->GetValue());

	if(emulated_controller)
	{
		const auto axis = emulated_controller->get_axis();
		const auto rotation = emulated_controller->get_rotation();

		m_left_draw->SetAxisValue(axis);
		m_right_draw->SetAxisValue(rotation);
	}

	update_motion_controls();
	update_touch_controls();
	update_rumble_controls(controller_base);
}

void VPADInputPanel::load_controller(const EmulatedControllerPtr& controller)
{
	InputPanel::load_controller(controller);

	const bool isToggle = static_cast<VPADController*>(controller.get())->is_screen_active_toggle();
	m_togglePadViewCheckBox->SetValue(isToggle);

	if (m_emulated_controller.lock() != controller)
		m_dsu_setup_selected = false;
	m_emulated_controller = controller;
	update_motion_controls();
	update_touch_controls();
}

void VPADInputPanel::on_motion_choice(wxCommandEvent& event)
{
	auto emulated_controller = m_emulated_controller.lock();
	const sint32 selection = m_motion_choice->GetSelection();
	if (!emulated_controller || selection == wxNOT_FOUND)
		return;

	const sint32 source_index = selection - kFirstControllerSource;
	m_dsu_setup_selected = source_index >= (sint32)m_motion_sources.size();
	if (!m_dsu_setup_selected)
	{
		ControllerPtr source;
		if (source_index >= 0)
			source = m_motion_sources[source_index];

		auto* vpad = static_cast<VPADController*>(emulated_controller.get());
		vpad->set_motion_enabled(selection != kSourceDisabled);
		vpad->set_prefer_native_motion(source != nullptr);
		for (const auto& controller : emulated_controller->get_controllers())
			controller->set_use_motion(controller == source);
	}
	update_motion_controls();
}

void VPADInputPanel::on_dsu_setup(wxCommandEvent& event)
{
	auto* settings = dynamic_cast<InputSettings2*>(wxGetTopLevelParent(this));
	if (!settings)
		return;
	if (settings->configure_dsu_motion())
		m_dsu_setup_selected = false;
	update_motion_controls();
}

void VPADInputPanel::on_touch_choice(wxCommandEvent& event)
{
	auto emulated_controller = m_emulated_controller.lock();
	const sint32 selection = m_touch_choice->GetSelection();
	if (!emulated_controller || selection == wxNOT_FOUND)
		return;

	auto* vpad = static_cast<VPADController*>(emulated_controller.get());
	if (selection == kSourceDisabled)
		vpad->set_touch_source("disabled");
	else if (selection == kSourceMouse)
		vpad->set_touch_source("mouse");
	else
		vpad->set_touch_source(m_touch_sources[selection - kFirstControllerSource]->uuid());
	update_touch_controls();
}

void VPADInputPanel::on_rumble_slider(wxCommandEvent& event)
{
	auto controller = m_rumble_controller.lock();
	if (!controller)
		return;

	stop_rumble_preview();
	const sint32 strength = m_rumble_slider->GetValue();
	controller->set_rumble((float)strength / 100.0f);
	if (strength > 0 && controller->is_connected() && controller->has_rumble())
	{
		controller->start_rumble();
		m_rumble_preview_controller = controller;
		m_rumble_preview_timer.StartOnce(250);
	}
	else
	{
		controller->stop_rumble();
	}
	update_rumble_controls(controller);
}

void VPADInputPanel::update_motion_controls()
{
	auto emulated_controller = m_emulated_controller.lock();
	if (!emulated_controller)
		return;

	// controllers stay listed while they're offline
	std::vector<ControllerPtr> sources;
	for (const auto& controller : emulated_controller->get_controllers())
	{
		const bool was_listed = std::ranges::find(m_motion_sources, controller) != m_motion_sources.end();
		if (controller->api() == InputAPI::DSUClient || controller->has_motion() || controller->get_settings().motion || was_listed)
			sources.push_back(controller);
	}
	std::ranges::stable_partition(sources, [](const ControllerPtr& controller) { return controller->api() != InputAPI::DSUClient; });
	m_motion_sources = std::move(sources);

	const bool has_dsu_source = std::ranges::any_of(m_motion_sources, [](const ControllerPtr& controller) { return controller->api() == InputAPI::DSUClient; });
	if (has_dsu_source)
		m_dsu_setup_selected = false;

	wxArrayString items;
	items.Add(_("Disabled"));
	items.Add(_("Right Mouse Click"));
	for (const auto& source : m_motion_sources)
		items.Add(get_source_label(source));
	if (!has_dsu_source)
		items.Add("DSUController");
	update_choice_items(m_motion_choice, items);

	sint32 selection;
	if (m_dsu_setup_selected)
		selection = (sint32)items.size() - 1;
	else
		selection = resolve_motion_selection(static_cast<VPADController*>(emulated_controller.get()));
	if (m_motion_choice->GetSelection() != selection)
		m_motion_choice->SetSelection(selection);

	ControllerPtr source;
	if (selection >= kFirstControllerSource && !m_dsu_setup_selected)
		source = m_motion_sources[selection - kFirstControllerSource];

	const bool show_setup = m_dsu_setup_selected || (source && source->api() == InputAPI::DSUClient);
	if (m_dsu_setup_button->IsShown() != show_setup)
	{
		m_dsu_setup_button->Show(show_setup);
		Layout();
	}

	if (m_dsu_setup_selected)
		update_tooltip(m_motion_choice, _("Set up a DSU server to use motion from another device."));
	else if (source)
		update_tooltip(m_motion_choice, wxString::FromUTF8(source->display_name()));
	else if (selection == kSourceMouse)
		update_tooltip(m_motion_choice, _("Hold right-click and drag on the game screen to tilt. Use the mouse wheel to roll."));
	else
		update_tooltip(m_motion_choice, _("Motion is disabled."));
}

sint32 VPADInputPanel::resolve_motion_selection(VPADController* vpad)
{
	if (!vpad->is_motion_enabled())
		return kSourceDisabled;
	if (!vpad->prefers_native_motion() || m_motion_sources.empty())
		return kSourceMouse;

	auto source = std::ranges::find_if(m_motion_sources, [](const ControllerPtr& controller) { return controller->get_settings().motion; });
	if (source == m_motion_sources.end())
	{
		// nothing was picked yet, so the controller the GamePad falls back to becomes the selection
		source = std::ranges::find(m_motion_sources, vpad->get_motion_controller());
		if (source == m_motion_sources.end())
			source = std::prev(m_motion_sources.end());
		(*source)->set_use_motion(true);
	}
	return kFirstControllerSource + (sint32)std::distance(m_motion_sources.begin(), source);
}

void VPADInputPanel::update_touch_controls()
{
	auto emulated_controller = m_emulated_controller.lock();
	if (!emulated_controller)
		return;

	auto* vpad = static_cast<VPADController*>(emulated_controller.get());
	auto touch_source = vpad->get_touch_source();

	// controllers stay listed while they're offline
	std::vector<ControllerPtr> sources;
	for (const auto& controller : emulated_controller->get_controllers())
	{
		const bool was_listed = std::ranges::find(m_touch_sources, controller) != m_touch_sources.end();
		if (controller->has_touchpad() || controller->uuid() == touch_source || was_listed)
			sources.push_back(controller);
	}
	m_touch_sources = std::move(sources);

	if (touch_source.empty() && !m_touch_sources.empty())
	{
		touch_source = m_touch_sources.front()->uuid();
		vpad->set_touch_source(touch_source);
	}

	wxArrayString items;
	items.Add(_("Disabled"));
	items.Add(_("Left Mouse Click"));
	for (const auto& source : m_touch_sources)
		items.Add(get_source_label(source));
	update_choice_items(m_touch_choice, items);

	ControllerPtr source;
	sint32 selection;
	const auto it = std::ranges::find_if(m_touch_sources, [&](const ControllerPtr& controller) { return controller->uuid() == touch_source; });
	if (it != m_touch_sources.end())
	{
		source = *it;
		selection = kFirstControllerSource + (sint32)std::distance(m_touch_sources.begin(), it);
	}
	else if (touch_source == "disabled")
	{
		selection = kSourceDisabled;
	}
	else
	{
		selection = kSourceMouse;
	}
	if (m_touch_choice->GetSelection() != selection)
		m_touch_choice->SetSelection(selection);

	if (source)
		update_tooltip(m_touch_choice, wxString::FromUTF8(source->display_name()) + "\n" + _("Use the touchpad for GamePad touch input. Left-clicking the GamePad screen also works."));
	else if (selection == kSourceMouse)
		update_tooltip(m_touch_choice, _("Left-click the GamePad screen to touch it."));
	else
		update_tooltip(m_touch_choice, _("GamePad touch input is disabled."));
}

void VPADInputPanel::update_rumble_controls(const ControllerPtr& controller)
{
	if (m_rumble_controller.lock() != controller)
		stop_rumble_preview();
	m_rumble_controller = controller;

	const bool has_rumble = controller && controller->is_connected() && controller->has_rumble();
	if (m_rumble_slider->IsEnabled() != has_rumble)
		m_rumble_slider->Enable(has_rumble);

	sint32 strength = 0;
	if (controller)
		strength = std::clamp((sint32)std::lround(controller->get_settings().rumble * 100.0f), 0, 100);
	if (m_rumble_slider->GetValue() != strength)
		m_rumble_slider->SetValue(strength);

	const auto strength_text = wxString::Format("%d%%", strength);
	if (m_rumble_text->GetLabel() != strength_text)
		m_rumble_text->SetLabel(strength_text);

	if (has_rumble)
		update_tooltip(m_rumble_slider, formatWxString(_("Rumble strength for {}. Set to 0% to disable."), controller->display_name()));
	else
		update_tooltip(m_rumble_slider, _("The selected controller has no rumble available."));
}

void VPADInputPanel::stop_rumble_preview()
{
	m_rumble_preview_timer.Stop();
	if (auto controller = m_rumble_preview_controller.lock())
		controller->stop_rumble();
	m_rumble_preview_controller.reset();
}
