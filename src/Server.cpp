#include "../inc/Server.hpp"
#include <cstring>
#include <sys/socket.h>
#include <exception>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <string>

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




	std::string message = client.get_in_buffer();
	std::string command;
	std::vector<std::string> params;
	std::istringstream iss(message);
	std::string token;

	// Check for prefix and remove it
	if (message[0] == ':') {
		iss >> token; // Read and discard the prefix
	}

	// Extract the command
	iss >> command;

	// Extract the parameters
	while (iss >> token)
	{
		if (token[0] == ':')
		{
			// Extract the trailing part
			std::string trailing;
			std::getline(iss, trailing);
			params.push_back(token.substr(1) + trailing);
			break;
		}
		else
		{
			params.push_back(token);
		}
	}




	if (command == std::string("PASS"))
		_cmd_pass(&client, params);
	else if (command == std::string("NICK"))
		_cmd_nick(&client, params);
	else if (command == std::string("USER"))
		_cmd_user(&client, params);
	else
	{
		std::string msg(std::string("421 ") + command + std::string(" :Unknown command\r\n"));
		send(fd, msg.c_str(), msg.length(), SO_NOSIGPIPE);

	}



	// Client is registered

	// to send back just write to the fd of the client     
}


// Cmd's Helpers

bool Server::_username_already_exists(const std::string& nickname)
{
	return std::find(_taken_usernames.begin(), _taken_usernames.end(), nickname) != _taken_usernames.end();
}

bool Server::_is_valid_nickname(const std::string& nickname)
{
	// Check for lenght 9
    if (nickname.empty() || nickname.length() > 9)
        return false;

    // Check if the first character is a letter or special character
    if (!std::isalpha(nickname[0]))
        return false;

	// valid nickname
	return true;
}

// Cmd's

void	Server::_cmd_pass(Client* client, std::vector<std::string> params)
{
	if (client->get_status() != pass)
	{
	// 462     ERR_ALREADYREGISTRED
	// 						":You may not reregister"

	// 				- Returned by the server to any link which tries to
	// 				change part of the registered details (such as
	// 				password or user details from second USER message).
		return;
	}
	// we ignore if more params are passed
	if (params.size() < 1)
	{
			// 461     ERR_NEEDMOREPARAMS
			// 				"<command> :Not enough parameters"

			// 		- Returned by the server by numerous commands to
			// 		indicate to the client that it didn't supply enough
			// 		parameters.
		return;
	}

    if (params[0] != _password)
	{
			// 464     ERR_PASSWDMISMATCH
			// 				":Password incorrect"

			// 		- Returned to indicate a failed attempt at registering
			// 		a connection for which a password was required and
			// 		was either not given or incorrect.
        return;
    }

	// valid password
	client->proceed_registration_status();
};

void	Server::_cmd_nick(Client* client, std::vector<std::string> params)
{
    if (client->get_status() == pass)
    {
        // 451 ERR_NOTREGISTERED
        // ":You have not registered"
        // Send the error message to the client
        return;
    }
	
	if (params.size() == 0)
	{
		// 431 ERR_NONICKNAMEGIVEN
		// ":No nickname given"
		// Send the error message to the client
		return;
	}	
	std::string new_nickname = params[0];	
	if (!_is_valid_nickname(new_nickname))
	{
		// 432 ERR_ERRONEUSNICKNAME
		// "<nick> :Erroneous nickname"
		// Send the error message to the client
		return;
	}

	if (_username_already_exists(new_nickname))
	{
		// Handle nickname collision
		// The client is directly connected, so send ERR_NICKCOLLISION
		// 436 ERR_NICKCOLLISION
		// "<nick> :Nickname collision KILL"
		// Send the error message to the client
		// disconnect client
		std::vector<std::string>::iterator it = std::find(_taken_usernames.begin(), _taken_usernames.end(), new_nickname);
		if (it != _taken_usernames.end())
			_taken_usernames.erase(it);
		return;
	}	


	// Update the client's nickname
	std::string old_nickname = client->get_nickname();
	client->set_nickname(new_nickname);	

	std::vector<std::string>::iterator it = std::find(_taken_usernames.begin(), _taken_usernames.end(), old_nickname);
	if (it != _taken_usernames.end())
	    _taken_usernames.erase(it);
	// If the client was already registered, send a notification to other users in the same channels
	if (client->get_status() == registered)
	{
		// Notify other users in the same channels about the nickname change
	}

	// if status is nick we proceed to user
	if (client->get_status() == nick)
		client->proceed_registration_status();
};

void	Server::_cmd_user(Client* client, std::vector<std::string> params)
{
	if (client->get_status() != user)
	{
		// 462     ERR_ALREADYREGISTRED
		//         ":You may not reregister"
		//
		// Returned by the server to any link which tries to
		// change part of the registered details (such as
		// password or user details from second USER message).
		return;
	}

	if (params.size() < 4)
	{
		// 461     ERR_NEEDMOREPARAMS
		//         "<command> :Not enough parameters"
		//
		// Returned by the server by numerous commands to
		// indicate to the client that it didn't supply enough
		// parameters.
		return;
	}

	std::string username = params[0];
	std::string hostname = params[1];
	std::string servername = params[2];
	std::string realname = params[3];

	// Ignore hostname and servername when USER comes from a directly connected client.
	// They will be used only in server-to-server communication.

	// Set the user's username and realname
	client->set_username(username);
	client->set_realname(realname);

	client->proceed_registration_status();

};
