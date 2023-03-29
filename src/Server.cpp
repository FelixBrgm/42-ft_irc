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
#include <ctime>
#include <iomanip>
#include <sstream>

Server::Server(int port, std::string password, std::string server_name) : _port(port), _password(password), _server_name(server_name), _nfds(0), _listenerfd(-1)
{
	std::time_t now = std::time(nullptr);
    std::tm* creation_tm = std::localtime(&now);

    std::stringstream date_stream;
    date_stream << std::put_time(creation_tm, "%d-%b-%Y"); // or "%b %d %Y" for an alternative format
	_server_creation_time = date_stream.str();
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
					_parse_incoming_data(_pollfds[i].fd);
			}
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
			{
				std::cerr << "Failed To Open Socket -> Client Could Not Connect" << std::endl;
				return;
			}
			else
				return ;
		}

		// Set O_NONBLOCK flag to enable nonblocking I/O
		int flags = fcntl(new_socket_fd, F_GETFL, 0);
		if (fcntl(new_socket_fd, F_SETFL, flags | O_NONBLOCK) < 0)
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
	Client&	client = _fd_to_client[fd];

	if (received_bytes == 0)
	{
		_unexpected_client_disconnection(&client);
		return;
	}

	if (received_bytes < 0)
	{
		_unexpected_client_disconnection(&client);
		return;
	}
	
	client.append_in_buffer(buf);


	while (true)
	{
		// (check if the message is bigger then 512) -> send error
		if (client.is_incoming_msg_too_long())
			;

		// msg needs to end with \r\n
		if (!client.is_incoming_msg_complete())
			return;



		std::string message = client.get_in_buffer();
		std::cout << "Received:" <<message.substr(0, message.find("\r\n")) << std::endl;
		client.clear_msg_in_buffer();
		std::string command;
		std::vector<std::string> params;
		std::istringstream iss(message);
		std::string token;

		// Check for prefix and remove it
		if (message[0] == ':')
		{
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

		// Remove trailing \r from the last parameter
		if (!params.empty() && !params.back().empty() && params.back().back() == '\r')
		{
			params.back().pop_back();
		}

		if (command == std::string("PASS"))
			_cmd_pass(&client, params);
		else if (command == std::string("NICK"))
			_cmd_nick(&client, params);
		else if (command == std::string("USER"))
			_cmd_user(&client, params);
		else if (command == std::string("PING"))
			_cmd_ping(&client, params);
		else if (command == std::string("CAP"))
		{
			if (params.size() > 0)
			{
				if (params[0] == "LS")
				{
					client.append_response_buffer("CAP * LS :\r\n");
				}
				else if (params[0] == "END")
				{
					// Do nothing, CAP negotiation has ended
				}
				else
				{
					client.append_response_buffer("421 * CAP :Unknown command\r\n");
				}
			}
			else
			{
				client.append_response_buffer("421 * CAP :Unknown command\r\n");
			}
		}
		else if (command == std::string("JOIN"))
			_cmd_join(&client, params);
		else if (command == std::string("PRIVMSG"))
			_cmd_privmsg(&client, params);
		else
		{
			client.append_response_buffer(std::string("421") + std::string(" * ") + command + std::string(" :Unknown command\r\n"));
		}



		// Client is registered

		// to send back just write to the fd of the client     
	}
	
}


// Cmd's Helpers






void Server::_unexpected_client_disconnection(Client* client)
{
    _send_quit_message_to_channels(client, "Client disconnected unexpectedly");
    _remove_client(client->get_fd());
}


void Server::_remove_client(int client_fd)
{
    // Remove the client from the _fd_to_client map
    _fd_to_client.erase(client_fd);

    // Remove the corresponding pollfd from the _pollfds vector
    for (std::vector<struct pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it)
    {
        if (it->fd == client_fd)
        {
            _pollfds.erase(it);
            break;
        }
    }
	_nfds--;
    // Close the client's file descriptor
    close(client_fd);
}


void Server::_send_quit_message_to_channels(Client* client, const std::string& quit_message)
{
	std::map<std::string, Channel*> joined_channels = client->get_joined_channels();
	std::string quit_msg = ":" + client->get_nickname() + "!~" + client->get_username() + " QUIT :" + quit_message + "\r\n";
	for (std::map<std::string, Channel*>::iterator it = joined_channels.begin(); it != joined_channels.end(); ++it)
	{
		Channel* channel = it->second;
		const std::vector<Client*>& clients_in_channel = channel->get_clients();
		for (std::vector<Client*>::const_iterator cit = clients_in_channel.begin(); cit != clients_in_channel.end(); ++cit)
		{
			Client* client_in_channel = *cit;
			client_in_channel->append_response_buffer(quit_msg);
		}
	}
}


Client* Server::_find_client_by_nickname(const std::string& nickname)
{
    for (std::map<int, Client>::iterator it = _fd_to_client.begin(); it != _fd_to_client.end(); ++it)
    {
        if (it->second.get_nickname() == nickname)
        {
            return &(it->second);
        }
    }
    return nullptr;
}

std::vector<std::string> Server::_split_str(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;

    while (std::getline(iss, token, delimiter))
    {
        tokens.push_back(token);
    }

    return tokens;
}



void Server::_welcome_new_user(Client* client)
{
    std::string nickname = client->get_nickname();
    std::string username = client->get_username();
    std::string realname = client->get_realname();
    std::string server_host = _server_name;

    // Send RPL_WELCOME: 001
    std::string rpl_welcome_msg = "001 " + nickname + " :Welcome to the Internet Relay Network " + nickname + "!" + username + "@" + realname + "\r\n";
    client->append_response_buffer(rpl_welcome_msg);

    // Send RPL_YOURHOST: 002
    std::string rpl_yourhost_msg = "002 " + nickname + " :Your host is " + server_host + ", running version 1.0\r\n";
    client->append_response_buffer(rpl_yourhost_msg);

    // Send RPL_CREATED: 003
    std::string rpl_created_msg = "003 " + nickname + " :This server was created " + _server_creation_time + "\r\n";
    client->append_response_buffer(rpl_created_msg);

    // Send RPL_MYINFO: 004
	std::string rpl_myinfo_msg = "004 " + nickname + " " + server_host + " 1.0 o o\r\n";
    client->append_response_buffer(rpl_myinfo_msg);

    // ... any other messages you want to send

    client->proceed_registration_status();
}

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
        client->append_response_buffer("462 :You may not reregister\r\n");
        return;
    }

    if (params.size() < 1)
    {
        client->append_response_buffer("461 PASS :Not enough parameters\r\n");
        return;
    }

    if (params[0] != _password)
    {
        client->append_response_buffer("464 :Password incorrect\r\n");
        return;
    }

	// valid password
	client->proceed_registration_status();
};

void	Server::_cmd_nick(Client* client, std::vector<std::string> params)
{


	if (client->get_status() == pass)
	{
		client->append_response_buffer("451 :You have not registered\r\n");
		return;
	}

	if (params.size() == 0)
	{

		client->append_response_buffer("431 :No nickname given\r\n");
		return;
	}    
	std::string new_nickname = params[0];    
	if (!_is_valid_nickname(new_nickname))
	{

		client->append_response_buffer("432 " + new_nickname + " :Erroneous nickname\r\n");
		return;
	}

	if (_username_already_exists(new_nickname))
	{
		// Handle nickname collision
		client->append_response_buffer("436 " + new_nickname + " :Nickname collision KILL\r\n");

		// disconnect client
		// std::vector<std::string>::iterator it = std::find(_taken_usernames.begin(), _taken_usernames.end(), new_nickname);
		// if (it != _taken_usernames.end())
		// 	_taken_usernames.erase(it);
		return;
	}	


	// Update the client's nickname
	std::string old_nickname = client->get_nickname();
	client->set_nickname(new_nickname);	

	std::vector<std::string>::iterator it = std::find(_taken_usernames.begin(), _taken_usernames.end(), old_nickname);
	if (it != _taken_usernames.end())
	    _taken_usernames.erase(it);
	
	_taken_usernames.push_back(new_nickname);

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
		client->append_response_buffer("462 :You may not reregister\r\n");
		return;
	}

	if (params.size() < 4)
	{
		client->append_response_buffer("461 USER :Not enough parameters\r\n");
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

	_welcome_new_user(client);

};

void	Server::_cmd_ping(Client* client, std::vector<std::string> params)
{
	if (params.size() < 1)
	{
		client->append_response_buffer("409 :No origin specified\r\n");
		return;
	}

	std::string answer = params[0];
	std::string response = "PONG " + answer + "\r\n";
	// Send Pong
	client->append_response_buffer(response);
}

// CHECK FOR BANS NOT DONE!!!!
void Server::_cmd_join(Client* client, const std::vector<std::string>& params)
{
	if (params.size() < 1)
	{
		client->append_response_buffer("461 * JOIN :Not enough parameters\r\n");
		return;
	}
	std::vector<std::string> channels = _split_str(params[0], ',');
	std::vector<std::string> keys;
	if (params.size() > 1)
	{
		keys = _split_str(params[1], ',');
	}

	for (size_t i = 0; i < channels.size(); ++i)
	{
		const std::string& channel_name = channels[i];
		std::string key = i < keys.size() ? keys[i] : "";

		if (_name_to_channel.find(channel_name) == _name_to_channel.end())
		{
			// Create the channel if it doesn't exist
			_name_to_channel[channel_name] = Channel(channel_name);

			// Grant operator status to the creator
			_name_to_channel[channel_name].add_operator(client->get_nickname());
		}

		Channel& channel = _name_to_channel[channel_name];

		channel.add_client(client);
		client->join_channel(channel_name, &channel);

		std::string join_msg = ":" + client->get_nickname() + "!~" + client->get_username() + " JOIN " + channel_name + "\r\n";
		const std::vector<Client*>& clients_in_channel = channel.get_clients();
		for (std::vector<Client*>::const_iterator it = clients_in_channel.begin(); it != clients_in_channel.end(); ++it)
		{
			Client* user_in_channel = *it;
			user_in_channel->append_response_buffer(join_msg);
		}

		// Send the list of users in the channel to the client
		std::string names_list = channel.get_names_list();
		client->append_response_buffer("353 " + client->get_nickname() + " = " + channel_name + " :" + names_list + "\r\n");
		client->append_response_buffer("366 " + client->get_nickname() + " " + channel_name + " :End of /NAMES list\r\n");
	}
}


void Server::_cmd_privmsg(Client* client, const std::vector<std::string>& params)
{
	if (params.size() < 2)
	{
		client->append_response_buffer("461 * PRIVMSG :Not enough parameters\r\n");
		return;
	}	
	std::string target_name = params[0];
	std::string message = params[1];	
	// Check if the target is a channel or a user
	if (target_name[0] == '#')
	{
		// Target is a channel
		std::map<std::string, Channel>::iterator channel_it = _name_to_channel.find(target_name);
		if (channel_it == _name_to_channel.end())
		{
			client->append_response_buffer("403 " + client->get_nickname() + " " + target_name + " :No such channel\r\n");
			return;
		}	
		Channel& channel = channel_it->second;
		if (!channel.contains_client(client))
		{
			client->append_response_buffer("404 " + client->get_nickname() + " " + target_name + " :Cannot send to channel\r\n");
			return;
		}	
		// Relay the message to all clients in the channel
		const std::vector<Client*>& clients_in_channel = channel.get_clients();
		for (std::vector<Client*>::const_iterator it = clients_in_channel.begin(); it != clients_in_channel.end(); ++it)
		{
			Client* client_in_channel = *it;
			if (client_in_channel != client)
			{
				std::string msg_to_send = ":" + client->get_nickname() + " PRIVMSG " + target_name + " :" + message + "\r\n";
				client_in_channel->append_response_buffer(msg_to_send);
			}
		}
	}
	else
	{
		Client* target_client = _find_client_by_nickname(target_name);
		if (target_client == nullptr)
		{
			client->append_response_buffer("401 " + client->get_nickname() + " " + target_name + " :No such nick\r\n");
			return;
		}
		std::string msg_to_send = ":" + client->get_nickname() + " PRIVMSG " + target_name + " :" + message + "\r\n";
		target_client->append_response_buffer(msg_to_send);
	}
}



void Server::_cmd_quit(Client* client, const std::vector<std::string>& params)
{
	std::string quit_message = "Client Quit";
	if (params.size() >= 1)
	{
		quit_message = params[0];
	}
	// Notify all channels the client is part of
	_send_quit_message_to_channels(client, quit_message);
	// Remove the client from the server
	_remove_client(client->get_fd());
}

void Server::_cmd_op(Client* client, const std::vector<std::string>& params)
{
	if (params.size() < 2)
	{
		client->append_response_buffer("461 * OP :Not enough parameters\r\n");
		return;
	}
	std::string channel_name = params[0];
	std::string target_nickname = params[1];
	// Check if the channel exists
	std::map<std::string, Channel>::iterator channel_it = _name_to_channel.find(channel_name);
	if (channel_it == _name_to_channel.end())
	{
		client->append_response_buffer("403 " + client->get_nickname() + " " + channel_name + " :No such channel\r\n");
		return;
	}
	Channel& channel = channel_it->second;
	// Check if the target user exists
	Client* target_client = _find_client_by_nickname(target_nickname);
	if (target_client == nullptr)
	{
		client->append_response_buffer("401 " + client->get_nickname() + " " + target_nickname + " :No such nick\r\n");
		return;
	}
	// Check if the client is an operator of the channel
	if (!channel.is_operator(client->get_nickname()))
	{
		client->append_response_buffer("482 " + client->get_nickname() + " " + channel_name + " :You're not channel operator\r\n");
		return;
	}
	// Grant operator status to the target user
	channel.add_operator(target_nickname);
	// Notify users in the channel
	std::string op_msg = ":" + client->get_nickname() + " MODE " + channel_name + " +o " + target_nickname + "\r\n";
	const std::vector<Client*>& clients_in_channel = channel.get_clients();
	for (std::vector<Client*>::const_iterator it = clients_in_channel.begin(); it != clients_in_channel.end(); ++it)
	{
		Client* user_in_channel = *it;
		user_in_channel->append_response_buffer(op_msg);
	}
}

// void Server::_cmd_kick(Client* client, const std::vector<std::string>& params)
// {
// 	if (params.size() < 2)
// 	{
// 		client->append_response_buffer("461 * KICK :Not enough parameters\r\n");
// 		return;
// 	}
// 	std::string channel_name = params[0];
// 	std::string target_nickname = params[1];
// 	std::string reason = params.size() > 2 ? params[2] : "No reason specified";
// 	// Check if the client is an operator of the channel
// 	Channel& channel = _name_to_channel[channel_name];
// 	if (!channel.is_operator(client->get_nickname()))
// 	{
// 		client->append_response_buffer("482 " + client->get_nickname() + " " + channel_name + " :You're not channel operator\r\n");
// 		return;
// 	}
// 	// Check if the target user is in the channel
// 	Client* target_client = _find_client_by_nickname(target_nickname);
// 	if (target_client == nullptr || !channel.contains_client(target_client))
// 	{
// 		client->append_response_buffer("441 " + client->get_nickname() + " " + target_nickname + " " + channel_name + " :They aren't on that channel\r\n");
// 		return;
// 	}
// 	// Remove the target user from the channel
// 	channel.remove_client(target_client);
// 	target_client->leave_channel(channel_name);
// 	// Notify clients in the channel
// 	std::string kick_msg = ":" + client->get_nickname() + " KICK " + channel_name + " " + target_nickname + " :" + reason + "\r\n";
// 	for (Client* user_in_channel : channel.get_clients())
// 	{
// 		user_in_channel->append_response_buffer(kick_msg);
// 	}
// 	// Notify the kicked user
// 	target_client->append_response_buffer(kick_msg);
// }