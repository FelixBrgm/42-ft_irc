#include "../inc/Client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <iostream>

Client::Client() : _registration_status(pass) {}

Client::Client(int fd) : _fd(fd), _registration_status(pass) {}

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


		_registration_status = other._registration_status;

		_joined_channels = other._joined_channels;
		_active_channel = other._active_channel;

		_in_buffer = other._in_buffer;
		_response_buffer = other._response_buffer;
		_out_buffer = other._in_buffer;
	}
	return *this;
}

void Client::append_response_buffer(std::string buffer)
{
	_response_buffer += buffer;
}

void Client::append_in_buffer(char* buffer)
{
	_in_buffer += std::string(buffer);
}

void Client::append_out_buffer(std::string buffer)
{
	_out_buffer += buffer;
}

bool Client::is_incoming_msg_complete() const
{
	return _in_buffer.find(std::string("\r\n")) != std::string::npos;
}	

bool Client::is_incoming_msg_too_long() const
{
	std::size_t index = _in_buffer.find(std::string("\r\n"));
	if (index != std::string::npos)
	{
		return index >= (MAX_MESSAGE_LENGHT - 1);
	}
	return _in_buffer.length() > MAX_MESSAGE_LENGHT - 2;
}

ClientStatus Client::get_status() const
{
	return _registration_status;
}

std::string Client::get_nickname() const
{
	return _nickname;
}
std::string Client::get_username() const
{
	return _username;
}
std::string Client::get_realname() const
{
	return _realname;
}
void Client::set_nickname(std::string& nickname)
{
	_nickname = nickname;
}

void Client::set_username(std::string& username)
{
	_username = username;
}

void Client::set_realname(std::string& realname)
{
	_realname = realname;
}


void Client::cut_msg_in_buffer()
{
	std::string::size_type index = _in_buffer.find(std::string("\r\n"));
	if (index != std::string::npos)
		_in_buffer.erase(0, index + 2);
}

void Client::clear_out_buffer()
{
	std::string::size_type index = _out_buffer.find(std::string("\r\n"));
	if (index != std::string::npos)
		_in_buffer.erase(0, index);
}


void Client::disconnect()
{
	
}

std::string Client::get_in_buffer()
{
	return _in_buffer;
}


bool Client::is_response_complete() const
{
	return _response_buffer.length() != 0;
};


void Client::send_out_buffer()
{
	int ret = send(_fd, NULL, 0, SO_NOSIGPIPE);
	if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
	{
	    // The writing operation would block
		return;
	}
	else if (ret == -1)
	{
	    // Handle error -> disconnect client
		return ;
	}


	_out_buffer = _response_buffer;
	_response_buffer.clear();

	int bytes_sent = send(_fd, _out_buffer.c_str(), _out_buffer.length(), SO_NOSIGPIPE);
	
	if (bytes_sent == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			// real send error -> dc client;
		}
	}
}


void Client::proceed_registration_status()
{
	if (_registration_status == pass)
		_registration_status = nick;
	else if (_registration_status == nick)
		_registration_status = user;
	else if (_registration_status == user)
		_registration_status = registered;
};


void Client::join_channel(std::string channel_name, Channel *channel_to_join)
{
	// maybe need to set active channel pointer

	_joined_channels[channel_name] = channel_to_join;
}

std::map<std::string, Channel*> Client::get_joined_channels()
{
	return _joined_channels;
}

int	Client::get_fd() const
{
	return _fd;
}
