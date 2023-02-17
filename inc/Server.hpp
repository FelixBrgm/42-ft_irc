#ifndef SERVER_HPP
#define SERVER_HPP
#include <string>
#include <map>
#include <vector>

#include <poll.h>
#include <arpa/inet.h>

#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"

#define MAX_MESSAGE_LENGHT  512
#define MAX_CLIENTS_POLLFDS 1024


class Server
{
	private:
		int																	_port;
		std::string															_password;
		struct pollfd														_fds[MAX_CLIENTS_POLLFDS];
		unsigned short														_nfds;
		struct sockaddr_in													_listener_socket_addr;
		int																	_listenerfd;
	
		std::map<int, Client>												_fd_to_client;					
		std::map<std::string, Channel>										_name_to_channel;
	
	public:
		Server(int port, std::string password);
		~Server();
	
		void	start();
		void	loop();
	
	private:
		void	_accept_new_connection();
		void	_receive_incoming_data(int fd);

};



#endif