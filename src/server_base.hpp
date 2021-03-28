#pragma once

#include "network/socket.hpp"
#include "scheduler.hpp"

class server_base
{
public:
	server_base(const network::address& bind_addr);
	~server_base() = default;

	void run();
	void stop();

	void send_command(const network::address& target, const std::string& command, const std::string& data) const;

protected:
	scheduler scheduler_{};
	network::socket socket_{};

private:
	volatile bool stopped_ = false;
	bool receive_data() const;
};