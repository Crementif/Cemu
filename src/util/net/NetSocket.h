#pragma once

// sockets are always non-blocking, receives wait up to the given timeout
class NetSocket
{
public:
	enum class AddressFamily
	{
		IPv4,
		IPv6,
	};

	enum class Type
	{
		Udp,
		Tcp,
	};

	// results are negative on error, a successful call never returns a negative size
	static constexpr sint32 kErrorTimeout = -1;
	static constexpr sint32 kErrorInterrupted = -2;
	static constexpr sint32 kErrorConnectionReset = -3;
	static constexpr sint32 kErrorOther = -4;

	class Address
	{
	public:
		static constexpr size_t kMaxNativeSize = 128;

		Address() = default;

		static std::optional<Address> Resolve(std::string_view host, uint16 port, std::optional<AddressFamily> family = std::nullopt);
		static Address Any(AddressFamily family, uint16 port = 0);
		static Address Loopback(AddressFamily family, uint16 port);
		static Address FromIPv4(uint32 networkOrderAddress, uint16 port);

		bool IsValid() const { return m_size != 0; }
		AddressFamily GetFamily() const;
		uint16 GetPort() const;

		bool operator==(const Address& other) const;

	private:
		friend class NetSocket;

		alignas(8) uint8 m_storage[kMaxNativeSize]{};
		uint32 m_size = 0;
	};

	NetSocket() = default;
	~NetSocket();
	NetSocket(NetSocket&& other) noexcept;
	NetSocket& operator=(NetSocket&& other) noexcept;
	NetSocket(const NetSocket&) = delete;
	NetSocket& operator=(const NetSocket&) = delete;

	static std::optional<NetSocket> Open(AddressFamily family, Type type);

	bool IsOpen() const { return m_handle != kInvalidHandle; }
	void Close();

	bool Bind(const Address& address);
	sint32 Connect(const Address& address, std::chrono::milliseconds timeout);
	bool Listen(sint32 backlog = 16);
	sint32 Accept(NetSocket& accepted, std::chrono::milliseconds timeout);

	// sends all of data unless an error occurs
	sint32 Send(const void* data, size_t size);
	sint32 SendTo(const void* data, size_t size, const Address& to);
	// a TCP receive of size 0 means the peer closed the connection
	sint32 Receive(void* buffer, size_t size, std::chrono::milliseconds timeout);
	sint32 ReceiveFrom(void* buffer, size_t size, Address& from, std::chrono::milliseconds timeout);

	// UDP only, wakes up a receive waiting in another thread, after which receives return kErrorInterrupted
	bool Interrupt();

	bool SetNoDelay(bool enable);

private:
	using NativeHandle = uintptr_t;
	static constexpr NativeHandle kInvalidHandle = ~NativeHandle(0);

	NetSocket(NativeHandle handle, Type type) : m_handle(handle), m_type(type) {}

	NativeHandle m_handle = kInvalidHandle;
	Type m_type = Type::Udp;
	std::atomic_bool m_interrupted = false;
};
