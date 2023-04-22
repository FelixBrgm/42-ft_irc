#include "../../inc/Server.hpp"

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

void Server::_cmd_quit(Client* client, const std::vector<std::string>& params)
{
	std::string quit_message = "Client Quit";
	if (params.size() >= 1)
	{
		quit_message = params[0];
	}
	_disconnect_client(client, quit_message);
}
