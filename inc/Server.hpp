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
		std::string															_server_creation_time;
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

		void _disconnect_client(Client* client, std::string quit_message);



		void _send_message_to_channel_members(Client *client, Channel *channel, const std::string& message);
		void _broadcast_to_all_joined_channels(Client *client, const std::string& message);

		void _unexpected_client_disconnection(Client* client);
		Client* _find_client_by_nickname(const std::string& nickname);
		void _send_quit_message_to_channels(Client* client, const std::string& quit_message);;
		void _remove_client(int client_fd);
		
		bool _username_already_exists(const std::string& nickname);
		bool _is_valid_nickname(const std::string& nickname);
		void _welcome_new_user(Client* client);
		std::vector<std::string> _split_str(const std::string& str, char delimiter);

		void	_cmd_channel_mode(Client* client, const std::vector<std::string>& params);
	
		// error messages



		// commands
		void	_cmd_pass(Client* client, std::vector<std::string> params);
		void	_cmd_nick(Client* client, std::vector<std::string> params);
		void	_cmd_user(Client* client, std::vector<std::string> params);
		void	_cmd_ping(Client* client, std::vector<std::string> params);
		void	_cmd_join(Client* client, const std::vector<std::string>& params);
		void 	_cmd_privmsg(Client* client, const std::vector<std::string>& params);
		void 	_cmd_quit(Client* client, const std::vector<std::string>& params = std::vector<std::string>());
		void	_cmd_op(Client* client, const std::vector<std::string>& params);
		void	_cmd_kick(Client* client, const std::vector<std::string>& params);
		void	_cmd_mode(Client* client, const std::vector<std::string>& params);
		void	_cmd_invite(Client* client, const std::vector<std::string>& params);
		void	_cmd_topic(Client* client, const std::vector<std::string>& params);


	void	_parse(std::string message);

	// Actions on the server
};

#endif