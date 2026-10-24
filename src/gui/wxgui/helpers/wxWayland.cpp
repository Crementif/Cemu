#include "wxgui/helpers/wxWayland.h"

#if ( BOOST_OS_LINUX || BOOST_OS_BSD ) && HAS_WAYLAND

#include <dlfcn.h>

bool wxWlIsWaylandWindow(wxWindow* window)
{
	GtkWidget* gtkWindow = static_cast<GtkWidget*>(window->GetHandle());
	GdkWindow* gdkWindow = gtk_widget_get_window(gtkWindow);
	return GDK_IS_WAYLAND_WINDOW(gdkWindow);
}

void wxWlSetAppId(wxFrame* frame, const char* applicationId)
{
	GtkWidget* gtkWindow = static_cast<GtkWidget*>(frame->GetHandle());
	gtk_widget_realize(gtkWindow);
	GdkWindow* gdkWindow = gtk_widget_get_window(gtkWindow);
	static auto gdk_wl_set_app_id = reinterpret_cast<void (*) (GdkWindow*, const char*)>(dlsym(nullptr, "gdk_wayland_window_set_application_id"));
	if (gdk_wl_set_app_id)
		gdk_wl_set_app_id(gdkWindow, applicationId);
}

void wxWlPointerLock::registry_add_object(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
	auto pointerLock = static_cast<wxWlPointerLock*>(data);
	if (!strcmp(interface, zwp_relative_pointer_manager_v1_interface.name))
		pointerLock->m_relative_pointer_manager = static_cast<zwp_relative_pointer_manager_v1*>(wl_registry_bind(registry, name, &zwp_relative_pointer_manager_v1_interface, 1));
	else if (!strcmp(interface, zwp_pointer_constraints_v1_interface.name))
		pointerLock->m_pointer_constraints = static_cast<zwp_pointer_constraints_v1*>(wl_registry_bind(registry, name, &zwp_pointer_constraints_v1_interface, 1));
}

void wxWlPointerLock::relative_motion(void* data, zwp_relative_pointer_v1* /*relativePointer*/, uint32_t /*utimeHi*/, uint32_t /*utimeLo*/, wl_fixed_t /*dx*/, wl_fixed_t /*dy*/, wl_fixed_t dxUnaccel, wl_fixed_t dyUnaccel)
{
	auto pointerLock = static_cast<wxWlPointerLock*>(data);
	// the motion has sub-pixel precision, the remainder is kept for the next event
	pointerLock->m_remainder_x += wl_fixed_to_double(dxUnaccel);
	pointerLock->m_remainder_y += wl_fixed_to_double(dyUnaccel);
	const sint32 x = (sint32)pointerLock->m_remainder_x;
	const sint32 y = (sint32)pointerLock->m_remainder_y;
	pointerLock->m_remainder_x -= x;
	pointerLock->m_remainder_y -= y;
	if (x != 0 || y != 0)
		pointerLock->m_motion_callback(x, y);
}

wxWlPointerLock::wxWlPointerLock(wxWindow* window, std::function<void(sint32, sint32)> motionCallback)
	: m_motion_callback(std::move(motionCallback))
{
	GtkWidget* widget = static_cast<GtkWidget*>(window->GetHandle());
	gtk_widget_realize(widget);
	GdkWindow* gdkWindow = gdk_window_get_toplevel(gtk_widget_get_window(widget));
	GdkDisplay* gdkDisplay = gdk_window_get_display(gdkWindow);
	GdkDevice* gdkPointer = gdk_seat_get_pointer(gdk_display_get_default_seat(gdkDisplay));
	if (!gdkPointer)
		return;
	wl_display* display = gdk_wayland_display_get_wl_display(gdkDisplay);
	wl_surface* surface = gdk_wayland_window_get_wl_surface(gdkWindow);
	wl_pointer* pointer = gdk_wayland_device_get_wl_pointer(gdkPointer);
	wl_registry* registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &m_registry_listener, this);
	wl_display_roundtrip(display);
	wl_registry_destroy(registry);
	if (!m_relative_pointer_manager || !m_pointer_constraints || !surface || !pointer)
		return;
	m_relative_pointer = zwp_relative_pointer_manager_v1_get_relative_pointer(m_relative_pointer_manager, pointer);
	zwp_relative_pointer_v1_add_listener(m_relative_pointer, &m_relative_pointer_listener, this);
	m_locked_pointer = zwp_pointer_constraints_v1_lock_pointer(m_pointer_constraints, surface, pointer, nullptr, ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
	wl_display_flush(display);
}

wxWlPointerLock::~wxWlPointerLock()
{
	if (m_locked_pointer)
		zwp_locked_pointer_v1_destroy(m_locked_pointer);
	if (m_relative_pointer)
		zwp_relative_pointer_v1_destroy(m_relative_pointer);
	if (m_pointer_constraints)
		zwp_pointer_constraints_v1_destroy(m_pointer_constraints);
	if (m_relative_pointer_manager)
		zwp_relative_pointer_manager_v1_destroy(m_relative_pointer_manager);
}

#endif	// BOOST_OS_LINUX && HAS_WAYLAND
