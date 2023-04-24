#include "../../inc/Server.hpp"

void	Server::_cmd_ping(Client* client, std::vector<std::string> params)
{
	if (params.size() < 1)
	{
		client->append_response_buffer("409 * :No origin specified\r\n");
		return;
	}

	std::string answer = params[0];
	std::string response = "PONG " + answer + "\r\n";
	// Send Pong
	client->append_response_buffer(response);
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

void Server::_cmd_who(Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        client->append_response_buffer("461 " + client->get_nickname() + " WHO :Not enough parameters\r\n");
        return;
    }

    std::string channel_name = params[0];

    if (!_name_to_channel.count(channel_name))
    {
        client->append_response_buffer("403 * " + channel_name + " :No such channel\r\n");
        return;
    }
    
    Channel& channel = _name_to_channel[channel_name];

    // Check if the client is in the channel
    if (!channel.contains_client(client))
    {
        client->append_response_buffer("442 " + client->get_nickname() + " " + channel_name + " :You're not on that channel\r\n");
        return;
    }

    // Send the list of users in the channel to the requesting client
	const std::vector<Client*>& clients_in_channel = channel.get_clients();

	// Iterate through the clients and send the message to each of them
	for (std::vector<Client*>::const_iterator it = clients_in_channel.begin(); it != clients_in_channel.end(); ++it)
	{
		Client* user_in_channel = *it;
        std::string user_info = "352 " + client->get_nickname() + " " + channel_name + " " + user_in_channel->get_username() + " ";

        // Use a placeholder for the hostname, or just omit it
        user_info += "* ";

        user_info += _server_name + " " + user_in_channel->get_nickname() + " ";

        // Check if the user is an operator
        if (channel.is_operator(user_in_channel->get_nickname()))
        {
            user_info += "@";
        }
        
        user_info += " :0 " + user_in_channel->get_realname() + "\r\n";
        client->append_response_buffer(user_info);
    }

    // Send the end of the WHO list message
    client->append_response_buffer("315 " + client->get_nickname() + " " + channel_name + " :End of /WHO list.\r\n");
}



void Server::_cmd_cap(Client* client, const std::vector<std::string>& params)
{
    if (params.size() > 0)
    {
        if (params[0] == "LS")
        {
            client->append_response_buffer("CAP * LS :\r\n");
        }
        else if (params[0] == "END")
        {
            // Do nothing, CAP negotiation has ended
        }
        else
        {
            client->append_response_buffer("421 * CAP :Unknown command\r\n");
        }
    }
    else
    {
        client->append_response_buffer("421 * CAP :Unknown command\r\n");
    }
}