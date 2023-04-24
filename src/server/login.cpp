#include "../../inc/Server.hpp"

#include <vector>
#include <string>
#include <algorithm>
void	Server::_cmd_pass(Client* client, std::vector<std::string> params)
{
	if (client->get_status() != pass)
	{
		client->append_response_buffer("462 * :You may not reregister\r\n");
		return;
	}

	if (params.size() < 1)
	{
		client->append_response_buffer("461 " + client->get_nickname() + " PASS :Not enough parameters\r\n");
		return;
	}

	if (params[0] != _password)
	{
		client->append_response_buffer("464 * :Password incorrect\r\n");
		return;
	}

	// valid password
	client->proceed_registration_status();
};

void	Server::_cmd_nick(Client* client, std::vector<std::string> params)
{
	if (client->get_status() == pass)
	{
		client->append_response_buffer("451 * :You have not registered\r\n");
		return;
	}

	if (params.size() == 0)
	{
		client->append_response_buffer("431 * :No nickname given\r\n");
		return;
	}

	std::string new_nickname = params[0];
	if (!_is_valid_nickname(new_nickname))
	{
		client->append_response_buffer("432 * " + new_nickname + " :Erroneous nickname\r\n");
		return;
	}

	if (_username_already_exists(new_nickname))
	{
		// Handle nickname in use
		client->append_response_buffer("433 * " + new_nickname + " :Nickname is already in use\r\n");
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
		_broadcast_to_all_clients_on_server(nick_change_msg);
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
		client->append_response_buffer("462 * :You may not reregister\r\n");
		return;
	}

	if (params.size() < 4)
	{
		client->append_response_buffer("461 " + client->get_nickname() + " USER :Not enough parameters\r\n");
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

void	Server::_welcome_new_user(Client* client)
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
