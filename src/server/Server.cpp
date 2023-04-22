#include "../../inc/Server.hpp"
#include <iostream>
#include <unistd.h>
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
	for (unsigned short i = 0; i < _nfds; i++)
		close(_pollfds[i].fd);
	close(_listenerfd);
	std::cout << "END OF PROGRAM" << std::endl;
};
