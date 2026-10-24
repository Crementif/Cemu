#include "input/api/SDL/SDLController.h"

#include "input/api/SDL/SDLControllerProvider.h"

SDLController::SDLController(const SDL_GUID& guid, size_t guid_index)
	: base_type(fmt::format("{}_", guid_index), fmt::format("Controller {}", guid_index + 1)), m_guid_index(guid_index),
	  m_guid(guid)
{
	char tmp[64];
	SDL_GUIDToString(m_guid, tmp, std::size(tmp));
	m_uuid += tmp;
}

SDLController::SDLController(const SDL_GUID& guid, size_t guid_index, std::string_view display_name)
	: base_type(fmt::format("{}_", guid_index), display_name), m_guid_index(guid_index), m_guid(guid)
{
	char tmp[64];
	SDL_GUIDToString(m_guid, tmp, std::size(tmp));
	m_uuid += tmp;
}

SDLController::~SDLController()
{
	if (m_controller)
	{
		SDL_CloseGamepad(m_controller);
		m_controller = nullptr;
	}
}

bool SDLController::is_connected()
{
	std::scoped_lock lock(m_controller_mutex);
	if (!m_controller)
	{
		reset_touch_input();
		return false;
	}

	if (!SDL_GamepadConnected(m_controller))
	{
		reset_touch_input();
		SDL_CloseGamepad(m_controller);
		m_controller = nullptr;
		return false;
	}

	return true;
}

bool SDLController::connect()
{
	if (is_connected())
		return true;

	m_has_rumble = false;
	const auto id = m_provider->get_gamepad_id(m_guid_index, m_guid);
	if (id == 0)
		return false;
	std::scoped_lock lock(m_controller_mutex);
	m_diid = id;

	m_controller = SDL_OpenGamepad(m_diid);

	if (!m_controller)
		return false;

	if (const char* name = SDL_GetGamepadName(m_controller))
		m_display_name = name;

	for (size_t i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; ++i)
		m_buttons[i] = SDL_GamepadHasButton(m_controller, (SDL_GamepadButton)i);
	for (size_t i = 0; i < SDL_GAMEPAD_AXIS_COUNT; ++i)
		m_axis[i] = SDL_GamepadHasAxis(m_controller, (SDL_GamepadAxis)i);
	m_has_accel = false;
	m_has_gyro = false;
	if (SDL_GamepadHasSensor(m_controller, SDL_SENSOR_ACCEL))
		m_has_accel = SDL_SetGamepadSensorEnabled(m_controller, SDL_SENSOR_ACCEL, true);
	if (SDL_GamepadHasSensor(m_controller, SDL_SENSOR_GYRO))
		m_has_gyro = SDL_SetGamepadSensorEnabled(m_controller, SDL_SENSOR_GYRO, true);
	m_has_touchpad = SDL_GetNumGamepadTouchpads(m_controller) > 0;
	m_has_rumble = SDL_RumbleGamepad(m_controller, 0, 0, 0);
	return true;
}

void SDLController::start_rumble()
{
	std::scoped_lock lock(m_controller_mutex);
	if (is_connected() && !m_has_rumble)
		return;
	if (m_settings.rumble <= 0)
		return;
	SDL_RumbleGamepad(m_controller, (Uint16)(m_settings.rumble * 0xFFFF), (Uint16)(m_settings.rumble * 0xFFFF), 5 * 1000);
}

void SDLController::stop_rumble()
{
	std::scoped_lock lock(m_controller_mutex);
	if (is_connected() && !m_has_rumble)
		return;
	SDL_RumbleGamepad(m_controller, 0, 0, 0);
}

MotionSample SDLController::get_motion_sample()
{
	if (is_connected() && has_motion())
		return m_provider->motion_sample(m_diid);
	return {};
}

void SDLController::reset_touch_input()
{
	m_active_touchpad = -1;
	m_active_finger = -1;
	m_touch_position = {};
	m_prev_touch_position = {};
}

void SDLController::update_touch_input()
{
	m_prev_touch_position = m_touch_position;
	auto read_finger = [this](sint32 touchpad, sint32 finger)
	{
		bool down = false;
		float x = 0.0f;
		float y = 0.0f;
		if (!SDL_GetGamepadTouchpadFinger(m_controller, touchpad, finger, &down, &x, &y, nullptr) || !down)
			return false;
		m_touch_position = {std::clamp(x, 0.0f, 1.0f), std::clamp(y, 0.0f, 1.0f)};
		return true;
	};

	// the same finger is followed until it is lifted, so a second finger doesn't move the touch
	if (m_active_touchpad >= 0 && read_finger(m_active_touchpad, m_active_finger))
		return;

	m_active_touchpad = -1;
	m_active_finger = -1;
	const sint32 touchpad_count = SDL_GetNumGamepadTouchpads(m_controller);
	for (sint32 touchpad = 0; touchpad < touchpad_count; ++touchpad)
	{
		const sint32 finger_count = SDL_GetNumGamepadTouchpadFingers(m_controller, touchpad);
		for (sint32 finger = 0; finger < finger_count; ++finger)
		{
			if (read_finger(touchpad, finger))
			{
				m_active_touchpad = touchpad;
				m_active_finger = finger;
				m_prev_touch_position = m_touch_position;
				return;
			}
		}
	}
}

void SDLController::load(const pugi::xml_node& node)
{
	std::scoped_lock lock(m_controller_mutex);
	// profiles saved without the capabilities are recognized by the PlayStation controller name
	const bool known_touchpad = m_display_name.find("DualSense") != std::string::npos ||
		m_display_name.find("DualShock") != std::string::npos || m_display_name.find("PS4") != std::string::npos;
	if (!is_connected())
	{
		m_has_touchpad = node.child("has_touchpad").text().as_bool(m_has_touchpad || known_touchpad);
		m_has_gyro = m_has_accel = node.child("has_motion").text().as_bool(has_motion() || known_touchpad);
	}
}

void SDLController::save(pugi::xml_node& node)
{
	std::scoped_lock lock(m_controller_mutex);
	node.append_child("has_touchpad").text().set(m_has_touchpad);
	node.append_child("has_motion").text().set(has_motion());
}

bool SDLController::has_touchpad()
{
	std::scoped_lock lock(m_controller_mutex);
	return m_has_touchpad;
}

bool SDLController::has_position()
{
	std::scoped_lock lock(m_controller_mutex);
	return is_connected() && m_active_touchpad >= 0;
}

glm::vec2 SDLController::get_position()
{
	std::scoped_lock lock(m_controller_mutex);
	return m_touch_position;
}

glm::vec2 SDLController::get_prev_position()
{
	std::scoped_lock lock(m_controller_mutex);
	return m_prev_touch_position;
}

PositionVisibility SDLController::GetPositionVisibility()
{
	return has_position() ? PositionVisibility::FULL : PositionVisibility::NONE;
}

std::string SDLController::get_button_name(uint64 button) const
{
	if (const char* name = SDL_GetGamepadStringForButton((SDL_GamepadButton)button))
		return name;
	return base_type::get_button_name(button);
}

ControllerState SDLController::raw_state()
{
	ControllerState result{};
	std::scoped_lock lock(m_controller_mutex);
	if (!is_connected())
		return result;
	update_touch_input();
	for (size_t i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; ++i)
	{
		if (m_buttons[i] && SDL_GetGamepadButton(m_controller, (SDL_GamepadButton)i))
			result.buttons.SetButtonState(i, true);
	}

	if (m_axis[SDL_GAMEPAD_AXIS_LEFTX])
		result.axis.x = (float)SDL_GetGamepadAxis(m_controller, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0f;
	if (m_axis[SDL_GAMEPAD_AXIS_LEFTY])
		result.axis.y = (float)SDL_GetGamepadAxis(m_controller, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0f;
	if (m_axis[SDL_GAMEPAD_AXIS_RIGHTX])
		result.rotation.x = (float)SDL_GetGamepadAxis(m_controller, SDL_GAMEPAD_AXIS_RIGHTX) / 32767.0f;
	if (m_axis[SDL_GAMEPAD_AXIS_RIGHTY])
		result.rotation.y = (float)SDL_GetGamepadAxis(m_controller, SDL_GAMEPAD_AXIS_RIGHTY) / 32767.0f;
	if (m_axis[SDL_GAMEPAD_AXIS_LEFT_TRIGGER])
		result.trigger.x = (float)SDL_GetGamepadAxis(m_controller, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / 32767.0f;
	if (m_axis[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER])
		result.trigger.y = (float)SDL_GetGamepadAxis(m_controller, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / 32767.0f;

	return result;
}
