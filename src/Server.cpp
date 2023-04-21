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
		socket_info.events = POLLIN | POLLOUT | POLLHUP;
		_pollfds.push_back(socket_info);

		_fd_to_client[new_socket_fd] = Client(new_socket_fd);

		std::cout << "Accepted Client: " << _nfds << std::endl;

		_nfds++;
	}
}

void	Server::_parse_incoming_data(int fd)
{
    Client& client = _fd_to_client[fd];

	while (true)
	{

		char buf[MAX_MESSAGE_LENGHT] = {0};
		// read incoming msg
		int received_bytes = recv(fd, buf, MAX_MESSAGE_LENGHT, 0);


		if (received_bytes == 0)
		{
			_unexpected_client_disconnection(&client);
			return;
		}


		if (received_bytes < 0)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
			{
				// No more data available, break the loop

			}
			else
			{
				_unexpected_client_disconnection(&client);
				return;
			}
		}

		client.append_in_buffer(buf);
	}
	while (true)
	{
		// (check if the message is bigger then 512) -> send error
		if (client.is_incoming_msg_too_long())
		{
			client.append_response_buffer("417 :Input line was too long\r\n");
			return;
		}
		
		// msg needs to end with \r\n
		if (!client.is_incoming_msg_complete())
			return;

		// std::cout << "FULL: " << message;
		std::string buffer = client.get_in_buffer();

		std::string message = buffer.substr(0, buffer.find("\r\n"));
		std::cout << "Message from " << client.get_nickname() << ": " << message << std::endl;
		client.cut_msg_in_buffer();


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
		else if (command == std::string("QUIT"))
			_cmd_quit(&client, params);
		else if (command == std::string("MODE"))
			_cmd_mode(&client, params);
		else if (command == std::string("KICK"))
			_cmd_kick(&client, params);
		else
		{
			client.append_response_buffer(std::string("421") + std::string(" * ") + command + std::string(" :Unknown command\r\n"));
		}
	}
	
}


void Server::_broadcast_to_all_joined_channels(Client *client, const std::string& message)
{
	std::map<std::string, Channel*> joined_channels = client->get_joined_channels();
	for (std::map<std::string, Channel*>::iterator it = joined_channels.begin(); it != joined_channels.end(); ++it)
	{
		Channel* channel = it->second;
		_send_message_to_channel_members(client, channel, message);
	}
}


// Cmd's Helpers
void Server::_send_message_to_channel_members(Client *client, Channel *channel, const std::string& message)
{
    // Get the clients in the channel
    const std::vector<Client*>& clients_in_channel = channel->get_clients();

    // Iterate through the clients and send the message to each of them
    for (std::vector<Client*>::const_iterator it = clients_in_channel.begin(); it != clients_in_channel.end(); ++it)
    {
        Client* user_in_channel = *it;
		if (client != user_in_channel)
        	user_in_channel->append_response_buffer(message);
    }
}





void Server::_unexpected_client_disconnection(Client* client)
{
	_disconnect_client(client, "Client disconnected unexpectedly");
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
void Server::_cmd_nick(Client* client, std::vector<std::string> params)
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
        // Handle nickname in use
        client->append_response_buffer("433 " + new_nickname + " :Nickname is already in use\r\n");
        return;
    }

    // Update the client's nickname
    std::string old_nickname = client->get_nickname();
    client->set_nickname(new_nickname);

    if (old_nickname.size() != 0)
    {
        std::vector<std::string>::iterator it = std::find(_taken_usernames.begin(), _taken_usernames.end(), old_nickname);
        if (it != _taken_usernames.end())
            _taken_usernames.erase(it);
    }

    _taken_usernames.push_back(new_nickname);

    // If the client was already registered, send a notification to other users in the same channels
    if (client->get_status() == registered)
    {
        // Notify other users in the same channels about the nickname change
        std::string nick_change_msg = ":" + old_nickname + " NICK " + new_nickname + "\r\n";
        client->append_response_buffer(nick_change_msg);

        _broadcast_to_all_joined_channels(client, nick_change_msg);
        return;
    }

    // if status is nick we proceed to user
    if (client->get_status() == nick)
        client->proceed_registration_status();
}

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

void Server::_cmd_join(Client* client, const std::vector<std::string>& params)
{

    if (client->get_status() != registered)
    {
        client->append_response_buffer("451 :You have not registered\r\n");
        return;
    }

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

		if (channel.contains_client(client) == true)
		{
			return;
		}

		// Check if the client is banned from the channel
		if (channel.is_banned(client->get_nickname()))
		{
			client->append_response_buffer("474 " + client->get_nickname() + " " + channel_name + " :Cannot join channel (+b)\r\n");
			_name_to_channel.erase(channel_name);
			return;
		}

		// TODO: check invite only

		
		channel.add_client(client);
		client->join_channel(channel_name, &channel);
		std::string join_msg = ":" + client->get_nickname() + "!~" + client->get_username() + " JOIN " + channel_name + "\r\n";
		_send_message_to_channel_members(client, &channel,join_msg);
		// Send the list of users in the channel to the client
		std::string names_list = channel.get_names_list();
		client->append_response_buffer("331 " + client->get_nickname() + " " + channel_name + " :No topic is set\r\n");
		client->append_response_buffer("353 " + client->get_nickname() + " = " + channel_name + " :" + names_list + "\r\n");
		client->append_response_buffer("366 " + client->get_nickname() + " " + channel_name + " :End of /NAMES list\r\n");
	}
}

void Server::_cmd_privmsg(Client* client, const std::vector<std::string>& params)
{

    if (client->get_status() != registered)
    {
        client->append_response_buffer("451 :You have not registered\r\n");
        return;
    }

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


void Server::_disconnect_client(Client* client, std::string quit_message)
{
	// Send a quit message to all channels the client is in + remove client from all channels it is in
	std::string quit_msg = ":" + client->get_nickname() + "!~" + client->get_username() + " QUIT :" + quit_message + "\r\n";
	_broadcast_to_all_joined_channels(client, quit_msg);

	// Remove the client from the taken username
	int client_fd = client->get_fd();
	if (client->get_nickname().size() != 0)
	{
		std::vector<std::string>::iterator it = std::find(_taken_usernames.begin(), _taken_usernames.end(), _fd_to_client[client_fd].get_nickname());
		if (it != _taken_usernames.end())
			_taken_usernames.erase(it);
	}

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

void Server::_cmd_quit(Client* client, const std::vector<std::string>& params)
{
	std::string quit_message = "Client Quit";
	if (params.size() >= 1)
	{
		quit_message = params[0];
	}
	_disconnect_client(client, quit_message);
}


void Server::_cmd_mode(Client* client, const std::vector<std::string>& params)
{
	if (client->get_status() != registered)
    {
        client->append_response_buffer("451 :You have not registered\r\n");
        return;
    }

    if (params.size() < 1)
    {
        client->append_response_buffer("461 * MODE :Not enough parameters\r\n");
        return;
    }
    std::string target = params[0];
    if (target[0] == '#' || target[0] == '&') // Channel mode
    {
        _cmd_channel_mode(client, params);
    }
    else // User mode
    {
        client->append_response_buffer("502 " + client->get_nickname() + " :Cannot change mode for other users\r\n");
    }
}

void Server::_cmd_channel_mode(Client* client, const std::vector<std::string>& params)
{
	if (client->get_status() != registered)
    {
        client->append_response_buffer("451 :You have not registered\r\n");
        return;
    }

    if (params.size() < 2)
    {
        client->append_response_buffer("461 * MODE :Not enough parameters\r\n");
        return;
    }

    std::string channel_name = params[0];
    std::string mode_string = params[1];

    // Check if the channel exists
    std::map<std::string, Channel>::iterator channel_it = _name_to_channel.find(channel_name);
    if (channel_it == _name_to_channel.end())
    {
        client->append_response_buffer("403 " + client->get_nickname() + " " + channel_name + " :No such channel\r\n");
        return;
    }

    Channel& channel = channel_it->second;

    // Check if the client is an operator of the channel
    if (!channel.is_operator(client->get_nickname()))
    {
        client->append_response_buffer("482 " + client->get_nickname() + " " + channel_name + " :You're not channel operator\r\n");
        return;
    }

       bool set_mode = true;
    size_t param_idx = 2;
    for (size_t i = 0; i < mode_string.length(); ++i)
    {
        char mode_char = mode_string[i];

        if (mode_char == '+' || mode_char == '-')
        {
            set_mode = (mode_char == '+');
        }
        else
        {
            switch (mode_char)
            {
                case 'i':
                    channel.set_is_invite_only(set_mode);
                    break;
                case 't':
                    channel.set_topic_restricted(set_mode);
                    break;
                case 'k':
                    if (param_idx < params.size())
                    {
                        channel.set_password(set_mode ? params[param_idx++] : "");
                    }
                    else
                    {
                        client->append_response_buffer("461 * MODE :Not enough parameters\r\n");
                        return;
                    }
                    break;
                case 'o':
                    if (param_idx < params.size())
                    {
                        channel.set_operator(set_mode, params[param_idx++]);
                    }
                    else
                    {
                        client->append_response_buffer("461 * MODE :Not enough parameters\r\n");
                        return;
                    }
                    break;
                case 'l':
                    if (set_mode && param_idx < params.size())
                    {
                        channel.set_limit(std::stoi(params[param_idx++]));
                    }
                    else if (!set_mode)
                    {
                        channel.remove_limit();
                    }
                    else
                    {
                        client->append_response_buffer("461 * MODE :Not enough parameters\r\n");
                        return;
                    }
                    break;
                default:
                    client->append_response_buffer("472 " + client->get_nickname() + " " + std::string(1, mode_char) + " :is unknown mode char to me\r\n");
                    break;
            }
        }
    }
}







void Server::_cmd_kick(Client* client, const std::vector<std::string>& params)
{
	if (params.size() < 2)
	{
		client->append_response_buffer("461 * KICK :Not enough parameters\r\n");
		return;
	}

	std::string channel_name = params[0];
	std::string target_nickname = params[1];
	std::string reason = params.size() > 2 ? params[2] : "No reason specified";

	// Check if the client is an operator of the channel
	if (!_name_to_channel.count(channel_name))
	{
		client->append_response_buffer("403 " + channel_name + " :No such channel\r\n");
		return;
	}
	Channel& channel = _name_to_channel[channel_name];

	if (!channel.is_operator(client->get_nickname()))
	{
		client->append_response_buffer("482 " + client->get_nickname() + " " + channel_name + " :You're not channel operator\r\n");
		return;
	}

	// Check if the target user is in the channel
	Client* target_client = _find_client_by_nickname(target_nickname);
	if (target_client == nullptr || !channel.contains_client(target_client))
	{
		client->append_response_buffer("441 " + client->get_nickname() + " " + target_nickname + " " + channel_name + " :They aren't on that channel\r\n");
		return;
	}

	// Notify all users joined in the channel user
	std::string kick_msg = ":" + client->get_nickname() + " KICK " + channel_name + " " + target_nickname + " :" + reason + "\r\n";
	_send_message_to_channel_members(target_client, &channel, kick_msg);
	target_client->append_response_buffer(kick_msg);

	// Remove the target user from the channel
	channel.remove_client(target_client);
	channel.remove_operator(target_client->get_nickname());
}