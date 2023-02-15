#include "../inc/Client.hpp"
#include <unistd.h>
#include <sys/socket.h>
Client::Client(int fd) : _fd(fd) {}

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
		_nickname = other._nickname;
		_username = other._username;
		_realname = other._realname;
		_fd = other._fd;
		_in_buffer = other._in_buffer;
		_out_buffer = other._in_buffer;
	}
	return *this;
}


void Client::set_user_data(std::string nickname, std::string username, std::string realname)
{
	_nickname = nickname;
	_username = username;
	_realname = realname;
}


void Client::append_buffer(std::string buffer)
{
	_in_buffer += buffer;
}


bool Client::is_incoming_msg_complete() const
{
	return _in_buffer.find(std::string("\r\n")) != std::string::npos;
}	
