#include "../inc/Client.hpp"
#include <unistd.h>
#include <sys/socket.h>

std::map<std::string, Channel>*	Client::_server_channels = NULL;

Client::Client() {};


Client::Client(int fd) : _fd(fd)
{

}

Client::~Client()
{
	close(_fd);
};

Client::Client(const Client& other)
{
	*this = other;
}

Client& Client::operator= (const Client& other)
{
	if (this != &other)
	{
		_fd = other._fd;

		_nickname = other._nickname;
		_username = other._username;
		_realname = other._realname;

		_is_registered = other._is_registered;

		_server_channels = other._server_channels;
		_active_channel = other._active_channel;

		_in_buffer = other._in_buffer;
		_out_buffer = other._in_buffer;
	}
	return *this;
}

void Client::setup_client_data(std::string nickname, std::string username, std::string realname)
{
	_nickname = nickname;
	_username = username;
	_realname = realname;
}

void Client::append_out_buffer(std::string buffer)
{
	_out_buffer += buffer;
}

void Client::append_in_buffer(std::string buffer)
{
	_in_buffer += buffer;
}

bool Client::is_incoming_msg_complete() const
{
	return _in_buffer.find(std::string("\r\n")) != std::string::npos;
}	

bool Client::is_registered() const
{
	return _is_registered;
}


void Client::clear_in_buffer()
{
	std::string::size_type index = _in_buffer.find(std::string("\r\n"));
	if (index != std::string::npos)
		_in_buffer.erase(0, index);
}

void Client::clear_out_buffer()
{
	std::string::size_type index = _out_buffer.find(std::string("\r\n"));
	if (index != std::string::npos)
		_in_buffer.erase(0, index);
}
