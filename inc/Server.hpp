#ifndef SERVER_HPP
#define SERVER_HPP
#include <string>
#include <poll.h>
#include <map>

#include <arpa/inet.h>

#define MAX_CLIENTS_POLLFDS 1024
#define MAX_MESSAGE_LENGHT  512
#include "../inc/Client.hpp"

class Server
{
private:
	int																	_port;
	std::string															_password;
	struct pollfd														_fds[MAX_CLIENTS_POLLFDS];
	unsigned short														_nfds;
	struct sockaddr_in													_listener_socket_addr;
	int																	_listenerfd;
	// key is name (must be unique), value is buffer to be sent
	std::map<int, Client>												_fd_to_unregistered_clients;					
	std::map<int, std::string>											_fd_to_username;
	std::map<std::string, std::vector<std::pair<std::string, bool> > >	_channels_to_usernames;
	std::map<std::string, Client>										_username_to_client;

public:
	Server(int port, std::string password);
	~Server();
	void	start();
	void	loop();

private:
	void	_accept();
	void	_handle(int fd, std::string buffer);

};



#endif