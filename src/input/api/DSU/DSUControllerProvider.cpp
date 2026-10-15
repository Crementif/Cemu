#include "input/api/DSU/DSUControllerProvider.h"
#include "input/api/DSU/DSUController.h"

#if BOOST_OS_WINDOWS
#include <boost/asio/detail/socket_option.hpp>
#include <winsock2.h>
#include <mstcpip.h>
#elif BOOST_OS_LINUX || BOOST_OS_MACOS
#include <sys/time.h>
#include <sys/socket.h>
#endif

DSUControllerProvider::DSUControllerProvider()
	: base_type(), m_uid(rand()), m_socket(m_io_service), m_heartbeat_timer(m_io_service), m_retry_timer(m_io_service)
{
	start();
}

DSUControllerProvider::DSUControllerProvider(const DSUProviderSettings& settings)
	: base_type(settings), m_uid(rand()), m_socket(m_io_service), m_heartbeat_timer(m_io_service), m_retry_timer(m_io_service)
{
	start();
}

DSUControllerProvider::~DSUControllerProvider()
{
	m_io_service.stop();
	if (m_io_thread.joinable())
		m_io_thread.join();
}

void DSUControllerProvider::start()
{
	if (!connect())
	{
		throw std::runtime_error("dsu client can't open the udp connection");
	}

	start_receive();
	start_heartbeat();
	m_io_thread = std::thread([this]
	{
		SetThreadName("DSU-client");
		m_io_service.run();
	});
	request_version();
}

std::vector<std::shared_ptr<ControllerBase>> DSUControllerProvider::get_controllers()
{
	std::vector<ControllerPtr> result;

	std::array<uint8_t, kMaxClients> indices;
	for (auto i = 0; i < kMaxClients; ++i)
		indices[i] = get_packet_index(i);

	request_pad_info();

	const auto controller_result = wait_update(indices, 3000);
	for (auto i = 0; i < kMaxClients; ++i)
	{
		if (controller_result[i] && is_connected(i))
			result.emplace_back(std::make_shared<DSUController>(i, m_settings));
	}

	return result;
}

bool DSUControllerProvider::connect()
{
	// already connected?
	if (m_receiver_endpoint.address().to_string() == get_settings().ip && m_receiver_endpoint.port() == get_settings().port)
		return true;

	try
	{
		using namespace boost::asio;

		ip::udp::resolver resolver(m_io_service);
		m_receiver_endpoint = *resolver.resolve(get_settings().ip, fmt::format("{}", get_settings().port)).cbegin();

		if (m_socket.is_open())
			m_socket.close();

		m_socket.open(ip::udp::v4());
		m_socket.bind(ip::udp::endpoint(ip::udp::v4(), 0));
#if BOOST_OS_WINDOWS
		// don't fail the next receive with WSAECONNRESET when a send gets an ICMP port unreachable reply
		BOOL reportConnReset = FALSE;
		DWORD bytesReturned = 0;
		WSAIoctl(m_socket.native_handle(), SIO_UDP_CONNRESET, &reportConnReset, sizeof(reportConnReset), nullptr, 0, &bytesReturned, nullptr, nullptr);
#endif
		// reset data
		m_state = {};
		m_prev_state = {};

		// restart threads
		return true;
	}
	catch (const std::exception& ex)
	{
		cemuLog_log(LogType::Force, "dsu client connect error: {}", ex.what());
		return false;
	}
}

bool DSUControllerProvider::is_connected(uint8_t index) const
{
	if (index >= kMaxClients)
		return false;

	std::scoped_lock lock(m_mutex[index]);
	return m_state[index].info.state == DsState::Connected;
}

DSUControllerProvider::ControllerState DSUControllerProvider::get_state(uint8_t index) const
{
	if (index >= kMaxClients)
		return {};

	std::scoped_lock lock(m_mutex[index]);
	return m_state[index];
}

DSUControllerProvider::ControllerState DSUControllerProvider::get_prev_state(uint8_t index) const
{
	if (index >= kMaxClients)
		return {};

	std::scoped_lock lock(m_mutex[index]);
	return m_prev_state[index];
}

std::array<bool, DSUControllerProvider::kMaxClients> DSUControllerProvider::wait_update(
	const std::array<uint8_t, kMaxClients>& indices, size_t timeout) const
{
	std::array<bool, kMaxClients> result{false, false, false, false};

	const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout);
	do
	{
		for (int i = 0; i < kMaxClients; ++i)
		{
			if (result[i])
				continue;

			std::unique_lock lock(m_mutex[i]);
			result[i] = indices[i] < m_state[i].packet_index;
		}

		if (std::all_of(result.cbegin(), result.cend(), [](const bool& v) { return v == true; }))
			break;

		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
		std::this_thread::yield();
	}
	while (std::chrono::steady_clock::now() < end);

	return result;
}

bool DSUControllerProvider::wait_update(uint8_t index, uint32_t packet_index, size_t timeout) const
{
	if (index >= kMaxClients)
		return false;

	std::unique_lock lock(m_mutex[index]);

	if (packet_index < m_state[index].packet_index)
		return true;

	const auto result = m_wait_cond[index].wait_for(lock, std::chrono::milliseconds(timeout),
	                                                [this, index, packet_index]()
	                                                {
		                                                return packet_index < m_state[index].packet_index;
	                                                });

	return result;
}

uint32_t DSUControllerProvider::get_packet_index(uint8_t index) const
{
	std::scoped_lock lock(m_mutex[index]);
	return m_state[index].packet_index;
}

template<typename T>
void DSUControllerProvider::send(const T& msg)
{
	boost::asio::post(m_io_service, [this, msg]
	{
		boost::system::error_code ec;
		m_socket.send_to(boost::asio::buffer(&msg, msg.GetSize()), m_receiver_endpoint, 0, ec);
	});
}

void DSUControllerProvider::request_version()
{
	send(VersionRequest(m_uid));
}

void DSUControllerProvider::request_pad_info()
{
	send(ListPorts(m_uid, 4, std::array<uint8_t, 4>{0, 1, 2, 3}));
}

void DSUControllerProvider::request_pad_info(uint8_t index)
{
	if (index >= kMaxClients)
		return;

	send(ListPorts(m_uid, 1, std::array<uint8_t, 4>{index}));
}

void DSUControllerProvider::request_pad_data()
{
	send(DataRequest(m_uid));
}

void DSUControllerProvider::request_pad_data(uint8_t index)
{
	if (index >= kMaxClients)
		return;

	// the heartbeat keeps the subscription alive, so only the first request is sent right away
	if (!m_subscribed[index].exchange(true))
		send(DataRequest(m_uid, index));
}

MotionSample DSUControllerProvider::get_motion_sample(uint8_t index) const
{
	if (index >= kMaxClients)
		return MotionSample();
	std::scoped_lock lock(m_mutex[index]);
	return m_state[index].motion_sample;
}


void DSUControllerProvider::start_receive()
{
	m_socket.async_receive_from(boost::asio::buffer(m_recv_buf), m_sender_endpoint, [this](const boost::system::error_code& ec, size_t len)
	{
		if (ec == boost::asio::error::operation_aborted)
			return;
		if (!ec)
		{
			handle_message(m_recv_buf.data(), len);
			start_receive();
			return;
		}

		m_retry_timer.expires_after(std::chrono::milliseconds(250));
		m_retry_timer.async_wait([this](const boost::system::error_code& timerEc)
		{
			if (!timerEc)
				start_receive();
		});
	});
}

void DSUControllerProvider::start_heartbeat()
{
	// servers stop streaming to clients that haven't requested data for a while, so keep re-requesting every subscribed pad
	m_heartbeat_timer.expires_after(std::chrono::seconds(1));
	m_heartbeat_timer.async_wait([this](const boost::system::error_code& ec)
	{
		if (ec)
			return;
		for (uint8_t i = 0; i < kMaxClients; ++i)
		{
			if (m_subscribed[i])
				send(DataRequest(m_uid, i));
		}
		start_heartbeat();
	});
}

void DSUControllerProvider::handle_message(const char* data, size_t len)
{
	if (len < sizeof(ServerMessage)) // cant be a valid message
		return;

	const auto msg = (const ServerMessage*)data;

	switch (msg->GetMessageType())
	{
	case MessageType::Version:
		{
			const auto rsp = (VersionResponse*)msg;
			if (!rsp->IsValid())
				return;

			m_server_version = rsp->GetVersion();
			// wdc
			break;
		}
	case MessageType::Information:
		{
			const auto info = (PortInfo*)msg;
			if (!info->IsValid())
				return;

			const uint8_t index = info->GetIndex();
			cemu_assert(index < kMaxClients);

			auto& mutex = m_mutex[index];
			std::scoped_lock lock(mutex);
			m_prev_state[index] = m_state[index];
			m_state[index] = *info;
			m_wait_cond[index].notify_all();
			break;
		}
	case MessageType::Data:
		{
			const auto rsp = (DataResponse*)msg;
			if (!rsp->IsValid())
				return;

			const uint8_t index = rsp->GetIndex();
			cemu_assert(index < kMaxClients);

			auto& mutex = m_mutex[index];
			std::scoped_lock lock(mutex);
			m_prev_state[index] = m_state[index];
			m_state[index] = *rsp;
			m_wait_cond[index].notify_all();
			// update motion info immediately, guaranteeing that we dont drop packets
			integrate_motion(index, *rsp);
			break;
		}
	}
}

void DSUControllerProvider::integrate_motion(uint8_t index, const DataResponse& data_response)
{
	const uint64 ts = data_response.GetMotionTimestamp();
	if (ts <= m_last_motion_timestamp[index])
	{
		const uint64 dif = m_last_motion_timestamp[index] - ts;
		if (dif >= 10000000) // timestamp more than 10 seconds in the past, a controller reset probably happened
			m_last_motion_timestamp[index] = 0;
		return;
	}

	const uint64 elapsedTime = ts - m_last_motion_timestamp[index];
	m_last_motion_timestamp[index] = ts;
	const double elapsedTimeD = (double)elapsedTime / 1000000.0;
	const auto& acc = data_response.GetAcceleration();
	const auto& gyro = data_response.GetGyro();

	m_motion_handler[index].processMotionSample((float)elapsedTimeD,
	                                            gyro.x * 0.0174533f,
	                                            gyro.y * 0.0174533f,
	                                            gyro.z * 0.0174533f,
	                                            acc.x,
	                                            -acc.y,
	                                            -acc.z);

	m_state[index].motion_sample = m_motion_handler[index].getMotionSample();
}

DSUControllerProvider::ControllerState& DSUControllerProvider::ControllerState::operator=(const PortInfo& port_info)
{
	info = port_info.GetInfo();
	last_update = std::chrono::steady_clock::now();
	packet_index++; // increase packet index for every packet we assign/recv
	return *this;
}

DSUControllerProvider::ControllerState& DSUControllerProvider::ControllerState::operator=(
	const DataResponse& data_response)
{
	this->operator=(static_cast<const PortInfo&>(data_response));
	data = data_response.GetData();
	return *this;
}