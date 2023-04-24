#include "../../inc/Server.hpp"

#include <sstream>
#include <unistd.h>

void Server::_broadcast_to_all_joined_channels(Client *client, const std::string& message)
{
	std::map<std::string, Channel*> joined_channels = client->get_joined_channels();
	for (std::map<std::string, Channel*>::iterator it = joined_channels.begin(); it != joined_channels.end(); ++it)
	{
		Channel* channel = it->second;
		_send_message_to_channel_members(client, channel, message, false);
	}
}

void Server::_broadcast_to_all_clients_on_server(const std::string& message)
{
	for (std::map<int, Client>::iterator it = _fd_to_client.begin(); it != _fd_to_client.end(); ++it)
	{
		Client* client = &(it->second);
		client->append_response_buffer(message);
	}
}

// Cmd's Helpers
void Server::_send_message_to_channel_members(Client *client, Channel *channel, const std::string& message, bool send_to_client_himself)

{
	// Get the clients in the channel
	const std::vector<Client*>& clients_in_channel = channel->get_clients();

	// Iterate through the clients and send the message to each of them
	for (std::vector<Client*>::const_iterator it = clients_in_channel.begin(); it != clients_in_channel.end(); ++it)
	{
		Client* user_in_channel = *it;
		if (client != user_in_channel || send_to_client_himself)
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
