#include "../inc/Server.hpp"
#include <cstring>
#include <sys/socket.h>
#include <exception>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <iostream>

Server::Server(int port, std::string password, std::string server_name) : _port(port), _password(password), _server_name(server_name), _nfds(0), _listenerfd(-1)
{

};

Server::~Server()
{
	std::cout << "END OF PROGRAM" << std::endl;
	for (unsigned short i = 0; i < _nfds; i++)
		close(_pollfds[i].fd);
	close(_listenerfd);
};


void Server::start()
{
	struct sockaddr_in	_listener_socket_addr;
	std::memset(&_listener_socket_addr, 0, sizeof(_listener_socket_addr));

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
	int flags = fcntl(_listenerfd, F_GETFL, 0);
	if (fcntl(_listenerfd, F_SETFL, flags | O_NONBLOCK) < 0)
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
	_pollfds[_nfds].fd = _listenerfd;
	_pollfds[_nfds].events = POLLIN | POLLOUT | POLLHUP;
	_nfds++;

	_event_loop();
}


void Server::_event_loop()
{
	// 512 is max in irc. \r\n included
	char buf[MAX_MESSAGE_LENGHT];
	while (true)
	{
		int	socket_change_count = poll(&_pollfds[0], _nfds, 0);

		if (socket_change_count < 0)
			throw std::runtime_error("Failed To Poll Socket/s");

		for (int i = 0; i < _nfds; i++)
		{
			// no event on socket
			if (_pollfds[i].revents == 0)
				continue;
			// error on socket -> remove from pollfds
			if (_pollfds[i].revents & POLLERR)
			{
				Client	to_disconnect = _fd_to_client[_pollfds[i].fd];

			}
				;//throw std::runtime_error("An Error Has Occured On A Filedescriptor");

			if (_pollfds[i].revents & POLL_HUP)
				;

			// new client is connecting
			if (_pollfds[i].fd == _listenerfd && _pollfds[i].revents & POLLIN)
				_accept_new_connection();
			else
			{
				if (_pollfds[i].revents & POLLIN)
					_parse_incoming_data(_pollfds[i].fd);
				else if ((_pollfds[i].revents & POLLOUT))
				{
					_fd_to_client[_pollfds[i].fd].clear_out_buffer();
				}
			}
		}
	}
}

void Server::_accept_new_connection()
{
	while (true)
	{
		int new_socket_fd = accept(_pollfds[0].fd, NULL, NULL);
		if (new_socket_fd < 0)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
				throw std::runtime_error("Failed To Open Socket");
			else
				return ;
		}

		// Set O_NONBLOCK flag to enable nonblocking I/O
		int flags = fcntl(new_socket_fd, F_GETFL, 0);
		if (fcntl(new_socket_fd, F_SETFL, flags | O_NONBLOCK) < 0)
			throw std::runtime_error("Failed To Set Socket To Non-Blocking");

		int optval = 1;
		// Set SO_NOSIGPIPE option to prevent SIGPIPE signals on write errors
		if (setsockopt(new_socket_fd, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval)))
			throw std::runtime_error("Failed To Set Socket To Not SIGPIPE");

		_pollfds[_nfds].fd = new_socket_fd;
		_pollfds[_nfds].events = POLLIN | POLLOUT;

		_fd_to_client[new_socket_fd] = Client(new_socket_fd);

		std::cout << "Accepted Client: " << _nfds << std::endl;

		_nfds++;
	}
}

void	Server::_parse_incoming_data(int fd)
{
	char buf[MAX_MESSAGE_LENGHT] = {0};
	// read incoming msg
	std::size_t received_bytes = recv(fd, buf, MAX_MESSAGE_LENGHT, 0);

	if (received_bytes < 0)
		throw std::runtime_error("Failed To Read Incoming Message");
	
	Client	client = _fd_to_client[fd];
	client.append_in_buffer(buf);

	// (check if the message is bigger then 512) -> send error
	if (client.is_incoming_msg_too_long())
		;
	
	// msg needs to end with \r\n
	if (!client.is_incoming_msg_complete())
		return;

	// command is not supported
	// if (!)


	ClientStatus client_status = client.get_status();
	// Client is NOT registered
	if (client_status != registered)
	{
		// Client not registered
		if (client_status == pass)
			_cmd_pass(client); // check for password msg
		else if (client_status == nick)
			; // check for nick msg
		else if (client_status == user)
			; // check for user msg

		// Parse the in_buf of unregisered_client into the nickname, ...
		// parse message (check if the message is bigger then 512)
		// add unregistered_client to username_to_client
		// add username to fd_to_username
		// delete unregistered_client form _ft_to_uregistered_clients
		return ;
	}
	
	// Client is registered


}





// Cmd's

void	Server::_cmd_pass(const Client& client)
{

};

void	Server::_cmd_nick(const Client& client)
{

};

void	Server::_cmd_user(const Client& client)
{

};
