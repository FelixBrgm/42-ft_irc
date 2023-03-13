#include "../inc/Channel.hpp"



Channel::Channel() {};
Channel::~Channel() {};


void	Channel::remove_client(Client *client)
{
	std::vector<Client*>::iterator	it = std::find(_channel_clients.begin(), _channel_clients.end(), client);
	_channel_clients.erase(it);
	it = std::find(_channel_operators.begin(), _channel_operators.end(), client);
	_channel_clients.erase(it);
}
