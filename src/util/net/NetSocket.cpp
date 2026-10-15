#include "util/net/NetSocket.h"

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#if defined(_WIN32)
using NativeSocket = SOCKET;
using AddressLength = int;
using IoLength = int;
using PollDescriptor = WSAPOLLFD;
constexpr NativeSocket kInvalidNative = INVALID_SOCKET;
constexpr int kSendFlags = 0;
constexpr short kPollReadable = POLLRDNORM;
constexpr short kPollWritable = POLLWRNORM;

static bool EnsureInitialized()
{
	static const bool s_initialized = []
	{
		WSADATA data;
		return WSAStartup(MAKEWORD(2, 2), &data) == 0;
	}();
	return s_initialized;
}

static int GetLastSocketError()
{
	return WSAGetLastError();
}

static bool IsConnectPending(int error)
{
	return error == WSAEWOULDBLOCK;
}

static void CloseNative(NativeSocket socket)
{
	closesocket(socket);
}

static int PollNative(PollDescriptor& descriptor, int timeoutMs)
{
	return WSAPoll(&descriptor, 1, timeoutMs);
}

static bool ConfigureNewSocket(NativeSocket socket, int type)
{
	SetHandleInformation(reinterpret_cast<HANDLE>(socket), HANDLE_FLAG_INHERIT, 0);
	// match other platforms, where an ICMP port unreachable reply doesn't fail the next UDP receive
	if (type == SOCK_DGRAM)
	{
		BOOL reportConnReset = FALSE;
		DWORD bytesReturned = 0;
		WSAIoctl(socket, SIO_UDP_CONNRESET, &reportConnReset, sizeof(reportConnReset), nullptr, 0, &bytesReturned, nullptr, nullptr);
	}
	u_long nonBlocking = 1;
	return ioctlsocket(socket, FIONBIO, &nonBlocking) == 0;
}

// stops another socket from taking over a port in use
static void ConfigureAddressReuse(NativeSocket socket, int)
{
	const int enable = 1;
	setsockopt(socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&enable), sizeof(enable));
}

static sint32 TranslateError(int error)
{
	switch (error)
	{
	case WSAEWOULDBLOCK:
	case WSAETIMEDOUT:
		return NetSocket::kErrorTimeout;
	case WSAECONNRESET:
	case WSAECONNABORTED:
		return NetSocket::kErrorConnectionReset;
	default:
		return NetSocket::kErrorOther;
	}
}
#else
using NativeSocket = int;
using AddressLength = socklen_t;
using IoLength = size_t;
using PollDescriptor = pollfd;
constexpr NativeSocket kInvalidNative = -1;
#if defined(MSG_NOSIGNAL)
constexpr int kSendFlags = MSG_NOSIGNAL;
#else
constexpr int kSendFlags = 0;
#endif
constexpr short kPollReadable = POLLIN;
constexpr short kPollWritable = POLLOUT;

static bool EnsureInitialized()
{
	return true;
}

static int GetLastSocketError()
{
	return errno;
}

static bool IsConnectPending(int error)
{
	return error == EINPROGRESS;
}

static void CloseNative(NativeSocket socket)
{
	close(socket);
}

static int PollNative(PollDescriptor& descriptor, int timeoutMs)
{
	return poll(&descriptor, 1, timeoutMs);
}

static bool ConfigureNewSocket(NativeSocket socket, int)
{
	fcntl(socket, F_SETFD, FD_CLOEXEC);
#if defined(SO_NOSIGPIPE)
	const int enable = 1;
	setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &enable, sizeof(enable));
#endif
	const int flags = fcntl(socket, F_GETFL, 0);
	return flags >= 0 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
}

// lets a port in TIME_WAIT be bound again
static void ConfigureAddressReuse(NativeSocket socket, int type)
{
	if (type != SOCK_STREAM)
		return;
	const int enable = 1;
	setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
}

static sint32 TranslateError(int error)
{
	// a signal ends a wait early, the caller retries like after a timeout
	if (error == EAGAIN || error == EWOULDBLOCK || error == EINTR)
		return NetSocket::kErrorTimeout;
	switch (error)
	{
	case ECONNRESET:
	case ECONNABORTED:
	case EPIPE:
		return NetSocket::kErrorConnectionReset;
	default:
		return NetSocket::kErrorOther;
	}
}
#endif

static_assert(sizeof(sockaddr_storage) <= NetSocket::Address::kMaxNativeSize);

static NativeSocket ToNative(uintptr_t handle)
{
	return static_cast<NativeSocket>(static_cast<intptr_t>(handle));
}

static uintptr_t FromNative(NativeSocket socket)
{
	return static_cast<uintptr_t>(static_cast<intptr_t>(socket));
}

static const sockaddr* AsSockaddr(const void* storage)
{
	return static_cast<const sockaddr*>(storage);
}

static int ToPollTimeout(std::chrono::milliseconds timeout)
{
	return static_cast<int>(std::clamp<std::chrono::milliseconds::rep>(timeout.count(), 0, (std::numeric_limits<int>::max)()));
}

static sint32 WaitFor(NativeSocket socket, short events, int timeoutMs)
{
	PollDescriptor descriptor{socket, events, 0};
	const int result = PollNative(descriptor, timeoutMs);
	if (result < 0)
		return TranslateError(GetLastSocketError());
	return result == 0 ? NetSocket::kErrorTimeout : 0;
}

static sint32 WaitReadable(NativeSocket socket, const std::atomic_bool& interrupted, std::chrono::milliseconds timeout)
{
	if (interrupted)
		return NetSocket::kErrorInterrupted;
	const sint32 result = WaitFor(socket, kPollReadable, ToPollTimeout(timeout));
	if (interrupted)
		return NetSocket::kErrorInterrupted;
	return result;
}

std::optional<NetSocket::Address> NetSocket::Address::Resolve(std::string_view host, uint16 port, std::optional<AddressFamily> family)
{
	if (!EnsureInitialized())
		return std::nullopt;

	addrinfo hints{};
	hints.ai_family = family ? (*family == AddressFamily::IPv4 ? AF_INET : AF_INET6) : AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	const std::string hostString(host);
	const std::string portString = std::to_string(port);
	addrinfo* results = nullptr;
	if (getaddrinfo(hostString.c_str(), portString.c_str(), &hints, &results) != 0 || !results)
		return std::nullopt;

	Address address;
	if (results->ai_addrlen <= sizeof(address.m_storage))
	{
		std::memcpy(address.m_storage, results->ai_addr, results->ai_addrlen);
		address.m_size = static_cast<uint32>(results->ai_addrlen);
	}
	freeaddrinfo(results);
	if (!address.IsValid())
		return std::nullopt;
	return address;
}

NetSocket::Address NetSocket::Address::Any(AddressFamily family, uint16 port)
{
	if (family == AddressFamily::IPv4)
		return FromIPv4(htonl(INADDR_ANY), port);
	Address address;
	sockaddr_in6 native{};
	native.sin6_family = AF_INET6;
	native.sin6_port = htons(port);
	native.sin6_addr = in6addr_any;
	std::memcpy(address.m_storage, &native, sizeof(native));
	address.m_size = sizeof(native);
	return address;
}

NetSocket::Address NetSocket::Address::Loopback(AddressFamily family, uint16 port)
{
	if (family == AddressFamily::IPv4)
		return FromIPv4(htonl(INADDR_LOOPBACK), port);
	Address address;
	sockaddr_in6 native{};
	native.sin6_family = AF_INET6;
	native.sin6_port = htons(port);
	native.sin6_addr = in6addr_loopback;
	std::memcpy(address.m_storage, &native, sizeof(native));
	address.m_size = sizeof(native);
	return address;
}

NetSocket::Address NetSocket::Address::FromIPv4(uint32 networkOrderAddress, uint16 port)
{
	Address address;
	sockaddr_in native{};
	native.sin_family = AF_INET;
	native.sin_port = htons(port);
	native.sin_addr.s_addr = networkOrderAddress;
	std::memcpy(address.m_storage, &native, sizeof(native));
	address.m_size = sizeof(native);
	return address;
}

NetSocket::AddressFamily NetSocket::Address::GetFamily() const
{
	return AsSockaddr(m_storage)->sa_family == AF_INET6 ? AddressFamily::IPv6 : AddressFamily::IPv4;
}

uint16 NetSocket::Address::GetPort() const
{
	if (!IsValid())
		return 0;
	if (GetFamily() == AddressFamily::IPv4)
		return ntohs(reinterpret_cast<const sockaddr_in*>(m_storage)->sin_port);
	return ntohs(reinterpret_cast<const sockaddr_in6*>(m_storage)->sin6_port);
}

bool NetSocket::Address::operator==(const Address& other) const
{
	if (!IsValid() || !other.IsValid())
		return IsValid() == other.IsValid();
	if (GetFamily() != other.GetFamily() || GetPort() != other.GetPort())
		return false;
	if (GetFamily() == AddressFamily::IPv4)
	{
		const auto a = reinterpret_cast<const sockaddr_in*>(m_storage);
		const auto b = reinterpret_cast<const sockaddr_in*>(other.m_storage);
		return a->sin_addr.s_addr == b->sin_addr.s_addr;
	}
	const auto a = reinterpret_cast<const sockaddr_in6*>(m_storage);
	const auto b = reinterpret_cast<const sockaddr_in6*>(other.m_storage);
	return std::memcmp(&a->sin6_addr, &b->sin6_addr, sizeof(a->sin6_addr)) == 0 && a->sin6_scope_id == b->sin6_scope_id;
}

NetSocket::~NetSocket()
{
	Close();
}

NetSocket::NetSocket(NetSocket&& other) noexcept
	: m_handle(other.m_handle), m_type(other.m_type), m_interrupted(other.m_interrupted.load())
{
	other.m_handle = kInvalidHandle;
	other.m_interrupted = false;
}

NetSocket& NetSocket::operator=(NetSocket&& other) noexcept
{
	if (this != &other)
	{
		Close();
		m_handle = other.m_handle;
		m_type = other.m_type;
		m_interrupted = other.m_interrupted.load();
		other.m_handle = kInvalidHandle;
		other.m_interrupted = false;
	}
	return *this;
}

std::optional<NetSocket> NetSocket::Open(AddressFamily family, Type type)
{
	if (!EnsureInitialized())
		return std::nullopt;

	const int nativeFamily = family == AddressFamily::IPv4 ? AF_INET : AF_INET6;
	const int nativeType = type == Type::Udp ? SOCK_DGRAM : SOCK_STREAM;
	const int nativeProtocol = type == Type::Udp ? IPPROTO_UDP : IPPROTO_TCP;
	const NativeSocket native = ::socket(nativeFamily, nativeType, nativeProtocol);
	if (native == kInvalidNative)
		return std::nullopt;
	NetSocket socket(FromNative(native), type);
	if (!ConfigureNewSocket(native, nativeType))
		return std::nullopt;
	ConfigureAddressReuse(native, nativeType);
	if (family == AddressFamily::IPv6)
	{
		const int enable = 1;
		setsockopt(native, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&enable), sizeof(enable));
	}
	return socket;
}

void NetSocket::Close()
{
	if (!IsOpen())
		return;
	CloseNative(ToNative(m_handle));
	m_handle = kInvalidHandle;
}

bool NetSocket::Bind(const Address& address)
{
	if (!IsOpen() || !address.IsValid())
		return false;
	return ::bind(ToNative(m_handle), AsSockaddr(address.m_storage), static_cast<AddressLength>(address.m_size)) == 0;
}

sint32 NetSocket::Connect(const Address& address, std::chrono::milliseconds timeout)
{
	if (!IsOpen() || !address.IsValid())
		return kErrorOther;
	const NativeSocket native = ToNative(m_handle);
	if (::connect(native, AsSockaddr(address.m_storage), static_cast<AddressLength>(address.m_size)) == 0)
		return 0;
	const int error = GetLastSocketError();
	if (!IsConnectPending(error))
		return TranslateError(error);
	if (const sint32 result = WaitFor(native, kPollWritable, ToPollTimeout(timeout)); result < 0)
		return result;
	int socketError = 0;
	AddressLength length = sizeof(socketError);
	if (getsockopt(native, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&socketError), &length) != 0)
		return TranslateError(GetLastSocketError());
	return socketError == 0 ? 0 : TranslateError(socketError);
}

bool NetSocket::Listen(sint32 backlog)
{
	if (!IsOpen())
		return false;
	return ::listen(ToNative(m_handle), backlog) == 0;
}

sint32 NetSocket::Accept(NetSocket& accepted, std::chrono::milliseconds timeout)
{
	if (!IsOpen())
		return kErrorOther;
	const NativeSocket native = ToNative(m_handle);
	if (const sint32 result = WaitFor(native, kPollReadable, ToPollTimeout(timeout)); result < 0)
		return result;
	const NativeSocket client = ::accept(native, nullptr, nullptr);
	if (client == kInvalidNative)
		return TranslateError(GetLastSocketError());
	NetSocket socket(FromNative(client), Type::Tcp);
	if (!ConfigureNewSocket(client, SOCK_STREAM))
		return kErrorOther;
	accepted = std::move(socket);
	return 0;
}

sint32 NetSocket::Send(const void* data, size_t size)
{
	if (!IsOpen())
		return kErrorOther;
	const NativeSocket native = ToNative(m_handle);
	size_t total = 0;
	while (total < size)
	{
		const auto sent = ::send(native, static_cast<const char*>(data) + total, static_cast<IoLength>(size - total), kSendFlags);
		if (sent >= 0)
		{
			total += static_cast<size_t>(sent);
			continue;
		}
		const sint32 error = TranslateError(GetLastSocketError());
		if (error != kErrorTimeout)
			return error;
		// the send buffer is full
		if (const sint32 result = WaitFor(native, kPollWritable, -1); result < 0 && result != kErrorTimeout)
			return result;
	}
	return static_cast<sint32>(total);
}

sint32 NetSocket::SendTo(const void* data, size_t size, const Address& to)
{
	if (!IsOpen())
		return kErrorOther;
	const auto sent = ::sendto(ToNative(m_handle), static_cast<const char*>(data), static_cast<IoLength>(size), kSendFlags,
		AsSockaddr(to.m_storage), static_cast<AddressLength>(to.m_size));
	if (sent < 0)
		return TranslateError(GetLastSocketError());
	return static_cast<sint32>(sent);
}

sint32 NetSocket::Receive(void* buffer, size_t size, std::chrono::milliseconds timeout)
{
	if (!IsOpen())
		return kErrorOther;
	const NativeSocket native = ToNative(m_handle);
	if (const sint32 result = WaitReadable(native, m_interrupted, timeout); result < 0)
		return result;
	const auto received = ::recv(native, static_cast<char*>(buffer), static_cast<IoLength>(size), 0);
	if (received < 0)
		return TranslateError(GetLastSocketError());
	return static_cast<sint32>(received);
}

sint32 NetSocket::ReceiveFrom(void* buffer, size_t size, Address& from, std::chrono::milliseconds timeout)
{
	if (!IsOpen())
		return kErrorOther;
	const NativeSocket native = ToNative(m_handle);
	if (const sint32 result = WaitReadable(native, m_interrupted, timeout); result < 0)
		return result;
	sockaddr_storage storage{};
	AddressLength length = sizeof(storage);
	const auto received = ::recvfrom(native, static_cast<char*>(buffer), static_cast<IoLength>(size), 0,
		reinterpret_cast<sockaddr*>(&storage), &length);
	if (received < 0)
		return TranslateError(GetLastSocketError());
	std::memcpy(from.m_storage, &storage, length);
	from.m_size = static_cast<uint32>(length);
	return static_cast<sint32>(received);
}

bool NetSocket::Interrupt()
{
	cemu_assert_debug(m_type == Type::Udp);
	m_interrupted = true;
	if (!IsOpen() || m_type != Type::Udp)
		return false;
	sockaddr_storage storage{};
	AddressLength length = sizeof(storage);
	if (getsockname(ToNative(m_handle), reinterpret_cast<sockaddr*>(&storage), &length) != 0)
		return false;
	Address address;
	std::memcpy(address.m_storage, &storage, length);
	address.m_size = static_cast<uint32>(length);
	if (address == Address::Any(address.GetFamily(), address.GetPort()))
		address = Address::Loopback(address.GetFamily(), address.GetPort());
	// the datagram wakes up the waiting receive
	const uint8 data = 0;
	return SendTo(&data, 1, address) == 1;
}

bool NetSocket::SetNoDelay(bool enable)
{
	if (!IsOpen())
		return false;
	const int value = enable ? 1 : 0;
	return setsockopt(ToNative(m_handle), IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&value), sizeof(value)) == 0;
}
