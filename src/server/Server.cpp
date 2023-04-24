#include "../../inc/Server.hpp"
#include <iostream>
#include <unistd.h>
#include <iomanip>
#include <sstream>


Server::Server(int port, std::string password, std::string server_name) : _port(port), _password(password), _server_name(server_name), _nfds(0), _listenerfd(-1)
{
	time_t now = time(NULL);
	tm* creation_tm = localtime(&now);

	char buffer[20];
	strftime(buffer, sizeof(buffer), "%d-%b-%Y", creation_tm); // or "%b %d %Y" for an alternative format

	_server_creation_time = std::string(buffer);
};

Server::~Server()
{
	for (unsigned short i = 0; i < _nfds; i++)
		close(_pollfds[i].fd);
	close(_listenerfd);
	std::cout << "END OF PROGRAM" << std::endl;
};
