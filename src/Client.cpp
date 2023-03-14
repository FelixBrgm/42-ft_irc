#include "../inc/Client.hpp"
#include <unistd.h>
#include <sys/socket.h>

Client::Client() {}

Client::Client(int fd) : _fd(fd) {}

Client::~Client() {}

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
		_hostname = other._hostname;

		_registration_status = other._registration_status;

		_joined_channels = other._joined_channels;
		_active_channel = other._active_channel;

		_in_buffer = other._in_buffer;
		_response_buffer = other._response_buffer;
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

void Client::append_out_buffer(char* buffer)
{
	_out_buffer += std::string(buffer);
}

void Client::append_in_buffer(char* buffer)
{
	_in_buffer += std::string(buffer);
}

bool Client::is_incoming_msg_complete() const
{
	return _in_buffer.find(std::string("\r\n")) != std::string::npos;
}	

bool Client::is_incoming_msg_too_long() const
{
	return MAX_MESSAGE_LENGHT < _in_buffer.length();
}

ClientStatus Client::get_status() const
{
	return _registration_status;
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


void Client::disconnect()
{
	if (_joined_channels.size())
	{
		// send that im disconnecting
		for (std::map<std::string, Channel*>::iterator it = _joined_channels.begin(); it != _joined_channels.end(); it++)
		{
			
		}
	}
}

std::string Client::get_in_buffer()
{
	return _in_buffer;
}
