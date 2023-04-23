#include "../../inc/Server.hpp"

#include <iostream>

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

		if (channel.is_full())
		{
			client->append_response_buffer("471 " + channel_name + " :Cannot join channel (+l)\r\n");
			return;
		}

		// Check if the client is banned from the channel
		if (channel.is_banned(client->get_nickname()))
		{
			client->append_response_buffer("474 " + channel_name + " :Cannot join channel (+b)\r\n");
			return;
		}

		// TODO: check invite only
		if (channel.get_is_invite_only() && !channel.is_invited(client->get_nickname()))
		{
			client->append_response_buffer("473 " + channel_name + " :Cannot join channel (+i)\r\n");
			return;
		}
		
		channel.add_client(client);
		client->join_channel(channel_name, &channel);
		std::string join_msg = ":" + client->get_nickname() + "!~" + client->get_username() + " JOIN " + channel_name + "\r\n";
		_send_message_to_channel_members(client, &channel,join_msg, false);
		// Send the list of users in the channel to the client
		std::string names_list = channel.get_names_list();
		std::string topic = channel.get_topic() == "" ? "No topic is set" : channel.get_topic();
		client->append_response_buffer("331 " + client->get_nickname() + " " + channel_name + " :" + topic +"\r\n");
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
	std::vector<std::string> target_names = _split_str(params[0], ',');
	std::string message = params[1];
	for (size_t i = 0; i < target_names.size(); i++)
	{
		std::string target_name = target_names[i];
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

    size_t param_idx = 1;

    while (param_idx < params.size()) {
        std::string mode_string = params[param_idx++];
        bool set_mode = true;

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
						_send_message_to_channel_members(NULL, &channel, ":" + client->get_nickname() + " MODE " + channel_name + " " + (set_mode ? "+i" : "-i") + "\r\n", true);
                        break;
                    case 't':
                        channel.set_topic_restricted(set_mode);
						_send_message_to_channel_members(NULL, &channel, ":" + client->get_nickname() + " MODE " + channel_name + " " + (set_mode ? "+t" : "-t") + "\r\n", true);
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
						_send_message_to_channel_members(NULL, &channel, ":" + client->get_nickname() + " MODE " + channel_name + " " + (set_mode ? "+k" : "-k") + " " + (set_mode ? channel.get_password() : "") + "\r\n", true);
                        break;
                    case 'o':
                		if (param_idx < params.size())
                		{
                		    std::string target_nickname = params[param_idx++];
							Client* target_client = _find_client_by_nickname(target_nickname);
                		    if (target_client)
                		    {
								if (set_mode)
									channel.add_operator(target_nickname);
								else
									channel.remove_operator(target_nickname);
								_send_message_to_channel_members(target_client, &channel, ":" + client->get_nickname() + " MODE " + channel_name + " " + (set_mode ? "+o" : "-o") + " " + target_nickname + "\r\n", true);
                		    }
                		    else
                		    {
                		        client->append_response_buffer("401 " + client->get_nickname() + " " + target_nickname + " :No such nick\r\n");
                		    }
                		}
                		else
                		{
                		    client->append_response_buffer("461 * MODE :Not enough parameters\r\n");
                		}
                		break;
                    case 'l':
                        if (set_mode && param_idx < params.size())
                        {
                            channel.set_user_limit(std::stoi(params[param_idx++]));
							_send_message_to_channel_members(NULL, &channel, ":" + client->get_nickname() + " MODE " + channel_name + " +l " + std::to_string(channel.get_user_limit()) + "\r\n", true);
                        }
                        else if (!set_mode)
                        {
                            channel.set_user_limit(0);
							_send_message_to_channel_members(NULL, &channel, ":" + client->get_nickname() + " MODE " + channel_name + " -l\r\n", true);
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
}}

void Server::_cmd_topic(Client* client, const std::vector<std::string>& params)
{
	// Check if channel is given
	if (params.size() < 1)
	{
		client->append_response_buffer("461 INVITE :Not enough parameters\r\n");
		return;
	}

	std::string target_channel_name = params[0];

	// Check if target_channel exists
	if (_name_to_channel.find(target_channel_name) == _name_to_channel.end())
	{
		client->append_response_buffer("403 " + target_channel_name + " :No such channel\r\n");
		return;
	}

	Channel& target_channel = _name_to_channel[target_channel_name];

	// Check if executing user is on channel
	if (!target_channel.contains_client(client))
	{
		client->append_response_buffer("442 " + target_channel_name + " :You're not on that channel\r\n");
		return;
	}

	// When params is one you want to get the topic
	if (params.size() == 1)
	{
		if (target_channel.get_topic() == std::string(""))
		{
			client->append_response_buffer("331 " + target_channel_name + " :No topic is set\r\n");
		}
		else
		{
			client->append_response_buffer("332 " + target_channel_name + " :" + target_channel.get_topic());
		}
		return;
	}

	// Check if operator permissions are needed && if ther are check if the executing user has them
	if (target_channel.get_is_topic_only_changeable_by_operators() && !target_channel.is_operator(client->get_nickname()))
	{
		client->append_response_buffer("482 " + target_channel_name + " :You're not channel operator\r\n");
		return;
	}

	std::string topic = params[1];
	target_channel.set_topic(topic);

	client->append_response_buffer("332 " + client->get_nickname() + " " + target_channel_name + " :" + target_channel.get_topic() + "\r\n");
}

void Server::_cmd_invite(Client* client, const std::vector<std::string>& params)
{
	// Check for enough parameters
	if (params.size() < 2)
	{
		client->append_response_buffer("461 INVITE :Not enough parameters\r\n");
		return;
	}

	std::string target_nickname = params[0];
	std::string target_channel_name = params[1];
	
	// Check if channel exists
	if (_name_to_channel.find(target_channel_name) == _name_to_channel.end())
	{
		client->append_response_buffer("403 " + target_channel_name + " :No such channel\r\n");
		return;
	}

	// Check if executing user is on channel
	Channel& target_channel = _name_to_channel[target_channel_name];
	std::cout << "channelname|" << target_channel.get_names_list() << std::endl;
	if (!target_channel.contains_client(client))
	{
		client->append_response_buffer("442 " + client->get_nickname() + " :You're not on that channel\r\n");
		return;
	}

	// Check if executing user is operator on channel
	if (!target_channel.is_operator(client->get_nickname()))
	{
		client->append_response_buffer("482 " + client->get_nickname() + " " + target_channel_name + " :You're not channel operator\r\n");
		return;
	}

	// Check if target_nickname exists
	Client* target_client = _find_client_by_nickname(target_nickname);
	if (target_client == nullptr)
	{
		client->append_response_buffer("401 " + target_nickname + " :No such nick/channel\r\n");
		return;
	}

	// Check if target_nickname is already in the channel
	if (target_channel.contains_client(target_client))
	{
		client->append_response_buffer("443 " + target_nickname + " " + target_channel_name + " :is already on channel\r\n");
		return;
	}

	// EXECUTE INVITE
	target_channel.add_invite(target_nickname);

	target_client->append_response_buffer("341 * " + target_nickname + " " + target_channel_name + "\r\n");

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
	_send_message_to_channel_members(target_client, &channel, kick_msg, false);
	target_client->append_response_buffer(kick_msg);

	// Remove the target user from the channel
	channel.remove_client(target_client);
	channel.remove_operator(target_client->get_nickname());
}