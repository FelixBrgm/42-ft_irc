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
	struct pollfd socket_info;
	std::memset(&socket_info, 0, sizeof(socket_info));
	socket_info.fd = _listenerfd;
	socket_info.events = POLLIN | POLLOUT | POLLHUP;
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
			// no event on socket
			if (_pollfds[i].revents == 0)
				continue;
			// error on socket -> remove from pollfds
			if ((_pollfds[i].revents & POLLERR) == POLLERR)
			{
				;// dc client and restructure vector 
			}

			if ((_pollfds[i].revents & POLLHUP) == POLLHUP)
			{
				;// dc client and restructure vector 
			}

			// new client is connecting
			if (_pollfds[i].fd == _listenerfd && (_pollfds[i].revents & POLLIN) == POLLIN)
				_accept_new_connection();
			else
			{
				if (_pollfds[i].revents & POLLIN)
					_parse_incoming_data(_pollfds[i].fd);
				else if ((_pollfds[i].revents & POLLOUT))
				{
					// look if something is in response buffer if no
					// copy res to outbuffer (never modify outbuffer besides here)
					// send to client
				}
			}
			

			//send
		}

		for (int i = 0; i < _nfds; i++)
		{
			Client& client = _fd_to_client[_pollfds[i].fd];

			if (!client.is_response_complete())
				continue;
			
			client.send_out_buffer();
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
				throw std::runtime_error("Failed To Open Socket"); // maybe not kill server, just write error msg to std err
			else
				return ;
		}

		// Set O_NONBLOCK flag to enable nonblocking I/O
		int flags = fcntl(new_socket_fd, F_GETFL, 0);
		if (fcntl(new_socket_fd, F_SETFL, flags | O_NONBLOCK) < 0)
			throw std::runtime_error("Failed To Set Socket To Non-Blocking"); // maybe not kill server, just write error msg to std err

		int optval = 1;
		// Set SO_NOSIGPIPE option to prevent SIGPIPE signals on write errors
		if (setsockopt(new_socket_fd, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval)))
			throw std::runtime_error("Failed To Set Socket To Not SIGPIPE"); // maybe not kill server, just write error msg to std err

		struct pollfd socket_info;
		std::memset(&socket_info, 0, sizeof(socket_info));
		socket_info.fd = new_socket_fd;
		socket_info.events = POLLIN | POLLOUT | POLLHUP;
		_pollfds.push_back(socket_info);

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

	if (received_bytes == 0)
		;// client closed remote unexpecteectly -> disconnect him
	if (received_bytes < 0)
		throw std::runtime_error("Failed To Read Incoming Message"); // just disconnect client, dont kill server
	
	Client&	client = _fd_to_client[fd];
	client.append_in_buffer(buf);


	// (check if the message is bigger then 512) -> send error
	if (client.is_incoming_msg_too_long())
		;
	
	// msg needs to end with \r\n
	if (!client.is_incoming_msg_complete())
		return;





	// PARSING OF PREFIX


	// Parse Prefix

	std::string client_message = client.get_in_buffer().substr(0, client.get_in_buffer().find("\r\n"));

	if (client_message[0] == ':')
	{
    	size_t prefix_end = client_message.find(' ');
    	client_message = client_message.substr(prefix_end);
	}

	std::string command = client_message.substr(0, client_message.find(std::string(" ")));

	std::vector<std::string> params;
	
	std::string temp = client_message.substr(client_message.find(std::string(" ")));
	temp = temp.substr(1);
	while (temp.length() && temp[0] != ':')
	{
		std::size_t index = temp.find(std::string(" "));
		params.push_back(temp.substr(0, index));
		if (index != std::string::npos)
		{
			temp = temp.substr(index);
			temp.substr(1);
		}
		else
		{
			temp.clear();
		}
	}
	if (temp.length())
	{
		temp.substr(1);
		params.push_back(temp);
	}

	if (command == std::string("PASS"))
		_cmd_pass(&client, params);
	else if (command == std::string("NICK"))
		_cmd_nick(&client, params);
	else if (command == std::string("USER"))
		_cmd_user(&client, params);
	else
	{
		;// send error 421
	}


	ClientStatus client_status = client.get_status();
	// Client is NOT registered
	if (client_status != registered)
	{
		// Client not registered
		if (client_status == pass)
			; //_cmd_pass(client); // check for password msg
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

	// to send back just write to the fd of the client     
}





// Cmd's

void	Server::_cmd_pass(Client* client, std::vector<std::string> params)
{
	(void)client;
	(void)params;
};

void	Server::_cmd_nick(Client* client, std::vector<std::string> params)

{
	(void)client;
	(void)params;
};

void	Server::_cmd_user(Client* client, std::vector<std::string> params)
{
	(void)params;
	(void)client;
};
