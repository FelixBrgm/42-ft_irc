#include "../inc/Server.hpp"
#include <cstring>
#include <sys/socket.h>
#include <exception>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <iostream>

Server::Server(int port, std::string password) : _port(port), _password(password), _nfds(0), _listenerfd(-1)
{
	std::memset(_fds, 0, sizeof(_fds));
	std::memset(&_listener_socket_addr, 0, sizeof(_listener_socket_addr));
};

Server::~Server()
{
	for (unsigned short i = 0; i < _nfds; i++)
		close(_fds[i].fd);
};





void Server::start()
{
	_listenerfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenerfd < 0)
		throw std::runtime_error("Failed To Open Socket");


 	int opt = 1;
    setsockopt(_listenerfd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));


	_listener_socket_addr.sin_port = htons(_port);
	_listener_socket_addr.sin_family = AF_INET;
	_listener_socket_addr.sin_len = sizeof(_listener_socket_addr);
	_listener_socket_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


	if (fcntl(_listenerfd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("Failed To Set Socket To Non-Blocking");

	if (bind(_listenerfd, reinterpret_cast<struct sockaddr*>(&_listener_socket_addr), _listener_socket_addr.sin_len) < 0)
		throw std::runtime_error("Failed To Bind Socket To Port");

	if (listen(_listenerfd, 128) < 0)
		throw std::runtime_error("Failed To Mark Socket As Passive");
	_nfds++;

	_fds->fd = _listenerfd;
	_fds->events = POLLIN;
	loop();
}

void Server::_accept()
{
	int new_socket_fd = accept(_fds[0].fd, NULL, NULL);
	if (new_socket_fd == EAGAIN || new_socket_fd == EWOULDBLOCK)
		;
	else if (new_socket_fd < 0)
		throw std::runtime_error("Failed To Open Socket");
	
	if (fcntl(new_socket_fd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("Failed To Set Socket To Non-Blocking");
	_fds[_nfds].fd = new_socket_fd;
	_fds[_nfds].events = POLLIN;
	_nfds++;
	std::cout << "accepted new client" << std::endl;
}

//


void Server::loop()
{
	char buf[255];
	memset(buf, 0, sizeof(buf));
	while (true)
	{
		// std::cout << "PREPOLL" << std::endl;

		int	socket_change_count = poll(_fds, _nfds, 0);
		// std::cout << "AFTERPOLL" << std::endl;

		if (socket_change_count < 0)
			throw std::runtime_error("Failed To Poll Socket/s");
		// std::cout << "LOOP" << std::endl;
		for (int i = 0; i < _nfds; i++)
		{

			if (_fds[i].revents == 0)
				continue;
			// std::cout << _nfds << std::endl;
			
			if (!(_fds[i].revents & POLLIN))
			{
				std::cout << i << std::endl;
				throw std::runtime_error("Failed To Recognize Event Returned By Poll (Should be POLLIN)");
			}
		

			if (_fds[i].fd == _listenerfd)
				_accept();
			else
			{
				recv(_fds[i].fd,buf, 255, 0);
				if (std::string(buf).size())
				std::cout << std::string(buf) << std::endl;
				memset(buf, 0, sizeof(buf));
			}
		}
	}
}