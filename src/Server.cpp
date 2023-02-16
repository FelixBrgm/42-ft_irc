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
	std::cout << "END OF PROGRAM" << std::endl;
	for (unsigned short i = 0; i < _nfds; i++)
		close(_fds[i].fd);
	close(_listenerfd);
};





void Server::start()
{
	// Create a socket to recieve incomming connections (LISTENER SOCKET)
	_listenerfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenerfd < 0)
		throw std::runtime_error("Failed To Open Socket");


	// Alloc the socket to be reused - so that multiple connection can be accepted
 	int opt = 1;
    setsockopt(_listenerfd, SOL_SOCKET, SO_NOSIGPIPE | SO_REUSEADDR, &opt, sizeof(opt));
// setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
	// Set attributes of the socket
	_listener_socket_addr.sin_port = htons(_port);
	_listener_socket_addr.sin_family = AF_INET;
	_listener_socket_addr.sin_len = sizeof(_listener_socket_addr);
	_listener_socket_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// Bind the listening socket to the port configured
	if (bind(_listenerfd, reinterpret_cast<struct sockaddr*>(&_listener_socket_addr), _listener_socket_addr.sin_len) < 0)
		throw std::runtime_error("Failed To Bind Socket To Port");
	
	// Sets the filedescriptor of the socket to non blocking
	if (fcntl(_listenerfd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("Failed To Set Socket To Non-Blocking");

	// Sets the fd into passiv mode and allows a maxium of 128 request to queue up
	if (listen(_listenerfd, 128) < 0)
		throw std::runtime_error("Failed To Mark Socket As Passive");

	// First fd in the array is the listener
	_fds[_nfds].fd = _listenerfd;
	_fds[_nfds].events = POLLIN;
	_nfds++;

	loop();
}


void Server::loop()
{
	// 512 is max in irc. \r\n included
	char buf[MAX_MESSAGE_LENGHT];
	while (true)
	{
		int	socket_change_count = poll(_fds, _nfds, 0);

		if (socket_change_count < 0)
			throw std::runtime_error("Failed To Poll Socket/s");

		for (int i = 0; i < _nfds; i++)
		{

			if (_fds[i].revents == 0)
				continue;
			// event happened on filedescriptor
			if (_fds[i].revents & POLLIN)
				;// handle 
			else if ((_fds[i].revents & POLLOUT))
				;//
			else
				throw std::runtime_error("Failed To Recognize Event Returned By Poll");

			if (_fds[i].fd == _listenerfd)
				_accept();
			else
			{
				std::size_t received_bytes = recv(_fds[i].fd, buf, 512, 0);
				if (received_bytes == -1)
					throw std::runtime_error("Failed To Read Incoming Message");

				_handle(_fds[i].fd, std::string(buf));
				memset(buf, 0, 1024);
			}
		}
		//send	
	}
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

	_fd_to_unregistered_clients[new_socket_fd] = Client(new_socket_fd);

	std::cout << "accepted new client number: " << _nfds << std::endl;
	_nfds++;
}

void Server::_handle(int fd, std::string buffer) {

	if (buffer.length() == 0) return;

	// When user data is not set in the client
	if (_fd_to_unregistered_clients.count(fd) == 1)
	{
		Client unregistered_client = _fd_to_unregistered_clients[fd];

		unregistered_client.append_buffer(buffer);

		if (unregistered_client.is_incoming_msg_complete())
		{
			// Parse the in_buf of unregistered_client into the nickname, ...
			// parse message (check if the message is bigger then 512)
			// add unregistered_client to username_to_client
			// add username to fd_to_username
			// delete unregistered_client form _ft_to_uregistered_clients
		}
		return;
	}

	// When user data is set in client
	std::string username = _fd_to_username[fd];
	Client cur_client = _username_to_client[username];

	cur_client.append_buffer(buffer);
	if (cur_client.is_incoming_msg_complete())
	{
		// parse message (check if the message is bigger then 512)
		// send message
	}
}

void Server::_parse(std::string message)
{
	// Check if the first char is a /
	// check if the thing afterwards is a command
	// have a if for each command
	if ()
}