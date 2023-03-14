#ifndef SERVER_HPP
#define SERVER_HPP
#include <string>
#include <map>
#include <vector>

#include <poll.h>
#include <arpa/inet.h>

#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"

#include "Constants.hpp"




class Server
{
	private:
		int																	_port;
		std::string															_password;
		std::string															_server_name;
		std::vector<struct pollfd>											_pollfds;
		unsigned short														_nfds;
		int																	_listenerfd;
	
		std::map<int, Client>												_fd_to_client;					
		std::map<std::string, Channel>										_name_to_channel;
		std::vector<std::string>											_taken_usernames;


	public:
		Server(int port, std::string password, std::string server_name = std::string("ft_irc"));
		~Server();
	

		void	start();
		void	_event_loop();
	
	private:
		void	_accept_new_connection();
		void	_parse_incoming_data(int fd);


		// helpers for commands
		bool	_client_username_already_exists();


		// error messages



		// commands
		void	_cmd_pass(Client* client, std::vector<std::string> params);
		void	_cmd_nick(Client* client, std::vector<std::string> params);
		void	_cmd_user(Client* client, std::vector<std::string> params);

};



#endif