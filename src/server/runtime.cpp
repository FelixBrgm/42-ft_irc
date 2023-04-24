#include "../../inc/Server.hpp"

#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <errno.h>

void Server::run()
{
	struct sockaddr_in	_listener_socket_addr;
	memset(&_listener_socket_addr, 0, sizeof(_listener_socket_addr));

	// Set attributes of the socket
	_listener_socket_addr.sin_port = htons(_port);
	_listener_socket_addr.sin_family = AF_INET;
	_listener_socket_addr.sin_addr.s_addr = INADDR_ANY;

	// Create a socket to recieve incomming connections (LISTENER SOCKET)
	_listenerfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenerfd < 0)
		throw std::runtime_error("Failed To Open Socket");

	int optval = 1;
	// Set SO_REUSEADDR option to allow reuse of local addresses
	if (setsockopt(_listenerfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
		throw std::runtime_error("Failed To Set Socket To Be Reusable");

	// Set O_NONBLOCK flag to enable nonblocking I/O

	if (fcntl(_listenerfd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("Failed To Set Socket To Non-Blocking");

	// Set SO_NOSIGPIPE option to prevent SIGPIPE signals on write errors
	if (setsockopt(_listenerfd, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval)))
		throw std::runtime_error("Failed To Set Socket To Not SIGPIPE");

	// Bind the listening socket to the port configured
	if (bind(_listenerfd, reinterpret_cast<struct sockaddr*>(&_listener_socket_addr), sizeof(_listener_socket_addr)) < 0)
		throw std::runtime_error("Failed To Bind Socket To Port");
	
	// Sets the fd into passiv mode and allows a maxium of 128 request to queue up
	if (listen(_listenerfd, 128) < 0)
		throw std::runtime_error("Failed To Mark Socket As Passive");

	// First fd in the array is the listener
	struct pollfd socket_info;
	std::memset(&socket_info, 0, sizeof(socket_info));
	socket_info.fd = _listenerfd;
	socket_info.events = POLLIN;
	_pollfds.push_back(socket_info);
	_nfds++;

	_event_loop();
}

void Server::_event_loop()
{
	// 512 is max in irc. \r\n included
	while (true)
	{
		int	socket_change_count = poll(&_pollfds[0], _nfds, 0);

		if (socket_change_count < 0)
			throw std::runtime_error("Failed To Poll Socket/s");

		for (int i = 0; i < _nfds; i++)
		{

			if (_pollfds[i].revents == 0)
				continue;
			// error on socket -> remove from pollfds
			if ((_pollfds[i].revents & POLLERR) == POLLERR)
			{
				_unexpected_client_disconnection(&_fd_to_client[_pollfds[i].fd]);
				break;
			}

			if ((_pollfds[i].revents & POLLHUP) == POLLHUP)
			{
				_unexpected_client_disconnection(&_fd_to_client[_pollfds[i].fd]);
				break;
			}


			// new client is connecting
			if (_pollfds[i].fd == _listenerfd && (_pollfds[i].revents & POLLIN) == POLLIN)
				_accept_new_connection();
			else
			{
				if (_pollfds[i].revents & POLLIN)
					_parse(_pollfds[i].fd);
			}
		}

		for (int i = 0; i < _nfds; i++)
		{
			std::map<int, Client>::iterator it = _fd_to_client.find(_pollfds[i].fd);
			if (it == _fd_to_client.end())
				continue;

    		Client& client = it->second;

			if (!client.is_response_complete())
				continue;
			if(client.send_out_buffer())
			{
				_unexpected_client_disconnection(&client);
			}
		}
	}
}

void Server::_accept_new_connection()
{
	std::cout << "ACCEPT NEW CONNECTION" << std::endl;
	while (true)
	{
		int new_socket_fd = accept(_pollfds[0].fd, NULL, NULL);
		if (new_socket_fd < 0)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				std::cerr << "Failed To Open Socket -> Client Could Not Connect" << std::endl;
				return;
			}
			else
				return ;
		}

		// Set O_NONBLOCK flag to enable nonblocking I/O

		if (fcntl(new_socket_fd, F_SETFL, O_NONBLOCK) < 0)
		{
				std::cerr << "Failed To Set Socket To Non-Blocking -> Client Could Not Connect" << std::endl;
				return;
		}
		int optval = 1;
		// Set SO_NOSIGPIPE option to prevent SIGPIPE signals on write errors
		if (setsockopt(new_socket_fd, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval)))
		{
				std::cerr << "Failed To Set Socket To Not SIGPIPE -> Client Could Not Connect" << std::endl;
				return;
		}
		struct pollfd socket_info;
		std::memset(&socket_info, 0, sizeof(socket_info));
		socket_info.fd = new_socket_fd;
		socket_info.events = POLLIN;
		_pollfds.push_back(socket_info);

		_fd_to_client[new_socket_fd] = Client(new_socket_fd);

		std::cout << "Accepted Client: " << _nfds << std::endl;

		_nfds++;
	}
}