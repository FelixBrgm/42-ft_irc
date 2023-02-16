#include <iostream>
#include "../inc/Server.hpp"

bool	valid_arguments(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage:  ./ircserv <port> <password>" << std::endl;
		return 0;
	}
	std::string	port_num(argv[1]);
    if (port_num.find_first_not_of("0123456789") != std::string::npos)
	{
		std::cerr << "Port Number Has To Be Numbers Only" << std::endl;
		return 0;
	}
	int			port = std::stoi(port_num);
	if (port < 1024 || 65536 < port)
	{
		std::cerr << "Port Number Has To Be In Range [1024, 65546]" << std::endl;
		return 0;
	}
	return 1;
}




int main(int argc, char* argv[])
{
	if (!valid_arguments(argc, argv))
		return 1;	

	Server a(std::stoi(std::string(argv[1])), std::string(argv[2]));
	a.start();

	return 0;
}