#include "../inc/Channel.hpp"

#include <algorithm>

Channel::Channel() {};
Channel::Channel(std::string name) : _name(name), _user_limit(0), _is_invite_only(false), _is_topic_only_changeable_by_operators(false) {};
Channel::~Channel() {};

// Client
void	Channel::add_client(Client *client)
{
	_channel_clients.push_back(client);
}

void Channel::remove_client(Client *client)
{
	std::vector<Client*>::iterator it = std::find(_channel_clients.begin(), _channel_clients.end(), client);
	if (it != _channel_clients.end())
		_channel_clients.erase(it);
}

bool Channel::contains_client(Client* client) const
{
	for (std::vector<Client*>::const_iterator it = _channel_clients.begin(); it != _channel_clients.end(); ++it)
	{
		if (*it == client)
		{
			return true;
		}
	}
	return false;
}


// Operator
void Channel::add_operator(const std::string& nickname)
{
	_channel_operators.push_back(nickname);
}

void Channel::remove_operator(const std::string& nickname)
{
	std::vector<std::string>::iterator it = std::find(_channel_operators.begin(), _channel_operators.end(), nickname);
	if (it != _channel_operators.end())
		_channel_operators.erase(it);
}

bool Channel::is_operator(const std::string& nickname) const
{
	std::vector<std::string>::const_iterator it = std::find(_channel_operators.begin(), _channel_operators.end(), nickname);
	return it != _channel_operators.end();
}


// Ban
void Channel::add_ban(const std::string& nickname)
{
	_banned_nicks.push_back(nickname);
}

void Channel::remove_ban(const std::string& nickname)
{
	std::vector<std::string>::iterator it = std::find(_banned_nicks.begin(), _banned_nicks.end(), nickname);
	if (it != _banned_nicks.end())
		_banned_nicks.erase(it);
}

bool Channel::is_banned(const std::string& nickname) const
{
	std::vector<std::string>::const_iterator it = std::find(_banned_nicks.begin(), _banned_nicks.end(), nickname);
	return it != _banned_nicks.end();
}


// Invite
void	Channel::add_invite(const std::string& nickname)
{
	_invited_nicks.push_back(nickname);
}

bool	Channel::is_invited(const std::string& nickname) const
{
	std::vector<std::string>::const_iterator it = std::find(_invited_nicks.begin(), _invited_nicks.end(), nickname);
	return it != _invited_nicks.end();
}

// Limit
bool Channel::is_full() const
{
	if (get_user_limit() == 0)
		return false;
	return get_user_limit() <= get_clients_count();
}

// Getters
std::string Channel::get_name() const
{
	return _name;
}

const std::vector<Client*>& Channel::get_clients() const
{
	return _channel_clients;
}

short Channel::get_clients_count() const
{
	return _channel_clients.size();
}

std::string Channel::get_names_list()
{
	std::string names_list;

	for (size_t i = 0; i < _channel_clients.size(); ++i)
	{
		Client* client = _channel_clients[i];
		std::string prefix = "";

		// Check if the client is an operator
		if (std::find(_channel_operators.begin(), _channel_operators.end(), client->get_nickname()) != _channel_operators.end())
		{
			prefix = "@";
		}

		names_list += prefix + client->get_nickname();

		// Add a space separator between names, but not after the last name
		if (i < _channel_clients.size() - 1)
		{
			names_list += " ";
		}
	}
	return names_list;
}


std::string Channel::get_password() const
{
	return _password;
}

unsigned short Channel::get_user_limit() const
{
	return _user_limit;
}


bool Channel::get_is_invite_only() const
{
	return _is_invite_only;
}

std::vector<std::string> Channel::get_invited_nicks() const
{
	return _invited_nicks;
}


bool Channel::get_is_topic_only_changeable_by_operators() const
{
	return _is_topic_only_changeable_by_operators;
}

std::string Channel::get_topic() const
{
	return _topic;
}


// Setters
void Channel::set_password(const std::string &password)
{
	_password = password;
}

void Channel::set_user_limit(unsigned short user_limit)
{
	_user_limit = user_limit;
}

void Channel::set_is_invite_only(bool is_invite_only)
{
	_is_invite_only = is_invite_only;
}

void Channel::set_topic_restricted(bool is_topic_only_changeable_by_operators)
{
	_is_topic_only_changeable_by_operators = is_topic_only_changeable_by_operators;
}

void Channel::set_topic(const std::string &topic)
{
	_topic = topic;
}


std::string Channel::get_mode_string() const
{
    std::string mode_str = "+";

    if (get_is_invite_only())
        mode_str += "i";
    if (get_is_topic_only_changeable_by_operators())
        mode_str += "t";
    if (!get_password().empty())
        mode_str += "k " + get_password() + " ";
    if (get_user_limit())
        mode_str += "l " + std::to_string(get_user_limit());

    return mode_str;
}