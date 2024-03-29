#include "../../inc/Server.hpp"

#include <iostream>
#include <sstream>
#include <errno.h>

void	Server::_parse(int fd)
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
				break;
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
			client.append_response_buffer("417 * :Input line was too long\r\n");
			return;
		}
		
		// msg needs to end with \r\n
		if (!client.is_incoming_msg_complete())
			return;

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
		if (!params.empty() && !params[params.size() - 1].empty() && params[params.size() - 1][params[params.size() - 1].size() - 1] == '\r')
		{
			params[params.size() - 1].resize(params[params.size() - 1].size() - 1);
		}

		std::cout << "PARAMS" << std::endl;
		for (size_t i = 0; i < params.size(); ++i)
		{
			std::cout << "|" << params[i] << "|" << std::endl;
		}
		std::cout << "END" << std::endl;


		if (command == std::string("PASS"))
			_cmd_pass(&client, params);
		else if (command == std::string("NICK"))
			_cmd_nick(&client, params);
		else if (command == std::string("USER"))
			_cmd_user(&client, params);
		else if (command == std::string("PING"))
			_cmd_ping(&client, params);
		else if (command == std::string("CAP"))
			_cmd_cap(&client, params);
		else if (command == std::string("JOIN"))
			_cmd_join(&client, params);
		else if (command == std::string("WHO"))
			_cmd_who(&client, params);
		else if (command == std::string("PRIVMSG"))
			_cmd_privmsg(&client, params);
		else if (command == std::string("QUIT"))
			_cmd_quit(&client, params);
		else if (command == std::string("MODE"))
			_cmd_mode(&client, params);
		else if (command == std::string("KICK"))
			_cmd_kick(&client, params);
		else if (command == std::string("INVITE"))
			_cmd_invite(&client, params);
		else if (command == std::string("TOPIC"))
			_cmd_topic(&client, params);
		else
		{
			client.append_response_buffer(std::string("421") + std::string(" * ") + command + std::string(" :Unknown command\r\n"));
		}
	}
}